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

#include "arcsoftdistortioncorrection.h"

#include <cutils/properties.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/system_properties.h>
#include <time.h>
#include <unistd.h>
#include <utils/Log.h>

#include "amcomdef.h"
#include "ammem.h"
#include "arcsoft_debuglog.h"
#include "arcsoft_distortion_correction.h"
#include "asvloffscreen.h"
#include "merror.h"
// used for performance

using namespace mialgo2;

static long long __attribute__((unused)) gettime()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((long long)time.tv_sec * 1000 + time.tv_usec / 1000);
}

ArcsoftDistortionCorrection::ArcsoftDistortionCorrection(MInt32 dsmode)
    : m_hDistortionCorrectionEngine(MNull),
      m_u32ImagePixelSize(INTERNAL_BUFFER_PIXEL_SIZE),
      m_pCalibration(MNull),
      m_dsmode(dsmode)
{
    memset(&mImageInput, 0, sizeof(mImageInput));
    memset(&mImageOutput, 0, sizeof(mImageOutput));
    memset(&m_DCInitParam, 0, sizeof(m_DCInitParam));
}

ArcsoftDistortionCorrection::~ArcsoftDistortionCorrection()
{
    uninit();
}

void ArcsoftDistortionCorrection::init(unsigned int camMode, unsigned int mode)
{
    arcInitCommonConfigSpec("persist.vendor.camera.arcsoft.log.ds");

    MRESULT ret = MOK;
#if USE_INTERNAL_BUFFER
    InitInternalBuffer();
#endif
    if (m_dsmode != 0) {
        if (MNull == m_pCalibration) {
            m_pCalibration = (unsigned char *)malloc(CALIBRATIONDATA_LEN);
            if (m_pCalibration) {
                memset(m_pCalibration, 0, CALIBRATIONDATA_LEN);
                m_DCInitParam.pCaliData = m_pCalibration;
                m_DCInitParam.i32CaliDataLen = CALIBRATIONDATA_LEN;
            }
        }
    }

    if (m_hDistortionCorrectionEngine) {
        ARC_LOGD("[ARCSOFT]: Sheng, distortioncorrection uninit handle=0x%p",
                 m_hDistortionCorrectionEngine);
        ARC_DC_Uninit(&m_hDistortionCorrectionEngine);
        m_hDistortionCorrectionEngine = MNull;
    }

    int lmode = ARC_DC_MODE_IMAGE_FRONT;
    if (m_dsmode == 1) {
        lmode = ARC_DC_MODE_ONLY_UNDISTORT;
    } else if (m_dsmode == 2) {
        switch (mode) {
        case RUNTIMEMODE_SNAPSHOT:
            if (camMode == 0)
                lmode = ARC_DC_MODE_IMAGE_REAR;
            else
                lmode = ARC_DC_MODE_IMAGE_FRONT;
            break;
        default:
            if (camMode == 0)
                lmode = ARC_DC_MODE_VIDEO_REAR;
            else
                lmode = ARC_DC_MODE_VIDEO_FRONT;
            break;
        }
    } else // default
    {
        switch (mode) {
        case RUNTIMEMODE_SNAPSHOT:
            lmode = ARC_DC_MODE_ONLY_SELFIE_IMAGE;
            break;
        default:
            lmode = ARC_DC_MODE_ONLY_SELFIE_VIDEO;
        }
    }

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.arc.ds.intensity", prop, "48");
    MInt32 nIntensity = (MInt32)atoi(prop);
    m_DCInitParam.fViewAngleH = nIntensity;

    if (m_dsmode == 0) {
        m_DCInitParam.pCaliData = NULL;
        m_DCInitParam.i32CaliDataLen = 0;
    }
    ret = ARC_DC_Init(&m_hDistortionCorrectionEngine, &m_DCInitParam, lmode);

    if (ret != MOK) {
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: Sheng, ARC_DC_Init error\n");
        m_hDistortionCorrectionEngine = MNull;
    }
    ARC_LOGD(
        "[ARCSOFT]: Sheng, distortioncorrection init ret=%ld, handle=0x%p mode=%d lmode=%d "
        "nIntensity=%d",
        ret, m_hDistortionCorrectionEngine, mode, lmode, nIntensity);
}

void ArcsoftDistortionCorrection::uninit()
{
    if (m_hDistortionCorrectionEngine) {
        ARC_LOGD("[ARCSOFT]: Sheng, distortioncorrection uninit handle=0x%p",
                 m_hDistortionCorrectionEngine);
        ARC_DC_Uninit(&m_hDistortionCorrectionEngine);
        m_hDistortionCorrectionEngine = MNull;
    }
    if (m_dsmode != 0) {
        if (m_pCalibration) {
            free(m_pCalibration);
            m_pCalibration = MNull;
        }
    }
    memset(&m_DCInitParam, 0, sizeof(m_DCInitParam));

#if USE_INTERNAL_BUFFER
    UninitInternalBuffer();
#endif
}

void ArcsoftDistortionCorrection::setcalibrationdata(const unsigned char *pCalib, unsigned int size)
{
    if (MNull == m_pCalibration) {
        m_pCalibration = (unsigned char *)malloc(CALIBRATIONDATA_LEN);
        if (m_pCalibration) {
            memset(m_pCalibration, 0, CALIBRATIONDATA_LEN);
        }
    }

#ifndef FAKE_ARC_CALIBRATION
    if (m_pCalibration && pCalib && size >= CALIBRATIONDATA_LEN) {
        memcpy(m_pCalibration, pCalib, CALIBRATIONDATA_LEN);
        m_DCInitParam.pCaliData = m_pCalibration;
        m_DCInitParam.i32CaliDataLen = CALIBRATIONDATA_LEN;
    } else {
        MLOGE(Mia2LogGroupPlugin,
              "[ARCSOFT]: Sheng, setcalibrationdata the input param is incorrect, pCalib=0x%p, "
              "size=%d",
              pCalib, size);
    }
#else
    (void *)pCalib;
    (void)size;
    if (m_pCalibration) {
        int cali_data_size = CALIBRATIONDATA_LEN;
        ReadCalibrationDataFromFile(CAL_DATA_PATH, m_pCalibration, cali_data_size);
        m_DCInitParam.pCaliData = m_pCalibration;
        m_DCInitParam.i32CaliDataLen = CALIBRATIONDATA_LEN;
    } else {
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: Sheng, setcalibrationdata m_pCalibration is null");
    }
#endif
}

void ArcsoftDistortionCorrection::process(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output,
                                          ARC_DC_FACE *pDCFace, int camMode, unsigned int rMode)
{
    (void *)output;
    MRESULT ret = MOK;
    MInt32 mode = ARC_DC_MODE_IMAGE_REAR;
    // ARC_DC_PARAM     mParam;

    if (input == MNull || output == MNull || pDCFace == MNull) {
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: Sheng, process input parameter error\n");
        return;
    }

    switch (rMode) {
    case RUNTIMEMODE_SNAPSHOT:
        if (camMode == HARDCODE_CAMERAID_REAR)
            mode = ARC_DC_MODE_IMAGE_REAR;
        else
            mode = ARC_DC_MODE_IMAGE_FRONT;
        break;
    case RUNTIMEMODE_PREVIEW:
        if (camMode == HARDCODE_CAMERAID_REAR)
            mode = ARC_DC_MODE_VIDEO_REAR;
        else
            mode = ARC_DC_MODE_VIDEO_FRONT;
        break;
    case RUNTIMEMODE_RECORD:
        if (camMode == HARDCODE_CAMERAID_REAR)
            mode = ARC_DC_MODE_VIDEO_REAR;
        else
            mode = ARC_DC_MODE_VIDEO_FRONT;
        break;
    default:
        break;
    }
    ARC_LOGD("[ARCSOFT]: Sheng, runtime mode is %d\n", mode);

    ARC_LOGD("[ARCSOFT]: Sheng, distortioncorrection proc version=%s\n",
             ARC_DC_GetVersion()->Version);

#if USE_INTERNAL_BUFFER
    FillInternalImage(&mImageInput, input);
#else
    setOffScreen(&mImageInput, input);
#endif

#if USE_INTERNAL_BUFFER
    FillInternalImage(&mImageOutput, output, FLAG_FILL_METADATA);
#else
    setOffScreen(&mImageOutput, output);
#endif

    int omask = 0;
    int nmask = 0xF0;
    int syscallres = syscall(__NR_sched_getaffinity, gettid(), sizeof(omask), &omask);
    syscallres = syscall(__NR_sched_setaffinity, gettid(), sizeof(nmask), &nmask);

    long long llStartTime = gettime();
    ARC_LOGD("[ARCSOFT]: Sheng, ARC_DC_Process before\n");
    ret = ARC_DC_Process(m_hDistortionCorrectionEngine, &mImageInput, &mImageOutput, pDCFace);
    ARC_LOGD("[ARCSOFT]: Sheng, ARC_DC_Process after ret=%ld -- %d X%d \n", ret,
             mImageOutput.i32Width, mImageOutput.i32Height);
    ARC_LOGD("[ARCSOFT]: Sheng, Performance time is %lld", gettime() - llStartTime);
    if (ret != MOK) {
        unsigned char *pSrc, *pDst;
        int height = 0;
        MLOGE(Mia2LogGroupPlugin,
              "[ARCSOFT]: Sheng, distortion correction Run Error, return normal Img\n");
        pDst = output->ppu8Plane[0];
        pSrc = mImageInput.ppu8Plane[0];
        for (height = 0; height < output->i32Height; height++) {
            memcpy(pDst, pSrc, output->i32Width);
            pSrc += mImageInput.pi32Pitch[0];
            pDst += output->pi32Pitch[0];
        }
        pDst = output->ppu8Plane[1];
        pSrc = mImageInput.ppu8Plane[1];
        for (height = 0; height < output->i32Height / 2; height++) {
            memcpy(pDst, pSrc, output->i32Width);
            pSrc += mImageInput.pi32Pitch[1];
            pDst += output->pi32Pitch[1];
        }
    }

#if USE_INTERNAL_BUFFER
    OutputInternalImage(output, &mImageOutput);
#endif

    return;
}

void ArcsoftDistortionCorrection::CopyImage(ASVLOFFSCREEN *pSrcImg, ASVLOFFSCREEN *pDstImg)
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

void ArcsoftDistortionCorrection::InitInternalBuffer()
{
#if USE_INTERNAL_BUFFER
    if (mImageInput.ppu8Plane[0])
        free(mImageInput.ppu8Plane[0]);
    mImageInput.ppu8Plane[0] = (unsigned char *)malloc(INTERNAL_BUFFER_PIXEL_SIZE);
    if (mImageOutput.ppu8Plane[0])
        free(mImageOutput.ppu8Plane[0]);
    mImageOutput.ppu8Plane[0] = (unsigned char *)malloc(INTERNAL_BUFFER_PIXEL_SIZE);
#endif
}

void ArcsoftDistortionCorrection::UninitInternalBuffer()
{
#if USE_INTERNAL_BUFFER
    if (mImageInput.ppu8Plane[0])
        free(mImageInput.ppu8Plane[0]);
    mImageInput.ppu8Plane[0] = NULL;

    if (mImageOutput.ppu8Plane[0])
        free(mImageOutput.ppu8Plane[0]);
    mImageOutput.ppu8Plane[0] = NULL;
#endif
}

void ArcsoftDistortionCorrection::setOffScreen(ASVLOFFSCREEN *pDstImg, ASVLOFFSCREEN *pSrcImg)
{
    pDstImg->u32PixelArrayFormat = pSrcImg->u32PixelArrayFormat;
    pDstImg->i32Width = pSrcImg->i32Width;
    pDstImg->i32Height = pSrcImg->i32Height;
    pDstImg->pi32Pitch[0] = pSrcImg->pi32Pitch[0];
    pDstImg->pi32Pitch[1] = pSrcImg->pi32Pitch[1];
    pDstImg->ppu8Plane[0] = pSrcImg->ppu8Plane[0];
    pDstImg->ppu8Plane[1] = pSrcImg->ppu8Plane[1];
}

void ArcsoftDistortionCorrection::FillInternalImage(ASVLOFFSCREEN *pDstImage,
                                                    ASVLOFFSCREEN *pSrcImage, int flag)
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
        ARC_LOGD("[ARCSOFT]: Sheng, FillInternalImage, buffer size is smaller. org=%d, new=%d\n",
                 INTERNAL_BUFFER_PIXEL_SIZE,
                 pDstImage->pi32Pitch[0] * pDstImage->i32Height * 3 / 2);
    }

    if (pDstImage->ppu8Plane[0]) {
        pDstImage->ppu8Plane[1] =
            pDstImage->ppu8Plane[0] + pDstImage->pi32Pitch[0] * pDstImage->i32Height;
        if (flag == FLAG_FILL_ALL)
            CopyImage(pSrcImage, pDstImage);
    }
}

void ArcsoftDistortionCorrection::OutputInternalImage(ASVLOFFSCREEN *pDstImage,
                                                      ASVLOFFSCREEN *pSrcImage, int flag)
{
    pDstImage->u32PixelArrayFormat = pSrcImage->u32PixelArrayFormat;
    pDstImage->i32Width = pSrcImage->i32Width;
    pDstImage->i32Height = pSrcImage->i32Height;
    pDstImage->pi32Pitch[0] = pSrcImage->pi32Pitch[0];
    pDstImage->pi32Pitch[1] = pSrcImage->pi32Pitch[1];
    pDstImage->pi32Pitch[2] = pSrcImage->pi32Pitch[2];

    if (pDstImage->ppu8Plane[0]) {
        if (flag == FLAG_FILL_ALL)
            CopyImage(pSrcImage, pDstImage);
    }
}

void ArcsoftDistortionCorrection::ReleaseImage(ASVLOFFSCREEN *pImage)
{
    if (pImage->ppu8Plane[0])
        free(pImage->ppu8Plane[0]);
    pImage->ppu8Plane[0] = NULL;
}

#ifdef FAKE_ARC_CALIBRATION
int ArcsoftDistortionCorrection::ReadCalibrationDataFromFile(const char *szFile,
                                                             unsigned char *pCaliBuff, int dataSize)
{
    ARC_LOGD("[ARCSOFT] %s:%d: (IN)", __func__, __LINE__);
    if (dataSize == 0 || NULL == pCaliBuff) {
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT] %s:%d: (OUT) failed !", __func__, __LINE__);
        return -1;
    }

    int bytes_read = 0;
    FILE *califile = NULL;
    califile = fopen(szFile, "r");
    if (NULL != califile) {
        bytes_read = fread(pCaliBuff, 1, dataSize, califile);
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT] : read calidata bytes_read = %d", bytes_read);
        fclose(califile);
    }
    ARC_LOGD("[ARCSOFT] ReadCalibrationDataFromFile dataSize:%d", dataSize);
    if (bytes_read != dataSize) {
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT] %s:%d: (OUT) failed !", __func__, __LINE__);
        return -1;
    }

    ARC_LOGD("[ARCSOFT] %s:%d: (OUT) success !", __func__, __LINE__);
    return 0;
}
#endif
