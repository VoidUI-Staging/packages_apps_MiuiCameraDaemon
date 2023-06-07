#ifndef __ARCSOFTDUALCAMHDRPLUGIN_H__
#define __ARCSOFTDUALCAMHDRPLUGIN_H__

#include <MiaPluginWraper.h>

#include "device/device.h"
#include "nodeMetaData.h"

class ArcsoftDualCamHdrPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        return "com.xiaomi.plugin.caphdr"; // Must match library name
    }
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual void destroy();
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual bool isEnabled(MiaParams settings);

private:
    virtual ~ArcsoftDualCamHdrPlugin();
    S32 PrepareToHDR_YUV_OFFSCREEN(DEV_IMAGE_BUF *image, ASVLOFFSCREEN *stImage);
    void setFlushStatus(bool value) { m_flustStatus = value; }
    S32 ProcessHdr(std::vector<DEV_IMAGE_BUF> inputMainVector, DEV_IMAGE_BUF *outputImage,
                   const std::vector<camera_metadata_t *> &logicalCamMetaVector);

    void iniEngine();

    NodeMetaData *m_pNodeMetaData;
    S32 m_flustStatus;
    camera_metadata_t *m_plogicalMeta;
    camera_metadata_t *m_pPhysicalMeta;
    camera_metadata_t *m_outputMetadata;
    S32 m_ProcessCount;
    MiaNodeInterface m_NodeInterface;
    S64 m_detectorRun;
    S64 m_detectorHdrRun;
    S64 m_detectorInit;
    S64 m_detectorPreProc;
    S64 m_detectorGetCropSize;
    S64 m_detectorUninit;
};

PLUMA_INHERIT_PROVIDER(ArcsoftDualCamHdrPlugin, PluginWraper);

PLUMA_CONNECTOR bool connect(mialgo2::ProviderManager &host)
{
    host.add(new ArcsoftDualCamHdrPluginProvider());
    return true;
}
#endif //__ARCSOFTDUALCAMHDRPLUGIN_H__
