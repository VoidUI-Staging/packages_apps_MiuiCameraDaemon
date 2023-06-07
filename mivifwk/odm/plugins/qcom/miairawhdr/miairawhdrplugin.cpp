#include "miairawhdrplugin.h"

#include "camlog.h"

#undef LOG_TAG
#define LOG_TAG "MIAI_RAWHDR"

static const uint32_t MinBLSValue = 1000;
static const uint32_t MaxBLSValue = 1050;
static const uint32_t defaultBLSValue = 1024;
static const uint32_t LSCTotalChannels = 4; ///< Total IFE channels R, Gr, Gb and B
static const uint32_t LSC_MESH_ROLLOFF_SIZE = 17 * 13;
static const uint8_t MISDK_HDR_ALGOUP = 2;
static const int32_t sIsDumpData =
    property_get_int32("persist.vendor.camera.capture.burst.rawhdrdump", 0);
static const uint8_t DOL_NUM = 3;

//////////////////////////////////////////////
static const int32_t MIRAWHDR_UPDATEREQINFO = 3; // ev0 index
//////////////////////////////////////////////
namespace mialgo2 {

MiAiRAWHdrPlugin::~MiAiRAWHdrPlugin() {}

int MiAiRAWHdrPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mFrameworkCameraId = createInfo->frameworkCameraId;
    MLOGI(Mia2LogGroupPlugin, "[%s] mFrameworkCameraId %d", LOG_TAG, mFrameworkCameraId);
    if (CAM_COMBMODE_FRONT == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_BOKEH == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_AUX == mFrameworkCameraId) {
        mFrontCamera = true;
    } else {
        mFrontCamera = false;
    }
    mRawHdr = NULL;
    if (!mRawHdr) {
        mRawHdr = new MiAIRAWHDR();
        mRawHdr->init(mFrameworkCameraId);
    }
    mNodeInterface = nodeInterface;

#if 0
    mLiteEnable = 0;
    mResumeNormalAlgo = property_get_int32("persist.vendor.camera.hdr.resume.normal", 2);
#endif
    return MIA_RETURN_OK;
}

bool MiAiRAWHdrPlugin::isEnabled(MiaParams settings)
{
    bool hdrEnable = false;
    void *pinputNum = NULL;
    isSdkAlgoUp(settings.metadata);
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.multiframe.inputNum", &pinputNum);
    if (NULL != pinputNum && *static_cast<uint8_t *>(pinputNum) > 1) {
        void *data = NULL;
        const char *hdrEnablePtr = "xiaomi.hdr.raw.enabled";
        VendorMetadataParser::getVTagValue(settings.metadata, hdrEnablePtr, &data);
        if (NULL != data) {
            hdrEnable = *static_cast<uint8_t *>(data);
        }
    }

    MLOGI(Mia2LogGroupPlugin, "[%s] rawhdr set enable value %d", LOG_TAG, hdrEnable);
    return hdrEnable;
}

void MiAiRAWHdrPlugin::getRawPattern(camera_metadata_t *metadata, MIAI_HDR_RAWINFO &rawInfo)
{
    void *pData = NULL;
    MultiCameraIds multiCameraRole;
    memset(&multiCameraRole, 0, sizeof(multiCameraRole));
    const char *MultiCameraRole = "com.qti.chi.multicamerainfo.MultiCameraIds";
    VendorMetadataParser::getVTagValue(metadata, MultiCameraRole, &pData);
    if (NULL != pData) {
        multiCameraRole = *static_cast<MultiCameraIds *>(pData);
        pData = NULL;
    } else {
        MLOGE(Mia2LogGroupPlugin, "[%s] get multicamerainfo.MultiCameraIds fail", LOG_TAG);
    }
    if (mSdkMode) {
        if (CAM_COMBMODE_REAR_WIDE == mFrameworkCameraId) {
            multiCameraRole.fwkCameraId = 8;
        } else if (CAM_COMBMODE_REAR_ULTRA == mFrameworkCameraId) {
            multiCameraRole.fwkCameraId = 2;
        }
    }

    camera_metadata_t *m_pStaticMetadata =
        StaticMetadataWraper::getInstance()->getStaticMetadata(multiCameraRole.fwkCameraId);
    VendorMetadataParser::getTagValue(m_pStaticMetadata,
                                      ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT, &pData);
    if (NULL != pData) {
        int bayerPattern = *static_cast<int32_t *>(pData);
        switch (bayerPattern) {
        case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB:
            rawInfo.raw_type = ColorFilterPattern::RGGB;
            break;
        case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG:
            rawInfo.raw_type = ColorFilterPattern::GRBG;
            break;
        case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG:
            rawInfo.raw_type = ColorFilterPattern::GBRG;
            break;
        case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR:
            rawInfo.raw_type = ColorFilterPattern::BGGR;
            break;
        default:
            rawInfo.raw_type = ColorFilterPattern::BGGR;
            break;
        }
        MLOGI(Mia2LogGroupPlugin, "[%s]: ChiColorFilterPattern = %d", LOG_TAG, rawInfo.raw_type);
    } else {
        MLOGD(Mia2LogGroupPlugin, "[%s]: Failed to get sensor bayer pattern", LOG_TAG);
    }
}

void MiAiRAWHdrPlugin::getRawInfo(MIAI_HDR_RAWINFO &rawInfo, int &inputNum,
                                  MiImageBuffer &inputFrame, ProcessRequestInfo *processRequestInfo)
{
    rawInfo.device_orientation = hdrMetaData.config.orientation;
    rawInfo.nframes = inputNum;
    rawInfo.width = inputFrame.width;
    rawInfo.height = inputFrame.height;
    rawInfo.stride = inputFrame.stride;
    rawInfo.bits_per_pixel = 16;

    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    camera_metadata_t *logicalMetaData = inputBuffers[0].pMetadata;
    getFaceInfo(logicalMetaData, inputFrame.width, inputFrame.height, rawInfo);

    void *pHDRData = NULL;

    for (int i = 0; i < inputNum; ++i) {
        pHDRData = NULL;
        camera_metadata_t *metadata = inputBuffers[i].pMetadata;
        camera_metadata_t *pPhyMetadata = inputBuffers[i].pPhyCamMetadata;
        if (mSdkMode) {
            pPhyMetadata = inputBuffers[i].pPhyCamMetadata ? inputBuffers[i].pPhyCamMetadata
                                                           : inputBuffers[i].pMetadata;
            pPhyMetadata = pPhyMetadata ? pPhyMetadata : inputBuffers[i].pMetadata;
        }

        VendorMetadataParser::getTagValue(metadata, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                          &pHDRData);
        if (NULL != pHDRData) {
            int32_t perFrameEv = *static_cast<int32_t *>(pHDRData);
            rawInfo.ev[i] = perFrameEv;
            MLOGD(Mia2LogGroupPlugin, "[%s]: Get bracketEv[%d] = %d", LOG_TAG, i, perFrameEv);
            pHDRData = NULL;
        } else {
            rawInfo.ev[i] = 404;
            MLOGI(Mia2LogGroupPlugin, "[%s]: Failed to get frame %d -> #%d's ev!", LOG_TAG,
                  processRequestInfo->frameNum, i);
        }

        VendorMetadataParser::getVTagValue(
            metadata, "org.quic.camera2.statsconfigs.AECFrameControl", &pHDRData);
        MFloat fTotalGain = 0;
        if (NULL != pHDRData) {
            AECFrameControl *pAECFrameControl = static_cast<AECFrameControl *>(pHDRData);
            rawInfo.shutter[i] =
                pAECFrameControl->exposureInfo[ExposureIndexSafe].exposureTime / 1e9;
            fTotalGain = pAECFrameControl->exposureInfo[ExposureIndexSafe].linearGain;
            if (i == 0) {
                rawInfo.luxindex = pAECFrameControl->luxIndex;
            }
            pHDRData = NULL;
        } else {
            MLOGI(Mia2LogGroupPlugin, "[%s]: Failed to get AEC info!", LOG_TAG);
        }

        VendorMetadataParser::getVTagValue(
            metadata, "org.quic.camera2.statsconfigs.AWBFrameControl", &pHDRData);
        if (NULL != pHDRData) {
            AWBFrameControl *awbControl = (AWBFrameControl *)pHDRData;
            if (i == 0) {
                rawInfo.red_gain = awbControl->AWBGains.rGain;
                rawInfo.blue_gain = awbControl->AWBGains.bGain;
            }
            rawInfo.CCT[i] = awbControl->colorTemperature;

            pHDRData = NULL;
        } else {
            MLOGI(Mia2LogGroupPlugin, "[%s]: Failed to get AEC info!", LOG_TAG);
        }

        VendorMetadataParser::getVTagValue(metadata, "xiaomi.hdr.stg.exposureInfo", &pHDRData);
        if (NULL != pHDRData) {
            StaggerExposureInfo info = *static_cast<StaggerExposureInfo *>(pHDRData);
            uint8_t index = i % DOL_NUM;

            switch (index) {
            case 0:
                rawInfo.analog_gain[i] = info.longSensorGain;
                rawInfo.digital_gain[i] = info.longIspGain;
                break;
            case 1:
                rawInfo.analog_gain[i] = info.middleSensorGain;
                rawInfo.digital_gain[i] = info.middleIspGain;
                break;
            case 2:
                rawInfo.analog_gain[i] = info.shortSensorGain;
                rawInfo.digital_gain[i] = info.shortIspGain;
                break;
            }
            MLOGI(Mia2LogGroupPlugin, "[%s], Get img[#%d] analog gain = %f, digital gain = %f",
                  LOG_TAG, i, rawInfo.analog_gain[i], rawInfo.digital_gain[i]);
            pHDRData = NULL;
        } else {
            VendorMetadataParser::getVTagValue(metadata, "com.qti.sensorbps.gain", &pHDRData);
            if (NULL != pHDRData) {
                rawInfo.digital_gain[i] = *static_cast<float *>(pHDRData);
                if (rawInfo.digital_gain[i] > 0.000001f || rawInfo.digital_gain[i] < 0.000001f) {
                    rawInfo.analog_gain[i] =
                        (float)((float)fTotalGain / (float)rawInfo.digital_gain[i]);
                    pHDRData = NULL;
                }
            } else {
                MLOGI(Mia2LogGroupPlugin, "[%s]: Failed to get Gain info!", LOG_TAG);
            }
        }

        LSCDimensionCap mLSCDim;
        memset(&mLSCDim, 0, sizeof(LSCDimensionCap));
        float m_lensShadingMap[LSCTotalChannels *
                               LSC_MESH_ROLLOFF_SIZE]; ///< Lens Shading Map per channel
        memset(m_lensShadingMap, 0, sizeof(m_lensShadingMap));
        VendorMetadataParser::getTagValue(pPhyMetadata, ANDROID_LENS_INFO_SHADING_MAP_SIZE,
                                          &pHDRData);
        if (NULL != pHDRData) {
            mLSCDim = *static_cast<LSCDimensionCap *>(pHDRData);
            MLOGI(Mia2LogGroupPlugin,
                  "[%s]: get ANDROID_LENS_INFO_SHADING_MAP_SIZE success mLSCDim=[%d %d]", LOG_TAG,
                  mLSCDim.width, mLSCDim.height);
            pHDRData = NULL;
        }
        VendorMetadataParser::getTagValue(pPhyMetadata, ANDROID_STATISTICS_LENS_SHADING_MAP,
                                          &pHDRData);
        if (NULL != pHDRData) {
            MLOGI(Mia2LogGroupPlugin, "[%s]: get ANDROID_STATISTICS_LENS_SHADING_MAP success",
                  LOG_TAG);
            memcpy(m_lensShadingMap, static_cast<float *>(pHDRData),
                   mLSCDim.width * mLSCDim.height * 4 * sizeof(float));
            pHDRData = NULL;
        }
        for (int channel = 0; channel < LSCTotalChannels; channel++) {
            for (uint16_t h = 0; h < mLSCDim.height; h++) {
                for (uint16_t w = 0; w < mLSCDim.width; w++) {
                    uint16_t index =
                        h * mLSCDim.width * LSCTotalChannels + w * LSCTotalChannels + channel;
                    switch (channel) {
                    case 0:
                        rawInfo.lsc[i].R[h][w] = m_lensShadingMap[index];
                        break;
                    case 1:
                        rawInfo.lsc[i].GR[h][w] = m_lensShadingMap[index];
                        break;
                    case 2:
                        rawInfo.lsc[i].GB[h][w] = m_lensShadingMap[index];
                        break;
                    case 3:
                        rawInfo.lsc[i].B[h][w] = m_lensShadingMap[index];
                        break;
                    }
                }
            }
        }
    }
}

void MiAiRAWHdrPlugin::getFaceInfo(camera_metadata_t *logicalMetaData, int width, int height,
                                   MIAI_HDR_RAWINFO &rawInfo)
{
    // get facenums
    camera_metadata_entry_t entry = {0};
    rawInfo.face_info.nFace = -1;
    uint32_t numElemsRect = sizeof(MRECT) / sizeof(uint32_t);
    if (0 !=
        find_camera_metadata_entry(logicalMetaData, ANDROID_STATISTICS_FACE_RECTANGLES, &entry)) {
        MLOGI(Mia2LogGroupPlugin, "get ANDROID_STATISTICS_FACE_RECTANGLES failed");
        return;
    }
    rawInfo.face_info.nFace = entry.count / numElemsRect;
    rawInfo.face_info.nFace = rawInfo.face_info.nFace < HDR_RAW_MAX_FACE_NUMBER
                                  ? rawInfo.face_info.nFace
                                  : HDR_RAW_MAX_FACE_NUMBER;
    MLOGI(Mia2LogGroupPlugin, "[MIAI_RAWHDR] face number %d", rawInfo.face_info.nFace);

    uint32_t mSnapshotWidth = width;
    uint32_t mSnapshotHeight = height;
    float_t mSensorWidth = 0;
    float_t mSensorHeight = 0;

    // get sensor size for mivifwk, rather than active size for hal
    ChiDimension activeSize = {0};
    void *pHDRData = NULL;
    const char *SensorSize = "xiaomi.sensorSize.size";
    VendorMetadataParser::getVTagValue(logicalMetaData, SensorSize, &pHDRData);
    if (NULL != pHDRData) {
        activeSize = *(ChiDimension *)(pHDRData);
        MLOGI(Mia2LogGroupPlugin, "[MIAI_RAWHDR] get sensor activeSize %d %d", activeSize.width,
              activeSize.height);
        // sensor size is 1/2 active size
        mSensorWidth = activeSize.width / 2;
        mSensorHeight = activeSize.height / 2;
    } else {
        mSensorWidth = mSnapshotWidth;
        mSensorHeight = mSnapshotHeight;
    }
    MLOGI(Mia2LogGroupPlugin, "[MIAI_RAWHDR] sensor: %d, %d; snapshot:%d, %d", mSensorWidth,
          mSensorHeight, mSnapshotWidth, mSnapshotHeight);

    // face rectangle
    pHDRData = NULL;
    VendorMetadataParser::getTagValue(logicalMetaData, ANDROID_STATISTICS_FACE_RECTANGLES,
                                      &pHDRData);
    if (NULL != pHDRData) {
        void *tmpData = reinterpret_cast<void *>(pHDRData);
        float zoomRatio = (float)mSnapshotWidth / mSensorWidth;
        float yOffSize = ((float)zoomRatio * mSensorHeight - mSnapshotHeight) / 2.0;
        MLOGI(Mia2LogGroupPlugin, "[MIAI_RAWHDR] zoomRatio:%f, yOffSize:%f", zoomRatio, yOffSize);

        uint32_t dataIndex = 0;
        for (int index = 0; index < rawInfo.face_info.nFace; index++) {
            int32_t xMin = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t yMin = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t xMax = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t yMax = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            MLOGI(Mia2LogGroupPlugin,
                  "[MIAI_RAWHDR] origin face [xMin,yMin,xMax,yMax][%d,%d,%d,%d]", xMin, yMin, xMax,
                  yMax);
            xMin = xMin * zoomRatio;
            xMax = xMax * zoomRatio;
            yMin = yMin * zoomRatio - yOffSize;
            yMax = yMax * zoomRatio - yOffSize;
            MLOGI(Mia2LogGroupPlugin, "[MIAI_RAWHDR] now face [xMin,yMin,xMax,yMax][%d,%d,%d,%d]",
                  xMin, yMin, xMax, yMax);

            rawInfo.face_info.rcFace[index].left = xMin;
            rawInfo.face_info.rcFace[index].top = yMin;
            rawInfo.face_info.rcFace[index].right = xMax;
            rawInfo.face_info.rcFace[index].bottom = yMax;
        }
    } else {
        MLOGI(Mia2LogGroupPlugin, "[MIAI_RAWHDR] faces are not found");
    }
}

ProcessRetStatus MiAiRAWHdrPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    uint8_t isSrHDR = 0;
    int32_t bracketEv[FRAME_MAX_INPUT_NUMBER];
    MIAI_HDR_RAWINFO rawInfo;
    memset(&rawInfo, 0, sizeof(rawInfo));
    memset(&rectRawHDRCropRegion, 0, sizeof(MRECT));
    MLOGI(Mia2LogGroupPlugin, "%s:%d", __func__, __LINE__);

    // need to fix
#if 0
    // to make sure fastshot S2S time, switch to lite algo when concurrent running task >=4
    // resume to normal algo when concurrent<=2
    if (mLiteEnable == 0 &&
        processRequestInfo->runningTaskNum >= processRequestInfo->maxConcurrentNum - 1)
        mLiteEnable = 1;
    else if (mLiteEnable == 1 && processRequestInfo->runningTaskNum <= mResumeNormalAlgo)
        mLiteEnable = 0;
    MLOGI(Mia2LogGroupPlugin, "fastshot, liteHDR=%d, runningTaskNum=%d, mResumeNormalAlgo=%d",
          mLiteEnable, processRequestInfo->runningTaskNum, mResumeNormalAlgo);
#endif

    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;
    int inputNum = inputBuffers.size();
    if (inputNum > FRAME_MAX_INPUT_NUMBER) {
        inputNum = FRAME_MAX_INPUT_NUMBER;
    }
    int32_t iIsDumpData = sIsDumpData;

    camera_metadata_t *logicalMetaData = inputBuffers[0].pMetadata;
    camera_metadata_t *outputMetaData = outputBuffers[0].pMetadata;
    camera_metadata_t *outPhyMetaData = outputBuffers[0].pPhyCamMetadata;
    camera_metadata_t *pPhyCamMetadata = inputBuffers[0].pPhyCamMetadata;

    if (mSdkMode) {
        outPhyMetaData = outputBuffers[0].pPhyCamMetadata ? outputBuffers[0].pPhyCamMetadata
                                                          : outputBuffers[0].pMetadata;
        pPhyCamMetadata = inputBuffers[0].pPhyCamMetadata ? inputBuffers[0].pPhyCamMetadata
                                                          : inputBuffers[0].pMetadata;
    }

    memset(mEV, 0, sizeof(mEV));
    setEv(inputNum, inputBuffers);
    getRawPattern(pPhyCamMetadata, rawInfo);

    struct MiImageBuffer inputFrame[FRAME_MAX_INPUT_NUMBER], outputFrame;

    memset(mRawHdr->aecExposureData, 0, sizeof(MiAECExposureData) * MAX_HDR_INPUT_NUM);
    memset(mRawHdr->awbGainParams, 0, sizeof(MiAWBGainParams) * MAX_HDR_INPUT_NUM);
    memset(mRawHdr->extraAecExposureData, 0, sizeof(MiExtraAECExposureData) * MAX_HDR_INPUT_NUM);
    memset(&m_hdrSnapshotInfo, 0, sizeof(SnapshotReqInfo));
    memset(&m_pHDRInputChecker, 0, sizeof(HDRINPUTCHECKER));
    memset(&hdrMetaData, 0, sizeof(HDRMetaData));

    m_pHDRInputChecker.needCheckInputInfo = false;
    m_pHDRInputChecker.isFrontCamera = mFrontCamera;
    m_pHDRInputChecker.inputNum = inputNum;
    m_hdrSnapshotInfo.hdrSnapshotType = (int32_t)(HDRSnapshotType::NoneHDR);

    void *pHDRData = NULL;
    int8_t cameraId = 0;
    camera_metadata_t *staticMetadata =
        StaticMetadataWraper::getInstance()->getStaticMetadata(cameraId);
    VendorMetadataParser::getVTagValue(
        staticMetadata, "com.xiaomi.camera.supportedfeatures.inputInfoChecker", &pHDRData);
    if (NULL != pHDRData) {
        int checkerAlgo =
            *(static_cast<int *>(pHDRData)) & (static_cast<int>(INFOCHECKERLIST::HDR));
        m_pHDRInputChecker.needCheckInputInfo = checkerAlgo > 0 ? true : false;
        MLOGD(Mia2LogGroupPlugin, "AlgoUpHDR isNeedCheckInputInfo = %d",
              m_pHDRInputChecker.needCheckInputInfo);
    }

    pHDRData = NULL;
    const char *phdrInfoTag = "xiaomi.ai.asd.SnapshotReqInfo";
    VendorMetadataParser::getVTagValue(logicalMetaData, phdrInfoTag, &pHDRData);
    if (NULL != pHDRData) {
        memcpy(&m_hdrSnapshotInfo, pHDRData, sizeof(SnapshotReqInfo));
    }

    pHDRData = NULL;
    VendorMetadataParser::getVTagValue(logicalMetaData, "xiaomi.ai.asd.miaiHDRMetaData", &pHDRData);
    if (NULL != pHDRData) {
        AiAsdHDRMetaData aiAsdHDRMetaData = *static_cast<AiAsdHDRMetaData *>(pHDRData);
        memcpy(&hdrMetaData, &aiAsdHDRMetaData.miaiHDRMetaData, sizeof(HDRMetaData));
        if (hdrMetaData.config.camera_id == SELFIE_CAMERA1) {
            if (1 == hdrMetaData.config.orientation || 3 == hdrMetaData.config.orientation) {
                hdrMetaData.config.orientation = (hdrMetaData.config.orientation + 2) % 4;
            }
        }
    } else {
        switch (mFrameworkCameraId) {
        case CAM_COMBMODE_FRONT:
            hdrMetaData.config.camera_id = SELFIE_CAMERA1;
            break;
        case CAM_COMBMODE_REAR_WIDE:
        case CAM_COMBMODE_REAR_TELE:
            hdrMetaData.config.camera_id = REAR_CAMERA1;
            break;
        case CAM_COMBMODE_REAR_ULTRA:
            hdrMetaData.config.camera_id = REAR_EXTRA_WIDE;
            break;
        case CAM_COMBMODE_REAR_MACRO:
            hdrMetaData.config.camera_id = REAR_MACRO;
            break;
        default:
            hdrMetaData.config.camera_id = REAR_CAMERA1;
        }
        pHDRData = NULL;
        VendorMetadataParser::getTagValue(logicalMetaData, ANDROID_JPEG_ORIENTATION, &pHDRData);
        int localOrientation = 0;
        if (NULL != pHDRData) {
            int orientation = *static_cast<int32_t *>(pHDRData);
            localOrientation = (orientation) % 360;
            MLOGI(Mia2LogGroupPlugin, "getMetaData jpeg orientation %d", localOrientation);
        }
        hdrMetaData.config.orientation = localOrientation / 90;
    }
    pHDRData = NULL;
    float fZoomRatio = 1.0;
    VendorMetadataParser::getTagValue(logicalMetaData, ANDROID_CONTROL_ZOOM_RATIO, &pHDRData);
    if (NULL != pHDRData) {
        fZoomRatio = *static_cast<float *>(pHDRData);
    }
    hdrMetaData.config.zoom = fZoomRatio;
    hdrMetaData.config.appHDRSnapshotType = m_hdrSnapshotInfo.hdrSnapshotType;

    MLOGI(Mia2LogGroupPlugin, "[MIAI_RAWHDR] hdrMetaData hdrSnapShotStyle = %d.",
          hdrMetaData.config.hdrSnapShotStyle);

    for (int i = 0; i < inputNum; i++) {
        convertImageParams(inputBuffers[i], inputFrame[i]);

        pHDRData = NULL;
        camera_metadata_t *metadata = inputBuffers[i].pMetadata;
        camera_metadata_t *pPhyMetadata = inputBuffers[i].pPhyCamMetadata;

        if (mSdkMode) {
            pPhyMetadata = inputBuffers[i].pPhyCamMetadata ? inputBuffers[i].pPhyCamMetadata
                                                           : inputBuffers[i].pMetadata;
            pPhyMetadata = pPhyMetadata ? pPhyMetadata : inputBuffers[i].pMetadata;
        }

        VendorMetadataParser::getTagValue(metadata, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                          &pHDRData);
        if (NULL != pHDRData) {
            int32_t perFrameEv = *static_cast<int32_t *>(pHDRData);
            bracketEv[i] = perFrameEv;
            MLOGD(Mia2LogGroupPlugin, "Get bracketEv[%d] = %d", i, perFrameEv);
        } else {
            bracketEv[i] = 404;
            MLOGI(Mia2LogGroupPlugin, "Failed to get frame %d -> #%d's ev!",
                  processRequestInfo->frameNum, i);
        }

        getBlackLevel(i, rawInfo, pPhyMetadata);

        if (iIsDumpData) {
            char inputbuf[128];
            snprintf(inputbuf, sizeof(inputbuf), "miaihdr_input_%d_%dx%d_ev[%d]", i,
                     inputFrame[i].width, inputFrame[i].height, bracketEv[i]);
            PluginUtils::dumpToFile(inputbuf, &inputFrame[i]);
        }
        pHDRData = NULL;
        VendorMetadataParser::getVTagValue(
            metadata, "org.quic.camera2.statsconfigs.AECFrameControl", &pHDRData);
        if (NULL != pHDRData) {
            AECFrameControl *pAECFrameControl = NULL;

            pAECFrameControl = static_cast<AECFrameControl *>(pHDRData);
            mRawHdr->aecExposureData[i].linearGain =
                pAECFrameControl->exposureInfo[ExposureIndexSafe].linearGain;
            mRawHdr->aecExposureData[i].sensitivity =
                pAECFrameControl->exposureInfo[ExposureIndexSafe].sensitivity;
            mRawHdr->aecExposureData[i].exposureTime =
                (unsigned int)pAECFrameControl->exposureInfo[ExposureIndexSafe].exposureTime;
            unsigned int expTime = mRawHdr->aecExposureData[i].exposureTime;

            mRawHdr->extraAecExposureData[i].luxindex = pAECFrameControl->luxIndex;
            mRawHdr->extraAecExposureData[i].real_drc_gain =
                pAECFrameControl->inSensorHDR3ExpTriggerOutput.realDRCGain;

            MLOGD(Mia2LogGroupPlugin,
                  "pAECFrameControl total gain -> %f, sensitivity -> %f, exp time -> %u",
                  mRawHdr->aecExposureData[i].linearGain, mRawHdr->aecExposureData[i].sensitivity,
                  expTime);
        }
        pHDRData = NULL;
        VendorMetadataParser::getVTagValue(
            metadata, "org.quic.camera2.statsconfigs.AWBFrameControl", &pHDRData);
        if (NULL != pHDRData) {
            AWBFrameControl *pAWBFrameControl = NULL;
            pAWBFrameControl = static_cast<AWBFrameControl *>(pHDRData);
            mRawHdr->awbGainParams[i].rGain = pAWBFrameControl->AWBGains.rGain;
            mRawHdr->awbGainParams[i].gGain = pAWBFrameControl->AWBGains.gGain;
            mRawHdr->awbGainParams[i].bGain = pAWBFrameControl->AWBGains.bGain;
            mRawHdr->awbGainParams[i].colorTemperature = pAWBFrameControl->colorTemperature;
            MLOGD(Mia2LogGroupPlugin,
                  "pAWBFrameControl rGain -> %f, gGain -> %f, bGain -> %f, colorTemperature-> %d",
                  mRawHdr->awbGainParams[i].rGain, mRawHdr->awbGainParams[i].gGain,
                  mRawHdr->awbGainParams[i].bGain, mRawHdr->awbGainParams[i].colorTemperature);
        }
        pHDRData = NULL;
        VendorMetadataParser::getVTagValue(metadata, "org.quic.camera2.tuning.mode.TuningMode",
                                           &pHDRData);
        if (NULL != pHDRData) {
            ChiTuningModeParameter *pTuningModeParameter = NULL;
            pTuningModeParameter = static_cast<ChiTuningModeParameter *>(pHDRData);
        }

        // get crop region, algo need use ev0 metadata to do Iealraw2YUV
        // now, crop region use third frame crop region, so need to confirm
        if (MIRAWHDR_UPDATEREQINFO == (i - 1)) {
            getInputZoomRect(pPhyMetadata, &inputFrame[0]);
            MLOGI(Mia2LogGroupPlugin, "%s frame %d, get crop region %p %p %p %p", LOG_TAG, i,
                  outputBuffers[0].pMetadata, inputBuffers[i].pMetadata,
                  outputBuffers[0].pPhyCamMetadata, inputBuffers[i].pPhyCamMetadata);
        }
    }

    if (mEV[0] < 0 && (outputBuffers[0].pMetadata != NULL && inputBuffers[2].pMetadata != NULL) &&
        (outputBuffers[0].pPhyCamMetadata != NULL && inputBuffers[2].pPhyCamMetadata != NULL)) {
        VendorMetadataParser::mergeMetadata(outputBuffers[0].pMetadata, inputBuffers[2].pMetadata);
        VendorMetadataParser::mergeMetadata(outputBuffers[0].pPhyCamMetadata,
                                            inputBuffers[2].pPhyCamMetadata);
    }

    convertImageParams(outputBuffers[0], outputFrame);

    getRawInfo(rawInfo, inputNum, inputFrame[0], processRequestInfo);
    std::string exif_info;
    int ret = processBuffer(inputFrame, inputNum, &outputFrame, logicalMetaData, &exif_info,
                            rawInfo, inputBuffers);
    if (ret != PROCSUCCESS) {
        resultInfo = PROCFAILED;
    }

    updateMetaData(outputMetaData, outPhyMetaData, rawInfo, &inputFrame[0]);

    if (mNodeInterface.pSetResultMetadata != NULL) {
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, processRequestInfo->frameNum,
                                          processRequestInfo->timeStamp, exif_info);
    }

    if (iIsDumpData) {
        char outputbuf[128];
        snprintf(outputbuf, sizeof(outputbuf), "mirawhdr_output_%dx%d", outputFrame.width,
                 outputFrame.height);
        PluginUtils::dumpToFile(outputbuf, &outputFrame);
    }

    return resultInfo;
}

ProcessRetStatus MiAiRAWHdrPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int MiAiRAWHdrPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    if (NULL == flushRequestInfo) {
        MLOGE(Mia2LogGroupPlugin, "Invaild argument: flushRequestInfo = %p", flushRequestInfo);
    } else {
        MLOGD(Mia2LogGroupPlugin, "Flush request Id %l64u from node", flushRequestInfo->frameNum);
    }
    mRawHdr->setFlushStatus(true);
    return 0;
}

void MiAiRAWHdrPlugin::destroy()
{
    if (mRawHdr) {
        mRawHdr->destroy();
        delete mRawHdr;
        mRawHdr = NULL;
    }
    MLOGI(Mia2LogGroupPlugin, "destory");
}

void MiAiRAWHdrPlugin::getAEInfo(camera_metadata_t *metaData, ARC_LLHDR_AEINFO *aeInfo,
                                 int *camMode)
{
    void *data = NULL;
    const char *bpsGain = "com.qti.sensorbps.gain";
    VendorMetadataParser::getVTagValue(metaData, bpsGain, &data);
    if (NULL != data) {
        aeInfo->fSensorGain = *static_cast<float *>(data);
    } else {
        aeInfo->fSensorGain = 1.0f;
    }

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

    data = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_LENS_FACING, &data);
    if (NULL != data) {
        uint8_t facing = *((uint8_t *)data);
        if (facing == 0)
            *camMode = ARC_MODE_FRONT_CAMERA;
        else
            *camMode = ARC_MODE_REAR_CAMERA;
    } else if (mFrontCamera) {
        *camMode = ARC_MODE_FRONT_CAMERA;
    } else {
        *camMode = ARC_MODE_REAR_CAMERA;
    }
    MLOGI(Mia2LogGroupPlugin, "[%s] get camera_mode %d", LOG_TAG, *camMode);

    aeInfo->fADRCGainMin = ArcHDRCommonAEParams[0].adrc_min;
    aeInfo->fADRCGainMax = ArcHDRCommonAEParams[0].adrc_max;

    data = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_SENSOR_SENSITIVITY, &data);
    if (NULL != data) {
        aeInfo->iso = *static_cast<int32_t *>(data);
    } else {
        aeInfo->iso = 100;
    }
    MLOGI(Mia2LogGroupPlugin, "[%s] get iso %d", LOG_TAG, aeInfo->iso);

    data = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_LENS_APERTURE, &data);
    if (NULL != data) {
        aeInfo->apture = *static_cast<float *>(data);
    } else {
        aeInfo->apture = 88;
    }

    MLOGI(Mia2LogGroupPlugin,
          "[%s]: fLuxIndex: %.2f fISPGain=%.2f fADRCGain=%.2f apture=%.2f iso=%d exposure=%.2f",
          LOG_TAG, aeInfo->fLuxIndex, aeInfo->fISPGain, aeInfo->fADRCGain, aeInfo->apture,
          aeInfo->iso, aeInfo->exposure);
}

void MiAiRAWHdrPlugin::generateExifInfo(MIAI_HDR_RAWINFO &rawInfo, std::string *exif_info,
                                        HDRMetaData *hdrMetaData, camera_metadata_t *metaData,
                                        std::vector<ImageParams> &inputBufferHandle)
{
    char img_crop_info[128] = {0};
    snprintf(img_crop_info, sizeof(img_crop_info), "rawhdr:{[l,t,w,h]=[%d %d %d %d]} ",
             hdrMetaData->config.cropSize[0], hdrMetaData->config.cropSize[1],
             hdrMetaData->config.cropSize[2], hdrMetaData->config.cropSize[3]);
    exif_info->append(img_crop_info);

    int32_t inputNum = rawInfo.nframes;
    if (6 == inputNum) {
        std::vector<int32_t> af(inputNum);
        char lens_target_position[128] = {0};
        for (int i = 0; i < inputNum; i++) {
            void *data = NULL;
            VendorMetadataParser::getVTagValue(inputBufferHandle[i].pMetadata,
                                               "xiaomi.exifInfo.lenstargetposition", &data);
            if (NULL != data) {
                int32_t lensTargetPosition = *static_cast<int32_t *>(data);
                af[i] = lensTargetPosition;
            } else {
                af[i] = -1; // consider -1 as wrong number
                MLOGW(Mia2LogGroupPlugin, "inputbuffer[%d] Failed to get lensTargetPosition!", i);
            }
        }
        snprintf(lens_target_position, sizeof(lens_target_position), "af:%d, %d, %d, %d, %d, %d ",
                 af[0], af[1], af[2], af[3], af[4], af[5]);
        exif_info->append(lens_target_position);
        MLOGI(Mia2LogGroupPlugin, "lenstargetposition value %s ", lens_target_position);
    }

#if (0)
    char mLiteEnableBuffer[32] = {0};
    snprintf(mLiteEnableBuffer, sizeof(mLiteEnableBuffer), " fastshot: %d", mLiteEnable);
    exif_info.append(mLiteEnableBuffer);
#endif

    exif_info->append(rawInfo.hdr_raw_exif_info);
}

int MiAiRAWHdrPlugin::processBuffer(struct MiImageBuffer *input, int inputNum,
                                    struct MiImageBuffer *output, camera_metadata_t *metaData,
                                    std::string *exif_info, MIAI_HDR_RAWINFO &rawInfo,
                                    std::vector<ImageParams> &inputBufferHandle)
{
    int result = 0;
    if (metaData == NULL) {
        MLOGE(Mia2LogGroupPlugin, "error metaData is null");
        return -1;
    }
    MLOGD(Mia2LogGroupPlugin, "processBuffer inputNum: %d, mFrameworkCameraId: %d", inputNum,
          mFrameworkCameraId);

    ARC_LLHDR_AEINFO aeInfo = {0};
    int camMode = 0;

    getAEInfo(metaData, &aeInfo, &camMode);

    if (mRawHdr) {
        mRawHdr->process(input, output, inputNum, mEV, aeInfo, camMode, hdrMetaData,
                         mRawHdr->aecExposureData, mRawHdr->awbGainParams, rawInfo);
    } else {
        MLOGD(Mia2LogGroupPlugin, "error mRawHdr is null");
        PluginUtils::miCopyBuffer(output, input);
    }
    MLOGD(Mia2LogGroupPlugin, "processBuffer exif: %s", rawInfo.hdr_raw_exif_info);

    generateExifInfo(rawInfo, exif_info, &hdrMetaData, metaData, inputBufferHandle);

    return result;
}

// fix me, need to get rect
void MiAiRAWHdrPlugin::getInputZoomRect(camera_metadata_t *metaData, struct MiImageBuffer *input)
{
    void *pData = NULL;
    ChiRect cropWindow = {0, 0, input->width, input->height};

    const char *cropInfo = "xiaomi.snapshot.cropRegion";
    VendorMetadataParser::getVTagValue(metaData, cropInfo, &pData);
    if (NULL != pData) {
        cropWindow = *(static_cast<ChiRect *>(pData));
        MLOGI(Mia2LogGroupPlugin, "[%s] get xiaomi.snapshot.cropRegion l,t,w,h=[%d %d %d %d]",
              LOG_TAG, cropWindow.left, cropWindow.top, cropWindow.width, cropWindow.height);
    } else if (NULL == pData) {
        MLOGI(Mia2LogGroupPlugin, "[%s] get SAT crop failed, try ANDROID_SCALER_CROP_REGION",
              LOG_TAG);
        VendorMetadataParser::getTagValue(metaData, ANDROID_SCALER_CROP_REGION, &pData);
        if (NULL != pData) {
            cropWindow = *(static_cast<ChiRect *>(pData));
            MLOGI(Mia2LogGroupPlugin, "[%s] get xiaomi.snapshot.cropRegion l,t,w,h=[%d %d %d %d]",
                  LOG_TAG, cropWindow.left, cropWindow.top, cropWindow.width, cropWindow.height);
        } else {
            MLOGI(Mia2LogGroupPlugin, "[%s] get xiaomi.snapshot.cropRegion fail!", LOG_TAG);
        }
    }

    rectRawHDRCropRegion.left = cropWindow.left;
    rectRawHDRCropRegion.top = cropWindow.top;
    rectRawHDRCropRegion.right = cropWindow.left + cropWindow.width;
    rectRawHDRCropRegion.bottom = cropWindow.top + cropWindow.height;

    hdrMetaData.config.cropSize[0] = cropWindow.left;
    hdrMetaData.config.cropSize[1] = cropWindow.top;
    hdrMetaData.config.cropSize[2] = cropWindow.width;
    hdrMetaData.config.cropSize[3] = cropWindow.height;

    MLOGI(Mia2LogGroupPlugin, " [%s] input cropregions(l,t,w,h)=(%d,%d,%d,%d)", LOG_TAG,
          rectRawHDRCropRegion.left, rectRawHDRCropRegion.top, rectRawHDRCropRegion.right,
          rectRawHDRCropRegion.bottom);
}

void MiAiRAWHdrPlugin::updateMetaData(camera_metadata_t *metaData, camera_metadata_t *phyMetadat,
                                      MIAI_HDR_RAWINFO &rawInfo, struct MiImageBuffer *input)
{
    ChiRect scalerCrop = {0, 0, input->width, input->height};
    scalerCrop.left = rectRawHDRCropRegion.left;
    scalerCrop.top = rectRawHDRCropRegion.top;
    scalerCrop.width = rectRawHDRCropRegion.right - rectRawHDRCropRegion.left;
    scalerCrop.height = rectRawHDRCropRegion.bottom - rectRawHDRCropRegion.top;

    hdrMetaData.config.cropSize[0] = scalerCrop.left;
    hdrMetaData.config.cropSize[1] = scalerCrop.top;
    hdrMetaData.config.cropSize[2] = scalerCrop.width;
    hdrMetaData.config.cropSize[3] = scalerCrop.height;

    VendorMetadataParser::setVTagValue(metaData, "xiaomi.snapshot.cropRegion", &scalerCrop, 4);

    MLOGI(Mia2LogGroupPlugin, "[%s] update xiaomi.snapshot.cropRegion [l,t,w,h]=[%d %d %d %d]",
          LOG_TAG, scalerCrop.left, scalerCrop.top, scalerCrop.width, scalerCrop.height);

    void *pHDRData = NULL;
    const char *pHDRInfoTag = "xiaomi.ai.asd.SnapshotReqInfo";
    size_t hdrTagCount = 0;
    VendorMetadataParser::getVTagValueCount(phyMetadat, pHDRInfoTag, &pHDRData, &hdrTagCount);
    if (NULL != pHDRData) {
        SnapshotReqInfo hdrSnapshotReqInfo;
        memcpy(&hdrSnapshotReqInfo, pHDRData, sizeof(SnapshotReqInfo));
        hdrSnapshotReqInfo.denoiseMode = static_cast<INT32>(rawInfo.denoise_switch_gain);

        if (!VendorMetadataParser::setVTagValue(phyMetadat, pHDRInfoTag, &hdrSnapshotReqInfo,
                                                hdrTagCount)) {
            MLOGI(Mia2LogGroupPlugin, "[%s] denoise mode is %d", LOG_TAG,
                  hdrSnapshotReqInfo.denoiseMode);
        } else {
            MLOGI(Mia2LogGroupPlugin, "[%s] denoise mode set failed", LOG_TAG);
        }
    }
}

void MiAiRAWHdrPlugin::setEv(int inputnum, std::vector<ImageParams> &inputBufferHandle)
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
            MLOGI(Mia2LogGroupPlugin, "AIHDR ev[%d] value = %d", numIndex, mEV[numIndex]);
        } else {
            mEV[numIndex] = 404;
            MLOGE(Mia2LogGroupPlugin, "Failed to get inputnumber  #%d's ev!", numIndex);
        }
    }
}

void MiAiRAWHdrPlugin::isSdkAlgoUp(camera_metadata_t *metadata)
{
    void *hdrMode = NULL;
    VendorMetadataParser::getVTagValue(metadata, "com.xiaomi.sessionparams.operation", &hdrMode);
    if (hdrMode != NULL) {
        int32_t result = *static_cast<int32_t *>(hdrMode);
        mSdkMode = (result == 0xFF0A) ? true : false;
    }
}
ChiRect MiAiRAWHdrPlugin::GetCropRegion(camera_metadata_t *metaData, int inputWidth,
                                        int inputHeight)
{
    ChiRect cropRegion = {};
    ChiRect outputRect;
    void *pData = NULL;
    int cameraId = 0;

    cropRegion = {0, 0, static_cast<uint32_t>(inputWidth), static_cast<uint32_t>(inputHeight)};
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.snapshot.cropRegion", &pData);
    if (NULL != pData) {
        cropRegion = *static_cast<ChiRect *>(pData);
    }

    // get RefCropSize
    pData = NULL;
    int refWidth = inputWidth, refHeight = inputWidth;
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.snapshot.fwkCameraId", &pData);
    if (NULL != pData) {
        cameraId = *static_cast<int32_t *>(pData);

        pData = NULL;
        camera_metadata_t *staticMetadata =
            StaticMetadataWraper::getInstance()->getStaticMetadata(cameraId);
        VendorMetadataParser::getTagValue(staticMetadata, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
                                          &pData);
        if (NULL != pData) {
            ChiRect &activeArray = *static_cast<ChiRect *>(pData);
            refWidth = activeArray.width;
            refHeight = activeArray.height;
        }
    }

    MLOGI(Mia2LogGroupPlugin, "original crop rect [%d %d %d %d] %d %d", cropRegion.left,
          cropRegion.top, cropRegion.width, cropRegion.height, refWidth, refHeight);
    int rectLeft = 0, rectTop = 0, rectWidth = 0, rectHeight = 0;
    int rectWidthOld = 0, rectHeightOld = 0;

    if (cropRegion.width == refWidth || cropRegion.height == refHeight) {
        rectLeft = 0;
        rectTop = 0;
        rectWidth = inputWidth;
        rectHeight = inputHeight;
    } else {
        rectLeft = cropRegion.left - (refWidth - inputWidth) / 2;
        rectLeft = rectLeft < 0 ? 0 : rectLeft;
        rectTop = cropRegion.top - (refHeight - inputHeight) / 2;
        rectTop = rectTop < 0 ? 0 : rectTop;
        rectWidthOld = cropRegion.width > inputWidth ? inputWidth : cropRegion.width;
        rectHeightOld = cropRegion.height > inputHeight ? inputHeight : cropRegion.height;

        rectWidth = cropRegion.width * inputWidth / refWidth;
        rectHeight = cropRegion.height * inputHeight / refHeight;
        rectLeft += (rectWidthOld - rectWidth) / 2;
        rectTop += (rectHeightOld - rectHeight) / 2;
    }

    cropRegion = {static_cast<UINT32>(rectLeft), static_cast<UINT32>(rectTop),
                  static_cast<UINT32>(rectWidth), static_cast<UINT32>(rectHeight)};

    MLOGI(Mia2LogGroupPlugin, "cam:%d,crop rect:[%d,%d,%d,%d], in=%dx%d,ref=%dx%d", cameraId,
          cropRegion.left, cropRegion.top, cropRegion.width, cropRegion.height, inputWidth,
          inputHeight, refWidth, refHeight);

    return cropRegion;
}

void MiAiRAWHdrPlugin::getBlackLevel(UINT32 inputNum, MIAI_HDR_RAWINFO &rawInfo,
                                     camera_metadata_t *pPhyMetadata)
{
    void *pHDRData = NULL;
    float blackLevel[4] = {0.0f};
    VendorMetadataParser::getTagValue(pPhyMetadata, ANDROID_SENSOR_DYNAMIC_BLACK_LEVEL, &pHDRData);

    if (pHDRData == nullptr) {
        MLOGE(Mia2LogGroupPlugin, "failed load black level, force");
    } else {
        memcpy(blackLevel, static_cast<float *>(pHDRData), 4 * sizeof(float));
    }

    switch (rawInfo.raw_type) {
    case RGGB:
        rawInfo.black_level[inputNum][0] = blackLevel[0];
        rawInfo.black_level[inputNum][1] = blackLevel[2];
        rawInfo.black_level[inputNum][2] = blackLevel[1];
        rawInfo.black_level[inputNum][3] = blackLevel[3];
        break;
    case GRBG:
        rawInfo.black_level[inputNum][0] = blackLevel[2];
        rawInfo.black_level[inputNum][1] = blackLevel[0];
        rawInfo.black_level[inputNum][2] = blackLevel[3];
        rawInfo.black_level[inputNum][3] = blackLevel[1];
        break;
    case GBRG:
        rawInfo.black_level[inputNum][0] = blackLevel[1];
        rawInfo.black_level[inputNum][1] = blackLevel[3];
        rawInfo.black_level[inputNum][2] = blackLevel[0];
        rawInfo.black_level[inputNum][3] = blackLevel[2];
        break;
    case BGGR:
        rawInfo.black_level[inputNum][0] = blackLevel[3];
        rawInfo.black_level[inputNum][1] = blackLevel[1];
        rawInfo.black_level[inputNum][2] = blackLevel[2];
        rawInfo.black_level[inputNum][3] = blackLevel[0];
        break;
    }
    for (int i = 0; i < 4; i++) {
        if ((rawInfo.black_level[inputNum][i] < MinBLSValue) ||
            (rawInfo.black_level[inputNum][i] > MaxBLSValue)) {
            rawInfo.black_level[inputNum][i] = defaultBLSValue;
        }
    }
    MLOGD(Mia2LogGroupPlugin, "[MIAI_RAWHDR]black level %.3f, %.3f, %.3f, %.3f",
          rawInfo.black_level[inputNum][0], rawInfo.black_level[inputNum][1],
          rawInfo.black_level[inputNum][2], rawInfo.black_level[inputNum][3]);
}
} // namespace mialgo2
