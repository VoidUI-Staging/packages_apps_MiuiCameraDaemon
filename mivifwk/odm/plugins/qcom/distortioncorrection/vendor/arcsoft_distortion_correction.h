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

#ifndef ARCSOFT_DC_H_20180919_1828
#define ARCSOFT_DC_H_20180919_1828


#ifdef ARC_DCDLL_EXPORTS
#ifdef PLATFORM_LINUX
#define ARC_DC_API __attribute__((visibility ("default")))
#else
#define ARC_DC_API __declspec(dllexport)
#endif
#else
#define ARC_DC_API
#endif

#include "asvloffscreen.h"
#include "ammem.h"
#include "merror.h"
#include "amcomdef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARC_DC_MODE_IMAGE_REAR    		0x0    //Use optical correction and face protection for Img
#define ARC_DC_MODE_IMAGE_FRONT    		0x1    //Use optical correction and face protection for Img
#define ARC_DC_MODE_VIDEO_REAR    		0x2    //Use optical correction and face protection for Video
#define ARC_DC_MODE_VIDEO_FRONT    		0x3    //Use optical correction and face protection for Video
#define ARC_DC_MODE_ONLY_UNDISTORT      0x4    //Use optical correction for Video and Img

#define ARC_DC_MODE_ONLY_SELFIE_IMAGE   0x6    //Use face protection for Img
#define ARC_DC_MODE_ONLY_SELFIE_VIDEO   0x7    //Use face protection for Video

enum ADC_OrientCode {
    ADC_FOC_0 = 0x1, 		// 0 degree
    ADC_FOC_90 = 0x2, 		// 90 degree
    ADC_FOC_270 = 0x3, 		// 270 degree
    ADC_FOC_180 = 0x4, 		// 180 degree
    ADC_FOC_30 = 0x5, 		// 30 degree
    ADC_FOC_60 = 0x6, 		// 60 degree
    ADC_FOC_120 = 0x7,		// 120 degree
    ADC_FOC_150 = 0x8, 		// 150 degree
    ADC_FOC_210 = 0x9,		// 210 degree
    ADC_FOC_240 = 0xa, 		// 240 degree
    ADC_FOC_300 = 0xb, 		// 300 degree
    ADC_FOC_330 = 0xc 		// 330 degree
};

//Face Info
typedef struct __tag_ARC_DC_FACE
{
	MInt32 i32FacesNum;            // [in] The face number
	MRECT *prtFace;                // [in] The faces rect
	MInt32 *i32faceOrient;         // [in] the angle of each face
} ARC_DC_FACE, *LPARC_DC_FACE;

////SDK param
//typedef struct __tag_ARC_DC_PARAM
//{
//} ARC_DC_PARAM, *LPARC_DC_PARAM;

//Init Params
typedef struct __tag_ARC_DC_INITPARAM
{
	MByte *pCaliData;						// [in]  The data of camera's calibration
	MInt32 i32CaliDataLen;					// [in]  The size of camera's calibration data
	MFloat fViewAngleH;						// [in] The intensity of face protection
} ARC_DC_INITPARAM, *LPARC_DC_INITPARAM;

/************************************************************************
 * The following functions
 ************************************************************************/
ARC_DC_API const MPBASE_Version *ARC_DC_GetVersion();

ARC_DC_API MRESULT
ARC_DC_Init(                				// return MOK if success, otherwise fail
		MHandle *phHandle,                	// [in/out] The algorithm engine will be initialized by this API
		ARC_DC_INITPARAM *pInitParam,    	// [in] The param to init the algorithm engine
		MInt32  nMode						// [in] such as : ARC_DC_MODE_IMAGE_REAR
);

//ARC_DC_API MRESULT
//ARC_DC_SetParam(                			// return MOK if success, otherwise fail
//		MHandle hHandle,                    // [in]  The algorithm engine
//		ARC_DC_PARAM *pParam                // [in]  The algorithm parameter
//);

ARC_DC_API MRESULT
ARC_DC_Process(                    			// return MOK if success, otherwise fail
		MHandle hHandle,                	// [in]  The algorithm engine
		LPASVLOFFSCREEN pSrcImg,        	// [in]  The off-screen of source image, will be changed after process
		LPASVLOFFSCREEN pDstImg,        	// [in/out]  The off-screen of destination image
		LPARC_DC_FACE stFaceInfo        	// [in] The face info for distortion correction
);

ARC_DC_API MRESULT
ARC_DC_Uninit(                				// return MOK if success, otherwise fail
		MHandle *phHandle                	// [in/out] The algorithm engine will be un-initialized by this API
);


#ifdef __cplusplus
}
#endif

#endif /* ARCSOFT_DC_H_20180919_1828 */
