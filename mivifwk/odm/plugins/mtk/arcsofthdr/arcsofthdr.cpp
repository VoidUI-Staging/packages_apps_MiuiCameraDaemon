#include "arcsofthdr.h"

#undef LOG_TAG
#define LOG_TAG "MIA_N_AHDR"

#define PROP_KEY_CAPTURE_BURST_DUMP "vendor.debug.hdr.capture.burst.hdrdump"
#define PROP_KEY_HDRCHK_FIXEV       "persist.camera.hdrchk.fixev" // value example "-1,-1,-1"
#define PROP_KEY_HDRCHK_DUMP        "persist.camera.hdr.checkerdump"

static void mibuffer_alloc_copy(struct MiImageBuffer *dstBuffer, struct MiImageBuffer *srcBuffer)
{
    memcpy(dstBuffer, srcBuffer, sizeof(struct MiImageBuffer));
    dstBuffer->plane[0] = (unsigned char *)malloc(dstBuffer->height * dstBuffer->stride * 3 / 2);
    dstBuffer->plane[1] = dstBuffer->plane[0] + dstBuffer->height * dstBuffer->stride;
    unsigned char *psrc = srcBuffer->plane[0];
    unsigned char *pdst = dstBuffer->plane[0];
    for (int h = 0; h < srcBuffer->height; h++) {
        memcpy(pdst, psrc, srcBuffer->width);
        psrc += srcBuffer->stride;
        pdst += dstBuffer->stride;
    }
    psrc = srcBuffer->plane[1];
    pdst = dstBuffer->plane[1];
    for (int h = 0; h < srcBuffer->height / 2; h++) {
        memcpy(pdst, psrc, srcBuffer->width);
        psrc += srcBuffer->stride;
        pdst += dstBuffer->stride;
    }
}

static void mibuffer_release(struct MiImageBuffer *mibuf)
{
    free(mibuf->plane[0]);
}

static double now_ms(void)
{
    struct timeval res;
    gettimeofday(&res, NULL);
    return 1000.0 * res.tv_sec + (double)res.tv_usec / 1e3;
}

static void WriteFileLog(char *file, const char *fmt, ...)
{
    FILE *pf = fopen(file, "ab");
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

ArcsoftHdr::ArcsoftHdr() {}

ArcsoftHdr::~ArcsoftHdr() {}

void ArcsoftHdr::init() {}

MVoid setOffScreen(ASVLOFFSCREEN *pImg, struct MiImageBuffer *miBuf)
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

void ArcsoftHdr::Process(struct MiImageBuffer *input, const arc_hdr_input_meta_t *pMeta,
                         struct MiImageBuffer *output)
{
    int input_num;
    int hdr_mode = MODE_HDR;
    char path[255];
    time_t now;
    struct tm *timenow;
    struct MiImageBuffer tmpMiBuffer;

    input_num = pMeta->input_num;
    mibuffer_alloc_copy(&tmpMiBuffer, &input[0]);

    if (pMeta->isHdrSr == 1) {
        hdr_mode = MODE_ZOOMHDR;
    } else if (pMeta->scene_type == 1) {
        hdr_mode = MODE_LLHDR;
    }

    MLOGI(Mia2LogGroupPlugin, "input_num=%d, camera_mode=%d, scene_type = %d, hdr_mode = %d",
          pMeta->input_num, pMeta->camera_mode, pMeta->scene_type, hdr_mode);

    int isDumpData = property_get_int32(PROP_KEY_CAPTURE_BURST_DUMP, 0);
    if (isDumpData) {
        time(&now);
        timenow = localtime(&now);
        for (int i = 0; i < input_num; i++) {
            if (0 == i) {
                if (hdr_mode == MODE_ZOOMHDR) {
                    snprintf(path, sizeof(path),
                             "%sarcsoft_hdr_%02d%02d%02d%02d%02d_%dx%d_zoomhdr_input_metadump.txt",
                             PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                             timenow->tm_hour, timenow->tm_min, timenow->tm_sec, input[i].width,
                             input[i].height);
                } else if (hdr_mode == MODE_LLHDR) {
                    snprintf(path, sizeof(path),
                             "%sarcsoft_hdr_%02d%02d%02d%02d%02d_%dx%d_llhdr_input_metadump.txt",
                             PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                             timenow->tm_hour, timenow->tm_min, timenow->tm_sec, input[i].width,
                             input[i].height);
                } else {
                    snprintf(path, sizeof(path),
                             "%sarcsoft_hdr_%02d%02d%02d%02d%02d_%dx%d_hdr_input_metadump.txt",
                             PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                             timenow->tm_hour, timenow->tm_min, timenow->tm_sec, input[i].width,
                             input[i].height);
                }

                WriteFileLog(path, "inputFrame_width = %d\ninputFrame_height = %d\ninputNum = %d\n",
                             input[i].width, input[i].height, input_num);

                for (int j = 0; j < input_num; j++) {
                    WriteFileLog(path, "EV_value[%d] = %d\n", j, pMeta->ev[j]);
                }

                for (int k = 0; k < pMeta->face_info.nFace; k++) {
                    WriteFileLog(
                        path,
                        "Face[%d]:\n TopLeft:[%d, %d]\n BotRight:[%d, %d] \n FaceOrient = %d \n", k,
                        pMeta->face_info.rcFace->left, pMeta->face_info.rcFace->top,
                        pMeta->face_info.rcFace->right, pMeta->face_info.rcFace->bottom,
                        *(pMeta->face_info.lFaceOrient));
                }
            }

            if (hdr_mode == MODE_ZOOMHDR) {
                snprintf(path, sizeof(path),
                         "%sarcsoft_hdr_%02d%02d%02d%02d%02d_%dx%d_%d_zoomhdr_input_ev[%d].nv21",
                         PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                         timenow->tm_hour, timenow->tm_min, timenow->tm_sec, input[i].width,
                         input[i].height, i, pMeta->ev[i]);
            } else if (hdr_mode == MODE_LLHDR) {
                snprintf(path, sizeof(path),
                         "%sarcsoft_hdr_%02d%02d%02d%02d%02d_%dx%d_%d_llhdr_input_ev[%d].nv21",
                         PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                         timenow->tm_hour, timenow->tm_min, timenow->tm_sec, input[i].width,
                         input[i].height, i, pMeta->ev[i]);
            } else {
                snprintf(path, sizeof(path),
                         "%sarcsoft_hdr_%02d%02d%02d%02d%02d_%dx%d_%d_hdr_input_ev[%d].nv21",
                         PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                         timenow->tm_hour, timenow->tm_min, timenow->tm_sec, input[i].width,
                         input[i].height, i, pMeta->ev[i]);
            }
            PluginUtils::miDumpBuffer(&input[i], path);
        }
    }

    int result = 0;
    switch (hdr_mode) {
    case MODE_ZOOMHDR:
        result = process_zoomhdr(input, pMeta, &tmpMiBuffer);
        break;
    case MODE_HDR:
        result = process_hdr(input, pMeta, &tmpMiBuffer);
        break;
    case MODE_LLHDR:
        result = process_llhdr(input, pMeta, &tmpMiBuffer);
        break;
    }

    PluginUtils::miCopyBuffer(output, &tmpMiBuffer);
    mibuffer_release(&tmpMiBuffer);
    if (isDumpData) {
        property_set(PROP_KEY_HDRCHK_DUMP, "1");
        if (hdr_mode == MODE_ZOOMHDR) {
            snprintf(path, sizeof(path),
                     "%sarcsoft_hdr_%02d%02d%02d%02d%02d_%dx%d_zoomhdr_output_ret_%d.nv21",
                     PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                     timenow->tm_hour, timenow->tm_min, timenow->tm_sec, output->width,
                     output->height, result);
        } else if (hdr_mode == MODE_LLHDR) {
            snprintf(path, sizeof(path),
                     "%sarcsoft_hdr_%02d%02d%02d%02d%02d_%dx%d_llhdr_output_ret_%d.nv21",
                     PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                     timenow->tm_hour, timenow->tm_min, timenow->tm_sec, output->width,
                     output->height, result);
        } else {
            snprintf(path, sizeof(path),
                     "%sarcsoft_hdr_%02d%02d%02d%02d%02d_%dx%d_hdr_output_ret_%d.nv21",
                     PluginUtils::getDumpPath(), timenow->tm_mon + 1, timenow->tm_mday,
                     timenow->tm_hour, timenow->tm_min, timenow->tm_sec, output->width,
                     output->height, result);
        }
        PluginUtils::miDumpBuffer(output, path);
    }
}

int ArcsoftHdr::process_hdr(struct MiImageBuffer *input, const arc_hdr_input_meta_t *pMeta,
                            struct MiImageBuffer *output)
{
    MRESULT ret = MOK;
    MInt32 nImagesFd = 0; // The FD of Src Images, 0: use cpu buffer, else : use ION buffer
    MInt32 nDstImgFd = 0; // The FD of dest Images, 0: use cpu buffer, else: use ION buffer
    MHandle phExpEffect = MNull;

    int input_num = pMeta->input_num;
    int sceneType = 0;
    int ev_step = 6;
    int index = HDR_PARAM_INDEX_REAR;
    int inputMode = ARC_HDR_INPUT_MODE_NLOSN;
    int cameraType = ARC_HDR_CAMERA_WIDE;
    if (pMeta->camera_mode == MI_FRONT_CAMERA) {
        index = HDR_PARAM_INDEX_FRONT;
        inputMode = ARC_HDR_INPUT_MODE_NLLSN2;
        cameraType = ARC_HDR_CAMERA_FRONT;
    } else if (pMeta->camera_mode == MI_UW_CAMERA) {
        cameraType = ARC_HDR_CAMERA_ULTRAWIDE;
    }

    FN_ARC_HDR_android_log_print fn_log_print = __android_log_print;
    ARC_HDR_SetLogLevel(ARC_HDR_LOGLEVEL_2, fn_log_print);

    ret = ARC_HDR_Init(&phExpEffect);
    if (ret != MOK) {
        MLOGE(Mia2LogGroupPlugin, "ARC_HDR_Init error\n");
        PluginUtils::miCopyBuffer(output, &input[0]);
        return ret;
    }

    MLOGI(Mia2LogGroupPlugin, "MiaNodeHdr get HDR algorithm library version: %s",
          ARC_HDR_GetVersion()->Version);

    ARC_HDR_FACEINFO faceinfo;
    MMemCpy(&faceinfo, &pMeta->face_info, sizeof(ARC_HDR_FACEINFO));
    MLOGI(Mia2LogGroupPlugin, "face_num: %d, first face: %d %d %d %d", faceinfo.nFace,
          faceinfo.rcFace[0].left, faceinfo.rcFace[0].top, faceinfo.rcFace[0].right,
          faceinfo.rcFace[0].bottom);

    ARC_HDR_PARAM param;
    memset(&param, 0, sizeof(ARC_HDR_PARAM));
    param.i32Brightness = ArcHDRParams[index].brightness;
    param.i32Contrast = ArcHDRParams[index].contrast;
    param.i32ToneLength = ArcHDRParams[index].tonelen;
    param.i32Saturation = ArcHDRParams[index].saturation;
    ARC_HDR_SetParam(phExpEffect, &param);

    ARC_HDR_INPUTINFO inputInfo;
    MMemSet(&inputInfo, 0, sizeof(inputInfo));
    inputInfo.i32ImgNum = input_num;
    for (int i = 0; i < input_num; i++) {
        setOffScreen(&inputInfo.InputImages[i], &input[i]);
        inputInfo.fEvCompVal[i] =
            (float)(pMeta->ev[i] / ev_step); // The EV value given to the algorithm needs /ev_step
        inputInfo.i32SrcImgFd[i] = nImagesFd;
        ARC_HDR_PreProcess(phExpEffect, &inputInfo, i);
    }

    arc_hdr_output_meta_t userData = {0};
    if (pMeta->camera_mode == MI_UW_CAMERA) {
        MLOGD(Mia2LogGroupPlugin, "framework camera id %d, call setcallback.",
              pMeta->frameworkCameraID);
        ARC_HDR_SetCallback(phExpEffect, NULL, &userData);
    } else {
        MLOGD(Mia2LogGroupPlugin, "framework camera id %d, no setcallback.",
              pMeta->frameworkCameraID);
    }

    ARC_HDR_SetSceneType(phExpEffect, sceneType);
    ARC_HDR_GetDefaultParam(phExpEffect, &param, inputMode, pMeta->iso, cameraType);

    ASVLOFFSCREEN pImg;
    setOffScreen(&pImg, output);
    double start = now_ms();
    ret = ARC_HDR_Process(phExpEffect, &faceinfo, &pImg, nDstImgFd, inputMode, pMeta->iso,
                          cameraType);
    double end = now_ms();
    MLOGI(Mia2LogGroupPlugin, "HDR_Process ret=%ld cost=%.2fms", ret, end - start);

    if (ret != MOK && ret != ARC_HDR_ERR_LARGE_MEAN_DIFF) {
        MLOGE(Mia2LogGroupPlugin, "Run  Error, return normal Img\n");
        PluginUtils::miCopyBuffer(output, &input[0]);
    }

    if (ARC_HDR_Uninit(&phExpEffect) != MOK)
        MLOGE(Mia2LogGroupPlugin, "Uninit Error\n");

    return ret;
}

int ArcsoftHdr::process_llhdr(struct MiImageBuffer *input, const arc_hdr_input_meta_t *pMeta,
                              struct MiImageBuffer *output)
{
    MRESULT ret = MOK;
    MHandle phExpEffect = MNull;
    ASVLOFFSCREEN SrcImgs[ARC_LLHDR_MAX_INPUT_IMAGE_NUM];
    ARC_LLHDR_PARAM param = {0};
    ARC_LLHDR_INPUTINFO srcImgs;
    ASVLOFFSCREEN outputImage = {0};
    MInt32 nRetIndex = -1;

    int camMode = ARC_LLHDR_CAMERA_REAR;
    if (pMeta->camera_mode == MI_UW_CAMERA)
        camMode = ARC_LLHDR_CAMERA_REAR_UW;
    else if (pMeta->camera_mode == MI_FRONT_CAMERA) {
        camMode = ARC_LLHDR_CAMERA_FRONT;
    } else {
        camMode = ARC_LLHDR_CAMERA_REAR;
    }

    setOffScreen(&outputImage, output);

    ret = ARC_LLHDR_Init(&phExpEffect);
    if (ret != MOK) {
        MLOGE(Mia2LogGroupPlugin, "ARC_LLHDR_Init error");
        PluginUtils::miCopyBuffer(output, &input[0]);
        return ret;
    }

    MLOGI(Mia2LogGroupPlugin, "get LLHDR algorithm library version: %s",
          ARC_LLHDR_GetVersion()->Version);

    MMemSet(&srcImgs, 0, sizeof(srcImgs));
    srcImgs.i32ImgNum = pMeta->input_num;
    for (int i = 0; i < pMeta->input_num; i++) {
        memset(&SrcImgs[i], 0, sizeof(ASVLOFFSCREEN));
        setOffScreen(&SrcImgs[i], &input[i]);
        srcImgs.pImages[i] = &SrcImgs[i];
        srcImgs.fEvCompVal[i] = (float)(pMeta->ev[i] / 6);
    }

    ARC_LLHDR_FACEINFO faceinfo = {0};
    MMemCpy(&faceinfo, &pMeta->face_info, sizeof(ARC_HDR_FACEINFO));
    MLOGI(Mia2LogGroupPlugin, "face_num: %d, first face: %d %d %d %d", faceinfo.nFace,
          faceinfo.rcFace[0].left, faceinfo.rcFace[0].top, faceinfo.rcFace[0].right,
          faceinfo.rcFace[0].bottom);

    ARC_LLHDR_GetDefaultParam(&param, pMeta->iso, camMode);

    ARC_LLHDR_ImgOrient imgOri;
    switch (pMeta->orientation) {
    case 0:
        imgOri = ARC_LLHDR_ImgOrient::ARC_LLHDR_Ori_0;
        break;
    case 90:
        imgOri = ARC_LLHDR_ImgOrient::ARC_LLHDR_Ori_1;
        break;
    case 180:
        imgOri = ARC_LLHDR_ImgOrient::ARC_LLHDR_Ori_2;
        break;
    case 270:
        imgOri = ARC_LLHDR_ImgOrient::ARC_LLHDR_Ori_3;
        break;
    }
    ARC_LLHDR_SetImgOri(phExpEffect, imgOri);

    MInt32 nEVStep = 6;
    ARC_LLHDR_SetEVStep(phExpEffect, nEVStep);

    ARC_LLHDR_DEVICE_TYPE deviceType = ARC_LLHDR_DEVICE_TYPE_XIAOMI_OTHER;
    ARC_LLHDR_SetDeviceType(phExpEffect, deviceType);

    double start = now_ms();
    ret = ARC_LLHDR_Process(phExpEffect, &srcImgs, &faceinfo, &outputImage, &nRetIndex, &param,
                            pMeta->iso, camMode);
    double end = now_ms();

    if (ret != MOK && ret != MERR_LLHDR_ALIGN_MERR && ret != MERR_LLHDR_INPUT_SEQUENCE_MERR) {
        MLOGE(Mia2LogGroupPlugin, "Run  Error, return normal Img");
        PluginUtils::miCopyBuffer(output, &input[0]);
    }

    MLOGI(Mia2LogGroupPlugin, "ARC_LLHDR_Process ret=%ld, nRetIndex=%d, cost=%.2fms", ret,
          nRetIndex, end - start);

    if (ARC_LLHDR_Uninit(&phExpEffect) != MOK)
        MLOGE(Mia2LogGroupPlugin, "Uninit Error");

    return ret;
}

int ArcsoftHdr::process_zoomhdr(struct MiImageBuffer *input, const arc_hdr_input_meta_t *pMeta,
                                struct MiImageBuffer *output)
{
    MInt32 mISOValue = pMeta->iso;
    MInt32 sceneType = 0;
    MInt32 cameraType = ARC_ZOOM_HDR_MODE_WIDE_CAMERA;

    MLOGI(Mia2LogGroupPlugin, "ArcsoftHdr get algorithm library version: %s",
          ARC_ZOOM_HDR_GetVersion()->Version);

    MHandle phExpEffect = MNull;
    MRESULT ret = ARC_ZOOM_HDR_Init(&phExpEffect, cameraType);
    if (ret != MOK) {
        MLOGE(Mia2LogGroupPlugin, "[MIALGO_HDR][ARCSOFT]: ARC_ZOOM_HDR_Init error\n");
        return ret;
    }

    ARC_ZOOM_HDR_SetSceneType(phExpEffect, sceneType);

    ARC_ZOOM_HDR_PARAM hdr_Param;
    MMemSet(&hdr_Param, 0, sizeof(ARC_ZOOM_HDR_PARAM));
    ARC_ZOOM_HDR_GetDefaultParam(phExpEffect, &hdr_Param);

    ARC_ZOOM_HDR_INPUTINFO m_InputInfo;
    MMemSet(&m_InputInfo, 0, sizeof(ARC_ZOOM_HDR_INPUTINFO));
    m_InputInfo.i32ImgNum = pMeta->input_num;
    for (int i = 0; i < pMeta->input_num; i++) {
        setOffScreen(&m_InputInfo.InputImages[i], input + i);
        ARC_ZOOM_HDR_PreProcess(phExpEffect, &m_InputInfo, i);
    }

    ARC_ZOOM_HDR_FACEINFO faceInfo;
    MMemSet(&faceInfo, 0, sizeof(ARC_ZOOM_HDR_FACEINFO));
    MMemCpy(&faceInfo, &pMeta->face_info, sizeof(ARC_HDR_FACEINFO));
    MLOGI(Mia2LogGroupPlugin, "face_num: %d, first face: %d %d %d %d", faceInfo.nFace,
          faceInfo.rcFace[0].left, faceInfo.rcFace[0].top, faceInfo.rcFace[0].right,
          faceInfo.rcFace[0].bottom);

    // support (ImgNumber == 1,2,3), if ImgNumber == 2, input order is normal/low, or normal/over.
    ASVLOFFSCREEN outputImage = {0};
    setOffScreen(&outputImage, output); // please select the ev != zero frames

    MRECT cropRect = pMeta->cropRegion;
    ARC_ZOOM_HDR_SetCropSize(phExpEffect, &cropRect);

    double start = now_ms();
    ret = ARC_ZOOM_HDR_Process(phExpEffect, &hdr_Param, &faceInfo, &outputImage);
    double end = now_ms();

    MLOGD(Mia2LogGroupPlugin,
          "[MIALGO_HDR][ARCSOFT]:ZOOM_HDR_Process ret=%ld cost=%.2f cameratype= %x", ret,
          end - start, cameraType);
    if (ret != MOK) {
        MLOGE(Mia2LogGroupPlugin, "[MIALGO_HDR][ARCSOFT]: Run  Error, return normal Img\n");
        PluginUtils::miCopyBuffer(output, &input[0]);
    }

    MRESULT retUnint = ARC_ZOOM_HDR_Uninit(&phExpEffect);
    if (retUnint != MOK)
        MLOGE(Mia2LogGroupPlugin, "[MIALGO_HDR][ARCSOFT]: Uninit Error\n");

    return ret;
}
