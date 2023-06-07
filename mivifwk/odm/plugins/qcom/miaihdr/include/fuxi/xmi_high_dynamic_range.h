/*******************************************************************************
Copyright(c) Xiaomi, All right reserved.

This file is Xiaomi's property. It contains Xiaomi's trade secret, proprietary
and confidential information.

The information and code contained in this file is only for authorized Xiaomi
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy,
distribute, modify, or take any action in reliance on it.

If you have received this file in error, please immediately notify Xiaomi and
permanently delete the original and any copy of any file and any printout
thereof.
*******************************************************************************/
#ifndef _Xiaomi_HDR_H_
#define _Xiaomi_HDR_H_

#include "amcomdef.h"
#include "ammem.h"
#include "arcsoft_high_dynamic_range.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "mihdr.h"
#define XMI_HDR_API                          ARC_HDR_API
#define XMI_MAX_HDRD_FACE_NUM                10
#define XMI_MAX_EV_NUM                       10
#define XMI_MAX_MFNR_SEQ                     12
#define XMI_MAX_CHECKER_EXTEND_EXIF_INFO_LEN 256
#define XMI_MAX_CHECKER_EXIF_INFO_LEN        128
#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
 * This struct is used to descripe face info
 * nFace The number of faces detected
 * rcFace The bounding box of face
 ************************************************************************/
typedef struct _tag_XMI_HDRD_FACEINFO
{
    MInt32 nFace;
    MRECT rcFace[XMI_MAX_HDRD_FACE_NUM];
} XMI_HDRD_FACEINFO, *LPXMI_HDRD_FACEINFO;

typedef struct _tag_XMI_HDR_DSPINFO
{
    MInt32 ev_list[XMI_MAX_EV_NUM];
} XMI_HDR_DSPINFO, *LPXMI_HDR_DSPINFO;

//#ifdef OFFSCREEN_EVLIST_INTERFACE
typedef struct __tag_HDR_YUV_OFFSCREEN
{
    MUInt32 u32PixelArrayFormat;
    MInt32 i32Width;
    MInt32 i32Height;
    MUInt8 *ppu8Plane[4];
    MInt32 pi32Pitch[4];
    MInt32 fd[4];
} HDR_YUV_OFFSCREEN, *LPHDR_YUV_OFFSCREEN;
typedef struct _tag_HDR_YUV_INPUTINFO
{
    MInt32 i32ImgNum;
    HDR_YUV_OFFSCREEN InputImages[ARC_HDR_MAX_INPUT_IMAGE_NUM];
} HDR_YUV_INPUTINFO, *LPHDR_YUV_INPUTINFO;
//#endif
typedef struct _tag_XMI_LLHDR_AEINFO
{
    MFloat fLuxIndex;      // useful	 , AEC param, luxindex, 		use to hdr trigger &&
                           // dynamic ev
    MFloat fISPGain;       // useful	 , AEC param, isp gain, 		use to hdr trigger
    MFloat fSensorGain;    // deprecated, AEC param, sensor gain, 		use to llhdr trigger
    MFloat fADRCGain;      // useful	 , AEC param, real adrc gain, 	use to hdr trigger && dynamic ev
    MFloat fADRCGainMax;   // deprecated, AEC param, max adrc gain,
    MFloat fADRCGainMin;   // deprecated, AEC param, min adrc gain,
    MFloat fADRCGainClamp; // useful	 , AEC param, adrc gain,		 use to dynamic ev
    MFloat fDarkGainClamp; // useful	 , AEC param, darkboost gain,	 use to dynamic ev
    MFloat fDarkBoostGain; // useful	 , AEC param, real dkboost gain, use to dynamic ev
    MFloat calculatedCDF0; // deprecated, AEC param, 0-bin value in bhist,
    MFloat calculatedCDF5; // deprecated, AEC param, 0~5-bin value in bhist,
    MFloat apture;         // deprecated, AEC param,
    MFloat exposure;       // useful	 , AEC param, shutter,			use to llhdr trigger
    int iso;               // useful	 , AEC param, iso,			    use to hdr trigger (just for
                           // india version)
    int exp_compensation;  // deprecated, AEC param, exposure compensation,
    int ae_state;          // useful	 , AEC param, current ae converge state,	use to hdr trigger
    HISTOGRAM_INFO histogram_info; // deprecated,
    MAGNET_INFO magnet_info;       // useful	 , mobile phone param, magnet angle,	use to hdr trigger
                                   // and magnet lock
    EXPOSURE_INFO exposure_info;   // deprecated,
    HDRBHIST_INFO histogram_HDRBhist; // useful	 , AEC param, hdrbhist, use to dynamic ev
    XMI_HDRD_FACEINFO
    faceInfo;  // useful	 , AEC param, face detect result,use to hdr trigger && dynamic ev
    int ui_ae; // useful	 , camera param, ui ev value,	 use to uiev logic
    int mfnr_req[XMI_MAX_MFNR_SEQ];   // useful, algo output param, mfnr request list, determine
                                      // whether to do mfnr for corresponding frame in ev list
    MIAI_HDR_ALGO_PARAMS algo_params; // useful, algo output param, hdr algo exif info
    char checker_exif_info[XMI_MAX_CHECKER_EXIF_INFO_LEN]; // useful, algo output param, checker
                                                           // algo exif info, maximum length is 128
    char checker_exif_info_256[XMI_MAX_CHECKER_EXTEND_EXIF_INFO_LEN]; // useful, algo output param,
                                                                      // checker algo exif info,
                                                                      // maximum length is 256
} XMI_LLHDR_AEINFO, *LPXMI_LLHDR_AEINFO;

#define XMI_MODE_REAR_CAMERA  ARC_MODE_REAR_CAMERA
#define XMI_MODE_FRONT_CAMERA ARC_MODE_FRONT_CAMERA

#define XMI_TYPE_QUALITY_FIRST     ARC_TYPE_QUALITY_FIRST
#define XMI_TYPE_PERFORMANCE_FIRST ARC_TYPE_PERFORMANCE_FIRST

#define XMI_HDR_MAX_INPUT_IMAGE_NUM ARC_HDR_MAX_INPUT_IMAGE_NUM

#define XMI_HDR_INPUTINFO   ARC_HDR_INPUTINFO
#define LPXMI_HDR_INPUTINFO LPARC_HDR_INPUTINFO

#define XMI_HDR_AEINFO   ARC_HDR_AEINFO
#define LPXMI_HDR_AEINFO LPARC_HDR_AEINFO

#define XMI_HDR_USERDATA   ARC_HDR_USERDATA
#define LPXMI_HDR_USERDATA LPARC_HDR_USERDATA

/************************************************************************
 * This function is implemented by the caller, registered with
 * any time-consuming processing functions, and will be called
 * periodically during processing so the caller application can
 * obtain the operation status (i.e., to draw a progress bar),
 * as well as determine whether the operation should be canceled or not
 ************************************************************************/
#define XMI_HDR_FNPROGRESS ARC_HDR_FNPROGRESS

/************************************************************************
 * This function is used to get version information of library
 ************************************************************************/
XMI_HDR_API const MPBASE_Version *XMI_HDR_GetVersion();

/************************************************************************
 * This struct is used to descripe HDR parameters
 ************************************************************************/

#define XMI_HDR_PARAM   ARC_HDR_PARAM
#define LPXMI_HDR_PARAM LPARC_HDR_PARAM

/************************************************************************
 * The function is used to init HDR engine
 * i32Mode: XMI_MODE_REAR_CAMERA for rear camera, XMI_MODE_FRONT_CAMERA
 * for front camera
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDR_Init( // return MOK if success, otherwise fail
    MHandle *phHandle,            // [out] The algorithm engine will be initialized by this API
    MInt32 i32Mode,               // [in] The model name for camera
    MInt32 i32Type // [in] The HDR type, XMI_TYPE_QUALITY_FIRST(0), XMI_TYPE_PERFORMANCE_FIRST(1)
);

/************************************************************************
 * The function is used to uninit HDR engine
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDR_Uninit( // return MOK if success, otherwise fail
    MHandle *phHandle // [in/out] The algorithm engine will be un-initialized by this API
);

/************************************************************************
 * The function is used to pre-process input image
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDR_PreProcess( // return MOK if success, otherwise fail
    MHandle hHandle,                    // [in]  The algorithm engine
    LPHDR_YUV_INPUTINFO pInputImages,   // [in]  The input images array
    LPHDR_EVLIST_AEC_INFO pEVAEInfo,    // [in]  The AE info of normal input image
    MInt32 i32InputImageIndex // [in]  The index of input image in the input image array, the last
                              // image must be high-exposure
);

/************************************************************************
 * The function is used to process input image
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDR_Process(  // return MOK if success, otherwise fail
    MHandle hHandle,                  // [in]  The algorithm engine
    LPHDR_YUV_INPUTINFO pInputImages, // [in]  The input images array
    LPHDR_EVLIST_AEC_INFO pEVAEInfo,
    LPXMI_HDR_PARAM pParam,      // [in]  The parameters for algorithm engine
    LPHDR_YUV_OFFSCREEN pDstImg, // [out]  The offscreen of result image
    HDRMetaData *hdrMetaData,    // [in/out] The meta data info of normal input image
    callback_fun fun, //[in] The callback function,for algo flush,only for 765G and 865 now,set null
                      // if not use.
    MVoid *pCallBackObj, //[in] The callback function,for memory read check,only for 765G and 865
                         // now,set null if not use.
    std::vector<std::pair<std::string, int>>
        *mapEV //[int] The evMap info. if not define USE_EV_MAP， set an empty vector

);

/************************************************************************
 * The function is used to detect how many frames to shot, and Ev for each frame.
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDR_ParameterDetect(
    LPASVLOFFSCREEN pPreviewImg, // [in] The input Preview data same ev as normal
    LPXMI_LLHDR_AEINFO pAEInfo,  // [in] The AE info of input image
    MInt32 i32Mode, // [in] The model name for camera, 0 for rear camera, 0x100 for front camera
    MInt32 i32Type, // [in] The HDR type, XMI_TYPE_QUALITY_FIRST(0), XMI_TYPE_PERFORMANCE_FIRST(1)
    MInt32 i32Orientation, // [in] This parameter is the clockwise rotate orientation of the input
                           // image. The accepted value is {0, 90, 180, 270}.
    MInt32 i32MinEV,       // [in] The min ev value, it is < 0
    MInt32 i32MaxEV,       // [in] The max ev value, it is > 0
    MInt32 i32EVStep,      // [in] The step ev value, it is >= 1
    MInt32 *pi32ShotNum,   // [out] The number of shot image, it must be 1, 2 or 3.
    MInt32 *pi32EVArray,   // [out] The ev value for each image
    MFloat *fConVal, // [out] The confidence array of the back light, fConVal[0] for HDR, fConVal[1]
                     // for LLHDR.
    MInt32 *i32MotionState,   // [in] The motion detection result
    MInt32 *pi32HdrDetect,    // [out] checker detect result
    HDRMetaData *hdrMetaData, // [in/out] The meta data info of normal input image
    LPMIAIHDR_INPUT input     // [int] The sensor info
);

/************************************************************************
 * The function is used to set call-back
 ************************************************************************/
XMI_HDR_API MVoid XMI_HDR_SetCallback(MHandle hHandle,               // [in]  The algorithm engine
                                      XMI_HDR_FNPROGRESS fnCallback, // [in]  The callback function
                                      MVoid *pUserData               // [in]  Caller-defined data
);

XMI_HDR_API MRESULT XMI_HDR_GetDefaultParam(LPXMI_HDR_PARAM pParam);

XMI_HDR_API const MPBASE_Version *XMI_HDRSR_GetVersion();

/********************************************************************
 * FUNCTIONS FOR HDR-SR *
 *********************************************************************/

/************************************************************************
 * The function is used to init HDR-SR engine
 * i32Mode: XMI_MODE_REAR_CAMERA for rear camera, XMI_MODE_FRONT_CAMERA
 * for front camera
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDRSR_Init( // return MOK if success, otherwise fail
    MHandle *phHandle,              // [out] The algorithm engine will be initialized by this API
    MInt32 i32Mode,                 // [in] The model name for camera
    MInt32 i32Type // [in] The HDR type, XMI_TYPE_QUALITY_FIRST(0), XMI_TYPE_PERFORMANCE_FIRST(1)
);

/************************************************************************
 * The function is used to uninit HDR-SR engine
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDRSR_Uninit( // return MOK if success, otherwise fail
    MHandle *phHandle // [in/out] The algorithm engine will be un-initialized by this API
);

/************************************************************************
 * The function is used to pre-process input image
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDRSR_PreProcess( // return MOK if success, otherwise fail
    MHandle hHandle,                      // [in]  The algorithm engine
    LPHDR_YUV_INPUTINFO pInputImages,     // [in]  The input images array
    LPHDR_EVLIST_AEC_INFO pEVAEInfo,      // [in]  The AE info of normal input image
    MInt32 i32InputImageIndex // [in]  The index of input image in the input image array, the last
                              // image must be high-exposure
);

/************************************************************************
 * The function is used to process input image
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDRSR_Process( // return MOK if success, otherwise fail
    MHandle hHandle,                   // [in]  The algorithm engine
    LPHDR_YUV_INPUTINFO pInputImages,  // [in]  The input images array
    LPHDR_EVLIST_AEC_INFO pEVAEInfo,
    LPXMI_HDR_PARAM pParam,      // [in]  The parameters for algorithm engine
    LPHDR_YUV_OFFSCREEN pDstImg, // [out]  The offscreen of result image
    HDRMetaData *hdrMetaData,    // [in/out] The meta data info of normal input image
    callback_fun fun, //[in] The callback function,for algo flush,only for 765G and 865 now,set null
                      // if not use.
    MVoid *pCallBackObj, //[in] The callback function,for memory read check,only for 765G and 865
                         // now,set null if not use.
    std::vector<std::pair<std::string, int>>
        *mapEV //[int] The evMap info. if not define USE_EV_MAP， set an empty vector

);

/************************************************************************
 * The function is used to detect how many frames to shot, and Ev for each frame.
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDRSR_ParameterDetect(
    LPASVLOFFSCREEN pPreviewImg, // [in] The input Preview data same ev as normal
    LPXMI_LLHDR_AEINFO pAEInfo,  // [in] The AE info of input image
    MInt32 i32Mode, // [in] The model name for camera, 0 for rear camera, 0x100 for front camera
    MInt32 i32Type, // [in] The HDR type, XMI_TYPE_QUALITY_FIRST(0), XMI_TYPE_PERFORMANCE_FIRST(1)
    MInt32 i32Orientation, // [in] This parameter is the clockwise rotate orientation of the input
                           // image. The accepted value is {0, 90, 180, 270}.
    MInt32 i32MinEV,       // [in] The min ev value, it is < 0
    MInt32 i32MaxEV,       // [in] The max ev value, it is > 0
    MInt32 i32EVStep,      // [in] The step ev value, it is >= 1
    MInt32 *pi32ShotNum,   // [out] The number of shot image, it must be 1, 2 or 3.
    MInt32 *pi32EVArray,   // [out] The ev value for each image
    MFloat *fConVal, // [out] The confidence array of the back light, fConVal[0] for HDR, fConVal[1]
                     // for LLHDR.
    MInt32 *i32MotionState,   // [in] The motion detection result
    MInt32 *pi32HdrDetect,    // [out] checker detect result
    HDRMetaData *hdrMetaData, // [in/out] The meta data info of normal input image
    LPMIAIHDR_INPUT input     // [int] The sensor info
);

/************************************************************************
 * The function is used to set call-back
 ************************************************************************/
XMI_HDR_API MVoid XMI_HDRSR_SetCallback(
    MHandle hHandle,               // [in]  The algorithm engine
    XMI_HDR_FNPROGRESS fnCallback, // [in]  The callback function
    MVoid *pUserData               // [in]  Caller-defined data
);

XMI_HDR_API MRESULT XMI_HDRSR_GetDefaultParam(LPXMI_HDR_PARAM pParam);

/********************************************************************
 * FUNCTIONS FOR HDR-BOKEH *
 *********************************************************************/

/************************************************************************
 * The function is used to init HDR-BOKEH engine
 * i32Mode: XMI_MODE_REAR_CAMERA for rear camera, XMI_MODE_FRONT_CAMERA
 * for front camera
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDRBOKEH_Init( // return MOK if success, otherwise fail
    MHandle *phHandle,                 // [out] The algorithm engine will be initialized by this API
    MInt32 i32Mode,                    // [in] The model name for camera
    MInt32 i32Type // [in] The HDR type, XMI_TYPE_QUALITY_FIRST(0), XMI_TYPE_PERFORMANCE_FIRST(1)
);

/************************************************************************
 * The function is used to uninit HDR-BOKEH engine
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDRBOKEH_Uninit( // return MOK if success, otherwise fail
    MHandle *phHandle // [in/out] The algorithm engine will be un-initialized by this API
);

/************************************************************************
 * The function is used to pre-process input image
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDRBOKEH_PreProcess( // return MOK if success, otherwise fail
    MHandle hHandle,                         // [in]  The algorithm engine
    LPHDR_YUV_INPUTINFO pInputImages,        // [in]  The input images array
    LPHDR_EVLIST_AEC_INFO pEVAEInfo,         // [in]  The AE info of normal input image
    MInt32 i32InputImageIndex // [in]  The index of input image in the input image array, the last
                              // image must be high-exposure
);

/************************************************************************
 * The function is used to process input image
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDRBOKEH_Process( // return MOK if success, otherwise fail
    MHandle hHandle,                      // [in]  The algorithm engine
    LPHDR_YUV_INPUTINFO pInputImages,     // [in]  The input images array
    LPHDR_EVLIST_AEC_INFO pEVAEInfo,
    LPXMI_HDR_PARAM pParam,      // [in]  The parameters for algorithm engine
    LPHDR_YUV_OFFSCREEN pDstImg, // [out]  The offscreen of result image
    HDRMetaData *hdrMetaData,    // [in/out] The meta data info of normal input image
    callback_fun fun, //[in] The callback function,for algo flush,only for 765G and 865 now,set null
                      // if not use.
    MVoid *pCallBackObj, //[in] The callback function,for memory read check,only for 765G and 865
                         // now,set null if not use.
    std::vector<std::pair<std::string, int>>
        *mapEV //[int] The evMap info. if not define USE_EV_MAP， set an empty vector

);

/************************************************************************
 * The function is used to detect how many frames to shot, and Ev for each frame.
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDRBOKEH_ParameterDetect(
    LPASVLOFFSCREEN pPreviewImg, // [in] The input Preview data same ev as normal
    LPXMI_LLHDR_AEINFO pAEInfo,  // [in] The AE info of input image
    MInt32 i32Mode, // [in] The model name for camera, 0 for rear camera, 0x100 for front camera
    MInt32 i32Type, // [in] The HDR type, XMI_TYPE_QUALITY_FIRST(0), XMI_TYPE_PERFORMANCE_FIRST(1)
    MInt32 i32Orientation, // [in] This parameter is the clockwise rotate orientation of the input
                           // image. The accepted value is {0, 90, 180, 270}.
    MInt32 i32MinEV,       // [in] The min ev value, it is < 0
    MInt32 i32MaxEV,       // [in] The max ev value, it is > 0
    MInt32 i32EVStep,      // [in] The step ev value, it is >= 1
    MInt32 *pi32ShotNum,   // [out] The number of shot image, it must be 1, 2 or 3.
    MInt32 *pi32EVArray,   // [out] The ev value for each image
    MFloat *fConVal, // [out] The confidence array of the back light, fConVal[0] for HDR, fConVal[1]
                     // for LLHDR.
    MInt32 *i32MotionState,   // [in] The motion detection result
    MInt32 *pi32HdrDetect,    // [out] checker detect result
    HDRMetaData *hdrMetaData, // [in/out] The meta data info of normal input image
    LPMIAIHDR_INPUT input     // [int] The sensor info
);

/************************************************************************
 * The function is used to set call-back
 ************************************************************************/
XMI_HDR_API MVoid XMI_HDRBOKEH_SetCallback(
    MHandle hHandle,               // [in]  The algorithm engine
    XMI_HDR_FNPROGRESS fnCallback, // [in]  The callback function
    MVoid *pUserData               // [in]  Caller-defined data
);

XMI_HDR_API MRESULT XMI_HDRBOKEH_GetDefaultParam(LPXMI_HDR_PARAM pParam);

XMI_HDR_API const MPBASE_Version *XMI_HDRBOKEH_GetVersion();

#ifdef __cplusplus
}
#endif

#endif // _Xiaomi_HDR_H_
