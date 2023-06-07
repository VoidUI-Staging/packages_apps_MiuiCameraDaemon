#include "nodeMetaData.h"

#include <cutils/properties.h>
#include <system/camera_metadata.h>

#include "chioemvendortag.h"
#include "chivendortag.h"
#include "device/device.h"

namespace mialgo2 {

#undef LOG_TAG
#define LOG_TAG "BOKEH_CAPTURE_BOKEH"

#undef LOG_GROUP
#define LOG_GROUP GROUP_PLUGIN

#define ROLE_REARBOKEH2X 61
#define ROLE_REARBOKEH1X 63

#define MAX_LINKED_SESSIONS 2

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
    DEV_IF_LOGW_RETURN_RET((*ppData == NULL), RET_ERR_ARG, "GetMetadata err");
    return ret;
}

S32 NodeMetaData::SetMetaData(camera_metadata_t *mdata, const char *pTagName, U32 tag, void *pData,
                              U32 size)
{
    DEV_IF_LOGE_RETURN_RET((NULL == mdata), RET_ERR_ARG, "pArg is null");
    DEV_IF_LOGE_RETURN_RET(((NULL == pTagName) && (tag == 0)), RET_ERR_ARG, "pArg is null");
    return VendorMetadataParser::setVTagValue(mdata, pTagName, pData, size);
}

S32 NodeMetaData::Init(U32 mode, U32 debug)
{
    m_mode = mode;
    m_debug = debug;
    return RET_OK;
}

S32 NodeMetaData::GetFNumberApplied(camera_metadata_t *mdata, FP32 *fNumber)
{
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.bokeh.fNumberApplied", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    CHAR fNumberChar[8] = {0};
    Dev_memcpy(fNumberChar, static_cast<CHAR *>(pData), sizeof(fNumberChar));
    (*fNumber) = atof(fNumberChar);
    return ret;
}

S32 NodeMetaData::GetRotateAngle(camera_metadata_t *mdata, U32 *RotateAngle)
{
    DEV_IF_LOGE_RETURN_RET(RotateAngle == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_ERR;
    void *pData = NULL;
    // alternative tag:"xiaomi.device.orientation"
#if defined(ZIYI_CAM)
    ret = GetMetaData(mdata, "xiaomi.device.orientation", 0, &pData, NULL);
#else
    ret = GetMetaData(mdata, NULL, ANDROID_JPEG_ORIENTATION, &pData, NULL);
#endif
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*RotateAngle) = *reinterpret_cast<S32 *>(pData);
    return RET_OK;
}

S32 NodeMetaData::GetInputMetadataBokeh(camera_metadata_t *mdata,
                                        InputMetadataBokeh *pInputMetaBokeh)
{
    DEV_IF_LOGE_RETURN_RET(pInputMetaBokeh == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "com.qti.chi.multicamerainputmetadata.InputMetadataBokeh", 0, &pData,
                      NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*pInputMetaBokeh) = *reinterpret_cast<InputMetadataBokeh *>(pData);
    return ret;
}

S32 NodeMetaData::GetMDMode(camera_metadata_t *mdata, S32 *mdmode)
{
    DEV_IF_LOGE_RETURN_RET(mdmode == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.bokeh.MDMode", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata MDMode err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata MDMode err");
    (*mdmode) = *reinterpret_cast<int *>(pData);
    return ret;
}

S32 NodeMetaData::GetLightingMode(camera_metadata_t *mdata, S32 *lightingMode)
{
    DEV_IF_LOGE_RETURN_RET(lightingMode == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.portrait.lighting", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*lightingMode) = (S32)(*reinterpret_cast<int *>(pData));
    //    DEV_LOGI("lightingMode =%d", *lightingMode);
    return ret;
}

S32 NodeMetaData::GetHdrEnabled(camera_metadata_t *mdata, S32 *hdrEnabled)
{
    DEV_IF_LOGE_RETURN_RET(hdrEnabled == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.bokeh.hdrEnabled", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata hdrEnabled err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata hdrEnabled err");
    (*hdrEnabled) = (S32)(*reinterpret_cast<bool *>(pData));
    //    DEV_LOGI("hdrEnabled =%d", *hdrEnabled);
    return ret;
}

S32 NodeMetaData::GetAsdEnabled(camera_metadata_t *mdata, S32 *asdEnabled)
{
    DEV_IF_LOGE_RETURN_RET(asdEnabled == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.ai.asd.enabled", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*asdEnabled) = (S32)(*reinterpret_cast<int *>(pData));
    //    DEV_LOGI("asdEnabled =%d", *asdEnabled);
    return ret;
}
S32 NodeMetaData::GetSceneDetected(camera_metadata_t *mdata, S32 *sceneDetected)
{
    DEV_IF_LOGE_RETURN_RET(sceneDetected == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.ai.asd.sceneDetected", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*sceneDetected) = (S32)(*reinterpret_cast<int *>(pData));
    //    DEV_LOGI("sceneDetected =%d", *sceneDetected);
    return ret;
}

S32 NodeMetaData::GetLaserDist(camera_metadata_t *mdata, LaserDistance *laserDist)
{
    DEV_IF_LOGE_RETURN_RET(laserDist == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.ai.misd.laserDist", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    memcpy((void *)laserDist, pData, sizeof(LaserDistance));
    return ret;
}

S32 NodeMetaData::GetSensitivity(camera_metadata_t *mdata, S32 *sensitivity)
{
    DEV_IF_LOGE_RETURN_RET(sensitivity == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, NULL, ANDROID_SENSOR_SENSITIVITY, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*sensitivity) = (S32)(*reinterpret_cast<int *>(pData));
    //    DEV_LOGI("sensitivity =%d", *sensitivity);
    return ret;
}

S32 NodeMetaData::GetLux_index(camera_metadata_t *mdata, FP32 *lux_index)
{
    DEV_IF_LOGE_RETURN_RET(lux_index == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "org.quic.camera2.statsconfigs.AECFrameControl", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*lux_index) = (FP32)(reinterpret_cast<AECFrameControl *>(pData)->luxIndex);
    //    DEV_LOGI("lux_index =%f", *lux_index);
    return ret;
}

S32 NodeMetaData::GetExposureTime(camera_metadata_t *mdata, U64 *exposureTime)
{
    DEV_IF_LOGE_RETURN_RET(exposureTime == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "org.quic.camera2.statsconfigs.AECFrameControl", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*exposureTime) = (U64)(
        reinterpret_cast<AECFrameControl *>(pData)->exposureInfo[ExposureIndexSafe].exposureTime);
    //    DEV_LOGI("expossure_time =%ld", *expossure_time);
    return ret;
}

S32 NodeMetaData::GetLinearGain(camera_metadata_t *mdata, FP32 *linearGain)
{
    DEV_IF_LOGE_RETURN_RET(linearGain == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "org.quic.camera2.statsconfigs.AECFrameControl", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*linearGain) = (FP32)(
        reinterpret_cast<AECFrameControl *>(pData)->exposureInfo[ExposureIndexSafe].linearGain);
    //    DEV_LOGI("linearGain =%ld", *linearGain);
    return ret;
}

S32 NodeMetaData::GetAECsensitivity(camera_metadata_t *mdata, FP32 *sensitivity)
{
    DEV_IF_LOGE_RETURN_RET(sensitivity == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "org.quic.camera2.statsconfigs.AECFrameControl", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*sensitivity) = (FP32)(
        reinterpret_cast<AECFrameControl *>(pData)->exposureInfo[ExposureIndexSafe].sensitivity);
    //    DEV_LOGI("sensitivity =%ld", *sensitivity);
    return ret;
}

S32 NodeMetaData::GetExpValue(camera_metadata_t *mdata, S32 *expValue)
{
    DEV_IF_LOGE_RETURN_RET(expValue == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_ERR;
    void *pData = NULL;
    ret = GetMetaData(mdata, NULL, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*expValue) = *reinterpret_cast<S32 *>(pData);
    return RET_OK;
}

S32 NodeMetaData::GetSuperNightEnabled(camera_metadata_t *mdata, S32 *superNightEnabled)
{
    DEV_IF_LOGE_RETURN_RET(superNightEnabled == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.bokeh.superNightEnabled", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata superNightEnabled err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata superNightEnabled err");
    (*superNightEnabled) = (S32)(*reinterpret_cast<uint8_t *>(pData));
    //    DEV_LOGI("superNightEnabled =%d", *superNightEnabled);
    return ret;
}

S32 NodeMetaData::GetSupernightMode(camera_metadata_t *mdata, S32 *supernightMode)
{
    DEV_IF_LOGE_RETURN_RET(supernightMode == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.mivi.supernight.mode", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*supernightMode) = (S32)(*reinterpret_cast<int *>(pData));
    //    DEV_LOGI("supernightMode =%d", *supernightMode);
    return ret;
}

S32 NodeMetaData::GetbeautyMode(camera_metadata_t *mdata, S32 *beautyMode)
{
    DEV_IF_LOGE_RETURN_RET(beautyMode == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.beauty.beautyMode", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    (*beautyMode) = (S32)(*reinterpret_cast<int *>(pData));
    //    DEV_LOGI("beautyMode =%d", *beautyMode);
    return ret;
}

S32 NodeMetaData::GetThermalLevel(camera_metadata_t *mdata, S32 *thermalLevel)
{
    DEV_IF_LOGE_RETURN_RET(thermalLevel == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.thermal.thermalLevel", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata thermalLevel err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata thermalLevel err");
    (*thermalLevel) = (S32)(*reinterpret_cast<int *>(pData));
    //    DEV_LOGI("thermalLevel =%d", *thermalLevel);
    return ret;
}

S32 NodeMetaData::GetfocusDistCm(camera_metadata_t *mdata, S32 *focusDistCm)
{
    DEV_IF_LOGE_RETURN_RET(focusDistCm == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    InputMetadataBokeh inputMetaBokeh = {0};
    ret = GetInputMetadataBokeh(mdata, &inputMetaBokeh);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    for (int i = 0; i < MAX_LINKED_SESSIONS; i++) {
        if (inputMetaBokeh.cameraMetadata[i].isValid &&
            (inputMetaBokeh.cameraMetadata[i].masterCameraId ==
             inputMetaBokeh.cameraMetadata[i].cameraId)) {
            *focusDistCm = inputMetaBokeh.cameraMetadata[i].focusDistCm;
        }
    }
    //    DEV_LOGI("focusDistCm =%d", *supernightMode);
    return ret;
}

S32 NodeMetaData::GetActiveArraySizeMain(camera_metadata_t *mdata, CHIRECT *activeArraySizeMain)
{
    DEV_IF_LOGE_RETURN_RET(activeArraySizeMain == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_OK;
    InputMetadataBokeh inputMetaBokeh = {0};
    ret = GetInputMetadataBokeh(mdata, &inputMetaBokeh);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    for (int i = 0; i < MAX_LINKED_SESSIONS; i++) {
        if (inputMetaBokeh.cameraMetadata[i].isValid &&
            (inputMetaBokeh.cameraMetadata[i].masterCameraId ==
             inputMetaBokeh.cameraMetadata[i].cameraId)) {
            (*activeArraySizeMain) = inputMetaBokeh.cameraMetadata[i].activeArraySize;
        }
    }
    //    DEV_LOGI("activeArraySizeMain [%d:%d]", activeArraySizeMain->width,
    //    activeArraySizeMain->height);
    return ret;
}

S32 NodeMetaData::MapRectToInput(CHIRECT SrcRect, CHIRECT SrcRefer, S32 DstReferInputWidth,
                                 S32 DstReferInputHeight, DEV_IMAGE_RECT *DstRect)
{
    DEV_IF_LOGW_RETURN_RET(
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
        DstRect->top = round((SrcRect.top - iTBOffset) * fLRRatio);
    } else {
        DstRect->top = 0;
    }
    DstRect->left = round(SrcRect.left * fLRRatio);
    DstRect->width = floor(SrcRect.width * fLRRatio);
    DstRect->height = floor(SrcRect.height * fLRRatio);
    DstRect->width &= 0xFFFFFFFE;
    DstRect->height &= 0xFFFFFFFE;
    return RET_OK;
}

S32 NodeMetaData::GetMaxFaceRect(camera_metadata_t *pMdata, DEV_IMAGE_BUF *inputMain,
                                 AlgoMultiCams::MiFaceRect *faceRect,
                                 AlgoMultiCams::MiPoint2i *faceCentor)
{
#define MAX_FACE_NUM_BOKEH 10
    DEV_IF_LOGE_RETURN_RET(
        (pMdata == NULL || faceRect == NULL || inputMain == NULL || faceRect == NULL), RET_ERR_ARG,
        "arg err");
    S32 ret = RET_OK;

    // reset face rect for no face
    memset(faceRect, 0, sizeof(AlgoMultiCams::MiFaceRect));

    U32 rotateAngle = 0;
    ret = GetRotateAngle(pMdata, &rotateAngle);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");

    CHIRECT cropRegion = {0};
    ret = GetCropRegion(pMdata, &cropRegion);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");

    void *pData = NULL;
    ret = GetMetaData(pMdata, (char *)"xiaomi.snapshot.faceRect", 0, &pData, NULL);
    DEV_IF_LOGW_RETURN_RET((ret != RET_OK || pData == NULL), RET_OK, "No face");

    FaceResult *pFaceResult = reinterpret_cast<FaceResult *>(pData);

    U64 LargeFaceRegion = 0;
    for (int i = 0; i < pFaceResult->faceNumber && i < MAX_FACE_NUM_BOKEH; ++i) {
        UINT32 dataIndex = 0;
        S32 xMin = pFaceResult->chiFaceInfo[i].left;
        S32 yMin = pFaceResult->chiFaceInfo[i].bottom;
        S32 xMax = pFaceResult->chiFaceInfo[i].right;
        S32 yMax = pFaceResult->chiFaceInfo[i].top;

        CHIRECT tmpFaceRect = {U32(xMin), U32(yMin), U32(xMax - xMin + 1), U32(yMax - yMin + 1)};

        DEV_LOGI("face[%d/%d] xMin %d, yMin %d, xMax %d, yMax %d, tmpFaceRect %d %d %d %d", i + 1,
                 pFaceResult->faceNumber, xMin, yMin, xMax, yMax, tmpFaceRect.left, tmpFaceRect.top,
                 tmpFaceRect.width, tmpFaceRect.height);

        DEV_IMAGE_RECT tmpDstFaceRect = {0};
        MapRectToInput(tmpFaceRect, cropRegion, inputMain->width, inputMain->height,
                       &tmpDstFaceRect);

        DEV_LOGI("face[%d/%d] MapRectToInput %d %d %d %d", i + 1, pFaceResult->faceNumber,
                 tmpDstFaceRect.left, tmpDstFaceRect.top, tmpDstFaceRect.width,
                 tmpDstFaceRect.height);

        if (LargeFaceRegion < (U64)tmpDstFaceRect.width * (U64)tmpDstFaceRect.height) {
            faceRect->x = tmpDstFaceRect.left;
            faceRect->y = tmpDstFaceRect.top;
            faceRect->width = tmpDstFaceRect.width;
            faceRect->height = tmpDstFaceRect.height;
            faceRect->angle = rotateAngle;

            LargeFaceRegion = (U64)tmpDstFaceRect.width * (U64)tmpDstFaceRect.height;
        }
    }

    if (faceCentor) {
        faceCentor->x = faceRect->x + faceRect->width / 2;
        faceCentor->y = faceRect->y + faceRect->height / 2;
        if (faceCentor->x <= 0 || faceCentor->y <= 0 || faceCentor->x > inputMain->width ||
            faceCentor->y > inputMain->height) {
            faceCentor->x = inputMain->width / 2;
            faceCentor->y = inputMain->height / 2;
        }
        DEV_LOGI("faceCentor (%d %d)", faceCentor->x, faceCentor->y);
    }

    return ret;
}

S32 NodeMetaData::GetfocusROI(camera_metadata_t *pMdata, DEV_IMAGE_BUF *inputMain,
                              DEV_IMAGE_RECT *focusROI)
{
    DEV_IF_LOGE_RETURN_RET((pMdata == NULL || inputMain == NULL || focusROI == NULL), RET_ERR_ARG,
                           "arg err");
    S32 ret = RET_OK;

    CHIRECT cropRegion = {0};
    ret = GetCropRegion(pMdata, &cropRegion);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");

    void *pData = NULL;
    ret = GetMetaData(pMdata, NULL, ANDROID_CONTROL_AF_REGIONS, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK || pData == NULL), RET_OK, "GetMetadata err");

    S32 *pAfRegion = reinterpret_cast<S32 *>(pData);
    CHIRECT afRect = {UINT32(pAfRegion[0]), UINT32(pAfRegion[1]),
                      UINT32(pAfRegion[2] - pAfRegion[0]), UINT32(pAfRegion[3] - pAfRegion[1])};

    ret = MapRectToInput(afRect, cropRegion, inputMain->width, inputMain->height, focusROI);
    return ret;
}

S32 NodeMetaData::GetfocusTP(camera_metadata_t *pMdata, DEV_IMAGE_BUF *inputMain,
                             DEV_IMAGE_POINT *focusTP)
{
    DEV_IF_LOGE_RETURN_RET((pMdata == NULL || inputMain == NULL || focusTP == NULL), RET_ERR_ARG,
                           "arg err");
    S32 ret = RET_OK;
    DEV_IMAGE_RECT focusROI = {0};
    ret = GetfocusROI(pMdata, inputMain, &focusROI);
    focusTP->x = focusROI.left + focusROI.width / 2;
    focusTP->y = focusROI.top + focusROI.height / 2;
    if (focusTP->x == 0 && focusTP->y == 0) {
        focusTP->x = focusROI.width / 2;
        focusTP->y = focusROI.height / 2;
    }
    //    DEV_LOGI("focusTP[%d %d]", focusTP->x, focusTP->y);
    return ret;
}

S32 NodeMetaData::GetZoomRatio(camera_metadata_t *mdata, FP32 *zoomRatio)
{
    DEV_IF_LOGE_RETURN_RET((zoomRatio == NULL || mdata == NULL), RET_ERR_ARG, "arg err");
    void *pData = NULL;
    S32 ret = GetMetaData(mdata, "xiaomi.snapshot.userZoomRatio", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetaData xiaomi.snapshot.userZoomRatio fail");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get xiaomi.snapshot.userZoomRatio err");
    *zoomRatio = *static_cast<FP32 *>(pData);
    return ret;
}

S32 NodeMetaData::GetFallbackStatus(camera_metadata_t *mdata, S8 *fallbackStatus)
{
    DEV_IF_LOGE_RETURN_RET((fallbackStatus == NULL || mdata == NULL), RET_ERR_ARG, "arg err");
    void *pData = NULL;
    S32 ret = GetMetaData(mdata, "xiaomi.bokeh.fallback", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata fallback fail");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata fallback err");
    *fallbackStatus = *static_cast<S8 *>(pData);
    return ret;
}
S32 NodeMetaData::GetStylizationType(camera_metadata_t *mdata, U8 *stylizationType)
{
    DEV_IF_LOGE_RETURN_RET((stylizationType == NULL || mdata == NULL), RET_ERR_ARG, "arg err");
    void *pData = NULL;
    S32 ret = GetMetaData(mdata, "com.xiaomi.sessionparams.stylizationType", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata fail");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    *stylizationType = *static_cast<U8 *>(pData);
    return ret;
}

S32 NodeMetaData::GetAFState(camera_metadata_t *mdata, U32 *afStatus)
{
    DEV_IF_LOGE_RETURN_RET(afStatus == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_ERR;
    void *pData = NULL;
    ret = GetMetaData(mdata, NULL, ANDROID_CONTROL_AF_STATE, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata ANDROID_CONTROL_AF_STATE err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata ANDROID_CONTROL_AF_STATE err");
    (*afStatus) = *reinterpret_cast<U32 *>(pData);
    return RET_OK;
}

S32 NodeMetaData::GetAFMode(camera_metadata_t *mdata, U32 *afMode)
{
    DEV_IF_LOGE_RETURN_RET(afMode == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_ERR;
    void *pData = NULL;
    ret = GetMetaData(mdata, NULL, ANDROID_CONTROL_AF_MODE, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata ANDROID_CONTROL_AF_MODE err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata ANDROID_CONTROL_AF_MODE err");
    (*afMode) = *reinterpret_cast<U32 *>(pData);
    return RET_OK;
}

S32 NodeMetaData::GetMotionCaptureType(camera_metadata_t *mdata, U32 *motionCaptureType)
{
    DEV_IF_LOGE_RETURN_RET(motionCaptureType == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_ERR;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.ai.misd.motionCaptureType", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata xiaomi.ai.misd.motionCaptureType err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR,
                           "get metadata xiaomi.ai.misd.motionCaptureType err");
    (*motionCaptureType) = *reinterpret_cast<U32 *>(pData);
    return RET_OK;
}

S32 NodeMetaData::GetBokehSensorConfig(camera_metadata_t *mdata,
                                       BokehSensorConfig *pBokehSensorConfig)
{
    DEV_IF_LOGE_RETURN_RET(pBokehSensorConfig == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_ERR;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.bokeh.BokehSensorConfig", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    *pBokehSensorConfig = *reinterpret_cast<BokehSensorConfig *>(pData);
    return RET_OK;
}

S32 NodeMetaData::GetCropRegion(camera_metadata_t *mdata, CHIRECT *pCropRegin)
{
    DEV_IF_LOGE_RETURN_RET(pCropRegin == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_ERR;
    void *pData = NULL;
    ret = GetMetaData(mdata, "xiaomi.snapshot.cropRegion", 0, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    *pCropRegin = *reinterpret_cast<CHIRECT *>(pData);
    DEV_LOGI("xiaomi.snapshot.cropRegion [%d %d %d %d]", pCropRegin->left, pCropRegin->top,
             pCropRegin->width, pCropRegin->height);
    return RET_OK;
}

S32 NodeMetaData::GetAndroidCropRegion(camera_metadata_t *mdata, CHIRECT *pCropRegin)
{
    DEV_IF_LOGE_RETURN_RET(pCropRegin == NULL, RET_ERR_ARG, "arg err");
    S32 ret = RET_ERR;
    void *pData = NULL;
    ret = GetMetaData(mdata, NULL, ANDROID_SCALER_CROP_REGION, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetadata err");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get metadata err");
    *pCropRegin = *reinterpret_cast<CHIRECT *>(pData);
    DEV_LOGI("ANDROID_SCALER_CROP_REGION [%d %d %d %d]", pCropRegin->left, pCropRegin->top,
             pCropRegin->width, pCropRegin->height);
    return RET_OK;
}

S32 NodeMetaData::GetAndroidZoomRatio(camera_metadata_t *mdata, FP32 *zoomRatio)
{
    DEV_IF_LOGE_RETURN_RET((zoomRatio == NULL || mdata == NULL), RET_ERR_ARG, "arg err");
    void *pData = NULL;
    S32 ret = GetMetaData(mdata, NULL, ANDROID_CONTROL_ZOOM_RATIO, &pData, NULL);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMetaData ANDROID_CONTROL_ZOOM_RATIO fail");
    DEV_IF_LOGE_RETURN_RET(pData == NULL, RET_ERR, "get ANDROID_CONTROL_ZOOM_RATIO err");
    *zoomRatio = *static_cast<FP32 *>(pData);
    return ret;
}

} // namespace mialgo2
