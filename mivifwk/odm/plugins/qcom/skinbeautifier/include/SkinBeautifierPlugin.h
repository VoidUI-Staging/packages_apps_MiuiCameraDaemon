#ifndef _SKINBEAUTIFIERPLUGIN_H_
#define _SKINBEAUTIFIERPLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "MiaPluginUtils.h"

// clang-format off
#include "SkinBeautifier.h"
#include "chivendortag.h"
#include "chioemvendortag.h"
// clang-format on

/// @brief Defines maximum number of linked session for Multicamera
#define MAX_LINKED_SESSIONS 2
#define MAX_ANALYIZE_FACE   3
#define MODE_AI_WATERMARK   0xCD

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
    ProcessRetStatus processBuffer(struct ImageParams *input, struct ImageParams *output,
                                   camera_metadata_t *metaData);
    void getBeautyParams(camera_metadata_t *metaData);
    void imageParamsToOffScreen(ASVLOFFSCREEN *offScreen, struct ImageParams *imageParams);
    void imageParamsToMiBuffer(struct MiImageBuffer *miBuffer, struct ImageParams *imageParams);
    void updateExifData(ProcessRequestInfo *processRequestInfo, ProcessRetStatus resultInfo,
                        double processTime);

    uint64_t mProcessCount; ///< The count for processed frame
    SkinBeautifier *mSkinBeautifier;
    bool mBeautyEnabled;
    uint32_t mFrameworkCameraId;

    int mBeautyMode;
    OutputMetadataFaceAnalyze mFaceAnalyze;
    int mOrientation;
    BeautyFeatureParams mBeautyFeatureParams;
    int mRegion;
    MiaNodeInterface mNodeInterface;
    std::string mInstanceName;
    bool mSdkYuvMode;
    UINT32 m_appModule;
    UINT32 m_appModuleMode;
};

PLUMA_INHERIT_PROVIDER(SkinBeautifierPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new SkinBeautifierPluginProvider());
    return true;
}

#endif // _SKINBEAUTIFIERPLUGIN_H_
