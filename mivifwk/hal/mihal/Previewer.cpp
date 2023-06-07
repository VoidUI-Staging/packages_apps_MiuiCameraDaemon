#include "Previewer.h"

#include <sstream>

#include "LogUtil.h"
#include "VendorCamera.h"

namespace mihal {

Previewer::Previewer(const CaptureRequest *fwkRequest, Session *session, bool isLowCapability)
    : RequestExecutor{fwkRequest, session},
      mIsFullMeta{false},
      mIsLowCapability{isLowCapability},
      mResultTs{-1}
{
    init();
}

Previewer::Previewer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                     bool isLowCapability)
    : RequestExecutor{fwkRequest, session},
      mIsFullMeta{false},
      mIsLowCapability{isLowCapability},
      mResultTs{-1}
{
    init();
}

void Previewer::init()
{
    uint32_t frame = getAndIncreaseFrameSeqToVendor(1);
    mStatus.store(0);
    std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
    if (mSession->getSessionType() != Session::SyncAlgoSessionType) {
        auto internalBuffer = getInternalQuickviewBuffer(frame);
        outBuffers.push_back(internalBuffer);
    }

    // set the bitsets for the requested stream buffers
    for (int i = 0; i < mFwkRequest->getOutBufferNum(); i++) {
        auto streamBuffer = mFwkRequest->getStreamBuffer(i);
        auto stream = streamBuffer->getStream();
        if (streamBuffer->isSnapshot()) {
            mPendingBuffers.set(stream->getStreamId());
            MLOGI(LogGroupRT, "parallel early snapshot frame: %d", frame);
            continue;
        }

        // NOTE: we only take non-overrided fwk buffer when Qcom preview pipeline can output more
        // than one ports
        if (streamBuffer->getStream()->isConfiguredToHal() && !mIsLowCapability) {
            outBuffers.push_back(streamBuffer);
        }
    }

    Metadata metadata = {};
    if (!mFwkRequest->getMeta()->isEmpty()) {
        metadata = *mFwkRequest->getMeta();
    }

    MLOGD(LogGroupRT, "%s: incoming [Preview], Qcom preview pipeline will output %s port(s)",
          getFrameLog(frame).c_str(), mIsLowCapability ? "One" : "Muti");
    mVendorRequest =
        std::make_unique<LocalRequest>(mFwkRequest->getCamera(), frame, outBuffers, metadata);

    initPendingSet();
}

void Previewer::initPendingSet()
{
    // set the bitsets for the requested stream buffers
    for (int i = 0; i < mVendorRequest->getOutBufferNum(); i++) {
        auto streamBuffer = mVendorRequest->getStreamBuffer(i);
        // auto stream = streamBuffer->getStream();
        auto stream = streamBuffer->getStream();
        mPendingBuffers.set(stream->getStreamId());
    }
}

int Previewer::execute()
{
    markReqBufsAsSentToHal(mVendorRequest.get());

    VendorCamera *camera = reinterpret_cast<VendorCamera *>(mVendorRequest->getCamera());
    camera->submitRequest2Vendor(mVendorRequest.get());
    return 0;
}

std::shared_ptr<StreamBuffer> Previewer::mapToRequestedBuffer(uint32_t frame,
                                                              const StreamBuffer *buffer) const
{
    return mVendorRequest->getBufferByStream(buffer);
}

int Previewer::processVendorResult(CaptureResult *result)
{
    MLLOGI(LogGroupRT, result->toString(),
           "%s: get result of:", getFrameLog(result->getFrameNumber()).c_str());

    // NOTE: the created request to vendor shares the same buffer with the one from framework
    // XXX: the result from vendor could't be sent to framework directly
    // 1. the frame number maybe different
    // 2. some streams in ConfigToVendor are not necessary to be sent to framework
    uint32_t frame = result->getFrameNumber();
    result->setFrameNumber(mFwkRequest->getFrameNumber());

    auto entry = result->findInMeta(ANDROID_SENSOR_TIMESTAMP);
    if (entry.count) {
        mResultTs = entry.data.i64[0];
    }

    std::vector<std::shared_ptr<StreamBuffer>> outFwkBuffers;
    for (int i = 0; i < result->getBufferNumber(); i++) {
        // NOTE: for internal buffer, mind the StreamBuffer object in result is not the
        //       one from request
        auto streamBuffer = mapToRequestedBuffer(frame, result->getStreamBuffer(i).get());
        MASSERT(streamBuffer);

        auto stream = streamBuffer->getStream();
        MLOGD(LogGroupRT, "%s: process preview buffer, its stream: %s", getFrameLog(frame).c_str(),
              stream->toString().c_str());
        mPendingBuffers.reset(stream->getStreamId());
        if (stream->isQuickview()) {
            // NOTE: by now, quickview buffer is the buffer we choose to fill the fwk buffers which
            // are not sent to Qcom
            fillFwkBufferFromVendorHal(mFwkRequest.get(), streamBuffer, outFwkBuffers);
            returnInternalQuickviewBuffer(streamBuffer, frame, mResultTs);
        } else {
            // NOTE: for other buffers which are bypased to Qcom, we can return them to app directly
            outFwkBuffers.push_back(streamBuffer);
        }
    }

    if (outFwkBuffers.size() || result->getPartial()) {
        // NOTE: wrap Metadata around camera3 metadata to make a LocalResults, this wrap won't clone
        // meta data
        auto toAppMeta = std::make_unique<Metadata>(result->toRaw()->result);
        LocalResult fwkResult{mFwkRequest->getCamera(), mFwkRequest->getFrameNumber(),
                              result->getPartial(), outFwkBuffers, std::move(toAppMeta)};
        forwardResultToFwk(&fwkResult);
    }

    result->setFrameNumber(frame);
    int32_t partial = result->getPartial();
    if (partial) {
        updateStatus(result);
        if (partial == result->getCamera()->getPartialResultCount())
            mIsFullMeta = true;
    }

    // check whether all the request buffers are returned
    std::ostringstream str;
    str << "fullmeta:" << mIsFullMeta << " pending:" << mPendingBuffers;
    MLOGD(LogGroupRT, "%s: finish processing this preview result, now: %s",
          getFrameLog(frame).c_str(), str.str().c_str());
    if (mIsFullMeta && mPendingBuffers.none()) {
        mStatus.store(mStatus.load() | RESULT_FINISH);
        complete(frame);
        return frame;
    }

    return frame;
}

std::vector<uint32_t> Previewer::getVendorRequestList() const
{
    return std::vector<uint32_t>{mVendorRequest->getFrameNumber()};
}

std::string Previewer::toString() const
{
    std::ostringstream str;

    str << "%s for camera " << getName().c_str() << mFwkRequest->getCamera()->getId()
        << ", fwk/vnd frame=(" << mFwkRequest->getFrameNumber() << ", "
        << mVendorRequest->getFrameNumber() << "), "
        << "with " << mVendorRequest->getOutBufferNum() << " buffers";

    return str.str();
}

int Previewer::processVendorNotify(const NotifyMessage *msg)
{
    MLOGD(LogGroupRT, "%s[Preview][Notify]: message from vendor: %s",
          getFrameLog(msg->getFrameNumber()).c_str(), msg->toString().c_str());

    if (msg->toRaw()->type == CAMERA3_MSG_SHUTTER) {
        mResultTs = (msg->getTimeStamp() != 0) ? (msg->getTimeStamp()) : -1;
    }

    tryNotifyToFwk(msg, mFwkRequest.get(), LogGroupRT, "[Preview]");
    mStatus.store(mStatus.load() | NOTIFY_FINISH);
    return msg->getFrameNumber();
}

int Previewer::processEarlyResult(std::shared_ptr<StreamBuffer> inputBuffer)
{
    int ret = 0;
    std::shared_ptr<StreamBuffer> snapshotBuffer;
    for (int i = 0; i < mFwkRequest->getOutBufferNum(); i++) {
        auto streamBuffer = mFwkRequest->getStreamBuffer(i);
        if (streamBuffer->isSnapshot()) {
            snapshotBuffer = streamBuffer;
            break;
        }
    }

    if (!snapshotBuffer) {
        MLOGE(LogGroupHAL, "no snapshot stream buffer int mFwkRequest");
        return -1;
    }

    snapshotBuffer->tryToCopyFrom(*inputBuffer, mFwkRequest->getMeta());

    if (ret != 0) {
        MLOGE(LogGroupHAL, "process fail ret; %d", ret);
        return ret;
    }

    std::vector<std::shared_ptr<StreamBuffer>> outFwkBuffers;
    outFwkBuffers.push_back(snapshotBuffer);

    uint32_t frame = mFwkRequest->getFrameNumber();
    LocalResult fResult{mFwkRequest->getCamera(), frame, 0, outFwkBuffers};
    MLLOGI(LogGroupHAL, fResult.toString(), "earlyReport: ");

    forwardResultToFwk(&fResult);
    onSnapCompleted(frame, getImageName(), RequestExecutor::HAL_SHUTTER_NOTIFY);

    auto stream = snapshotBuffer->getStream();
    mPendingBuffers.reset(stream->getStreamId());
    if (mIsFullMeta && mPendingBuffers.none()) {
        complete(frame);
    }

    return -1;
}

bool Previewer::isFinish(uint32_t vndFrame, int32_t *frames = nullptr)
{
    frames[0] = vndFrame;
    frames[1] = -1;
    if (mStatus.load() == ALL_FINISH)
        returnInternalFrameTimestamp(vndFrame, mResultTs);
    return mStatus.load() == ALL_FINISH;
}

} // namespace mihal
