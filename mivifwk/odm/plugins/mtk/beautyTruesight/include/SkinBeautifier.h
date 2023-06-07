#ifndef SKINBEAUTIFIER_H_
#define SKINBEAUTIFIER_H_

//#include <MiaPluginWraper.h>
#include <stdint.h>

#define ZIYI_CAM 1

//#include <miapluginwraper.h>
//#include <miapluginutils.h>
//#include "miapostproctype.h"
#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <MiaPluginWraper.h>
#include <xiaomi/MiCommonTypes.h>

#include "SkinBeautifierDefV2.h"

#if defined(ZIYI_CAM)
#include "SkinBeautifierDefV2.h"
#else
#include "SkinBeautifierDefV2.h"
#endif

#include <beauty.h>

typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef int32_t INT32;

//#include "chioemvendortag.h"
//#include "chivendortag.h"

namespace TrueSight {
class XRealityImage;
class FaceFeatures;
} // namespace TrueSight

#define CLASSICAL_FIXED_VALUE     0.8
#define NATIVE_FIXED_VALUE        0.3
#define BEAUTY_WHITEN_FIXED_VALUE 30
#define MAX_RESOURCES_PATH_LEN    256
#define MAX_FOREHEAD_POINT_NUM    40
//#define RESOURCES_PATH             "/sdcard/Android/data/com.android.camera/files/resources/"
#define RESOURCES_PATH             "/vendor/etc/camera/resources/"
#define CLASSICAL_PATH             "render/Effect/effect_mode_classical.json"
#define NATIVE_PATH                "render/Effect/effect_mode_native.json"
#define OTHER_FRONT_PATH           "render/Effect/effect_mode_otherFront.json"
#define OTHER_FRONT_CLASSICAL_PATH "render/Effect/effect_mode_classical_otherFront.json"
#define OTHER_FRONT_NATIVE_PATH    "render/Effect/effect_mode_native_otherFront.json"
#define OTHER_REAR_PATH            "render/Effect/effect_mode_otherRear.json"
#define VIDEO_FRONT_PATH           "render/Effect/effect_mode_video.json"

enum REGION_TYPE {
    REGION_GL = 0,
    REGION_CN,
    REGION_IN,
};

using namespace mialgo2;

class SkinBeautifier
{
public:
    SkinBeautifier();
    virtual ~SkinBeautifier();
    void SetFeatureParams(BeautyFeatureParams &beautyFeatureParams, int &appModule);
    bool Initialize(bool isFront, int region);
    int Process(struct MiImageBuffer *input, struct MiImageBuffer *output,
                MiFDBeautyParam &faceDetectedParams, int mode);
    void IsFullFeature(bool flag) { this->mBeautyFullFeatureFlag = flag; };
    void SetCameraPosition(bool isFrontCamera); /// <set camera position is front
    void ResetPreviewOutput();
    void SetImageOrientation(int orientation); /// <set oriention for mialgo process
    void SetFrameInfo(CFrameInfo &frameInfo);  /// <set frame info for mialgo process
    void SetBeautyDimensionEnable(bool beautyDimensionEnable)
    {
        this->mBeautyDimensionEnable = beautyDimensionEnable;
    }; /// <set BeautyDimensionEnable

private:
    bool ProcessCapture(struct MiImageBuffer *input, struct MiImageBuffer *output,
                        MiFDBeautyParam &faceDetectedParams);
    int ProcessPreview(struct MiImageBuffer *input, struct MiImageBuffer *output);
    double GetCurrentSecondValue();
    void LoadBeautyResources(int beautyMode);
    void LoadMakeupResources(int beautyMode, int makeupMode);
    void LoadLightResources(int beautyMode, int lightMode);
    int SetChangedEffectParamsValue(BeautyFeatureInfo &info, bool isSwitches, int beautyMode,
                                    bool isReset);
    void ResetAllParameters(BeautyFeatureParams &beautyFeatureParams);
    int TSConvert2MIFaceFeatures(TrueSight::FaceFeatures faceFeatures, MiFDBeautyParam &miFDParam);
    // bool getUpdateMiFDBeautyParam(MiFDBeautyParam &faceDetectedParams);

    int mWidth;
    int mHeight;
    int mInitReady;
    bool mBeautyFullFeatureFlag;
    bool mIsFrontCamera;
    int mfeatureCounts;
    int mImageOrientation;
    bool mIsReLoadMakeupFilterAlpha; /// <点击无美颜之后，判断是否需要重导入妆容信息
    bool mFaceParsingEnable;
    bool mForeheadEnable;
    bool mEyeRobustEnable;
    bool mMouthRobustEnable;
    bool mIsResetFixInfo; /// <点击无美颜之后，内置固定美颜参数重置

    // bool SkinGlossRatioIsLoad;
    bool mBeautyDimensionEnable;
    int mRegion;
    bool m_genderEnabled;
    bool m_mouthMaskEnabled;
    bool m_foreheadEnabled;

    BeautyFeatureParams *m_pBeautyFeatureParams; /// <mialgo effect params needed, from APP
    BeautyFeatureInfo *m_pFeatureParams;         /// <mialgo effect params needed, from APP

    TrueSight::XRealityImage *m_pBeautyShotCaptureAlgorithm; /// <mialgo capture pointer
    TrueSight::FaceDetector *m_pFaceDetector;                /// <mialgo face detect pointer
    TrueSight::FaceFeatures *m_pFaceFeatures;                /// <mialgo face features pointer
    TrueSight::FrameInfo *m_pFrameInfo;                      /// <mialgo frame info pointer
    bool m_E2EAcneEnabled;
    bool m_LogEnable;
};

#endif /* SKINBEAUTIFIER_H_ */
