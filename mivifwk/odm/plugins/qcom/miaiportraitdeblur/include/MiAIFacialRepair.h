#ifndef __MI_NODEMIAIFACIALREPAIR__
#define __MI_NODEMIAIFACIALREPAIR__

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "chivendortag.h"
#include "miaifacemiui.h"

namespace mialgo2 {

class MiAIFacialRepair : public PluginWraper
{
public:
    static std::string getPluginName() { return "com.xiaomi.plugin.miaiportraitdeblur"; }
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual void destroy();
    virtual bool isEnabled(MiaParams settings);

private:
    ~MiAIFacialRepair();
    int processBuffer(MiImageBuffer *input, MiImageBuffer *outputBokeh, MiImageBuffer *outputDepth,
                      camera_metadata_t *metaData);
    int mEnableStatus;
    MiAIFaceMiui *mNodeInstance;
    MiaNodeInterface mNodeInterface;
    uint32_t mFrameworkCameraId;
    aiface::MIAIParams m_facialrepairparams;
};

PLUMA_INHERIT_PROVIDER(MiAIFacialRepair, PluginWraper);

PLUMA_CONNECTOR
bool connect(ProviderManager &host)
{
    host.add(new MiAIFacialRepairProvider());
    return true;
}

} // namespace mialgo2
#endif //__MI_NODEMIBOKEH__
