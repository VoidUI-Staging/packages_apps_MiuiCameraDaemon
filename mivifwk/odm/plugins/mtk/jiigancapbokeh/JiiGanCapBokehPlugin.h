#ifndef _JIIGANCAPBOKEHPLUGIN_H_
#define _JIIGANCAPBOKEHPLUGIN_H_

#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <MiaPluginWraper.h>
#include <MiaProvider.h>
#include <VendorMetadataParser.h>
#include <pluma.h>
#include <system/camera_metadata.h>

#include "JiiGanCapBokeh.h"

using namespace mialgo2;
class JiiGanCapBokhPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.capbokeh";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    //    int deInit();

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings) { return true; }

private:
    ~JiiGanCapBokhPlugin();
    MiaNodeIntSenseCapBokeh *mBokehInstance;
};

PLUMA_INHERIT_PROVIDER(JiiGanCapBokhPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(ProviderManager &host)
{
    host.add(new JiiGanCapBokhPluginProvider());
    return true;
}

#endif // _JIIGANCAPBOKEHPLUGIN_H_
