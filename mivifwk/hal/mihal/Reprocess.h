#ifndef __REPROCESSS_CAMERA_N_H__
#define __REPROCESSS_CAMERA_N_H__

#include <MiaPostProcIntf.h>

#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <mutex>

#include "AppSnapshotController.h"
#include "CameraDevice.h"
#include "MiviSettings.h"
#include "ResultCallback.h"
#include "ThreadPool.h"

namespace mihal {

using namespace mialgo2;

class PostProcessor;
class PostProcCallback : public MiaPostProcSessionCb
{
public:
    PostProcCallback(PostProcessor *postprocessor) : MiaPostProcSessionCb()
    {
        mProcessor = postprocessor;
    }
    void onSessionCallback(MiaResultCbType type, uint32_t frame, int64_t timeStamp,
                           void *msgData) override;

private:
    PostProcessor *mProcessor;
};

class BufferCallback : public MiaFrameCb
{
public:
    BufferCallback(PostProcessor *postprocessor) : MiaFrameCb() { mProcessor = postprocessor; }
    void release(uint32_t frame, int64_t timeStampUs, MiaFrameType type, uint32_t streamIdx,
                 buffer_handle_t handle) override;
    int getBuffer(int64_t timeStampUs, OutSurfaceType type, buffer_handle_t *buffer) override;

private:
    PostProcessor *mProcessor;
};
class PostProcessor : public std::enable_shared_from_this<PostProcessor>
{
public:
    enum CameraCombinationMode {
        CAM_COMBMODE_UNKNOWN = 0x0,
        CAM_COMBMODE_REAR_WIDE = 0x01,
        CAM_COMBMODE_REAR_TELE = 0x02,
        CAM_COMBMODE_REAR_ULTRA = 0x03,
        CAM_COMBMODE_REAR_MACRO = 0x04,
        CAM_COMBMODE_FRONT = 0x11,
        CAM_COMBMODE_FRONT_AUX = 0x12,
        CAM_COMBMODE_REAR_SAT_WT = 0x201,
        CAM_COMBMODE_REAR_SAT_WU = 0x202,
        CAM_COMBMODE_FRONT_SAT = 0x203,
        CAM_COMBMODE_REAR_SAT_T_UT = 0x204,
        CAM_COMBMODE_REAR_BOKEH_WT = 0x301,
        CAM_COMBMODE_REAR_BOKEH_WU = 0x302,
        CAM_COMBMODE_FRONT_BOKEH = 0x303,
        CAM_COMBMODE_REAR_SAT_UW_W = 0x304,
    };

    enum ErrorState {
        ReprocessSuccess,
        ReprocessError,
        ReprocessCancelled = 2,
    };

    enum FinishState {
        NoFinish,
        FinishError,
        FinishSuccess = 2,
    };

    PostProcessor(
        const std::string signature, std::shared_ptr<StreamConfig> config,
        std::chrono::milliseconds waitms,
        std::function<void(const std::string &sessionName)> monitorImageCategory = nullptr);
    ~PostProcessor();
    void setInvitationInfo(std::shared_ptr<ResultData> Info, std::shared_ptr<Metadata> meta,
                           std::function<void(uint64_t)> fRemove);
    void preProcessCaptureRequest(std::shared_ptr<PreProcessConfig> config);
    void processInputRequest(std::shared_ptr<CaptureRequest> request, bool first,
                             std::function<void(int64_t)> fAnchor,
                             std::function<void(const CaptureResult *, bool)> fFeedback,
                             uint64_t fwkFrameNum, bool isEarlyPicDone);
    static void processErrorRequest(
        const std::shared_ptr<CaptureRequest> &request, uint64_t fwkFrameNum,
        std::function<void(const CaptureResult *, bool)> fFeedback = nullptr);
    void processOutputRequest(std::shared_ptr<CaptureRequest> request, uint64_t fwkFrameNum);
    int quickFinishTask(uint64_t fwkFrameNum, const std::string fileName, bool needResult);
    void flush(bool cancel = false);
    bool isBusy();
    void setExifInfo(int64_t timestamp, std::string exifinfo);
    void setAnchor(int64_t timestamp);
    void setError(ErrorState errorState, int64_t timestamp);
    void processMialgoResult(uint32_t frame, int64_t timeStampUs, MiaFrameType type,
                             uint32_t streamIdx, buffer_handle_t handle);
    int getBuffer(int64_t timeStampUs, OutSurfaceType type, buffer_handle_t *buffer);
    void removeMission(uint64_t fwkFrameNum);
    void inviteForMockRequest(uint64_t fwkFrameNum, const std::string &imageName, bool invite);
    void notifyBGServiceHasDied(int32_t clientId);

    std::atomic<bool> mFlushed;
    std::atomic_flag mFlushFlag = ATOMIC_FLAG_INIT;

private:
    typedef std::map<buffer_handle_t, std::shared_ptr<StreamBuffer>> *INPUTBUFF;
    uint32_t getCameraModeByRoleId(int32_t roleId, const Metadata *meta);
    uint32_t getOpModeByRoleId(int32_t roleId) { return 0; };
    void buildInputFrame(INPUTBUFF inputBuffers, const std::shared_ptr<CaptureRequest> &request,
                         MiaFrame &inFrame, uint32_t index);
    void convert2MiaImage(MiaFrame &image, std::shared_ptr<StreamBuffer> &streamBuffer);
    struct RequestRecord
    {
        RequestRecord()
            : setInfo(false),
              errorState(ReprocessSuccess),
              cameraRequest(nullptr),
              finishResult(NoFinish),
              invited(false),
              index(0),
              fAnchor(nullptr),
              referenceFrameIndex(-1),
              useLastEv0(false),
              isEarlyPicDone(false),
              flush(false){};
        ~RequestRecord();
        std::shared_ptr<CaptureRequest> cameraRequest;
        std::set<int64_t> timestamps; // first frame ts
        std::map<uint32_t /*streamIdx*/, std::shared_ptr<StreamBuffer>> outputBuffers;
        ErrorState errorState;
        std::string exifInfo;
        std::shared_ptr<ResultData> mInvitationInfo;
        uint32_t referenceFrameIndex;
        std::map<uint32_t, std::unique_ptr<Metadata>> allMeta;
        std::unique_ptr<Metadata> meta;       // first frame meta
        std::function<void(int64_t)> fAnchor; // Anchor callback
        std::function<void(const CaptureResult *, bool)> fFeedback;
        std::function<void(uint64_t)> fRemove; // remove mBusyPostProcessors
        std::set<uint32_t> inputFrame;
        std::map<buffer_handle_t, std::shared_ptr<StreamBuffer>> inputBuffers;
        bool setInfo;
        std::mutex mMutex;
        mutable std::condition_variable mCond;
        FinishState finishResult;
        std::weak_ptr<PostProcessor> weakThis;
        bool invited;
        bool flush;
        uint32_t index;
        bool useLastEv0;
        bool isEarlyPicDone;
    };

    void updateReferenceFrameIndex(std::shared_ptr<CaptureRequest> request,
                                   std::shared_ptr<RequestRecord> tempRecord);

    void *mImpl;
    std::string mSignature;
    std::unique_ptr<PostProcCallback> mPostProcCallback;
    std::unique_ptr<BufferCallback> mBufferCallback;
    std::chrono::milliseconds mWaitMillisecs;
    std::unique_ptr<ThreadPool> mThread;

    bool mInitDone;
    bool mRaisePriority;
    std::mutex mInitMutex;
    mutable std::condition_variable mInitCond;

    std::function<void(const std::string &sessionName)> mMonitorImageCategory;

public:
    std::mutex mRequestMutex;
    std::map<uint64_t /* Fwk frame number */, std::shared_ptr<RequestRecord>> mOutputRequests;
};

class PostProcessorManager
{
public:
    static PostProcessorManager &getInstance();
    ~PostProcessorManager();
    void configureStreams(
        std::string signature, std::shared_ptr<StreamConfig> config,
        std::shared_ptr<ResultData> Info = nullptr, std::shared_ptr<Metadata> meta = nullptr,
        std::function<void(const std::string &sessionName)> monitorImageCategory = nullptr);
    void preProcessCaptureRequest(uint64_t fwkFrameNum, std::shared_ptr<PreProcessConfig> config);
    void processInputRequest(std::string signature, std::shared_ptr<CaptureRequest> request,
                             bool first, std::function<void(int64_t)> fAnchor,
                             std::function<void(const CaptureResult *, bool)> fFeedback,
                             uint64_t fwkFrameNum, bool isEarlyPicDone);
    void processOutputRequest(std::shared_ptr<CaptureRequest> request, uint64_t fwkFrameNum);
    void quickFinishTask(uint64_t fwkFrameNum, std::string fileName, bool needResult);
    void flush();
    void flush(std::string signature);
    void clear();
    void removeMission(uint64_t fwkFrameNum);
    void inviteForMockRequest(uint64_t fwkFrameNum, const std::string &imageName, bool invite);
    void notifyBGServiceHasDied(int32_t clientId);

private:
    PostProcessorManager();
    PostProcessorManager(const PostProcessorManager &) = delete;
    PostProcessorManager &operator=(const PostProcessorManager &) = delete;
    void reclaimLoop();

    std::mutex mAlgoMutex;
    std::map<std::string /* mialgo signature */, std::shared_ptr<PostProcessor>> mPostProcessors;
    std::mutex mCountMutex;
    std::map<std::string /* mialgo signature */, uint32_t> mMissionCount;
    std::mutex mBusyMutex;
    std::map<uint64_t /* frame_number */, std::pair<std::string, std::weak_ptr<PostProcessor>>>
        mBusyPostProcessors;
    std::chrono::milliseconds mWaitMillisecs;
    std::thread mThread;
    mutable std::condition_variable mReclaimCond;
    std::string mCurrentSig;
    std::atomic_bool mQuit;
    // Callback when the PostProcessor::RequestRecord is destructed, remove the old algorithm that
    // is not used
    std::function<void(uint64_t)> mfRemove;
};

#define DARKROOM PostProcessorManager::getInstance()

} // namespace mihal

#endif