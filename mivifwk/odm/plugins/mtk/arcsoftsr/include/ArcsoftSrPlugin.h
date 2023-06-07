#ifndef _ARCSOFTSR_PLUGIN_H_
#define _ARCSOFTSR_PLUGIN_H_

#include <MiaPluginWraper.h>
#include <system/camera_metadata.h>
// lijinglin change
#include "arcsoftsr.h"

class ArcsoftSrPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.arcsoftsr";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

private:
    ~ArcsoftSrPlugin();

    mialgo2::ArcSoftSR *mARCSOFTSR;

    // int ProcessBuffer(struct MiImageBuffer *input, int input_num, struct MiImageBuffer *output,
    //                   void** metaData, void** physMeta);
    // lijinglin change below if need
    // int getMode(void *metaData);
    // int srGetCropRegion(void *, struct MiImageBuffer *input, sr_rect_t *pRect);
    // int srGetISO(void *metaData);

    int mProcessCount;
    // TETRASSR *mTETRASSR;
    uint32_t m_width;
    uint32_t m_height;
    // uint32_t m_cameraId;
    // float m_userZoomRatio;
};

PLUMA_INHERIT_PROVIDER(ArcsoftSrPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new ArcsoftSrPluginProvider());
    return true;
}

#endif // _ARCSOFTSR_PLUGIN_H_
