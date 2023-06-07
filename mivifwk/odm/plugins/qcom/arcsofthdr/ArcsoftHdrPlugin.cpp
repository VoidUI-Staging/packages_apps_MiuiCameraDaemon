#include "ArcsoftHdrPlugin.h"

#define FRAME_MAX_INPUT_NUMBER 8
static const uint8_t MISDK_HDR_ALGOUP = 2;

ArcsoftHdrPlugin::~ArcsoftHdrPlugin()
{
    MLOGI(Mia2LogGroupPlugin, "%s:%d", __func__, __LINE__);
}

int ArcsoftHdrPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mFrameworkCameraId = createInfo->frameworkCameraId;
    m_format = createInfo->inputFormat;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.algoengine.arcsofthdr.drawdump", prop, "0");
    mDrawDump = atoi(prop);
    memset(&m_HdrInfo, 0, sizeof(m_HdrInfo));
    if (CAM_COMBMODE_FRONT == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_BOKEH == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_AUX == mFrameworkCameraId) {
        mFrontCamera = true;
    } else {
        mFrontCamera = false;
    }

    mHdr = NULL;
    if (!mHdr) {
        mHdr = new ArcsoftHdr();
        mHdr->init();
    }
    return 0;
}

ProcessRetStatus ArcsoftHdrPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    // get inputbuffer from different port in srhdr
    std::vector<ImageParams> inputBuffersTmp;
    for (auto &it : processRequestInfo->inputBuffersMap) {
        for (auto &vit : it.second) {
            inputBuffersTmp.push_back(vit);
        }
    }
    auto &inputBuffers = inputBuffersTmp;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;

    int inputNum = inputBuffers.size();
    if (inputNum > FRAME_MAX_INPUT_NUMBER) {
        inputNum = FRAME_MAX_INPUT_NUMBER;
    }

    camera_metadata_t *metaData = inputBuffers[0].pMetadata;
    memset(mEV, 0, sizeof(mEV));
    setEv(inputNum, inputBuffers);

    struct MiImageBuffer inputFrame[FRAME_MAX_INPUT_NUMBER], outputFrame;
    for (int i = 0; i < inputNum; i++) {
        convertImageParams(inputBuffers[i], inputFrame[i]);
    }
    convertImageParams(outputBuffers[0], outputFrame);

    MLOGI(Mia2LogGroupPlugin, "%s:%d processBuffer framenum %d begin", __func__, __LINE__,
          processRequestInfo->frameNum);

    int ret = processBuffer(inputFrame, inputNum, &outputFrame, metaData);
    if (ret != PROCSUCCESS) {
        resultInfo = PROCFAILED;
    }
    MLOGI(Mia2LogGroupPlugin, "%s:%d processBuffer framenum %d ret: %d", __func__, __LINE__,
          processRequestInfo->frameNum, ret);

    return resultInfo;
}

ProcessRetStatus ArcsoftHdrPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int ArcsoftHdrPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

void ArcsoftHdrPlugin::destroy()
{
    if (mHdr) {
        delete mHdr;
        mHdr = NULL;
    }
}

bool ArcsoftHdrPlugin::isSdkAlgoUp(camera_metadata_t *metadata)
{
    void *hdrMode = NULL;
    bool hdrEnabled = false;
    VendorMetadataParser::getVTagValue(metadata, "com.xiaomi.algo.hdrMode", &hdrMode);
    if (hdrMode != NULL) {
        uint8_t result = *static_cast<uint8_t *>(hdrMode);
        hdrEnabled = (result == MISDK_HDR_ALGOUP) ? true : false;
    }
    return hdrEnabled;
}

bool ArcsoftHdrPlugin::isEnabled(MiaParams settings)
{
    bool hdrEnable = false;
    void *pinputNum = NULL;
    if (isSdkAlgoUp(settings.metadata)) {
        hdrEnable = true;
    } else {
        VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.multiframe.inputNum",
                                           &pinputNum);
        if (NULL != pinputNum && *static_cast<uint8_t *>(pinputNum) > 1) {
            void *data = NULL;
            const char *hdrEnablePtr = "xiaomi.hdr.enabled";
            VendorMetadataParser::getVTagValue(settings.metadata, hdrEnablePtr, &data);
            if (NULL != data) {
                hdrEnable = *static_cast<uint8_t *>(data);
            }
            if (hdrEnable == false) {
                void *data = NULL;
                const char *hdrEnablePtr = "xiaomi.hdr.sr.enabled";
                VendorMetadataParser::getVTagValue(settings.metadata, hdrEnablePtr, &data);
                if (NULL != data) {
                    hdrEnable = *static_cast<uint8_t *>(data);
                }
            }
        }
    }
    return hdrEnable;
}

int ArcsoftHdrPlugin::processBuffer(struct MiImageBuffer *input, int inputNum,
                                    struct MiImageBuffer *output, camera_metadata_t *metaData)
{
    if (metaData == NULL) {
        MLOGE(Mia2LogGroupPlugin, "error metaData is null");
        return PROCFAILED;
    }
    MLOGD(Mia2LogGroupPlugin, "processBuffer inputNum: %d, mFrameworkCameraId: %d", inputNum,
          mFrameworkCameraId);

    int camMode = 0;
    getHDRInfo(metaData, &camMode);

    if (mHdr && mEV[0] == 0) {
        mHdr->process(input, output, inputNum, mEV, m_HdrInfo, camMode);
    } else {
        MLOGD(Mia2LogGroupPlugin, "error mHdr is null");
        PluginUtils::miCopyBuffer(output, input);
    }

    if (mDrawDump) {
        for (int index = 0; index < m_HdrInfo.m_FaceInfo.nFace; index++)
            DrawRect(output, m_HdrInfo.m_FaceInfo.rcFace[index], 5, 0);
    }

    return PROCSUCCESS;
}

void ArcsoftHdrPlugin::setEv(int inputnum, std::vector<ImageParams> &inputBufferHandle)
{
    if (inputnum > MAX_EV_NUM) {
        MLOGE(Mia2LogGroupPlugin, "hdr inputnum out of mEV array length");
        return;
    }
    for (int numIndex = 0; numIndex < inputnum; numIndex++) {
        void *data = NULL;
        VendorMetadataParser::getTagValue(inputBufferHandle[numIndex].pMetadata,
                                          ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &data);
        if (NULL != data) {
            int32_t perFrameEv = *static_cast<int32_t *>(data);
            mEV[numIndex] = perFrameEv;
        } else {
            mEV[numIndex] = 404;
            MLOGE(Mia2LogGroupPlugin, "Failed to get inputnumber  #%d's ev!", numIndex);
        }
    }
}

void ArcsoftHdrPlugin::getHDRInfo(camera_metadata_t *metaData, int *camMode)
{
    ARC_LLHDR_AEINFO *aeInfo = &(m_HdrInfo.m_AeInfo);
    void *data = NULL;
    const char *bpsGain = "com.qti.sensorbps.gain";
    VendorMetadataParser::getVTagValue(metaData, bpsGain, &data);
    if (NULL != data) {
        aeInfo->fSensorGain = *static_cast<float *>(data);
    } else {
        aeInfo->fSensorGain = 1.0f;
    }

    MIRECT pCropRegion;
    memset(&pCropRegion, 0, sizeof(pCropRegion));
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.snapshot.cropRegion", &data);
    if (data != NULL) {
        pCropRegion = *reinterpret_cast<MIRECT *>(data);
    }

    data = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_LENS_FACING, &data);
    // if (NULL != data) {
    //     uint8_t facing = *((uint8_t *)data);
    //     if (facing == 0)
    //         *camMode = ARC_HDR_CAMERA_FRONT;
    //     else
    //         *camMode = ARC_HDR_CAMERA_WIDE;
    // } else if (mFrontCamera) {
    //     *camMode = ARC_HDR_CAMERA_FRONT;
    // } else {
    //     *camMode = ARC_HDR_CAMERA_WIDE;
    // }
    MLOGI(Mia2LogGroupPlugin, "MiaNodeHdr get camera_mode %d", *camMode);

    data = NULL;
    memset(&m_HdrInfo.m_FaceInfo, 0, sizeof(m_HdrInfo.m_FaceInfo));
    do {
        VendorMetadataParser::getVTagValue(metaData, "org.quic.camera2.oemfdresults.OEMFDResults",
                                           &data);
        if (NULL == data) {
            m_HdrInfo.m_FaceInfo.nFace = 0;
            MLOGI(Mia2LogGroupPlugin,
                  "[ARCSOFT]: get meta OEMFDResults fail! pFaceInfo->i32FaceNum = 0");
            break;
        }
        FaceROIInformation *pFaceROIInfo = reinterpret_cast<FaceROIInformation *>(data);
        m_HdrInfo.m_FaceInfo.nFace = pFaceROIInfo->ROICount < 8 ? pFaceROIInfo->ROICount : 8;
        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]: get meta OEMFDResults success! pFaceInfo->i32FaceNum %d",
              m_HdrInfo.m_FaceInfo.nFace);
        if (m_HdrInfo.m_FaceInfo.nFace <= 0) {
            m_HdrInfo.m_FaceInfo.nFace = 0;
            break;
        }

        uint32_t imageWidth = m_format.width;
        float ratio = 0;
        if (0 != pCropRegion.width) {
            ratio = (float)imageWidth / pCropRegion.width;
        }
        if (ratio <= 0) {
            ratio = 1.0f;
        }
        MInt32 i32Orientation = 0;
        getOrientation(&i32Orientation, metaData);

        switch (i32Orientation) {
        case 0:
            i32Orientation = 1;
            break;
        case 90:
            i32Orientation = 2;
            break;
        case 180:
            i32Orientation = 4;
            break;
        case 270:
            i32Orientation = 3;
            break;
        default:
            i32Orientation = 1;
        }
        for (MInt32 index = 0; index < m_HdrInfo.m_FaceInfo.nFace; index++) {
            uint32_t xMin = pFaceROIInfo->unstabilizedROI[index].faceRect.left - pCropRegion.left;
            uint32_t yMin = pFaceROIInfo->unstabilizedROI[index].faceRect.top - pCropRegion.top;
            uint32_t xMax = xMin + pFaceROIInfo->unstabilizedROI[index].faceRect.width - 1;
            uint32_t yMax = yMin + pFaceROIInfo->unstabilizedROI[index].faceRect.height - 1;

            // convert face rectangle to be based on current input dimension
            m_HdrInfo.m_FaceInfo.rcFace[index].left = xMin * ratio;
            m_HdrInfo.m_FaceInfo.rcFace[index].top = yMin * ratio;
            m_HdrInfo.m_FaceInfo.rcFace[index].right = xMax * ratio;
            m_HdrInfo.m_FaceInfo.rcFace[index].bottom = yMax * ratio;
            m_HdrInfo.m_FaceInfo.lFaceOrient[index] = i32Orientation;
        }
    } while (0);

    data = NULL;
    VendorMetadataParser::getVTagValue(metaData, "org.quic.camera2.statsconfigs.AECFrameControl",
                                       &data);
    if (NULL != data) {
        aeInfo->fLuxIndex = static_cast<AECFrameControl *>(data)->luxIndex;
        aeInfo->fISPGain =
            static_cast<AECFrameControl *>(data)->exposureInfo[ExposureIndexSafe].linearGain;
        aeInfo->fADRCGain =
            static_cast<AECFrameControl *>(data)->exposureInfo[ExposureIndexSafe].sensitivity /
            static_cast<AECFrameControl *>(data)->exposureInfo[ExposureIndexShort].sensitivity;
        aeInfo->exposure =
            static_cast<AECFrameControl *>(data)->exposureInfo[ExposureIndexSafe].exposureTime;
    } else {
        aeInfo->fLuxIndex = 50.0f;
        aeInfo->fISPGain = 1.0f;
        aeInfo->fADRCGain = 1.0f;
        aeInfo->exposure = 1.0f;
    }

    aeInfo->fADRCGainMin = ArcHDRCommonAEParams[0].adrc_min;
    aeInfo->fADRCGainMax = ArcHDRCommonAEParams[0].adrc_max;

    data = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_SENSOR_SENSITIVITY, &data);
    if (NULL != data) {
        aeInfo->iso = *static_cast<int32_t *>(data);
    } else {
        aeInfo->iso = 100;
    }
    MLOGI(Mia2LogGroupPlugin, "MiaNodeHdr get iso %d", aeInfo->iso);

    data = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_LENS_APERTURE, &data);
    if (NULL != data) {
        aeInfo->apture = *static_cast<float *>(data);
    } else {
        aeInfo->apture = 88;
    }

    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: fLuxIndex: %.2f fISPGain=%.2f fADRCGain=%.2f ",
          aeInfo->fLuxIndex, aeInfo->fISPGain, aeInfo->fADRCGain);
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: apture=%.2f iso=%d exposure=%.2f", aeInfo->apture,
          aeInfo->iso, aeInfo->exposure);
}

void ArcsoftHdrPlugin::getOrientation(MInt32 *pOrientation, camera_metadata_t *metaData)
{
    if (NULL == pOrientation || NULL == metaData)
        return;
    void *pData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_JPEG_ORIENTATION, &pData);
    if (NULL == pData) {
        *pOrientation = 0;
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: get meta ANDROID_JPEG_ORIENTATION fail!");
        return;
    }
    *pOrientation = *static_cast<int32_t *>(pData);
}

void ArcsoftHdrPlugin::DrawRect(struct MiImageBuffer *pOffscreen, MRECT RectDraw, MInt32 nLineWidth,
                                MInt32 iType)
{
    if (MNull == pOffscreen) {
        return;
    }
    nLineWidth = (nLineWidth + 1) / 2 * 2;
    MInt32 nPoints = nLineWidth <= 0 ? 1 : nLineWidth;

    MRECT RectImage = {0, 0, (MInt32)pOffscreen->width, (MInt32)pOffscreen->height};
    MRECT rectIn = intersection(RectDraw, RectImage);

    MInt32 nLeft = rectIn.left;
    MInt32 nTop = rectIn.top;
    MInt32 nRight = rectIn.right;
    MInt32 nBottom = rectIn.bottom;
    // MUInt8* pByteY = pOffscreen->ppu8Plane[0];
    int nRectW = nRight - nLeft;
    int nRecrH = nBottom - nTop;
    if (nTop + nPoints + nRecrH > pOffscreen->height) {
        nTop = pOffscreen->height - nPoints - nRecrH;
    }
    if (nTop < nPoints / 2) {
        nTop = nPoints / 2;
    }
    if (nBottom - nPoints < 0) {
        nBottom = nPoints;
    }
    if (nBottom + nPoints / 2 > pOffscreen->height) {
        nBottom = pOffscreen->height - nPoints / 2;
    }
    if (nLeft + nPoints + nRectW > pOffscreen->width) {
        nLeft = pOffscreen->width - nPoints - nRectW;
    }
    if (nLeft < nPoints / 2) {
        nLeft = nPoints / 2;
    }

    if (nRight - nPoints < 0) {
        nRight = nPoints;
    }
    if (nRight + nPoints / 2 > pOffscreen->width) {
        nRight = pOffscreen->width - nPoints / 2;
    }

    nRectW = nRight - nLeft;
    nRecrH = nBottom - nTop;

    if (pOffscreen->format == CAM_FORMAT_YUV_420_NV12 ||
        pOffscreen->format == CAM_FORMAT_YUV_420_NV21) {
        // draw the top line
        MUInt8 *pByteYTop =
            pOffscreen->plane[0] + (nTop - nPoints / 2) * pOffscreen->stride + nLeft - nPoints / 2;
        MUInt8 *pByteVTop = pOffscreen->plane[1] + ((nTop - nPoints / 2) / 2) * pOffscreen->stride +
                            nLeft - nPoints / 2;
        MUInt8 *pByteYBottom = pOffscreen->plane[0] +
                               (nTop + nRecrH - nPoints / 2) * pOffscreen->stride + nLeft -
                               nPoints / 2;
        MUInt8 *pByteVBottom = pOffscreen->plane[1] +
                               ((nTop + nRecrH - nPoints / 2) / 2) * pOffscreen->stride + nLeft -
                               nPoints / 2;
        for (int i = 0; i < nPoints; i++) {
            pByteVTop = pOffscreen->plane[1] + ((nTop + i - nPoints / 2) / 2) * pOffscreen->stride +
                        nLeft - nPoints / 2;
            pByteVBottom = pOffscreen->plane[1] +
                           ((nTop + i + nRecrH - nPoints / 2) / 2) * pOffscreen->stride + nLeft -
                           nPoints / 2;
            memset(pByteYTop, 255, nRectW + nPoints);
            memset(pByteYBottom, 255, nRectW + nPoints);
            pByteYTop += pOffscreen->stride;
            pByteYBottom += pOffscreen->stride;
            memset(pByteVTop, 60 * iType, nRectW + nPoints);
            memset(pByteVBottom, 60 * iType, nRectW + nPoints);
        }
        for (int i = 0; i < nRecrH; i++) {
            MUInt8 *pByteYLeft =
                pOffscreen->plane[0] + (nTop + i) * pOffscreen->stride + nLeft - nPoints / 2;
            MUInt8 *pByteYRight = pOffscreen->plane[0] + (nTop + i) * pOffscreen->stride + nLeft +
                                  nRectW - nPoints / 2;
            MUInt8 *pByteVLeft = pOffscreen->plane[1] + (((nTop + i) / 2) * pOffscreen->stride) +
                                 nLeft - nPoints / 2;
            MUInt8 *pByteVRight = pOffscreen->plane[1] + (((nTop + i) / 2) * pOffscreen->stride) +
                                  nLeft + nRectW - nPoints / 2;
            memset(pByteYLeft, 255, nPoints);
            memset(pByteYRight, 255, nPoints);
            memset(pByteVLeft, 60 * iType, nPoints);
            memset(pByteVRight, 60 * iType, nPoints);
        }
    }
}

MRECT ArcsoftHdrPlugin::intersection(const MRECT &a, const MRECT &d)
{
    int l = MATHL_MAX(a.left, d.left);
    int t = MATHL_MAX(a.top, d.top);
    int r = MATH_MIN(a.right, d.right);
    int b = MATH_MIN(a.bottom, d.bottom);
    if (l >= r || t >= b) {
        l = 0;
        t = 0;
        r = 0;
        b = 0;
    }
    MRECT c = {l, t, r, b};
    return c;
}
