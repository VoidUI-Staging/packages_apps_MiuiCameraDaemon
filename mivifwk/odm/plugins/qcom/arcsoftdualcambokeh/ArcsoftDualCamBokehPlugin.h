#ifndef __ARCSOFTDUALCAMBOKEHPLUGIN_H__
#define __ARCSOFTDUALCAMBOKEHPLUGIN_H__

#include <MiaPluginWraper.h>

#include "mi_type.h"
#include "nodeMetaData.h"

class ArcsoftDualCamBokehPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        return "com.xiaomi.plugin.capbokeh"; // Must match library name
    }
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual void destroy();
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual bool isEnabled(MiaParams settings) { return true; }

private:
    virtual ~ArcsoftDualCamBokehPlugin();
    // void setFlushStatus(bool value) { m_flustStatus = value; }
    S32 ProcessBokeh(DEV_IMAGE_BUF *inputMain, DEV_IMAGE_BUF *inputDepth, DEV_IMAGE_BUF *outputMain,
                     DEV_IMAGE_BUF *outputAux);
    S32 PrepareYUV_OFFSCREEN(DEV_IMAGE_BUF *image, ASVLOFFSCREEN *stImage);
    S32 iniEngine();
    NodeMetaData *m_pNodeMetaData;
    void *m_hCaptureBokeh;
    camera_metadata_t *m_plogicalMeta;
    camera_metadata_t *m_pPhysicalMeta;
    S32 m_ProcessCount;
    AlgoBokehLaunchParams m_bokehlaunchParams;
    MiaNodeInterface m_NodeInterface;
    S32 m_blurLevel;
    S64 m_detectorInit;
    S64 m_detectorRun;
    S64 m_detectorBokehRun;
    S64 m_detectorUninit;
    S64 m_detectorReset;
    S64 m_detectorCropResizeImg;
    S32 m_superNightEnabled;
    S32 m_hdrBokehEnabled;
};

PLUMA_INHERIT_PROVIDER(ArcsoftDualCamBokehPlugin, PluginWraper);

PLUMA_CONNECTOR bool connect(mialgo2::ProviderManager &host)
{
    host.add(new ArcsoftDualCamBokehPluginProvider());
    return true;
}
#endif //__ARCSOFTDUALCAMBOKEHPLUGIN_H__
