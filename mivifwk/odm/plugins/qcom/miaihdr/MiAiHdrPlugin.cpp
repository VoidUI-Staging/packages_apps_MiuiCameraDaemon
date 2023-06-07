#include "MiAiHdrPlugin.h"

#include "camlog.h"

#undef LOG_TAG
#define LOG_TAG "MIA_N_HDR"

#define FRAME_MAX_INPUT_NUMBER 5
static const uint8_t MISDK_HDR_ALGOUP = 2;
static const int32_t sIsDumpData =
    property_get_int32("persist.vendor.camera.capture.burst.hdrdump", 0);

static const int32_t debugFaceInfo = property_get_int32("persist.vendor.camera.debugFaceInfo", 0);

//////////////////////////////////////////////
static const int32_t MIHDR_UnderEvExpIndex = 1; // ev- index
//////////////////////////////////////////////
namespace mialgo2 {

MiAiHdrPlugin::~MiAiHdrPlugin() {}

int MiAiHdrPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mFrameworkCameraId = createInfo->frameworkCameraId;
    if (CAM_COMBMODE_FRONT == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_BOKEH == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_AUX == mFrameworkCameraId) {
        mFrontCamera = true;
    } else {
        mFrontCamera = false;
    }
    mHdr = NULL;
    if (!mHdr) {
        mHdr = new MiAIHDR();
        mHdr->init();
    }
    mNodeInterface = nodeInterface;

    return MIA_RETURN_OK;
}

bool MiAiHdrPlugin::isEnabled(MiaParams settings)
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

ProcessRetStatus MiAiHdrPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    uint8_t isSrHDR = 0;
    HDRMetaData hdrMetaData = {};

    int32_t bracketEv[FRAME_MAX_INPUT_NUMBER];
    MLOGI(Mia2LogGroupPlugin, "%s:%d", __func__, __LINE__);

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
    int32_t iIsDumpData = sIsDumpData;

    camera_metadata_t *logicalMetaData = inputBuffers[0].pMetadata;
    camera_metadata_t *physicalMetaData = inputBuffers[0].pPhyCamMetadata;
    memset(mEV, 0, sizeof(mEV));
    setEv(inputNum, inputBuffers);

    struct mialgo2::MiImageBuffer inputFrame[FRAME_MAX_INPUT_NUMBER], outputFrame;

    memset(mHdr->aecExposureData, 0, sizeof(MiAECExposureData) * MAX_HDR_INPUT_NUM);
    memset(mHdr->awbGainParams, 0, sizeof(MiAWBGainParams) * MAX_HDR_INPUT_NUM);
    memset(mHdr->extraAecExposureData, 0, sizeof(MiExtraAECExposureData) * MAX_HDR_INPUT_NUM);
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

    pHDRData = NULL;
    VendorMetadataParser::getVTagValue(logicalMetaData, "xiaomi.hdr.sr.enabled", &pHDRData);
    if (NULL != pHDRData) {
        isSrHDR = *static_cast<UINT8 *>(pHDRData);
        if (1 == isSrHDR) {
            hdrMetaData.config.ev0_preprocess = PREPROCESS_SR;
            m_hdrSnapshotInfo.hdrSnapshotType = (int32_t)(HDRSnapshotType::AlgoUpSRHDR);
        }
    }

    MLOGI(Mia2LogGroupPlugin,
          "Get HDR hdrMetaData:cameraID %d, after orientation = %d type %d zoom %f processEV0 %d",
          hdrMetaData.config.camera_id, hdrMetaData.config.orientation, hdrMetaData.config.hdr_type,
          hdrMetaData.config.zoom, hdrMetaData.config.ev0_preprocess);

    hdrMetaData.config.appHDRSnapshotType = m_hdrSnapshotInfo.hdrSnapshotType;

    for (int i = 0; i < inputNum; i++) {
        convertImageParams(inputBuffers[i], inputFrame[i]);

        pHDRData = NULL;
        camera_metadata_t *metadata = inputBuffers[i].pMetadata;
        camera_metadata_t *pPhyMetadata = inputBuffers[i].pPhyCamMetadata;
        int hdrSnapshotType = m_hdrSnapshotInfo.hdrSnapshotType;
        if (1 == m_pHDRInputChecker.isFrontCamera &&
            HDRSnapshotType::AlgoUpFinalZSLHDR == hdrSnapshotType) {
            hdrSnapshotType = HDRSnapshotType::AlgoUpFrontFinalZSLHDR;
        }

        VendorMetadataParser::getTagValue(metadata, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                          &pHDRData);
        if (NULL != pHDRData) {
            int32_t perFrameEv = *static_cast<int32_t *>(pHDRData);
            bracketEv[i] = perFrameEv;
            MLOGD(Mia2LogGroupPlugin, "Get bracketEv[%d] = %d", i, perFrameEv);
        } else {
            bracketEv[i] = 404;
            MLOGE(Mia2LogGroupPlugin, "Failed to get frame %d -> #%d's ev!",
                  processRequestInfo->frameNum, i);
        }
        m_pHDRInputChecker.EVValue[i] = bracketEv[i];
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
            mHdr->aecExposureData[i].linearGain =
                pAECFrameControl->exposureInfo[ExposureIndexSafe].linearGain;
            mHdr->aecExposureData[i].sensitivity =
                pAECFrameControl->exposureInfo[ExposureIndexSafe].sensitivity;
            mHdr->aecExposureData[i].exposureTime =
                (unsigned int)pAECFrameControl->exposureInfo[ExposureIndexSafe].exposureTime;
            unsigned int expTime = mHdr->aecExposureData[i].exposureTime;

            mHdr->extraAecExposureData[i].luxindex = pAECFrameControl->luxIndex;
            mHdr->extraAecExposureData[i].real_drc_gain =
                pAECFrameControl->inSensorHDR3ExpTriggerOutput.realDRCGain;
            m_pHDRInputChecker.aecFrameControl[i] = *pAECFrameControl;

            MLOGD(Mia2LogGroupPlugin,
                  "pAECFrameControl total gain -> %f, sensitivity -> %f, exp time -> %u",
                  mHdr->aecExposureData[i].linearGain, mHdr->aecExposureData[i].sensitivity,
                  expTime);
        }
        pHDRData = NULL;
        VendorMetadataParser::getVTagValue(
            metadata, "org.quic.camera2.statsconfigs.AWBFrameControl", &pHDRData);
        if (NULL != pHDRData) {
            AWBFrameControl *pAWBFrameControl = NULL;
            pAWBFrameControl = static_cast<AWBFrameControl *>(pHDRData);
            mHdr->awbGainParams[i].rGain = pAWBFrameControl->AWBGains.rGain;
            mHdr->awbGainParams[i].gGain = pAWBFrameControl->AWBGains.gGain;
            mHdr->awbGainParams[i].bGain = pAWBFrameControl->AWBGains.bGain;
            mHdr->awbGainParams[i].colorTemperature = pAWBFrameControl->colorTemperature;
            MLOGD(Mia2LogGroupPlugin,
                  "pAWBFrameControl rGain -> %f, gGain -> %f, bGain -> %f, colorTemperature-> %d",
                  mHdr->awbGainParams[i].rGain, mHdr->awbGainParams[i].gGain,
                  mHdr->awbGainParams[i].bGain, mHdr->awbGainParams[i].colorTemperature);
        }
        pHDRData = NULL;
        VendorMetadataParser::getVTagValue(metadata, "org.quic.camera2.tuning.mode.TuningMode",
                                           &pHDRData);
        if (NULL != pHDRData) {
            ChiTuningModeParameter *pTuningModeParameter = NULL;
            pTuningModeParameter = static_cast<ChiTuningModeParameter *>(pHDRData);
            m_pHDRInputChecker.tuningParam[i] = *pTuningModeParameter;
        }

        // get crop region only for 2'st frame, such as ev(0 - -/0 - +)
        if (MIHDR_UnderEvExpIndex == i) {
            pHDRData = NULL;
            CHIRECT cropRegion = {0};
            // #ifdef INGRES_CAM
            //          cropRegion = GetCropRegion(metadata, inputFrame[i].width,
            //          inputFrame[i].height);

            cropRegion = GetCropRegion(metadata, inputFrame[i].width, inputFrame[i].height);

            hdrMetaData.config.cropSize[0] = cropRegion.left;
            hdrMetaData.config.cropSize[1] = cropRegion.top;
            hdrMetaData.config.cropSize[2] = cropRegion.width;
            hdrMetaData.config.cropSize[3] = cropRegion.height;

            if (1 == isSrHDR && 0 != m_hdrSnapshotInfo.srCropRegion.width &&
                0 != m_hdrSnapshotInfo.srCropRegion.height) {
                hdrMetaData.config.cropSize[0] = m_hdrSnapshotInfo.srCropRegion.left;
                hdrMetaData.config.cropSize[1] = m_hdrSnapshotInfo.srCropRegion.top;
                hdrMetaData.config.cropSize[2] = m_hdrSnapshotInfo.srCropRegion.width;
                hdrMetaData.config.cropSize[3] = m_hdrSnapshotInfo.srCropRegion.height;
                MLOGD(Mia2LogGroupPlugin, "use sr crop region for hdr algo size [%d, %d, %d,%d]",
                      hdrMetaData.config.cropSize[0], hdrMetaData.config.cropSize[1],
                      hdrMetaData.config.cropSize[2], hdrMetaData.config.cropSize[3]);
            }
        }
    }

    MLOGD(Mia2LogGroupPlugin, "hdrMetaData.config.ui_ae = %d", hdrMetaData.config.ui_ae);

    convertImageParams(outputBuffers[0], outputFrame);

    std::string exif_info;
    int ret = processBuffer(inputFrame, inputNum, &outputFrame, logicalMetaData, physicalMetaData,
                            &exif_info, hdrMetaData);
    if (ret != PROCSUCCESS) {
        resultInfo = PROCFAILED;
    }
    MLOGI(Mia2LogGroupPlugin, "processBuffer ret: %d", ret);

    if (mNodeInterface.pSetResultMetadata != NULL) {
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, processRequestInfo->frameNum,
                                          processRequestInfo->timeStamp, exif_info);
    }

    if (iIsDumpData) {
        char outputbuf[128];
        snprintf(outputbuf, sizeof(outputbuf), "miaihdr_output_%dx%d", outputFrame.width,
                 outputFrame.height);
        PluginUtils::dumpToFile(outputbuf, &outputFrame);
    }

    return resultInfo;
}

ProcessRetStatus MiAiHdrPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int MiAiHdrPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    if (NULL == flushRequestInfo) {
        MLOGE(Mia2LogGroupPlugin, "Invaild argument: flushRequestInfo = %p", flushRequestInfo);
    } else {
        MLOGD(Mia2LogGroupPlugin, "Flush request Id %l64u from node", flushRequestInfo->frameNum);
    }
    mHdr->setFlushStatus(true);
    return 0;
}

void MiAiHdrPlugin::destroy()
{
    if (mHdr) {
        mHdr->destroy();
        delete mHdr;
        mHdr = NULL;
    }
    MLOGI(Mia2LogGroupPlugin, "destory");
}

void MiAiHdrPlugin::getAEInfo(camera_metadata_t *metaData, ARC_LLHDR_AEINFO *aeInfo, int *camMode)
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
    MLOGI(Mia2LogGroupPlugin, "MiAiHdrPlugin get camera_mode %d", *camMode);

    aeInfo->fADRCGainMin = ArcHDRCommonAEParams[0].adrc_min;
    aeInfo->fADRCGainMax = ArcHDRCommonAEParams[0].adrc_max;

    data = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_SENSOR_SENSITIVITY, &data);
    if (NULL != data) {
        aeInfo->iso = *static_cast<int32_t *>(data);
    } else {
        aeInfo->iso = 100;
    }
    MLOGI(Mia2LogGroupPlugin, "MiAiHdrPlugin get iso %d", aeInfo->iso);

    data = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_LENS_APERTURE, &data);
    if (NULL != data) {
        aeInfo->apture = *static_cast<float *>(data);
    } else {
        aeInfo->apture = 88;
    }

    MLOGI(Mia2LogGroupPlugin, "[MiAiHdrPlugin]: fLuxIndex: %.2f fISPGain=%.2f fADRCGain=%.2f ",
          aeInfo->fLuxIndex, aeInfo->fISPGain, aeInfo->fADRCGain);
    MLOGI(Mia2LogGroupPlugin, "[MiAiHdrPlugin]: apture=%.2f iso=%d exposure=%.2f", aeInfo->apture,
          aeInfo->iso, aeInfo->exposure);
}

void MiAiHdrPlugin::generateExifInfo(int inputNum, ARC_LLHDR_AEINFO &aeInfo, std::string *exif_info,
                                     camera_metadata_t *metaData, HDRMetaData *hdrMetaData)
{
    aeInfo.algo_params.frame_id = hdrMetaData->config.frameNum;
    aeInfo.algo_params.apture = aeInfo.apture;
    aeInfo.algo_params.exposure = aeInfo.exposure;
    aeInfo.algo_params.iso = aeInfo.iso;
    aeInfo.algo_params.exp_compensation = aeInfo.exp_compensation;
    for (int i = 0; i < inputNum; i++) {
        aeInfo.algo_params.hdr_ev[i] = mEV[i];
        aeInfo.algo_params.luxindex[i] = mHdr->extraAecExposureData[i].luxindex;
        aeInfo.algo_params.real_drc_gain[i] = mHdr->extraAecExposureData[i].real_drc_gain;
        aeInfo.algo_params.exposureTime[i] = mHdr->aecExposureData[i].exposureTime;
        aeInfo.algo_params.linear_gain[i] = mHdr->aecExposureData[i].linearGain;
        aeInfo.algo_params.sensitivity[i] = mHdr->aecExposureData[i].sensitivity;
    }

    std::string ev_buf = "";
    std::string aec_buf;
    for (size_t i = 0; i < inputNum; i++) {
        char tmp_buf[8] = {0};
        char tmp_aec_buf[256] = {0};
        snprintf(tmp_buf, sizeof(tmp_buf), "%d ", aeInfo.algo_params.hdr_ev[i]);
        ev_buf.append(tmp_buf);
        snprintf(
            tmp_aec_buf, sizeof(tmp_aec_buf),
            " [EV:%d AECInfo: LUX:%f RealDrc:%.3f ExposureTime:%ld LinearGain:%f Sensitivity:%.3f "
            "SM:%d UC:%d F1:%d F2:%d SC:%d EF:%d]",
            aeInfo.algo_params.hdr_ev[i], aeInfo.algo_params.luxindex[i],
            aeInfo.algo_params.real_drc_gain[i], aeInfo.algo_params.exposureTime[i],
            aeInfo.algo_params.linear_gain[i], aeInfo.algo_params.sensitivity[i],
            m_pHDRInputChecker.tuningParam[i].TuningMode[1].subMode.value,
            m_pHDRInputChecker.tuningParam[i].TuningMode[2].subMode.usecase,
            m_pHDRInputChecker.tuningParam[i].TuningMode[3].subMode.feature1,
            m_pHDRInputChecker.tuningParam[i].TuningMode[4].subMode.feature2,
            m_pHDRInputChecker.tuningParam[i].TuningMode[5].subMode.scene,
            m_pHDRInputChecker.tuningParam[i].TuningMode[6].subMode.effect);
        aec_buf.append(tmp_aec_buf);
    }

    char tmp_aec_buf[128] = {0};
    snprintf(tmp_aec_buf, sizeof(tmp_aec_buf),
             " [ISO:%d] [Apture:%.3f] [Exposure:%.3f] [EXPCompensation:%d] ",
             aeInfo.algo_params.iso, aeInfo.algo_params.apture, aeInfo.algo_params.exposure,
             aeInfo.algo_params.exp_compensation);
    aec_buf.append(tmp_aec_buf);

    char imgSize_hdrtype_buf[128] = {0};
    snprintf(imgSize_hdrtype_buf, sizeof(imgSize_hdrtype_buf),
             "[crop size left:%d top:%d width:%d height:%d] isSRHDR:%d",
             hdrMetaData->config.cropSize[0], hdrMetaData->config.cropSize[1],
             hdrMetaData->config.cropSize[2], hdrMetaData->config.cropSize[3],
             hdrMetaData->config.ev0_preprocess);
    aec_buf.append(imgSize_hdrtype_buf);

    char buf[1024] = {0};
    snprintf(
        buf, sizeof(buf),
        " miaihdr:{num:%d cameraID:%d SnapShotType:%d zoom:%d type:%d "
        "rotation:%d width:%d height:%d "
        "discardFrame:%d ghostVal:%.3f] globalGhostVal:%.3f stillImageVal:%.3f faceNum:%d "
        "ev1x:%.3f Ymul:%.3f ev0ExpoArea:%.3f ev0FaceMean:%.3f fusionPhase2:%d usePWL1st:%d "
        "usePWL2nd:%d pWL1stLine1First:%.3f pwl1stLine1Second:%.3f pwl1stLine2First:%.3f "
        "pwl1stLine2Second:%.3f "
        "pwl2stLine1First:%.3f pwl2stLine1Second:%.3f pwl2stLine2First:%.3f pwl2stLine2Second:%.3f "
        "linearMul:%.3f linearMinus:%.3f frameID:%ld ev:%s hdrAlgoExif:%s}",
        inputNum, hdrMetaData->config.camera_id, hdrMetaData->config.appHDRSnapshotType,
        hdrMetaData->config.zoom, hdrMetaData->config.hdr_type, aeInfo.algo_params.rotate,
        aeInfo.algo_params.width, aeInfo.algo_params.height, aeInfo.algo_params.is_discard_frame,
        aeInfo.algo_params.ghost_val, aeInfo.algo_params.global_ghost_val,
        aeInfo.algo_params.still_image_val, aeInfo.algo_params.faceNum, aeInfo.algo_params.ev1x,
        aeInfo.algo_params.y_mul, aeInfo.algo_params.ev0_expo_area,
        aeInfo.algo_params.ev0_face_mean, aeInfo.algo_params.is_fusion_phase2,
        aeInfo.algo_params.is_use_PWL_1st, aeInfo.algo_params.is_use_PWL_2nd,
        aeInfo.algo_params.pwl_1st_line1_first, aeInfo.algo_params.pwl_1st_line1_second,
        aeInfo.algo_params.pwl_1st_line2_first, aeInfo.algo_params.pwl_1st_line2_second,
        aeInfo.algo_params.pwl_2nd_line1_first, aeInfo.algo_params.pwl_2nd_line1_second,
        aeInfo.algo_params.pwl_2nd_line2_first, aeInfo.algo_params.pwl_2nd_line2_second,
        aeInfo.algo_params.linear_mul, aeInfo.algo_params.linear_minus, aeInfo.algo_params.frame_id,
        ev_buf.c_str(), mHdr->m_hdrAlgoExif.c_str());
    *exif_info = buf;
    exif_info->append(aec_buf);
}

int MiAiHdrPlugin::processBuffer(struct mialgo2::MiImageBuffer *input, int inputNum,
                                 struct mialgo2::MiImageBuffer *output, camera_metadata_t *metaData,
                                 camera_metadata_t *phyMetadata, std::string *exif_info,
                                 HDRMetaData &hdrMetaData)
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
    getFaceInfo(metaData, phyMetadata, input[0].width, input[0].height, hdrMetaData);

    HDRCheckResult hdrResult = HDRCheckResultSuccess;
    if (true == m_pHDRInputChecker.needCheckInputInfo) {
        switch ((HDRSnapshotType)(m_hdrSnapshotInfo.hdrSnapshotType)) {
        case HDRSnapshotType::AlgoUpNormalHDR:
            hdrResult =
                IsValidCheckHDRInputInfo(m_pHDRInputChecker, HDRSnapshotType::AlgoUpNormalHDR);
            break;
        case HDRSnapshotType::AlgoUpFinalZSLHDR:
            hdrResult =
                IsValidCheckHDRInputInfo(m_pHDRInputChecker, HDRSnapshotType::AlgoUpFinalZSLHDR);
            break;
        case HDRSnapshotType::AlgoUpSRHDR:
            hdrResult = IsValidCheckHDRInputInfo(m_pHDRInputChecker, HDRSnapshotType::AlgoUpSRHDR);
            break;
        default:
            MLOGI(Mia2LogGroupPlugin, "hdr snapshot type is out of range.");
            break;
        }
        MLOGI(Mia2LogGroupPlugin, "[MiAiHdrPlugin]: AlgoUpHDR result:%d", hdrResult);
        if (HDRCheckResultSuccess != hdrResult) {
            char CamErrorString[256] = {0};
            sprintf(CamErrorString,
                    "[MIAI_HDR]Hdr tuning mode error, Error code: %d, "
                    "hdrSnapshotType: %d",
                    hdrResult, m_hdrSnapshotInfo.hdrSnapshotType);
            MLOGI(Mia2LogGroupPlugin, "[MiAiHdrPlugin] %s", CamErrorString);
            camlog::send_message_to_mqs(CamErrorString, false);
        }
    }

    if (mHdr) {
        mHdr->process(input, output, inputNum, mEV, aeInfo, camMode, hdrMetaData,
                      mHdr->aecExposureData, mHdr->awbGainParams);
        generateExifInfo(inputNum, aeInfo, exif_info, metaData, &hdrMetaData);
    } else {
        MLOGD(Mia2LogGroupPlugin, "error mHdr is null");
        PluginUtils::miCopyBuffer(output, input);
    }

    if (debugFaceInfo) {
        DebugFaceRoi(output, hdrMetaData);
    }
    return result;
}

void MiAiHdrPlugin::setEv(int inputnum, std::vector<ImageParams> &inputBufferHandle)
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

    MLOGI(Mia2LogGroupPlugin, "AIHDR ev value = %d, %d, %d", mEV[0], mEV[1], mEV[2]);
}

bool MiAiHdrPlugin::isSdkAlgoUp(camera_metadata_t *metadata)
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
ChiRect MiAiHdrPlugin::GetCropRegion(camera_metadata_t *metaData, int inputWidth, int inputHeight)
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
    int refWidth = inputWidth, refHeight = inputHeight;
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
        rectLeft = static_cast<int>(cropRegion.left) - (refWidth - inputWidth) / 2;
        rectLeft = rectLeft < 0 ? 0 : rectLeft;
        rectTop = static_cast<int>(cropRegion.top) - (refHeight - inputHeight) / 2;
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

void MiAiHdrPlugin::getFaceInfo(camera_metadata_t *logicalMetaData,
                                camera_metadata_t *physicalMetaData, int width, int height,
                                HDRMetaData &hdrMetaData)
{
    // get face info
    void *pData = NULL;
    FaceResult faceRect = {};
    uint32_t faceCnt = 0;
    if (physicalMetaData != NULL) {
        VendorMetadataParser::getVTagValue(physicalMetaData, "xiaomi.snapshot.faceRect", &pData);
    } else {
        VendorMetadataParser::getVTagValue(logicalMetaData, "xiaomi.snapshot.faceRect", &pData);
    }

    if (NULL != pData) {
        faceRect = *static_cast<FaceResult *>(pData);
        faceCnt = faceRect.faceNumber;
        MLOGI(Mia2LogGroupPlugin, "[MIAI_HDR] face number %d", faceCnt);
    } else {
        MLOGW(Mia2LogGroupPlugin, "[MIAI_HDR] get faceRect failed");
    }

    // get crop region
    ChiRect cropRegion = {};
    pData = NULL;
    cropRegion = {0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    VendorMetadataParser::getVTagValue(logicalMetaData, "xiaomi.snapshot.cropRegion", &pData);
    if (NULL != pData) {
        cropRegion = *static_cast<ChiRect *>(pData);
    } else {
        MLOGI(Mia2LogGroupPlugin, "[MIAI_HDR] get crop failed, try ANDROID_SCALER_CROP_REGION");
        VendorMetadataParser::getTagValue(logicalMetaData, ANDROID_SCALER_CROP_REGION, &pData);
        if (NULL != pData) {
            cropRegion = *reinterpret_cast<ChiRect *>(pData);
        } else {
            MLOGI(Mia2LogGroupPlugin, "[MIAI_HDR] get CropRegion failed");
        }
    }

    MLOGI(Mia2LogGroupPlugin, "[MIAI_HDR] cropRegion: %d, %d, %d, %d", cropRegion.left,
          cropRegion.top, cropRegion.width, cropRegion.height);

    // prepare for map
    std::vector<ChiRect> faceROIInList;
    ChiRect faceROI = {};
    uint32_t right = 0;
    uint32_t bottom = 0;
    uint32_t faceCntMax = faceCnt > MAX_FACE_NUM ? MAX_FACE_NUM : faceCnt;
    for (int i = 0; i < faceCntMax; i++) {
        faceROI = {0, 0, 0, 0};
        faceROI.left = faceRect.chiFaceInfo[i].left;
        faceROI.top = faceRect.chiFaceInfo[i].top < faceRect.chiFaceInfo[i].bottom
                          ? faceRect.chiFaceInfo[i].top
                          : faceRect.chiFaceInfo[i].bottom;
        right = faceRect.chiFaceInfo[i].right;
        bottom = faceRect.chiFaceInfo[i].top > faceRect.chiFaceInfo[i].bottom
                     ? faceRect.chiFaceInfo[i].top
                     : faceRect.chiFaceInfo[i].bottom;
        faceROI.width = right - faceROI.left;
        faceROI.height = bottom - faceROI.top;

        MLOGI(Mia2LogGroupPlugin, "[MIAI_HDR] faceROI: %d, %d, %d, %d", faceROI.left, faceROI.top,
              faceROI.width, faceROI.height);

        faceROIInList.push_back(faceROI);
    }

    ChiRect bufRect = {};
    bufRect.width = width;
    bufRect.height = height;

    std::vector<ChiRect> faceROIOutList;
    MapFaceROIToImageBuffer(faceROIInList, faceROIOutList, cropRegion, bufRect);

    right = 0;
    bottom = 0;

    // pass face info to hdrMetadata
    hdrMetaData.config.face_info.nFace = faceROIOutList.size();
    for (int index = 0; index < faceROIOutList.size(); index++) {
        hdrMetaData.config.face_info.rcFace[index].left = faceROIOutList[index].left;
        hdrMetaData.config.face_info.rcFace[index].top = faceROIOutList[index].top;
        right = faceROIOutList[index].left + faceROIOutList[index].width;
        hdrMetaData.config.face_info.rcFace[index].right = right;
        bottom = faceROIOutList[index].top + faceROIOutList[index].height;
        hdrMetaData.config.face_info.rcFace[index].bottom = bottom;
    }
}

void YUV_drawRect(unsigned char *dst_buf, int w, int h, int lpitch, int x0, int y0, int x1, int y1,
                  unsigned long rgba)
{
    unsigned char r, g, b, y, cb, cr;
    b = (rgba >> 8) & 0xFF;
    g = (rgba >> 16) & 0xFF;
    r = (rgba >> 24) & 0xFF;
    y = 16 + 0.257 * r + 0.504 * g + 0.098 * b;
    cb = 128 - 0.148 * r - 0.291 * g + 0.439 * b;
    cr = 128 + 0.439 * r - 0.368 * g - 0.071 * b;

    int rect_width, rect_height;
    x0 = x0 & 0xFFFE;
    y0 = y0 & 0xFFFE;
    x1 = x1 & 0xFFFE;
    y1 = y1 & 0xFFFE;
    rect_width = x1 - x0;
    rect_height = y1 - y0;

    unsigned char *xoff;
    unsigned char *yoff;
    xoff = dst_buf + y0 * lpitch + x0;
    memset(xoff, y, rect_width); // rowline
    xoff = dst_buf + (y0 + 1) * lpitch + x0;
    memset(xoff, y, rect_width);

    xoff = dst_buf + (y1 - 1) * lpitch + x0;
    memset(xoff, y, rect_width); // colline
    xoff = dst_buf + y1 * lpitch + x0;
    memset(xoff, y, rect_width);

    // y first
    for (int i = 0; i < rect_height; i++) {
        yoff = dst_buf + (i + y0) * lpitch + x0;
        *yoff = y;
        yoff = dst_buf + (i + y0) * lpitch + x0 + 1;
        *yoff = y;

        yoff = dst_buf + (i + y0) * lpitch + x1 - 1;
        *yoff = y;
        yoff = dst_buf + (i + y0) * lpitch + x1;
        *yoff = y;
    }

    // cb next
    xoff = dst_buf + lpitch * h + y0 * lpitch / 4 + x0 / 2;
    memset(xoff, cb, rect_width / 2);

    xoff = dst_buf + lpitch * h + y1 * lpitch / 4 + x0 / 2;
    memset(xoff, cb, rect_width / 2);

    for (int i = 0; i < rect_height; i += 2) {
        yoff = dst_buf + lpitch * h + (i + y0) * lpitch / 4 + x0 / 2;
        *yoff = cb;

        yoff = dst_buf + lpitch * h + (i + y0) * lpitch / 4 + x1 / 2;
        *yoff = cb;
    }

    // cr last
    xoff = dst_buf + lpitch * h + lpitch * h / 4 + y0 * lpitch / 4 + x0 / 2;
    memset(xoff, cr, rect_width / 2);

    xoff = dst_buf + lpitch * h + lpitch * h / 4 + y1 * lpitch / 4 + x0 / 2;
    memset(xoff, cr, rect_width / 2);

    for (int i = 0; i < rect_height; i += 2) {
        yoff = dst_buf + lpitch * h + lpitch * h / 4 + (i + y0) * lpitch / 4 + x0 / 2;
        *yoff = cr;

        yoff = dst_buf + lpitch * h + lpitch * h / 4 + (i + y0) * lpitch / 4 + x1 / 2;
        *yoff = cr;
    }
}

void MiAiHdrPlugin::DebugFaceRoi(struct MiImageBuffer *outputImageFrame, HDRMetaData &hdrMetaData)
{
    int nStride = outputImageFrame->stride;

    for (int i = 0; i < hdrMetaData.config.face_info.nFace; i++) {
        int x1 = hdrMetaData.config.face_info.rcFace[i].left;
        int x2 = hdrMetaData.config.face_info.rcFace[i].right;
        int y1 = hdrMetaData.config.face_info.rcFace[i].top;
        int y2 = hdrMetaData.config.face_info.rcFace[i].bottom;

        YUV_drawRect((UCHAR *)outputImageFrame->plane[0], outputImageFrame->width,
                     outputImageFrame->height, nStride, x1, y1, x2, y2, 0);
    }
}

void MiAiHdrPlugin::MapFaceROIToImageBuffer(std::vector<ChiRect> &rFaceROIIn,
                                            std::vector<ChiRect> &rFaceROIOut, ChiRect cropInfo,
                                            ChiRect bufferSize)
{
    if (0 >= cropInfo.width || 0 >= cropInfo.height)
        return;

    float ratioW = (float)bufferSize.width / (float)cropInfo.width;
    float ratioH = (float)bufferSize.height / (float)cropInfo.height;
    float xOffSize = (ratioH * (float)cropInfo.width - bufferSize.width) / 2;
    float yOffSize = (ratioW * (float)cropInfo.height - bufferSize.height) / 2;
    ChiRect faceROI = {};
    unsigned faceNum = rFaceROIIn.size();

    for (unsigned i = 0; i < faceNum; i++) {
        faceROI = rFaceROIIn[i];
        if (cropInfo.left <= faceROI.left && faceROI.left <= cropInfo.left + cropInfo.width &&
            cropInfo.left <= faceROI.left + faceROI.width &&
            faceROI.left + faceROI.width <= cropInfo.left + cropInfo.width &&
            cropInfo.top <= faceROI.top && faceROI.top <= cropInfo.top + cropInfo.height &&
            cropInfo.top <= faceROI.top + faceROI.height &&
            faceROI.top + faceROI.height <= cropInfo.top + cropInfo.height) {
            faceROI.left -= cropInfo.left;
            faceROI.top -= cropInfo.top;

            if (ratioW > ratioH) {
                faceROI.left = (float)(faceROI.left) * ratioW;
                faceROI.top = (float)(faceROI.top) * ratioW - yOffSize;
                faceROI.width = (float)(faceROI.width) * ratioW;
                faceROI.height = (float)(faceROI.height) * ratioW;
            } else {
                faceROI.left = (float)(faceROI.left) * ratioH - xOffSize;
                faceROI.top = (float)(faceROI.top) * ratioH;
                faceROI.width = (float)(faceROI.width) * ratioH;
                faceROI.height = (float)(faceROI.height) * ratioH;
            }

            rFaceROIOut.push_back(faceROI);
        }
    }
}

} // namespace mialgo2
