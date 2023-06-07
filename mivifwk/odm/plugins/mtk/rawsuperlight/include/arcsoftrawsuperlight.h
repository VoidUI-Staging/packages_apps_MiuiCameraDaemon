#ifndef _MIA_ARCSOFTRAWSUPERLIGHT_H_
#define _MIA_ARCSOFTRAWSUPERLIGHT_H_

#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>

#include "arcsoft_super_night_multi_raw.h"

#define FRAME_MIN_INPUT_NUMBER        4
#define FRAME_DEFAULT_INPUT_NUMBER    8
#define FRAME_DEFAULT_EXPOSURE_STRING "-18,-12,-6,0,6,6,6,6"
#define FORMAT_YUV_420_NV21           0x01
#define FORMAT_YUV_420_NV12           0x02

#define HARDCODE_CAMERAID_REAR  0
#define HARDCODE_CAMERAID_FRONT 0x100

#define USE_INTERNAL_BUFFER        0
#define INTERNAL_BUFFER_PIXEL_SIZE 4132 * 3124 * 2

#define FLAG_FILL_ALL      0
#define FLAG_FILL_METADATA 1

#define ARCSOFT_MAX_FACE_NUMBER 8

// camera state
#define ARC_SN_CAMERA_STATE_UNKNOWN  0
#define ARC_SN_CAMERA_STATE_TRIPOD   1
#define ARC_SN_CAMERA_STATE_HAND     2
#define ARC_SN_CAMERA_STATE_AUTOMODE 3

// the METADATA_VALUE_NUMBER_MAX & SuperNightRawValue are defined on qcom platform
// transplant to MTK platform by wubin6 2020.5.29
#define METADATA_VALUE_NUMBER_MAX 16

typedef struct
{
    MUInt32 validFrameCount;
    MInt32 Ev[METADATA_VALUE_NUMBER_MAX];
    MFloat ISPGain[METADATA_VALUE_NUMBER_MAX];
    MFloat SensorGain[METADATA_VALUE_NUMBER_MAX];
    MFloat Shutter[METADATA_VALUE_NUMBER_MAX];
} SuperNightRawValue;
// end transplanting

using namespace mialgo2;

class ArcsoftRawSuperlight
{
public:
    ArcsoftRawSuperlight();
    virtual ~ArcsoftRawSuperlight();
    MRESULT init(MInt32 camMode, MInt32 camState, MInt32 i32Width, MInt32 i32Height,
                 MPBASE_Version *buildVer, MInt32 AlgoLogLevel, MRECT *pZoomRect,
                 MInt32 i32LuxIndex);
    MRESULT process(MInt32 *input_fd, ASVLOFFSCREEN *input, MInt32 *output_fd,
                    ASVLOFFSCREEN *output, MRECT *pOutImgRect, MInt32 *pExposure, MUInt32 input_num,
                    MInt32 sceneMode, MInt32 camState, ARC_SN_RAWINFO *info,
                    ARC_SN_FACEINFO *faceinfo, MInt32 *correctImageRet, MInt32 i32EVoffset,
                    MInt32 i32DeviceOrientation);
    void setInitStatus(int value) { m_initStatus = value; }
    MRESULT setSuperNightRawValue(SuperNightRawValue *pSuperNightRawValue, MUInt32 input_num);
    MRESULT uninit();
    MRESULT processPrepare(MInt32 *input_fd, ASVLOFFSCREEN *input, MInt32 camState,
                           MInt32 inputIndex, MInt32 input_Num, ARC_SN_RAWINFO *rawInfo,
                           MInt32 *pExposure);
    MRESULT processCorrectImage(MInt32 *output_fd, ASVLOFFSCREEN *output, MInt32 camState,
                                ARC_SN_RAWINFO *normalRawInfo);

private:
    void InitInternalBuffer();
    void UninitInternalBuffer();
    void CopyImage(ASVLOFFSCREEN *pSrcImg, ASVLOFFSCREEN *pDstImg);
    // void CopyImage(struct MiImageBuffer *pSrcImg, ASVLOFFSCREEN *pDstImg);
    // void CopyImage(ASVLOFFSCREEN *pSrcImg, struct MiImageBuffer *pDstImg);
    void setOffScreen(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage);
    // void setOffScreen(ASVLOFFSCREEN *pImg, struct MiImageBuffer *miBuf);
    // void FillInternalImage(ASVLOFFSCREEN *pDstImage, struct MiImageBuffer *miBuf, int flag =
    // FLAG_FILL_ALL);
    void FillInternalImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage,
                           int flag = FLAG_FILL_ALL);
    // void OutputInternalImage(struct MiImageBuffer *miBuf, ASVLOFFSCREEN *pSrcImage, int flag =
    // FLAG_FILL_ALL);
    void OutputInternalImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage,
                             int flag = FLAG_FILL_ALL);
    void AllocCopyImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage);
    void ReleaseImage(ASVLOFFSCREEN *pImage);
    bool checkUserCancel();
    static MRESULT SuperNightProcessCallback(
        MLong lProgress, // Not impletement
        MLong lStatus,   // Not impletement
        MVoid *pParam    // Thre "pUserData" param of function ARC_SN_Process
    );

private:
    MHandle m_hRawSuperlightEngine;
    ASVLOFFSCREEN mImageInput[FRAME_DEFAULT_INPUT_NUMBER];
    ASVLOFFSCREEN mImageOutput;
    ASVLOFFSCREEN mImageCache;
    unsigned int u32ImagePixelSize;
    int m_initStatus;
    SuperNightRawValue m_superNightRawValue;
    bool m_isPreprocessSucess;
};
#endif
