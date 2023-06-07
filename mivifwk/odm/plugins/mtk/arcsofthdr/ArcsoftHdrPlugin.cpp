#include "ArcsoftHdrPlugin.h"

#define FRAME_MAX_INPUT_NUMBER 8
#define MAX_EV_NUM             8

#undef LOG_TAG
#define LOG_TAG "MIA_N_HDR"

ArcsoftHdrPlugin::~ArcsoftHdrPlugin()
{
    MLOGI(Mia2LogGroupPlugin, "%s:%d", __func__, __LINE__);
}

int ArcsoftHdrPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    m_frameworkCameraId = pCreateInfo->frameworkCameraId;
    if (CAM_COMBMODE_FRONT == m_frameworkCameraId ||
        CAM_COMBMODE_FRONT_BOKEH == m_frameworkCameraId ||
        CAM_COMBMODE_FRONT_AUX == m_frameworkCameraId) {
        m_cameraMode = MI_FRONT_CAMERA;
    } else if (CAM_COMBMODE_REAR_ULTRA == m_frameworkCameraId) {
        m_cameraMode = MI_UW_CAMERA;
    } else {
        m_cameraMode = MI_REAR_CAMERA;
    }

    m_Hdr = NULL;
    if (!m_Hdr) {
        m_Hdr = new ArcsoftHdr();
        m_Hdr->init();
    }

    memset(&m_FaceInfo, 0, sizeof(ARC_HDR_FACEINFO));
    m_FaceInfo.rcFace = (PMRECT)malloc(sizeof(MRECT) * MAX_FACE_NUM);
    m_FaceInfo.lFaceOrient = (MInt32 *)malloc(sizeof(MInt32) * MAX_FACE_NUM);
    memset(m_FaceInfo.rcFace, 0, sizeof(MRECT) * MAX_FACE_NUM);
    memset(m_FaceInfo.lFaceOrient, 0, sizeof(MInt32) * MAX_FACE_NUM);
    m_captureWidth = 0;
    m_captureHeight = 0;

    return 0;
}

ProcessRetStatus ArcsoftHdrPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    auto &phInputBuffer = pProcessRequestInfo->inputBuffersMap.at(0);
    auto &phOutputBuffer = pProcessRequestInfo->outputBuffersMap.at(0);
    int inputNum = 0;
    for (auto &&[port, vec] : pProcessRequestInfo->inputBuffersMap) {
        inputNum += vec.size();
    }
    if (inputNum > FRAME_MAX_INPUT_NUMBER) {
        inputNum = FRAME_MAX_INPUT_NUMBER;
    }
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.algoengine.arcsofthdr.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);

    camera_metadata_t *pMetaData = phInputBuffer[0].pMetadata;
    if (pMetaData == NULL) {
        return resultInfo;
    }

    MLOGI(Mia2LogGroupPlugin, "get inputNum: %d", inputNum);
    if (inputNum == 1) {
        return resultInfo;
    }

    memset(m_ev, 0, sizeof(m_ev));
    setEv(inputNum, pProcessRequestInfo->inputBuffersMap);

    struct MiImageBuffer inputFrame[FRAME_MAX_INPUT_NUMBER], outputFrame;
    int index = 0;
    for (auto &&[port, vec] : pProcessRequestInfo->inputBuffersMap) {
        for (auto &&image : vec) {
            inputFrame[index].format = image.format.format;
            inputFrame[index].width = image.format.width;
            inputFrame[index].height = image.format.height;
            inputFrame[index].plane[0] = image.pAddr[0];
            inputFrame[index].plane[1] = image.pAddr[1];
            inputFrame[index].stride = phInputBuffer[0].planeStride;
            inputFrame[index].scanline = phInputBuffer[0].sliceheight;
            inputFrame[index].numberOfPlanes = phInputBuffer[0].numberOfPlanes;
            if (iIsDumpData) {
                char inputbuf[128];
                snprintf(inputbuf, sizeof(inputbuf), "arcsofthdr_input[%d]_%d_%dx%d", index,
                         m_ev[index], inputFrame[index].width,
                         inputFrame[index].height); // Modify naming rules
                PluginUtils::dumpToFile(inputbuf, &inputFrame[index]);
            }
            index++;
        }
    }

    outputFrame.format = phOutputBuffer[0].format.format;
    outputFrame.width = phOutputBuffer[0].format.width;
    outputFrame.height = phOutputBuffer[0].format.height;
    outputFrame.plane[0] = phOutputBuffer[0].pAddr[0];
    outputFrame.plane[1] = phOutputBuffer[0].pAddr[1];
    outputFrame.stride = phOutputBuffer[0].planeStride;
    outputFrame.scanline = phOutputBuffer[0].sliceheight;
    outputFrame.numberOfPlanes = phOutputBuffer[0].numberOfPlanes;

    int ret = processBuffer(inputFrame, inputNum, &outputFrame, pMetaData);
    if (ret != PROCSUCCESS) {
        resultInfo = PROCFAILED;
    }
    MLOGI(Mia2LogGroupPlugin, "%s:%d ProcessBuffer ret: %d", __func__, __LINE__, ret);

    if (!ret && iIsDumpData) {
        char outputbuf[128];
        snprintf(outputbuf, sizeof(outputbuf), "arcsofthdr_output_%dx%d", outputFrame.width,
                 outputFrame.height);
        PluginUtils::dumpToFile(outputbuf, &outputFrame);
    }

    return resultInfo;
}

ProcessRetStatus ArcsoftHdrPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int ArcsoftHdrPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

void ArcsoftHdrPlugin::destroy()
{
    if (m_Hdr) {
        delete m_Hdr;
        m_Hdr = NULL;
    }
    if (m_FaceInfo.rcFace) {
        free(m_FaceInfo.rcFace);
        m_FaceInfo.rcFace = NULL;
    }

    if (m_FaceInfo.lFaceOrient) {
        free(m_FaceInfo.lFaceOrient);
        m_FaceInfo.lFaceOrient = NULL;
    }
}

bool ArcsoftHdrPlugin::isEnabled(MiaParams settings)
{
    void *pData = NULL;
    bool hdrEnable = false;
    const char *HdrEnable = "xiaomi.hdr.enabled";
    VendorMetadataParser::getVTagValue(settings.metadata, HdrEnable, &pData);
    if (NULL != pData) {
        hdrEnable = *static_cast<uint8_t *>(pData);
    }

    pData = NULL;
    int hdrSrEnable = false;
    const char *HdrSrEnable = "xiaomi.hdr.sr.enabled";
    VendorMetadataParser::getVTagValue(settings.metadata, HdrSrEnable, &pData);
    if (NULL != pData) {
        hdrSrEnable = *static_cast<uint8_t *>(pData);
        m_isHdrSr = hdrSrEnable;
    }
    MLOGD(Mia2LogGroupPlugin, "Get hdrSrEnable: %d", hdrSrEnable);

    if (hdrEnable || hdrSrEnable) {
        return true;
    } else {
        return false;
    }
}

int ArcsoftHdrPlugin::processBuffer(struct MiImageBuffer *input, int input_num,
                                    struct MiImageBuffer *output, camera_metadata_t *metaData)
{
    if (metaData == NULL) {
        MLOGE(Mia2LogGroupPlugin, "error metaData is null");
        return PROCFAILED;
    }
    MLOGD(Mia2LogGroupPlugin, "ProcessBuffer input_num: %d, m_frameworkCameraId: %d", input_num,
          m_frameworkCameraId);

    m_captureWidth = input->width;
    m_captureHeight = input->height;
    m_inputNum = input_num;
    arc_hdr_input_meta_t meta = {0};
    getAEInfo(metaData, &meta);
    meta.input_num = input_num;
    meta.frameworkCameraID = m_frameworkCameraId;

    if (m_Hdr) {
        m_Hdr->Process(input, &meta, output);
    } else {
        MLOGD(Mia2LogGroupPlugin, "error m_Hdr is null");
        PluginUtils::miCopyBuffer(output, input);
    }

    return PROCSUCCESS;
}

void ArcsoftHdrPlugin::setEv(int inputnum,
                             std::map<uint32_t, std::vector<ImageParams>> &phInputBufferMap)
{
    if (inputnum > MAX_EV_NUM) {
        MLOGE(Mia2LogGroupPlugin, "hdr inputnum out of m_ev array length");
        return;
    }
    int index = 0;
    for (auto &&[port, vec] : phInputBufferMap) {
        for (auto &&image : vec) {
            void *pData = NULL;
            VendorMetadataParser::getTagValue(image.pMetadata,
                                              ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &pData);
            if (NULL != pData) {
                int32_t perFrameEv = *static_cast<int32_t *>(pData);
                m_ev[index] = perFrameEv;
                MLOGI(Mia2LogGroupPlugin, "get m_ev[%d] = %d", index, m_ev[index]);
            } else {
                m_ev[index] = 404;
                MLOGE(Mia2LogGroupPlugin, "Failed to get inputnumber  #%d's ev!", index);
            }
            if (++index >= inputnum) {
                break;
            }
        }
    }
}

void ArcsoftHdrPlugin::getAEInfo(camera_metadata_t *metaData, arc_hdr_input_meta_t *pAeInfo)
{
    void *pData = NULL;
    const char *BpsGain = "com.xiaomi.sensorbps.gain";
    VendorMetadataParser::getVTagValue(metaData, BpsGain, &pData);
    if (NULL != pData) {
        pAeInfo->fSensorGain = *static_cast<float *>(pData);
    } else {
        pAeInfo->fSensorGain = 1.0f;
    }

    pData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.hdr.sr.enabled", &pData);
    if (NULL != pData) {
        pAeInfo->isHdrSr = *static_cast<uint8_t *>(pData);
    } else {
        pAeInfo->isHdrSr = 0;
    }
    MLOGI(Mia2LogGroupPlugin, "get isHdrSr status: %d", pAeInfo->isHdrSr);

    pData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.hdr.hdrChecker.sceneType", &pData);
    if (NULL != pData) {
        pAeInfo->scene_type = *static_cast<int32_t *>(pData);
    }
    MLOGI(Mia2LogGroupPlugin, "get hdr scenetype %d", pAeInfo->scene_type);

    pData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "com.xiaomi.statsconfigs.AecInfo", &pData);
    if (NULL != pData) {
        pAeInfo->fLuxIndex = static_cast<InputMetadataAecInfo *>(pData)->luxIndex;
        pAeInfo->fISPGain = static_cast<InputMetadataAecInfo *>(pData)->linearGain;
        pAeInfo->fADRCGain = static_cast<InputMetadataAecInfo *>(pData)->adrcgain;
        pAeInfo->exposure = static_cast<InputMetadataAecInfo *>(pData)->exposureTime;
    } else {
        pAeInfo->fLuxIndex = 50.0f;
        pAeInfo->fISPGain = 1.0f;
        pAeInfo->fADRCGain = 1.0f;
        pAeInfo->exposure = 1.0f;
    }

    pAeInfo->camera_mode = m_cameraMode;
    pAeInfo->fADRCGainMin = HARDCODE_MIN_ADRCGAIN_VALUE;
    pAeInfo->fADRCGainMax = HARDCODE_MAX_ADRCGAIN_VALUE;

    pData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_SENSOR_SENSITIVITY, &pData);
    if (NULL != pData) {
        pAeInfo->iso = *static_cast<int32_t *>(pData);
    } else {
        pAeInfo->iso = 100;
    }
    MLOGI(Mia2LogGroupPlugin, "get iso %d", pAeInfo->iso);

    pData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_LENS_APERTURE, &pData);
    if (NULL != pData) {
        pAeInfo->apture = *static_cast<float *>(pData);
    } else {
        pAeInfo->apture = 88;
    }

    pData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.hdr.hdrChecker", &pData);
    if (NULL != pData) {
        OutputMetadataHdrChecker hdrChecker = *static_cast<OutputMetadataHdrChecker *>(pData);
        for (uint32_t i = 0; i < hdrChecker.number; i++) {
            pAeInfo->ev[i] = hdrChecker.ev[i];
            MLOGI(Mia2LogGroupPlugin, "Get HDR CHECKER: EV[%d]: %d", i, hdrChecker.ev[i]);
        }
    } else {
        MLOGD(Mia2LogGroupPlugin, "failed to get ev, get it from HAL");
        for (int32_t i = 0; i < m_inputNum; i++) {
            pAeInfo->ev[i] = m_ev[i]; // Get from the setEV() function
            MLOGI(Mia2LogGroupPlugin, "Get HDR CHECKER: EV[%d]: %d", i, pAeInfo->ev[i]);
        }
    }

    pData = NULL;
    uint32_t m_sensorWidth = 0;
    uint32_t m_sensorHeight = 0;
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.sensorSize.size", &pData);
    if (NULL != pData) {
        MiDimension *pDim = (MiDimension *)(pData);
        m_sensorWidth = pDim->width;
        m_sensorHeight = pDim->height;
    } else {
        MLOGI(Mia2LogGroupPlugin, "get SensorSize failed, use capturesize!");
        m_sensorWidth = m_captureWidth;
        m_sensorHeight = m_captureHeight;
    }

    pData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_JPEG_ORIENTATION, &pData);
    if (NULL != pData) {
        pAeInfo->orientation = *static_cast<int32_t *>(pData);
        MLOGE(Mia2LogGroupPlugin, "get orientation = %d\n", pAeInfo->orientation);
    }

    MIRECT pCropRegion = {0};
    pData = NULL;
    memset(&pCropRegion, 0, sizeof(pCropRegion));
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.superResolution.cropRegionMtk", &pData);
    if (NULL == pData) {
        VendorMetadataParser::getTagValue(metaData, ANDROID_SCALER_CROP_REGION, &pData);
    }

    if (NULL != pData) {
        pCropRegion = *static_cast<MIRECT *>(pData);
        MLOGI(Mia2LogGroupPlugin, "get ZoomCropRegion before: [%d %d %d %d]", pCropRegion.left,
              pCropRegion.top, pCropRegion.width, pCropRegion.height);
    } else {
        pCropRegion.left = (m_sensorWidth - m_captureWidth) / 2;
        pCropRegion.top = (m_sensorHeight - m_captureHeight) / 2;
        pCropRegion.width = m_captureWidth;
        pCropRegion.height = m_captureHeight;
        MLOGI(Mia2LogGroupPlugin, "get ZoomCropRegion failed, use full size: [%d %d %d %d]",
              pCropRegion.left, pCropRegion.top, pCropRegion.width, pCropRegion.height);
    }

    pAeInfo->cropRegion.left = pCropRegion.left;
    pAeInfo->cropRegion.top = pCropRegion.top;
    pAeInfo->cropRegion.right = pCropRegion.left + pCropRegion.width;
    pAeInfo->cropRegion.bottom = pCropRegion.top + pCropRegion.height;
    MLOGE(Mia2LogGroupPlugin,
          "get cropregion for arcsoft HDR:[left top right bottom] = [%d %d %d %d]\n",
          pAeInfo->cropRegion.left, pAeInfo->cropRegion.top, pAeInfo->cropRegion.right,
          pAeInfo->cropRegion.bottom);

    float XRatio = (float)m_captureWidth / m_sensorWidth;
    float YRatio = (float)m_captureHeight / m_sensorHeight;
    MLOGI(Mia2LogGroupPlugin, "XRatio %f, YRatio: %f", XRatio, YRatio);

    int TopOffSize = 0;
    int LeftOffSize = 0;
    float zoomRatio = 1.0;

    if (XRatio == 1) {
        TopOffSize = m_sensorHeight * (1 - YRatio) * 0.5;
        zoomRatio = (float)m_sensorWidth / pCropRegion.width;
    }

    if (YRatio == 1) {
        LeftOffSize = m_sensorWidth * (1 - XRatio) * 0.5;
        zoomRatio = (float)m_sensorHeight / pCropRegion.height;
    }
    MLOGI(Mia2LogGroupPlugin, "zoomRatio : %f OffSize x = %d, y = %d", zoomRatio, LeftOffSize,
          TopOffSize);

    FDMetadataResults face_Info = {0};
    auto setFaceInfo = [](camera_metadata_t *meta, FDMetadataResults &face_info) {
        void *pData = NULL;
        size_t tcount = 0;

        // VendorMetadataParser::getTagValueCount(meta, ANDROID_STATISTICS_FACE_RECTANGLES, &pData,
        // tcount);
        if (NULL == pData) {
            VendorMetadataParser::getTagValueCount(meta, ANDROID_STATISTICS_FACE_RECTANGLES, &pData,
                                                   &tcount);
        }
        if (NULL != pData) {
            face_info.numFaces = ((int32_t)tcount) / 4;

            if (face_info.numFaces > FDMaxFaces)
                face_info.numFaces = FDMaxFaces;

            MLOGD(Mia2LogGroupPlugin, "get mFaceInfo.numFaces = %d", face_info.numFaces);
            memcpy(face_info.faceRect, pData, face_info.numFaces * sizeof(MIRECT));

            for (int i = 0; i < face_info.numFaces; i++) {
                // face rect get from ANDROID_STATISTICS_FACE_RECTANGLES is [left, top, right,
                // bottom], so we need to trans it to [left, top, width, height]
                face_info.faceRect[i].width -= face_info.faceRect[i].left;
                face_info.faceRect[i].height -= face_info.faceRect[i].top;
                MLOGD(Mia2LogGroupPlugin, "get face info: [%u %u %u %u]",
                      face_info.faceRect[i].left, face_info.faceRect[i].top,
                      face_info.faceRect[i].width, face_info.faceRect[i].height);
            }
        }
    };
    setFaceInfo(metaData, face_Info);

    memset(m_FaceInfo.rcFace, 0, sizeof(MRECT) * MAX_FACE_NUM);
    memset(m_FaceInfo.lFaceOrient, 0, sizeof(MInt32) * MAX_FACE_NUM);
    m_FaceInfo.nFace = face_Info.numFaces < MAX_FACE_NUM ? face_Info.numFaces : MAX_FACE_NUM;
    MLOGI(Mia2LogGroupPlugin, "get face_num: %d", m_FaceInfo.nFace);

    if (m_FaceInfo.nFace > 0) {
        for (int index = 0; index < m_FaceInfo.nFace; index++) {
            int32_t xMin = face_Info.faceRect[index].left;
            int32_t yMin = face_Info.faceRect[index].top;
            int32_t xMax = face_Info.faceRect[index].left + face_Info.faceRect[index].width;
            int32_t yMax = face_Info.faceRect[index].top + face_Info.faceRect[index].height;
            MLOGI(Mia2LogGroupPlugin, "orig face_Info[%d]: [%d %d %d %d] Width=%d Height=%d ",
                  index, xMin, yMin, xMax, yMax, xMax - xMin, yMax - yMin);

            switch (pAeInfo->orientation) {
            case 0:
            default:
                m_FaceInfo.lFaceOrient[index] = 1;
                break;
            case 90:
                m_FaceInfo.lFaceOrient[index] = 2;
                break;
            case 180:
                m_FaceInfo.lFaceOrient[index] = 4;
                break;
            case 270:
                m_FaceInfo.lFaceOrient[index] = 3;
                break;
            }

            xMin = (xMin - (int32_t)pCropRegion.left) * zoomRatio;
            xMax = (xMax - (int32_t)pCropRegion.left) * zoomRatio;
            yMin = (yMin - (int32_t)pCropRegion.top) * zoomRatio;
            yMax = (yMax - (int32_t)pCropRegion.top) * zoomRatio;

            auto checkCoordinates = [](int32_t &value, int32_t min, int32_t max) {
                value = (value < min) ? min : ((value > max) ? max : value);
            };

            checkCoordinates(xMin, 0, m_captureWidth);
            checkCoordinates(xMax, 0, m_captureWidth);
            checkCoordinates(yMin, 0, m_captureHeight);
            checkCoordinates(yMax, 0, m_captureHeight);

            m_FaceInfo.rcFace[index].left = xMin;
            m_FaceInfo.rcFace[index].top = yMin;
            m_FaceInfo.rcFace[index].right = xMax;
            m_FaceInfo.rcFace[index].bottom = yMax;

            MLOGI(Mia2LogGroupPlugin,
                  "after m_FaceInfo.rcFace[%d]: [%d %d %d %d] Width=%d Height=%d face_ori %d %d",
                  index, xMin, yMin, xMax, yMax, xMax - xMin, yMax - yMin, pAeInfo->orientation,
                  m_FaceInfo.lFaceOrient[index]);
        }
    }

    memcpy(&pAeInfo->face_info, &m_FaceInfo, sizeof(ARC_HDR_FACEINFO));

    MLOGD(Mia2LogGroupPlugin, "[ARCSOFT]: fLuxIndex: %.2f fISPGain=%.2f fADRCGain=%.2f ",
          pAeInfo->fLuxIndex, pAeInfo->fISPGain, pAeInfo->fADRCGain);
    MLOGD(Mia2LogGroupPlugin, "[ARCSOFT]: apture=%.2f iso=%d exposure=%.2f", pAeInfo->apture,
          pAeInfo->iso, pAeInfo->exposure);
}
