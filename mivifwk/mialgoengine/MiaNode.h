#ifndef __MI_NODE__
#define __MI_NODE__

#include <sys/syscall.h>
#include <sys/types.h>

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <set>
#include <unordered_map>

#include "MiaImage.h"
#include "MiaJsonUtils.h"
#include "MiaPipeline.h"
#include "MiaPluginModule.h"
#include "MiaSettings.h"
#include "MiaUtil.h"
#include "ThreadSafeQueue.h"

#define MAX_THRESHOLD 50
static const int32_t PSI_PRINT_THRESHOLD_MS = 10000;
const std::string PSI_BLOCKS_PATH[] = {"/proc/pressure/cpu", "/proc/pressure/memory",
                                       "/proc/pressure/io"};
/*
 * If you modify PERFLOGNUMBER and PERFLOGLONG
 * you also need to modify the PERFLOGNUMBER and PERFLOGLONG
 * in /vendor/xiaomi/proprietary/mivifwk/1.0/extendcamhal/src/hal/ExtendHal3Entry.cpp
 */
#define PERFLOGNUMBER           100
#define PERFLOGLONG             150
#define PERFLOCK_FREQ_THRE_CPU7 2000000
extern char s_AlgoPerfInfo[PERFLOGNUMBER][PERFLOGLONG]; ///< save log

namespace mialgo2 {

using MiaImageQueue = ThreadSafeQueue<sp<MiaImage>>;

// input buffer as to output buffer
static const uint32_t INPLACE_MODE = (1 << 0);
// support multithreading concurrency
static const uint32_t CONCURRENCY_MODE = (1 << 1);
// support scale process
static const uint32_t SCALE_MODE = (1 << 2);
// support frame by frame get input image
static const uint32_t SIGGETOUTPUT_MODE = (1 << 3);
// support frame by frame processing
static const uint32_t SIGFRAME_MODE = (1 << 4);
// support frame by frame release inputImage after use
static const uint32_t SIGRELEASEINPUT_MODE = (1 << 5);
// support frame by frame release outputImage after use
static const uint32_t SIGRELEASEOUTPUT_MODE = (1 << 6);
// virtual node, do one outputport buffer to more childNode
static const uint32_t VIRTUAL_MODE = (1 << 9);
// as an internal node,but get outputbuffer from External
static const uint32_t VIRTUAL_SINKNODE = (1 << 10);

static const int MaxConcurrentNum = 5;

static const int32_t sIsAbnorDumpMode =
    property_get_int32("persist.vendor.camera.algoengine.abnorDump", 1);

struct PostMiaPreProcParams
{
    std::map<uint32_t, MiaImageFormat> inputFormats;
    std::map<uint32_t, MiaImageFormat> outputFormats;
    std::shared_ptr<PreMetaProxy> metadata;
};

/*
 *  NodeCB: The node callback is defined for interaction with algorithm session
 *  The callback provider should implement the interfaces to be as a listenor
 *  and for node to acquire intermediate buffers
 */
class NodeCB
{
public:
    virtual ~NodeCB() {}
    virtual void onResultComing(MiaResultCbType type, uint32_t frameNum, int64_t timeStamp,
                                std::string &data) = 0;
    virtual int getRunningTaskNum() = 0;
    virtual void increaseUsingThreadNum(int increment) = 0;
    virtual bool isLIFOSupportedOfSession() = 0;
};

class MiaNode : public virtual RefBase
{
public:
    static void setResultMetadata(void *owner, uint32_t frameNum, int64_t timeStamp,
                                  std::string &result);

    static void setOutputFormat(void *owner, ImageFormat imageFormat);

    static void setResultBuffer(void *owner, uint32_t frameNum);

    static void releaseableUnneedBuffer(void *owner, int portId, int releaseBufferNumber,
                                        PluginReleaseBuffer releaseBufferMode);

    MiaNode();

    virtual ~MiaNode();

    virtual CDKResult init(struct NodeItem nodeItem, MiaCreateParams *params);

    virtual int preProcess(PostMiaPreProcParams *preParams);

    virtual bool isEnable(MiaParams settings);

    virtual void enqueueTask(sp<Pipeline> pipeline) = 0;

    virtual void flush(bool isForced);

    void getCurrentHWStatus();

    void printPsiInfo() const;

    inline void setNodeCB(NodeCB *nodeCb) { mNodeCB = nodeCb; }

    inline void setNodeMask(uint32_t nodeMask) { mNodeMask = nodeMask; }

    inline uint32_t getNodeMask() { return mNodeMask; }

    inline NodeCB *getNodeCB() { return mNodeCB; }

    inline void setNodeName(std::string name) { mNodeName = name; };

    inline void setNodeInstance(std::string instance) { mNodeInstance = instance; };

    inline const char *getNodeName() { return mNodeName.c_str(); };

    inline std::string getNodeInstance() { return mNodeInstance; };

    virtual void reInit();

    virtual void processTask() = 0;

    bool processTaskError(sp<Pipeline> &pipeline, NodeProcessInfo nodeProcessInfo,
                          int32_t &abnormal, int32_t &frame_num);

    CDKResult getInputBuffer(sp<Pipeline> &pipeline,
                             std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImages);

    CDKResult getOutputBuffer(const sp<Pipeline> &pipeline,
                              const std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImages,
                              std::map<uint32_t, std::vector<sp<MiaImage>>> &outputImages);

    std::map<uint32_t, std::vector<ImageParams>> buildBufferParams(
        const std::map<uint32_t, std::vector<sp<MiaImage>>> &imageMap);

    CDKResult processRequest(std::map<uint32_t, std::vector<sp<MiaImage>>> &inputFrameMap,
                             std::map<uint32_t, std::vector<sp<MiaImage>>> &outputFrameMap,
                             int32_t &abnormal, int32_t &frame_num);

    CDKResult processRequest(std::map<uint32_t, std::vector<sp<MiaImage>>> &, int32_t &abnormal,
                             int32_t &frame_num);

    virtual bool checkTaskEffectiveness(const sp<Pipeline> &pipeline, CDKResult &info,
                                        NodeProcessInfo &nodeProcessInfo);

    virtual CDKResult deInit();

    void dumpImageInfo(const std::vector<ImageParams> &buffers, std::string imageName,
                       std::string type, uint32_t portId);

    void updateProcFreqExifData(sp<MiaImage> &image, sp<Pipeline> &pipeline, bool readyToExecute);

    int32_t updateAbnImageExifData(MiAbnormalParam &pAbnormalParam, MiAbnormalDetect &abnDetect);

    int32_t updateAbnImageExifDataV1(ProcessRequestInfo &requestInfo);

    int32_t updateAbnImageExifDataV2(ProcessRequestInfoV2 &requestInfo);

    void signalThead(bool &isProcessDone, std::condition_variable &, std::mutex &);

    void timeoutDetection(uint32_t tid, bool &isProcessDone, std::condition_variable &,
                          std::mutex &);

    void quickFinish(const std::string &pipelineName);

    // for timeout dection
#if defined(USERDEBUG_CAM)
    static const int64_t kProcessWaitTimeoutSec = 30; // 30s, for Asan
#else
    static const int64_t kProcessWaitTimeoutSec = 20; // 20s
#endif

    uint32_t mNodeMask; // from nodeMask
    std::string mNodeName;
    std::string mNodeInstance;
    bool mOutputBufferNeedCheck;
    NodeCB *mNodeCB;
    PluginWraper *mPluginWraper;
    int mInputFrameNum;
    double mProcStartTime;
    std::mutex mProcTimeLock;
    std::map<int64_t, double> mProcStartTimes; // timeStamp<->processStartTime
    bool mHasProcessThread;

    std::mutex mProcessLock;
    // Ensure that the same MiaNode serves only one MiaPipeline at a time
    sp<Pipeline> mPipeline;
    std::priority_queue<std::pair<int, sp<Pipeline>>> mRequestQueue;
    std::mutex mBufferLock;

    int32_t m_currentTemp;
    int32_t m_currentCpuFreq0;
    int32_t m_currentCpuFreq4;
    int32_t m_currentCpuFreq7;
    int32_t m_currentGpuFreq;
    int32_t m_maxCpuFreq0;
    int32_t m_maxCpuFreq4;
    int32_t m_maxCpuFreq7;
    int32_t m_maxGpuFreq;

    // for dump
    bool mOpenDump;
    int mLastDumpInputIndex;
    int mLastDumpOutputIndex;
    std::string mLastDumpFileName;

    static uint32_t m_uiLogNumber; ///< log index
    ProcessRequestInfo mAbnImageCheckForFrameByFrame;

    // close LIFO for mivi2.0 burst snapshot
    bool mIsMiHalBurstMode;

    // for quickfinish
    std::atomic<int> mInPluginThreadNum;
    std::condition_variable mQuickFinishCond;
    std::mutex mQuickFinishMux;
};

class ConcurrencyNode : public MiaNode
{
public:
    ConcurrencyNode();

    virtual ~ConcurrencyNode();

    virtual void enqueueTask(sp<Pipeline> pipeline);

private:
    virtual void processTask();
};

class SigframeNode : public MiaNode
{
public:
    SigframeNode();

    virtual ~SigframeNode();

    virtual void enqueueTask(sp<Pipeline> pipeline);

private:
    virtual void processTask();
    virtual bool checkTaskEffectiveness(const sp<Pipeline> &pipeline, CDKResult &info,
                                        NodeProcessInfo &nodeProcessInfo);
    bool needGetOutBufferThisTime(int mergeInputNum);
};

class OtherNode : public MiaNode
{
public:
    OtherNode();

    virtual ~OtherNode();

    virtual void enqueueTask(sp<Pipeline> pipeline);

private:
    virtual void processTask();
};

class VirtualNode : public MiaNode // one inputport to more outputport
{
public:
    VirtualNode(){};
    virtual ~VirtualNode(){};
    virtual CDKResult init(struct NodeItem nodeItem, MiaCreateParams *params)
    {
        mNodeMask = nodeItem.nodeMask;
        setNodeName(std::string(nodeItem.nodeName.c_str()));
        setNodeInstance(std::string(nodeItem.nodeInstance.c_str()));
        return MIAS_OK;
    };
    virtual CDKResult deInit() { return MIAS_OK; };
    virtual void reInit(){};
    virtual int preProcess(PostMiaPreProcParams *preParams) { return 0; };
    virtual void flush(bool isForced){};
    virtual bool isEnable(MiaParams settings) { return true; };
    virtual void enqueueTask(sp<Pipeline> pipeline);

private:
    virtual void processTask();
};

class MiaNodeFactory : virtual public RefBase
{
public:
    MiaNodeFactory();

    virtual ~MiaNodeFactory();

    void CreateNodeInstance(const uint32_t mNodeMask, sp<MiaNode> &FactoryNode);
};

} // namespace mialgo2

#endif // END define __MI_NODE__
