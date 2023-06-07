#ifndef _PUREVIEW_PLUGIN_H_
#define _PUREVIEW_PLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include <mutex>
#include <thread>

#include "MiaPluginUtils.h"
#include "chivendortag.h"
#include "pureView_api.h"
#include "pureView_basic.h"

using namespace std;

enum { INIT = 0, NEEDPROC, PROCESSED, NEEDFLUSH, FLUSHED, END };

class PureViewPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.pureview";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual void destroy();
    virtual bool isEnabled(MiaParams settings);

private:
    ~PureViewPlugin();
    int processFrame(uint32_t inputFrameNum);
    void PerFrameProcessDonecallback(PV_IMG *);
    void loopFunc();
    int ProcessBuffer(PV_IMG *input, int input_num, PV_IMG *output, int64_t timeStamp,
                      void **metaData, void **physMeta);
    int GetISO(void *metaData);
    ChiRect GetCropRegion(camera_metadata_t *metaData, int inputWidth, int inputHeight);
    PV_FACE_INFO GetFaceList(void *metaData, PV_IMG input);
    MiaNodeInterface mNodeInterface;
    PV_CONFIG pvConfig = {0};
    PV_IMG *pvInput = NULL;
    PV_IMG pvOutput;
    uint32_t mOutputFrameFormat;
    uint32_t mInputFrameFormat;
    std::mutex mImageMutex;
    std::mutex mMutexLoop;
    condition_variable mCondLoop;
    std::mutex mMutexPerFrameReturn;
    condition_variable mCondPerFrameReturn;
    condition_variable mCondLastFrameReturn;
    thread mProcThread;
    uint32_t mFrameNum;
    int mStatus; // in lock
    uint32_t mMultiNum = 1;
    bool mInProgress = false;
    void *mMetaData[PV_MAX_FRAME_NUM] = {NULL};
    void *mPhysMeta[PV_MAX_FRAME_NUM] = {NULL};
};

PLUMA_INHERIT_PROVIDER(PureViewPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new PureViewPluginProvider());
    return true;
}

#endif // _PUREVIEW_PLUGIN_H_
