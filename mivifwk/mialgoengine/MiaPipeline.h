#include <algorithm>
#include <atomic>
#include <stack>

#include "MiaNode.h"
#include "MiaPluginWraper.h"
#include "MiaSettings.h"
#include "ThreadPool.h"
#include "ThreadSafeQueue.h"

#ifndef __MIA_PIPELINE__
#define __MIA_PIPELINE__

using namespace mialgo2;

namespace mialgo2 {

using MiaImageQueue = ThreadSafeQueue<sp<MiaImage>>;

typedef enum { NoFlush, Expediting, ForcedFlush } FlushStatus;
enum NodeProcessInfo { ProcessOK, GetInputError, GetOutputError, FlushTask, NodeProcessError };
extern const char *NodeProcessInfoToChar[];

class Pipeline;
class MiaNode;

class PipelineCB
{
public:
    virtual ~PipelineCB() {}
    virtual sp<MiaNode> getProcessNode(std::string instance) = 0;
    virtual sp<MiaImage> acquireOutputBuffer(MiaImageFormat format, int64_t timestamp,
                                             uint32_t sinkPortId = 0,
                                             MiaImageType newImageType = MI_IMG_INTERNAL) = 0;
    virtual void releaseBuffer(sp<MiaImage> frame, bool rComplete = false) = 0;
    virtual void onResultComing(MiaResultCbType type, uint32_t frameNum, int64_t timeStamp,
                                std::string &data) = 0;

    virtual void moveToIdleQueue(sp<Pipeline> pipeline) = 0;

    virtual bool rebuildLinks(MiaParams &settings, PipelineInfo &pipeline,
                              uint32_t firstFrameInputFormat,
                              std::vector<MiaImageFormat> &sinkBufferFormat) = 0;
};

class Pipeline : public virtual RefBase
{
public:
    Pipeline(camera_metadata_t *metadata, MiaCreateParams *params, MiaProcessParams *processParams,
             int64_t timestampUs, const std::vector<uint32_t> &sourcePortIdVec);

    void setSession(PipelineCB *session);

    inline void setPipelineName(std::string name) { mName = name; }

    inline const char *getPipelineName() { return mName.c_str(); }

    inline void setFIFOPriority(int priority) { mFIFOPriority = priority; }

    inline int getFIFOPriority() const { return mFIFOPriority; }

    inline void setLIFOPriority(int priority) { mLIFOPriority = priority; }

    inline int getLIFOPriority() const { return mLIFOPriority; }

    inline int64_t getFirstFrameTimestamp() const { return mTimestampUs; }

    FlushStatus getFlushStatus() { return mFlushStatus; }

    void start();

    bool enqueueSourceImage(sp<MiaImage>, uint32_t);

    void flush(bool isForced);

    void quickFinish(bool needImageResult);

    void notifyNextNode(std::string instance, NodeProcessInfo nodeProcessInfo, int abnormal,
                        int32_t frame_num);

    void releaseInputAndTryNotifyNextNode(const std::string &instance);

    void releaseOutputAndTryNotifyNextNode(const std::string &instance);

    CDKResult getOutputImage(const std::string &nodeInstance, int nodeMask,
                             const std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImages,
                             const std::map<int, MiaImageFormat> &imageFormats,
                             std::map<uint32_t, std::vector<sp<MiaImage>>> &outputImages);

    CDKResult getInputImage(const std::string &instance, int nodeMask,
                            std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImages);

    CDKResult pluginReleaseUnneedBuffer(const std::string &instance, int portId, int releaseNumber,
                                        PluginReleaseBuffer releaseBufferMode);

private:
    bool rebuildLinks();

    void postProcTask();

    bool isInplaceNode(const std::string &nodeInstance, int nodeMask);

    CDKResult postProcess(std::string instance);

    sp<MiaImage> getSourceImage(uint32_t srcPortId);

    CDKResult releaseInputBuffers(std::string instance, bool releaseOneInputBuffer = false);

    CDKResult releaseOutBuffers(int linkIndex, LinkType linkType,
                                bool releaseOneOutputBuffer = false);

    void releaseUnusedBuffer();

    void countDynamicOutBufferNum(std::map<int, int> &dynamicBufferNum, const std::string &instance,
                                  const std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImages);

    void customOutportBufferNum(std::map<int, int> &dynamicBufferNum, const std::string &instance,
                                const std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImages,
                                const std::map<int, int> &inportBufferNum,
                                std::vector<int> &outports);

    void updateMiviProcessTime2Exif(sp<MiaImage> &image);

    bool checkInputFormat(sp<MiaImage> inputImage, const LinkItem &linkItem, int portId);

    struct PipelineInfo mPipelineInfo;
    PipelineCB *mSessionCB;
    MiaParams mSettings;

    std::string mName;
    std::vector<MiaImageFormat> mSinkBufferFormats;
    uint32_t mFirstFrameInputFormat;
    std::set<std::string> mDisableSourceSet;
    int mLIFOPriority;
    int mFIFOPriority;
    int64_t mTimestampUs;
    double mMiviProcessStartTime;

    std::queue<std::string> mRunSequence;
    std::map<std::string, int> mDependenceNum;
    int mRunningNodeNum;
    bool mHasProcessError;
    uint32_t mSinkNumToReturn; // = sinkLink number
    FlushStatus mFlushStatus;
    // nodeInstance<-->this nodeInstance's all inputLink's index in pipelineLinks
    std::map<std::string, std::vector<int>> mInputLinkMap;
    // nodeInstance<-->this nodeInstance's all outputLink's index in pipelineLinks
    std::map<std::string, std::vector<int>> mOutputLinkMap;

    std::mutex mLock;
    std::condition_variable mCond;
    std::mutex mQuickFinishLock;

    std::mutex mIsCallExternalLock;
    // Flag indicating whether the exception handling of this task has been called back
    bool mIsCallExternal;

    std::mutex mRecordProcessNodeLock;
    std::set<std::string> mCurrentProcessNodes;

    std::mutex mSourceBufferLock;
    std::map<uint32_t, MiaImageQueue> mSourceBufferQueueMap;

    struct bufferSpace
    {
        std::map<int, MiaImageQueue> inputBuffers;      // inputPort <-> images
        std::map<int, MiaImageQueue> outputBuffers;     // outputPort <-> images
        std::map<int, MiaImageQueue> usingInputBuffers; // inputPort <-> images
    };
    std::map<std::string, bufferSpace> mBuffers; // instance<->bufferSpace
};

} // namespace mialgo2
#endif
