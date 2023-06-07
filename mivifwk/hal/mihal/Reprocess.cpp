#include "Reprocess.h"

#include <dlfcn.h>
#include <sys/resource.h>

#include <chrono>

#include "BGService.h"
#include "CameraDevice.h"
#include "Metadata.h"
#include "OfflineDebugDataUtils.h"
#include "SdkUtils.h"

namespace mihal {
using namespace mialgo2;
using namespace vendor::xiaomi::hardware::bgservice::implementation;
using namespace std::chrono_literals;

#define RESULTTHREADNUM   4
#define POSTPROCTHREADNUM 1
#define MANAGERTHREADNUM  2

#define SHIFTBITNUM 32

#define KEYFRAMEID_IN_MULTIFRAMEALGO_FRONTSN 4
#define KEYFRAMEID_IN_MULTIFRAMEALGO_BOKEHSN 3
#define KEYFRAMEID_IN_MULTIFRAMEALGO_SN      3
#define KEYFRAMEID_IN_MULTIFRAMEALGO_ELL     8

void PostProcCallback ::onSessionCallback(MiaResultCbType type, uint32_t frame, int64_t timeStamp,
                                          void *msgData)
{
    if (!mProcessor) {
        MLOGE(LogGroupHAL, "[onSessionCallback] mProcessor is invalid");
        return;
    }

    if (type == MiaResultCbType::MIA_META_CALLBACK) {
        std::vector<std::string> rawResult = *static_cast<std::vector<std::string> *>(msgData);
        if (rawResult.empty()) {
            MLOGW(LogGroupHAL, "rawResult is empty");
            return;
        }

        std::string resultValue = "MiviFwkResult:{";
        for (int i = 0; i < rawResult.size(); i++) {
            resultValue += "";
            resultValue += rawResult[i];
        }
        resultValue += "}";
        MLOGD(LogGroupHAL, "frame: %d timeStamp: %" PRId64 "", frame, timeStamp);

        mProcessor->setExifInfo(timeStamp, resultValue);
    } else if (type == MiaResultCbType::MIA_ANCHOR_CALLBACK) {
        mProcessor->setAnchor(timeStamp);
    } else if (type == MiaResultCbType::MIA_ABNORMAL_CALLBACK) {
        mProcessor->setError(PostProcessor::ReprocessError, timeStamp);
    }

    MLOGD(LogGroupHAL, "[onSessionCallback] frame: %d  finish", frame);
}

void BufferCallback::release(uint32_t frame, int64_t timeStampUs, MiaFrameType type,
                             uint32_t streamIdx, buffer_handle_t handle)
{
    MLOGI(LogGroupHAL, "[BufferCallback] release begin frame:%d type:%d streamIdx:%d", frame, type,
          streamIdx);

    if (!mProcessor) {
        MLOGE(LogGroupHAL, "[BufferCallback] mProcessor is invalid");
        return;
    }

    mProcessor->processMialgoResult(frame, timeStampUs, type, streamIdx, handle);
}

int BufferCallback::getBuffer(int64_t timeStampUs, OutSurfaceType type, buffer_handle_t *buffer)
{
    if (!mProcessor) {
        MLOGE(LogGroupHAL, "[BufferCallback] mProcessor is invalid");
        return -ENOENT;
    }

    if (mProcessor->mFlushed.load()) {
        MLOGD(LogGroupHAL, "[BufferCallback] Flush is enbale");
        return -ENOENT;
    }

    return mProcessor->getBuffer(timeStampUs, type, buffer);
}

PostProcessor::PostProcessor(
    std::string signature, std::shared_ptr<StreamConfig> config, std::chrono::milliseconds waitms,
    std::function<void(const std::string &sessionName)> monitorImageCategory)
    : mWaitMillisecs(waitms), mRaisePriority(false)
{
    mImpl = nullptr;
    mInitDone = false;
    mFlushed.store(false);
    mFlushFlag.clear();
    mSignature = signature;
    mMonitorImageCategory = monitorImageCategory;

    // Asynchronous non-blocking execute for input request
    mThread = std::make_unique<ThreadPool>(POSTPROCTHREADNUM);
    mThread->enqueue([this, signature, config] {
        MLOGI(LogGroupHAL, "[PostProcessor] create signature :%s, config:%s", signature.c_str(),
              config->toString().c_str());

        mPostProcCallback = std::move(std::make_unique<PostProcCallback>(this));
        mBufferCallback = std::move(std::make_unique<BufferCallback>(this));

        int32_t roleId = config->getCamera()->getRoleId();
        uint32_t operationMode = config->getOpMode();
        mialgo2::MiaCreateParams mCreateParams;
        mCreateParams.cameraMode = getCameraModeByRoleId(roleId, config->getMeta());
        mCreateParams.operationMode = operationMode;

        mCreateParams.processMode = MiaPostProcMode::OfflineSnapshot;
        mCreateParams.pSessionParams = const_cast<camera_metadata *>(config->getMeta()->peek());

        if (static_cast<uint32_t>(SessionOperation::Bokeh) == operationMode &&
            static_cast<int32_t>(CameraDevice::CameraRoleId::RoleIdFront != roleId))
            mCreateParams.logicalCameraId = 0;
        else
            mCreateParams.logicalCameraId = atoi(config->getCamera()->getId().c_str());

        MLOGI(
            LogGroupHAL,
            "[PostProcessor] create opmode 0x%x, roleId %d, cameraMode: 0x%x, logicalCameraId: %d",
            operationMode, roleId, mCreateParams.cameraMode, mCreateParams.logicalCameraId);

        mCreateParams.sessionCb = mPostProcCallback.get();
        mCreateParams.frameCb = mBufferCallback.get();

        std::vector<std::shared_ptr<Stream>> streams =
            (static_cast<LocalConfig *>(config.get()))->getStreams();
        for (auto stream : streams) {
            MiaImageFormat Miaformat = {
                .format = static_cast<uint32_t>(stream->getFormat()),
                .width = stream->getWidth(),
                .height = stream->getHeight(),
            };

            auto stream_type = stream->getType();
            if (stream_type == CAMERA3_STREAM_OUTPUT) {
                mCreateParams.outputFormat.push_back(Miaformat);
            } else if (stream_type == CAMERA3_STREAM_INPUT) {
                mCreateParams.inputFormat.push_back(Miaformat);
            } else {
                MLOGE(LogGroupHAL, "postproc param not support format type: %d", stream_type);
                return;
            }
        }
        std::lock_guard<std::mutex> lg(mInitMutex);
        mImpl = MiaPostProc_Create(&mCreateParams);
        if (!mImpl) {
            MLOGE(LogGroupHAL, "failed to create postproc signature %s", mSignature.c_str());
        }
        mInitDone = true;
        mInitCond.notify_one();
    });
}

PostProcessor::~PostProcessor()
{
    MLOGD(LogGroupHAL, "PostProcessor %s destruct E", mSignature.c_str());

    if (!mInitDone) {
        std::unique_lock<std::mutex> lk(mInitMutex);

        MLOGD(LogGroupHAL, "PostProcessor %s wait for mialgo create finish", mSignature.c_str());
        mInitCond.wait(lk, [this]() { return mInitDone; });
    }

    if (mImpl) {
        if (!mFlushed.load())
            flush(true);

        MiaPostProc_Destroy(mImpl);
        mImpl = nullptr;
    }

    if (mMonitorImageCategory) {
        mMonitorImageCategory(mSignature);
    }

    MLOGD(LogGroupHAL, "PostProcessor %s destruct X", mSignature.c_str());
}

PostProcessor::RequestRecord::~RequestRecord()
{
    for (auto &iter : inputBuffers) {
        iter.second->releaseBuffer();
        iter.second->getStream()->releaseVirtualBuffer();
    }

    std::shared_ptr<LocalResult> localResult = nullptr;
    if (cameraRequest) {
        std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
        for (int i = 0; i < cameraRequest->getOutBufferNum(); i++) {
            auto tempbuff = cameraRequest->getStreamBuffer(i);
            if (finishResult != FinishSuccess)
                tempbuff->setErrorBuffer();
            outBuffers.push_back(tempbuff);
        }

        uint32_t requestFrameNum =
            mInvitationInfo->isSDK ? mInvitationInfo->frameNumber : cameraRequest->getFrameNumber();

        if (finishResult == FinishSuccess && !mInvitationInfo->isSDK && !allMeta.empty()) {
            auto resultMeta = std::make_unique<Metadata>();

            MLOGI(LogGroupHAL,
                  "[Exif][Offline DebugData][getReference] reference frame index: %d, Vndframe: %d",
                  referenceFrameIndex, mInvitationInfo->frameNumber);

            if (!(allMeta.count(referenceFrameIndex))) {
                MLOGI(LogGroupHAL,
                      "[Exif][Offline DebugData][getReference] update reference frame index: %d->0,"
                      " MetaNum: %zu, Vndframe: %d",
                      referenceFrameIndex, allMeta.size(), mInvitationInfo->frameNumber);
                referenceFrameIndex = 0;
            }

            resultMeta = std::move(allMeta[referenceFrameIndex]);
            // Note::Snsc need wait postprocess finish

            // NOTE: Offline DebugData start
            for (auto &iter : allMeta) {
                if (iter.first != referenceFrameIndex) {
                    auto entry = iter.second->find(QCOM_3A_DEBUG_DATA);
                    if (entry.count) {
                        DebugData *pDebugData = reinterpret_cast<DebugData *>(entry.data.u8);
                        MLOGI(LogGroupHAL,
                              "[Offline DebugData]reference frame index: %d, pDebugData: %p",
                              referenceFrameIndex, pDebugData);
                        if (pDebugData != NULL && pDebugData->pData != NULL) {
                            SubDebugDataReference(pDebugData);
                        }
                    }
                }
            }
            // NOTE: Offline DebugData end

            auto entry = resultMeta->find(QCOM_3A_DEBUG_DATA);
            if (entry.count) {
                DebugData *data = nullptr;
                data = reinterpret_cast<DebugData *>(entry.data.u8);
                MLOGD(LogGroupHAL, "[Offline DebugData][3A_Debug] DebugData:%p, size:%zu",
                      data->pData, data->size);
            }

            // NOTE: update output size info
            resultMeta->update(*meta);

            if (exifInfo.size()) {
                resultMeta->update(MI_MIVI_ALGO_EXIF, exifInfo.c_str());
            }

            if (mInvitationInfo->imageName.size()) {
                resultMeta->update(MI_SNAPSHOT_IMAGE_NAME, mInvitationInfo->imageName.c_str());
                MLOGI(LogGroupHAL, "[RequestRecord][IMAGE_NAME] fwkFrame:%d  set imageName %s",
                      mInvitationInfo->frameNumber, mInvitationInfo->imageName.c_str());
            }

            if (mInvitationInfo->isSDK) {
                resultMeta->update(ANDROID_SENSOR_TIMESTAMP, &(*timestamps.begin()), 1);
            }

            localResult = std::make_shared<LocalResult>(cameraRequest->getCamera(), requestFrameNum,
                                                        1, outBuffers, std::move(resultMeta));
        } else {
            // First Notify CAMERA3_MSG_ERROR_RESULT, According to the definition of Camera2,
            // onCaptureFailed and onCaptureCompleted cannot be called at the same Request.
            // NOTE: In Mockcamera, Since static meta ANDROID_REQUEST_PARTIAL_RESULT_COUNT is set to
            // 1, partial result mechanism is therefore disabled and in this occasion,
            // partial_result field in every camera3_capture_result must be 1 even though such
            // result doesn't contain any metadata.
            localResult = std::make_shared<LocalResult>(cameraRequest->getCamera(), requestFrameNum,
                                                        1, outBuffers);
        }

        MLOGI(LogGroupHAL, "[RequestRecord][~RequestRecord] fwkFrame:%d return output buffer",
              mInvitationInfo->frameNumber);

        cameraRequest->getCamera()->processCaptureResult(localResult.get());
    } else {
        MLOGW(LogGroupHAL, "[RequestRecord][release] fwkFrame:%d Request can not find",
              mInvitationInfo->frameNumber);
    }

    if (fFeedback != nullptr) {
        MLOGI(LogGroupHAL, "[PostProcessor][release] fwkFrame:%d Notify App",
              mInvitationInfo->frameNumber);
        fFeedback(localResult.get(), isEarlyPicDone);
        fFeedback = nullptr;
    }

    uint64_t fakeFwkFrameNum = mInvitationInfo->frameNumber;
    fakeFwkFrameNum = fakeFwkFrameNum << SHIFTBITNUM | mInvitationInfo->sessionId;

    fRemove(fakeFwkFrameNum);
}

void PostProcessor::setInvitationInfo(std::shared_ptr<ResultData> Info,
                                      std::shared_ptr<Metadata> meta,
                                      std::function<void(uint64_t)> fRemove)
{
    std::lock_guard<std::mutex> lk(mRequestMutex);
    if (mFlushed.load())
        return;
    auto record = std::make_shared<RequestRecord>();
    record->mInvitationInfo = Info;
    record->fRemove = fRemove;
    record->weakThis = shared_from_this();
    record->meta = std::make_unique<Metadata>(meta->release(), Metadata::TakeOver);
    uint64_t fakeFwkFrameNum = Info->frameNumber;
    fakeFwkFrameNum = fakeFwkFrameNum << SHIFTBITNUM | Info->sessionId;
    mOutputRequests[fakeFwkFrameNum] = record;
    MLOGI(LogGroupHAL, "[PostProcessor] setInvitationInfo :%s, fwkFrame:%d, fakeFwkFrameNum 0x%lx",
          Info->imageName.c_str(), Info->frameNumber, fakeFwkFrameNum);
}

void PostProcessor::preProcessCaptureRequest(std::shared_ptr<PreProcessConfig> config)
{
    if (mFlushed.load()) {
        return;
    }
    if (!mRaisePriority) {
        mThread->setThreadPoolPriority(-3); // -3 = 117 - 120, 117 is priority
        mRaisePriority = true;
        MLOGD(LogGroupHAL, "[PostProcessor]: %s raise priority to 117", mSignature.c_str());
    }

    mThread->enqueue([this, config] {
        if (mFlushed.load()) {
            return;
        }
        MiaPreProcParams preProcParams{};
        std::vector<camera3_stream> streams = config->getStreams();
        for (int i = 0; i < streams.size(); i++) {
            camera3_stream stream = streams[i];
            MiaImageFormat miaFormat = {
                .width = stream.width,
                .height = stream.height,
                .format = static_cast<uint32_t>(stream.format),
            };

            if (stream.physical_camera_id && strlen(stream.physical_camera_id) > 0) {
                miaFormat.cameraId = atoi(stream.physical_camera_id);
            } else {
                miaFormat.cameraId = atoi(config->getCamera()->getId().c_str());
            }

            if (stream.stream_type == CAMERA3_STREAM_OUTPUT) {
                preProcParams.outputFormats.insert({i, miaFormat});
            } else if (stream.stream_type == CAMERA3_STREAM_INPUT) {
                preProcParams.inputFormats.insert({i, miaFormat});
            }

            MLOGD(LogGroupHAL,
                  "[PostProcessor] stream[%d] type: %d, size: %dx%d, format %d cameraId: %d", i,
                  stream.stream_type, stream.width, stream.height, stream.format,
                  miaFormat.cameraId);
        }

        Metadata meta = *(config->getMeta());
        camera_metadata_t *metadata = meta.release();
        std::shared_ptr<PostProcPreMetaProxy> sharedPreMeta =
            std::make_shared<PostProcPreMetaProxy>(metadata);
        preProcParams.metadata = sharedPreMeta;

        if (!mFlushFlag.test_and_set()) {
            MiaPostProc_PreProcess(mImpl, &preProcParams);
            mFlushFlag.clear();
        }
    });
}

void PostProcessor::processInputRequest(std::shared_ptr<CaptureRequest> request, bool first,
                                        std::function<void(int64_t)> fAnchor,
                                        std::function<void(const CaptureResult *, bool)> fFeedback,
                                        uint64_t fwkFrameNum, bool isEarlyPicDone)
{
    if (mFlushed.load()) {
        MLOGW(LogGroupHAL,
              "[PostProcessor][processInputRequest]:Can not process InputRequest in flush  "
              "fwkFrame:%d",
              fwkFrameNum >> SHIFTBITNUM);

        processErrorRequest(request, fwkFrameNum, first ? fFeedback : nullptr);
        return;
    }

    mThread->enqueue([this, request, first, fAnchor, fFeedback, fwkFrameNum, isEarlyPicDone] {
        if (request->getInputBufferNum() == 0 && request->getOutBufferNum() == 0) {
            MLOGE(LogGroupHAL,
                  "[PostProcessor][processInputRequest]:InputRequest vndFrame:%d Buffer is empty",
                  request->getFrameNumber());
            return;
        }

        std::unique_lock<std::mutex> lk(mRequestMutex);
        if (mFlushed.load()) {
            processErrorRequest(request, fwkFrameNum, first ? fFeedback : nullptr);
            return;
        }

        std::shared_ptr<RequestRecord> tempRecord = nullptr;
        auto iter = mOutputRequests.find(fwkFrameNum);
        if (iter != mOutputRequests.end()) {
            tempRecord = iter->second;
            lk.unlock();
            std::lock_guard<std::mutex> lkRecord(tempRecord->mMutex);
            updateReferenceFrameIndex(request, tempRecord);

            auto entryTime = request->findInMeta(ANDROID_SENSOR_TIMESTAMP);
            if (entryTime.count) {
                tempRecord->timestamps.insert(entryTime.data.i64[0]);
            }
            tempRecord->isEarlyPicDone = isEarlyPicDone;
            tempRecord->inputFrame.insert(request->getFrameNumber());
            if (!tempRecord->fAnchor) {
                tempRecord->fAnchor = fAnchor;
            }
            if (first) {
                tempRecord->fFeedback = fFeedback;
                if (request->getOutBufferNum()) {
                    tempRecord->cameraRequest = request; // SDK request
                }
            }

            for (int i = 0; i < request->getOutBufferNum(); i++) {
                MiaProcessParams reprocParam{};
                MiaFrame outputFrame{};

                outputFrame.callback = mBufferCallback.get();
                outputFrame.frameNumber = request->getFrameNumber();
                outputFrame.streamId = i;

                auto outStreamBuffer = request->getStreamBuffer(i);
                tempRecord->outputBuffers[i] = outStreamBuffer;
                convert2MiaImage(outputFrame, outStreamBuffer);

                auto camera = request->getCamera();
                outputFrame.format.cameraId = atoi(camera->getId().c_str());

                reprocParam.outputNum = 1;
                reprocParam.outputFrame = &outputFrame;

                MLOGI(LogGroupHAL,
                      "[PostProcessor][processInputRequest]: send output buffer to mialgo:%s "
                      "fwkFrame:%d streamId %d",
                      mSignature.c_str(), fwkFrameNum >> SHIFTBITNUM, i);

                if (!mFlushFlag.test_and_set()) {
                    MiaPostProc_Process(mImpl, &reprocParam);
                    mFlushFlag.clear();
                }
            }

            for (int i = 0; i < request->getInputBufferNum(); i++) {
                MiaProcessParams reprocParam{};
                MiaFrame inputFrame{};

                reprocParam.frameNum = request->getFrameNumber();
                buildInputFrame(&(tempRecord->inputBuffers), request, inputFrame, i);
                reprocParam.inputNum = 1;
                reprocParam.inputFrame = &inputFrame;

                MLOGI(LogGroupHAL,
                      "[PostProcessor][processInputRequest]: send input buffer to mialgo:%s "
                      "fwkFrame:%d ,vndFrame:%d",
                      mSignature.c_str(), fwkFrameNum >> SHIFTBITNUM, request->getFrameNumber());

                if (!mFlushFlag.test_and_set()) {
                    MiaPostProc_Process(mImpl, &reprocParam);
                    mFlushFlag.clear();
                }
            }

            camera_metadata *tempmeta = (const_cast<Metadata *>(request->getMeta()))->release();
            if (!tempmeta)
                MLOGD(LogGroupHAL, "null pointer dereference");
            tempRecord->allMeta.insert(
                {tempRecord->index, std::make_unique<Metadata>(tempmeta, Metadata::TakeOver)});
            tempRecord->index += 1;
        } else {
            MLOGW(LogGroupHAL,
                  "[PostProcessor][processInputRequest]:Can not find mission for InputRequest "
                  "fwkFrame:%d",
                  fwkFrameNum >> SHIFTBITNUM);
            processErrorRequest(request, fwkFrameNum, first ? fFeedback : nullptr);
        }
    });
}

void PostProcessor::inviteForMockRequest(uint64_t fwkFrameNum, const std::string &imageName,
                                         bool invite)
{
    if (mFlushed.load()) {
        MLOGW(LogGroupHAL,
              "[PostProcessor][inviteForMockRequest]:Can not process inviteForMockRequest in "
              "flush fwkFrame:%d",
              fwkFrameNum >> SHIFTBITNUM);
        return;
    }

    std::unique_lock<std::mutex> lk(mRequestMutex);
    if (mFlushed.load()) {
        return;
    }

    std::shared_ptr<RequestRecord> record = nullptr;
    auto iter = mOutputRequests.find(fwkFrameNum);
    if (iter != mOutputRequests.end()) {
        record = iter->second;
        lk.unlock();
        std::lock_guard<std::mutex> lkRecord(record->mMutex);
        record->mInvitationInfo->imageName = imageName;

        if (!record->invited && invite) {
            MLOGI(LogGroupHAL,
                  "[PostProcessor][inviteForMockRequest]: BGServiceWrap resultCallback to "
                  "mialgo:%s, fwkFrame:%d",
                  mSignature.c_str(), fwkFrameNum >> SHIFTBITNUM);

            record->invited = true;
            auto time = std::chrono::steady_clock::now().time_since_epoch();
            std::chrono::nanoseconds ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(time);
            record->mInvitationInfo->timeStampUs = ns.count();
            BGServiceWrap::getInstance()->resultCallback(*(record->mInvitationInfo));
        }
    }
}

void PostProcessor::processErrorRequest(const std::shared_ptr<CaptureRequest> &request,
                                        uint64_t fwkFrameNum,
                                        std::function<void(const CaptureResult *, bool)> fFeedback)
{
    std::shared_ptr<LocalResult> localResult = nullptr;
    if (request->getOutBufferNum() > 0) {
        std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
        for (int i = 0; i < request->getOutBufferNum(); i++) {
            auto buff = request->getStreamBuffer(i);
            buff->setErrorBuffer();
            outBuffers.push_back(buff);
        }
        localResult = std::make_shared<LocalResult>(request->getCamera(), request->getFrameNumber(),
                                                    0, outBuffers);
        request->getCamera()->processCaptureResult(localResult.get());
    }

    for (int i = 0; i < request->getInputBufferNum(); i++) {
        auto iter = request->getInputStreamBuffer(i);
        iter->releaseBuffer();
        iter->getStream()->releaseVirtualBuffer();
    }

    if (fFeedback) {
        fFeedback(localResult.get(), true);
    }
}

void PostProcessor::processOutputRequest(std::shared_ptr<CaptureRequest> request,
                                         uint64_t fwkFrameNum)
{
    std::lock_guard<std::mutex> lk(mRequestMutex);
    if (mFlushed.load()) {
        processErrorRequest(request, fwkFrameNum);
        return;
    }

    std::shared_ptr<RequestRecord> record = nullptr;
    auto iter = mOutputRequests.find(fwkFrameNum);
    if (iter != mOutputRequests.end()) {
        record = iter->second;
        std::lock_guard<std::mutex> lkRecord(record->mMutex);
        for (int i = 0; i < request->getOutBufferNum(); i++) {
            auto outStreamBuffer = request->getStreamBuffer(i);
            int32_t streamId =
                reinterpret_cast<uintptr_t>(outStreamBuffer->getStream()->getCookie());
            record->outputBuffers[streamId] = outStreamBuffer;
        }
        record->cameraRequest = request;
        record->setInfo = true;
        record->mCond.notify_all();
        MLOGI(LogGroupHAL, "[PostProcessor][processOutputRequest]:Set OutputRequest fwkFrame:%d",
              fwkFrameNum >> SHIFTBITNUM);
    } else {
        MLOGE(LogGroupHAL,
              "[PostProcessor][processOutputRequest]:Can not find mission for OutputRequest "
              "fwkFrame:%d ",
              fwkFrameNum >> SHIFTBITNUM);
        processErrorRequest(request, fwkFrameNum);
    }
}

int PostProcessor::quickFinishTask(uint64_t fwkFrameNum, const std::string fileName,
                                   bool needResult)
{
    if (mFlushed.load()) {
        return 0;
    }

    mThread->enqueue([this, fwkFrameNum, fileName, needResult]() {
        if (mFlushed.load()) {
            return;
        }

        QuickFinishMessageInfo messageInfo{fileName, needResult /*isNeedImageResult*/};
        auto ret = MiaPostProc_QuickFinish(mImpl, messageInfo); // CDKResult

        MLOGD(LogGroupHAL,
              "[PostProcessor][QuickFinishTask] MiaPostProc_QuickFinish ret %d, fwkFrame:%d", ret,
              fwkFrameNum >> SHIFTBITNUM);
    });
    return 0;
}

void PostProcessor::flush(bool cancel)
{
    MLOGD(LogGroupHAL, "PostProcessor %s flush Begin", mSignature.c_str());
    while (mFlushFlag.test_and_set()) {
    }

    std::unique_lock<std::mutex> lk(mRequestMutex);
    std::set<std::shared_ptr<RequestRecord>> rm;
    mFlushed.store(true);
    for (auto &iter : mOutputRequests) {
        std::lock_guard<std::mutex> lkRecord(iter.second->mMutex);
        iter.second->flush = cancel;
        iter.second->mCond.notify_one();
        rm.insert(iter.second);
    }

    lk.unlock();
    MiaPostProc_Flush(mImpl, true);

    lk.lock();
    mOutputRequests.clear();
    lk.unlock();
}

bool PostProcessor::isBusy()
{
    std::lock_guard<std::mutex> lk(mRequestMutex);
    return !mOutputRequests.empty();
}

void PostProcessor::setExifInfo(int64_t timestamp, std::string exifinfo)
{
    std::lock_guard<std::mutex> lk(mRequestMutex);
    for (auto &iter : mOutputRequests) {
        if (iter.second->timestamps.find(timestamp) != iter.second->timestamps.end()) {
            std::lock_guard<std::mutex> lkRecord(iter.second->mMutex);
            iter.second->exifInfo = exifinfo;
            break;
        }
    }
}

void PostProcessor::setAnchor(int64_t timestamp)
{
    bool tempFind = false;
    std::lock_guard<std::mutex> lk(mRequestMutex);
    for (auto &iter : mOutputRequests) {
        auto it = iter.second->timestamps.find(timestamp);
        if (it != iter.second->timestamps.end()) {
            std::lock_guard<std::mutex> lkRecord(iter.second->mMutex);
            if (iter.second->fAnchor) {
                iter.second->fAnchor(timestamp);
            } else {
                MLOGW(LogGroupHAL, "[PostProcessor][setAnchor] anchor callback is nullptr");
            }
            iter.second->referenceFrameIndex = std::distance(iter.second->timestamps.begin(), it);
            auto index = iter.second->referenceFrameIndex;
            // Support Algoup Anchor, such as snsc
            MLOGD(LogGroupHAL,
                  "[Exif][Offline DebugData][getReference] anchor reference frame index: %d, "
                  "vndFrame:%d",
                  index, *(iter.second->inputFrame.begin()) + index);
            tempFind = true;
            break;
        }
    }

    if (!tempFind)
        MLOGE(LogGroupHAL, "[PostProcessor][setAnchor] Request can not find");
}

void PostProcessor::setError(ErrorState errorState, int64_t timestamp)
{
    bool tempFind = false;
    std::lock_guard<std::mutex> lk(mRequestMutex);
    for (auto &iter : mOutputRequests) {
        if (iter.second->timestamps.find(timestamp) != iter.second->timestamps.end()) {
            std::lock_guard<std::mutex> lkRecord(iter.second->mMutex);
            iter.second->errorState = errorState;
            tempFind = true;
            break;
        }
    }

    if (!tempFind)
        MLOGW(LogGroupHAL, "[PostProcessor][setError] Request can not find");
}

void PostProcessor::processMialgoResult(uint32_t frame, int64_t timeStampUs, MiaFrameType type,
                                        uint32_t streamIdx, buffer_handle_t handle)
{
    std::shared_ptr<RequestRecord> record = nullptr;
    bool tempFind = false;
    uint64_t fwkFrame = 0;
    {
        std::lock_guard<std::mutex> lk(mRequestMutex);
        for (auto &iter : mOutputRequests) {
            if (iter.second->timestamps.find(timeStampUs) != iter.second->timestamps.end()) {
                record = iter.second;
                fwkFrame = iter.first;
                tempFind = true;
                break;
            }
        }
    }

    if (!tempFind) {
        MLOGW(LogGroupHAL, "[PostProcessor][processMialgoResult] Request can not find");
        return;
    }

    Singleton<ThreadPool>::getInstance()->enqueue([record{std::move(record)}, frame, type,
                                                   streamIdx, handle, fwkFrame] {
        std::unique_lock<std::mutex> lkRecord(record->mMutex);

        if (type == MiaFrameType::MI_FRAME_TYPE_OUTPUT) {
            MLOGI(LogGroupHAL,
                  "[PostProcessor][processMialgoResult] fwkFrame:%d vndframe:%d "
                  "processMialgoResult Output",
                  fwkFrame >> SHIFTBITNUM, frame);

            if (record->cameraRequest == nullptr) {
                MLOGE(LogGroupHAL, "[PostProcessor][processMialgoResult] Request can not find");
                return;
            }

            if (!frame || record->errorState) {
                record->finishResult = FinishError;
                record->outputBuffers.clear();
            } else {
                record->outputBuffers.erase(streamIdx);
                if (record->outputBuffers.empty())
                    record->finishResult = FinishSuccess;
            }

            MLOGI(LogGroupHAL,
                  "[PostProcessor][processMialgoResult] fwkFrame:%d vndframe:%d finishResult %d",
                  fwkFrame >> SHIFTBITNUM, frame, record->finishResult);
            if (!record->finishResult) {
                return;
            }

            lkRecord.unlock();

            std::shared_ptr<PostProcessor> strongThis = record->weakThis.lock();
            if (strongThis) {
                std::lock_guard<std::mutex> lk(strongThis->mRequestMutex);
                MLOGI(LogGroupHAL, "[PostProcessor][processMialgoResult] fwkFrame:%d erase Output",
                      fwkFrame >> SHIFTBITNUM);
                strongThis->mOutputRequests.erase(fwkFrame);
            }
        } else {
            MLOGI(LogGroupHAL,
                  "[PostProcessor][processMialgoResult] fwkFrame:%d vndframe:%d "
                  "processMialgoResult Input",
                  fwkFrame >> SHIFTBITNUM, frame);

            auto iter = record->inputBuffers.find(handle);
            if (iter != record->inputBuffers.end()) {
                iter->second->releaseBuffer();
                iter->second->getStream()->releaseVirtualBuffer();
                record->inputBuffers.erase(handle);
            } else {
                MLOGE(LogGroupHAL,
                      "[PostProcessor][processMialgoResult] input buffer handle is invalid");
                return;
            }
            lkRecord.unlock();

            if (record->inputBuffers.empty()) {
                if (!frame || record->errorState) {
                    std::shared_ptr<PostProcessor> strongThis = record->weakThis.lock();
                    if (strongThis) {
                        std::lock_guard<std::mutex> lk(strongThis->mRequestMutex);
                        MLOGI(LogGroupHAL,
                              "[PostProcessor][processMialgoResult] fwkFrame:%d vndframe:%d erase "
                              "Input",
                              fwkFrame >> SHIFTBITNUM, frame);
                        strongThis->mOutputRequests.erase(fwkFrame);
                    }
                }
            }
        }
    });
}

void PostProcessor::notifyBGServiceHasDied(int32_t clientId)
{
    std::shared_ptr<RequestRecord> record = nullptr;
    std::lock_guard<std::mutex> lk(mRequestMutex);
    for (auto &iter : mOutputRequests) {
        record = iter.second;
        std::lock_guard<std::mutex> lkRecord(record->mMutex);
        if (record->mInvitationInfo->clientId == clientId && !record->setInfo) {
            MLOGI(LogGroupHAL, "[PostProcessor][notifyBGServiceHasDied] Service died. clientId: %d",
                  clientId);
            if (record->mInvitationInfo->imageName.empty()) {
                continue;
            }
            QuickFinishMessageInfo messageInfo{record->mInvitationInfo->imageName,
                                               false /*isNeedImageResult*/};
            auto ret = MiaPostProc_QuickFinish(mImpl, messageInfo); // CDKResult
            record->setInfo = true;
            record->mCond.notify_all();
        }
    }
}

int PostProcessor::getBuffer(int64_t timeStampUs, OutSurfaceType type, buffer_handle_t *buffer)
{
    *buffer = nullptr;
    std::shared_ptr<RequestRecord> record = nullptr;
    bool tempFind = false;
    uint64_t fwkFrame = 0;
    std::unique_lock<std::mutex> lk(mRequestMutex);
    for (auto &iter : mOutputRequests) {
        if (iter.second->timestamps.find(timeStampUs) != iter.second->timestamps.end()) {
            record = iter.second;
            fwkFrame = iter.first;
            tempFind = true;
            break;
        }
    }

    if (!tempFind) {
        MLOGE(LogGroupHAL, "[PostProcessor][getBuffer] Request can not find");
        return -ENOENT;
    }

    lk.unlock();
    std::unique_lock<std::mutex> lkRecord(record->mMutex);
    auto begin = std::chrono::system_clock::now();
    MLOGI(LogGroupHAL, "[PostProcessor][getBuffer] fwkFrame:%d Request buffer",
          fwkFrame >> SHIFTBITNUM);

    if (!record->cameraRequest) {
        MLOGI(LogGroupHAL, "[PostProcessor][getBuffer] Request fwkFrame:%d for mock camera",
              fwkFrame >> SHIFTBITNUM);

        if (!record->invited) {
            record->invited = true;
            auto time = std::chrono::steady_clock::now().time_since_epoch();
            std::chrono::nanoseconds ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(time);
            record->mInvitationInfo->timeStampUs = ns.count();
            bool ret = BGServiceWrap::getInstance()->resultCallback(*(record->mInvitationInfo));
            if (!ret) {
                buffer = nullptr;
                return -ENOMEM;
            }

            for (auto &iterImg : record->mInvitationInfo->imageForamts) {
                MLOGI(LogGroupRT,
                      "[PostProcessor][getBuffer][MockInvitations][GreenPic]: invited mock buffers "
                      "size:%uX%u, format: 0x%x",
                      iterImg.width, iterImg.height, iterImg.format);
            }
        }

        if (!BGServiceWrap::getInstance()->hasEeventCallback(record->mInvitationInfo->clientId)) {
            MLOGW(LogGroupHAL, "App %d not setEventCallback or died ",
                  record->mInvitationInfo->clientId);
            buffer = nullptr;
            return -ENOMEM;
        }

        auto ret = record->mCond.wait_for(lkRecord, mWaitMillisecs, [&record, this]() {
            return (record->setInfo || mFlushed.load());
        });

        if (!ret || mFlushed.load()) {
            MLOGE(LogGroupHAL,
                  "[PostProcessor][getBuffer] timedout to wait or flushing the request fwkFrame:%d",
                  fwkFrame >> SHIFTBITNUM);
            *buffer = nullptr;
            return -ENOMEM;
        }
    }

    auto iter = record->outputBuffers.find(type);
    if (iter == record->outputBuffers.end()) {
        *buffer = nullptr;
        MLOGE(LogGroupHAL,
              "[PostProcessor][getBuffer]the  request fwkFrame:%d, streamId:%d can not get "
              "buffer.App buffer is not enough",
              fwkFrame >> SHIFTBITNUM, type);
    } else
        *buffer = iter->second->getBuffer()->getHandle();

    lkRecord.unlock();
    if (*buffer == nullptr) {
        lk.lock();
        mOutputRequests.erase(fwkFrame);
        lk.unlock();
    }

    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    int64_t duration_count = duration.count();

    MLOGI(LogGroupHAL,
          "[PostProcessor][getBuffer] fwkFrame:%d Request buffer Finish, cost time %zd ms",
          fwkFrame >> SHIFTBITNUM, duration_count);
    return 0;
}

void PostProcessor::removeMission(uint64_t fwkFrameNum)
{
    if (mFlushed.load()) {
        return;
    }

    std::unique_lock<std::mutex> lk(mRequestMutex);
    std::shared_ptr<mihal::PostProcessor::RequestRecord> record = nullptr;

    auto iter = mOutputRequests.find(fwkFrameNum);
    if (iter != mOutputRequests.end()) {
        record = iter->second;
        mOutputRequests.erase(fwkFrameNum);
        lk.unlock();
        MLOGI(LogGroupHAL, "[PostProcessor][removeMission] fwkFrame:%d change fRemove",
              fwkFrameNum >> SHIFTBITNUM);
    }
}

uint32_t PostProcessor::getCameraModeByRoleId(int32_t roleId, const Metadata *meta)
{
    switch (roleId) {
    case CameraDevice::RoleIdRearWide:
        return CAM_COMBMODE_REAR_WIDE;
    case CameraDevice::RoleIdFront:
        return CAM_COMBMODE_FRONT;
    case CameraDevice::RoleIdRearTele:
    case CameraDevice::RoleIdRearTele2x:
    case CameraDevice::RoleIdRearTele3x:
    case CameraDevice::RoleIdRearTele5x:
    case CameraDevice::RoleIdRearTele10x:
        return CAM_COMBMODE_REAR_TELE;
    case CameraDevice::RoleIdRearUltra:
    case CameraDevice::RoleIdRear3PartSat:
        return CAM_COMBMODE_REAR_ULTRA;
    case CameraDevice::RoleIdRearMacro:
        return CAM_COMBMODE_REAR_MACRO;
    case CameraDevice::RoleIdFrontAux:
        return CAM_COMBMODE_FRONT_AUX;
    case CameraDevice::RoleIdRearSat:
    case CameraDevice::RoleIdRearSatUWT:
    case CameraDevice::RoleIdRearSatUWTT4x:
    case CameraDevice::RoleIdParallelVt:
        return CAM_COMBMODE_REAR_SAT_WU;
    case CameraDevice::RoleIdRearBokeh1x:
    case CameraDevice::RoleIdRearBokeh2x:
        return CAM_COMBMODE_REAR_BOKEH_WU;
    default:
        MLOGW(LogGroupHAL, "roleId %d is not supported", roleId);
        return CAM_COMBMODE_UNKNOWN;
    }

    return CAM_COMBMODE_UNKNOWN;
}

void PostProcessor::buildInputFrame(INPUTBUFF inputBuffers,
                                    const std::shared_ptr<CaptureRequest> &request,
                                    MiaFrame &inFrame, uint32_t index)
{
    auto inputStreamBuffer = request->getInputStreamBuffer(index);
    if (inputStreamBuffer == nullptr) {
        MLOGE(LogGroupHAL, "vndFrame:%u inputStreamBuffer is null", request->getFrameNumber());
        return;
    }

    Metadata copiedMeta{*request->getMeta()};
    convert2MiaImage(inFrame, inputStreamBuffer);

    GraBuffer *buffer = inputStreamBuffer->getBuffer();
    MASSERT(buffer->getHandle());
    if (inputBuffers) {
        (*inputBuffers)[buffer->getHandle()] = inputStreamBuffer;
    }

    const char *phyCamera = inputStreamBuffer->getStream()->getPhyId();
    camera_metadata *phyRawMeta = nullptr;
    if (phyCamera && strlen(phyCamera) > 0) {
        for (int i = 0; i < request->getPhyMetaNum(); i++) {
            if (!strcmp(request->getPhyCamera(i), phyCamera)) {
                Metadata copiedPhyMeta{*request->getPhyMeta(i)};
                phyRawMeta = copiedPhyMeta.release();
                break;
            }
        }
    }

    // NOTE: update "xiaomi.multiframe.inputNum" for each buffer
    copiedMeta.update(request->getBufferSpecificMeta(inputStreamBuffer.get()));

    inFrame.metaWraper = std::make_shared<DynamicMetadataWraper>(copiedMeta.release(), phyRawMeta);
    inFrame.callback = mBufferCallback.get();

    // XXX:TODO: the ID returned from getStreamId() may not match to algofwk
    // at the algofwk point of view, the streamID is used to match the port, which is
    // a predefined hardcode. we need to add a mapping for the right one
    inFrame.frameNumber = request->getFrameNumber();
    inFrame.streamId = inputStreamBuffer->getMialgoPort();

    MLOGI(LogGroupHAL, "input[%d] vndFrame:%u streamId %d", index, inFrame.frameNumber,
          inFrame.streamId);
}

void PostProcessor::convert2MiaImage(MiaFrame &image, std::shared_ptr<StreamBuffer> &streamBuffer)
{
    Stream *stream = streamBuffer->getStream();
    image.format.width = stream->getWidth();
    image.format.height = stream->getHeight();
    image.format.format = stream->getFormat();

    const char *phyCamera = stream->getPhyId();
    if (phyCamera && strlen(phyCamera) > 0) {
        image.format.cameraId = atoi(phyCamera);
    } else {
        image.format.cameraId = atoi(stream->getCamera()->getId().c_str());
    }

    GraBuffer *buffer = streamBuffer->getBuffer();
    image.pBuffer.phHandle = buffer->getHandle();
    image.pBuffer.pAddr[0] = image.pBuffer.pAddr[1] = NULL;
    image.pBuffer.fd[0] = image.pBuffer.fd[1] = -1;

    MLOGI(LogGroupHAL, "width %d height %d format %d cameraId %d", image.format.width,
          image.format.height, image.format.format, image.format.cameraId);
}

void PostProcessor::updateReferenceFrameIndex(std::shared_ptr<CaptureRequest> request,
                                              std::shared_ptr<RequestRecord> tempRecord)
{
    if (tempRecord->referenceFrameIndex == -1) {
        static int32_t debugKeyFrameId =
            property_get_int32("persist.vendor.camera.3adebug.keyframeId", -1);
        if (debugKeyFrameId > -1) {
            tempRecord->referenceFrameIndex = debugKeyFrameId;
            MLOGI(LogGroupHAL,
                  "[Exif][Offline DebugData][getReference] debug reference frame index: %d",
                  debugKeyFrameId);
            return;
        }
        // For RawHdr/SR/SN/ELLC/frontSN/BokehSN, specify the referenceFrameIndex directly
        auto entry = request->findInMeta(MI_RAWHDR_ENABLE);
        bool enableRawHDR = false;
        if (entry.count)
            enableRawHDR = entry.data.u8[0];

        entry = request->findInMeta(MI_SUPER_RESOLUTION_ENALBE);
        bool enableSR = false;
        if (entry.count)
            enableSR = entry.data.u8[0];

        if (enableRawHDR || enableSR) {
            tempRecord->referenceFrameIndex = 0;
            MLOGI(LogGroupHAL,
                  "[Exif][Offline DebugData][getReference] RawHDR/SR reference frame index: 0");
            return;
        }

        entry = request->findInMeta(MI_SUPERNIGHT_ENABLED); // Front supernight
        if (entry.count && entry.data.u8[0] && tempRecord->referenceFrameIndex == -1) {
            tempRecord->referenceFrameIndex = KEYFRAMEID_IN_MULTIFRAMEALGO_FRONTSN - 1;
            MLOGI(LogGroupHAL,
                  "[Exif][Offline DebugData][getReference] Front SN reference frame index: %u",
                  tempRecord->referenceFrameIndex);
            // #define KEYFRAMEID_IN_MULTIFRAMEALGO_FRONTSN  4
            return;
        }

        entry = request->findInMeta(MI_BOKEH_SUPERNIGHT_ENABLE);
        if (entry.count && entry.data.u8[0] && tempRecord->referenceFrameIndex == -1) {
            tempRecord->referenceFrameIndex = KEYFRAMEID_IN_MULTIFRAMEALGO_BOKEHSN - 1;
            // #define KEYFRAMEID_IN_MULTIFRAMEALGO_BOKEHSN  3
            MLOGI(LogGroupHAL,
                  "[Exif][Offline DebugData][getReference] Bokeh SN reference frame index: %u",
                  tempRecord->referenceFrameIndex);
            return;
        }

        entry = request->findInMeta(MI_MIVI_SUPERNIGHT_MODE);
        if (entry.count && tempRecord->referenceFrameIndex == -1) {
            uint8_t snMode = entry.data.u8[0];
            if (snMode > MIVISuperNightNone && snMode < MIVISuperLowLightHand) {
                tempRecord->referenceFrameIndex = KEYFRAMEID_IN_MULTIFRAMEALGO_SN - 1;
                // #define KEYFRAMEID_IN_MULTIFRAMEALGO_SN  3
                MLOGI(LogGroupHAL,
                      "[Exif][Offline DebugData][getReference] SN reference frame index: %u",
                      tempRecord->referenceFrameIndex);
                return;
            } else if (snMode >= MIVISuperLowLightHand && snMode < MIVINightMotionHand) {
                tempRecord->referenceFrameIndex = KEYFRAMEID_IN_MULTIFRAMEALGO_ELL - 1;
                // #define KEYFRAMEID_IN_MULTIFRAMEALGO_ELL 8
                MLOGI(LogGroupHAL,
                      "[Exif][Offline DebugData][getReference] ELL reference frame index: %u",
                      tempRecord->referenceFrameIndex);
                return;
            }
        }
        // SNSC according MiAlgo Anchor

        // For YUVHDR/Bokeh HDR/SR HDR/first frame is not ev0, select first ev0 as reference frame
        entry = request->findInMeta(MI_HDR_ENABLE);
        bool enableYUVHDR = false;
        if (entry.count)
            enableYUVHDR = entry.data.u8[0];

        entry = request->findInMeta(MI_BOKEH_HDR_ENABLE);
        bool enableBokehHDR = false;
        if (entry.count)
            enableBokehHDR = entry.data.u8[0];

        entry = request->findInMeta(MI_HDR_SR_ENABLE);
        bool enableHDRSR = false;
        if (entry.count)
            enableHDRSR = entry.data.u8[0];

        entry = request->findInMeta(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
        int32_t ev = -1;
        if (entry.count) {
            ev = entry.data.i32[0];
        }

        // For first frame is ev0, select last ev0 as reference frame
        if (!(enableYUVHDR || enableBokehHDR || enableHDRSR) && tempRecord->index == 0 && ev == 0)
            tempRecord->useLastEv0 = true;

        if (ev == 0) // first ev 0
            tempRecord->referenceFrameIndex = tempRecord->index;
    } else if (tempRecord->useLastEv0) {
        auto entry = request->findInMeta(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
        if (entry.count) {
            int32_t ev = entry.data.i32[0];
            if (ev == 0) // first ev 0
                tempRecord->referenceFrameIndex = tempRecord->index;
        }
    }
}

PostProcessorManager &PostProcessorManager::getInstance()
{
    static PostProcessorManager mInstance;
    return mInstance;
}

// PostProcessorManager::configureStreams mustn't be called concurrently.
// Otherwise, some mialgoengine may be deleted unexpectedly.
void PostProcessorManager::configureStreams(
    std::string signature, std::shared_ptr<StreamConfig> config, std::shared_ptr<ResultData> Info,
    std::shared_ptr<Metadata> meta,
    std::function<void(const std::string &sessionName)> monitorImageCategory)
{
    std::lock_guard<std::mutex> lkAlgo(mAlgoMutex);
    auto iter = mPostProcessors.find(signature);
    std::shared_ptr<PostProcessor> processor;
    mCurrentSig = signature;
    bool clear = false;
    if (iter == mPostProcessors.end()) {
        processor = std::make_shared<PostProcessor>(signature, config, mWaitMillisecs,
                                                    monitorImageCategory);
        mPostProcessors[signature] = processor;
        clear = true;
        MLOGD(LogGroupHAL, "[PostProcessorManager]: now %d PostProcessors alive",
              mPostProcessors.size());
    } else {
        processor = iter->second;
    }
    if (Info) {
        MLOGD(LogGroupHAL, "[PostProcessorManager]:setInvitationInfo signature:%s",
              signature.c_str());

        processor->setInvitationInfo(Info, meta, mfRemove);
        {
            std::lock_guard<std::mutex> lk(mBusyMutex);
            uint64_t fakeFwkFrameNum = Info->frameNumber;
            fakeFwkFrameNum = fakeFwkFrameNum << SHIFTBITNUM | Info->sessionId;
            mBusyPostProcessors[fakeFwkFrameNum] =
                std::make_pair<std::string, std::weak_ptr<PostProcessor>>(signature.c_str(),
                                                                          processor);
        }
    }

    {
        std::lock_guard<std::mutex> lkCount(mCountMutex);
        mMissionCount[signature] += Info ? 1 : 0;
    }
    if (clear) {
        mReclaimCond.notify_one();
    }
}

void PostProcessorManager::preProcessCaptureRequest(uint64_t fwkFrameNum,
                                                    std::shared_ptr<PreProcessConfig> config)
{
    MLOGD(LogGroupHAL,
          "[PostProcessorManager]:preProcessCaptureRequest preProcessCaptureRequest:%d",
          fwkFrameNum >> SHIFTBITNUM);

    std::lock_guard<std::mutex> lk(mBusyMutex);
    auto iter = mBusyPostProcessors.find(fwkFrameNum);
    if (iter != mBusyPostProcessors.end()) {
        auto processor = (iter->second.second).lock();
        if (processor) {
            processor->preProcessCaptureRequest(config);
        } else
            MLOGW(LogGroupHAL,
                  "[PostProcessorManager][preProcessCaptureRequest] fwkFrame:%d PostProcessor is "
                  "invalid",
                  fwkFrameNum >> SHIFTBITNUM);
    } else {
        MLOGE(
            LogGroupHAL,
            "[PostProcessorManager][preProcessCaptureRequest] PostProcessor:fwkFrame:%d can't find",
            fwkFrameNum >> SHIFTBITNUM);
    }
}

void PostProcessorManager::processInputRequest(
    std::string signature, std::shared_ptr<CaptureRequest> request, bool first,
    std::function<void(int64_t)> fAnchor,
    std::function<void(const CaptureResult *, bool)> fFeedback, uint64_t fwkFrameNum,
    bool isEarlyPicDone)
{
    MLOGD(LogGroupHAL, "[PostProcessorManager]:processInputRequest signature:%s",
          signature.c_str());
    bool findPost = false;
    {
        std::lock_guard<std::mutex> lk(mBusyMutex);
        auto iter = mBusyPostProcessors.find(fwkFrameNum);

        if (iter != mBusyPostProcessors.end()) {
            auto processor = (iter->second.second).lock();
            if (processor) {
                findPost = true;
                processor->processInputRequest(request, first, fAnchor, fFeedback, fwkFrameNum,
                                               isEarlyPicDone);
            } else {
                MLOGW(LogGroupHAL,
                      "[PostProcessorManager][processInputRequest] fwkFrame:%d PostProcessor is "
                      "invalid",
                      fwkFrameNum >> SHIFTBITNUM);
            }
        } else {
            MLOGW(LogGroupHAL,
                  "[PostProcessorManager][processInputRequest] PostProcessor:%s can not find",
                  signature.c_str());
        }
    }
    if (!findPost) {
        PostProcessor::processErrorRequest(request, fwkFrameNum, first ? fFeedback : nullptr);
    }
}

void PostProcessorManager::processOutputRequest(std::shared_ptr<CaptureRequest> request,
                                                uint64_t fwkFrameNum)
{
    MLOGD(LogGroupHAL, "[PostProcessorManager]:processOutputRequest fwkFrame:%d",
          fwkFrameNum >> SHIFTBITNUM);
    std::unique_lock<std::mutex> lk(mBusyMutex);
    auto iter = mBusyPostProcessors.find(fwkFrameNum);
    bool findPost = false;
    if (iter != mBusyPostProcessors.end()) {
        lk.unlock();
        auto processor = (iter->second.second).lock();
        if (processor) {
            findPost = true;
            processor->processOutputRequest(request, fwkFrameNum);
        } else {
            MLOGW(
                LogGroupHAL,
                "[PostProcessorManager][processOutputRequest] fwkFrame:%d PostProcessor is invalid",
                fwkFrameNum >> SHIFTBITNUM);
        }

    } else {
        lk.unlock();
        MLOGW(LogGroupHAL,
              "[PostProcessorManager][processOutputRequest] fwkFrame:%d PostProcessor can not find",
              fwkFrameNum >> SHIFTBITNUM);
    }

    if (!findPost) {
        PostProcessor::processErrorRequest(request, fwkFrameNum);
    }
}

void PostProcessorManager::quickFinishTask(uint64_t fwkFrameNum, std::string fileName,
                                           bool needResult)
{
    MLOGD(LogGroupHAL, "[PostProcessorManager][quickFinishTask] fwkFrame:%d ",
          fwkFrameNum >> SHIFTBITNUM);
    std::lock_guard<std::mutex> lk(mBusyMutex);
    auto iter = mBusyPostProcessors.find(fwkFrameNum);
    if (iter != mBusyPostProcessors.end()) {
        auto processor = (iter->second.second).lock();
        if (processor)
            processor->quickFinishTask(fwkFrameNum, fileName, needResult);
        else
            MLOGE(LogGroupHAL,
                  "[PostProcessorManager][quickFinishTask] fwkFrame:%d PostProcessor is invalid",
                  fwkFrameNum >> SHIFTBITNUM);
    } else
        MLOGE(LogGroupHAL,
              "[PostProcessorManager][quickFinishTask] fwkFrame:%d PostProcessor can not find",
              fwkFrameNum >> SHIFTBITNUM);
}

// When the APP exits abnormally, the framework will call  mock camera flush.
// When  call  mock camera flush, if  mock camera has pending requests,
// it will call PostProcessorManager::flush(),
// and  PostProcessorManager will force all algorithms to flush.
void PostProcessorManager::flush()
{
    MLOGD(LogGroupHAL, "[PostProcessorManager][flush] flush begin");

    std::lock_guard<std::mutex> lk(mAlgoMutex);
    for (auto &iter : mPostProcessors) {
        iter.second->flush(true);
    }
    mPostProcessors.clear();

    std::lock_guard<std::mutex> lkCount(mCountMutex);
    mMissionCount.clear();

    MLOGD(LogGroupHAL, "[PostProcessorManager][flush] flush end");
}

void PostProcessorManager::flush(std::string signature)
{
    {
        std::lock_guard<std::mutex> lk(mBusyMutex);
        std::set<uint32_t> rm;
        for (auto &iter : mBusyPostProcessors) {
            if (iter.second.first == signature)
                rm.insert(iter.first);
        }
        for (auto &iter : rm) {
            mBusyPostProcessors.erase(iter);
        }
    }
    {
        std::lock_guard<std::mutex> lk(mAlgoMutex);
        auto iter = mPostProcessors.find(signature);
        if (iter != mPostProcessors.end()) {
            iter->second->flush(true);
            mPostProcessors.erase(iter);
        } else {
            MLOGE(LogGroupHAL, "[PostProcessorManager][flush] PostProcessor:%s can not find",
                  signature.c_str());
        }
    }
}

void PostProcessorManager::clear()
{
    {
        std::lock_guard<std::mutex> lk(mBusyMutex);
        mBusyPostProcessors.clear();
    }
    std::set<std::shared_ptr<PostProcessor>> rmProcessors;
    {
        std::lock_guard<std::mutex> lk(mAlgoMutex);
        if (mPostProcessors.size() == 0)
            return;

        for (auto &iter : mPostProcessors) {
            rmProcessors.insert(iter.second);
        }

        mPostProcessors.clear();
        std::lock_guard<std::mutex> lkCount(mCountMutex);
        mMissionCount.clear();
    }
    MLOGD(LogGroupHAL, "[PostProcessorManager][clear] clear all PostProcessors");
}

void PostProcessorManager::removeMission(uint64_t fwkFrameNum)
{
    MLOGD(LogGroupHAL, "[PostProcessorManager][removeMission] fwkFrame:%d ",
          fwkFrameNum >> SHIFTBITNUM);
    std::unique_lock<std::mutex> lk(mBusyMutex);

    auto iter = mBusyPostProcessors.find(fwkFrameNum);
    if (iter != mBusyPostProcessors.end()) {
        auto processor = (iter->second.second).lock();
        lk.unlock();
        if (processor) {
            processor->removeMission(fwkFrameNum);
        } else {
            MLOGE(LogGroupHAL,
                  "[PostProcessorManager][removeMission] fwkFrame:%d PostProcessor is invalid",
                  fwkFrameNum >> SHIFTBITNUM);
        }
    }
}

void PostProcessorManager::inviteForMockRequest(uint64_t fwkFrameNum, const std::string &imageName,
                                                bool invite)
{
    std::lock_guard<std::mutex> lk(mBusyMutex);
    auto iter = mBusyPostProcessors.find(fwkFrameNum);
    if (iter != mBusyPostProcessors.end()) {
        auto processor = (iter->second.second).lock();
        if (processor) {
            MLOGD(LogGroupHAL, "[PostProcessorManager][inviteForMockRequest] fwkFrame:%d invite:%d",
                  fwkFrameNum >> SHIFTBITNUM, invite);
            processor->inviteForMockRequest(fwkFrameNum, imageName, invite);
        } else
            MLOGE(LogGroupHAL,
                  "[PostProcessorManager][inviteForMockRequest] fwkFrame:%d PostProcessor is "
                  "invalid",
                  fwkFrameNum >> SHIFTBITNUM);
    }
}

void PostProcessorManager::notifyBGServiceHasDied(int32_t clientId)
{
    std::lock_guard<std::mutex> lk(mAlgoMutex);
    for (auto &iter : mPostProcessors) {
        auto processor = iter.second;
        processor->notifyBGServiceHasDied(clientId);
    }
}

void PostProcessorManager::reclaimLoop()
{
    setpriority(PRIO_PROCESS, 0, 10); // 10 = 130 - 120, 130 is priority

    while (true) {
        std::unique_lock<std::mutex> lkAlgo(mAlgoMutex);
        std::unique_lock<std::mutex> lkCount(mCountMutex, defer_lock);
        mReclaimCond.wait(lkAlgo, [this, &lkCount]() {
            if (mQuit) {
                return true;
            }

            MLOGD(LogGroupHAL, "[PostProcessorManager][reclaimLoop]:now %d PostProcessors alive",
                  mPostProcessors.size());
            if (mPostProcessors.size() < 2) {
                return false;
            }

            lkCount.lock();
            for (auto &iter : mMissionCount) {
                if (iter.first != mCurrentSig && iter.second == 0) {
                    return true;
                }
            }

            lkCount.unlock();
            return false;
        });

        if (mQuit) {
            return;
        }

        std::set<std::string> rmSig;
        std::set<std::shared_ptr<PostProcessor>> rm;
        {
            for (auto &iter : mMissionCount) {
                if (iter.first != mCurrentSig && iter.second == 0) {
                    rmSig.insert(iter.first);
                }
            }
            for (auto &iter : rmSig) {
                mMissionCount.erase(iter);
            }
        }
        lkCount.unlock();

        if (rmSig.size()) {
            for (auto &iter : rmSig) {
                auto procIter = mPostProcessors.find(iter);
                if (procIter != mPostProcessors.end()) {
                    rm.insert(procIter->second);
                    mPostProcessors.erase(procIter);
                }
            }
            MLOGD(LogGroupHAL, "[PostProcessorManager]:now %d PostProcessors alive",
                  mPostProcessors.size());
        }
        lkAlgo.unlock();
    }
} // namespace mihal

PostProcessorManager::PostProcessorManager()
{
    MiviSettings::getVal("MillisecondsWaitingForMockRequest", mWaitMillisecs, 10000);
    mQuit.store(false);
    mThread = std::thread([this] { reclaimLoop(); });
    // Asynchronous process mialgo result
    Singleton<ThreadPool>::Instance(RESULTTHREADNUM, -3);
    mfRemove = [this](uint64_t fwkFrameNum) {
        std::string signature;

        {
            std::lock_guard<std::mutex> lk(mBusyMutex);
            MLOGD(LogGroupHAL,
                  "[PostProcessorManager]:fRemove remove mBusyPostProcessors fwkFrame:%d",
                  fwkFrameNum >> SHIFTBITNUM);
            auto iter = mBusyPostProcessors.find(fwkFrameNum);
            if (iter != mBusyPostProcessors.end()) {
                signature = mBusyPostProcessors[fwkFrameNum].first;
                mBusyPostProcessors.erase(fwkFrameNum);
            }
            for (auto &task : mBusyPostProcessors) {
                MLOGD(LogGroupHAL, "[PostProcessorManager]:remain PostProcessors %s, fwkFrame:%d",
                      task.second.first.c_str(), task.first >> SHIFTBITNUM);
            }
        }

        if (signature.size()) {
            std::lock_guard<std::mutex> lkCount(mCountMutex);
            auto iter = mMissionCount.find(signature);
            if (iter != mMissionCount.end()) {
                iter->second = iter->second ? iter->second - 1 : 0;
                MLOGD(LogGroupHAL, "[PostProcessorManager]: PostProcessors %s,mission count %d",
                      iter->first.c_str(), iter->second);
                if (!iter->second) {
                    mReclaimCond.notify_one();
                }
            }
        }
    };
};

PostProcessorManager::~PostProcessorManager()
{
    clear();
    mQuit.store(true);
    mReclaimCond.notify_one();
    if (mThread.joinable()) {
        mThread.join();
    }
}

} // namespace mihal
