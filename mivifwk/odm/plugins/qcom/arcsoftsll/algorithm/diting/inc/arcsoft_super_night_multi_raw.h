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
#ifndef _ARCSOFT_SUPER_NIGHT_MULTI_RAW_H_
#define _ARCSOFT_SUPER_NIGHT_MULTI_RAW_H_

#include "amcomdef.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "merror.h"

#ifdef SNDLL_EXPORTS
#define ARC_SN_API __declspec(dllexport)
#else
#define ARC_SN_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ARC_SN_MAX_INPUT_IMAGE_NUM 20

// camera mode
#define CAMERA_MODE_XIAOMI_L2_SDM8450_IMX707_Y \
    0x762 // 1890  	  L2_SDM8450(IMX707) OIS_Y Pure_RAW_14_16//W
#define CAMERA_MODE_XIAOMI_L2_SDM8450_IJN1_N_UW \
    0x763 // 1891  	  L2_SDM8450(JN1) OIS_N Pure_RAW_14_16//UW
#define CAMERA_MODE_XIAOMI_L2_SDM8450_IJN1_N_TELE \
    0x764 // 1892  	  L2_SDM8450(JN1) OIS_N Pure_RAW_14_16//Tele

#define CAMERA_MODE_XIAOMI_L3_SDM8450_IMX766_Y \
    0x765 // 1893  	  L3_SDM8450(IMX766) OIS_Y Pure_RAW_14_16//W
#define CAMERA_MODE_XIAOMI_L3_SDM8450_OV13B10_N \
    0x766 // 1894  	  L3_SDM8450(OV13B10) OIS_N Pure_RAW_14_16//UW

#define CAMERA_MODE_XIAOMI_L18_SDM8450_IMX766_Y \
    0x768 // 1896  	  L18_SDM8450(MX766) OIS_Y Pure_RAW_14_16//W
#define CAMERA_MODE_XIAOMI_L18_SDM8450_OV13B10_N \
    0x769 // 1897  	  L18_SDM8450(OV13B10) OIS_N Pure_RAW_14_16//UW

#define CAMERA_MODE_XIAOMI_L10_SDM8450_IMX686 \
    0x785 // 1925 	  L10_SDM8450(IMX686) OIS_N Pure_RAW_14_16//W
#define CAMERA_MODE_XIAOMI_L10_SDM8450_OV8856 \
    0x786 // 1926 	  L10_SDM8450(OV8856) OIS_N Pure_RAW_14_16//UW

#define CAMERA_MODE_XIAOMI_L12_INT_PRO_SDM8475_S5KHP1 \
    0x7C1 //1985  	  L12_INT_PRO_SDM8475(S5KHP1 OIS_Y)//W
#define CAMERA_MODE_XIAOMI_L12_INT_PRO_SDM8475_S5K4H7 \
    0x7C2 //1986  	  L12_INT_PRO_SDM8475(S5K4H7)//UW

#define CAMERA_MODE_XIAOMI_L12_INT_SDM8475_HM6 \
    0x7C5 //1989  	  L12_INT_SDM8475(HM6 OIS_Y)//W
#define CAMERA_MODE_XIAOMI_L12_INT_SDM8475_S5K4H7 \
    0x7C6 //1990  	  L12_INT_SDM8475(S5K4H7)//UW

// camera state
#define ARC_SN_CAMERA_STATE_UNKNOWN  0
#define ARC_SN_CAMERA_STATE_TRIPOD   1
#define ARC_SN_CAMERA_STATE_HAND     2
#define ARC_SN_CAMERA_STATE_AUTOMODE 3

#define MERR_SN_BAD_SCENE_NOT_DO_HDR 14

typedef struct _tag_ARC_SN_INPUTINFO
{
    MInt32
        i32CameraState; // ARC_SN_CAMERA_STATE_UNKNOW,ARC_SN_CAMERA_STATE_TRIPOD,ARC_SN_CAMERA_STATE_HAND
    MInt32 i32CurIndex;
    MInt32 i32ImgNum;
    ASVLOFFSCREEN InputImages[ARC_SN_MAX_INPUT_IMAGE_NUM];
    MFloat InputImagesEV[ARC_SN_MAX_INPUT_IMAGE_NUM];
    MInt32 i32InputFd[ARC_SN_MAX_INPUT_IMAGE_NUM];
} ARC_SN_INPUTINFO, *LPARC_SN_INPUTINFO;

/************************************************************************
 * This function is implemented by the caller, registered with
 * any time-consuming processing functions, and will be called
 * periodically during processing so the caller application can
 * obtain the operation status (i.e., to draw a progress bar),
 * as well as determine whether the operation should be canceled or not
 ************************************************************************/
typedef MRESULT (*ARC_SN_FNPROGRESS)(MLong lProgress, // The percentage of the current operation
                                     MLong lStatus,   // The current status at the moment
                                     MVoid *pParam    // Caller-defined data
);

/************************************************************************
must same as __android_log_print()
************************************************************************/
typedef int (*FN_ARC_SN_android_log_print)(int prio, const char *tag, const char *fmt, ...);

typedef struct _tag_ARC_SN_Userdata
{
    MRECT lCropRect;   // [out] the crop rect
    MVoid *lreserved1; // [in]  reserved data 1
    MVoid *lreserved2; // [in]  reserved data 2
} ARC_SN_Userdata, *LPARC_SN_Userdata;

typedef struct _tag_ARC_SN_FACEINFO
{
    MInt32 i32FaceNum;
    MRECT *pFaceRects;
    MInt32 i32FaceOrientation;
} ARC_SN_FACEINFO, *LPARC_SN_FACEINFO;

typedef enum {
    ARC_SN_SCENEMODE_UNKNOW = 0,
    ARC_SN_SCENEMODE_INDOOR,
    ARC_SN_SCENEMODE_OUTDOOR,
    ARC_SN_SCENEMODE_LOWLIGHT,
    ARC_SN_SCENEMODE_PORTRAIT,
    ARC_SN_SCENEMODE_NIGHTCLUB,
    ARC_SN_SCENEMODE_TRIPOD,
    ARC_SN_SCENEMODE_SE_BOKEH
} ARC_SN_SCENE_MODE;

typedef struct _tag_ARC_SN_RawInfo
{
    MInt32 i32RawType;                        // [in]  The type of raw data
    MInt32 i32EV[ARC_SN_MAX_INPUT_IMAGE_NUM]; // [in]  The EV for image
    MInt32 i32BrightLevel[4];          // [in]  The most bright level for image. Bit width: 12 bit.
    MInt32 i32BlackLevel[4];           // [in]  The most black level for image. Bit width: 12 bit.
    MFloat fWbGain[4];                 // [in]  The white balance gains for image
    MFloat fShutter;                   // [in]  Shutter speed
    MFloat fSensorGain;                // [in]  Sensor gain
    MFloat fISPGain;                   // [in]  ISP gain
    MFloat fTotalGain;                 // [in]  Total gain
    MInt32 i32LuxIndex;                // [in]  Lux index
    MInt32 i32ExpIndex;                // [in]  Exposure index
    MFloat fAdrcGain;                  // [in]  ADRC gain
    MBool bEnableLSC;                  // [in]  enable or disable LSC
    MInt32 i32LSChannelLength[4];      // [in]  The length of LensShading each channel
    MFloat fLensShadingTable[4][1024]; // [in]  The LensShading table
    MFloat fCCM[9];                    // [in]  The CCM
    MFloat fCCT;                       // [in]  The Correlated Color temperature
} ARC_SN_RAWINFO, *LPARC_SN_RAWINFO;

/************************************************************************
 * This function is used to get version information of library
 ************************************************************************/
ARC_SN_API const MPBASE_Version *ARC_SN_GetVersion();

/************************************************************************
 * This function is used to set log level and callback
 ************************************************************************/
// Notes: Use ARC_SN_SetLogLevel() to set fn_log_print is not thread-safe because performance.
//       So please not call it frequently, and suggest to call it before SDK created.
//       fn_log_print will be set to global value, so please keep the life longer than SDK's.
//       You can set NULL to clean it.
ARC_SN_API MRESULT
ARC_SN_SetLogLevel(MInt32 i32LogLevel, // 0 is disable log and 1 is enable all log
                   FN_ARC_SN_android_log_print fn_log_print = MNull);

/************************************************************************
 * The function is used to init engine
 ************************************************************************/
#define ARC_SN_VendorTag_bypass "persist.vendor.camera.arcsoft.sn.bypass"
#define ARC_SN_VendorTag_dump \
    "persist.vendor.camera.arcsoft.sn.dump"       // A:"/data/vendor/camera/supernight/"
                                                  // B:"/sdcard/DCIM/Camera/"
ARC_SN_API MRESULT ARC_SN_Init(MHandle *phHandle, // The handle for ArcSoft Super Night
                               MInt32 i32CameraMode, MInt32 i32CameraState,
                               MInt32 i32Width,          // The input image width (in pixel)
                               MInt32 i32Height,         // The input image height (in pixel)
                               MRECT *pZoomRect = MNull, // [in] The Zoom Rect
                               MInt32 i32LuxIndex = 0    // The LuxIndex of 0EV frame
);

/************************************************************************
 * The function is used to uninit  engine
 ************************************************************************/
ARC_SN_API MRESULT ARC_SN_Uninit(MHandle *phHandle // The handle for ArcSoft Super Night
);

/************************************************************************
 * The function is used to super night  preprocess
 ************************************************************************/
ARC_SN_API MRESULT ARC_SN_PreProcess(MHandle hHandle // [in] The handle
);

/************************************************************************
 * The function is used to add input frame, one frame once
 ************************************************************************/
ARC_SN_API MRESULT ARC_SN_AddOneInputInfo(
    MHandle hHandle,                         // [in] The handle for ArcSoft Super Night
    LPARC_SN_RAWINFO pRawInfo,               // [in] RawInfo of the frame, which from camera system
    LPARC_SN_INPUTINFO pSNInputInfo,         //  [in] input image data,add one frame once
    LPARC_SN_INPUTINFO pSNResultInfo = MNull // [out] the middle result for tripod
);

/************************************************************************
 * The function is used to super night  process
 ************************************************************************/
ARC_SN_API MRESULT ARC_SN_Process(
    MHandle hHandle,                  // [in] The handle for ArcSoft Super Night
    LPARC_SN_FACEINFO pFaceInfo,      // [in] FaceInfo of the brightest frame
    LPARC_SN_INPUTINFO pSNResultInfo, // [out] the image data for ArcSoft Super Night
    MVoid *pReserved,                 // Reserved
    MInt32 sceneMode, MRECT *pOutImgRect, ARC_SN_FNPROGRESS fnCallback,
    MVoid *pUserData,       // user data, use when special case
    MInt32 i32EVoffset = 0, // user set ev_offset, ev_step is 6. ev_offset = 0, no tackle;ev_offset
                            // > 0, brighten ev_offset; ev_offset < 0, darken ev_offset
    MInt32 i32DeviceOrientation = 0 // The device orientation, 0, 90, 180, 270;
);

/************************************************************************
 * The function is used to Correct image
 ************************************************************************/
ARC_SN_API MRESULT ARC_SN_CorrectImage(
    MHandle hHandle,                // [in] The handle for ArcSoft Super Night
    LPARC_SN_RAWINFO pRawInfo,      // [in] RawInfo of the frame, which from camera system
    LPARC_SN_INPUTINFO pSNInputInfo // [in & out]tansfer in the input[0], do OB andr LSC
);

/************************************************************************
The function used to post process
************************************************************************/
ARC_SN_API MRESULT ARC_SN_PostProcess(MHandle hHandle // [in] The handle for Super Night
);

ARC_SN_API MRESULT ARC_SN_HTP_Init();

ARC_SN_API MRESULT ARC_SN_HTP_UnInit();

#ifdef __cplusplus
}
#endif

#endif // _ARCSOFT_SUPER_NIGHT_MULTI_RAW_H_
