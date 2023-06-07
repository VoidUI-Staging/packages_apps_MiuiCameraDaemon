#ifndef _MIAIHDR_H_
#define _MIAIHDR_H_

#include "MiaPluginUtils.h"
#include "mihdr.h"
#include "xmi_high_dynamic_range.h"
#include "xmi_low_light_hdr.h"
// clang-format off
#include "chivendortag.h"
#include "chioemvendortag.h"
// clang-format on

namespace mialgo2 {

#define MAX_CAMERA_ID 4
#define MAX_PARAM_NUM 5

#define MAX_EV_NUM                 12
#define MAX_CONFIG_ITEM_LINE_CNT   3
#define MI_UI9_HDR_PARAMS_FILEPATH "/vendor/etc/camera/hdr_ui9_params.config"
#define ARC_USE_ALL_ION_BUFFER
#define FRAME_MAX_INPUT_NUMBER 8

enum { MODE_HDR = 0, MODE_LLHDR, MODE_SUPER_LOW_LIGHT_HDR, MODE_SRHDR };
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
    MIAI_HDR_ALGO_PARAMS algo_params;
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

typedef struct
{
    float luxindex;
    float real_drc_gain;
} MiExtraAECExposureData;

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

class MiAIHDR
{
public:
    MiAIHDR();
    virtual ~MiAIHDR();
    void init();
    void destroy();
    void process(struct MiImageBuffer *input, struct MiImageBuffer *output, int input_num, int *ev,
                 ARC_LLHDR_AEINFO &mAeInfo, int camMode, HDRMetaData hdrMetaData,
                 MiAECExposureData *aecExposureData, MiAWBGainParams *awbGainParams);
    ARC_HDR_USERDATA getUserData();

    void load_config(ArcHDRCommonAEParam *pArcAEParams, ArcHDRParam *pArcHDRParam);
    void setCrop(bool crop) { m_isCrop = crop; };
    void setFlushStatus(bool value) { m_flustStatus = value; };

    MiAECExposureData aecExposureData[FRAME_MAX_INPUT_NUMBER];
    MiAWBGainParams awbGainParams[FRAME_MAX_INPUT_NUMBER];
    MiExtraAECExposureData extraAecExposureData[FRAME_MAX_INPUT_NUMBER];
    std::string m_hdrAlgoExif;

private:
    void process_hdr_xmi(struct MiImageBuffer *input, struct MiImageBuffer *output, int input_num,
                         ARC_LLHDR_AEINFO &mAeInfo, int camMode, HDRMetaData hdrMetaData,
                         HDR_EVLIST_AEC_INFO &aeInfo);
    void process_low_light_hdr_xmi(struct MiImageBuffer *input, struct MiImageBuffer *output,
                                   int input_num, ARC_LLHDR_AEINFO &mAeInfo, int camMode,
                                   HDRMetaData hdrMetaData);
    void process_hdr_sr_xmi(struct MiImageBuffer *input, struct MiImageBuffer *output,
                            int input_num, ARC_LLHDR_AEINFO &mAeInfo, int camMode,
                            HDRMetaData hdrMetaData);
    static MRESULT hdrProcessCb(MLong lProgress, // Not impletement
                                MLong lStatus,   // Not impletement
                                MVoid *pParam);

#ifndef ARC_USE_ALL_ION_BUFFER
    void mibuffer_alloc_copy(struct MiImageBuffer *dstBuffer, struct MiImageBuffer *srcBuffer);
    void mibuffer_release(struct MiImageBuffer *mibuf);
#endif
    double nowMSec(void);
    void WriteFileLog(char *file, const char *fmt, ...);
    void setOffScreen(ASVLOFFSCREEN *pImg, struct MiImageBuffer *miBuf);
    void setOffScreenHDR(HDR_YUV_OFFSCREEN *pImg, struct MiImageBuffer *miBuf);

private:
    ARC_HDR_USERDATA m_userData;
    bool m_isCrop;
    bool m_flustStatus;
};

} // namespace mialgo2
#endif
