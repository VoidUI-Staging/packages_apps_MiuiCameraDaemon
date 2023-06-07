#include "arcsoftsr.h"

#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <VendorMetadataParser.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>

#include <fstream>
#include <iostream>
#include <string>

#include "amcomdef.h"
#include "ammem.h"
#include "arcsoft_mf_superresolution.h"
#include "asvloffscreen.h"
#include "merror.h"

#undef LOG_TAG
#define LOG_TAG "ARCSOFTSR"

#define DUMP_FILE_PATH "/data/vendor/camera/"

using namespace std;

using namespace mialgo2;

#define abs(x, y)          ((x) > (y) ? (x) - (y) : (y) - (x))
#define MIN(a, b)          (((a) > (b)) ? (b) : (a))
#define FOV_DIFF_TOLERANCE (3)
#define DIFFVALUE          0.001f
#define RATIO_4_3          1.333333
inline int32_t div_round(int32_t const numerator, int32_t const denominator)
{
    return ((numerator < 0) ^ (denominator < 0)) ? ((numerator - denominator / 2) / denominator)
                                                 : ((numerator + denominator / 2) / denominator);
}
ArcSoftSR::ArcSoftSR(uint32_t width, uint32_t height)
    : m_hArcSoftSRAlgo(NULL), m_imageWidth(width), m_imageHeight(height)
{
    MLOGI(Mia2LogGroupPlugin, "ArcSoftSR: width = %d,height = %d", m_imageWidth, m_imageHeight);
}

ArcSoftSR::~ArcSoftSR() {}

void ArcSoftSR::setOffScreen(ASVLOFFSCREEN *pImg, struct mialgo2::MiImageBuffer *miBuf)
{
    if (miBuf->format == mialgo2::CAM_FORMAT_YUV_420_NV12)
        pImg->u32PixelArrayFormat = ASVL_PAF_NV12;
    else
        pImg->u32PixelArrayFormat = ASVL_PAF_NV21;

    // pImg->u32PixelArrayFormat = ASVL_PAF_NV21;

    pImg->i32Width = miBuf->width;
    pImg->i32Height = miBuf->height;
    // stride
    pImg->pi32Pitch[0] = miBuf->stride;
    pImg->pi32Pitch[1] = miBuf->stride;
    pImg->ppu8Plane[0] = miBuf->plane[0];
    pImg->ppu8Plane[1] = miBuf->plane[1];
}

MInt32 ArcSoftSR::getCameraType(camera_metadata_t *metaData)
{
    MInt32 cameratype = ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0;

    void *pData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "com.xiaomi.multicamerainfo.MultiCameraIdRole",
                                       &pData);
    CameraRoleType currentCameraRole = CameraRoleTypeWide;
    if (pData) {
        currentCameraRole = *static_cast<CameraRoleType *>(pData);
        MLOGI(Mia2LogGroupPlugin, "ArcSoftSR getCameraType currentCameraRole: %u",
              currentCameraRole);
    } else {
        MLOGI(Mia2LogGroupPlugin, "ArcSoftSR getCameraType error: %p", pData);
    }
    int FakeSatEnable = false;
    pData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.insensorzoom.enabled", &pData);
    if (NULL != pData) {
        FakeSatEnable = *static_cast<int *>(pData);
    }
    switch (currentCameraRole) {
    case CameraRoleTypeWide:
#ifdef MATISSE_CAMERA
        if (FakeSatEnable) {
            cameratype = ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0_RM;
            strcpy(cameraTypeName, "l11_wide_isz");
        } else {
            cameratype = ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0;
            strcpy(cameraTypeName, "l11_wide");
        }
#elif defined RUBENS_CAMERA || defined(YUECHU_CAMERA)
        if (FakeSatEnable) {
            cameratype = ARC_MFSR_CAMERA_TYPE_REAR_WIDE_1_RM;
            strcpy(cameraTypeName, "l11a_wide_isz");
        } else {
            cameratype = ARC_MFSR_CAMERA_TYPE_REAR_WIDE_1;
            strcpy(cameraTypeName, "l11a_wide");
        }
#elif defined PLATO_CAMERA
        if (FakeSatEnable) {
            cameratype = ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0_RM;
            strcpy(cameraTypeName, "l12a_wide_isz");
        } else {
            cameratype = ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0;
            strcpy(cameraTypeName, "l12a_wide");
        }
#elif defined DAUMIER_CAMERA
        if (FakeSatEnable) {
            cameratype = ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0_RM;
            strcpy(cameraTypeName, "l2m_wide_isz");
        } else {
            cameratype = ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0;
            strcpy(cameraTypeName, "l2m_wide");
        }
#else
        cameratype = ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0;
        strcpy(cameraTypeName, "wide");
#endif
        break;
    default:
        cameratype = ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0;
        strcpy(cameraTypeName, "unknown");
        break;
    }
    MLOGD(Mia2LogGroupPlugin, "ArcSoftSR: getCameraType(%d) = 0x%X cameraTypeName = %s",
          currentCameraRole, cameratype, cameraTypeName);
    return cameratype;
}
int ArcSoftSR::getCropRegion(camera_metadata_t *metaData, struct MiImageBuffer *input,
                             MRECT *arsoftCrop)
{
    MICropInfo cropInfo = {};
    MIRECT cropRegion = {};
    MIImageShiftData shift;
    void *pData = NULL;

    pData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.superResolution.cropRegionMtk", &pData);
    if (NULL != pData) {
        cropRegion = *static_cast<MIRECT *>(pData);
        MLOGD(Mia2LogGroupPlugin, "ArcSoftSR get crop region from SAT cropregion: [%d %d %d %d]",
              cropRegion.left, cropRegion.top, cropRegion.width, cropRegion.height);
        m_userZoomRatio = (float)(cropRegion.left * 2 + cropRegion.width) / cropRegion.width;
    } else {
        pData = NULL;
        VendorMetadataParser::getTagValue(metaData, ANDROID_SCALER_CROP_REGION, &pData);
        if (NULL != pData) {
            cropRegion = *static_cast<MIRECT *>(pData);
            MLOGD(Mia2LogGroupPlugin,
                  "ArcSoftSR get crop region from MTK cropregion: [%d %d %d %d]", cropRegion.left,
                  cropRegion.top, cropRegion.width, cropRegion.height);
            m_userZoomRatio = (float)(cropRegion.left * 2 + cropRegion.width) / cropRegion.width;
        } else {
            MLOGE(Mia2LogGroupPlugin,
                  "ArcSoftSR get android crop region failed,use FULL-FOV instead !!");
            cropRegion = {0, 0, static_cast<uint32_t>(input->width),
                          static_cast<uint32_t>(input->height)};
            m_userZoomRatio = 1.0f;
        }
        shift = {0, 0};
    }

    MiDimension sensor_size = {0};
    MIRECT SR_crop_region = {0};

    pData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.sensorSize.size", &pData);
    if (NULL != pData) {
        sensor_size = *static_cast<MiDimension *>(pData);
        MLOGI(Mia2LogGroupPlugin, "ArcSoftSR get sensor_size: [%d %d]", sensor_size.width,
              sensor_size.height);
    }
#if defined(MATISSE_CAMERA) || defined(RUBENS_CAMERA) || defined(PLATO_CAMERA) || \
    defined(DAUMIER_CAMERA) || defined(YUECHU_CAMERA)
    int refWidth = 0, refHeight = 0;
    refWidth = sensor_size.width;
    refHeight = sensor_size.height;
    // clang-format off
    /************************************************************************************************
    +-----------------------------+
    |                             |
    |   +-------------------+     |    +-----------------------------+
    |   |                   |     |    |                             |
    |   |                   |     |    |   +-------------------+     |    +------------------+
    |   |                   |     |    |   |                   |     |    |  +-----------+   |
    |   |                   |     +--->+   |                   |     +--->+  |           |   |
    |   |                   |     |    |   |                   |     |    |  +-----------+   |
    |   |                   |     |    |   +-------------------+     |    +------------------+
    |   +-------------------+     |    |                             |
    |                             |    +-----------------------------+
    +-----------------------------+     16:9 for example:                 preZoom applied:
    sensorDomain(4:3)                   tmpCrop(after align aspectRatio)  (input buf domain)
    refWidth=6016,refHeight=4512        refWidth=6016,refHeight=3384      width=1600,height=900
    cropRegion=[1504,1128,3008,2256]    tmpCrop=[1504,846,3008,1692]      crop=[400,225,800,450]
    *************************************************************************************************/
    // clang-format on

    // align cropRegion's aspect ratio with input buffer
    float tmpLeft = (float)cropRegion.left;
    float tmpTop = (float)cropRegion.top;
    float tmpWidth = (float)cropRegion.width;
    float tmpHeight = (float)cropRegion.height;

    float xRatio = (float)cropRegion.width / input[0].width;
    float yRatio = (float)cropRegion.height / input[0].height;
    constexpr float tolerance = 0.01f;
    float preZoomRatio = 1;

    if (yRatio > xRatio + tolerance) {
        tmpHeight = (float)input[0].height * xRatio;
        float tmpRefHeight = (float)input[0].height * refWidth / input[0].width;
        float delta = (refHeight - tmpRefHeight) / 2.0;
        tmpTop = cropRegion.top + (cropRegion.height - tmpHeight) / 2.0 - delta;
        preZoomRatio = (float)input[0].width / refWidth;
    } else if (xRatio > yRatio + tolerance) {
        tmpWidth = (float)input[0].width * yRatio;
        float tmpRefWidth = (float)input[0].width * refHeight / input[0].height;
        float delta = (refWidth - tmpRefWidth) / 2.0;
        tmpLeft = cropRegion.left + (cropRegion.width - tmpWidth) / 2.0 - delta;
        preZoomRatio = (float)input[0].height / refHeight;
    }

    // adjust crop according to preZoom into buffer domain
    unsigned int rectLeft, rectTop, rectWidth, rectHeight;

    rectWidth = tmpWidth * preZoomRatio;
    rectLeft = tmpLeft * preZoomRatio;
    rectHeight = tmpHeight * preZoomRatio;
    rectTop = tmpTop * preZoomRatio;

    // now we validate the rect, and ajust it if out-of-bondary found.
    if (rectLeft + rectWidth > input[0].width) {
        if (rectWidth <= input[0].width) {
            rectLeft = input[0].width - rectWidth;
        } else {
            rectLeft = 0;
            rectWidth = input[0].width;
        }
        MLOGW(Mia2LogGroupPlugin, "crop left or width is wrong, ajusted it manually!!");
    }
    if (rectTop + rectHeight > input[0].height) {
        if (rectHeight <= input[0].height) {
            rectTop = input[0].height - rectHeight;
        } else {
            rectTop = 0;
            rectHeight = input[0].height;
        }
        MLOGW(Mia2LogGroupPlugin, "crop top or height is wrong, ajusted it manually!!");
    }

    /// very important
    MLOGI(Mia2LogGroupPlugin, "sr inital caculated crop = (%d %d %d %d)", rectLeft, rectTop,
          rectWidth, rectHeight);
    // adjust SR cropregion
    rectWidth &= ~(0x1);
    rectHeight &= ~(0x1);
    if ((2 * rectLeft + rectWidth) != input[0].width) {
        rectLeft = (input[0].width - rectWidth) / 2;
    }
    if ((2 * rectTop + rectHeight) != input[0].height) {
        rectTop = (input[0].height - rectHeight) / 2;
    }
    MLOGI(Mia2LogGroupPlugin, "sr aligned caculated crop = (%d %d %d %d)", rectLeft, rectTop,
          rectWidth, rectHeight);

    // adjust no-standard 4:3 sensor active size ratio.
    if (abs((float)refWidth / refHeight, RATIO_4_3) > DIFFVALUE) {
        MLOGI(Mia2LogGroupPlugin, "The sensor size is no-standard 4:3 ratio");
        // 1. calculate hal view cropregion
        unsigned int halRectLeft, halRectTop, halRectWidth, halRectHeight;
        // 1.1 pillarbox/letterbox
        if (cropRegion.width * input[0].height > cropRegion.height * input[0].width) {
            halRectWidth = div_round(cropRegion.height * input[0].width, input[0].height);
            halRectHeight = cropRegion.height;
            halRectLeft = cropRegion.left + ((cropRegion.width - halRectWidth) >> 1);
            if (halRectLeft < 0 && abs(halRectLeft, 0) < FOV_DIFF_TOLERANCE)
                halRectLeft = 0;
            halRectTop = cropRegion.top;
        } else {
            halRectWidth = cropRegion.width;
            halRectHeight = div_round(cropRegion.width * input[0].height, input[0].width);
            halRectLeft = cropRegion.left;
            halRectTop = cropRegion.top + ((cropRegion.height - halRectHeight) >> 1);
            if (halRectTop < 0 && abs(halRectTop, 0) < FOV_DIFF_TOLERANCE) {
                halRectTop = 0;
            }
        }
        // 1.2 make sure hw limitation
        halRectWidth &= ~(0x1);
        halRectHeight &= ~(0x1);
        MLOGI(Mia2LogGroupPlugin, "hal inital caculated crop = (%d %d %d %d)", halRectLeft,
              halRectTop, halRectWidth, halRectHeight);

        // 1.3 adjust hal crop left/top
        if ((2 * halRectLeft + halRectWidth) != refWidth) {
            halRectLeft = (refWidth - halRectWidth) / 2;
        }
        if ((2 * halRectTop + halRectHeight) != refHeight) {
            halRectTop = (refHeight - halRectHeight) / 2;
        }
        MLOGI(Mia2LogGroupPlugin, "hal final caculated crop = (%d %d %d %d)", halRectLeft,
              halRectTop, halRectWidth, halRectHeight);

        // 2. adjust sr cropregion under no-standard 4:3 sensor ratio.
        float zoomSrPicRatio = (float)rectWidth / rectHeight;
        float zoomHalPicRatio = (float)halRectWidth / halRectHeight;
        if (abs(zoomSrPicRatio, zoomHalPicRatio) > DIFFVALUE) {
            // adjust crop according to zoomHalPicRatio
            MLOGI(Mia2LogGroupPlugin,
                  "There is a big difference between zoomSrPicRatio and zoomHalPicRatio");
            float wTemp = abs((float)rectWidth / rectHeight, zoomHalPicRatio);
            float wTemp1 = abs((float)rectWidth / (rectHeight + 2), zoomHalPicRatio);
            float hTemp = abs((float)(rectWidth + 2) / rectHeight, zoomHalPicRatio);
            if (wTemp1 == MIN(MIN(wTemp, wTemp1), hTemp)) {
                rectHeight += 2;
                rectTop = (input[0].height - rectHeight) / 2;
            } else if (hTemp == MIN(MIN(wTemp, wTemp1), hTemp)) {
                rectWidth += 2;
                rectLeft = (input[0].width - rectWidth) / 2;
            }
        }
    }
    MLOGI(Mia2LogGroupPlugin, "final caculated crop = (%d %d %d %d)", rectLeft, rectTop, rectWidth,
          rectHeight);

    SR_crop_region.left = rectLeft;
    SR_crop_region.top = rectTop;
    SR_crop_region.width = rectWidth;
    SR_crop_region.height = rectHeight;
#else
    int32_t t_width = 0;
    int32_t t_height = 0;

    t_width = (sensor_size.width - input[0].width) / 2;
    t_height = (sensor_size.height - input[0].height) / 2;

    SR_crop_region.left = cropRegion.left - t_width;
    SR_crop_region.top = cropRegion.top - t_height;
    SR_crop_region.width = cropRegion.width;
    SR_crop_region.height = cropRegion.height;

#endif

    MLOGD(Mia2LogGroupPlugin, "ArcSoftSR get SR_crop_region: [%d %d %d %d]", SR_crop_region.left,
          SR_crop_region.top, SR_crop_region.width, SR_crop_region.height);
    arsoftCrop->left = SR_crop_region.left;
    arsoftCrop->top = SR_crop_region.top;
    arsoftCrop->right = SR_crop_region.left + SR_crop_region.width;
    arsoftCrop->bottom = SR_crop_region.top + SR_crop_region.height;

    MLOGD(Mia2LogGroupPlugin,
          "cam %d, "
          "rect[L,T,W,H][%d,%d,%d,%d]->[%d,%d,%d,%d],arsoftCrop[L,T,R,B][%d,%d,%d,%d],shift=%dx%d,"
          "in=%dx%d,sensorsize=%dx%d,userZoom=%f",
          m_cameraId, cropRegion.left, cropRegion.top, cropRegion.width, cropRegion.height,
          SR_crop_region.left, SR_crop_region.top, SR_crop_region.width, SR_crop_region.height,
          arsoftCrop->left, arsoftCrop->top, arsoftCrop->right, arsoftCrop->bottom,
          shift.horizonalShift, shift.verticalShift, input[0].width, input[0].height,
          sensor_size.width, sensor_size.height, m_userZoomRatio);

    return MIA_RETURN_OK;
}
void ArcSoftSR::getMetadata(camera_metadata_t **inputmetadata, camera_metadata_t **inputphymeta,
                            struct MiImageBuffer *input, int input_num,
                            ARC_MFSR_META_DATA *Metadata, MRECT *Cropregion, MInt32 *RefIndex,
                            LPARC_MFSR_FACEINFO Faceinfo)
{
    int32_t iso[input_num];
    int64_t exp_time[input_num];
    float luxindex[input_num];
    float ispgain[input_num];
    float sensorgain[input_num];
    // get cropregion
    getCropRegion(inputmetadata[0], input, Cropregion);
    void *pData = NULL;
    for (int i = 0; i < input_num; i++) {
        // get iso
        VendorMetadataParser::getTagValue(inputmetadata[i], ANDROID_SENSOR_SENSITIVITY, &pData);
        if (NULL != pData)
            iso[i] = *static_cast<int32_t *>(pData);
        Metadata[i].nISO = iso[i];
        MLOGD(Mia2LogGroupPlugin, "ArcSoftSR get iso[%d] %d", i, iso[i]);
        // get exp_time
        pData = NULL;
        VendorMetadataParser::getTagValue(inputmetadata[i], ANDROID_SENSOR_EXPOSURE_TIME, &pData);
        if (NULL != pData)
            exp_time[i] = *static_cast<int64_t *>(pData);
        Metadata[i].nExpoTime = exp_time[i];
        MLOGD(Mia2LogGroupPlugin, "ArcSoftSR get exp_time[%d] %d", i, exp_time[i]);
        // get LuxIndex ispgain
        pData = NULL;
        VendorMetadataParser::getVTagValue(inputmetadata[i], "com.xiaomi.statsconfigs.AecInfo",
                                           &pData);
        if (NULL != pData) {
            luxindex[i] = static_cast<InputMetadataAecInfo *>(pData)->luxIndex;
            ispgain[i] = static_cast<InputMetadataAecInfo *>(pData)->linearGain;
            Metadata[i].fLuxIndex = luxindex[i];
            Metadata[i].fDigitalGain = ispgain[i];
            MLOGD(Mia2LogGroupPlugin, "ArcSoftSR get luxindex[%d] %f ispgain[%d] %f", i,
                  luxindex[i], i, ispgain[i]);
        }
        // get sensorgain
        pData = NULL;
        VendorMetadataParser::getVTagValue(inputmetadata[i], "com.xiaomi.sensorbps.gain", &pData);
        if (NULL != pData)
            sensorgain[i] = *static_cast<float *>(pData);
        Metadata[i].fSensorGain = sensorgain[i];
        MLOGD(Mia2LogGroupPlugin, "ArcSoftSR get sensorgain[%d] %f", i, sensorgain[i]);
    }
    // get RefIndex
    pData = NULL;
    VendorMetadataParser::getVTagValue(inputmetadata[input_num - 1],
                                       "xiaomi.superResolution.refindex", &pData);
    if (NULL != pData) {
        *RefIndex = *static_cast<MInt32 *>(pData);
        MLOGI(Mia2LogGroupPlugin, "ArcSoftSR get RefIndex %d", *RefIndex);
    } else {
        MLOGE(Mia2LogGroupPlugin, "ArcSoftSR get RefIndex filed!");
    }

    pData = NULL;
    int hdrSrEnable = false;
    VendorMetadataParser::getVTagValue(inputmetadata[input_num - 1], "xiaomi.hdr.sr.enabled",
                                       &pData);
    if (NULL != pData) {
        hdrSrEnable = *static_cast<int *>(pData);
        if (hdrSrEnable) {
            *RefIndex = input_num - 1;
            MLOGI(Mia2LogGroupPlugin, "HDR+SR enable set RefIndex = last frame");
        }
    }

    // get faceinfo
    if ((*RefIndex >= 0) && (*RefIndex < input_num)) {
        getFaceInfo(inputmetadata[*RefIndex], Faceinfo);
        MLOGI(Mia2LogGroupPlugin, "ArcSoftSR get Faceinfo by RefIndex %d", *RefIndex);
    } else {
        getFaceInfo(inputmetadata[0], Faceinfo);
        MLOGI(Mia2LogGroupPlugin, "ArcSoftSR get Faceinfo default");
    }
}
void ArcSoftSR::getFaceInfo(camera_metadata_t *metaData, LPARC_MFSR_FACEINFO faceInfo)
{
    faceInfo->nFace = 0;
    void *data = NULL;
    size_t tcount = 0;
    VendorMetadataParser::getTagValueCount(metaData, ANDROID_STATISTICS_FACE_RECTANGLES, &data,
                                           &tcount);
    if (NULL == data) {
        faceInfo->nFace = 0;
        return;
    }
    uint32_t numElemRect = sizeof(MIRECT) / sizeof(uint32_t);
    faceInfo->nFace = (int32_t)(tcount / numElemRect);
    MLOGI(Mia2LogGroupPlugin, "ArcSoftSR GetFaceInfo i32FaceNum:%d", faceInfo->nFace);

    faceInfo->rcFace = new MRECT[faceInfo->nFace];
    faceInfo->lFaceOrient = new MInt32[faceInfo->nFace];

    memset(faceInfo->rcFace, 0, sizeof(MRECT) * faceInfo->nFace);
    memset(faceInfo->lFaceOrient, 0, sizeof(MInt32) * faceInfo->nFace);
    // face rect get from ANDROID_STATISTICS_FACE_RECTANGLES is [left, top, right, bottom]
    MIRECT *rect = new MIRECT[faceInfo->nFace];
    memcpy(rect, data, sizeof(MIRECT) * faceInfo->nFace);

    data = NULL;
    int32_t faceOrientation = 0;
    VendorMetadataParser::getTagValue(metaData, ANDROID_JPEG_ORIENTATION, &data);
    if (NULL != data) {
        faceOrientation = *static_cast<int32_t *>(data);
    }
    for (MInt32 index = 0; index < faceInfo->nFace; index++) {
        faceInfo->rcFace[index].left = rect[index].left;
        faceInfo->rcFace[index].top = rect[index].top;
        faceInfo->rcFace[index].right = rect[index].width;
        faceInfo->rcFace[index].bottom = rect[index].height;
        MLOGI(Mia2LogGroupPlugin, "ArcSoftSR facerect [left,top,right,bottom][%d,%d,%d,%d]",
              faceInfo->rcFace[index].left, faceInfo->rcFace[index].top,
              faceInfo->rcFace[index].right, faceInfo->rcFace[index].bottom);
        faceInfo->lFaceOrient[index] = faceOrientation;
    }

    delete[] rect;
    rect = NULL;
}

int ArcSoftSR::ProcessBuffer(struct MiImageBuffer *input, int input_num,
                             struct MiImageBuffer *output, camera_metadata_t **metaData,
                             camera_metadata_t **physMeta)
{
    int32_t iIsDumpData = property_get_int32("persist.vendor.camera.arcsoftsr.dump", 0);
    int result = PROCSUCCESS;
    MHandle hEnhancer = NULL;
    ASVLOFFSCREEN SrcImgs[input_num];
    ASVLOFFSCREEN DstImg;
    MRECT CropRegion = {0};
    MInt32 RefIndex = -1; // The output reference image index if set lRefNum = -1
    MInt32 CameraType = ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0;
    ARC_MFSR_META_DATA Metadata[input_num];
    ARC_MFSR_PARAM Param = {0};
    ARC_MFSR_FACEINFO faceInfo = {0};
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.arcsoftsr.loglevel", prop, "2");
    int32_t Loglevel = (int32_t)atoi(prop);
    ARC_MFSR_SetLogLevel(Loglevel);
    // get library version
    MLOGI(Mia2LogGroupPlugin, "ArcSoftSR: get algorithm library version: %s",
          ARC_MFSR_GetVersion()->Version);

    // prepare image
    memset(SrcImgs, 0, sizeof(SrcImgs));
    memset(&DstImg, 0, sizeof(DstImg));
    for (int i = 0; i < input_num; i++) {
        setOffScreen(&SrcImgs[i], (input + i));
    }
    setOffScreen(&DstImg, output);

    // get cameratype
    for (int i = 0; i < input_num; i++) {
        CameraType = getCameraType(metaData[0]);
        Metadata[i].nCameraType = CameraType;
    }
    // get metadata
    getMetadata(metaData, physMeta, input, input_num, Metadata, &CropRegion, &RefIndex, &faceInfo);

    // init
    result = ARC_MFSR_Init(&hEnhancer, &SrcImgs[0], &DstImg);
    MLOGD(Mia2LogGroupPlugin, "ArcSoftSR: ARC_MFSR_Init result=0x%X, hEnhancer=0x%X", result,
          hEnhancer);
    if (PROCSUCCESS != result)
        goto exit;
    // setcropregion
    result = ARC_MFSR_SetCropRegion(hEnhancer, &CropRegion);
    MLOGD(Mia2LogGroupPlugin, "ArcSoftSR: ARC_MFSR_SetCropRegion result=0x%X", result);
    // preprocess for each input frame
    for (int i = 0; i < input_num; i++) {
        result = ARC_MFSR_PreProcess(hEnhancer, &SrcImgs[i], i, &Metadata[i]);
        MLOGD(Mia2LogGroupPlugin, "ArcSoftSR: ARC_MFSR_PreProcess result=0x%X", result);
        if (PROCSUCCESS != result)
            goto exit;
    }
    // setrefindex
    result = ARC_MFSR_SetRefIndex(hEnhancer, &RefIndex);
    MLOGD(Mia2LogGroupPlugin, "ArcSoftSR: ARC_MFSR_SetRefIndex result=0x%X, refindex=%d", result,
          RefIndex);
    // setfaceinfo
    result = ARC_MFSR_SetFaceInfo(hEnhancer, &faceInfo);
    MLOGD(Mia2LogGroupPlugin, "ArcSoftSR: ARC_MFSR_SetFaceInfo result=0x%X", result);
    if (iIsDumpData) {
        for (int i = 0; i < input_num; i++) {
            char inbuf[128];
            snprintf(inbuf, sizeof(inbuf),
                     "sr_input_%d_%s_%dx%d_crop_(%d-%d-%d-%d)_iso_%d_exp_%d_anchor_%d", i,
                     cameraTypeName, input[i].width, input[i].height, CropRegion.left,
                     CropRegion.top, CropRegion.right, CropRegion.bottom, Metadata[i].nISO,
                     Metadata[i].nExpoTime, RefIndex);
            MLOGD(Mia2LogGroupPlugin, "arsoftsr dumpToFile %p", &input[i]);
            PluginUtils::dumpToFile(inbuf, &input[i]);
        }
    }
    // get param
    result = ARC_MFSR_GetDefaultParam(hEnhancer, &Param);
    MLOGD(Mia2LogGroupPlugin, "ArcSoftSR: ARC_MFSR_GetDefaultParam result=0x%X", result);
    // process
    result = ARC_MFSR_Process(hEnhancer, &Param, &DstImg);
    MLOGD(Mia2LogGroupPlugin, "ArcSoftSR: ARC_MFSR_Process result=0x%X", result);
    if (PROCSUCCESS != result)
        goto exit;
    if (hEnhancer != NULL) {
        ARC_MFSR_UnInit(&hEnhancer);
        hEnhancer = NULL;
    }
    if (iIsDumpData) {
        char outbuf[128];
        snprintf(outbuf, sizeof(outbuf), "sr_output_%s_%dx%d_crop(%d-%d-%d-%d)", cameraTypeName,
                 output->width, output->height, CropRegion.left, CropRegion.top, CropRegion.right,
                 CropRegion.bottom);
        PluginUtils::dumpToFile(outbuf, output);
    }
exit:
    if (hEnhancer != NULL) {
        ARC_MFSR_UnInit(&hEnhancer);
        hEnhancer = NULL;
    }

    return result;
}
