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

class SkinBeautifierPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.skinbeautifier";
    }

    virtual int Initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus ProcessRequest(ProcessRequestInfo *pProcessRequestInfo);

    virtual ProcessRetStatus ProcessRequest(ProcessRequestInfoV2 *pProcessRequestInfo);

    virtual int FlushRequest(FlushRequestInfo *pFlushRequestInfo);

    virtual void Destory();

    virtual bool IsEnabled(MiaParams settings);

    // AEC static definitions
    static const uint8_t ExposureIndexShort = 0; ///< Exposure setting index for short exposure
    static const uint8_t ExposureIndexLong = 1;  ///< Exposure setting index for long exposure
    static const uint8_t ExposureIndexSafe =
        2; ///< Exposure setting index for safe exposure (between short and long)
public:
    SkinBeautifierPlugin();

private:
    ~SkinBeautifierPlugin();
    ProcessRetStatus ProcessBuffer(ImageParams *input, ImageParams *output, void *metaData);
    bool getBeautyParams(void *pMetaData);
    void getFrameInfo(void *pMetaData);
#if defined(ZIYI_CAM)
    void updateExifData(ProcessRequestInfo *processRequestInfo, ProcessRetStatus resultInfo,
                        double processTime);
    void updateFaceInfo(ProcessRequestInfo *processRequestInfo, MiFDBeautyParam *faceFeatureParams);
#else
    void imageParamsToOffScreen(ASVLOFFSCREEN *offScreen, struct ImageParams *imageParams);
#endif

    void imageParamsToMiBuffer(struct MiImageBuffer *miBuffer, struct ImageParams *imageParams);
    bool getFaceInfo(ProcessRequestInfo *processRequestInfo);
    bool drawFaceBox(char *buffer, MiFDBeautyParam &faceFeatureParams, int width, int height,
                     int stride, bool isIn, char mask);
    bool drawFaceRect(char *buffer, float fx, float fy, float fw, float fh, int w, int h,
                      int stride, char mask);
    bool drawPoints(char *buffer, struct position *points, int num, int w, int h, int stride,
                    char mask);

#if 0
    uint64_t         mProcessCount;   ///< The count for processed frame
    bool             m_isBeautyIntelligent;
    SkinBeautifier*  mSkinBeautifier;
    bool             m_beautyEnabled;
    uint32_t         m_frameworkCameraId;
    uint32_t         mRegion;
    int mOrientation;

    int m_BeautyMode;
    OutputMetadataFaceAnalyze   m_faceAnalyze;
    BeautyFeatureParams mBeautyFeatureParams;
#endif
    uint64_t mProcessCount; ///< The count for processed frame
    SkinBeautifier *mSkinBeautifier;
    bool mBeautyEnabled;
    uint32_t mFrameworkCameraId;

    int mBeautyMode;
    int mOrientation;
    int mRegion;
    int mWidth;
    int mHeight;
    int mAppModule;
    OutputMetadataFaceAnalyze mFaceAnalyze;
    BeautyFeatureParams mBeautyFeatureParams;
    CFrameInfo mFrameInfo;
    MiFDBeautyParam mFaceFeatureParams;
    MiFDFaceLumaInfo mFaceLuma;
    // MiaNodeInterface mNodeInterface;
    bool mDoFaceWhiten;
    int mFaceWhitenRatio;
    bool mIsFrontCamera;
    int32_t sWhitenRatioWithFace;
    int32_t sIsAlgoEnabled;
    std::string sModuleInfo;
    bool m_debugFaceDrawGet;
    bool m_LogEnable;
    uint32_t m_local_operation_mode;
    bool m_isDumpData;
};

PLUMA_INHERIT_PROVIDER(SkinBeautifierPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new SkinBeautifierPluginProvider());
    return true;
}

#endif // _MEMCPYPLUGIN_H_::
