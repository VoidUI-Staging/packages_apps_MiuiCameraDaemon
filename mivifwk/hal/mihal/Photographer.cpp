#include "Photographer.h"

#include <chrono>
#include <sstream>

#include "Camera3Plus.h"
#include "CameraMode.h"
#include "ExifUtils.h"
#include "GraBuffer.h"
#include "JpegCompressor.h"
#include "LogUtil.h"
#include "MialgoTaskMonitor.h"
#include "MiviSettings.h"
#include "Stream.h"
#include "VendorCamera.h"

namespace mihal {

using namespace std::chrono_literals;
using namespace vendor::xiaomi::hardware::bgservice::implementation;

namespace {

std::set<uint32_t /* TAG */> forcedTags = {
    ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
    ANDROID_CONTROL_ZOOM_RATIO, // For fovCrop
    ANDROID_SCALER_CROP_REGION,
};

std::vector<const char * /* TAG */> forcedTagStrings = {
    // clang-format off
    MI_HDR_ENABLE,             MI_MFNR_ENABLE,
    MI_DEPURPLE_ENABLE,        MI_FLIP_ENABLE,
    MI_BOKEH_HDR_ENABLE,       MI_PORTRAIT_LIGHTING,
    MI_MULTIFRAME_INPUTNUM,    MI_SUPER_RESOLUTION_ENALBE,
    MI_CAPTURE_FUSION_IS_ON,   MI_HDR_SR_ENABLE,
    MI_SNAPSHOT_IMAGE_NAME,    MI_MIVI_SUPERNIGHT_MODE,
    MI_SUPERNIGHT_ENABLED,     MI_MIVI_NIGHT_MOTION_MODE,
    MI_RAWHDR_ENABLE,          MI_FAKESAT_ENABLED,
    MI_AI_ASD_ASD_EXIF_INFO,   MI_HEICSNAPSHOT_ENABLED,
    MI_BURST_MIALGO_TOTAL_NUM, MI_SNAPSHOT_SHOTTYPE
    // clang-format on
};

} // namespace

Photographer::Photographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                           const Metadata &meta, std::shared_ptr<StreamConfig> darkroomConfig)
    : RequestExecutor{fwkRequest, session},
      mExtraMetadata{meta},
      mDarkroomConfig{darkroomConfig},
      mFinalFinished{false},
      mQuickShotTimerDone{false},
      mFLush{false},
      mQuickShotDelayTime{0},
      mMasterPhyId{0xFF}
{
    mPhtgphType = Photographer_SIMPLE;
    mIsMiviAnchor = false;
    mQuickShotEnabled = false;
    mFakeQuickShotEnabled = false;
    mAnchorFrameQuickView = false;
    mDropEnable = false;
    mIsMFNRHDR = false;
    mStartedNum = 0;
    mAnchorCB = nullptr;
    mFakeFwkFrameNum = fwkRequest->getFrameNumber();
    mFakeFwkFrameNum = mFakeFwkFrameNum << 32 | session->getId();
    MICAM_TRACE_ASYNC_BEGIN_F(MialgoTraceCapture, 0, "MiCapture_%s", getImageName().c_str());
    mAttachedFwkReq.insert({fwkRequest->getFrameNumber(), fwkRequest});
    asyncConfigDarkroom();
    mAttachedCount.store(supportAttachFwkPreviewReq() ? mVendorRequests.size() : 1);
}

Photographer::Photographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                           std::shared_ptr<StreamConfig> darkroomConfig)
    : Photographer{fwkRequest, session, Metadata{}, darkroomConfig}
{
}

Photographer::Photographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                           std::shared_ptr<Stream> extraStream, Metadata extraMetadata,
                           std::shared_ptr<StreamConfig> darkroomConfig)
    : Photographer{fwkRequest, session, extraMetadata, darkroomConfig}
{
    uint32_t frame = getAndIncreaseFrameSeqToVendor(1);
    // TODO: to be compatible with fwkRequest which has preview streams
    std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
    if (mSession->getSessionType() != Session::SyncAlgoSessionType) {
        // Cut off the fwk preview stream buffer and insert the mihal internal preview stream buffer
        outBuffers.push_back(getInternalQuickviewBuffer(frame));
    }

    for (int i = 0; i < mFwkRequest->getOutBufferNum(); i++) {
        auto buffer = mFwkRequest->getStreamBuffer(i);
        if (buffer->getStream()->isConfiguredToHal()) {
            outBuffers.push_back(buffer);
        }
    }

    std::shared_ptr<StreamBuffer> extraBuffer = extraStream->requestBuffer();
    outBuffers.push_back(extraBuffer);

    // FIXME: here we could be able to avoid a meta copy
    auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame, outBuffers,
                                                  *fwkRequest->getMeta());

    auto record = std::make_unique<RequestRecord>(this, std::move(request));
    mVendorRequests.insert({frame, std::move(record)});
}

Photographer::~Photographer()
{
    onSnapCompleted(mFwkRequest->getFrameNumber(), getImageName(),
                    RequestExecutor::HAL_VENDOR_REPORT);
    if (!mQuickShotTimerDone.load() && mQuickShotEnabled) {
        mQuickShotTimer->cancel();
        onSnapCompleted(mFwkRequest->getFrameNumber(), getImageName(),
                        RequestExecutor::HAL_BUFFER_RESULT);
    }
    MLOGD(LogGroupHAL, "Destructor Photographer  fwkFrame:%d", mFwkRequest->getFrameNumber());
    MICAM_TRACE_ASYNC_END_F(MialgoTraceCapture, 0, "MiCapture_%s", getImageName().c_str());

    if (mSession->getPendingFinalSnapCount() <= 0) {
        SessionManager::getInstance()->signalReclaimSession();
    }

    mEarlypicTimer->cancel();
}

void Photographer::RequestRecord::initPendingSet()
{
    // set the bitsets for the requested stream buffers
    for (int i = 0; i < request->getOutBufferNum(); i++) {
        auto streamBuffer = request->getStreamBuffer(i);
        // TODO: FIXME: create the perfect rich Stream when creating with the config information
        auto stream = streamBuffer->getStream();
        uint32_t streamId = stream->getStreamId();
        pendingBuffers.set(streamId);
    }
    pendingMetaCount = request->getCamera()->getPartialResultCount();
    mStatus.store(0);
}

Photographer::RequestRecord::RequestRecord(Photographer *phtgph, const CaptureRequest *fwkRequest,
                                           uint32_t frameNumber)
    : isFullMeta{false},
      isErrorMsg{false},
      photographer{phtgph},
      darkroomOutNum{0},
      resultTs{-1},
      isReportError{false}
{
    request = std::make_unique<LocalRequest>(*fwkRequest);
    request->setFrameNumber(frameNumber);

    initPendingSet();
}

Photographer::RequestRecord::RequestRecord(Photographer *phtgph,
                                           std::unique_ptr<CaptureRequest> vendorRequest)
    : isFullMeta{false},
      isErrorMsg{false},
      isReportError{false},
      photographer{phtgph},
      darkroomOutNum{0},
      resultTs{-1}
{
    request = std::move(vendorRequest);
    initPendingSet();
}

int Photographer::execute()
{
    if (mVendorRequests.empty()) {
        MLOGE(LogGroupHAL, "no requets");
        return -ENOENT;
    }

    for (auto &pair : mVendorRequests) {
        CaptureRequest *request = pair.second->request.get();

        markReqBufsAsSentToHal(request);

        // WORKAROUND:NUWA-23328:fix "wait for vendor hal return snapshot result time out" when
        // bokeh hdr/md Ev0 without preview but Ev-, Ev-- with preview
        int enableBokehOptimize;
        MiviSettings::getVal("enableBokehOptimize", enableBokehOptimize, 0);
        if ((mPhtgphType == Photographer_BOKEH_MD || mPhtgphType == Photographer_BOKEH_HDR) &&
            !supportAttachFwkPreviewReq() && enableBokehOptimize) {
            resetHasSentQuickviewBufferToHal();
        }

        // Dump Meta for Debug Qcom process
        std::string dump = request->getMeta()->toStringByTags(forcedTags, &forcedTagStrings);
        MLLOGI(LogGroupRT, dump, "Vndframe %d request meta:\n", request->getFrameNumber());
        MLLOGI(LogGroupHAL, request->toString(), "mihal snapshot request coming:");

        VendorCamera *camera = reinterpret_cast<VendorCamera *>(request->getCamera());
        // FIXME: what if the vendor HAL returned a error during all the requests?
        camera->submitRequest2Vendor(request);
    }

    return 0;
}

void Photographer::signalFlush()
{
    MLOGI(LogGroupHAL, "[Photographer][Flush]fwkframe %d", mFwkRequest->getFrameNumber());
}

void Photographer::enableShotWithDealyTime(uint64_t delayTime)
{
    mQuickShotEnabled = true;
    mQuickShotDelayTime = delayTime;
    if (mQuickShotDelayTime > 0) {
        mQuickShotTimer = Timer::createTimer();
        std::weak_ptr<Photographer> weakThis{shared_from_this()};
        mQuickShotTimer->runAfter(mQuickShotDelayTime, [weakThis]() {
            std::shared_ptr<Photographer> that = weakThis.lock();
            if (!that) {
                return;
            }
            that->onSnapCompleted(that->mFwkRequest->getFrameNumber(), that->getImageName(),
                                  RequestExecutor::HAL_BUFFER_RESULT);
            that->setQuickShotTimerDone();
        });
    } else {
        // NOTE: if delaytime is 0, we do not need timer
        mQuickShotTimerDone = true;
    }
}

void Photographer::earlypicTimer()
{
    uint32_t delytime = 300;
    mEarlypicTimer->runAfter(delytime, [this]() {
        if (mEarlypicDone) {
            return;
        } else {
            if (doAnchorFrameAsEarlyPicture() && !mIsMiviAnchor) {
                MLOGW(LogGroupHAL,
                      "[EarlyPic] vendor AnchorFrame return too late, so Use mihal's anchor");
                uint32_t snapshotFrame = mVendorRequests.begin()->first;
                uint32_t anchorFrame = snapshotFrame - 1;
                mSession->anchorFrameEvent(0, anchorFrame, snapshotFrame);
            }
        }
    });
}

int Photographer::processVendorNotify(const NotifyMessage *msg)
{
    auto frame = msg->getFrameNumber();
    RequestRecord *record = getRequestRecord(frame);
    MASSERT(record);
    MLOGI(LogGroupHAL, "%s[Notify]: message from vendor:%s", getFrameLog(frame).c_str(),
          msg->toString().c_str());

    // Notify mock camera to send capture request in advance during burst shot
    if (!msg->isErrorMsg() && frame == mVendorRequests.begin()->first &&
        getSessionType() != Session::SyncAlgoSessionType) {
        if (mPhtgphType != Photographer_BURST) {
            if (doAnchorFrameAsEarlyPicture() && !mAnchored) {
                earlypicTimer();
            } else {
                // NOTE: if mihal return QuickView(EarlyPic),then we should return last preview
                // buffer right now
                auto snapshotFrame = frame;
                mSession->sendNotifyPreviewFrame(snapshotFrame);
                MLOGI(LogGroupHAL,
                      "%s[EarlyPic][Promise]:Notify mihal EarlyPic , use last priview buffer as "
                      "QuickView buffer.",
                      getFrameLog(frame).c_str());
            }
        } else {
            if (!isDropped() && !mInvite) {
                mInvite = true;
                bool invite = false;
                auto imageName = getImageName();
                auto frameNum = mFakeFwkFrameNum;

                uint32_t maxInviteSize = 0;
                MiviSettings::getVal("MockCameraMaxInviteThreshold", maxInviteSize, 6);
                if (mSession->getPendingFinalSnapCount() <= maxInviteSize) {
                    invite = true;
                }
                darkRoomEnqueue([frameNum, imageName, invite] {
                    DARKROOM.inviteForMockRequest(frameNum, imageName, invite);
                });
            }
        }
    }

    if (!msg->isErrorMsg()) {
        if ((mStartedNum == 0) && (mQuickShotDelayTime == 0) && mQuickShotEnabled) {
            onSnapCompleted(mFwkRequest->getFrameNumber(), getImageName(),
                            RequestExecutor::HAL_BUFFER_RESULT);
        }

        mStartedNum++;
        if (mStartedNum == mVendorRequests.size()) {
            onSnapCompleted(mFwkRequest->getFrameNumber(), getImageName(),
                            RequestExecutor::HAL_SHUTTER_NOTIFY);
            if (mFakeQuickShotEnabled) {
                onSnapCompleted(mFwkRequest->getFrameNumber(), getImageName(),
                                RequestExecutor::HAL_BUFFER_RESULT);
            }
        }
    }
    if (msg->getErrorCode() != CAMERA3_MSG_ERROR_BUFFER) {
        record->mStatus.store(record->mStatus.load() | NOTIFY_FINISH);
    }

    if (msg->getErrorCode() == CAMERA3_MSG_ERROR_RESULT ||
        msg->getErrorCode() == CAMERA3_MSG_ERROR_BUFFER) {
        record->isErrorMsg = true;
        // NOTE: If notify returns CAMERA3_MSG_ERROR_BUFFER, it won't affect meta return.
        // But if notify returns CAMERA3_MSG_ERROR_RESULT, remaining metas may not return.
        // According to the code annotation of camera3_capture_result in camera3.h:
        // 'If notify has been called with ERROR_RESULT, all further partial results for that
        // frame are ignored by the framework.'
        // So, vendor may chose not to return meta after returning error result msg.
        if (msg->getErrorCode() == CAMERA3_MSG_ERROR_RESULT) {
            record->pendingMetaCount = 0;
            MLOGW(LogGroupHAL, "%s[Notify]: error result msg, remaining metas may not return!!!",
                  getFrameLog(frame).c_str());

            // NOTE: If notify returns CAMERA3_MSG_ERROR_RESULT after all buffers have
            // returned.remaining metas may not return.the function of "processVendorResult" may not
            // execute.it need to execute the function of "complete" here to destruct the current
            // photographer.
            if (record->isReportError) {
                complete(frame);
                MLOGW(LogGroupHAL,
                      "%s[ProcessResult][KEY]: remaining metas may not return,need to execute "
                      "complete() here ",
                      getFrameLog(frame).c_str());
            }
        }
    }

    if (msg->toRaw()->type == CAMERA3_MSG_SHUTTER) {
        record->resultTs = (msg->getTimeStamp() > 0) ? (msg->getTimeStamp()) : -1;
    }

    // NOTE: for single-frame phg or mutiple-frame phg which doesn't support attach preview
    if (!supportAttachFwkPreviewReq()) {
        if (frame != mVendorRequests.begin()->first) {
            MLOGD(LogGroupHAL, "%s[Notify]: skip message because it's non first vndFrame:%u",
                  getFrameLog(frame).c_str(), mVendorRequests.begin()->first);
            return frame;
        }

        // NOTE: if this is not parallel snapshot, we forawrd message
        std::string logPrefix = "[" + getName() + "]";
        tryNotifyToFwk(msg, mFwkRequest.get(), LogGroupHAL, logPrefix.c_str());
    } else {
        // NOTE: if this photographer support attach preview
        uint32_t fwkFrame = getFwkFrameNumber(frame);

        // NOTE: the first attached fwk request is always a snapshot(see the Photographer's cstr),
        // the others are attached preview
        std::unique_lock<std::mutex> locker{mAttachedFwkReqMutex};
        auto it = mAttachedFwkReq.find(fwkFrame);
        if (it != mAttachedFwkReq.end()) {
            locker.unlock();
            MLOGI(LogGroupHAL,
                  "%s[Notify]: matches an attached fwkFrame:%u request, try to forward the msg",
                  getFrameLog(frame).c_str(), fwkFrame);

            std::string logPrefix = "[" + getName() + "]";
            tryNotifyToFwk(msg, it->second.get(), LogGroupHAL, logPrefix.c_str());
        } else {
            MLOGI(LogGroupHAL,
                  "%s[AttachPreview][Notify]: expected fwkFrame:%u req hasn't been attached, stash "
                  "away vendor msg",
                  getFrameLog(frame).c_str(), fwkFrame);
            auto tempVendorMsg = std::make_shared<NotifyMessage>(msg->getCamera(), msg->toRaw());
            mNotifyForComingPreviews[fwkFrame].push_back(tempVendorMsg);
        }
    }
    return frame;
}

int Photographer::processMetaResult(CaptureResult *result)
{
    uint32_t frame = result->getFrameNumber();

    // NOTE: for parallel snapshot, phg doesn't have a corresponding fwk request
    // NOTE: Final jpeg use mock result meta instead of this meta
    if (frame == mVendorRequests.begin()->first) {
        // NOTE: make sure no physical metadata sent to fwk
        PhysicalMetas phyMetas = result->getPhyMetas();
        if (phyMetas.size()) {
            auto meta = std::make_unique<Metadata>(result->getMeta()->peek());
            auto phyMeta = phyMetas.front().second;
            meta->update(*phyMeta, ANDROID_STATISTICS_FACE_RECTANGLES);
            LocalResult nonPhyResult{mFwkRequest->getCamera(), mFwkRequest->getFrameNumber(),
                                     result->getPartial(), std::move(meta)};
            MLOGD(LogGroupHAL, "%s[ProcessResult][Meta]: actual meta to fwk:\n\t%s",
                  getFrameLog(frame).c_str(), nonPhyResult.toString().c_str());
            forwardResultToFwk(&nonPhyResult);
        } else {
            // NOTE: we should carefully handle the case where result has both meta and buffer
            // FIXME: if a vendor result contains both meta and buffer then this vendor result will
            // be split into two fwk results(one only contains meta and the other only contains
            // buffer). Therefore, for such single vendor result, we need two inter process
            // communications which is time consuming, try to refactor this in the future
            auto meta = std::make_unique<Metadata>(result->getMeta()->peek());
            LocalResult onlyMetaResult{mFwkRequest->getCamera(), mFwkRequest->getFrameNumber(),
                                       result->getPartial(), std::move(meta)};
            MLOGD(LogGroupHAL, "%s[ProcessResult][Meta]: actual meta to fwk:\n\t%s",
                  getFrameLog(frame).c_str(), onlyMetaResult.toString().c_str());
            forwardResultToFwk(&onlyMetaResult);
        }
    }

    RequestRecord *record = getRequestRecord(frame);
    auto entry = result->findInMeta(ANDROID_SENSOR_TIMESTAMP);
    if (entry.count) {
        record->resultTs = (entry.data.i64[0] > 0) ? (entry.data.i64[0]) : -1;
    }
    collectMetadata(result);
    if (result->getPartial() == result->getCamera()->getPartialResultCount()) {
        if (supportAttachFwkPreviewReq()) {
            std::lock_guard<std::recursive_mutex> locker{mMutex};
            record->isFullMeta = true;
            sendResultForAttachedPreview(frame);
        } else {
            record->isFullMeta = true;
        }
    }

    if (record->pendingMetaCount) {
        record->pendingMetaCount--;
        MLOGI(LogGroupHAL, "%s[ProcessResult][KEY]: remaining meta: %d", getFrameLog(frame).c_str(),
              record->pendingMetaCount);
        if (!record->pendingMetaCount) {
            MLOGI(LogGroupHAL, "%s[ProcessResult][KEY]: all metas received",
                  getFrameLog(frame).c_str());
        }
    }

    return 0;
}

int Photographer::processErrorResult(CaptureRequest *vendorRequest)
{
    uint32_t frame = vendorRequest->getFrameNumber();
    MLOGW(LogGroupHAL, "%s[ProcessResult]: buffer/meta error/request", getFrameLog(frame).c_str());

    for (int i = 0; i < vendorRequest->getOutBufferNum(); i++) {
        auto streamBuffer = vendorRequest->getStreamBuffer(i);
        // NOTE: Occasionally, some error result will contain normal snapshot buffers. We should
        // flush the darkroom only if at least one snapshot buffer of the result is error
        if (streamBuffer->isSnapshot()) {
            streamBuffer->releaseBuffer();
            MLOGI(LogGroupHAL, "[AppSnapshotController]:%s releaseVirtualBuffer from stream: %p",
                  getFrameLog(frame).c_str(), streamBuffer->getStream()->toRaw());
            streamBuffer->getStream()->releaseVirtualBuffer();
        }
    }

    return 0;
}

int Photographer::processBufferResult(const CaptureResult *result)
{
    uint32_t frame = result->getFrameNumber();
    RequestRecord *record = getRequestRecord(frame);
    MASSERT(record);

    std::vector<std::shared_ptr<StreamBuffer>> outFwkBuffers;
    for (int i = 0; i < result->getBufferNumber(); i++) {
        // NOTE: we need handle this case that no error notify callback but only returns error
        // buffers from vendor, or no error notify callback and status is zero but the buffer
        // handle is null from vendor. Than we should go to mihal error processing.
        StreamBuffer *vendorRawBuffer = result->getStreamBuffer(i).get();
        const buffer_handle_t *rawHandle = vendorRawBuffer->toRawHandle();
        bool isErrorSnapBuffer = false;
        if (vendorRawBuffer->isSnapshot()) {
            if (vendorRawBuffer->getStatus() || !(*rawHandle))
                isErrorSnapBuffer = true;
        }
        if (isErrorSnapBuffer) {
            RequestRecord *record = getRequestRecord(frame);
            MASSERT(record);
            record->isErrorMsg = true;
            MLOGW(LogGroupHAL, "%s[ProcessResult][Buffer]: returns error buffer for stream:%p",
                  getFrameLog(frame).c_str(), vendorRawBuffer->getStream());
        }

        auto streamBuffer = mapToRequestedBuffer(frame, vendorRawBuffer);
        MASSERT(streamBuffer);

        auto stream = streamBuffer->getStream();
        uint32_t streamId = stream->getStreamId();
        MLOGI(LogGroupHAL, "%s[ProcessResult][Buffer]: buffer of stream %s come back",
              getFrameLog(frame).c_str(), stream->toString(0).c_str());

        // NOTE: we will use quickview buffer to fill fwk buffers which are not sent to Qcom
        if (stream->isQuickview()) {
            getAttachedFwkBuffers(frame, streamBuffer, outFwkBuffers);
        }
        record->pendingBuffers.reset(streamId);
        if (!stream->isInternal())
            outFwkBuffers.push_back(streamBuffer);
    }

    // if all the out buffers are from preview/realtime streams, then just deliver to framework
    if (outFwkBuffers.size()) {
        LocalResult lr{result->getCamera(), getFwkFrameNumber(frame), 0, outFwkBuffers};
        forwardResultToFwk(&lr);
    }

    return 0;
}

int Photographer::processVendorResult(CaptureResult *result)
{
    MLOGI(LogGroupHAL, "%s[ProcessResult]: get vendor result: %s",
          getFrameLog(result->getFrameNumber()).c_str(), result->toString().c_str());

    uint32_t frame = result->getFrameNumber();
    if (result->getPartial()) {
        auto entry = result->findInMeta(MI_ANCHOR_FRAME_TS);
        if (entry.count && !mAnchored && doAnchorFrameAsEarlyPicture() && !mIsMiviAnchor) {
            int64_t anchorTimestamp = entry.data.i64[0];
            if (anchorTimestamp != 0xFF) {
                auto entry = result->findInMeta(MI_ANCHOR_FRAME_NUM);
                if (entry.count) {
                    int32_t anchorFrameNum = entry.data.i32[0];
                    sendAnchorFrame(anchorTimestamp, anchorFrameNum,
                                    mVendorRequests.begin()->first);
                } else {
                    sendAnchorFrame(anchorTimestamp, mVendorRequests.begin()->first - 1,
                                    mVendorRequests.begin()->first);
                }
            } else {
                sendAnchorFrame(0, mVendorRequests.begin()->first - 1,
                                mVendorRequests.begin()->first);
            }
        }

        processMetaResult(result);
    }

    if (result->getBufferNumber()) {
        processBufferResult(result);
    }

    RequestRecord *record = getRequestRecord(frame);
    MASSERT(record);

    MLOGD(LogGroupHAL, "%s[ProcessResult]: finish processing this vendor result, record status:%s",
          getFrameLog(frame).c_str(), record->onelineReport().c_str());

    if (record->isFullMeta || record->isErrorMsg || isDropped()) {
        // NOTE: If current capture request should do anchor frame quick view, but timestamp of
        // anchor frame hasn't return due to misbehavior of vendor hal, we need to
        // sendAnchorFrame here to ensure that each snapshot request has a corresponding
        // EarlyPic.
        if (doAnchorFrameAsEarlyPicture() && !mAnchored && !mIsMiviAnchor) {
            MLOGW(LogGroupHAL,
                  "%s[ProcessResult][EarlyPic]: all results have returned but didn't trigger "
                  "anchor event, force to trigger it!!",
                  getFrameLog(frame).c_str());
            sendAnchorFrame(0, mVendorRequests.begin()->first - 1, mVendorRequests.begin()->first);
        }

        if (record->pendingBuffers.none()) {
            mFirstFinish++;
            if (frame == mVendorRequests.rbegin()->first) {
                // Vendor Request Complete
                if (mPhtgphType == Photographer_VENDOR_MFNR ||
                    (mPhtgphType == Photographer_HDR && mIsMFNRHDR)) {
                    mSession->subPendingVendorMFNR();
                    onMFNRRequestChanged();
                }
            }
            // Note : If the current request fails to obtain snapshot buffer/meta or is an invalid
            // request before Burst, the Darkroom Reprocess will not be performed. Among them, the
            // request for non-first frame needs to cancel the task waiting in Darkroom through
            // quickFinishDarkroomTask.

            uint32_t firstFrame = mVendorRequests.begin()->first;
            if (result->isSnapshotError() || record->isErrorMsg || mFinalFinished || isDropped()) {
                if (!mFinalFinished) {
                    mFinalFinished = true;
                    MLOGW(LogGroupHAL,
                          "%s: [Fulfill][3/3]: mock job done (because of error vendor snapshot"
                          ":%d /request isDropped:%d)",
                          getFrameLog(frame).c_str(),
                          result->isSnapshotError() || record->isErrorMsg, isDropped());

                    BGServiceWrap::getInstance()->onCaptureFailed(getImageName(),
                                                                  mFwkRequest->getFrameNumber());
                    auto imageName = getImageName();
                    auto frameNum = mFakeFwkFrameNum;
                    if (mFirstFinish > 1) {
                        darkRoomEnqueue([frameNum, imageName] {
                            DARKROOM.quickFinishTask(frameNum, imageName, false);
                        });
                    } else {
                        darkRoomEnqueue([frameNum] { DARKROOM.removeMission(frameNum); });

                        mSession->decreasePendingFinalSnapCount();
                        if (getSessionType() == Session::SyncAlgoSessionType) {
                            std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
                            for (int i = 0; i < mFwkRequest->getOutBufferNum(); i++) {
                                std::shared_ptr<StreamBuffer> streamBuffer =
                                    mFwkRequest->getStreamBuffer(i);
                                Stream *stream = streamBuffer->getStream();
                                if (!stream->isSnapshot()) {
                                    continue;
                                }
                                streamBuffer->setErrorBuffer();
                                outBuffers.push_back(streamBuffer);
                            }
                            if (outBuffers.size()) {
                                LocalResult lr{mFwkRequest->getCamera(),
                                               mFwkRequest->getFrameNumber(), 0, outBuffers};
                                MLOGI(LogGroupHAL, "[SDK]return a error result for incomplete: %s",
                                      lr.toString().c_str());
                                forwardResultToFwk(&lr);
                            }
                        }
                    }
                }

                if (!record->isReportError) {
                    record->isReportError = true;
                    processErrorResult(record->request.get());
                }
                // NOTE: If buffer error has returned but full meta has not returned, we should not
                // remove phg from mVendorInflights. Otherwise, when this meta returns later, it can
                // not get executor from mVendorInflights.
                if (!record->pendingMetaCount || record->isFullMeta) {
                    MLOGI(LogGroupHAL,
                          "%s[ProcessResult][KEY]: all metas received due to meta error",
                          getFrameLog(frame).c_str());
                    // NOTE: When phg has multi frames, no matter which frame returns error, we
                    // should only notify mialgo once to decrease the use count of current mialgo.
                    complete(frame);
                }
                return frame;
            }

            MLOGI(LogGroupHAL, "%s[ProcessResult][KEY]: all buffers/metas received",
                  getFrameLog(frame).c_str());
            goToDarkroom(frame);

            complete(frame);
            return frame;
        }
    }

    return frame;
}

bool Photographer::isFinish(uint32_t vndFrame, int32_t *frames = nullptr)
{
    if (mVendorRequests.begin()->first == vndFrame) {
        RequestRecord *record = getRequestRecord(vndFrame);
        bool finish = (record->mStatus.load() == ALL_FINISH && mAttachedCount.load() == 0);
        frames[0] = finish ? vndFrame : -1;
        frames[1] = -1;
        if (finish)
            MLOGI(LogGroupHAL, "[ProcessResult][KEY]: remove  first vndFrame:%u", vndFrame);
        return finish;
    } else {
        RequestRecord *record = getRequestRecord(vndFrame);
        bool finish1 = (record->mStatus.load() == ALL_FINISH);
        frames[0] = finish1 ? vndFrame : -1;
        if (finish1)
            MLOGI(LogGroupHAL, "[ProcessResult][KEY]: remove  vndFrame:%u", vndFrame);
        auto first = mVendorRequests.begin()->first;
        RequestRecord *firstRecord = getRequestRecord(first);

        std::lock_guard<std::mutex> locker{mAttachedFwkReqMutex};
        bool finish2 = (firstRecord->mStatus.load() == ALL_FINISH && mAttachedCount.load() == 0);
        frames[1] = finish2 ? first : -1;
        if (finish2)
            MLOGI(LogGroupHAL, "[ProcessResult][KEY]: remove  first vndFrame:%u", first);
        return finish1 | finish2;
    }
}

std::vector<uint32_t> Photographer::getVendorRequestList() const
{
    std::vector<uint32_t> requests;
    for (auto &pair : mVendorRequests)
        requests.push_back(pair.first);

    return requests;
}

std::string Photographer::toString() const
{
    std::ostringstream str;

    str << "" << getName() << " for camera " << mFwkRequest->getCamera()->getId()
        << ", fwkFrame:" << mFwkRequest->getFrameNumber() << ", contains " << mVendorRequests.size()
        << " requests:";

    for (const auto &pair : mVendorRequests) {
        auto &record = pair.second;
        str << "\n\t\tframe " << pair.first << " : " << record->request->getOutBufferNum()
            << " buffers"
            << ", pending=" << record->pendingBuffers;
    }

    return str.str();
}

bool Photographer::isFinalPicFinished() const
{
    return mFinalFinished.load();
}

int Photographer::processEarlyResult(std::shared_ptr<StreamBuffer> inputBuffer)
{
    if (mEarlypicFlag.test_and_set()) {
        MLOGD(LogGroupHAL, "%s[EarlyPic]: have filled earlypic buffer,this time will pass",
              getLogPrefix().c_str());
        return 0;
    }
    mEarlypicDone = true;
    mEarlypicTimer->cancel();
    MLOGI(LogGroupHAL, "%s[EarlyPic]: ready to fill early pic buffer", getLogPrefix().c_str());

    int ret = 0;
    std::shared_ptr<StreamBuffer> snapshotBuffer;
    for (int i = 0; i < mFwkRequest->getOutBufferNum(); i++) {
        std::shared_ptr<StreamBuffer> streamBuffer = mFwkRequest->getStreamBuffer(i);
        if (streamBuffer->isSnapshot()) {
            snapshotBuffer = streamBuffer;
            break;
        }
    }

    if (!snapshotBuffer) {
        MLOGE(LogGroupHAL, "%s[EarlyPic]: no snapshot stream buffer int mFwkRequest",
              getLogPrefix().c_str());
        return -1;
    }

    if (inputBuffer != nullptr) {
        snapshotBuffer->tryToCopyFrom(*inputBuffer, mFwkRequest->getMeta());
    } else {
        MLOGW(LogGroupHAL, "%s[EarlyPic]: set mFwkRequest stream buffer error",
              getLogPrefix().c_str());
        snapshotBuffer->setErrorBuffer();
    }

    std::vector<std::shared_ptr<StreamBuffer>> outFwkBuffers;
    getErrorBuffers(outFwkBuffers);
    if (!outFwkBuffers.empty()) {
        MLOGI(LogGroupHAL,
              "%s[EarlyPic]: set error to some fwk buffers, refer to earlyReport for detail",
              getLogPrefix().c_str());
    }
    outFwkBuffers.push_back(snapshotBuffer);

    LocalResult fResult{mFwkRequest->getCamera(), mFwkRequest->getFrameNumber(), 0, outFwkBuffers};
    MLOGI(LogGroupHAL,
          "%s[EarlyPic]: finish filling early pic buffer, forward earlyReport to app:%s",
          getLogPrefix().c_str(), fResult.toString().c_str());

    forwardResultToFwk(&fResult);

    if (!mQuickShotEnabled && !mFakeQuickShotEnabled) {
        onSnapCompleted(mFwkRequest->getFrameNumber(), getImageName(),
                        RequestExecutor::HAL_SHUTTER_NOTIFY);
    }
    return ret;
}

int Photographer::asyncConfigDarkroom()
{
    mSignature = "DefaultSig";
    if (mDarkroomConfig) {
        auto entry = mDarkroomConfig->getMeta()->find(MI_MIVI_POST_SESSION_SIG);
        if (entry.count) {
            mSignature = reinterpret_cast<const char *>(entry.data.u8);
        }

        MLOGI(LogGroupHAL, "%s[PostProcess][Config][GreenPic][KEY]: I will be sent to mialgo:%s",
              getLogPrefix().c_str(), mSignature.c_str());

        mInvitation = getInvitation(mFwkRequest->getFrameNumber());
        mInvitation->imageName = getImageName();
        std::shared_ptr<Metadata> tempMeta =
            std::make_shared<Metadata>(mExtraMetadata.release(), Metadata::TakeOver);
        auto sig = mSignature;
        auto config = mDarkroomConfig;
        auto invite = mInvitation;
        darkRoomEnqueue([sig, config, invite, tempMeta] {
            DARKROOM.configureStreams(
                sig, config, invite, tempMeta, [](const std::string &sessionName) {
                    MialgoTaskMonitor::getInstance()->deleteMialgoTaskSig(sessionName);
                });
        });
    }
    return 0;
}

void Photographer::setErrorRealtimeBuffers(std::vector<std::shared_ptr<StreamBuffer>> &buffers)
{
    for (int i = 0; i < mFwkRequest->getOutBufferNum(); i++) {
        auto streamBuffer = mFwkRequest->getStreamBuffer(i);
        if (streamBuffer->isSnapshot())
            continue;

        // NOTE: if buffers are sent to Qcom, then they will be filled by Qcom
        if (streamBuffer->isSentToHal())
            continue;

        // NOTE: for other buffers which are not sent to Qcom, set them as error
        streamBuffer->setErrorBuffer();
        buffers.push_back(streamBuffer);
    }
}

int Photographer::collectMetadata(const CaptureResult *result)
{
    uint32_t frame = result->getFrameNumber();
    RequestRecord *record = getRequestRecord(frame);
    MASSERT(record);

    auto &meta = record->collectedMeta;
    // FIXME: is it possible to get lock free? since the meta operation is time costly.
    // Because the buffer is always after meta, and only if the buffer comes we
    // then go to read the collected buffer
    if (!meta) {
        meta = std::make_unique<Metadata>(*result->getMeta());
    } else {
        meta->append(*result->getMeta());
    }

    PhysicalMetas phyMetas = result->getPhyMetas();
    if (phyMetas.size()) {
        auto phyMeta = phyMetas.front().second;
        meta->update(*phyMeta, ANDROID_STATISTICS_FACE_RECTANGLES);
        record->phyMetas = phyMetas;
        for (auto &pair : phyMetas) {
            pair.second->safeguard();
        }
    }

    return 0;
}

Photographer::RequestRecord *Photographer::getRequestRecord(uint32_t frame) const
{
    auto it = mVendorRequests.find(frame);
    if (it == mVendorRequests.end()) {
        MLOGE(LogGroupHAL, "no record for result %d", frame);
        return nullptr;
    }

    return it->second.get();
}

void Photographer::buildPreRequestToDarkroom()
{
    RequestRecord *record = getRequestRecord(mVendorRequests.begin()->first);
    MASSERT(record);

    CaptureRequest *vendorRequest = record->request.get();

    // NOTE: update metadata
    Metadata meta{};
    meta = *(mDarkroomConfig->getModifyMeta());
    meta.update(*(vendorRequest->getMeta()));
    mSession->getMode()->setupPreprocessForcedMeta(meta);

    std::vector<camera3_stream> streams;
    for (int i = 0; i < vendorRequest->getOutBufferNum(); i++) {
        std::shared_ptr<StreamBuffer> streamBuffer = vendorRequest->getStreamBuffer(i);
        if (streamBuffer->getStream()->isSnapshot()) {
            camera3_stream stream = *(streamBuffer->getStream()->toRaw());
            stream.stream_type = CAMERA3_STREAM_INPUT;
            streams.push_back(stream);
        }
    }
    if (!mInvitation)
        mInvitation = getInvitation(mFwkRequest->getFrameNumber());
    auto outputFormats = mInvitation->imageForamts;
    for (int i = 0; i < outputFormats.size(); i++) {
        camera3_stream stream{};
        stream.format = outputFormats[0].format;
        stream.width = outputFormats[0].width;
        stream.height = outputFormats[0].height;
        stream.stream_type = CAMERA3_STREAM_OUTPUT;
        streams.push_back(stream);
        MLOGI(LogGroupHAL, "[addToMockInvitations][GreenPic]: invited mock buffers size:%uX%u",
              outputFormats[0].width, outputFormats[0].height);
    }

    auto preProcessConfig =
        std::make_shared<PreProcessConfig>(vendorRequest->getCamera(), streams, &meta);
    auto frame = mFakeFwkFrameNum;

    darkRoomEnqueue(
        [frame, preProcessConfig] { DARKROOM.preProcessCaptureRequest(frame, preProcessConfig); });
}

std::shared_ptr<CaptureRequest> Photographer::buildRequestToDarkroom(uint32_t frame)
{
    if (getSessionType() == Session::SyncAlgoSessionType) {
        return buildSyncRequestToDarkroom(frame);
    } else {
        return buildAsyncRequestToDarkroom(frame);
    }
}

std::shared_ptr<CaptureRequest> Photographer::buildSyncRequestToDarkroom(uint32_t frame)
{
    RequestRecord *record = getRequestRecord(frame);
    MASSERT(record);
    update3ADebugData(record);
    CaptureRequest *vendorRequest = record->request.get();

    std::unique_lock<std::recursive_mutex> locker{mMutex};
    std::vector<std::shared_ptr<StreamBuffer>> inputBuffers;
    for (int i = 0; i < vendorRequest->getOutBufferNum(); i++) {
        std::shared_ptr<StreamBuffer> streamBuffer = vendorRequest->getStreamBuffer(i);
        if (streamBuffer->getStream()->isInternal() && !streamBuffer->isQuickview()) {
            inputBuffers.push_back(streamBuffer);
        }
    }

    std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
    auto fwkFrame = mFwkRequest->getFrameNumber();
    bool firstReqToVendor = (frame == mVendorRequests.begin()->first);

    if (firstReqToVendor) {
        for (int i = 0; i < mFwkRequest->getOutBufferNum(); i++) {
            std::shared_ptr<StreamBuffer> streamBuffer = mFwkRequest->getStreamBuffer(i);
            Stream *stream = streamBuffer->getStream();
            if (!stream->isInternal() && !stream->isSnapshot()) {
                continue;
            }
            outBuffers.push_back(streamBuffer);
        }
    }

    locker.unlock();
    // FIXME: optimization: try to avoid meta copy here
    auto req = std::make_shared<LocalRequest>(vendorRequest->getCamera(), frame, inputBuffers,
                                              outBuffers, *record->collectedMeta, record->phyMetas);

    setupMetaForRequestToDarkroom(req.get());
    MLOGI(LogGroupHAL, "inputBuffers format:%d", inputBuffers[0]->getStream()->getFormat());
    if (firstReqToVendor)
        MLOGI(LogGroupHAL, "outputBuffers format:%d", outBuffers[0]->getStream()->getFormat());

    char name[10];

    // MialgoEngine need same name for one process,so set the name is first frame of
    // mVendorRequests.
    sprintf(name, "SDK %d", mVendorRequests.begin()->first);
    req->updateMeta(MI_SNAPSHOT_IMAGE_NAME, name);

    std::string searchKey = "MialgoTaskConsumption." + getName();
    auto sig = mSignature;
    auto session = mSession;
    auto fakeNum = mFakeFwkFrameNum;
    darkRoomEnqueue([sig, req, firstReqToVendor, searchKey, fwkFrame, session, frame, fakeNum] {
        DARKROOM.processInputRequest(
            sig, req, firstReqToVendor, nullptr,
            [searchKey, fwkFrame, sig, session, frame](const CaptureResult *result,
                                                       bool isEarlyPicDone) {
                int consumption;
                MiviSettings::getVal(searchKey, consumption, 0);
                MLOGD(LogGroupHAL,
                      "[AppSnapshotController]: my task finished by Signature: %s, fwkFrame: "
                      "%d,consumption "
                      "released: %d",
                      sig.c_str(), fwkFrame, consumption);
                MialgoTaskMonitor::getInstance()->onMialgoTaskNumChanged(sig, consumption, fwkFrame,
                                                                         false);
                if (session) {
                    session->updatePendingFwkRecords(result);
                    session->removeFromVendorInflights(frame);
                    MLOGD(LogGroupHAL,
                          "[RequestToDarkroom]: [Fulfill]:SDK updatePendingFwkRecords, fwkFrame:%d",
                          fwkFrame);
                }
            },
            fakeNum, true);
    });

    return req;
}

std::shared_ptr<CaptureRequest> Photographer::buildAsyncRequestToDarkroom(uint32_t frame)
{
    RequestRecord *record = getRequestRecord(frame);
    MASSERT(record);
    update3ADebugData(record);

    CaptureRequest *vendorRequest = record->request.get();
    auto firstFrame = mVendorRequests.begin()->first;
    std::unique_lock<std::recursive_mutex> locker{mMutex};
    std::vector<std::shared_ptr<StreamBuffer>> inputBuffers;
    for (int i = 0; i < vendorRequest->getOutBufferNum(); i++) {
        std::shared_ptr<StreamBuffer> streamBuffer = vendorRequest->getStreamBuffer(i);
        if (streamBuffer->getStream()->isInternal()) {
            if (!streamBuffer->isQuickview())
                inputBuffers.push_back(streamBuffer);
            else
                returnInternalFrameTimestamp(frame, record->resultTs);
        }
    }
    auto fwkFrame = mFwkRequest->getFrameNumber();
    std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
    bool firstReqToVendor = (frame == firstFrame);

    locker.unlock();

    // FIXME: optimization: try to avoid meta copy here
    auto req = std::make_shared<LocalRequest>(vendorRequest->getCamera(), frame, inputBuffers,
                                              outBuffers, *record->collectedMeta, record->phyMetas);

    MLOGI(LogGroupHAL, "frame %d inputBuffers size: %d outBuffers size: %d", frame,
          inputBuffers.size(), outBuffers.size());

    setupMetaForRequestToDarkroom(req.get());

    std::string searchKey = "MialgoTaskConsumption." + getName();
    auto tempId = mSession->getId();
    std::function<void(int64_t)> fAnchor = nullptr;
    auto isMiviAnchor = mIsMiviAnchor;

    if (!mAnchorCB) {
        mAnchorCB = [tempId, isMiviAnchor, firstFrame](int64_t anchorTimestamp) {
            if (isMiviAnchor) {
                auto sharedSession = SessionManager::getInstance()->getSession(tempId);
                if (!sharedSession)
                    return;
                if (anchorTimestamp != -1) {
                    uint32_t anchorFrameNum =
                        sharedSession->getMode()->findAnchorVndframeByTs(anchorTimestamp);
                    if (anchorFrameNum != -1) {
                        MLOGI(LogGroupHAL, "[PostProcess][EarlyPic]: MiviAnchor frame num %d",
                              anchorFrameNum);
                        sharedSession->anchorFrameEvent(anchorTimestamp, anchorFrameNum,
                                                        firstFrame);
                    } else {
                        sharedSession->anchorFrameEvent(anchorTimestamp, firstFrame - 1,
                                                        firstFrame);
                    }
                } else {
                    sharedSession->anchorFrameEvent(0, firstFrame - 1, firstFrame);
                }
            }
        };
    }
    fAnchor = mAnchorCB;
    auto sig = mSignature;
    auto session = mSession;
    auto fakeNum = mFakeFwkFrameNum;
    bool isEarlyPicDone = (mPhtgphType == Photographer_BURST) ? true : mEarlypicDone.load();
    darkRoomEnqueue([sig, req, firstReqToVendor, fAnchor, searchKey, fwkFrame, tempId, fakeNum,
                     frame, isEarlyPicDone] {
        DARKROOM.processInputRequest(
            sig, req, firstReqToVendor, fAnchor,
            [searchKey, fwkFrame, sig, tempId, frame](const CaptureResult *result,
                                                      bool isEarlyPicDone) {
                int consumption;
                MiviSettings::getVal(searchKey, consumption, 0);
                MLOGD(LogGroupHAL,
                      "[AppSnapshotController]: my task finished by Signature: %s, "
                      "fwkFrame:%d,consumption released: %d",
                      sig.c_str(), fwkFrame, consumption);
                MialgoTaskMonitor::getInstance()->onMialgoTaskNumChanged(sig, consumption, fwkFrame,
                                                                         false);
                auto sharedSession = SessionManager::getInstance()->getSession(tempId);
                if (sharedSession) {
                    if (!isEarlyPicDone) {
                        sharedSession->processEarlySnapshotResult(frame, nullptr, true);
                    }
                    sharedSession->decreasePendingFinalSnapCount();
                    sharedSession->getMode()->onMihalBufferChanged();
                    if (sharedSession->getPendingFinalSnapCount() <= 0) {
                        sharedSession = nullptr;
                        SessionManager::getInstance()->signalReclaimSession();
                    }
                    MLOGD(LogGroupHAL,
                          "[RequestToDarkroom]: [Fulfill]:decrease mPendingMiHALSnapCount, "
                          "fwkFrame:%d,isEarlyPicDone:%d",
                          fwkFrame, isEarlyPicDone);
                }
            },
            fakeNum, isEarlyPicDone);
    });

    return nullptr;
}

int Photographer::setupMetaForRequestToDarkroom(LocalRequest *request)
{
    uint32_t frame = request->getFrameNumber();
    RequestRecord *record = getRequestRecord(frame);
    MASSERT(record);
    CaptureRequest *vendorRequest = record->request.get();

    // NOTE: for "xiaomi.multiframe.inputNum", different buffer should be set different value
    for (int i = 0; i < request->getInputBufferNum(); ++i) {
        auto buffer = request->getInputStreamBuffer(i);
        auto &meta = request->getBufferSpecificMeta(buffer.get());

        MASSERT(buffer->getMialgoPort() >= 0);
        int32_t multiFrameNum = mBufferNumPerPort[buffer->getMialgoPort()];
        meta.update(MI_MULTIFRAME_INPUTNUM, &multiFrameNum, 1);
    }

    // NOTE: the burstNum and burstIdx are used by ReprocessCamera to tell
    // whether a request send to ReprocessCamera has output buffers
    int32_t burstNum = mVendorRequests.size();
    request->updateMeta(MI_BURST_TOTAL_NUM, &burstNum, 1);

    // NOTE: just tell mialgoengine how many vendor request will coming soon
    if (!mMialgoNum) {
        for (auto &itr : mVendorRequests) {
            for (int j = 0; j < itr.second->request->getOutBufferNum(); j++) {
                auto streambuffer = itr.second->request->getStreamBuffer(j);
                if (streambuffer->getStream()->isSnapshot()) {
                    mMialgoNum++;
                }
            }
        }
        request->updateMeta(MI_BURST_MIALGO_TOTAL_NUM, &mMialgoNum, 1);
    }

    // burst index must be counted from 1
    int32_t burstIdx = frame - mVendorRequests.begin()->first + 1;
    request->updateMeta(MI_BURST_CURRENT_INDEX, &burstIdx, 1);

    // For sdk multiCam caculate TotalSize In GetUnalignedBufferSize
    int32_t maxJpegSize = 0;

    if (getSessionType() == Session::SyncAlgoSessionType) {
        maxJpegSize = getMaxJpegSize();
    } else {
        maxJpegSize = CameraManager::getInstance()->getMockCameraMaxJpegSize();
    }
    if (maxJpegSize > 0) {
        MLOGI(LogGroupHAL, "update maxJpegSize %d", maxJpegSize);
        request->updateMeta(MI_MIVI_MAX_JPEGSIZE, &maxJpegSize, 1);
    }
    if (mDarkroomConfig) {
        const Metadata *meta = mDarkroomConfig->getMeta();
        auto entry = meta->find(MI_MIVI_POST_SESSION_SIG);
        if (entry.count)
            request->updateMeta(entry);
    }

    int32_t captureHint = 0;

    if (mPhtgphType == Photographer_BURST) {
        captureHint = 1;
        std::string name = getImageName();
        name = name + std::to_string(frame);
        request->updateMeta(MI_SNAPSHOT_IMAGE_NAME, name.c_str());
    }

    request->updateMeta(MI_BURST_CAPTURE_HINT, &captureHint, 1);

    mergeForceMetaToDarkroom(vendorRequest, request);

    return 0;
}

void Photographer::updateAnchorFrameQuickView()
{
    int maskBit = getAnchorFrameMaskBit();
    int anchorFrameMask = mFwkRequest->getCamera()->getAnchorFrameMask();
    bool isEnable = ((maskBit & anchorFrameMask) != 0);
    MLOGI(LogGroupHAL, "[EarlyPic] maskBit: 0x%x, anchorFrameMask: 0x%x isEnable %d", maskBit,
          anchorFrameMask, isEnable);

    mAnchorFrameQuickView = isEnable;
}

bool Photographer::canAttachFwkRequest(std::shared_ptr<const CaptureRequest> request)
{
    if (!supportAttachFwkPreviewReq()) {
        return false;
    }

    uint32_t frame = request->getFrameNumber();
    if (frame > mFwkRequest->getFrameNumber() &&
        frame < mFwkRequest->getFrameNumber() + mVendorRequests.size()) {
        return true;
    } else {
        return false;
    }
}

bool Photographer::supportAttachFwkPreviewReq()
{
    // NOTE: only muti frame snapshot which takes quickview buffer to HAL can attach preview
    return ((mVendorRequests.size() > 1) && mHasSentQuickviewBufferToHal &&
            !isForceDonotAttachPreview());
}

void Photographer::attachFwkRequest(std::shared_ptr<const CaptureRequest> request)
{
    uint32_t frame = request->getFrameNumber();
    MLOGI(LogGroupHAL, "[%s][AttachPreview]: fwkFrame:%u", getName().c_str(), frame);

    std::unique_lock<std::recursive_mutex> recordLocker{mMutex};
    std::unique_lock<std::mutex> locker{mAttachedFwkReqMutex};
    mAttachedFwkReq.insert({frame, request});
    auto itNotify = mNotifyForComingPreviews.find(frame);
    if (itNotify != mNotifyForComingPreviews.end()) {
        auto toFwk = itNotify->second;
        mNotifyForComingPreviews.erase(itNotify);
        MLOGI(LogGroupHAL,
              "[%s][AttachPreview][Notify]: fwkFrame:%u preview attached, try to forward stashed "
              "msgs",
              getName().c_str(), frame);
        for (auto notifyMsg : toFwk) {
            tryNotifyToFwk(notifyMsg.get(), request.get(), LogGroupHAL, "[Preview]");
        }
    }

    uint32_t vndFrame = getVndFrameNumber(frame);
    RequestRecord *record = getRequestRecord(vndFrame);
    MASSERT(record);

    // NOTE: use isFullMeta to avoid twice result
    if (record->isFullMeta) {
        MLOGI(LogGroupHAL,
              "[%s][AttachPreview][Meta]: fwkFrame:%u preview attached, forward stashed meta",
              getName().c_str(), frame);
        sendResultForAttachedPreviewImpl(request.get(), record);
    }
    locker.unlock();
    recordLocker.unlock();

    locker.lock();
    if (record->cachedForAttachedPreview) {
        std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
        auto srcStreamBuffer = record->cachedForAttachedPreview;
        fillFwkBufferFromVendorHal(request.get(), srcStreamBuffer, outBuffers);
        if (outBuffers.size()) {
            LocalResult lr{request->getCamera(), frame, 0, outBuffers};
            forwardResultToFwk(&lr);
        }

        returnInternalQuickviewBuffer(srcStreamBuffer, vndFrame, record->resultTs);
        record->cachedForAttachedPreview = nullptr;
    }
}
void Photographer::removeAttachFwkRequest(uint32_t frame)
{
    if (mAttachedCount.load() > 0) {
        mAttachedFwkReq.erase(frame);
        mAttachedCount--;
    }
}

int Photographer::sendResultForAttachedPreviewImpl(const CaptureRequest *request,
                                                   const RequestRecord *record)
{
    uint32_t fwkFrame = request->getFrameNumber();
    uint32_t vndFrame = getVndFrameNumber(fwkFrame);
    uint32_t paratialCount = request->getCamera()->getPartialResultCount();
    auto meta = std::make_unique<Metadata>(record->collectedMeta->peek());
    LocalResult lr{request->getCamera(), fwkFrame, paratialCount, std::move(meta)};
    MLOGI(LogGroupHAL,
          "%s[ProcessResult][AttachPreview][Meta]: full meta for attached fwkFrame:%u preview",
          getFrameLog(vndFrame).c_str(), fwkFrame);

    return forwardResultToFwk(&lr);
}

int Photographer::sendResultForAttachedPreview(uint32_t frame)
{
    std::unique_lock<std::mutex> locker{mAttachedFwkReqMutex};
    uint32_t fwkPreviewFrame = getFwkFrameNumber(frame);
    auto resultIt = mAttachedFwkReq.find(fwkPreviewFrame);
    RequestRecord *record = getRequestRecord(frame);
    if (resultIt != mAttachedFwkReq.end() && fwkPreviewFrame != mFwkRequest->getFrameNumber()) {
        locker.unlock();
        auto fwkPreview = resultIt->second;
        sendResultForAttachedPreviewImpl(fwkPreview.get(), record);
    }

    return 0;
}

void Photographer::getAttachedFwkBuffers(uint32_t frame,
                                         std::shared_ptr<StreamBuffer> srcStreamBuffer,
                                         std::vector<std::shared_ptr<StreamBuffer>> &outBuffers)
{
    RequestRecord *record = getRequestRecord(frame);
    MASSERT(record);

    uint32_t fwkFrame = getFwkFrameNumber(frame);
    std::unique_lock<std::mutex> locker{mAttachedFwkReqMutex};
    auto resultIt = mAttachedFwkReq.find(fwkFrame);

    if (resultIt != mAttachedFwkReq.end()) {
        locker.unlock();
        auto fwkPreview = resultIt->second;

        fillFwkBufferFromVendorHal(fwkPreview.get(), srcStreamBuffer, outBuffers);
        returnInternalQuickviewBuffer(srcStreamBuffer, frame, record->resultTs);
    } else if (isForceDonotAttachPreview()) {
        // NOTE: This case support photographer that need to request preview buffer but without
        // attach preview and it's sub request preview result won't send back to fwk.
        // eg: HdrBokehPhotographer, SEBokehPhotographer.
        MLOGD(LogGroupHAL,
              "%s[ProcessResult][Buffer]: won't attach preview, return quickview buffer to "
              "internal list",
              getFrameLog(frame).c_str());

        returnInternalQuickviewBuffer(srcStreamBuffer, frame, record->resultTs);
    } else {
        MLOGD(LogGroupHAL,
              "%s[ProcessResult][AttachPreview][Buffer]: will attach preview, temporarily stash "
              "away quickview buffer",
              getFrameLog(frame).c_str());
        record->cachedForAttachedPreview = srcStreamBuffer;
    }
}

int Photographer::getAnchorFrameMaskBit()
{
    bool isFrontCamera = mFwkRequest->getCamera()->isFrontCamera();
    bool isSuperNight = isSupernightEnable();
    int modeType = getCameraModeType();

    if (modeType == CameraMode::MODE_PORTRAIT) {
        return isFrontCamera ? FRONT_PORTRAIT : (isSuperNight ? BACK_SUPER_NIGHT : BACK_PORTRAIT);
    } else if (modeType == CameraMode::MODE_SUPER_NIGHT) {
        if (mPhtgphType == Photographer_SN && !isFrontCamera) {
            return BACK_SUPER_NIGHT;
        }
    } else if (modeType == CameraMode::MODE_CAPTURE) {
        if (mPhtgphType == Photographer_BURST) {
            return 0;
        } else if (mPhtgphType == Photographer_SR) {
            return BACK_SR;
        } else if (mPhtgphType == Photographer_HDR) {
            if (isFrontCamera) {
                return FRONT_HDR;
            } else {
                return BACK_HDR;
            }
        } else if (mPhtgphType == Photographer_HDRSR) {
            if (isFrontCamera) {
                return FRONT_NORMAL;
            } else {
                return BACK_NORMAL;
            }
        } else if (mPhtgphType == Photographer_HD_REMOSAIC) {
            return BACK_HD_REMOSAIC;
        } else if (mPhtgphType == Photographer_HD_UPSCALE) {
            return isFrontCamera ? FRONT_NORMAL : BACK_HD_UPSCALE;
        } else if (mPhtgphType == Photographer_SN) {
            if (!isFrontCamera)
                return BACK_SUPER_NIGHT;
            else
                return 0;
        } else if (mPhtgphType == Photographer_VENDOR_MFNR ||
                   mPhtgphType == Photographer_MIHAL_MFNR) {
            if (!isFrontCamera) {
                return BACK_NORMAL;
            } else {
                return FRONT_NORMAL;
            }
        } else if (mPhtgphType == Photographer_FUSION) {
            if (!isFrontCamera) {
                return 0;
            } else {
                return BACK_FUSION;
            }
        } else {
            // NOTE: if type is Photographer_SIMPLE, we don't need anchor frame by default
            return 0;
        }
    }

    return 0;
}

void Photographer::mergeForceMetaToDarkroom(const CaptureRequest *src, LocalRequest *dest) const
{
    for (uint32_t tag : forcedTags) {
        auto entry = src->findInMeta(tag);
        if (entry.count)
            dest->updateMeta(entry);
    }

    for (const char *tag : forcedTagStrings) {
        auto entry = src->findInMeta(tag);
        if (entry.count)
            dest->updateMeta(entry);
    }

    std::string dump = dest->getMeta()->toStringByTags(forcedTags, &forcedTagStrings);
    MLLOGI(LogGroupRT, dump, "merged meta to darkroom for frame %d:\n", dest->getFrameNumber());
}

void Photographer::mergeForceMetaToMockResult(const CaptureRequest *src,
                                              std::shared_ptr<LocalResult> dest) const
{
    for (const char *tag : forcedTagStrings) {
        auto entry = src->findInMeta(tag);
        if (entry.count)
            dest->updateMeta(entry);
    }
}

uint32_t Photographer::getFwkFrameNumber(uint32_t frame)
{
    uint32_t firstFrame = mVendorRequests.begin()->first;
    return mFwkRequest->getFrameNumber() + (frame - firstFrame);
}

uint32_t Photographer::getVndFrameNumber(uint32_t frame)
{
    uint32_t offset = frame - mFwkRequest->getFrameNumber();
    return offset + mVendorRequests.begin()->first;
}

std::shared_ptr<StreamBuffer> Photographer::mapToRequestedBuffer(uint32_t frame,
                                                                 const StreamBuffer *buffer) const
{
    RequestRecord *record = getRequestRecord(frame);
    CaptureRequest *request = record->request.get();
    MASSERT(request);

    return request->getBufferByStream(buffer);
}

int64_t Photographer::buildTimestamp()
{
    auto time = std::chrono::steady_clock::now().time_since_epoch();
    std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(time);

    return ns.count();
}

int Photographer::complete(uint32_t frame)
{
    RequestRecord *record = getRequestRecord(frame);
    record->mStatus.store(record->mStatus.load() | RESULT_FINISH);
    return frame;
}

int Photographer::goToDarkroom(uint32_t frame)
{
    mSentToDarkroomReqCnt++;
    if (mSentToDarkroomReqCnt == mVendorRequests.size()) {
        MonitorMialgoTask(true);
        mFinalFinished = true;
    }

    if (Session::AsyncAlgoSessionType == getSessionType()) {
        if (frame == mVendorRequests.rbegin()->first && !mQuickShotEnabled &&
            !mFakeQuickShotEnabled) {
            onSnapCompleted(mFwkRequest->getFrameNumber(), getImageName(),
                            RequestExecutor::HAL_BUFFER_RESULT);
        }
    } else if (Session::SyncAlgoSessionType == getSessionType()) {
        // NOTE: if mihal request doesn't take quickview buffer to HAL, then fwk's buffers which are
        // not sent to Qcom will not be filled and since in SDK scenes, we do not have early pic
        // thread to help us set these buffer error, therefore we need to manually set error to
        // avoid buffer leak
        // NOTE: for muti frame snapshot, we should make sure that this set error behavior only
        // happen once
        if (frame == mVendorRequests.begin()->first) {
            std::vector<std::shared_ptr<StreamBuffer>> outFwkBuffers;
            getErrorBuffers(outFwkBuffers);
            if (!outFwkBuffers.empty()) {
                MLOGI(LogGroupHAL, "[sdk] set error to some fwk buffers");
                LocalResult fResult{mFwkRequest->getCamera(), mFwkRequest->getFrameNumber(), 0,
                                    outFwkBuffers};
                forwardResultToFwk(&fResult);
            }
        }
    }
    buildRequestToDarkroom(frame);
    return 0;
}

void Photographer::MonitorMialgoTask(bool isSendToMialgo)
{
    std::string searchKey = "MialgoTaskConsumption." + getName();
    auto fwkFrame = mFwkRequest->getFrameNumber();
    int consumption;
    MiviSettings::getVal(searchKey, consumption, 0);
    if (isSendToMialgo) {
        MLOGD(LogGroupHAL,
              "[AppSnapshotController]: send new task to mialgo Signature: %s, Fwk frame "
              "%d,consumption : %d",
              mSignature.c_str(), fwkFrame, consumption);
        MialgoTaskMonitor::getInstance()->onMialgoTaskNumChanged(mSignature, consumption, fwkFrame,
                                                                 true);
    } else {
        MLOGD(
            LogGroupHAL,
            "[AppSnapshotController]: my task finished by Signature: %s, Fwk frame %d,consumption "
            "released: %d",
            mSignature.c_str(), fwkFrame, consumption);
        MialgoTaskMonitor::getInstance()->onMialgoTaskNumChanged(mSignature, consumption, fwkFrame,
                                                                 false);
    }
}

int Photographer::isSupportMfnrBokeh()
{
    camera_metadata_ro_entry entry =
        mFwkRequest->getCamera()->findInStaticInfo(MI_MFNR_BOKEH_SUPPORTED);
    if (entry.count) {
        return entry.data.u8[0];
    } else {
        return 0;
    }
}

void Photographer::update3ADebugData(RequestRecord *record)
{
    {
        auto entry = record->collectedMeta->find(QCOM_3A_DEBUG_DATA);
        if (entry.count) {
            DebugData *data = nullptr;
            data = reinterpret_cast<DebugData *>(entry.data.u8);
            MLOGD(LogGroupHAL, "[Offline DebugData][3A_Debug] logicalMeta, data:%p, size:%zu",
                  data->pData, data->size);
        }

        entry = record->collectedMeta->find(MI_OPERATION_MODE);
        if (entry.count) {
            int32_t opMode = entry.data.i32[0];
            MLOGD(LogGroupHAL, "[Offline DebugData][3A_Debug] logicalMeta opMode:0x%x", opMode);
        }
    }

    // NOTE: For multicamera, we update 3A debug data form master physical meta to final meta.
    if (record->phyMetas.size() > 0) {
        for (auto &pair : record->phyMetas) {
            uint32_t phyId = atoi(pair.first.c_str());
            auto phyMeta = pair.second;
            auto entry = phyMeta->find(QCOM_3A_DEBUG_DATA);
            if (entry.count) {
                DebugData *data = nullptr;
                data = reinterpret_cast<DebugData *>(entry.data.u8);
                MLOGD(LogGroupHAL,
                      "[Offline DebugData][3A_Debug] phyMeta [id:%s], data:%p, size:%zu",
                      pair.first.c_str(), data->pData, data->size);

                if (phyId == getMasterPhyId()) {
                    record->collectedMeta->update(*phyMeta, QCOM_3A_DEBUG_DATA);
                }
            }

            entry = phyMeta->find(MI_OPERATION_MODE);
            if (entry.count) {
                int32_t opMode = entry.data.i32[0];
                MLOGD(LogGroupHAL, "[Offline DebugData][3A_Debug] phyMeta of id:%s opMode:0x%x",
                      pair.first.c_str(), opMode);
                if (phyId == getMasterPhyId()) {
                    record->collectedMeta->update(*phyMeta, MI_OPERATION_MODE);
                }
            }
        }
    }
}

// NOTE: decide which mialgo port every buffer will be sent to
void Photographer::decideMialgoPortBufferSentTo()
{
    // NOTE: decide which port buffer will be sent to
    decideMialgoPortImpl();

    // NOTE: calculate how many buffers will be sent to specific mialgo port
    for (auto &pair : mVendorRequests) {
        CaptureRequest *request = pair.second->request.get();

        for (int i = 0; i < request->getOutBufferNum(); ++i) {
            auto buffer = request->getStreamBuffer(i);

            // NOTE: mialgo port < 0  means this buffer will not be sent to mialgo
            if (buffer->getMialgoPort() < 0) {
                continue;
            }

            int mialgoPort = buffer->getMialgoPort();

            MLOGD(LogGroupHAL, "buffer from stream:%p will be sent to mialgo port:%d",
                  buffer->getStream()->toRaw(), mialgoPort);
            if (mBufferNumPerPort.count(mialgoPort)) {
                mBufferNumPerPort[mialgoPort]++;
            } else {
                mBufferNumPerPort[mialgoPort] = 1;
            }
        }
    }
}

} // namespace mihal
