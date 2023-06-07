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
#define ARCDCIR_API __attribute__((visibility("default")))
#else
#define ARCDCIR_API __declspec(dllexport)
#endif
#else
#define ARCDCIR_API
#endif

#include "ammem.h"
#include "arcsoft_dualcam_common_refocus.h"
#include "asvloffscreen.h"
#include "merror.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tag_ARC_DCIR_PARAM
{
    MInt32 i32ImgDegree; // [in]  The degree of input images
    MFloat fMaxFOV;      // [in]  The maximum camera FOV among horizontal and vertical in degree
    ARC_DCR_FACE_PARAM faceParam; // [in]  The information of faces in the left image
} ARC_DCIR_PARAM, *LPARC_DCIR_PARAM;

typedef struct _tag_ARC_DCIR_REFOCUS_PARAM
{
    MPOINT ptFocus;          // [in]  The focus point to decide which region should be clear
    MInt32 i32BlurIntensity; // [in]  The intensity of blur,Range is [0,100], default as 50.
} ARC_DCIR_REFOCUS_PARAM, *LPARC_DCIR_REFOCUS_PARAM;

typedef struct _tag_ARC_DCIR_ENHANCE_PARAM
{
    MPOINT ptFocus; // [in]  The focus point to decide which region should be clear
    MInt32
        i32Intensity; //[in] set the intensity of MF noise removal Range is [0,100], default is 40.
    MInt32 i32LightIntensity;   //[in] set the intensity of brightness enhancement Range is [0,100],
                                // default is 0.
    MInt32 i32SharpenIntensity; //[in] set the intensity of sharpnessRange is [0,100], default is 40
    MInt32 i32CurrentISO;       //[in] set the iso of current images
} ARC_DCIR_ENHANCE_PARAM, *LPARC_DCIR_ENHANCE_PARAM;

#define ARC_DCIR_MAX_INPUT_IMAGES 6
typedef struct _tag_ARC_DCIR_EHINPUTINFO
{
    MLong lImgNum;                                      // The count of input images
    LPASVLOFFSCREEN pImages[ARC_DCIR_MAX_INPUT_IMAGES]; // The images
} ARC_DCIR_INPUTINFO, *LPARC_DCIR_EHINPUTINFO;

/************************************************************************
 * the following functions for Arcsoft disparity data
 ************************************************************************/

ARCDCIR_API const MPBASE_Version *ARC_DCIR_GetVersion();

// The process mode define
#define ARC_DCIR_NORMAL_MODE 0x01

#define ARC_DCIR_VenderTag_bypass         "persist.vendor.camera.arcsoft.bokeh_image.bypass"
#define ARC_DCIR_VenderTag_dump           "persist.vendor.camera.arcsoft.bokeh_image.dump"
#define ARC_DCIR_VenderTag_dumppath       "/data/vendor/camera/bokeh_imagedump/"
#define ARC_DCIR_VenderTag_dumppathsdcard "/sdcard/DCIM/Camera/bokeh_imagedump/"

ARCDCIR_API MRESULT ARC_DCIR_SetLogLevel(MInt32 i32LogLevel);

ARCDCIR_API MRESULT ARC_DCIR_Init( // return MOK if success, otherwise fail
    MHandle *phHandle,             // [out] The algorithm engine will be initialized by this API
    MInt32 i32Mode // [in]  The mode of algorithm engine,default is ARC_DCIR_NORMAL_MODE
);
ARCDCIR_API MRESULT ARC_DCIR_Uninit( // return MOK if success, otherwise fail
    MHandle *phHandle // [in/out] The algorithm engine will be un-initialized by this API
);
ARCDCIR_API MRESULT ARC_DCIR_SetCameraImageInfo( // return MOK if success, otherwise fail
    MHandle hHandle,                             // [in]  The algorithm engine
    LPARC_REFOCUSCAMERAIMAGE_PARAM pstParam      // [in]  Camera and image information
);

ARCDCIR_API MRESULT ARC_DCIR_SetCaliData( // return MOK if success, otherwise fail
    MHandle hHandle,                      // [in]  The algorithm engine
    LPARC_DC_CALDATA pCaliData            // [in]   Calibration Data
);
ARCDCIR_API MRESULT ARC_DCIR_SetDistortionCoef( // return MOK if success, otherwise fail
    MHandle hHandle,                            // [in]  The algorithm engine
    MFloat *pMainDistortionCoef, // [in]  The main image distortion coefficient,size is float[11]
    MFloat *pAuxDistortionCoef // [in]  The auxiliary image distortion coefficient,size is float[11]
);
ARCDCIR_API MRESULT ARC_DCIR_CalcDisparityData( // return MOK if success, otherwise fail
    MHandle hHandle,                            // [in]  The algorithm engine
    LPASVLOFFSCREEN pMainImg,   // [in]  The offscreen of main image input. Generally, Left image or
                                // Wide image as Main.
    LPASVLOFFSCREEN pAuxImg,    // [in]  The offscreen of auxiliary image input.
    LPARC_DCIR_PARAM pDCIRParam // [in]  The parameters for algorithm engine
);

ARCDCIR_API MRESULT ARC_DCIR_GetDisparityDataSize( // return MOK if success, otherwise fail
    MHandle hHandle,                               // [in]  The algorithm engine
    MInt32 *pi32Size                               // [out] The size of disparity map
);

ARCDCIR_API MRESULT ARC_DCIR_GetDisparityData( // return MOK if success, otherwise fail
    MHandle hHandle,                           // [in]  The algorithm engine
    MVoid *pDisparityData                      // [out] The data of disparity map
);

ARCDCIR_API MRESULT ARC_DCIR_Process(  // return MOK if success, otherwise fail
    MHandle hHandle,                   // [in]  The algorithm engine
    MVoid *pDisparityData,             // [in]  The data of disparity data
    MInt32 i32DisparityDataSize,       // [in]  The size of disparity data
    LPASVLOFFSCREEN pMainImg,          // [in]  The off-screen of main image
    LPARC_DCIR_REFOCUS_PARAM pRFParam, // [in]  The parameter for refocusing image
    LPASVLOFFSCREEN pDstImg            // [out]  The off-screen of result image
);

ARCDCIR_API MRESULT ARC_DCIR_Reset( // return MOK if success, otherwise fail
    MHandle hHandle                 // [in/out] The algorithm engine will be reset by this API
);

ARCDCIR_API MRESULT ARC_DCIR_SetShineParam( // return MOK if success, otherwise fail
    MHandle hHandle,       // [in/out] The algorithm engine will be reset by this API
    MInt32 i32lShineThres, // [in]  The parameter for bokeh shine threshold,the range [1,5]
    MInt32 i32ShineLevel   // [in]  The parameter for bokeh shine level ,the range [1,255]
);

#define ARC_DCIR_REFOCUS_EFFECT_MODE_BOKEH 0
#define ARC_DCIR_REFOCUS_EFFECT_MODE_ART   1

ARCDCIR_API MRESULT ARC_DCIR_SetRefocusEffectMode( // return MOK if success, otherwise fail
    MHandle hEngine,                               // [in]  The algorithm engine
    MInt32 i32Mode                                 // [in]  The mode of refocus effect
);

ARCDCIR_API MRESULT ARC_DCIR_SetArtBokehMask( // return MOK if success, otherwise fail
    MHandle hHandle,                          // [in]  The algorithm engine
    LPASVLOFFSCREEN pShapeMaskImg // [in]  The mask input��8 bit��gray image ��size 142x142
);
#define ARC_DCIR_REFOCUS_ART_DEGREE_0   0
#define ARC_DCIR_REFOCUS_ART_DEGREE_90  1
#define ARC_DCIR_REFOCUS_ART_DEGREE_180 2
#define ARC_DCIR_REFOCUS_ART_DEGREE_270 3

// Set the art bokeh rotate degree
ARCDCIR_API MRESULT ARC_DCIR_SetArtBokehDegree( // return MOK if success, otherwise fail
    MHandle hHandle,                            // [in]  The algorithm engine
    MInt32 i32Degree                            // [in]  The degree of rotate
);

ARCDCIR_API MRESULT ARC_DCIR_SetNoiseOnOff( // return whether art bokeh can do or not
    MHandle hHandle,                        // [in]  The algorithm engine
    MBool bNoiseOn                          // [in]  Set Noise on or off
);

#define ARC_DCIR_NOISE_LEVEL_1 1
#define ARC_DCIR_NOISE_LEVEL_2 2

ARCDCIR_API MRESULT ARC_DCIR_SetNoiseLevel( // return whether art bokeh can do or not
    MHandle hHandle,                        // [in]  The algorithm engine
    MInt32 i32NoiseLevel                    // [in]  Set ISO of the Image
);

ARCDCIR_API MRESULT ARC_DCIR_SetCurrentISO( // return MOK if success, otherwise fail
    MHandle hHandle,                        // [in] The algorithm engine
    MInt32 i32CurrentISO                    // [in] carrent Image ISO
);

#define ARC_DCIR_REFOCUS_CAMERA_ROLL_0   0
#define ARC_DCIR_REFOCUS_CAMERA_ROLL_90  1
#define ARC_DCIR_REFOCUS_CAMERA_ROLL_180 2
#define ARC_DCIR_REFOCUS_CAMERA_ROLL_270 3

ARCDCIR_API MRESULT ARC_DCIR_SetCameraRoll(MHandle hEngine, // [in]  The algorithm engine
                                           MInt32 i32Degree // [in] Camera roll degree
);
ARCDCIR_API MRESULT ARC_DCIR_SetFullWeakBlur(MHandle hEngine,   // [in]  The algorithm engine
                                             MInt32 i32blurSize // [in] blur[0,3]
);

// Bokeh mode
#define ARC_DCIR_BOKEH_NORMAL 0
#define ARC_DCIR_BOKEH_LITE   1
ARCDCIR_API MRESULT ARC_DCIR_SetBokehCalcMode( // return MOK if success, otherwise fail
    MHandle hHandle,                           // [in] The algorithm engine
    MInt32 i32Mode                             // [in] Bokeh mode
);

#ifdef __cplusplus
}
#endif

#endif /* ARCSOFT_DUALCAM_IMAGE_REFOCUS_H_ */