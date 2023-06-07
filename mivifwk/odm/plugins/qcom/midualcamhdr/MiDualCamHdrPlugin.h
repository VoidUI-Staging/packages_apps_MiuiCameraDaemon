#ifndef __MIDUALCAMHDRPLUGIN_H__
#define __MIDUALCAMHDRPLUGIN_H__

#include <MiaPluginWraper.h>

#include "device/device.h"
#include "nodeMetaData.h"

class MiDualCamHdrPlugin : public PluginWraper
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
    virtual ~MiDualCamHdrPlugin();
    S32 PrepareToHDR_YUV_OFFSCREEN(DEV_IMAGE_BUF *image, HDR_YUV_OFFSCREEN *stImage);
    void setFlushStatus(bool value) { m_flustStatus = value; }
    static MRESULT ProcessHdrCb(MLong lProgress, MLong lStatus, MVoid *pParam);
    S32 ProcessHdr(std::vector<DEV_IMAGE_BUF> inputMainVector, DEV_IMAGE_BUF *outputImage,
                   const std::vector<camera_metadata_t *> &logicalCamMetaVector);

    NodeMetaData *m_pNodeMetaData;
    S32 m_flustStatus;
    camera_metadata_t *m_plogicalMeta;
    camera_metadata_t *m_pPhysicalMeta;
    S32 m_ProcessCount;
    MiaNodeInterface m_NodeInterface;
    S64 m_detectorRun;
    S64 m_detectorHdrRun;
};

PLUMA_INHERIT_PROVIDER(MiDualCamHdrPlugin, PluginWraper);

PLUMA_CONNECTOR bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiDualCamHdrPluginProvider());
    return true;
}
#endif //__MIDUALCAMHDRPLUGIN_H__
