#ifndef _ARCSOFTHDR_H_
#define _ARCSOFTHDR_H_

#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <cutils/properties.h>
#include <math.h>
#include <stdio.h>
#include <utils/Log.h>

#include "arcsoft_high_dynamic_range.h"
#include "arcsoft_low_light_hdr.h"
#include "arcsoft_zoom_high_dynamic_range.h"

using namespace mialgo2;

#define MAX_CAMERA_ID 4
#define MAX_PARAM_NUM 5
#define MAX_BUF_LEN   1024

#define HARDCODE_CAMERAID_REAR          0
#define HARDCODE_CAMERAID_FRONT         0x100
#define HARDCODE_MIN_ADRCGAIN_VALUE     4.0f
#define HARDCODE_MAX_ADRCGAIN_VALUE     8.0f
#define HARDCODE_HDR_CHECKER_CONF       0.7f
#define HARDCODE_HDR_CHECKER_THRESH_MAX 0.8f
#define HARDCODE_HDR_CHECKER_THRESH_MIN 0.6f
#define MAX_EV_NUM                      8

enum { MODE_HDR = 0, MODE_LLHDR, MODE_ZOOMHDR };

enum {
    MI_REAR_CAMERA = 0,
    MI_FRONT_CAMERA,
    MI_UW_CAMERA,
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
    int camera_mode;
    int input_num;
    int ev[8];
    int orientation;
    int scene_type;
    int isHdrSr;
    MRECT cropRegion;
    unsigned int frameworkCameraID;
    ARC_HDR_FACEINFO face_info;
} arc_hdr_input_meta_t;

typedef struct
{
    int is_do_crop;
    MRECT crop_rect;
} arc_hdr_output_meta_t;

typedef struct
{
    // int isBackLight;
    // int backLight_degree;
    float conval;
    int capture_num;
    int ev_value[MAX_EV_NUM];
    int scene_flag;
    int motion_state;
    int scene_type;
} arc_hdr_checker_result_t;

enum {
    HDR_PARAM_INDEX_REAR,
    HDR_PARAM_INDEX_FRONT,
};

typedef struct
{
    int tonelen;
    int brightness;
    int saturation;
    int contrast;
    int ev[MAX_EV_NUM];
} ArcHDRParam;

static ArcHDRParam ArcHDRParams[] = {
    //(tonelen, bright, saturation, contrast, r_ev0, r_ev-, r_ev+)
    // REAR camera
    {30, 0, 0, 0, {0, -10, 6}},
    // FRONT camera
    {1, 0, 0, 0, {0, -12, 0}},
};

class ArcsoftHdr
{
public:
    ArcsoftHdr();
    virtual ~ArcsoftHdr();
    void init();
    void Process(struct MiImageBuffer *input, const arc_hdr_input_meta_t *input_meta,
                 struct MiImageBuffer *output);

private:
    int process_hdr(struct MiImageBuffer *input, const arc_hdr_input_meta_t *pMeta,
                    struct MiImageBuffer *output);
    int process_llhdr(struct MiImageBuffer *input, const arc_hdr_input_meta_t *pMeta,
                      struct MiImageBuffer *output);
    int process_zoomhdr(struct MiImageBuffer *input, const arc_hdr_input_meta_t *pMeta,
                        struct MiImageBuffer *output);
};

#endif
