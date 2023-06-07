#include "nodeMetaDataUp.h"

#include <system/camera_metadata.h>

#include "device/device.h"
namespace mialgo2 {

#undef LOG_TAG
#define LOG_TAG                     "LDC-METADATA-UP"
#define DEFAULT_ULTRA_SENSOR_WIDTH  3264
#define DEFAULT_ULTRA_SENSOR_HEIGHT 2448
NodeMetaData::NodeMetaData()
{ // @suppress("Class members should be properly initialized")
}

NodeMetaData::~NodeMetaData() {}

S32 NodeMetaData::GetMetaData(void *mdata, const char *pTagName, U32 tag, void **ppData, U32 *pSize)
{
    // DEV_IF_LOGE_RETURN_RET((NULL == MetadataUtils::getInstance()), RET_ERR_ARG, "pArg is null");
    DEV_IF_LOGE_RETURN_RET((NULL == mdata), RET_ERR_ARG, "pArg is null");
    DEV_IF_LOGE_RETURN_RET(((NULL == pTagName) && (tag == 0)), RET_ERR_ARG, "pArg is null");
    S32 ret = RET_ERR;
    if (pTagName != NULL) {
        ret = VendorMetadataParser::getVTagValue((camera_metadata_t *)mdata, pTagName, ppData);
    } else {
        ret = VendorMetadataParser::getTagValue((camera_metadata_t *)mdata, tag, ppData);
    }
    DEV_IF_LOGE_RETURN_RET((*ppData == NULL), RET_ERR_ARG, "GetMetadata err");
    return ret;
}

S32 NodeMetaData::SetMetaData(void *mdata, const char *pTagName, U32 tag, void *pData, U32 size)
{
    // DEV_IF_LOGE_RETURN_RET((NULL == MetadataUtils::getInstance()), RET_ERR_ARG, "pArg is null");
    DEV_IF_LOGE_RETURN_RET((NULL == mdata), RET_ERR_ARG, "pArg is null");
    DEV_IF_LOGE_RETURN_RET(((NULL == pTagName) && (tag == 0)), RET_ERR_ARG, "pArg is null");
    S32 ret = RET_ERR;
    if (pTagName != NULL) {
        VendorMetadataParser::setVTagValue((camera_metadata_t *)mdata, pTagName, pData, size);
    } else {
        VendorMetadataParser::setTagValue((camera_metadata_t *)mdata, tag, pData, size);
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

S32 NodeMetaData::GetCameraVendorId(void *mdata, U32 *pCameraVendorId)
{
    DEV_IF_LOGE_RETURN_RET(pCameraVendorId == NULL, RET_ERR_ARG, "arg err");
    void *pData = NULL;
    S32 ret = GetMetaData(mdata, "xiaomi.sensor.vendorId", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "GetMetadata err");
    U32 VendorId = *reinterpret_cast<U32 *>(pData);
    *pCameraVendorId = VendorId;
    //  DEV_LOGI("Camera Vendor Id=%d", *pCameraVendorId);
    return RET_OK;
}

S32 NodeMetaData::SetCameraVendorId(void *mdata, U64 frameNum, U32 *pCameraVendorId)
{
    DEV_IF_LOGE_RETURN_RET(pCameraVendorId == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    ret = SetMetaData(mdata, "xiaomi.sensor.vendorId", 0, pCameraVendorId, sizeof(U32));
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "set metadata err");
    return RET_OK;
}

S32 NodeMetaData::GetCameraTypeId(void *mdata, U32 *pCameraTypeId)
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

S32 NodeMetaData::GetLdcLevel(void *mdata, U64 frameNum, U32 *pLdcLevel)
{
    DEV_IF_LOGE_RETURN_RET(pLdcLevel == NULL, RET_ERR_ARG, "arg err");
    S32 ret;
    void *pData = NULL;
    U8 ldcLevel = 0;
    ret = GetMetaData(mdata, "xiaomi.distortion.ultraWideDistortionLevel", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    ldcLevel = pData ? *reinterpret_cast<U8 *>(pData) : 0;
    *pLdcLevel = ldcLevel;
    //	DEV_LOGI("LDC level=%d", *pLdcLevel);
    return RET_OK;
}

S32 NodeMetaData::GetCropRegion(void *mdata, U64 frameNum, MIRECT *pCropRegion,
                                uint32_t mode = StreamConfigModeSAT)
{
    DEV_IF_LOGE_RETURN_RET(pCropRegion == NULL, RET_ERR_ARG, "arg err");
    S32 ret;
    void *pData = NULL;
    // if (mode == StreamConfigModeSAT  ){
    //    ret = MetadataUtils::getInstance()->getVTagValue(mdata,
    //    "xiaomi.superResolution.cropRegionMtk", &pData);
    //}
    // else if((mode == StreamConfigModeMiuiManual) || (mode == StreamConfigModeMiuiUWSuperNight) ||
    // (mode == StreamConfigModeAlgoupNormal)){
    ret = GetMetaData(mdata, NULL, ANDROID_SCALER_CROP_REGION, &pData, NULL);
    //}

    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    *pCropRegion = *reinterpret_cast<MIRECT *>(pData);
    DEV_LOGI("get crop region from : [%d %d %d %d]", pCropRegion->left, pCropRegion->top,
             pCropRegion->width, pCropRegion->height);
    //      DEV_LOGI("GetCropRegion frame %" PRIu64 " crop(%d,%d,%d,%d)",
    //      frameNum,pCropRegion->left, pCropRegion->top, pCropRegion->width, pCropRegion->height);
    return RET_OK;
}

S32 NodeMetaData::GetCropRegionRef(void *mdata, U64 frameNum, MIRECT *pCropRegionRef)
{
    DEV_IF_LOGE_RETURN_RET(pCropRegionRef == NULL, RET_ERR_ARG, "arg err");
    S32 ret;
    void *pData = NULL;
    ret = GetMetaData(mdata, "org.quic.camera2.ref.cropsize.RefCropSize", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    MIDIMENSION *CropRegionRef = reinterpret_cast<MIDIMENSION *>(pData);
    pCropRegionRef->top = 0;
    pCropRegionRef->left = 0;
    pCropRegionRef->height = CropRegionRef->height;
    pCropRegionRef->width = CropRegionRef->width;
    //    DEV_LOGI("CropRegionRef frame %" PRIu64 " cropRef(%d,%d,%d,%d)",
    //    frameNum,pCropRegionRef->left, pCropRegionRef->top, pCropRegionRef->width,
    //    pCropRegionRef->height);
    return RET_OK;
}

S32 NodeMetaData::GetRotateAngle(void *mdata, U64 frameNum, U32 *RotateAngle,
                                 NODE_METADATA_MODE mode)
{
    DEV_IF_LOGE_RETURN_RET(RotateAngle == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_ERR;
    void *pData = NULL;
    if (mode == NODE_METADATA_MODE_PREVIEW) {
        ret = GetMetaData(mdata, "xiaomi.device.orientation", 0, &pData, NULL);
    } else if (mode == NODE_METADATA_MODE_CAPTURE) {
        ret = GetMetaData(mdata, NULL, ANDROID_JPEG_ORIENTATION, &pData, NULL);
    }
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    *RotateAngle = *reinterpret_cast<S32 *>(pData);
    return RET_OK;
}

S32 NodeMetaData::GetActiveArraySize(void *mdata, MIRECT *pActiveArraySize)
{
    DEV_IF_LOGE_RETURN_RET(pActiveArraySize == NULL, RET_ERR_ARG, "arg err");
    S32 ret;
    void *pData = NULL;
    ret = GetMetaData(mdata, NULL, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    *pActiveArraySize = *reinterpret_cast<MIRECT *>(pData);
    DEV_LOGI("GetActiveArraySize() rect(%d,%d,%d,%d)", pActiveArraySize->left,
             pActiveArraySize->top, pActiveArraySize->width, pActiveArraySize->height);
    return RET_OK;
}

S32 NodeMetaData::GetFaceInfo(void *mdata, U64 frameNum, NODE_METADATA_MODE mode,
                              NODE_METADATA_FACE_INFO *pFaceInfo, MIRECT cropRegin,
                              MIRECT sensorSize)
{
    DEV_IF_LOGE_RETURN_RET(pFaceInfo == NULL, RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET(pFaceInfo->imgHeight == 0, RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET(pFaceInfo->imgWidth == 0, RET_ERR_ARG, "arg err");
    S32 ret;
    void *pData = NULL;
    U32 dataSize = 0;
    U32 faceNum = 0;
    size_t tcount = 0;

    // dont process aildc on cshot
    bool cshotenable = false;
    VendorMetadataParser::getVTagValue((camera_metadata_t *)mdata,
                                       "com.mediatek.cshotfeature.capture", &pData);
    if (NULL != pData) {
        cshotenable = *static_cast<uint8_t *>(pData);
        DEV_LOGI("get cshotenable: %d", cshotenable);
        if (cshotenable)
            return RET_OK;
    }

    pData = NULL;

    ret = VendorMetadataParser::getVTagValueCount(
        (camera_metadata_t *)mdata, "xiaomi.statistics.faceRectangles", &pData, &tcount);
    DEV_LOGI("get xiaomi.statistics.faceRectangles %d", ret);
    if (NULL == pData) {
        ret = VendorMetadataParser::getTagValueCount(
            (camera_metadata_t *)mdata, ANDROID_STATISTICS_FACE_RECTANGLES, &pData, &tcount);
        DEV_LOGI("get ANDROID_STATISTICS_FACE_RECTANGLES %d", ret);
    }

    if (NULL != pData) {
        faceNum = ((int32_t)tcount) / 4;
        if (0 == faceNum) {
            return RET_OK;
        }

        DEV_LOGI("ldc capture faceNum %d,%d", faceNum, ret);
    } else {
        DEV_LOGI("It did not get face tag and do not detect face.");
        return RET_ERR;
    }

    pFaceInfo->num =
        faceNum > NODE_METADATA_FACE_INFO_NUM_MAX ? NODE_METADATA_FACE_INFO_NUM_MAX : faceNum;
    const float AspectRatioTolerance = 0.01f;
    int gapHeight = 0;
    float ratioWidth = (float)pFaceInfo->imgWidth / sensorSize.width;
    float ratioHeight = (float)pFaceInfo->imgHeight / sensorSize.height;
    float heigthRwidth = (float)sensorSize.height / sensorSize.width;

    if (ratioWidth > (ratioHeight + AspectRatioTolerance)) {
        // 16:9 case run to here, it means height is cropped.
        gapHeight = ((pFaceInfo->imgWidth * heigthRwidth) - pFaceInfo->imgHeight) / 2;
    } // sh

    for (uint i = 0, j = 0; i < pFaceInfo->num; i++) {
        S32 xmin = reinterpret_cast<S32 *>(pData)[j++];
        S32 ymin = reinterpret_cast<S32 *>(pData)[j++];
        S32 xmax = reinterpret_cast<S32 *>(pData)[j++];
        S32 ymax = reinterpret_cast<S32 *>(pData)[j++];

        DEV_LOGI("Ldc capture face origin: x1 = %d y1 = %d  x2 = %d y2 = %d gapHeight = %d.", xmin,
                 ymin, xmax, ymax, gapHeight);

        xmin = xmin * ratioWidth;
        xmax = xmax * ratioWidth;
        ymin = ymin * ratioWidth;
        ymax = ymax * ratioWidth;

        ymin = ymin - gapHeight;
        ymax = ymax - gapHeight;

        xmin = xmin > 0 ? xmin : 0;
        xmax = xmax > 0 ? xmax : 0;
        ymin = ymin > 0 ? ymin : 0;
        ymax = ymax > 0 ? ymax : 0;

        // convert face rectangle to be based on current input dimension
        pFaceInfo->face[i].left = xmin;
        pFaceInfo->face[i].top = ymin;
        pFaceInfo->face[i].width = xmax - xmin;
        pFaceInfo->face[i].height = ymax - ymin;
    }
    U32 RotateAngle = 0;
    ret = GetRotateAngle(mdata, frameNum, &RotateAngle, mode);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetRotateAngle err");

    ret = GetMetaData(mdata, NULL, ANDROID_STATISTICS_FACE_SCORES, &pData, &dataSize);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    for (uint i = 0; i < pFaceInfo->num; i++) {
        U8 *pScore = reinterpret_cast<U8 *>(pData);
        pFaceInfo->score[i] = pScore[i];
        pFaceInfo->orient[i] = RotateAngle;
    }
    for (uint i = 0; i < pFaceInfo->num; i++) {
        DEV_LOGI("FACE INFO frameNum=%" PRIu64 ",%d/%d,(%d,%d,%d,%d),score=%d orient=%d", frameNum,
                 i + 1, faceNum, pFaceInfo->face[i].left, pFaceInfo->face[i].top,
                 pFaceInfo->face[i].width, pFaceInfo->face[i].height, pFaceInfo->score[i],
                 pFaceInfo->orient[i]);
    }
    pFaceInfo->topOffset = 0;
    return RET_OK;
}

S32 NodeMetaData::SetFaceInfo(void *mdata, U64 frameNum, NODE_METADATA_MODE mode,
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
S32 NodeMetaData::GetQcfa(void *mdata, U64 frameNum, U32 *pQcfa)
{
    DEV_IF_LOGE_RETURN_RET(pQcfa == NULL, RET_ERR_ARG, "arg err");
    S32 ret;
    void *pData = NULL;
    U8 qcfa_f = 0;
    ret = GetMetaData(mdata, "xiaomi.quadcfa.supported", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    qcfa_f = pData ? *reinterpret_cast<U8 *>(pData) : 0;
    *pQcfa = qcfa_f;
    //    DEV_LOGI("QCFA CAM=%d", *pQcfa);
    return ret;
}

S32 NodeMetaData::GetRoleTypeId(void *mdata, U64 frameNum, U32 *pRoleTypeId)
{
    S32 ret;
    void *pData = NULL;
    ret = GetMetaData(mdata, "com.qti.chi.multicamerainfo.MultiCameraIds", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "Get metadata err");
    CameraRoleType currentCameraRole = CameraRoleTypeDefault;
    MultiCameraIds multiCameraId = *static_cast<MultiCameraIds *>(pData);
    currentCameraRole = multiCameraId.currentCameraRole;
    *pRoleTypeId = currentCameraRole;
    //    DEV_LOGI("CAMERA TYPE : currentId = %d logicalId = %d RoleTypeId = %d masterRoleId=%d",
    //    multiCameraId.currentCameraId,
    //            multiCameraId.logicalCameraId, multiCameraId.currentCameraRole,
    //            multiCameraId.masterCameraId);
    return ret;
}

S32 NodeMetaData::GetSensorSize(void *mdata, MIRECT *pSensorSize)
{
    DEV_IF_LOGE_RETURN_RET(pSensorSize == NULL, RET_ERR_ARG, "arg err");
    S32 ret;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.sensorSize.size", 0, &pData, NULL);
    if ((ret != RET_OK) || (pData == NULL)) {
        pSensorSize->left = 0;
        pSensorSize->top = 0;
        pSensorSize->height = DEFAULT_ULTRA_SENSOR_HEIGHT;
        pSensorSize->width = DEFAULT_ULTRA_SENSOR_WIDTH;
        DEV_LOGI("set SensorSize default value %d %d", pSensorSize->width, pSensorSize->height);
    }
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    MIDIMENSION *dimensionSize = (MiDimension *)(pData);
    pSensorSize->left = 0;
    pSensorSize->top = 0;
    pSensorSize->height = dimensionSize->height;
    pSensorSize->width = dimensionSize->width;
    DEV_LOGI("Get SensorSize %d %d", pSensorSize->width, pSensorSize->height);
    return RET_OK;
}

S32 NodeMetaData::GetZoomRatio(void *mdata, float *pZoomRatio)
{
    S32 ret;
    void *pData = NULL;
    ret = GetMetaData(mdata, NULL, ANDROID_CONTROL_ZOOM_RATIO, &pData, NULL);

    DEV_IF_LOGE_RETURN_RET(ret != 0, ret, "Get ZoomRatio err");
    *pZoomRatio = *reinterpret_cast<float *>(pData);
    DEV_LOGI("Get ZoomRatio %f %f %d", *pZoomRatio, ret);
    return RET_OK;
}

} // namespace mialgo2
