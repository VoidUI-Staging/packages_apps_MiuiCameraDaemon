#ifndef BEAUTYPARAMS_H
#define BEAUTYPARAMS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/Log.h>

#include "BeautyShot_Image_Algorithm.h"
#include "BeautyShot_Video_Algorithm.h"

#define XIAOMI_FB_FEATURE_LEVEL_1 0
// preview: soften max=70 deblemish=0
// capture: soften max=70 deblemish=1

#define XIAOMI_FB_FEATURE_LEVEL_2 1
// preview: soften max=70 deblemish=0 faceslender=?  eyelarge=?
// capture: soften max=70 deblemish=1 faceslender=?  eyelarge=?

#define XIAOMI_FB_FEATURE_LEVEL_3 2

#define XIAOMI_FB_FEATURE_LEVEL XIAOMI_FB_FEATURE_LEVEL_2

struct BeautyFeatureInfo
{
    const char *TagName;
    int featureKey;
    bool featureMask;
    int featureValue;
};

struct BeautyFeatureParams
{
    const int featureCounts = sizeof(beautyFeatureInfo) == 0
                                  ? 0
                                  : sizeof(beautyFeatureInfo) / sizeof(beautyFeatureInfo[0]);
#if defined(DAUMIER_CAMERA)
    BeautyFeatureInfo beautyFeatureInfo[8] =
#else
    BeautyFeatureInfo beautyFeatureInfo[7] =
#endif
        {{"xiaomi.beauty.skinSmoothRatio", ABS_KEY_BASIC_SKIN_SOFTEN, true, 0},
         {"xiaomi.beauty.enlargeEyeRatio", ABS_KEY_SHAPE_EYE_ENLARGEMENT, true, 0},
         {"xiaomi.beauty.slimFaceRatio", ABS_KEY_SHAPE_FACE_SLENDER, true, 0},
         {"xiaomi.beauty.noseRatio", ABS_KEY_SHAPE_NOSE_AUGMENT_SHAPE, true, 0},
         {"xiaomi.beauty.chinRatio", ABS_KEY_SHAPE_CHIN_LENGTHEN, true, 0},
         {"xiaomi.beauty.lipsRatio", ABS_KEY_SHAPE_LIP_PLUMP, true, 0},
#if defined(DAUMIER_CAMERA)
         {"xiaomi.beauty.hairlineRatio", ABS_KEY_SHAPE_HAIRLINE, true, 0},
         {"xiaomi.beauty.eyeBrowDyeRatio", ABS_KEY_MAKEUP_LEVEL_NEW, true, 0}
#else
         {"xiaomi.beauty.hairlineRatio", ABS_KEY_SHAPE_HAIRLINE, true, 0}
#endif
    };
};

enum BeautyMode {
    BEAUTY_MODE_CAPTURE = 0,
    BEAUTY_MODE_VIDEO_CAPTURE,
    BEAUTY_MODE_FRONT_BOKEH_CAPTURE,
    BEAUTY_MODE_REAR_BOKEH_CAPTURE,
    BEAUTY_MODE_PREVIEW,
    BEAUTY_MODE_RECORD,
    BEAUTY_MODE_NUM,
};

/////////////////////////// for ui slider configuration start////////////////
/*enum e_advance_beauty_features{
    ADVANCE_SOFTEN,
    ADVANCE_EYE_ENLARGE,
    ADVANCE_FACE_SLENDER,
    ADVANCE_FACE_DEBLEMISH,
    ADVANCE_BEAUTY_FEATURE_NUM
};

const static char *ADVANCE_BEAUTY_FEATURE_TAGS[ADVANCE_BEAUTY_FEATURE_NUM] = {
    "skinSmoothRatio",
    "enlargeEyeRatio",
    "slimFaceRatio",
    "???"
};

const static int ADVANCE_BEAUTY_FEATURE_TAGS_MASK[ADVANCE_BEAUTY_FEATURE_NUM] = {
    1,//"skinSmoothRatio",//
    1,//"enlargeEyeRatio",//
    1,//"slimFaceRatio",//
    0
};*/

/////////////////////////// for ui slider configuration end ////////////////

/*#define FEATURE_INVALID_KEY 0
const static char *FeatureKeyArrayNote[]={
    "FACE_SOFTEN",
    "EYE_ENLARGEMENT",
    "FACE_SLENDER",
    "DEBLEMISH"
};

const static int FeatureKeyArray[]={
    ABS_KEY_BASIC_SKIN_SOFTEN,
    ABS_KEY_SHAPE_EYE_ENLARGEMENT,
    ABS_KEY_SHAPE_FACE_SLENDER,
    ABS_KEY_BASIC_DE_BLEMISH
};

const static int FeatureLimitArray[]={
    SOFTEN_LIMIT,
    EYELARGE_LIMIT,
    FACESLENDER_LIMIT,
    DEBLEMISH_LIMIT
};*/

#endif // BEAUTYPARAMS_H
