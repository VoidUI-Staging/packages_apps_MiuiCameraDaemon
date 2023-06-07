/*******************************************************************************
   Copyright(c) XiaoMI, All right reserved.
*******************************************************************************/

#ifndef MI_SUPERNIGHT_RAW_H
#define MI_SUPERNIGHT_RAW_H

#include "mi_com_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INPUT_MAX_NUM 20

// camera state
#define CAMERA_STATE_UNKNOWN 0
#define CAMERA_STATE_TRIPOD  1
#define CAMERA_STATE_HAND    2
#define CAMERA_STATE_AUTO    3

// camera lens mode
#define LENS_REAR_WIDE 0x0100 // rear wide
#define LENS_REAR_UW   0x0200 // rear uw
#define LENS_REAR_TELE 0x0400 // rear tele
#define LENS_MASK      0xFF00 // lens mask

// feature
#define FEATURE_LL   0x01   // lowlight
#define FEATURE_SN   0x02   // normal mode SN supernight
#define FEATURE_SE   0x03   // fast mode SE supernight
#define FEATURE_MASK 0x00FF // bit mask

typedef struct _ORGIMAGE
{
    MUINT32 u32PixelArrayFormat;
    MINT32 i32Width;
    MINT32 i32Height;
    MUINT8 *ppu8Plane[4];
    MINT32 pi32Pitch[4];
} ORGIMAGE, *LPORGIMAGE;

typedef struct _MI_SN_INPUTINFO
{
    MINT32 i32CameraState;
    MINT32 i32CurIndex;
    MINT32 i32ImgNum;
    ORGIMAGE InputImages[INPUT_MAX_NUM];
    MFLOAT InputImagesEV[INPUT_MAX_NUM];
    MINT32 i32InputFd[INPUT_MAX_NUM];
} MI_SN_INPUTINFO, *LPMI_SN_INPUTINFO;

typedef MLONG (*MI_SN_FNPROGRESS)(MLONG lProgress, // The percentage of the current operation
                                  MLONG lStatus,   // The current status at the moment
                                  MVOID *pParam    // Caller-defined data
);

typedef struct _MI_SN_Userdata
{
    RECT lCropRect;    // [out] the crop rect
    MPVOID lreserved1; // [in]  reserved data 1
    MPVOID lreserved2; // [in]  reserved data 2
} MI_SN_Userdata, *LPMI_SN_Userdata;

typedef struct _MI_SN_FACEINFO
{
    MINT32 i32FaceNum;
    RECT *pFaceRects;
    MINT32 i32FaceOrientation;
} MI_SN_FACEINFO, *LPMI_SN_FACEINFO;

typedef enum {
    SCENEMODE_UNKNOW = 0,
    SCENEMODE_INDOOR,
    SCENEMODE_OUTDOOR,
    SCENEMODE_LOWLIGHT,
    SCENEMODE_PORTRAIT,
    SCENEMODE_NIGHTCLUB,
    SCENEMODE_TRIPOD,
    SCENEMODE_SE_BOKEH
} SN_SCENE_MODE;

typedef struct _MI_SN_RAWINFO
{
    MINT32 i32RawType;                 // [in]  The type of raw data
    MINT32 i32EV[INPUT_MAX_NUM];       // [in]  The EV for image
    MINT32 i32BrightLevel[4];          // [in]  The most bright level for image. Bit width: 12 bit.
    MINT32 i32BlackLevel[4];           // [in]  The most black level for image. Bit width: 12 bit.
    MFLOAT fWbGain[4];                 // [in]  The white balance gains for image
    MFLOAT fShutter;                   // [in]  Shutter speed
    MFLOAT fSensorGain;                // [in]  Sensor gain
    MFLOAT fISPGain;                   // [in]  ISP gain
    MFLOAT fTotalGain;                 // [in]  Total gain
    MINT32 i32LuxIndex;                // [in]  Lux index
    MINT32 i32ExpIndex;                // [in]  Exposure index
    MFLOAT fAdrcGain;                  // [in]  ADRC gain
    MLONG bEnableLSC;                  // [in]  enable or disable LSC
    MINT32 i32LSChannelLength[4];      // [in]  The length of LensShading each channel
    MFLOAT fLensShadingTable[4][1024]; // [in]  The LensShading table
    MFLOAT fCCM[9];                    // [in]  The CCM
    MFLOAT fCCT;                       // [in]  The Correlated Color temperature
} MI_SN_RAWINFO, *LPMI_SN_RAWINFO;

/************************************************************************
 * The function is used to init engine
 ************************************************************************/
MLONG MI_SN_Init(MPVOID *phHandle, // The handle for engine
                 MINT32 i32CameraMode, MINT32 i32CameraState,
                 MINT32 i32Width,          // The input image width (in pixel)
                 MINT32 i32Height,         // The input image height (in pixel)
                 RECT *pZoomRect = nullptr // [in] The Zoom Rect
);

/************************************************************************
 * The function is used to uninit  engine
 ************************************************************************/
MLONG MI_SN_Uninit(MPVOID *phHandle // The handle for engine
);

/************************************************************************
 * The function is used to super night  preprocess
 ************************************************************************/
MLONG MI_SN_PreProcess(MPVOID hHandle // [in] The handle
);

/************************************************************************
 * The function is used to add input frame, one frame once
 ************************************************************************/
MLONG MI_SN_AddOneInputInfo(
    MPVOID hHandle,                           // [in] The handle for engine
    LPMI_SN_RAWINFO pRawInfo,                 // [in] RawInfo of the frame, which from camera system
    LPMI_SN_INPUTINFO pSNInputInfo,           // [in] input image data,add one frame once
    LPMI_SN_INPUTINFO pSNResultInfo = nullptr // [out] the middle result for tripod
);

/************************************************************************
 * The function is used to super night  process
 ************************************************************************/
MLONG MI_SN_Process(
    MPVOID hHandle,                  // [in] The handle for engine
    LPMI_SN_FACEINFO pFaceInfo,      // [in] FaceInfo of the brightest frame
    LPMI_SN_INPUTINFO pSNResultInfo, // [out] the image data for args
    MVOID *pReserved,                // Reserved
    MINT32 sceneMode, RECT *pOutImgRect, MI_SN_FNPROGRESS fnCallback,
    MVOID *pUserData,       // user data, use when special case
    MINT32 i32EVoffset = 0, // user set ev_offset, ev_step is 6. ev_offset = 0, no tackle;ev_offset
                            // > 0, brighten ev_offset; ev_offset < 0, darken ev_offset
    MINT32 i32DeviceOrientation = 0 // The device orientation, 0, 90, 180, 270;
);

/************************************************************************
 * The function is used to Correct image
 ************************************************************************/
MLONG MI_SN_CorrectImage(
    MPVOID hHandle,                // [in] The handle for engine
    LPMI_SN_RAWINFO pRawInfo,      // [in] RawInfo of the frame, which from camera system
    LPMI_SN_INPUTINFO pSNInputInfo // [in & out]tansfer in the input[0], do OB andr LSC
);

/************************************************************************
The function used to post process
************************************************************************/
MLONG MI_SN_PostProcess(MPVOID hHandle // [in] The handle for engine
);

MLONG MI_SN_HTP_Init();

MLONG MI_SN_HTP_UnInit();

#ifdef __cplusplus
}
#endif

#endif // MI_SUPERNIGHT_RAW_H
