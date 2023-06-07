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
#define MAX_FACE_ROI                   10
#define MAX_EYELAND_POINT              5

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
typedef struct _tag_ARC_LLHDR_PARAM
{
    MInt32 i32Intensity;        // Range is [0,100], default is 40.
    MInt32 i32LightIntensity;   // Range is [0,100], default is 30.
    MInt32 i32Saturation;       // Range is [0,100], default is 50.
    MInt32 i32SharpenIntensity; // Range is [0,100], default is 40.
    MBool bEnableFDInside;      // Range is [0,1], default is 0;
} ARC_LLHDR_PARAM, *LPARC_LLHDR_PARAM;

// the camera type
#define ARC_LLHDR_CAMERA_REAR  0
#define ARC_LLHDR_CAMERA_FRONT 1

// [in]  cameratype ,ARC_LLHDR_CAMERA_REAR and ARC_LLHDR_CAMERA_FRONT
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
} ARC_LLHDR_INPUTINFO, *LPARC_LLHDR_INPUTINFO;

typedef struct _tag_FDPoint
{
    MFloat x; ///< Co-ordinate indicating x value
    MFloat y; ///< Co-ordinate indicating y value
} HDRFDPoint;

typedef struct _tag_EyeLandmark
{
    MInt8 faceroinum;
    HDRFDPoint left[MAX_EYELAND_POINT];
    HDRFDPoint right[MAX_EYELAND_POINT];
} EYELAND_INFO;

typedef struct _tag_EyesLandmark_INFO
{
    MInt8 eyepairnum;
    EYELAND_INFO eyeLand_info[MAX_FACE_ROI];
} EYES_LANDMARK_INFO, *LPEYES_LANDMARK_INFO;

typedef struct _tag_ARC_FACEINFO
{
    MInt32 nFace;        // The number of faces detected
    MRECT *rcFace;       // The bounding box of face
    MInt32 *lFaceOrient; // The angle of face
} ARC_LLHDR_FACEINFO, *LPARC_LLHDR_FACEINFO;

LOWLIGHTHDR_API MRESULT ARC_LLHDR_Init( // return MOK if success, otherwise fail
    MHandle *phHandle // [out] The algorithm engine will be initialized by this API
);

LOWLIGHTHDR_API MRESULT ARC_LLHDR_Uninit( // return MOK if success, otherwise fail
    MHandle *phHandle // [in/out] The algorithm engine will be un-initialized by this API
);

LOWLIGHTHDR_API MRESULT ARC_LLHDR_Process( // return MOK if success, otherwise fail
    MHandle hHandle,                       // [in]  The algorithm engine
    LPARC_LLHDR_INPUTINFO pSrcImgs,        // [in]  The offscreen of source images
    LPARC_LLHDR_FACEINFO pFaceInfo,        // [in]  The input face info
    LPASVLOFFSCREEN pDstImg,               // [out] The offscreen of result image,this can be NULL
    MInt32 *pRetIndexInSrcImgs, // [out] if the pDstImg == NULL,the result will save in the
                                // pSrcImgs->pImages[*pRetIndexInSrcImgs]
    LPARC_LLHDR_PARAM pParam,   // [in]  The parameters for algorithm engine
    MInt32 isoValue,            // [in]  The Value of ISO
    MInt32 cameratype // [in]  The Camera type,ARC_LLHDR_CAMERA_REAR and ARC_LLHDR_CAMERA_FRONT

);

LOWLIGHTHDR_API MVoid ARC_LLHDR_SetDRCGain(
    MHandle hEnhancer, // [in]  The algorithm engine
    MFloat drcGainVal  // [in]  The parameter of DRC gain Value
);

LOWLIGHTHDR_API MVoid ARC_LLHDR_SetISPGain(
    MHandle hEnhancer, // [in]  The algorithm engine
    MFloat ispGainVal  // [in]  The parameter of ISP gain Value
);

LOWLIGHTHDR_API MVoid ARC_LLHDR_SetEVValue(MHandle hEnhancer, // [in]  The algorithm engine
                                           MFloat EvValue     // [in] The parameter of EV Value
);

LOWLIGHTHDR_API MVoid
ARC_LLHDR_SetCallback(MHandle hHandle,                 // [in]  The algorithm engine
                      ARC_LLHDR_FNPROGRESS fnCallback, // [in]  The callback function
                      MVoid *pUserData);

#ifdef __cplusplus
}
#endif

#endif // _ARCSOFT_LOWLIGHTLLHDR_H_
