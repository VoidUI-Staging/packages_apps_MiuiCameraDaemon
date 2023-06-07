#ifndef __FRONTBOKEHPLUGIN_H__
#define __FRONTBOKEHPLUGIN_H__

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>
#include "device/device.h"
#include "mialgo_det_seg_deploy.h"
#include "monocam_bokeh_proc.h"
#include "nodeMetaData.h"

class FrontBokehPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        return "com.xiaomi.plugin.frontbokeh"; // Must match library name
    }
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual void destroy();
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual bool isEnabled(MiaParams settings);

private:
    virtual ~FrontBokehPlugin();
    int LibDepthInit(camera_metadata_t *pMetadata, DEV_IMAGE_BUF *pInputFrame);
    int LibDepthProcess(camera_metadata_t *pMetadata, DEV_IMAGE_BUF *pInputImage,
                        MialgoImg_AIO *pPotraitMaskImg);
    int LibBlurInit(camera_metadata_t *pMetadata, DEV_IMAGE_BUF *pInputImage);
    int LibBlurProcess(camera_metadata_t *pMetadata, DEV_IMAGE_BUF *pInputImage,
                       MialgoImg_AIO *pPotraitMaskImg, DEV_IMAGE_BUF *pOutputImage,
                       DEV_IMAGE_BUF *pOutputDepthImage);
    S32 PrepareToMiImage(DEV_IMAGE_BUF *pImage, AlgoMultiCams::MiImage *pStImage);

    MiaNodeInterface m_NodeInterface;
    NodeMetaData *m_pNodeMetaData;
    void *m_hDepthEngine;
    void *m_hBlurEngine;
    int m_ProcessedFrame;
    U32 m_depthInitRetry;
    U32 m_BlurInitRetry;
};

PLUMA_INHERIT_PROVIDER(FrontBokehPlugin, PluginWraper);

PLUMA_CONNECTOR bool connect(mialgo2::ProviderManager &host)
{
    host.add(new FrontBokehPluginProvider());
    return true;
}

#endif //__FRONTBOKEHPLUGIN_H__