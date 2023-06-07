/*******************************************************************************
Copyright ArcSoft Corporation Limited, All right reserved.

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
#ifndef _ARCSOFT_BHDR_H_2_1_20200325_
#define _ARCSOFT_BHDR_H_2_1_20200325_

#include "asvloffscreen.h"
#include "merror.h"
#include "amcomdef.h"
#include "ammem.h"

#ifdef BOKEH_HDRDLL_EXPORTS
#define ARC_BHDR_API __declspec(dllexport)
#else
#define ARC_BHDR_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 @brief Define the Error codes
 @ARC_BHDR_ERR_INVALID_LOW_EXPOSURED_IMAGE Not used low exposure image
 @ARC_BHDR_ERR_INVALID_OVER_EXPOSURED_IMAGE Not used over exposure image
 @ARC_BHDR_ERR_ALIGN_FAIL Other 2 image align failed, just use normal image
 @ARC_BHDR_ERR_INVALID_EXPOSURE Invalid exposure of input images
 @ARC_BHDR_ERR_CONTAIN_GHOST Image contain ghost
 @ARC_BHDR_ERR_GHOST_IN_OVERREGION Image contain ghost in over region
*/
#define ARC_BHDR_ERR_INVALID_LOW_EXPOSURED_IMAGE		(MERR_BASIC_BASE + 20)
#define ARC_BHDR_ERR_INVALID_OVER_EXPOSURED_IMAGE	    (MERR_BASIC_BASE + 21)
#define ARC_BHDR_ERR_ALIGN_FAIL						    (MERR_BASIC_BASE + 22)
#define ARC_BHDR_ERR_INVALID_EXPOSURE				    (MERR_BASIC_BASE + 23)
#define ARC_BHDR_ERR_CONTAIN_GHOST				        (MERR_BASIC_BASE + 24)
#define ARC_BHDR_ERR_GHOST_IN_OVERREGION			    (MERR_BASIC_BASE + 25)

/**
 @brief This define is used for different camera(i32Mode)
 @ARC_BHDR_MODE_WIDE_CAMERA The rear wide camera with 2 frames input(exp:ev0 ev-)
*/
#define ARC_BHDR_MODE_WIDE_CAMERA				    0x0

#define ARC_BHDR_MAX_INPUT_IMAGE_NUM		        2

/**
 @brief This struct is used to describe input info
 @param i32ImgNum[in] The number for captured image
 @param InputImages[in] The input images array
*/
typedef struct _tag_ARC_BHDR_INPUTINFO {
	MInt32				i32ImgNum;
	ASVLOFFSCREEN		InputImages[ARC_BHDR_MAX_INPUT_IMAGE_NUM];
} ARC_BHDR_INPUTINFO, *LPARC_BHDR_INPUTINFO;

/**
 @brief This struct is used to describe face info
 @param nFace[in] The number of faces detected
 @param rcFace[in] The bounding box of face
 @param lFaceOrient[in] The angle of face, 1:0 degree,2:90 degree,3:270 degree,4:180 degree
*/
typedef struct _tag_ARC_BHDR_FACEINFO {
	MInt32				nFace;
	MRECT 				*rcFace;
	MInt32				*lFaceOrient;
} ARC_BHDR_FACEINFO, *LPARC_BHDR_FACEINFO;

/**
 @brief This struct is implemented by the caller, registered with
 any time-consuming processing functions, and will be called
 periodically during processing so the caller application can
 obtain the operation status (i.e., to draw a progress bar),
 as well as determine whether the operation should be canceled or not
 @param i32Progress[out] The percentage of the current operation
 @param i32Status[out] The current status at the moment
 @param pParam[out] Caller-defined data
*/
typedef MRESULT (*ARC_BHDR_FNPROGRESS) (
	MInt32		        i32Progress,
	MInt32		        i32Status,
	MVoid		        *pParam
);

/**
 @brief This function is used to get version information of library
 @return MPBASE_Version
*/
ARC_BHDR_API const MPBASE_Version *ARC_BHDR_GetVersion();

/**
 @brief This struct is used to descripe HDR parameters
 @param i32ToneLength[in] Adjustable parameter for tone mapping, the range is 0~100
 @param i32Brightness[in] Adjustable parameter for brightness, the range is -100~+100
 @param i32Saturation[in] Adjustable parameter for saturation, the range is 0~+100
 @param i32Contrast[in] Adjustable parameter for contrast, the range is 0~+100
*/
typedef struct _tag_ARC_BHDR_Param {
	MInt32				i32ToneLength;
	MInt32				i32Brightness;
	MInt32				i32Saturation;
	MInt32				i32Contrast;
} ARC_BHDR_PARAM , *LPARC_BHDR_PARAM;


/************************************************************************
* The function is used to init  engine
************************************************************************/
//Sample code:
//FN_ARC_BHDR_android_log_print fn_log_print = __android_log_print;
// ARC_BHDR_SetLogLevel(
//        ARC_BHDR_LOGLEVEL_1,
//        fn_log_print
//        );
//Notes: Use ARC_BHDR_SetLogLevel() to set fn_log_print is not thread-safe because performance.
//       So please don't call it frequently, and suggest to call it before SDK created.
//       fn_log_print will be set to global value, so please keep the life longer than SDK's.
//       You can set NULL to clean it.
typedef int (*FN_ARC_BHDR_android_log_print)(int prio, const char* tag, const char* fmt, ...);
#define ARC_BHDR_LOGLEVEL_0     0
#define ARC_BHDR_LOGLEVEL_1     1
#define ARC_BHDR_LOGLEVEL_2     2
ARC_BHDR_API MVoid ARC_BHDR_SetLogLevel(
        // [in] ARC_BHDR_LOGLEVEL_0 is disable log and ARC_BHDR_LOGLEVEL_2 is enable all log.
        MInt32        i32LogLevel,
        FN_ARC_BHDR_android_log_print fn_log_print = MNull
);
#define ARC_BHDR_VendorTag_bypass "persist.vendor.camera.arcsoft.bhdr.bypass"
#define ARC_BHDR_VendorTag_dump   "persist.vendor.camera.arcsoft.bhdr.dump"

/**
 @brief The function is used to init HDR engine
 @param phHandle[in/out] The algorithm engine will be initialized by this API
 @param i32Mode[in]: The mode name for camera
 @return MOK if success, otherwise fail
*/
ARC_BHDR_API MRESULT ARC_BHDR_Init(
	MHandle				*phHandle,
	MInt32				i32Mode
);

/**
 @brief The function is used to uninit HDR engine
 @param phHandle[in/out] The algorithm engine will be un-initialized by this API
 @return return MOK if success, otherwise fail
*/
ARC_BHDR_API MRESULT ARC_BHDR_Uninit(
	MHandle				*phHandle
);

/**
 @brief The function is used to set scene type before ARC_BHDR_GetDefaultParam
 @param hHandle[in] The algorithm engine
 @param i32SceneType[in] The detected scene type, default is 0
 @return return MOK if success, otherwise fail
*/
ARC_BHDR_API MRESULT ARC_BHDR_SetSceneType(
		MHandle			hHandle,
		MInt32	        i32SceneType
);

/**
 @brief The function is used to pre-process input image
 @param hHandle[in] The algorithm engine
 @param pInputImages[in] The input images array
 @param i32InputImageIndex[in] The index of input image in the input image array
 @return return MOK if success, otherwise fail
*/
ARC_BHDR_API MRESULT ARC_BHDR_PreProcess(
        MHandle				    hHandle,
        LPARC_BHDR_INPUTINFO	pInputImages,
        MInt32				    i32InputImageIndex
);

/**
 @brief The function is used to process input image
 @param hHandle[in] The algorithm engine
 @param pParam[in] The parameters for algorithm engine
 @param pFaceInfo[in] The face info of input normal image(ev0)
 @param pDstImg[out] The offscreen of result image
 @return MOK if success, otherwise fail
*/
ARC_BHDR_API MRESULT ARC_BHDR_Process(
        MHandle				    hHandle,
        LPARC_BHDR_PARAM		pParam,
        LPARC_BHDR_FACEINFO     pFaceInfo,
        LPASVLOFFSCREEN         pDstImg
);

/**
 * The function is used to get crop rect after process
 * @param hHandle[in] The algorithm engine
 * @param pRect[out] The crop rect
 * @return MOK if success, otherwise fail
 */
ARC_BHDR_API MRESULT ARC_BHDR_GetCropSize(
        MHandle				    hHandle,
        PMRECT                  pRect
);

/**
 @brief The function is used to set call-back
 @param hHandle[in] The algorithm engine
 @param fnCallback[in] The callback function
 @param pUserData[in/out] Caller-defined data
 @return MVoid
*/
ARC_BHDR_API MVoid  ARC_BHDR_SetCallback(
        MHandle				hHandle,
        ARC_BHDR_FNPROGRESS	fnCallback,
        MVoid				*pUserData
);

/**
 @brief The function is used to get default param
 @param hHandle[in] The algorithm engine
 @param pParam[in/out] The default param for sdk
 @return MOK if success, otherwise fail
*/
ARC_BHDR_API MRESULT ARC_BHDR_GetDefaultParam(
        MHandle				hHandle,
        LPARC_BHDR_PARAM     pParam
);

#ifdef __cplusplus
}
#endif

#endif // _ARCSOFT_BHDR_H_2_1_20200325_
