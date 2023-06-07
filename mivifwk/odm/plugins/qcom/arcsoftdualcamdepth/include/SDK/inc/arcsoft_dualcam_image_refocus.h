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
#ifndef ARCSOFT_DUALCAM_IMAGE_REFOCUS_H_
#define ARCSOFT_DUALCAM_IMAGE_REFOCUS_H_


#ifdef ARC_DCIRDLL_EXPORTS
#ifdef PLATFORM_LINUX
#define ARCDCIR_API __attribute__((visibility ("default")))
#else
#define ARCDCIR_API __declspec(dllexport)
#endif
#else
#define ARCDCIR_API
#endif

#include "asvloffscreen.h"
#include "merror.h"
#include "ammem.h"
#include "arcsoft_dualcam_common_refocus.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tag_ARC_DCIR_PARAM {
	MFloat		fMaxFOV;						// [in]  The maximum camera FOV among horizontal and vertical in degree
	ARC_DCR_FACE_PARAM	faceParam;				// [in]  The information of faces in the left image
} ARC_DCIR_PARAM, *LPARC_DCIR_PARAM;

#define ARC_DCIR_REFOCUS_CAMERA_ROLL_0      0
#define ARC_DCIR_REFOCUS_CAMERA_ROLL_90     1
#define ARC_DCIR_REFOCUS_CAMERA_ROLL_180     2
#define ARC_DCIR_REFOCUS_CAMERA_ROLL_270     3

// Bokeh mode
#define  ARC_DCIR_BOKEH_NORMAL      0
#define  ARC_DCIR_BOKEH_LITE        1
typedef struct _tag_ARC_DCIR_METADATA {
    MInt32 i32CameraRoll;                   // [in] Camera roll degree;
	MInt32 i32CalMode;                     // [in]
} ARC_DCIR_METADATA, *LPARC_DCIR_METADATA;


/************************************************************************
 * the following functions for Arcsoft disparity data
 ************************************************************************/
 
ARCDCIR_API const MPBASE_Version *ARC_DCIR_GetVersion();

//The process mode define
#define ARC_DCIR_NORMAL_MODE						0x01

#define ARC_DCIR_VenderTag_bypass     "persist.vendor.camera.arcsoft.bokeh_image.bypass"
#define ARC_DCIR_VenderTag_dump        "persist.vendor.camera.arcsoft.bokeh_image.dump"
#define ARC_DCIR_VenderTag_dumppath    "/data/vendor/camera/bokeh_imagedump/"
#define ARC_DCIR_VenderTag_dumppathsdcard "/sdcard/DCIM/Camera/bokeh_imagedump/"

ARCDCIR_API MRESULT ARC_DCIR_SetLogLevel(
        MInt32           i32LogLevel
);

ARCDCIR_API MRESULT ARC_DCIR_Init(                  // return MOK if success, otherwise fail
		MHandle *phHandle,                			// [out] The algorithm engine will be initialized by this API
		MInt32 i32Mode                				// [in]  The mode of algorithm engine,default is ARC_DCIR_NORMAL_MODE
);
ARCDCIR_API MRESULT ARC_DCIR_Uninit(                // return MOK if success, otherwise fail
		MHandle *phHandle                    		// [in/out] The algorithm engine will be un-initialized by this API
);
ARCDCIR_API MRESULT ARC_DCIR_SetCameraImageInfo(    // return MOK if success, otherwise fail
		MHandle hHandle,                            // [in]  The algorithm engine
		LPARC_REFOCUSCAMERAIMAGE_PARAM pstParam        // [in]  Camera and image information
);

ARCDCIR_API MRESULT ARC_DCIR_SetCaliData(			// return MOK if success, otherwise fail
	MHandle				hHandle ,   				// [in]  The algorithm engine
	LPARC_DC_CALDATA   pCaliData                    // [in]   Calibration Data
	);
ARCDCIR_API MRESULT ARC_DCIR_SetDistortionCoef(		// return MOK if success, otherwise fail
		MHandle				hHandle ,				// [in]  The algorithm engine
		MFloat				*pMainDistortionCoef,	// [in]  The main image distortion coefficient,size is float[11]
		MFloat				*pAuxDistortionCoef		// [in]  The auxiliary image distortion coefficient,size is float[11]
);
ARCDCIR_API MRESULT ARC_DCIR_CalcDisparityData(		// return MOK if success, otherwise fail
	MHandle				hHandle,					// [in]  The algorithm engine
	LPASVLOFFSCREEN		pMainImg,					// [in]  The offscreen of main image input. Generally, Left image or Wide image as Main.
	LPASVLOFFSCREEN		pAuxImg,					// [in]  The offscreen of auxiliary image input.
	LPARC_DCIR_PARAM	pDCIRParam,		 			// [in]  The parameters for algorithm engine
    LPARC_DCIR_METADATA pMetaData
);

ARCDCIR_API MRESULT ARC_DCIR_GetDisparityDataSize(		// return MOK if success, otherwise fail
	MHandle				hHandle,					    // [in]  The algorithm engine
	MInt32				*pi32Size						// [out] The size of disparity map
);

ARCDCIR_API MRESULT ARC_DCIR_GetDisparityData(			// return MOK if success, otherwise fail
	MHandle				hHandle,						// [in]  The algorithm engine
	MVoid				*pDisparityData						// [out] The data of disparity map
);

ARCDCIR_API MRESULT ARC_DCIR_Reset(                // return MOK if success, otherwise fail
		MHandle             hHandle                // [in/out] The algorithm engine will be reset by this API
);


#define ARC_DCIR_NORMAL_MODE				0x01
#define ARC_DCIR_NIGHT_MODE                 0X02
ARCDCIR_API MRESULT  ARC_DCIR_SetCaptureMode(
        MHandle				        hEngine,			     // [in]  The algorithm engine
        MInt32                      i32Mode                  // [in] Normal model or night mode
);

ARCDCIR_API MRESULT  ARC_DCIR_SetNightLevel(		// return MOK if success, otherwise fail
        MHandle				   hEngine,		      	// [in]  The algorithm engine
        MUInt32                i32NightLevel         //[in] value 1,2(night,extreme night)

);

#ifdef __cplusplus
}
#endif


#endif /* ARCSOFT_DUALCAM_IMAGE_REFOCUS_H_ */