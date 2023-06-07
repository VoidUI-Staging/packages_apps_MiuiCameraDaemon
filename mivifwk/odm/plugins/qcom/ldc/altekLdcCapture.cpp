#include "altekLdcCapture.h"
#include <mutex>
#include "MiaPluginUtils.h"

namespace mialgo2 {

#undef LOG_TAG
#define LOG_TAG "LDC-PLUGIN"

#ifndef ALTEK_LDC_CAPTURE_CONFIG
#define ALTEK_LDC_CAPTURE_CONFIG ALTEK_LDC_CAPTURE_MODE_AI
#endif

#ifndef ALTEK_LDC_FACE_MASK_CONFIG
#define ALTEK_LDC_FACE_MASK_CONFIG ALTEK_LDC_FACE_MASK_ENABLE
#endif

// static S32 g_isLowCamLevel = property_get_int32("ro.boot.camera.config", -1);
static std::mutex g_altekLdcCapMtx;

AltekLdcCapture::AltekLdcCapture(U32 debug)
{
    m_debug = debug;
    m_captureMode = ALTEK_LDC_CAPTURE_CONFIG;
    m_useAltekMask = ALTEK_LDC_FACE_MASK_CONFIG;
    Device->detector->Create(&m_detectorRunId, MARK("m_detectorRunId"));
    Device->detector->Create(&m_detectorInitId, MARK("m_detectorInitId"));
}

AltekLdcCapture::~AltekLdcCapture()
{
    S32 ret = RET_OK;
    Device->detector->Destroy(&m_detectorRunId);
    Device->detector->Destroy(&m_detectorInitId);
    std::lock_guard<std::mutex> lock(g_altekLdcCapMtx);
    if (m_libStatus != LDC_LIBSTATUS_INVALID) {
        if (m_captureMode == ALTEK_LDC_CAPTURE_MODE_AI) {
            ret = alAILDC_Buffer_Deinit(1);
            DEV_IF_LOGI((ret == RET_OK), "CaptureLDCDestoryWhenClose Done.");
            DEV_IF_LOGE((ret != RET_OK), "CaptureLDCDestoryWhenClose failed!");
            ResetLibPara();
        } else {
            // m_captureMode == ALTEK_LDC_CAPTURE_MODE_H
            // TODO.
        }
    }
}

void AltekLdcCapture::ResetLibPara()
{
    m_libWidth = 0;
    m_libHeight = 0;
    m_libStride = 0;
    m_libSliceHeight = 0;
    m_libFormat = 0;
    m_libStatus = LDC_LIBSTATUS_INVALID;
}

bool AltekLdcCapture::CheckNeedInitLib(DEV_IMAGE_BUF *input)
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
            DEV_LOGI("NeedInitLib: format not match.");
            needInit = true;
            break;
        }
        if (m_libStatus != LDC_LIBSTATUS_VALID) {
            DEV_LOGI("NeedInitLib: libStatus not valid.");
            needInit = true;
            break;
        }
    } while (0);

    return needInit;
}

S32 AltekLdcCapture::GetPackData(U32 vendorId, void **ppData, S64 *pSize)
{
    // vendorId is used to distinguish different lens modules, most projects are unique.
    // Can be used if necessary.
    S32 ret = RET_OK;
    S64 packDataSize = 0;
    void *packDataBuf = NULL;
    const char *fileName = "/odm/etc/camera/altekldc_packdata_0x01.bin";
    packDataSize = Device->file->GetSize(fileName);
    DEV_IF_LOGE_RETURN_RET((0 == packDataSize), RET_ERR, "Get packdata size err");
    packDataBuf = malloc(packDataSize);
    DEV_IF_LOGE_RETURN_RET((NULL == packDataBuf), RET_ERR, "packdata malloc err");
    ret = Device->file->ReadOnce(fileName, packDataBuf, packDataSize, 0);
    DEV_IF_LOGE_RETURN_RET((ret != packDataSize), RET_ERR, "read packdata err");
    DEV_IF_LOGI((ret == packDataSize), "LDC read packdata done [%s].", fileName);
    *ppData = packDataBuf;
    *pSize = packDataSize;
    return RET_OK;
}

S32 AltekLdcCapture::GetTuningData(void **ppData, S64 *pSize)
{
    S32 ret = RET_OK;
    S64 tuningDataSize = 0;
    void *tuningDataBuf = NULL;
    const char *fileName = "/odm/etc/camera/altekldc_tuningdata_ai.bin";
    tuningDataSize = Device->file->GetSize(fileName);
    DEV_IF_LOGE_RETURN_RET((0 == tuningDataSize), RET_ERR, "Get tuning bin size err");
    tuningDataBuf = malloc(tuningDataSize);
    DEV_IF_LOGE_RETURN_RET((NULL == tuningDataBuf), RET_ERR, "tuning bin malloc err");
    ret = Device->file->ReadOnce(fileName, tuningDataBuf, tuningDataSize, 0);
    DEV_IF_LOGE_RETURN_RET((ret != tuningDataSize), RET_ERR, "read tuning bin err");
    DEV_IF_LOGI((ret == tuningDataSize), "LDC read tuning bin done [%s].", fileName);
    *ppData = tuningDataBuf;
    *pSize = tuningDataSize;
    return RET_OK;
}

S32 AltekLdcCapture::GetModelData(void **ppData, S64 *pSize)
{
    S32 ret = RET_OK;
    S64 modelDataSize = 0;
    void *modelDataBuf = NULL;
    const char *fileName = "/odm/etc/camera/altekldc_segmodel.bin";
    modelDataSize = Device->file->GetSize(fileName);
    DEV_IF_LOGE_RETURN_RET((0 == modelDataSize), RET_ERR, "Get model size err");
    modelDataBuf = malloc(modelDataSize);
    DEV_IF_LOGE_RETURN_RET((NULL == modelDataBuf), RET_ERR, "model malloc err");
    ret = Device->file->ReadOnce(fileName, modelDataBuf, modelDataSize, 0);
    DEV_IF_LOGE_RETURN_RET((ret != modelDataSize), RET_ERR, "read model err");
    DEV_IF_LOGI((ret == modelDataSize), "LDC read model done [%s].", fileName);
    *ppData = modelDataBuf;
    *pSize = modelDataSize;
    return RET_OK;
}

S32 AltekLdcCapture::FreeData(void **ppData)
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
S32 AltekLdcCapture::InitLib(const LdcProcessInputInfo &inputInfo)
{
    S32 ret = 0;
    char algoVersion[256] = {0};
    DEV_IMAGE_BUF *input = inputInfo.inputBuf;
    DEV_IMAGE_BUF *output = inputInfo.outputBuf;

    bool needInit = CheckNeedInitLib(input);
    DEV_IF_LOGI_RETURN_RET(needInit == false, RET_OK, "No initialization required.");

    if (m_libStatus != LDC_LIBSTATUS_INVALID) {
        if (m_captureMode == ALTEK_LDC_CAPTURE_MODE_AI) {
            ret = alAILDC_Buffer_Deinit(1);
            DEV_IF_LOGI((ret == RET_OK), "alAILDC_Buffer_Deinit Done.");
            DEV_IF_LOGE((ret != RET_OK), "alAILDC_Buffer_Deinit failed!");
            ResetLibPara();
        } else {
            // m_captureMode == ALTEK_LDC_CAPTURE_MODE_H
            // TODO.
        }
    }

    if (m_captureMode == ALTEK_LDC_CAPTURE_MODE_AI) {
        alAILDC_VersionInfo_Get(algoVersion, sizeof(algoVersion));
        char workingPath[] = "/data/vendor/camera/";
        alAILDC_Set_Working_Path(workingPath, sizeof(workingPath));
        if (m_useAltekMask == ALTEK_LDC_FACE_MASK_ENABLE) {
            // Use V2 init API(packdata + tuningbin + modelbin).
            S64 packdataSize = 0;
            void *packdataBuf = NULL;
            GetPackData(0, &packdataBuf, &packdataSize);
            S64 tuningdataSize = 0;
            void *tuningdataBuf = NULL;
            GetTuningData(&tuningdataBuf, &tuningdataSize);
            S64 modeldataSize = 0;
            void *modeldataBuf = NULL;
            GetModelData(&modeldataBuf, &modeldataSize);
            alAILDC_IMG_FORMAT algoFormat = GetAltekLDCFormat(input->format);
            DEV_LOGI("alAILDC_Buffer_Init_v2 +++");
            Device->detector->Begin(m_detectorInitId, MARK("LDCInitWhenLaunch"), 10000);
            ret = alAILDC_Buffer_Init_v2(
                1, input->width, input->height, output->width, output->height, input->stride[0],
                input->sliceHeight[0], output->stride[0], output->sliceHeight[0], algoFormat,
                (unsigned char *)packdataBuf, packdataSize, (unsigned char *)tuningdataBuf,
                tuningdataSize, (unsigned char *)modeldataBuf, modeldataSize);
            Device->detector->End(m_detectorInitId);
            DEV_IF_LOGE((ret != RET_OK), "alAILDC_Buffer_Init_v2 failed!! %d", ret);
            DEV_IF_LOGI((ret == RET_OK), "alAILDC_Buffer_Init_v2 Done.");
            FreeData(&packdataBuf);
            FreeData(&tuningdataBuf);
            FreeData(&modeldataBuf);
        } else {
            // m_useAltekMask == ALTEK_LDC_FACE_MASK_DISABLE
            // Use normal init API(packdata + tuningbin).
            // TODO.
        }
    } else {
        // m_captureMode == ALTEK_LDC_CAPTURE_MODE_H
        // TODO.
    }

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

    DEV_LOGI(
        "ALTEK-LDC Capture INIT Done: Ret:%d Input:%d*%d(%d*%d), Output:%d*%d(%d*%d) format=%d",
        ret, input->width, input->height, input->stride[0], input->sliceHeight[0], output->width,
        output->height, output->stride[0], output->sliceHeight[0], input->format);
    return ret;
}

S32 AltekLdcCapture::Process(LdcProcessInputInfo &inputInfo)
{
    // lock to prevent re-entry, especially when deinit triggered while another LDC is processing.
    std::lock_guard<std::mutex> lock(g_altekLdcCapMtx);
    DEV_LOGI("ALTEK-LDC Process START #%llu.", inputInfo.frameNum);
    S32 ret = RET_OK;
    DEV_IMAGE_BUF *input = inputInfo.inputBuf;
    DEV_IMAGE_BUF *output = inputInfo.outputBuf;

    ret = InitLib(inputInfo);
    DEV_IF_LOGE_RETURN_RET((m_libStatus != LDC_LIBSTATUS_VALID), RET_ERR, "Lib not ready bypass.");

    // Burstshot does not protect face.
    if (TRUE == inputInfo.isBurstShot) {
        inputInfo.faceInfo.num = 0;
    }

    if (m_captureMode == ALTEK_LDC_CAPTURE_MODE_AI) {
        if (m_useAltekMask == ALTEK_LDC_FACE_MASK_ENABLE) {
            alAILDC_RECT inputWOI = {
                .x = static_cast<unsigned short>(inputInfo.zoomWOI.left),
                .y = static_cast<unsigned short>(inputInfo.zoomWOI.top),
                .width = static_cast<unsigned short>(inputInfo.zoomWOI.width),
                .height = static_cast<unsigned short>(inputInfo.zoomWOI.height)};
            alAILDC_FACE_INFO inputFaceInfo = {0};
            inputFaceInfo.count = inputInfo.faceInfo.num;
            inputFaceInfo.src_img_width = input->width;
            inputFaceInfo.src_img_height = input->height;
            for (int i = 0; i < inputFaceInfo.count; i++) {
                inputFaceInfo.face[i].score = inputInfo.faceInfo.score[i];
                inputFaceInfo.face[i].roi.height = inputInfo.faceInfo.face[i].height;
                inputFaceInfo.face[i].roi.width = inputInfo.faceInfo.face[i].width;
                inputFaceInfo.face[i].roi.x = inputInfo.faceInfo.face[i].left;
                inputFaceInfo.face[i].roi.y = inputInfo.faceInfo.face[i].top;
            }
            alAILDC_SET_SCREEN_ORIENTATION orient = alAILDC_SCREEN_ANGLE_0;
            if (inputInfo.faceInfo.orient[0] == 0) {
                orient = alAILDC_SCREEN_ANGLE_0;
            } else if (inputInfo.faceInfo.orient[0] == 90) {
                orient = alAILDC_SCREEN_ANGLE_90;
            } else if (inputInfo.faceInfo.orient[0] == 180) {
                orient = alAILDC_SCREEN_ANGLE_180;
            } else if (inputInfo.faceInfo.orient[0] == 270) {
                orient = alAILDC_SCREEN_ANGLE_270;
            }
            DEV_LOGI("alAILDC_Set_ScreenOrientation +++");
            alAILDC_Set_ScreenOrientation(1, orient);
            DEV_LOGI("alAILDC_Set_ScreenOrientation ---");
            Device->detector->Begin(m_detectorRunId, MARK("CaptureLDCProc"), 10000);
            DEV_LOGI("alAILDC_Buffer_Run +++");
            ret = alAILDC_Buffer_Run(
                1, (unsigned char *)input->plane[0], (unsigned char *)input->plane[1],
                (unsigned char *)output->plane[0], (unsigned char *)output->plane[1], 0, nullptr,
                &inputWOI, &inputFaceInfo, nullptr);
            DEV_IF_LOGE((ret != RET_OK), "alAILDC_Buffer_Run failed!");
            DEV_IF_LOGI((ret == RET_OK), "alAILDC_Buffer_Run Done.");
            Device->detector->End(m_detectorRunId);
        } else {
            // m_useAltekMask == ALTEK_LDC_FACE_MASK_DISABLE
            // TODO.
        }
    } else {
        // m_captureMode == ALTEK_LDC_CAPTURE_MODE_H
        // TODO.
    }

    DEV_LOGI("ALTEK-LDC Process EXIT #%llu.", inputInfo.frameNum);
    DEV_IF_LOGE((ret != RET_OK), "ALTEK-LDC Process RUN ERR");
    return ret;
}

alAILDC_IMG_FORMAT AltekLdcCapture::GetAltekLDCFormat(int srcFormat)
{
    alAILDC_IMG_FORMAT altekFormat = alAILDC_IMAGE_FORMAT_NV12;
    switch (srcFormat) {
    case DEV_IMAGE_FORMAT_YUV420NV21:
        altekFormat = alAILDC_IMAGE_FORMAT_NV21;
        break;
    case DEV_IMAGE_FORMAT_YUV420P010:
        altekFormat = alAILDC_IMAGE_FORMAT_P010_MSB;
        break;
    default:
        altekFormat = alAILDC_IMAGE_FORMAT_NV12;
    }
    return altekFormat;
}

} // namespace mialgo2