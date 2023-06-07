#ifndef _SUPERPORTRAIT_PLUGIN_H_
#define _SUPERPORTRAIT_PLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "superportrait.h"

class SuperPortraitPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.superportrait";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings) { return true; }

private:
    ~SuperPortraitPlugin();
    void updateSuperPortraitParams(camera_metadata_t *metadata);
    int mProcessCount;
    int32_t mSuperportraitMode;
    uint32_t mSuperportraitLevel;
    SuperPortrait *mNodeInstance;
    uint32_t mOrientation;
};

PLUMA_INHERIT_PROVIDER(SuperPortraitPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new SuperPortraitPluginProvider());
    return true;
}

#endif // _SUPERPORTRAIT_PLUGIN_H_
