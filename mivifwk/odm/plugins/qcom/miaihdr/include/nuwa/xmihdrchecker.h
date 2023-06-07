#ifndef _XMI_HDR_CHECKER_H_
#define _XMI_HDR_CHECKER_H_

#include "arcsoft_high_dynamic_range.h"
#include "arcsoft_low_light_hdr.h"
#include "mihdr.h"
#include "xmi_high_dynamic_range.h"

#define XMI_HDR_API ARC_HDR_API

/************************************************************************
 * checker input data
 ************************************************************************/
union HDRCheckerMetaData {
    char _[512];
    struct _metadata
    {
        unsigned int scene_det_type; // scene detection output in asd node, note: the result is only
                                     // available when the scene detection in asd node of some
                                     // project is always on.
    } metadata;
    HDRCheckerMetaData() { metadata.scene_det_type = 0; }
};

/************************************************************************
 * The function is used to detect how many frames to shot, and Ev for each frame.
 * Note: Currently L1 use only
 ************************************************************************/
XMI_HDR_API MRESULT XMI_HDR_Checker_Process(
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
    MInt32 *i32MotionState,                 // [in] The motion detection result
    MInt32 *pi32HdrDetect,                  // [out] checker detect result
    HDRMetaData *hdrMetaData,               // [in/out] The meta data info of normal input image
    LPMIAIHDR_INPUT input,                  // [int] The sensor info
    HDRCheckerMetaData *pHDRCheckerMetaData // [in] The meta data info of hdr-checker
);

/************************************************************************
* The function is used to release checker memory usage.
* ret:   0: release success
        -1: release failed, means checker instance has not been initialized and does not need to be
release.

note: checker is not multi-thread safe, it needs to ensure that to be called by multi-thread
************************************************************************/
int finalizeHDRChecker();

#endif