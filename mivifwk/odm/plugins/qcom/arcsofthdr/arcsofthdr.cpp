#include "arcsofthdr.h"

#define MAX_BUF_LEN PROPERTY_VALUE_MAX

#undef LOG_TAG
#define LOG_TAG "MIA_N_AHDR"

ArcsoftHdr::ArcsoftHdr() {}

ArcsoftHdr::~ArcsoftHdr()
{
    m_isCrop = false;
}

void ArcsoftHdr::init()
{
    // load_config(ArcHDRCommonAEParams, ArcHDRParams);
    m_isCrop = false;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.algoengine.arcsofthdr.dump", prop, "0");
    m_iIsDumpData = atoi(prop);
    pEV = NULL;
}

MVoid ArcsoftHdr::setOffScreen(ASVLOFFSCREEN *pImg, struct MiImageBuffer *miBuf)
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

void ArcsoftHdr::process(struct MiImageBuffer *input, struct MiImageBuffer *output, int input_num,
                         int *ev, MI_HDR_INFO &mHdrInfo, int camMode)
{
    int hdr_mode = MODE_HDR;
    int processRes = MOK;
    pEV = ev;

    struct tm *timenow;
    time_t now;
    time(&now);
    timenow = localtime(&now);
    struct timeval res;
    gettimeofday(&res, NULL);
    MInt32 msec = (MInt32)res.tv_usec / 1e3;

    for (int i = 0; i < input_num; i++) {
        MLOGD(Mia2LogGroupPlugin, "[MIALGO_HDR][ARCSOFT] process ev[%d] = %d", i, ev[i]);
    }

    if (isLLHDR(input_num, pEV)) {
        hdr_mode = MODE_LLHDR;
    } else {
        hdr_mode = MODE_HDR;
    }
    char metadata[255];
    char prop[255];
    memset(metadata, 0, sizeof(metadata));
    if (m_iIsDumpData) {
        for (int i = 0; i < input_num; i++) {
            memset(prop, 0, sizeof(prop));
            if (hdr_mode == MODE_LLHDR) {
                if (0 == i) {
                    snprintf(metadata, sizeof(metadata),
                             "%sllhdrintput_%02d%02d%02d%02d%02d%03d_%d_%dx%d_ISO%d_ev_%d",
                             PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                             timenow->tm_hour, timenow->tm_min, timenow->tm_sec, (MInt32)msec, i,
                             input[i].width, input[i].height, mHdrInfo.m_AeInfo.iso, ev[i]);
                }

                snprintf(prop, sizeof(prop),
                         "%sllhdrintput_%02d%02d%02d%02d%02d%03d_%d_%dx%d_ISO%d_ev_%d.nv12",
                         PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                         timenow->tm_hour, timenow->tm_min, timenow->tm_sec, (MInt32)msec, i,
                         input[i].width, input[i].height, mHdrInfo.m_AeInfo.iso, ev[i]);
            } else {
                if (0 == i) {
                    snprintf(metadata, sizeof(metadata),
                             "%shdrintput_%02d%02d%02d%02d%02d%03d_%d_%dx%d_ISO%d_ev_%d",
                             PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                             timenow->tm_hour, timenow->tm_min, timenow->tm_sec, (MInt32)msec, i,
                             input[i].width, input[i].height, mHdrInfo.m_AeInfo.iso, ev[i]);
                }

                snprintf(prop, sizeof(prop),
                         "%shdrintput_%02d%02d%02d%02d%02d%03d_%d_%dx%d_ISO%d_ev_%d.nv12",
                         PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                         timenow->tm_hour, timenow->tm_min, timenow->tm_sec, (MInt32)msec, i,
                         input[i].width, input[i].height, mHdrInfo.m_AeInfo.iso, ev[i]);
            }
            PluginUtils::miDumpBuffer(&input[i], prop);
        }
    }

    switch (hdr_mode) {
    case MODE_HDR:
        processRes = process_hdr(input, output, input_num, mHdrInfo, camMode, metadata);
        break;
    case MODE_LLHDR:
        processRes = process_low_light_hdr(input, output, input_num, mHdrInfo, camMode, metadata);
        break;
    }

    if (m_iIsDumpData) {
        property_set("persist.camera.hdr.checkerdump", "1");
        if (hdr_mode == MODE_LLHDR) {
            snprintf(prop, sizeof(prop),
                     "%sllhdroutput_%02d%02d%02d%02d%02d%03d_%d_%dx%d_ISO%d_%d.nv12",
                     PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                     timenow->tm_hour, timenow->tm_min, timenow->tm_sec, (MInt32)msec, input_num,
                     output->width, output->height, mHdrInfo.m_AeInfo.iso, processRes);
        } else {
            snprintf(prop, sizeof(prop),
                     "%shdroutput_%02d%02d%02d%02d%02d%03d_%d_%dx%d_ISO%d_%d.nv12",
                     PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                     timenow->tm_hour, timenow->tm_min, timenow->tm_sec, (MInt32)msec, input_num,
                     output->width, output->height, mHdrInfo.m_AeInfo.iso, processRes);
        }
        PluginUtils::miDumpBuffer(output, prop);
    }
}

int ArcsoftHdr::process_hdr(struct MiImageBuffer *input, struct MiImageBuffer *output,
                            int input_num, MI_HDR_INFO &mHdrInfo, int camMode, char *metadata)
{
    // MInt32 mCameraType = ARC_HDR_CAMERA_WIDE;
    ARC_HDR_PARAM arcParam;
    ARC_HDR_INPUTINFO arcInputInfo;
    MHandle phExpEffect = MNull;
    // to do;
    MInt32 cameraType = ARC_HDR_MODE_WIDE_CAMERA;
    MInt32 sceneType = 0;

    MInt32 nImagesFd = 0; // The FD of Src Images, 0: use cpu buffer, else : use ION buffer
    MInt32 nDstImgFd = 0; // The FD of dest Images, 0: use cpu buffer, else: use ION buffer
    MFloat fEvValue[ARC_HDR_MAX_INPUT_IMAGE_NUM] = {0};
    ARC_HDR_FACEINFO faceInfo;
    // MInt32 mInputMode = ARC_HDR_INPUT_MODE_NLO;
    MInt32 arcISOValue = mHdrInfo.m_AeInfo.iso;

    (void *)output;
    MLOGI(Mia2LogGroupPlugin, "ArcsoftHdr get algorithm library version: %s",
          ARC_HDR_GetVersion()->Version);

    MRESULT ret = ARC_HDR_Init(&phExpEffect, cameraType);
    if (ret != MOK) {
        MLOGE(Mia2LogGroupPlugin, "[MIALGO_HDR][ARCSOFT]: ARC_HDR_Init error\n");
        return ret;
    }

    MMemSet(&arcInputInfo, 0, sizeof(arcInputInfo));
    arcInputInfo.i32ImgNum = input_num;
    // if (camMode == ARC_HDR_CAMERA_FRONT) {
    //     mCameraType = ARC_HDR_CAMERA_FRONT;
    // } else {
    //     mCameraType = ARC_HDR_CAMERA_WIDE;
    // }

    // if (pEV[2] >= 0)
    //     mInputMode = ARC_HDR_INPUT_MODE_NLO;
    // else
    //     mInputMode = ARC_HDR_INPUT_MODE_NLL;

    ARC_HDR_SetSceneType(phExpEffect, sceneType);
    ARC_HDR_GetDefaultParam(phExpEffect, mHdrInfo.m_AeInfo.iso, &arcParam);

    for (int i = 0; i < input_num; i++) {
        setOffScreen(&arcInputInfo.InputImages[i], input + i);
        if (pEV != NULL) {
            arcInputInfo.fEvCompVal[i] = (MFloat)pEV[i] / 6;
        }
        ret = ARC_HDR_PreProcess(phExpEffect, &arcInputInfo, i);
    }
    MMemSet(&faceInfo, 0, sizeof(ARC_HDR_FACEINFO));
    if (mHdrInfo.m_FaceInfo.nFace > 0) {
        faceInfo.nFace = mHdrInfo.m_FaceInfo.nFace;
        faceInfo.rcFace = mHdrInfo.m_FaceInfo.rcFace;
        faceInfo.lFaceOrient = mHdrInfo.m_FaceInfo.lFaceOrient;
    }

    // if (m_iIsDumpData) {
    //     DumpMetadata(metadata, &arcInputInfo, &arcParam, &mHdrInfo, camMode, sceneType,
    //     mInputMode);
    // }

    // support (ImgNumber == 1,2,3), if ImgNumber == 2, input order is normal/low, or normal/over.
    ASVLOFFSCREEN outputImage = {0};

#ifndef ARC_USE_ALL_ION_BUFFER
    setOffScreen(&outputImage, &input[input_num - 1]); // please select the ev != zero frames
#else
    setOffScreen(&outputImage, output); // please select the ev != zero frames
#endif
    double start = nowMSec();

    ret = ARC_HDR_Process(phExpEffect, &arcParam, &faceInfo, arcISOValue, &outputImage, nDstImgFd);
    double end = nowMSec();
    MLOGD(Mia2LogGroupPlugin,
          "[MIALGO_HDR][ARCSOFT]:HDR_Process ret=%ld cost=%.2f bright=%d, "
          "saturation=%d, contrast=%d cameratype=%x\n",
          ret, end - start, arcParam.i32Brightness, arcParam.i32Saturation, arcParam.i32Contrast,
          cameraType);
    if (ret != MOK) {
        MLOGE(Mia2LogGroupPlugin, "[MIALGO_HDR][ARCSOFT]: Run  Error, return normal Img\n");
        PluginUtils::miCopyBuffer(output, &input[0]);
    }
#ifndef ARC_USE_ALL_ION_BUFFER
    else {
        PluginUtils::miCopyBuffer(output, &input[input_num - 1]);
    }
#endif

    MRESULT retUnint = ARC_HDR_Uninit(&phExpEffect);
    if (retUnint != MOK)
        MLOGE(Mia2LogGroupPlugin, "[MIALGO_HDR][ARCSOFT]: Uninit Error\n");

    return ret;
}

int ArcsoftHdr::process_low_light_hdr(struct MiImageBuffer *input, struct MiImageBuffer *output,
                                      int input_num, MI_HDR_INFO &mHdrInfo, int camMode,
                                      char *metadata)
{
    MRESULT ret = MOK;
    MHandle phExpEffect = MNull;
    ASVLOFFSCREEN SrcImgs[ARC_LLHDR_MAX_INPUT_IMAGE_NUM];
    ARC_LLHDR_PARAM arcParam = {0};
    ARC_LLHDR_FACEINFO faceInfo = {0};
    ARC_LLHDR_INPUTINFO arcInputInfo;
    ARC_LLHDR_DEVICE_TYPE deviceType = ARC_LLHDR_DEVICE_TYPE_XIAOMI_OTHER;

    ASVLOFFSCREEN outputImage = {0};
#ifndef ARC_USE_ALL_ION_BUFFER
    struct MiImageBuffer tmpMiBuffer;
    PluginUtils::miCopyBuffer(&tmpMiBuffer, &input[0]);
    setOffScreen(&outputImage, &tmpMiBuffer); // please select the ev != zero frames
#else
    setOffScreen(&outputImage, output); // please select the ev != zero frames
#endif
    ret = ARC_LLHDR_Init(&phExpEffect);
    if (ret != MOK) {
        MLOGE(Mia2LogGroupPlugin, "[AMIGLGO_LLHDR][ARCSOFT]: ARC_LLHDR_Init error\n");
        return ret;
    }

    MMemSet(&arcInputInfo, 0, sizeof(arcInputInfo));
    arcInputInfo.i32ImgNum = input_num;
    for (int i = 0; i < input_num; i++) {
        memset(&SrcImgs[i], 0, sizeof(ASVLOFFSCREEN));
        setOffScreen(&SrcImgs[i], input + i);
        arcInputInfo.pImages[i] = &SrcImgs[i];
        if (pEV != NULL) {
            arcInputInfo.fEvCompVal[i] = (MFloat)pEV[i];
        }
    }
    MMemSet(&faceInfo, 0, sizeof(ARC_HDR_FACEINFO));
    if (mHdrInfo.m_FaceInfo.nFace > 0) {
        faceInfo.nFace = mHdrInfo.m_FaceInfo.nFace;
        faceInfo.rcFace = mHdrInfo.m_FaceInfo.rcFace;
        faceInfo.lFaceOrient = mHdrInfo.m_FaceInfo.lFaceOrient;
    }

    camMode = ARC_LLHDR_CAMERA_REAR;
    ret = ARC_LLHDR_GetDefaultParamForDeviceType(&arcParam, mHdrInfo.m_AeInfo.iso, camMode,
                                                 (MInt32)deviceType);
    ARC_LLHDR_ImgOrient imgOri = ARC_LLHDR_ImgOrient::ARC_LLHDR_Ori_0;
    ARC_LLHDR_SetImgOri(phExpEffect, imgOri);
    MInt32 nEVStep = 6;
    ARC_LLHDR_SetEVStep(phExpEffect, nEVStep);
    ARC_LLHDR_SetDeviceType(phExpEffect, deviceType);

    // if (m_iIsDumpData) {
    //     DumpLLHDRMetadata(metadata, &arcInputInfo, &faceInfo, arcParam, mHdrInfo.m_AeInfo.iso,
    //     imgOri,
    //                       nEVStep, camMode);
    // }

    MInt32 nRetIndex = -1;
    double start = nowMSec();
    ret = ARC_LLHDR_Process(phExpEffect, &arcInputInfo, &faceInfo, &outputImage, &nRetIndex,
                            &arcParam, mHdrInfo.m_AeInfo.iso, ARC_LLHDR_CAMERA_REAR);
    double end = nowMSec();
    if (ret != MOK) {
        MLOGE(Mia2LogGroupPlugin, "[AMIGLGO_LLHDR][ARCSOFT]: Run  Error, return normal Img\n");
        PluginUtils::miCopyBuffer(output, &input[0]);
    }
#ifndef ARC_USE_ALL_ION_BUFFER
    else {
        PluginUtils::miCopyBuffer(output, &tmpMiBuffer);
    }
    PluginUtils::miFreeBuffer(&tmpMiBuffer);
#endif
    MLOGE(Mia2LogGroupPlugin,
          "[AMIGLGO_LLHDR][ARCSOFT]: ARC_LLHDR_Process ret=%ld-nRetIndex=%d-cost=%.2f\n", ret,
          nRetIndex, end - start);
    ret = ARC_LLHDR_Uninit(&phExpEffect);
    if (ret != MOK)
        MLOGE(Mia2LogGroupPlugin, "[AMIGLGO_LLHDR][ARCSOFT]: Uninit Error\n");

    return ret;
}

void ArcsoftHdr::load_config(ArcHDRCommonAEParam *pArcAEParams, ArcHDRParam *pArcHDRParam)
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
    MLOGW(Mia2LogGroupPlugin, "load_config:hdr:  common param: %.2f %.2f %d %.2f %.2f %.2f %.2f\n",
          pArcAEParams->lux_min, pArcAEParams->lux_max, pArcAEParams->checkermode,
          pArcAEParams->confval_min, pArcAEParams->confval_max, pArcAEParams->adrc_min,
          pArcAEParams->adrc_max);
    fclose(pf);
}

double ArcsoftHdr::nowMSec(void)
{
    struct timeval res;
    gettimeofday(&res, NULL);
    return 1000.0 * res.tv_sec + (double)res.tv_usec / 1e3;
}

// void ArcsoftHdr::DumpMetadata(char *file, LPARC_HDR_INPUTINFO pInputImages, LPARC_HDR_PARAM
// param,
//                               LPMI_HDR_INFO pHdrInfo, MInt32 cameraType, MInt32 sceneType,
//                               int inputMode)
// {
//     if (NULL == file) {
//         return;
//     }

//     MLOGD(Mia2LogGroupPlugin, "ArcsoftHdr::DumpMetadata E");

//     char params[255];
//     memset(params, 0, sizeof(params));
//     snprintf(params, sizeof(params), "%s_metadata.txt", file);
//     FILE *pf = fopen(params, "a");
//     if (pf) {
//         fprintf(pf, "[0]\n");
//         fprintf(pf, "realWidth=%d\n", pInputImages->InputImages[0].i32Width);
//         fprintf(pf, "realHeight=%d\n", pInputImages->InputImages[0].i32Height);
//         fprintf(pf, "inputFrameNum=%d\n", pInputImages->i32ImgNum);
//         for (MInt32 i = 0; i < pInputImages->i32ImgNum; i++) {
//             fprintf(pf, "fEvCompVal_%d=%f\n", i, pInputImages->fEvCompVal[i]);
//         }

//         fprintf(pf, "[FACE_NUM]\n");
//         fprintf(pf, "Face=%d\n", pHdrInfo->m_FaceInfo.nFace);
//         for (MInt32 i = 0; i < pHdrInfo->m_FaceInfo.nFace; i++) {
//             fprintf(pf, "[Face%d]\n", i);
//             fprintf(pf, "left=%d\n", pHdrInfo->m_FaceInfo.rcFace[i].left);
//             fprintf(pf, "top=%d\n", pHdrInfo->m_FaceInfo.rcFace[i].top);
//             fprintf(pf, "right=%d\n", pHdrInfo->m_FaceInfo.rcFace[i].right);
//             fprintf(pf, "bottom=%d\n", pHdrInfo->m_FaceInfo.rcFace[i].bottom);
//             fprintf(pf, "lfaceOrient=%d\n", pHdrInfo->m_FaceInfo.lFaceOrient[i]);
//         }
//         fclose(pf);
//     }

//     memset(params, 0, sizeof(params));
//     snprintf(params, sizeof(params), "%s.txt", file);
//     pf = fopen(params, "a");
//     if (pf) {
//         fprintf(pf, "%s\n", ARC_HDR_GetVersion()->Version);
//         fprintf(pf, "[0]\n");
//         fprintf(pf, "lBrightness=%d\n", param->i32Brightness);
//         fprintf(pf, "lToneSaturation=%d\n", param->i32Saturation);
//         fprintf(pf, "lCurveContrast=%d\n", param->i32Contrast);
//         fprintf(pf, "lPortraitBrightness=%d\n", param->i32PortraitBrightness);
//         fprintf(pf, "lInputMode=%d\n", inputMode);
//         fprintf(pf, "ISOValue=%d\n", pHdrInfo->m_AeInfo.iso);
//         fprintf(pf, "CameraType=%d\n", cameraType);
//         fclose(pf);
//     }
//     MLOGD(Mia2LogGroupPlugin, "ArcsoftHdr::DumpMetadata X");
// }

// void ArcsoftHdr::DumpLLHDRMetadata(char *file, ARC_LLHDR_INPUTINFO *pInputImages,
//                                    ARC_LLHDR_FACEINFO *pFaceInfo, ARC_LLHDR_PARAM param,
//                                    MInt32 isoValue, ARC_LLHDR_ImgOrient ImgOri, MInt32 i32EvStep,
//                                    MInt32 cameraType)
// {
//     if (NULL == file) {
//         return;
//     }

//     MLOGD(Mia2LogGroupPlugin, "ArcsoftllHdr::DumpMetadata E");
//     char params[255];
//     memset(params, 0, sizeof(params));
//     snprintf(params, sizeof(params), "%s_metadata.txt", file);
//     FILE *pf = fopen(params, "a");
//     if (pf) {
//         fprintf(pf, "[0]\n");
//         fprintf(pf, "iso=%d\n", isoValue);
//         fprintf(pf, "ispgain=0.00000\n");
//         fprintf(pf, "drcgain=0.00000\n");
//         fprintf(pf, "evvalue=0.00000\n");
//         fprintf(pf, "imageorient=%d\n", ImgOri);
//         fprintf(pf, "evstep=%d\n", i32EvStep);
//         fprintf(pf, "cameratype=%d\n", cameraType);
//         fprintf(pf, "inputFrameNum=%d\n", pInputImages->i32ImgNum);
//         for (MInt32 i = 0; i < pInputImages->i32ImgNum; i++) {
//             fprintf(pf, "fEvCompVal_%d=%f\n", i, pInputImages->fEvCompVal[i] / 6);
//         }

//         fprintf(pf, "facenum=%d\n", pFaceInfo->nFace);
//         for (MInt32 i = 0; i < pFaceInfo->nFace; i++) {
//             fprintf(pf, "[face_%d]\n", i);
//             fprintf(pf, "left=%d\n", pFaceInfo->rcFace[i].left);
//             fprintf(pf, "top=%d\n", pFaceInfo->rcFace[i].top);
//             fprintf(pf, "right=%d\n", pFaceInfo->rcFace[i].right);
//             fprintf(pf, "bottom=%d\n", pFaceInfo->rcFace[i].bottom);
//             fprintf(pf, "orient=%d\n", pFaceInfo->lFaceOrient[i]);
//         }
//         fclose(pf);
//     }
//     MLOGD(Mia2LogGroupPlugin, "ArcsoftllHdr::DumpMetadata X");
// }

bool ArcsoftHdr::isLLHDR(int inputNum, MInt32 *pEv)
{
    int negEvCount = 0;
    for (int i = 0; i < inputNum; ++i) {
        if (pEv[i] < 0) {
            negEvCount++;
        }
    }
    return negEvCount == 2;
}