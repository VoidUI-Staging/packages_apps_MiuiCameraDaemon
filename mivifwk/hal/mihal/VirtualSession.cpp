#include "VirtualSession.h"

namespace mihal {

using namespace mialgo2;

VirtualSession::VirtualSession(VirtualCamera *camera, const StreamConfig *config)
    : mPostProc{nullptr}, mCamera{camera}, mHeicSnapshot{false}, mHeicRatio{1.0f}
{
    initialize(config);
}

VirtualSession::~VirtualSession()
{
    destroy();
    mPostProc = nullptr;
}

int VirtualSession::initialize(const StreamConfig *config)
{
    camera3_stream_configuration *rawConfig = config->toRaw();

    for (int i = 0; i < rawConfig->num_streams; i++) {
        if (rawConfig->streams[i]->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED &&
            rawConfig->streams[i]->data_space == (int32_t)HAL_DATASPACE_HEIF) {
            mHeicSnapshot = true;
            mHeicRatio = static_cast<float>(rawConfig->streams[i]->width) /
                         static_cast<float>(rawConfig->streams[i]->height);
        }
    }

    MiaCreateParams createParams{};
    createParams.cameraMode = 0;
    if (mHeicSnapshot) {
        createParams.operationMode = 0xEF01; // StreamConfigModeMiuiCameraHeic
    } else {
        createParams.operationMode = 0xEF00; // StreamConfigModeMiuiCameraJpeg
    }

    createParams.processMode = MiaPostProcMode::OfflineSnapshot;
    createParams.pSessionParams = nullptr;
    createParams.logicalCameraId = 0;

    MLOGI(LogGroupHAL, "opmode 0x%x, camMode 0x%x logicalCamId %d hasHeicStream %d heicSnapshot %f",
          createParams.operationMode, createParams.cameraMode, createParams.logicalCameraId,
          mHeicSnapshot, mHeicRatio);

    createParams.sessionCb = this;
    createParams.frameCb = this;

    std::vector<std::shared_ptr<Stream>> streams =
        static_cast<const LocalConfig *>(config)->getStreams();
    for (auto stream : streams) {
        MiaImageFormat Miaformat = {
            .format = static_cast<uint32_t>(stream->getFormat()),
            .width = stream->getWidth(),
            .height = stream->getHeight(),
        };

        auto stream_type = stream->getType();
        if (stream_type == CAMERA3_STREAM_OUTPUT) {
            createParams.outputFormat.push_back(Miaformat);
        } else if (stream_type == CAMERA3_STREAM_INPUT) {
            createParams.inputFormat.push_back(Miaformat);
        } else {
            MLOGE(LogGroupHAL, "postproc param not support format type: %d", stream_type);
            return -ENOEXEC;
        }
    }

    mPostProc = MiaPostProc_Create(&createParams);
    if (!mPostProc) {
        MLOGE(LogGroupHAL, "failed to create postproc session for %s", mCamera->getId().c_str());
        return -ENOEXEC;
    }

    return 0;
}

int VirtualSession::destroy()
{
    if (!mPostProc) {
        return 0;
    }

    MiaPostProc_Destroy(mPostProc);

    return 0;
}

int VirtualSession::flush()
{
    while (mFlushFlag.test_and_set()) {
    }

    mPendingRequests.clear();

    if (!mPostProc) {
        return 0;
    }

    MiaPostProc_Flush(mPostProc, true);

    return 0;
}

int VirtualSession::processRequest(std::shared_ptr<CaptureRequest> request)
{
    int ret = 0;
    uint32_t inputNum = request->getInputBufferNum();
    uint32_t outNum = request->getOutBufferNum();
    MASSERT(inputNum && outNum);

    uint32_t frame = request->getFrameNumber();
    updateRequestSettings(request);
    decideMialgoPort(request);

    auto entry = request->findInMeta(ANDROID_SENSOR_TIMESTAMP);
    MASSERT(entry.count);
    auto timeStamp = entry.data.i64[0];

    MLOGD(LogGroupHAL, "frame:%d, inputNum:%d, outNum;%d, timeStamp:%" PRId64, frame, inputNum,
          outNum, timeStamp);

    std::unique_lock<std::mutex> lock(mMutex);
    mPendingRequests.insert({frame, std::make_unique<RequestRecord>(request, timeStamp)});
    lock.unlock();

    for (int i = 0; i < outNum; i++) {
        MiaProcessParams reprocParam{};
        MiaFrame outputFrame{};

        outputFrame.callback = this;
        outputFrame.timeStampUs = timeStamp;
        outputFrame.frameNumber = frame;

        auto outStreamBuffer = request->getStreamBuffer(i);
        if (outStreamBuffer == nullptr) {
            MLOGW(LogGroupHAL, "output stream buffer is null");
            return -1;
        }
        outputFrame.streamId = outStreamBuffer->getMialgoPort();
        convert2MiaImage(outputFrame, outStreamBuffer.get());

        reprocParam.frameNum = frame;
        reprocParam.outputNum = 1;
        reprocParam.outputFrame = &outputFrame;

        if (!mFlushFlag.test_and_set()) {
            ret = MiaPostProc_Process(mPostProc, &reprocParam);
            mFlushFlag.clear();
        }
    }

    for (int i = 0; i < inputNum; i++) {
        MiaProcessParams reprocParam{};
        MiaFrame inputFrame{};

        inputFrame.callback = this;
        inputFrame.streamId = i;
        inputFrame.timeStampUs = timeStamp;
        inputFrame.frameNumber = frame;

        auto inputStreamBuffer = request->getInputStreamBuffer(i);
        if (inputStreamBuffer == nullptr) {
            MLOGW(LogGroupHAL, "input stream buffer is null");
            return -1;
        }

        inputFrame.streamId = inputStreamBuffer->getMialgoPort();
        convert2MiaImage(inputFrame, inputStreamBuffer.get());
        Metadata copiedMeta{*request->getMeta()};
        inputFrame.metaWraper =
            std::make_shared<DynamicMetadataWraper>(copiedMeta.release(), nullptr);

        reprocParam.frameNum = frame;
        reprocParam.inputNum = 1;
        reprocParam.inputFrame = &inputFrame;

        if (!mFlushFlag.test_and_set()) {
            MiaPostProc_Process(mPostProc, &reprocParam);
            mFlushFlag.clear();
        }
    }

    return ret;
}

void VirtualSession::onSessionCallback(MiaResultCbType type, uint32_t frame, int64_t timeStamp,
                                       void *msgData)
{
    if (type == MiaResultCbType::MIA_ABNORMAL_CALLBACK) {
        MLOGW(LogGroupHAL, "Result error frame: %d timeStamp: %" PRId64 "", frame, timeStamp);

        // TODO:
    }
}

void VirtualSession::release(uint32_t frame, int64_t timeStampUs, MiaFrameType type,
                             uint32_t streamIdx, buffer_handle_t handle)
{
    std::lock_guard<std::mutex> locker{mMutex};
    if (!mPendingRequests.count(frame)) {
        MLOGE(LogGroupHAL, "request for frame %d not found", frame);
        return;
    }

    RequestRecord *record = mPendingRequests[frame].get();
    CaptureRequest *request = record->mRequest.get();
    if (type == MiaFrameType::MI_FRAME_TYPE_OUTPUT) {
        MLOGI(LogGroupHAL, "[ReleaseOut] release the output buffer frame:%u", frame);
        std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
        for (int i = 0; i < request->getOutBufferNum(); i++) {
            auto streamBuffer = request->getStreamBuffer(i);
            if (streamBuffer == nullptr) {
                MLOGW(LogGroupHAL, "output stream buffer is null");
                return;
            }

            MLOGI(LogGroupHAL, "streamBuffer: %p, handle: %p", streamBuffer.get(), handle);
            MLOGI(LogGroupHAL, "getBuffer %p", streamBuffer->getBuffer());
            if (streamBuffer->getBuffer()->getHandle() != handle)
                continue;

            outBuffers.push_back(streamBuffer);
            record->mMialgoPendingStreams.erase(streamBuffer->getStream()->toRaw());
        }

        if (!record->mShutterReturn) {
            record->mShutterReturn = true;
            NotifyMessage msg{mCamera, frame, record->mTimestamp};
            msg.notify();
        }

        if (!record->mMetaReturn) {
            record->mMetaReturn = true;
            auto fMeta = std::make_unique<Metadata>(*(request->getMeta()));
            LocalResult fResult{mCamera, frame, 1, outBuffers, std::move(fMeta)};
            fResult.updateMeta(ANDROID_SENSOR_TIMESTAMP, &record->mTimestamp, 1);
            mCamera->processCaptureResult(&fResult);
        } else {
            LocalResult fResult{mCamera, frame, 1, outBuffers};
            mCamera->processCaptureResult(&fResult);
        }
    } else {
        MLOGI(LogGroupHAL, "[ReleaseIn] release the input buffer frame:%u", frame);

        auto streamBuffer = request->getInputStreamBuffer(streamIdx);
        GraBuffer *buffer = streamBuffer->getBuffer();
        MASSERT(buffer->getHandle() == handle);
        record->mMialgoPendingStreams.erase(streamBuffer->getStream()->toRaw());
        LocalResult fResult{mCamera, frame, 1, streamBuffer};
        mCamera->processCaptureResult(&fResult);
    }

    if (record->mMialgoPendingStreams.empty()) {
        mPendingRequests.erase(frame);
        MLOGI(LogGroupHAL, "final process frame:%u", frame);
    }
}

int VirtualSession::getBuffer(int64_t timeStampUs, OutSurfaceType type, buffer_handle_t *buffer)
{
    return 0;
}

void VirtualSession::convert2MiaImage(MiaFrame &image, const StreamBuffer *streamBuffer)
{
    Stream *stream = streamBuffer->getStream();
    image.format.width = stream->getWidth();
    image.format.height = stream->getHeight();
    image.format.format = stream->getFormat();
    image.format.cameraId = atoi(stream->getCamera()->getId().c_str());

    GraBuffer *buffer = streamBuffer->getBuffer();
    image.pBuffer.phHandle = buffer->getHandle();
    image.pBuffer.pAddr[0] = image.pBuffer.pAddr[1] = NULL;
    image.pBuffer.fd[0] = image.pBuffer.fd[1] = -1;

    MLOGI(LogGroupHAL, "width %d height %d format %d cameraId %d portId %d", image.format.width,
          image.format.height, image.format.format, image.format.cameraId, image.streamId);
}

void VirtualSession::updateRequestSettings(std::shared_ptr<CaptureRequest> request)
{
    LocalRequest *lr = reinterpret_cast<LocalRequest *>(request.get());
    int32_t multiFrameNum = 1;
    lr->updateMeta(MI_MULTIFRAME_INPUTNUM, &multiFrameNum, 1);

    int32_t maxJpegSize = mCamera->getVTCameraMaxJpegSize();
    if (maxJpegSize > 0) {
        MLOGI(LogGroupHAL, "update maxJpegSize %d", maxJpegSize);
        lr->updateMeta(MI_MIVI_MAX_JPEGSIZE, &maxJpegSize, 1);
    }

    uint8_t flipEnable = 0;
    lr->updateMeta(MI_FLIP_ENABLE, &flipEnable, 1);

    lr->updateMeta(MI_HEICSNAPSHOT_ENABLED, &mHeicSnapshot, 1);
}

void VirtualSession::decideMialgoPort(std::shared_ptr<CaptureRequest> request)
{
    int mialgoPort = 0;
    for (int i = 0; i < request->getInputBufferNum(); i++) {
        auto streamBuffer = request->getInputStreamBuffer(i);
        if (streamBuffer != nullptr) {
            streamBuffer->setMialgoPort(mialgoPort);
            mialgoPort++;
        }
    }

    for (int i = 0; i < request->getOutBufferNum(); i++) {
        auto streamBuffer = request->getStreamBuffer(i);
        if (streamBuffer != nullptr) {
            int port = 0;
            Stream *stream = streamBuffer->getStream();
            if (stream->isHeicYuv()) {
                port = 0;
            }
            if (stream->isHeicThumbnail()) {
                port = 1;
            }
            streamBuffer->setMialgoPort(port);
        }
    }
}

VirtualSession::RequestRecord::RequestRecord(std::shared_ptr<CaptureRequest> request,
                                             int64_t timestamp)
    : mRequest{request}, mMetaReturn{false}, mShutterReturn{false}, mTimestamp{timestamp}
{
    for (int i = 0; i < mRequest->getInputBufferNum(); i++) {
        auto streamBuffer = mRequest->getInputStreamBuffer(i);
        if (streamBuffer != nullptr) {
            mMialgoPendingStreams.insert(streamBuffer->getStream()->toRaw());
        }
    }

    for (int i = 0; i < mRequest->getOutBufferNum(); i++) {
        auto streamBuffer = mRequest->getStreamBuffer(i);
        mMialgoPendingStreams.insert(streamBuffer->getStream()->toRaw());
    }
}

} // namespace mihal
