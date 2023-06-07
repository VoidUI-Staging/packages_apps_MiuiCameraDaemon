
#ifndef _ARCSOFTHDR_H_
#define _ARCSOFTHDR_H_

#include "arcsoft_high_dynamic_range.h"
//#include <unistd.h>
#define MAX_CAMERA_ID       4
#define MAX_PARAM_NUM       5
#define MAX_INPUT_NUM       12
#define FORMAT_YUV_420_NV21 0x01
#define FORMAT_YUV_420_NV12 0x02

#define MAX_EV_NUM                 20
#define MAX_CONFIG_ITEM_LINE_CNT   3
#define MI_UI9_HDR_PARAMS_FILEPATH "/vendor/etc/camera/hdr_ui9_params.config"
#define ARC_USE_ALL_ION_BUFFER

enum {
    MODE_HDR = 0,
    MODE_LLHDR,
    MODE_SUPER_LOW_LIGHT_HDR,
};
typedef struct _tag_ARC_LLHDR_AEINFO
{
    MFloat fLuxIndex;
    MFloat fISPGain;
    MFloat fSensorGain;
    MFloat fADRCGain;
    MFloat fADRCGainMax;
    MFloat fADRCGainMin;
    MFloat apture;
    MFloat exposure;
    int iso;
    int exp_compensation;
} ARC_LLHDR_AEINFO, *LPARC_LLHDR_AEINFO;

typedef struct
{
    // int isBackLight;
    // int backLight_degree;
    float conval;
    int capture_num;
    int ev_value[MAX_EV_NUM];
} arcsoft_hdr_checker;
typedef struct
{
    float lux_min;
    float lux_max;
    int checkermode;
    float confval_min;
    float confval_max;
    float adrc_min;
    float adrc_max;
} ArcHDRCommonAEParam;

typedef struct
{
    int tonelen;
    int brightness;
    int saturation;
    int contrast;
    int ev[MAX_EV_NUM];
} ArcHDRParam;
enum {
    EV_AUTO = 0,
    EV_MANUAL,
};
enum {
    HDR_PARAM_INDEX_REAR,
    HDR_PARAM_INDEX_FRONT,
};
static ArcHDRCommonAEParam ArcHDRCommonAEParams[] = {
    ///(lux_min, lux_max, checkermode, c_min, c_max, drc_min, drc_max)
    // CommonAEParam
    {250, 300, 1, 0.6, 0.8, 4.0, 8.0},
};
static ArcHDRParam ArcHDRParams[] = {
    //(tonelen, bright, saturation, contrast, r_ev0, r_ev-, r_ev+)
    // REAR camera
    {20, 0, 5, 50, {0, -10, 6}},
    // FRONT camera
    {1, 0, 0, 0, {0, -12, 0}},
};

#endif
