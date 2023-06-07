#ifndef _ARCSOFTHDR_PLUGIN_H_
#define _ARCSOFTHDR_PLUGIN_H_

#include <MiaPluginWraper.h>
#include <MiaProvider.h>
#include <VendorMetadataParser.h>
#include <pluma.h>
#include <system/camera_metadata.h>

#include "arcsofthdr.h"
#include "chistatsproperty.h"
#include "chistatspropertydefines.h"

#define MATHL_MAX(a, b) ((a) > (b) ? (a) : (b))
#define MATH_MIN(a, b)  ((a) < (b) ? (a) : (b))

using MIRECT = CHIRECT;

class ArcsoftHdrPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.arcsofthdr";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

    // AEC static definitions
    static const uint8_t ExposureIndexShort = 0; ///< Exposure setting index for short exposure
    static const uint8_t ExposureIndexLong = 1;  ///< Exposure setting index for long exposure
    static const uint8_t ExposureIndexSafe =
        2; ///< Exposure setting index for safe exposure (between short and long)

private:
    ~ArcsoftHdrPlugin();

    void getHDRInfo(camera_metadata_t *metaData, int *camMode);
    void setEv(int inputnum, std::vector<ImageParams> &inputBuffer);

    int processBuffer(struct MiImageBuffer *input, int inputNum, struct MiImageBuffer *output,
                      camera_metadata_t *metaData);
    void getOrientation(MInt32 *pOrientation, camera_metadata_t *metaData);

    void DrawRect(struct MiImageBuffer *pOffscreen, MRECT RectDraw, MInt32 nLineWidth,
                  MInt32 iType);
    MRECT intersection(const MRECT &a, const MRECT &d);

    bool isSdkAlgoUp(camera_metadata_t *metadata);

    ArcsoftHdr *mHdr;
    bool mFrontCamera;
    uint32_t mFrameworkCameraId;
    int mEV[MAX_EV_NUM];
    ImageFormat m_format;
    MI_HDR_INFO m_HdrInfo;
    int mDrawDump;
};

PLUMA_INHERIT_PROVIDER(ArcsoftHdrPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new ArcsoftHdrPluginProvider());
    return true;
}

#endif // _ARCSOFTHDR_PLUGIN_H_
