#include "MiaLdcPlugin.h"
#include "device/device.h"
#include "nodeMetaDataUp.h"

#undef LOG_TAG
#define LOG_TAG "LDC-PLUGIN"
#ifndef LDC_CAPTURE_ALGO_VENDOR
#define LDC_CAPTURE_ALGO_VENDOR LDC_CAPTURE_VENDOR_MIPHONE
#endif

static const int32_t g_ldcDebug = property_get_int32("persist.vendor.camera.LDC.debug", 0);
static const int32_t g_needDumpLdc = property_get_int32("persist.vendor.algoengine.ldc.dump", 0);

namespace mialgo2 {

LDCPlugin::~LDCPlugin()
{
    Device->Deinit();
}

S32 LDCPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    Device->Init();
    m_debug = mergerDebugInfo(g_ldcDebug, g_needDumpLdc);
    // MiBokehLdc::GetInstance()->SetDebugLevel(m_debug);
    m_captureVendor = LDC_CAPTURE_ALGO_VENDOR;
    m_fwkCameraId = createInfo->frameworkCameraId;
    m_pNodeMetaData = NULL;
    m_nodeInterface = nodeInterface;
#if defined(SOCRATES_CAM)
    if (m_captureVendor == LDC_CAPTURE_VENDOR_ALTEK && m_pAltekLdcCapture == NULL)
        m_pAltekLdcCapture = new AltekLdcCapture(m_debug);
#else
    if (m_captureVendor == LDC_CAPTURE_VENDOR_MIPHONE && m_pMiLdcCapture == NULL)
        m_pMiLdcCapture = new MiLdcCapture(m_debug);
#endif
    if (m_pNodeMetaData == NULL) {
        m_pNodeMetaData = new NodeMetaData();
        m_pNodeMetaData->Init(NODE_METADATA_MODE_CAPTURE, m_debug);
    }
    DEV_LOGI("Plugin initialize: %p, algoVendor = %d", this, m_captureVendor);
    return RET_OK;
}

bool LDCPlugin::isEnabled(MiaParams settings)
{
    bool needBypass = false;
    void *pData = NULL;

    do {
        // Check debug property
        if ((m_debug & LDC_DEBUG_BYPASS_CAPTURE) != 0) {
            MLOGI(Mia2LogGroupPlugin, "LdcBypass: force bypass capture");
            needBypass = true;
            break;
        }

        // Check LDC Level
        pData = NULL;
        uint8_t ldcLevel = 0;
        const char *ldcLevelTagName = "xiaomi.distortion.ultraWideDistortionLevel";
        VendorMetadataParser::getVTagValue(settings.metadata, ldcLevelTagName, &pData);
        if (NULL != pData) {
            ldcLevel = *static_cast<uint8_t *>(pData);
            // MLOGI(Mia2LogGroupPlugin, "ldcLevel = %u", ldcLevel);
        } else {
            MLOGW(Mia2LogGroupPlugin, "\"xiaomi.distortion.ultraWideDistortionLevel\" not found!");
        }
        if (ldcLevel != LDC_LEVEL_ENABLE) {
            MLOGI(Mia2LogGroupPlugin, "LdcBypass: ldcLevel=%d", ldcLevel);
            needBypass = true;
            break;
        }

        // Check camera combmode
        if (m_fwkCameraId != CAM_COMBMODE_REAR_ULTRA) {
            MLOGI(Mia2LogGroupPlugin, "LdcBypass: m_fwkCameraId=%u", m_fwkCameraId);
            needBypass = true;
            break;
        }

#if defined(THOR_CAM) || defined(UNICORN_CAM) || defined(MAYFLY_CAM) || defined(ZIZHAN_CAM)
        // Check bypass SW LDC when Burstshot use qcom ICA LDC.
        pData = NULL;
        S32 captureHint = 0;
        VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.burst.captureHint", &pData);
        if (NULL != pData) {
            captureHint = *static_cast<int32_t *>(pData);
            // MLOGI(Mia2LogGroupPlugin, "captureHint = %d", captureHint);
        } else {
            MLOGW(Mia2LogGroupPlugin, "\"xiaomi.burst.captureHint\" not found!");
        }
        if (captureHint == 1) {
            MLOGD(Mia2LogGroupPlugin, "LdcBypass: is burst shot");
            needBypass = true;
            break;
        }
#endif

    } while (0);

    return !needBypass;
}

int LDCPlugin::preProcess(PreProcessInfo preProcessInfo)
{
    S32 ret = RET_OK;
    // Sometimes 1x session plugin::destroy called before 0.6x session preprocess, avoid
    // unnecessary algo destroy can use LdcCapture::m_combMode(fwkcameraId);
    return ret;
}

ProcessRetStatus LDCPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    S32 ret = RET_OK;
    ProcessRetStatus resultInfo = PROCSUCCESS;
    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;

    camera_metadata_t *metadata = inputBuffers[0].pMetadata;
    camera_metadata_t *pPhyMeta = inputBuffers[0].pPhyCamMetadata;
    camera_metadata_t *priorMeta = NULL;
    DEV_IMAGE_BUF inputFrame, outputFrame;
    struct ImageParams requestInputBuffer = inputBuffers[0];
    struct ImageParams requestOutputBuffer = outputBuffers[0];
    ret |= Device->image->MatchToImage((void *)&requestInputBuffer, &inputFrame);
    ret |= Device->image->MatchToImage((void *)&requestOutputBuffer, &outputFrame);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), resultInfo, "Match buffer err");
    LdcProcessInputInfo inputInfo = {};
    inputInfo.frameNum = processRequestInfo->frameNum;
    inputInfo.inputBuf = &inputFrame;
    inputInfo.outputBuf = &outputFrame;
    DEV_LOGI(
        "Plugin RUN "
        "frameNum=%lu,in:%dx%d(%dx%d),fmt=%d,out:%dx%d(%dx%d),fmt=%d",
        inputInfo.frameNum, inputFrame.width, inputFrame.height, inputFrame.stride[0],
        inputFrame.sliceHeight[0], inputFrame.format, outputFrame.width, outputFrame.height,
        outputFrame.stride[0], outputFrame.sliceHeight[0], outputFrame.format);

    inputInfo.isSATMode = FALSE;
    m_pNodeMetaData->GetCaptureHint(metadata, &inputInfo.isBurstShot);
    // m_pNodeMetaData->GetAppMode(metadata, &inputInfo.isSATMode);
    m_pNodeMetaData->GetCameraVendorId(metadata, &inputInfo.vendorId);
    if (pPhyMeta) {
        inputInfo.hasPhyMeta = TRUE;
        priorMeta = pPhyMeta;
    } else {
        inputInfo.hasPhyMeta = FALSE;
        priorMeta = metadata;
    }
    inputInfo.cropRegion = {0, 0, inputFrame.width, inputFrame.height};
    inputInfo.sensorSize = {0, 0, inputFrame.width, inputFrame.height};
    m_pNodeMetaData->GetCropRegion(priorMeta, &inputInfo);
    m_pNodeMetaData->GetZoomRatio(priorMeta, &inputInfo.zoomRatio);
    m_pNodeMetaData->GetActiveArraySize(priorMeta, &inputInfo.sensorSize);
    m_pNodeMetaData->GetFaceInfo(priorMeta, inputInfo, &inputInfo.faceInfo);
    TransformZoomWOI(&inputInfo.zoomWOI, &inputInfo.cropRegion, &inputInfo.sensorSize,
                     inputInfo.inputBuf, inputInfo.hasPhyMeta);

    // Draw face rect
    if ((m_debug & LDC_DEBUG_INPUT_DRAW_FACE_BOX) != 0) {
        DEV_IMAGE_RECT faceRect = {0, 0, 0, 0};
        for (int i = 0; i < inputInfo.faceInfo.num; i++) {
            faceRect.height = inputInfo.faceInfo.face[i].height;
            faceRect.left = inputInfo.faceInfo.face[i].left;
            faceRect.top = inputInfo.faceInfo.face[i].top;
            faceRect.width = inputInfo.faceInfo.face[i].width;
            Device->image->DrawRect(&inputFrame, faceRect, 5, 5);
        }
    }

    // Print meta to logcat, dump to txt.
    DumpProcInfo(&inputInfo);

    if ((m_debug & LDC_DEBUG_DUMP_IMAGE_CAPTURE) != 0) {
        Device->image->DumpImage(&inputFrame, LOG_TAG, inputInfo.frameNum, "CAPTURE.in.nv12");
    }
#if defined(SOCRATES_CAM)
    if (m_captureVendor == LDC_CAPTURE_VENDOR_ALTEK) {
        ret = m_pAltekLdcCapture->Process(inputInfo);
    }
#else
    if (m_captureVendor == LDC_CAPTURE_VENDOR_MIPHONE) {
        ret = m_pMiLdcCapture->Process(inputInfo);
    }
#endif
    if (((m_debug & LDC_DEBUG_DUMP_IMAGE_CAPTURE) != 0) && ret == RET_OK) {
        Device->image->DumpImage(&outputFrame, LOG_TAG, inputInfo.frameNum, "CAPTURE.out.nv12");
    }

    // Set exif info in final jpeg.
    UpdateExifInfo(processRequestInfo->frameNum, processRequestInfo->timeStamp, ret, &inputInfo);

    // Proc error do memcpy.
    if (ret != RET_OK) {
        DEV_LOGI("proc error do memcpy...");
        ret = Device->image->Copy(&outputFrame, &inputFrame);
        DEV_IF_LOGE_RETURN_RET((ret != RET_OK), ProcessRetStatus::PROCFAILED, "memcpy err");
    }
    resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus LDCPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int LDCPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

void LDCPlugin::destroy()
{
#if defined(SOCRATES_CAM)
    if (m_captureVendor == LDC_CAPTURE_VENDOR_ALTEK && m_pAltekLdcCapture != NULL) {
        delete m_pAltekLdcCapture;
        m_pAltekLdcCapture = NULL;
    }
#else
    if (m_captureVendor == LDC_CAPTURE_VENDOR_MIPHONE && m_pMiLdcCapture != NULL) {
        delete m_pMiLdcCapture;
        m_pMiLdcCapture = NULL;
    }
#endif
    if (m_pNodeMetaData) {
        delete m_pNodeMetaData;
        m_pNodeMetaData = NULL;
    }
    MLOGI(Mia2LogGroupPlugin, "Destory %p done.", this);
}

void LDCPlugin::TransformZoomWOI(LDCRECT *zoomWOI, LDCRECT *cropRegion, LDCRECT *sensorSize,
                                 DEV_IMAGE_BUF *input, bool isSAT)
{
    DEV_IF_LOGE_RETURN((NULL == cropRegion), "cropRegion nullptr error");
    DEV_IF_LOGE_RETURN((NULL == input), "input nullptr error");
    const float AspectRatioTolerance = 0.01f;
    const uint cropAlign = 0x2U;
    float scaleWidth = 0;
    float scaleHeight = 0;
    scaleWidth = (float)input->width / cropRegion->width;
    scaleHeight = (float)input->height / cropRegion->height;
    if (scaleHeight + AspectRatioTolerance < scaleWidth) {
        // 16:9
        zoomWOI->height =
            (U32)((input->height * cropRegion->width) / input->width) & ~(cropAlign - 1);
        zoomWOI->top = cropRegion->top + ((cropRegion->height - zoomWOI->height) / 2);
        zoomWOI->left = cropRegion->left;
        zoomWOI->width = cropRegion->width;
    } else if (scaleWidth + AspectRatioTolerance < scaleHeight) {
        // 1:1
        zoomWOI->width =
            (U32)((input->width * cropRegion->height) / input->height) & ~(cropAlign - 1);
        zoomWOI->left = cropRegion->left + ((cropRegion->width - zoomWOI->width) / 2);
        zoomWOI->top = cropRegion->top;
        zoomWOI->height = cropRegion->height;
    } else {
        // 4:3
        if (TRUE == isSAT) {
            // In SAT mode, preview use "xiaomi.smoothTransition.UWDisplayCrop" to get crop info.
            // This tag is based on strict size(width/height = 4/3),whereas the crop info used for
            // snapshot is not.
            U32 refWidth = input->width > sensorSize->width ? sensorSize->width : input->width;
            U32 refHeight = input->height > sensorSize->height ? sensorSize->height : input->height;
            zoomWOI->width = (cropRegion->width * refWidth / sensorSize->width);
            zoomWOI->height = (cropRegion->height * refHeight / sensorSize->height);
            zoomWOI->left = cropRegion->left + (cropRegion->width - zoomWOI->width) / 2;
            zoomWOI->top = cropRegion->top + (cropRegion->height - zoomWOI->height) / 2;
        } else {
            zoomWOI->width = cropRegion->width;
            zoomWOI->height = cropRegion->height;
            zoomWOI->left = cropRegion->left;
            zoomWOI->top = cropRegion->top;
        }
    }
}

void LDCPlugin::UpdateExifInfo(uint32_t frameNum, int64_t timeStamp, int32_t procRet,
                               const LdcProcessInputInfo *pProcessInfo)
{
    DEV_IF_LOGE_RETURN((NULL == pProcessInfo), "pProcessInfo is nullptr");

    if (m_nodeInterface.pSetResultMetadata != NULL) {
        char buf[1024] = {0};
        LDCRECT cropRegion = pProcessInfo->cropRegion;
        LDCRECT zoomWOI = pProcessInfo->zoomWOI;
        snprintf(buf, sizeof(buf), "LDC:{on:1 procRet:%d cropRegion:%d,%d,%d,%d WOI:%d,%d,%d,%d}",
                 procRet, cropRegion.left, cropRegion.top, cropRegion.width, cropRegion.height,
                 zoomWOI.left, zoomWOI.top, zoomWOI.width, zoomWOI.height);
        std::string results(buf);
        m_nodeInterface.pSetResultMetadata(m_nodeInterface.owner, frameNum, timeStamp, results);
    }
}

void LDCPlugin::DumpProcInfo(const LdcProcessInputInfo *pProcessInfo)
{
    DEV_IF_LOGE_RETURN((NULL == pProcessInfo), "pProcessInfo is nullptr");

    // Print to logcat per frame
    DEV_LOGI("Frame Num: #%lld", pProcessInfo->frameNum);
    DEV_LOGI("VendorID = %d, ZoomRatio = %f", pProcessInfo->vendorId, pProcessInfo->zoomRatio);
    DEV_LOGI("SensorSize = (%d,%d,%d,%d)", pProcessInfo->sensorSize.left,
             pProcessInfo->sensorSize.top, pProcessInfo->sensorSize.width,
             pProcessInfo->sensorSize.height);
    DEV_LOGI("CropRegion = (%d,%d,%d,%d)", pProcessInfo->cropRegion.left,
             pProcessInfo->cropRegion.top, pProcessInfo->cropRegion.width,
             pProcessInfo->cropRegion.height);
    DEV_LOGI("ZoomWOI = (%d,%d,%d,%d)", pProcessInfo->zoomWOI.left, pProcessInfo->zoomWOI.top,
             pProcessInfo->zoomWOI.width, pProcessInfo->zoomWOI.height);

    // Dump to .txt
    if ((m_debug & LDC_DEBUG_DUMP_IMAGE_CAPTURE) != 0) {
        char dumpBuf[1024 * 4] = {0};
        sprintf(dumpBuf + strlen(dumpBuf), "FACE NUM [%d]\n", pProcessInfo->faceInfo.num);
        for (int i = 0; i < pProcessInfo->faceInfo.num; i++) {
            sprintf(dumpBuf + strlen(dumpBuf), "FACE [%d/%d] [x=%d,y=%d,w=%d,h=%d] S[%d]\n", i + 1,
                    pProcessInfo->faceInfo.num, pProcessInfo->faceInfo.face[i].left,
                    pProcessInfo->faceInfo.face[i].top, pProcessInfo->faceInfo.face[i].width,
                    pProcessInfo->faceInfo.face[i].height, pProcessInfo->faceInfo.score[i]);
        }
        sprintf(dumpBuf + strlen(dumpBuf), "WOI %d,%d,%d,%d\n", pProcessInfo->zoomWOI.left,
                pProcessInfo->zoomWOI.top, pProcessInfo->zoomWOI.width,
                pProcessInfo->zoomWOI.height);
        Device->image->DumpData(dumpBuf, strlen(dumpBuf), LOG_TAG, pProcessInfo->frameNum,
                                "procInfo.txt");
    }
}

} // namespace mialgo2
