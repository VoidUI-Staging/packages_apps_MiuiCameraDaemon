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
#ifndef _ARCSOFT_HDR_H_5_0_20201113_
#define _ARCSOFT_HDR_H_5_0_20201113_

#include "amcomdef.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "merror.h"

#ifdef HDRDLL_EXPORTS
#define ARC_HDR_API __declspec(dllexport)
#else
#define ARC_HDR_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 @brief Define the Error codes
 @ARC_HDR_ERR_TOO_MANY_FRAMES Too many frames input
 @ARC_HDR_ERR_INIT_OCL Initial ocl error
 @ARC_HDR_ERR_UNSUPPORT_ISO Un-supported iso
 @ARC_HDR_ERR_INVALID_FACE_INFO Invalid face info
 @ARC_HDR_ERR_INVALID_INPUT_MODE Invalid input mode
 @ARC_HDR_ERR_UNSUPPORT_EV Un-supported ev
 @ARC_HDR_ERR_INVALID_OFFSCREEN Invalid offscreen
 @ARC_HDR_ERR_LARGE_MEAN_DIFF Large mean diff
 @ARC_HDR_ERR_FACE_DETECT FaceDetect engine error
 @ARC_HDR_ERR_UNSUPPORT_FD Unsupported fd for ion
 @ARC_HDR_ERR_INVALID_SCENE_TYPE Invalid scene type
 @ARC_HDR_ERR_INVALID_PARAMS Invalid param
 @ARC_HDR_ERR_SMALL_NL_MEAN_DIFF The mean between EV0 and EV- is too small
 @ARC_HDR_ERR_OVER_MOVE Over move case
*/
#define ARC_HDR_ERR_TOO_MANY_FRAMES    0x0080
#define ARC_HDR_ERR_INIT_OCL           0x0081
#define ARC_HDR_ERR_UNSUPPORT_ISO      0x0082
#define ARC_HDR_ERR_INVALID_FACE_INFO  0x0083
#define ARC_HDR_ERR_INVALID_INPUT_MODE 0x0084
#define ARC_HDR_ERR_UNSUPPORT_EV       0x0085
#define ARC_HDR_ERR_INVALID_OFFSCREEN  0x0086
#define ARC_HDR_ERR_LARGE_MEAN_DIFF    0x0087
#define ARC_HDR_ERR_FACE_DETECT        0x0088
#define ARC_HDR_ERR_UNSUPPORT_FD       0x0089
#define ARC_HDR_ERR_INVALID_SCENE_TYPE 0x0090
#define ARC_HDR_ERR_INVALID_PARAMS     0x0093
#define ARC_HDR_ERR_SMALL_NL_MEAN_DIFF 0x0095
#define ARC_HDR_ERR_OVER_MOVE          0x1001

/**
 @brief This define is used for camera type
 @ARC_HDR_CAMERA_WIDE The rear wide camera
 @ARC_HDR_CAMERA_FRONT The front camera
*/

#define ARC_HDR_CAMERA_WIDE      0x0
#define ARC_HDR_CAMERA_FRONT     0x200
#define ARC_HDR_CAMERA_ULTRAWIDE 0x500

/**
 @brief This define is used to descripe the input EV sequence
 @ARC_HDR_INPUT_MODE_NLOSN normal low over special-normal(EV0, EV-2, EV+1, EV0:a brightened version
 of EV-2), only for rear camera mode
 @ARC_HDR_INPUT_MODE_NLLSN2 normal low low special-normal(EV0, EV-2, EV-3, EV0:a brightened version
 of EV-3), only for front camera mode
*/
#define ARC_HDR_INPUT_MODE_NLOSN  0x0003
#define ARC_HDR_INPUT_MODE_NLLSN2 0x000A

#define ARC_HDR_MAX_INPUT_IMAGE_NUM 4

/**
 @brief This struct is used to descripe input info
 @param i32ImgNum[in] The number for captured image
 @param InputImages[in] The input images array
 @param i32SrcImgFd[in] The FD of Src Images, 0: use cpu buffer, else : use ION buffer
 @param fEvCompVal[in] The exposure compensation of input images, such as,
                       the value of normal exposure image is 0,
                       the value of low exposure image may be -2.0 or -3.0, etc.
*/
typedef struct _tag_ARC_HDR_INPUTINFO
{
    MInt32 i32ImgNum;
    ASVLOFFSCREEN InputImages[ARC_HDR_MAX_INPUT_IMAGE_NUM];
    MInt32 i32SrcImgFd[ARC_HDR_MAX_INPUT_IMAGE_NUM];
    MFloat fEvCompVal[ARC_HDR_MAX_INPUT_IMAGE_NUM];
} ARC_HDR_INPUTINFO, *LPARC_HDR_INPUTINFO;

/**
 @brief This struct is used to descripe face info
 @param nFace[in] The number of faces detected
 @param rcFace[in] The bounding box of face
 @param lFaceOrient[in] The angle of face, 1:0 degree,2:90 degree,3:270 degree,4:180 degree
*/
typedef struct _tag_ARC_HDR_FACEINFO
{
    MInt32 nFace;
    MRECT *rcFace;
    MInt32 *lFaceOrient;
} ARC_HDR_FACEINFO, *LPARC_HDR_FACEINFO;

/**
 @brief This struct is used to describe HDR parameters
 @param i32ToneLength[in] Adjustable parameter for tone mapping, the range is 0~100
 @param i32Brightness[in] Adjustable parameter for brightness, the range is -100~+100
 @param i32Saturation[in] Adjustable parameter for saturation, the range is 0~+100
 @param i32Contrast[in] Adjustable parameter for contrast, the range is -100~+100
*/
typedef struct _tag_ARC_HDR_Param
{
    MInt32 i32ToneLength;
    MInt32 i32Brightness;
    MInt32 i32Saturation;
    MInt32 i32Contrast;
} ARC_HDR_PARAM, *LPARC_HDR_PARAM;

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
typedef MRESULT (*ARC_HDR_FNPROGRESS)(MInt32 i32Progress, MInt32 i32Status, MVoid *pParam);

/**
 @brief This function is used to get version information of library
 @return MPBASE_Version
*/
ARC_HDR_API const MPBASE_Version *ARC_HDR_GetVersion();

/************************************************************************
 * The function is used to init  engine
 ************************************************************************/
// Sample code:
// FN_ARC_HDR_android_log_print fn_log_print = __android_log_print;
// ARC_HDR_SetLogLevel(
//        ARC_HDR_LOGLEVEL_1,
//        fn_log_print
//        );
// Notes: Use ARC_HDR_SetLogLevel() to set fn_log_print is not thread-safe because performance.
//       So please don't call it frequently, and suggest to call it before SDK created.
//       fn_log_print will be set to global value, so please keep the life longer than SDK's.
//       You can set NULL to clean it.
typedef int (*FN_ARC_HDR_android_log_print)(int prio, const char *tag, const char *fmt, ...);
#define ARC_HDR_LOGLEVEL_0 0
#define ARC_HDR_LOGLEVEL_1 1
#define ARC_HDR_LOGLEVEL_2 2
ARC_HDR_API MVoid ARC_HDR_SetLogLevel(
    // [in] ARC_HDR_LOGLEVEL_0 is disable log and ARC_HDR_LOGLEVEL_2 is enable all log.
    MInt32 i32LogLevel, FN_ARC_HDR_android_log_print fn_log_print = MNull);
#define ARC_HDR_VendorTag_bypass "persist.vendor.camera.arcsoft.hdr.bypass"
#define ARC_HDR_VendorTag_dump   "persist.vendor.camera.arcsoft.hdr.dump"

/**
 @brief The function is used to init HDR engine, only initialize HDR, and not support
 log/bypass/dump functions when call it.
 @param phHandle[in/out] The algorithm engine will be initialized by this API
 @return MOK if success, otherwise fail
*/
ARC_HDR_API MRESULT ARC_HDR_Init(MHandle *phHandle);

/**
 @brief The function is used to uninit HDR engine
 @param phHandle[in/out] The algorithm engine will be un-initialized by this API
 @return return MOK if success, otherwise fail
*/
ARC_HDR_API MRESULT ARC_HDR_Uninit(MHandle *phHandle);

/**
 @brief The function is used to set reference frame
 @param hHandle[in] The algorithm engine
 @param i32RefIndex[in] The index of reference frame(ev0)
 @return return MOK if success, otherwise fail
*/
ARC_HDR_API MRESULT ARC_HDR_SetRefIndex(MHandle hHandle, MInt32 i32RefIndex);

/**
 @brief The function is used to set scene type before ARC_HDR_GetDefaultParam
 @param hHandle[in] The algorithm engine
 @param i32SceneType[in] The detected scene type, default is 0
 @return return MOK if success, otherwise fail
*/
ARC_HDR_API MRESULT ARC_HDR_SetSceneType(MHandle hHandle, MInt32 i32SceneType);

/**
 @brief The function is used to get default param
 @param hHandle[in] The algorithm engine
 @param pParam[in/out] The default param for sdk
 @param inputMode[in] The input mode of input images
 @param isoValue[in] The iso of normal image
 @param cameraType[in] The Camera type, ARC_HDR_CAMERA_WIDE and ARC_HDR_CAMERA_FRONT
 @return MOK if success, otherwise fail
*/
ARC_HDR_API MRESULT ARC_HDR_GetDefaultParam(MHandle hHandle, LPARC_HDR_PARAM pParam,
                                            MInt32 inputMode, MInt32 isoValue, MInt32 cameraType);

/**
 @brief The function is used to set sdk param before ARC_HDR_PreProcess
 @param hHandle[in] The algorithm engine
 @param pParam[in] The param for sdk
 @return return MOK if success, otherwise fail
*/
ARC_HDR_API MRESULT ARC_HDR_SetParam(MHandle hHandle, LPARC_HDR_PARAM pParam);

/**
 @brief The function is used to pre-process input image
 @param hHandle[in] The algorithm engine
 @param pInputImages[in] The input images array
 @param i32InputIndex[in] The index of input image in the input image array
 @return return MOK if success, otherwise fail
*/
ARC_HDR_API MRESULT ARC_HDR_PreProcess(MHandle hHandle, LPARC_HDR_INPUTINFO pInputImages,
                                       MInt32 i32InputIndex);

/**
 @brief The function is used to process input image
 @param hHandle[in] The algorithm engine
 @param pFaceInfo[in] The face info of input normal image(ev0)
 @param pDstImg[out] The offscreen of result image
 @param i32DstImgFD[in] The FD of dest Images, 0: use cpu buffer, else: use ION buffer
 @param inputMode[in] The input mode of input images, e.g. ARC_HDR_INPUT_MODE_NLOSN
 @param isoValue[in] The iso of normal image
 @param cameraType[in] The Camera type, ARC_HDR_CAMERA_WIDE and ARC_HDR_CAMERA_FRONT
 @return MOK if success, otherwise fail
*/
ARC_HDR_API MRESULT ARC_HDR_Process(MHandle hHandle, LPARC_HDR_FACEINFO pFaceInfo,
                                    LPASVLOFFSCREEN pDstImg, MInt32 i32DstImgFD, MInt32 inputMode,
                                    MInt32 isoValue, MInt32 cameraType);

/**
 @brief The function is used to set call-back
 @param hHandle[in] The algorithm engine
 @param fnCallback[in] The callback function
 @param pUserData[in/out] Caller-defined data
 @return MVoid
*/
ARC_HDR_API MVoid ARC_HDR_SetCallback(MHandle hHandle, ARC_HDR_FNPROGRESS fnCallback,
                                      MVoid *pUserData);

#ifdef __cplusplus
}
#endif

#endif // _ARCSOFT_HDR_H_5_0_20201113_
