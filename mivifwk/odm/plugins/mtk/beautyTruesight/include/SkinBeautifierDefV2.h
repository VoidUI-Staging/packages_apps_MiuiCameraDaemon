#ifndef SKINBEAUTIFIERDEFV2_H
#define SKINBEAUTIFIERDEFV2_H

//#include <miapluginwraper.h>
//#include "MiDebugUtils.h"
// #include "BeautyShot_Video_Algorithm.h"
// #include "BeautyShot_Image_Algorithm.h"

#include <TrueSight/Common/ImageUtils.h>
#include <TrueSight/GLUtils/EGLEnv.h>
#include <TrueSight/Utils/ImageCodecs.h>
#include <TrueSight/XReality/XRealityImage.h>
#include <TrueSight/XReality/XRealityPreview.h>
#include <TrueSight/face/FaceDetector.h>
#include <TrueSight/face/FaceTracker.h>

#include <TrueSight/TrueSight.hpp>
#include <TrueSight/TrueSightVerifiy.hpp>
#include <TrueSight/Utils/TimeUtils.hpp>

// using namespace midebug;

#define ZIYI_CAM 1

#undef MIAlgoBFLogTag
#define MIAlgoBFLogTag     "MIALGO_SKINBEAUTIFIER"
#define MLOGD_BF(fmt, ...) MLOGD(Mia2LogGroupPlugin, "%s: " fmt, MIAlgoBFLogTag, ##__VA_ARGS__)
#define MLOGI_BF(fmt, ...) MLOGI(Mia2LogGroupPlugin, "%s: " fmt, MIAlgoBFLogTag, ##__VA_ARGS__)
#define MLOGW_BF(fmt, ...) MLOGW(Mia2LogGroupPlugin, "%s: " fmt, MIAlgoBFLogTag, ##__VA_ARGS__)
#define MLOGE_BF(fmt, ...) MLOGE(Mia2LogGroupPlugin, "%s: " fmt, MIAlgoBFLogTag, ##__VA_ARGS__)

#define FD_Max_Landmark  118
#define FD_Max_FaceCount 10
#define FD_Max_FACount   3

#define RELATED_SWITCHES_COUNT   2
#define RELATED_LIGHT_COUNT      1
#define FOREHEAD_DETECT_BOUNDARY 4
struct BeautyFeatureInfo
{
    const char *tagName;
    bool featureMask;
    int featureValue;
    int lastFeatureValue;
    bool needAdjusted;
    uint32_t applicableModeMask;
    TrueSight::ParameterFlag featureFlag;
};

struct BeautyFeatureParams
{
    const int featureCounts = sizeof(beautyFeatureInfo) == 0
                                  ? 0
                                  : sizeof(beautyFeatureInfo) / sizeof(beautyFeatureInfo[0]);

    BeautyFeatureInfo beautyFeatureMode = {
        "beautyMode",
        true,
        0,
        -1,
        false,
        0,
        TrueSight::kParameterFlag_Invalid}; /// <美颜类型 0-经典 1-原生

    BeautyFeatureInfo beautyFeatureInfo[16] = {
        {"skinSmoothRatio", true, 0, -1, false, 127,
         TrueSight::kParameterFlag_FaceRetouch}, /// <磨皮 0-100
        {"whitenRatio", true, 0, -1, false, 127,
         TrueSight::kParameterFlag_SkinToneMapping}, /// <美白 0-100
        {"stereoPerceptionRatio", true, 0, -1, false, 127,
         TrueSight::kParameterFlag_FaceStereoShadow}, /// <立体 0-100
        {"eyeBrowDyeRatio", true, 0, -1, false, 127,
         TrueSight::kParameterFlag_MakeupFilterAlpha}, /// <补妆 0-100
        {"slimFaceRatio", true, 0, -1, false, 127,
         TrueSight::kParameterFlag_FaceLift}, /// <瘦脸 0-100
        {"headNarrowRatio", true, 0, -1, false, 127,
         TrueSight::kParameterFlag_HeadNarrow}, /// <小头 0-100
        {"enlargeEyeRatio", true, 0, -1, false, 127,
         TrueSight::kParameterFlag_FacialRefine_EyeZoom}, /// <大眼 0-100
        {"noseRatio", true, 0, -1, true, 127,
         TrueSight::kParameterFlag_FacialRefine_Nose}, /// <瘦鼻 0-100
        {"noseTipRatio", true, 0, -1, true, 127,
         TrueSight::kParameterFlag_FacialRefine_NoseTip}, /// <鼻尖调整 -100-100
        {"templeRatio", true, 0, -1, true, 127,
         TrueSight::kParameterFlag_FacialRefine_Temple}, /// <太阳穴 -100-100
        {"cheekBoneRatio", true, 0, -1, true, 127,
         TrueSight::kParameterFlag_FacialRefine_CheekBone}, /// <颧骨调整 -100-100
        {"jawRatio", true, 0, -1, true, 127,
         TrueSight::kParameterFlag_FacialRefine_Jaw}, /// <下颌调整 -100-100
        {"chinRatio", true, 0, -1, true, 127,
         TrueSight::kParameterFlag_FacialRefine_Chin}, /// <下巴 -100-100
        {"lipsRatio", true, 0, -1, true, 127,
         TrueSight::kParameterFlag_FacialRefine_MouthSize}, /// <嘴型 -100-100
        {"hairlineRatio", true, 0, -1, true, 127,
         TrueSight::kParameterFlag_FacialRefine_HairLine}, /// <发际线 -100-100
        {"hairPuffyRatio", true, 0, -1, false, 3,
         TrueSight::kParameterFlag_FacialRefine_SkullEnhance} /// <蓬蓬发顶 0-100
    };
    const int fixedFeatureCounts =
        sizeof(fixedBeautyFeatureInfo) == 0
            ? 0
            : sizeof(fixedBeautyFeatureInfo) / sizeof(fixedBeautyFeatureInfo[0]);
    BeautyFeatureInfo fixedBeautyFeatureInfo[4] = {
        {"BrightEye", true, 45, -1, false, 51,
         TrueSight::kParameterFlag_BrightEye}, /// <亮眼-固定值45-前置拍照/人像使用
        {"Spotless", true, 100, -1, false, 63,
         TrueSight::kParameterFlag_Spotless}, /// <祛斑去痘-随磨皮值变化
        {"LightenPouch", true, 70, -1, false, 51,
         TrueSight::kParameterFlag_LightenPouch}, /// <黑眼圈-固定值70
        {"Rhytidectomy", true, 30, -1, false, 51,
         TrueSight::kParameterFlag_Rhytidectomy}, /// <法令纹-固定值30
    };

    const int beautyMakeupCounts =
        sizeof(beautyMakeupInfo) == 0 ? 0 : sizeof(beautyMakeupInfo) / sizeof(beautyMakeupInfo[0]);
    BeautyFeatureInfo beautyMakeupMode = {"beautyStyle",
                                          true,
                                          0,
                                          0,
                                          false,
                                          0,
                                          TrueSight::kParameterFlag_Invalid}; /// <妆容类型 0-无妆容
    BeautyFeatureInfo beautyMakeupInfo[2] = {
        {"beautyStyleLevel", true, 0, -1, false, 3,
         TrueSight::kParameterFlag_MakeupFilterAlpha}, /// <妆容滤镜-妆容调整:
                                                       /// 0.0-1.0，和补（美）妆一样效果
        {"filterAlphaRatio", true, 0, -1, false, 3,
         TrueSight::kParameterFlag_FilterAlpha} /// <滤镜透明度:        0.0-1.0
    };

    const int beautyRelatedSwitchesCounts =
        sizeof(beautyRelatedSwitchesInfo) == 0
            ? 0
            : sizeof(beautyRelatedSwitchesInfo) / sizeof(beautyRelatedSwitchesInfo[0]);
    BeautyFeatureInfo beautyRelatedSwitchesInfo[RELATED_SWITCHES_COUNT] = {
        {"makeupGender", true, 0, -1, false, 0,
         TrueSight::
             kParameterFlag_Magic_MakeupGenderMaleDegree}, /// <是否区分男女妆容:
                                                           /// 0.0关闭男女分妆容(男生开启妆容),
                                                           /// 1.0开启男女分妆(男生关闭妆容)
        {"removeNevus", true, 0, -1, false, 0,
         TrueSight::kParameterFlag_E2E_RemoveNevus} /// <拍后：祛除痣: 0.0关闭, 1.0开启
    };

    const int beautyLightCounts =
        sizeof(beautyLightInfo) == 0 ? 0 : sizeof(beautyLightInfo) / sizeof(beautyLightInfo[0]);
    BeautyFeatureInfo beautyLightMode = {"ambientLightingType",
                                         true,
                                         0,
                                         0,
                                         false,
                                         0,
                                         TrueSight::kParameterFlag_Invalid}; /// <光效类型 0-无光效
    BeautyFeatureInfo beautyLightInfo[RELATED_LIGHT_COUNT] = {
        {"lightAlpha", true, 0, -1, false, 3, TrueSight::kParameterFlag_LightAlpha} /// <氛围光效
    };
};
struct CFrameInfo
{
    TrueSight::CameraType camera_type = TrueSight::CameraType::kCameraInvalid;
    int iso = 150;          // iso when image captured, use default = 150 when unknown;
    int nLuxIndex = 170;    // 室内室外Lux index参数  默认值 170(室外)
    int nExpIndex = 340;    // AE曝光值 default = 340
    int nFlashTag = 0;      // 是否使用闪光灯, 0否, 1是
    float fExposure = 0.0f; // 曝光时间;  default = 0.0
    float fAdrc;            // adrc数值 正常范围为0.0-3.0
    int nColorTemperature;  // 色温, 正常数值范围0-10000
};

// static const uint8_t AWBNumCCMRows = 3;       ///< Number of rows for Color Correction Matrix
// static const uint8_t AWBNumCCMCols = 3;       ///< Number of columns for Color Correction Matrix
// static const uint8_t MaxCCMs = 3;             ///< Max number of CCMs supported
// static const uint32_t AWBAlgoDecisionMax = 3; ///< Number of AWB decision output
// static const uint8_t AWBAlgoSAOutputConfSize = 10; ///< Number of AWB SA output confidence size
// ///< @brief Defines the format of the color correction matrix

// typedef struct
// {
//     float rGain; ///< Red gains
//     float gGain; ///< Green gains
//     float bGain; ///< Blue gains
// } AWBGainParams;

// typedef struct
// {
//     bool isCCMOverrideEnabled;               ///< Flag indicates if CCM override is enabled.
//     float CCM[AWBNumCCMRows][AWBNumCCMCols]; ///< The color correction matrix
//     float CCMOffset[AWBNumCCMRows];          ///< The offsets for color correction matrix
// } AWBCCMParams;

// /// @brief Defines the format of the decision data for AWB algorithm.
// typedef struct
// {
//     float rg;
//     float bg;
// } AWBDecisionPoint;

// typedef struct
// {
//     AWBDecisionPoint point[AWBAlgoDecisionMax]; ///< AWB decision point in R/G B/G coordinate
//     float correlatedColorTemperature[AWBAlgoDecisionMax]; ///< Correlated Color Temperature:
//     degrees
//                                                           ///< Kelvin (K)
//     uint32_t decisionCount;                               ///< Number of AWB decision
// } AWBAlgoDecisionInformation;

// typedef struct
// {
//     float SAConf;         ///< Scene Analyzer module output confidence
//     char *pSADescription; ///< Description of Scene Analyzer
// } AWBSAConfInfo;

// typedef struct
// {
//     // Internal Member Variables
//     AWBGainParams AWBGains;                              ///< R/G/B gains
//     uint32_t colorTemperature;                           ///< Color temperature
//     AWBCCMParams AWBCCM[MaxCCMs];                        ///< Color Correction Matrix Value
//     uint32_t numValidCCMs;                               ///< Number of Valid CCMs
//     AWBAlgoDecisionInformation AWBDecision;              ///< AWB decesion information
//     AWBSAConfInfo SAOutputConf[AWBAlgoSAOutputConfSize]; ///< SA output confidence
// } AWBFrameControl;

// struct position
// {
//     float x;
//     float y;
// };

// struct milandmarks
// {
//     struct position points[FD_Max_Landmark];
// };

// struct mouthmask
// {
//     unsigned char      mouth_mask[64][64];
// };

// struct mouthmatrix
// {
//     float              mouth_matrix [6];
// };

// struct milandmarksvis
// {
//     float value[FD_Max_Landmark];
// };

/* struct faceroi
{
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
};

struct dimension
{
    uint32_t width;
    uint32_t height;
};

struct ForeheadPoints
{
    float points[80];
};
struct MiFDBeautyParam
{
    uint32_t miFaceCountNum;
    struct faceroi face_roi[FD_Max_FaceCount];
    struct milandmarks milandmarks[FD_Max_FACount];
    struct milandmarksvis milandmarksvis[FD_Max_FACount];
    struct mouthmask   mouthmask[FD_Max_FACount];
    struct mouthmatrix mouthmatrix[FD_Max_FACount];
    struct ForeheadPoints foreheadPoints[FD_Max_FACount];
    struct dimension   refdimension;
    int32_t            lmk_accuracy_type[FD_Max_FACount];
    float              male_score[FD_Max_FACount];
    float              female_score[FD_Max_FACount];
    float              adult_score[FD_Max_FACount];
    float              child_score[FD_Max_FACount];
    float              age[FD_Max_FACount];
    int32_t            faceid[FD_Max_FaceCount];
    float              rollangle[FD_Max_FaceCount];
    float              confidence[FD_Max_FaceCount];
    bool               isMouthMaskEnabled;
    bool               isGenderEnabled;
    bool               isForeheadEnabled;
    bool               size_adjusted;
}; */

struct MiFDFaceLumaInfo
{
    float faceROISize;
    float facelumaVSframeluma;
    unsigned int cameraId;
    bool AECLocked;
};

enum BeautyModes {
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

enum AppModule {
    APP_MODULE_DEFAULT_CAPTURE = 0,
    APP_MODULE_RECORD_VIDEO,
    APP_MODULE_PORTRAIT,
    APP_MODULE_SUPER_NIGHT,
    APP_MODULE_SHORT_VIDEO,
    APP_MODULE_DUAL_VIDEO,
    APP_MODULE_EIS_VIDEO,
    APP_MODULE_NUM,
};

enum BeautyEffectMode {
    BEAUTY_EFFECT_MODE_CLASSICAL = 0,
    BEAUTY_EFFECT_MODE_NATIVE,
    BEAUTY_EFFECT_MODE_OTHER_FRONT,
    BEAUTY_EFFECT_MODE_OTHER_REAR,
    BEAUTY_EFFECT_MODE_POPTRAIT_CLASSICAL,
    BEAUTY_EFFECT_MODE_POPTRAIT_NATIVE,
    BEAUTY_EFFECT_MODE_VIDEO_FRONT,
    BEAUTY_EFFECT_MODE_NUM,
};

/* BeautyModeMask
    0 - Classical mode , MASK  1 << 0
    1 - Native mode, MASK      1 << 1
    2 - Front other mode, MASK 1 << 2
    3 - Rear other mode, MASK  1 << 3
    4 - Portrait classical mode, MASK 1<< 4
    5 - Portrait native mode, MASK 1<< 5
    6 - Front video mode, MASK 1<< 6*/
static uint32_t gBeautyModeMask[BEAUTY_EFFECT_MODE_NUM] = {1 << 0, 1 << 1, 1 << 2, 1 << 3,
                                                           1 << 4, 1 << 5, 1 << 6};

// typedef enum StreamConfigMode {
//     StreamConfigModeNormal = 0x0000,                ///< Normal stream configuration operation
//     mode StreamConfigModeSAT = 0x8001,                   ///< Xiaomi SAT StreamConfigModeBokeh =
//     0x8002,                 ///< Xiaomi Bokeh StreamConfigModeMiuiManual = 0x8003, ///< Xiaomi
//     Manual mode StreamConfigModeMiuiVideo = 0x8004,             ///< Xiaomi Video mode
//     StreamConfigModeMiuiZslFront = 0x8005,          ///< Xiaomi zsl for front camera
//     StreamConfigModeMiuiFaceUnlock = 0x8006,        ///< Xiaomi faceunlock for front camera
//     StreamConfigModeMiuiQcfaFront = 0x8007,         ///< Xiaomi Qcfa for front camera
//     StreamConfigModeMiuiPanorama = 0x8008,          ///< Xiaomi Panorama mode
//     StreamConfigModeMiuiVideoBeauty = 0x8009,       ///< Xiaomi video beautifier
//     StreamConfigModeMiuiSuperNight = 0x800A,        ///< Xiaomi super night
//     StreamConfigModeMiuiShortVideo = 0x8030,        ///< Xiaomi short video mode
//     StreamConfigModeHFR120FPS = 0x8078,             ///< Xiaomi HFR 120fps
//     StreamConfigModeHFR240FPS = 0x80F0,             ///< Xiaomi HFR 240fps
//     StreamConfigModeFrontBokeh = 0x80F1,            ///< Xiaomi single front Bokeh
//     StreamConfigModeUltraPixelPhotography = 0x80F3, ///< Xiaomi zsl for rear qcfa camera, only
//     used
//                                                     ///< for upscale binning size to full size
//     StreamConfigModeTestMemcpy = 0xEF01,            /// MiPostProcService_test yuv->sw->sw->yuv
//     StreamConfigModeTestCpyJpg = 0xEF02,            /// MiPostProcService_test yuv->sw->hw->jpg
//     StreamConfigModeTestReprocess = 0xEF03, /// MiPostProcService_test yuv->sw->reprocess->yuv
//     StreamConfigModeTestBokeh = 0xEF04,     /// MiPostProcService_test dual bokeh
//     StreamConfigModeTestMulti = 0xEF05,     /// MiPostProcService_test multi frame plugin

//     StreamConfigModeThirdPartyNormal = 0xFF0A,
//     StreamConfigModeThirdPartyMFNR = 0xFF0B,
//     StreamConfigModeThirdPartySuperNight = 0xFF0C,  ///< thirdparty super night
//     StreamConfigModeThirdPartySmartEngine = 0xFF0E, ///< thridparty Smart Engine
//     StreamConfigModeThirdPartyFastMode = 0xFF0F,    ///< thridparty fast mode
//     StreamConfigModeThirdPartyBokeh = 0xFF12,       ///< thridparty bokeh mode
//     StreamConfigModeVendorEnd = 0xEFFF, ///< End value for vendor-defined stream configuration
//     modes
// } STREAMCONFIGMODE;

enum BeautyStyleType {
    BEAUTY_STYLE_MODE_NATIVE = 0,
    BEAUTY_STYLE_MODE_DY,
    BEAUTY_STYLE_MODE_XZ,
    BEAUTY_STYLE_MODE_YQ,
    BEAUTY_STYLE_MODE_RM,
    BEAUTY_STYLE_MODE_QJ,
    BEAUTY_STYLE_MODE_YK,
    BEAUTY_STYLE_MAX,
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

static bool GetFaceWhitenRatioByFaceLumaInfo(MiFDFaceLumaInfo faceLumaInfo, int luxIndex,
                                             int *faceWhitenRatio)
{
    // Wide
    const float OutdoorLuxIndexWide = 200.0;
    const float IndoorLuxIndexStartWide = 240.0;
    const float IndoorLuxIndexEndWide = 330.0;
    const float LowlightLuxIndexWide = 375.0;

    const float OutdoorFaceBLbrightnessWide = 70.0;
    const float OutdoorFaceNMbrightnessWide = 15.0;
    const float OutdoorFaceFLbrightnessWide = 0.0;

    const float IndoorFaceBLbrightnessWide = 65.0;
    const float IndoorFaceNMbrightnessWide = 20.0;
    const float IndoorFaceFLbrightnessWide = 5.0;

    const float LowlightFaceBLbrightnessWide = 60.0;
    const float LowlightFaceNMbrightnessWide = 50.0;
    const float LowlightFaceFLbrightnessWide = 40.0;

    const float FaceBLRatioWide = 0.4;
    const float FaceNMRatioStartWide = 0.8;
    const float FaceNMRatioEndWide = 1.4;
    const float FaceFLRatioWide = 2.5;

    const float FaceMinSizeWide = 0.03;

    // UltraWide
    const float OutdoorLuxIndexUltraWide = 200.0;
    const float IndoorLuxIndexStartUltraWide = 240.0;
    const float IndoorLuxIndexEndUltraWide = 280.0;
    const float LowlightLuxIndexUltraWide = 330.0;

    const float OutdoorFaceBLbrightnessUltraWide = 25.0;
    const float OutdoorFaceNMbrightnessUltraWide = 0.0;
    const float OutdoorFaceFLbrightnessUltraWide = 0.0;

    const float IndoorFaceBLbrightnessUltraWide = 25.0;
    const float IndoorFaceNMbrightnessUltraWide = 0.0;
    const float IndoorFaceFLbrightnessUltraWide = 0.0;

    const float LowlightFaceBLbrightnessUltraWide = 15.0;
    const float LowlightFaceNMbrightnessUltraWide = 0.0;
    const float LowlightFaceFLbrightnessUltraWide = 0.0;

    const float FaceBLRatioUltraWide = 0.4;
    const float FaceNMRatioStartUltraWide = 0.6;
    const float FaceNMRatioEndUltraWide = 1.4;
    const float FaceFLRatioUltraWide = 2.5;

    const float FaceMinSizeUltraWide = 0.03;

    static float StorefacelumaVSframeluma = 0.0;
    float faceLumaVSframeLuma = 0.0;
    float facebrightness = 0.0;

    bool doFaceWhiten = false;
    float outdoorLuxIndex = 0.0;
    float indoorLuxIndexStart = 0.0;
    float indoorLuxIndexEnd = 0.0;
    float lowlightLuxIndex = 0.0;

    float outdoorFaceBLbrightness = 0.0;
    float outdoorFaceNMbrightness = 0.0;
    float outdoorFaceFLbrightness = 0.0;

    float indoorFaceBLbrightness = 0.0;
    float indoorFaceNMbrightness = 0.0;
    float indoorFaceFLbrightness = 0.0;

    float lowlightFaceBLbrightness = 0.0;
    float lowlightFaceNMbrightness = 0.0;
    float lowlightFaceFLbrightness = 0.0;

    float faceBLRatio = 0.0;
    float faceNMRatioStart = 0.0;
    float faceNMRatioEnd = 0.0;
    float faceFLRatio = 0.0;

    float faceMinSize = 0.0;

    if (faceLumaInfo.cameraId == 0) {
        // Wide
        outdoorLuxIndex = OutdoorLuxIndexWide;
        indoorLuxIndexStart = IndoorLuxIndexStartWide;
        indoorLuxIndexEnd = IndoorLuxIndexEndWide;
        lowlightLuxIndex = LowlightLuxIndexWide;

        outdoorFaceBLbrightness = OutdoorFaceBLbrightnessWide;
        outdoorFaceNMbrightness = OutdoorFaceNMbrightnessWide;
        outdoorFaceFLbrightness = OutdoorFaceFLbrightnessWide;

        indoorFaceBLbrightness = IndoorFaceBLbrightnessWide;
        indoorFaceNMbrightness = IndoorFaceNMbrightnessWide;
        indoorFaceFLbrightness = IndoorFaceFLbrightnessWide;

        lowlightFaceBLbrightness = LowlightFaceBLbrightnessWide;
        lowlightFaceNMbrightness = LowlightFaceNMbrightnessWide;
        lowlightFaceFLbrightness = LowlightFaceFLbrightnessWide;

        faceBLRatio = FaceBLRatioWide;
        faceNMRatioStart = FaceNMRatioStartWide;
        faceNMRatioEnd = FaceNMRatioEndWide;
        faceFLRatio = FaceFLRatioWide;

        faceMinSize = FaceMinSizeWide;
    } else if (faceLumaInfo.cameraId == 3) {
        // UltraWide
        outdoorLuxIndex = OutdoorLuxIndexUltraWide;
        indoorLuxIndexStart = IndoorLuxIndexStartUltraWide;
        indoorLuxIndexEnd = IndoorLuxIndexEndUltraWide;
        lowlightLuxIndex = LowlightLuxIndexUltraWide;

        outdoorFaceBLbrightness = OutdoorFaceBLbrightnessUltraWide;
        outdoorFaceNMbrightness = OutdoorFaceNMbrightnessUltraWide;
        outdoorFaceFLbrightness = OutdoorFaceFLbrightnessUltraWide;

        indoorFaceBLbrightness = IndoorFaceBLbrightnessUltraWide;
        indoorFaceNMbrightness = IndoorFaceNMbrightnessUltraWide;
        indoorFaceFLbrightness = IndoorFaceFLbrightnessUltraWide;

        lowlightFaceBLbrightness = LowlightFaceBLbrightnessUltraWide;
        lowlightFaceNMbrightness = LowlightFaceNMbrightnessUltraWide;
        lowlightFaceFLbrightness = LowlightFaceFLbrightnessUltraWide;

        faceBLRatio = FaceBLRatioUltraWide;
        faceNMRatioStart = FaceNMRatioStartUltraWide;
        faceNMRatioEnd = FaceNMRatioEndUltraWide;
        faceFLRatio = FaceFLRatioUltraWide;

        faceMinSize = FaceMinSizeUltraWide;
    }

    if ((faceLumaInfo.AECLocked == 1) && (faceLumaInfo.faceROISize > 0)) {
        faceLumaVSframeLuma = StorefacelumaVSframeluma;
    }

    if ((faceLumaInfo.faceROISize > faceMinSize) && (faceLumaInfo.facelumaVSframeluma > 0)) {
        float FaceTempValue1 = 0;
        float FaceTempValue2 = 0;
        faceLumaVSframeLuma = faceLumaInfo.facelumaVSframeluma;
        StorefacelumaVSframeluma = faceLumaVSframeLuma;

        if (luxIndex < outdoorLuxIndex) {
            if (faceLumaVSframeLuma < faceBLRatio) {
                facebrightness = outdoorFaceBLbrightness;
            } else if ((faceLumaVSframeLuma >= faceBLRatio) &&
                       (faceLumaVSframeLuma < faceNMRatioStart)) {
                facebrightness =
                    LinearInterpolationRatio(faceBLRatio, faceNMRatioStart, outdoorFaceBLbrightness,
                                             outdoorFaceNMbrightness, faceLumaVSframeLuma);
            } else if ((faceLumaVSframeLuma >= faceNMRatioStart) &&
                       (faceLumaVSframeLuma < faceNMRatioEnd)) {
                facebrightness = outdoorFaceNMbrightness;
            } else if ((faceLumaVSframeLuma >= faceNMRatioEnd) &&
                       (faceLumaVSframeLuma < faceFLRatio)) {
                facebrightness =
                    LinearInterpolationRatio(faceNMRatioEnd, faceFLRatio, outdoorFaceNMbrightness,
                                             outdoorFaceFLbrightness, faceLumaVSframeLuma);
            } else if (faceLumaVSframeLuma >= faceFLRatio) {
                facebrightness = outdoorFaceFLbrightness;
            }
        } else if ((luxIndex >= outdoorLuxIndex) && (luxIndex < indoorLuxIndexStart)) {
            if (faceLumaVSframeLuma < faceBLRatio) {
                facebrightness = LinearInterpolationRatio(outdoorLuxIndex, indoorLuxIndexStart,
                                                          outdoorFaceBLbrightness,
                                                          indoorFaceBLbrightness, luxIndex);
            } else if ((faceLumaVSframeLuma >= faceBLRatio) &&
                       (faceLumaVSframeLuma < faceNMRatioStart)) {
                FaceTempValue1 = LinearInterpolationRatio(outdoorLuxIndex, indoorLuxIndexStart,
                                                          outdoorFaceBLbrightness,
                                                          indoorFaceBLbrightness, luxIndex);
                FaceTempValue2 = LinearInterpolationRatio(outdoorLuxIndex, indoorLuxIndexStart,
                                                          outdoorFaceNMbrightness,
                                                          indoorFaceNMbrightness, luxIndex);
                facebrightness =
                    LinearInterpolationRatio(faceBLRatio, faceNMRatioStart, FaceTempValue1,
                                             FaceTempValue2, faceLumaVSframeLuma);
            } else if ((faceLumaVSframeLuma >= faceNMRatioStart) &&
                       (faceLumaVSframeLuma < faceNMRatioEnd)) {
                facebrightness = LinearInterpolationRatio(outdoorLuxIndex, indoorLuxIndexStart,
                                                          outdoorFaceNMbrightness,
                                                          indoorFaceNMbrightness, luxIndex);
            } else if ((faceLumaVSframeLuma >= faceNMRatioEnd) &&
                       (faceLumaVSframeLuma < faceFLRatio)) {
                FaceTempValue1 = LinearInterpolationRatio(outdoorLuxIndex, indoorLuxIndexStart,
                                                          outdoorFaceNMbrightness,
                                                          indoorFaceNMbrightness, luxIndex);
                FaceTempValue2 = LinearInterpolationRatio(outdoorLuxIndex, indoorLuxIndexStart,
                                                          outdoorFaceFLbrightness,
                                                          indoorFaceFLbrightness, luxIndex);
                facebrightness =
                    LinearInterpolationRatio(faceNMRatioEnd, faceFLRatio, FaceTempValue1,
                                             FaceTempValue2, faceLumaVSframeLuma);
            } else if (faceLumaVSframeLuma >= faceFLRatio) {
                facebrightness = LinearInterpolationRatio(outdoorLuxIndex, indoorLuxIndexStart,
                                                          outdoorFaceFLbrightness,
                                                          indoorFaceFLbrightness, luxIndex);
            }
        } else if ((luxIndex >= indoorLuxIndexStart) && (luxIndex < indoorLuxIndexEnd)) {
            if (faceLumaVSframeLuma < faceBLRatio) {
                facebrightness = indoorFaceBLbrightness;
            } else if ((faceLumaVSframeLuma >= faceBLRatio) &&
                       (faceLumaVSframeLuma < faceNMRatioStart)) {
                facebrightness =
                    LinearInterpolationRatio(faceBLRatio, faceNMRatioStart, indoorFaceBLbrightness,
                                             indoorFaceNMbrightness, faceLumaVSframeLuma);
            } else if ((faceLumaVSframeLuma >= faceNMRatioStart) &&
                       (faceLumaVSframeLuma < faceNMRatioEnd)) {
                facebrightness = indoorFaceNMbrightness;
            } else if ((faceLumaVSframeLuma >= faceNMRatioEnd) &&
                       (faceLumaVSframeLuma < faceFLRatio)) {
                facebrightness =
                    LinearInterpolationRatio(faceNMRatioEnd, faceFLRatio, indoorFaceNMbrightness,
                                             indoorFaceFLbrightness, faceLumaVSframeLuma);
            } else if (faceLumaVSframeLuma >= faceFLRatio) {
                facebrightness = indoorFaceFLbrightness;
            }
        } else if ((luxIndex >= indoorLuxIndexEnd) && (luxIndex < lowlightLuxIndex)) {
            if (faceLumaVSframeLuma < faceBLRatio) {
                facebrightness = LinearInterpolationRatio(indoorLuxIndexEnd, lowlightLuxIndex,
                                                          indoorFaceBLbrightness,
                                                          lowlightFaceBLbrightness, luxIndex);
            } else if ((faceLumaVSframeLuma >= faceBLRatio) &&
                       (faceLumaVSframeLuma < faceNMRatioStart)) {
                FaceTempValue1 = LinearInterpolationRatio(indoorLuxIndexEnd, lowlightLuxIndex,
                                                          indoorFaceBLbrightness,
                                                          lowlightFaceBLbrightness, luxIndex);
                FaceTempValue2 = LinearInterpolationRatio(indoorLuxIndexEnd, lowlightLuxIndex,
                                                          indoorFaceNMbrightness,
                                                          lowlightFaceNMbrightness, luxIndex);
                facebrightness =
                    LinearInterpolationRatio(faceBLRatio, faceNMRatioStart, FaceTempValue1,
                                             FaceTempValue2, faceLumaVSframeLuma);
            } else if ((faceLumaVSframeLuma >= faceNMRatioStart) &&
                       (faceLumaVSframeLuma < faceNMRatioEnd)) {
                facebrightness = LinearInterpolationRatio(indoorLuxIndexEnd, lowlightLuxIndex,
                                                          indoorFaceNMbrightness,
                                                          lowlightFaceNMbrightness, luxIndex);
            } else if ((faceLumaVSframeLuma >= faceNMRatioEnd) &&
                       (faceLumaVSframeLuma < faceFLRatio)) {
                FaceTempValue1 = LinearInterpolationRatio(indoorLuxIndexEnd, lowlightLuxIndex,
                                                          indoorFaceNMbrightness,
                                                          lowlightFaceNMbrightness, luxIndex);
                FaceTempValue2 = LinearInterpolationRatio(indoorLuxIndexEnd, lowlightLuxIndex,
                                                          indoorFaceFLbrightness,
                                                          lowlightFaceFLbrightness, luxIndex);
                facebrightness =
                    LinearInterpolationRatio(faceBLRatio, faceNMRatioStart, FaceTempValue1,
                                             FaceTempValue2, faceLumaVSframeLuma);
            } else if (faceLumaVSframeLuma >= faceFLRatio) {
                facebrightness = LinearInterpolationRatio(indoorLuxIndexEnd, lowlightLuxIndex,
                                                          indoorFaceFLbrightness,
                                                          lowlightFaceFLbrightness, luxIndex);
            }
        } else if (luxIndex > lowlightLuxIndex) {
            if (faceLumaVSframeLuma < faceBLRatio) {
                facebrightness = lowlightFaceBLbrightness;
            } else if ((faceLumaVSframeLuma >= faceBLRatio) &&
                       (faceLumaVSframeLuma < faceNMRatioStart)) {
                facebrightness = LinearInterpolationRatio(
                    faceBLRatio, faceNMRatioStart, lowlightFaceBLbrightness,
                    lowlightFaceNMbrightness, faceLumaVSframeLuma);
            } else if ((faceLumaVSframeLuma >= faceNMRatioStart) &&
                       (faceLumaVSframeLuma < faceNMRatioEnd)) {
                facebrightness = lowlightFaceNMbrightness;
            } else if ((faceLumaVSframeLuma >= faceNMRatioEnd) &&
                       (faceLumaVSframeLuma < faceFLRatio)) {
                facebrightness =
                    LinearInterpolationRatio(faceNMRatioEnd, faceFLRatio, lowlightFaceNMbrightness,
                                             lowlightFaceFLbrightness, faceLumaVSframeLuma);
            } else if (faceLumaVSframeLuma >= faceFLRatio) {
                facebrightness = lowlightFaceFLbrightness;
            }
        }
    }
    if (facebrightness != 0) {
        doFaceWhiten = true;
    } else {
        doFaceWhiten = false;
    }
    *faceWhitenRatio = static_cast<int>(facebrightness);

    // MLOGD_BF("mDoFaceWhiten: %d, faceWhitenRatio: %d", doFaceWhiten, *faceWhitenRatio);

    return doFaceWhiten;
}

static const char *ParameterFlagToString(int flag)
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
#if defined(ZIYI_CAM)
    else if (flag == TrueSight::kParameterFlag_FacialRefine_SkullEnhance) {
        return "FacialRefine_SkullEnhance";
    } else if (flag == TrueSight::kParameterFlag_LightAlpha) {
        return "LightAlpha";
    }
#endif
    return "";
}

enum LightStyleType {
    LIGHT_STYLE_MODE_NATIVE = 0,
    LIGHT_STYLE_MODE_GENDER,
    LIGHT_STYLE_MODE_COLD,
    LIGHT_STYLE_MODE_WARM,
    LIGHT_STYLE_MODE_BLUE,
    LIGHT_STYLE_MODE_PURPLE,
    LIGHT_STYLE_MAX,
};

static const char *GetBeautyStyleData(int beautyStyleType)
{
    const char *beautyStylePath[BEAUTY_STYLE_MAX] = {
        "/vendor/etc/camera/resources/render/Effect/60_Makeup/mode_native.json",
        "/vendor/etc/camera/resources/render/Effect/60_Makeup/mode_DY.json",
        "/vendor/etc/camera/resources/render/Effect/60_Makeup/mode_XZ.json",
        "/vendor/etc/camera/resources/render/Effect/60_Makeup/mode_YQ.json",
        "/vendor/etc/camera/resources/render/Effect/60_Makeup/mode_RM.json",
        "/vendor/etc/camera/resources/render/Effect/60_Makeup/mode_QJ.json",
        "/vendor/etc/camera/resources/render/Effect/60_Makeup/mode_YK.json"};

    if (0 < beautyStyleType && beautyStyleType < BEAUTY_STYLE_MAX) {
        return beautyStylePath[beautyStyleType];
    }
    return beautyStylePath[BEAUTY_STYLE_MODE_NATIVE];
}

static const char *GetLightStyleData(int lightStyleType)
{
    const char *lightStylePath[LIGHT_STYLE_MAX] = {
        "",
        "/vendor/etc/camera/resources/render/Effect/65_Rule/mode_ZS.json",
        "/vendor/etc/camera/resources/render/Effect/65_Rule/mode_LS.json",
        "/vendor/etc/camera/resources/render/Effect/65_Rule/mode_HF.json",
        "/vendor/etc/camera/resources/render/Effect/65_Rule/mode_YY.json",
        "/vendor/etc/camera/resources/render/Effect/65_Rule/mode_NX.json",

    };

    if (0 < lightStyleType && lightStyleType < LIGHT_STYLE_MAX) {
        return lightStylePath[lightStyleType];
    }
    return lightStylePath[LIGHT_STYLE_MODE_NATIVE];
}

#endif // SKINBEAUTIFIERDEFV2_H
