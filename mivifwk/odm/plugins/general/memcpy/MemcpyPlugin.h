#ifndef _MEMCPYPLUGIN_H_
#define _MEMCPYPLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>

class MemcpyPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.memcpy";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings) { return true; }

private:
    ~MemcpyPlugin();
    MiaNodeInterface mNodeInterface;
};

PLUMA_INHERIT_PROVIDER(MemcpyPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MemcpyPluginProvider());
    return true;
}

#endif // _MEMCPYPLUGIN_H_
