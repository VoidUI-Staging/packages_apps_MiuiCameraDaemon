#ifndef _MI_SUPERMOON_
#define _MI_SUPERMOON_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "MiaPluginUtils.h"
#include "supermoon.h"

class MiSuperMoonPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.supermoon";
    }

    virtual int initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *pProcessRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *pProcessRequestInfo);

    virtual int flushRequest(FlushRequestInfo *pFlushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

private:
    ~MiSuperMoonPlugin();
    float supermoongetZoom(camera_metadata_t *pMetaData);

    //    mialgo2::supermoon*      supermoon;
    bool supermoon_init;
    MiaNodeInterface m_NodeInterface;
};
PLUMA_INHERIT_PROVIDER(MiSuperMoonPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiSuperMoonPluginProvider());
    return true;
}

#endif // mi_supermoon
