/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MIA_OFFLINESESSION__
#define __MIA_OFFLINESESSION__

#include <stdatomic.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <utils/List.h>
#include <utils/Vector.h>
#include <vndk/hardware_buffer.h>
#include <vndk/window.h>

#include <atomic>
#include <map>
#include <set>

#include "MiaBufferManager.h"
#include "MiaImage.h"
#include "MiaJsonUtils.h"
#include "MiaMemReclaim.h"
#include "MiaNode.h"
#include "MiaPerfLock.h"
#include "MiaPerfManager.h"
#include "MiaPipeline.h"
#include "MiaPipelinePruner.h"
#include "MiaPostProcIntf.h"
#include "MiaSettings.h"
#include "Timer.h"

namespace mialgo2 {
using MiaImageQueue = ThreadSafeQueue<sp<MiaImage>>;

#define TASK_STATE_INPROCESSING 0
#define TASK_STATE_META_DONE    1
#define TASK_STATE_FRAME_DONE   2
#define TASK_STATE_DONE         (TASK_STATE_META_DONE | TASK_STATE_FRAME_DONE)
struct Task
{
    Task() : state(0), oFrame(NULL)
    {
        iFrames.clear();
        CLEAR(frame);
    }
    uint32_t state;
    // INPUT a port to frame map
    std::map<uint32_t, sp<MiaImage>> iFrames;
    // OUTPUT
    // MiaFrame to carry the callback to return this frame
    MiaFrame frame;
    sp<MiaImage> oFrame;
};

static const int THREAD_POOL_NUMS = 12;
static const uint64_t NSEC_PER_SEC = 1000000000ull;
static const uint32_t QtimerFrequency = 19200000; ///< QTimer Freq = 19.2 MHz
static const uint32_t MAX_SAMPLES_BUF_SIZE = 200; ///< Max sample size

typedef struct
{
    uint32_t gcount;
    uint64_t ts;
    uint64_t te;
    float x[MAX_SAMPLES_BUF_SIZE];
    float y[MAX_SAMPLES_BUF_SIZE];
    float z[MAX_SAMPLES_BUF_SIZE];
    uint64_t t[MAX_SAMPLES_BUF_SIZE];
} ChiFrameGyro;

/// Convert from nano secs to micro secs
#define NanoToMicro(x) (x / 1000)

inline uint64_t QtimerTicksToQtimerNano(uint64_t ticks)
{
    return (uint64_t(double(ticks) * double(NSEC_PER_SEC) / double(QtimerFrequency)));
}

struct exifInfoOfRequest
{
    exifInfoOfRequest()
    {
        mLastRequestSourceBufferNum = 0;
        mFirstTimestamp = 0;
        mFirstFrameNum = 0;
        mB2YCount = 0;
        mB2YCostTotal = 0.0;
        mShutterLag = 0;
        mGetFrameStartTimestamp = 0;
        mGetFrameEndTimestamp = 0;
        mAnchorFrameIndex = 0;
        mTimestamp.clear();
    }

    int mLastRequestSourceBufferNum;
    uint64_t mFirstTimestamp;
    uint32_t mFirstFrameNum;
    int mB2YCount;
    double mB2YCostTotal;
    int32_t mShutterLag;
    uint64_t mGetFrameStartTimestamp;
    uint64_t mGetFrameEndTimestamp;
    int32_t mAnchorFrameIndex;
    std::map<uint32_t, uint64_t> mTimestamp; // frameIndex <-> frameTimestamp
};

struct requestInfo
{
    uint32_t frameNumNotCome;
    sp<Pipeline> pipeline;
    exifInfoOfRequest exifInfoOfRequest;
};

typedef struct MiaRect
{
    uint32_t left;   // x coordinate for top-left point
    uint32_t top;    // y coordinate for top-left point
    uint32_t width;  // width
    uint32_t height; // height
} MIARECT;

/// @brief rectangle
typedef struct MiFaceInfo
{
    int left;   // The leftmost coordinate of the rectangle
    int top;    // Coordinates of the top edge of the rectangle
    int right;  // The rightmost coordinate of the rectangle
    int bottom; // Coordinates of the lowest side of the rectangle
} MiFaceInfo;

typedef struct MiRectEXT
{
    int left;   // x coordinate for top-left point
    int top;    // y coordinate for top-left point
    int right;  // x coordinate for bottom-right point
    int bottom; // y coordinate for bottom-right point
} MIRECTEXT;

typedef struct
{
    int faceNumber;
    MiRectEXT miFaceInfo[10];
} MiFaceResult;

class BufferQueueWrapper
{
public:
    /// Static create function to create an instance of the object
    static BufferQueueWrapper *create(MiaImageFormat &format, buffer_type type, uint32_t wrapperId,
                                      uint32_t numBuffers = DEFAULT_BUFFER_QUEUE_DEPTH);

    sp<MiaImage> dequeueBuffer();

    void enqueueBuffer(sp<MiaImage> frame);

    void flush();

    void destroy();

    bool isMatchFormat(MiaImageFormat format);
    void clearOutDefaultBuffer();
    void clearOutThresholdBuffer();
    int getFreeBufferQueueSize();
    int getTotalBufferSize();
    void setBufferThresholdValue(uint32_t bufferThresholdValue);

private:
    BufferQueueWrapper() = default;

    virtual ~BufferQueueWrapper(){};

    CDKResult initialize(MiaImageFormat &format, buffer_type type, uint32_t wrapperId,
                         uint32_t numBuffers);

    void checkBufferState();

    CDKResult increaseImageQueue(uint32_t numBuffers);

    MiaImageFormat mBufferFormat;
    sp<MiaBufferManager> mPbufferManager;
    MiaImageQueue mBufferQueue;
    uint32_t mBufferWrapperId;
    Mutex mBufferMutex;
    int mThresholdValue;
};

class MiaOfflineSession : public NodeCB, public PipelineCB
{
public:
    static MiaOfflineSession *create(MiaCreateParams *params);

    CDKResult preProcess(MiaPreProcParams *preParams);

    virtual CDKResult postProcess(MiaProcessParams *sessionParams);

    bool rebuildLinks(MiaParams &settings, PipelineInfo &pipeline, uint32_t firstFrameInputFormat,
                      std::vector<MiaImageFormat> &sinkBufferFormat) override;

    CDKResult flush(bool isForced);

    QFResult quickFinish(QuickFinishMessageInfo &messageInfo);

    virtual void onResultComing(MiaResultCbType type, uint32_t frameNum, int64_t timeStamp,
                                std::string &data);

    virtual sp<MiaImage> acquireOutputBuffer(MiaImageFormat format, int64_t timestamp,
                                             uint32_t sinkPortId,
                                             MiaImageType newImageType) override;

    virtual void releaseBuffer(sp<MiaImage> frame, bool rComplete) override;

    void moveToIdleQueue(sp<Pipeline> pipeline) override;

    void increaseUsingThreadNum(int increment) override;

    virtual int getRunningTaskNum() override;
    virtual bool isLIFOSupportedOfSession() { return mSupportLIFO; };

    sp<MiaNode> getProcessNode(std::string instance) override;

    ~MiaOfflineSession();

private:
    MiaOfflineSession();

    MiaOfflineSession(const MiaOfflineSession &) = delete;
    MiaOfflineSession &operator=(const MiaOfflineSession &) = delete;

    CDKResult initialize(MiaCreateParams *params);

    CDKResult buildGraph(MiaCreateParams *params);

    void destroy();

    // for memory reclaim
    void memoryMonitor();

    void initMemReclaim(MiaCreateParams *pParams);

    // for update exif
    void prepareForExifUpdate(sp<MiaImage> frame, camera_metadata_t *metadata,
                              exifInfoOfRequest &exifInfoOfRequest);

    void updateExifInfo(camera_metadata_t *metadata, exifInfoOfRequest &exifInfoOfRequest);

    void signalThread();

    void timeoutDetection(uint32_t tid, std::string &initInfo);
    void getFaceExifInfo(camera_metadata_t *metaData, uint32_t w, uint32_t h, uint32_t frameNum,
                         int64_t timeStamp);

    // for timeout detection
    static const int64_t kInitWaitTimeoutSec = 10; // 10s
    std::condition_variable mTimeoutCond;
    std::mutex mTimeoutMux;
    std::string mTimeoutNode;
    bool mIsInitDone;

    MiaPostProcSessionCb *mSessionCb;
    MiaFrameCb *mFrameCb;
    MiaCreateParams mCreateParams; ///< Postproc Create Params
    std::mutex mSinkImageMux;
    std::multimap<int64_t, sp<MiaImage>> mSinkImageMap;
    std::map<std::string, sp<MiaNode>> mNodes; // NodeInstance <-> node
    sp<PipelinePruner> mPipelinePruner;
    std::string mSelectedPipelineName;
    std::set<BufferQueueWrapper *> mBufferQueueWrapperSet;
    Mutex mBufferQueueLock;
    uint32_t mBufferWraperCount;
    uint32_t mInitBufferQueueDepth;
    std::vector<uint32_t> mSourcePortIdVec;

    int mReltime;
    uint32_t mSessionRequestCnt;
    uint32_t mDefaultReclaimCnt;
    uint32_t mBufferThresholdValue;
    Mutex mBufferReclaimMutex;
    FakeTimer mTimer;
    Mutex mAlgoResultLock;
    std::map<int64_t, std::vector<std::string>> mAlgoResultInfo;
    std::mutex mAcquireBufferMutex;

    std::set<sp<Pipeline>> mBusyPipelines;
    sp<Pipeline> mIdlePipeline;
    std::map<std::string, requestInfo> mInputNotEnoughPipe;
    Mutex mIdleLock;
    std::mutex mBusyLock;
    std::condition_variable mBusyCond;
    std::mutex mFlushLock;
    int mNodeThreadNum;
    std::mutex mAllNodeFinishLock;
    std::condition_variable mAllNodeFinishCond;
    std::atomic<int> mFIFOPipePriority;
    std::atomic<int> mLIFOPipePriority;

    bool mIsBurstMode;
    bool mSupportLIFO; // Is a supported in the current case
};

} // namespace mialgo2

#endif //__MIA_OFFLINESESSION__
