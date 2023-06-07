#include "miLdcCapture.h"
#include <mutex>
#include "MiaPluginUtils.h"
#include "mialgo_det_seg_deploy.h"

namespace mialgo2 {

#undef LOG_TAG
#define LOG_TAG "LDC-PLUGIN"

// static S32 g_isLowCamLevel = property_get_int32("ro.boot.camera.config", -1);

static std::mutex g_miLdcCapMtx;

MiLdcCapture::MiLdcCapture(U32 debug)
{
    m_debug = debug;
    Device->detector->Create(&m_detectorRunId, MARK("m_detectorRunId"));
    Device->detector->Create(&m_detectorInitId, MARK("m_detectorInitId"));
}

MiLdcCapture::~MiLdcCapture()
{
    S32 ret = RET_OK;
    Device->detector->Destroy(&m_detectorRunId);
    Device->detector->Destroy(&m_detectorInitId);
    std::lock_guard<std::mutex> lock(g_miLdcCapMtx);
    if (m_libStatus != LDC_LIBSTATUS_INVALID) {
        ret = MIALGO_CaptureLDCDestoryWhenClose(&m_hMiCaptureLDC);
        DEV_IF_LOGI((ret == RET_OK), "CaptureLDCDestoryWhenClose Done.");
        DEV_IF_LOGE((ret != RET_OK), "CaptureLDCDestoryWhenClose failed!");
        ResetLibPara();
    }
    if (LDC_FACE_MASK_MIALGODETSEG == FACE_MASK_TYPE && true == m_segLibInitFlag) {
        ret = ModelRelease(&m_hMiDetSeg);
        DEV_IF_LOGI((0 == ret), "MISEG_DET: ModelRelease Done.");
        DEV_IF_LOGW((0 != ret), "MISEG_DET: ModelRelease failed.");
    }
}

void MiLdcCapture::ResetLibPara()
{
    m_libWidth = 0;
    m_libHeight = 0;
    m_libStride = 0;
    m_libSliceHeight = 0;
    m_libFormat = 0;
    m_libStatus = LDC_LIBSTATUS_INVALID;
}

bool MiLdcCapture::CheckNeedInitLib(DEV_IMAGE_BUF *input)
{
    bool needInit = false;
    do {
        if (m_libWidth != input->width || m_libHeight != input->height) {
            DEV_LOGI("NeedInitLib: width/height not match.");
            needInit = true;
            break;
        }
        if (m_libStride != input->stride[0] || m_libSliceHeight != input->sliceHeight[0]) {
            DEV_LOGI("NeedInitLib: stride not match.");
            needInit = true;
            break;
        }
        if (m_libFormat != input->format) {
            DEV_LOGI("NeedInitLib: format not match");
            needInit = true;
            break;
        }
        if (m_libStatus != LDC_LIBSTATUS_VALID) {
            DEV_LOGI("NeedInitLib: libStatus not valid");
            needInit = true;
            break;
        }
    } while (0);

    return needInit;
}

S32 MiLdcCapture::GetCalibBin(U32 vendorId, void **ppData)
{
    // vendorId is used to distinguish different lens modules, most projects are unique.
    // Can be used if necessary.
    S32 ret = RET_OK;
    S32 packDataSize = 0;
    void *packDataBuf = NULL;
    const char *fileName = "/odm/etc/camera/MILDC_CALIB_0x01.bin";
    packDataSize = Device->file->GetSize(fileName);
    DEV_IF_LOGE_RETURN_RET((0 == packDataSize), RET_ERR, "Get calib size err");
    packDataBuf = malloc(packDataSize);
    DEV_IF_LOGE_RETURN_RET((NULL == packDataBuf), RET_ERR, "Calin malloc err");
    ret = Device->file->ReadOnce(fileName, packDataBuf, packDataSize, 0);
    DEV_IF_LOGE_RETURN_RET((ret != packDataSize), RET_ERR, "read file err");
    DEV_IF_LOGI((ret == packDataSize), "LDC read calib done [%s].", fileName);
    *ppData = packDataBuf;
    ret = RET_OK;
    return ret;
}

S32 MiLdcCapture::FreeCalibBin(void **ppData)
{
    if (ppData != NULL) {
        if (*ppData) {
            free(*ppData);
            *ppData = NULL;
        }
    }

    return RET_OK;
}

// Should hold mutex before calling this func
S32 MiLdcCapture::InitLib(const LdcProcessInputInfo &inputInfo)
{
    S32 ret = 0;
    DEV_IMAGE_BUF *input = inputInfo.inputBuf;
    DEV_IMAGE_BUF *output = inputInfo.outputBuf;

    bool needInit = CheckNeedInitLib(input);
    DEV_IF_LOGI_RETURN_RET(needInit == false, RET_OK, "No initialization required.");

    if (m_libStatus != LDC_LIBSTATUS_INVALID) {
        ret = MIALGO_CaptureLDCDestoryWhenClose(&m_hMiCaptureLDC);
        DEV_IF_LOGI((ret == RET_OK), "CaptureLDCDestoryWhenClose Done.");
        DEV_IF_LOGE((ret != RET_OK), "CaptureLDCDestoryWhenClose failed!");
        ResetLibPara();
    }

    MIALGO_CaptureLDCVersionGet();
    void *packDataBuf = NULL;
    ret = GetCalibBin(0, &packDataBuf); // alloc calib buffer.
    DEV_IF_LOGE((ret != RET_OK), "GetPackData failed!");
    CaptureLDCLaunchParams launchPara = {
        .imgWidth = static_cast<int>(input->width),
        .imgHeight = static_cast<int>(input->height),
        .inImgStrideW = static_cast<int>(input->stride[0]),
        .inImgStrideH = static_cast<int>(input->sliceHeight[0]),
        .outImgStrideW = static_cast<int>(output->stride[0]),
        .outImgStrideH = static_cast<int>(output->sliceHeight[0]),
        .pCalibBuf = static_cast<unsigned char *>(packDataBuf),
        .jsonFileName = "/odm/etc/camera/MILDC_CAPTURE_PARAMS.json",
        .inImgFmt = GetMiLDCFormat(input->format)};
    Device->detector->Begin(m_detectorInitId, MARK("LDCInitWhenLaunch"), 10000);
    DEV_LOGI("MIALGO_CaptureLDCInitWhenLaunch +++");
    ret = MIALGO_CaptureLDCInitWhenLaunch(&m_hMiCaptureLDC, &launchPara);
    Device->detector->End(m_detectorInitId);
    DEV_IF_LOGE((ret != RET_OK), "CaptureLDCInitWhenLaunch failed!! %d", ret);
    DEV_IF_LOGI((ret == RET_OK), "CaptureLDCInitWhenLaunch Done.");
    FreeCalibBin(&packDataBuf); // free calib buffer.

    if (ret == RET_OK) {
        m_libFormat = input->format;
        m_libWidth = input->width;
        m_libHeight = input->height;
        m_libStride = input->stride[0];
        m_libSliceHeight = input->sliceHeight[0];
        m_libStatus = LDC_LIBSTATUS_VALID;
    } else {
        m_libStatus = LDC_LIBSTATUS_ERRORSTATE;
    }

    DEV_LOGI("Mi-LDC Capture INIT Done: Ret:%d Input:%d*%d(%d*%d), Output:%d*%d(%d*%d) format=%d",
             ret, input->width, input->height, input->stride[0], input->sliceHeight[0],
             output->width, output->height, output->stride[0], output->sliceHeight[0],
             input->format);
    return ret;
}

S32 MiLdcCapture::Process(LdcProcessInputInfo &inputInfo)
{
    // lock to prevent re-entry, especially when deinit triggered while another LDC is processing.
    std::lock_guard<std::mutex> lock(g_miLdcCapMtx);
    DEV_LOGI("MI-LDC Process START #%llu.", inputInfo.frameNum);
    S32 ret = RET_OK;
    DEV_IMAGE_BUF *input = inputInfo.inputBuf;
    DEV_IMAGE_BUF *output = inputInfo.outputBuf;

    ret = InitLib(inputInfo);
    DEV_IF_LOGE_RETURN_RET((m_libStatus != LDC_LIBSTATUS_VALID), RET_ERR, "Lib not ready bypass.");

    // Burstshot does not protect face.
    if (TRUE == inputInfo.isBurstShot) {
        inputInfo.faceInfo.num = 0;
    }
    // Confirm whether the mask algo needs to be init.
    if (inputInfo.faceInfo.num > 0) {
        if (LDC_FACE_MASK_MIALGODETSEG == FACE_MASK_TYPE && FALSE == m_segLibInitFlag) {
            int maskInitRet = 0;

            ModelInitParams initPara = {.det_model_name = "human_detect",
                                        .seg_model_name = "mialgo_seg_ldc",
                                        .asset_path = "/vendor/etc/camera", // AIVS
                                        .srcImageWidth = static_cast<int>(input->width),
                                        .srcImageHeight = static_cast<int>(input->height),
                                        .modelType = 2,        // 0->rgb, 1->nv12, 2->LDC
                                        .use_detector = FALSE, // switch off detect
                                        .detRuntime = AI_VISION_RUNTIME_DSP_AIO,
                                        .detPriority = AI_VISION_PRIORITY_HIGH_AIO,
                                        .detPerformance = AI_VISION_PERFORMANCE_HIGH_AIO,
                                        .segRuntime = AI_VISION_RUNTIME_DSP_AIO,
                                        .segPriority = AI_VISION_PRIORITY_HIGH_AIO,
                                        .segPerformance = AI_VISION_PERFORMANCE_HIGH_AIO};

            DEV_LOGI("MISEG_DET: Init[1] width:%d height:%d use_detector:%d",
                     initPara.srcImageWidth, initPara.srcImageHeight, initPara.use_detector);
            DEV_LOGI("MISEG_DET: Init[2] Det: runtime:%d priority:%d performance:%d",
                     initPara.detRuntime, initPara.detPriority, initPara.detPerformance);
            DEV_LOGI("MISEG_DET: Init[3] Seg: runtime:%d priority:%d performance:%d",
                     initPara.segRuntime, initPara.segPriority, initPara.segPerformance);
            DEV_LOGI("MISEG_DET: Init[4] DetModel:%s, SegModel:%s", initPara.det_model_name.c_str(),
                     initPara.seg_model_name.c_str());
            maskInitRet = ModelLaunch(&m_hMiDetSeg, &initPara);
            DEV_IF_LOGI((0 == maskInitRet), "MISEG_DET: ModelLaunch Done.");
            DEV_IF_LOGW((0 != maskInitRet), "MISEG_DET: ModelLaunch failed.");
            if (maskInitRet == RET_OK) {
                m_segLibInitFlag = TRUE;
            }
            if (m_segLibInitFlag == FALSE) {
                inputInfo.faceInfo.num = 0;
            }
        }
    }

    CaptureLDCInitParams initParams = {};
    CaptureLDCImg imgInY = {input->plane[0], input->fd[0]};
    CaptureLDCImg imgInUV = {input->plane[1], input->fd[1]};
    CaptureLDCImg mask = {NULL, 0};
    initParams.maskWidth = 0;
    initParams.maskHeight = 0;
    DEV_IMAGE_BUF maskDepthoutput = {0};
    // AILDC get mask, HLDC not need.
    if (LDC_FACE_MASK_MIALGODETSEG == FACE_MASK_TYPE && inputInfo.faceInfo.num > 0 &&
        TRUE == m_segLibInitFlag) {
        int maskRet = RET_OK;
        maskRet = GetMaskFromMiDetSeg(inputInfo, &maskDepthoutput);
        // Get facemask error, do once non-faceprotect process.
        if (maskRet != RET_OK) {
            inputInfo.faceInfo.num = 0;
        }
        mask = {maskDepthoutput.plane[0], 0};
        initParams.maskWidth = maskDepthoutput.width;
        initParams.maskHeight = maskDepthoutput.height;
    }

    initParams.pImgInY = &imgInY;
    initParams.pImgInUV = &imgInUV;
    initParams.pPotraitMask = &mask;
    initParams.zoomWOI.x = inputInfo.zoomWOI.left;
    initParams.zoomWOI.y = inputInfo.zoomWOI.top;
    initParams.zoomWOI.width = inputInfo.zoomWOI.width;
    initParams.zoomWOI.height = inputInfo.zoomWOI.height;
    initParams.faceInfo.fNum = inputInfo.faceInfo.num;
    for (int i = 0; i < inputInfo.faceInfo.num; i++) {
        initParams.faceInfo.face[i].score = inputInfo.faceInfo.score[i];
        initParams.faceInfo.face[i].roi.height = inputInfo.faceInfo.face[i].height;
        initParams.faceInfo.face[i].roi.width = inputInfo.faceInfo.face[i].width;
        initParams.faceInfo.face[i].roi.x = inputInfo.faceInfo.face[i].left;
        initParams.faceInfo.face[i].roi.y = inputInfo.faceInfo.face[i].top;
    }
    initParams.imgOrientation = CAPTURE_LDC_ORIENTATION_0;
    if (inputInfo.faceInfo.orient[0] == 0) {
        initParams.imgOrientation = CAPTURE_LDC_ORIENTATION_0;
    } else if (inputInfo.faceInfo.orient[0] == 90) {
        initParams.imgOrientation = CAPTURE_LDC_ORIENTATION_90;
    } else if (inputInfo.faceInfo.orient[0] == 180) {
        initParams.imgOrientation = CAPTURE_LDC_ORIENTATION_180;
    } else if (inputInfo.faceInfo.orient[0] == 270) {
        initParams.imgOrientation = CAPTURE_LDC_ORIENTATION_270;
    }
    if (inputInfo.faceInfo.num == 0) {
        initParams.pPotraitMask = nullptr;
    }
    MIALGO_CaptureLDCInitEachPic(&m_hMiCaptureLDC, &initParams);
    CaptureLDCImg imgOutY = {output->plane[0], output->fd[0]};
    CaptureLDCImg imgOutUV = {output->plane[1], output->fd[1]};
    Device->detector->Begin(m_detectorRunId, MARK("CaptureLDCProc"), 10000);
    ret = MIALGO_CaptureLDCProc(&m_hMiCaptureLDC, &imgOutY, &imgOutUV);
    Device->detector->End(m_detectorRunId);
    DEV_IF_LOGE((ret != RET_OK), "CaptureLDCProc failed!");
    DEV_IF_LOGI((ret == RET_OK), "CaptureLDCProc Done.");
    MIALGO_CaptureLDCDestoryEachPic(&m_hMiCaptureLDC);
    if (maskDepthoutput.plane[0] != NULL) {
        Device->image->Free(&maskDepthoutput);
    }

    DEV_LOGI("MI-LDC Process EXIT #%llu.", inputInfo.frameNum);
    DEV_IF_LOGE((ret != RET_OK), "MI-LDC Process RUN ERR");
    return ret;
}

CAPTURE_LDC_IMG_FORMAT MiLdcCapture::GetMiLDCFormat(int srcFormat)
{
    CAPTURE_LDC_IMG_FORMAT miFormat = CAPTURE_LDC_IMG_NV12;
    switch (srcFormat) {
    case DEV_IMAGE_FORMAT_YUV420NV21:
        miFormat = CAPTURE_LDC_IMG_NV21;
        break;
    case DEV_IMAGE_FORMAT_YUV420P010:
        miFormat = CAPTURE_LDC_IMG_P010;
        break;
    default:
        miFormat = CAPTURE_LDC_IMG_NV12;
    }
    return miFormat;
}

S32 MiLdcCapture::GetMaskFromMiDetSeg(LdcProcessInputInfo &inputInfo,
                                      DEV_IMAGE_BUF *maskDepthoutput)
{
    int ret = RET_OK;
    DEV_IMAGE_BUF *input = inputInfo.inputBuf;

    ModelProcParams procPara = {.rotate_angle = static_cast<int>(inputInfo.faceInfo.orient[0]),
                                .srcImage.imgWidth = static_cast<int>(input->width),
                                .srcImage.imgHeight = static_cast<int>(input->height),
                                .srcImage.pYData = input->plane[0],
                                .srcImage.pUVData = input->plane[1],
                                .srcImage.stride = static_cast<int>(input->stride[0]),
                                .srcImage.img_format = MIALGO_IMG_NV12_AIO};
    DEV_LOGI("MISEG_DET: procIn angle:%d width:%d height:%d stride:%d", procPara.rotate_angle,
             procPara.srcImage.imgWidth, procPara.srcImage.imgHeight, procPara.srcImage.stride);
    double startTime = PluginUtils::nowMSec();
    ret = ModelRun(&m_hMiDetSeg, &procPara);
    DEV_IF_LOGE_RETURN_RET((0 != ret), ret, "MISEG_DET: ModelRun failed.");
    DEV_IF_LOGI((0 == ret), "MISEG_DET: ModelRun Done, cost %.2fms.",
                PluginUtils::nowMSec() - startTime);
    DEV_LOGI("MISEG_DET: procOut ptr = %p, (%d,%d) stride:%d", procPara.potraitMask.data,
             procPara.potraitMask.imgWidth, procPara.potraitMask.imgHeight,
             procPara.potraitMask.stride);
    ret = Device->image->Alloc(maskDepthoutput, procPara.potraitMask.imgWidth,
                               procPara.potraitMask.imgWidth, procPara.potraitMask.imgHeight,
                               procPara.potraitMask.imgHeight, 0,
                               (DEV_IMAGE_FORMAT)DEV_IMAGE_FORMAT_Y8, MARK("maskDepthoutput"));
    DEV_IF_LOGE_RETURN_RET((0 != ret), ret, "MISEG_DET: Alloc failed.");
    DEV_IMAGE_BUF inputMask;
    inputMask.width = procPara.potraitMask.imgWidth;
    inputMask.height = procPara.potraitMask.imgHeight;
    inputMask.stride[0] = procPara.potraitMask.imgWidth;
    inputMask.format = DEV_IMAGE_FORMAT_Y8;
    inputMask.plane[0] = (char *)procPara.potraitMask.data;
    ret = Device->image->Copy(maskDepthoutput, &inputMask);
    if (ret != 0) {
        DEV_LOGI("MISEG_DET: copy failed!");
    }
    if ((m_debug & LDC_DEBUG_DUMP_IMAGE_CAPTURE) != 0) {
        Device->image->DumpImage(maskDepthoutput, LOG_TAG, inputInfo.frameNum,
                                 "maskDepthoutput.out.Y8");
    }
    return ret;
}

} // namespace mialgo2
