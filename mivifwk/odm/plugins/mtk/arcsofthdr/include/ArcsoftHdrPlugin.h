#ifndef _ARCSOFTHDR_PLUGIN_H_
#define _ARCSOFTHDR_PLUGIN_H_

#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <MiaPluginWraper.h>
#include <MiaProvider.h>
#include <VendorMetadataParser.h>
#include <pluma.h>
#include <system/camera_metadata.h>

#include "arcsofthdr.h"

class ArcsoftHdrPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.hdr";
    }

    virtual int initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *pProcessRequestInfo);

    ProcessRetStatus processRequest(ProcessRequestInfoV2 *pProcessRequestInfo);

    virtual int flushRequest(FlushRequestInfo *pFlushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

    // AEC static definitions
    static const uint8_t ExposureIndexShort = 0; ///< Exposure setting index for short exposure
    static const uint8_t ExposureIndexLong = 1;  ///< Exposure setting index for long exposure
    static const uint8_t ExposureIndexSafe =
        2; ///< Exposure setting index for safe exposure (between short and long)

private:
    ~ArcsoftHdrPlugin();

    void getAEInfo(camera_metadata_t *metaData, arc_hdr_input_meta_t *pAeInfo);
    void setEv(int inputnum, std::map<uint32_t, std::vector<ImageParams>> &phInputBufferMap);

    int processBuffer(struct MiImageBuffer *input, int input_num, struct MiImageBuffer *output,
                      camera_metadata_t *metaData);

    ArcsoftHdr *m_Hdr;
    ARC_HDR_FACEINFO m_FaceInfo;
    int m_cameraMode;
    uint32_t m_frameworkCameraId;
    int m_ev[MAX_EV_NUM];
    int32_t m_inputNum;
    uint32_t m_captureWidth;
    uint32_t m_captureHeight;
    uint8_t m_isHdrSr;
};

PLUMA_INHERIT_PROVIDER(ArcsoftHdrPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new ArcsoftHdrPluginProvider());
    return true;
}

#endif // _ARCSOFTHDR_PLUGIN_H_
