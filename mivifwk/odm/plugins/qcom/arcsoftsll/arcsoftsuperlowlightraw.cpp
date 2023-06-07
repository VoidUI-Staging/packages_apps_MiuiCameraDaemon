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

#include "arcsoftsuperlowlightraw.h"

#include <cutils/properties.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include "amcomdef.h"
#include "ammem.h"
#include "arcsoft_super_night_multi_raw.h"
#include "asvloffscreen.h"
#include "merror.h"

ArcsoftSuperLowlightRaw *ArcsoftSuperLowlightRaw::m_ArcsoftSLLRawSingleton(NULL);
std::mutex ArcsoftSuperLowlightRaw::m_mutex;
ArcsoftSuperLowlightRaw::Garbo ArcsoftSuperLowlightRaw::gc;

static long long __attribute__((unused)) gettime()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((long long)time.tv_sec * 1000 + time.tv_usec / 1000);
}

ArcsoftSuperLowlightRaw *ArcsoftSuperLowlightRaw::getInstance()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_ArcsoftSLLRawSingleton == NULL) {
        m_ArcsoftSLLRawSingleton = new ArcsoftSuperLowlightRaw();
        MLOGI(Mia2LogGroupPlugin, "ArcsoftSuperLowlightRaw[%p] construction",
              m_ArcsoftSLLRawSingleton);
    }
    return m_ArcsoftSLLRawSingleton;
}

bool ArcsoftSuperLowlightRaw::getInitFlag()
{
    return m_initFlag;
}

void ArcsoftSuperLowlightRaw::initHTP()
{
    if (false == m_initHTPFlag) {
        MRESULT ret = INIT_VALUE;
        for (int cnt = 0; cnt < 4; cnt++) {
            putEnv(5);
            double htpStartTime = gettime();
            ret = ARC_SN_HTP_Init();
            m_initHTPFlag = true;
            if (ret != MOK) {
                MLOGI(Mia2LogGroupPlugin,
                      "[ARCSOFT]: ARC_SN_HTP_Init error:%ld cost time: %f retry:%d", ret,
                      static_cast<double>(gettime() - htpStartTime), cnt);
                ARC_SN_HTP_UnInit();
            } else {
                MLOGI(Mia2LogGroupPlugin,
                      "[ARCSOFT]: NODESLL, ARC_SN_HTP_Init ret=%ld cost time: %f", ret,
                      static_cast<double>(gettime() - htpStartTime));
                break;
            }
        }

        if (ret != MOK) {
            MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: ARC_SN_HTP_Init error:%ld\n", ret);
        }
    }
}

void ArcsoftSuperLowlightRaw::unInitHTP()
{
    if (true == m_initHTPFlag) {
        double htpStartTime = gettime();
        ARC_SN_HTP_UnInit();
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: ARC_SN_HTP_UnInit cost time: %f",
              static_cast<double>(gettime() - htpStartTime));
        m_initHTPFlag = false;
    }
}

ArcsoftSuperLowlightRaw::ArcsoftSuperLowlightRaw()
    : m_hSuperLowlightRawEngine(MNull), u32ImagePixelSize(INTERNAL_BUFFER_PIXEL_SIZE)
{
    memset(mImageInput, 0, sizeof(mImageInput));
    memset(&mImageOutput, 0, sizeof(mImageOutput));
    memset(&mImageCache, 0, sizeof(mImageCache));
    memset(&m_superNightRawValue, 0, sizeof(SuperNightRawValue));

    m_initStatus = -1;
    m_initFlag = false;
    m_initHTPFlag = false;
    m_isPreprocessSucess = false;
}

ArcsoftSuperLowlightRaw::~ArcsoftSuperLowlightRaw()
{
    uninit();
}

bool ArcsoftSuperLowlightRaw::deteckTimeout(int64_t flushTimeoutSec)
{
    std::unique_lock<std::mutex> lck(m_timeoutMux);
    bool isTimeOut = false;
    if (m_timeoutCond.wait_for(lck, std::chrono::seconds(flushTimeoutSec)) ==
        std::cv_status::timeout) {
        isTimeOut = true;
    }

    return isTimeOut;
}

MRESULT ArcsoftSuperLowlightRaw::init(MInt32 camMode, MInt32 camState, MInt32 i32Width,
                                      MInt32 i32Height, MPBASE_Version *buildVer, MRECT *pZoomRect,
                                      MInt32 i32LuxIndex)
{
    std::unique_lock<std::mutex> lck(m_initMux);
    if (i32Width <= 0 || i32Height <= 0) {
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: ArcsoftSuperLowlightRaw::init failed, param(%dx%d)",
              i32Width, i32Height);
        return MERR_INVALID_PARAM;
    }

    // If the last process has not ended, first flush the last process, and then process the
    // current task
    MLOGI(Mia2LogGroupPlugin, "SuperLowlightRaw:%p initialed status:%d", this, m_initFlag);
    if (m_initFlag == true) {
        setInitStatus(0);
        if (deteckTimeout(FlushTimeoutSec)) {
            MLOGE(Mia2LogGroupPlugin, "flush the lastest process time out [%d]s", FlushTimeoutSec);
            return RET_XIOMI_TIME_OUT;
        }
    }
    m_initFlag = true;

#if USE_INTERNAL_BUFFER
    InitInternalBuffer();
#endif

    const MPBASE_Version *v = ARC_SN_GetVersion();
    if (NULL != v) {
        buildVer->BuildDate = v->BuildDate;
        buildVer->CopyRight = v->CopyRight;
        buildVer->lBuild = v->lBuild;
        buildVer->lCodebase = v->lCodebase;
        buildVer->lMajor = v->lMajor;
        buildVer->lMinor = v->lMinor;
        buildVer->Version = v->Version;
        MLOGI(Mia2LogGroupPlugin, "NODESLL  %s %s %ld %ld", v->BuildDate, v->Version, v->lCodebase,
              v->lBuild);
    }

    MRESULT ret = INIT_VALUE;
    do {
        for (int i = 0; i < 4; i++) {
            putEnv(5);
            if (m_hSuperLowlightRawEngine) {
                ARC_SN_Uninit(&m_hSuperLowlightRawEngine);
                MLOGI(Mia2LogGroupPlugin,
                      "[ARCSOFT]: NODESLL, ArcsoftSuperLowlightRaw::init supperlowlight engine "
                      "uninit "
                      "first.");

                m_hSuperLowlightRawEngine = MNull;
            }
            ret = ARC_SN_Init(&m_hSuperLowlightRawEngine, camMode, camState, i32Width, i32Height,
                              pZoomRect, i32LuxIndex);
            if (ret != MOK) {
                MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: ARC_SN_Init error:%ld retry times:%d", ret,
                      i);
            } else {
                MLOGI(Mia2LogGroupPlugin,
                      "[ARCSOFT]: NODESLL, superlowlight init ret=%ld, camera mode=0x%x, camera "
                      "state=%d "
                      "handle=0x%p, %dx%d",
                      ret, camMode, camState, m_hSuperLowlightRawEngine, i32Width, i32Height);
                break;
            }
        }

        if (ret != MOK) {
            MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: ARC_SN_Init error:%ld\n", ret);
        }

        ret = ARC_SN_PreProcess(m_hSuperLowlightRawEngine);
        if (ret != MOK) {
            m_isPreprocessSucess = false;
            MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: ARC_SN_PreProcess error:%ld\n", ret);
            break;
        } else {
            m_isPreprocessSucess = true;
        }
    } while (false);

    return ret;
}

MRESULT ArcsoftSuperLowlightRaw::uninit()
{
    MRESULT ret = MOK;

    if (m_hSuperLowlightRawEngine) {
        // PostProcess
        ret = ARC_SN_PostProcess(m_hSuperLowlightRawEngine);
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: NODESLL ARC_SN_PostProcess::ret = %ld \n", ret);
        if (ret != MOK) {
            MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: NODESLL, ARC_SN_PostProcess::error::ret = %ld \n",
                  ret);
        }

        m_initStatus = -1;

        ret = ARC_SN_Uninit(&m_hSuperLowlightRawEngine);
        if (ret != MOK)
            MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: Uninit Error\n");
        m_hSuperLowlightRawEngine = MNull;
    }

#if USE_INTERNAL_BUFFER
    UninitInternalBuffer();
#endif

    if (mImageCache.ppu8Plane[0] != NULL) {
        free(mImageCache.ppu8Plane[0]);
        mImageCache.ppu8Plane[0] = NULL;
    }

    m_initFlag = false;
    m_timeoutCond.notify_one();
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: notify the algo process complete");

    return ret;
}

bool ArcsoftSuperLowlightRaw::checkUserCancel()
{
    return (m_initStatus == 1) ? false : true;
}

MRESULT ArcsoftSuperLowlightRaw::SuperNightProcessCallback(
    MLong lProgress, // Not impletement
    MLong lStatus,   // Not impletement
    MVoid *pParam    // Thre "pUserData" param of function ARC_SN_Process
)
{
    lStatus = 0;
    lProgress = 0;

    ArcsoftSuperLowlightRaw *pSuperNight = (ArcsoftSuperLowlightRaw *)pParam;
    ;
    if (pSuperNight->checkUserCancel()) {
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT] %s cancel", __func__);
        return MERR_USER_CANCEL;
    }

    return MERR_NONE;
}

MRESULT ArcsoftSuperLowlightRaw::setSuperNightRawValue(SuperNightRawValue *pSuperNightRawValue,
                                                       MUInt32 input_num)
{
    if (MNull == pSuperNightRawValue || pSuperNightRawValue->validFrameCount != input_num) {
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: NODESLL, pSuperNightRawValue parameter NULL\n");
        return MERR_INVALID_PARAM;
    }

    for (int i = 0; i < pSuperNightRawValue->validFrameCount; i++) {
        m_superNightRawValue.Ev[i] = pSuperNightRawValue->Ev[i];
        m_superNightRawValue.ISPGain[i] = pSuperNightRawValue->ISPGain[i];
        m_superNightRawValue.Shutter[i] = pSuperNightRawValue->Shutter[i];
        m_superNightRawValue.SensorGain[i] = pSuperNightRawValue->SensorGain[i];
    }

    return MOK;
}

MRESULT ArcsoftSuperLowlightRaw::processPrepare(MInt32 *input_fd, ASVLOFFSCREEN *input,
                                                MInt32 camState, MInt32 inputIndex,
                                                MInt32 input_Num, ARC_SN_RAWINFO *rawInfo)
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
    nSNInputInfo.InputImagesEV[0] = rawInfo->i32EV[0];
    nSNInputInfo.i32InputFd[0] = (MInt32)(*(input_fd));
    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]: NODESLL, superlowlight input fd[%d]=%d ev[%d]=%f from input[%d]=%d "
          "input_fd[%d]=%d",
          inputIndex, nSNInputInfo.i32InputFd[0], inputIndex, nSNInputInfo.InputImagesEV[0],
          inputIndex, rawInfo->i32EV[0], inputIndex, (*(input_fd)));

    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]: NODESLL, frame[%d] fISPGain=%f SensorGain=%f Shutter=%f EV=%d BLS:[%d, %d, "
          "%d, %d]",
          inputIndex, rawInfo->fISPGain, rawInfo->fSensorGain, rawInfo->fShutter, rawInfo->i32EV[0],
          rawInfo->i32BlackLevel[0], rawInfo->i32BlackLevel[1], rawInfo->i32BlackLevel[2],
          rawInfo->i32BlackLevel[3]);
    if (m_isPreprocessSucess) {
        ret = ARC_SN_AddOneInputInfo(m_hSuperLowlightRawEngine, rawInfo, &nSNInputInfo);
    }
    return ret;
}

MRESULT ArcsoftSuperLowlightRaw::processCorrectImage(MInt32 *output_fd, ASVLOFFSCREEN *output,
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
            m_hSuperLowlightRawEngine, // [in] The handle for ArcSoft Super Night
            normalRawInfo,             // [in] RawInfo of the frame, which from camera system
            &nSNResultInfo             // [in & out]tansfer in the input[0], do OB andr LSC
        );
    }
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: NODESLL, bEnableLSC=%d correctImage ret=%d\n",
          normalRawInfo->bEnableLSC, ret);
    return ret;
}

MRESULT ArcsoftSuperLowlightRaw::processAlgo(MInt32 *output_fd, ASVLOFFSCREEN *output,
                                             ASVLOFFSCREEN *normalInput, MInt32 camState,
                                             MInt32 sceneMode, MRECT *pOutImgRect,
                                             ARC_SN_FACEINFO *faceinfo, MInt32 *correctImageRet,
                                             ARC_SN_RAWINFO *normalRawInfo, MInt32 i32EVoffset,
                                             MInt32 i32DeviceOrientation)
{
    MRESULT ret = INIT_VALUE;
    ARC_SN_INPUTINFO nSNResultInfo;
    MMemSet(&nSNResultInfo, 0, sizeof(ARC_SN_INPUTINFO));

    if (mImageCache.ppu8Plane[0] == NULL) {
        mImageCache.i32Width = normalInput->i32Width;
        mImageCache.i32Height = normalInput->i32Height;
        mImageCache.u32PixelArrayFormat = normalInput->u32PixelArrayFormat;
        mImageCache.pi32Pitch[0] = normalInput->pi32Pitch[0];
        mImageCache.pi32Pitch[1] = normalInput->pi32Pitch[1];
        mImageCache.pi32Pitch[2] = normalInput->pi32Pitch[2];
        mImageCache.ppu8Plane[0] =
            (unsigned char *)malloc(normalInput->i32Height * normalInput->pi32Pitch[0]);

        if (mImageCache.ppu8Plane[0] == NULL) {
            MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: NODESLL,  malloc failed for mImageCache\n");
        }
    }
    CopyImage(&mImageCache, normalInput);
    nSNResultInfo.i32ImgNum = 1;
    nSNResultInfo.i32CameraState = camState;
    nSNResultInfo.i32CurIndex = nSNResultInfo.i32ImgNum - 1;
    nSNResultInfo.i32InputFd[0] = (MInt32)(*output_fd);

#if USE_INTERNAL_BUFFER
    FillInternalImage(&mImageOutput, output, FLAG_FILL_METADATA);
    memcpy(&nSNResultInfo.InputImages[0], &mImageOutput, sizeof(ASVLOFFSCREEN));
#else
    setOffScreen(&nSNResultInfo.InputImages[0], output);
#endif

    if (m_isPreprocessSucess == false) {
        CopyImage(output, &mImageCache);
        if (1 == normalRawInfo->bEnableLSC) {
            *correctImageRet = ARC_SN_CorrectImage(
                m_hSuperLowlightRawEngine, // [in] The handle for ArcSoft Super Night
                normalRawInfo,             // [in] RawInfo of the frame, which from camera system
                &nSNResultInfo             // [in & out]tansfer in the input[0], do OB andr LSC
            );
        }
        MLOGE(Mia2LogGroupPlugin,
              "[ARCSOFT]: NODESLL, supernight preprocess error,correctImage ret=%d\n",
              *correctImageRet);
        {
#if USE_INTERNAL_BUFFER
            OutputInternalImage(output, &nSNResultInfo.InputImages[0]);
#endif
        }
    } else {
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: NODESLL, ARC_SN_Process before\n");

        ret = ARC_SN_Process(m_hSuperLowlightRawEngine, faceinfo,
                             &nSNResultInfo, // output image
                             MNull,          //&nParam,          //dont use it anymore
                             sceneMode,      // ARC_SN_SCENEMODE_LOWLIGHT
                             pOutImgRect,    // output rect
                             SuperNightProcessCallback, (MVoid *)this, i32EVoffset,
                             i32DeviceOrientation);

        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]: NODESLL, ARC_SN_Process after ret=%ld, outImageRect[%d,%d,%d,%d]\n", ret,
              pOutImgRect->left, pOutImgRect->top, pOutImgRect->right, pOutImgRect->bottom);
        if (MOK != ret && MERR_USER_CANCEL != ret && MERR_INVALID_PARAM != ret && 14 != ret) {
            if (1 == normalRawInfo->bEnableLSC) {
                CopyImage(output, &mImageCache);
                *correctImageRet = ARC_SN_CorrectImage(
                    m_hSuperLowlightRawEngine, // [in] The handle for ArcSoft Super Night
                    normalRawInfo, // [in] RawInfo of the frame, which from camera system
                    &nSNResultInfo // [in & out]tansfer in the input[0], do OB andr LSC
                );
                MLOGI(Mia2LogGroupPlugin,
                      "[ARCSOFT]: NODESLL, super lowlight Run Error, enableLSC "
                      "%ld CorrectImage ret=%d\n",
                      normalRawInfo->bEnableLSC, (*correctImageRet));
            }
        } else {
#if USE_INTERNAL_BUFFER
            OutputInternalImage(output, &nSNResultInfo.InputImages[0]);
#endif
        }
    }
    return ret;
}

MRESULT ArcsoftSuperLowlightRaw::process(MInt32 *input_fd, ASVLOFFSCREEN *input, MInt32 *output_fd,
                                         ASVLOFFSCREEN *output, MRECT *pOutImgRect,
                                         MInt32 *pExposure, MUInt32 input_num, MInt32 sceneMode,
                                         MInt32 camState, ARC_SN_RAWINFO *rawInfo,
                                         ARC_SN_FACEINFO *faceinfo, MInt32 *correctImageRet)
{
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
    *correctImageRet = INIT_VALUE;

    if (input == MNull || output == MNull || pOutImgRect == MNull || pExposure == MNull ||
        input_num <= 0) {
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: NODESLL, process input parameter error\n");
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
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: iIndexNormalEV %d\n", iIndexNormalEV);

    if (m_hSuperLowlightRawEngine == MNull) {
        // Todo, NODESLL, need check init/uninit function
        MLOGE(Mia2LogGroupPlugin,
              "[ARCSOFT]: NODESLL, supperlowlight engine hasn't been initialized.");
        CopyImage(output, &input[iIndexNormalEV]);
        return MERR_UNKNOWN;
    }

    nSNInputInfo.i32ImgNum = input_num;
    nSNInputInfo.i32CameraState = camState; // ARC_SN_CAMERA_STATE_UNKNOW,ARC_SN_CAMERA_STATE_HAND
    for (MUInt32 i = 0; i < input_num; i++) // Initialize m_InputFrames.InputImages
    {
#if USE_INTERNAL_BUFFER
        FillInternalImage(&mImageInput[i], input + i);
        memcpy(&nSNInputInfo.InputImages[0], &mImageInput[i], sizeof(ASVLOFFSCREEN));
#else
        setOffScreen(&nSNInputInfo.InputImages[0], input + i);
#endif
        // Todo, NODESLL, need to check i32CurIndex;
        nSNInputInfo.i32CurIndex = i;
        // Todo, NODESLL, need to check the InputImagesEV
        nSNInputInfo.InputImagesEV[0] = (float)(*(pExposure + i));
        // Todo, NODESLL, need to check the FD handle
        nSNInputInfo.i32InputFd[0] = (MInt32)(*(input_fd + i));
        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]: NODESLL, superlowlight input fd[%d]=%d ev[%d]=%f from input[%d]=%d "
              "input_fd[%d]=%d",
              i, nSNInputInfo.i32InputFd[0], i, nSNInputInfo.InputImagesEV[0], i,
              (*(pExposure + i)), i, (*(input_fd + i)));

        rawInfo->fISPGain = m_superNightRawValue.ISPGain[i];
        rawInfo->fShutter = m_superNightRawValue.Shutter[i];
        rawInfo->i32EV[i] = m_superNightRawValue.Ev[i];
        rawInfo->fSensorGain = m_superNightRawValue.SensorGain[i];
        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]: NODESLL, frame[%d] fISPGain=%f SensorGain=%f Shutter=%f EV=%d", i,
              m_superNightRawValue.ISPGain[i], m_superNightRawValue.SensorGain[i],
              m_superNightRawValue.Shutter[i], m_superNightRawValue.Ev[i]);
        if (m_isPreprocessSucess) {
            ARC_SN_AddOneInputInfo(m_hSuperLowlightRawEngine, rawInfo, &nSNInputInfo);
        }
    }

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
            MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: NODESLL,  malloc failed for mImageCache\n");
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
        if (1 == rawInfo->bEnableLSC) {
            *correctImageRet = ARC_SN_CorrectImage(
                m_hSuperLowlightRawEngine, // [in] The handle for ArcSoft Super Night
                rawInfo,                   // [in] RawInfo of the frame, which from camera system
                &nSNResultInfo             // [in & out]tansfer in the input[0], do OB andr LSC
            );
        }
        MLOGE(
            Mia2LogGroupPlugin,
            "[ARCSOFT]: NODESLL, supernight preprocess error , return cacheImage index %d ret=%d\n",
            iIndexNormalEV, *correctImageRet);
        {
#if USE_INTERNAL_BUFFER
            OutputInternalImage(output, &nSNResultInfo.InputImages[0]);
#endif
        }
    } else {
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: NODESLL, ARC_SN_Process before\n");
        ret = ARC_SN_Process(m_hSuperLowlightRawEngine, faceinfo,
                             &nSNResultInfo, // output image
                             MNull,          //&nParam,          //dont use it anymore
                             sceneMode,      // ARC_SN_SCENEMODE_LOWLIGHT
                             pOutImgRect,    // output rect
                             SuperNightProcessCallback, (MVoid *)this);

        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]: NODESLL, ARC_SN_Process after ret=%ld, outImageRect[%d,%d,%d,%d]\n", ret,
              pOutImgRect->left, pOutImgRect->top, pOutImgRect->right, pOutImgRect->bottom);
        if (MOK != ret && MERR_USER_CANCEL != ret) {
            if (1 == rawInfo->bEnableLSC) {
                CopyImage(output, &mImageCache);
                *correctImageRet = ARC_SN_CorrectImage(
                    m_hSuperLowlightRawEngine, // [in] The handle for ArcSoft Super Night
                    rawInfo,       // [in] RawInfo of the frame, which from camera system
                    &nSNResultInfo // [in & out]tansfer in the input[0], do OB andr LSC
                );
                MLOGE(Mia2LogGroupPlugin,
                      "[ARCSOFT]: NODESLL, super lowlight Run Error, return index %d enableLSC "
                      "%ld CorrectImage ret=%d\n",
                      iIndexNormalEV, rawInfo->bEnableLSC, (*correctImageRet));
            }
        } else {
#if USE_INTERNAL_BUFFER
            OutputInternalImage(output, &nSNResultInfo.InputImages[0]);
#endif
        }
    }

    return ret;
}

void ArcsoftSuperLowlightRaw::CopyImage(ASVLOFFSCREEN *pDstImg, ASVLOFFSCREEN *pSrcImg)
{
    if (pSrcImg == NULL || pDstImg == NULL) {
        MLOGE(Mia2LogGroupPlugin,
              "[ARCSOFT]: NODESLL, CopyImage intput parameter error, pDstImg = %p, pSrcImg = %p",
              pDstImg, pSrcImg);
        return;
    }
    if (pDstImg->u32PixelArrayFormat != pSrcImg->u32PixelArrayFormat) {
        MLOGE(Mia2LogGroupPlugin,
              "[ARCSOFT]: NODESLL, super lowlight CopyImage error, the format is not same, dst "
              "format %d, src format %d",
              pDstImg->u32PixelArrayFormat, pSrcImg->u32PixelArrayFormat);
        return;
    }

    int i;
    unsigned char *l_pSrc = NULL;
    unsigned char *l_pDst = NULL;

    l_pSrc = pSrcImg->ppu8Plane[0];
    l_pDst = pDstImg->ppu8Plane[0];
    for (i = 0; i < pDstImg->i32Height; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->pi32Pitch[0]);
        l_pDst += pDstImg->pi32Pitch[0];
        l_pSrc += pSrcImg->pi32Pitch[0];
    }
}

void ArcsoftSuperLowlightRaw::InitInternalBuffer()
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

void ArcsoftSuperLowlightRaw::UninitInternalBuffer()
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

void ArcsoftSuperLowlightRaw::setOffScreen(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage)
{
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

void ArcsoftSuperLowlightRaw::FillInternalImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage,
                                                int flag)
{
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
        MLOGE(Mia2LogGroupPlugin,
              "[ARCSOFT]: NODESLL, FillInternalImage, buffer size is smaller. org=%d, new=%d\n",
              INTERNAL_BUFFER_PIXEL_SIZE, pDstImage->pi32Pitch[0] * pDstImage->i32Height * 3 / 2);
    }

    if (pDstImage->ppu8Plane[0] && flag == FLAG_FILL_ALL) {
        CopyImage(pDstImage, pSrcImage);
    }
}

void ArcsoftSuperLowlightRaw::OutputInternalImage(ASVLOFFSCREEN *pDstImage,
                                                  ASVLOFFSCREEN *pSrcImage, int flag)
{
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

void ArcsoftSuperLowlightRaw::ReleaseImage(ASVLOFFSCREEN *pImage)
{
    if (pImage->ppu8Plane[0])
        free(pImage->ppu8Plane[0]);
    pImage->ppu8Plane[0] = NULL;
}

bool ArcsoftSuperLowlightRaw::putEnv(int maxloop)
{
    char *curAdspEnvPath = NULL;
    int retEnv = 0;
    for (int i = 0; i <= maxloop; i++) {
        retEnv = putenv(AdspEnvPath);
        curAdspEnvPath = getenv("ADSP_LIBRARY_PATH");
        if (NULL != curAdspEnvPath && NULL != strstr(AdspEnvPath, curAdspEnvPath)) {
            MLOGI(Mia2LogGroupPlugin,
                  "[ARCSOFT]: put env success, cur adsp env path:%s (Reference path:%s)",
                  curAdspEnvPath, AdspEnvPath);
            return true;
        } else {
            MLOGI(
                Mia2LogGroupPlugin,
                "[ARCSOFT]: put env fail, cur adsp env path:%s (Reference path:%s) retry times:%d",
                curAdspEnvPath, AdspEnvPath, i);
        }
    }
    return false;
}