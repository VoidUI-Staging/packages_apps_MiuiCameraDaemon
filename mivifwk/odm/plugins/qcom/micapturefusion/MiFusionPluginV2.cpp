#include "MiFusionPluginV2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dualcam_fusion_proc.h"

#undef LOG_TAG
#define LOG_TAG "MIA_N_CPFUSION"

#define CALI_DATA_LEN_2048 2048
#define CALI_DATA_LEN_4096 4096

using namespace mialgo2;

static CaliConfigInfo sCaliConfigInfo[2] = {{"com.xiaomi.dcal.wu.data", CALIB_TYPE_W_U},
                                            {"com.xiaomi.dcal.wt.data", CALIB_TYPE_W_T}};

static const int32_t sIsDumpData = property_get_int32("persist.vendor.camera.fusion.dump", 0);
static const int32_t sBCopyImage = property_get_int32("persist.vendor.camera.fusion.copyimage", 0);

static int readDataFromFile(const char *path, void **buf)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        MLOGE(Mia2LogGroupPlugin, "open file(%s) failed.", path);
        return 0;
    }

    int status = fseek(fp, 0, SEEK_END);
    if (0 != status) {
        MLOGE(Mia2LogGroupPlugin,
              "seek to file (%s) end failed: %d", path, status);
        fclose(fp);
        return 0;
    }
    int fileSize = ftell(fp);
    if (0 >= fileSize) {
        MLOGE(Mia2LogGroupPlugin,
              "calculate file (%s) size failed: %d", path, fileSize);
        fclose(fp);
        return 0;
    }
    if (NULL != buf) {
        status = fseek(fp, 0, SEEK_SET);
        if (0 != status) {
            MLOGE(Mia2LogGroupPlugin,
                  "seek to file (%s) begin failed: %d", path, status);
            fclose(fp);
            return 0;
        }
        *buf = (void *)malloc(fileSize);
        int size = fread(*buf, 1, fileSize, fp);
        if (size != fileSize) {
            MLOGE(Mia2LogGroupPlugin,
                  "file size: %d and readed size: %d not equal", fileSize, size);
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
    int ret = MIA_RETURN_OK;
    m_pFusionHandle = NULL;
    m_pMasterPhyMetadata = NULL;
    m_pSlaverPhyMetadata = NULL;
    m_fusionNodeInterface = nodeInterface;
    AlgoFusionLaunchParams launchParams = {};

    // get fusion algorithm version
    DualCamVersion_Fusion();

    // set calibration buffer
    char filename[FILE_PATH_LENGTH];
    snprintf(filename, sizeof(filename), "%s/%s", "/data/vendor/camera",
             sCaliConfigInfo[1].path.c_str());
    int size = readDataFromFile(filename, &m_pCaliData[1]);
    MLOGI(Mia2LogGroupPlugin, "MiFusion read calibration data type:%d, size:%d",
          sCaliConfigInfo[1].type, size);

    if (size == CALI_DATA_LEN_2048 || size == CALI_DATA_LEN_4096) {
        launchParams.pCalibBuf = static_cast<unsigned char *>(m_pCaliData[1]);
    } else {
        MLOGE(Mia2LogGroupPlugin,
              "mifusion init error : filr(%s) size(%d) diff cali size(%d or %d)", filename, size,
              CALI_DATA_LEN_2048, CALI_DATA_LEN_4096);
        if (NULL != m_pCaliData[1]) {
            free(m_pCaliData[1]);
            m_pCaliData[1] = NULL;
        }
        return MIA_RETURN_INVALID_PARAM;
    }

    char jsonFileName[FILE_PATH_LENGTH] = "vendor/etc/camera/dualcam_fusion_params_K1.json";
    int mainImgW = 4080;
    int mainImgH = 3060;
    // int mainImgS = 4080;
    int auxImgW = 4000;
    int auxImgH = 3000;
    // int auxImgS = 4000;

    launchParams.mainImgW = mainImgW;
    launchParams.mainImgH = mainImgH;
    launchParams.auxImgW = auxImgW;
    launchParams.auxImgH = auxImgH;
    launchParams.jsonFileName = jsonFileName;

    ret = MIALGO_FusionInitWhenLaunch(&m_pFusionHandle, &launchParams);
    if (MIA_RETURN_OK != ret) {
        MLOGE(Mia2LogGroupPlugin, "MiFusion Err: MIALGO_DualCamInitWhenLaunch err with ret:%d\n",
              ret);
    }

    MLOGI(Mia2LogGroupPlugin, "MiFusion initialize ret:%d", ret);
    return ret;
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

    int32_t iIsDumpData = sIsDumpData;
    int32_t bCopyImage = sBCopyImage;

    m_debugDumpImage = iIsDumpData;
    m_processedFrame = pProcessRequestInfo->frameNum;
    settings.metadata = pProcessRequestInfo->phInputBuffer[0].pMetadata;
    int inputBufferSize = pProcessRequestInfo->phInputBuffer.size();

    MiImageBuffer inputFrameMaster = { 0 },
                  inputFrameSlave = { 0 },
                  outputFrame = { 0 };
    inputFrameMaster.format = pProcessRequestInfo->phInputBuffer[0].format.format;
    inputFrameMaster.width = pProcessRequestInfo->phInputBuffer[0].format.width;
    inputFrameMaster.height = pProcessRequestInfo->phInputBuffer[0].format.height;
    inputFrameMaster.plane[0] = pProcessRequestInfo->phInputBuffer[0].pAddr[0];
    inputFrameMaster.plane[1] = pProcessRequestInfo->phInputBuffer[0].pAddr[1];
    inputFrameMaster.stride = pProcessRequestInfo->phInputBuffer[0].planeStride;
    inputFrameMaster.scanline = pProcessRequestInfo->phInputBuffer[0].sliceheight;
    // inputFrameMaster.numberOfPlanes = pProcessRequestInfo->phOutputBuffer[0].numberOfPlanes;
    m_pMasterPhyMetadata = pProcessRequestInfo->phInputBuffer[0].pPhyCamMetadata;
    MLOGI(Mia2LogGroupPlugin, "mifusion inputFrameMaster:format:%d %d*%d masterPhyMeta:%p",
          inputFrameMaster.format, inputFrameMaster.width, inputFrameMaster.height,
          m_pMasterPhyMetadata);

    if (inputFrameMaster.plane[0] == NULL || inputFrameMaster.plane[1] == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s:%d wrong input", __func__, __LINE__);
    }

    if (inputBufferSize > 1 && isEnabled(settings) == true) {
        inputFrameSlave.format = pProcessRequestInfo->phInputBuffer[1].format.format;
        inputFrameSlave.width = pProcessRequestInfo->phInputBuffer[1].format.width;
        inputFrameSlave.height = pProcessRequestInfo->phInputBuffer[1].format.height;
        inputFrameSlave.plane[0] = pProcessRequestInfo->phInputBuffer[1].pAddr[0];
        inputFrameSlave.plane[1] = pProcessRequestInfo->phInputBuffer[1].pAddr[1];
        inputFrameSlave.stride = pProcessRequestInfo->phInputBuffer[1].planeStride;
        inputFrameSlave.scanline = pProcessRequestInfo->phInputBuffer[1].sliceheight;
        // inputFrameSlave.numberOfPlanes = pProcessRequestInfo->phOutputBuffer[1].numberOfPlanes;
        m_pSlaverPhyMetadata = pProcessRequestInfo->phInputBuffer[1].pPhyCamMetadata;
        MLOGI(Mia2LogGroupPlugin, "mifusion inputFrameSlave:format:%d %d*%d slavePhyMeta:%p",
              inputFrameSlave.format, inputFrameSlave.width, inputFrameSlave.height,
              m_pSlaverPhyMetadata);
        if (inputFrameSlave.plane[0] == NULL || inputFrameSlave.plane[1] == NULL) {
            MLOGE(Mia2LogGroupPlugin, "%s:%d wrong input", __func__, __LINE__);
        }
    }

    outputFrame.format = pProcessRequestInfo->phOutputBuffer[0].format.format;
    outputFrame.width = pProcessRequestInfo->phOutputBuffer[0].format.width;
    outputFrame.height = pProcessRequestInfo->phOutputBuffer[0].format.height;
    outputFrame.plane[0] = pProcessRequestInfo->phOutputBuffer[0].pAddr[0];
    outputFrame.plane[1] = pProcessRequestInfo->phOutputBuffer[0].pAddr[1];
    outputFrame.stride = pProcessRequestInfo->phOutputBuffer[0].planeStride;
    outputFrame.scanline = pProcessRequestInfo->phOutputBuffer[0].sliceheight;
    outputFrame.numberOfPlanes = pProcessRequestInfo->phOutputBuffer[0].numberOfPlanes;
    if (outputFrame.plane[0] == NULL || outputFrame.plane[1] == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s:%d mifusion wrong output", __func__, __LINE__);
    }

    if (bCopyImage == 1 || inputBufferSize == 1 || isEnabled(settings) == false) {
        ret = PluginUtils::miCopyBuffer(&outputFrame, &inputFrameMaster);
        m_algoRunResult = -1;
    } else {
        ret = processBuffer(&inputFrameMaster, &inputFrameSlave, &outputFrame, settings.metadata);
        if (ret != MIA_RETURN_OK) {
            MLOGD(Mia2LogGroupPlugin, "Mifusion processBuffer failed ret:%d, bypass fusion", ret);
            ret = PluginUtils::miCopyBuffer(&outputFrame, &inputFrameMaster);
            m_algoRunResult = -1;
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
        uint32_t masterScaleInt = m_masterScale;
        uint32_t masterScaleDec = (m_masterScale - masterScaleInt) * 100;
        uint32_t slaverScaleInt = m_slaverScale;
        uint32_t slaverScaleDec = (m_slaverScale - slaverScaleInt) * 100;
        snprintf(outputbuf, sizeof(outputbuf), "mifusion_output_%dx%d_wide%d-%d_tele%d-%d_ret%d",
                 outputFrame.width, outputFrame.height, masterScaleInt, masterScaleDec,
                 slaverScaleInt, slaverScaleDec, m_algoRunResult);
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
    int ret = MIALGO_FusionDestoryEachPic(&m_pFusionHandle);
    MLOGD(Mia2LogGroupPlugin, "MiFusion flush each picture handle ret:%d\n", ret);
    return 0;
}

void MiFusionPlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, "%p MiFusionPlugin release", this);

    int ret = MIALGO_FusionDestoryEachPic(&m_pFusionHandle);
    MLOGD(Mia2LogGroupPlugin, "Mifusion destroy each picture handle ret:%d\n", ret);

    const uint32_t kCaliCount = sizeof(sCaliConfigInfo) / sizeof(sCaliConfigInfo[0]);
    for (int i = 0; i < kCaliCount && i < MAX_CALI_CNT; i++) {
        if (NULL != m_pCaliData[i]) {
            free(m_pCaliData[i]);
            m_pCaliData[i] = NULL;
        }
    }

    if (m_pFusionHandle != NULL) {
        free(m_pFusionHandle);
        m_pFusionHandle = NULL;
    }
}

CDKResult MiFusionPlugin::processBuffer(MiImageBuffer *masterImage, MiImageBuffer *slaveImage,
                                        MiImageBuffer *outputFusion, camera_metadata_t *pMetaData)
{
    if (masterImage == NULL || slaveImage == NULL || outputFusion == NULL || pMetaData == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s error invalid param %p %p %p %p", __func__, masterImage,
              slaveImage, outputFusion, pMetaData);
        return MIA_RETURN_INVALID_PARAM;
    }

    int result = MIA_RETURN_OK;
    double startTime = PluginUtils::nowMSec();

    // get image orientation
    int orientation = 0;
    void *pData = NULL;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_JPEG_ORIENTATION, &pData);
    if (NULL != pData) {
        orientation = *static_cast<int *>(pData);
    }
    MLOGD(Mia2LogGroupPlugin, "Mifusion main image orietation: %d", orientation);

    // set algo input buffer and params
    if (MIA_RETURN_OK == result) {
        result = setFusionParams(masterImage, slaveImage, pMetaData);
        if (MIA_RETURN_OK != result) {
            MLOGE(Mia2LogGroupPlugin, "Mifusion SetAlgoInputParams failed:%d", result);
            return result;
        }
    }

    // algo process
    AlgoFusionImg resultImageYUV = conversionImage(outputFusion);
    result = MIALGO_FusionProc(&m_pFusionHandle, &resultImageYUV);
    if (0 == result) {
        MLOGD(Mia2LogGroupPlugin, "Mifusion algo run sucess, ret: %d", result);
    } else {
        MLOGE(Mia2LogGroupPlugin, "Mifusion algo run failed, ret: %d", result);
    }
    m_algoRunResult = result;

    MLOGI(Mia2LogGroupPlugin, "Mifusion algrithm time: frameNum %d costTime = %.2f",
          m_processedFrame, PluginUtils::nowMSec() - startTime);

    return result;
}

CDKResult MiFusionPlugin::setFusionParams(MiImageBuffer *inputMaster, MiImageBuffer *inputSlave,
                                          camera_metadata_t *pMeta)
{
    int result = MIA_RETURN_OK;
    MiFusionInputMeta inputMetaDC;
    memset(&inputMetaDC, 0, sizeof(MiFusionInputMeta));
    result = fillInputMeta(&inputMetaDC, pMeta);
    if (result != MIA_RETURN_OK) {
        MLOGE(Mia2LogGroupPlugin, "Mifusion fillInputMeta failed case get meta error");
        return result;
    }

    int32_t masterIndex = 0;
    int32_t slaveIndex = 1;
    for (int index = 0; index < MAX_CAMERA_CNT; index++) {
        if (inputMetaDC.cameraMeta[index].cameraId ==
            inputMetaDC.cameraMeta[index].masterCameraId) {
            masterIndex = index;
        } else {
            slaveIndex = index;
        }
    }

    // calculate the scale
    int32_t masterActiveSizeWidth = 0;
    int32_t slaveActiveSizeWidth = 0;
    int32_t masterScaleWidth = 0;
    int32_t slaveScaleWidth = 0;
    WeightedRegion masterFocusRegion;
    WeightedRegion auxFocusRegion;
    masterActiveSizeWidth = inputMetaDC.cameraMeta[masterIndex].activeArraySize.width;
    masterScaleWidth = inputMetaDC.cameraMeta[masterIndex].userCropRegion.width;
    masterFocusRegion = inputMetaDC.cameraMeta[masterIndex].afFocusROI;
    slaveActiveSizeWidth = inputMetaDC.cameraMeta[slaveIndex].activeArraySize.width;
    slaveScaleWidth = inputMetaDC.cameraMeta[slaveIndex].userCropRegion.width;
    auxFocusRegion = inputMetaDC.cameraMeta[slaveIndex].afFocusROI;

    MLOGI(Mia2LogGroupPlugin,
          "Mifusion mainActiveWidth=%d, mainScaleWidth=%d auxActiveWidth=%d, auxScaleWidth=%d",
          masterActiveSizeWidth, masterScaleWidth, slaveActiveSizeWidth, slaveScaleWidth);

    float currentUserZoomRatio = inputMetaDC.currentZoomRatio;
    float masterScale = 1.0f;
    float slaveScale = 1.0f;
    masterScale = masterActiveSizeWidth * 1.0 / masterScaleWidth;
    slaveScale = slaveActiveSizeWidth * 1.0 / slaveScaleWidth;
    MLOGI(Mia2LogGroupPlugin, "Mifusion user zoom: %.3f, scale master: %.3f slave: %.3f",
          currentUserZoomRatio, masterScale, slaveScale);
    m_masterScale = masterScale;
    m_slaverScale = slaveScale;

    // set fusion algo input param
    AlgoFusionImg mainImageYUV = conversionImage(inputMaster);
    AlgoFusionImg auxImageYUV = conversionImage(inputSlave);
    AlgoFusionInitParams initParams = {};
    initParams.pMainImg = &mainImageYUV;
    initParams.pAuxImg = &auxImageYUV;
    initParams.cameraParams.scale.mainScale = masterScale;
    initParams.cameraParams.scale.auxScale = slaveScale;
    initParams.cameraParams.scale.zoomRatio = currentUserZoomRatio;
    initParams.cameraParams.main.luma = inputMetaDC.cameraMeta[masterIndex].lux;
    initParams.cameraParams.aux.luma = inputMetaDC.cameraMeta[slaveIndex].lux;

    void *pData = NULL;
    uint32_t satDistance = MI_SAT_DEFAULT_DISTANCE;
    VendorMetadataParser::getVTagValue(pMeta, "xiaomi.capturefusion.satDistance", &pData);
    if (NULL != pData) {
        satDistance = *static_cast<uint32_t *>(pData);
        initParams.cameraParams.main.distance = satDistance * 10.0;
        initParams.cameraParams.aux.distance = satDistance * 10.0;
    } else {
        // use master focus distance
        initParams.cameraParams.main.distance =
            inputMetaDC.cameraMeta[masterIndex].focusDistCm * 10;
        initParams.cameraParams.aux.distance = inputMetaDC.cameraMeta[slaveIndex].focusDistCm * 10;
        MLOGE(Mia2LogGroupPlugin, "mifusion get satDistance failed, use focusDistCm");
    }

    if (inputMetaDC.cameraMeta[masterIndex].afState ==
            static_cast<uint32_t>(MiAFState::PassiveFocused) ||
        inputMetaDC.cameraMeta[masterIndex].afState ==
            static_cast<uint32_t>(MiAFState::FocusedLocked)) {
        initParams.cameraParams.main.isAfStable = true;
    }

    if (inputMetaDC.cameraMeta[slaveIndex].afState ==
            static_cast<uint32_t>(MiAFState::PassiveFocused) ||
        inputMetaDC.cameraMeta[slaveIndex].afState ==
            static_cast<uint32_t>(MiAFState::FocusedLocked)) {
        initParams.cameraParams.aux.isAfStable = true;
    }

    bool isQcfaSensor = 0;
    ChiRectINT masterCropRegion;
    ChiRectINT slaveCropRegion;
    ChiRectINT masterAspectCropRegion;
    ChiRectINT slaveAspectCropRegion;
    ImageFormat masterFormat;
    ImageFormat slaveFormat;
    ChiRect masterActivesize = inputMetaDC.cameraMeta[masterIndex].activeArraySize;
    ChiRect slaveActivesize = inputMetaDC.cameraMeta[slaveIndex].activeArraySize;

    isQcfaSensor = inputMetaDC.cameraMeta[masterIndex].isQuadCFASensor;
    masterCropRegion.left = inputMetaDC.cameraMeta[masterIndex].userCropRegion.left;
    masterCropRegion.top = inputMetaDC.cameraMeta[masterIndex].userCropRegion.top;
    masterCropRegion.width = inputMetaDC.cameraMeta[masterIndex].userCropRegion.width;
    masterCropRegion.height = inputMetaDC.cameraMeta[masterIndex].userCropRegion.height;

    masterFormat.format = inputMaster->format;
    masterFormat.width = inputMaster->width;
    masterFormat.height = inputMaster->height;
    masterAspectCropRegion =
        covCropInfoAcordAspectRatio(masterCropRegion, masterFormat, masterActivesize, isQcfaSensor);
    initParams.cameraParams.main.cropRect.x = masterAspectCropRegion.left;
    initParams.cameraParams.main.cropRect.y = masterAspectCropRegion.top;
    initParams.cameraParams.main.cropRect.width = masterAspectCropRegion.width;
    initParams.cameraParams.main.cropRect.height = masterAspectCropRegion.height;

    MLOGI(Mia2LogGroupPlugin,
          "Mifusion master Scale: %.3f, luma: %.3f distance: %.3f, afStable %d, "
          "cropRect [%d, %d, %d, %d], isQCFA %d, frameNum:%d",
          initParams.cameraParams.scale.mainScale, initParams.cameraParams.main.luma,
          initParams.cameraParams.main.distance, initParams.cameraParams.main.isAfStable,
          initParams.cameraParams.main.cropRect.x, initParams.cameraParams.main.cropRect.y,
          initParams.cameraParams.main.cropRect.width, initParams.cameraParams.main.cropRect.height,
          isQcfaSensor, m_processedFrame);

    isQcfaSensor = inputMetaDC.cameraMeta[slaveIndex].isQuadCFASensor;
    slaveCropRegion.left = inputMetaDC.cameraMeta[slaveIndex].userCropRegion.left;
    slaveCropRegion.top = inputMetaDC.cameraMeta[slaveIndex].userCropRegion.top;
    slaveCropRegion.width = inputMetaDC.cameraMeta[slaveIndex].userCropRegion.width;
    slaveCropRegion.height = inputMetaDC.cameraMeta[slaveIndex].userCropRegion.height;

    slaveFormat.format = inputMaster->format;
    slaveFormat.width = inputMaster->width;
    slaveFormat.height = inputMaster->height;
    slaveAspectCropRegion =
        covCropInfoAcordAspectRatio(slaveCropRegion, slaveFormat, slaveActivesize, isQcfaSensor);
    initParams.cameraParams.aux.cropRect.x = slaveAspectCropRegion.left;
    initParams.cameraParams.aux.cropRect.y = slaveAspectCropRegion.top;
    initParams.cameraParams.aux.cropRect.width = slaveAspectCropRegion.width;
    initParams.cameraParams.aux.cropRect.height = slaveAspectCropRegion.height;

    MLOGI(Mia2LogGroupPlugin,
          "Mifusion slaver Scale: %.3f, luma: %.3f distance: %.3f, afStable %d, "
          "cropRect [%d, %d, %d, %d], isQCFA %d, frameNum:%d",
          initParams.cameraParams.scale.auxScale, initParams.cameraParams.aux.luma,
          initParams.cameraParams.aux.distance, initParams.cameraParams.aux.isAfStable,
          initParams.cameraParams.aux.cropRect.x, initParams.cameraParams.aux.cropRect.y,
          initParams.cameraParams.aux.cropRect.width, initParams.cameraParams.aux.cropRect.height,
          isQcfaSensor, m_processedFrame);

    // set focus center point
    ChiRectINT dstFocusRect;
    memset(&dstFocusRect, 0, sizeof(ChiRectINT));
    ChiRectINT focusRect = {masterFocusRegion.xMin, masterFocusRegion.yMin,
                            (uint32_t)(masterFocusRegion.xMax - masterFocusRegion.xMin + 1),
                            (uint32_t)(masterFocusRegion.yMax - masterFocusRegion.yMin + 1)};
    dstFocusRect =
        covCropInfoAcordAspectRatio(focusRect, masterFormat, masterActivesize, isQcfaSensor);
    initParams.fp.x = dstFocusRect.left + dstFocusRect.width / 2;
    initParams.fp.y = dstFocusRect.top + dstFocusRect.height / 2;
    MLOGI(Mia2LogGroupPlugin, "Mifusion focus roi: [%d, %d, %d, %d] set fp:[%d, %d]",
          dstFocusRect.left, dstFocusRect.top, dstFocusRect.width, dstFocusRect.height,
          initParams.fp.x, initParams.fp.y);

    // dump algo input buffer and params
    if (m_debugDumpImage) {
        char inputbufMaster[128];
        char inputbufSlaver[128];
        uint32_t masterScaleInt = m_masterScale;
        uint32_t masterScaleDec = (m_masterScale - masterScaleInt) * 100;
        snprintf(inputbufMaster, sizeof(inputbufMaster), "mifusion_input_master_%dx%d_scale%d-%d",
                 inputMaster->width, inputMaster->height, masterScaleInt, masterScaleDec);
        PluginUtils::dumpToFile(inputbufMaster, inputMaster);

        uint32_t slaverScaleInt = m_slaverScale;
        uint32_t slaverScaleDec = (m_slaverScale - slaverScaleInt) * 100;
        snprintf(inputbufSlaver, sizeof(inputbufSlaver), "mifusion_input_slave_%dx%d_scale%d-%d",
                 inputSlave->width, inputSlave->height, slaverScaleInt, slaverScaleDec);
        PluginUtils::dumpToFile(inputbufSlaver, inputSlave);

        char dumpInfoBuf[128] = "mifusion_dumpInfo";
        PluginUtils::dumpStrToFile(
            dumpInfoBuf,
            "mS%.2f\nmC[%d,%d,%d,%d]\nmL%.1f\nsS%.2f\nsC[%d,%d,%d,%d]\nsL%.1f\n"
            "D:%.1f\n",
            initParams.cameraParams.scale.mainScale, initParams.cameraParams.main.cropRect.x,
            initParams.cameraParams.main.cropRect.y, initParams.cameraParams.main.cropRect.width,
            initParams.cameraParams.main.cropRect.height, initParams.cameraParams.main.luma,
            initParams.cameraParams.scale.auxScale, initParams.cameraParams.aux.cropRect.x,
            initParams.cameraParams.aux.cropRect.y, initParams.cameraParams.aux.cropRect.width,
            initParams.cameraParams.aux.cropRect.height, initParams.cameraParams.aux.luma,
            initParams.cameraParams.main.distance);
    }

    result = MIALGO_FusionInitEachPic(&m_pFusionHandle, &initParams);
    if (result != MIA_RETURN_OK) {
        MLOGE(Mia2LogGroupPlugin, "Mifusion InitEachPic failed! error code:%d", result);
    }

    return result;
}

CDKResult MiFusionPlugin::fillInputMeta(MiFusionInputMeta *pInputMeta, camera_metadata_t *pMeta)
{
    CDKResult result = MIA_RETURN_OK;
    void *pData = NULL;
    int masterCameraId = 0;
    int masterIndex = 0;
    int slaverIndex = 0;
    std::vector<int> cameraIndexTable;

    // get MasterPhyMetadata camera index
    result = VendorMetadataParser::getVTagValue(
        m_pMasterPhyMetadata, "com.qti.chi.metadataOwnerInfo.MetadataOwner", &pData);
    if (NULL != pData) {
        masterIndex = *static_cast<int32_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "Mifusion get MasterPhyMetadata camera index:%d", masterIndex);
    }
    cameraIndexTable.push_back(masterIndex);

    // get SlaverPhyMetadata camera index
    pData = NULL;
    result = VendorMetadataParser::getVTagValue(
        m_pSlaverPhyMetadata, "com.qti.chi.metadataOwnerInfo.MetadataOwner", &pData);
    if (NULL != pData) {
        slaverIndex = *static_cast<int32_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "Mifusion SlaverPhyMetadata camera index:%d", slaverIndex);
    }
    cameraIndexTable.push_back(slaverIndex);

    // to be sure masterCameraId
    pData = NULL;
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
        MLOGE(Mia2LogGroupPlugin, "Mifusion get master cameraId failed!");
    }

    for (int cameraIndex = 0; cameraIndex < MAX_CAMERA_CNT; cameraIndex++) {
        camera_metadata_t *pPhyMeta = NULL;
        pInputMeta->cameraMeta[cameraIndex].cameraId = cameraIndexTable[cameraIndex];
        pInputMeta->cameraMeta[cameraIndex].masterCameraId = masterCameraId;
        if (cameraIndexTable[cameraIndex] == masterCameraId) {
            pPhyMeta = m_pMasterPhyMetadata;
        } else {
            pPhyMeta = m_pSlaverPhyMetadata;
        }

        pData = NULL;
        ChiRect cropRegion = {};
        VendorMetadataParser::getVTagValue(pPhyMeta, "xiaomi.capturefusion.cropInfo", &pData);
        if (NULL != pData) {
            cropRegion = *static_cast<ChiRect *>(pData);
            cropRegion.left >>= 1;
            cropRegion.top >>= 1;
            cropRegion.width >>= 1;
            cropRegion.height >>= 1;
        } else {
            MLOGE(Mia2LogGroupPlugin, "mifusion get android crop region failed !!");
        }
        pInputMeta->cameraMeta[cameraIndex].userCropRegion.left = cropRegion.left;
        pInputMeta->cameraMeta[cameraIndex].userCropRegion.top = cropRegion.top;
        pInputMeta->cameraMeta[cameraIndex].userCropRegion.width = cropRegion.width;
        pInputMeta->cameraMeta[cameraIndex].userCropRegion.height = cropRegion.height;
        MLOGI(Mia2LogGroupPlugin, "mifusion cameraIndex:%d usercropregion (%d,%d,%d,%d)",
              cameraIndex, pInputMeta->cameraMeta[cameraIndex].userCropRegion.left,
              pInputMeta->cameraMeta[cameraIndex].userCropRegion.top,
              pInputMeta->cameraMeta[cameraIndex].userCropRegion.width,
              pInputMeta->cameraMeta[cameraIndex].userCropRegion.height);

        pData = NULL;
        StreamCropInfo fovResCropInfo = {{0}};
        VendorMetadataParser::getVTagValue(pPhyMeta, "com.qti.camera.streamCropInfo.StreamCropInfo",
                                           &pData);
        pData = NULL;
        WeightedRegion afRegion = {0};
        VendorMetadataParser::getTagValue(pPhyMeta, ANDROID_CONTROL_AF_REGIONS, &pData);
        if (NULL != pData) {
            afRegion = *static_cast<WeightedRegion *>(pData);
        } else {
            MLOGD(Mia2LogGroupPlugin, "mifusion get ANDROID_CONTROL_AF_REGIONS failed !!");
        }

        /*
        pInputMeta->cameraMeta[cameraIndex].afFocusROI.xMin = afRegion.xMin >> 1;
        pInputMeta->cameraMeta[cameraIndex].afFocusROI.yMin = afRegion.yMin >> 1;
        pInputMeta->cameraMeta[cameraIndex].afFocusROI.xMax = afRegion.xMax >> 1;
        pInputMeta->cameraMeta[cameraIndex].afFocusROI.yMax = afRegion.yMax >> 1;
        pInputMeta->cameraMeta[cameraIndex].afFocusROI.weight = afRegion.weight;
        */

        MLOGI(Mia2LogGroupPlugin, "mifusion cameraIndex:%d afFocusROI (%d,%d,%d,%d) weight %d",
              cameraIndex, pInputMeta->cameraMeta[cameraIndex].afFocusROI.xMin,
              pInputMeta->cameraMeta[cameraIndex].afFocusROI.yMin,
              pInputMeta->cameraMeta[cameraIndex].afFocusROI.xMax,
              pInputMeta->cameraMeta[cameraIndex].afFocusROI.yMax,
              pInputMeta->cameraMeta[cameraIndex].afFocusROI.weight);

        pData = NULL;
        VendorMetadataParser::getTagValue(pPhyMeta, ANDROID_CONTROL_AF_STATE, &pData);
        if (NULL != pData) {
            pInputMeta->cameraMeta[cameraIndex].afState = *static_cast<uint32_t *>(pData);
        } else {
            MLOGD(Mia2LogGroupPlugin, "mifusion get ANDROID_CONTROL_AF_STATE failed !!");
        }

        pData = NULL;
        VendorMetadataParser::getTagValue(pPhyMeta, ANDROID_LENS_FOCUS_DISTANCE, &pData);
        if (NULL != pData) {
            float *pAFDistance = static_cast<float *>(pData);
            pInputMeta->cameraMeta[cameraIndex].focusDistCm =
                static_cast<uint32_t>(*pAFDistance > 0 ? (100.0f / *pAFDistance) : 1000.0f);
        } else {
            MLOGE(Mia2LogGroupPlugin, "mifusion get ANDROID_LENS_FOCUS_DISTANCE failed !!");
        }

        pData = NULL;
        float *pluxValue;
        VendorMetadataParser::getVTagValue(pPhyMeta, "com.qti.chi.statsaec.AecLux", &pData);
        if (NULL != pData) {
            pluxValue = static_cast<float *>(pData);
            pInputMeta->cameraMeta[cameraIndex].lux = *pluxValue;
        } else {
            MLOGE(Mia2LogGroupPlugin, "mifusion get AecLux failed !!");
        }

        pData = NULL;
        camera_metadata_t *staticMetadata =
            StaticMetadataWraper::getInstance()->getStaticMetadata(cameraIndexTable[cameraIndex]);
        result = VendorMetadataParser::getTagValue(staticMetadata,
                                                   ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &pData);
        if (result == 0 && NULL != pData) {
            ChiRect &activeArray = *static_cast<ChiRect *>(pData);
            pInputMeta->cameraMeta[cameraIndex].activeArraySize.left = activeArray.left;
            pInputMeta->cameraMeta[cameraIndex].activeArraySize.top = activeArray.top;
            pInputMeta->cameraMeta[cameraIndex].activeArraySize.width = activeArray.width;
            pInputMeta->cameraMeta[cameraIndex].activeArraySize.height = activeArray.height;
            MLOGI(Mia2LogGroupPlugin, "mifusion cameraIndex:%d activeArraySize (%d,%d,%d,%d)",
                  cameraIndex, pInputMeta->cameraMeta[cameraIndex].activeArraySize.left,
                  pInputMeta->cameraMeta[cameraIndex].activeArraySize.top,
                  pInputMeta->cameraMeta[cameraIndex].activeArraySize.width,
                  pInputMeta->cameraMeta[cameraIndex].activeArraySize.height);
        } else {
            MLOGE(Mia2LogGroupPlugin,
                  "mifusion failed to get ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE");
        }

        pData = NULL;
        bool isQuadCFASensor = true;
        VendorMetadataParser::getVTagValue(
            staticMetadata, "org.codeaurora.qcamera3.quadra_cfa.is_qcfa_sensor", &pData);
        if (NULL != pData) {
            isQuadCFASensor = *static_cast<bool *>(pData);
            pInputMeta->cameraMeta[cameraIndex].isQuadCFASensor = isQuadCFASensor;
        } else {
            MLOGE(Mia2LogGroupPlugin, "mifusion get is_qcfa_sensor failed !!");
        }

        MLOGI(Mia2LogGroupPlugin, "mifusion cameraindex:%d, afstate:%d, afdis:%d, lux:%f, QCFA:%d",
              cameraIndex, pInputMeta->cameraMeta[cameraIndex].afState,
              pInputMeta->cameraMeta[cameraIndex].focusDistCm,
              pInputMeta->cameraMeta[cameraIndex].lux,
              pInputMeta->cameraMeta[cameraIndex].isQuadCFASensor);
    }

    pData = NULL;
    VendorMetadataParser::getVTagValue(pMeta, "com.qti.chi.pixelshift.PixelShift", &pData);
    if (NULL != pData) {
        pInputMeta->pixelShift = *static_cast<ImageShiftData *>(pData);
        // this shift base to input imagesize
        MLOGI(Mia2LogGroupPlugin, "mifusion pixelShift (%d, %d)",
              pInputMeta->pixelShift.horizonalShift, pInputMeta->pixelShift.verticalShift);
    } else {
        MLOGD(Mia2LogGroupPlugin, "mifusion get pMeta PixelShift failed");
    }

    pData = NULL;
    result = VendorMetadataParser::getTagValue(pMeta, ANDROID_CONTROL_ZOOM_RATIO, &pData);
    if (NULL != pData) {
        pInputMeta->currentZoomRatio = *static_cast<float *>(pData);
    }

    return result;
}

AlgoFusionImg MiFusionPlugin::conversionImage(MiImageBuffer *imgInput)
{
    MLOGD(Mia2LogGroupPlugin,
          "PrepareImage width: %d, height: %d, planeStride: %d, sliceHeight:%d, plans: %d, format "
          "0x%x",
          imgInput->width, imgInput->height, imgInput->stride, imgInput->scanline,
          imgInput->numberOfPlanes, imgInput->format);

    AlgoFusionImg outImage = {0};

    if (imgInput->format == CAM_FORMAT_YUV_420_NV12) {
        outImage.format = FUSION_FORMAT_NV12;
    } else if (imgInput->format == CAM_FORMAT_YUV_420_NV21) {
        outImage.format = FUSION_FORMAT_NV21;
    } else {
        MLOGE(Mia2LogGroupPlugin, "Mifusion unsupported image format: %d", imgInput->format);
    }

    outImage.width = imgInput->width;
    outImage.height = imgInput->height;
    outImage.strideW = imgInput->stride;
    outImage.strideH = imgInput->scanline;
    outImage.pData = (void *)imgInput->plane[0];
    outImage.fd = imgInput->fd[0];

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
