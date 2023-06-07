#include "arcsoftrawsuperlight.h"

#include <cutils/properties.h>
#include <stdio.h>
#include <utils/Condition.h>
#include <utils/Log.h>
#include <utils/Mutex.h>

#include "arcsoft_super_night_multi_raw.h"

#define INIT_VALUE 33

ArcsoftRawSuperlight::ArcsoftRawSuperlight()
    : m_hRawSuperlightEngine(MNull), u32ImagePixelSize(INTERNAL_BUFFER_PIXEL_SIZE)
{
    memset(mImageInput, 0, sizeof(mImageInput));
    memset(&mImageOutput, 0, sizeof(mImageOutput));
    memset(&mImageCache, 0, sizeof(mImageCache));
    memset(&m_superNightRawValue, 0, sizeof(SuperNightRawValue));

    m_initStatus = -1;
    m_isPreprocessSucess = false;
}

ArcsoftRawSuperlight::~ArcsoftRawSuperlight()
{
    uninit();
}

MRESULT ArcsoftRawSuperlight::init(MInt32 camMode, MInt32 camState, MInt32 i32Width,
                                   MInt32 i32Height, MPBASE_Version *buildVer, MInt32 AlgoLogLevel,
                                   MRECT *pZoomRect, MInt32 i32LuxIndex)
{
    if (i32Width <= 0 || i32Height <= 0) {
        ALOGE("[ARCSOFT]: ArcsoftRawSuperlight::init failed, param(%dx%d)", i32Width, i32Height);
        return MERR_INVALID_PARAM;
    }
    ARC_SN_SetLogLevel(AlgoLogLevel);
#if USE_INTERNAL_BUFFER
    InitInternalBuffer();
#endif

    if (m_hRawSuperlightEngine) {
        ARC_SN_Uninit(&m_hRawSuperlightEngine);
        ALOGE(
            "[ARCSOFT]: RawSuperlightPlugin, ArcsoftRawSuperlight::init rawsupperlight engine "
            "uninit first.");
        m_hRawSuperlightEngine = MNull;
    }

    const MPBASE_Version *v = ARC_SN_GetVersion();
    if (NULL != v) {
        *buildVer = *v;
        ALOGE("RawSuperlightPlugin %s %s", v->BuildDate, v->Version);
    }

    MRESULT ret = INIT_VALUE;
    do {
        ret = ARC_SN_Init(&m_hRawSuperlightEngine, camMode, camState, i32Width, i32Height,
                          pZoomRect, i32LuxIndex);
        if (ret != MOK) {
            ALOGE("[ARCSOFT]: ARC_SN_Init error:%ld\n", ret);
            break;
        }
        ALOGI(
            "[ARCSOFT]: RawSuperlightPlugin, superlowlight init ret=%ld, camera mode=0x%x, camera "
            "state=%d handle=0x%p, %dx%d",
            ret, camMode, camState, m_hRawSuperlightEngine, i32Width, i32Height);
        ret = ARC_SN_PreProcess(m_hRawSuperlightEngine);
        if (ret != MOK) {
            m_isPreprocessSucess = false;
            ALOGE("[ARCSOFT]: ARC_SN_PreProcess error:%ld\n", ret);
            break;
        } else {
            m_isPreprocessSucess = true;
        }
    } while (false);

    return ret;
}

MRESULT ArcsoftRawSuperlight::uninit()
{
    MRESULT ret = MOK;
    if (m_hRawSuperlightEngine) {
        // PostProcess
        ret = ARC_SN_PostProcess(m_hRawSuperlightEngine);
        ALOGI("[ARCSOFT]: RawSuperlightPlugin, ARC_SN_PostProcess::ret = %ld \n", ret);
        if (ret != MOK) {
            ALOGE("[ARCSOFT]: RawSuperlightPlugin, ARC_SN_PostProcess::error::ret = %ld \n", ret);
        }

        m_initStatus = -1;

        ret = ARC_SN_Uninit(&m_hRawSuperlightEngine);
        if (ret != MOK)
            ALOGE("[ARCSOFT]: Uninit Error\n");
        m_hRawSuperlightEngine = MNull;
    }

#if USE_INTERNAL_BUFFER
    UninitInternalBuffer();
#endif

    if (mImageCache.ppu8Plane[0] != NULL) {
        free(mImageCache.ppu8Plane[0]);
        mImageCache.ppu8Plane[0] = NULL;
    }

    return ret;
}

bool ArcsoftRawSuperlight::checkUserCancel()
{
    return (m_initStatus == 1) ? false : true;
}

MRESULT ArcsoftRawSuperlight::SuperNightProcessCallback(
    MLong lProgress, // Not impletement
    MLong lStatus,   // Not impletement
    MVoid *pParam    // Thre "pUserData" param of function ARC_SN_Process
)
{
    lStatus = 0;
    lProgress = 0;

    ArcsoftRawSuperlight *pSuperNight = (ArcsoftRawSuperlight *)pParam;
    ;
    if (pSuperNight->checkUserCancel()) {
        ALOGE("[ARCSOFT] %s cancel", __func__);
        return MERR_USER_CANCEL;
    }

    return MERR_NONE;
}

MRESULT ArcsoftRawSuperlight::setSuperNightRawValue(SuperNightRawValue *pSuperNightRawValue,
                                                    MUInt32 input_num)
{
    if (MNull == pSuperNightRawValue || pSuperNightRawValue->validFrameCount != input_num) {
        ALOGE("[ARCSOFT]: RawSuperlightPlugin, pSuperNightRawValue parameter NULL\n");
        return MERR_INVALID_PARAM;
    }

    ALOGE("[ARCSOFT]: pSuperNightRawValue->validFrameCount:%d\n",
          pSuperNightRawValue->validFrameCount); // debug +++
    for (int i = 0; i < pSuperNightRawValue->validFrameCount; i++) {
        m_superNightRawValue.Ev[i] = pSuperNightRawValue->Ev[i];
        m_superNightRawValue.ISPGain[i] = pSuperNightRawValue->ISPGain[i];
        m_superNightRawValue.Shutter[i] = pSuperNightRawValue->Shutter[i];
        m_superNightRawValue.SensorGain[i] = pSuperNightRawValue->SensorGain[i];
    }

    return MOK;
}

MRESULT ArcsoftRawSuperlight::process(MInt32 *input_fd, ASVLOFFSCREEN *input, MInt32 *output_fd,
                                      ASVLOFFSCREEN *output, MRECT *pOutImgRect, MInt32 *pExposure,
                                      MUInt32 input_num, MInt32 sceneMode, MInt32 camState,
                                      ARC_SN_RAWINFO *rawInfo, ARC_SN_FACEINFO *faceinfo,
                                      MInt32 *correctImageRet, MInt32 i32EVoffset,
                                      MInt32 i32DeviceOrientation)
{
    *correctImageRet = INIT_VALUE;
    (void *)output;
    (void *)faceinfo;
    MRESULT ret = INIT_VALUE;
    ARC_SN_RAWINFO nRawInfo;
    ARC_SN_FACEINFO nFaceInfo;
    ARC_SN_INPUTINFO nSNInputInfo;
    ARC_SN_INPUTINFO nSNResultInfo;
    // ARC_SN_PARAM nParam;

    MMemSet(&nRawInfo, 0, sizeof(ARC_SN_RAWINFO));
    MMemSet(&nFaceInfo, 0, sizeof(ARC_SN_FACEINFO));
    MMemSet(&nSNInputInfo, 0, sizeof(ARC_SN_INPUTINFO));
    MMemSet(&nSNResultInfo, 0, sizeof(ARC_SN_INPUTINFO));
    // MMemSet(&nParam, 0, sizeof(nParam));   //don't used it anymore

    if (input == MNull || output == MNull || pOutImgRect == MNull || pExposure == MNull ||
        input_num <= 0) {
        ALOGE("[ARCSOFT]: RawSuperlightPlugin, process input parameter error\n");
        return MERR_INVALID_PARAM;
    }

#if USE_INTERNAL_BUFFER
    if (input_num > FRAME_DEFAULT_INPUT_NUMBER) {
        input_num = FRAME_DEFAULT_INPUT_NUMBER;
    }
#endif

    MUInt32 iIndexNormalEV = 0;
    for (MUInt32 index = 1; index < input_num; index++) {
        if (abs(*(pExposure + iIndexNormalEV)) > abs(*(pExposure + index)))
            iIndexNormalEV = index;
    }
    ALOGE("[ARCSOFT]: iIndexNormalEV %d\n", iIndexNormalEV);

    if (m_hRawSuperlightEngine == MNull) {
        // Todo, RawSuperlightPlugin, need check init/uninit function
        ALOGE("[ARCSOFT]: RawSuperlightPlugin, supperlowlight engine hasn't been initialized.");
        CopyImage(output, &input[iIndexNormalEV]);
        return MERR_UNKNOWN;
    }

    nSNInputInfo.i32ImgNum = input_num;
    nSNInputInfo.i32CameraState = camState; // ARC_SN_CAMERA_STATE_UNKNOW,ARC_SN_CAMERA_STATE_HAND

    if (mImageCache.ppu8Plane[0] == NULL) {
        mImageCache.i32Width = input[iIndexNormalEV].i32Width;
        mImageCache.i32Height = input[iIndexNormalEV].i32Height;
        mImageCache.u32PixelArrayFormat = input[iIndexNormalEV].u32PixelArrayFormat;
        mImageCache.pi32Pitch[0] = input[iIndexNormalEV].pi32Pitch[0];
        mImageCache.pi32Pitch[1] = input[iIndexNormalEV].pi32Pitch[1];
        mImageCache.pi32Pitch[2] = input[iIndexNormalEV].pi32Pitch[2];
        mImageCache.ppu8Plane[0] = (unsigned char *)malloc(input[iIndexNormalEV].i32Height *
                                                           input[iIndexNormalEV].pi32Pitch[0]);

        if (mImageCache.ppu8Plane[0] == NULL) {
            ALOGE("[ARCSOFT]: RawSuperlightPlugin,  malloc failed for mImageCache\n");
        }
    }
    CopyImage(&mImageCache, &input[iIndexNormalEV]);

    nSNResultInfo.i32ImgNum = 1;
    nSNResultInfo.i32CameraState = nSNInputInfo.i32CameraState;
    nSNResultInfo.i32CurIndex = nSNResultInfo.i32ImgNum - 1;
    nSNResultInfo.i32InputFd[0] = (MInt32)(*output_fd);
#if USE_INTERNAL_BUFFER
    FillInternalImage(&mImageOutput, output, FLAG_FILL_METADATA);
    memcpy(&nSNResultInfo.InputImages[0], &mImageOutput, sizeof(ASVLOFFSCREEN));
#else
    setOffScreen(&nSNResultInfo.InputImages[0], output);
#endif

    sceneMode = ARC_SN_SCENEMODE_LOWLIGHT;

    if (m_isPreprocessSucess == false) {
        CopyImage(output, &mImageCache);
        if (1 == rawInfo[0].bEnableLSC) {
            *correctImageRet = ARC_SN_CorrectImage(
                m_hRawSuperlightEngine, // [in] The handle for ArcSoft Super Night
                &rawInfo[0],            // [in] RawInfo of the frame, which from camera system
                &nSNResultInfo          // [in & out]tansfer in the input[0], do OB andr LSC
            );
        }
        ALOGE(
            "[ARCSOFT]: RawSuperlightPlugin, super lowlight Run Error, return cacheImage index %d "
            "ret=%d\n",
            iIndexNormalEV, *correctImageRet);
        {
#if USE_INTERNAL_BUFFER
            OutputInternalImage(output, &nSNResultInfo.InputImages[0]);
#endif
        }
    } else {
        ALOGI("[ARCSOFT]: RawSuperlightPlugin, ARC_SN_Process before\n");
        ret = ARC_SN_Process(m_hRawSuperlightEngine, &faceinfo[0],
                             &nSNResultInfo, // output image
                             MNull,          //&nParam,          //dont use it anymore
                             sceneMode,      // ARC_SN_SCENEMODE_LOWLIGHT
                             pOutImgRect,    // output rect
                             SuperNightProcessCallback, (MVoid *)this, i32EVoffset,
                             i32DeviceOrientation);

        ALOGI(
            "[ARCSOFT]: RawSuperlightPlugin, ARC_SN_Process after ret=%ld, "
            "outImageRect[%d,%d,%d,%d]\n",
            ret, pOutImgRect->left, pOutImgRect->top, pOutImgRect->right, pOutImgRect->bottom);
        ret = MOK;
#if USE_INTERNAL_BUFFER
        OutputInternalImage(output, &nSNResultInfo.InputImages[0]);
#endif
    }
    return ret;
}

MRESULT ArcsoftRawSuperlight::processPrepare(MInt32 *input_fd, ASVLOFFSCREEN *input,
                                             MInt32 camState, MInt32 inputIndex, MInt32 input_Num,
                                             ARC_SN_RAWINFO *rawInfo, MInt32 *pExposure)
{
    MRESULT ret = INIT_VALUE;
    int32_t inputNum = input_Num;
    ARC_SN_INPUTINFO nSNInputInfo;
    MMemSet(&nSNInputInfo, 0, sizeof(ARC_SN_INPUTINFO));
#if USE_INTERNAL_BUFFER
    if (inputNum > FRAME_DEFAULT_INPUT_NUMBER) {
        inputNum = FRAME_DEFAULT_INPUT_NUMBER;
    }
#endif

    nSNInputInfo.i32ImgNum = inputNum;
    nSNInputInfo.i32CameraState = camState;

#if USE_INTERNAL_BUFFER
    FillInternalImage(&mImageInput[0], input);
    memcpy(&nSNInputInfo.InputImages[0], &mImageInput[0], sizeof(ASVLOFFSCREEN));
#else
    setOffScreen(&nSNInputInfo.InputImages[0], input);
#endif
    nSNInputInfo.i32CurIndex = inputIndex;
    nSNInputInfo.InputImagesEV[0] = pExposure[inputIndex];
    nSNInputInfo.i32InputFd[0] = (MInt32)(*(input_fd));
    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]: NODESLL, superlowlight input fd[%d]=%d ev[%d]=%f from input[%d]=%d "
          "input_fd[%d]=%d",
          inputIndex, nSNInputInfo.i32InputFd[0], inputIndex, nSNInputInfo.InputImagesEV[0],
          inputIndex, pExposure[inputIndex], inputIndex, (*(input_fd)));

    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]: NODESLL, frame[%d] fISPGain=%f SensorGain=%f Shutter=%f EV=%d BLS:[%d, %d, "
          "%d, %d]",
          inputIndex, rawInfo->fISPGain, rawInfo->fSensorGain, rawInfo->fShutter, rawInfo->i32EV[0],
          rawInfo->i32BlackLevel[0], rawInfo->i32BlackLevel[1], rawInfo->i32BlackLevel[2],
          rawInfo->i32BlackLevel[3]);
    if (m_isPreprocessSucess) {
        ret = ARC_SN_AddOneInputInfo(m_hRawSuperlightEngine, rawInfo, &nSNInputInfo);
    }
    return ret;
}

MRESULT ArcsoftRawSuperlight::processCorrectImage(MInt32 *output_fd, ASVLOFFSCREEN *output,
                                                  MInt32 camState, ARC_SN_RAWINFO *normalRawInfo)
{
    MRESULT ret = INIT_VALUE;
    ARC_SN_INPUTINFO nSNResultInfo;
    MMemSet(&nSNResultInfo, 0, sizeof(ARC_SN_INPUTINFO));
    nSNResultInfo.i32ImgNum = 1;
    nSNResultInfo.i32CameraState = camState;
    nSNResultInfo.i32CurIndex = nSNResultInfo.i32ImgNum - 1;
    nSNResultInfo.i32InputFd[0] = (MInt32)(*output_fd);
    setOffScreen(&nSNResultInfo.InputImages[0], output);

    if (1 == normalRawInfo->bEnableLSC) {
        ret = ARC_SN_CorrectImage(
            m_hRawSuperlightEngine, // [in] The handle for ArcSoft Super Night
            normalRawInfo,          // [in] RawInfo of the frame, which from camera system
            &nSNResultInfo          // [in & out]tansfer in the input[0], do OB andr LSC
        );
    }
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: NODESLL, bEnableLSC=%d correctImage ret=%d\n",
          normalRawInfo->bEnableLSC, ret);
    return ret;
}

// TODO: For YUV format or RAW format
void ArcsoftRawSuperlight::CopyImage(ASVLOFFSCREEN *pDstImg, ASVLOFFSCREEN *pSrcImg)
{
    if (pSrcImg == NULL || pDstImg == NULL) {
        ALOGE("[ARCSOFT]: 1, CopyImage intput parameter error, pDstImg = %p, pSrcImg = %p", pDstImg,
              pSrcImg);
        return;
    }
    if (pDstImg->u32PixelArrayFormat != pSrcImg->u32PixelArrayFormat) {
        ALOGE(
            "[ARCSOFT]: 1, super lowlight CopyImage error, the format is not same, dst format %d, "
            "src format %d",
            pDstImg->u32PixelArrayFormat, pSrcImg->u32PixelArrayFormat);
        return;
    }

    memcpy(pDstImg->ppu8Plane[0], pSrcImg->ppu8Plane[0],
           pSrcImg->i32Width * pSrcImg->i32Height * 2);
}

void ArcsoftRawSuperlight::InitInternalBuffer()
{
    for (int i = 0; i < FRAME_DEFAULT_INPUT_NUMBER; i++) {
        if (mImageInput[i].ppu8Plane[0])
            free(mImageInput[i].ppu8Plane[0]);
        mImageInput[i].ppu8Plane[0] = (unsigned char *)malloc(INTERNAL_BUFFER_PIXEL_SIZE);
    }
    if (mImageOutput.ppu8Plane[0])
        free(mImageOutput.ppu8Plane[0]);
    mImageOutput.ppu8Plane[0] = (unsigned char *)malloc(INTERNAL_BUFFER_PIXEL_SIZE);
}

void ArcsoftRawSuperlight::UninitInternalBuffer()
{
    for (int i = 0; i < FRAME_DEFAULT_INPUT_NUMBER; i++) {
        if (mImageInput[i].ppu8Plane[0])
            free(mImageInput[i].ppu8Plane[0]);
        mImageInput[i].ppu8Plane[0] = NULL;
    }
    if (mImageOutput.ppu8Plane[0])
        free(mImageOutput.ppu8Plane[0]);
    mImageOutput.ppu8Plane[0] = NULL;
}

void ArcsoftRawSuperlight::setOffScreen(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage)
{
    ALOGI("[ARCSOFT]: setOffScreen u32PixelArrayFormat is %d", pSrcImage->u32PixelArrayFormat);
    pDstImage->u32PixelArrayFormat = pSrcImage->u32PixelArrayFormat;
    pDstImage->i32Width = pSrcImage->i32Width;
    pDstImage->i32Height = pSrcImage->i32Height;
    pDstImage->pi32Pitch[0] = pSrcImage->pi32Pitch[0];
    pDstImage->pi32Pitch[1] = pSrcImage->pi32Pitch[1];
    pDstImage->pi32Pitch[2] = pSrcImage->pi32Pitch[2];
    pDstImage->ppu8Plane[0] = pSrcImage->ppu8Plane[0];
    pDstImage->ppu8Plane[1] = pSrcImage->ppu8Plane[1];
    pDstImage->ppu8Plane[2] = pSrcImage->ppu8Plane[2];
}

void ArcsoftRawSuperlight::FillInternalImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage,
                                             int flag)
{
    ALOGI("[ARCSOFT]: FillInternalImage u32PixelArrayFormat is %d, flag:%d",
          pSrcImage->u32PixelArrayFormat, flag);
    pDstImage->u32PixelArrayFormat = pSrcImage->u32PixelArrayFormat;
    pDstImage->i32Width = pSrcImage->i32Width;
    pDstImage->i32Height = pSrcImage->i32Height;
    pDstImage->pi32Pitch[0] = pSrcImage->pi32Pitch[0];
    pDstImage->pi32Pitch[1] = pSrcImage->pi32Pitch[1];
    pDstImage->pi32Pitch[2] = pSrcImage->pi32Pitch[2];

    if (pDstImage->pi32Pitch[0] * pDstImage->i32Height > INTERNAL_BUFFER_PIXEL_SIZE) {
        if (pDstImage->ppu8Plane[0])
            free(pDstImage->ppu8Plane[0]);
        pDstImage->ppu8Plane[0] =
            (unsigned char *)malloc(pDstImage->pi32Pitch[0] * pDstImage->i32Height);
        ALOGE(
            "[ARCSOFT]: RawSuperlightPlugin, FillInternalImage, buffer size is smaller. org=%d, "
            "new=%d\n",
            INTERNAL_BUFFER_PIXEL_SIZE, pDstImage->pi32Pitch[0] * pDstImage->i32Height * 3 / 2);
    }

    if (pDstImage->ppu8Plane[0] && flag == FLAG_FILL_ALL) {
        CopyImage(pDstImage, pSrcImage);
    }
}

void ArcsoftRawSuperlight::OutputInternalImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage,
                                               int flag)
{
    ALOGI("[ARCSOFT]: OutputInternalImage u32PixelArrayFormat is %d, flag:%d",
          pSrcImage->u32PixelArrayFormat, flag);
    pDstImage->u32PixelArrayFormat = pSrcImage->u32PixelArrayFormat;
    pDstImage->i32Width = pSrcImage->i32Width;
    pDstImage->i32Height = pSrcImage->i32Height;
    pDstImage->pi32Pitch[0] = pSrcImage->pi32Pitch[0];
    pDstImage->pi32Pitch[1] = pSrcImage->pi32Pitch[1];
    pDstImage->pi32Pitch[2] = pSrcImage->pi32Pitch[2];

    if (pDstImage->ppu8Plane[0] && flag == FLAG_FILL_ALL) {
        CopyImage(pDstImage, pSrcImage);
    }
}

void ArcsoftRawSuperlight::ReleaseImage(ASVLOFFSCREEN *pImage)
{
    if (pImage->ppu8Plane[0])
        free(pImage->ppu8Plane[0]);
    pImage->ppu8Plane[0] = NULL;
}
