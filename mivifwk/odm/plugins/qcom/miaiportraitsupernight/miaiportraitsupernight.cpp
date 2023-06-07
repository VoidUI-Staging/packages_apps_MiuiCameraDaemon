#include "miaiportraitsupernight.h"

#include <cutils/properties.h>
#include <stdio.h>
#include <utils/Log.h>

#include "amcomdef.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "miai_supernight.h"

#define MAX_EV_NUM  6
#define MAX_BUF_LEN PROPERTY_VALUE_MAX

#undef LOG_TAG
#define LOG_TAG "MIA_N_MPORTARITSUPERNIGHT"

MiAiSuperLowlight::MiAiSuperLowlight()
    : m_hSuperLowlightEngine(MNull), u32ImagePixelSize(INTERNAL_BUFFER_PIXEL_SIZE)
{
    memset(mImageInput, 0, sizeof(mImageInput));
    memset(&mImageOutput, 0, sizeof(mImageOutput));
}

MiAiSuperLowlight::~MiAiSuperLowlight()
{
    uninit();
}

std::string MiAiSuperLowlight::getversion()
{
    return ARC_SN_GetVersion()->Version;
}

void MiAiSuperLowlight::init()
{
    if (m_hSuperLowlightEngine) {
        ALOGW("[MIAI]: need to release supperlowlight engine first.");
        ARC_SN_Uninit(&m_hSuperLowlightEngine);
        m_hSuperLowlightEngine = MNull;
    }

    ALOGD("[MIAI]: superlowlight proc version=%s\n", ARC_SN_GetVersion()->Version);

    mAlgoInitRet = ARC_SN_Init(HARDCODE_CAMERAID_FRONT, &m_hSuperLowlightEngine);
    if (mAlgoInitRet != MOK) {
        ALOGE("[MIAI]: MIAI_SN_Init error:%ld\n", mAlgoInitRet);
        m_hSuperLowlightEngine = MNull;
    }
#if USE_INTERNAL_BUFFER
    InitInternalBuffer();
#endif
}

void MiAiSuperLowlight::uninit()
{
    ALOGD("[MIAI]: superlowlight unInit proc version=%s\n", ARC_SN_GetVersion()->Version);
    int ret = ARC_SN_Uninit(&m_hSuperLowlightEngine);
    if (ret != MOK)
        ALOGE("[MIAI]: Uninit Error\n");
    else
        m_hSuperLowlightEngine = MNull;

#if USE_INTERNAL_BUFFER
    UninitInternalBuffer();
#endif
}

void MiAiSuperLowlight::process(struct MiImageBuffer *input, struct MiImageBuffer *output,
                                int *pExposure, int input_num, int camMode, float gain,
                                LPMIAI_LLHDR_AEINFO pAeInfo, int flashon_mode, int orient,
                                int bypass, AdditionalParam additionalParam)
{
    (void *)output;
    MRESULT ret = mAlgoInitRet;
    ARC_SN_INPUTINFO m_InputFrames;
    ARC_SN_PARAM m_Param;

    if (input == MNull || output == MNull || pExposure == MNull || input_num <= 0) {
        ALOGE("[MIAI]: process input parameter error\n");
        return;
    }

#if USE_INTERNAL_BUFFER
    if (input_num > FRAME_DEFAULT_INPUT_NUMBER) {
        input_num = FRAME_DEFAULT_INPUT_NUMBER;
    }
#endif

    if (mAlgoInitRet != MOK) {
        ALOGE("[MIAI]: MIAI_SN_Init error:%ld\n", ret);
        int iEV0 = 0;
        for (int i = 0; i < input_num; i++) {
            if (0 == *(pExposure + i)) {
                iEV0 = i;
                break;
            }
        }
        ALOGE("[MIAI]: MIAI_SN_Init return index %d\n", iEV0);
        PluginUtils::miCopyBuffer(output, &input[iEV0]);
        return;
    }
    ALOGD("[MIAI]: superlowlight init ret=%ld, handle=0x%p", ret, m_hSuperLowlightEngine);

    memset(&m_InputFrames, 0, sizeof(m_InputFrames));
    memset(&m_Param, 0, sizeof(m_Param));

    m_InputFrames.i32ImgNum = input_num;
    m_InputFrames.i32CameraState = ARC_SN_CAMERA_STATE_HAND;
    m_InputFrames.i32CurIndex = m_InputFrames.i32ImgNum - 1;

    for (int i = 0; i < input_num; i++) // Initialize m_InputFrames.InputImages
    {
#if USE_INTERNAL_BUFFER
        FillInternalImage(&mImageInput[i], input + i);
        memcpy(&m_InputFrames.InputImages[i], &mImageInput[i], sizeof(ASVLOFFSCREEN));
#else
        setOffScreen(&m_InputFrames.InputImages[i], input + i);
#endif
        m_InputFrames.InputImagesEV[i] = (float)(*(pExposure + i));
        ALOGD("[MIAI]: superlowlight input ev[%d]=%f", i, m_InputFrames.InputImagesEV[i]);
    }

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.miai.sll.brightness", prop, "0");
    m_Param.i32CurveBrightness = atoi(prop);
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.miai.sll.contrast", prop, "0");
    m_Param.i32CurveContrast = atoi(prop);
    property_get("persist.camera.miai.sll.sharpness", prop, "13");
    m_Param.i32Sharpness = atoi(prop);

    ALOGI("[MIAI]: totalGain=%.2f, brightness=%d, contrast=%d, bypass=%d\n", gain,
          m_Param.i32CurveBrightness, m_Param.i32CurveContrast, bypass);

    ASVLOFFSCREEN pOutputImg;
#if USE_INTERNAL_BUFFER
    FillInternalImage(&mImageOutput, output, FLAG_FILL_METADATA);
    memcpy(&pOutputImg, &mImageOutput, sizeof(ASVLOFFSCREEN));
#else
    setOffScreen(&pOutputImg, output);
#endif

    ALOGI("[MIAI]: MIAI_SN_Process before\n");
#if defined(ZEUS_CAM) || defined(CUPID_CAM) || defined(INGRES_CAM) || defined(ZIZHAN_CAM) || \
    defined(MAYFLY_CAM) || defined(DITING_CAM)
    ret = ARC_SN_Process(m_hSuperLowlightEngine, gain, &m_InputFrames, &pOutputImg, &m_Param, NULL,
                         orient, pAeInfo, flashon_mode, additionalParam.macePath, bypass);
#else
    ret = ARC_SN_Process(m_hSuperLowlightEngine, gain, &m_InputFrames, &pOutputImg, &m_Param, NULL,
                         orient, pAeInfo, flashon_mode, bypass);
#endif

    ALOGI("[MIAI]: MIAI_SN_Process after ret=%ld\n", ret);
    if (ret != MOK) {
        int iEV0 = 0;
        for (int i = 0; i < input_num; i++) {
            if (0 == *(pExposure + i)) {
                iEV0 = i;
                break;
            }
        }
        CopyImage(&m_InputFrames.InputImages[iEV0], output);
        ALOGI("[MIAI]: super lowlight Run Error, return index %d\n", iEV0);
    } else {
#if USE_INTERNAL_BUFFER
        OutputInternalImage(output, &pOutputImg);
#endif
    }

    return;
}

void MiAiSuperLowlight::CopyImage(ASVLOFFSCREEN *pSrcImg, ASVLOFFSCREEN *pDstImg)
{
    if (pSrcImg == NULL || pDstImg == NULL)
        return;

    int i;
    unsigned char *l_pSrc = NULL;
    unsigned char *l_pDst = NULL;

    l_pSrc = pSrcImg->ppu8Plane[0];
    l_pDst = pDstImg->ppu8Plane[0];
    for (i = 0; i < pDstImg->i32Height; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->i32Width);
        l_pDst += pDstImg->pi32Pitch[0];
        l_pSrc += pSrcImg->pi32Pitch[0];
    }

    l_pSrc = pSrcImg->ppu8Plane[1];
    l_pDst = pDstImg->ppu8Plane[1];
    for (i = 0; i < pDstImg->i32Height / 2; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->i32Width);
        l_pDst += pDstImg->pi32Pitch[1];
        l_pSrc += pSrcImg->pi32Pitch[1];
    }
}

void MiAiSuperLowlight::CopyImage(struct MiImageBuffer *pSrcImg, ASVLOFFSCREEN *pDstImg)
{
    if (pSrcImg == NULL || pDstImg == NULL)
        return;

    int i;
    unsigned char *l_pSrc = NULL;
    unsigned char *l_pDst = NULL;

    l_pSrc = pSrcImg->plane[0];
    l_pDst = pDstImg->ppu8Plane[0];
    for (i = 0; i < pDstImg->i32Height; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->i32Width);
        l_pDst += pDstImg->pi32Pitch[0];
        l_pSrc += pSrcImg->stride;
    }

    l_pSrc = pSrcImg->plane[1];
    l_pDst = pDstImg->ppu8Plane[1];
    for (i = 0; i < pDstImg->i32Height / 2; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->i32Width);
        l_pDst += pDstImg->pi32Pitch[1];
        l_pSrc += pSrcImg->stride;
    }
}

void MiAiSuperLowlight::CopyImage(ASVLOFFSCREEN *pSrcImg, struct MiImageBuffer *pDstImg)
{
    if (pSrcImg == NULL || pDstImg == NULL)
        return;

    int i;
    unsigned char *l_pSrc = NULL;
    unsigned char *l_pDst = NULL;

    l_pSrc = pSrcImg->ppu8Plane[0];
    l_pDst = pDstImg->plane[0];
    for (i = 0; i < pDstImg->height; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->width);
        l_pDst += pDstImg->stride;
        l_pSrc += pSrcImg->pi32Pitch[0];
    }

    l_pSrc = pSrcImg->ppu8Plane[1];
    l_pDst = pDstImg->plane[1];
    for (i = 0; i < pDstImg->height / 2; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->width);
        l_pDst += pDstImg->stride;
        l_pSrc += pSrcImg->pi32Pitch[1];
    }
}

void MiAiSuperLowlight::InitInternalBuffer()
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

void MiAiSuperLowlight::UninitInternalBuffer()
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

void MiAiSuperLowlight::setOffScreen(ASVLOFFSCREEN *pImg, struct MiImageBuffer *miBuf)
{
    if (miBuf->format == FORMAT_YUV_420_NV12)
        pImg->u32PixelArrayFormat = ASVL_PAF_NV12;
    else
        pImg->u32PixelArrayFormat = ASVL_PAF_NV21;

    pImg->i32Width = miBuf->width;
    pImg->i32Height = miBuf->height;
    pImg->pi32Pitch[0] = miBuf->stride;
    pImg->pi32Pitch[1] = miBuf->stride;
    pImg->ppu8Plane[0] = miBuf->plane[0];
    pImg->ppu8Plane[1] = miBuf->plane[1];
}

void MiAiSuperLowlight::FillInternalImage(ASVLOFFSCREEN *pDstImage, struct MiImageBuffer *miBuf,
                                          int flag)
{
    if (miBuf->format == FORMAT_YUV_420_NV12)
        pDstImage->u32PixelArrayFormat = ASVL_PAF_NV12;
    else
        pDstImage->u32PixelArrayFormat = ASVL_PAF_NV21;
    pDstImage->i32Width = miBuf->width;
    pDstImage->i32Height = miBuf->height;
    pDstImage->pi32Pitch[0] = miBuf->stride;
    pDstImage->pi32Pitch[1] = miBuf->stride;
    pDstImage->pi32Pitch[2] = miBuf->stride;

    if (pDstImage->pi32Pitch[0] * pDstImage->i32Height * 3 / 2 > INTERNAL_BUFFER_PIXEL_SIZE) {
        if (pDstImage->ppu8Plane[0])
            free(pDstImage->ppu8Plane[0]);
        pDstImage->ppu8Plane[0] =
            (unsigned char *)malloc(pDstImage->pi32Pitch[0] * pDstImage->i32Height * 3 / 2);
        ALOGI("[MIAI]: FillInternalImage, buffer size is smaller. org=%d, new=%d\n",
              INTERNAL_BUFFER_PIXEL_SIZE, pDstImage->pi32Pitch[0] * pDstImage->i32Height * 3 / 2);
    }

    if (pDstImage->ppu8Plane[0]) {
        pDstImage->ppu8Plane[1] =
            pDstImage->ppu8Plane[0] + pDstImage->pi32Pitch[0] * pDstImage->i32Height;
        if (flag == FLAG_FILL_ALL)
            CopyImage(miBuf, pDstImage);
    }
}

void MiAiSuperLowlight::FillInternalImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage,
                                          int flag)
{
    pDstImage->u32PixelArrayFormat = pSrcImage->u32PixelArrayFormat;
    pDstImage->i32Width = pSrcImage->i32Width;
    pDstImage->i32Height = pSrcImage->i32Height;
    pDstImage->pi32Pitch[0] = pSrcImage->pi32Pitch[0];
    pDstImage->pi32Pitch[1] = pSrcImage->pi32Pitch[1];
    pDstImage->pi32Pitch[2] = pSrcImage->pi32Pitch[2];

    if (pDstImage->pi32Pitch[0] * pDstImage->i32Height * 3 / 2 > INTERNAL_BUFFER_PIXEL_SIZE) {
        if (pDstImage->ppu8Plane[0])
            free(pDstImage->ppu8Plane[0]);
        pDstImage->ppu8Plane[0] =
            (unsigned char *)malloc(pDstImage->pi32Pitch[0] * pDstImage->i32Height * 3 / 2);
        ALOGI("[MIAI]: FillInternalImage, buffer size is smaller. org=%d, new=%d\n",
              INTERNAL_BUFFER_PIXEL_SIZE, pDstImage->pi32Pitch[0] * pDstImage->i32Height * 3 / 2);
    }

    if (pDstImage->ppu8Plane[0]) {
        pDstImage->ppu8Plane[1] =
            pDstImage->ppu8Plane[0] + pDstImage->pi32Pitch[0] * pDstImage->i32Height;
        if (flag == FLAG_FILL_ALL)
            CopyImage(pSrcImage, pDstImage);
    }
}

void MiAiSuperLowlight::OutputInternalImage(struct MiImageBuffer *miBuf, ASVLOFFSCREEN *pSrcImage,
                                            int flag)
{
    if (pSrcImage->u32PixelArrayFormat == ASVL_PAF_NV12)
        miBuf->format = FORMAT_YUV_420_NV12;
    else
        miBuf->format = FORMAT_YUV_420_NV21;
    miBuf->width = pSrcImage->i32Width;
    miBuf->height = pSrcImage->i32Height;
    miBuf->stride = pSrcImage->pi32Pitch[0];

    if (miBuf->plane[0]) {
        if (flag == FLAG_FILL_ALL)
            CopyImage(pSrcImage, miBuf);
    }
}

void MiAiSuperLowlight::ReleaseImage(ASVLOFFSCREEN *pImage)
{
    if (pImage->ppu8Plane[0])
        free(pImage->ppu8Plane[0]);
    pImage->ppu8Plane[0] = NULL;
}
