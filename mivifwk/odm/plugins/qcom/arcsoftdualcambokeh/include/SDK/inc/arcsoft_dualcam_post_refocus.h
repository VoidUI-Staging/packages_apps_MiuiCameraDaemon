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
#ifndef DUALCAMREFOCUS_ARCSOFT_DUALCAM_POST_REFOCUS_H
#define DUALCAMREFOCUS_ARCSOFT_DUALCAM_POST_REFOCUS_H
#ifdef ARC_DCIRDLL_EXPORTS
#ifdef PLATFORM_LINUX
#define ARCDCIR_API __attribute__((visibility ("default")))
#else
#define ARCDCIR_API __declspec(dllexport)
#endif
#else
#define ARCDCIR_API
#endif

//#include "arcsoft_dualcam_common_refocus.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "ammem.h"
#include "arcsoft_dualcam_common_refocus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARC_DCIRP_VenderTag_bypass     "persist.vendor.camera.arcsoft.bokeh_image.bypass"
#define ARC_DCIRP_VenderTag_dump        "persist.vendor.camera.arcsoft.bokeh_image.dump"
#define ARC_DCIRP_VenderTag_dumppath    "/data/vendor/camera/bokeh_imagedump/"
#define ARC_DCIRP_VenderTag_dumppathsdcard "/sdcard/DCIM/Camera/bokeh_imagedump/"

#define ARC_DCIR_NOISE_LEVEL_1      1
#define ARC_DCIR_NOISE_LEVEL_2      2

typedef struct _tag_ARC_DCIRP_REFOCUS_PARAM {
    MPOINT		ptFocus;						    // [in]  The focus point to decide which region should be clear
    MInt32		i32BlurIntensity;					// [in]  The intensity of blur,Range is [0,100], default as 50.
    MInt32		i32lShineThres;		           // [in]  The parameter for bokeh shine threshold,the range [1,5]
    MInt32		i32ShineLevel;		           // [in]  The parameter for bokeh shine level ,the range [1,255]

} ARC_DCIRP_REFOCUS_PARAM, *LPARC_DCIRP_REFOCUS_PARAM;

typedef struct _tag_ARC_DCIRP_METADATA {
    MInt32      i32CurrentISO;
    MInt32		bNoiseOn;
    MInt32      i32NoiseLevel;
} ARC_DCIRP_METADATA, *LPARC_DCIRP_METADATA;

ARCDCIR_API const MPBASE_Version *ARC_DCIRP_GetVersion();


ARCDCIR_API MRESULT ARC_DCIRP_SetLogLevel(
        MInt32           i32LogLevel
);
#define ARC_DCIRP_POST_REFOCUS_MODE		   		    0x02

ARCDCIR_API MRESULT ARC_DCIRP_Init(                  // return MOK if success, otherwise fail
        MHandle *phHandle,                			// [out] The algorithm engine will be initialized by this API
        MInt32 i32Mode                				// [in]  The mode of algorithm engine,default is ARC_DCIRP_POST_REFOCUS_MODE
);
ARCDCIR_API MRESULT ARC_DCIRP_Uninit(                // return MOK if success, otherwise fail
        MHandle *phHandle                    		// [in/out] The algorithm engine will be un-initialized by this API
);


ARCDCIR_API MRESULT ARC_DCIRP_Process(					// return MOK if success, otherwise fail
MHandle						hHandle,				// [in]  The algorithm engine
MVoid						*pDisparityData,		// [in]  The data of disparity data
MInt32						i32DisparityDataSize,	// [in]  The size of disparity data
LPASVLOFFSCREEN				pMainImg,				// [in]  The off-screen of main image
LPARC_DCIRP_REFOCUS_PARAM	pRFParam,				// [in]  The parameter for refocusing image
LPARC_DCIRP_METADATA        pMetaDat,
LPASVLOFFSCREEN				pDstImg					// [out]  The off-screen of result image
);
ARCDCIR_API MRESULT ARC_DCIRP_CropResizeImg(					// return MOK if success, otherwise fail
        MHandle			            	hHandle ,   				    // [in]  The algorithm engine
        LPASVLOFFSCREEN		            pHDRBokehImg,		            // [in] The offscreen of enhance image
        MRECT*                          pHDRRect,                      // [in] Current main image crop rect
        LPASVLOFFSCREEN		            pHDRDestImg		               // [in/Out] The offscreen of enhance image
);

ARCDCIR_API MRESULT ARC_DCIRP_Reset(                // return MOK if success, otherwise fail
        MHandle             hHandle                // [in/out] The algorithm engine will be reset by this API
);
#ifdef __cplusplus
}
#endif
#endif //DUALCAMREFOCUS_ARCSOFT_DUALCAM_POST_REFOCUS_H
