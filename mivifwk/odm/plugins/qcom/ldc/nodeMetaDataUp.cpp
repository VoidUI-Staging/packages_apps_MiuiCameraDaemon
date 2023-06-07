#include "nodeMetaDataUp.h"
#include <system/camera_metadata.h>

namespace mialgo2 {

#undef LOG_TAG
#define LOG_TAG "LDC-PLUGIN"

NodeMetaData::NodeMetaData() : m_mode(0), m_debug(0) {}

NodeMetaData::~NodeMetaData() {}

S32 NodeMetaData::GetMetaData(camera_metadata_t *mdata, const char *pTagName, U32 tag,
                              void **ppData, U32 *pSize)
{
    DEV_IF_LOGE_RETURN_RET((NULL == mdata), RET_ERR_ARG, "pArg is null");
    DEV_IF_LOGE_RETURN_RET(((NULL == pTagName) && (tag == 0)), RET_ERR_ARG, "pArg is null");
    S32 ret = RET_ERR;
    if (pTagName != NULL) {
        ret = VendorMetadataParser::getVTagValue(mdata, pTagName, ppData);
    } else {
        ret = VendorMetadataParser::getTagValue(mdata, tag, ppData);
    }
    DEV_IF_LOGE_RETURN_RET((*ppData == NULL), RET_ERR_ARG, "GetMetadata err");
    return ret;
}

S32 NodeMetaData::SetMetaData(camera_metadata_t *mdata, const char *pTagName, U32 tag, void *pData,
                              U32 size)
{
    DEV_IF_LOGE_RETURN_RET((NULL == mdata), RET_ERR_ARG, "pArg is null");
    DEV_IF_LOGE_RETURN_RET(((NULL == pTagName) && (tag == 0)), RET_ERR_ARG, "pArg is null");
    S32 ret = RET_ERR;
    if (pTagName != NULL) {
        ret = VendorMetadataParser::setVTagValue(mdata, pTagName, pData, size);
    } else {
        // ret = VendorMetadataParser::setVTagValue(mdata, (VendorTag) tag, pData, size);
    }
    DEV_IF_LOGE_RETURN_RET((ret = !RET_OK), RET_ERR_ARG, "GetMetadata err");
    return ret;
}

S32 NodeMetaData::Init(U32 mode, U32 debug)
{
    m_mode = mode;
    m_debug = debug;
    return RET_OK;
}

S32 NodeMetaData::GetCameraVendorId(camera_metadata_t *mdata, U32 *pCameraVendorId)
{
    DEV_IF_LOGE_RETURN_RET(pCameraVendorId == NULL, RET_ERR_ARG, "arg err");
    *pCameraVendorId = property_get_int32("persist.vendor.camera.rearUltra.vendorID", 0);
    DEV_LOGI("Camera Vendor Id=%d", *pCameraVendorId);
    return RET_OK;
}

S32 NodeMetaData::SetCameraVendorId(camera_metadata_t *mdata, U64 frameNum, U32 *pCameraVendorId)
{
    DEV_IF_LOGE_RETURN_RET(pCameraVendorId == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    ret = SetMetaData(mdata, "xiaomi.sensor.vendorId", 0, pCameraVendorId, sizeof(U32));
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "set metadata err");
    return RET_OK;
}

S32 NodeMetaData::GetCameraTypeId(camera_metadata_t *mdata, U32 *pCameraTypeId)
{
    DEV_IF_LOGE_RETURN_RET(pCameraTypeId == NULL, RET_ERR_ARG, "arg err");
    void *pData = NULL;
    S32 ret = GetMetaData(mdata, "org.codeaurora.qcamera3.logicalCameraType.logical_camera_type", 0,
                          &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "GetMetadata err");
    U8 camType = *reinterpret_cast<U8 *>(pData);
    *pCameraTypeId = camType;
    //	DEV_LOGI("Camera Type Id=%d", *pCameraTypeId);
    return RET_OK;
}

void NodeMetaData::GetLdcLevel(camera_metadata_t *pMeta, uint32_t *pLdcLevel)
{
    if (pMeta == NULL || pLdcLevel == NULL) {
        DEV_LOGW("GetLdcLevel input error!");
        return;
    }
    *pLdcLevel = 0;
    void *pData = NULL;
    const char *levelTag = "xiaomi.distortion.ultraWideDistortionLevel";
    VendorMetadataParser::getVTagValue(pMeta, levelTag, &pData);
    if (pData != NULL) {
        *pLdcLevel = *reinterpret_cast<uint8_t *>(pData);
        DEV_LOGI("Ldc level : %d", *pLdcLevel);
    } else {
        DEV_LOGI("\"xiaomi.distortion.ultraWideDistortionLevel\" not found!");
    }
    return;
}

S32 NodeMetaData::GetCropRegion(camera_metadata_t *pMetadata, LdcProcessInputInfo *pInputInfo)
{
    DEV_IF_LOGE_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "GetCropRegion pMetaData is null!");
    DEV_IF_LOGE_RETURN_RET(pInputInfo == NULL, RET_ERR_ARG, "GetCropRegion pInputInfo is null!");
    S32 ret;
    void *pData = NULL;
    ret = VendorMetadataParser::getVTagValue(pMetadata, "xiaomi.snapshot.cropRegion", &pData);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetCropRegion failed!");
    pInputInfo->cropRegion = *reinterpret_cast<LDCRECT *>(pData);
    // DEV_LOGI("GetCropRegion first crop:(%d,%d,%d,%d)", pInputInfo->cropRegion.left,
    // pInputInfo->cropRegion.top, pInputInfo->cropRegion.width, pInputInfo->cropRegion.height);

    pData = NULL;
    BOOL remosaicEnabled = FALSE;
    ret = VendorMetadataParser::getVTagValue(pMetadata, "xiaomi.remosaic.enabled", &pData);
    if (0 == ret) {
        remosaicEnabled = *reinterpret_cast<BOOL *>(pData);
        // DEV_LOGI("remosaicEnabled = %d", remosaicEnabled);
    } else {
        DEV_LOGI("GetRemosaicEnabled failed!");
    }

    // uint32_t qcfaFactor = 1;
    // // For burstshot.
    // // TODO：burstshot add phymeta.
    // if (qcfaFactor != 1 && pInputInfo->isSATMode && !pInputInfo->hasPhyMeta) {
    //     pInputInfo->cropRegion.left /= qcfaFactor;
    //     pInputInfo->cropRegion.top /= qcfaFactor;
    //     pInputInfo->cropRegion.width /= qcfaFactor;
    //     pInputInfo->cropRegion.height /= qcfaFactor;
    //     DEV_LOGI("GetCropRegion burstshot override to (%d,%d,%d,%d)",
    //     pInputInfo->cropRegion.left, pInputInfo->cropRegion.top, pInputInfo->cropRegion.width,
    //     pInputInfo->cropRegion.height);
    // }
    // For HD.
    // if (qcfaFactor != 1 && remosaicEnabled == TRUE) {
    //     pInputInfo->cropRegion.left /= qcfaFactor;
    //     pInputInfo->cropRegion.top /= qcfaFactor;
    //     pInputInfo->cropRegion.width /= qcfaFactor;
    //     pInputInfo->cropRegion.height /= qcfaFactor;
    //     DEV_LOGI("GetCropRegion HD to (%d,%d,%d,%d)", pInputInfo->cropRegion.left,
    //     pInputInfo->cropRegion.top, pInputInfo->cropRegion.width, pInputInfo->cropRegion.height);
    // }
    return RET_OK;
}

S32 NodeMetaData::GetRotateAngle(camera_metadata_t *mdata, U64 frameNum, U32 *RotateAngle)
{
    DEV_IF_LOGE_RETURN_RET(RotateAngle == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_ERR;
    void *pData = NULL;
    // alternative tag:"xiaomi.device.orientation"
    ret = GetMetaData(mdata, NULL, ANDROID_JPEG_ORIENTATION, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    *RotateAngle = *reinterpret_cast<S32 *>(pData);
    return RET_OK;
}

S32 NodeMetaData::GetActiveArraySize(camera_metadata_t *pMetaData, LDCRECT *pActiveArraySize)
{
    DEV_IF_LOGE_RETURN_RET(pActiveArraySize == NULL, RET_ERR_ARG, "arg err");
    namespace VMP = VendorMetadataParser;
    S32 ret;
    void *pData = NULL;
    S32 cameraId = 0;
    // Get cameraId:
    ret = VMP::getVTagValue(pMetaData, "xiaomi.snapshot.fwkCameraId", &pData);
    DEV_IF_LOGE_RETURN_RET(ret != 0, ret, "Get MetadataOwner err");
    cameraId = *static_cast<S32 *>(pData);
    // DEV_LOGI("Get cameraId = %d", cameraId);

    // Get ActiveArraySize from static meta:
    camera_metadata_t *staticMeta =
        StaticMetadataWraper::getInstance()->getStaticMetadata(cameraId);
    ret = VMP::getTagValue(staticMeta, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &pData);
    DEV_IF_LOGE_RETURN_RET(ret != 0, ret, "Get ACTIVE_ARRAY_SIZE err");
    *pActiveArraySize = *static_cast<LDCRECT *>(pData);
    // DEV_LOGI("Get ACTIVE_ARRAY_SIZE: (%d,%d,%d,%d)",
    // pActiveArraySize->left, pActiveArraySize->top, pActiveArraySize->width,
    // pActiveArraySize->height);

    return RET_OK;
}
S32 NodeMetaData::GetFaceInfo(camera_metadata_t *pMeta, const LdcProcessInputInfo &inputInfo,
                              LDCFACEINFO *pFaceInfo)
{
    DEV_IF_LOGE_RETURN_RET(pFaceInfo == NULL, RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET(pMeta == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    const char *faceRectTag = "xiaomi.snapshot.faceRect";
    FaceResult *pfaceRect = NULL;
    VendorMetadataParser::getVTagValue(pMeta, faceRectTag, &pData);
    if (NULL != pData) {
        pfaceRect = reinterpret_cast<FaceResult *>(pData);
        pFaceInfo->num = pfaceRect->faceNumber;
    } else {
        pFaceInfo->num = 0;
        DEV_LOGI("Not find faces.");
        return RET_ERR;
    }

    LDCRECT cropRegion = inputInfo.cropRegion;
    float scaleWidth = (float)cropRegion.width / inputInfo.inputBuf->width;
    float scaleHeight = (float)cropRegion.height / inputInfo.inputBuf->height;
    float bufferToCropRatio = (float)inputInfo.inputBuf->width / cropRegion.width;
    const float AspectRatioTolerance = 0.01f;
    S32 topShift = 0;
    S32 leftShift = 0;
    if (scaleHeight > scaleWidth + AspectRatioTolerance) {
        // 16:9 height is cropped.
        S32 deltacropHeight = (S32)((float)inputInfo.inputBuf->height * scaleWidth);
        cropRegion.top = cropRegion.top + (cropRegion.height - deltacropHeight) / 2;
        topShift = (inputInfo.sensorSize.height - inputInfo.inputBuf->height) / 2;
        leftShift = (inputInfo.sensorSize.width - inputInfo.inputBuf->width) / 2;
        bufferToCropRatio = (float)inputInfo.inputBuf->width / cropRegion.width;
    } else if (scaleWidth > scaleHeight + AspectRatioTolerance) {
        // 1:1 width is cropped, only SAT.
        S32 deltacropWidth = (S32)((float)inputInfo.inputBuf->width * scaleHeight);
        cropRegion.left = cropRegion.left + (cropRegion.width - deltacropWidth) / 2;
        topShift = (inputInfo.sensorSize.height - inputInfo.inputBuf->height) / 2;
        leftShift = (inputInfo.sensorSize.width - inputInfo.inputBuf->width) / 2;
        bufferToCropRatio = (float)inputInfo.inputBuf->height / cropRegion.height;
    }

    for (U32 i = 0; i < pFaceInfo->num; ++i) {
        ChiRectEXT curFaceInfo = pfaceRect->chiFaceInfo[i];
        S32 xMin = static_cast<S32>(curFaceInfo.left);
        S32 yMin = static_cast<S32>(curFaceInfo.bottom);
        S32 xMax = static_cast<S32>(curFaceInfo.right);
        S32 yMax = static_cast<S32>(curFaceInfo.top);
        xMin = xMin - (S32)cropRegion.left;
        xMax = xMax - (S32)cropRegion.left;
        yMin = yMin - (S32)cropRegion.top;
        yMax = yMax - (S32)cropRegion.top;
        xMin = (xMin < 0) ? 0 : xMin;
        xMax = (xMax < 0) ? 0 : xMax;
        yMin = (yMin < 0) ? 0 : yMin;
        yMax = (yMax < 0) ? 0 : yMax;
        xMin *= bufferToCropRatio;
        xMax *= bufferToCropRatio;
        yMin *= bufferToCropRatio;
        yMax *= bufferToCropRatio;
        pFaceInfo->face[i].left = xMin;
        pFaceInfo->face[i].top = yMin;
        pFaceInfo->face[i].width = xMax - xMin + 1;
        pFaceInfo->face[i].height = yMax - yMin + 1;
    }

    U32 RotateAngle = 0;
    ret = GetRotateAngle(pMeta, inputInfo.frameNum, &RotateAngle);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetRotateAngle err");

    ret = GetMetaData(pMeta, NULL, ANDROID_STATISTICS_FACE_SCORES, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    for (uint i = 0; i < pFaceInfo->num; i++) {
        U8 *pScore = reinterpret_cast<U8 *>(pData);
        pFaceInfo->score[i] = pScore[i];
        pFaceInfo->orient[i] = RotateAngle;
    }
    for (uint i = 0; i < pFaceInfo->num; i++) {
        DEV_LOGI("FACE INFO frameNum=%" PRIu64 ",%d/%d,(%d,%d,%d,%d),score=%d orient=%d",
                 inputInfo.frameNum, i + 1, pFaceInfo->num, pFaceInfo->face[i].left,
                 pFaceInfo->face[i].top, pFaceInfo->face[i].width, pFaceInfo->face[i].height,
                 pFaceInfo->score[i], pFaceInfo->orient[i]);
    }
    return RET_OK;
}

S32 NodeMetaData::SetFaceInfo(camera_metadata_t *mdata, U64 frameNum, NODE_METADATA_MODE mode,
                              NODE_METADATA_FACE_INFO *pFaceInfo)
{
    DEV_IF_LOGE_RETURN_RET(pFaceInfo == NULL, RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET(pFaceInfo->imgHeight == 0, RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET(pFaceInfo->imgWidth == 0, RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET(pFaceInfo->sensorActiveArraySize.width == 0, RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET(pFaceInfo->sensorActiveArraySize.height == 0, RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET(pFaceInfo->num >= NODE_METADATA_FACE_INFO_NUM_MAX, RET_ERR_ARG,
                           "face num err");
    U32 faceRect[NODE_METADATA_FACE_INFO_NUM_MAX * 4];
    for (uint i = 0, j = 0; i < pFaceInfo->num; i++, j += 4) {
        U32 xmin = pFaceInfo->face[i].left;
        U32 ymin = pFaceInfo->face[i].top + pFaceInfo->topOffset;
        U32 xmax = xmin + pFaceInfo->face[i].width - 1;
        U32 ymax = ymin + pFaceInfo->face[i].height - 1;
        float ratio = (float)pFaceInfo->sensorActiveArraySize.width / pFaceInfo->imgWidth;
        // convert face rectangle to be based on current input dimension
        faceRect[j + 0] = xmin * ratio;
        faceRect[j + 1] = ymin * ratio;
        faceRect[j + 2] = xmax * ratio;
        faceRect[j + 3] = ymax * ratio;
    }
    S32 ret =
        SetMetaData(mdata, NULL, ANDROID_STATISTICS_FACE_RECTANGLES, &faceRect, pFaceInfo->num * 4);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "set metadata err");
    return RET_OK;
}

// S32 NodeMetaData::GetRoleTypeId(camera_metadata_t *mdata, U64 frameNum, U32 *pRoleTypeId)
// {
//     S32 ret;
//     void *pData = NULL;
//     ret = GetMetaData(mdata, "com.qti.chi.multicamerainfo.MultiCameraIds", 0, &pData, NULL);
//     DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "Get metadata err");
//     CameraRoleType currentCameraRole = CameraRoleTypeDefault;
//     MultiCameraIds multiCameraId = *static_cast<MultiCameraIds *>(pData);
//     currentCameraRole = multiCameraId.currentCameraRole;
//     *pRoleTypeId = currentCameraRole;
//     //    DEV_LOGI("CAMERA TYPE : currentId = %d logicalId = %d RoleTypeId = %d masterRoleId=%d",
//     //    multiCameraId.currentCameraId,
//     //            multiCameraId.logicalCameraId, multiCameraId.currentCameraRole,
//     //            multiCameraId.masterCameraId);
//     return ret;
// }

S32 NodeMetaData::GetSensorSize(camera_metadata_t *mdata, MIRECT *pSensorSize)
{
    DEV_IF_LOGE_RETURN_RET(pSensorSize == NULL, RET_ERR_ARG, "arg err");
    S32 ret;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.sensorSize.size", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    CHIDIMENSION *dimensionSize = (CHIDIMENSION *)(pData);
    pSensorSize->left = 0;
    pSensorSize->top = 0;
    pSensorSize->height = dimensionSize->height;
    pSensorSize->width = dimensionSize->width;
    //  DEV_LOGI("Get SensorSize %d %d", pSensorSize->width, pSensorSize->height);
    return RET_OK;
}

S32 NodeMetaData::GetZoomRatio(camera_metadata_t *pMetadata, float *pZoomRatio)
{
    S32 ret;
    void *pData = NULL;
    ret = VendorMetadataParser::getTagValue(pMetadata, ANDROID_CONTROL_ZOOM_RATIO, &pData);
    DEV_IF_LOGE_RETURN_RET(ret != 0, ret, "Get ZoomRatio err");
    *pZoomRatio = *static_cast<float *>(pData);
    // DEV_LOGI("Get ZoomRatio %f ", *pZoomRatio);
    return RET_OK;
}

// S32 NodeMetaData::GetAppMode(camera_metadata_t *pMetadata, BOOL *pIsSATMode)
// {
//     DEV_IF_LOGE_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "GetAppMode pMetadata is null!");
//     S32 ret = RET_OK;
//     void *pData = NULL;
//     ret = VendorMetadataParser::getVTagValue(pMetadata, "xiaomi.app.module", &pData);
//     DEV_IF_LOGE_RETURN_RET(ret != 0, ret, "GetAppMode failed!");
//     MiuiCameraAppModule appMode = *reinterpret_cast<MiuiCameraAppModule *>(pData);
//     if (appMode == MiuiCameraAppModule::MODE_CAPTURE) {
//         *pIsSATMode = TRUE;
//     }
//     // DEV_LOGI("GetAppMode: %#X", appMode);
//     return RET_OK;
// }

S32 NodeMetaData::GetCaptureHint(camera_metadata_t *pMetadata, BOOL *pIsBurstShot)
{
    DEV_IF_LOGW_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "GetCaptureHint pMetadata is null!");
    S32 ret = RET_OK;
    void *pData = NULL;
    S32 captureHint = 0;
    ret = VendorMetadataParser::getVTagValue(pMetadata, "xiaomi.burst.captureHint", &pData);
    DEV_IF_LOGW_RETURN_RET(ret != 0, ret, "GetCaptureHint failed!");
    if (pData != NULL) {
        captureHint = *static_cast<int32_t *>(pData);
        *pIsBurstShot = captureHint;
    }
    // DEV_LOGI("GetCaptureHint %d", captureHint);
    return RET_OK;
}
} // namespace mialgo2
