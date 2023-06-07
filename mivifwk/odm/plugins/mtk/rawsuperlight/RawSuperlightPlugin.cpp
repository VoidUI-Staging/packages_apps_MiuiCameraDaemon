#include "RawSuperlightPlugin.h"
//#include "gr_priv_handle.h"
#include <mtkcam-halif/device/1.x/types.h>
#include <ui/gralloc_extra.h>

#undef LOG_TAG
#define LOG_TAG "RawSuperlightPlugin"

#define SN_DUMP_PATH "/data/vendor/camera_dump/"

using namespace mialgo2;
using NSCam::MRational;

static long long __attribute__((unused)) gettime()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((long long)time.tv_sec * 1000 + time.tv_usec / 1000);
}

RawSuperlightPlugin::~RawSuperlightPlugin()
{
    MLOGD(Mia2LogGroupPlugin, "[ARCSOFT]: now distruct -->> RawSuperlightPlugin\n");
    if (m_hArcsoftRawSuperlight) {
        delete m_hArcsoftRawSuperlight;
        m_hArcsoftRawSuperlight = NULL;
    }
    if (m_firstInput) {
        delete m_firstInput;
        m_firstInput = NULL;
    }
    if (m_copyFromInput) {
        delete m_copyFromInput;
        m_copyFromInput = NULL;
    }
    if (m_outputImgParam) {
        delete m_outputImgParam;
        m_outputImgParam = NULL;
    }
    if (m_curInputImgParam) {
        delete m_curInputImgParam;
        m_curInputImgParam = NULL;
    }

    m_flushing = false;
};

static void __attribute__((unused)) gettimestr(char *timestr, size_t size)
{
    time_t currentTime;
    struct tm *timeInfo;
    time(&currentTime);
    timeInfo = localtime(&currentTime);
    strftime(timestr, size, "%Y%m%d_%H%M%S", timeInfo);
}

int RawSuperlightPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    // 1. init
    m_processedFrame = 0;
    m_camState = ARC_SN_CAMERA_STATE_HAND;
    m_InitRet = -1;
    m_frameworkCameraId = 0;
    memset(&m_FaceParams, 0, sizeof(ARC_SN_FACEINFO));
    memset(&m_OutImgRect, 0, sizeof(MRECT));
    memset(&m_sensorActiveArray, 0, sizeof(MIRECT));
    memset(&m_format, 0, sizeof(ImageFormat));
    m_flushing = false;
    // m_format = createInfo->outputFormat;
    mNodeInterface = nodeInterface;
    mLSCDim.width = 0;
    mLSCDim.height = 0;
    m_nightMode = false;
    m_isOISSupport = false;
    m_processedFrame = 0;
    m_CopyFrameIndex = 2;
    m_FakeProcTime = 1;
    m_rawPattern = BGGR;
    m_devOrientation = 0;
    memset(&m_algoVersion, 0, sizeof(MPBASE_Version));
    m_outputImgParam = new ImageParams;
    m_curInputImgParam = new ImageParams;
    m_copyFromInput = new ImageParams;
    m_firstInput = new ImageParams;
    memset(m_outputImgParam, 0, sizeof(ImageParams));
    memset(m_curInputImgParam, 0, sizeof(ImageParams));
    memset(m_copyFromInput, 0, sizeof(ImageParams));
    memset(m_firstInput, 0, sizeof(ImageParams));
    m_nightMode = ARC_SN_CAMERA_STATE_HAND;
    m_tuningParam = {0};
    m_InputIndex = 0;
    m_flushFlag = false;

    // NOTE: because SCALE_MODE is support in mialgoengine now, the outputFormat (size) is from
    //       Sink Buffer, not the real outputFormat (size) of this node. Just use the inputFormat in
    //       createInfo instead.
    ImageFormat imageFormat = {CAM_FORMAT_RAW16, createInfo->inputFormat.width,
                               createInfo->inputFormat.height};
    mNodeInterface.pSetOutputFormat(mNodeInterface.owner, imageFormat);
    m_format = imageFormat;

    // 2. prepare resource
    char prop[PROPERTY_VALUE_MAX];
    // get m_dumpMask
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.sll.dump", prop, "0");
    m_dumpMask = atoi(prop);
    // get m_BypassForDebug
    property_get("persist.vendor.camera.sll.bypass", prop, "0");
    m_BypassForDebug = (int32_t)atoi(prop);
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: m_BypassForDebug = %d", m_BypassForDebug);

    // get m_isCropInfoUpdate
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.sll.update", prop, "1");
    m_isCropInfoUpdate = (int32_t)atoi(prop);

    // get m_camState
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.sll.state", prop, "2"); // ARC_SN_CAMERA_STATE_HAND
    m_camState = (int32_t)atoi(prop);

    /*debug for algo:
      0:disable
      1:enable*/
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.sll.algo", prop, "1");
    m_AlgoLogLevel = atoi(prop);

    m_isSENeeded = (createInfo->operationMode == StreamConfigModeMiuiSuperNight) ? false : true;

    uint32_t cameraId = createInfo->frameworkCameraId;
    m_frameworkCameraId = createInfo->frameworkCameraId;

    if (m_isSENeeded) {
        m_nightMode = m_camState = ARC_SN_CAMERA_STATE_AUTOMODE;
        m_frameworkCameraId = CAM_COMBMODE_REAR_WIDE;
        MLOGE(Mia2LogGroupPlugin, " SE mode, cameraId=%d", cameraId);
    } else {
        m_nightMode = m_camState = ARC_SN_CAMERA_STATE_HAND; // ARC_SN_CAMERA_STATE_AUTOMODE
        MLOGE(Mia2LogGroupPlugin, " night mode, cameraId=%d", cameraId);
    }

    m_hArcsoftRawSuperlight = NULL;
    if (!m_hArcsoftRawSuperlight && m_BypassForDebug != 2) {
        m_hArcsoftRawSuperlight = new ArcsoftRawSuperlight();
        // get m_CamMode
#if defined MATISSE_CAMERA
        if (m_isSENeeded == true) {
            m_CamMode = CAMERA_MODE_XIAOMI_L11_D2000_HM2_Y;
        } else {
            m_CamMode = (cameraId == CAM_COMBMODE_REAR_WIDE)
                            ? CAMERA_MODE_XIAOMI_L11_D2000_HM2_Y
                            : CAMERA_MODE_XIAOMI_L11_D2000_SKG4H7_N;
        }
#elif defined RUBENS_CAMERA || defined(YUECHU_CAMERA)
        if (m_isSENeeded == true) {
            m_CamMode = CAMERA_MODE_XIAOMI_L11A_D1500_IMX582_Y;
        } else {
            m_CamMode = (cameraId == CAM_COMBMODE_REAR_WIDE)
                            ? CAMERA_MODE_XIAOMI_L11A_D1500_IMX582_Y
                            : CAMERA_MODE_XIAOMI_L11A_D1500_SKG4H7_N;
        }
#elif defined PLATO_CAMERA
#ifdef PLATO_CAMERA_CN
        m_CamMode = (cameraId == CAM_COMBMODE_REAR_WIDE) ? CAMERA_MODE_XIAOMI_L12A_D1500_OV64B40
                                                         : CAMERA_MODE_XIAOMI_L12A_D1500_S5K4H7;
#else
        m_CamMode = (cameraId == CAM_COMBMODE_REAR_WIDE) ? CAMERA_MODE_XIAOMI_L12A_INT_D1500_S5KHM6
                                                         : CAMERA_MODE_XIAOMI_L12A_INT_D1500_S5K4H7;
#endif
#elif defined DAUMIER_CAMERA
        m_CamMode = (cameraId == CAM_COMBMODE_REAR_WIDE) ? CAMERA_MODE_XIAOMI_L2M_D9000_IMX707
                                                         : CAMERA_MODE_XIAOMI_L2M_D9000_OV13B;
#else
        m_CamMode = (cameraId == CAM_COMBMODE_REAR_WIDE) ? CAMERA_MODE_XIAOMI_L11_D2000_HM2_Y
                                                         : CAMERA_MODE_XIAOMI_L11_D2000_SKG4H7_N;
#endif
        if (m_InitRet != MOK) {
            // place init at the time of first frame arrived

            // m_hArcsoftRawSuperlight->init(m_CamMode, m_camState, m_format.width, m_format.height,
            //                              &m_algoVersion, m_AlgoLogLevel, m_pZoomRect,
            //                              m_i32LuxIndex);
            // m_hArcsoftRawSuperlight->setInitStatus(1);
        }
    }

    if (CAM_COMBMODE_FRONT == cameraId || CAM_COMBMODE_FRONT_BOKEH == cameraId ||
        CAM_COMBMODE_FRONT_AUX == cameraId) {
        m_bFrontCamera = true;
    } else {
        m_bFrontCamera = false;
    }

    memset(&m_ExtRawValue, 0, sizeof(m_ExtRawValue));
    return 0;
}

void RawSuperlightPlugin::destroy()
{
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: m_hArcsoftRawSuperlight:%d", m_hArcsoftRawSuperlight);
    if (m_hArcsoftRawSuperlight) {
        m_hArcsoftRawSuperlight->setInitStatus(0);
        m_hArcsoftRawSuperlight->uninit();
        m_InitRet = -1;
        delete m_hArcsoftRawSuperlight;
        m_hArcsoftRawSuperlight = NULL;
    }

    m_flushing = true;
}

ProcessRetStatus RawSuperlightPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    std::lock_guard<std::mutex> lock(m_flushMutex);
    ProcessRetStatus resultInfo = PROCSUCCESS;
    memset(bracketEv, 0, FRAME_MAX_INPUT_NUMBER * sizeof(bracketEv[0]));

    camera_metadata_t *bMetaData =
        (camera_metadata_t *)processRequestInfo->inputBuffersMap.begin()->second[0].pMetadata;
    if (bMetaData == NULL) {
        return resultInfo;
    }

    int inputNum = processRequestInfo->inputBuffersMap.begin()->second.size();
    int outputNum = processRequestInfo->outputBuffersMap.begin()->second.size();
    if (inputNum > FRAME_MAX_INPUT_NUMBER) {
        inputNum = FRAME_MAX_INPUT_NUMBER;
    }
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: get inputNum = %d,outputNum = %d", inputNum, outputNum);

    if (!inputNum) {
        MLOGE(Mia2LogGroupPlugin, "FrameNum %d port %d input buffer is NULL!",
              processRequestInfo->frameNum,
              processRequestInfo->inputBuffersMap.begin()->second[0].portId);
        resultInfo = PROCFAILED;
        return resultInfo;
    }

    if (processRequestInfo->isFirstFrame && !outputNum) {
        MLOGE(Mia2LogGroupPlugin, "FrameNum %d port %d as first frame, output buffer is NULL!",
              processRequestInfo->frameNum,
              processRequestInfo->outputBuffersMap.begin()->second[0].portId);
        resultInfo = PROCFAILED;
        return resultInfo;
    }

    if (m_flushFlag) {
        if (m_hArcsoftRawSuperlight) {
            m_hArcsoftRawSuperlight->setInitStatus(0);
            MLOGI(Mia2LogGroupPlugin, "set user cancel!");
        }
        setNextStat(PROCESSUINIT);
        processRequestStat();
        m_flushFlag = false;
        return resultInfo;
    }

    if (processRequestInfo->isFirstFrame && outputNum > 0) {
        resetSnapshotParam();
        // no need copy Platform Dependencies
        memcpy(m_outputImgParam, &processRequestInfo->outputBuffersMap.begin()->second[0],
               sizeof(ImageParams) - sizeof(void *));
        MLOGI(Mia2LogGroupPlugin,
              "FrameNum %d output Stride: %d, height: %d, format:%d logiMata:%p "
              "phyMeta:%p imgAddr:%p",
              processRequestInfo->frameNum, m_outputImgParam->planeStride,
              m_outputImgParam->sliceheight, m_outputImgParam->format.format,
              m_outputImgParam->pMetadata, m_outputImgParam->pPhyCamMetadata,
              m_outputImgParam->pAddr[0]);
    }

    if (m_InputIndex == m_CopyFrameIndex) {
        memcpy(m_copyFromInput, &processRequestInfo->inputBuffersMap.begin()->second[0],
               sizeof(ImageParams) - sizeof(void *));
    } else if (m_InputIndex == 0) {
        memcpy(m_firstInput, &processRequestInfo->inputBuffersMap.begin()->second[0],
               sizeof(ImageParams) - sizeof(void *));
    }

    if (inputNum > 0) {
        memcpy(m_curInputImgParam, &processRequestInfo->inputBuffersMap.begin()->second[0],
               sizeof(ImageParams) - sizeof(void *));
        MLOGI(Mia2LogGroupPlugin,
              "FrameNum %d input[%d] planeStride: %d, sliceheight: %d, format: %d logiMata: %p "
              "phyMeta: %p imageAddr: %p",
              processRequestInfo->frameNum, m_InputIndex.load(), m_curInputImgParam->planeStride,
              m_curInputImgParam->sliceheight, m_curInputImgParam->format.format,
              m_curInputImgParam->pMetadata, m_curInputImgParam->pPhyCamMetadata,
              m_curInputImgParam->pAddr[0]);
        preProcessInput();
    }

    // process here
    if (PROCESSEND != getCurrentStat() && PROCESSERROR != getCurrentStat()) {
        if (m_BypassForDebug) {
            MLOGD(Mia2LogGroupPlugin, "algo process is bypassed");
            setNextStat(PROCESSBYPASS);
        } else if (processRequestInfo->isFirstFrame) {
            MLOGI(Mia2LogGroupPlugin, "process firstFrame");
            setNextStat(PROCESSINIT);
        } else {
            MLOGI(Mia2LogGroupPlugin, "process frame %d", m_InputIndex.load());
            setNextStat(PROCESSPRE);
        }
    }

    processRequestStat();
    m_InputIndex++;

    // int ret = processBuffer(inputFrame, inputNum, &outputFrame, pMetaData, inputFD, outputFD);
    //
    //  if (ret != PROCSUCCESS) {
    //      resultInfo = PROCFAILED;
    //  }
    //
    // if (resultInfo == PROCSUCCESS) {
    //     if (m_BypassForDebug) {
    //         transferdata[0] = (int32_t)MiaResultDataType::MIA_RAWSN_DATA;
    //         transferdata[1] = 24;
    //         transferdata[2] = 8;
    //         transferdata[3] = 4616;
    //         transferdata[4] = 3456;
    //         resultInfo.resultData = (int32_t *)transferdata;
    //     } else {
    //         transferdata[0] = (int32_t)MiaResultDataType::MIA_RAWSN_DATA;
    //         transferdata[1] = m_OutImgRect.left;
    //         transferdata[2] = m_OutImgRect.top;
    //         transferdata[3] = m_OutImgRect.right;
    //         transferdata[4] = m_OutImgRect.bottom;
    //         resultInfo.resultData = (int32_t *)transferdata;
    //     }
    //     int32_t *p = (int32_t *)resultInfo.resultData;
    //     MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: resultInfo.dataType: %d,(%d %d %d %d),addr:%p",
    //     *p,*(p+1),*(p+2),*(p+3),*(p+4),resultInfo.resultData);
    // }

    //    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: ret = %d", ret);
    //
    // if (m_BypassForDebug) {
    //    char outputbuf[128];
    //    snprintf(outputbuf, sizeof(outputbuf), "arcsoftsll_output%dx%d", outputFrame.width,
    //             outputFrame.height);
    //    PluginUtils::dumpToFile(outputbuf, &outputFrame);
    //}

    mProcessCount++;
    return resultInfo;
}

ProcessRetStatus RawSuperlightPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

void RawSuperlightPlugin::copyAvailableBuffer()
{
    ImageParams *src = m_firstInput == NULL ? NULL : m_firstInput;
    MLOGD(Mia2LogGroupPlugin, "src addr is %p,buffer is %p", src, src->pAddr[0]);
    int index = 0;
    if (m_InputIndex > m_CopyFrameIndex) {
        src = m_copyFromInput;
        index = m_CopyFrameIndex;
    }
    if (!src) {
        MLOGE(Mia2LogGroupPlugin, "no available buffer to copy");
        return;
    }

    MLOGD(Mia2LogGroupPlugin, "src addr is %p,buffer is %p", src, src->pAddr[0]);
    if (m_outputImgParam->pAddr[0] && src->pAddr[0]) {
        int correctImageRet = INIT_VALUE;
        MiImageBuffer inputFrameBuf, outputFrameBuf;
        ASVLOFFSCREEN asvl_outputFrame;
        imageToMiBuffer(m_outputImgParam, &outputFrameBuf);
        imageToMiBuffer(src, &inputFrameBuf);
        PluginUtils::miCopyBuffer(&outputFrameBuf, &inputFrameBuf);

        prepareImage(&outputFrameBuf, asvl_outputFrame, m_outputImgParam->fd[0]);
        if (m_hArcsoftRawSuperlight) {
            correctImageRet = m_hArcsoftRawSuperlight->processCorrectImage(
                &m_outputImgParam->fd[0], &asvl_outputFrame, m_camState, &m_RawInfo[index]);
        }
        MLOGI(Mia2LogGroupPlugin, "copy EV[%d] to output buffer, correctImageRet: %d",
              m_RawInfo[index].i32EV[0], correctImageRet);
        setNextStat(PROCESSUINIT);
    }
}
int RawSuperlightPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    int ret = 0;
    MLOGI(Mia2LogGroupPlugin, "flush isForced %d m_InputIndex %d", flushRequestInfo->isForced,
          m_InputIndex.load());
    m_flushFlag = true;

    if (m_hArcsoftRawSuperlight && (m_InputIndex == m_algoInputNum - 1)) {
        m_hArcsoftRawSuperlight->setInitStatus(0);
        MLOGI(Mia2LogGroupPlugin, "set user cancel");
    }

    std::lock_guard<std::mutex> lock(m_flushMutex);
    if ((PROCESSEND != getCurrentStat()) && (PROCESSUINIT != getCurrentStat())) {
        copyAvailableBuffer();
        setNextStat(PROCESSUINIT);
        processRequestStat();
    }
    m_flushFlag = false;

    MLOGI(Mia2LogGroupPlugin, "flush end %p", this);
    return ret;
}

ProcessRetStatus RawSuperlightPlugin::processBuffer(struct MiImageBuffer *input, int inputNum,
                                                    struct MiImageBuffer *output,
                                                    camera_metadata_t **pMetaData, int32_t *inFD,
                                                    int32_t outFD)
{
    // int32_t processRet = PROCSUCCESS;
    // int32_t correctImageRet = PROCSUCCESS;
    // ASVLOFFSCREEN inputFrame[ARC_SN_MAX_INPUT_IMAGE_NUM], outputFrame;
    // int32_t i32ISOValue[FRAME_MAX_INPUT_NUMBER] = {0};
    // ARC_SN_RAWINFO rawInfo[FRAME_MAX_INPUT_NUMBER];
    // int32_t mNumberOfInputImages = inputNum;
    // MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: the input image count is %d", mNumberOfInputImages);
    //
    // MMemSet(&m_OutImgRect, 0, sizeof(MRECT));
    // MMemSet(m_FaceParams, 0, sizeof(ARC_SN_FACEINFO) * FRAME_MAX_INPUT_NUMBER);
    // m_processTime = 0;
    //
    // if (pMetaData == NULL) {
    //    MLOGE(Mia2LogGroupPlugin, "error pMetaData is null %s", __func__);
    //    return PROCFAILED;
    //}
    //
    // MLOGI(Mia2LogGroupPlugin, "ProcessBuffer input_num: %d, format:%d, m_format.format:%d",
    //      inputNum, output->format, m_format.format);
    //
    // MIARAWFORMAT rawFormat = {0};
    // switch (m_format.format) {
    // case CAM_FORMAT_RAW10:
    //    rawFormat.bitsPerPixel = 10;
    //    break;
    // case CAM_FORMAT_RAW12:
    // case CAM_FORMAT_RAW16:
    //    rawFormat.bitsPerPixel = 12;
    //    break;
    //}
    //
    //// add ORIENTATION
    // void *pData = NULL;
    // int32_t orientation = 0;
    // VendorMetadataParser::getTagValue(*pMetaData, ANDROID_JPEG_ORIENTATION, &pData);
    // if (NULL != pData) {
    //    orientation = *static_cast<int32_t *>(pData);
    //    m_devOrientation = (orientation) % 360;
    //    MLOGI(Mia2LogGroupPlugin, "get Orientation from tag ANDROID_JPEG_ORIENTATION, value:%d",
    //          m_devOrientation);
    //}
    //
    //// int camMode    = m_bFrontCamera ? HARDCODE_CAMERAID_FRONT : HARDCODE_CAMERAID_REAR;
    //
    ////if (mNumberOfInputImages >= FRAME_MIN_INPUT_NUMBER) {
    //    char fileNameBuf[128];
    //    char timeBuf[128];
    //    gettimestr(timeBuf, sizeof(timeBuf));
    //    for (uint32_t i = 0; i < mNumberOfInputImages; i++) {
    //        getRawInfo(pMetaData[i], &rawFormat, &rawInfo[i], i);
    //        getFaceInfo(pMetaData[i], &m_FaceParams[i]);
    //        prepareImage(&input[i], inputFrame[i], inFD[i]);
    //        getISOValueInfo(pMetaData[i], &i32ISOValue[i]);
    //        if (m_dumpMask & 0x1) {
    //            memset(fileNameBuf, 0, sizeof(fileNameBuf));
    //            getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "input", timeBuf);
    //            dumpImageData(&input[i], m_InputIndex, fileNameBuf);
    //        }
    //
    //        if (m_dumpMask & 0x2) {
    //            memset(fileNameBuf, 0, sizeof(fileNameBuf));
    //            getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "rawinfo", timeBuf);
    //            dumpRawInfoExt(&input[i], m_InputIndex, mNumberOfInputImages,
    //            ARC_SN_SCENEMODE_LOWLIGHT,
    //                           m_CamMode, &m_FaceParams[i], &rawInfo[i], fileNameBuf);
    //        }
    //
    //        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: i %d, format: %d", m_InputIndex.load(),
    //        input->format);
    //    }
    //
    //    if(m_InputIndex == 0){
    //    prepareImage(output, outputFrame, outFD);
    //
    //    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  arcsoft process begin hdl:%p bypass:%d
    //    copyIndex:%d",
    //          m_hArcsoftRawSuperlight, m_BypassForDebug, m_CopyFrameIndex);
    //
    //    }
    //
    //
    //    double startTime = gettime();
    //    if ((m_hArcsoftRawSuperlight == NULL) || m_BypassForDebug) {
    //        PluginUtils::miCopyBuffer(output, &input[m_CopyFrameIndex]);
    //    } else if (m_hArcsoftRawSuperlight) {
    //
    //        processRet = m_hArcsoftRawSuperlight->process(
    //            inFD, inputFrame, &outFD, &outputFrame, &m_OutImgRect, rawInfo[0].i32EV,
    //            mNumberOfInputImages, ARC_SN_SCENEMODE_LOWLIGHT, m_camState, rawInfo,
    //            m_FaceParams, &correctImageRet, m_EVoffset, m_devOrientation);
    //        updateCropRegionMetaData(pMetaData[0], &m_OutImgRect);
    //        for (int i = 0; i < FRAME_MAX_INPUT_NUMBER; i++) {
    //            if (m_FaceParams[i].i32FaceNum != 0) {
    //                delete[] m_FaceParams[i].pFaceRects;
    //                m_FaceParams[i].pFaceRects = NULL;
    //            }
    //        }
    //    }
    //
    //    MLOGI(Mia2LogGroupPlugin,
    //          "[ARCSOFT]:  supernight arcsoft process end, process time:%f ms, processRet:%d",
    //          gettime() - startTime, processRet);
    //
    //    if (m_dumpMask & 0x1) {
    //        getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "output", timeBuf);
    //        dumpImageData(output, (uint32_t)i32ISOValue[0], fileNameBuf);
    //    }
    //
    //    m_processedFrame++;
    ////} else {
    ////    MLOGI(Mia2LogGroupPlugin,
    ////          "[ARCSOFT]:  the input frame number is %d at least, don't process
    /// superlowlightraw", /          FRAME_MIN_INPUT_NUMBER); / PluginUtils::miCopyBuffer(output,
    /// input);
    ////}
    //
    // if (processRet == PROCSUCCESS)
    //    return PROCSUCCESS;
    return PROCFAILED;
}

void RawSuperlightPlugin::updateCropRegionMetaData(camera_metadata_t *meta, MRECT *pCropRect)
{
    if (m_isCropInfoUpdate) {
        void *pData = NULL;

        VendorMetadataParser::getTagValue(meta, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &pData);
        if (!pData) {
            MLOGE(Mia2LogGroupPlugin,
                  "[ARCSOFT]  updateCropRegionMetaData get active array size failed");
            return;
        }
        uint32_t ui32ActiveArrayWidth = (reinterpret_cast<MIRECT *>(pData))->width;
        uint32_t ui32ActiveArrayHeight = (reinterpret_cast<MIRECT *>(pData))->height;
        uint32_t ui32SensorActiveArrayWidth = m_sensorActiveArray.width;
        uint32_t ui32SensorActiveArrayHeight = m_sensorActiveArray.height;

        pData = NULL;
        VendorMetadataParser::getTagValue(meta, ANDROID_SCALER_CROP_REGION, &pData);

        if (pData != NULL) {
            MIRECT *pCropWindow = static_cast<MIRECT *>(pData);
            MFloat ratio_h = 1.0f;
            MFloat ratio_v = 1.0f;

            if (ui32ActiveArrayWidth > 0 && ui32ActiveArrayHeight > 0 &&
                ui32SensorActiveArrayWidth > 0 && ui32SensorActiveArrayHeight > 0) {
                // todo, sheng, check ratio
                ratio_h =
                    (MFloat)((MFloat)ui32SensorActiveArrayWidth / (MFloat)ui32ActiveArrayWidth);
                ratio_v =
                    (MFloat)((MFloat)ui32SensorActiveArrayHeight / (MFloat)ui32ActiveArrayHeight);
                if ((pCropWindow->left < (uint32_t)pCropRect->left) ||
                    (pCropWindow->top < (uint32_t)pCropRect->top)) {
                    pCropWindow->left = (pCropRect->left * ratio_h);
                    pCropWindow->top = (pCropRect->top * ratio_v);
                    pCropWindow->width = (pCropRect->right - pCropRect->left) * ratio_h;
                    pCropWindow->height = (pCropRect->bottom - pCropRect->top) * ratio_v;
                } else {
                    // TODO:check whether need to modify crop coordinate
                    // pCropWindow->left += pCropRect->left >> 1;
                    // pCropWindow->top += pCropRect->top >> 1;
                }
            }
            MLOGI(Mia2LogGroupPlugin,
                  "[ArcSoft] updateCropRegionMetaData "
                  "ANDROID_SCALER_CROP_REGION(l,t,w,h)=(%d,%d,%d,%d) ratio(h,v)=(%f,%f)  "
                  "(%d,%d,%d,%d)",
                  pCropWindow->left, pCropWindow->top, pCropWindow->width, pCropWindow->height,
                  ratio_h, ratio_v, pCropRect->left, pCropRect->top,
                  (pCropRect->right - pCropRect->left), (pCropRect->bottom - pCropRect->top));
        }
    }
}

void RawSuperlightPlugin::copyImage(struct MiImageBuffer *dst, struct MiImageBuffer *src)
{
    dst->width = src->width;
    dst->height = src->height;
    dst->format = src->format;
    dst->stride = src->stride;
    dst->scanline = src->scanline;

    for (uint32_t i = 0; src->plane[i] != NULL; i++) {
        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]: Memcpy i %d, dst->plane[i] %p, src->plane[i] %p, size %d", i,
              dst->plane[i], src->plane[i], src->stride * src->scanline);
        memcpy(dst->plane[i], src->plane[i], src->stride * src->scanline);
    }
}

void RawSuperlightPlugin::getISOValueInfo(camera_metadata_t *meta, MInt32 *pISOValue)
{
    if (meta == NULL)
        return;

    MInt32 i32ISOValue = 0;
    void *pData = NULL;

    VendorMetadataParser::getTagValue(meta, ANDROID_SENSOR_SENSITIVITY, &pData);
    if (NULL != pData) {
        i32ISOValue = *static_cast<int32_t *>(pData);
    } else {
        i32ISOValue = 100;
    }

    if (pISOValue != NULL) {
        *pISOValue = i32ISOValue;
    }
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getISOValue ISOValue = %d", i32ISOValue);
    return;
}

void RawSuperlightPlugin::dumpRawInfoExt(struct MiImageBuffer *pNodeBuff, int index, int count,
                                         MInt32 i32SceneMode, MInt32 i32CamMode,
                                         ARC_SN_FACEINFO *pFaceParams, ARC_SN_RAWINFO *rawInfo,
                                         char *namePrefix)
{
    uint32_t nStride = 0;
    uint32_t nSliceheight = 0;
    uint32_t format = pNodeBuff->format;
    char filename[256];

    if (CAM_FORMAT_RAW10 == format || CAM_FORMAT_RAW12 == format || CAM_FORMAT_RAW16 == format ||
        CAM_FORMAT_YUV_420_NV12 == format || CAM_FORMAT_YUV_420_NV21 == format) {
        nStride = pNodeBuff->stride;
        nSliceheight = pNodeBuff->scanline;
    }

    memset(filename, 0, sizeof(filename));

    snprintf(filename, sizeof(filename), "%s%s.txt", SN_DUMP_PATH, namePrefix);

    FILE *pf = fopen(filename, "a");
    if (pf) {
        int face_cnt = 0;
        if (pFaceParams != NULL) {
            face_cnt = pFaceParams->i32FaceNum;
        }
        float redGain = rawInfo->fWbGain[0];
        float blueGain = rawInfo->fWbGain[3];
        switch (rawInfo->i32RawType) {
        case ASVL_PAF_RAW10_GRBG_16B:
        case ASVL_PAF_RAW12_GRBG_16B:
        case ASVL_PAF_RAW14_GRBG_16B:
        case ASVL_PAF_RAW16_GRBG_16B:
            redGain = rawInfo->fWbGain[1];
            blueGain = rawInfo->fWbGain[2];
            break;
        case ASVL_PAF_RAW10_GBRG_16B:
        case ASVL_PAF_RAW12_GBRG_16B:
        case ASVL_PAF_RAW14_GBRG_16B:
        case ASVL_PAF_RAW16_GBRG_16B:
            redGain = rawInfo->fWbGain[2];
            blueGain = rawInfo->fWbGain[1];
            break;
        case ASVL_PAF_RAW10_BGGR_16B:
        case ASVL_PAF_RAW12_BGGR_16B:
        case ASVL_PAF_RAW14_BGGR_16B:
        case ASVL_PAF_RAW16_BGGR_16B:
            redGain = rawInfo->fWbGain[3];
            blueGain = rawInfo->fWbGain[0];
            break;
        default:
            break;
        }

        if (index == 0)
            fprintf(pf, "ArcSoft SuperNightRaw Algorithm:%s\n", m_algoVersion.Version);
        fprintf(pf, "[InputFrameIndex:%d]\n", index);

        fprintf(pf, "redGain:%.6f\nblueGain:%.6f\n", redGain, blueGain);
        fprintf(pf, "totalGain:%.6f\nsensorGain:%.6f\nispGain:%.6f\n",
                m_ExtRawValue.SensorGain[index] * m_ExtRawValue.ISPGain[index],
                m_ExtRawValue.SensorGain[index], m_ExtRawValue.ISPGain[index]);
        fprintf(pf, "EV[%d]:%d\n", index, rawInfo->i32EV[index]);
        fprintf(pf, "shutter:%.6f\nluxIndex:%d\nexpIndex:%d\n", rawInfo->fShutter,
                rawInfo->i32LuxIndex, rawInfo->i32ExpIndex);
        fprintf(pf, "blackLevel[0]:%d\nblackLevel[1]:%d\nblackLevel[2]:%d\nblackLevel[3]:%d\n",
                rawInfo->i32BlackLevel[0], rawInfo->i32BlackLevel[1], rawInfo->i32BlackLevel[2],
                rawInfo->i32BlackLevel[3]);

        fprintf(pf, "adrc_gain:%.6f\n", rawInfo->fAdrcGain);
        fprintf(pf, "scene_mode:%d\ncamera_state:%d\ncamera_mode:%d\n", i32SceneMode, m_camState,
                i32CamMode);
        fprintf(pf, "raw_format:%d\n", rawInfo->i32RawType);
        fprintf(pf, "device_orientation:%d\n", m_devOrientation);
        fprintf(pf, "face_cnt:%d\n", face_cnt);
        for (MInt32 i = 0; i < face_cnt; i++) {
            fprintf(pf, "left:%d\ntop:%d\nright:%d\nbottom:%d\nrotation:%d\n",
                    pFaceParams->pFaceRects[i].left, pFaceParams->pFaceRects[i].top,
                    pFaceParams->pFaceRects[i].right, pFaceParams->pFaceRects[i].bottom,
                    pFaceParams->i32FaceOrientation);
        }
        fprintf(pf, "cct:%.6f\n", rawInfo->fCCT);
        fprintf(pf, "ccm:");
        for (MInt32 i = 0; i < 9; i++) {
            fprintf(pf, "%.6f,", rawInfo->fCCM[i]);
        }

        fprintf(pf, "\n");
        if (index == count - 1) {
            fprintf(pf, "bEnableLSC:%d\nnLscChannelLength:%d\n", rawInfo->bEnableLSC ? 1 : 0,
                    rawInfo->i32LSChannelLength[0]);
            fprintf(pf, "lsc_x_col:17\n");
            fprintf(pf, "lsc_x_row:%d\n\n\n",
                    rawInfo->i32LSChannelLength[0] / 17 +
                        (rawInfo->i32LSChannelLength[0] % 17 ? 1 : 0));
            char a[4][10] = {"R", "Gr", "Gb", "B"};
            for (int i = 0; i < 4; i++) {
                fprintf(pf, " =======LSC Table %s=======", a[i]);
                for (int j = 0; j < rawInfo->i32LSChannelLength[i]; j++) {
                    if (j % 17 == 0)
                        fprintf(pf, "\n");
                    fprintf(pf, "%.6f, ", rawInfo->fLensShadingTable[i][j]);
                }
                fprintf(pf, "\n\n");
            }
        }
        fclose(pf);
    }
}

void RawSuperlightPlugin::dumpImageData(struct MiImageBuffer *pNodeBuff, uint32_t index,
                                        const char *namePrefix)
{
    uint32_t format = pNodeBuff->format;

    if (format == CAM_FORMAT_RAW10 || format == CAM_FORMAT_RAW12 || format == CAM_FORMAT_RAW16) {
        int nStride = pNodeBuff->stride;
        int nSliceheight = pNodeBuff->scanline;
        char filename[256];
        memset(filename, 0, sizeof(char) * 256);
        switch (m_rawPattern) {
        case Y: ///< Monochrome pixel pattern.
            snprintf(filename, sizeof(filename), "%s%s_%d.Y", SN_DUMP_PATH, namePrefix, index);
            break;
        case YUYV: ///< YUYV pixel pattern.
            snprintf(filename, sizeof(filename), "%s%s_%d.YUYV", SN_DUMP_PATH, namePrefix, index);
            break;
        case YVYU: ///< YVYU pixel pattern.
            snprintf(filename, sizeof(filename), "%s%s_%d.YVYU", SN_DUMP_PATH, namePrefix, index);
            break;
        case UYVY: ///< UYVY pixel pattern.
            snprintf(filename, sizeof(filename), "%s%s_%d.UYVY", SN_DUMP_PATH, namePrefix, index);
            break;
        case VYUY: ///< VYUY pixel pattern.
            snprintf(filename, sizeof(filename), "%s%s_%d.VYUY", SN_DUMP_PATH, namePrefix, index);
            break;
        case RGGB: ///< RGGB pixel pattern.
            snprintf(filename, sizeof(filename), "%s%s_%d.RGGB", SN_DUMP_PATH, namePrefix, index);
            break;
        case GRBG: ///< GRBG pixel pattern.
            snprintf(filename, sizeof(filename), "%s%s_%d.GRBG", SN_DUMP_PATH, namePrefix, index);
            break;
        case GBRG: ///< GBRG pixel pattern.
            snprintf(filename, sizeof(filename), "%s%s_%d.GBRG", SN_DUMP_PATH, namePrefix, index);
            break;
        case BGGR: ///< BGGR pixel pattern.
            snprintf(filename, sizeof(filename), "%s%s_%d.BGGR", SN_DUMP_PATH, namePrefix, index);
            break;
        case RGB: ///< RGB pixel pattern.
            snprintf(filename, sizeof(filename), "%s%s_%d.RGB", SN_DUMP_PATH, namePrefix, index);
            break;
        default:
            snprintf(filename, sizeof(filename), "%s%s_%d.DATA", SN_DUMP_PATH, namePrefix, index);
            break;
        }

        int file_fd = open(filename, O_RDWR | O_CREAT, 0777);

        if (file_fd) {
            uint32_t bytes_write;
            bytes_write =
                write(file_fd, pNodeBuff->plane[0], pNodeBuff->stride * pNodeBuff->scanline);
            close(file_fd);
        } else {
            MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]  Canot open %s", filename);
        }
    }
    // raw supernight do not support yuv format dump
    /*
    else if (YUV420NV12 == format || YUV420NV21 == format)
    {
        int nStride = imageFormat.formatParams.yuvFormat[0].planeStride;
        int nSliceheight = imageFormat.formatParams.yuvFormat[0].sliceHeight;
        CHAR filename[256];
        memset(filename, 0, sizeof(CHAR) * 256);
        snprintf(filename, sizeof(filename), "%s%llu_%s_%dx%d_%d.nv21",
                 SN_DUMP_PATH, gettime(), namePrefix,nStride,nSliceheight,index);
        int file_fd = open(filename,O_RDWR|O_CREAT,0777);

        CHIIMAGE& chiImage = pNodeBuff->pImageList[0];
        if(file_fd)
        {
            UINT bytes_write;
            bytes_write = write(file_fd,chiImage.pAddr[0],pNodeBuff->planeSize[0]);
            bytes_write = write(file_fd,chiImage.pAddr[1],pNodeBuff->planeSize[1]);
            close(file_fd);
        }
    }
    */
    else {
        MLOGE(Mia2LogGroupPlugin,
              "[ARCSOFT]  can't dump data since doesn't support this format(%d)", format);
    }
}

void RawSuperlightPlugin::prepareImage(struct MiImageBuffer *pNodeBuff, ASVLOFFSCREEN &stImage,
                                       MInt32 &fd)
{
    uint32_t format = pNodeBuff->format;
    if (format != CAM_FORMAT_RAW10 && format != CAM_FORMAT_RAW12 && format != CAM_FORMAT_RAW16) {
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT] format(%d) is not supported!", format);
        return;
    }

    stImage.i32Width = pNodeBuff->width;
    stImage.i32Height = pNodeBuff->height;
    // TODO: fd = ?
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT] stImage(%dx%d)", stImage.i32Width, stImage.i32Height);

    MIARAWFORMAT rawFormat = {0};
    switch (m_format.format) {
    case CAM_FORMAT_RAW10:
        rawFormat.bitsPerPixel = 10;
        break;
    case CAM_FORMAT_RAW12:
    case CAM_FORMAT_RAW16:
        rawFormat.bitsPerPixel = 12;
        break;
    }
    stImage.u32PixelArrayFormat = getRawType(&rawFormat);
    // for(uint32_t i = 0; pNodeBuff->plane[i] != NULL; i++) // TODO:
    for (uint32_t i = 0; i < pNodeBuff->numberOfPlanes; i++) // TODO:
    {
        stImage.ppu8Plane[i] = pNodeBuff->plane[i];
        stImage.pi32Pitch[i] = pNodeBuff->stride;
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT] RawPlain16 stImage.pi32Pitch[%d] = %d", i,
              stImage.pi32Pitch[i]);
        MLOGI(Mia2LogGroupPlugin, "ShengX, RawPlain16 FD=%d", pNodeBuff->fd[i]);
    }
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT] the image buffer format is 0x%x",
          stImage.u32PixelArrayFormat);
}

void RawSuperlightPlugin::getRawInfo(camera_metadata_t *meta, MIARAWFORMAT *rawFormat,
                                     ARC_SN_RAWINFO *rawInfo, int32_t index)
{
    void *pData = NULL;
    uint8_t bayerPattern = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR;

    VendorMetadataParser::getVTagValue(meta, "xiaomi.sensorinfo.colorFilterArrangement", &pData);
    if (NULL != pData) {
        bayerPattern = *static_cast<uint8_t *>(pData);
    } else {
        MLOGE(Mia2LogGroupPlugin, "Failed to get sensor bayer pattern");
    }
    switch (bayerPattern) {
    case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB:
        m_rawPattern = RGGB;
        break;
    case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG:
        m_rawPattern = GRBG;
        break;
    case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG:
        m_rawPattern = GBRG;
        break;
    case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR:
        m_rawPattern = BGGR;
        break;
    }
    rawInfo->i32RawType = getRawType(rawFormat);

    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->i32RawType is %d",
          rawInfo->i32RawType);
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo SuperNightRawExpValue count %d",
          m_ExtRawValue.validFrameCount);
    for (uint32_t i = 0; i < m_ExtRawValue.validFrameCount; i++) {
        rawInfo->i32EV[i] = m_ExtRawValue.Ev[i];
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->i32EV[%d] =%d", i,
              rawInfo->i32EV[i]);
    }

    if (m_hArcsoftRawSuperlight->setSuperNightRawValue(&m_ExtRawValue,
                                                       m_ExtRawValue.validFrameCount)) {
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]:  set SuperNight Raw Value fail");
    }

    pData = NULL;
    int white_lelvel_dynamic = 0;
    VendorMetadataParser::getVTagValue(meta, "xiaomi.supernight.brightlevel", &pData);
    if (NULL != pData) {
        white_lelvel_dynamic = *static_cast<int32_t *>(pData);
    }

    for (int i = 0; i < 4; i++) {
        rawInfo->i32BrightLevel[i] = white_lelvel_dynamic;
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->i32BrightLevel[%d] = %d", i,
              white_lelvel_dynamic);
    }

    float black_level[4] = {0.0f};
    pData = NULL;
    VendorMetadataParser::getVTagValue(meta, "xiaomi.supernight.blacklevel", &pData);
    if (NULL != pData) {
        memcpy(black_level, static_cast<int32_t *>(pData), 4 * sizeof(float));
    }

    getBlackLevel(m_ExtRawValue.ISPGain[index] * m_ExtRawValue.SensorGain[index], m_BlackLevel);
    rawInfo->i32BlackLevel[0] = m_BlackLevel.r;
    rawInfo->i32BlackLevel[1] = m_BlackLevel.gr;
    rawInfo->i32BlackLevel[2] = m_BlackLevel.gb;
    rawInfo->i32BlackLevel[3] = m_BlackLevel.b;

    for (int i = 0; i < 4; i++) {
        //  rawInfo->i32BlackLevel[i] = (int)black_level[i];
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->i32BlackLevel[%d] = %d", i,
              rawInfo->i32BlackLevel[i]);
    }

    getLSCDataInfo(meta, rawInfo);

    pData = NULL;
    VendorMetadataParser::getVTagValue(meta, "xiaomi.supernight.fwbgain", &pData);

    if (NULL != pData) {
        memcpy(rawInfo->fWbGain, pData, 4 * sizeof(float));
        awbGainMapping(rawInfo->i32RawType, rawInfo->fWbGain);
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->fWbGain[0]  = %f",
              rawInfo->fWbGain[0]);
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->fWbGain[1]  = %f",
              rawInfo->fWbGain[1]);
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->fWbGain[2]  = %f",
              rawInfo->fWbGain[2]);
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->fWbGain[3]  = %f",
              rawInfo->fWbGain[3]);
    }

    // get LuxIndex, TotalGain, AdrcGain, ExpIndex, Shutter, ISPGain
    pData = NULL;
    VendorMetadataParser::getVTagValue(meta, "xiaomi.supernight.luxindex", &pData);
    if (NULL != pData) {
        float fLuxIndex = *static_cast<int32_t *>(pData);
        rawInfo->i32LuxIndex = (int32_t)fLuxIndex;
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->i32LuxIndex = %d",
              rawInfo->i32LuxIndex);
    }

    pData = NULL;
    VendorMetadataParser::getVTagValue(meta, "xiaomi.supernight.fadrcgain", &pData);
    if (NULL != pData) {
        rawInfo->fAdrcGain = (MFloat) * static_cast<int32_t *>(pData);
        // AdrcGain transfered by MTK's platform is very abnormal, so use 1 temporarily
        rawInfo->fAdrcGain = 1;
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->fAdrcGain = %f",
              rawInfo->fAdrcGain);
    }

    pData = NULL;
    VendorMetadataParser::getVTagValue(meta, "xiaomi.supernight.expindex", &pData);
    if (NULL != pData) {
        rawInfo->i32ExpIndex = *static_cast<int32_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->i32ExpIndex = %d",
              rawInfo->i32ExpIndex);
    }

    pData = NULL;
    VendorMetadataParser::getVTagValue(meta, "xiaomi.supernight.fshutter", &pData);
    if (NULL != pData) {
        rawInfo->fShutter = (MFloat) * static_cast<int64_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->fShutter = %f",
              rawInfo->fShutter);
    }

    // arcsoft's request
    rawInfo->fSensorGain = m_ExtRawValue.SensorGain[index];
    rawInfo->fISPGain = m_ExtRawValue.ISPGain[index];
    rawInfo->fTotalGain = rawInfo->fSensorGain * rawInfo->fISPGain;
    rawInfo->i32ExpIndex = rawInfo->fShutter;
    rawInfo->fShutter = rawInfo->i32ExpIndex / 1e9;

    pData = NULL;
    VendorMetadataParser::getVTagValue(meta, "xiaomi.supernight.fcct", &pData);
    if (NULL != pData) {
        rawInfo->fCCT = (MFloat) * static_cast<int32_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawInfo rawInfo->fCCT = %f", rawInfo->fCCT);
    } else {
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  fail to getRawInfo CCT");
    }

    pData = NULL;
    VendorMetadataParser::getTagValue(meta, ANDROID_COLOR_CORRECTION_TRANSFORM, &pData);
    if (NULL != pData) {
        MRational *ccm = static_cast<MRational *>(pData);
        for (uint32_t i = 0; i < 9; i++) {
            ccm[i].denominator = (ccm[i].denominator == 0) ? 1 : ccm[i].denominator;
            rawInfo->fCCM[i] = (static_cast<float>(static_cast<int32_t>(ccm[i].numerator)) /
                                static_cast<float>(static_cast<int32_t>(ccm[i].denominator)));
        }
        MLOGI(Mia2LogGroupPlugin, "get ccm: %f %f %f | %f %f %f | %f %f %f", rawInfo->fCCM[0],
              rawInfo->fCCM[1], rawInfo->fCCM[2], rawInfo->fCCM[3], rawInfo->fCCM[4],
              rawInfo->fCCM[5], rawInfo->fCCM[6], rawInfo->fCCM[7], rawInfo->fCCM[8]);
    } else {
        MLOGE(Mia2LogGroupPlugin, "get ANDROID_COLOR_CORRECTION_TRANSFORM fail");
    }
}

void RawSuperlightPlugin::awbGainMapping(MInt32 i32RawType, MFloat *pArcAWBGain)
{
    if (pArcAWBGain == NULL) {
        MLOGE(Mia2LogGroupPlugin,
              "[ARCSOFT]:  =======================awbGainMapping input params "
              "error=====================");
        return;
    }

    float tmp = 0.0f;
    switch (i32RawType) {
    case ASVL_PAF_RAW10_RGGB_16B:
    case ASVL_PAF_RAW12_RGGB_16B:
    case ASVL_PAF_RAW14_RGGB_16B:
    case ASVL_PAF_RAW16_RGGB_16B:
        return;
    case ASVL_PAF_RAW10_GRBG_16B:
    case ASVL_PAF_RAW12_GRBG_16B:
    case ASVL_PAF_RAW14_GRBG_16B:
    case ASVL_PAF_RAW16_GRBG_16B:
        tmp = pArcAWBGain[1];
        pArcAWBGain[1] = pArcAWBGain[0];
        pArcAWBGain[0] = tmp;
        pArcAWBGain[2] = pArcAWBGain[3];
        pArcAWBGain[3] = tmp;
        return;
    case ASVL_PAF_RAW10_GBRG_16B:
    case ASVL_PAF_RAW12_GBRG_16B:
    case ASVL_PAF_RAW14_GBRG_16B:
    case ASVL_PAF_RAW16_GBRG_16B:
        tmp = pArcAWBGain[0];
        pArcAWBGain[0] = pArcAWBGain[1];
        pArcAWBGain[1] = pArcAWBGain[3];
        pArcAWBGain[3] = pArcAWBGain[2];
        pArcAWBGain[2] = tmp;
        return;
    case ASVL_PAF_RAW10_BGGR_16B:
    case ASVL_PAF_RAW12_BGGR_16B:
    case ASVL_PAF_RAW14_BGGR_16B:
    case ASVL_PAF_RAW16_BGGR_16B:
        tmp = pArcAWBGain[0];
        pArcAWBGain[0] = pArcAWBGain[3];
        pArcAWBGain[3] = tmp;
        return;
    default:
        MLOGE(Mia2LogGroupPlugin,
              "[ARCSOFT]:  =======================awbGainMapping dont have the matched "
              "format=====================");
        return;
    }
}

void RawSuperlightPlugin::getLSCDataInfo(camera_metadata_t *meta, ARC_SN_RAWINFO *rawInfo)
{
    if (meta == NULL)
        return;

    MInt32 i32ISOValue = 0;
    void *pData = NULL;

    // qcom method getting LSCControl enable
    /*
    VendorMetadataParser::getNTagValue(meta, "com.qti.chi.lsccontrolinfo.LSCControl", &pData);

    if(NULL == pData)
    {
        MLOGE(Mia2LogGroupPlugin, "failed to get vendorTagBase of LSCControl");
    }

    if (pData != NULL)
    {
        rawInfo->bEnableLSC = *static_cast< bool* >(pData);
        MLOGI(Mia2LogGroupPlugin, "set bEnableLSC to [%ld] for arcsoft supernight sdk",
    rawInfo->bEnableLSC);
    }
    */
    // workaround
    rawInfo->bEnableLSC = 1;

    pData = NULL;
    VendorMetadataParser::getTagValue(meta, ANDROID_LENS_INFO_SHADING_MAP_SIZE, &pData);

    if (NULL != pData) {
        mLSCDim = *static_cast<LSCDimensionCap *>(pData);
    }

    // workaround
    mLSCDim.width = 17;
    mLSCDim.height = 13;

    pData = NULL;
    // VendorMetadataParser::getVTagValue(meta, "android.statistics.lensShadingMap", &pData);
    VendorMetadataParser::getTagValue(meta, ANDROID_STATISTICS_LENS_SHADING_MAP, &pData);

    if (NULL != pData) {
        memcpy(m_lensShadingMap, static_cast<float *>(pData),
               mLSCDim.width * mLSCDim.height * LSCTotalChannels * sizeof(float));
    }
    // MTK method
    int index_R = 0;
    int index_Gr = 0;
    int index_Gb = 0;
    int index_B = 0;
    int totalMapSize = mLSCDim.width * mLSCDim.height * LSCTotalChannels;

    for (int i = 0; i < totalMapSize; i++) {
        int j = i % 4;
        switch (j) {
        case (0):
            rawInfo->fLensShadingTable[0][index_R++] = m_lensShadingMap[i];
            continue;
        case (1):
            rawInfo->fLensShadingTable[1][index_Gr++] = m_lensShadingMap[i];
            continue;
        case (2):
            rawInfo->fLensShadingTable[2][index_Gb++] = m_lensShadingMap[i];
            continue;
        case (3):
            rawInfo->fLensShadingTable[3][index_B++] = m_lensShadingMap[i];
            continue;
        default:
            MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:write lsc table error");
            break;
        }
    }

    for (int i = 0; i < LSCTotalChannels; i++) {
        int j = 0;
        rawInfo->i32LSChannelLength[i] = mLSCDim.width * mLSCDim.height;
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  [%d] mLSCDim width = %d,height = %d len:%d ", i,
              mLSCDim.width, mLSCDim.height, rawInfo->i32LSChannelLength[i]);
        // qcom method
        /*
        for (uint16_t v = 0; v < mLSCDim.height; v++)
        {
            for (uint16_t h = 0; h < mLSCDim.width; h++)
            {
                uint16_t index = v * mLSCDim.width * LSCTotalChannels + h * LSCTotalChannels + i;
                rawInfo->fLensShadingTable[i][j] = m_lensShadingMap[index];
                j++;
            }
        }
        */

        if (m_dumpMask & 0x4) {
            dumpBLSLSCParameter(m_lensShadingMap);
            dumpSdkLSCParameter(&rawInfo->fLensShadingTable[i][0], i);
        }
    }

    return;
}

void RawSuperlightPlugin::dumpBLSLSCParameter(const float *lensShadingMap)
{
    FILE *pFp = NULL;
    static char dumpFilePath[256] = {
        '\0',
    };
    memset(dumpFilePath, '\0', sizeof(dumpFilePath));
    snprintf(dumpFilePath, sizeof(dumpFilePath),
             "%sDumpBLSLSCPara_FN-%" PRIu64 "_BLS-xx_LSC-xx.txt", SN_DUMP_PATH, m_processedFrame);

    if (NULL == (pFp = fopen(dumpFilePath, "wt"))) {
        MLOGE(Mia2LogGroupPlugin, "DumpBLSLSCPara creation fail");
    } else {
        fprintf(pFp, "LSC table channel 1\n");
        for (uint16_t v = 0; v < mLSCDim.height; v++) {
            for (uint16_t h = 0; h < mLSCDim.width; h++) {
                uint16_t index = v * mLSCDim.width * LSCTotalChannels + h * LSCTotalChannels;
                fprintf(pFp, "%12f ", lensShadingMap[index]);
            }
            fprintf(pFp, "\n");
        }

        fprintf(pFp, "LSC table channel 2\n");
        for (uint16_t v = 0; v < mLSCDim.height; v++) {
            for (uint16_t h = 0; h < mLSCDim.width; h++) {
                uint16_t index = v * mLSCDim.width * LSCTotalChannels + h * LSCTotalChannels + 1;
                fprintf(pFp, "%12f ", lensShadingMap[index]);
            }
            fprintf(pFp, "\n");
        }

        fprintf(pFp, "LSC table channel 3\n");
        for (uint16_t v = 0; v < mLSCDim.height; v++) {
            for (uint16_t h = 0; h < mLSCDim.width; h++) {
                uint16_t index = v * mLSCDim.width * LSCTotalChannels + h * LSCTotalChannels + 2;
                fprintf(pFp, "%12f ", lensShadingMap[index]);
            }
            fprintf(pFp, "\n");
        }

        fprintf(pFp, "LSC table channel 4\n");
        for (uint16_t v = 0; v < mLSCDim.height; v++) {
            for (uint16_t h = 0; h < mLSCDim.width; h++) {
                uint16_t index = v * mLSCDim.width * LSCTotalChannels + h * LSCTotalChannels + 3;
                fprintf(pFp, "%12f ", lensShadingMap[index]);
            }
            fprintf(pFp, "\n");
        }
        fclose(pFp);
    }
    pFp = NULL;
}

void RawSuperlightPlugin::dumpSdkLSCParameter(const float *lensShadingMap, int32_t channel)
{
    FILE *pFp = NULL;
    static char dumpFilePath[256] = {
        '\0',
    };
    memset(dumpFilePath, '\0', sizeof(dumpFilePath));
    snprintf(dumpFilePath, sizeof(dumpFilePath) - 1,
             "%sDumpBLSLSCPara_FN-%" PRIu64 "_BLS-xx_LSC-%d.txt", SN_DUMP_PATH, m_processedFrame,
             channel);

    if (NULL == (pFp = fopen(dumpFilePath, "wt"))) {
        MLOGE(Mia2LogGroupPlugin, "DumpBLSLSCPara creation fail");
    } else {
        fprintf(pFp, "LSC table channel %d\n", channel);
        for (uint16_t index = 0; index < mLSCDim.width * mLSCDim.height;) {
            fprintf(pFp, "%12f ", lensShadingMap[index]);
            if (++index % mLSCDim.width == 0)
                fprintf(pFp, "\n");
        }

        fclose(pFp);
    }
    pFp = NULL;
}

int32_t RawSuperlightPlugin::getRawType(MIARAWFORMAT *pMiaRawFormat)
{
    int32_t bitsPerPixel = pMiaRawFormat->bitsPerPixel;

    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawType bitsPerPixel = %d MiaColorFilterPattern = %d",
          bitsPerPixel, m_rawPattern);
    switch (m_rawPattern) {
    case Y:    ///< Monochrome pixel pattern.
    case YUYV: ///< YUYV pixel pattern.
    case YVYU: ///< YVYU pixel pattern.
    case UYVY: ///< UYVY pixel pattern.
    case VYUY: ///< VYUY pixel pattern.
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawType don't handle this colorFilterPattern %d",
              m_rawPattern);
        break;
    case RGGB: ///< RGGB pixel pattern.
        if (bitsPerPixel == 10) {
            return ASVL_PAF_RAW10_RGGB_16B;
        } else if (bitsPerPixel == 12) {
            return ASVL_PAF_RAW12_RGGB_16B;
        } else if (bitsPerPixel == 14) {
            return ASVL_PAF_RAW14_RGGB_16B;
        } else if (bitsPerPixel == 16) {
            return ASVL_PAF_RAW16_RGGB_16B;
        } else {
            return ASVL_PAF_RAW10_RGGB_16B;
        }
    case GRBG: ///< GRBG pixel pattern.
        if (bitsPerPixel == 10) {
            return ASVL_PAF_RAW10_GRBG_16B;
        } else if (bitsPerPixel == 12) {
            return ASVL_PAF_RAW12_GRBG_16B;
        } else if (bitsPerPixel == 14) {
            return ASVL_PAF_RAW14_GRBG_16B;
        } else if (bitsPerPixel == 16) {
            return ASVL_PAF_RAW16_GRBG_16B;
        } else {
            return ASVL_PAF_RAW10_GRBG_16B;
        }
    case GBRG: ///< GBRG pixel pattern.
        if (bitsPerPixel == 10) {
            return ASVL_PAF_RAW10_GBRG_16B;
        } else if (bitsPerPixel == 12) {
            return ASVL_PAF_RAW12_GBRG_16B;
        } else if (bitsPerPixel == 14) {
            return ASVL_PAF_RAW14_GBRG_16B;
        } else if (bitsPerPixel == 16) {
            return ASVL_PAF_RAW16_GBRG_16B;
        } else {
            return ASVL_PAF_RAW10_GBRG_16B;
        }
    case BGGR: ///< BGGR pixel pattern.
        if (bitsPerPixel == 10) {
            return ASVL_PAF_RAW10_BGGR_16B;
        } else if (bitsPerPixel == 12) {
            return ASVL_PAF_RAW12_BGGR_16B;
        } else if (bitsPerPixel == 14) {
            return ASVL_PAF_RAW14_BGGR_16B;
        } else if (bitsPerPixel == 16) {
            return ASVL_PAF_RAW16_BGGR_16B;
        } else {
            return ASVL_PAF_RAW10_BGGR_16B;
        }
        break;
    case RGB: ///< RGB pixel pattern.
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getRawType don't hanlde this colorFilterPattern %d",
              m_rawPattern);
        break;
    default:
        break;
    }

    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]:  =======================getRawType dont have the matched "
          "format=====================");
    return 0;
}

void RawSuperlightPlugin::getFaceInfo(camera_metadata_t *meta, ARC_SN_FACEINFO *pFaceInfo)
{
    void *pData = NULL;
    MiDimension SensorSize = {0};
    int32_t result = PROCSUCCESS;

    size_t tcount = 0;
    VendorMetadataParser::getTagValueCount(meta, ANDROID_STATISTICS_FACE_RECTANGLES, &pData,
                                           &tcount);
    if (NULL == pData) {
        pFaceInfo->i32FaceNum = 0;
        return;
    }
    pFaceInfo->i32FaceNum = (int32_t)(tcount / 4) < ARCSOFT_MAX_FACE_NUMBER
                                ? (int32_t)(tcount / 4)
                                : ARCSOFT_MAX_FACE_NUMBER;
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getFaceInfo i32FaceNum:%d", pFaceInfo->i32FaceNum);

    result = getSensorSize(meta, &SensorSize);
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getFaceInfo i32FaceNum:%d activesize(%d,%d)",
          pFaceInfo->i32FaceNum, SensorSize.width, SensorSize.height);

    uint32_t imageWidth = m_format.width;
    float ratio = 1.0f;

    if (SensorSize.width != 0) {
        ratio = (float)imageWidth / SensorSize.width;
    }

    pFaceInfo->pFaceRects = new MRECT[pFaceInfo->i32FaceNum];
    memcpy(pFaceInfo->pFaceRects, pData, sizeof(MRECT) * pFaceInfo->i32FaceNum);
    for (MInt32 index = 0; index < pFaceInfo->i32FaceNum; index++) {
        pFaceInfo->pFaceRects[index].left *= ratio;
        pFaceInfo->pFaceRects[index].top *= ratio;
        pFaceInfo->pFaceRects[index].right *= ratio;
        pFaceInfo->pFaceRects[index].bottom *= ratio;
        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]:  getFaceInfo ratio:(%f) faceinfo: (l,t,r,b)=(%d,%d,%d,%d)", ratio,
              pFaceInfo->pFaceRects[index].left, pFaceInfo->pFaceRects[index].top,
              pFaceInfo->pFaceRects[index].right, pFaceInfo->pFaceRects[index].bottom);
    }

    MInt32 i32Orientation = 0;
    pData = NULL;
    VendorMetadataParser::getTagValue(meta, ANDROID_JPEG_ORIENTATION, &pData);
    if (NULL != pData) {
        i32Orientation = *static_cast<MInt32 *>(pData);
        MLOGI(Mia2LogGroupPlugin, "get i32Orientation from tag ANDROID_JPEG_ORIENTATION, value: %d",
              i32Orientation);
    }

    pFaceInfo->i32FaceOrientation = i32Orientation;
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  getFaceInfo orientation: %d <-- %d",
          pFaceInfo->i32FaceOrientation, i32Orientation);
}

int32_t RawSuperlightPlugin::getSensorSize(camera_metadata_t *meta, MiDimension *pSensorSize)
{
    void *pData = NULL;
    VendorMetadataParser::getVTagValue(meta, "xiaomi.sensorSize.size", &pData);
    if (NULL == pData) {
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]  getSensorSize failed");
        return PROCFAILED;
    }

    *pSensorSize = *static_cast<MiDimension *>(pData);
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]  getSensorSize(w,h) = (%d, %d)", pSensorSize->width,
          pSensorSize->height);

    return PROCSUCCESS;
}

void RawSuperlightPlugin::getBlackLevel(float totalGain, BlackLevel &blackLevel)
{
    int index = 0;
    int length = sizeof(BlackLevelTable) / (sizeof(MInt32) * MAX_BLACK_LEVEL_COLUMN);

    switch (m_CamMode) {
    case CAMERA_MODE_XIAOMI_L11_D2000_HM2_Y:
        memcpy(BlackLevelTable, BlackLevelTableWideMatisse, sizeof(BlackLevelTableWideMatisse));
        length = sizeof(BlackLevelTableWideMatisse) / (sizeof(MInt32) * MAX_BLACK_LEVEL_COLUMN);
        if (m_isSENeeded) {
            memcpy(BlackLevelTable, BlackLevelTableWideMatisseSE,
                   sizeof(BlackLevelTableWideMatisseSE));
            length =
                sizeof(BlackLevelTableWideMatisseSE) / (sizeof(MInt32) * MAX_BLACK_LEVEL_COLUMN);
        }
        break;
    case CAMERA_MODE_XIAOMI_L11A_D1500_IMX582_Y:
        memcpy(BlackLevelTable, BlackLevelTableWideRubens, sizeof(BlackLevelTableWideRubens));
        length = sizeof(BlackLevelTableWideRubens) / (sizeof(MInt32) * MAX_BLACK_LEVEL_COLUMN);
        if (m_isSENeeded) {
            memcpy(BlackLevelTable, BlackLevelTableWideRubensSE,
                   sizeof(BlackLevelTableWideRubensSE));
            length =
                sizeof(BlackLevelTableWideRubensSE) / (sizeof(MInt32) * MAX_BLACK_LEVEL_COLUMN);
        }
        break;
    case CAMERA_MODE_XIAOMI_L11_D2000_SKG4H7_N:
    case CAMERA_MODE_XIAOMI_L11A_D1500_SKG4H7_N:
        memcpy(BlackLevelTable, BlackLevelTableUW, sizeof(BlackLevelTableUW));
        length = sizeof(BlackLevelTableUW) / (sizeof(MInt32) * MAX_BLACK_LEVEL_COLUMN);
        break;
    case CAMERA_MODE_XIAOMI_L2M_D9000_OV13B:
        memcpy(BlackLevelTable, BlackLevelTableUWDaumierSNSE, sizeof(BlackLevelTableUWDaumierSNSE));
        length = sizeof(BlackLevelTableUWDaumierSNSE) / (sizeof(MInt32) * MAX_BLACK_LEVEL_COLUMN);
        break;
    default:
        memset(BlackLevelTable, 0, sizeof(BlackLevelTable));
        break;
    }

    while (index < length) {
        if (totalGain < BlackLevelTable[index].gkey) {
            break;
        } else {
            index++;
        }
    }

    if ((index >= 1) && index < length) {
        blackLevel = BlackLevelTable[index - 1];
    } else if (totalGain >= BlackLevelTable[length - 1].gkey) {
        blackLevel = BlackLevelTable[length - 1];
    } else {
        blackLevel = BlackLevelTable[0];
    }
}

void RawSuperlightPlugin::getDumpFileName(char *fileName, size_t size, const char *fileType,
                                          const char *timeBuf)
{
    char buf[128];
    memset(buf, 0, sizeof(buf));

    if (m_frameworkCameraId == CAM_COMBMODE_REAR_WIDE) {
        snprintf(buf, sizeof(buf), "W_");
    } else if (m_frameworkCameraId == CAM_COMBMODE_REAR_ULTRA) {
        snprintf(buf, sizeof(buf), "U_");
    } else if (m_frameworkCameraId == CAM_COMBMODE_FRONT) {
        snprintf(buf, sizeof(buf), "F_");
    }

    if (m_isSENeeded) {
        strncat(buf, "SE", sizeof(buf) - strlen(buf) - 1);
    } else {
        strncat(buf, "SN", sizeof(buf) - strlen(buf) - 1);
    }
    snprintf(fileName, size, "IMG_%s_%s_%s_%dx%d", timeBuf, buf, fileType, m_format.width,
             m_format.height);
}

bool RawSuperlightPlugin::isEnabled(MiaParams settings)
{
    void *pData = NULL;
    bool rawSnEnable = false;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.supernight.raw.enabled", &pData);
    // VendorMetadataParser::getTagValue(metadata, XIAOMI_SuperNight_FEATURE_ENABLED,&pData);
    if (NULL != pData) {
        rawSnEnable = *static_cast<int *>(pData);
    }
    MLOGD(Mia2LogGroupPlugin, "Get rawSnEnable: %d", rawSnEnable);
    return rawSnEnable;
}

void RawSuperlightPlugin::processRequestStat()
{
    do {
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: process Stat ============================= %s",
              SLLProcStateStrings[static_cast<uint32_t>(m_processStatus)]);

        switch (getCurrentStat()) {
        case PROCESSINIT:
            processInitAlgo();
            break;
        case PROCESSPRE:
            processBuffer();
            break;
        case PROCESSRUN:
            processAlgo();
            break;
        case PROCESSUINIT:
            processUnintAlgo();
            break;
        case PROCESSERROR:
            processError();
            break;
        case PROCESSPREWAIT:
            setNextStat(PROCESSPRE);
            break;
        case PROCESSBYPASS:
            processByPass();
            break;
        case PROCESSEND:
            return;
        }
    } while ((PROCESSPRE == getCurrentStat()) || (PROCESSUINIT == getCurrentStat()) ||
             (PROCESSRUN == getCurrentStat()));
}

void RawSuperlightPlugin::setNextStat(const PROCESSSTAT &stat)
{
    MLOGI(Mia2LogGroupPlugin, "set status %s --> %s ",
          SLLProcStateStrings[static_cast<uint32_t>(m_processStatus)],
          SLLProcStateStrings[static_cast<uint32_t>(stat)]);
    m_processStatus = stat;
}

void RawSuperlightPlugin::resetSnapshotParam()
{
    m_InputIndex = 0;
    m_algoInputNum = 0;
    m_processStatus = PROCESSINIT;

    m_processedFrame = 0;
    m_InitRet = -1;
    m_frameworkCameraId = 0;

    m_flushing = false;
    mLSCDim.width = 0;
    mLSCDim.height = 0;
    m_rawPattern = BGGR;
    m_devOrientation = 0;
    m_flushFlag = false;

    memset(&m_ExtRawValue, 0, sizeof(m_ExtRawValue));
    MMemSet(&m_OutImgRect, 0, sizeof(MRECT));
    MMemSet(m_FaceParams, 0, sizeof(ARC_SN_FACEINFO) * FRAME_MAX_INPUT_NUMBER);
}

void RawSuperlightPlugin::processInitAlgo()
{
    std::lock_guard<std::mutex> lock(m_initMutex);

    double startTime = gettime();
    m_InitRet =
        m_hArcsoftRawSuperlight->init(m_CamMode, m_camState, m_format.width, m_format.height,
                                      &m_algoVersion, m_AlgoLogLevel, m_pZoomRect, m_i32LuxIndex);

    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]: RawSuperlight init cameMode:0x%x camState %d w*h=[%d*%d] "
          " ret: %d time: %f ms",
          m_CamMode, m_camState, m_format.width, m_format.height, m_InitRet, gettime() - startTime);

    if (RET_XIOMI_TIME_OUT == m_InitRet) {
        MLOGI(Mia2LogGroupPlugin,
              "flush the last snapshot Time Out ,"
              "the cur Photo will copy buffer directly without night algorithm process!");
        setNextStat(PROCESSERROR);
        processRequestStat();
        return;
    }

    m_hArcsoftRawSuperlight->setInitStatus(1);

    if (!m_InitRet) {
        setNextStat(PROCESSPRE);
    } else {
        setNextStat(PROCESSERROR);
        processRequestStat();
    }
}

void RawSuperlightPlugin::imageToMiBuffer(ImageParams *image, MiImageBuffer *miBuf)
{
    if ((NULL == image) || (NULL == miBuf)) {
        MLOGD(Mia2LogGroupPlugin, "[ARCSOFT] null point! image=%p miBuf=%p", image, miBuf);
        return;
    }
    miBuf->format = image->format.format;
    miBuf->width = image->format.width;
    miBuf->height = image->format.height;
    miBuf->plane[0] = image->pAddr[0];
    miBuf->plane[1] = image->pAddr[1];
    miBuf->stride = image->planeStride;
    miBuf->scanline = image->sliceheight;
    miBuf->numberOfPlanes = image->numberOfPlanes;
    miBuf->fd[0] = image->fd[0];
    miBuf->fd[1] = image->fd[1];
    miBuf->fd[2] = image->fd[2];
}

int RawSuperlightPlugin::processAlgo()
{
    struct MiImageBuffer output;
    ASVLOFFSCREEN outputFrame;
    int32_t outputFD;
    output.format = m_outputImgParam->format.format;
    output.width = m_outputImgParam->format.width;
    output.height = m_outputImgParam->format.height;
    output.plane[0] = m_outputImgParam->pAddr[0];
    output.plane[1] = m_outputImgParam->pAddr[1];
    output.stride = m_outputImgParam->planeStride;
    output.scanline = m_outputImgParam->sliceheight;
    output.numberOfPlanes = m_outputImgParam->numberOfPlanes;

    outputFD = m_outputImgParam->fd[0];
    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]: outputFrame format:%d, width:%d, height:%d, "
          "plane[0,1]=[0x%x,0x%x],stride:%d,scanline:%d,numberOfPlanes:%d,outputFD:%d",
          output.format, output.width, output.height, output.plane[0], output.plane[1],
          output.stride, output.scanline, output.numberOfPlanes, outputFD); // debug +++

    prepareImage(&output, outputFrame, outputFD);

    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  arcsoft process begin hdl:%p bypass:%d copyIndex:%d",
          m_hArcsoftRawSuperlight, m_BypassForDebug, m_CopyFrameIndex);

    int32_t processRet = PROCSUCCESS;
    int32_t correctImageRet = PROCSUCCESS;
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: the input image count is %d", m_algoInputNum);

    if (m_InputIndex + 1 >= FRAME_MIN_INPUT_NUMBER) {
        double startTime = gettime();
        if ((m_hArcsoftRawSuperlight == NULL) || m_BypassForDebug) {
            MiImageBuffer inputFrameBuf, outputFrameBuf;
            imageToMiBuffer(m_outputImgParam, &outputFrameBuf);
            imageToMiBuffer(m_copyFromInput, &inputFrameBuf);
            PluginUtils::miCopyBuffer(&outputFrameBuf, &inputFrameBuf);
        } else if (m_hArcsoftRawSuperlight) {
            processRet = m_hArcsoftRawSuperlight->process(
                m_InputFD, m_InputFrame, &outputFD, &outputFrame, &m_OutImgRect,
                m_RawInfo[m_InputIndex].i32EV, m_algoInputNum, ARC_SN_SCENEMODE_LOWLIGHT,
                m_camState, m_RawInfo, m_FaceParams, &correctImageRet, m_EVoffset,
                m_devOrientation);
            updateCropRegionMetaData(m_pMetaData, &m_OutImgRect);
            for (int i = 0; i < FRAME_MAX_INPUT_NUMBER; i++) {
                if (m_FaceParams[i].i32FaceNum != 0) {
                    delete[] m_FaceParams[i].pFaceRects;
                    m_FaceParams[i].pFaceRects = NULL;
                }
            }
        }

        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]:  supernight arcsoft process end, process time:%f ms, processRet:%d",
              gettime() - startTime, processRet);

        int32_t isoValue;
        getISOValueInfo(m_pMetaData, &isoValue);
        if (m_dumpMask & 0x1) {
            char fileNameBuf[128];
            getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "output", m_dumpTimeBuf);
            dumpImageData(&output, (uint32_t)isoValue, fileNameBuf);
        }

        m_processedFrame++;
    } else {
        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]:  the input frame number is %d at least, don't process superlowlightraw",
              FRAME_MIN_INPUT_NUMBER);
        MiImageBuffer inputFrameBuf, outputFrameBuf;
        imageToMiBuffer(m_outputImgParam, &outputFrameBuf);
        imageToMiBuffer(m_copyFromInput, &inputFrameBuf);
        PluginUtils::miCopyBuffer(&outputFrameBuf, &inputFrameBuf);
    }
    setNextStat(PROCESSUINIT);

    if (processRet == PROCSUCCESS)
        return PROCSUCCESS;
    return PROCFAILED;
}

int RawSuperlightPlugin::processBuffer()
{
    if (PROCESSEND == getCurrentStat()) {
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: PROCESSEND");
        return 0;
    }

    int ret = 0;
    int32_t inputFrameFD;
    MiImageBuffer inputFrameBuf = {0};
    ASVLOFFSCREEN asvl_inputFrame;

    imageToMiBuffer(m_curInputImgParam, &inputFrameBuf);
    prepareImage(&inputFrameBuf, asvl_inputFrame, inputFrameFD);
    if (m_hArcsoftRawSuperlight) {
        ret = m_hArcsoftRawSuperlight->processPrepare(&inputFrameFD, &asvl_inputFrame, m_camState,
                                                      m_InputIndex, m_InputIndex + 1,
                                                      &m_RawInfo[m_InputIndex], bracketEv);
    }
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: processBuffer m_InputIndex %d, processPre ret: %d",
          m_InputIndex.load(), ret);

    if (!ret) {
        setNextStat(PROCESSPREWAIT);
        if (m_InputIndex == m_algoInputNum - 1) {
            setNextStat(PROCESSRUN);
        }
    } else {
        setNextStat(PROCESSERROR);
        processRequestStat();
    }
    return ret;
}

void RawSuperlightPlugin::processByPass()
{
    if (m_InputIndex == m_algoInputNum - 1) {
        if (m_outputImgParam->pAddr[0] && m_copyFromInput->pAddr[0]) {
            MiImageBuffer inputFrameBuf, outputFrameBuf;
            imageToMiBuffer(m_outputImgParam, &outputFrameBuf);
            imageToMiBuffer(m_copyFromInput, &inputFrameBuf);
            PluginUtils::miCopyBuffer(&outputFrameBuf, &inputFrameBuf);

            MLOGI(Mia2LogGroupPlugin, "copy InputIndex: %d to output", m_CopyFrameIndex);

            setNextStat(PROCESSEND);
        }
    } else {
        setNextStat(PROCESSBYPASS);
    }
}

void RawSuperlightPlugin::processUnintAlgo()
{
    m_hArcsoftRawSuperlight->uninit();
    m_InitRet = -1;
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: process UnintAlgo");
    setNextStat(PROCESSEND);
}

void RawSuperlightPlugin::processError()
{
    if (m_InputIndex == m_algoInputNum - 1) {
        if (m_outputImgParam->pAddr[0] && m_copyFromInput->pAddr[0]) {
            int correctImageRet = INIT_VALUE;
            MiImageBuffer inputFrameBuf, outputFrameBuf;
            ASVLOFFSCREEN asvl_outputFrame;
            imageToMiBuffer(m_outputImgParam, &outputFrameBuf);
            imageToMiBuffer(m_copyFromInput, &inputFrameBuf);
            PluginUtils::miCopyBuffer(&outputFrameBuf, &inputFrameBuf);

            prepareImage(&outputFrameBuf, asvl_outputFrame, m_outputImgParam->fd[0]);
            if (m_hArcsoftRawSuperlight) {
                correctImageRet = m_hArcsoftRawSuperlight->processCorrectImage(
                    &m_outputImgParam->fd[0], &asvl_outputFrame, m_camState,
                    &m_RawInfo[m_CopyFrameIndex]);
            }
            MLOGI(Mia2LogGroupPlugin, "copy EV[%d] to output buffer, correctImageRet: %d",
                  m_RawInfo[m_CopyFrameIndex].i32EV[0], correctImageRet);
            setNextStat(PROCESSUINIT);
        }
    }
}

void RawSuperlightPlugin::preProcessInput()
{
    // prepare input frame
    struct MiImageBuffer inputFrame;
    camera_metadata_t *pMetaData;
    int32_t i32ISOValue;

    inputFrame.format = m_curInputImgParam->format.format;
    inputFrame.width = m_curInputImgParam->format.width;
    inputFrame.height = m_curInputImgParam->format.height;
    inputFrame.plane[0] = m_curInputImgParam->pAddr[0];
    inputFrame.plane[1] = m_curInputImgParam->pAddr[1];
    inputFrame.stride = m_curInputImgParam->planeStride;
    inputFrame.scanline = m_curInputImgParam->sliceheight;
    inputFrame.numberOfPlanes = m_curInputImgParam->numberOfPlanes;

    m_InputFD[m_InputIndex] = m_curInputImgParam->fd[0];

    m_ExtRawValue.validFrameCount += 1;

    void *pData = NULL;
    pMetaData = m_curInputImgParam->pMetadata;
    // 1.get ev
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &pData);
    if (NULL != pData) {
        int32_t perFrameEv = *static_cast<int32_t *>(pData);
        bracketEv[m_InputIndex] = perFrameEv;
        m_ExtRawValue.Ev[m_InputIndex] = bracketEv[m_InputIndex];
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: Get bracketEv[%d] = %d", m_InputIndex.load(),
              perFrameEv);
    } else {
        bracketEv[m_InputIndex] = 404;
        MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: Failed to get frame -> #%d's ev!",
              m_InputIndex.load());
    }

    // 2.get isp gain
    pData = NULL;

    if (m_InputIndex == 0) {
        VendorMetadataParser::getTagValue(pMetaData, ANDROID_JPEG_ORIENTATION, &pData);
        if (NULL != pData) {
            int32_t orientation = 0;
            orientation = *static_cast<int32_t *>(pData);
            m_devOrientation = (orientation) % 360;
            MLOGI(Mia2LogGroupPlugin, "get Orientation from tag ANDROID_JPEG_ORIENTATION, value:%d",
                  m_devOrientation);
        }
        pData = NULL;
        VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.multiframe.inputNum", &pData);
        if (NULL != pData) {
            m_algoInputNum = *static_cast<int32_t *>(pData);
            MLOGI(Mia2LogGroupPlugin, "get supernight inputNum = %d", m_algoInputNum);
        } else {
            MLOGI(Mia2LogGroupPlugin, "get multiframe inputNum fail");
        }

        m_pMetaData = pMetaData;
    }
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.supernight.fispgain", &pData);
    // VendorMetadataParser::getTagValue(pMetaData, XIAOMI_SUPERNIGHT_FISPGAIN, &pData);
    if (NULL != pData) {
        int32_t i32IspGain = *static_cast<int32_t *>(pData);
        m_ExtRawValue.ISPGain[m_InputIndex] = (MFloat)((MFloat)i32IspGain / 4096.0f);
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: get ISPGain[%d] from xiaomi.supernight.fispgain: %f",
              m_InputIndex.load(), m_ExtRawValue.ISPGain[m_InputIndex]);
    }
    // 3.get sensor gain
    pData = NULL;
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.supernight.fsensorgain", &pData);
    if (NULL != pData) {
        int32_t i32SensorGain = *static_cast<int32_t *>(pData);
        m_ExtRawValue.SensorGain[m_InputIndex] = (MFloat)((MFloat)i32SensorGain / 1024.0f);
        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]: get SensorGain[%d] from xiaomi.supernight.fsensorgain: %f",
              m_InputIndex.load(), m_ExtRawValue.SensorGain[m_InputIndex]);
    }
    // 4.get shutter
    pData = NULL;
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.supernight.fshutter", &pData);
    if (NULL != pData) {
        m_ExtRawValue.Shutter[m_InputIndex] = (MFloat) * static_cast<int64_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: get Shutter[%d] from xiaomi.supernight.fshutter: %f",
              m_InputIndex.load(), m_ExtRawValue.Shutter[m_InputIndex]);
    }

    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]: %d, format:%d, w/h=(%dx%d), stride:%d, scanline:%d, numofplanes:%d, "
          "plane[0,1]=(%p,%p),fd(0,1)=(%d,%d)",
          m_InputIndex.load(), inputFrame.format, inputFrame.width, inputFrame.height,
          inputFrame.stride, inputFrame.scanline, inputFrame.numberOfPlanes, inputFrame.plane[0],
          inputFrame.plane[1], m_curInputImgParam->fd[0], m_curInputImgParam->fd[1]);

    MIARAWFORMAT rawFormat = {0};
    switch (m_format.format) {
    case CAM_FORMAT_RAW10:
        rawFormat.bitsPerPixel = 10;
        break;
    case CAM_FORMAT_RAW12:
    case CAM_FORMAT_RAW16:
        rawFormat.bitsPerPixel = 12;
        break;
    }

    int32_t i = m_InputIndex.load();
    char fileNameBuf[128];
    if (0 == m_InputIndex) {
        gettimestr(m_dumpTimeBuf, sizeof(m_dumpTimeBuf));
        MLOGI(Mia2LogGroupPlugin, "m_dumpTimeBuf:%s", m_dumpTimeBuf);
    }
    getRawInfo(pMetaData, &rawFormat, &m_RawInfo[m_InputIndex], m_InputIndex.load());
    getFaceInfo(pMetaData, &m_FaceParams[i]);
    prepareImage(&inputFrame, m_InputFrame[i], m_InputFD[i]);
    getISOValueInfo(pMetaData, &i32ISOValue);
    if (m_dumpMask & 0x1) {
        memset(fileNameBuf, 0, sizeof(fileNameBuf));
        getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "input", m_dumpTimeBuf);
        dumpImageData(&inputFrame, m_InputIndex, fileNameBuf);
    }

    if (m_dumpMask & 0x2) {
        memset(fileNameBuf, 0, sizeof(fileNameBuf));
        getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "rawinfo", m_dumpTimeBuf);
        dumpRawInfoExt(&inputFrame, m_InputIndex, m_algoInputNum, ARC_SN_SCENEMODE_LOWLIGHT,
                       m_CamMode, &m_FaceParams[i], &m_RawInfo[i], fileNameBuf);
    }

    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: i %d, format: %d", m_InputIndex.load(),
          inputFrame.format);
}
