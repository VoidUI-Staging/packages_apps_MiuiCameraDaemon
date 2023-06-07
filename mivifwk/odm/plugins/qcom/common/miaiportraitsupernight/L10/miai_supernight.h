/*******************************************************************************
©2019 ArcSoft Corporation Limited. All rights reserved.

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

#ifndef _ARCSOFT_PORTRAITSUPERNIGHT_H_
#define _ARCSOFT_PORTRAITSUPERNIGHT_H_


#ifdef ARC_PORTRAITSUPERNIGHTDLL_EXPORTS
#ifdef PLATFORM_LINUX
#define ARC_SN_API __attribute__((visibility ("default")))
#else
#define ARC_SN_API __declspec(dllexport)
#endif
#else
#define ARC_SN_API
#endif

#include "asvloffscreen.h"
#include "merror.h"
#include "ammem.h"
#ifdef __cplusplus
extern "C" {
#endif

#define ARC_SN_MAX_INPUT_IMAGE_NUM			20

#define ARC_SN_CAMERA_STATE_UNKNOW			0
#define ARC_SN_CAMERA_STATE_TRIPOD			1
#define ARC_SN_CAMERA_STATE_HAND			2

typedef struct _tag_MIAI_LLHDR_AEINFO {
    MFloat      fLuxIndex;//lux_idx
    MFloat      fISPGain;
    MFloat      fSensorGain;//real_gain
    MFloat      fADRCGain;//adrc_gain
    MFloat      fADRCGainMax;//8.00
    MFloat      fADRCGainMin;//4.00
    MFloat      apture;
    MFloat      exposure;
    int         iso;//iso_value
    int         exp_time;//exp_time
} MIAI_LLHDR_AEINFO, *LPMIAI_LLHDR_AEINFO;

/************************************************************************
* This struct is used to descripe ASN input info
************************************************************************/
typedef struct _tag_ARC_SN_INPUTINFO {
	MInt32				i32CameraState;	//ARC_SN_CAMERA_STATE_UNKNOW,ARC_SN_CAMERA_STATE_TRIPOD,ARC_SN_CAMERA_STATE_HAND
	MInt32				i32CurIndex;
	MInt32				i32ImgNum;
	ASVLOFFSCREEN		InputImages[ARC_SN_MAX_INPUT_IMAGE_NUM];
	MFloat 				InputImagesEV[ARC_SN_MAX_INPUT_IMAGE_NUM];
} ARC_SN_INPUTINFO, *LPARC_SN_INPUTINFO;

/************************************************************************
* This struct is used to descripe ASN parameters
************************************************************************/
typedef struct _tag_ARC_SN_Param {
	MInt32					i32CurveBrightness;	    // [in]  Adjustable parameter for brightness, the range is 0~+100
	MInt32					i32CurveContrast;		// [in]  Adjustable parameter for contrast, the range is -100~+100
	MInt32					i32Sharpness;			// [in]  Adjustable parameter for sharpness, the range is 0~+100
} ARC_SN_PARAM , *LPARC_SN_PARAM;

/************************************************************************
* This function is implemented by the caller, registered with
* any time-consuming processing functions, and will be called
* periodically during processing so the caller application can
* obtain the operation status (i.e., to draw a progress bar),
* as well as determine whether the operation should be canceled or not
************************************************************************/
typedef MRESULT (*ARC_SN_FNPROGRESS) (
	MInt32		i32Progress,			// The percentage of the current operation
	MInt32		i32Status,				// The current status at the moment
	MVoid		*pParam					// Caller-defined data
);

/************************************************************************
* This function is used to get version information of library
************************************************************************/
ARC_SN_API const MPBASE_Version *ARC_SN_GetVersion();

/************************************************************************
* The function is used to init  engine
************************************************************************/
#define ASN_SN_FRONT_CAMERA					0x200

ARC_SN_API MRESULT  ARC_SN_Init(
	MInt32					i32CameraMode,	// The Camera type,ASN_SN_BACK_CAMERA and ASN_SN_FRONT_CAMERA
    MHandle                 *phHandle		// The handle for ArcSoft Super Night
);

/************************************************************************
* The function is used to uninit  engine
************************************************************************/
ARC_SN_API MRESULT  ARC_SN_Uninit(
    MHandle                 *phHandle		// The handle for ArcSoft Super Night
);

/************************************************************************
* The function is used to super night  process
************************************************************************/

ARC_SN_API MRESULT ARC_SN_Process(
	MHandle					hHandle,         // [in] The handle for ArcSoft Super Night
	MFloat					fTotalGain,		 // [in] input gain of last image
	LPARC_SN_INPUTINFO		pInputInfo,      // [in] input image data
	LPASVLOFFSCREEN			pResultImg,		 // [out] the image data for ArcSoft Super Night
	LPARC_SN_PARAM			pParam,          // [in] parmar of bright and tone
	MRECT *                 pCropOut,        //  [out] crop region,if input image data is YUV, pCropOut should be NULL 
	MInt32 					orient,
	LPMIAI_LLHDR_AEINFO     pAeInfo,
	MInt32 					flashstate,
	MPChar                  macePath,
	MInt32					passby = 0
	);

// ARC_SN_API MRESULT ARC_SN_Process(
// 	MHandle					hHandle,         // [in] The handle for ArcSoft Super Night
// 	MFloat					fTotalGain,		 // [in] input gain of last image
// 	LPARC_SN_INPUTINFO		pInputInfo,      // [in] input image data
// 	LPASVLOFFSCREEN			pResultImg,		 // [out] the image data for ArcSoft Super Night
// 	LPARC_SN_PARAM			pParam,          // [in] parmar of bright and tone
// 	MRECT *                 pCropOut         //  [out] crop region,if input image data is YUV, pCropOut should be NULL 
// 	);
/************************************************************************
* The function is reserved to super night callback
************************************************************************/
ARC_SN_API MRESULT  ARC_SN_SetCallback(
	MHandle				hHandle,			// [in]  The algorithm engine
	ARC_SN_FNPROGRESS	fnCallback,			// [in]  The callback function
	MVoid				*pUserData
	);

#ifdef __cplusplus
}
#endif

#endif /* ARCSOFT_PORTRAITSUPERNIGHT_H */

