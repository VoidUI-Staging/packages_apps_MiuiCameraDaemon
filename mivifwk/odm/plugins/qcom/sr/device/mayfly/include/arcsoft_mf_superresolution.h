/*******************************************************************************
@2019 ArcSoft Corporation Limited. All rights reserved.

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
#ifndef _ARCSOFT_MFSUPERRESOLUTION_H_
#define _ARCSOFT_MFSUPERRESOLUTION_H_

#include "asvloffscreen.h"
#include "merror.h"
#include "amcomdef.h"
#include "ammem.h"

#ifdef MFSUPERRESOLUTIONDLL_EXPORTS
#define MFSUPERRESOLUTION_API __declspec(dllexport)
#else
#define MFSUPERRESOLUTION_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is implemented by the caller, registered with any time-consuming processing functions,
 * and will be called periodically during processing so the caller application can obtain the operation
 * status (i.e., to draw a progress bar), as well as determine whether the operation should be canceled or not
 */
typedef MRESULT (*ARC_MFSR_FNPROGRESS) (
	MLong		lProgress,
	MLong		lStatus,
	MVoid		*pParam
);

/**
 * @brief This function is used to get version information of library
 */
MFSUPERRESOLUTION_API const MPBASE_Version *ARC_MFSR_GetVersion();

/**
 * This struct is used to define input meta data for the algorithm engine
 */
typedef struct _tag_ARC_MFSR_META_DATA
{
	MInt32 	nCameraType;
	MInt32 	nISO;
	MUInt64 nExpoTime;
	MFloat	fEVValue;
	MFloat 	fLuxIndex;
	MFloat 	fDRCGain;
	MFloat 	fDarkBoostGain;
	MFloat 	fSensorGain;
	MFloat 	fDigitalGain;
} ARC_MFSR_META_DATA, *LPARC_MFSR_META_DATA;

/**
 * @brief Define for camera type
 */
#define ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0        0x00    // L3S_IMX707

/**
 * @brief Define for max image num
 */
#define ARC_MFSR_MAX_INPUT_IMAGES		         10

/**
 * face orientation is anticlockwise, and ARC_FACE_ORIENTATION_0 is up orientation
 */
#define ARC_FACE_ORIENTATION_0    0
#define ARC_FACE_ORIENTATION_90   1
#define ARC_FACE_ORIENTATION_180  2
#define ARC_FACE_ORIENTATION_270  3

/**
 * This struct is used to describe face info
 * @param nFace The number of faces detected
 * @param rcFace The bounding box of face
 * @param lFaceOrient The angle of face
 */
typedef struct _tag_ARC_MFSR_FRT_FACEINFO {
	MInt32		nFace;
	MRECT 		*rcFace;
	MInt32	    *lFaceOrient;
} ARC_MFSR_FACEINFO, *LPARC_MFSR_FACEINFO;

/**
 * @brief This struct is used to describe sdk param
 * @param i32SharpenIntensity [in] The intensity if image sharpness, range is [0,100]
 * @param i32DenoiseIntensity [in] The intensity of image denoise, range is [0,100]
 * @param i32MoonSceneFlag [in] The flag of moon scene, 0 means normal scene, range is [0,1]
 */
typedef struct _tag_ARC_MFSR_PARAM {
	MInt32				i32SharpenIntensity;
	MInt32				i32DenoiseIntensity;
	MInt32              i32MoonSceneFlag;
} ARC_MFSR_PARAM, *LPARC_MFSR_PARAM;


//Sample code:
//FN_ARC_MFSR_android_log_print fn_log_print = __android_log_print;
// ARC_MFSR_SetLogLevel(
//        ARC_MFSR_LOGLEVEL_1,
//        fn_log_print
//        );
//Notes: Use ARC_MFSR_SetLogLevel() to set fn_log_print is not thread-safe because performance.
//       So please don't call it frequently, and suggest to call it before SDK created.
//       fn_log_print will be set to global value, so please keep the life longer than SDK's.
//       You can set NULL to clean it.
typedef int (*FN_ARC_MFSR_android_log_print)(int prio, const char* tag, const char* fmt, ...);
#define ARC_MFSR_LOGLEVEL_0     0
#define ARC_MFSR_LOGLEVEL_1     1
#define ARC_MFSR_LOGLEVEL_2     2
MFSUPERRESOLUTION_API MVoid ARC_MFSR_SetLogLevel(
        // [in] ARC_MFSR_LOGLEVEL_0 is disable log and ARC_MFSR_LOGLEVEL_2 is enable all log.
        MInt32        i32LogLevel,
FN_ARC_MFSR_android_log_print fn_log_print = MNull
);
#define ARC_MFSR_VendorTag_bypass "persist.vendor.camera.arcsoft.mfsr.bypass"
#define ARC_MFSR_VendorTag_dump   "persist.vendor.camera.arcsoft.mfsr.dump"
/**
 * @brief This function is used to create algorithm engine
 * @param phEnhancer [out] The algorithm engine will be initialized by this API
 * @param pSrcImg [in] The offscreen of source image, do not need image data
 * @param pDstImg [in] The offscreen of result image, do not need image data
 * @return MOK if success, otherwise fail
 */
MFSUPERRESOLUTION_API MRESULT ARC_MFSR_Init(
	MHandle				    *phEnhancer,
	LPASVLOFFSCREEN         pSrcImg,
	LPASVLOFFSCREEN         pDstImg
);

/**
 * @brief This function is used to destroy algorithm engine
 * @param phEnhancer [in/out] The algorithm engine will be un-initialized by this API
 * @return return MOK if success, otherwise fail
 */
MFSUPERRESOLUTION_API MRESULT ARC_MFSR_UnInit(
	MHandle				    *phEnhancer
);

/**
 * @brief This function is used to set crop region
 * @param hEnhancer [in] The algorithm engine
 * @param pRtScaleRegion [in] The region of zoom rectangle
 * @return MOK if success, otherwise fail
 */
MFSUPERRESOLUTION_API MRESULT ARC_MFSR_SetCropRegion (
		MHandle				hEnhancer,
		MRECT				*pRtScaleRegion
);

/**
 * @brief This function is used to do some pre-process work for algorithm
 * @param hEnhancer [in] The algorithm engine
 * @param pSrcImg [in] The offscreen of source image, should not be modified until all process end,
 * and the image num should be between ARC_MFSR_MIN_INPUT_IMAGES and ARC_MFSR_MAX_INPUT_IMAGES
 * @param i32ImgIndex [in] The index of current input image
 * @param pMetaData [in] The meta data for each frame
 * @return MOK if success, otherwise fail
 */
MFSUPERRESOLUTION_API MRESULT ARC_MFSR_PreProcess(
	MHandle				    hEnhancer,
    LPASVLOFFSCREEN		    pSrcImg,
	MInt32        		    i32ImgIndex,
	LPARC_MFSR_META_DATA	pMetaData
);

/**
 * This function is used to get defaulter parameter
 * @param hEnhancer [in] The algorithm engine
 * @param pParam [out] Default parameter
 * @return MOK if success, otherwise fail
 */
MFSUPERRESOLUTION_API MRESULT ARC_MFSR_GetDefaultParam(
		MHandle hEnhancer,
		LPARC_MFSR_PARAM pParam
);

/**
 * @brief This function is used to set reference index, if *pRefIndex < 0, will used default reference index inner
 * @param hEnhancer [in] The algorithm engine
 * @param pRefIndex [in/out] The reference image index, if *pRefIndex < 0, this parameter will be set as real index by algorithm
 * @return MOK if success, otherwise fail
 */
MFSUPERRESOLUTION_API MRESULT ARC_MFSR_SetRefIndex (
		MHandle				hEnhancer,
		MInt32				*pRefIndex
);

/**
 * This function is used to set face info outside
 * @param hEnhancer [in] The algorithm engine
 * @param pFaceInfo [in] The face info of reference image, if pFaceInfo is nullptr, will use face info detected bt sdk
 * @return MOK if success, otherwise fail
 */
MFSUPERRESOLUTION_API MRESULT ARC_MFSR_SetFaceInfo (
		MHandle				hEnhancer,
		LPARC_MFSR_FACEINFO pFaceInfo
);

/**
 * @brief This function is the main process for algorithm
 * @param hEnhancer [in] The algorithm engine
 * @param pSRParam [in] The parameters for algorithm engine
 * @param pDstImg [out] The offscreen of result image
 * @return MOK if success, otherwise fail
 */
MFSUPERRESOLUTION_API MRESULT ARC_MFSR_Process(
	MHandle				    hEnhancer,
	LPARC_MFSR_PARAM		pSRParam,
	LPASVLOFFSCREEN		    pDstImg
);

/**
 * @brief This function is used to set callback function
 * @param hEnhancer [in] The algorithm engine
 * @param fnCallback [in] The callback function
 * @param pUserData [in] Caller-defined data
 */
MFSUPERRESOLUTION_API MVoid  ARC_MFSR_SetCallback(
	MHandle				    hEnhancer,
	ARC_MFSR_FNPROGRESS     fnCallback,
	MVoid				    *pUserData
);

#ifdef __cplusplus
}
#endif

#endif // _ARCSOFT_MFSUPERRESOLUTION_H_
