#ifndef __MIDUALCAMDEPTHPLUGIN_H__
#define __MIDUALCAMDEPTHPLUGIN_H__

#include <MiaPluginWraper.h>

#include "camxcdktypes.h"
#include "chioemvendortag.h"
#include "device/device.h"
#include "mi_type.h"
#include "mialgo_dynamic_blur_control.h"
#include "nodeMetaData.h"

class MiDualCamDepthPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        return "com.xiaomi.plugin.capdepth"; // Must match library name
    }
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual void destroy();
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual bool isEnabled(MiaParams settings) { return true; }

private:
    virtual ~MiDualCamDepthPlugin();
    S32 InitDepth(DEV_IMAGE_BUF *inputMain, DEV_IMAGE_BUF *inputAux);
    S32 PrepareToMiImage(DEV_IMAGE_BUF *image, AlgoMultiCams::MiImage *stImage);
    S32 GetImageScale(U32 width, U32 height, AlgoMultiCams::ALGO_CAPTURE_FRAME &Scale);
    S64 GetPackData(char *fileName, void **ppData0, U64 *pSize0);
    S64 FreePackData(void **ppData0);
    void setFlushStatus(bool value) { m_flustStatus = value; }
    S32 ProcessDepth(DEV_IMAGE_BUF *inputMain, DEV_IMAGE_BUF *inputAux, DEV_IMAGE_BUF *outputDepth,
                     S32 *depthDataSize);
    NodeMetaData *m_pNodeMetaData;
    void *mCalibrationFront;
    void *mCalibrationWU;
    void *mCalibrationWT;
    U64 m_CalibrationWTLength;
    U64 m_CalibrationWULength;
    U64 m_CalibrationFrontLength;
    void *m_hCaptureDepth;
    S32 m_blurLevel;
    S32 m_superNightEnabled;
    S32 m_flustStatus;
    camera_metadata_t *m_plogicalMeta;
    camera_metadata_t *m_pPhysicalMeta;
    S32 m_ProcessCount;
    S32 m_LiteMode;
    AlgoDepthLaunchParams m_depthlaunchParams;
    MiaNodeInterface m_NodeInterface;
    S32 m_MDMode;
    S64 m_detectorRun;
    S64 m_detectorDepthRun;
    S8 m_bokeh2xFallback;
};

PLUMA_INHERIT_PROVIDER(MiDualCamDepthPlugin, PluginWraper);

PLUMA_CONNECTOR bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiDualCamDepthPluginProvider());
    return true;
}
#endif //__MIDUALCAMDEPTHPLUGIN_H__
