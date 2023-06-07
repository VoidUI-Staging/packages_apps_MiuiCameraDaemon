#include "MiFusionPlugin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "optical_zoom_fusion.h"

#undef LOG_TAG
#define LOG_TAG "MIA_N_CPFUSION"

#define CALI_DATA_LEN_2048 2048
#define CALI_DATA_LEN_4096 4096

using namespace mialgo2;

struct CaliConfigInfo
{
    std::string path;
    WCalibDataType type;
};

static CaliConfigInfo sCaliConfigInfo[2] = {
    {"com.xiaomi.dcal.wu.data", WCalibDataType::WCALIB_TYPE_W_U},
    {"com.xiaomi.dcal.wt.data", WCalibDataType::WCALIB_TYPE_W_T}};

const char fusionModelPath[] = "/vendor/etc/camera/fusion_models";

static long long dumpTimeIndex;

static int readDataFromFile(const char *path, void **buf)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        MLOGE(Mia2LogGroupPlugin, "open file(%s) failed.", path);
        return 0;
    }

    int status = fseek(fp, 0, SEEK_END);
    if (0 != status) {
        MLOGE(Mia2LogGroupPlugin, "seek to file (%s) end failed: %d", path, status);
        fclose(fp);
        return 0;
    }
    int fileSize = ftell(fp);
    if (0 >= fileSize) {
        MLOGE(Mia2LogGroupPlugin, "calculate file (%s) size failed: %d", path, fileSize);
        fclose(fp);
        return 0;
    }
    if (NULL != buf) {
        status = fseek(fp, 0, SEEK_SET);
        if (0 != status) {
            MLOGE(Mia2LogGroupPlugin, "seek to file (%s) begin failed: %d", path, status);
            fclose(fp);
            return 0;
        }
        *buf = (void *)malloc(fileSize);
        int size = fread(*buf, 1, fileSize, fp);
        if (size != fileSize) {
            MLOGE(Mia2LogGroupPlugin, "file size: %d and readed size: %d not equal", fileSize,
                  size);
            fclose(fp);
            return size;
        }
    } else {
        MLOGE(Mia2LogGroupPlugin, "calibration buffer is null");
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return fileSize;
}

MiFusionPlugin::~MiFusionPlugin()
{
    MLOGI(Mia2LogGroupPlugin, "now distruct -->> MiFusionPlugin");
}

int MiFusionPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    m_pFusionInstance = NULL;
    m_pMasterPhyMetadata = NULL;
    m_pSlaverPhyMetadata = NULL;
    m_iszStatus = InSensorZoomState::InSensorZoomNone;
    m_fusionNodeInterface = nodeInterface;

    if (!m_pFusionInstance) {
        m_pFusionInstance = new wa::OpticalZoomFusion();
        MLOGI(Mia2LogGroupPlugin, "MiFusion fusion algo version:%s",
              m_pFusionInstance->getVersion());
    }
    CDKResult initResult = iniEngine();
    MLOGI(Mia2LogGroupPlugin, "MiFusion initialize iniEngine:%d", initResult);
    return initResult;
}

bool MiFusionPlugin::isEnabled(MiaParams settings)
{
    void *pData = NULL;
    bool miFusionEnable = false;
    const char *FusionEnabled = "xiaomi.capturefusion.isFusionOn";
    VendorMetadataParser::getVTagValue(settings.metadata, FusionEnabled, &pData);
    if (NULL != pData) {
        miFusionEnable = *static_cast<bool *>(pData);
    }
    MLOGI(Mia2LogGroupPlugin, "MiFusion isFusionOn:%d", miFusionEnable);
    return miFusionEnable;
}

ProcessRetStatus MiFusionPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    MiaParams settings = {0};
    int ret = 0;
    MLOGI(Mia2LogGroupPlugin, "mifusion enter processRequest frameNum:%d",
          pProcessRequestInfo->frameNum);

    int32_t iIsDumpData = property_get_int32("persist.vendor.camera.fusion.dump", 0);
    int32_t bCopyImage = property_get_int32("persist.vendor.camera.fusion.copyimage", 0);

    m_debugDumpImage = iIsDumpData;
    m_processedFrame = pProcessRequestInfo->frameNum;
    uint32_t inputPorts = pProcessRequestInfo->inputBuffersMap.size();
    auto &inputBuffers = pProcessRequestInfo->inputBuffersMap.at(0);

    settings.metadata = inputBuffers[0].pMetadata;
    int inputBufferSize = inputBuffers.size();

    void *pData = NULL;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.capturefusion.fusionType",
                                       &pData);
    if (NULL != pData) {
        m_fusionType = *static_cast<FusionPipelineType *>(pData);
    } else {
        MLOGD(Mia2LogGroupPlugin, "mifusion get fusionType failed, use PipelineTypeUWW");
    }

    mialgo2::MiImageBuffer inputFrameMaster = {0}, inputFrameSlave = {0}, outputFrame = {0};

    convertImageParams(inputBuffers[0], inputFrameMaster);
    m_pMasterPhyMetadata = inputBuffers[0].pPhyCamMetadata;
    MLOGI(Mia2LogGroupPlugin, "mifusion inputFrameMaster:format:%d %d*%d masterPhyMeta:%p",
          inputFrameMaster.format, inputFrameMaster.width, inputFrameMaster.height,
          m_pMasterPhyMetadata);

    if (inputFrameMaster.plane[0] == NULL || inputFrameMaster.plane[1] == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s:%d wrong input", __func__, __LINE__);
    }

    if (inputPorts > 1 && isEnabled(settings) == true) {
        auto &inputAuxBuffers = pProcessRequestInfo->inputBuffersMap.at(1);
        convertImageParams(inputAuxBuffers[0], inputFrameSlave);

        m_pSlaverPhyMetadata = inputAuxBuffers[0].pPhyCamMetadata;
        MLOGI(Mia2LogGroupPlugin, "mifusion inputFrameSlave:format:%d %d*%d slavePhyMeta:%p",
              inputFrameSlave.format, inputFrameSlave.width, inputFrameSlave.height,
              m_pSlaverPhyMetadata);
        if (inputFrameSlave.plane[0] == NULL || inputFrameSlave.plane[1] == NULL) {
            MLOGE(Mia2LogGroupPlugin, "%s:%d wrong input", __func__, __LINE__);
        }
    }

    auto &outputBuffers = pProcessRequestInfo->outputBuffersMap.begin()->second;
    convertImageParams(outputBuffers[0], outputFrame);
    if (outputFrame.plane[0] == NULL || outputFrame.plane[1] == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s:%d mifusion wrong output", __func__, __LINE__);
    }

    if (bCopyImage == 1 || inputBufferSize == 1 || isEnabled(settings) == false) {
        m_algoRunResult = -1;
        ret = PluginUtils::miCopyBuffer(&outputFrame, &inputFrameMaster);
    } else {
        ret = processBuffer(&inputFrameMaster, &inputFrameSlave, &outputFrame, settings.metadata);
        // if processBuffer setAlgoInput filed, we return master buffer.
        if (ret != MIA_RETURN_OK) {
            MLOGD(Mia2LogGroupPlugin, "mifusion setAlgoInput ret:%d, bypass fusion", ret);
            m_algoRunResult = -1;
            ret = PluginUtils::miCopyBuffer(&outputFrame, &inputFrameMaster);
        }
    }

    if (m_fusionNodeInterface.pSetResultMetadata != NULL) {
        std::string results = "MiSnapshotFusion:" + std::to_string(m_algoRunResult + 1);
        m_fusionNodeInterface.pSetResultMetadata(m_fusionNodeInterface.owner,
                                                 pProcessRequestInfo->frameNum,
                                                 pProcessRequestInfo->timeStamp, results);
        MLOGD(Mia2LogGroupPlugin, "mifusion setExif %s", results.c_str());
    }

    if (iIsDumpData) {
        char outputbuf[128];
        uint32_t masterScaleInt = m_debugMasterScale;
        uint32_t masterScaleDec = (m_debugMasterScale - masterScaleInt) * 100;
        uint32_t slaverScaleInt = m_debugSlaverScale;
        uint32_t slaverScaleDec = (m_debugSlaverScale - slaverScaleInt) * 100;
        snprintf(outputbuf, sizeof(outputbuf),
                 "mifusion_output_%dx%d_master%d-%d_slaver%d-%d_ret%d", outputFrame.width,
                 outputFrame.height, masterScaleInt, masterScaleDec, slaverScaleInt, slaverScaleDec,
                 m_algoRunResult);
        PluginUtils::dumpToFile(outputbuf, &outputFrame);
    }

    if (ret != 0) {
        resultInfo = PROCFAILED;
    }
    return resultInfo;
}

ProcessRetStatus MiFusionPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int MiFusionPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

void MiFusionPlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);
    if (NULL != m_pIntsenceData) {
        free(m_pIntsenceData);
        m_pIntsenceData = NULL;
    }

    /*
    const uint32_t kCaliCount = sizeof(sCaliConfigInfo) / sizeof(sCaliConfigInfo[0]);
    for (int i = 0; i < kCaliCount && i < MAX_CALI_CNT; i++) {
        if (NULL != m_pCaliData[i]) {
            free(m_pCaliData[i]);
            m_pCaliData[i] = NULL;
        }
    }
    */

    if (m_pFusionInstance != NULL) {
        delete m_pFusionInstance;
        m_pFusionInstance = NULL;
    }
    MLOGD(Mia2LogGroupPlugin, "MiFusionPlugin release");
}

CDKResult MiFusionPlugin::iniEngine()
{
    CDKResult ret = MIA_RETURN_OK;
    bool result = false;
    // This interface and config bin file can select fusion algo type NN or CV
    WOpZFusionConfig intsenceConfig;
    const char kStaticConfigPath[] = "/vendor/etc/camera/fusion_mecp.bin";

    int intsenceDataSize = readDataFromFile(kStaticConfigPath, &m_pIntsenceData);
    if (m_pIntsenceData != NULL) {
        intsenceConfig.mecpBlob.data = m_pIntsenceData;
        intsenceConfig.mecpBlob.length = intsenceDataSize;

        result = m_pFusionInstance->init(intsenceConfig);
        if (result == true) {
            MLOGD(Mia2LogGroupPlugin, "mifusion init intsenceConfig sucess");
        } else {
            ret = MIA_RETURN_UNKNOWN_ERROR;
            MLOGE(Mia2LogGroupPlugin, "mifusion init intsenceConfig failed");
        }
    } else {
        ret = MIA_RETURN_OPEN_FAILED;
        MLOGE(Mia2LogGroupPlugin, "mifusion can not read fusion_mecp.bin");
    }
    /*
    // read calibration data
    const uint32_t kCaliCount = sizeof(sCaliConfigInfo) / sizeof(sCaliConfigInfo[0]);
    WCalibDataBlob caliDataBlob[kCaliCount];
    uint32_t caliCount = 0;
    WCalibInfo caliInfo;

    char filename[FILE_PATH_LENGTH];
    for (uint8_t i = 0; i < kCaliCount && i < MAX_CALI_CNT; i++) {
        snprintf(filename, sizeof(filename), "%s/%s", "/data/vendor/camera",
                 sCaliConfigInfo[i].path.c_str());
        int size = readDataFromFile(filename, &m_pCaliData[i]);
        MLOGI(Mia2LogGroupPlugin, "mifusion read calibration data Count %d, type %d, size[%d]: %d",
              kCaliCount, sCaliConfigInfo[i].type, i, size);

        if (size == CALI_DATA_LEN_2048 || size == CALI_DATA_LEN_4096) {
            caliDataBlob[i].blob.data = (void *)m_pCaliData[i];
            caliDataBlob[i].blob.length = size;
            caliDataBlob[i].type = sCaliConfigInfo[i].type;
            caliCount++;
        } else {
            MLOGE(Mia2LogGroupPlugin, "mifusion error : filr(%s) size(%d) diff cali size(%d or %d)",
                  filename, size, CALI_DATA_LEN_2048, CALI_DATA_LEN_4096);
            for (int i = 0; i < kCaliCount && i < MAX_CALI_CNT; i++) {
                if (NULL != m_pCaliData[i]) {
                    free(m_pCaliData[i]);
                    m_pCaliData[i] = NULL;
                }
            }
            return ret;
        }
    }

    caliInfo.calibData = caliDataBlob;
    caliInfo.counts = caliCount;
    result = m_pFusionInstance->setCalibData(caliInfo);
    if (false == result) {
        MLOGE(Mia2LogGroupPlugin, "mifusion setCalibData failed");
        ret = MIAS_INVALID_PARAM;
    }
    */
    WModelInfo model_info;
    model_info.path = const_cast<WInt8 *>(fusionModelPath);
    result = m_pFusionInstance->setModel(model_info);
    if (false == result) {
        MLOGE(Mia2LogGroupPlugin, "mifusion setModel failed");
        ret = MIAS_INVALID_PARAM;
    }

    return ret;
}

CDKResult MiFusionPlugin::processBuffer(MiImageBuffer *masterImage, MiImageBuffer *slaveImage,
                                        MiImageBuffer *outputFusion, camera_metadata_t *pMetaData)
{
    int result = MIA_RETURN_OK;
    if (pMetaData == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s error input MetaData is null", __func__);
        return MIA_RETURN_INVALID_PARAM;
    }

    double startTime = PluginUtils::nowMSec();

    if (masterImage == NULL || slaveImage == NULL || outputFusion == NULL || pMetaData == NULL) {
        MLOGE(Mia2LogGroupPlugin, "error invalid param %p %p %p %p", masterImage, slaveImage,
              outputFusion, pMetaData);
        return MIAS_INVALID_PARAM;
    }

    // get image orientation
    int orientation = 0;
    void *pData = NULL;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_JPEG_ORIENTATION, &pData);
    if (NULL != pData) {
        orientation = *static_cast<int *>(pData);
    }

    switch (orientation) {
    case 0:
        m_orient = wa::Image::ROTATION_0;
        break;
    case 90:
        m_orient = wa::Image::ROTATION_90;
        break;
    case 180:
        m_orient = wa::Image::ROTATION_180;
        break;
    case 270:
        m_orient = wa::Image::ROTATION_270;
        break;
    default:
        m_orient = wa::Image::ROTATION_0;
    }
    MLOGD(Mia2LogGroupPlugin, "main image orietation: %d", orientation);

    result = setAlgoInputImages(masterImage, slaveImage);
    if (MIA_RETURN_OK != result) {
        MLOGD(Mia2LogGroupPlugin, "ERR! SetAlgoInputImages failed:%d", result);
        return result;
    }

    // Set Algo Input Params
    if (MIA_RETURN_OK == result) {
        result = setAlgoInputParams(masterImage, slaveImage, pMetaData);
        if (MIA_RETURN_OK != result) {
            MLOGD(Mia2LogGroupPlugin, "ERR! SetAlgoInputParams failed:%d", result);
            return result;
        }
    }

    if (m_debugDumpImage) {
        char inputbufMaster[128];
        char inputbufSlaver[128];
        uint32_t masterScaleInt = m_debugMasterScale;
        uint32_t masterScaleDec = (m_debugMasterScale - masterScaleInt) * 100;
        snprintf(inputbufMaster, sizeof(inputbufMaster), "mifusion_input_master_%dx%d_scale%d-%d",
                 masterImage->width, masterImage->height, masterScaleInt, masterScaleDec);
        PluginUtils::dumpToFile(inputbufMaster, masterImage);

        uint32_t slaverScaleInt = m_debugSlaverScale;
        uint32_t slaverScaleDec = (m_debugSlaverScale - slaverScaleInt) * 100;
        snprintf(inputbufSlaver, sizeof(inputbufSlaver), "mifusion_input_slave_%dx%d_scale%d-%d",
                 slaveImage->width, slaveImage->height, slaverScaleInt, slaverScaleDec);
        PluginUtils::dumpToFile(inputbufSlaver, slaveImage);
    }

    // Set Algo Output Images  and run
    if (MIA_RETURN_OK == result) {
        wa::Image outImage = prepareImage(outputFusion);
        m_algoRunResult = m_pFusionInstance->run(&outImage);
        if (0 == m_algoRunResult) {
            MLOGD(Mia2LogGroupPlugin, "fusion algo run sucess, ret: %d", m_algoRunResult);
        } else {
            MLOGD(Mia2LogGroupPlugin, "fusion algo run failed, ret: %d", m_algoRunResult);
            result = MIA_RETURN_INVALID_PARAM;
        }
    }

    MLOGI(Mia2LogGroupPlugin, "fusion algrithm time: frameNum %d costTime = %.2f", m_processedFrame,
          PluginUtils::nowMSec() - startTime);
    return MIA_RETURN_OK;
}

CDKResult MiFusionPlugin::setAlgoInputImages(MiImageBuffer *inputMaster, MiImageBuffer *inputSlave)
{
    CDKResult result = MIA_RETURN_OK;
    MLOGD(Mia2LogGroupPlugin, "[KS_FUSION] (master->pImageBuffer = %p)", inputMaster);
    MLOGD(Mia2LogGroupPlugin, "[KS_FUSION] (slaver->pImageBuffer = %p)", inputSlave);

    // Set Algo Input Images
    wa::Image mainImage;
    wa::Image auxImage;

    if (m_fusionType == PipelineTypeWT) {
        mainImage = prepareImage(inputMaster);
        mainImage.setExifRoleType(wa::Image::ROLE_WIDE);
        auxImage = prepareImage(inputSlave);
        auxImage.setExifRoleType(wa::Image::ROLE_TELE);
    } else {
        mainImage = prepareImage(inputMaster);
        mainImage.setExifRoleType(wa::Image::ROLE_ULTRA);
        auxImage = prepareImage(inputSlave);
        auxImage.setExifRoleType(wa::Image::ROLE_WIDE);
    }

    bool ret = m_pFusionInstance->setImages(auxImage, mainImage); //(ultra wide, wide)
    if (!ret) // return true if success, false otherwise.
    {
        MLOGD(Mia2LogGroupPlugin, "[KS_FUSION] setImages failed");
        result = MIA_RETURN_INVALID_PARAM;
    }

    return result;
}

CDKResult MiFusionPlugin::setAlgoInputParams(MiImageBuffer *inputMaster, MiImageBuffer *inputSlave,
                                             camera_metadata_t *pMeta)
{
    bool result = 0;

    InputMetadataOpticalZoom inputMetaOZ;
    memset(&inputMetaOZ, 0, sizeof(InputMetadataOpticalZoom));

    result = fillInputMetadata(&inputMetaOZ, pMeta);

    int32_t masterIndex = 0;
    int32_t slaveIndex = 1;
    for (int index = 0; index < MAX_CAMERA_CNT; index++) {
        if (inputMetaOZ.cameraMetadata[index].cameraId ==
            inputMetaOZ.cameraMetadata[index].masterCameraId) {
            masterIndex = index;
        } else {
            slaveIndex = index;
        }
    }

    void *pData = NULL;
    ImageShiftData masterShift = {0, 0};
    VendorMetadataParser::getVTagValue(pMeta, "com.qti.chi.pixelshift.PixelShift", &pData);
    if (NULL != pData) {
        masterShift = *static_cast<ImageShiftData *>(pData);
        // this shift base to input imagesize
        MLOGI(Mia2LogGroupPlugin, "mifusion masterShift (%d, %d)", masterShift.horizonalShift,
              masterShift.verticalShift);
    } else {
        MLOGD(Mia2LogGroupPlugin, "mifusion get pMeta PixelShift failed");
    }

    int32_t masterActiveSizeWidth = 0;
    int32_t masterActiveSizeHeight = 0;
    int32_t slaveActiveSizeWidth = 0;
    int32_t masterScaleWidth = 0;
    int32_t slaveScaleWidth = 0;
    ChiRect masterFovRectIFE;
    ChiRect slaveFovRectIFE;
    WeightedRegion masterFocusRegion;
    WeightedRegion auxFocusRegion;
    masterActiveSizeWidth = inputMetaOZ.cameraMetadata[masterIndex].activeArraySize.width;
    masterActiveSizeHeight = inputMetaOZ.cameraMetadata[masterIndex].activeArraySize.height;

    masterScaleWidth = inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.width;
    masterFovRectIFE = inputMetaOZ.cameraMetadata[masterIndex].fovRectIFE;
    masterFocusRegion = inputMetaOZ.cameraMetadata[masterIndex].afFocusROI;

    slaveActiveSizeWidth = inputMetaOZ.cameraMetadata[slaveIndex].activeArraySize.width;
    slaveScaleWidth = inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.width;
    slaveFovRectIFE = inputMetaOZ.cameraMetadata[slaveIndex].fovRectIFE;
    auxFocusRegion = inputMetaOZ.cameraMetadata[slaveIndex].afFocusROI;

    MLOGI(Mia2LogGroupPlugin,
          "mainActiveSizeWidth: %d, mainScaleWidth: %d mainFovRectIFE [%d, %d, %d, %d]",
          masterActiveSizeWidth, masterScaleWidth, masterFovRectIFE.left, masterFovRectIFE.top,
          masterFovRectIFE.width, masterFovRectIFE.height);
    MLOGI(Mia2LogGroupPlugin,
          "auxActiveSizeWidth: %d, auxScaleWidth: %d auxFovRectIFE [%d, %d, %d, %d]",
          slaveActiveSizeWidth, slaveScaleWidth, slaveFovRectIFE.left, slaveFovRectIFE.top,
          slaveFovRectIFE.width, slaveFovRectIFE.height);

    pData = NULL;
    float currentUserZoomRatio = 1.0;
    VendorMetadataParser::getTagValue(pMeta, ANDROID_CONTROL_ZOOM_RATIO, &pData);
    if (NULL != pData) {
        currentUserZoomRatio = *static_cast<float *>(pData);
    } else {
        MLOGE(Mia2LogGroupPlugin, "mifusion get ANDROID_CONTROL_ZOOM_RATIO failed!");
    }
    pData = NULL;
    float sensorScale = 1.0f;
    if (m_iszStatus == InSensorZoomState::InSensorZoomCrop) {
        result =
            VendorMetadataParser::getVTagValue(pMeta, "xiaomi.capturefusion.sensorScale", &pData);
        if (result == 0 && pData != NULL) {
            sensorScale = *static_cast<float_t *>(pData);
        } else {
            // Protection logic to prevent errors in calculating Scale
            // caused by Meta pickup exceptions
            // The default Crop for InSensorZoom on L1 is 2X
            sensorScale = 2.0f;
            MLOGE(Mia2LogGroupPlugin, "mifusion get sensorScale meta error");
        }
    }
    MLOGI(Mia2LogGroupPlugin, "sensor scale: %.3f", sensorScale);
    float masterScale = 1.0f;
    float slaveScale = 1.0f;
    {
        masterScale = (masterActiveSizeWidth * 1.0 / masterScaleWidth) * sensorScale;
        slaveScale = slaveActiveSizeWidth * 1.0 / slaveScaleWidth;
        MLOGI(Mia2LogGroupPlugin, "user zoom: %.3f, scale master: %.3f slave: %.3f",
              currentUserZoomRatio, masterScale, slaveScale);
    }
    m_debugMasterScale = masterScale;
    m_debugSlaverScale = slaveScale;

    // set fusion param
    WOpZFusionParams fusionParams;
    bool isQcfaSensor = 0;
    ChiRectINT masterCropRegion;
    ChiRectINT slaveCropRegion;
    ChiRectINT masterAspectCropRegion;
    ChiRectINT slaveAspectCropRegion;
    ImageFormat masterFormat;
    ImageFormat slaveFormat;
    fusionParams.scale.zoomFactor = currentUserZoomRatio;

    pData = NULL;
    uint32_t satDistance = MI_SAT_DEFAULT_DISTANCE;
    VendorMetadataParser::getVTagValue(pMeta, "xiaomi.capturefusion.satDistance", &pData);
    if (NULL != pData) {
        satDistance = *static_cast<uint32_t *>(pData);
        fusionParams.main.distance = satDistance * 10.0;
        fusionParams.aux.distance = satDistance * 10.0;
    } else {
        // use master focus distance
        fusionParams.main.distance = inputMetaOZ.cameraMetadata[masterIndex].focusDistCm * 10;
        fusionParams.aux.distance = inputMetaOZ.cameraMetadata[masterIndex].focusDistCm * 10;
        MLOGD(Mia2LogGroupPlugin, "mifusion get satDistance failed, use focusDistCm");
    }

    // K1 is W&T fusion, master wide is aux, slaver tele is main
    // K8 is UW&W fusion, master UW is aux, slaver W is main
    if (m_fusionType == PipelineTypeWT) {
        fusionParams.scale.auxScale = masterScale;
        fusionParams.aux.luma = inputMetaOZ.cameraMetadata[masterIndex].lux;
        bool afStable = inputMetaOZ.cameraMetadata[masterIndex].afState ==
                            static_cast<uint32_t>(MiAFState::PassiveFocused) ||
                        inputMetaOZ.cameraMetadata[masterIndex].afState ==
                            static_cast<uint32_t>(MiAFState::FocusedLocked);
        fusionParams.aux.isAfStable = afStable ? WTrue : WFalse;
        isQcfaSensor = inputMetaOZ.cameraMetadata[masterIndex].isQuadCFASensor;
        ChiRect masterActivesize = inputMetaOZ.cameraMetadata[masterIndex].activeArraySize;
        masterCropRegion.left = inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.left;
        masterCropRegion.top = inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.top;
        masterCropRegion.width = inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.width;
        masterCropRegion.height = inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.height;

        masterFormat.format = inputMaster->format;
        masterFormat.width = inputMaster->width;
        masterFormat.height = inputMaster->height;
        masterAspectCropRegion = covCropInfoAcordAspectRatio(masterCropRegion, masterFormat,
                                                             masterActivesize, isQcfaSensor);
        fusionParams.aux.cropRect = {
            static_cast<WInt32>(masterAspectCropRegion.left + masterShift.horizonalShift),
            static_cast<WInt32>(masterAspectCropRegion.top + masterShift.verticalShift),
            static_cast<WInt32>(masterAspectCropRegion.width + masterAspectCropRegion.left +
                                masterShift.horizonalShift),
            static_cast<WInt32>(masterAspectCropRegion.height + masterAspectCropRegion.top +
                                masterShift.verticalShift)};

        MLOGI(Mia2LogGroupPlugin,
              "masterScale: %.3f,isQuadCFASensor %d, luma: %.3f distance: %.3f, afState %d, "
              "cropRect [%d, %d, %d, %d], shift:[%d, %d] frameNum:%d",
              fusionParams.scale.auxScale, isQcfaSensor, fusionParams.aux.luma,
              fusionParams.aux.distance, inputMetaOZ.cameraMetadata[masterIndex].afState,
              fusionParams.aux.cropRect.left, fusionParams.aux.cropRect.top,
              fusionParams.aux.cropRect.right, fusionParams.aux.cropRect.bottom,
              masterShift.horizonalShift, masterShift.verticalShift, m_processedFrame);

        fusionParams.scale.mainScale = slaveScale;
        fusionParams.main.luma = inputMetaOZ.cameraMetadata[slaveIndex].lux;
        afStable = inputMetaOZ.cameraMetadata[slaveIndex].afState ==
                       static_cast<uint32_t>(MiAFState::PassiveFocused) ||
                   inputMetaOZ.cameraMetadata[slaveIndex].afState ==
                       static_cast<uint32_t>(MiAFState::FocusedLocked);
        fusionParams.main.isAfStable = afStable ? WTrue : WFalse;
        isQcfaSensor = inputMetaOZ.cameraMetadata[slaveIndex].isQuadCFASensor;
        ChiRect slaveActivesize = inputMetaOZ.cameraMetadata[slaveIndex].activeArraySize;
        slaveCropRegion.left = inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.left;
        slaveCropRegion.top = inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.top;
        slaveCropRegion.height = inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.height;
        slaveCropRegion.width = inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.width;

        slaveFormat.format = inputSlave->format;
        slaveFormat.width = inputSlave->width;
        slaveFormat.height = inputSlave->height;
        slaveAspectCropRegion = covCropInfoAcordAspectRatio(slaveCropRegion, slaveFormat,
                                                            slaveActivesize, isQcfaSensor);
        fusionParams.main.cropRect = {
            static_cast<WInt32>(slaveAspectCropRegion.left),
            static_cast<WInt32>(slaveAspectCropRegion.top),
            static_cast<WInt32>(slaveAspectCropRegion.width + slaveAspectCropRegion.left),
            static_cast<WInt32>(slaveAspectCropRegion.height + slaveAspectCropRegion.top)};
        MLOGI(Mia2LogGroupPlugin,
              "slaveScale: %.3f, isQcfaSensor %d, luma: %.3f distance: %.3f, afState %d, cropRect "
              "[%d, %d, %d, %d] frameNum:%d",
              fusionParams.scale.mainScale, isQcfaSensor, fusionParams.main.luma,
              fusionParams.main.distance, inputMetaOZ.cameraMetadata[slaveIndex].afState,
              fusionParams.main.cropRect.left, fusionParams.main.cropRect.top,
              fusionParams.main.cropRect.right, fusionParams.main.cropRect.bottom,
              m_processedFrame);
    } else {
        fusionParams.scale.auxScale = masterScale;
        fusionParams.aux.luma = inputMetaOZ.cameraMetadata[masterIndex].lux;
        bool afStable = inputMetaOZ.cameraMetadata[masterIndex].afState ==
                            static_cast<uint32_t>(MiAFState::PassiveFocused) ||
                        inputMetaOZ.cameraMetadata[masterIndex].afState ==
                            static_cast<uint32_t>(MiAFState::FocusedLocked);

        fusionParams.aux.isAfStable = afStable ? WTrue : WFalse;
        isQcfaSensor = inputMetaOZ.cameraMetadata[masterIndex].isQuadCFASensor;
        ChiRect masterActivesize = inputMetaOZ.cameraMetadata[masterIndex].activeArraySize;
        masterCropRegion.left = inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.left;
        masterCropRegion.top = inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.top;
        masterCropRegion.width = inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.width;
        masterCropRegion.height = inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.height;

        masterFormat.format = inputMaster->format;
        masterFormat.width = inputMaster->width;
        masterFormat.height = inputMaster->height;
        masterAspectCropRegion = covCropInfoAcordAspectRatio(masterCropRegion, masterFormat,
                                                             masterActivesize, isQcfaSensor);

        fusionParams.aux.cropRect = {
            static_cast<WInt32>(masterAspectCropRegion.left),
            static_cast<WInt32>(masterAspectCropRegion.top),
            static_cast<WInt32>(masterAspectCropRegion.width + masterAspectCropRegion.left),
            static_cast<WInt32>(masterAspectCropRegion.height + masterAspectCropRegion.top)};

        MLOGI(Mia2LogGroupPlugin,
              "masterScale: %.3f,isQuadCFASensor %d, luma: %.3f distance: %.3f, afState %d, "
              "cropRect [%d, %d, %d, %d], shift:[%d, %d] frameNum:%d",
              fusionParams.scale.auxScale, isQcfaSensor, fusionParams.aux.luma,
              fusionParams.aux.distance, inputMetaOZ.cameraMetadata[masterIndex].afState,
              fusionParams.aux.cropRect.left, fusionParams.aux.cropRect.top,
              fusionParams.aux.cropRect.right, fusionParams.aux.cropRect.bottom,
              masterShift.horizonalShift, masterShift.verticalShift, m_processedFrame);

        fusionParams.scale.mainScale = slaveScale;
        fusionParams.main.luma = inputMetaOZ.cameraMetadata[slaveIndex].lux;
        afStable = inputMetaOZ.cameraMetadata[slaveIndex].afState ==
                       static_cast<uint32_t>(MiAFState::PassiveFocused) ||
                   inputMetaOZ.cameraMetadata[slaveIndex].afState ==
                       static_cast<uint32_t>(MiAFState::FocusedLocked);
        fusionParams.main.isAfStable = afStable ? WTrue : WFalse;
        isQcfaSensor = inputMetaOZ.cameraMetadata[slaveIndex].isQuadCFASensor;

        ChiRect slaveActivesize = inputMetaOZ.cameraMetadata[slaveIndex].activeArraySize;
        slaveCropRegion.left = inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.left;
        slaveCropRegion.top = inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.top;
        slaveCropRegion.height = inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.height;
        slaveCropRegion.width = inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.width;

        slaveFormat.format = inputSlave->format;
        slaveFormat.width = inputSlave->width;
        slaveFormat.height = inputSlave->height;
        slaveAspectCropRegion = covCropInfoAcordAspectRatio(slaveCropRegion, slaveFormat,
                                                            slaveActivesize, isQcfaSensor);

        fusionParams.main.cropRect = {
            static_cast<WInt32>(slaveAspectCropRegion.left),
            static_cast<WInt32>(slaveAspectCropRegion.top),
            static_cast<WInt32>(slaveAspectCropRegion.width + slaveAspectCropRegion.left),
            static_cast<WInt32>(slaveAspectCropRegion.height + slaveAspectCropRegion.top)};

        MLOGI(Mia2LogGroupPlugin,
              "slaveScale: %.3f, isQcfaSensor %d, luma: %.3f distance: %.3f, afState %d, cropRect "
              "[%d, %d, %d, %d] frameNum:%d",
              fusionParams.scale.mainScale, isQcfaSensor, fusionParams.main.luma,
              fusionParams.main.distance, inputMetaOZ.cameraMetadata[slaveIndex].afState,
              fusionParams.main.cropRect.left, fusionParams.main.cropRect.top,
              fusionParams.main.cropRect.right, fusionParams.main.cropRect.bottom,
              m_processedFrame);
    }

    result = m_pFusionInstance->setConfig(fusionParams);
    if (result != true) {
        MLOGI(Mia2LogGroupPlugin, "setConfig failed, result: %d", result);
        return MIA_RETURN_INVALID_PARAM;
    }

    // focus roi
    ChiRectINT dstFocusRect;
    memset(&dstFocusRect, 0, sizeof(ChiRectINT));

    ChiRectINT focusRect = {masterFocusRegion.xMin, masterFocusRegion.yMin,
                            (uint32_t)(masterFocusRegion.xMax - masterFocusRegion.xMin + 1),
                            (uint32_t)(masterFocusRegion.yMax - masterFocusRegion.yMin + 1)};

    ImageFormat mainFormat;
    mainFormat.format = inputMaster->format;
    mainFormat.width = inputMaster->width;
    mainFormat.height = inputMaster->height;
    isQcfaSensor = inputMetaOZ.cameraMetadata[masterIndex].isQuadCFASensor;
    ChiRect masterActivesize = inputMetaOZ.cameraMetadata[masterIndex].activeArraySize;
    /*
    if (isQcfaSensor == true) {
        masterActivesize.left >>= 1;
        masterActivesize.top >>= 1;
        masterActivesize.width >>= 1;
        masterActivesize.height >>= 1;
    }
    mapRectToInputData(focusRect, masterActivesize, masterAspectCropRegion, masterShift, mainFormat,
                       dstFocusRect);
    MLOGI(Mia2LogGroupPlugin, "set focus roi: [%d, %d, %d, %d]", dstFocusRect.left,
          dstFocusRect.top, dstFocusRect.width, dstFocusRect.height);
    */
    WPoint2i focus;
    dstFocusRect =
        covCropInfoAcordAspectRatio(focusRect, masterFormat, masterActivesize, isQcfaSensor);
    focus.x = dstFocusRect.left + dstFocusRect.width / 2;
    focus.y = dstFocusRect.top + dstFocusRect.height / 2;
    MLOGI(Mia2LogGroupPlugin, "mifusion set focus roi: [%d, %d, %d, %d]", dstFocusRect.left,
          dstFocusRect.top, dstFocusRect.width, dstFocusRect.height);

    result = m_pFusionInstance->setFocus(focus);
    if (result != true) {
        MLOGI(Mia2LogGroupPlugin, "mifusion setFocus failed, result: %d", result);
    }
    // draw focus roi and point for debug
    /*
    MIPOINT focusPoint;
    focusPoint.x = focus.x;
    focusPoint.y = focus.y;
    DrawPoint(inputMaster, focusPoint, 10, 0);
    DrawRectROI(inputMaster, dstFocusRect, 5, 0);
    */

    if (m_debugDumpImage) {
        char dumpInfoBuf[128] = "mifusion_dumpInfo";
        PluginUtils::dumpStrToFile(
            dumpInfoBuf,
            "mS%.2f\nmC[%d,%d,%d,%d]\nmQ%d\nmL%.1f\nsS%.2f\nsC[%d,%d,%d,%d]\nsQ%d\n"
            "sL%.1f\nD:%.1f\n",
            m_debugMasterScale, inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.left,
            inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.top,
            inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.width,
            inputMetaOZ.cameraMetadata[masterIndex].userCropRegion.height,
            inputMetaOZ.cameraMetadata[masterIndex].isQuadCFASensor,
            inputMetaOZ.cameraMetadata[masterIndex].lux, m_debugSlaverScale,
            inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.left,
            inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.top,
            inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.width,
            inputMetaOZ.cameraMetadata[slaveIndex].userCropRegion.height,
            inputMetaOZ.cameraMetadata[slaveIndex].isQuadCFASensor,
            inputMetaOZ.cameraMetadata[slaveIndex].lux, fusionParams.main.distance);
    }

    return MIA_RETURN_OK;
}

CDKResult MiFusionPlugin::fillInputMetadata(InputMetadataOpticalZoom *psatInputMeta,
                                            camera_metadata_t *pMeta)
{
    CDKResult result = MIAS_OK;
    void *pData = NULL;
    void *pDataRefCrop = NULL;

    int masterCameraId = 0;
    int masterIndex = 0;
    int slaverIndex = 0;
    std::vector<int> cameraIndexTable;
    result = VendorMetadataParser::getVTagValue(
        m_pMasterPhyMetadata, "com.qti.chi.metadataOwnerInfo.MetadataOwner", &pData);
    if (NULL != pData) {
        masterIndex = *static_cast<int32_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "mifusion MetadataOwner master:%d", masterIndex);
    } else {
        MLOGD(Mia2LogGroupPlugin, "mifusion get master index failed !!");
    }
    result = VendorMetadataParser::getVTagValue(
        m_pSlaverPhyMetadata, "com.qti.chi.metadataOwnerInfo.MetadataOwner", &pData);
    if (NULL != pData) {
        slaverIndex = *static_cast<int32_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "mifusion MetadataOwner slaver:%d", slaverIndex);
    } else {
        MLOGD(Mia2LogGroupPlugin, "mifusion get slaver index failed !!");
    }
    cameraIndexTable.push_back(masterIndex);
    cameraIndexTable.push_back(slaverIndex);

    camera_metadata_t *pMasterMeta = NULL;
    camera_metadata_t *pSlaverMeta = NULL;
    result = VendorMetadataParser::getVTagValue(m_pMasterPhyMetadata,
                                                "com.qti.chi.multicamerainfo.MasterCamera", &pData);
    if (NULL != pData) {
        bool isMasterCamera = *static_cast<bool *>(pData);
        if (isMasterCamera == true) {
            masterCameraId = masterIndex;
        } else {
            masterCameraId = slaverIndex;
        }
    } else {
        MLOGD(Mia2LogGroupPlugin, "mifusion get master cameraId failed!");
    }

    void *pIszStatus = NULL;

    for (int cameraIndex = 0; cameraIndex < MAX_CAMERA_CNT; cameraIndex++) {
        psatInputMeta->cameraMetadata[cameraIndex].isValid = true;
        psatInputMeta->cameraMetadata[cameraIndex].cameraId = cameraIndexTable[cameraIndex];
        psatInputMeta->cameraMetadata[cameraIndex].masterCameraId = masterCameraId;

        camera_metadata_t *staticMetadata =
            StaticMetadataWraper::getInstance()->getStaticMetadata(cameraIndexTable[cameraIndex]);

        result = VendorMetadataParser::getVTagValue(
            pMeta, "xiaomi.superResolution.inSensorZoomState", &pIszStatus);
        if (pIszStatus != NULL && result == 0) {
            m_iszStatus = *static_cast<int *>(pIszStatus);
            MLOGI(Mia2LogGroupPlugin, "mifusion get current InSensorZoom status: %d", m_iszStatus);
        } else {
            MLOGI(Mia2LogGroupPlugin, "mifusion failed to get InSensorZoom status");
        }

        if (cameraIndexTable[cameraIndex] == masterCameraId &&
            m_iszStatus >= InSensorZoomState::InSensorZoomBinning) {
            result = VendorMetadataParser::getVTagValue(
                m_pMasterPhyMetadata, "org.quic.camera2.ref.cropsize.RefCropSize", &pDataRefCrop);
            if (pDataRefCrop != NULL && result == 0) {
                const RefCropWindowSize &refCropSize =
                    *static_cast<RefCropWindowSize *>(pDataRefCrop);
                psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.width =
                    refCropSize.width;
                psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.height =
                    refCropSize.height;
                psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.left = 0;
                psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.top = 0;
                MLOGI(Mia2LogGroupPlugin,
                      "mifusion cameraIndex:%d isz activeArraySize (%d,%d,%d,%d)", cameraIndex,
                      psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.left,
                      psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.top,
                      psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.width,
                      psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.height);
            } else {
                MLOGE(Mia2LogGroupPlugin, "mifusion failed to get RefCropSize metadata");
            }
        } else {
            result = VendorMetadataParser::getTagValue(
                staticMetadata, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &pData);
            if (pData != NULL && result == 0) {
                ChiRect &activeArray = *static_cast<ChiRect *>(pData);
                psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.left = activeArray.left;
                psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.top = activeArray.top;
                psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.width =
                    activeArray.width;
                psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.height =
                    activeArray.height;
                MLOGI(Mia2LogGroupPlugin, "mifusion cameraIndex:%d activeArraySize (%d,%d,%d,%d)",
                      cameraIndex, psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.left,
                      psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.top,
                      psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.width,
                      psatInputMeta->cameraMetadata[cameraIndex].activeArraySize.height);
            } else {
                MLOGE(Mia2LogGroupPlugin,
                      "mifusion failed to get ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE Params");
            }
        }

        int32_t isQuadCFASensor = 0;
        VendorMetadataParser::getVTagValue(
            staticMetadata, "org.codeaurora.qcamera3.quadra_cfa.is_qcfa_sensor", &pData);
        if (NULL != pData) {
            isQuadCFASensor = *static_cast<int32_t *>(pData);
        } else {
            MLOGD(Mia2LogGroupPlugin, "mifusion get is_qcfa_sensor failed !!");
        }
        if (isQuadCFASensor == 1) {
            psatInputMeta->cameraMetadata[cameraIndex].isQuadCFASensor = 1;
        } else {
            psatInputMeta->cameraMetadata[cameraIndex].isQuadCFASensor = 0;
        }

        camera_metadata_t *pPhyMeta = NULL;
        ChiRect cropRegion = {};
        if (cameraIndexTable[cameraIndex] == masterCameraId) {
            pPhyMeta = m_pMasterPhyMetadata;
            VendorMetadataParser::getVTagValue(m_pMasterPhyMetadata,
                                               "xiaomi.snapshot.residualCropRegion", &pData);
            if (NULL != pData) {
                cropRegion = *static_cast<ChiRect *>(pData);
            } else {
                MLOGE(Mia2LogGroupPlugin, "mifusion get crop region failed !!");
            }
        } else {
            pPhyMeta = m_pSlaverPhyMetadata;
            VendorMetadataParser::getVTagValue(m_pSlaverPhyMetadata,
                                               "xiaomi.capturefusion.cropInfo", &pData);
            if (NULL != pData) {
                cropRegion = *static_cast<ChiRect *>(pData);
                if (isQuadCFASensor == 1) {
                    cropRegion.left >>= 1;
                    cropRegion.top >>= 1;
                    cropRegion.width >>= 1;
                    cropRegion.height >>= 1;
                }
            } else {
                MLOGE(Mia2LogGroupPlugin, "mifusion get crop region failed !!");
            }
        }

        psatInputMeta->cameraMetadata[cameraIndex].userCropRegion.left = cropRegion.left;
        psatInputMeta->cameraMetadata[cameraIndex].userCropRegion.top = cropRegion.top;
        psatInputMeta->cameraMetadata[cameraIndex].userCropRegion.width = cropRegion.width;
        psatInputMeta->cameraMetadata[cameraIndex].userCropRegion.height = cropRegion.height;
        MLOGI(Mia2LogGroupPlugin, "mifusion cameraIndex:%d usercropregion (%d,%d,%d,%d)",
              cameraIndex, psatInputMeta->cameraMetadata[cameraIndex].userCropRegion.left,
              psatInputMeta->cameraMetadata[cameraIndex].userCropRegion.top,
              psatInputMeta->cameraMetadata[cameraIndex].userCropRegion.width,
              psatInputMeta->cameraMetadata[cameraIndex].userCropRegion.height);

        StreamCropInfo fovResCropInfo = {{0}};
        VendorMetadataParser::getVTagValue(pPhyMeta, "com.qti.camera.streamCropInfo.StreamCropInfo",
                                           &pData);
        if (NULL != pData) {
            fovResCropInfo = *static_cast<StreamCropInfo *>(pData);
        } else {
            MLOGE(Mia2LogGroupPlugin, "mifusion get StreamCropInfo failed !!");
        }
        psatInputMeta->cameraMetadata[cameraIndex].fovRectIFE.left =
            fovResCropInfo.cropWRTActiveArray.left;
        psatInputMeta->cameraMetadata[cameraIndex].fovRectIFE.top =
            fovResCropInfo.cropWRTActiveArray.top;
        psatInputMeta->cameraMetadata[cameraIndex].fovRectIFE.width =
            fovResCropInfo.cropWRTActiveArray.width;
        psatInputMeta->cameraMetadata[cameraIndex].fovRectIFE.height =
            fovResCropInfo.cropWRTActiveArray.height;

        MLOGI(Mia2LogGroupPlugin, "mifusion cameraIndex:%d fovRectIFE (%d,%d,%d,%d)", cameraIndex,
              psatInputMeta->cameraMetadata[cameraIndex].fovRectIFE.left,
              psatInputMeta->cameraMetadata[cameraIndex].fovRectIFE.top,
              psatInputMeta->cameraMetadata[cameraIndex].fovRectIFE.width,
              psatInputMeta->cameraMetadata[cameraIndex].fovRectIFE.height);

        WeightedRegion afRegion = {0};
        VendorMetadataParser::getTagValue(pPhyMeta, ANDROID_CONTROL_AF_REGIONS, &pData);
        if (NULL != pData) {
            afRegion = *static_cast<WeightedRegion *>(pData);
        } else {
            MLOGD(Mia2LogGroupPlugin, "mifusion get ANDROID_CONTROL_AF_REGIONS failed !!");
        }

        psatInputMeta->cameraMetadata[cameraIndex].afFocusROI.xMin = afRegion.xMin >> 1;
        psatInputMeta->cameraMetadata[cameraIndex].afFocusROI.yMin = afRegion.yMin >> 1;
        psatInputMeta->cameraMetadata[cameraIndex].afFocusROI.xMax = afRegion.xMax >> 1;
        psatInputMeta->cameraMetadata[cameraIndex].afFocusROI.yMax = afRegion.yMax >> 1;
        psatInputMeta->cameraMetadata[cameraIndex].afFocusROI.weight = afRegion.weight;

        MLOGI(Mia2LogGroupPlugin, "mifusion cameraIndex:%d afFocusROI (%d,%d,%d,%d) weight %d",
              cameraIndex, psatInputMeta->cameraMetadata[cameraIndex].afFocusROI.xMin,
              psatInputMeta->cameraMetadata[cameraIndex].afFocusROI.yMin,
              psatInputMeta->cameraMetadata[cameraIndex].afFocusROI.xMax,
              psatInputMeta->cameraMetadata[cameraIndex].afFocusROI.yMax,
              psatInputMeta->cameraMetadata[cameraIndex].afFocusROI.weight);

        VendorMetadataParser::getTagValue(pPhyMeta, ANDROID_CONTROL_AF_STATE, &pData);
        if (NULL != pData) {
            psatInputMeta->cameraMetadata[cameraIndex].afState = *static_cast<uint32_t *>(pData);
        } else {
            MLOGD(Mia2LogGroupPlugin, "mifusion get ANDROID_CONTROL_AF_STATE failed !!");
        }

        VendorMetadataParser::getTagValue(pPhyMeta, ANDROID_LENS_FOCUS_DISTANCE, &pData);
        if (NULL != pData) {
            float *pAFDistance = static_cast<float *>(pData);
            psatInputMeta->cameraMetadata[cameraIndex].focusDistCm =
                static_cast<uint32_t>(*pAFDistance > 0 ? (100.0f / *pAFDistance) : 1000.0f);
        } else {
            MLOGD(Mia2LogGroupPlugin, "mifusion get ANDROID_LENS_FOCUS_DISTANCE failed !!");
        }

        float *pluxValue;
        VendorMetadataParser::getVTagValue(pPhyMeta, "com.qti.chi.statsaec.AecLux", &pData);
        if (NULL != pData) {
            pluxValue = static_cast<float *>(pData);
            psatInputMeta->cameraMetadata[cameraIndex].lux = *pluxValue;
        } else {
            MLOGD(Mia2LogGroupPlugin, "mifusion get AecLux failed !!");
        }

        MLOGI(Mia2LogGroupPlugin, "mifusion cameraindex:%d, afstate:%d, afdis:%d, lux:%f, QCFA:%d",
              cameraIndex, psatInputMeta->cameraMetadata[cameraIndex].afState,
              psatInputMeta->cameraMetadata[cameraIndex].focusDistCm,
              psatInputMeta->cameraMetadata[cameraIndex].lux,
              psatInputMeta->cameraMetadata[cameraIndex].isQuadCFASensor);
    }

    return result;
}

wa::Image MiFusionPlugin::prepareImage(MiImageBuffer *imgInput)
{
    MLOGD(Mia2LogGroupPlugin,
          "PrepareImage width: %d, height: %d, planeStride: %d, sliceHeight:%d, plans: %d, format "
          "0x%x",
          imgInput->width, imgInput->height, imgInput->stride, imgInput->scanline,
          imgInput->numberOfPlanes, imgInput->format);
    wa::Image::ImageType imageType;
    wa::Image::DataType dataType;

    if (imgInput->format == CAM_FORMAT_YUV_420_NV12) {
        imageType = wa::Image::IMAGETYPE_NV12;
        dataType = wa::Image::DT_8U;
    } else if (imgInput->format == CAM_FORMAT_YUV_420_NV21) {
        imageType = wa::Image::IMAGETYPE_NV21;
        dataType = wa::Image::DT_8U;
    } else {
        imageType = wa::Image::IMAGETYPE_MAX;
        dataType = wa::Image::DT_8U;
        MLOGD(Mia2LogGroupPlugin, "unsupported image format: %d", imgInput->format);
    }

    int width = imgInput->width;
    int height = imgInput->height;
    int stride = imgInput->stride;
    int sliceHeight = imgInput->scanline;

    wa::Image outImage(stride, sliceHeight, imgInput->plane[0], imgInput->plane[1], NULL, imageType,
                       dataType,
                       wa::Image::ROTATION_0); // rear sensor default ROTATION_0
    outImage.setImageWH(width, height);

    return outImage;
}

ChiRectINT MiFusionPlugin::covCropInfoAcordAspectRatio(ChiRectINT cropRegion,
                                                       ImageFormat imageFormat, ChiRect activeSize,
                                                       bool isQcfaSensor)
{
    // adjust crop acording to aspect ratio.
    if (isQcfaSensor == true) {
        activeSize.left >>= 1;
        activeSize.top >>= 1;
        activeSize.width >>= 1;
        activeSize.height >>= 1;

        cropRegion.left >>= 1;
        cropRegion.top >>= 1;
        cropRegion.width >>= 1;
        cropRegion.height >>= 1;
    }
    MLOGI(Mia2LogGroupPlugin, "Aspect cropRegion [%d, %d, %d, %d]", cropRegion.left, cropRegion.top,
          cropRegion.width, cropRegion.height);
    int32_t rectLeft, rectTop;
    int32_t rectWidth, rectHeight;
    int32_t imageWide = static_cast<int32_t>(imageFormat.width);
    int32_t imageHeight = static_cast<int32_t>(imageFormat.height);
    int32_t refWidth = activeSize.width;
    int32_t refHeight = activeSize.height;

    // now calulate center cropregion
    int32_t center_left = (refWidth - cropRegion.width) / 2;
    int32_t center_top = (refHeight - cropRegion.height) / 2;
    // calculate the shift base on active size
    ImageShiftData shift = {cropRegion.left - center_left, cropRegion.top - center_top};
    // calculate aspect ratio shift
    shift.horizonalShift = shift.horizonalShift * imageWide / refWidth;
    shift.verticalShift = shift.verticalShift * imageHeight / refHeight;
    // calculate aspect ratio crop
    rectWidth = cropRegion.width * imageWide / refWidth;
    rectLeft = (imageWide - rectWidth) / 2;
    rectHeight = cropRegion.height * imageHeight / refHeight;
    rectTop = (imageHeight - rectHeight) / 2;

    rectLeft += shift.horizonalShift;
    rectTop += shift.verticalShift;

    // now we validate the rect, and adjust it if out-of-bondary found.
    if (rectLeft < 0) {
        MLOGW(Mia2LogGroupPlugin, "crop region left is wrong, we ajusted it manually!! left=%d",
              rectLeft);
        rectLeft = 0;
    }
    if (rectTop < 0) {
        MLOGW(Mia2LogGroupPlugin, "crop region top is wrong, we ajusted it manually!! top=%d",
              rectTop);
        rectTop = 0;
    }
    if (rectLeft + rectWidth > imageWide) {
        if (rectWidth <= imageWide) {
            rectLeft = imageWide - rectWidth;
        } else {
            rectLeft = 0;
            rectWidth = imageWide;
        }
        MLOGW(Mia2LogGroupPlugin, "crop region left or width is wrong, we ajusted it manually!!");
    }
    if (rectTop + rectHeight > imageHeight) {
        if (rectHeight <= imageHeight) {
            rectTop = imageHeight - rectHeight;
        } else {
            rectTop = 0;
            rectHeight = imageHeight;
        }
        MLOGW(Mia2LogGroupPlugin, "crop region top or height is wrong, we ajusted it manually!!");
    }
    MLOGI(Mia2LogGroupPlugin, "validate the rect, cropRegion [%d, %d, %d, %d]", rectLeft, rectTop,
          rectWidth, rectHeight);
    return {rectLeft, rectTop, static_cast<uint32_t>(rectWidth), static_cast<uint32_t>(rectHeight)};
}

void MiFusionPlugin::mapRectToInputData(ChiRect &rtSrcRect, ChiRect activeSize,
                                        ChiRectINT &asptCropRegion, ImageShiftData shifData,
                                        ImageFormat imageFormat, ChiRect &rtDstRectOnInputData)
{
    int32_t nDataWidth = static_cast<int32_t>(imageFormat.width);
    int32_t nDataHeight = static_cast<int32_t>(imageFormat.height);
    MLOGD(Mia2LogGroupPlugin,
          "rtSrcRect w:%d, h:%d, rtIFERect w:%d, h:%d, nDataWidth:%d, nDataHeight:%d",
          rtSrcRect.width, rtSrcRect.height, asptCropRegion.width, asptCropRegion.height,
          nDataWidth, nDataHeight);
    if (rtSrcRect.width <= 0 || rtSrcRect.height <= 0 || asptCropRegion.width <= 0 ||
        asptCropRegion.height <= 0 || nDataWidth <= 0 || nDataHeight <= 0)
        return;
    int32_t refWidth = activeSize.width;   // 4// 4:3 wide
    int32_t refHeight = activeSize.height; // 4:3 height
    ChiRect SrcRect = rtSrcRect;
    // now calulate center cropregion
    int32_t center_left = (refWidth - SrcRect.width) / 2;
    int32_t center_top = (refHeight - SrcRect.height) / 2;
    // calculate the shift base on active size
    ImageShiftData shift = {(int)SrcRect.left - center_left, (int)SrcRect.top - center_top};
    // calculate aspect ratio shift
    shift.horizonalShift = shift.horizonalShift * nDataWidth / refWidth;
    shift.verticalShift = shift.verticalShift * nDataHeight / refHeight;

    int32_t rectLeft, rectTop;
    rectLeft = (nDataWidth - SrcRect.width) / 2;
    rectTop = (nDataHeight - SrcRect.height) / 2;
    rectLeft += shift.horizonalShift;
    rectTop += shift.verticalShift;

    rectLeft -= asptCropRegion.left;
    rectTop -= asptCropRegion.top;

    rectLeft -= shifData.horizonalShift;
    rectTop -= shifData.verticalShift;

    float fLRRatio = nDataWidth / (float)asptCropRegion.width;
    int iTBOffset = (((int)asptCropRegion.height) -
                     (int)asptCropRegion.width * (int)nDataHeight / (int)nDataWidth) /
                    2;
    if (iTBOffset < 0)
        iTBOffset = 0;

    rtDstRectOnInputData.left = round(rectLeft * fLRRatio);
    rtDstRectOnInputData.top = round((rectTop - iTBOffset) * fLRRatio);
    rtDstRectOnInputData.width = floor(SrcRect.width * fLRRatio);
    rtDstRectOnInputData.height = floor(SrcRect.height * fLRRatio);
    rtDstRectOnInputData.width &= 0xFFFFFFFE;
    rtDstRectOnInputData.height &= 0xFFFFFFFE;

    MLOGD(Mia2LogGroupPlugin,
          "capture fusion rtSrcRect(l,t,w,h)= [%d,%d,%d,%d], rtIFERect(l,t,w,h)= [%d,%d,%d,%d], "
          "rtDstRectOnInputData[%d,%d,%d,%d]",
          rtSrcRect.left, rtSrcRect.top, rtSrcRect.width, rtSrcRect.height, asptCropRegion.left,
          asptCropRegion.top, asptCropRegion.width, asptCropRegion.height,
          rtDstRectOnInputData.left, rtDstRectOnInputData.top, rtDstRectOnInputData.width,
          rtDstRectOnInputData.height);
}

void MiFusionPlugin::writeFileLog(char *file, const char *fmt, ...)
{
    FILE *pf = fopen(file, "ab+");
    if (pf) {
        va_list ap;
        char buf[256] = {0};
        va_start(ap, fmt);
        vsnprintf(buf, 256, fmt, ap);
        va_end(ap);
        fwrite(buf, strlen(buf) * sizeof(char), sizeof(char), pf);
        fclose(pf);
    }
}
