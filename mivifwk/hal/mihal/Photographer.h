#ifndef __PHOTOGRAPHER_H__
#define __PHOTOGRAPHER_H__

#include <bitset>
#include <mutex>
#include <set>

#include "Reprocess.h"
#include "Session.h"

namespace mihal {

// pronounce: pho'tographer, the most simple photographer: only one request with only
// one extra snapshot stream added to the original request, but may have some preview
// streams(preview/scan/...)
class Photographer : public enable_shared_from_this<Photographer>,
                     public AlgoSession::RequestExecutor
{
public:
    enum PgState {
        PG_Staging,
        PG_Complete,
        PG_Cancelled,
    };

    // NOTE: mivistreamformat.json uses the definition here
    enum PhotographerType {
        Photographer_SIMPLE = 0,
        Photographer_HDR = 1,
        Photographer_SR = 2,
        Photographer_SN = 3,
        Photographer_BURST = 4,
        Photographer_HDRSR = 5,
        Photographer_FUSION = 6,
        Photographer_SR_FUSION = 7,
        Photographer_VENDOR_MFNR = 8,
        Photographer_MIHAL_MFNR = 9,
        Photographer_BOKEH = 10,
        Photographer_BOKEH_MD = 11,
        Photographer_BOKEH_HDR = 12,
        Photographer_BOKEH_SE = 13,
        Photographer_HD_REMOSAIC = 14,
        Photographer_HD_UPSCALE = 15,
        Photographer_MANUAL = 16,
    };

    // detail: https://xiaomi.f.mioffice.cn/sheets/shtk4ewHKIRIuap1UmcEm1KoI8b
    enum AnchorFrameMask {
        FRONT_NORMAL = 0x1,
        FRONT_PORTRAIT = 0x2,
        FRONT_HDR = 0x4,
        FRONT_MFNR = 0x8,
        ANCHOR_MANUAL = 0x10,
        BACK_NORMAL = 0x100,
        BACK_SR = 0x200,
        BACK_PORTRAIT = 0x400,
        BACK_HD_UPSCALE = 0x800,
        BACK_HD_REMOSAIC = 0x1000,
        BACK_HDR = 0x2000,
        BACK_SUPER_NIGHT = 0x4000,
        BACK_AINR = 0x8000,
        BACK_MFNR = 0x10000,
        BACK_FUSION = 0x20000,
    };

    Photographer() = delete;

    Photographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                 const Metadata &meta, std::shared_ptr<StreamConfig> darkroomConfig = nullptr);

    Photographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                 std::shared_ptr<StreamConfig> darkroomConfig = nullptr);

    // in case of using as concreted class for simple snapshot
    Photographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                 std::shared_ptr<Stream> extraStream, Metadata extraMetadata = {},
                 std::shared_ptr<StreamConfig> darkroomConfig = nullptr);

    virtual ~Photographer();

    int execute() override;
    int processVendorNotify(const NotifyMessage *msg) override;
    int processVendorResult(CaptureResult *result) override;
    std::vector<uint32_t> getVendorRequestList() const override;
    std::string toString() const override;
    int processEarlyResult(std::shared_ptr<StreamBuffer> inputBuffer) override;
    bool doAnchorFrameAsEarlyPicture() override { return mAnchorFrameQuickView || mIsMiviAnchor; }
    bool canAttachFwkRequest(std::shared_ptr<const CaptureRequest> request) override;
    bool supportAttachFwkPreviewReq();
    void attachFwkRequest(std::shared_ptr<const CaptureRequest> request) override;
    void removeAttachFwkRequest(uint32_t frame) override;
    bool isFinish(uint32_t vndFrame, int32_t *frames) override;
    virtual std::shared_ptr<CaptureRequest> buildRequestToDarkroom(uint32_t frame);
    virtual void buildPreRequestToDarkroom();
    bool testAnchoredStatus()
    {
        mAnchored = true;
        return mAnchorFlag.test_and_set();
    }
    // XXX: there maybe race condition between attachMockRequest and mockRequestAttached
    // let's fix it in the final version with asnc-ly getting buffer

    std::string getName() const override { return "Photographer"; }
    bool isFinalPicFinished() const override;
    std::shared_ptr<StreamBuffer> requestStreambuffers(uint32_t frame, uint32_t streamIdx);
    void signalFlush() override;
    // NOTE: monitor requests sent between mihal and mialgo
    // if isSendToMialgo is true, then a new task(aka: all reqeusts in a photographer) has been sent
    // from mihal to mialgo if isSendToMialgo is false, then a task is processed done and sent from
    // mialgo to mihal
    void MonitorMialgoTask(bool isSendToMialgo);

    inline void setPhtgphType(int type) { mPhtgphType = type; }
    inline int getPhtgphType() const { return mPhtgphType; }
    void enableShotWithDealyTime(uint64_t delayTime);
    void enableDrop() { mDropEnable = true; }
    bool isDropped() { return mDropEnable == true; }
    void updateAnchorFrameQuickView();

    bool needWaitQuickShotTimerDone() { return (mQuickShotEnabled && !mQuickShotTimerDone); }

    // NOTE: determine whether bokeh mfnr is supported
    int isSupportMfnrBokeh();

    // NOTE: to check whether vndFrameNum's RequestRecord still has mihal buffer pending in mialgo

    // NOTE: print the mialgo processing status of vndFrameNum's RequestRecord
    virtual std::string mialgoProcessingStatusReport(uint32_t vndFrameNum)
    {
        std::ostringstream str;
        str << ", check vndFrame:" << vndFrameNum << " 's mialgo processing status: ";
        str << "is error status: " << isErrorStatus();
        return str.str();
    }

    void setMasterPhyId(uint32_t id) { mMasterPhyId = id; }

    bool isMFNRHDR() { return mIsMFNRHDR == true; }

    // NOTE: decide which mialgo port every buffer will be sent to
    void decideMialgoPortBufferSentTo();

    inline void setQuickShotTimerDone() { mQuickShotTimerDone = true; }

protected:
    struct RequestRecord
    {
        RequestRecord(Photographer *phtgph, const CaptureRequest *fwkRequest, uint32_t frameNumber);
        RequestRecord(Photographer *phtgph, std::unique_ptr<CaptureRequest> vendorRequest);

        void initPendingSet();

        std::string onelineReport()
        {
            std::ostringstream str;
            str << "fullmeta:" << isFullMeta << " pending:" << pendingBuffers;
            return str.str();
        }

        std::unique_ptr<CaptureRequest> request;
        uint32_t pendingMetaCount;
        bool isFullMeta;
        bool isErrorMsg;
        bool isReportError;
        std::atomic<int64_t> resultTs;
        std::bitset<16> pendingBuffers;
        std::unique_ptr<Metadata> collectedMeta;
        PhysicalMetas phyMetas;
        uint32_t darkroomOutNum;
        std::shared_ptr<StreamBuffer> cachedForAttachedPreview;
        Photographer *photographer;
        mutable std::mutex mMutex;
        std::atomic_int mStatus;
    };

    virtual int processMetaResult(CaptureResult *result);
    virtual int processBufferResult(const CaptureResult *result);
    virtual int processErrorResult(CaptureRequest *vendorRequest);
    virtual void getErrorBuffers(std::vector<std::shared_ptr<StreamBuffer>> &buffers)
    {
        // NOTE: if we don't send quickview buffer to Qcom, we need to set fwk buffers which are not
        // sent to Qcom as error
        if (!mHasSentQuickviewBufferToHal) {
            setErrorRealtimeBuffers(buffers);
        }
    }
    virtual int complete(uint32_t frame);
    virtual bool skipQuickview() { return true; }
    virtual bool isSupernightEnable() { return false; }
    int collectMetadata(const CaptureResult *result);
    RequestRecord *getRequestRecord(uint32_t frame) const;

    void setErrorRealtimeBuffers(std::vector<std::shared_ptr<StreamBuffer>> &buffers);
    int getAnchorFrameMaskBit();
    void mergeForceMetaToDarkroom(const CaptureRequest *src, LocalRequest *dest) const;
    void mergeForceMetaToMockResult(const CaptureRequest *src,
                                    std::shared_ptr<LocalResult> dest) const;
    uint32_t getFwkFrameNumber(uint32_t frame);
    void getAttachedFwkBuffers(uint32_t frame, std::shared_ptr<StreamBuffer> srcStreamBuffer,
                               std::vector<std::shared_ptr<StreamBuffer>> &outBuffers);
    int sendResultForAttachedPreview(uint32_t frame);
    int goToDarkroom(uint32_t frame) override;
    std::shared_ptr<StreamBuffer> mapToRequestedBuffer(uint32_t frame,
                                                       const StreamBuffer *buffer) const override;
    int64_t buildTimestamp();

    std::map<uint32_t /* frame number */, std::unique_ptr<RequestRecord>> mVendorRequests;
    std::mutex mAttachedFwkReqMutex;
    std::map<uint32_t, std::shared_ptr<const CaptureRequest>> mAttachedFwkReq;
    std::map<uint32_t /* frame number */, int64_t /* timestamp */> mAttachedPreviewTs;
    // NOTE: there may be mutiple notifies for each fwk frame
    std::map<uint32_t /*fwk frame num*/, std::vector<std::shared_ptr<NotifyMessage>>>
        mNotifyForComingPreviews;

    Metadata mExtraMetadata;
    int mAnchorFrameMask;
    bool mAnchorFrameQuickView;
    int mPhtgphType;
    bool mIsMiviAnchor;
    std::shared_ptr<StreamConfig> mDarkroomConfig;
    mutable std::recursive_mutex mMutex;
    bool mQuickShotEnabled;
    uint64_t mQuickShotDelayTime;
    std::shared_ptr<Timer> mQuickShotTimer;
    // NOTE: FakeQuickShot like MiviQuickShot, make shot2shot in last shutter return.
    bool mFakeQuickShotEnabled;

    std::atomic<bool> mDropEnable;
    bool mIsMFNRHDR; // HDR Ev[0] from mfnr
    int mStartedNum;
    int32_t mMialgoNum = 0;

    // NOTE: record how many buffers will be sent to specific mialgo port
    std::map<int /*mialgo port*/, int /*number of buffer*/> mBufferNumPerPort;

private:
    int setupMetaForRequestToDarkroom(LocalRequest *request);
    int asyncConfigDarkroom();
    // NOTE: get the internal vendor frame for the attach preview
    uint32_t getVndFrameNumber(uint32_t frame);
    int sendResultForAttachedPreviewImpl(const CaptureRequest *request,
                                         const RequestRecord *record);
    void earlypicTimer();

    std::shared_ptr<CaptureRequest> buildSyncRequestToDarkroom(uint32_t frame);
    std::shared_ptr<CaptureRequest> buildAsyncRequestToDarkroom(uint32_t frame);

    void update3ADebugData(RequestRecord *record);
    uint32_t getMasterPhyId() { return mMasterPhyId; }

    uint32_t findAnchorVndframeByTs(int64_t ts)
    {
        return mSession->getMode()->findAnchorVndframeByTs(ts);
    }

    // NOTE: default implementation, the mialgo port this buffer will be sent to is the same order
    // in which this buffer appears in the capture request this buffer belongs to.
    virtual void decideMialgoPortImpl()
    {
        for (auto &pair : mVendorRequests) {
            CaptureRequest *request = pair.second->request.get();

            int mialgoPort = 0;
            for (int i = 0; i < request->getOutBufferNum(); ++i) {
                auto buffer = request->getStreamBuffer(i);

                // NOTE: for every internal snapshot buffer in each request, 0th buffer will be sent
                // to 0th mialgo port, 1st will be sent to the 1st mialgo port
                if (buffer->getStream()->isInternal() && !buffer->isQuickview()) {
                    buffer->setMialgoPort(mialgoPort);
                    mialgoPort++;
                }
            }
        }
    }

    std::atomic<bool> mFinalFinished;
    std::atomic<bool> mQuickShotTimerDone;
    bool mFLush;
    std::atomic<bool> mAnchored = false;
    std::atomic_flag mAnchorFlag = ATOMIC_FLAG_INIT;
    std::shared_ptr<Timer> mEarlypicTimer = Timer::createTimer();
    std::string mSignature;
    std::shared_ptr<ResultData> mInvitation;
    uint64_t mFakeFwkFrameNum;

    // NOTE: used to count how many request are actually sent to darkroom, when all the requests are
    // sent to mialgoengine, we can notify mialgoTaskMonitor that a new task has been sent to mialgo
    uint32_t mSentToDarkroomReqCnt = 0;

    std::string mMialgoSignature;
    bool mHasErrorNotifyToMialgo = false;
    std::atomic<bool> mEarlypicDone = false;
    std::atomic_flag mEarlypicFlag = ATOMIC_FLAG_INIT;
    std::atomic<bool> mInvite = false;
    std::atomic<int> mFirstFinish = 0;
    uint32_t mMasterPhyId;
    std::atomic<int> mAttachedCount;
    std::function<void(int64_t)> mAnchorCB;
};

} // namespace mihal

#endif
