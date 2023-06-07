#ifndef _SKINBEAUTIFIERPLUGIN_H_
#define _SKINBEAUTIFIERPLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "MiaPluginUtils.h"
#include "SkinBeautifier.h"

/// @brief Defines maximum number of linked session for Multicamera
#define MAX_LINKED_SESSIONS 2
#define MAX_ANALYIZE_FACE   3

class SkinBeautifierPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.tsskinbeautifier";
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
    ~SkinBeautifierPlugin();
    ProcessRetStatus processBuffer(struct ImageParams *input, struct ImageParams *output,
                                   camera_metadata_t *metaData);
    void getBeautyParams(camera_metadata_t *metaData);
    void getFrameInfo(camera_metadata_t *metaData);
    // void imageParamsToOffScreen(ASVLOFFSCREEN *offScreen, struct ImageParams *imageParams);
    void imageParamsToMiBuffer(struct MiImageBuffer *miBuffer, struct ImageParams *imageParams);
    void updateExifData(ProcessRequestInfo *processRequestInfo, ProcessRetStatus resultInfo,
                        double processTime);


    uint64_t mProcessCount; ///< The count for processed frame
    SkinBeautifier *mSkinBeautifier;
    bool mBeautyEnabled;
    uint32_t mFrameworkCameraId;

    int mSDKYUVMode;
    int mOrientation;
    int mRegion;
    int mWidth;
    int mHeight;
    int mAppModule;
    OutputMetadataFaceAnalyze mFaceAnalyze;
    BeautyFeatureParams mBeautyFeatureParams;
    FrameInfo mFrameInfo;
    MiFDBeautyParam mFaceFeatureParams;

    std::string mInstanceName;
    MiaNodeInterface mNodeInterface;

    bool mIsFrontCamera;
    bool mInitFinish;
};

PLUMA_INHERIT_PROVIDER(SkinBeautifierPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new SkinBeautifierPluginProvider());
    return true;
}

#endif // _SKINBEAUTIFIERPLUGIN_H_
