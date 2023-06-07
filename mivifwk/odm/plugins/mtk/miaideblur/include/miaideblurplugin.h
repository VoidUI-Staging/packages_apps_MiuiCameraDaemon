#ifndef _MIAI_DEBLUR_PLUGIN_H_
#define _MIAI_DEBLUR_PLUGIN_H_

#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include <mutex>
#include <thread>

#include "abstypes.h"
#include "miai_buffer.h"
#include "miai_deblur.h"

struct BeautyFeatureInfo
{
    const char *TagName;
    int featureKey;
    bool featureMask;
    int featureValue;
};

struct CameraFrameRatio
{
    int frameIndex;
};

struct BeautyFeatureParams
{
    const int featureCounts = sizeof(beautyFeatureInfo) == 0
                                  ? 0
                                  : sizeof(beautyFeatureInfo) / sizeof(beautyFeatureInfo[0]);

    BeautyFeatureInfo beautyFeatureInfo[7] = {
        {"xiaomi.beauty.skinSmoothRatio", ABS_KEY_BASIC_SKIN_SOFTEN, true, 0},
        {"xiaomi.beauty.enlargeEyeRatio", ABS_KEY_SHAPE_EYE_ENLARGEMENT, true, 0},
        {"xiaomi.beauty.slimFaceRatio", ABS_KEY_SHAPE_FACE_SLENDER, true, 0},
        {"xiaomi.beauty.noseRatio", ABS_KEY_SHAPE_NOSE_AUGMENT_SHAPE, true, 0},
        {"xiaomi.beauty.chinRatio", ABS_KEY_SHAPE_CHIN_LENGTHEN, true, 0},
        {"xiaomi.beauty.lipsRatio", ABS_KEY_SHAPE_LIP_PLUMP, true, 0},
        {"xiaomi.beauty.hairlineRatio", ABS_KEY_SHAPE_HAIRLINE, true, 0}};
};

struct CameraSceneDetectResult
{
    float confidence;
    int resultFlag;
};

class MiAIDeblurPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.miaideblur";
    }

    virtual int initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *pProcessRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *pProcessRequestInfo);

    virtual int flushRequest(FlushRequestInfo *pFlushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

private:
    ~MiAIDeblurPlugin();
    mia_deblur::MIAIDeblurParams mDeblurParams;
    void getMetaData(camera_metadata_t *pMetadata);
    bool isDeblurEnable(camera_metadata_t *pMetadata);
    bool isByPassRequest(camera_metadata_t *pMetadata);
    void initDeblur(mia_deblur::SceneType sceneType);
    void getFaceInfo(camera_metadata_t *metaData, mia_deblur::FaceInfo *faceInfo, int w, int h);
    float mZoom = 0.0f;
    int mIszState = 0;
    uint32_t mFrameworkCameraId;
    int mSceneFlag = 0;
    const int SCENE_COUNT = 1;
    int *initFlags = NULL;

    std::mutex mMutex;
    std::thread *mPostMiDeblurInitThread;
    bool mPostMiDeblurInitFinish;
};

PLUMA_INHERIT_PROVIDER(MiAIDeblurPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiAIDeblurPluginProvider());
    return true;
}

#endif // _TETRASSR_PLUGIN_H_
