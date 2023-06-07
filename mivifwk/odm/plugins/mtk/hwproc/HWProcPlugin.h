#ifndef _MEMCPYPLUGIN_H_
#define _MEMCPYPLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <mtkcam-halif/device/1.x/types.h> //CaptureRequest, Stream, #IMetadata;

#include "MiPluginAdaptor.h"
#include "MiPostProcDeviceUtil.h"

using NSCam::IMetadata;
using namespace mialgo2;
using namespace mizone::mipostprocinterface;

class HWProcPlugin : public PluginWraper, public MiPostProcPluginCb
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.hwproc";
    }

    void processResultDone(CallbackInfos &callbackInfos);

    // keep the node's handle;
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processInfo);

    // not support this mode;
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processInfo2);

    // do nothing!
    virtual int flushRequest(FlushRequestInfo *flushInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

private:
    ~HWProcPlugin();

    MiaNodeInterface mNodeInterface;
    mizone::mipostprocinterface::CreationInfos mCreateInfo;
    std::shared_ptr<MiPluginAdaptor> mProcAdp;
    int64_t mFwkTimeStamp;
    ProcessRequestInfo *mProcReqInfo;

    bool mIsProcessDone;
    ProcessRetStatus mResultInfo;
    std::mutex mIsProcessDoneMutex;
    std::condition_variable mIsProcessDoneCond;

    std::string mNodeInstanceName;
    uint32_t mOperationMode;
    uint32_t mCameraMode;
};

PLUMA_INHERIT_PROVIDER(HWProcPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new HWProcPluginProvider());
    return true;
}

#endif // _MEMCPYPLUGIN_H_
