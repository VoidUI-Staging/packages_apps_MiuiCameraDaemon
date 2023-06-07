#ifndef _MISWFLIPPLUGIN_H_
#define _MISWFLIPPLUGIN_H_

#include <MiaMetadataUtils.h>
#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>
#include <xiaomi/MiMetaData.h>

#include "MiaPluginUtils.h"

using namespace mialgo2;

class MiSwFlipPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.swflip";
    }

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
    ~MiSwFlipPlugin();
    ProcessRetStatus processBuffer(MiImageBuffer *input, MiImageBuffer *output, void *metaData);
    void process(MiImageBuffer *input, MiImageBuffer *output);

    uint64_t mProcessCount; ///< The count for processed frame
    int32_t m_orientation;
    MiaNodeInterface mNodeInterface;
};

PLUMA_INHERIT_PROVIDER(MiSwFlipPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiSwFlipPluginProvider());
    return true;
}

#endif // _MISWFLIPPLUGIN_H_::
