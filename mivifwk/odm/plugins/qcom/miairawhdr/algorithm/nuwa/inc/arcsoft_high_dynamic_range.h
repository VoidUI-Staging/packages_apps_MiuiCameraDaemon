/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary
and confidential information.

The information and code contained in this file is only for authorized ArcSoft
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy,
distribute, modify, or take any action in reliance on it.

If you have received this file in error, please immediately notify ArcSoft and
permanently delete the original and any copy of any file and any printout
thereof.
*******************************************************************************/
#ifndef _ARCSOFT_HDR_H_
#define _ARCSOFT_HDR_H_

#include <string.h>

#include "amcomdef.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "merror.h"

#ifdef HDRDLL_EXPORTS
#define ARC_HDR_API __declspec(dllexport)
#else
#define ARC_HDR_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Define the Error codes
#define HDR_ERR_INVALID_LOW_EXPOSURED_IMAGE  (MERR_BASIC_BASE + 20)
#define HDR_ERR_INVALID_OVER_EXPOSURED_IMAGE (MERR_BASIC_BASE + 21)
#define HDR_ERR_ALIGN_FAIL                   (MERR_BASIC_BASE + 22)
#define HDR_ERR_INVALID_EXPOSURE             (MERR_BASIC_BASE + 23)
#define HDR_ERR_CONTAIN_GHOST                (MERR_BASIC_BASE + 24)
#define HDR_ERR_GHOST_IN_OVERREGION          (MERR_BASIC_BASE + 25)

// Define the mode name(i32Mode)
#define ARC_MODE_REAR_CAMERA  0
#define ARC_MODE_FRONT_CAMERA 0x100

#define ARC_TYPE_QUALITY_FIRST     0
#define ARC_TYPE_PERFORMANCE_FIRST 1

#define ARC_HDR_MAX_INPUT_IMAGE_NUM 6
#define HDR_ALGO_LENGTH             16
#define HDR_EV_MAX_LEN              12
#define HDR_RAW_MAX_LEN             10
#define HDR_RAW_LSC_ROW_NUM         13
#define HDR_RAW_LSC_COL_NUM         17
#define HDR_RAW_EXIF_LEN            512
#define HDR_RAW_BLACK_LEVEL_LEN     4
#define HDR_RAW_MAX_FACE_NUMBER     8
typedef struct _tag_MIAI_HDR_LSC_TABLE
{
    MFloat GR[HDR_RAW_LSC_ROW_NUM][HDR_RAW_LSC_COL_NUM];
    MFloat GB[HDR_RAW_LSC_ROW_NUM][HDR_RAW_LSC_COL_NUM];
    MFloat R[HDR_RAW_LSC_ROW_NUM][HDR_RAW_LSC_COL_NUM];
    MFloat B[HDR_RAW_LSC_ROW_NUM][HDR_RAW_LSC_COL_NUM];
} MIAI_HDR_LSC_TABLE;

typedef struct _tag_MIAI_HDR_FACEINFO
{
    MInt32 nFace;
    MRECT rcFace[HDR_RAW_MAX_FACE_NUMBER];
} MIAI_HDR_FACEINFO, *LPMIAI_HDR_FACEINFO;
typedef enum ColorFilterPattern {
    Y,    ///< Monochrome pixel pattern.
    YUYV, ///< YUYV pixel pattern.
    YVYU, ///< YVYU pixel pattern.
    UYVY, ///< UYVY pixel pattern.
    VYUY, ///< VYUY pixel pattern.
    RGGB, ///< RGGB pixel pattern.
    GRBG, ///< GRBG pixel pattern.
    GBRG, ///< GBRG pixel pattern.
    BGGR, ///< BGGR pixel pattern.
    RGB   ///< RGB pixel pattern.
} ColorFilterPattern;
typedef struct _tag_MIAI_HDR_RAWINFO
{
    ColorFilterPattern raw_type; // raw格式
    MInt32 device_orientation;   //相机朝向
    MInt32 nframes;              //输入帧数
    MInt32 width;                // raw图宽度
    MInt32 height;               // raw图高度
    MInt32 stride;               // raw图stride, 如果有的话
    MInt32 bits_per_pixel;       //单个像素的位数。例子:K1上idealraw是14, mipi-raw是10
    MFloat red_gain;             // awb参数
    MFloat blue_gain;            // awb参数
    MFloat luxindex;             // luxindex
    MFloat analog_gain[HDR_RAW_MAX_LEN];     //每帧的analog gain
    MFloat digital_gain[HDR_RAW_MAX_LEN];    //每帧的digital gain
    MFloat shutter[HDR_RAW_MAX_LEN];         //每帧的曝光时间
    MFloat CCT[HDR_RAW_MAX_LEN];             //每帧的色温
    MInt32 ev[HDR_RAW_MAX_LEN];              //每帧的ev值
    MIAI_HDR_LSC_TABLE lsc[HDR_RAW_MAX_LEN]; //每帧的lsc矩阵
    MFloat
        denoise_switch_gain; //传统降噪与ai降噪切换阈值，需要根据这个阈值调用两套不同的tuning mode
    MIAI_HDR_FACEINFO face_info;                                  // 人脸个数及坐坐标
    MFloat black_level[HDR_RAW_MAX_LEN][HDR_RAW_BLACK_LEVEL_LEN]; //每帧的黑电平
    MChar hdr_raw_exif_info[HDR_RAW_EXIF_LEN];
} MIAI_HDR_RAWINFO;
typedef struct _tag_ARC_HDR_INPUTINFO
{
    MInt32 i32ImgNum;
    ASVLOFFSCREEN InputImages[ARC_HDR_MAX_INPUT_IMAGE_NUM];
} ARC_HDR_INPUTINFO, *LPARC_HDR_INPUTINFO;

typedef struct _tag_MAGNET_INFO
{
    MFloat x;         ///< Angular velocity about x axis.
    MFloat y;         ///< Angular velocity about y axis.
    MFloat z;         ///< Angular velocity about z axis.
    MInt64 timestamp; ///< Qtimer Timestamp when the sample was captured.
} MAGNET_INFO;

typedef struct _tag_EXPOSURE_INFO
{
    MFloat fSensitivityShort;
    MFloat fSensitivitySafe;
    MFloat fSensitivityLong;
} EXPOSURE_INFO;

typedef struct _tag_HISTOGRAM_INFO
{
    MUInt32 histogram_enable;
    MUInt32 histogram_stats_type;
    MUInt32 histogram_buckets;
    MUInt32 histogram_max_count;
    MUInt32 *histogram_stats;
} HISTOGRAM_INFO;

typedef struct _tag_HDRBHIST_INFO
{
    MUInt32 histogram_enable;   // > 0 indicate enable, 0 indicate invalid
    MUInt32 histogram_Y_enable; // > 0 indicate enable, 0 indicate invalid
    MUInt32 red_buckets;        // number of bins of Red channel
    MUInt32 green_buckets;      // number of bins of Green channel
    MUInt32 blue_buckets;       // number of bins of Blue channel
    MUInt32 y_buckets;          // number of bins of Blue channel
    MUInt32 *histogram_stats_R;
    MUInt32 *histogram_stats_G;
    MUInt32 *histogram_stats_B;
    MUInt32 *histogram_stats_Y;
    MUInt32 histogram_Y_GTM_enable; // > 0 indicate enable, 0 indicate invalid
    MUInt32 y_gtm_buckets;          // number of bins of Blue channel
    MUInt32 *histogram_stats_Y_GTM;
} HDRBHIST_INFO;

typedef struct _MIAI_HDR_ALGO_PARAMS
{
    _MIAI_HDR_ALGO_PARAMS()
    {
        apture = 0.f;
        exposure = 0.f;
        iso = 0;
        exp_compensation = 0;
        memset(git_commit_id, 0, HDR_ALGO_LENGTH * sizeof(char));
        rotate = -1;
        width = -1;
        height = -1;
        memset(implement_branch, 0, HDR_ALGO_LENGTH * sizeof(char));
        memset(normalize_branch, 0, HDR_ALGO_LENGTH * sizeof(char));
        // deghost params
        is_discard_frame = false;
        ghost_val = -1.f;
        global_ghost_val = -1.f;
        still_image_val = -1.f;
        // merge params
        faceNum = -1;
        ev1x = -1.f;
        y_mul = -1.f;
        ev0_expo_area = -1.f;
        ev0_face_mean = -1.f;
        is_fusion_phase2 = false;
        is_use_PWL_1st = false;
        is_use_PWL_2nd = false;
        pwl_1st_line1_first = 0.f;
        pwl_1st_line1_second = 0.f;
        pwl_1st_line2_first = 0.f;
        pwl_1st_line2_second = 0.f;
        pwl_2nd_line1_first = 0.f;
        pwl_2nd_line1_second = 0.f;
        pwl_2nd_line2_first = 0.f;
        pwl_2nd_line2_second = 0.f;
        linear_mul = 0.f;
        linear_minus = 0.f;
        hdr_enable = 0;
        memset(hdr_ev, 0, HDR_EV_MAX_LEN * sizeof(int));
        frame_id = 0;
        memset(luxindex, 0, HDR_EV_MAX_LEN * sizeof(float));
        memset(real_drc_gain, 0, HDR_EV_MAX_LEN * sizeof(float));
        memset(exposureTime, 0, HDR_EV_MAX_LEN * sizeof(uint64_t));
        memset(linear_gain, 0, HDR_EV_MAX_LEN * sizeof(float));
        memset(sensitivity, 0, HDR_EV_MAX_LEN * sizeof(float));
    }
    _MIAI_HDR_ALGO_PARAMS &operator=(const _MIAI_HDR_ALGO_PARAMS &other)
    {
        apture = other.apture;
        exposure = other.exposure;
        iso = other.iso;
        exp_compensation = other.exp_compensation;
        memcpy(git_commit_id, other.git_commit_id, sizeof(other.git_commit_id));
        rotate = other.rotate;
        width = other.width;
        height = other.height;
        memcpy(implement_branch, other.implement_branch, sizeof(other.implement_branch));
        memcpy(normalize_branch, other.normalize_branch, sizeof(other.normalize_branch));
        // deghost params
        is_discard_frame = other.is_discard_frame;
        ghost_val = other.ghost_val;
        global_ghost_val = other.global_ghost_val;
        still_image_val = other.still_image_val;
        // merge params
        faceNum = other.faceNum;
        ev1x = other.ev1x;
        y_mul = other.y_mul;
        ev0_expo_area = other.ev0_expo_area;
        ev0_face_mean = other.ev0_face_mean;
        is_fusion_phase2 = other.is_fusion_phase2;
        is_use_PWL_1st = other.is_use_PWL_1st;
        is_use_PWL_2nd = other.is_use_PWL_2nd;
        pwl_1st_line1_first = other.pwl_1st_line1_first;
        pwl_1st_line1_second = other.pwl_1st_line1_second;
        pwl_1st_line2_first = other.pwl_1st_line2_first;
        pwl_1st_line2_second = other.pwl_1st_line2_second;
        pwl_2nd_line1_first = other.pwl_2nd_line1_first;
        pwl_2nd_line1_second = other.pwl_2nd_line1_second;
        pwl_2nd_line2_first = other.pwl_2nd_line2_first;
        pwl_2nd_line2_second = other.pwl_2nd_line2_second;
        linear_mul = other.linear_mul;
        linear_minus = other.linear_minus;
        hdr_enable = other.hdr_enable;
        frame_id = other.frame_id;
        memcpy(luxindex, other.luxindex, HDR_EV_MAX_LEN * sizeof(float));
        memcpy(real_drc_gain, other.real_drc_gain, HDR_EV_MAX_LEN * sizeof(float));
        memcpy(hdr_ev, other.hdr_ev, HDR_EV_MAX_LEN * sizeof(int));
        memcpy(exposureTime, other.exposureTime, HDR_EV_MAX_LEN * sizeof(uint64_t));
        memcpy(linear_gain, other.linear_gain, HDR_EV_MAX_LEN * sizeof(float));
        memcpy(sensitivity, other.sensitivity, HDR_EV_MAX_LEN * sizeof(float));
        return *this;
    }
    float apture;
    float exposure;
    int iso;
    int exp_compensation;
    char git_commit_id[HDR_ALGO_LENGTH];
    int rotate;
    int width;
    int height;
    char implement_branch[HDR_ALGO_LENGTH];
    char normalize_branch[HDR_ALGO_LENGTH];
    // deghost params
    bool is_discard_frame;
    float ghost_val;
    float global_ghost_val;
    float still_image_val;
    // merge params
    int faceNum;               //人脸数量
    float ev1x;                // vs真实ev值
    float y_mul;               // ev6提亮系数
    float ev0_expo_area;       // ev0过曝区域
    float ev0_face_mean;       // ev0人脸均值
    bool is_fusion_phase2;     //是否二次融合
    bool is_use_PWL_1st;       //首次是否使用直线提亮
    bool is_use_PWL_2nd;       //二次融合是否使用提亮
    float pwl_1st_line1_first; //调亮直线1斜率截距
    float pwl_1st_line1_second;
    float pwl_1st_line2_first; //调亮直线2斜率截距
    float pwl_1st_line2_second;
    float pwl_2nd_line1_first;
    float pwl_2nd_line1_second;
    float pwl_2nd_line2_first;
    float pwl_2nd_line2_second;
    float linear_mul;
    float linear_minus; //发蒙调整斜率截距
    int hdr_enable;
    int hdr_ev[HDR_EV_MAX_LEN];
    int64_t frame_id;
    float luxindex[HDR_EV_MAX_LEN];
    float real_drc_gain[HDR_EV_MAX_LEN];
    uint64_t exposureTime[HDR_EV_MAX_LEN];
    float linear_gain[HDR_EV_MAX_LEN];
    float sensitivity[HDR_EV_MAX_LEN];
} MIAI_HDR_ALGO_PARAMS;

typedef struct _tag_ARC_HDR_AEINFO
{
    MFloat fLuxIndex;
    MFloat fISPGain;
    MFloat fSensorGain;
    MFloat fADRCGain;
    MFloat fADRCGainMax;
    MFloat fADRCGainMin;
    MFloat fADRCGainClamp;
    MFloat fDarkGainClamp;
    MFloat fDarkBoostGain;
    MFloat calculatedCDF0;
    MFloat calculatedCDF5;
    int ae_state;
    HISTOGRAM_INFO histogram_info;
    MAGNET_INFO magnet_info;
    EXPOSURE_INFO exposure_info;
    HDRBHIST_INFO histogram_HDRBhist;
    MIAI_HDR_ALGO_PARAMS algo_params;
    MIAI_HDR_RAWINFO miai_hdr_rawinfo;
} ARC_HDR_AEINFO, *LPARC_HDR_AEINFO;

typedef struct _tag_ARC_HDR_Userdata
{
    MRECT rtCropRect;       // [out] The effective rectangle of hdr result
    MInt32 i32DoCropResize; // [in]  Do crop-reisze or not, 0: do not crop-resize result image, 1:
                            // do crop-resize result image
} ARC_HDR_USERDATA, *LPARC_HDR_USERDATA;

/************************************************************************
 * This function is implemented by the caller, registered with
 * any time-consuming processing functions, and will be called
 * periodically during processing so the caller application can
 * obtain the operation status (i.e., to draw a progress bar),
 * as well as determine whether the operation should be canceled or not
 ************************************************************************/
typedef MRESULT (*ARC_HDR_FNPROGRESS)(MInt32 i32Progress, // The percentage of the current operation
                                      MInt32 i32Status,   // The current status at the moment
                                      MVoid *pParam       // Caller-defined data
);

/************************************************************************
 * This function is used to get version information of library
 ************************************************************************/
ARC_HDR_API const MPBASE_Version *ARC_HDR_GetVersion();

/************************************************************************
 * This struct is used to descripe HDR parameters
 ************************************************************************/
typedef struct _tag_ARC_HDR_Param
{
    MInt32 i32ToneLength;    // [in]  Adjustable parameter for tone mapping, the range is 0~100.
    MInt32 i32Brightness;    // [in]  Adjustable parameter for brightness, the range is -100~+100.
    MInt32 i32Saturation;    // [in]  Adjustable parameter for saturation, the range is 0~+100.
    MInt32 i32CurveContrast; // [in]  Adjustable parameter for contrast, the range is 0~+100.
} ARC_HDR_PARAM, *LPARC_HDR_PARAM;

/************************************************************************
 * The function is used to init HDR engine
 * i32Mode: ARC_MODE_REAR_CAMERA for rear camera, ARC_MODE_FRONT_CAMERA
 * for front camera
 ************************************************************************/
ARC_HDR_API MRESULT ARC_HDR_Init( // return MOK if success, otherwise fail
    MHandle *phHandle,            // [out] The algorithm engine will be initialized by this API
    MInt32 i32Mode,               // [in] The model name for camera
    MInt32 i32Type // [in] The HDR type, ARC_TYPE_QUALITY_FIRST(0), ARC_TYPE_PERFORMANCE_FIRST(1)
);

/************************************************************************
 * The function is used to uninit HDR engine
 ************************************************************************/
ARC_HDR_API MRESULT ARC_HDR_Uninit( // return MOK if success, otherwise fail
    MHandle *phHandle // [in/out] The algorithm engine will be un-initialized by this API
);

/************************************************************************
 * The function is used to pre-process input image
 ************************************************************************/
ARC_HDR_API MRESULT ARC_HDR_PreProcess( // return MOK if success, otherwise fail
    MHandle hHandle,                    // [in]  The algorithm engine
    LPARC_HDR_INPUTINFO pInputImages,   // [in]  The input images array
    LPARC_HDR_AEINFO pAEInfo,           // [in]  The AE info of normal input image
    MInt32 i32InputImageIndex // [in]  The index of input image in the input image array, the last
                              // image must be high-exposure
);

/************************************************************************
 * The function is used to process input image
 ************************************************************************/
ARC_HDR_API MRESULT ARC_HDR_Process( // return MOK if success, otherwise fail
    MHandle hHandle,                 // [in]  The algorithm engine
    LPARC_HDR_PARAM pParam,          // [in]  The parameters for algorithm engine
    LPASVLOFFSCREEN pDstImg          // [out]  The offscreen of result image
);

/************************************************************************
 * The function is used to detect how many frames to shot, and Ev for each frame.
 ************************************************************************/
ARC_HDR_API MRESULT ARC_HDR_ParameterDetect(
    LPASVLOFFSCREEN pPreviewImg, // [in] The input Preview data same ev as normal
    LPARC_HDR_AEINFO pAEInfo,    // [in] The AE info of input image
    MInt32 i32Mode, // [in] The model name for camera, 0 for rear camera, 0x100 for front camera
    MInt32 i32Type, // [in] The HDR type, ARC_TYPE_QUALITY_FIRST(0), ARC_TYPE_PERFORMANCE_FIRST(1)
    MInt32 i32Orientation, // [in] This parameter is the clockwise rotate orientation of the input
                           // image. The accepted value is {0, 90, 180, 270}.
    MInt32 i32MinEV,       // [in] The min ev value, it is < 0
    MInt32 i32MaxEV,       // [in] The max ev value, it is > 0
    MInt32 i32EVStep,      // [in] The step ev value, it is >= 1
    MInt32 *pi32ShotNum,   // [out] The number of shot image, it must be 1, 2 or 3.
    MInt32 *pi32EVArray,   // [out] The ev value for each image
    MFloat *fConVal // [out] The confidence array of the back light, fConVal[0] for HDR, fConVal[1]
                    // for LLHDR.
);

/************************************************************************
 * The function is used to set call-back
 ************************************************************************/
ARC_HDR_API MVoid ARC_HDR_SetCallback(MHandle hHandle,               // [in]  The algorithm engine
                                      ARC_HDR_FNPROGRESS fnCallback, // [in]  The callback function
                                      MVoid *pUserData               // [in]  Caller-defined data
);

ARC_HDR_API MRESULT ARC_HDR_GetDefaultParam(LPARC_HDR_PARAM pParam);

#ifdef __cplusplus
}
#endif

#endif // _ARCSOFT_HDR_H_
