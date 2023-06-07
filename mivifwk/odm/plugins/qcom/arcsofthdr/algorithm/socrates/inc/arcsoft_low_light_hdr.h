/*******************************************************************************
Copyright(c) ArcSoft Corporation Limited, All right reserved.

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
#ifndef _ARCSOFT_LOW_LIGHT_HDR_H_
#define _ARCSOFT_LOW_LIGHT_HDR_H_

#include "amcomdef.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "merror.h"

#ifdef LOWLIGHTHDRDLL_EXPORTS
#define LOWLIGHTHDR_API __declspec(dllexport)
#else
#define LOWLIGHTHDR_API
#endif

#define MERR_LLHDR_ALIGN_MERR          (23)
#define MERR_LLHDR_INPUT_SEQUENCE_MERR (24)

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
 * This function is implemented by the caller, registered with
 * any time-consuming processing functions, and will be called
 * periodically during processing so the caller application can
 * obtain the operation status (i.e., to draw a progress bar),
 * as well as determine whether the operation should be canceled or not
 ************************************************************************/
typedef MRESULT (*ARC_LLHDR_FNPROGRESS)(
    MLong i32Progress, // The percentage of the current operation
    MLong i32Status,   // The current status at the moment
    MVoid *pUserData   // Caller-defined data
);

/************************************************************************
 * This function is used to get version information of library
 ************************************************************************/
LOWLIGHTHDR_API const MPBASE_Version *ARC_LLHDR_GetVersion();

/************************************************************************
 * This function is used to get default parameters of library
 ************************************************************************/

typedef enum _tag_ARC_LLHDR_DEVICE_TYPE {
    ARC_LLHDR_DEVICE_TYPE_XIAOMI_L12 = 0,
    ARC_LLHDR_DEVICE_TYPE_XIAOMI_OTHER = 1
} ARC_LLHDR_DEVICE_TYPE;

typedef struct _tag_ARC_LLHDR_PARAM
{
    MInt32 i32Intensity;        // Range is [0,100], default is 30.
    MInt32 i32LightIntensity;   // Range is [0,100], default is 0.
    MInt32 i32Saturation;       // Range is [0,100], default is 50.
    MInt32 i32SharpenIntensity; // Range is [0,100], default is 0.

    MInt32 i32YReprocIntensity;  // Range is [0,100], default is 15.
    MInt32 i32UVReprocIntensity; // Range is [0,100], default is 15.

    MBool bEnableFDInside; // Range is [0,1], default is 0;

} ARC_LLHDR_PARAM, *LPARC_LLHDR_PARAM;

// the camera type
#define ARC_LLHDR_CAMERA_REAR    0
#define ARC_LLHDR_CAMERA_FRONT   1
#define ARC_LLHDR_CAMERA_REAR_UW 2

typedef enum _tag_ARC_LLHDR_Img_Orientation {
    ARC_LLHDR_Ori_0 = 0,   // 0 degree
    ARC_LLHDR_Ori_1 = 90,  // 90 degree
    ARC_LLHDR_Ori_2 = 180, // 180 degree
    ARC_LLHDR_Ori_3 = 270  // 270 degree
} ARC_LLHDR_ImgOrient;

// [in]  cameratype ,ARC_LLHDR_CAMERA_REAR,ARC_LLHDR_CAMERA_REAR_UW,and ARC_LLHDR_CAMERA_FRONT
LOWLIGHTHDR_API MRESULT ARC_LLHDR_GetDefaultParam(LPARC_LLHDR_PARAM pParam, MInt32 isoValue,
                                                  MInt32 cameratype);

/************************************************************************
 * The functions is used to perform image night hdr
 ************************************************************************/
#define ARC_LLHDR_MAX_INPUT_IMAGE_NUM 8

typedef struct _tag_ARC_LLHDR_INPUTINFO
{
    MInt32 i32ImgNum;
    LPASVLOFFSCREEN pImages[ARC_LLHDR_MAX_INPUT_IMAGE_NUM];
    MFloat fEvCompVal[ARC_LLHDR_MAX_INPUT_IMAGE_NUM]; // The exposure compensation of input images,
                                                      // such as, the value of normal exposure image
                                                      // is 0.0, the value of low exposure image may
                                                      // be -1.0, -2.0 or -3.0, etc.
} ARC_LLHDR_INPUTINFO, *LPARC_LLHDR_INPUTINFO;

typedef struct _tag_ARC_LLHDR_FACEINFO
{
    MInt32 nFace;        // The number of faces detected
    MRECT *rcFace;       // The bounding box of face
    MInt32 *lFaceOrient; // The angle of face,anticlockwise degree:0,90,180,270
} ARC_LLHDR_FACEINFO, *LPARC_LLHDR_FACEINFO;

#define ARC_LLHDR_LOGLEVEL_0 0
#define ARC_LLHDR_LOGLEVEL_1 1
#define ARC_LLHDR_LOGLEVEL_2 2
LOWLIGHTHDR_API MVoid ARC_LLHDR_SetLogLevel(
    MInt32 i32LogLevel // [in] ARC_LLHDR_LOGLEVEL_0 is disable log and ARC_LLHDR_LOGLEVEL_2 is
                       // enable all log.
);

#define ARC_LLHDR_VendorTag_bypass "persist.vendor.camera.arcsoft.hdr.bypass"
#define ARC_LLHDR_VendorTag_dump   "persist.vendor.camera.arcsoft.hdr.dump"
LOWLIGHTHDR_API MRESULT ARC_LLHDR_Init( // return MOK if success, otherwise fail
    MHandle *phHandle // [out] The algorithm engine will be initialized by this API
);

LOWLIGHTHDR_API MRESULT ARC_LLHDR_Uninit( // return MOK if success, otherwise fail
    MHandle *phHandle // [in/out] The algorithm engine will be un-initialized by this API
);

LOWLIGHTHDR_API MVoid ARC_LLHDR_SetImgOri(
    MHandle hEnhancer,         // [in] The algorithm engine
    ARC_LLHDR_ImgOrient ImgOri // [in] The value of image orientation
);

LOWLIGHTHDR_API MVoid ARC_LLHDR_SetEVStep(MHandle hEnhancer, // [in]  The algorithm engine
                                          MInt32 i32EvStep   // [in]  The parameter of EV step Value
);

LOWLIGHTHDR_API MRESULT ARC_LLHDR_Process( // return MOK if success, otherwise fail
    MHandle hHandle,                       // [in]  The algorithm engine
    LPARC_LLHDR_INPUTINFO pSrcImgs,        // [in]  The offscreen of source images
    LPARC_LLHDR_FACEINFO pFaceInfo,        // [in]  The input face info
    LPASVLOFFSCREEN pDstImg,               // [out] The offscreen of result image,this can be NULL
    MInt32 *pRetIndexInSrcImgs, // [out] if the pDstImg == NULL,the result will save in the
                                // pSrcImgs->pImages[*pRetIndexInSrcImgs];
                                //        or pSrcImgs->pImages[*pRetIndexInSrcImgs] is the
                                //        referenced source image which the pDstImg based on.
    LPARC_LLHDR_PARAM pParam,   // [in]  The parameters for algorithm engine
    MInt32 isoValue,            // [in]  The Value of ISO
    MInt32 cameratype // [in]  The Camera type,ARC_LLHDR_CAMERA_REAR,ARC_LLHDR_CAMERA_REAR_UW and
                      // ARC_LLHDR_CAMERA_FRONT

);

LOWLIGHTHDR_API MVoid
ARC_LLHDR_SetCallback(MHandle hHandle,                 // [in]  The algorithm engine
                      ARC_LLHDR_FNPROGRESS fnCallback, // [in]  The callback function
                      MVoid *pUserData);

LOWLIGHTHDR_API MVoid ARC_LLHDR_SetDeviceType( // must be call before ARC_LLHDR_Process
    MHandle hHandle,                           // [in]  The algorithm engine
    ARC_LLHDR_DEVICE_TYPE i32DeviceType        // [in] default is ARC_LLHDR_DEVICE_TYPE_XIAOMI_L12
);

// [in]  cameratype ,ARC_LLHDR_CAMERA_REAR,ARC_LLHDR_CAMERA_REAR_UW,and ARC_LLHDR_CAMERA_FRONT
LOWLIGHTHDR_API MRESULT ARC_LLHDR_GetDefaultParamForDeviceType(LPARC_LLHDR_PARAM pParam,
                                                               MInt32 isoValue, MInt32 cameraType,
                                                               MInt32 deviceType);

#ifdef __cplusplus
}
#endif

#endif // _ARCSOFT_LOWLIGHTLLHDR_H_
