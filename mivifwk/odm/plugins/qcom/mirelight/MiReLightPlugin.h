#ifndef __MIRELIGHTPLUGIN_H__
#define __MIRELIGHTPLUGIN_H__

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>

#include "mi_portrait_3d_lighting_image.h"
//#include "chioemvendortag.h"

class MiReLightPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.relight";
    }

    virtual int initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *pProcessRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *pProcessRequestInfo);

    virtual int flushRequest(FlushRequestInfo *pFlushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

private:
    ~MiReLightPlugin();
    void PrepareRelightImage(MIIMAGEBUFFER *dstimage, struct MiImageBuffer *srtImage);
    int m_lightingMode;
    MHandle m_hRelightingHandle;
    int m_width;
    int m_height;
    uint32_t m_operationMode;
    bool m_RearCamera;
    MiaNodeInterface m_NodeInterface;
};
PLUMA_INHERIT_PROVIDER(MiReLightPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiReLightPluginProvider());
    return true;
}

#endif //__MIRELIGHTPLUGIN_H__
