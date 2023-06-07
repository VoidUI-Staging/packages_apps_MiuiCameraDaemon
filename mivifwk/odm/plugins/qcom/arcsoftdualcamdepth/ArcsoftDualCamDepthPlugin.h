#ifndef __ARCSOFTDUALCAMDEPTHPLUGIN_H__
#define __ARCSOFTDUALCAMDEPTHPLUGIN_H__

#include <MiaPluginWraper.h>

#include "camxcdktypes.h"
#include "chioemvendortag.h"
#include "device/device.h"
#include "mi_type.h"
#include "mialgo_dynamic_blur_control.h"
#include "nodeMetaData.h"

typedef struct tag_AlgoDepthSetParam
{
    BokehSensorConfig bokehSensorConfig;
    ARC_DC_CALDATA pCaliData;
} AlgoDepthSetParam;

class ArcsoftDualCamDepthPlugin : public PluginWraper
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
    virtual ~ArcsoftDualCamDepthPlugin();
    S64 GetPackData(char *fileName, void **ppData0, U64 *pSize0);
    S64 FreePackData(void **ppData0);
    void setFlushStatus(bool value) { m_flustStatus = value; }
    S32 ProcessDepth(DEV_IMAGE_BUF *inputMain, DEV_IMAGE_BUF *inputAux, DEV_IMAGE_BUF *outputDepth);
    S32 InitDepth(void);
    S32 PrepareYUV_OFFSCREEN(DEV_IMAGE_BUF *image, ASVLOFFSCREEN *stImage);
    NodeMetaData *m_pNodeMetaData;
    void *mCalibrationWU;
    void *mCalibrationWT;
    U64 m_CalibrationWULength;
    void *m_hCaptureDepth;
    S32 m_blurLevel;
    S32 m_superNightEnabled;
    S32 m_flustStatus;
    camera_metadata_t *m_plogicalMeta;
    camera_metadata_t *m_pPhysicalMeta;
    S32 m_ProcessCount;
    S32 m_LiteMode;
    MiaNodeInterface m_NodeInterface;
    S64 m_detectorRun;
    S64 m_detectorInit;
    S64 m_detectorReset;
    S64 m_detectorUninit;
    S64 m_detectorCalcDisparityData;
    S64 m_detectorGetDisparityDataSize;
    S64 m_detectorGetDisparityData;
    AlgoDepthSetParam m_algoDepthSetParams;
};

PLUMA_INHERIT_PROVIDER(ArcsoftDualCamDepthPlugin, PluginWraper);

PLUMA_CONNECTOR bool connect(mialgo2::ProviderManager &host)
{
    host.add(new ArcsoftDualCamDepthPluginProvider());
    return true;
}
#endif //__ARCSOFTDUALCAMDEPTHPLUGIN_H__
