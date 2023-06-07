#ifndef __MIDUALCAMBOKEHPLUGIN_H__
#define __MIDUALCAMBOKEHPLUGIN_H__

#include <MiaPluginWraper.h>

#include "MiMDBokeh.h"
#include "chioemvendortag.h"
#include "mi_type.h"

class MiDualCamBokehPlugin : public PluginWraper
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
    virtual ~MiDualCamBokehPlugin();
    S32 InitBokeh(DEV_IMAGE_BUF *inputMain);
    S32 PrepareToMiImage(DEV_IMAGE_BUF *image, AlgoMultiCams::MiImage *stImage);
    S32 GetImageScale(U32 width, U32 height, AlgoMultiCams::ALGO_CAPTURE_FRAME &Scale);
    void setFlushStatus(bool value) { m_flustStatus = value; }
    S32 ProcessBokeh(DEV_IMAGE_BUF *inputMain, DEV_IMAGE_BUF *inputDepth,
                     DEV_IMAGE_BUF *outputMain);
    NodeMetaData *m_pNodeMetaData;
    MiMDBokeh *m_pMiMDBokeh;
    void *m_hCaptureBokeh;
    S32 m_blurLevel;
    S32 m_superNightEnabled;
    S32 m_flustStatus;
    camera_metadata_t *m_plogicalMeta;
    camera_metadata_t *m_pPhysicalMeta;
    S32 m_ProcessCount;
    S32 m_LiteMode;
    S32 m_MDLiteMode;
    AlgoBokehLaunchParams m_bokehlaunchParams;
    MiaNodeInterface m_NodeInterface;
    S32 m_MDMode;
    S64 m_detectorInit;
    S64 m_detectorRun;
    S64 m_detectorBokehRun;
    S8 m_bokeh2xFallback;
};

PLUMA_INHERIT_PROVIDER(MiDualCamBokehPlugin, PluginWraper);

PLUMA_CONNECTOR bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiDualCamBokehPluginProvider());
    return true;
}
#endif //__MIDUALCAMBOKEHPLUGIN_H__
