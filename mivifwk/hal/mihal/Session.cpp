#include "Session.h"

#include <string.h>
#include <sys/prctl.h>

#include <chrono>

#include "CameraManager.h"
#include "LogUtil.h"
#include "Metadata.h"
#include "MiHal3BufferMgr.h"
#include "MialgoTaskMonitor.h"
#include "MihalMonitor.h"
#include "MiviSettings.h"
#include "OfflineDebugDataUtils.h"
#include "Photographer.h"
#include "Previewer.h"
#include "Reprocess.h"
#include "SdkMode.h"
#include "VendorCamera.h"

using namespace android;
using namespace vendor::xiaomi::hardware::bgservice::implementation;

namespace mihal {

#define DARKROOMTHREADNUM      1
#define DARKROOMCLEARTHREADNUM 2
using namespace std::chrono_literals;

namespace {

uint32_t getUniqueId()
{
    static std::atomic<uint32_t> id{};

    return id.fetch_add(1);
}

} // namespace

Session::Session(VendorCamera *vendorCamera, uint8_t sessionType)
    : mVendorCamera{vendorCamera}, mInited{false}, mFrameNumToVendor{0}, mSessionType(sessionType)
{
    mConfigToVendor = nullptr;
    // set the session id
    int32_t cameraId = std::stoi(vendorCamera->getId());
    mId = getUniqueId() << 8 | (cameraId & 0xff);
}

int Session::flush()
{
    return mVendorCamera->flush2Vendor();
}

void Session::signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams)
{
    mVendorCamera->signalStreamFlush2Vendor(numStream, streams);
}

camera3_buffer_request_status Session::requestStreamBuffers(
    uint32_t num_buffer_reqs, const camera3_buffer_request *buffer_reqs,
    /*out*/ uint32_t *num_returned_buf_reqs,
    /*out*/ camera3_stream_buffer_ret *returned_buf_reqs)
{
    return mVendorCamera->requestStreamBuffers(num_buffer_reqs, buffer_reqs, num_returned_buf_reqs,
                                               returned_buf_reqs);
}

void Session::returnStreamBuffers(uint32_t num_buffers, const camera3_stream_buffer *const *buffers)
{
    return mVendorCamera->returnStreamBuffers(num_buffers, buffers);
}

NonAlgoSession::NonAlgoSession(VendorCamera *vendorCamera, std::shared_ptr<StreamConfig> config,
                               std::shared_ptr<EcologyAdapter> ecoAdapet)
    : Session(vendorCamera, NonAlgoSessionType), mEcologyAdapter(ecoAdapet)
{
    // NOTE: don't worry about the life time of mConfigToVendor, it can be proved that
    // mConfigToVendor remains valid until VendorCamera send it to vendor
    mConfigFromFwk = std::make_unique<LocalConfig>(*config);

    mConfigToVendor = mConfigFromFwk.get();

    if (mEcologyAdapter != nullptr) {
        mEcologyAdapter->buildVideoConfig(reinterpret_cast<LocalConfig *>(mConfigToVendor));
    }

    MLOGI(LogGroupHAL, " for camera %s, finalized config to vendor:\n\t%s",
          vendorCamera->getId().c_str(), config->toString().c_str());

    mInited = true;
}

NonAlgoSession::~NonAlgoSession()
{
    MLOGI(LogGroupHAL, "session 0x%x destroyed", getId());
}

int NonAlgoSession::processRequest(std::shared_ptr<CaptureRequest> request)
{
    incSessionFrameNum();

    // if mEcologyAdapter is nullptr,it means sdk not support current app
    if (mEcologyAdapter != nullptr && !request->getMeta()->isEmpty()) {
        LocalRequest lq(request->getCamera(), request->toRaw());
        mEcologyAdapter->buildVideoRequest(&lq);
        return mVendorCamera->submitRequest2Vendor(&lq);
    }

    return mVendorCamera->submitRequest2Vendor(request.get());
}

namespace {

void processResult_(const camera3_callback_ops *ops, const camera3_capture_result *result)
{
    const AlgoSession::Callback *callback = static_cast<const AlgoSession::Callback *>(ops);
    callback->processResultFunc(result);
}

void notify_(const camera3_callback_ops *ops, const camera3_notify_msg *msg)
{
    const AlgoSession::Callback *callback = static_cast<const AlgoSession::Callback *>(ops);
    callback->notifyFunc(msg);
}

camera3_buffer_request_status requestStreamBuffers_(
    const camera3_callback_ops *ops, uint32_t num_buffer_reqs,
    const camera3_buffer_request *buffer_reqs,
    /*out*/ uint32_t *num_returned_buf_reqs,
    /*out*/ camera3_stream_buffer_ret *returned_buf_reqs)
{
    const AsyncAlgoSession::Callback *callback =
        static_cast<const AsyncAlgoSession::Callback *>(ops);
    return callback->requestStreamBuffersFunc(num_buffer_reqs, buffer_reqs, num_returned_buf_reqs,
                                              returned_buf_reqs);
}

void returnStreamBuffers_(const camera3_callback_ops *ops, uint32_t num_buffers,
                          const camera3_stream_buffer *const *buffers)
{
    const AsyncAlgoSession::Callback *callback =
        static_cast<const AsyncAlgoSession::Callback *>(ops);
    callback->returnStreamBuffersFunc(num_buffers, buffers);
}

} // namespace

AlgoSession::AlgoSession(VendorCamera *vendorCamera, uint8_t sessionType)
    : Session{vendorCamera, sessionType}
{
    mPendingMiHALSnapCount = 0;
    mPendingVendorHALSnapCount = 0;
    mPendingVendorMFNRSnapCount = 0;
    mFirstFrameArrived = false;
    mFirstFrameRequested = false;
    mAnchorFrameEnabled = false;
    mClientId = 0;
    mDarkRoomThread = std::make_unique<ThreadPool>(DARKROOMTHREADNUM, -3);
}

AlgoSession::RequestExecutor::RequestExecutor(const CaptureRequest *fwkRequest, Session *session)
    : mFwkRequest{std::make_shared<LocalRequest>(*fwkRequest)},
      mSession{static_cast<AlgoSession *>(session)}
{
    auto entry = mFwkRequest->findInMeta(MI_SNAPSHOT_IMAGE_NAME);
    if (entry.count) {
        mImageName = reinterpret_cast<const char *>(entry.data.u8);
    }
}

AlgoSession::RequestExecutor::RequestExecutor(std::shared_ptr<const CaptureRequest> fwkRequest,
                                              Session *session)
    : mFwkRequest{std::make_shared<LocalRequest>(*fwkRequest)},
      mSession{static_cast<AlgoSession *>(session)}
{
    auto entry = mFwkRequest->findInMeta(MI_SNAPSHOT_IMAGE_NAME);
    if (entry.count) {
        mImageName = reinterpret_cast<const char *>(entry.data.u8);
    }
}

std::shared_ptr<NotifyMessage> AsyncAlgoSession::RequestExecutor::generateFwkNotifyMessage(
    const NotifyMessage *vendorMsg, const CaptureRequest *fwkReq)
{
    MASSERT(vendorMsg);
    MASSERT(fwkReq);

    // NOTE: if msg didn't contain stream info, we can bypass it with fwk frame num
    if (!vendorMsg->isErrorMsg() || vendorMsg->getErrorCode() != CAMERA3_MSG_ERROR_BUFFER) {
        return std::make_shared<NotifyMessage>(*vendorMsg, fwkReq->getFrameNumber());
    }

    camera3_stream *matchedFwkStream = nullptr;
    if (vendorMsg->getErrorStream()) {
        matchedFwkStream = getMatchedFwkStream(vendorMsg->getErrorStream(), fwkReq);
    }

    if (matchedFwkStream) {
        return std::make_shared<NotifyMessage>(vendorMsg->getCamera(), fwkReq->getFrameNumber(),
                                               matchedFwkStream);
    }

    return nullptr;
}

camera3_stream *AsyncAlgoSession::RequestExecutor::getMatchedFwkStream(camera3_stream *vendorStream,
                                                                       const CaptureRequest *fwkReq)
{
    if (!vendorStream) {
        return nullptr;
    }

    // NOTE: 'Matched' means vendorStream is either identical to a fwk stream (which is bypassed to
    // Qcom) or has a corresponding fwk stream (which is overrided by mihal stream during
    // configuring stream)

    auto richVndSteam = Stream::toRichStream(vendorStream);
    // NOTE: this vnd stream is actually a fwk stream
    if (!richVndSteam->isInternal())
        return vendorStream;

    // NOTE: this vnd stream has a corresponding fwk stream
    if (richVndSteam->hasPeerStream())
        return richVndSteam->getPeerStream()->toRaw();

    return nullptr;
}

int AsyncAlgoSession::RequestExecutor::tryNotifyToFwk(const NotifyMessage *vendorMsg,
                                                      const CaptureRequest *fwkReq,
                                                      Mia2LogGroup logGroup, const char *logPrefix)
{
    auto fwkMsg = generateFwkNotifyMessage(vendorMsg, fwkReq);
    if (!fwkMsg) {
        MASSERT(vendorMsg->isErrorMsg());
        MLOGW(logGroup,
              "%s[Notify]: (vndFrame:%u fwkFrame:%u) error vendor stream is: %p, didn't find "
              "matched fwk stream, discard this msg !",
              logPrefix, vendorMsg->getFrameNumber(), fwkReq->getFrameNumber(),
              vendorMsg->getErrorStream());
        return -1;
    }

    MLOGI(logGroup, "%s[Notify]: (vndFrame:%u fwkFrame:%u) msg to fwk: %s", logPrefix,
          vendorMsg->getFrameNumber(), fwkReq->getFrameNumber(), fwkMsg->toString().c_str());
    return forwardNotifyToFwk(fwkMsg.get());
}

int AlgoSession::RequestExecutor::forwardResultToFwk(LocalResult *result)
{
    auto ret = mSession->updatePendingFwkRecords(result);
    if (!ret) {
        if (isSupportFovCrop()) {
            auto entry = result->findInMeta(ANDROID_CONTROL_ZOOM_RATIO);
            if (entry.count) {
                float QcZoomRatio = entry.data.f[0];
                float UIZoomRatio = QcZoomRatio / getFovCropZoomRatio();
                MLOGI(LogGroupRT, "Revert reault zoomRatio %0.2fX->%0.2fX", QcZoomRatio,
                      UIZoomRatio);
                result->updateMeta(ANDROID_CONTROL_ZOOM_RATIO, &UIZoomRatio, 1);
            }
        }
        mFwkRequest->getCamera()->processCaptureResult(result);
    } else {
        MLOGW(LogGroupHAL,
              "%s[ProcessResult][Buffer]: Not found fwkFrame:%u , cause it was returned by mihal",
              getLogPrefix().c_str(), result->getFrameNumber());
    }
    return 0;
}

int AlgoSession::RequestExecutor::forwardNotifyToFwk(const NotifyMessage *msg)
{
    msg->notify();

    if (msg->getCamera()->isMockCamera())
        return 0;

    mSession->updatePendingFwkRecords(msg);

    return 0;
}

std::string AlgoSession::RequestExecutor::getImageName() const
{
    return mImageName;
}

int AlgoSession::RequestExecutor::fillFwkBufferFromVendorHal(
    const CaptureRequest *fwkRequest, std::shared_ptr<StreamBuffer> srcVndBuffer,
    std::vector<std::shared_ptr<StreamBuffer>> &outBuffers)
{
    uint32_t fwkFrame = fwkRequest->getFrameNumber();
    MLOGI(LogGroupRT,
          "%s[ProcessResult][Buffer]: found fwkFrame:%u request with output buffer num:%u",
          getLogPrefix().c_str(), fwkFrame, fwkRequest->getOutBufferNum());

    for (int i = 0; i < fwkRequest->getOutBufferNum(); i++) {
        auto fwkStreamBuffer = fwkRequest->getStreamBuffer(i);
        auto fwkStream = fwkStreamBuffer->getStream();

        // NOTE: if buffer is sent to Qcom, then they will be filled by Qcom and mihal shouldn't
        // touch them
        if (fwkStreamBuffer->isSentToHal()) {
            MLOGD(LogGroupRT,
                  "%s[ProcessResult][Buffer]: stream:%p's buffer is sent to Hal, don't touch it",
                  getLogPrefix().c_str(), fwkStream->toRaw());
            continue;
        }

        // NOTE: fwk snapshot buffer will be filled by early pic thread in CameraMode
        if (fwkStreamBuffer->isSnapshot())
            continue;

        if (fwkStream->isTrivial()) {
            MLOGD(LogGroupRT,
                  "%s[ProcessResult][Buffer]: stream:%p is trivial, set its buffer as error",
                  getLogPrefix().c_str(), fwkStream->toRaw());
            fwkStreamBuffer->setErrorBuffer();
            outBuffers.push_back(fwkStreamBuffer);
            continue;
        }

        // NOTE: if buffer is not trivial, we should try to fill it
        fwkStreamBuffer->tryToCopyFrom(*srcVndBuffer);
        MLOGD(LogGroupRT,
              "%s[ProcessResult][Buffer]: stream:%p's buffer done processing, buffer status:%s",
              getLogPrefix().c_str(), fwkStream->toRaw(),
              fwkStreamBuffer->getStatus() ? "ERROR" : "OK");
        outBuffers.push_back(fwkStreamBuffer);
    }

    return 0;
}

void AlgoSession::RequestExecutor::markReqBufsAsSentToHal(CaptureRequest *vndRequest)
{
    // NOTE: mark each buffer in request as beenSentToHal;
    for (int i = 0; i < vndRequest->getOutBufferNum(); ++i) {
        auto buffer = vndRequest->getStreamBuffer(i);
        buffer->markSentToHal();

        if (buffer->isQuickview()) {
            mHasSentQuickviewBufferToHal = true;
        }
    }
}

int AlgoSession::addToVendorInflights(std::shared_ptr<RequestExecutor> executor)
{
    // NOTE: must insert to the inflights before execute the request, because the vendor hal
    //       may call the process_capture_result immediatly in some cases
    std::lock_guard<std::recursive_mutex> locker{mMutex};
    std::vector<uint32_t /* FrameNumber */> requests = executor->getVendorRequestList();

    for (auto frameNum : requests) {
        mVendorInflights.insert({frameNum, executor});
    }

    return 0;
}

// NOTE:For snapshots, photograher needs to return result meta, buffer, Earlypic, so it is necessary
// to ensure that all processing procedures have been completed during the life cycle of
// photograher. Among them, mFinalFinished can indicate whether the processing results of meta and
// Darkroom have been returned, and the mVndAnchorFrames of the map structure can help quickly
// locate whether the corresponding snapshot frame returns Earlypic
int AlgoSession::removeFromVendorInflights(uint32_t frame)
{
    std::lock_guard<std::recursive_mutex> locker{mMutex};
    mVendorInflights.erase(frame);

    return 0;
}

void AlgoSession::anchorFrameEvent(int64_t timestamp /* not used*/, uint32_t anchorFrame,
                                   uint32_t snapshotFrame)
{
    auto executor = getExecutor(snapshotFrame);

    if (!executor) {
        MLOGE(LogGroupHAL,
              "[EarlyPic]: executor should support anchor frame pic but not found!! searched "
              "vndFrame:%u",
              snapshotFrame);
        return;
    }

    if (!executor->doAnchorFrameAsEarlyPicture()) {
        MLOGW(LogGroupHAL, "[EarlyPic]: anchor frame is not enabled");
        return;
    }

    if (executor->testAnchoredStatus())
        return;

    // NOTE:In order to facilitate Executor management, processAnchorFrameResult and MiAlgoEngine
    // out buffer frame uniformly use first Snapshot vndFrame as index to retrieve Executor
    uint32_t fisrtSnapFrame = executor->getVendorRequestList()[0];
    if (snapshotFrame != fisrtSnapFrame) {
        MLOGI(LogGroupHAL, "%s[EarlyPic]: Replace snapshotFrame %u with first vndFrame %d",
              executor->getLogPrefix().c_str(), snapshotFrame, fisrtSnapFrame);
        snapshotFrame = fisrtSnapFrame;
    }

    MLOGI(LogGroupHAL,
          "%s[ProcessResult][EarlyPic]: Use meta anchor frame, target anchor vndFrame:%u, "
          "timestamp(%" PRId64 ")",
          executor->getLogPrefix().c_str(), anchorFrame, timestamp);
    mCameraMode->notifyAnchorFrame(anchorFrame, snapshotFrame);
}

int AlgoSession::insertPendingFwkRecords(std::shared_ptr<CaptureRequest> request)
{
    std::unique_lock<std::recursive_mutex> locker{mPendingRecordsMutex};

    mPendingFwkRecords.insert(
        {request->getFrameNumber(), std::make_unique<FwkRequestRecord>(request)});

    // Tips: Just for debug, when flushing, found some request hasn't been processed for a long time
    if (request->getFrameNumber() - mPendingFwkRecords.begin()->first > 64) {
        MLOGW(LogGroupHAL,
              "[Statistic]: current fwkFrame:%u, first pending fwkFrame:%u don't been processed "
              "long time, pending count %d",
              request->getFrameNumber(), mPendingFwkRecords.begin()->first,
              mPendingFwkRecords.size());
        // MASSERT(false);
    }
    return 0;
}

int AlgoSession::updatePendingFwkRecordsImpl(uint32_t frame, UpdateRecordFunc func)
{
    std::lock_guard<std::recursive_mutex> lock{mPendingRecordsMutex};
    auto it = mPendingFwkRecords.find(frame);
    if (it == mPendingFwkRecords.end()) {
        MLOGW(LogGroupHAL, "[ProcessResult]: pending fwkFrame:%u not found", frame);
        return -ENOENT;
    }

    auto &record = it->second;
    if (func)
        func(record.get());

    if (record->isCompleted()) {
        // NOTE: for debug
        if (record->fwkRequest->isSnapshot()) {
            MLOGD(LogGroupHAL, "[fwkFrame:%u][KEY]: [Fulfill][2/3]: vendor job done", frame);
        }

        mPendingFwkRecords.erase(it);
        {
            std::lock_guard<std::mutex> lk(mAttMutex);
            auto it = mAttachVendorFrame.find(frame);
            if (it != mAttachVendorFrame.end()) {
                auto executor = getExecutor(it->second);
                if (executor)
                    executor->removeAttachFwkRequest(frame);
                mAttachVendorFrame.erase(it);
            }
        }

        MLOGD(LogGroupRT,
              "[fwkFrame:%u][Statistic]: early jobs completed, now pending fwk req num:%zu", frame,
              mPendingFwkRecords.size());
    }

    return 0;
}

int AlgoSession::updatePendingFwkRecords(const CaptureResult *result)
{
    if (!mFirstFrameArrived && result->getBufferNumber()) {
        mFirstFrameArrived = true;
        MLOGI(LogGroupHAL, "[Performance] session 0x%x received first frame", getId());
        createDarkroom();
    }

    return updatePendingFwkRecordsImpl(
        result->getFrameNumber(),
        [result](FwkRequestRecord *record) { return record->updateByResult(result); });
}

int AlgoSession::updatePendingFwkRecords(const NotifyMessage *msg)
{
    return updatePendingFwkRecordsImpl(msg->getFrameNumber(), [msg](FwkRequestRecord *record) {
        return record->updateByNotify(msg);
    });
}

int AlgoSession::flushPendingFwkRecords()
{
    MLOGI(LogGroupHAL, "%zu requests need to be flushed", mPendingFwkRecords.size());
    {
        std::lock_guard<std::recursive_mutex> lock{mPendingRecordsMutex};
        for (auto &p : mPendingFwkRecords) {
            auto &record = p.second;
            if (record->isCompleted())
                continue;

            record->flush();

            // NOTE: for debug
            if (record->fwkRequest->isSnapshot()) {
                MLOGD(LogGroupHAL,
                      "[fwkFrame:%u]: [Fulfill][2/3]: vendor job done (in flush!! will lose other "
                      "Fulfill)",
                      record->fwkRequest->getFrameNumber());
                {
                    // 1、if pick  earlypic after flush ,previews have been set error and can not
                    // insert mInternalQuickviewBuffers when flush,then the action of picking
                    // earlypic will fail.Need to execute removeAttachFwkRequest() to destruct
                    // photographer
                    // 2、if pick  earlypic after flush,mPendingFwkRecords have been cleared,also
                    // need to execute removeAttachFwkRequest() to destruct photographer
                    std::unique_lock<std::mutex> lk(mAttMutex);
                    auto it = mAttachVendorFrame.find(p.first);
                    if (it != mAttachVendorFrame.end()) {
                        auto executor = getExecutor(it->second);
                        mAttachVendorFrame.erase(it);
                        if (executor) {
                            executor->removeAttachFwkRequest(p.first);
                            lk.unlock();
                            MLOGD(LogGroupHAL,
                                  "[fwkFrame:%u]: [Fulfill][2/3]: vendor job done (in flush!! "
                                  "removeAttachFwkRequest)",
                                  p.first);
                            int32_t *rmFrames = new int32_t[2];
                            if (executor->isFinish(executor->getVendorRequestList()[0], rmFrames)) {
                                for (int i = 0; i < 2; i++) {
                                    if (*(rmFrames + i) != -1)
                                        removeFromVendorInflights(*(rmFrames + i));
                                }
                            }
                            delete[] rmFrames;
                        }
                    }
                }
            } else {
                MLOGD(LogGroupRT, "[fwkFrame:%u][Preview]:", record->fwkRequest->getFrameNumber());
            }
        }
        mPendingFwkRecords.clear();
    }
    waitCameramodeLoop();

    return 0;
}

std::shared_ptr<AsyncAlgoSession::RequestExecutor> AlgoSession::getExecutor(uint32_t frame,
                                                                            bool ignore)
{
    std::unique_lock<std::recursive_mutex> locker{mMutex};
    auto it = mVendorInflights.find(frame);
    if (it == mVendorInflights.end()) {
        locker.unlock();
        if (ignore) {
            MLOGW(LogGroupHAL,
                  "vndFrame:%d  is before burst or postprocessor callback ,may not found in vendor "
                  "inflights",
                  frame);
        } else {
            MLOGE(LogGroupHAL, "vndFrame:%d not found in vendor inflights", frame);
        }
        return nullptr;
    }

    std::shared_ptr<RequestExecutor> executor = it->second;
    locker.unlock();

    return executor;
}

void AlgoSession::insertPendingSnapRecords(uint32_t frame)
{
    std::unique_lock<std::mutex> locker{mRecordMutex};
    std::bitset<RequestExecutor::HAL_MESSAGE_COUNT> recorder;
    mPendingSnapshotRecord.insert({frame, recorder});
    locker.unlock();

    mPendingMiHALSnapCount += 1;
    mPendingVendorHALSnapCount += 1;
}

int AlgoSession::updatePendingSnapRecords(uint32_t frame, std::string imageName, int type)
{
    MLOGD(LogGroupHAL, "[EarlyPic]: fwkFrame:%u,supported type %d", frame, type);
    if (type == RequestExecutor::HAL_VENDOR_REPORT) {
        mPendingVendorHALSnapCount -= 1;
        if (mPendingVendorHALSnapCount == 0) {
            std::lock_guard<std::mutex> locker{mSnapMutex};
            mSnapCond.notify_one();
        }
        return 0;
    }

    if (type != RequestExecutor::HAL_BUFFER_RESULT && type != RequestExecutor::HAL_SHUTTER_NOTIFY) {
        MLOGE(LogGroupHAL, "[Photographer]: unsupported type %d", type);
        return -1;
    }

    std::unique_lock<std::mutex> locker{mRecordMutex};
    auto it = mPendingSnapshotRecord.find(frame);
    if (it == mPendingSnapshotRecord.end()) {
        MLOGE(LogGroupHAL, "[Photographer]: fwkFrame:%u not found in snapshot record", frame);
        return -1;
    }

    auto &recorder = it->second;
    recorder.set(type);

    if (recorder.all()) {
        mPendingSnapshotRecord.erase(it);
        MLOGI(LogGroupHAL,
              "[fwkFrame:%u][ProcessResult][KEY][Statistic]: [Fulfill][1/3]: imageName:%s "
              "complete, now pending snapshot req num:%zu",
              frame, imageName.c_str(), mPendingSnapshotRecord.size());
        locker.unlock();

        BGServiceWrap::getInstance()->onCaptureCompleted(imageName, frame);
    }

    return 0;
}

int AlgoSession::processResult(CaptureResult *result)
{
    // NOTE: must use a shared_ptr to access the executor, otherwise the executor maybe deleted
    auto frame = result->getFrameNumber();
    auto executor = getExecutor(frame);
    if (executor) {
        executor->processVendorResult(result);
        int32_t *rmFrames = new int32_t[2];
        if (executor->isFinish(frame, rmFrames)) {
            for (int i = 0; i < 2; i++) {
                if (*(rmFrames + i) != -1)
                    removeFromVendorInflights(*(rmFrames + i));
            }
        }
        delete[] rmFrames;
        return 0;
    } else {
        // NOTE: According to the code annotation of camera3_capture_result in camera3.h:
        // 'If notify has been called with ERROR_RESULT, all further partial results for that
        // frame are ignored by the framework.'
        // The case above may be happend before, so we ignore it.
        MLOGW(LogGroupHAL,
              "no executor found for frame %d, it's phg has been destoryed, so"
              " try to ignore this result",
              frame);
        return 0;
    }
}

void AlgoSession::notify(const NotifyMessage *msg)
{
    auto frame = msg->getFrameNumber();
    auto executor = getExecutor(frame);

    if (executor) {
        executor->processVendorNotify(msg);
        int32_t *rmFrames = new int32_t[2];
        if (executor->isFinish(frame, rmFrames)) {
            for (int i = 0; i < 2; i++) {
                if (*(rmFrames + i) != -1)
                    removeFromVendorInflights(*(rmFrames + i));
            }
        }
        delete[] rmFrames;
    } else {
        // NOTE: If error result has returned but shutter come back after phg has been destroyed,
        // we don't need to return the shutter for this capture.
        MLOGW(LogGroupHAL, "no executor found for vndFrame:%d, it may has error returned", frame);
    }
}

void AlgoSession::signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams)
{
    std::vector<const camera3_stream *> streamsFlush2Vendor;
    mConfigToVendor->forEachStream([&streamsFlush2Vendor](const camera3_stream *stream2Vendor) {
        streamsFlush2Vendor.push_back(stream2Vendor);
        return 0;
    });

    MLOGI(LogGroupHAL, "[Hal3BufferMgr]: flush %zu streams to vendor", streamsFlush2Vendor.size());
    mVendorCamera->signalStreamFlush2Vendor(streamsFlush2Vendor.size(), streamsFlush2Vendor.data());

    // NOTE: when fwk signal stream flush, we need to wait and return all buffers.
    mVendorCamera->getFwkStreamHal3BufMgr()->signalStreamFlush(numStream, streams);
}

camera3_buffer_request_status AlgoSession::requestStreamBuffers(
    uint32_t num_buffer_reqs, const camera3_buffer_request *buffer_reqs,
    /*out*/ uint32_t *num_returned_buf_reqs,
    /*out*/ camera3_stream_buffer_ret *returned_buf_reqs)
{
    camera3_buffer_request_status finalStatus = CAMERA3_BUF_REQ_OK;
    *num_returned_buf_reqs = num_buffer_reqs;

    for (int i = 0; i < num_buffer_reqs; i++) {
        MLOGI(LogGroupRT, "[Hal3BufferMgr]: camera %s, stream:%p format:%d -> buffer num:%d",
              mVendorCamera->getId().c_str(), buffer_reqs[i].stream, buffer_reqs[i].stream->format,
              buffer_reqs[i].num_buffers_requested);

        auto stream = Stream::toRichStream(buffer_reqs[i].stream);
        MASSERT(stream);

        if (stream->isInternal()) {
            // NOTE: process internal buffer request
            std::vector<StreamBuffer *> buffers;
            int ret = stream->requestBuffers(buffer_reqs[i].num_buffers_requested, buffers);

            if (ret) {
                MLOGE(LogGroupHAL, "[Hal3BufferMgr] failed request for stream %p raw %p",
                      stream.get(), stream->toRaw());
                finalStatus = CAMERA3_BUF_REQ_FAILED_PARTIAL;
            }

            auto &retBuffer = returned_buf_reqs[i];
            retBuffer.stream = stream->toRaw();
            retBuffer.status = ret ? CAMERA3_PS_BUF_REQ_NO_BUFFER_AVAILABLE : CAMERA3_PS_BUF_REQ_OK;
            retBuffer.num_output_buffers = buffers.size();
            for (size_t j = 0; j < buffers.size(); ++j) {
                retBuffer.output_buffers[j] = *buffers[j]->toRaw();
            }
        } else {
            // NOTE: process fwk stream buffer request
            auto buffers = mVendorCamera->getFwkStreamHal3BufMgr()->getBuffer(
                buffer_reqs[i].stream, buffer_reqs[i].num_buffers_requested);

            if (buffers.empty()) {
                finalStatus = CAMERA3_BUF_REQ_FAILED_PARTIAL;
                MLOGW(LogGroupHAL, "[Hal3BufferMgr]: request %d buffers failed for stream: %p",
                      buffer_reqs[i].num_buffers_requested, buffer_reqs[i].stream);
            }

            auto &ret = returned_buf_reqs[i];
            ret.stream = buffer_reqs[i].stream;
            ret.status =
                buffers.empty() ? CAMERA3_PS_BUF_REQ_NO_BUFFER_AVAILABLE : CAMERA3_PS_BUF_REQ_OK;
            ret.num_output_buffers = buffers.size();
            for (size_t j = 0; j < buffers.size(); ++j) {
                ret.output_buffers[j] = buffers[j];
            }
        }
    }

    return finalStatus;
}
int AlgoSession::createDarkroom()
{
    auto configToDarkroom = mCameraMode->buildConfigToDarkroom();

    std::string sig;
    auto entry = configToDarkroom->getMeta()->find(MI_MIVI_POST_SESSION_SIG);
    if (entry.count)
        sig = reinterpret_cast<const char *>(entry.data.u8);
    MLOGD(LogGroupHAL, "[PostProcess][Config][KEY]: first snapshot comes, async create a mialgo:%s",
          sig.c_str());

    darkRoomEnqueue([sig, configToDarkroom] {
        DARKROOM.configureStreams(
            sig, configToDarkroom, nullptr, nullptr, [](const std::string &sessionName) -> void {
                MialgoTaskMonitor::getInstance()->deleteMialgoTaskSig(sessionName);
            });
    });

    return 0;
}

int AlgoSession::darkRoomEnqueue(std::function<void()> task)
{
    return mDarkRoomThread->enqueue(task);
}

void AlgoSession::returnStreamBuffers(uint32_t num_buffers,
                                      const camera3_stream_buffer *const *buffers)
{
    MLOGI(LogGroupHAL, "[Hal3BufferMgr]: return %d buffers for", num_buffers);

    std::ostringstream str;
    std::vector<camera3_stream_buffer> retFwkBufs;

    for (int i = 0; i < num_buffers; i++) {
        // MLOGD(LogGroupHAL, "return buffer for stream:%p", buffers[i]->stream);
        str << " " << buffers[i]->stream;
        auto mihalStream = Stream::toRichStream(buffers[i]->stream);
        MASSERT(mihalStream);

        if (mihalStream->isInternal()) {
            mihalStream->returnStreamBuffer(*buffers[i]);
        } else {
            // NOTE: since return fwk buffers is IPC, we can collect all buffers and return them
            // once to reduce IPC num
            retFwkBufs.push_back(*buffers[i]);
        }
    }

    mVendorCamera->getFwkStreamHal3BufMgr()->returnStreamBuffers(retFwkBufs);

    str << std::endl;
    MLOGD(LogGroupHAL, "[Hal3BufferMgr]: %s", str.str().c_str());
}

int32_t AlgoSession::getMaxJpegSize() const
{
    int32_t maxSize = 0;
    camera_metadata_ro_entry entry = mVendorCamera->findInStaticInfo(ANDROID_JPEG_MAX_SIZE);
    if (entry.count) {
        maxSize = entry.data.i32[0];
    }
    return maxSize;
}

std::atomic<int> AsyncAlgoSession::sAsyncAlgoSessionCnt = 0;

std::shared_ptr<AsyncAlgoSession> AsyncAlgoSession::create(VendorCamera *vendorCamera,
                                                           std::shared_ptr<StreamConfig> config)
{
    auto asyncSession =
        std::shared_ptr<AsyncAlgoSession>{new AsyncAlgoSession{vendorCamera, config}};

    return asyncSession;
}

AsyncAlgoSession::AsyncAlgoSession(VendorCamera *vendorCamera, std::shared_ptr<StreamConfig> config)
    : AlgoSession{vendorCamera, AsyncAlgoSessionType},
      mExitThread{false},
      mLatestSnapVndFrame{UINT32_MAX},
      mSupportDownCapture{false},
      mVendorCallbacks{
          // camera3_callback_ops
          {processResult_, notify_, requestStreamBuffers_, returnStreamBuffers_},
          // processResultFunc
          [this](const camera3_capture_result *result) {
              // NOTE: add some debug log
              for (int i = 0; i < result->num_output_buffers; ++i) {
                  std::ostringstream str;
                  auto buffer = result->output_buffers[i];
                  // NOTE: if buffer.status is CAMERA3_BUFFER_STATUS_ERROR, continue
                  if (buffer.status == CAMERA3_BUFFER_STATUS_ERROR)
                      continue;

                  str << "[RT] result contains buffer: stream:0x" << buffer.stream << ", ";

                  str << "buffer_handle_t*:0x";
                  if (buffer.buffer != nullptr) {
                      str << buffer.buffer;
                  } else {
                      str << "NULL";
                  }
                  str << ", ";

                  str << "native_handle:0x";
                  if (buffer.buffer != nullptr && *buffer.buffer != nullptr) {
                      str << *buffer.buffer;
                  } else {
                      str << "NULL";
                  }
                  str << ", ";

                  str << "status:" << buffer.status << ", ";
                  str << "acquire_fence:" << buffer.acquire_fence << ", ";
                  str << "release_fence:" << buffer.release_fence;

                  MLOGD(LogGroupHAL, "%s", str.str().c_str());
              }

              RemoteResult resultWapper{mVendorCamera, result};
              processResult(&resultWapper);
          },
          // notifyFunc
          [this](const camera3_notify_msg *msg) {
              NotifyMessage msgWrapper{mVendorCamera, msg};
              notify(&msgWrapper);
          },
          // requestStreamBuffersFunc
          [this](uint32_t num_buffer_reqs, const camera3_buffer_request *buffer_reqs,
                 /*out*/ uint32_t *num_returned_buf_reqs,
                 /*out*/ camera3_stream_buffer_ret *returned_buf_reqs) {
              return requestStreamBuffers(num_buffer_reqs, buffer_reqs, num_returned_buf_reqs,
                                          returned_buf_reqs);
          },
          // returnStreamBuffersFunc
          [this](uint32_t num_buffers, const camera3_stream_buffer *const *buffers) {
              returnStreamBuffers(num_buffers, buffers);
          },
      },
      mTotalJpegBurstNum{0},
      mCurrentJpegBurstNum{0}
{
    mConfigFromFwk = std::make_shared<LocalConfig>(*config);
    if (sAsyncAlgoSessionCnt == 0) {
        MiDebugInterface::getInstance()->startOfflineLogger();
    }
    sAsyncAlgoSessionCnt++;

    mCameraMode = createCameraMode(mConfigFromFwk.get());
    if (!mCameraMode)
        return;

    mConfigToVendor = mCameraMode->buildConfigToVendor();
    if (!mConfigToVendor)
        return;

    mAnchorFrameEnabled = vendorCamera->isSupportAnchorFrame();

    MiviSettings::getVal("supportDownCapture", mSupportDownCapture, false);

    MLLOGV(LogGroupHAL, mConfigToVendor->toString(),
           " for camera %s, anchorframeEnabled: %d, finalized config to vendor:",
           vendorCamera->getId().c_str(), mAnchorFrameEnabled);

    mInited = true;

    auto entry = mConfigFromFwk->getMeta()->find(MI_MIVI_BGSERVICE_CLIENTID);
    if (entry.count) {
        mClientId = entry.data.i32[0];
        MLOGI(LogGroupHAL, "mClientId: %d", mClientId);
        BGServiceWrap::getInstance()->setClientId(mClientId);
    }
}

AsyncAlgoSession::~AsyncAlgoSession()
{
    // TODO: handle the exit of the thread
    if (mDarkroomThread.joinable()) {
        std::unique_lock<std::recursive_mutex> locker(mMutex);
        mExitThread = true;
        locker.unlock();

        mCond.notify_one();
        mDarkroomThread.join();
    }

    --sAsyncAlgoSessionCnt;
    if (sAsyncAlgoSessionCnt < 0) {
        sAsyncAlgoSessionCnt = 0;
    }
    MLOGI(LogGroupHAL, "[Statistic] async session 0x%x destroyed, remains %d sessions", getId(),
          sAsyncAlgoSessionCnt.load());

    if (sAsyncAlgoSessionCnt == 0) {
        MiDebugInterface::getInstance()->stopOfflineLogger();
    }
}

void AsyncAlgoSession::endConfig()
{
    mCameraMode->updateFwkOverridedStream();

    // NOTE: enlarge the QR Scan stream max buffer size to 16
    if (mVendorCamera->supportBufferManagerAPI()) {
        mConfigToVendor->forEachStream([](std::shared_ptr<Stream> stream) {
            if (stream->isInternal())
                return 0;

            if (stream->isRealtime())
                stream->setMaxBuffers(stream->getMaxBuffers() + 8);

            return 0;
        });
    }
}

int AsyncAlgoSession::processRequest(std::shared_ptr<CaptureRequest> remoteRequest)
{
    std::shared_ptr<LocalRequest> request =
        std::make_shared<LocalRequest>(remoteRequest->getCamera(), remoteRequest->toRaw());
    if (!mFirstFrameRequested) {
        mFirstFrameRequested = true;
        MLOGI(LogGroupHAL, "[Performance] session 0x%x request first frame %d", getId(),
              request->getFrameNumber());
    }

    if (request->isSnapshot()) {
        if (!mClientId) {
            mClientId = BGServiceWrap::getInstance()->getClientId();
        }

        MLLOGI(LogGroupHAL, request->toString(),
               "[fwkFrame:%u][ProcessRequest][KEY]: snapshot request coming: ([Promise]: "
               "imageName *.jpg complete && vendor job done && mock job done)",
               request->getFrameNumber());
    } else {
        MLOGD(LogGroupRT, "[fwkFrame:%u]: preview request coming:\n\t%s", request->getFrameNumber(),
              request->toString().c_str());
    }

    return processVendorRequest(request);
}

int AsyncAlgoSession::flush()
{
    int pendingVendorCount = getPendingVendorSnapCount();
    int pendingFinalCount = getPendingFinalSnapCount();
    MLOGD(LogGroupHAL, "%s[Flush][Statistic]: get pending snaphsot count vendor:%d, mihal:%d",
          getLogPrefix().c_str(), pendingVendorCount, pendingFinalCount);

    if (pendingVendorCount > 0) {
        std::unique_lock<std::mutex> locker{mSnapMutex};

        int ret = mSnapCond.wait_for(locker, mCameraMode->mWaitFlushMillisecs,
                                     [this]() { return !getPendingVendorSnapCount(); });
        if (!ret) {
            MLOGW(LogGroupHAL,
                  "%s[Flush][KEY]: wait for vendor hal return snapshot result time out!",
                  getLogPrefix().c_str());
        }
    }
    mFlush = true;
    MLOGI(LogGroupHAL, "%s[Flush][KEY]: start flush vendor, [Promise]: finish flush vendor",
          getLogPrefix().c_str());
    int ret = mVendorCamera->flush2Vendor();
    MLOGI(LogGroupHAL,
          "%s[Flush][KEY]: [Fulfill]: finish flush vendor:ret=%d, now flush mihal pending "
          "requests...",
          getLogPrefix().c_str(), ret);

    flushPendingFwkRecords();

    return ret;
}

int AsyncAlgoSession::resetResource()
{
    mCameraMode->setRetired();

    return mConfigFromFwk->forEachStream([](std::shared_ptr<Stream> stream) {
        stream->setRetired();
        return 0;
    });
}

int AsyncAlgoSession::processEarlySnapshotResult(uint32_t frame,
                                                 std::shared_ptr<StreamBuffer> inputBuffer,
                                                 bool ignore)
{
    auto executor = getExecutor(frame, ignore);
    if (executor == nullptr) {
        return 0;
    }
    auto ret = executor->processEarlyResult(inputBuffer);

    int32_t *rmFrames = new int32_t[2];
    if (executor->isFinish(frame, rmFrames)) {
        for (int i = 0; i < 2; i++) {
            if (*(rmFrames + i) != -1)
                removeFromVendorInflights(*(rmFrames + i));
        }
    }
    delete[] rmFrames;

    return ret;
}

bool AsyncAlgoSession::hasPendingPhotographers() const
{
    std::lock_guard<std::recursive_mutex> locker{mMutex};
    bool hasPendingPhotographers = false;

    for (auto &p : mVendorInflights) {
        // NOTE: we will overlook previewer since we only care about snapshot
        // XXX: actually, when session is put into offline list, then it shouldn't contain previewer
        // since before unregister session, app will flush mihal after which all previewers should
        // finish processing
        if (p.second->isPreviewExecutor()) {
            continue;
        }

        auto phgExecutor = p.second;

        MLOGD(LogGroupHAL,
              "session 0x%x at least has phg[fwkFrame:%u] pending, print its status: hasCb: %d,",
              getId(), phgExecutor->getId(),
              BGServiceWrap::getInstance()->hasEeventCallback(mClientId));
        hasPendingPhotographers = true;
        break;
    }

    if (!hasPendingPhotographers) {
        hasPendingPhotographers = (mPendingMiHALSnapCount.load() > 0);
    }

    if (!hasPendingPhotographers) {
        MLOGD(LogGroupHAL, "session 0x%x has no pending photographers", getId());
    }

    return hasPendingPhotographers;
}

void AsyncAlgoSession::flushExecutor()
{
    std::lock_guard<std::recursive_mutex> locker{mMutex};
    for (auto &p : mVendorInflights) {
        auto executor = p.second;
        if (executor && (p.first == executor->getVendorRequestList()[0])) {
            executor->signalFlush();
        }
    }
}

StreamBuffer *AsyncAlgoSession::getFwkSnapshotBuffer(CaptureRequest *request)
{
    StreamBuffer *streamBuffer = nullptr;
    for (int i = 0; i < request->getOutBufferNum(); i++) {
        auto sb = request->getStreamBuffer(i);
        auto stream = sb->getStream();
        if (!stream->isInternal() && sb->isSnapshot()) {
            streamBuffer = sb.get();
            break;
        }
    }

    return streamBuffer;
}

StreamBuffer *AsyncAlgoSession::getScanBuffer(CaptureRequest *request)
{
    StreamBuffer *streamBuffer = nullptr;
    for (int i = 0; i < request->getOutBufferNum(); i++) {
        auto sb = request->getStreamBuffer(i);
        auto stream = sb->getStream();
        if (stream->isRealtime()) {
            if (stream->isPreview())
                continue;

            if (stream->isAuxPreview())
                continue;

            streamBuffer = sb.get();
            break;
        }
    }

    return streamBuffer;
}

std::shared_ptr<AsyncAlgoSession::RequestExecutor> AsyncAlgoSession::createExecutor(
    std::shared_ptr<CaptureRequest> fwkRequest)
{
    std::shared_ptr<RequestExecutor> executor;

    LocalRequest *lr = reinterpret_cast<LocalRequest *>(fwkRequest.get());

    float UIZoomRatio = mCameraMode->getUIZoomRatio();
    if (mCameraMode->isSupportFovCrop()) {
        float QcZoomRatio = UIZoomRatio * mCameraMode->getFovCropZoomRatio();
        MLOGI(LogGroupRT, "Update request zoomRatio %0.2fX->%0.2fX", UIZoomRatio, QcZoomRatio);
        lr->updateMeta(ANDROID_CONTROL_ZOOM_RATIO, &QcZoomRatio, 1);
    }

    if (fwkRequest->isSnapshot()) {
        insertPendingSnapRecords(fwkRequest->getFrameNumber());
        // Reset the initialization of some tags to prevent exceptions caused by incorrect app
        // settings.
        mCameraMode->applyDefaultSettingsForSnapshot(fwkRequest);
        executor = mCameraMode->createPhotographer(fwkRequest);
    } else {
        executor = std::make_shared<Previewer>(fwkRequest, this, mCameraMode->isHalLowCapability());
    }

    return executor;
}

AsyncAlgoSession::FwkRequestRecord::FwkRequestRecord(std::shared_ptr<CaptureRequest> request)
    : fwkRequest{request}, partialResult{0}, notifyDone(false)
{
    for (int i = 0; i < request->getOutBufferNum(); i++) {
        auto streamBuffer = request->getStreamBuffer(i);
        camera3_stream *raw = streamBuffer->getStream()->toRaw();
        pendingStreams.insert(raw);
    }
}

int AsyncAlgoSession::FwkRequestRecord::updateByResult(const CaptureResult *result)
{
    if (result->getPartial() > partialResult)
        partialResult = result->getPartial();

    for (int i = 0; i < result->getBufferNumber(); i++) {
        auto streamBuffer = result->getStreamBuffer(i);
        camera3_stream *raw = streamBuffer->getStream()->toRaw();
        if (!pendingStreams.erase(raw)) {
            MLOGE(LogGroupHAL, "stream 0x%p not found in original request:%d", raw,
                  result->getFrameNumber());
        }
    }

    return 0;
}

int AsyncAlgoSession::FwkRequestRecord::updateByNotify(const NotifyMessage *msg)
{
    if (!msg->isErrorMsg()) {
        notifyDone = true;
        return 0;
    }

    MLOGD(LogGroupRT, "error message: %s", msg->toString().c_str());

    int error = msg->getErrorCode();
    // NOTE: for CAMERA3_MSG_ERROR_BUFFER, we can not erase the pending, since there must be
    //       a coupled error buffer result issued from vendor, then that result will erase
    if (error == CAMERA3_MSG_ERROR_RESULT || error == CAMERA3_MSG_ERROR_REQUEST) {
        partialResult = fwkRequest->getCamera()->getPartialResultCount();
    }

    if (error != CAMERA3_MSG_ERROR_BUFFER) {
        notifyDone = true;
    }

    return 0;
}

int AsyncAlgoSession::FwkRequestRecord::flush()
{
    std::vector<std::shared_ptr<StreamBuffer>> buffers;
    for (int i = 0; i < fwkRequest->getOutBufferNum(); i++) {
        auto streamBuffer = fwkRequest->getStreamBuffer(i);
        camera3_stream *raw = streamBuffer->getStream()->toRaw();
        if (!pendingStreams.count(raw))
            continue;

        streamBuffer->setErrorBuffer();
        buffers.push_back(streamBuffer);
        NotifyMessage msg{fwkRequest->getCamera(), fwkRequest->getFrameNumber(), raw};
        MLOGI(LogGroupHAL, "new a buffer error message: %s", msg.toString().c_str());
        msg.notify();
    }

    if (partialResult < fwkRequest->getCamera()->getPartialResultCount()) {
        NotifyMessage msg{fwkRequest->getCamera(), fwkRequest->getFrameNumber(),
                          CAMERA3_MSG_ERROR_RESULT};
        MLOGI(LogGroupHAL, "new a meta error message: %s", msg.toString().c_str());
        msg.notify();
    }

    if (buffers.size()) {
        LocalResult lr{fwkRequest->getCamera(), fwkRequest->getFrameNumber(), 0, buffers};
        MLOGI(LogGroupHAL, "return a error result for incomplete: %s", lr.toString().c_str());
        fwkRequest->getCamera()->processCaptureResult(&lr);
    }

    return 0;
}

bool AsyncAlgoSession::FwkRequestRecord::isCompleted() const
{
    return !pendingStreams.size() && notifyDone &&
           partialResult == fwkRequest->getCamera()->getPartialResultCount();
}

void AsyncAlgoSession::darkroomRequestLoop() {}

void AsyncAlgoSession::setReqErrorAndForwardResult(std::shared_ptr<CaptureRequest> request)
{
    NotifyMessage msg{request->getCamera(), request->getFrameNumber(), CAMERA3_MSG_ERROR_REQUEST};
    MLOGI(LogGroupHAL, "notify for unsubmitted: %s", msg.toString().c_str());
    msg.notify();

    std::vector<std::shared_ptr<StreamBuffer>> buffers;
    for (int i = 0; i < request->getOutBufferNum(); i++) {
        auto streamBuffer = request->getStreamBuffer(i);
        streamBuffer->setErrorBuffer();
        buffers.push_back(streamBuffer);
    }

    LocalResult lr{request->getCamera(), request->getFrameNumber(), 0, buffers};
    MLOGI(LogGroupHAL, "error result for unsubmitted: %s", lr.toString().c_str());
    request->getCamera()->processCaptureResult(&lr);
}

int AsyncAlgoSession::processVendorRequest(std::shared_ptr<CaptureRequest> request)
{
    // NOTE: if mFlush is true , then it means this session is in flush state and we just
    // sent error result to fwk
    // WARNING: capture requests shouldn't come in burst, otherwise, only one request can acquire
    // lock and others will be treated as error
    if (mFlush) {
        MLOGI(LogGroupHAL, "session 0x%x is in flush, set fwkframe:%u request as error.", getId(),
              request->getFrameNumber());
        setReqErrorAndForwardResult(request);
        return 0;
    }

    if (!request->getMeta()->isEmpty()) {
        mCameraMode->updateSettings(request.get());
        updateSupportDownCapture(request.get());
    }

    {
        std::unique_lock<std::recursive_mutex> locker{mMutex};
        if (!request->isSnapshot() && !mVendorInflights.empty()) {
            auto executor = mVendorInflights.rbegin()->second;
            if (executor->canAttachFwkRequest(request)) {
                locker.unlock();
                {
                    std::lock_guard<std::mutex> lk(mAttMutex);
                    mAttachVendorFrame[request->getFrameNumber()] =
                        executor->getVendorRequestList()[0];
                }

                insertPendingFwkRecords(request);
                executor->attachFwkRequest(request);

                int32_t *rmFrames = new int32_t[2];
                if (executor->isFinish(executor->getVendorRequestList()[0], rmFrames)) {
                    for (int i = 0; i < 2; i++) {
                        if (*(rmFrames + i) != -1)
                            removeFromVendorInflights(*(rmFrames + i));
                    }
                }
                delete[] rmFrames;

                return 0;
            }
        }
    }
    insertPendingFwkRecords(request);

    auto executor = createExecutor(request);
    if (mSupportDownCapture && executor->getName() == "BurstPhotographer" &&
        mLatestSnapVndFrame != UINT32_MAX) {
        auto lastSnapExecutor = getExecutor(mLatestSnapVndFrame, true);
        if (lastSnapExecutor != nullptr) {
            std::shared_ptr<Photographer> pht =
                std::static_pointer_cast<Photographer>(lastSnapExecutor);
            if (pht->getName() != "BurstPhotographer" && !pht->isDropped()) {
                pht->enableDrop();
                MLOGI(LogGroupHAL, "[DropFrame] %s vndFrame:%u", pht->getLogPrefix().c_str(),
                      mLatestSnapVndFrame);
            }
        }
    }

    addToVendorInflights(executor);
    if (request->isSnapshot()) {
        {
            std::lock_guard<std::mutex> lk(mAttMutex);
            mAttachVendorFrame[request->getFrameNumber()] = executor->getVendorRequestList()[0];
        }
        {
            std::shared_ptr<Photographer> pht = std::static_pointer_cast<Photographer>(executor);
            if (pht->getPhtgphType() == Photographer::Photographer_VENDOR_MFNR ||
                (pht->getPhtgphType() == Photographer::Photographer_HDR && pht->isMFNRHDR()))
                addPendingVendorMFNR();
        }

        mCameraMode->onMihalBufferChanged();
        std::vector<uint32_t /* vnd FrameNumber */> frames = executor->getVendorRequestList();
        mLatestSnapVndFrame = frames[0];

        // NOTE: burst snapshots' early pic should be set as error
        if (executor->getName() != "BurstPhotographer") {
            MLOGI(LogGroupHAL, "%s[EarlyPic]: add phg's corresponding vndFrame:%u to snapshot list",
                  executor->getLogPrefix().c_str(), frames[0]);
            if (!executor->doAnchorFrameAsEarlyPicture()) {
                MLOGI(LogGroupHAL,
                      "%s[EarlyPic]: Use Mihal anchor frame, [Promise]:Notify mihal EarlyPic.",
                      executor->getLogPrefix().c_str());
            }
        } else {
            float zoomRatio = mCameraMode->getUIZoomRatio();
            if (mTotalJpegBurstNum == 0) {
                // TODO: refactor this ugly code later
                if (zoomRatio < 1.0f) {
                    mTotalJpegBurstNum = 30;
                } else {
                    mTotalJpegBurstNum = 50;
                }
            }
            mCurrentJpegBurstNum++;
            if (mCurrentJpegBurstNum > mTotalJpegBurstNum) {
                std::shared_ptr<Photographer> pht =
                    std::static_pointer_cast<Photographer>(executor);
                pht->enableDrop();
                MLOGI(LogGroupHAL, "[DropFrame] %s vndFrame:%u", pht->getLogPrefix().c_str(),
                      frames[0]);
            }
        }
        PsiMemMonitor::getInstance()->snapshotAdd();
    } else {
        mCurrentJpegBurstNum = 0;
        mTotalJpegBurstNum = 0;
    }

    return executor->execute();
}

std::unique_ptr<CameraMode> AsyncAlgoSession::createCameraMode(const StreamConfig *configFromFwk)
{
    uint32_t roleId = mVendorCamera->getRoleId();
    uint32_t operationMode = configFromFwk->getOpMode();
    MLOGI(LogGroupHAL, "opmode 0x%x, roleId %d", operationMode, roleId);

    return std::make_unique<CameraMode>(this, mVendorCamera, configFromFwk);
}

SessionManager *SessionManager::getInstance()
{
    static SessionManager mgr{};

    return &mgr;
}

SessionManager::SessionManager() : mExitReclaimThread{false}
{
    EventCallbackMgr::getInstance()->registerEventCB(onClientEvent);
    mReclaimThread = std::thread{[this]() { sessionReclaimLoop(); }};
    mDarkRoomClearThread = std::make_unique<ThreadPool>(DARKROOMCLEARTHREADNUM);
}

SessionManager::~SessionManager()
{
    MLOGI(LogGroupHAL, "[SessionManager]: destruct E, [Promise]: destruct X");

    std::unique_lock<std::mutex> locker(mMutex);
    mExitReclaimThread = true;
    locker.unlock();

    mReclaimCond.notify_one();
    mReclaimThread.join();

    MLOGI(LogGroupHAL, "[SessionManager]: [Fulfill]: destruct X");
}

void SessionManager::onClientEvent(EventData data)
{
    MLOGI(LogGroupHAL, "type: %d, clientId: %d", data.type, data.clientId);
    if (data.type == EventType::ClientDied) {
        SessionManager::getInstance()->flushSessionExecutor(data.clientId);
    }
}

void SessionManager::flushSessionExecutor(int32_t clientId)
{
    DARKROOM.notifyBGServiceHasDied(clientId);

    std::unique_lock<std::mutex> locker(mMutex);
    for (auto &session : mActiveSessions) {
        if (session.second->getClientId() == clientId)
            session.second->flushExecutor();
    }

    for (auto &session : mOfflineSessions) {
        if (session.second->getClientId() == clientId)
            session.second->flushExecutor();
    }
}

void SessionManager::sessionReclaimLoop()
{
    std::unique_lock<std::mutex> locker(mMutex);
    prctl(PR_SET_NAME, "MiHalSMLoop");
    while (true) {
        uint32_t sessionId = 0;
        mReclaimCond.wait(locker, [this, &sessionId]() {
            if (mExitReclaimThread) {
                return true;
            }

            for (auto itr = mOfflineSessions.begin(); itr != mOfflineSessions.end(); itr++) {
                // NOTE: if offline session has pending in-process executor, we shouldn't delete
                // this session
                if (!itr->second->hasPendingPhotographers()) {
                    sessionId = itr->first;
                    return true;
                }
            }

            return false;
        });

        if (mExitReclaimThread) {
            MLOGI(LogGroupHAL, "[SessionManager]: reclaim thread exit");
            return;
        }

        auto itr = mOfflineSessions.find(sessionId);

        // NOTE: now this offline session can be safely deleted
        auto logString = itr->second->getLogPrefix();

        // NOTE: since delete session is time consuming, we temporarily move the session's
        // shared_ptr into a temp variable and delete it with mutex unlocked
        auto tempPtr = std::move(itr->second);
        mOfflineSessions.erase(itr);

        auto remainSessionNum = mActiveSessions.size() + mOfflineSessions.size();
        locker.unlock();
        MLOGI(LogGroupHAL, "[SessionManager]: delete %s E, [Promise]: delete %s X",
              logString.c_str(), logString.c_str());
        tempPtr = nullptr;

        if (!remainSessionNum) {
            // NOTE: Offline DebugData
            SetSessionStatus(false);

            mDarkRoomClearThread->enqueue([]() { DARKROOM.clear(); });
        }

        locker.lock();
        MLOGI(LogGroupHAL,
              "[SessionManager]: [Fulfill]: delete %s X, after delete, now %zu:%zu sessions in "
              "active:offline registries",
              logString.c_str(), mActiveSessions.size(), mOfflineSessions.size());
    }
}

void SessionManager::signalReclaimSession()
{
    std::unique_lock<std::mutex> locker(mMutex);
    mReclaimCond.notify_one();
}

void SessionManager::signalReclaimSessionWithoutLock()
{
    mReclaimCond.notify_one();
}

int SessionManager::registerSession(std::shared_ptr<AsyncAlgoSession> session)
{
    std::unique_lock<std::mutex> locker(mMutex);

    uint32_t id = session->getId();
    mActiveSessions.insert({id, session});
    locker.unlock();

    mCond.notify_one();
    MLOGI(LogGroupHAL, "[SessionManager][Statistic]: session:0x%x registered, total:%zu", id,
          mActiveSessions.size());

    return 0;
}

int SessionManager::unregisterSession(std::shared_ptr<AsyncAlgoSession> session)
{
    if (!session)
        return 0;

    // NOTE: first, do some clean up: unregister fwk streams
    session->resetResource();

    std::lock_guard<std::mutex> locker(mMutex);
    uint32_t id = session->getId();
    if (!mActiveSessions.count(id)) {
        MLOGW(LogGroupHAL, "[SessionManager]: no registered session:0x%x", id);
        return -ENOENT;
    }

    mActiveSessions.erase(id);
    mOfflineSessions.insert({id, session});
    MLOGI(LogGroupHAL, "[SessionManager]: move session:0x%x to offline list", id);

    signalReclaimSessionWithoutLock();

    return 0;
}

std::shared_ptr<AsyncAlgoSession> SessionManager::getSession(uint32_t sessionId) const
{
    std::unique_lock<std::mutex> locker(mMutex);

    bool ret = mCond.wait_for(
        locker, 500ms, [this]() { return mActiveSessions.size() || mOfflineSessions.size(); });
    if (!ret) {
        MLOGE(LogGroupHAL, "[SessionManager]: none sessions found");
        return nullptr;
    }

    std::shared_ptr<AsyncAlgoSession> session = nullptr;
    if (sessionId == -1 && mActiveSessions.size()) {
        // defaultly get the latest session
        session = mActiveSessions.rbegin()->second;
    } else {
        auto it = mActiveSessions.find(sessionId);
        if (it != mActiveSessions.end())
            session = it->second;

        if (!session) {
            auto it = mOfflineSessions.find(sessionId);
            if (it != mOfflineSessions.end())
                session = it->second;
        }
    }

    MLOGI(LogGroupHAL, "[SessionManager]: get session %s by sessionId:0x%x",
          session ? "success" : "failed", sessionId);

    return session;
}

SyncAlgoSession::SyncAlgoSession(VendorCamera *vendorCamera, std::shared_ptr<StreamConfig> config,
                                 std::shared_ptr<EcologyAdapter> ecoAdapter)
    : AlgoSession{vendorCamera, SyncAlgoSessionType},
      mVendorCallbacks{
          {processResult_, notify_, requestStreamBuffers_, returnStreamBuffers_},
          [this](const camera3_capture_result *result) {
              RemoteResult resultWapper{mVendorCamera, result};
              processResult(&resultWapper);
          },
          [this](const camera3_notify_msg *msg) {
              NotifyMessage msgWrapper{mVendorCamera, msg};
              notify(&msgWrapper);
          },
          // requestStreamBuffersFunc
          [this](uint32_t num_buffer_reqs, const camera3_buffer_request *buffer_reqs,
                 /*out*/ uint32_t *num_returned_buf_reqs,
                 /*out*/ camera3_stream_buffer_ret *returned_buf_reqs) {
              return requestStreamBuffers(num_buffer_reqs, buffer_reqs, num_returned_buf_reqs,
                                          returned_buf_reqs);
          },
          // returnStreamBuffersFunc
          [this](uint32_t num_buffers, const camera3_stream_buffer *const *buffers) {
              returnStreamBuffers(num_buffers, buffers);
          },
      },
      mEcologyAdapter(ecoAdapter)
{
    MiDebugInterface::getInstance()->startOfflineLogger();

    if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == config->toRaw()->streams[0]->format) {
        mConfigFromFwk = std::make_shared<LocalConfig>(*config);
    } else {
        // sort the streams
        std::vector<std::shared_ptr<Stream>> streams;
        // this is take a seat for preview stream
        streams.push_back(nullptr);
        auto raw = config->toRaw();
        for (int i = 0; i < config->getStreamNums(); i++) {
            auto stream = RemoteStream::create(vendorCamera, raw->streams[i]);
            if (raw->streams[i]->format != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
                stream->setStreamProp(Stream::SNAPSHOT);
                streams.push_back(stream);
            } else {
                stream->setStreamProp(Stream::PRIMARY_PREVIEW);
                streams[0] = stream;
            }
        }

        // Metadata meta{config->toRaw()->session_parameters, Metadata::CopyNow};
        Metadata meta{*config->getMeta()};
        mConfigFromFwk = std::make_shared<LocalConfig>(streams, config->getOpMode(), meta);
    }

    mCameraMode = createCameraMode(mConfigFromFwk.get());
    if (!mCameraMode)
        return;

    mConfigToVendor = mCameraMode->buildConfigToVendor();
    if (!mConfigToVendor)
        return;

    MLOGI(LogGroupHAL, " for camera %s, finalized config to vendor:\n\t%s",
          vendorCamera->getId().c_str(), mConfigToVendor->toString().c_str());

    mInited = true;
}

SyncAlgoSession::~SyncAlgoSession()
{
    MLOGI(LogGroupHAL, "session 0x%x destroyed", getId());
    MiDebugInterface::getInstance()->stopOfflineLogger();
}

int SyncAlgoSession::processRequest(std::shared_ptr<CaptureRequest> remoteRequest)
{
    std::shared_ptr<LocalRequest> request =
        std::make_shared<LocalRequest>(remoteRequest->getCamera(), remoteRequest->toRaw());

    insertPendingFwkRecords(request);
    std::shared_ptr<RequestExecutor> executor;

    StreamBuffer *snapshotBuffer = nullptr;
    for (int i = 0; i < request->getOutBufferNum(); i++) {
        if (request->getStreamBuffer(i)->isSnapshot()) {
            snapshotBuffer = request->getStreamBuffer(i).get();
            break;
        }
    }

    if (snapshotBuffer != nullptr) {
        MLLOGI(LogGroupHAL, request->toString(), "framework snapshot request coming:");
        mEcologyAdapter->buildRequest(request, true, mVendorCamera->getRoleId());
        mCameraMode->updateSettings(request.get());
        if (nullptr == snapshotBuffer->getBuffer()->getHandle() &&
            mVendorCamera->supportBufferManagerAPI()) {
            auto buffers = mVendorCamera->getFwkStreamHal3BufMgr()->getBuffer(
                snapshotBuffer->getStream()->toRaw(), 1);
            if (buffers.empty()) {
                MLOGE(LogGroupHAL, "request buffer from stream: %p FAIL",
                      snapshotBuffer->getStream()->toRaw());
                MASSERT(false);
            } else {
                snapshotBuffer->setLatedBuffer(buffers[0]);
            }
        }
        executor = mCameraMode->createPhotographer(request);
        // NOTE: SDK scene doesn't need to attach preview
        executor->forceDonotAttachPreview();
    } else {
        MLOGV(LogGroupRT, "preview request coming:\n\t%s", request->toString().c_str());
        if (!request->getMeta()->isEmpty()) {
            mEcologyAdapter->buildRequest(request, false, mVendorCamera->getRoleId());
            mCameraMode->updateSettings(request.get());
        }
        executor = std::make_shared<Previewer>(request, this);
    }

    addToVendorInflights(executor);

    return executor->execute();
}

std::unique_ptr<CameraMode> SyncAlgoSession::createCameraMode(const StreamConfig *configFromFwk)
{
    return std::make_unique<SdkMode>(this, mVendorCamera, configFromFwk, mEcologyAdapter);
}

void SyncAlgoSession::endConfig()
{
    mCameraMode->updateFwkOverridedStream();
}

int SyncAlgoSession::flush()
{
    MLOGI(LogGroupHAL, "SyncAlgoSession session flush");
    int ret = mVendorCamera->flush2Vendor();
    MLOGI(LogGroupHAL, "SyncAlgoSession session flush compelete");
    // MLOGI(LogGroupHAL, "finish flush in vendor:ret=%d, now flush mihal pending requests...",
    // ret);

    DARKROOM.flush();
    flushPendingFwkRecords();
    return ret;
}

} // namespace mihal
