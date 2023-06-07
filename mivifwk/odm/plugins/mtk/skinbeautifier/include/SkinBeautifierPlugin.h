#ifndef _SKINBEAUTIFIERPLUGIN_H_
#define _SKINBEAUTIFIERPLUGIN_H_

#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "SkinBeautifier.h"

/// @brief Defines maximum number of linked session for Multicamera
#define MAX_LINKED_SESSIONS 2
#define MAX_ANALYIZE_FACE   3

using namespace mialgo2;

// MTK_CONFIGURE_FLEX_YUV_EXACT_FORMAT
typedef enum enum_flex_yuv_exact_format {
    MTK_CONFIGURE_FLEX_YUV_YV12,
    MTK_CONFIGURE_FLEX_YUV_NV12,
    MTK_CONFIGURE_FLEX_YUV_NV21,
} enum_flex_yuv_exact_format_t;

class SkinBeautifierPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.skinbeautifier";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

private:
    ~SkinBeautifierPlugin();
    ProcessRetStatus ProcessBuffer(ImageParams *input, ImageParams *output,
                                   camera_metadata_t *metaData);
    void getBeautyParams(camera_metadata_t *pMetaData);
    void imageParamsToOffScreen(ASVLOFFSCREEN *offScreen, struct ImageParams *imageParams);

    void imageParamsToMiBuffer(struct MiImageBuffer *miBuffer, struct ImageParams *imageParams);

    uint64_t mProcessCount; ///< The count for processed frame
    bool m_isBeautyIntelligent;
    SkinBeautifier *mSkinBeautifier;
    bool m_beautyEnabled;
    uint32_t m_frameworkCameraId;
    uint32_t mRegion;
    int mOrientation;

    int m_BeautyMode;
    OutputMetadataFaceAnalyze m_faceAnalyze;
    BeautyFeatureParams mBeautyFeatureParams;
};

PLUMA_INHERIT_PROVIDER(SkinBeautifierPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new SkinBeautifierPluginProvider());
    return true;
}

#endif // _MEMCPYPLUGIN_H_::
