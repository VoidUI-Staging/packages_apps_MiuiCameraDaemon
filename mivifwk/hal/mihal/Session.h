#ifndef __SESSION_H__
#define __SESSION_H__

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "CamGeneralDefinitions.h"
#include "Camera3Plus.h"
#include "CameraDevice.h"
#include "CameraMode.h"
#include "EventCallbackMgr.h"
#include "GraBuffer.h"
#include "LogUtil.h"
#include "Metadata.h"
#include "ThreadPool.h"
#include "VendorCamera.h"

namespace mihal {

class VendorCamera;

class Session
{
public:
    enum SessionType {
        AsyncAlgoSessionType,
        SyncAlgoSessionType,
        NonAlgoSessionType,
    };

    Session(VendorCamera *vendorCamera, uint8_t sessionType);
    virtual ~Session(){};

    virtual std::string getName() const = 0;
    std::string getLogPrefix()
    {
        if (!mLogPrefix.empty()) {
            return mLogPrefix;
        } else {
            std::ostringstream str;
            str << "[" << getName() << ":0x" << std::hex << getId() << "]";
            mLogPrefix = str.str();
            return mLogPrefix;
        }
    }

    bool inited() const { return mInited; }
    uint32_t getId() const { return mId; }
    uint8_t getSessionType() const { return mSessionType; }

    // TODO: rename to more unique ones
    // called by CameraDevice, from FW down to HAL
    virtual int processRequest(std::shared_ptr<CaptureRequest> request) = 0;
    virtual int flush();
    virtual int resetResource() { return 0; };
    virtual camera3_callback_ops *getSessionCallback() = 0;
    virtual StreamConfig *getConfig2Vendor() { return mConfigToVendor; };
    virtual void endConfig(){};

    uint32_t getSessionFrameNum() { return mFrameNumToVendor.load(); };
    void updateSessionFrameNum(uint32_t frameNum) { mFrameNumToVendor.store(frameNum); };
    uint32_t incSessionFrameNum() { return mFrameNumToVendor.fetch_add(1); }
    virtual void signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams);
    virtual camera3_buffer_request_status requestStreamBuffers(
        uint32_t num_buffer_reqs, const camera3_buffer_request *buffer_reqs,
        /*out*/ uint32_t *num_returned_buf_reqs,
        /*out*/ camera3_stream_buffer_ret *returned_buf_reqs);
    virtual void returnStreamBuffers(uint32_t num_buffers,
                                     const camera3_stream_buffer *const *buffers);

    // Called by vendor hal directly
    // virtual int processResult(CaptureResult *result);

    std::atomic_bool mFlush = false;

protected:
    VendorCamera *mVendorCamera;
    bool mInited;
    uint32_t mId;
    uint8_t mSessionType;
    StreamConfig *mConfigToVendor;
    std::atomic<uint32_t> mFrameNumToVendor;

    std::string mLogPrefix;
};

class NonAlgoSession final : public Session
{
public:
    NonAlgoSession() = delete;
    NonAlgoSession(VendorCamera *vendorCamera, std::shared_ptr<StreamConfig> config,
                   std::shared_ptr<EcologyAdapter> ecoAdapter);
    virtual ~NonAlgoSession();

    std::string getName() const override { return "NonAlgoSession"; }

    int processRequest(std::shared_ptr<CaptureRequest> request) override;
    // int processResult(CaptureResult *result) override;
    camera3_callback_ops *getSessionCallback() override { return mVendorCamera; }

private:
    std::unique_ptr<StreamConfig> mConfigFromFwk;
    std::shared_ptr<EcologyAdapter> mEcologyAdapter;
};

class AlgoSession : public Session
{
public:
    AlgoSession(VendorCamera *vendorCamera, uint8_t sessionType);
    virtual ~AlgoSession() { mVendorInflights.clear(); };
    CameraMode *getMode() { return mCameraMode.get(); };

    class RequestExecutor
    {
    public:
        enum SnapMessageType {
            HAL_BUFFER_RESULT = 0,
            HAL_SHUTTER_NOTIFY = 1,
            HAL_MESSAGE_COUNT = 2,
            HAL_FINAL_REPORT = 3,  // algofwk process  finished
            HAL_VENDOR_REPORT = 4, // received snapshot buffer from vendor hal
        };

        enum FinishStatus {
            NOTIFY_FINISH = 1,
            RESULT_FINISH = 2,
            ALL_FINISH = 3,
        };

        // NOTE: here we must clone the remote fwkRequest, since the remote one may be destroied
        RequestExecutor(const CaptureRequest *fwkRequest, Session *session);

        // if it's a LocalRequest created by the caller, then we could store it directly
        RequestExecutor(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session);
        virtual ~RequestExecutor() = default;

        virtual int execute() = 0;
        virtual int processVendorNotify(const NotifyMessage *msg)
        {
            forwardNotifyToFwk(msg);
            return 0;
        }
        virtual int processVendorResult(CaptureResult *result) = 0;
        virtual std::vector<uint32_t> getVendorRequestList() const = 0;
        virtual std::string toString() const = 0;
        virtual int processEarlyResult(std::shared_ptr<StreamBuffer> inputBuffer) { return 0; }
        virtual bool doAnchorFrameAsEarlyPicture() { return false; };
        virtual bool canAttachFwkRequest(std::shared_ptr<const CaptureRequest> request)
        {
            return false;
        }
        virtual void attachFwkRequest(std::shared_ptr<const CaptureRequest> request) {}
        virtual void removeAttachFwkRequest(uint32_t frame) {}
        virtual std::string getName() const = 0;
        virtual bool isFinalPicFinished() const { return true; }
        virtual bool testAnchoredStatus() { return true; }
        std::shared_ptr<StreamBuffer> getInternalQuickviewBuffer(uint32_t frame)
        {
            return mSession->mCameraMode->getInternalQuickviewBuffer(frame);
        }
        void returnInternalQuickviewBuffer(std::shared_ptr<StreamBuffer> streamBuffer,
                                           uint32_t frame, int64_t timestamp)
        {
            mSession->mCameraMode->returnInternalQuickviewBuffer(streamBuffer, frame, timestamp);
        }

        void returnInternalFrameTimestamp(uint32_t frame, int64_t timestamp)
        {
            mSession->mCameraMode->returnInternalFrameTimestamp(frame, timestamp);
        }
        uint32_t getId() const { return mFwkRequest->getFrameNumber(); }
        virtual std::string getImageName() const;
        virtual void signalFlush(){};
        uint8_t getSessionType() { return mSession->mSessionType; }
        int32_t getClientId() { return mSession->mClientId; }

        std::string getLogPrefix()
        {
            if (!mLogPrefix.empty()) {
                return mLogPrefix;
            } else {
                std::ostringstream str;
                str << "[" << getName() << "]"
                    << " its:[fwkFrame:" << mFwkRequest->getFrameNumber() << "]";
                mLogPrefix = str.str();
                return mLogPrefix;
            }
        }
        bool isErrorStatus() const { return mGetMockBufferFail; }
        void setGetMockBufferFail() { mGetMockBufferFail = true; }
        // NOTE: used to print frame number debug info
        std::string getFrameLog(uint32_t vndFrame)
        {
            std::ostringstream str;
            str << getLogPrefix() << " info from:[vndFrame:" << vndFrame << "]";
            return str.str();
        };

        void forceDonotAttachPreview() { mIsForceDonotAttachPreview = true; }
        bool isForceDonotAttachPreview() { return mIsForceDonotAttachPreview; }

        void resetHasSentQuickviewBufferToHal() { mHasSentQuickviewBufferToHal = false; }

        bool isPreviewExecutor() { return getName() == "Previewer"; }
        bool isSnapshotExecutor() { return !isPreviewExecutor(); }
        virtual bool isFinish(uint32_t vndFrame, int32_t *frames) = 0;

        // NOTE: used to report the processing status in mialgo
        virtual std::string mialgoProcessingStatusReport(uint32_t vndFrameNum) { return " "; }

    protected:
        uint32_t getAndIncreaseFrameSeqToVendor(uint32_t num)
        {
            return mSession->mFrameNumToVendor.fetch_add(num);
        }

        virtual int complete(uint32_t frame) { return frame; }
        virtual int goToDarkroom(uint32_t frame) { return 0; }
        void updateStatus(CaptureResult *result)
        {
            auto entry = result->findInMeta(MI_MFNR_LLS_NEEDED);
            if (entry.count) {
                mSession->mCameraMode->mLlsNeed = entry.data.u8[0];
            }
            mSession->mCameraMode->updateStatus(result);
        }
        int getCameraModeType() { return mSession->mCameraMode->getCameraModeType(); }
        int getDarkroomOutputNumber() { return mSession->mCameraMode->getDarkroomOutputNumber(); }
        std::shared_ptr<ResultData> getInvitation(uint32_t frame)
        {
            return mSession->mCameraMode->inviteForMockRequest(frame);
        }

        int darkRoomEnqueue(std::function<void()> task) { return mSession->darkRoomEnqueue(task); }

        // NOTE: notify related function
        // NOTE: util function to convert vendorMsg to fwkMsg
        // @param: 'vendorMsg' is message returned from Qcom hal
        // @param: 'fwkReq': some app streams may be overrided by mihal streams so that the hal
        // ERROR_BUFFER message may contain a stream which is unknow to app, we need to convert them
        // into streams in fwkReq
        // @ret: a msg which can be sent to fwk
        std::shared_ptr<NotifyMessage> generateFwkNotifyMessage(const NotifyMessage *vendorMsg,
                                                                const CaptureRequest *fwkReq);
        // NOTE: get the fwk stream which matches vendorStream ('match' means either fwk stream and
        // vendorStream are the same stream or fwk stream is overrided by vendorStream (e.g. fwk
        // preview stream is overrided by mihal preview stream))
        // @ret: the matched stream in fwkReq, if not found, then this function will return nullptr
        camera3_stream *getMatchedFwkStream(camera3_stream *vendorStream,
                                            const CaptureRequest *fwkReq);
        int tryNotifyToFwk(const NotifyMessage *vendorMsg, const CaptureRequest *fwkReq,
                           Mia2LogGroup logGroup, const char *logPrefix);
        int forwardNotifyToFwk(const NotifyMessage *msg);

        int forwardResultToFwk(LocalResult *result);
        void copyFromPreviewToEarlyYuv(GraBuffer *srcBuffer, GraBuffer *dstBuffer);
        int convertToJpeg(std::shared_ptr<StreamBuffer> inputStreamBuffer,
                          std::shared_ptr<StreamBuffer> outputStreamBuffer);

        /**
         *   HAL_BUFFER_RESULT  message type
         * If parallel snapshot is enabled, this function will be called when HAL returns shutter
         * notify Instead, this function is called only when HAL snapshot buffer returns
         *
         *  HAL_SHUTTER_NOTIFY message type
         * Indicates that the vendor camera returns snapshot notify
         */
        int onSnapCompleted(uint32_t frame, std::string imageName, int type)
        {
            return mSession->updatePendingSnapRecords(frame, imageName, type);
        }

        void sendAnchorFrame(int64_t timestamp, uint32_t anchorFrame, uint32_t snapshotFrame)
        {
            mSession->anchorFrameEvent(timestamp, anchorFrame, snapshotFrame);
        }

        virtual std::shared_ptr<StreamBuffer> mapToRequestedBuffer(
            uint32_t frame, const StreamBuffer *buffer) const = 0;

        // NOTE: fill fwk buffers which are from vendor hal
        // @param:
        // fwkRequest is the target fwk request
        // srcVndBuffer is the buffer we use to fill fwk req's buffer
        // outBuffers is used to collect those filled buffer and they will get forwarded to app
        // later
        int fillFwkBufferFromVendorHal(const CaptureRequest *fwkRequest,
                                       std::shared_ptr<StreamBuffer> srcVndBuffer,
                                       std::vector<std::shared_ptr<StreamBuffer>> &outBuffers);

        void markReqBufsAsSentToHal(CaptureRequest *vndRequest);

        // NOTE: get the maxJpegSize same as service to do JPEG.
        int32_t getMaxJpegSize() const { return mSession->getMaxJpegSize(); }

        void onMFNRRequestChanged() { mSession->mCameraMode->onMihalBufferChanged(); };

        float getUIZoomRatio() { return mSession->mCameraMode->getUIZoomRatio(); };

        float getFovCropZoomRatio() { return mSession->mCameraMode->getFovCropZoomRatio(); };

        bool isSupportFovCrop() { return mSession->mCameraMode->isSupportFovCrop(); };

        std::shared_ptr<const CaptureRequest> mFwkRequest;
        // NOTE: just for debug
        std::string mLogPrefix;

        // NOTE: this variable is used to denote whether this executor has successfully get mock
        // buffer
        bool mGetMockBufferFail = false;

        // NOTE: if executor doesn't sent quickview buffer to Qcom, then we need to use early pic
        // thread to fill fwk's buffers which are not sent to Qcom
        bool mHasSentQuickviewBufferToHal = false;

        bool mIsForceDonotAttachPreview = false;
        std::string mImageName;

    protected:
        AlgoSession *mSession;
    };

    struct Callback : public camera3_callback_ops
    {
        std::function<void(const camera3_capture_result *result)> processResultFunc;
        std::function<void(const camera3_notify_msg *)> notifyFunc;
        std::function<camera3_buffer_request_status(
            uint32_t num_buffer_reqs, const camera3_buffer_request *buffer_reqs,
            /*out*/ uint32_t *num_returned_buf_reqs,
            /*out*/ camera3_stream_buffer_ret *returned_buf_reqs)>
            requestStreamBuffersFunc;
        std::function<void(uint32_t num_buffers, const camera3_stream_buffer *const *buffers)>
            returnStreamBuffersFunc;
    };

    int32_t getClientId() { return mClientId; }
    void anchorFrameEvent(int64_t timestamp, uint32_t anchorFrame, uint32_t snapshotFrame);
    int getPendingFinalSnapCount() const { return mPendingMiHALSnapCount.load(); }
    void decreasePendingFinalSnapCount()
    {
        mPendingMiHALSnapCount -= 1;
        MLOGD(LogGroupHAL, "[AlgoSession]:decrease mPendingMiHALSnapCount, present count is %d",
              mPendingMiHALSnapCount.load());
    };
    int updatePendingFwkRecords(const CaptureResult *result);
    int updatePendingFwkRecords(const NotifyMessage *msg);
    int getPendingVendorSnapCount() const { return mPendingVendorHALSnapCount.load(); }
    virtual int removeFromVendorInflights(uint32_t frame);
    void signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams) override;
    camera3_buffer_request_status requestStreamBuffers(
        uint32_t num_buffer_reqs, const camera3_buffer_request *buffer_reqs,
        /*out*/ uint32_t *num_returned_buf_reqs,
        /*out*/ camera3_stream_buffer_ret *returned_buf_reqs) override;
    void returnStreamBuffers(uint32_t num_buffers,
                             const camera3_stream_buffer *const *buffers) override;
    int32_t getMaxJpegSize() const;
    int32_t mClientId;

    int32_t getPendingVendorMFNR() const { return mPendingVendorMFNRSnapCount.load(); }
    void addPendingVendorMFNR() { mPendingVendorMFNRSnapCount.fetch_add(1); }
    void subPendingVendorMFNR()
    {
        if (mPendingVendorMFNRSnapCount.fetch_sub(1) < 0) {
            MLOGD(LogGroupHAL,
                  "mPendingVendorMFNRSnapCount:%d should not be less than 0, add 1 to restore",
                  mPendingVendorMFNRSnapCount.load());
            mPendingVendorMFNRSnapCount.fetch_add(1);
        }
    };
    void waitCameramodeLoop() { return mCameraMode->waitLoop(); }
    int darkRoomEnqueue(std::function<void()> task);
    void sendNotifyPreviewFrame(uint32_t snapshotFrame)
    {
        mCameraMode->notifyLastFrame(snapshotFrame);
    }

protected:
    struct FwkRequestRecord
    {
        FwkRequestRecord() = delete;
        FwkRequestRecord(std::shared_ptr<CaptureRequest> request);
        int updateByResult(const CaptureResult *result);
        int updateByNotify(const NotifyMessage *msg);
        int flush();
        bool isCompleted() const;

        std::shared_ptr<CaptureRequest> fwkRequest;
        std::set<camera3_stream *> pendingStreams;
        uint32_t partialResult;
        bool notifyDone;
    };

    using UpdateRecordFunc = std::function<int(FwkRequestRecord *record)>;
    int processResult(CaptureResult *result);
    void notify(const NotifyMessage *msg);
    std::shared_ptr<RequestExecutor> getExecutor(uint32_t frame, bool ignore = false);
    int addToVendorInflights(std::shared_ptr<RequestExecutor> executor);
    int insertPendingFwkRecords(std::shared_ptr<CaptureRequest> request);

    int updatePendingFwkRecordsImpl(uint32_t frame, UpdateRecordFunc func);
    int flushPendingFwkRecords();
    void insertPendingSnapRecords(uint32_t frame);
    // This is the key function to control S2S
    int updatePendingSnapRecords(uint32_t frame, std::string imageName, int type);

    int createDarkroom();
    std::shared_ptr<StreamConfig> mConfigFromFwk;
    // bit0: sat camera early picture, bit1: parallel camera shutter notify
    std::mutex mRecordMutex;
    std::map<uint32_t /* frameNumber */, std::bitset<RequestExecutor::HAL_MESSAGE_COUNT>>
        mPendingSnapshotRecord;
    std::atomic<int> mPendingMiHALSnapCount;
    std::atomic<int> mPendingVendorHALSnapCount;
    std::atomic<int> mPendingVendorMFNRSnapCount;
    mutable std::mutex mSnapMutex;
    std::condition_variable_any mSnapCond;

    // NOTE: inflights that are waiting for VendorHAL to process
    // this map is used to handle the returned result: there are more than 1 results for
    // each request, some parts only have metas, mapping from framenumber to the vendor
    // request help us to tell more info about the frame/result
    std::map<uint32_t /* frameNumber */, std::shared_ptr<RequestExecutor>> mVendorInflights;
    std::map<uint32_t /* frameNumber */, std::unique_ptr<FwkRequestRecord>> mPendingFwkRecords;
    mutable std::recursive_mutex mPendingRecordsMutex;
    // NOTE: for FINAL pic with full algos applied
    std::unique_ptr<CameraMode> mCameraMode;
    // true: vendor hal anchor pick enabled.
    bool mAnchorFrameEnabled;
    std::unique_ptr<ThreadPool> mDarkRoomThread;

    mutable std::recursive_mutex mMutex;
    std::condition_variable_any mCond;
    mutable std::mutex mAttMutex;
    std::map<uint32_t /* FwkframeNumber */, uint32_t /* VendorframeNumber */> mAttachVendorFrame;
    bool mFirstFrameArrived;
    bool mFirstFrameRequested;
    // CameraApp pid
};

class AsyncAlgoSession final : public AlgoSession,
                               public std::enable_shared_from_this<AsyncAlgoSession>
{
public:
    static std::shared_ptr<AsyncAlgoSession> create(VendorCamera *vendorCamera,
                                                    std::shared_ptr<StreamConfig> config);
    AsyncAlgoSession() = delete;
    // Session(CameraDevice *vendorCamera, const StreamConfig &config, SessionCallback callbacks);
    virtual ~AsyncAlgoSession();

    std::string getName() const override { return "AsyncAlgoSession"; }

    // TODO: rename to more unique ones
    // called by CameraDevice, from FW down to HAL
    int processRequest(std::shared_ptr<CaptureRequest> request) override;
    int flush() override;
    int resetResource() override;

    int processEarlySnapshotResult(uint32_t frame, std::shared_ptr<StreamBuffer> inputBuffer,
                                   bool ignore = false);
    bool hasPendingPhotographers() const;
    void flushExecutor();

    camera3_callback_ops *getSessionCallback() override { return &mVendorCallbacks; };
    void endConfig() override;

private:
    AsyncAlgoSession(VendorCamera *vendorCamera, std::shared_ptr<StreamConfig> config);

    // Called by vendor hal directly

    void darkroomRequestLoop();
    int processVendorRequest(std::shared_ptr<CaptureRequest> request);
    std::unique_ptr<CameraMode> createCameraMode(const StreamConfig *configFromFwk);
    std::shared_ptr<RequestExecutor> createExecutor(std::shared_ptr<CaptureRequest> fwkRequest);

    StreamBuffer *getFwkSnapshotBuffer(CaptureRequest *request);
    StreamBuffer *getScanBuffer(CaptureRequest *request);

    // NOTE: set requests which are not processed as error and send results according to camera3.h
    void setReqErrorAndForwardResult(std::shared_ptr<CaptureRequest> request);

    // NOTE: for SAT SN Burst, no Down Capture
    void updateSupportDownCapture(CaptureRequest *request)
    {
        auto entry = request->findInMeta(MI_BURST_DOWN_CAPTURE);
        if (entry.count) {
            mSupportDownCapture = entry.data.i32[0];
            MLOGD(LogGroupHAL, "mSupportDownCapture %d", mSupportDownCapture);
        }
    }

    static std::atomic<int> sAsyncAlgoSessionCnt;

    uint32_t mLatestSnapVndFrame;
    bool mSupportDownCapture;

    bool mExitThread;
    // NOTE: consider for the two policy options: callback or relay?
    //       should this/these device(s) be passed from the VendorCamera
    //       or created by the session itself?
    Callback mVendorCallbacks;

    // inflights that are waiting for mock requests to reprocess in darkroom
    std::queue<std::pair<uint32_t /* frameNumber */, std::shared_ptr<Photographer>>>
        mDarkroomInflights;
    std::queue<std::tuple<uint32_t /* firstFrame */, std::shared_ptr<StreamConfig>>>
        mDarkroomConfigs;
    std::queue<std::shared_ptr<PreProcessConfig>> mDarkroomPreProcessConfigs;
    std::queue<std::tuple<uint32_t /* firstFrame */, std::string, std::shared_ptr<StreamConfig>>>
        mDarkroomQuickFinishTasks;

    // start the threads when the first snapshot request comes
    // FIXME: let's try to remove mDarkroomThread once the getBuffer() mechanism is ready
    std::thread mDarkroomThread;

    int32_t mTotalJpegBurstNum;
    int32_t mCurrentJpegBurstNum;
};

class SessionManager final
{
public:
    static SessionManager *getInstance();
    ~SessionManager();

    int registerSession(std::shared_ptr<AsyncAlgoSession> session);
    int unregisterSession(std::shared_ptr<AsyncAlgoSession> session);

    std::shared_ptr<AsyncAlgoSession> getSession(uint32_t sessionId = -1) const;

    void signalReclaimSession();

    static void onClientEvent(EventData data);

private:
    SessionManager();

    // NOTE: monitor offline session state and reclaim them when they have no pending in-progress
    // executor
    void sessionReclaimLoop();

    // NOTE: used in some locked context
    void signalReclaimSessionWithoutLock();

    // Terminate the requestStreambuffers of Photographer
    void flushSessionExecutor(int32_t clientId);

    // FIXME: is it possible to have more than one active session?
    std::map<uint32_t /* session ID */, std::shared_ptr<AsyncAlgoSession>> mActiveSessions;
    std::map<uint32_t /* session ID */, std::shared_ptr<AsyncAlgoSession>> mOfflineSessions;
    mutable std::mutex mMutex;
    mutable std::condition_variable mCond;

    // NOTE: thread used to reclaim offline session
    std::thread mReclaimThread;
    std::condition_variable mReclaimCond;
    bool mExitReclaimThread;

    std::unique_ptr<ThreadPool> mDarkRoomClearThread;
};

class SyncAlgoSession : public AlgoSession, public std::enable_shared_from_this<SyncAlgoSession>
{
public:
    SyncAlgoSession() = delete;
    SyncAlgoSession(VendorCamera *vendorCamera, std::shared_ptr<StreamConfig> config,
                    std::shared_ptr<EcologyAdapter> ecoAdapter);
    virtual ~SyncAlgoSession();

    std::string getName() const override { return "SyncAlgoSession"; }

    int processRequest(std::shared_ptr<CaptureRequest> request) override;
    // int processResult(CaptureResult *result) override;
    camera3_callback_ops *getSessionCallback() override { return &mVendorCallbacks; }
    void endConfig() override;
    int flush() override;

private:
    Callback mVendorCallbacks;
    std::unique_ptr<CameraMode> createCameraMode(const StreamConfig *configFromFwk);
    std::shared_ptr<EcologyAdapter> mEcologyAdapter;
};

} // namespace mihal

#endif
