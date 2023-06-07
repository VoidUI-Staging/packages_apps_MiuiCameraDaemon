/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _MIZONE_MIALGOCAMMODE_H_
#define _MIZONE_MIALGOCAMMODE_H_

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>

#include "MiCamMode.h"
#include "MiCamPerfLock.h"
#include "MiCamPerfManager.h"
#include "MiImageBuffer.h"
#include "MiZoneTypes.h"

namespace mizone {

class MiCamDevCallback;
class MiVendorCamera;

class MiCamDecision;
class AlgoEngine;
class AppSnapshotController;
class MiBufferManager;
class DecoupleUtil;
class MiCapture;
struct MiConfig;
struct MiStream;

struct MiResult
{
    uint32_t frameNum;
    std::shared_ptr<MiCapture> capture;

    std::shared_ptr<CaptureResult> result;
    std::shared_ptr<NotifyMsg> msg;
};

struct FwkRequestRecord
{
    FwkRequestRecord(const CaptureRequest &request)
    {
        fwkFrameNum = request.frameNumber;
        fwkRequest = request;
        for (auto &&buffer : request.outputBuffers) {
            pendingStreams[buffer.streamId] = true;
            outstandingBuffers[buffer.streamId] = 0;
            if (buffer.bufferId != 0 && buffer.buffer != nullptr) {
                outstandingBuffers[buffer.streamId]++;
            }
        }
        for (auto &&buffer : request.inputBuffers) {
            pendingStreams[buffer.streamId] = true;
            outstandingBuffers[buffer.streamId] = 0;
            if (buffer.bufferId != 0 && buffer.buffer != nullptr) {
                outstandingBuffers[buffer.streamId]++;
            }
        }
        isFullMetadata = false;
    };
    uint32_t fwkFrameNum;
    CaptureRequest fwkRequest;
    MiMetadata result;
    MiMetadata halResult;
    uint64_t shutter;
    bool isFullMetadata;
    std::map<int32_t, bool> pendingStreams;
    std::map<int32_t, int32_t> outstandingBuffers;
};

class MiAlgoCamMode : public MiCamMode
{
public:
    MiAlgoCamMode(const CreateInfo &info);
    ~MiAlgoCamMode() override;
    MiAlgoCamMode(const MiAlgoCamMode &) = delete;
    MiAlgoCamMode &operator=(const MiAlgoCamMode &) = delete;

public:
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // External Interface (for MiCamSession)
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    int configureStreams(const StreamConfiguration &requestedConfiguration,
                         HalStreamConfiguration &halConfiguration) override;
    int processCaptureRequest(const std::vector<CaptureRequest> &requests,
                              uint32_t &numRequestProcessed) override;
    void processCaptureResult(const std::vector<CaptureResult> &results) override;
    void notify(const std::vector<NotifyMsg> &msgs) override;
    int flush() override;
    int close() override;

    MiCamModeType getModeType() override { return AlgoMode; };

    // for HAL Buffer Manage (HBM)
    void signalStreamFlush(const std::vector<int32_t> &streamIds,
                           uint32_t streamConfigCounter) override;
    void requestStreamBuffers(const std::vector<BufferRequest> &bufReqs,
                              requestStreamBuffers_cb cb) override;
    void returnStreamBuffers(const std::vector<StreamBuffer> &buffers) override;

    // for Offline Session
    int switchToOffline(const std::vector<int64_t> &streamsToKeep,
                        CameraOfflineSessionInfo &offlineSessionInfo) override;
    void setCallback(std::shared_ptr<MiCamDevCallback> deviceCallback) override;
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //// Internal Operations (for submodule, i.e., MiCapture, AlgoEngine...)
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    void forwardResultToFwk(const CaptureResult &result);
    void forwardNotifyToFwk(const NotifyMsg &msg);
    int forwardRequestToVnd(const CaptureRequest &request);

    // For MiCamDecision
    void updateSettings(const MiMetadata &settings);
    void updateStatus(const MiMetadata &status);
    int queryFeatureSetting(const MiMetadata &info, uint32_t &frameCount,
                            std::vector<MiMetadata> &settings);

    // For quick view
    void updateQuickViewBuffer(uint32_t fwkFrameNum, uint64_t shutter, const StreamBuffer &buffer);
    bool notifyQuickView(uint32_t fwkFrameNum, uint64_t shutter);
    void returnQuickViewError(uint32_t fwkFrameNum);
    void addAnchorFrame(uint32_t fwkFrameNum, uint32_t anchorFwkFrameNum);
    bool nv21ToNv12(std::shared_ptr<MiImageBuffer> in);
    void startQuickviewResultThread();
    void exitQuickviewResultThread();
    void quickviewResultThreadLoop();
    uint32_t getAnchorFrameNumber(uint64_t shutter);
    int updatePendingSnapRecords(uint32_t fwkFrameNum, std::string imageName, int type);
    void notifyCaptureFail(uint32_t fwkFrameNum, std::string imageName);

    // For AlgoEngine
    int reprocess(uint32_t fwkFrameNum, std::map<uint32_t, std::shared_ptr<MiRequest>> requests,
                  std::string captureMode, bool frameFirstArrived = true);

    void quickFinish(std::string pipelineName);
    void onReprocessResultComing(const CaptureResult &result);
    void onReprocessCompleted(uint32_t fwkFrameNum);

    // For AppSnapshotController
    void splitString(std::string &captureTypeName, std::ostringstream &typeName,
                     std::string &decisionModeStr);
    void onAlgoTaskNumChanged(bool isSendToMialgo, std::string postProcessName);
    void onMemoryChanged(std::map<int32_t, int32_t> memoryInfo);
    int32_t bufferUsePredicForNextSnapshot(uint32_t streamId);
    bool canCamModeHandleNextSnapshot(std::map<int32_t, int32_t> memoryInfo);
    void setRetired();
    int64_t getShot2shotTimeDiff();
    void calShot2shotTimeDiff();
    int32_t getPendingSnapCount();
    int32_t getPendingVendorHALSnapCount();
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
    }

    void onPreviewCaptureCompleted(uint32_t fwkFrameNum, uint32_t vndFrameNum);
    void onSnapshotCaptureCompleted(uint32_t fwkFrameNum,
                                    const std::vector<uint32_t> &vndFrameNums);

    std::shared_ptr<MiBufferManager> getBufferManager() { return mBufferManager; };
    bool getFwkBuffer(const CaptureRequest &fwkRequest, int32_t fwkStreamId,
                      StreamBuffer &bufferOutput);

    uint32_t fetchAddVndFrameNum(uint32_t increment = 1);

    int goToAlgoEngine(uint32_t frame);
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

protected:
    static const int kDataCollectWaitTimeoutMs = 3000; // 3000 ms

    void resultThreadLoop();
    void algoEngineRequestLoop();
    void reprocessedResultThreadLoop();

    void startResultThread();
    void startAlgoEngineThread();
    void startReprocessedResultThread();

    void exitResultThread();
    void exitAlgoEngineThread();
    void exitReprocessedResultThread();
    void exitAllThreads();

    void initCameraInfo(const CreateInfo &info);

    bool isPreviewRequest(const CaptureRequest &req);
    bool isSnapshotRequest(const CaptureRequest &req);
    static bool isPreviewStream(const Stream &stream);
    static bool isSnapshotStream(const Stream &stream);
    static bool isSnapshotRawStream(const Stream &stream);
    static bool isTuningDataStream(const Stream &stream);
    bool isStreamSupportOffline(int32_t streamId);

    void prepareFwkConfig(const StreamConfiguration &requestedConfiguration);
    bool IsRetired() { return mIsRetired.load(); }

    void releaseAllQuickViewBuffers();

    void returnErrorRequest(const CaptureRequest &request);

    static bool isFwkRequestCompleted(const std::shared_ptr<FwkRequestRecord> &fwkRequestRecord);
    void onFwkRequestCompleted(uint32_t fwkFrameNum);

    std::shared_ptr<FwkRequestRecord> getFwkRequestRecordLocked(uint32_t fwkFrameNum);

    // for Offline Session
    bool waitSnapshotDataCollected();
    void collectOfflineSessionInfo(CameraOfflineSessionInfo &offlineSessionInfo /* output */);

    bool packTuningBuffer(const std::shared_ptr<FwkRequestRecord> &fwkRequestRecord,
                          const PostProcParams *pTuningData,
                          const std::shared_ptr<MiImageBuffer> &tuningBuffer /* output */);

    // Create info
    CameraInfo mCameraInfo;
    MiMetadata mStaticInfo;
    std::shared_ptr<MiCamDevCallback> mDeviceCallback;
    std::shared_ptr<MiVendorCamera> mDeviceSession;
    std::map<int32_t, MiMetadata> mPhysicalMetadataMap;

    std::unique_ptr<MiCamDecision> mDecision;
    std::unique_ptr<AlgoEngine> mAlgoEngine;
    std::shared_ptr<MiBufferManager> mBufferManager;
    std::shared_ptr<DecoupleUtil> mDecoupleUtil;

    std::shared_ptr<MiConfig> mFwkConfig;
    std::shared_ptr<MiConfig> mVndConfig;

    // Result thread
    std::thread mResultThread;
    std::atomic<bool> mExitResultThread;
    std::deque<MiResult> mPendingVndResults;
    std::mutex mResultsMutex;
    std::condition_variable mResultCond;

    // AlgoEngineThread
    std::thread mAlgoEngineThread;
    std::atomic<bool> mExitAlgoEngineThread;
    // <vndFrameNum, MiCapture>
    std::deque<std::pair<uint32_t, std::shared_ptr<MiCapture>>> mPendingMiCaptures;
    std::mutex mMiCaptureMutex;
    std::condition_variable mMiCaptureCond;

    // ReprocessedResult thread
    std::thread mReprocessedResultThread;
    std::atomic<bool> mExitReprocessedResultThread;
    std::deque<CaptureResult> mPendingReprocessedResults;
    std::mutex mReprocessedResultsMutex;
    std::condition_variable mReprocessedResultCond;

    std::atomic<bool> mPauseReprocessedCallback;
    std::mutex mPauseReprocessedCallbackMutex;
    std::condition_variable mPauseReprocessedCallbackCond;

    // Inflight framework requests, which will be hold until returning to fwk
    // <fwkFrameNum, FwkRequestRecord>
    std::map<uint32_t, std::shared_ptr<FwkRequestRecord>> mInflightFwkRequests;
    std::mutex mInflightFwkRequestsMutex;

    // Inflight vendor requests, which will be hold until returning from core session
    // <vndFrameNum, MiCapture>
    std::map<uint32_t, std::shared_ptr<MiCapture>> mInflightVndRequests;
    std::mutex mInflightVndRequestsMutex;
    std::atomic<uint32_t> mVndFrameNum;
    // NOTE: mSnapshotDoneCond will signal when every Snapshot Capture done (data collected),
    //       switchToOffline() call will wait it to check Snapshot data collected or not.
    std::condition_variable mSnapshotDoneCond;

    // For quick view
    //      <shutter, StreamBuffer>
    std::map<uint64_t, StreamBuffer> mQuickViewBuffers;
    //      <fwkFrameNum, shutter>
    std::map<uint32_t, uint64_t> mQuickViewFwkNumToShutter;
    //       <shutter, fwkFrameNum>
    std::map<uint64_t, uint32_t> mQuickViewShutterToFwkNum;
    //<fwkFrameNum, anchorFwkFrameNum>
    std::map<uint32_t, uint32_t> mAnchorFrames;
    std::mutex mQuickviewResultsMutex;
    std::condition_variable mQuickviewResultCond;
    std::atomic<bool> mExitQuickviewResultThread;
    std::thread mQuickviewResultThread;
    std::mutex mRecordMutex;
    std::map<uint32_t /* frameNumber */, std::bitset<SnapMessageType::HAL_MESSAGE_COUNT>>
        mPendingSnapshotRecord;

    // NOTE: in current implementation, mQuickViewBuffers only visited from mResultThread,
    //       it can be visited lock free
    // std::mutex mQuickViewBuffersMutex;

    // For app sanpshot control
    int64_t mThreshold;
    std::mutex mMetaResultLock;
    std::mutex mCheckStatusLock;
    std::atomic<bool> mMiZoneBufferCanHandleNextShot;
    // NOTE: to record buffer consumption when do snapshot
    std::map<uint32_t /*stream id*/, std::atomic<int>> mBuffersConsumed;
    std::map<uint32_t /*stream id*/, std::atomic<int>> mOneBuffersConsumed;
    std::map<uint32_t /*fwkFrameNum*/, std::string> mFwkNumToCaptureTypeName;
    // NOTE: when a CameraMode is retired, its status is no longer useful for judging whether mihal
    // can handle next snapshot
    std::atomic<bool> mIsRetired;
    int64_t mShot2ShotTimediff;
    int64_t mPreSnapshotTime;
    std::atomic<int> mPendingMiHALSnapCount;
    std::atomic<int> mPendingVendorHALSnapCount;
    std::atomic<int> mPendingVendorMFNRSnapCount;
    int mMialgoSessionCapacity;
    std::map<uint32_t, std::map<uint32_t, int32_t>> mStreamToFwkNumber;
    // For capture reqeust boost
    PerfLock *mPerfLock;

    CaptureRequest mLatestPreviewRequest;

    std::mutex mPostProcDataLock;
    std::map<uint32_t /*fwkFrameNum*/,
             std::map<int32_t, std::vector<std::shared_ptr<MiPostProcData>>>>
        mPostProcData;
    bool mFirstPreviewArrived = false;
};

} // namespace mizone

#endif //_MIZONE_MIALGOCAMMODE_H_
