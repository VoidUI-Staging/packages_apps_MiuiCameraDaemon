#ifndef SKINBEAUTIFIERDEFV2_H
#define SKINBEAUTIFIERDEFV2_H

#include "MiDebugUtils.h"
// #include "BeautyShot_Video_Algorithm.h"
// #include "BeautyShot_Image_Algorithm.h"
/* #include "ammem.h"
#include "asvloffscreen.h"
#include "AISFReferenceInter.h"
#include "AISFCommonDef.h"
#include "abstypes.h" */

#include <TrueSight/TrueSight.hpp>
#include <TrueSight/TrueSightVerifiy.hpp>

#include <TrueSight/Common/ImageUtils.h>
#include <TrueSight/Utils/ImageCodecs.h>
#include <TrueSight/Utils/TimeUtils.hpp>

#include <TrueSight/face/FaceDetector.h>
#include <TrueSight/face/FaceTracker.h>

#include <TrueSight/XReality/XRealityPreview.h>
#include <TrueSight/XReality/XRealityImage.h>

#include <TrueSight/GLUtils/EGLEnv.h>

using namespace midebug;

#undef MIAlgoBFLogTag
#define MIAlgoBFLogTag "MIALGO_SKINBEAUTIFIER"
#define MLOGD_BF(fmt, ...) \
    MLOGD(Mia2LogGroupPlugin, "%s: " fmt, MIAlgoBFLogTag, ##__VA_ARGS__)
#define MLOGI_BF(fmt, ...) \
    MLOGI(Mia2LogGroupPlugin, "%s: " fmt, MIAlgoBFLogTag, ##__VA_ARGS__)
#define MLOGW_BF(fmt, ...) \
    MLOGW(Mia2LogGroupPlugin, "%s: " fmt, MIAlgoBFLogTag, ##__VA_ARGS__)
#define MLOGE_BF(fmt, ...) \
    MLOGE(Mia2LogGroupPlugin, "%s: " fmt, MIAlgoBFLogTag, ##__VA_ARGS__)

#define FD_Max_Landmark  118
#define FD_Max_FaceCount 10
#define FD_Max_FACount   3

#define RELATED_SWITCHES_COUNT 2

struct BeautyFeatureInfo
{
    const char*                 tagName;
    bool                        featureMask;
    int                         featureValue;
    int                         lastFeatureValue;
    bool                        needAdjusted;
    uint32_t                    applicableModeMask;
    TrueSight::ParameterFlag    featureFlag;
};

struct BeautyFeatureParams
{
    const int           featureCounts           =
        sizeof(beautyFeatureInfo) == 0 ? 0 : sizeof(beautyFeatureInfo) / sizeof(beautyFeatureInfo[0]);

    BeautyFeatureInfo   beautyFeatureMode       =
    {    "beautyMode",              true,   0,  -1,  false,  0,  TrueSight::kParameterFlag_Invalid};         /// <美颜类型 0-经典 1-原生

    BeautyFeatureInfo   beautyFeatureInfo[16]   =
    {
        {"skinSmoothRatio",         true,   0,  -1,  false,  63,  TrueSight::kParameterFlag_FaceRetouch},              /// <磨皮 0-100
        {"whitenRatio",             true,   0,  -1,  false,  63,  TrueSight::kParameterFlag_SkinToneMapping},          /// <美白 0-100
        {"stereoPerceptionRatio",   true,   0,  -1,  false,  63,  TrueSight::kParameterFlag_FaceStereoShadow},         /// <立体 0-100
        {"eyeBrowDyeRatio",         true,   0,  -1,  false,  63,  TrueSight::kParameterFlag_MakeupFilterAlpha},        /// <补妆 0-100
        {"slimFaceRatio",           true,   0,  -1,  false,  63,  TrueSight::kParameterFlag_FaceLift},                 /// <瘦脸 0-100
        {"headNarrowRatio",         true,   0,  -1,  false,  63,  TrueSight::kParameterFlag_HeadNarrow},               /// <小头 0-100
        {"hairPuffyRatio",          true,   0,  -1,  false,  51,  TrueSight::kParameterFlag_FacialRefine_SkullEnhance},/// <蓬蓬发顶 0-100
        {"enlargeEyeRatio",         true,   0,  -1,  false,  63,  TrueSight::kParameterFlag_FacialRefine_EyeZoom},     /// <大眼 0-100
        {"noseRatio",               true,   0,  -1,  true,   63,  TrueSight::kParameterFlag_FacialRefine_Nose},        /// <瘦鼻 0-100
        {"noseTipRatio",            true,   0,  -1,  true,   63,  TrueSight::kParameterFlag_FacialRefine_NoseTip},     /// <鼻尖调整 -100-100
        {"templeRatio",             true,   0,  -1,  true,   63,  TrueSight::kParameterFlag_FacialRefine_Temple},      /// <太阳穴 -100-100
        {"cheekBoneRatio",          true,   0,  -1,  true,   63,  TrueSight::kParameterFlag_FacialRefine_CheekBone},   /// <颧骨调整 -100-100
        {"jawRatio",                true,   0,  -1,  true,   63,  TrueSight::kParameterFlag_FacialRefine_Jaw},         /// <下颌调整 -100-100
        {"chinRatio",               true,   0,  -1,  true,   63,  TrueSight::kParameterFlag_FacialRefine_Chin},        /// <下巴 -100-100
        {"lipsRatio",               true,   0,  -1,  true,   63,  TrueSight::kParameterFlag_FacialRefine_MouthSize},   /// <嘴型 -100-100
        {"hairlineRatio",           true,   0,  -1,  true,   63,  TrueSight::kParameterFlag_FacialRefine_HairLine}     /// <发际线 -100-100
        /* {"darkCirclesRatio",        false,  0,  false,  TrueSight::kParameterFlag_LightenPouch},             /// <黑眼圈 0-100
        {"nasolabiaFoldsRatio",     false,  0,  false,  TrueSight::kParameterFlag_Rhytidectomy}              /// <法令纹 0-100 */
    };
    const int           fixedFeatureCounts           =
        sizeof(fixedBeautyFeatureInfo) == 0 ? 0 : sizeof(fixedBeautyFeatureInfo) / sizeof(fixedBeautyFeatureInfo[0]);
    BeautyFeatureInfo   fixedBeautyFeatureInfo[5]    =
    {
        {"BrightEye",               true,  45,  -1, false,  3,  TrueSight::kParameterFlag_BrightEye},                /// <亮眼-固定值45-拍照-默认拍照使用,其他为0
        {"Spotless",                true,  100, -1, false,  63, TrueSight::kParameterFlag_Spotless},                 /// <祛斑去痘-随磨皮值变化
        {"LightenPouch",            true,  70,  -1, false,  55, TrueSight::kParameterFlag_LightenPouch},             /// <黑眼圈-固定值70
        {"Rhytidectomy",            true,  50,  -1, false,  55, TrueSight::kParameterFlag_Rhytidectomy},             /// <法令纹-固定值50
        {"ColorToneAlpha",          true,  100, -1, false,  3,  TrueSight::kParameterFlag_ColorToneAlpha}            /// <全局颜色透明度调整
    };

    const int           beautyRelatedSwitchesCounts                            =
        sizeof(beautyRelatedSwitchesInfo) == 0 ? 0 : sizeof(beautyRelatedSwitchesInfo) / sizeof(beautyRelatedSwitchesInfo[0]);
    BeautyFeatureInfo   beautyRelatedSwitchesInfo[RELATED_SWITCHES_COUNT]      =
    {
        {"makeupGender",            true,  0, -1, false,  0,  TrueSight::kParameterFlag_Magic_MakeupGenderMaleDegree},      /// <是否区分男女妆容: 0.0关闭男女分妆容(男生开启妆容), 1.0开启男女分妆(男生关闭妆容)
        {"removeNevus",             true,  0, -1, false,  0,  TrueSight::kParameterFlag_E2E_RemoveNevus}          /// <拍后：祛除痣: 0.0关闭, 1.0开启
    };
};
struct FrameInfo
{
    TrueSight::CameraType camera_type = TrueSight::CameraType::kCameraInvalid;
    int iso = 150;                // iso when image captured, use default = 150 when unknown;
    int nLuxIndex = 170;          // 室内室外Lux index参数  默认值 170(室外)
    int nExpIndex = 340;          // AE曝光值 default = 340
    int nFlashTag = 0;            // 是否使用闪光灯, 0否, 1是
    float fExposure = 0.0f;       // 曝光时间;  default = 0.0
    float fAdrc;                  // adrc数值 正常范围为0.0-3.0
    int nColorTemperature;        // 色温, 正常数值范围0-10000
};

enum BeautyModes
{
    BEAUTY_MODE_CAPTURE = 0,
    BEAUTY_MODE_VIDEO_CAPTURE,
    BEAUTY_MODE_FRONT_BOKEH_CAPTURE,
    BEAUTY_MODE_REAR_BOKEH_CAPTURE,
    BEAUTY_MODE_PREVIEW,
    BEAUTY_MODE_FRONT_BOKEH_PREVIEW,
    BEAUTY_MODE_REAR_BOKEH_PREVIEW,
    BEAUTY_MODE_RECORD,
    BEAUTY_MODE_NUM,
};

enum AppModule
{
    APP_MODULE_DEFAULT_CAPTURE=0,
    APP_MODULE_RECORD_VIDEO,
    APP_MODULE_PORTRAIT,
    APP_MODULE_SUPER_NIGHT,
    APP_MODULE_SHORT_VIDEO,
    APP_MODULE_DUAL_VIDEO,
    APP_MODULE_DEFAULT_FRONT_CAPTURE,
    APP_MODULE_NUM,
};

enum BeautyEffectMode
{
    BEAUTY_EFFECT_MODE_CLASSICAL=0,
    BEAUTY_EFFECT_MODE_NATIVE,
    BEAUTY_EFFECT_MODE_OTHER_FRONT,
    BEAUTY_EFFECT_MODE_OTHER_REAR,
    BEAUTY_EFFECT_MODE_POPTRAIT_CLASSICAL,
    BEAUTY_EFFECT_MODE_POPTRAIT_NATIVE,
    BEAUTY_EFFECT_MODE_NUM,
};

/* BeautyModeMask
    0 - Classical mode , MASK  1 << 0
    1 - Native mode, MASK      1 << 1
    2 - Front other mode, MASK 1 << 2
    3 - Rear other mode, MASK  1 << 3*/
static uint32_t gBeautyModeMask[BEAUTY_EFFECT_MODE_NUM] =
{
    1 << 0,
    1 << 1,
    1 << 2,
    1 << 3,
    1 << 4,
    1 << 5
};


static float LinearInterpolationRatio(float X1, float X2, float Y1, float Y2, float InputX)
{
    float OutputY = 0.0f;

    if (X1 == X2) {
        OutputY = Y1;
    } else {
        OutputY = ((InputX - X2) / (X1 - X2)) * Y1 + ((InputX - X1) / (X2 - X1)) * Y2;
    }

    return OutputY;
}

static const char* ParameterFlagToString(int flag)
{
    if (flag == TrueSight::kParameterFlag_FaceRetouch) {
        return "FaceRetouch";
    } else if (flag == TrueSight::kParameterFlag_FacialRefine_EyeZoom) {
        return "FacialRefine_EyeZoom";
    } else if (flag == TrueSight::kParameterFlag_FaceLift) {
        return "FaceLift";
    } else if (flag == TrueSight::kParameterFlag_FacialRefine_Nose) {
        return "FacialRefine_Nose";
    } else if (flag == TrueSight::kParameterFlag_FacialRefine_Chin) {
        return "FacialRefine_Chin";
    } else if (flag == TrueSight::kParameterFlag_FacialRefine_MouthSize) {
        return "FacialRefine_MouthSize";
    } else if (flag == TrueSight::kParameterFlag_FacialRefine_HairLine) {
        return "FacialRefine_HairLine";
    } else if (flag == TrueSight::kParameterFlag_SkinToneMapping) {
        return "SkinToneMapping";
    } else if (flag == TrueSight::kParameterFlag_FaceStereoShadow) {
        return "FaceStereoShadow";
    } else if (flag == TrueSight::kParameterFlag_MakeupFilterAlpha) {
        return "MakeupFilterAlpha";
    } else if (flag == TrueSight::kParameterFlag_LightenPouch) {
        return "LightenPouch";
    } else if (flag == TrueSight::kParameterFlag_Rhytidectomy) {
        return "Rhytidectomy";
    } else if (flag == TrueSight::kParameterFlag_Spotless) {
        return "Spotless";
    } else if (flag == TrueSight::kParameterFlag_FacialRefine_Jaw) {
        return "FacialRefine_Jaw";
    } else if (flag == TrueSight::kParameterFlag_HeadNarrow) {
        return "HeadNarrow";
    } else if (flag == TrueSight::kParameterFlag_FacialRefine_NoseTip) {
        return "FacialRefine_NoseTip";
    } else if (flag == TrueSight::kParameterFlag_FacialRefine_Temple) {
        return "FacialRefine_Temple";
    } else if (flag == TrueSight::kParameterFlag_FacialRefine_CheekBone) {
        return "FacialRefine_CheekBone";
    } else if (flag == TrueSight::kParameterFlag_BrightEye) {
        return "BrightEye";
    } else if (flag == TrueSight::kParameterFlag_E2E_RemoveNevus) {
        return "RemoveNevus";
    } else if (flag == TrueSight::kParameterFlag_Magic_MakeupGenderMaleDegree) {
        return "MakeupGender";
    } else if (flag == TrueSight::kParameterFlag_FilterAlpha) {
        return "FilterAlpha";
    } else if (flag == TrueSight::kParameterFlag_ColorToneAlpha) {
        return "ColorToneAlpha";
    }
    return "";
}

#endif // SKINBEAUTIFIERDEFV2_H
