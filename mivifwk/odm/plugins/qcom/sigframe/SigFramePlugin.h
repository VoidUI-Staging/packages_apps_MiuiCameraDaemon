#ifndef _SigFramePlugin_H_
#define _SigFramePlugin_H_

#include <condition_variable> // condition_variable
#include <memory>
#include <mutex> // mutex, unique_lock
#include <string>
#include <thread> // thread

#include "MiaPluginUtils.h"
#include "MiaPluginWraper.h"
#include "VendorMetadataParser.h"

using namespace std;

using ImageQueue = queue<shared_ptr<MiImageBuffer>>;

struct ImageQueueWrapper
{
    uint32_t frameNum;
    ImageQueue imageQueue;
};

enum { INIT = 0, NEEDPROC, PROCESSED, NEEDFLUSH, FLUSHD, END };

class SigFramePlugin : public PluginWraper
{
public:
    static string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.sigframe";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    void loopFunc();

    int processFrame(uint32_t inputFrameNum);

    int cpyFrame(MiImageBuffer *dstFrame, MiImageBuffer *srcFrame);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings) { return true; }

private:
    ~SigFramePlugin();
    MiaNodeInterface mNodeInterface;
    std::mutex mImageMutex;
    queue<ImageQueueWrapper> mInputQWrapperQueue;
    map<uint32_t, ImageQueue> mOutputQueueMap; // store frameNum and image
    std::mutex mMutex;
    condition_variable mCond;
    thread mProcThread;
    int mStatus; // in lock
    uint32_t pMultiNum;
};

PLUMA_INHERIT_PROVIDER(SigFramePlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new SigFramePluginProvider());
    return true;
}

#endif // _SigFramePlugin_H_
