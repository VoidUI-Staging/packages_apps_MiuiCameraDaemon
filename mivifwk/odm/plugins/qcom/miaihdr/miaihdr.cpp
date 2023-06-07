#include "miaihdr.h"

#include <cutils/properties.h>
#include <stdio.h>
#include <unistd.h>
#include <utils/Log.h>

#include <mutex>
#include <thread>

#include "amcomdef.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "xmi_low_light_hdr.h"

#define MAX_BUF_LEN PROPERTY_VALUE_MAX
#define MAX_FD_NUM  4
#undef LOG_TAG
#define LOG_TAG "MIA_N_MHDR"

static const int32_t sIsBypassHdrAlgo =
    property_get_int32("persist.vendor.camera.capture.bypass.hdralgo", 0);

static const int32_t shdrdebug =
    property_get_int32("persist.vendor.camera.capture.burst.hdrdebug", 0);

MHandle sHdrExpEffect = MNull;
std::mutex g_mutex;

namespace mialgo2 {

MiAIHDR::MiAIHDR() : m_isCrop(false), m_flustStatus(false)
{
    memset(aecExposureData, 0, sizeof(aecExposureData));
    memset(awbGainParams, 0, sizeof(awbGainParams));
    memset(extraAecExposureData, 0, sizeof(extraAecExposureData));
    memset(&m_userData, 0, sizeof(m_userData));
}

MiAIHDR::~MiAIHDR()
{
    m_isCrop = false;
    m_flustStatus = false;
    memset(&m_userData, 0, sizeof(m_userData));
}

void MiAIHDR::init()
{
    std::unique_lock<std::mutex> mtx_locker(g_mutex);
    load_config(ArcHDRCommonAEParams, ArcHDRParams);
    memset(&m_userData, 0, sizeof(ARC_HDR_USERDATA));
    m_isCrop = false;
    m_flustStatus = false;

    if (sHdrExpEffect == MNull) {
        MLOGI(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: XMI_HDR_Init\n");
        MRESULT ret = XMI_HDR_Init(&sHdrExpEffect, 0, ARC_TYPE_QUALITY_FIRST);
        if (ret != MOK) {
            MLOGE(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: XMI_HDR_Init error\n");
            return;
        }
    } else {
        MLOGI(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: XMI_HDR_Init not called\n");
    }
}

void MiAIHDR::destroy()
{
    std::unique_lock<std::mutex> mtx_locker(g_mutex);
    if (sHdrExpEffect != MNull) {
        MLOGI(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: XMI_HDR_Uninit\n");
        int ret = XMI_HDR_Uninit(&sHdrExpEffect);
        sHdrExpEffect = MNull;
        if (ret != MOK)
            MLOGE(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: XMI_HDR_Uninit Error\n");
    } else {
        MLOGI(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: XMI_HDR_Uninit not called.\n");
    }
}

MVoid MiAIHDR::setOffScreen(ASVLOFFSCREEN *pImg, struct MiImageBuffer *miBuf)
{
    if (miBuf->format == CAM_FORMAT_YUV_420_NV12)
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

MVoid MiAIHDR::setOffScreenHDR(HDR_YUV_OFFSCREEN *pImg, struct MiImageBuffer *miBuf)
{
    if (miBuf->format == CAM_FORMAT_YUV_420_NV12)
        pImg->u32PixelArrayFormat = ASVL_PAF_NV12;
    else
        pImg->u32PixelArrayFormat = ASVL_PAF_NV21;

    pImg->i32Width = miBuf->width;
    pImg->i32Height = miBuf->height;
    pImg->pi32Pitch[0] = miBuf->stride;
    pImg->pi32Pitch[1] = miBuf->stride;
    pImg->ppu8Plane[0] = miBuf->plane[0];
    pImg->ppu8Plane[1] = miBuf->plane[1];
    pImg->fd[0] = miBuf->fd[0];
    pImg->fd[1] = miBuf->fd[1];
}

MRESULT MiAIHDR::hdrProcessCb(MLong lProgress, // Not impletement
                              MLong lStatus,   // Not impletement
                              MVoid *pParam)
{
    lStatus = 0;
    lProgress = 0;
    MiAIHDR *pMiAiHdr = (MiAIHDR *)pParam;
    ;
    if (pMiAiHdr == NULL || pMiAiHdr->m_flustStatus) {
        MLOGE(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR] %s cancel", __func__);
        return MERR_USER_CANCEL;
    }

    return MERR_NONE;
}

void MiAIHDR::process(struct MiImageBuffer *input, struct MiImageBuffer *output, int input_num,
                      int *ev, ARC_LLHDR_AEINFO &mAeInfo, int camMode, HDRMetaData hdrMetaData,
                      MiAECExposureData *aecExposureData, MiAWBGainParams *awbGainParams)
{
    std::unique_lock<std::mutex> mtx_locker(g_mutex);
    int32_t isBypassHdrAlgo = sIsBypassHdrAlgo;
    int hdr_mode = MODE_HDR;

    ARC_HDR_AEINFO arcHdrAEInfo = {0};
    HDR_EVLIST_AEC_INFO aeInfo = {0};
    aeInfo.pAEInfo = &arcHdrAEInfo;

    struct tm *timenow;
    time_t now;
    time(&now);
    timenow = localtime(&now);

    for (int i = 0; i < input_num; i++) {
        MLOGD(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR] process ev[%d] = %d", i, ev[i]);
    }
    for (int i = 0; i < input_num; i++) {
        aeInfo.ev_list[i] = ev[i];
        aeInfo.expossure_time[i] = aecExposureData[i].exposureTime;
        aeInfo.sensitivity[i] = aecExposureData[i].sensitivity;
        aeInfo.total_gain[i] = aecExposureData[i].linearGain;
        ALOGD("[MIALGO_HDR][MIAI_HDR] process ev[%d] = %d", i, ev[i]);
    }

    switch (input_num) {
    case 2:
        hdr_mode = MODE_SRHDR;
        break;
    case 3:
        hdr_mode = MODE_HDR;
        break;
    case 4:
    default:
        hdr_mode = MODE_LLHDR;
    }
    if (0 == isBypassHdrAlgo) {
        switch (hdr_mode) {
        case MODE_HDR:
            process_hdr_xmi(input, output, input_num, mAeInfo, camMode, hdrMetaData, aeInfo);
            break;
        case MODE_LLHDR:
            process_low_light_hdr_xmi(input, output, input_num, mAeInfo, camMode, hdrMetaData);
            break;
            //     case MODE_SRHDR:
            //         hdrMetaData.config.hdr_type = MIAI_HDR_SR;
            //         process_hdr_sr_xmi(input, output, input_num, mAeInfo, camMode, hdrMetaData);
        }
    } else // set isBypassHdrAlgo=1/2/3/4, and return i'st input frame
    {
        if (1 <= isBypassHdrAlgo && input_num >= isBypassHdrAlgo) {
            PluginUtils::miCopyBuffer(output, &input[isBypassHdrAlgo - 1]);
        } else {
            PluginUtils::miCopyBuffer(output, &input[0]);
        }
    }
}

void MiAIHDR::process_hdr_xmi(struct MiImageBuffer *input, struct MiImageBuffer *output,
                              int input_num, ARC_LLHDR_AEINFO &mAeInfo, int camMode,
                              HDRMetaData hdrMetaData, HDR_EVLIST_AEC_INFO &aeInfo)
{
    ARC_HDR_PARAM m_Param;
    HDR_YUV_INPUTINFO m_InputInfo;

    aeInfo.pAEInfo->fLuxIndex = mAeInfo.fLuxIndex;
    aeInfo.pAEInfo->fISPGain = mAeInfo.fISPGain;
    aeInfo.pAEInfo->fSensorGain = mAeInfo.fSensorGain;
    aeInfo.pAEInfo->fADRCGain = mAeInfo.fADRCGain;
    aeInfo.pAEInfo->fADRCGainMax = mAeInfo.fADRCGainMax;
    aeInfo.pAEInfo->fADRCGainMin = mAeInfo.fADRCGainMin;

    (void *)output;
    MRESULT ret = MOK;
    MLOGD(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: proc version=%s",
          XMI_HDR_GetVersion()->Version);
    double init_start = nowMSec();
    if (sHdrExpEffect == MNull) {
        ret = XMI_HDR_Init(&sHdrExpEffect, 0, ARC_TYPE_QUALITY_FIRST);
        MLOGI(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: hdr reinit ret=%ld", ret);
    }
    MLOGI(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR] XMI_HDR_Init cost:%.2f",
          nowMSec() - init_start);
    if (m_isCrop)
        XMI_HDR_SetCallback(sHdrExpEffect, NULL, &m_userData);

    ret = XMI_HDR_GetDefaultParam(&m_Param);

    MMemSet(&m_InputInfo, 0, sizeof(m_InputInfo));
    m_InputInfo.i32ImgNum = input_num;
    int fd_index = 0;
    for (int i = 0; i < input_num; i++) // Initialize m_InputInfo.InputImages
    {
        for (int j = 0; j < MAX_FD_NUM; j++) {
            hdrMetaData.config.fd[fd_index] = (input + i)->fd[j];
            fd_index++;
        }
        setOffScreenHDR(&m_InputInfo.InputImages[i], input + i);
        // ret = XMI_HDR_PreProcess(sHdrExpEffect, &m_InputInfo, &aeInfo, i);
    }

    for (int i = 0; i < MAX_FD_NUM; i++) {
        hdrMetaData.config.fd[fd_index] = output->fd[i];
        fd_index++;
    }

    int index = HDR_PARAM_INDEX_REAR;
    if (camMode == ARC_MODE_FRONT_CAMERA) {
        index = HDR_PARAM_INDEX_FRONT;
    }

    m_Param.i32ToneLength = ArcHDRParams[index].tonelen;
    m_Param.i32Brightness = ArcHDRParams[index].brightness;
    m_Param.i32Saturation = ArcHDRParams[index].saturation;
    m_Param.i32CurveContrast = ArcHDRParams[index].contrast;

    // support (ImgNumber == 1,2,3), if ImgNumber == 2, input order is normal/low, or
    // normal/over.
    HDR_YUV_OFFSCREEN outputImage = {0};

#ifndef ARC_USE_ALL_ION_BUFFER
    setOffScreenHDR(&outputImage, &input[input_num - 1]); // please select the ev != zero frames
#else
    setOffScreenHDR(&outputImage, output); // please select the ev != zero frames
#endif
    // hdrdebug = 1 2 or 3 means output is the first second or third frame.
    if (shdrdebug > 0 && shdrdebug < 4) {
        int32_t returnimgID = shdrdebug - 1;
        MLOGI(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: By pass HDR, return normal Img %d \n",
              returnimgID);
        PluginUtils::miCopyBuffer(output, &input[returnimgID]);
    } else {
        double start = nowMSec();
        MLOGD(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: crop size: %d %d %d %d \n",
              hdrMetaData.config.cropSize[0], hdrMetaData.config.cropSize[1],
              hdrMetaData.config.cropSize[2], hdrMetaData.config.cropSize[3]);
        ret = XMI_HDR_Process(sHdrExpEffect, &m_InputInfo, &aeInfo, &m_Param, &outputImage,
                              &hdrMetaData, MiAIHDR::hdrProcessCb, (void *)this, NULL);
        double end = nowMSec();
        MLOGD(Mia2LogGroupPlugin,
              "[MIALGO_HDR][MIAI_HDR]:HDR_Process ret=%ld cost=%.2f tonelen=%d, bright=%d, "
              "saturation=%d, contrast=%d\n",
              ret, end - start, m_Param.i32ToneLength, m_Param.i32Brightness, m_Param.i32Saturation,
              m_Param.i32CurveContrast);
        memcpy(&(mAeInfo.algo_params), &(aeInfo.pAEInfo->algo_params),
               sizeof(MIAI_HDR_ALGO_PARAMS));

        // generate hdr exif info
        char exifbuf[64] = {0};
        snprintf(exifbuf, sizeof(exifbuf), "ret:%ld cost:%.2f", ret, end - start);
        m_hdrAlgoExif = exifbuf;
    }

    if (ret != MOK && ret != HDR_ERR_GHOST_IN_OVERREGION) {
        MLOGI(Mia2LogGroupPlugin,
              "[CAM_LOG_SYSTEM][MIAI_HDR]: process fail ret=%ld, return normal Img", ret);
        PluginUtils::miCopyBuffer(output, &input[0]);
    }
#ifndef ARC_USE_ALL_ION_BUFFER
    else {
        PluginUtils::miCopyBuffer(output, &input[input_num - 1]);
    }
#endif

    if (m_isCrop)
        MLOGD(Mia2LogGroupPlugin,
              "[MIALGO_HDR][MIAI_HDR]:HDR_Process setCallback m_userData.rect: left=%d, top=%d, "
              "right=%d, bottom=%d",
              m_userData.rtCropRect.left, m_userData.rtCropRect.top, m_userData.rtCropRect.right,
              m_userData.rtCropRect.bottom);
}

void MiAIHDR::process_low_light_hdr_xmi(struct MiImageBuffer *input, struct MiImageBuffer *output,
                                        int input_num, ARC_LLHDR_AEINFO &mAeInfo, int camMode,
                                        HDRMetaData hdrMetaData)
{
    MRESULT ret = MOK;
    MHandle phExpEffect = MNull;
    ASVLOFFSCREEN SrcImgs[ARC_LLHDR_MAX_INPUT_IMAGE_NUM];
    ARC_LLHDR_PARAM m_Param = {0};
    ARC_LLHDR_FACEINFO faceInfo = {0};
    ARC_LLHDR_INPUTINFO m_InputInfo;

    ASVLOFFSCREEN outputImage = {0};
#ifndef ARC_USE_ALL_ION_BUFFER
    struct MiImageBuffer tmpMiBuffer;
    mibuffer_alloc_copy(&tmpMiBuffer, &input[0]);
    setOffScreen(&outputImage, &tmpMiBuffer); // please select the ev != zero frames
#else
    setOffScreen(&outputImage, output);    // please select the ev != zero frames
#endif
    ret = XMI_LLHDR_Init(&phExpEffect);
    if (ret != MOK) {
        MLOGE(Mia2LogGroupPlugin, "[AMIGLGO_LLHDR][MIAI_HDR]: ARC_LLHDR_Init error\n");
        return;
    }

    MMemSet(&m_InputInfo, 0, sizeof(m_InputInfo));
    m_InputInfo.i32ImgNum = input_num;
    for (int i = 0; i < input_num; i++) {
        memset(&SrcImgs[i], 0, sizeof(ASVLOFFSCREEN));
        setOffScreen(&SrcImgs[i], input + i);
        m_InputInfo.pImages[i] = &SrcImgs[i];
    }

    camMode = ARC_LLHDR_CAMERA_REAR;
    ret = XMI_LLHDR_GetDefaultParam(&m_Param, mAeInfo.iso, camMode);
    XMI_LLHDR_SetDRCGain(phExpEffect, mAeInfo.fADRCGain);
    XMI_LLHDR_SetISPGain(phExpEffect, mAeInfo.fISPGain);
    m_Param.i32Intensity = 65;
    // m_Param.i32SharpenIntensity = 50;
    MInt32 nRetIndex = -1;
    double start = nowMSec();
    ret =
        XMI_LLHDR_Process(phExpEffect, &m_InputInfo, &faceInfo, &outputImage, &nRetIndex, &m_Param,
                          mAeInfo.iso, &hdrMetaData, MiAIHDR::hdrProcessCb, (void *)this, NULL);
    double end = nowMSec();
    if (ret != MOK) {
        MLOGE(Mia2LogGroupPlugin, "[AMIGLGO_LLHDR][MIAI_HDR]: Run  Error, return normal Img\n");
        PluginUtils::miCopyBuffer(output, &input[0]);
    }
#ifndef ARC_USE_ALL_ION_BUFFER
    else {
        PluginUtils::miCopyBuffer(output, &tmpMiBuffer);
    }
    mibuffer_release(&tmpMiBuffer);
#endif
    MLOGE(Mia2LogGroupPlugin,
          "[AMIGLGO_LLHDR][MIAI_HDR]: ARC_LLHDR_Process ret=%ld-nRetIndex=%d-cost=%.2f\n", ret,
          nRetIndex, end - start);
    ret = XMI_LLHDR_Uninit(&phExpEffect);
    if (ret != MOK)
        MLOGE(Mia2LogGroupPlugin, "[AMIGLGO_LLHDR][MIAI_HDR]: Uninit Error\n");
}

// void MiAIHDR::process_hdr_sr_xmi(struct MiImageBuffer *input, struct MiImageBuffer *output,
//                                  int input_num, ARC_LLHDR_AEINFO &mAeInfo, int camMode,
//                                  HDRMetaData hdrMetaData)
// {
//     ARC_HDR_PARAM m_Param;
//     ARC_HDR_INPUTINFO m_InputInfo;
//     MHandle sHdrExpEffect = MNull;

//     ARC_HDR_AEINFO aeInfo = {0};
//     aeInfo.fLuxIndex = mAeInfo.fLuxIndex;
//     aeInfo.fISPGain = mAeInfo.fISPGain;
//     aeInfo.fSensorGain = mAeInfo.fSensorGain;
//     aeInfo.fADRCGain = mAeInfo.fADRCGain;
//     aeInfo.fADRCGainMax = mAeInfo.fADRCGainMax;
//     aeInfo.fADRCGainMin = mAeInfo.fADRCGainMin;

//     (void *)output;
//     MLOGD(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: proc version=%s\n",
//           XMI_HDRSR_GetVersion()->Version);

//     MRESULT ret = XMI_HDRSR_Init(&sHdrExpEffect, camMode, ARC_TYPE_QUALITY_FIRST);
//     if (ret != MOK) {
//         MLOGE(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: ARC_HDR_Init error\n");
//         return;
//     }
//     if (m_isCrop)
//         XMI_HDRSR_SetCallback(sHdrExpEffect, NULL, &m_userData);

//     ret = XMI_HDRSR_GetDefaultParam(&m_Param);

//     MMemSet(&m_InputInfo, 0, sizeof(m_InputInfo));
//     m_InputInfo.i32ImgNum = input_num;

//     for (int i = 0; i < input_num; i++) // Initialize m_InputInfo.InputImages
//     {
//         setOffScreen(&m_InputInfo.InputImages[i], input + i);
//         // ret = XMI_HDR_PreProcess(sHdrExpEffect, &m_InputInfo, &aeInfo, i);
//     }

//     int index = HDR_PARAM_INDEX_REAR;
//     if (camMode == ARC_MODE_FRONT_CAMERA) {
//         index = HDR_PARAM_INDEX_FRONT;
//     }

//     m_Param.i32ToneLength = ArcHDRParams[index].tonelen;
//     m_Param.i32Brightness = ArcHDRParams[index].brightness;
//     m_Param.i32Saturation = ArcHDRParams[index].saturation;
//     m_Param.i32CurveContrast = ArcHDRParams[index].contrast;

//     // support (ImgNumber == 1,2,3), if ImgNumber == 2, input order is normal/low, or
//     normal/over. ASVLOFFSCREEN outputImage = {0};

// #ifndef ARC_USE_ALL_ION_BUFFER
//     setOffScreen(&outputImage, &input[input_num - 1]); // please select the ev != zero frames
// #else
//     setOffScreen(&outputImage, output); // please select the ev != zero frames
// #endif
//     char prop[PROPERTY_VALUE_MAX];
//     memset(prop, 0, sizeof(prop));
//     property_get("persist.vendor.camera.capture.burst.hdrdebug", prop, "0");
//     int hdrdebug = atoi(prop);
//     // hdrdebug = 1 2 or 3 means output is the first second or third frame.
//     if (hdrdebug > 0 && hdrdebug < 4) {
//         int returnimgID = hdrdebug - 1;
//         MLOGI(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: By pass HDR, return normal Img %d \n",
//               returnimgID);
//         PluginUtils::miCopyBuffer(output, &input[returnimgID]);
//     } else {
//         double start = nowMSec();
//         ret = XMI_HDRSR_Process(sHdrExpEffect, &m_InputInfo, &aeInfo, &m_Param, &outputImage,
//                                 &hdrMetaData, MiAIHDR::hdrProcessCb, (void *)this, NULL);
//         double end = nowMSec();
//         MLOGD(Mia2LogGroupPlugin,
//               "[MIALGO_HDR][MIAI_HDR]:HDR_Process ret=%ld cost=%.2f tonelen=%d, bright=%d, "
//               "saturation=%d, contrast=%d\n",
//               ret, end - start, m_Param.i32ToneLength, m_Param.i32Brightness,
//               m_Param.i32Saturation, m_Param.i32CurveContrast);
//     }
//     if (ret != MOK && ret != HDR_ERR_GHOST_IN_OVERREGION) {
//         MLOGE(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: Run  Error, return normal Img\n");
//         PluginUtils::miCopyBuffer(output, &input[0]);
//     }
//     //mAeInfo.algo_params = aeInfo.algo_params;
// #ifndef ARC_USE_ALL_ION_BUFFER
//     else { PluginUtils::miCopyBuffer(output, &input[input_num - 1]); }
// #endif

//     if (m_isCrop)
//         MLOGD(Mia2LogGroupPlugin,
//               "[MIALGO_HDR][MIAI_HDR]:HDR_Process setCallback m_userData.rect: left=%d, top=%d, "
//               "right=%d, bottom=%d",
//               m_userData.rtCropRect.left, m_userData.rtCropRect.top, m_userData.rtCropRect.right,
//               m_userData.rtCropRect.bottom);

//     ret = XMI_HDRSR_Uninit(&sHdrExpEffect);
//     if (ret != MOK)
//         MLOGE(Mia2LogGroupPlugin, "[MIALGO_HDR][MIAI_HDR]: Uninit Error\n");
// }

void MiAIHDR::load_config(ArcHDRCommonAEParam *pArcAEParams, ArcHDRParam *pArcHDRParam)
{
    if (access(MI_UI9_HDR_PARAMS_FILEPATH, F_OK | R_OK) != 0)
        return;
    FILE *pf = fopen(MI_UI9_HDR_PARAMS_FILEPATH, "rb");
    if (pf == NULL)
        return;
    int i = 0;
    char oneLine[1024];
    unsigned int tmp = 0;
    while (!feof(pf)) {
        memset(oneLine, 0, sizeof(oneLine));
        fgets(oneLine, sizeof(oneLine) - 1, pf);
        if (oneLine[0] != '{' || strlen(oneLine) < 50) {
            continue;
        }
        for (tmp = strlen(oneLine) - 4; tmp < strlen(oneLine); tmp++) {
            if (oneLine[tmp] == '}' || oneLine[tmp] == ',' || oneLine[tmp] == '\r' ||
                oneLine[tmp] == '\n')
                oneLine[tmp] = 0;
        }

        char *recursion = &oneLine[1];
        if (i == 0) {
            pArcAEParams->lux_min = atof(strtok_r(&oneLine[1], ",", &recursion));
            pArcAEParams->lux_max = atof(strtok_r(NULL, ",", &recursion));
            pArcAEParams->checkermode = atoi(strtok_r(NULL, ",", &recursion));
            pArcAEParams->confval_min = atof(strtok_r(NULL, ",", &recursion));
            pArcAEParams->confval_max = atof(strtok_r(NULL, ",", &recursion));
            pArcAEParams->adrc_min = atof(strtok_r(NULL, ",", &recursion));
            pArcAEParams->adrc_max = atof(strtok_r(NULL, ",", &recursion));
        } else {
            pArcHDRParam[i - 1].tonelen = atoi(strtok_r(&oneLine[1], ",", &recursion));
            pArcHDRParam[i - 1].brightness = atoi(strtok_r(NULL, ",", &recursion));
            pArcHDRParam[i - 1].saturation = atoi(strtok_r(NULL, ",", &recursion));
            pArcHDRParam[i - 1].contrast = atoi(strtok_r(NULL, ",", &recursion));
            pArcHDRParam[i - 1].ev[0] = atoi(strtok_r(NULL, ",", &recursion));
            pArcHDRParam[i - 1].ev[1] = atoi(strtok_r(NULL, ",", &recursion));
            pArcHDRParam[i - 1].ev[2] = atoi(strtok_r(NULL, ",", &recursion));
        }
        i++;
        if (i == MAX_CONFIG_ITEM_LINE_CNT)
            break;
    }
    MLOGI(Mia2LogGroupPlugin, "load_config:hdr:common param: %.2f %.2f %d %.2f %.2f %.2f %.2f\n",
          pArcAEParams->lux_min, pArcAEParams->lux_max, pArcAEParams->checkermode,
          pArcAEParams->confval_min, pArcAEParams->confval_max, pArcAEParams->adrc_min,
          pArcAEParams->adrc_max);
    fclose(pf);
}

#ifndef ARC_USE_ALL_ION_BUFFER
void MiAIHDR::mibuffer_alloc_copy(struct MiImageBuffer *dstBuffer, struct MiImageBuffer *srcBuffer)
{
    memcpy(dstBuffer, srcBuffer, sizeof(struct MiImageBuffer));
    dstBuffer->Plane[0] = (unsigned char *)malloc(dstBuffer->height * dstBuffer->Pitch[0] * 3 / 2);
    dstBuffer->Plane[1] = dstBuffer->Plane[0] + dstBuffer->height * dstBuffer->Pitch[0];
    unsigned char *psrc = srcBuffer->Plane[0];
    unsigned char *pdst = dstBuffer->Plane[0];
    for (int h = 0; h < srcBuffer->height; h++) {
        memcpy(pdst, psrc, srcBuffer->width);
        psrc += srcBuffer->Pitch[0];
        pdst += dstBuffer->Pitch[0];
    }
    psrc = srcBuffer->Plane[1];
    pdst = dstBuffer->Plane[1];
    for (int h = 0; h < srcBuffer->height / 2; h++) {
        memcpy(pdst, psrc, srcBuffer->width);
        psrc += srcBuffer->Pitch[1];
        pdst += dstBuffer->Pitch[1];
    }
}
void MiAIHDR::mibuffer_release(struct MiImageBuffer *mibuf)
{
    if (mibuf->Plane[0])
        free(mibuf->Plane[0]);
    mibuf->Plane[0] = NULL;
}
#endif

double MiAIHDR::nowMSec(void)
{
    struct timeval res;
    gettimeofday(&res, NULL);
    return 1000.0 * res.tv_sec + (double)res.tv_usec / 1e3;
}

void MiAIHDR::WriteFileLog(char *file, const char *fmt, ...)
{
    FILE *pf = fopen(file, "wb");
    if (pf) {
        va_list ap;
        char buf[MAX_BUF_LEN] = {0};
        va_start(ap, fmt);
        vsnprintf(buf, MAX_BUF_LEN, fmt, ap);
        va_end(ap);
        fwrite(buf, strlen(buf) * sizeof(char), sizeof(char), pf);
        fclose(pf);
    }
}
ARC_HDR_USERDATA MiAIHDR::getUserData()
{
    return m_userData;
}

} // namespace mialgo2
