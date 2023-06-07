#ifndef SKINBEAUTIFIERDEF_H
#define SKINBEAUTIFIERDEF_H

#include "MiDebugUtils.h"
#include "BeautyShot_Video_Algorithm.h"
#include "BeautyShot_Image_Algorithm.h"

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

#define XIAOMI_FB_FEATURE_LEVEL_1 0
// preview: soften max=70 deblemish=0
// capture: soften max=70 deblemish=1

#define XIAOMI_FB_FEATURE_LEVEL_2 1
// preview: soften max=70 deblemish=0 faceslender=?  eyelarge=?
// capture: soften max=70 deblemish=1 faceslender=?  eyelarge=?

#define XIAOMI_FB_FEATURE_LEVEL_3 2
// only for capture

#define XIAOMI_FB_FEATURE_LEVEL XIAOMI_FB_FEATURE_LEVEL_2

struct BeautyFeatureInfo
{
    const char *tagName;
    int featureKey;
    bool featureMask;
    int featureValue;
};

struct BeautyStyleInfo
{
    const char* styleTagName;
    int         styleValue;
    const char* styleLevelTagName;
    int         styleLevelFeatureKey;
    int         styleLevelValue;
};



struct BeautyStyleFileData
{
    char* styleFileData = NULL;
    int styleFileSize = 0;
};


struct BeautyFeatureParams
{
    const int           featureCounts           =
        sizeof(beautyFeatureInfo) == 0 ? 0 : sizeof(beautyFeatureInfo) / sizeof(beautyFeatureInfo[0]);

    BeautyFeatureInfo   beautyFeatureMode       =
        {    "beautyMode",  0,   true,   0};                 /// <美颜类型 0-经典 1-原生 2-后置

    BeautyFeatureInfo   beautyFeatureInfo[8]    =
    {
#if defined(RENOIR_CAM)
        {"skinColorType",   ABS_KEY_BASIC_FOUNDATION_COLOR,     true,   0},
      //{"skinColorRatio",  ABS_KEY_BASIC_FOUNDATION,           true,   0},
       // {"skinGlossRatio",  ABS_KEY_FACIAL_GLOSS_LIGHT,         false,  0},
#else
        {"skinColorType",   ABS_KEY_BASIC_FOUNDATION_COLOR,     false,  0},
      //{"skinColorRatio",  ABS_KEY_BASIC_FOUNDATION_COLOR,     false,  0},
       // {"skinGlossRatio",  ABS_KEY_FACIAL_GLOSS_LIGHT,         false,  0},
#endif
        {"skinSmoothRatio", ABS_KEY_BASIC_SKIN_SOFTEN,          true,   0},
        {"enlargeEyeRatio", ABS_KEY_SHAPE_EYE_ENLARGEMENT,      true,   0},
        {"slimFaceRatio",   ABS_KEY_SHAPE_FACE_SLENDER,         true,   0},
        {"noseRatio",       ABS_KEY_SHAPE_NOSE_AUGMENT_SHAPE,   true,   0},
        {"chinRatio",       ABS_KEY_SHAPE_CHIN_LENGTHEN,        true,   0},
        {"lipsRatio",       ABS_KEY_SHAPE_LIP_PLUMP,            true,   0},
        {"hairlineRatio",   ABS_KEY_SHAPE_HAIRLINE,             true,   0}
    };

    BeautyStyleInfo 　　beautyStyleInfo =
     {
　　　　　"beautyStyle",      0,
　　　　　"beautyStyleLevel", ABS_KEY_MAKEUP_LEVEL_NEW,0
     };

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

enum BeautyStyleType
{
  Beauty_Style_NONE=0,
  Beauty_Style_Skin_gloss,
  Beauty_Style_Nude,
  Beauty_Style_Pink,
  Beauty_Style_blue,
  Beauty_Style_Neutral,
  Beauty_Style_Masculine,
 // Beauty_Style_Neutral_m3,
 //Beauty_Style_Soft,
  Beauty_Style_max,
};

enum AppModule
{
    APP_MODULE_DEFAULT_CAPTURE=0,
    APP_MODULE_RECORD_VIDEO,
    APP_MODULE_FRONT_PORTRAIT,
    APP_MODULE_REAR_PORTRAIT,
    APP_MODULE_SUPER_NIGHT,
    APP_MODULE_SHORT_VIDEO,
    APP_MODULE_DUAL_VIDEO,
    APP_MODULE_NUM,
};

#endif // SKINBEAUTIFIERDEF_H