#ifndef _SR_PLUGIN_H_
#define _SR_PLUGIN_H_

#include <VendorMetadataParser.h>

#include "algorithmManager.h"

class SrPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.sr";
    }

    virtual int initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *pProcessRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *pProcessRequestInfo);

    virtual int flushRequest(FlushRequestInfo *pFlushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

    SrPlugin();

private:
    ~SrPlugin();
    unsigned int mProcessCount;
    SessionInfo sessionInfo;
};

PLUMA_INHERIT_PROVIDER(SrPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new SrPluginProvider());
    return true;
}

#endif // _ALMALENCESR_PLUGIN_H_
