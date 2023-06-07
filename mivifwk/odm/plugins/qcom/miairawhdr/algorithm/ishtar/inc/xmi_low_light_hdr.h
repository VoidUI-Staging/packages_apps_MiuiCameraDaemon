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
#ifndef _XMI_LOW_LIGHT_HDR_H_
#define _XMI_LOW_LIGHT_HDR_H_

#include "arcsoft_low_light_hdr.h"
#include "mihdr.h"

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
#define XMI_LLHDR_FNPROGRESS ARC_LLHDR_FNPROGRESS

/************************************************************************
 * This function is used to get version information of library
 ************************************************************************/
LOWLIGHTHDR_API const MPBASE_Version *XMI_LLHDR_GetVersion();

/************************************************************************
 * This function is used to get default parameters of library
 ************************************************************************/
#define XMI_LLHDR_PARAM   ARC_LLHDR_PARAM
#define LPXMI_LLHDR_PARAM LPARC_LLHDR_PARAM

// the camera type
#define XMI_LLHDR_CAMERA_REAR  ARC_LLHDR_CAMERA_REAR
#define XMI_LLHDR_CAMERA_FRONT ARC_LLHDR_CAMERA_REAR

// [in]  cameratype ,ARC_LLHDR_CAMERA_REAR and ARC_LLHDR_CAMERA_FRONT
LOWLIGHTHDR_API MRESULT XMI_LLHDR_GetDefaultParam(LPXMI_LLHDR_PARAM pParam, MInt32 isoValue,
                                                  MInt32 cameratype);

/************************************************************************
 * The functions is used to perform image night hdr
 ************************************************************************/
#define XMI_LLHDR_MAX_INPUT_IMAGE_NUM ARC_LLHDR_MAX_INPUT_IMAGE_NUM

#define XMI_LLHDR_INPUTINFO   ARC_LLHDR_INPUTINFO
#define LPXMI_LLHDR_INPUTINFO LPARC_LLHDR_INPUTINFO

#define XMI_LLHDR_FACEINFO   ARC_LLHDR_FACEINFO
#define LPXMI_LLHDR_FACEINFO LPARC_LLHDR_FACEINFO

LOWLIGHTHDR_API MRESULT XMI_LLHDR_Init( // return MOK if success, otherwise fail
    MHandle *phHandle // [out] The algorithm engine will be initialized by this API
);

LOWLIGHTHDR_API MRESULT XMI_LLHDR_Uninit( // return MOK if success, otherwise fail
    MHandle *phHandle // [in/out] The algorithm engine will be un-initialized by this API
);

LOWLIGHTHDR_API MRESULT XMI_LLHDR_Process( // return MOK if success, otherwise fail
    MHandle hHandle,                       // [in]  The algorithm engine
    LPXMI_LLHDR_INPUTINFO pSrcImgs,        // [in]  The offscreen of source images
    LPXMI_LLHDR_FACEINFO pFaceInfo,        // [in]  The input face info
    LPASVLOFFSCREEN pDstImg,               // [out] The offscreen of result image,this can be NULL
    MInt32 *pRetIndexInSrcImgs, // [out] if the pDstImg == NULL,the result will save in the
                                // pSrcImgs->pImages[*pRetIndexInSrcImgs]
    LPXMI_LLHDR_PARAM pParam,   // [in]  The parameters for algorithm engine
    MInt32 isoValue,            // [in]  The Value of ISO
    HDRMetaData *hdrMetaData,   // [in/out] The meta data info of normal input image
    callback_fun fun, //[in] The callback function,for algo flush,only for 765G and 865 now,set null
                      // if not use.
    MVoid *pCallBackObj, //[in] The callback function,for memory read check,only for 765G and 865
                         // now,set null if not use.
    std::vector<std::pair<std::string, int>>
        *mapEV //[int] The evMap info. if not define USE_EV_MAP， set an empty vector
);

LOWLIGHTHDR_API MVoid XMI_LLHDR_SetDRCGain(
    MHandle hEnhancer, // [in]  The algorithm engine
    MFloat drcGainVal  // [in]  The parameter of DRC gain Value
);

LOWLIGHTHDR_API MVoid XMI_LLHDR_SetISPGain(
    MHandle hEnhancer, // [in]  The algorithm engine
    MFloat ispGainVal  // [in]  The parameter of ISP gain Value
);

LOWLIGHTHDR_API MVoid XMI_LLHDR_SetEVValue(MHandle hEnhancer, // [in]  The algorithm engine
                                           MFloat EvValue     // [in] The parameter of EV Value
);

LOWLIGHTHDR_API MVoid
XMI_LLHDR_SetCallback(MHandle hHandle,                 // [in]  The algorithm engine
                      XMI_LLHDR_FNPROGRESS fnCallback, // [in]  The callback function
                      MVoid *pUserData);

LOWLIGHTHDR_API MRESULT XMI_REDEYE_REDUCTION( // return MOK if success, otherwise fail
    MHandle hHandle,                          // [in]  The algorithm engine
    LPASVLOFFSCREEN pSrcImg,                  // [in]  The offscreen of source image
    LPEYES_LANDMARK_INFO pEyeslandInfo,       // [in]  The input eyeland info
    MInt32 orientation,     // [in] reservation tag, The orientation of source image
    LPASVLOFFSCREEN pDstImg // [out] The offscreen of result image, this can be NULL
);

#ifdef __cplusplus
}
#endif

#endif // _XMI_LOWLIGHTLLHDR_H_
