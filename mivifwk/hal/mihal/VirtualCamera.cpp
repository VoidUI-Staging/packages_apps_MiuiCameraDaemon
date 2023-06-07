#include "VirtualCamera.h"

#include "Camera3Plus.h"
#include "Metadata.h"
#include "MiHal3BufferMgr.h"
#include "MiviSettings.h"
#include "OfflineDebugDataUtils.h"
#include "VirtualSession.h"

namespace mihal {

VirtualCamera::VirtualCamera(int cameraId, const camera_info &info)
    : CameraDevice{std::to_string(cameraId)},
      mRawInfo{info},
      mOverrideInfo{std::make_unique<Metadata>()},
      mDefaultSettings{10, 100},
      mSession{nullptr},
      mFlush{false},
      mFwkHal3BufMgr{std::make_shared<MiHal3BufferMgr>(this)},
      mMaxJpegSize{0}
{
    *mOverrideInfo = mRawInfo.static_camera_characteristics;
    mHal3Device.common.version = mRawInfo.device_version;

    int32_t partialCount = 1;
    mOverrideInfo->update(ANDROID_REQUEST_PARTIAL_RESULT_COUNT, &partialCount, 1);

    camera_metadata_entry entry = mOverrideInfo->find(ANDROID_JPEG_MAX_SIZE);
    if (entry.count) {
        mMaxJpegSize = *entry.data.i32;
    }

    initDefaultSettings();
}

VirtualCamera::~VirtualCamera() {}

int VirtualCamera::open()
{
    MLOGI(LogGroupHAL, "[MiHalOp]%s: open", getLogPrefix().c_str());
    SetVTStatus(true);
    return 0;
}

int VirtualCamera::close()
{
    MLOGI(LogGroupHAL, "[MiHalOp]%s: Begin close", getLogPrefix().c_str());
    mConfig = nullptr;
    mSession = nullptr;
    SetVTStatus(false);

    MLOGI(LogGroupHAL, "[MiHalOp]%s: End close", getLogPrefix().c_str());
    return 0;
}

int VirtualCamera::getCameraInfo(camera_info *info) const
{
    if (!info)
        return -EINVAL;

    *info = mRawInfo;
    info->static_camera_characteristics = mOverrideInfo->peek();

    return 0;
}

void VirtualCamera::initDefaultSettings()
{
    // JUST for test. is it possible to give a empty default setting?
    int32_t requestId = 1;
    mDefaultSettings.update(ANDROID_REQUEST_ID, &requestId, 1);
}

const camera_metadata *VirtualCamera::defaultSettings(int) const
{
    MLOGI(LogGroupHAL, "[MiHalOp]%s: get default setting", getLogPrefix().c_str());
    return mDefaultSettings.peek();
}

int VirtualCamera::checkIsConfigValid(std::shared_ptr<StreamConfig> config)
{
    // NOTE: for VTS test where the test script will send many strange stream config
    int ret = 0;
    config->forEachStream([&](const camera3_stream *stream) {
        if (stream->width == 0 || stream->height == 0 || stream->format == -1) {
            MLOGE(LogGroupHAL, "invalid parameter for stream: %p", stream);
            ret = -EINVAL;
            return -EINTR;
        }

        return 0;
    });

    return ret;
}

int VirtualCamera::configureStreams(std::shared_ptr<StreamConfig> config)
{
    auto checkResult = checkIsConfigValid(config);
    if (checkResult) {
        MLOGE(LogGroupHAL, "invalid stream config");
        return checkResult;
    }

    mFlush = false;

    camera3_stream_configuration *rawConfig = config->toRaw();
    for (int i = 0; i < rawConfig->num_streams; i++) {
        rawConfig->streams[i]->max_buffers = 2;
    }
    mFwkHal3BufMgr->configureStreams(config->toRaw());

    // NOTE: mConfig.reset here is important to hold the registry if new is the same with the old
    mConfig.reset(nullptr);
    StreamConfig *hold = new LocalConfig{*config};
    MASSERT(hold);
    mConfig = std::unique_ptr<StreamConfig, std::function<void(StreamConfig *)>>{
        hold, [](StreamConfig *cfg) {
            cfg->forEachStream([](std::shared_ptr<Stream> stream) {
                stream->setRetired();
                return 0;
            });
            delete cfg;
        }};

    MLOGI(LogGroupHAL, "[MiHalOp]%s[Config]: config:%s", getLogPrefix().c_str(),
          mConfig->toString().c_str());

    processConfig();

    if (mSession) {
        mSession->flush();
        mSession = nullptr;
    }
    mSession = std::make_unique<VirtualSession>(this, mConfig.get());

    MLOGI(LogGroupHAL, "[MiHalOp][Config] End %s", getLogPrefix().c_str());

    for (int i = 0; i < rawConfig->num_streams; i++) {
        camera3_stream_t *stream = rawConfig->streams[i];
        Camera3HalStream *halStream = reinterpret_cast<Camera3HalStream *>(stream->reserved[0]);
        MLOGI(LogGroupHAL, "streams[%d] format: %d, data_space: 0x%x, usage: 0x%x", i,
              rawConfig->streams[i]->format, rawConfig->streams[i]->data_space,
              rawConfig->streams[i]->usage);
        if (halStream) {
            MLOGI(LogGroupHAL, "stream[%d] overrideFormat: 0x%x, producer usage: 0x%x",
                  halStream->overrideFormat, halStream->producerUsage);
        }
    }

    return 0;
}

void VirtualCamera::processConfig()
{
    mConfig->forEachStream([](std::shared_ptr<Stream> stream) {
        Camera3HalStream *halStream = stream->getHalStream();
        if (stream->isHeicThumbnail()) {
            stream->setUsage(0x2003);
            halStream->producerUsage = stream->getUsage();
            halStream->overrideFormat = HALPixelFormatBlob;

            stream->setHalStream(halStream);
        } else if (stream->isHeicYuv()) {
            stream->setUsage(0x8430322);
            halStream->producerUsage = stream->getUsage();
            // NOTE: fix "E QC2Comp : [heicE_34] image conversion failed"
            halStream->overrideFormat = HALPixelFormatNV12Encodeable; // 0x102

            stream->setHalStream(halStream);
        }

        return 0;
    });
}

int VirtualCamera::processCaptureRequest(std::shared_ptr<CaptureRequest> request)
{
    MLOGI(LogGroupHAL, "%s: request:%s", getLogPrefix().c_str(), request->toString().c_str());

    std::shared_ptr<LocalRequest> localRequest =
        std::make_shared<LocalRequest>(request->getCamera(), request->toRaw());

    mFwkHal3BufMgr->processCaptureRequest(localRequest->toRaw());

    StreamBuffer *snapshotBuffer = nullptr;
    for (int i = 0; i < localRequest->getOutBufferNum(); i++) {
        StreamBuffer *snapshotBuffer = localRequest->getStreamBuffer(i).get();
        if (nullptr == snapshotBuffer->getBuffer()->getHandle()) {
            auto buffers = mFwkHal3BufMgr->getBuffer(snapshotBuffer->getStream()->toRaw(), 1);
            if (buffers.empty()) {
                MLOGE(LogGroupHAL, "request buffer from stream: %p fail",
                      snapshotBuffer->getStream()->toRaw());
                MASSERT(false);
            } else {
                snapshotBuffer->setLatedBuffer(buffers[0]);
            }
        }
    }

    std::unique_lock<std::recursive_mutex> lock{mMutex};
    mPendingVirtualRecords.insert(
        {localRequest->getFrameNumber(), std::make_unique<VirtualRequestRecord>(localRequest)});
    lock.unlock();

    mSession->processRequest(localRequest);

    return 0;
}

int VirtualCamera::updatePendingVirtualRecords(const CaptureResult *result)
{
    std::lock_guard<std::recursive_mutex> lock{mMutex};
    auto frame = result->getFrameNumber();
    auto it = mPendingVirtualRecords.find(frame);
    if (it == mPendingVirtualRecords.end()) {
        MLOGW(LogGroupHAL, "%s: not found in pending record", getLogPrefixWithFrame(frame).c_str());
        return -ENOENT;
    }

    auto &record = it->second;

    if (result->getPartial() > record->mPartialResult)
        record->mPartialResult = result->getPartial();

    for (int i = 0; i < result->getBufferNumber(); i++) {
        auto streamBuffer = result->getStreamBuffer(i);
        camera3_stream *raw = streamBuffer->getStream()->toRaw();
        if (!record->mPendingStreams.erase(raw)) {
            MLOGE(LogGroupHAL, "%s: try update record, but stream 0x%p not found",
                  getLogPrefixWithFrame(frame).c_str(), raw);
        }
    }

    if (!record->mPendingStreams.size()) {
        mPendingVirtualRecords.erase(it);
        MLOGI(LogGroupHAL, "%s: job completed", getLogPrefixWithFrame(frame).c_str());
    }

    return 0;
}

int VirtualCamera::flushPendingRequests(const CaptureResult *result)
{
    std::lock_guard<std::recursive_mutex> lock{mMutex};
    auto frame = result->getFrameNumber();
    auto it = mPendingVirtualRecords.find(frame);
    if (it != mPendingVirtualRecords.end()) {
        auto &record = it->second;

        if (result->getPartial() > record->mPartialResult)
            record->mPartialResult = result->getPartial();

        for (int i = 0; i < result->getBufferNumber(); i++) {
            auto streamBuffer = result->getStreamBuffer(i);
            if (!streamBuffer) {
                continue;
            }
            camera3_stream *raw = streamBuffer->getStream()->toRaw();
            if (!record->mPendingStreams.erase(raw)) {
                MLOGW(LogGroupHAL, "%s: stream 0x%p not found",
                      getLogPrefixWithFrame(frame).c_str(), raw);
            }
        }
    }

    mRawCallbacks2Fwk->process_capture_result(mRawCallbacks2Fwk, result->toRaw());

    return 0;
}

void VirtualCamera::processCaptureResult(const CaptureResult *result)
{
    mFwkHal3BufMgr->processCaptureResult(result->toRaw());

    if (mFlush) {
        return;
    }
    uint32_t frame = result->getFrameNumber();
    MLOGI(LogGroupHAL, "[Fulfill]%s[ProcessResult]:%s", getLogPrefixWithFrame(frame).c_str(),
          result->toString().c_str());

    updatePendingVirtualRecords(result);
    mRawCallbacks2Fwk->process_capture_result(mRawCallbacks2Fwk, result->toRaw());
}

void VirtualCamera::notify(const camera3_notify_msg *msg)
{
    NotifyMessage nm{this, msg};
    MLOGI(LogGroupHAL, "[Fulfill]%s[Notify]:%s", getLogPrefixWithFrame(nm.getFrameNumber()).c_str(),
          nm.toString().c_str());

    mRawCallbacks2Fwk->notify(mRawCallbacks2Fwk, msg);
}

int VirtualCamera::flush()
{
    MLOGI(LogGroupHAL, "[MiHalOp]%s[Flush]: Begin flush, pendding %zu", getLogPrefix().c_str(),
          mPendingVirtualRecords.size());
    mFlush = true;

    {
        std::lock_guard<std::recursive_mutex> lock{mMutex};
        for (auto &p : mPendingVirtualRecords) {
            auto &record = p.second;
            record->flush();
        }
        mPendingVirtualRecords.clear();
    }

    if (mSession) {
        mSession->flush();
    }

    MLOGI(LogGroupHAL, "[MiHalOp]%s[Flush]: End flush", getLogPrefix().c_str());
    return 0;
}

void VirtualCamera::signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams)
{
    MLOGI(LogGroupHAL, "[MiHalOp]%s begin signalStreamFlush", getLogPrefix().c_str());

    (void *)&numStream;
    (void *)streams;

    // NOTE: when fwk signal stream flush, we need to wait and return all buffers.
    mFwkHal3BufMgr->signalStreamFlush(numStream, streams);

    MLOGI(LogGroupHAL, "[MiHalOp]%s End signalStreamFlush", getLogPrefix().c_str());
}

VirtualCamera::VirtualRequestRecord::VirtualRequestRecord(std::shared_ptr<CaptureRequest> request)
    : mVirtualRequest{request}, mPartialResult{0}
{
    for (int i = 0; i < request->getInputBufferNum(); i++) {
        auto streamBuffer = request->getInputStreamBuffer(i);
        camera3_stream *raw = streamBuffer->getStream()->toRaw();
        mPendingStreams.insert(raw);
    }

    for (int i = 0; i < request->getOutBufferNum(); i++) {
        auto streamBuffer = request->getStreamBuffer(i);
        camera3_stream *raw = streamBuffer->getStream()->toRaw();
        mPendingStreams.insert(raw);
    }
}

int VirtualCamera::VirtualRequestRecord::flush()
{
    std::vector<std::shared_ptr<StreamBuffer>> buffers;

    for (int i = 0; i < mVirtualRequest->getOutBufferNum(); i++) {
        auto streamBuffer = mVirtualRequest->getStreamBuffer(i);
        if (!streamBuffer) {
            continue;
        }
        camera3_stream *raw = streamBuffer->getStream()->toRaw();
        if (!mPendingStreams.count(raw))
            continue;

        streamBuffer->setErrorBuffer();
        buffers.push_back(streamBuffer);
        NotifyMessage msg{mVirtualRequest->getCamera(), mVirtualRequest->getFrameNumber(), raw};
        MLOGI(LogGroupHAL, "[Virtual][Flush]buffer error message: %s", msg.toString().c_str());
        msg.notify();
    }

    if (mPartialResult < mVirtualRequest->getCamera()->getPartialResultCount()) {
        // Notify Error, call for OnCaptureFailed in CameraCaptureSession.CaptureCallback
        NotifyMessage msg{mVirtualRequest->getCamera(), mVirtualRequest->getFrameNumber(),
                          CAMERA3_MSG_ERROR_RESULT};
        MLOGI(LogGroupHAL, "[Virtual][Flush]meta error message: %s", msg.toString().c_str());
        msg.notify();
    }

    if (buffers.size()) {
        LocalResult lr{mVirtualRequest->getCamera(), mVirtualRequest->getFrameNumber(), 0, buffers};
        MLOGI(LogGroupHAL, "[Virtual][Flush]error result for incomplete: %s",
              lr.toString().c_str());
        mVirtualRequest->getCamera()->flushPendingRequests(&lr);
    }

    return 0;
}

} // namespace mihal
