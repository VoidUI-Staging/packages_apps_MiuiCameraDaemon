#ifndef _ARCSOFTHDR_H_
#define _ARCSOFTHDR_H_

#include <cutils/properties.h>
#include <stdio.h>
#include <utils/Log.h>

#include "MiaPluginUtils.h"
#include "amcomdef.h"
#include "ammem.h"
#include "arcsoft_high_dynamic_range.h"
#include "arcsoft_low_light_hdr.h"
#include "asvloffscreen.h"
#include "merror.h"

#define MAX_CAMERA_ID 4
#define MAX_PARAM_NUM 5

#define MAX_EV_NUM                 8
#define MAX_CONFIG_ITEM_LINE_CNT   3
#define MI_UI9_HDR_PARAMS_FILEPATH "/vendor/etc/camera/hdr_ui9_params.config"
#define ARC_USE_ALL_ION_BUFFER

using namespace mialgo2;

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

typedef struct _tag_MI_HDR_FACERCINFO
{
    MInt32 nFace;
    MRECT rcFace[8];
    MInt32 lFaceOrient[8];
} MI_HDR_FACERCINFO, *LPMI_HDR_FACERCINFO;

typedef struct _tag_MI_HDR_INFO
{
    ARC_LLHDR_AEINFO m_AeInfo;
    MI_HDR_FACERCINFO m_FaceInfo;
} MI_HDR_INFO, *LPMI_HDR_INFO;

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

class ArcsoftHdr
{
public:
    ArcsoftHdr();
    virtual ~ArcsoftHdr();
    void init();

    void process(struct MiImageBuffer *input, struct MiImageBuffer *output, int input_num, int *ev,
                 MI_HDR_INFO &mHdrInfo, int camMode);
    void load_config(ArcHDRCommonAEParam *pArcAEParams, ArcHDRParam *pArcHDRParam);
    void setCrop(bool crop) { m_isCrop = crop; };

private:
    int process_hdr(struct MiImageBuffer *input, struct MiImageBuffer *output, int input_num,
                    MI_HDR_INFO &mHdrInfo, int camMode, char *metadata);
    int process_low_light_hdr(struct MiImageBuffer *input, struct MiImageBuffer *output,
                              int input_num, MI_HDR_INFO &mHdrInfo, int camMode, char *metadata);
#ifndef ARC_USE_ALL_ION_BUFFER
    void mibuffer_alloc_copy(struct MiImageBuffer *dstBuffer, struct MiImageBuffer *srcBuffer);
    void mibuffer_release(struct MiImageBuffer *mibuf);
#endif
    double nowMSec(void);
    MVoid setOffScreen(ASVLOFFSCREEN *pImg, struct MiImageBuffer *miBuf);
    bool isLLHDR(int inputNum, MInt32 *pEv);
    // void DumpMetadata(char *file, LPARC_HDR_INPUTINFO pInputImages, LPARC_HDR_PARAM param,
    //                   LPMI_HDR_INFO pHdrInfo, MInt32 cameraType, MInt32 sceneType, int
    //                   inputMode);
    // void DumpLLHDRMetadata(char *file, ARC_LLHDR_INPUTINFO *pInputImages,
    //                        ARC_LLHDR_FACEINFO *pFaceInfo, ARC_LLHDR_PARAM param, MInt32 isoValue,
    //                        ARC_LLHDR_ImgOrient ImgOri, MInt32 i32EvStep, MInt32 cameraType);

private:
    bool m_isCrop;
    int m_iIsDumpData;
    int *pEV;
};

#endif
