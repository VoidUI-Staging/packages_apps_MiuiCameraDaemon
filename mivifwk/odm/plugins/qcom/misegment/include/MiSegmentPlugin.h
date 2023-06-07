#ifndef _MISEGMENTPLUGIN_H_
#define _MISEGMENTPLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>

#include "MiaPluginUtils.h"
#include "misegmentmiui.h"

class MiSegmentPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.misegment";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

private:
    ~MiSegmentPlugin();
    ProcessRetStatus processBuffer(MiImageBuffer *input, MiImageBuffer *output,
                                   camera_metadata_t *metaData);
    void process(MiImageBuffer *input, MiImageBuffer *output, int32_t sceneflag);

    uint64_t mProcessCount; ///< The count for processed frame
    MiSegmentMiui *mNodeInstance;
    uint32_t mIsPreview;
    int32_t mOrientation;
    int64_t mTimestamp;
};

PLUMA_INHERIT_PROVIDER(MiSegmentPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiSegmentPluginProvider());
    return true;
}

#endif // _MISEGMENTPLUGIN_H_::
