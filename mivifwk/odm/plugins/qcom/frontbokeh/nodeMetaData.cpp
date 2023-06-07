#include "nodeMetaData.h"

#include <cutils/properties.h>
#include <system/camera_metadata.h>
#include "device/device.h"

namespace mialgo2 {

#undef LOG_TAG
#define LOG_TAG "FRONT_BOKEH_CAPTURE_META"

#undef LOG_GROUP
#define LOG_GROUP GROUP_PLUGIN

#define FACE_INFO_NUM_MAX 10

NodeMetaData::NodeMetaData() : m_mode(0), m_debug(0) {}

NodeMetaData::~NodeMetaData() {}

S32 NodeMetaData::GetMetaData(camera_metadata_t *pMetadata, const char *pTagName, U32 tag,
                              void **ppData, U32 *pSize)
{
    DEV_IF_LOGE_RETURN_RET((NULL == pMetadata), RET_ERR_ARG, "pMetadata is null");
    DEV_IF_LOGE_RETURN_RET(((NULL == pTagName) && (tag == 0)), RET_ERR_ARG, "pTagName is null");
    S32 ret = RET_ERR;
    if (pTagName != NULL) {
        ret = VendorMetadataParser::getVTagValue(pMetadata, pTagName, ppData);
    } else {
        ret = VendorMetadataParser::getTagValue(pMetadata, tag, ppData);
    }
    DEV_IF_LOGW_RETURN_RET((*ppData == NULL), RET_ERR_ARG, "GetMetadata err");
    return ret;
}

S32 NodeMetaData::SetMetaData(camera_metadata_t *pMetadata, const char *pTagName, U32 tag,
                              void *pData, U32 size)
{
    DEV_IF_LOGE_RETURN_RET((NULL == pMetadata), RET_ERR_ARG, "pMetadata is null");
    DEV_IF_LOGE_RETURN_RET(((NULL == pTagName) && (tag == 0)), RET_ERR_ARG, "pTagName is null");
    return VendorMetadataParser::setVTagValue(pMetadata, pTagName, pData, size);
}

S32 NodeMetaData::Init(U32 mode, U32 debug)
{
    m_mode = mode;
    m_debug = debug;
    return RET_OK;
}

S32 NodeMetaData::GetFNumberApplied(camera_metadata_t *pMetadata, FP32 *pFNumber)
{
    DEV_IF_LOGE_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "pMetadata is null!");
    DEV_IF_LOGE_RETURN_RET(pFNumber == NULL, RET_ERR_ARG, "pCropRegion is null!");

    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(pMetadata, "xiaomi.bokeh.fNumberApplied", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK || pData == NULL), RET_ERR, "GetMetaData err");

    CHAR fNumberChar[8] = {0};
    Dev_memcpy(fNumberChar, static_cast<CHAR *>(pData), sizeof(fNumberChar));
    (*pFNumber) = atof(fNumberChar);
    return ret;
}

S32 NodeMetaData::GetOrientation(camera_metadata_t *pMetadata, S32 *pOrientation)
{
    DEV_IF_LOGE_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "pMetadata is null!");
    DEV_IF_LOGE_RETURN_RET(pOrientation == NULL, RET_ERR_ARG, "arg err");

    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(pMetadata, "xiaomi.device.orientation", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK || pData == NULL), RET_ERR, "GetMetaData err");
    (*pOrientation) = *reinterpret_cast<S32 *>(pData);
    DEV_LOGI("Orientation =%d", *pOrientation);
    return RET_OK;
}

S32 NodeMetaData::GetRotateAngle(camera_metadata_t *pMetadata, S32 *pRotateAngle)
{
    DEV_IF_LOGE_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "pMetadata is null!");
    DEV_IF_LOGE_RETURN_RET(pRotateAngle == NULL, RET_ERR_ARG, "arg err");

    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(pMetadata, NULL, ANDROID_JPEG_ORIENTATION, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK || pData == NULL), RET_ERR, "GetMetaData err");
    (*pRotateAngle) = *reinterpret_cast<S32 *>(pData);
    DEV_LOGI("RotateAngle =%d", *pRotateAngle);
    return RET_OK;
}

S32 NodeMetaData::GetSensitivity(camera_metadata_t *pMetadata, S32 *pSensitivity)
{
    DEV_IF_LOGE_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "pMetadata is null!");
    DEV_IF_LOGE_RETURN_RET(pSensitivity == NULL, RET_ERR_ARG, "arg err");

    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(pMetadata, NULL, ANDROID_SENSOR_SENSITIVITY, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK || pData == NULL), RET_ERR, "GetMetaData err");
    (*pSensitivity) = (S32)(*reinterpret_cast<int *>(pData));
    DEV_LOGI("sensitivity =%d", *pSensitivity);
    return ret;
}

S32 NodeMetaData::GetThermalLevel(camera_metadata_t *pMetadata, S32 *pThermalLevel)
{
    DEV_IF_LOGE_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "pMetadata is null!");
    DEV_IF_LOGE_RETURN_RET(pThermalLevel == NULL, RET_ERR_ARG, "arg err");

    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(pMetadata, "xiaomi.thermal.thermalLevel", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK || pData == NULL), RET_ERR, "GetMetaData err");
    (*pThermalLevel) = (S32)(*reinterpret_cast<int *>(pData));
    DEV_LOGI("thermalLevel =%d", *pThermalLevel);
    return ret;
}

S32 MapRectToInput(CHIRECT &SrcRect, CHIRECT &SrcRefer, S32 DstReferInputWidth,
                   S32 DstReferInputHeight, S32 rotateAngle, DEV_IMAGE_RECT &DstRect)
{
    DEV_IF_LOGE_RETURN_RET(
        (SrcRect.width <= 0 || SrcRect.height <= 0 || SrcRefer.width <= 0 || SrcRefer.height <= 0 ||
         DstReferInputWidth <= 0 || DstReferInputHeight <= 0),
        RET_ERR, "arg err");

    if (SrcRect.left > SrcRefer.left) {
        SrcRect.left -= SrcRefer.left;
    } else {
        SrcRect.left = 0;
    }
    if (SrcRect.top > SrcRefer.top) {
        SrcRect.top -= SrcRefer.top;
    } else {
        SrcRect.top = 0;
    }

    float fLRRatio = DstReferInputWidth / (float)SrcRefer.width;
    // float fTBRatio = nDataHeight / (float)rtIFERect.height;
    int iTBOffset =
        (((int)SrcRefer.height) - (int)SrcRefer.width * DstReferInputHeight / DstReferInputWidth) /
        2;
    if (iTBOffset < 0) {
        iTBOffset = 0;
    }
    if (SrcRect.top > (float)iTBOffset) {
        DstRect.top = round((SrcRect.top - iTBOffset) * fLRRatio);
    } else {
        DstRect.top = 0;
    }
    DstRect.left = round(SrcRect.left * fLRRatio);
    DstRect.width = floor(SrcRect.width * fLRRatio);
    DstRect.height = floor(SrcRect.height * fLRRatio);
    DstRect.width &= 0xFFFFFFFE;
    DstRect.height &= 0xFFFFFFFE;

    // snapt front do flip
    if (rotateAngle == 0 || rotateAngle == 180) {
        DstRect.top = DstReferInputHeight - (DstRect.top + DstRect.height);
        if (DstRect.top > DstReferInputHeight) {
            DstRect.top = 0;
        }
    } else {
        DstRect.left = DstReferInputWidth - (DstRect.left + DstRect.width);
        if (SrcRect.left > DstReferInputWidth) {
            SrcRect.left = 0;
        }
    }

    return RET_OK;
}

S32 NodeMetaData::GetCropRegion(camera_metadata_t *pMetadata, CHIRECT *pCropRegion)
{
    DEV_IF_LOGE_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "pMetadata is null!");
    DEV_IF_LOGE_RETURN_RET(pCropRegion == NULL, RET_ERR_ARG, "pCropRegion is null!");

    S32 ret = RET_OK;
    void *pData = NULL;
    ret = VendorMetadataParser::getVTagValue(pMetadata, "xiaomi.snapshot.cropRegion", &pData);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK || pData == NULL), RET_ERR, "GetMetaData err");

    *pCropRegion = *reinterpret_cast<CHIRECT *>(pData);

    DEV_LOGI("CropRegion: (%d,%d,%d,%d)", pCropRegion->left, pCropRegion->top, pCropRegion->width,
             pCropRegion->height);

    return RET_OK;
}

S32 NodeMetaData::GetActiveArraySize(camera_metadata_t *pMetadata, CHIRECT *pActiveArraySize)
{
    DEV_IF_LOGE_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "pMetadata NULL");
    DEV_IF_LOGE_RETURN_RET(pActiveArraySize == NULL, RET_ERR_ARG, "pActiveArraySize is NULL");

    S32 ret = RET_OK;
    void *pData = NULL;
    S32 cameraId = 0;
    // Get cameraId:
    ret = VendorMetadataParser::getVTagValue(pMetadata, "xiaomi.snapshot.fwkCameraId", &pData);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK || pData == NULL), RET_ERR, "Get fwkCameraId err");
    cameraId = *static_cast<S32 *>(pData);

    // Get ActiveArraySize from static meta:
    camera_metadata_t *staticMeta =
        StaticMetadataWraper::getInstance()->getStaticMetadata(cameraId);
    ret = VendorMetadataParser::getTagValue(staticMeta, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
                                            &pData);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK || pData == NULL), RET_ERR, "Get ACTIVE_ARRAY_SIZE err");
    int *pArray = static_cast<int *>(pData);
    *pActiveArraySize = *reinterpret_cast<CHIRECT *>(pData);
    DEV_LOGI("cameraId %d Get ACTIVE_ARRAY_SIZE: (%d, %d, %d, %d) %d %d %d %d", cameraId,
             pActiveArraySize->left, pActiveArraySize->top, pActiveArraySize->width,
             pActiveArraySize->height, pArray[0], pArray[1], pArray[2], pArray[3]);

    return RET_OK;
}

S32 NodeMetaData::GetMaxFaceRect(camera_metadata_t *pMetadata, S32 width, S32 height,
                                 AlgoMultiCams::MiFaceRect *pFaceRect,
                                 AlgoMultiCams::MiPoint2i *pFaceCentor)
{
    DEV_IF_LOGE_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "pMetadata NULL");
    DEV_IF_LOGE_RETURN_RET((width <= 0 || height <= 0), RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET(pFaceRect == NULL, RET_ERR_ARG, "faceRect NULL");

    S32 ret = RET_OK;
    void *pData = NULL;

    if (NULL != pFaceCentor) {
        pFaceCentor->x = width / 2;
        pFaceCentor->y = height / 2;
    }

    pFaceRect->x = 0;
    pFaceRect->y = 0;
    pFaceRect->width = 0;
    pFaceRect->height = 0;

#if 0
    CHIRECT activeArraySize = {0};
    ret = GetActiveArraySize(pMetadata, &activeArraySize);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetActiveArraySize err");
    activeArraySize.left *= 2;
    activeArraySize.top *= 2;
    activeArraySize.width *= 2;
    activeArraySize.height *= 2;
#endif

    CHIRECT cropRegion = {0};
    ret = GetCropRegion(pMetadata, &cropRegion);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetCropRegion err");

    S32 rotateAngle = 0;
    ret = GetRotateAngle(pMetadata, &rotateAngle);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetOrientation err");

    ret = GetMetaData(pMetadata, (char *)"xiaomi.snapshot.faceRect", 0, &pData, NULL);
    DEV_IF_LOGW_RETURN_RET((ret != RET_OK || pData == NULL), RET_OK, "No face");

    FaceResult *pFaceResult = reinterpret_cast<FaceResult *>(pData);
    S32 faceNum = pFaceResult->faceNumber;
    DEV_IF_LOGI_RETURN_RET(faceNum == 0, RET_OK, "faceNum = 0");

    U64 LargeFaceRegion = 0;
    for (int i = 0; i < faceNum && i < FACE_INFO_NUM_MAX; i++) {
        UINT32 dataIndex = 0;
        S32 xMin = pFaceResult->chiFaceInfo[i].left;
        S32 yMin = pFaceResult->chiFaceInfo[i].bottom;
        S32 xMax = pFaceResult->chiFaceInfo[i].right;
        S32 yMax = pFaceResult->chiFaceInfo[i].top;

        CHIRECT tmpFaceRect = {U32(xMin), U32(yMin), U32(xMax - xMin + 1), U32(yMax - yMin + 1)};

        DEV_LOGI("face[%d/%d] xMin %d, yMin %d, xMax %d, yMax %d, tmpFaceRect %d %d %d %d", i + 1,
                 faceNum, xMin, yMin, xMax, yMax, tmpFaceRect.left, tmpFaceRect.top,
                 tmpFaceRect.width, tmpFaceRect.height);

        DEV_IMAGE_RECT tmpDstFaceRect = {0};
        MapRectToInput(tmpFaceRect, cropRegion, width, height, rotateAngle, tmpDstFaceRect);

        DEV_LOGI("face[%d/%d] MapRectToInput %d %d %d %d", i + 1, faceNum, tmpDstFaceRect.left,
                 tmpDstFaceRect.top, tmpDstFaceRect.width, tmpDstFaceRect.height);

        if (LargeFaceRegion < (U64)tmpDstFaceRect.width * (U64)tmpDstFaceRect.height) {
            pFaceRect->x = tmpDstFaceRect.left;
            pFaceRect->y = tmpDstFaceRect.top;
            pFaceRect->width = tmpDstFaceRect.width;
            pFaceRect->height = tmpDstFaceRect.height;
            pFaceRect->angle = rotateAngle;

            LargeFaceRegion = (U64)tmpDstFaceRect.width * (U64)tmpDstFaceRect.height;
        }
    }

    if (pFaceCentor && faceNum) {
        pFaceCentor->x = pFaceRect->x + pFaceRect->width / 2;
        pFaceCentor->y = pFaceRect->y + pFaceRect->height / 2;
        if (pFaceCentor->x <= 0 || pFaceCentor->y <= 0 || pFaceCentor->x > width ||
            pFaceCentor->y > height) {
            pFaceCentor->x = width / 2;
            pFaceCentor->y = height / 2;
        }
    }

    /*
    ret = GetMetaData(NULL, NULL, ANDROID_STATISTICS_FACE_SCORES, frameNum, &pData, NULL,
    ChiMetadataDynamic); DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");

    for (int i = 0; i < faceNum && i < FACE_INFO_NUM_MAX; i++) {
        UINT8 *pScore = reinterpret_cast<UINT8 *>(pData);
        DEV_LOGI("[%d/%d] xmin1 %d ymin1 %d xmax1 %d ymax1 %d", i+1, faceNum, pScore[i]);
    }
    */

    return RET_OK;
}

} // namespace mialgo2
