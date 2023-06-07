#ifndef _MIAIIEPLUGIN_H_
#define _MIAIIEPLUGIN_H_

#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>

#include <mutex>

#include "AIIECapture.h"

using namespace mialgo2;

class MiAiiePlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.miaiie";
    }
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

private:
    ~MiAiiePlugin();
    int processBuffer(MiImageBuffer *input, MiImageBuffer *output, camera_metadata_t *metaData);

    std::mutex mProcessMutex;
    int mProcessCount;
    AIIECapture *mNodeInstanceCapture;
    bool mInitAIIECapture;
    unsigned char *mInputDataY;
    unsigned char *mInputDataUV;
    unsigned char *mOutputDataY;
    unsigned char *mOutputDataUV;
};

PLUMA_INHERIT_PROVIDER(MiAiiePlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiAiiePluginProvider());
    return true;
}

#endif // _MIAIIEPLUGIN_H_::
