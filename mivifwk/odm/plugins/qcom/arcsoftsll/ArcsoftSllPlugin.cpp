#include "ArcsoftSllPlugin.h"

#include <stdarg.h>
#include <utils/String8.h>

#include <algorithm>

#include "MiaPluginUtils.h"
#include "MiaPostProcType.h"
#include "camlog.h"

#undef LOG_TAG
#define LOG_TAG "ArcSLLPlugin"

using namespace mialgo2;

const static int32_t EV0 = 0;
static const char g_RearNightDumpDirectory[] = "/data/vendor/camera/RearNight/";
static long long __attribute__((unused)) gettime()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((long long)time.tv_sec * 1000 + time.tv_usec / 1000);
}

static void __attribute__((unused)) gettimestr(char *timestr, size_t size)
{
    char str[128];
    time_t currentTime;
    struct tm *timeInfo;
    time(&currentTime);
    timeInfo = localtime(&currentTime);
    strftime(str, size, "%Y%m%d_%H%M%S", timeInfo);

    struct timeval time;
    gettimeofday(&time, NULL);
    snprintf(timestr, size, "%s_%03d", str, (time.tv_usec / 1000));
}

int printAndroidAndOfflineLog(int prio, const char *tag, const char *fmt, ...)
{
    // ANDROID_LOG_UNKNOWN = 0, ANDROID_LOG_DEFAULT = 1, ANDROID_LOG_VERBOSE = 2, ANDROID_LOG_DEBUG
    // = 3, ANDROID_LOG_INFO = 4, ANDROID_LOG_WARN = 5, ANDROID_LOG_ERROR = 6, ANDROID_LOG_FATAL =
    // 7, ANDROID_LOG_SILENT = 8
    const char *level[10] = {"U", "T", "V", "D", "I", "W", "E", "F", "S"};
    char strBuffer[midebug::Log::MaxLogLength] = {0};
    va_list args;
    va_start(args, fmt);
    snprintf(strBuffer, sizeof(strBuffer), " %s ", tag);
    vsnprintf(strBuffer + strlen(strBuffer), sizeof(strBuffer) - strlen(strBuffer) - 1, fmt, args);
    va_end(args);

    // use ANDROID_LOG_INFO level to print algo log, avoiding creating jira for error level log.
    Log::writeLogToFile(Mia2LogGroupPlugin, level[4], "%s", strBuffer);
    return 0;
}

ArcsoftSllPlugin::~ArcsoftSllPlugin() {}

int ArcsoftSllPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    if (NULL == createInfo) {
        MLOGE(Mia2LogGroupPlugin, "%s:%d Invalid argument: createInfo is NULL.", __func__,
              __LINE__);
        return 0;
    }

    m_hSuperLowlightRaw = NULL;
    m_format = createInfo->inputFormat;
    mNodeInterface = nodeInterface;
    mLSCDim.width = 0;
    mLSCDim.height = 0;
    m_isOISSupport = false;
    m_needUninitHTP = false;
    m_hasInitNormalInput = false;
    m_isSupportSat = true;
    m_flushFlag = false;
    m_timeOutFlag = false;
    m_procRet = m_imgCorrectRet = INIT_VALUE;
    m_InitRet = INIT_VALUE;
    m_processedFrame = 0;
    m_processTime = 0;
    m_FakeProcTime = 0;
    m_InputIndex = 0;
    m_algoInputNum = 0;
    m_preEVDiff = INIT_EV_VALUE;
    m_frameworkCameraId = 0;
    m_rawPattern = RGGB;
    m_camState = ARC_SN_CAMERA_STATE_HAND;
    m_SceneMode = ARC_SN_SCENEMODE_LOWLIGHT;
    m_processStatus = PROCESSINIT;
    m_superNightProMode = DefaultMode;
    m_outputImgParam = new ImageParams;
    m_normalInput = new ImageParams;
    m_closestEV0Input = new ImageParams;
    memset(m_outputImgParam, 0, sizeof(ImageParams));
    memset(m_normalInput, 0, sizeof(ImageParams));
    memset(m_closestEV0Input, 0, sizeof(ImageParams));
    memset(&m_FaceParams, 0, sizeof(ARC_SN_FACEINFO));
    m_pFaceRects = new MRECT[ARCSOFT_MAX_FACE_NUMBER];
    m_FaceParams.pFaceRects = m_pFaceRects;
    memset(&m_OutImgRect, 0, sizeof(MRECT));
    memset(&m_algoVersion, 0, sizeof(MPBASE_Version));
    memset(&m_tuningParam, 0, sizeof(ChiTuningModeParameter));
    m_currentCpuFreq1 = 0;
    m_currentCpuFreq4 = 0;
    m_currentCpuFreq7 = 0;
    m_maxCpuFreq1 = 0;
    m_maxCpuFreq4 = 0;
    m_maxCpuFreq7 = 0;
#ifdef INGRES_CAM
    m_isSupportSat = false;
#endif
    m_snapshotMode = SNAPSHOTMODE::SM_NONE;
    switch (createInfo->operationMode) {
    case mialgo2::StreamConfigModeMiuiSuperNight:
    case mialgo2::StreamConfigModeThirdPartySuperNight:
        m_snapshotMode = SNAPSHOTMODE::SM_NIGHT;
        break;
    case mialgo2::StreamConfigModeSAT:
        m_snapshotMode = SNAPSHOTMODE::SM_SE_SAT;
        break;
    case mialgo2::StreamConfigModeBokeh:
        m_snapshotMode = SNAPSHOTMODE::SM_SE_BOKEN;
        break;
    }
    MLOGD(
        Mia2LogGroupPlugin,
        "[ARCSOFT]: frameworkCameraId: %d logicalCameraId: %d operationMode: 0x%x snapshotMode: %s",
        createInfo->frameworkCameraId, createInfo->logicalCameraId, createInfo->operationMode,
        SnapshotModeString[static_cast<uint32_t>(m_snapshotMode)]);
    ImageFormat imageFormat = {CAM_FORMAT_RAW16, createInfo->outputFormat.width,
                               createInfo->outputFormat.height};
    mNodeInterface.pSetOutputFormat(mNodeInterface.owner, imageFormat);
    setProperty();
    if (m_isOpenAlgoLog) {
        ARC_SN_SetLogLevel(1, printAndroidAndOfflineLog);
    }

    return 0;
}

void ArcsoftSllPlugin::processRequestStat()
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

void ArcsoftSllPlugin::processUnintAlgo()
{
    if ((m_hSuperLowlightRaw) && (m_hSuperLowlightRaw->getInitFlag()) &&
        (RET_XIOMI_TIME_OUT != m_InitRet) && (MERR_INVALID_PARAM != m_InitRet) &&
        (INIT_VALUE != m_InitRet)) {
        m_hSuperLowlightRaw->uninit();
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: process UnintAlgo");
    }
    setNextStat(PROCESSEND);
}

void ArcsoftSllPlugin::processInitAlgo()
{
    std::lock_guard<std::mutex> lock(m_initMutex);
    camera_metadata_t *pInputMetadata = (m_curInputImgParam->pPhyCamMetadata == NULL)
                                            ? m_curInputImgParam->pMetadata
                                            : m_curInputImgParam->pPhyCamMetadata;

    m_hSuperLowlightRaw = ArcsoftSuperLowlightRaw::getInstance();
    if (m_hSuperLowlightRaw) {
        double startTime = gettime();
        m_InitRet = m_hSuperLowlightRaw->init(m_CamMode, m_camState, m_format.width,
                                              m_format.height, &m_algoVersion, &m_InputZoomRect,
                                              m_rawInfoCurFrame.i32LuxIndex);
        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]: SLL init cameMode:0x%x camState %d sceneMode %d w*h=[%d*%d] "
              "inputZoomRect=[%d %d %d %d] luxIndex:%d ret: %d time: %f ms",
              m_CamMode, m_camState, m_SceneMode, m_format.width, m_format.height,
              m_InputZoomRect.left, m_InputZoomRect.top, m_InputZoomRect.right,
              m_InputZoomRect.bottom, m_rawInfoCurFrame.i32LuxIndex, m_InitRet,
              gettime() - startTime);

#ifdef MCAM_SW_DIAG
        void *pData = NULL;
        VendorMetadataParser::getVTagValue(pInputMetadata, "xiaomi.diag.mask", &pData);
        if (NULL != pData) {
            if (RET_ADSP_INIT_ERROR == m_InitRet) {
                MLOGE(Mia2LogGroupPlugin, "[ARCSOFT]: algo init error:%ld", m_InitRet);
                abort();
            }
        }
#endif

        if (RET_XIOMI_TIME_OUT == m_InitRet) {
            MLOGI(Mia2LogGroupPlugin,
                  "flush the last snapshot Time Out [%lds],"
                  "the cur Photo will copy buffer directly without night algorithm process!",
                  FlushTimeoutSec);
            m_timeOutFlag = true;
            setNextStat(PROCESSERROR);
            processRequestStat();
            return;
        }

        m_hSuperLowlightRaw->setInitStatus(1);
        if (!m_InitRet) {
            setNextStat(PROCESSPRE);
        } else {
            setNextStat(PROCESSERROR);
            processRequestStat();
        }
    }
}

int ArcsoftSllPlugin::processBuffer()
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
    prepareImage(inputFrameBuf, asvl_inputFrame, inputFrameFD);
    if (m_hSuperLowlightRaw) {
        ret =
            m_hSuperLowlightRaw->processPrepare(&inputFrameFD, &asvl_inputFrame, m_camState,
                                                m_InputIndex, m_InputIndex + 1, &m_rawInfoCurFrame);
    }
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: processBuffer m_InputIndex %d, processPre ret: %d",
          m_InputIndex.load(), ret);

#ifdef MCAM_SW_DIAG
    camera_metadata_t *pInputMata = m_curInputImgParam->pPhyCamMetadata
                                        ? m_curInputImgParam->pPhyCamMetadata
                                        : m_curInputImgParam->pMetadata;
    ParamDiag(pInputMata);
#endif

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

int ArcsoftSllPlugin::processAlgo()
{
    int ret = 0;
    int32_t correctImageRet = INIT_VALUE;
    int32_t i32EVoffset = 0;
    MiImageBuffer normalInputFrameBuf, outputFrameBuf;
    ASVLOFFSCREEN asvl_outputFrame, asvl_normalInputFrame;

    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]: output logiMeta: %p PhyMeta: %p closestEV0Input logipMeta:%p PhyMeta: %p, "
          "processeFrameNum = %d",
          m_outputImgParam->pMetadata, m_outputImgParam->pPhyCamMetadata,
          m_closestEV0Input->pMetadata, m_closestEV0Input->pPhyCamMetadata, m_processedFrame);

    camera_metadata_t *pOutputMetadata = (m_outputImgParam->pPhyCamMetadata == NULL)
                                             ? m_outputImgParam->pMetadata
                                             : m_outputImgParam->pPhyCamMetadata;
    camera_metadata_t *pInputMetadata = (m_closestEV0Input->pPhyCamMetadata == NULL)
                                            ? m_closestEV0Input->pMetadata
                                            : m_closestEV0Input->pPhyCamMetadata;
    if (pOutputMetadata != NULL && pInputMetadata != NULL) {
        VendorMetadataParser::mergeMetadata(pOutputMetadata, pInputMetadata);
    }

    imageToMiBuffer(m_outputImgParam, &outputFrameBuf);
    prepareImage(outputFrameBuf, asvl_outputFrame, m_outputImgParam->fd[0]);

    imageToMiBuffer(m_closestEV0Input, &normalInputFrameBuf);
    prepareImage(normalInputFrameBuf, asvl_normalInputFrame, m_closestEV0Input->fd[0]);

    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]: arcsoft process begin hdl:%p faketime:%d camState:%d temp:%d cpu1:%d/%d,"
          "cpu4:%d/%d, cpu7:%d/%d",
          m_hSuperLowlightRaw, m_FakeProcTime, m_camState, m_currentTemp, m_currentCpuFreq1,
          m_maxCpuFreq1, m_currentCpuFreq4, m_maxCpuFreq4, m_currentCpuFreq7, m_maxCpuFreq7);

    double startTime = gettime();
    if (m_hSuperLowlightRaw) {
        ret = m_hSuperLowlightRaw->processAlgo(&m_outputImgParam->fd[0], &asvl_outputFrame,
                                               &asvl_normalInputFrame, m_camState, m_SceneMode,
                                               &m_OutImgRect, &m_FaceParams, &correctImageRet,
                                               &m_closestEV0RawInfo, i32EVoffset, m_devOrientation);
        if (m_FakeProcTime > 0) {
            int cnt = 0;
            while (1) {
                usleep(1 * 1000 * 1000);
                if ((++cnt >= m_FakeProcTime)) {
                    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: cnt:%d", cnt);
                    break;
                }
            }
        }

        m_procRet = ret;
        m_imgCorrectRet = correctImageRet;
        updateCropRegionMetaData(&m_OutImgRect, pOutputMetadata);
    }
    m_processTime = gettime() - startTime;
    getCurrentHWStatus();

    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]: supernight arcsoft process end, cost time:%f temp:%d cpu1:%d/%d, cpu4:%d/%d, "
          "cpu7:%d/%d",
          m_processTime, m_currentTemp, m_currentCpuFreq1, m_maxCpuFreq1, m_currentCpuFreq4,
          m_maxCpuFreq4, m_currentCpuFreq7, m_maxCpuFreq7);

    UpdateMetaData(&m_closestEV0RawInfo, correctImageRet, m_procRet);

#ifdef MCAM_SW_DIAG
    void *pData = NULL;
    VendorMetadataParser::getVTagValue(pOutputMetadata, "xiaomi.diag.mask", &pData);
    if (NULL != pData) {
        int32_t nightDiaginfo = *static_cast<int32_t *>(pData);
        if (NightCapture == (NightCapture & nightDiaginfo)) {
            if ((m_procRet != MOK) && (m_procRet != MERR_USER_CANCEL) && (m_procRet != 14)) {
                MLOGE(Mia2LogGroupPlugin, "[ParamDiag]: crash here because of m_procRet:%d-(0/6)",
                      m_procRet);
                char isSochanged[256] = "";
                // When the value of logsystem is 1 or 3, the send_message_to_mqs interface is
                // called When the value of logsystem is 2, it means that the exception needs to be
                // ignored
                property_get("vendor.camera.sensor.logsystem", isSochanged, "0");
                if (atoi(isSochanged) == 2) {
                    abort();
                }

                if ((atoi(isSochanged) == 1) || (atoi(isSochanged) == 3)) {
                    char CamErrorString[256] = {0};
                    sprintf(CamErrorString,
                            "[SuperNight][ParamDiag-MIVI]: crash here for debug only!");
                    MLOGE(Mia2LogGroupPlugin, "[SuperNight][ParamDiag-MIVI] %s", CamErrorString);
                    camlog::send_message_to_mqs(CamErrorString, false);
                }
            }
        }
    }
#endif

    if (m_dumpMask & 0x1) {
        int32_t i32ISOValue = 0;
        char fileNameBuf[128] = {0};
        memset(fileNameBuf, 0, sizeof(fileNameBuf));
        getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "output", m_dumpTimeBuf);
        getISOValueInfo(&i32ISOValue, pInputMetadata);
        dumpToFile(fileNameBuf, &outputFrameBuf, i32ISOValue, "output");
    }
    m_processedFrame++;
    setNextStat(PROCESSUINIT);

    return ret;
}

void ArcsoftSllPlugin::processError()
{
    if (m_InputIndex == m_algoInputNum - 1) {
        if (m_outputImgParam->pAddr[0] && m_closestEV0Input->pAddr[0]) {
            int correctImageRet = INIT_VALUE;
            MiImageBuffer inputFrameBuf, outputFrameBuf;
            ASVLOFFSCREEN asvl_outputFrame;
            imageToMiBuffer(m_outputImgParam, &outputFrameBuf);
            imageToMiBuffer(m_closestEV0Input, &inputFrameBuf);
            PluginUtils::miCopyBuffer(&outputFrameBuf, &inputFrameBuf);

            prepareImage(outputFrameBuf, asvl_outputFrame, m_outputImgParam->fd[0]);
            if (m_hSuperLowlightRaw) {
                correctImageRet = m_hSuperLowlightRaw->processCorrectImage(
                    &m_outputImgParam->fd[0], &asvl_outputFrame, m_camState, &m_closestEV0RawInfo);
                m_imgCorrectRet = correctImageRet;
            }
            MLOGI(Mia2LogGroupPlugin, "copy EV[%d] to output buffer, correctImageRet: %d",
                  m_closestEV0RawInfo.i32EV[0], correctImageRet);
            UpdateMetaData(&m_closestEV0RawInfo, correctImageRet, m_procRet);
            setNextStat(PROCESSUINIT);
        }
    }
}

void ArcsoftSllPlugin::preProcessInput()
{
    char fileNameBuf[128];
    MiImageBuffer inputFrameBuf;
    camera_metadata_t *pInputMata = m_curInputImgParam->pPhyCamMetadata
                                        ? m_curInputImgParam->pPhyCamMetadata
                                        : m_curInputImgParam->pMetadata;
    if (0 == m_InputIndex) {
        getMetaData(pInputMata);
        initAlgoParam();
        getCurrentHWStatus();
        getInputZoomRect(&m_InputZoomRect, &m_firstImgRect, pInputMata);
        gettimestr(m_dumpTimeBuf, sizeof(m_dumpTimeBuf));
        uint64_t dumpTime = gettime();
        m_dumpTime[0] = dumpTime / 1000000;
        m_dumpTime[1] = dumpTime % 1000000;
        MLOGI(Mia2LogGroupPlugin, "m_dumpTimeBuf:%s", m_dumpTimeBuf);
    }

    memset(&m_rawInfoCurFrame, 0, sizeof(ARC_SN_RAWINFO));
    getInputRawInfo(m_curInputImgParam, pInputMata, &m_rawInfoCurFrame);

    if ((EV0 == m_rawInfoCurFrame.i32EV[0]) && (false == m_hasInitNormalInput)) {
        CopyRawInfo(&m_rawInfoCurFrame, &m_normalRawInfo, 1);
        memcpy(m_closestEV0Input, m_curInputImgParam, sizeof(ImageParams));
        getFaceInfo(&m_FaceParams, pInputMata);
        m_hasInitNormalInput = true;
    }

    // record the frame closest to EV0
    int32_t evDiff = m_rawInfoCurFrame.i32EV[0] - EV0;
    if (abs(evDiff) < abs(m_preEVDiff)) {
        m_preEVDiff = evDiff;
        CopyRawInfo(&m_rawInfoCurFrame, &m_closestEV0RawInfo, 1);
        memcpy(m_closestEV0Input, m_curInputImgParam, sizeof(ImageParams));
        MLOGI(Mia2LogGroupPlugin, "update closestEV0 frame index:%d EV:%d", m_InputIndex.load(),
              m_rawInfoCurFrame.i32EV[0]);
    }

    if (m_dumpMask) {
        if (m_dumpMask & 0x1) {
            memset(fileNameBuf, 0, sizeof(fileNameBuf));
            getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "input", m_dumpTimeBuf);
            imageToMiBuffer(m_curInputImgParam, &inputFrameBuf);
            dumpToFile(fileNameBuf, &inputFrameBuf, m_InputIndex, "input");
        }

        if (m_dumpMask & 0x2) {
            memset(fileNameBuf, 0, sizeof(fileNameBuf));
            getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "rawinfo", m_dumpTimeBuf);
            dumpRawInfoExt((*m_curInputImgParam), m_InputIndex, m_algoInputNum, m_SceneMode,
                           m_CamMode, &m_FaceParams, &m_rawInfoCurFrame, fileNameBuf);
        }
    }
}

void ArcsoftSllPlugin::processByPass()
{
    MiImageBuffer inputFrameBuf, outputFrameBuf;
    if (m_InputIndex == (m_algoInputNum - 1)) {
        if (m_outputImgParam->pAddr[0] && m_closestEV0Input->pAddr[0]) {
            imageToMiBuffer(m_outputImgParam, &outputFrameBuf);
            imageToMiBuffer(m_closestEV0Input, &inputFrameBuf);
            PluginUtils::miCopyBuffer(&outputFrameBuf, &inputFrameBuf);
            MLOGI(Mia2LogGroupPlugin, "copy InputIndex: %d to output", m_InputIndex.load());
        }
        if (m_FakeProcTime > 0) {
            int cnt = 0;
            while (1) {
                usleep(1 * 1000 * 1000);
                if ((++cnt >= m_FakeProcTime)) {
                    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: cnt:%d", cnt);
                    break;
                }
            }
        }
        setNextStat(PROCESSEND);
    } else {
        setNextStat(PROCESSBYPASS);
    }
}

ProcessRetStatus ArcsoftSllPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    std::lock_guard<std::mutex> lock(m_flushMutex);
    ProcessRetStatus resultInfo = PROCSUCCESS;

    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;
    uint32_t inNum = inputBuffers.size();
    uint32_t outNum = outputBuffers.size();
    MLOGI(Mia2LogGroupPlugin, "processRequestInfo frameNum %d inNum %d outNum %d",
          processRequestInfo->frameNum, inNum, outNum);

    if (!inNum) {
        MLOGE(Mia2LogGroupPlugin, "FrameNum %d stream %d input buffer is NULL!",
              processRequestInfo->frameNum, processRequestInfo->streamId);
        resultInfo = PROCFAILED;
        return resultInfo;
    }

    if (processRequestInfo->isFirstFrame && !outNum) {
        MLOGE(Mia2LogGroupPlugin, "FrameNum %d stream %d as first frame, output buffer is NULL!",
              processRequestInfo->frameNum, processRequestInfo->streamId);
        resultInfo = PROCFAILED;
        return resultInfo;
    }

    if (m_flushFlag) {
        if (m_hSuperLowlightRaw) {
            m_hSuperLowlightRaw->setInitStatus(0);
            MLOGI(Mia2LogGroupPlugin, "set user cancel!");
        }
        setNextStat(PROCESSUINIT);
        processRequestStat();
        m_flushFlag = false;
        return resultInfo;
    }

    std::shared_ptr<PluginPerfLockManager> boostCpu(new PluginPerfLockManager(3000)); // 3000 ms

    if (processRequestInfo->isFirstFrame && outNum > 0) {
        resetSnapshotParam();
        memcpy(m_outputImgParam, &outputBuffers[0], sizeof(ImageParams));
        memcpy(m_normalInput, &inputBuffers[0], sizeof(ImageParams));
        memcpy(m_closestEV0Input, &inputBuffers[0], sizeof(ImageParams));
        m_outputFrameNum = processRequestInfo->frameNum;
        m_outputTimeStamp = processRequestInfo->timeStamp;
        m_format.width = m_outputImgParam->format.width;
        m_format.height = m_outputImgParam->format.height;
        MLOGI(Mia2LogGroupPlugin,
              "FrameNum %d output Stride: %d, height: %d, format:%d logiMata:%p "
              "phyMeta:%p imgAddr:%p",
              processRequestInfo->frameNum, m_outputImgParam->planeStride,
              m_outputImgParam->sliceheight, m_outputImgParam->format.format,
              m_outputImgParam->pMetadata, m_outputImgParam->pPhyCamMetadata,
              m_outputImgParam->pAddr[0]);
    }

    if (inNum > 0) {
        m_curInputImgParam = &inputBuffers[0];
        MLOGI(Mia2LogGroupPlugin,
              "FrameNum %d input[%d] planeStride: %d, sliceheight: %d, format: %d logiMata: %p "
              "phyMeta: %p imageAddr: %p",
              processRequestInfo->frameNum, m_InputIndex.load(), m_curInputImgParam->planeStride,
              m_curInputImgParam->sliceheight, m_curInputImgParam->format.format,
              m_curInputImgParam->pMetadata, m_curInputImgParam->pPhyCamMetadata,
              m_curInputImgParam->pAddr[0]);
        preProcessInput();
    }

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

    return resultInfo;
}

ProcessRetStatus ArcsoftSllPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo;
    resultInfo = PROCSUCCESS;
    return resultInfo;
}

void ArcsoftSllPlugin::initAlgoParam()
{
    // init m_camState
    if (m_camStateForDebug) {
        m_camState = m_camStateForDebug;
    } else if ((m_snapshotMode == SNAPSHOTMODE::SM_SE_SAT) ||
               (m_snapshotMode == SNAPSHOTMODE::SM_SE_BOKEN)) {
        m_camState = ARC_SN_CAMERA_STATE_AUTOMODE;
    } else if (m_snapshotMode == SNAPSHOTMODE::SM_NIGHT) {
        m_camState = ARC_SN_CAMERA_STATE_HAND;
    } else {
        m_camState = ARC_SN_CAMERA_STATE_UNKNOWN;
    }

    // init m_SceneMode
    if (m_snapshotMode == SNAPSHOTMODE::SM_SE_BOKEN) {
        m_SceneMode = ARC_SN_SCENEMODE_SE_BOKEH;
    } else {
        m_SceneMode = ARC_SN_SCENEMODE_LOWLIGHT;
    }

    // init m_CamMode
    m_CamMode = ARC_SN_CAMERA_MODE_UNKNOWN;
    uint32_t wideCamMode = -1;
    uint32_t ultraCamMode = -1;
    uint32_t teleCamMode = -1;

#ifdef ZEUS_CAM
    wideCamMode = CAMERA_MODE_XIAOMI_L2_SDM8450_IMX707_Y;
    ultraCamMode = CAMERA_MODE_XIAOMI_L2_SDM8450_IJN1_N_UW;
    teleCamMode = CAMERA_MODE_XIAOMI_L2_SDM8450_IJN1_N_TELE;
#elif defined THOR_CAM
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    wideCamMode = CAMERA_MODE_XIAOMI_L1_8450_IMX989;
    ultraCamMode = CAMERA_MODE_XIAOMI_L1_8450_IMX586_UW;
    teleCamMode = CAMERA_MODE_XIAOMI_L1_8450_IMX586_TELE;
    property_get("ro.boot.camera.config", prop, "0");
    int PropID = atoi(prop);
    if (1 == PropID) /* config of L1A */
    {
        wideCamMode = CAMERA_MODE_XIAOMI_L1A_8450_IMX989;
        ultraCamMode = CAMERA_MODE_XIAOMI_L1A_8450_JN1_UW;
        teleCamMode = CAMERA_MODE_XIAOMI_L1A_8450_JN1_TELE;
    }
#elif defined CUPID_CAM
    wideCamMode = CAMERA_MODE_XIAOMI_L3_SDM8450_IMX766_Y;
    ultraCamMode = CAMERA_MODE_XIAOMI_L3_SDM8450_OV13B10_N;
#elif defined INGRES_CAM
    wideCamMode = CAMERA_MODE_XIAOMI_L10_SDM8450_IMX686;
    ultraCamMode = CAMERA_MODE_XIAOMI_L10_SDM8450_OV8856;
#elif defined ZIZHAN_CAM
    wideCamMode = CAMERA_MODE_XIAOMI_L18_SDM8450_IMX766_Y;
    ultraCamMode = CAMERA_MODE_XIAOMI_L18_SDM8450_OV13B10_N;
#elif defined MAYFLY_CAM
    wideCamMode = CAMERA_MODE_XIAOMI_L2_SDM8450_IMX707_Y;
    ultraCamMode = CAMERA_MODE_XIAOMI_L3_SDM8450_OV13B10_N;
#elif defined DITING_CAM
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    wideCamMode = CAMERA_MODE_XIAOMI_L12_INT_PRO_SDM8475_S5KHP1;
    ultraCamMode = CAMERA_MODE_XIAOMI_L12_INT_PRO_SDM8475_S5K4H7;
    property_get("ro.boot.camera.config", prop, "0");
    int PropID = atoi(prop);
    if (1 == PropID) /* config of L12 cn */
    {
        wideCamMode = CAMERA_MODE_XIAOMI_L12_INT_SDM8475_HM6;
        ultraCamMode = CAMERA_MODE_XIAOMI_L12_INT_SDM8475_S5K4H7;
    }

#endif
    switch (m_cameraRoleID) {
    case LENS_ULTRA:
        m_CamMode = ultraCamMode;
        break;
    case LENS_WIDE:
        m_CamMode = wideCamMode;
        break;
    case LENS_TELE:
        m_CamMode = teleCamMode;
        break;
    default:
        m_CamMode = ARC_SN_CAMERA_MODE_UNKNOWN;
    }
}

void ArcsoftSllPlugin::resetSnapshotParam()
{
    m_procRet = m_imgCorrectRet = INIT_VALUE;
    m_InitRet = INIT_VALUE;
    m_processTime = 0;
    m_InputIndex = 0;
    m_algoInputNum = 0;
    m_processStatus = PROCESSINIT;
    m_hasInitNormalInput = false;
    m_timeOutFlag = false;
    m_preEVDiff = 0xFFFF;

    memset(&m_tuningParam, 0, sizeof(ChiTuningModeParameter));
    memset(m_dumpTimeBuf, 0, sizeof(m_dumpTimeBuf));
}

void ArcsoftSllPlugin::getCropRegionSmallToLarge(ChiRect *cropRegion, const uint32_t refWidth,
                                                 const uint32_t refHeight,
                                                 const uint32_t targetWidth,
                                                 const uint32_t targetHeight)
{
    if ((targetWidth < refWidth || targetHeight < refHeight) ||
        (refWidth == targetWidth && refHeight == targetHeight)) {
        MLOGI(Mia2LogGroupPlugin, "refHeight:(w,h):(%d,%d),target:(w,h):(%d,%d)", refWidth,
              refHeight, targetWidth, targetHeight);
        return;
    }
    MLOGI(Mia2LogGroupPlugin, "rect(l,t,w,h):(%d,%d,%d,%d),red(w,h):(%d,%d)", cropRegion->left,
          cropRegion->top, cropRegion->width, cropRegion->height, refWidth, refHeight);
    // adjust crop according to preZoom into buffer domain
    float preZoomRatio_width = static_cast<float>(targetWidth) / refWidth;
    float preZoomRatio_height = static_cast<float>(targetHeight) / refHeight;
    float preZoomRatio = min(preZoomRatio_width, preZoomRatio_height);
    MLOGW(Mia2LogGroupPlugin, "preZoomRatio_width %f, preZoomRatio_height %f, preZoomRatio %f",
          preZoomRatio_width, preZoomRatio_height, preZoomRatio);

    float tmpLeft, tmpTop, tmpWidth, tmpHeight;
    tmpWidth = cropRegion->width * preZoomRatio;
    tmpLeft = cropRegion->left * preZoomRatio;
    tmpHeight = cropRegion->height * preZoomRatio;
    tmpTop = cropRegion->top * preZoomRatio;

    cropRegion->left = tmpLeft;
    cropRegion->top = tmpTop;
    cropRegion->width = tmpWidth;
    cropRegion->height = tmpHeight * targetWidth / targetHeight;

    float tmpRefHeight = static_cast<float>(refHeight) * targetWidth / refWidth;
    float delta = (targetHeight - tmpRefHeight) / 2.0;
    cropRegion->top = tmpTop + delta - (cropRegion->height - tmpHeight) / 2.0;
    MLOGI(Mia2LogGroupPlugin, "---->rect(%d,%d,%d,%d),target(w,h):%d,%d", cropRegion->left,
          cropRegion->top, cropRegion->width, cropRegion->height, targetWidth, targetHeight);
}

void ArcsoftSllPlugin::getCropRegionLargeToSmall(ChiRect *cropRegion, const uint32_t refWidth,
                                                 const uint32_t refHeight,
                                                 const uint32_t targetWidth,
                                                 const uint32_t targetHeight)
{
    if ((targetWidth > refWidth || targetHeight > refHeight) ||
        (refWidth == targetWidth && refHeight == targetHeight)) {
        MLOGI(Mia2LogGroupPlugin, "refHeight:(w,h):(%d,%d),target:(w,h):(%d,%d)", refWidth,
              refHeight, targetWidth, targetHeight);
        return;
    }
    float tmpLeft = cropRegion->left;
    float tmpTop = cropRegion->top;
    float tmpWidth = cropRegion->width;
    float tmpHeight = cropRegion->height;
    MLOGI(Mia2LogGroupPlugin, "rect(l,t,w,h):(%d,%d,%d,%d),red(w,h):(%d,%d)", cropRegion->left,
          cropRegion->top, cropRegion->width, cropRegion->height, refWidth, refHeight);
    float xRatio = tmpWidth / targetWidth;
    float yRatio = tmpHeight / targetHeight;
    float xy = xRatio / yRatio;
    MLOGI(Mia2LogGroupPlugin, "xRatio:%f,yRatio:%f,xy:%f", xRatio, yRatio, xy);

    constexpr float tolerance = 0.0001f;
    if (yRatio > xRatio + tolerance) {
        tmpHeight = static_cast<float>(targetHeight) * xRatio;
        float tmpRefHeight = static_cast<float>(targetHeight) * refWidth / targetWidth;
        float delta = (refHeight - tmpRefHeight) / 2.0;
        tmpTop = cropRegion->top + (cropRegion->height - tmpHeight) / 2.0 - delta;
    } else if (xRatio > yRatio + tolerance) {
        tmpWidth = static_cast<float>(targetWidth) * yRatio;
        float tmpRefWidth = static_cast<float>(targetWidth) * refHeight / targetHeight;
        float delta = (refWidth - tmpRefWidth) / 2.0;
        tmpLeft = cropRegion->left + (cropRegion->width - tmpWidth) / 2.0 - delta;
    }
    // adjust crop according to preZoom into buffer domain
    unsigned int rectLeft, rectTop, rectWidth, rectHeight;
    float preZoomRatio_width = static_cast<float>(targetWidth) / refWidth;
    float preZoomRatio_height = static_cast<float>(targetHeight) / refHeight;
    float preZoomRatio = max(preZoomRatio_width, preZoomRatio_height);
    MLOGW(Mia2LogGroupPlugin, "preZoomRatio_width %f, preZoomRatio_height %f, preZoomRatio %f",
          preZoomRatio_width, preZoomRatio_height, preZoomRatio);
    // ZoomRatio
    rectWidth = tmpWidth * preZoomRatio;
    rectLeft = tmpLeft * preZoomRatio;
    rectHeight = tmpHeight * preZoomRatio;
    rectTop = tmpTop * preZoomRatio;

    // now we validate the rect, and ajust it if out-of-bondary found.
    if (rectLeft + rectWidth > targetWidth) {
        if (rectWidth <= targetWidth) {
            rectLeft = targetWidth - rectWidth;
        } else {
            rectLeft = 0;
            rectWidth = targetWidth;
        }
        MLOGW(Mia2LogGroupPlugin, "crop left or width is wrong, ajusted it manually!!");
    }
    if (rectTop + rectHeight > targetHeight) {
        if (rectHeight <= targetHeight) {
            rectTop = targetHeight - rectHeight;
        } else {
            rectTop = 0;
            rectHeight = targetHeight;
        }
        MLOGW(Mia2LogGroupPlugin, "crop top or height is wrong, ajusted it manually!!");
    }
    cropRegion->left = rectLeft;
    cropRegion->top = rectTop;
    cropRegion->width = rectWidth;
    cropRegion->height = rectHeight;
    MLOGI(Mia2LogGroupPlugin, "---->rect(%d,%d,%d,%d),target(w,h):%d,%d", cropRegion->left,
          cropRegion->top, cropRegion->width, cropRegion->height, targetWidth, targetHeight);
}

void ArcsoftSllPlugin::getInputZoomRect(MRECT *pInputZoomRect, ChiRect *pFirstImageRect,
                                        camera_metadata_t *metaData)
{
    void *pData = NULL;
    ChiRect cropWindow = {0, 0, m_format.width, m_format.height};
    if (isSE()) {
        VendorMetadataParser::getVTagValue(metaData, "xiaomi.snapshot.cropRegion", &pData);
        if (pData != NULL) {
            cropWindow = *(static_cast<ChiRect *>(pData));
            MLOGI(Mia2LogGroupPlugin,
                  "[ArcSoft] get xiaomi.snapshot.cropRegion l,t,w,h=[%d %d %d %d]", cropWindow.left,
                  cropWindow.top, cropWindow.width, cropWindow.height);
        } else {
            MLOGI(Mia2LogGroupPlugin, "[ArcSoft] get xiaomi.snapshot.cropRegion fail!");
        }
    } else {
        float zoomRatio = 1.0;
        VendorMetadataParser::getTagValue(metaData, ANDROID_CONTROL_ZOOM_RATIO, &pData);
        if (NULL != pData) {
            zoomRatio = *static_cast<float *>(pData);
            MLOGI(Mia2LogGroupPlugin, "[ArcSoft] zoomRatio ratio:%f", zoomRatio);
            pData = NULL;
        } else {
            MLOGI(Mia2LogGroupPlugin, "[ArcSoft] get zoom ratio fail!");
        }
        zoomRatio = (1.0f < zoomRatio) ? zoomRatio : 1.0f;
        cropWindow.width = m_format.width / zoomRatio;
        cropWindow.height = m_format.height / zoomRatio;
        cropWindow.left = (m_format.width - cropWindow.width) / 2.0f;
        cropWindow.top = (m_format.height - cropWindow.height) / 2.0f;
    }
    pFirstImageRect->left = cropWindow.left;
    pFirstImageRect->top = cropWindow.top;
    pFirstImageRect->width = cropWindow.width;
    pFirstImageRect->height = cropWindow.height;
    if (isSE()) {
        ChiRect senorArraySize = {0, 0, 4096, 3072};
        getActiveArraySize(&senorArraySize, metaData);
        getCropRegionLargeToSmall(&cropWindow, senorArraySize.width, senorArraySize.height,
                                  m_format.width, m_format.height);
    }
    pInputZoomRect->left = cropWindow.left;
    pInputZoomRect->top = cropWindow.top;
    pInputZoomRect->right = cropWindow.left + cropWindow.width;
    pInputZoomRect->bottom = cropWindow.top + cropWindow.height;
    MLOGI(Mia2LogGroupPlugin, " [ArcSoft] input cropregions(l,t,w,h)=(%d,%d,%d,%d)",
          pInputZoomRect->left, pInputZoomRect->top, pInputZoomRect->right, pInputZoomRect->bottom);
}

void ArcsoftSllPlugin::updateCropRegionMetaData(MRECT *pCropRect, camera_metadata_t *metaData)
{
    if (m_isCropInfoUpdate && pCropRect && metaData) {
        if ((pCropRect->left > m_InputZoomRect.left) || (pCropRect->top > m_InputZoomRect.top)) {
            MLOGI(Mia2LogGroupPlugin, "[ArcSoft] output rect [l,t,r,b]=[%d %d %d %d]",
                  pCropRect->left, pCropRect->top, pCropRect->right, pCropRect->bottom);
            if (isSE()) {
                ChiRect scalerCrop = {0, 0, m_format.width, m_format.height};
                scalerCrop.left = pCropRect->left;
                scalerCrop.top = pCropRect->top;
                scalerCrop.width = pCropRect->right - pCropRect->left;
                scalerCrop.height = pCropRect->bottom - pCropRect->top;
                ChiRect senorArraySize = {0, 0, 4096, 3072};
                getActiveArraySize(&senorArraySize, metaData);
                getCropRegionSmallToLarge(&scalerCrop, m_format.width, m_format.height,
                                          senorArraySize.width, senorArraySize.height);
                VendorMetadataParser::setVTagValue(metaData, "xiaomi.snapshot.cropRegion",
                                                   &scalerCrop, 4);
                MLOGI(Mia2LogGroupPlugin,
                      "[ArcSoft] multicam update algo output cropRegion [l,t,w,h]=[%d %d %d %d]",
                      scalerCrop.left, scalerCrop.top, scalerCrop.width, scalerCrop.height);
            } else {
                float zoomRatio = 1.0;
                if (m_format.width > 0) {
                    zoomRatio = static_cast<float>(m_format.width) /
                                static_cast<float>(pCropRect->right - pCropRect->left);
                    zoomRatio = (1.0f < zoomRatio) ? zoomRatio : 1.0f;

                    VendorMetadataParser::setTagValue(metaData, ANDROID_CONTROL_ZOOM_RATIO,
                                                      &zoomRatio, 1);
                    MLOGI(Mia2LogGroupPlugin, "[ArcSoft] update zoomRatio ratio:%f", zoomRatio);
                }
            }
        } else {
            // In multicamera, the crop region of multiple input frames may be different.
            // The first-frame crop region is used in the algo, so update this crop region to output
            // meta
            if (isSE()) {
                if ((m_InputZoomRect.right > m_InputZoomRect.left) &&
                    (m_InputZoomRect.bottom > m_InputZoomRect.top)) {
                    VendorMetadataParser::setVTagValue(metaData, "xiaomi.snapshot.cropRegion",
                                                       &m_firstImgRect, 4);
                    MLOGI(Mia2LogGroupPlugin,
                          "[ArcSoft] multicam update algo input cropRegion [l,t,w,h]=[%d %d %d %d]",
                          m_firstImgRect.left, m_firstImgRect.top, m_firstImgRect.width,
                          m_firstImgRect.height);
                }
            }
        }
    }
}

void ArcsoftSllPlugin::UpdateMetaData(ARC_SN_RAWINFO *rawInfo, int correctImageRet, int processRet)
{
    char debugInfoBuf[2048] = {0};
    char buf[1024] = {0};
    char checkResult[1024] = {0};
    sprintf(buf, " supernight=on,");
    strcat(debugInfoBuf, buf);
    SuperNightDebugInfo superNightDebugInfo = {0};
    superNightDebugInfo.nightMode = m_camState;
    superNightDebugInfo.algoVersion = (int32_t)m_algoVersion.lBuild;
    superNightDebugInfo.processTime = (int32_t)m_processTime;
    superNightDebugInfo.processRet = processRet;
    memcpy(superNightDebugInfo.EV, m_inputEV, sizeof(m_inputEV));
    memcpy(superNightDebugInfo.dumpTime, m_dumpTime, sizeof(m_dumpTime));
    superNightDebugInfo.initRet = m_InitRet;
    superNightDebugInfo.correctImageRet = correctImageRet;
    superNightDebugInfo.isOISSupport = m_isOISSupport;
    superNightDebugInfo.isLSCControlEnable = rawInfo->bEnableLSC;
    superNightDebugInfo.tuningParam = m_tuningParam;

    uint64_t dumpTime = static_cast<uint64_t>(superNightDebugInfo.dumpTime[0]) * 1000000 +
                        superNightDebugInfo.dumpTime[1];
    snprintf(buf, sizeof(buf),
             "%s [V%d] [Time:%dms] [InitRet:%d][processRet:%d,%d] [EV:%d,%d,%d,%d,%d,%d,%d,%d] "
             "[LSC:%d] [OIS:%d]"
             "[Tuning: D:%d S:%d U:%d F:%d %d S:%d E:%d] [dumpTime:%" PRIu64 "] [TimeOut:%d]",
             checkResult, superNightDebugInfo.algoVersion, superNightDebugInfo.processTime,
             superNightDebugInfo.initRet, superNightDebugInfo.processRet,
             superNightDebugInfo.correctImageRet, superNightDebugInfo.EV[0],
             superNightDebugInfo.EV[1], superNightDebugInfo.EV[2], superNightDebugInfo.EV[3],
             superNightDebugInfo.EV[4], superNightDebugInfo.EV[5], superNightDebugInfo.EV[6],
             superNightDebugInfo.EV[7], superNightDebugInfo.isLSCControlEnable,
             superNightDebugInfo.isOISSupport,
             superNightDebugInfo.tuningParam.TuningMode[0].subMode.value,
             superNightDebugInfo.tuningParam.TuningMode[1].subMode.value,
             superNightDebugInfo.tuningParam.TuningMode[2].subMode.usecase,
             superNightDebugInfo.tuningParam.TuningMode[3].subMode.feature1,
             superNightDebugInfo.tuningParam.TuningMode[4].subMode.feature2,
             superNightDebugInfo.tuningParam.TuningMode[5].subMode.scene,
             superNightDebugInfo.tuningParam.TuningMode[6].subMode.effect, dumpTime, m_timeOutFlag);
    strcat(debugInfoBuf, buf);
    snprintf(buf, sizeof(buf), "[Temperature:%d] [CpuFreq:%d,%d,%d]", m_currentTemp,
             m_currentCpuFreq1, m_currentCpuFreq4, m_currentCpuFreq7);
    strcat(debugInfoBuf, buf);
    std::string snexif(debugInfoBuf);
    if (mNodeInterface.pSetResultMetadata != NULL) {
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, m_outputFrameNum, m_outputTimeStamp,
                                          snexif);
    }
}

void ArcsoftSllPlugin::getCurrentHWStatus()
{
    auto getDataFromPath = [](const char *path, int32_t &data) {
        int32_t fd = open(path, O_RDONLY);
        if (fd >= 0) {
            int32_t readSize = 0;
            char readBuffer[16] = {0};
            readSize = read(fd, &readBuffer[0], 15);
            if (readSize > 0) {
                readBuffer[readSize] = 0;
                data = atoi(&readBuffer[0]);
            } else {
                MLOGI(Mia2LogGroupPlugin, "cannot get HW status");
            }
            close(fd);
        } else {
            MLOGI(Mia2LogGroupPlugin, "cannot open HW status node, errno:%s", strerror(errno));
        }
    };

    std::string currentFilePath = "";
    getDataFromPath(
        MiaInternalProp::getInstance()->getString(CurrentTempFilePath, currentFilePath).c_str(),
        m_currentTemp);
    getDataFromPath("/sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq", m_currentCpuFreq1);
    getDataFromPath("/sys/devices/system/cpu/cpufreq/policy4/scaling_cur_freq", m_currentCpuFreq4);
    getDataFromPath("/sys/devices/system/cpu/cpufreq/policy7/scaling_cur_freq", m_currentCpuFreq7);
    getDataFromPath("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq", m_maxCpuFreq1);
    getDataFromPath("/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq", m_maxCpuFreq4);
    getDataFromPath("/sys/devices/system/cpu/cpufreq/policy7/scaling_max_freq", m_maxCpuFreq7);
}

void ArcsoftSllPlugin::setProperty()
{
    char prop[PROPERTY_VALUE_MAX];

    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.sll.dump", prop, "0");
    m_dumpMask = atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.sll.bypass", prop, "0");
    m_BypassForDebug = atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.sll.time", prop, "0");
    m_FakeProcTime = atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.capture.crop.update", prop, "1");
    m_isCropInfoUpdate = (int)atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.sll.state", prop,
                 "0"); // ARC_SN_CAMERA_STATE_HAND
    m_camStateForDebug = (int)atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.sll.debug", prop, "0");
    m_isDebugLog = (int)atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.sll.Tripod", prop, "0");
    m_isTripod = (int)atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.sll.algolog", prop, "1");
    m_isOpenAlgoLog = (int)atoi(prop);
}

uint32_t ArcsoftSllPlugin::getRawType(uint32_t rawFormat)
{
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  GetRawType rawFormat = 0x%x ChiColorFilterPattern = %d",
          rawFormat, m_rawPattern);
    switch (m_rawPattern) {
    case Y:    ///< Monochrome pixel pattern.
    case YUYV: ///< YUYV pixel pattern.
    case YVYU: ///< YVYU pixel pattern.
    case UYVY: ///< UYVY pixel pattern.
    case VYUY: ///< VYUY pixel pattern.
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  GetRawType don't hanlde this colorFilterPattern %d",
              m_rawPattern);
        break;
    case RGGB: ///< RGGB pixel pattern.
        if (CAM_FORMAT_RAW10 == rawFormat) {
            return ASVL_PAF_RAW10_RGGB_16B;
        } else if (CAM_FORMAT_RAW12 == rawFormat) {
            return ASVL_PAF_RAW12_RGGB_16B;
        } else if (CAM_FORMAT_RAW16 == rawFormat) {
            return ASVL_PAF_RAW14_RGGB_16B;
        } else {
            return ASVL_PAF_RAW10_RGGB_16B;
        }
    case GRBG: ///< GRBG pixel pattern.
        if (CAM_FORMAT_RAW10 == rawFormat) {
            return ASVL_PAF_RAW10_GRBG_16B;
        } else if (CAM_FORMAT_RAW12 == rawFormat) {
            return ASVL_PAF_RAW12_GRBG_16B;
        } else if (CAM_FORMAT_RAW16 == rawFormat) {
            return ASVL_PAF_RAW14_GRBG_16B;
        } else {
            return ASVL_PAF_RAW10_GRBG_16B;
        }
    case GBRG: ///< GBRG pixel pattern.
        if (CAM_FORMAT_RAW10 == rawFormat) {
            return ASVL_PAF_RAW10_GBRG_16B;
        } else if (CAM_FORMAT_RAW12 == rawFormat) {
            return ASVL_PAF_RAW12_GBRG_16B;
        } else if (CAM_FORMAT_RAW16 == rawFormat) {
            return ASVL_PAF_RAW14_GBRG_16B;
        } else {
            return ASVL_PAF_RAW10_GBRG_16B;
        }
    case BGGR: ///< BGGR pixel pattern.
        if (CAM_FORMAT_RAW10 == rawFormat) {
            return ASVL_PAF_RAW10_BGGR_16B;
        } else if (CAM_FORMAT_RAW12 == rawFormat) {
            return ASVL_PAF_RAW12_BGGR_16B;
        } else if (CAM_FORMAT_RAW16 == rawFormat) {
            return ASVL_PAF_RAW14_BGGR_16B;
        } else {
            return ASVL_PAF_RAW10_BGGR_16B;
        }
        break;
    case RGB: ///< RGB pixel pattern.
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:  GetRawType don't hanlde this colorFilterPattern %d",
              m_rawPattern);
        break;
    default:
        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]:  =======================GetRawType dont have the matched "
              "format=====================");
        break;
    }
    return 0;
}

void ArcsoftSllPlugin::getLSCDataInfo(ARC_SN_RAWINFO *rawInfo, camera_metadata_t *metaData)
{
    if (NULL == rawInfo || NULL == metaData)
        return;

    void *pData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "com.qti.chi.lsccontrolinfo.LSCControl", &pData);
    if (pData != NULL) {
        rawInfo->bEnableLSC = *static_cast<bool *>(pData);
        MLOGI(Mia2LogGroupPlugin, "get lsccontrolinfo success bEnableLSC %ld", rawInfo->bEnableLSC);
        pData = NULL;
    }
    rawInfo->bEnableLSC = 1;
    VendorMetadataParser::getTagValue(metaData, ANDROID_LENS_INFO_SHADING_MAP_SIZE, &pData);
    if (NULL != pData) {
        mLSCDim = *static_cast<LSCDimensionCap *>(pData);
        MLOGI(Mia2LogGroupPlugin, "get ANDROID_LENS_INFO_SHADING_MAP_SIZE success mLSCDim=[%d %d]",
              mLSCDim.width, mLSCDim.height);
        pData = NULL;
    }
    VendorMetadataParser::getTagValue(metaData, ANDROID_STATISTICS_LENS_SHADING_MAP, &pData);
    if (NULL != pData) {
        MLOGI(Mia2LogGroupPlugin, "get ANDROID_STATISTICS_LENS_SHADING_MAP success");
        memcpy(m_lensShadingMap, static_cast<float *>(pData),
               mLSCDim.width * mLSCDim.height * 4 * sizeof(float));
        pData = NULL;
    }

    for (int i = 0; i < LSCTotalChannels; i++) {
        int j = 0;
        rawInfo->i32LSChannelLength[i] = mLSCDim.width * mLSCDim.height;
        for (uint16_t v = 0; v < mLSCDim.height; v++) {
            for (uint16_t h = 0; h < mLSCDim.width; h++) {
                uint16_t index = v * mLSCDim.width * LSCTotalChannels + h * LSCTotalChannels + i;
                rawInfo->fLensShadingTable[i][j] = m_lensShadingMap[index];
                j++;
            }
        }

        if (m_dumpMask & 0x4) {
            dumpBLSLSCParameter(m_lensShadingMap);
            dumpSdkLSCParameter(&rawInfo->fLensShadingTable[i][0], i);
        }
    }

    return;
}

void ArcsoftSllPlugin::mappingAWBGain(MInt32 i32RawType, AWBGainParams *pQcomAWBGain,
                                      MFloat *pArcAWBGain)
{
    if (pQcomAWBGain == NULL || pArcAWBGain == NULL) {
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: AWBGainMapping input params error");
        return;
    }

    switch (i32RawType) {
    case ASVL_PAF_RAW10_RGGB_16B:
    case ASVL_PAF_RAW12_RGGB_16B:
    case ASVL_PAF_RAW14_RGGB_16B:
        pArcAWBGain[0] = pQcomAWBGain->rGain;
        pArcAWBGain[1] = 1.0f;
        pArcAWBGain[2] = 1.0f;
        pArcAWBGain[3] = pQcomAWBGain->bGain;
        return;
    case ASVL_PAF_RAW10_GRBG_16B:
    case ASVL_PAF_RAW12_GRBG_16B:
    case ASVL_PAF_RAW14_GRBG_16B:
        pArcAWBGain[0] = 1.0f;
        pArcAWBGain[1] = pQcomAWBGain->rGain;
        pArcAWBGain[2] = pQcomAWBGain->bGain;
        pArcAWBGain[3] = 1.0f;
        return;
    case ASVL_PAF_RAW10_GBRG_16B:
    case ASVL_PAF_RAW12_GBRG_16B:
    case ASVL_PAF_RAW14_GBRG_16B:
        pArcAWBGain[0] = 1.0f;
        pArcAWBGain[1] = pQcomAWBGain->bGain;
        pArcAWBGain[2] = pQcomAWBGain->rGain;
        pArcAWBGain[3] = 1.0f;
        return;
    case ASVL_PAF_RAW10_BGGR_16B:
    case ASVL_PAF_RAW12_BGGR_16B:
    case ASVL_PAF_RAW14_BGGR_16B:
        pArcAWBGain[0] = pQcomAWBGain->bGain;
        pArcAWBGain[1] = 1.0f;
        pArcAWBGain[2] = 1.0f;
        pArcAWBGain[3] = pQcomAWBGain->rGain;
        return;
    default:
        MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]:WBGainMapping dont have the matched format");
        return;
    }
}

void ArcsoftSllPlugin::prepareImage(MiImageBuffer nodeBuff, ASVLOFFSCREEN &stImage, MInt32 &fd)
{
    uint32_t format = nodeBuff.format;
    if (format != CAM_FORMAT_YUV_420_NV21 && format != CAM_FORMAT_YUV_420_NV12 &&
        format != CAM_FORMAT_RAW16) {
        MLOGD(Mia2LogGroupPlugin, "[ARCSOFT] format(%d) is not supported!", format);
        return;
    }

    m_format.width = stImage.i32Width = nodeBuff.width;
    m_format.height = stImage.i32Height = nodeBuff.height;

    fd = nodeBuff.fd[0];
    MLOGD(Mia2LogGroupPlugin, "[ARCSOFT] stImage(%dx%d) fd:%d %d", stImage.i32Width,
          stImage.i32Height, fd, nodeBuff.fd[1]);
    if (format == CAM_FORMAT_YUV_420_NV12) {
        stImage.u32PixelArrayFormat = ASVL_PAF_NV12;
        for (int i = 0; i < nodeBuff.numberOfPlanes; i++) {
            stImage.ppu8Plane[i] = nodeBuff.plane[i];
            stImage.pi32Pitch[i] = (int)nodeBuff.stride;
            MLOGD(Mia2LogGroupPlugin, "[ARCSOFT] YUV420NV12 stImage.pi32Pitch[%d] = %d", i,
                  stImage.pi32Pitch[i]);
        }
    } else if (format == CAM_FORMAT_YUV_420_NV21) {
        stImage.u32PixelArrayFormat = ASVL_PAF_NV21;
        for (int i = 0; i < nodeBuff.numberOfPlanes; i++) {
            stImage.ppu8Plane[i] = nodeBuff.plane[i];
            stImage.pi32Pitch[i] = (int)nodeBuff.stride;
            MLOGD(Mia2LogGroupPlugin, "[ARCSOFT] YUV420NV21 stImage.pi32Pitch[%d] = %d", i,
                  stImage.pi32Pitch[i]);
        }
    } else if (format == CAM_FORMAT_RAW16) {
        // Todo, need double check the format ASVL_PAF_RAW16_RGGB_16B
        stImage.u32PixelArrayFormat = getRawType(format);
        for (int i = 0; i < nodeBuff.numberOfPlanes; i++) {
            stImage.ppu8Plane[i] = nodeBuff.plane[i];
            stImage.pi32Pitch[i] = (int)nodeBuff.stride;
            MLOGD(Mia2LogGroupPlugin, "[ARCSOFT] RawPlain16LSB14bit stImage.pi32Pitch[%d] = %d", i,
                  stImage.pi32Pitch[i]);
        }
        MLOGD(Mia2LogGroupPlugin, "[ARCSOFT]  the image buffer format is 0x%x",
              stImage.u32PixelArrayFormat);
    }
}

void ArcsoftSllPlugin::imageToMiBuffer(ImageParams *image, MiImageBuffer *miBuf)
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

void ArcsoftSllPlugin::getCameraIdRole(camera_metadata_t *metaData)
{
    if (NULL == metaData) {
        MLOGD(Mia2LogGroupPlugin, "metaData null point!");
        return;
    }
    void *pData = NULL;
    int roleID = RoleIdTypeNone;
    int fwkCameraId = 0;
    if ((m_snapshotMode == SNAPSHOTMODE::SM_SE_SAT && m_isSupportSat) ||
        (m_snapshotMode == SNAPSHOTMODE::SM_SE_BOKEN)) {
        MultiCameraIds multiCameraRole;
        const char *MultiCameraRole = "com.qti.chi.multicamerainfo.MultiCameraIds";
        VendorMetadataParser::getVTagValue(metaData, MultiCameraRole, &pData);
        if (NULL != pData) {
            multiCameraRole = *static_cast<MultiCameraIds *>(pData);
            m_fwkCameraId = multiCameraRole.fwkCameraId;
            fwkCameraId = multiCameraRole.fwkCameraId;
            roleID = multiCameraRole.currentCameraRole;
            MLOGI(
                Mia2LogGroupPlugin,
                "MultiCamera: curCamId = %d logiCamId = %d masterCamId = %d currentCameraRole = %d"
                "masterCameraRole = %d fwcamId = %d",
                multiCameraRole.currentCameraId, multiCameraRole.logicalCameraId,
                multiCameraRole.masterCameraId, multiCameraRole.currentCameraRole,
                multiCameraRole.masterCameraRole, multiCameraRole.fwkCameraId);
        } else {
            MLOGE(Mia2LogGroupPlugin, "get multicamerainfo.MultiCameraIds fail");
        }
        m_pStaticMetadata = StaticMetadataWraper::getInstance()->getStaticMetadata(fwkCameraId);

        switch (roleID) {
        case CameraRoleType::CameraRoleTypeUltraWide:
            m_cameraRoleID = SensorRoleType::LENS_ULTRA;
            break;
        case CameraRoleType::CameraRoleTypeWide:
            m_cameraRoleID = SensorRoleType::LENS_WIDE;
            break;
        case CameraRoleType::CameraRoleTypeTele:
            m_cameraRoleID = SensorRoleType::LENS_TELE;
            break;
        case CameraRoleType::CameraRoleTypeTele4X:
            m_cameraRoleID = SensorRoleType::LENS_TELE;
            break;
        default:
            m_cameraRoleID = SensorRoleType::LENS_NONE;
        }
    } else {
        VendorMetadataParser::getVTagValue(metaData, "xiaomi.snapshot.fwkCameraId", &pData);
        if (NULL != pData) {
            fwkCameraId = *static_cast<uint32_t *>(pData);
            m_fwkCameraId = fwkCameraId;
            MLOGI(Mia2LogGroupPlugin, "single camera, framework cameraId: %d", fwkCameraId);
        } else {
            MLOGE(Mia2LogGroupPlugin, "single camera, get framework cameraId fail");
        }
        m_pStaticMetadata = StaticMetadataWraper::getInstance()->getStaticMetadata(fwkCameraId);

        pData = NULL;
        VendorMetadataParser::getVTagValue(m_pStaticMetadata, "com.xiaomi.cameraid.role.cameraId",
                                           &pData);
        if (NULL != pData) {
            roleID = *static_cast<int *>(pData);
            MLOGI(Mia2LogGroupPlugin, "single camera get cameraid: %d, roleID: %d ", fwkCameraId,
                  roleID);
        } else {
            MLOGE(Mia2LogGroupPlugin, "single camera get roleID fail");
        }

        switch (roleID) {
        case RoleIdRearUltra:
            m_cameraRoleID = SensorRoleType::LENS_ULTRA;
            break;
        case RoleIdRearWide:
            m_cameraRoleID = SensorRoleType::LENS_WIDE;
            break;
        case RoleIdRearTele:
            m_cameraRoleID = SensorRoleType::LENS_TELE;
            break;
        case RoleIdRearTele4x:
            m_cameraRoleID = SensorRoleType::LENS_TELE;
            break;
        default:
            m_cameraRoleID = SensorRoleType::LENS_NONE;
        }
    }
}

void ArcsoftSllPlugin::getSensorSize(ChiRect *pSensorSize, camera_metadata_t *metaData)
{
    if (NULL == metaData) {
        MLOGD(Mia2LogGroupPlugin, "metaData null point!");
        return;
    }

    void *pData = NULL;
    const char *sensorSizeTag = "xiaomi.sensorSize.size";
    VendorMetadataParser::getVTagValue(metaData, sensorSizeTag, &pData);
    if (NULL != pData) {
        CHIDIMENSION *dimensionSize = (CHIDIMENSION *)(pData);
        pSensorSize->left = 0;
        pSensorSize->top = 0;
        pSensorSize->height = dimensionSize->height;
        pSensorSize->width = dimensionSize->width;
        MLOGI(Mia2LogGroupPlugin, "get xiaomi.sensorSize w*h= %d * %d", pSensorSize->width,
              pSensorSize->height);
    } else {
        MLOGI(Mia2LogGroupPlugin, "get xiaomi.sensorSize fail");
    }
}

void ArcsoftSllPlugin::getMetaData(camera_metadata_t *pMetaData)
{
    getCameraIdRole(pMetaData);

    void *pData = NULL;
    VendorMetadataParser::getVTagValue(pMetaData, "org.quic.camera2.tuning.mode.TuningMode",
                                       &pData);
    if (NULL != pData) {
        memcpy(&m_tuningParam, pData, sizeof(ChiTuningModeParameter));
    }

    getOrientation(&m_devOrientation, pMetaData);

    pData = NULL;
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.multiframe.inputNum", &pData);
    if (NULL != pData) {
        m_algoInputNum = *static_cast<unsigned int *>(pData);
        MLOGI(Mia2LogGroupPlugin, "get multiframe inputNum = %d", m_algoInputNum);
    } else {
        MLOGI(Mia2LogGroupPlugin, "get multiframe inputNum fail");
    }
}

void ArcsoftSllPlugin::getInputRawInfo(ImageParams *inputParam, camera_metadata_t *pMetaData,
                                       ARC_SN_RAWINFO *rawInfo)
{
    if (inputParam == NULL || rawInfo == NULL) {
        MLOGI(Mia2LogGroupPlugin, "inputParam or rawInfo null point");
        return;
    }

    void *pData = NULL;
    uint8_t bayerPattern = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;

    if (NULL != pMetaData) {
        VendorMetadataParser::getTagValue(pMetaData, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                          &pData);
        if (NULL != pData) {
            rawInfo->i32EV[0] = *static_cast<int *>(pData);
            m_inputEV[m_InputIndex] = rawInfo->i32EV[0];
            pData = NULL;
        }

        VendorMetadataParser::getVTagValue(pMetaData,
                                           "org.quic.camera2.statsconfigs.AECFrameControl", &pData);
        if (NULL != pData) {
            rawInfo->fShutter = static_cast<AECFrameControl *>(pData)
                                    ->exposureInfo[ExposureIndexSafe]
                                    .exposureTime /
                                1e9;
            rawInfo->i32LuxIndex = (int)static_cast<AECFrameControl *>(pData)->luxIndex;
            rawInfo->fTotalGain =
                static_cast<AECFrameControl *>(pData)->exposureInfo[ExposureIndexSafe].linearGain;
            rawInfo->i32ExpIndex = (int)static_cast<AECFrameControl *>(pData)
                                       ->exposureInfo[ExposureIndexSafe]
                                       .exposureTime;
            pData = NULL;
        }

        VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.mivi.supernight.adrcGain", &pData);
        if (NULL != pData) {
            rawInfo->fAdrcGain = *static_cast<float *>(pData);
            pData = NULL;
        } else {
            rawInfo->fAdrcGain = 1.0;
        }

        VendorMetadataParser::getVTagValue(pMetaData, "com.qti.sensorbps.gain", &pData);
        if (NULL != pData) {
            rawInfo->fISPGain = *static_cast<float *>(pData);
            if (rawInfo->fISPGain > 0.000001f || rawInfo->fISPGain < 0.000001f) {
                rawInfo->fSensorGain =
                    (float)((float)rawInfo->fTotalGain / (float)rawInfo->fISPGain);
            }
        }
        pData = NULL;

#ifdef MCAM_SW_DIAG
        m_lumaValue[m_InputIndex] =
            (rawInfo->fISPGain) * (rawInfo->fSensorGain) * (rawInfo->fShutter);
#endif

        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]:rawinfo frame[%d] EV:%d ISPGain: %f SensorGain: %f Shutter: %f "
              "iLuxIndex: %d totalGain: %f adrcGain: %f expIndex: %d",
              m_InputIndex.load(), rawInfo->i32EV[0], rawInfo->fISPGain, rawInfo->fSensorGain,
              rawInfo->fShutter, rawInfo->i32LuxIndex, rawInfo->fTotalGain, rawInfo->fAdrcGain,
              rawInfo->i32ExpIndex);
    }

    VendorMetadataParser::getTagValue(m_pStaticMetadata,
                                      ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT, &pData);
    if (NULL != pData) {
        bayerPattern = *static_cast<int32_t *>(pData);
        pData = NULL;
    } else {
        MLOGD(Mia2LogGroupPlugin, "[ARCSOFT]: Failed to get sensor bayer pattern");
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

    uint32_t rawFormat = inputParam->format.format;
    rawInfo->i32RawType = getRawType(rawFormat);
    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: rawFormat:0x%x bayerPattern %d rawInfo->i32RawType:0x%x",
          rawFormat, bayerPattern, rawInfo->i32RawType);

    int white_lelvel_dynamic = 0;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_SENSOR_DYNAMIC_WHITE_LEVEL, &pData);
    if (NULL != pData) {
        white_lelvel_dynamic = *static_cast<int32_t *>(pData);
        pData = NULL;
    }

    for (int i = 0; i < 4; i++) {
        rawInfo->i32BrightLevel[i] = white_lelvel_dynamic;
    }

    float black_level[4] = {0.0f};
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_SENSOR_DYNAMIC_BLACK_LEVEL, &pData);
    if (NULL != pData) {
        memcpy(black_level, static_cast<int32_t *>(pData), 4 * sizeof(float));
    }

    for (int i = 0; i < 4; i++) {
        rawInfo->i32BlackLevel[i] = (int)black_level[i];
        if ((rawInfo->i32BlackLevel[i] < MinBLSValue) ||
            (rawInfo->i32BlackLevel[i] > MaxBLSValue)) {
            rawInfo->i32BlackLevel[i] = defaultBLSValue;
        }
    }
    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]: BrightLevel[0-3] :%d,%d,%d,%d,2BlackLeve[0-3] = %d,%d,%d,%d",
          rawInfo->i32BrightLevel[0], rawInfo->i32BrightLevel[1], rawInfo->i32BrightLevel[2],
          rawInfo->i32BrightLevel[3], rawInfo->i32BlackLevel[0], rawInfo->i32BlackLevel[1],
          rawInfo->i32BlackLevel[2], rawInfo->i32BlackLevel[3]);

    getLSCDataInfo(rawInfo, pMetaData);

    pData = NULL;
    VendorMetadataParser::getVTagValue(pMetaData, "org.quic.camera2.statsconfigs.AWBFrameControl",
                                       &pData);
    if (NULL != pData) {
        AWBFrameControl *awbControl = (AWBFrameControl *)pData;
        mappingAWBGain(rawInfo->i32RawType, &awbControl->AWBGains, rawInfo->fWbGain);
        rawInfo->fCCT = awbControl->colorTemperature;
        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]:  awbControl: rGain = %f,gGain = %f,bGain =%f"
              "fWbGain:%f,%f,%f,%f",
              "ffCT:%f", awbControl->AWBGains.rGain, awbControl->AWBGains.gGain,
              awbControl->AWBGains.bGain, rawInfo->fWbGain[0], rawInfo->fWbGain[1],
              rawInfo->fWbGain[2], rawInfo->fWbGain[3], rawInfo->fCCT);
    }

    pData = NULL;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_COLOR_CORRECTION_TRANSFORM, &pData);
    if (NULL != pData) {
        CHIRATIONAL *ccm = static_cast<CHIRATIONAL *>(pData);
        for (uint32_t i = 0; i < 9; i++) {
            ccm[i].denominator = (ccm[i].denominator == 0) ? 1 : ccm[i].denominator;
            rawInfo->fCCM[i] = (static_cast<float>(static_cast<int32_t>(ccm[i].numerator)) /
                                static_cast<float>(static_cast<int32_t>(ccm[i].denominator)));
        }
        MLOGI(Mia2LogGroupPlugin, "get ccm: %f %f %f /n %f %f %f /n %f %f %f", rawInfo->fCCM[0],
              rawInfo->fCCM[1], rawInfo->fCCM[2], rawInfo->fCCM[3], rawInfo->fCCM[4],
              rawInfo->fCCM[5], rawInfo->fCCM[6], rawInfo->fCCM[7], rawInfo->fCCM[8]);
    } else {
        MLOGI(Mia2LogGroupPlugin, "get ANDROID_COLOR_CORRECTION_TRANSFORM fail");
    }
}

void ArcsoftSllPlugin::CopyRawInfo(ARC_SN_RAWINFO *rawInfo, ARC_SN_RAWINFO *rawInfoForProcess,
                                   int32_t inputNum)
{
    if ((NULL == rawInfo) || (NULL == rawInfoForProcess)) {
        return;
    }
    rawInfoForProcess->i32RawType = rawInfo->i32RawType;
    rawInfoForProcess->fShutter = rawInfo->fShutter;
    rawInfoForProcess->fSensorGain = rawInfo->fSensorGain;
    rawInfoForProcess->fISPGain = rawInfo->fISPGain;
    rawInfoForProcess->fTotalGain = rawInfo->fTotalGain;
    rawInfoForProcess->i32LuxIndex = rawInfo->i32LuxIndex;
    rawInfoForProcess->i32ExpIndex = rawInfo->i32ExpIndex;
    rawInfoForProcess->fAdrcGain = rawInfo->fAdrcGain;
    rawInfoForProcess->bEnableLSC = rawInfo->bEnableLSC;

    for (int i = 0; i < 4; i++) {
        rawInfoForProcess->i32BrightLevel[i] = rawInfo->i32BrightLevel[i];
    }

    for (int i = 0; i < 4; i++) {
        rawInfoForProcess->i32BlackLevel[i] = rawInfo->i32BlackLevel[i];
    }

    for (int i = 0; i < 4; i++) {
        rawInfoForProcess->fWbGain[i] = rawInfo->fWbGain[i];
    }

    for (int i = 0; i < LSCTotalChannels; i++) {
        rawInfoForProcess->i32LSChannelLength[i] = rawInfo->i32LSChannelLength[i];
        for (int v = 0; v < mLSCDim.height; v++) {
            for (int h = 0; h < mLSCDim.width; h++) {
                int index = v * mLSCDim.width + h;
                rawInfoForProcess->fLensShadingTable[i][index] =
                    rawInfo->fLensShadingTable[i][index];
            }
        }
    }

    for (int i = 0; i < inputNum; i++) {
        rawInfoForProcess->i32EV[i] = rawInfo->i32EV[i];
    }
}

void ArcsoftSllPlugin::getFaceInfo(ARC_SN_FACEINFO *pFaceInfo, camera_metadata_t *metaData)
{
    if (pFaceInfo == NULL || pFaceInfo->pFaceRects == NULL) {
        return;
    }

    void *pData = NULL;
    MRECT activeRect = {0};
    pFaceInfo->i32FaceNum = 0;

    // get facenums
    void *data = NULL;
    camera_metadata_entry_t entry = {0};
    uint32_t numElemsRect = sizeof(MRECT) / sizeof(uint32_t);
    if (0 != find_camera_metadata_entry(metaData, ANDROID_STATISTICS_FACE_RECTANGLES, &entry)) {
        MLOGI(Mia2LogGroupPlugin, "get ANDROID_STATISTICS_FACE_RECTANGLES failed");
        return;
    }

    pFaceInfo->i32FaceNum = entry.count / numElemsRect;
    pFaceInfo->i32FaceNum = pFaceInfo->i32FaceNum < ARCSOFT_MAX_FACE_NUMBER
                                ? pFaceInfo->i32FaceNum
                                : ARCSOFT_MAX_FACE_NUMBER;
    if (0 == pFaceInfo->i32FaceNum) {
        return;
    }

    uint32_t mSnapshotWidth = m_format.width;
    uint32_t mSnapshotHeight = m_format.height;
    uint32_t mSensorWidth = 0;
    uint32_t mSensorHeight = 0;

    // get sensor size for mivifwk, rather than active size for hal
    ChiDimension activeSize = {0};
    data = NULL;
    const char *SensorSize = "xiaomi.sensorSize.size";
    VendorMetadataParser::getVTagValue(metaData, SensorSize, &data);
    if (NULL != data) {
        activeSize = *(ChiDimension *)(data);
        MLOGI(Mia2LogGroupPlugin, "get sensor activeSize %d %d", activeSize.width,
              activeSize.height);
        // sensor size is 1/2 active size
        mSensorWidth = activeSize.width / 2;
        mSensorHeight = activeSize.height / 2;
    } else {
        mSensorWidth = mSnapshotWidth;
        mSensorHeight = mSnapshotHeight;
    }
    MLOGI(Mia2LogGroupPlugin, "sensor: %d, %d; snapshot:%d, %d", mSensorWidth, mSensorHeight,
          mSnapshotWidth, mSnapshotHeight);

    // face rectangle
    data = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_STATISTICS_FACE_RECTANGLES, &data);
    if (NULL != data) {
        void *tmpData = reinterpret_cast<void *>(data);
        float zoomRatio = (float)mSnapshotWidth / mSensorWidth;
        float yOffSize = ((float)zoomRatio * mSensorHeight - mSnapshotHeight) / 2.0;
        MLOGI(Mia2LogGroupPlugin, "zoomRatio:%f, yOffSize:%f", zoomRatio, yOffSize);

        uint32_t dataIndex = 0;
        for (int index = 0; index < pFaceInfo->i32FaceNum; index++) {
            int32_t xMin = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t yMin = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t xMax = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t yMax = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            MLOGI(Mia2LogGroupPlugin, "origin face [xMin,yMin,xMax,yMax][%d,%d,%d,%d]", xMin, yMin,
                  xMax, yMax);
            xMin = xMin * zoomRatio;
            xMax = xMax * zoomRatio;
            yMin = yMin * zoomRatio - yOffSize;
            yMax = yMax * zoomRatio - yOffSize;
            MLOGI(Mia2LogGroupPlugin, "now face [xMin,yMin,xMax,yMax][%d,%d,%d,%d]", xMin, yMin,
                  xMax, yMax);

            pFaceInfo->pFaceRects[index].left = xMin;
            pFaceInfo->pFaceRects[index].top = yMin;
            pFaceInfo->pFaceRects[index].right = xMax;
            pFaceInfo->pFaceRects[index].bottom = yMax;
        }
    } else {
        MLOGI(Mia2LogGroupPlugin, "faces are not found");
    }

    pFaceInfo->i32FaceOrientation = m_devOrientation;
}

void ArcsoftSllPlugin::getOrientation(MInt32 *pOrientation, camera_metadata_t *metaData)
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

void ArcsoftSllPlugin::getActiveArraySize(ChiRect *pRect, camera_metadata_t *metaData)
{
    if (NULL == pRect || NULL == metaData)
        return;

    void *pData = NULL;
    camera_metadata *pMeta = StaticMetadataWraper::getInstance()->getStaticMetadata(m_fwkCameraId);
    VendorMetadataParser::getTagValue(pMeta, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &pData);
    if (!pData) {
        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]: get meta ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE fail!");
        return;
    }
    ChiRect *pRectData = reinterpret_cast<ChiRect *>(pData);
    pRect->left = pRectData->left;
    pRect->top = pRectData->top;
    pRect->width = pRectData->width;
    pRect->height = pRectData->height;
    MLOGI(Mia2LogGroupPlugin,
          "[ARCSOFT]: get meta ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE success! rect:[%d %d, %d "
          "%d],m_fwkCameraId:%d",
          pRectData->left, pRectData->top, pRectData->width, pRectData->height, m_fwkCameraId);
}

void ArcsoftSllPlugin::getISOValueInfo(int *pISOValue, camera_metadata_t *metaData)
{
    if (metaData == NULL)
        return;

    int i32ISOValue = 0;
    void *pData = NULL;

    VendorMetadataParser::getTagValue(metaData, ANDROID_SENSOR_SENSITIVITY, &pData);
    if (NULL != pData) {
        i32ISOValue = *static_cast<int *>(pData);
        MLOGI(Mia2LogGroupPlugin,
              "[ARCSOFT]: get meta ANDROID_SENSOR_SENSITIVITY success! i32ISOValue %d",
              i32ISOValue);
    } else {
        VendorMetadataParser::getTagValue(metaData, ANDROID_SENSOR_SENSITIVITY, &pData);
        if (NULL != pData) {
            i32ISOValue = *static_cast<int *>(pData);
            MLOGI(Mia2LogGroupPlugin,
                  "[ARCSOFT]: get meta ANDROID_SENSOR_SENSITIVITY success! i32ISOValue %d",
                  i32ISOValue);
        } else {
            i32ISOValue = 100;
            MLOGI(Mia2LogGroupPlugin,
                  "[ARCSOFT]: get meta ANDROID_SENSOR_SENSITIVITY fail! i32ISOValue = 100");
        }
    }

    if (pISOValue != NULL) {
        *pISOValue = i32ISOValue;
    }
    return;
}

void ArcsoftSllPlugin::debugFaceRoi(ImageParams pNodeBuff)
{
    int nStride = pNodeBuff.planeStride;
    // CHIIMAGE& chiImage = pNodeBuff->pImageList[0];
    for (int i = 0; i < m_FaceParams.i32FaceNum; i++) {
        int x1 = m_FaceParams.pFaceRects[i].left;
        int x2 = m_FaceParams.pFaceRects[i].right;
        int y1 = m_FaceParams.pFaceRects[i].top;
        int y2 = m_FaceParams.pFaceRects[i].bottom;

        for (int k = 0; k < 4; k++) {
            unsigned char *buffer = (unsigned char *)pNodeBuff.pAddr[0] + nStride * (y1 + k);
            uint16_t *addr_base = (uint16_t *)buffer;
            for (int j = x1; j < x2; j++) {
                addr_base[j] = 0xffff;
            }
        }

        for (int k = 0; k < 4; k++) {
            unsigned char *buffer = (unsigned char *)pNodeBuff.pAddr[0] + nStride * (y2 + k);
            uint16_t *addr_base = (uint16_t *)buffer;

            for (int j = x1; j < x2; j++) {
                addr_base[j] = 0xffff;
            }
        }

        for (int k = y1; k < y2; k++) {
            unsigned char *buffer = (unsigned char *)pNodeBuff.pAddr[0] + nStride * k;
            uint16_t *addr_base = (uint16_t *)buffer;

            for (int j = x1; j < x1 + 4; j++) {
                addr_base[j] = 0xffff;
            }

            for (int j = x2; j < x2 + 4; j++) {
                addr_base[j] = 0xffff;
            }
        }
    }
}

void ArcsoftSllPlugin::dumpSdkLSCParameter(const float *lensShadingMap, int channel)
{
    if (NULL == lensShadingMap) {
        MLOGI(Mia2LogGroupPlugin, "lensShadingMap null point");
        return;
    }

    FILE *pFp = NULL;
    static char dumpFilePath[256] = {
        '\0',
    };
    memset(dumpFilePath, '\0', sizeof(dumpFilePath));
    snprintf(dumpFilePath, sizeof(dumpFilePath) - 1,
             "/data/vendor/camera/DumpBLSLSCPara_FN-%" PRIu64 "_BLS-xx_LSC-%d.txt",
             m_processedFrame, channel);

    if (NULL == (pFp = fopen(dumpFilePath, "wt"))) {
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

void ArcsoftSllPlugin::dumpBLSLSCParameter(const float *lensShadingMap)
{
    if (NULL == lensShadingMap) {
        MLOGI(Mia2LogGroupPlugin, "lensShadingMap null point");
        return;
    }

    FILE *pFp = NULL;
    static char dumpFilePath[256] = {
        '\0',
    };
    memset(dumpFilePath, '\0', sizeof(dumpFilePath));
    snprintf(dumpFilePath, sizeof(dumpFilePath),
             "/data/vendor/camera/DumpBLSLSCPara_FN-%" PRIu64 "_BLS-xx_LSC-xx.txt",
             m_processedFrame);

    if (NULL == (pFp = fopen(dumpFilePath, "wt"))) {
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

void ArcsoftSllPlugin::dumpRawInfoExt(ImageParams pNodeBuff, int index, int count,
                                      MInt32 i32SceneMode, MInt32 i32CamMode,
                                      ARC_SN_FACEINFO *pFaceParams, ARC_SN_RAWINFO *rawInfo,
                                      char *fileName)
{
    int nStride = 0;
    int nSliceheight = 0;
    char buf[256];

    nStride = pNodeBuff.planeStride;
    nSliceheight = pNodeBuff.sliceheight;
    snprintf(buf, sizeof(buf), "%s.txt", fileName);

    FILE *pf = fopen(buf, "a");
    if (pf) {
        int face_cnt = 0;
        if (pFaceParams != NULL) {
            face_cnt = pFaceParams->i32FaceNum;
        }
        float redGain = rawInfo->fWbGain[0];
        float blueGain = rawInfo->fWbGain[3];
        float greenGain = rawInfo->fWbGain[3];
        switch (rawInfo->i32RawType) {
        case ASVL_PAF_RAW10_GRBG_16B:
        // case ASVL_PAF_RAW12_GRBG_16B:
        case ASVL_PAF_RAW14_GRBG_16B:
            greenGain = rawInfo->fWbGain[0];
            redGain = rawInfo->fWbGain[1];
            blueGain = rawInfo->fWbGain[2];
            break;
        case ASVL_PAF_RAW10_GBRG_16B:
        // case ASVL_PAF_RAW12_GBRG_16B:
        case ASVL_PAF_RAW14_GBRG_16B:
            greenGain = rawInfo->fWbGain[0];
            redGain = rawInfo->fWbGain[2];
            blueGain = rawInfo->fWbGain[1];
            break;
        case ASVL_PAF_RAW10_BGGR_16B:
        // case ASVL_PAF_RAW12_BGGR_16B:
        case ASVL_PAF_RAW14_BGGR_16B:
            greenGain = rawInfo->fWbGain[1];
            redGain = rawInfo->fWbGain[3];
            blueGain = rawInfo->fWbGain[0];
            break;
        case ASVL_PAF_RAW10_RGGB_16B:
        case ASVL_PAF_RAW14_RGGB_16B:
            greenGain = rawInfo->fWbGain[1];
            redGain = rawInfo->fWbGain[0];
            blueGain = rawInfo->fWbGain[3];
            break;
        default:
            break;
        }

        if (index == 0) {
            const MPBASE_Version *arcVersion = ARC_SN_GetVersion();
            fprintf(pf, "ArcSoft SuperNightRaw Algorithm:%s\n", arcVersion->Version);
        }
        fprintf(pf, "[InputFrameIndex:%d]\n", index);

        fprintf(pf, "redGain:%.6f\ngreenGain:%.6f\nblueGain:%.6f\n", redGain, greenGain, blueGain);
        fprintf(pf, "totalGain:%.6f\nsensorGain:%.6f\nispGain:%.6f\n",
                rawInfo->fSensorGain * rawInfo->fISPGain, rawInfo->fSensorGain, rawInfo->fISPGain);
        fprintf(pf, "adrc_gain:%.6f\n", rawInfo->fAdrcGain);
        fprintf(pf, "EV[%d]:%d\n", index, rawInfo->i32EV[0]);
        fprintf(pf, "bFlashUsed:0\n");
        fprintf(pf, "CCT:%f\n", rawInfo->fCCT);
        fprintf(pf, "CCM:%f, %f, %f, %f, %f, %f, %f, %f, %f\n", rawInfo->fCCM[0], rawInfo->fCCM[1],
                rawInfo->fCCM[2], rawInfo->fCCM[3], rawInfo->fCCM[4], rawInfo->fCCM[5],
                rawInfo->fCCM[6], rawInfo->fCCM[7], rawInfo->fCCM[8]);
        fprintf(pf, "shutter:%.6f\nluxIndex:%d\nexpIndex:%.0f\n", rawInfo->fShutter,
                rawInfo->i32LuxIndex, rawInfo->fShutter * 1e9);
        fprintf(pf, "blackLevel[0]:%d\nblackLevel[1]:%d\nblackLevel[2]:%d\nblackLevel[3]:%d\n",
                rawInfo->i32BlackLevel[0], rawInfo->i32BlackLevel[1], rawInfo->i32BlackLevel[2],
                rawInfo->i32BlackLevel[3]);
        fprintf(pf, "\n");

        if (index == count - 1) {
            fprintf(pf, "scene_mode:%d\ncamera_state:%d\ncamera_mode:%d\n", i32SceneMode,
                    m_camState, i32CamMode);
            fprintf(pf, "raw_format:%d\n", rawInfo->i32RawType);
            fprintf(pf, "device_orientation:%d\n", m_devOrientation);
            fprintf(pf, "zoom_left:%d\n", m_InputZoomRect.left);
            fprintf(pf, "zoom_top:%d\n", m_InputZoomRect.top);
            fprintf(pf, "zoom_right:%d\n", m_InputZoomRect.right);
            fprintf(pf, "zoom_bottom:%d\n", m_InputZoomRect.bottom);
            fprintf(pf, "face_cnt:%d\n", face_cnt);
            for (signed int i = 0; i < face_cnt; i++) {
                fprintf(pf, "left:%d\ntop:%d\nright:%d\nbottom:%d\nrotation:%d\n",
                        pFaceParams->pFaceRects[i].left, pFaceParams->pFaceRects[i].top,
                        pFaceParams->pFaceRects[i].right, pFaceParams->pFaceRects[i].bottom,
                        pFaceParams->i32FaceOrientation);
            }
            fprintf(pf, "\n");

            fprintf(pf, "bEnableLSC:%d\nnLscChannelLength:%d\n",
                    m_closestEV0RawInfo.bEnableLSC ? 1 : 0,
                    m_closestEV0RawInfo.i32LSChannelLength[0]);
            fprintf(pf, "lsc_x_col:17\n");
            fprintf(pf, "lsc_x_row:%d\n\n\n",
                    m_closestEV0RawInfo.i32LSChannelLength[0] / 17 +
                        (m_closestEV0RawInfo.i32LSChannelLength[0] % 17 ? 1 : 0));
            char a[4][10] = {"R", "Gr", "Gb", "B"};
            for (int i = 0; i < 4; i++) {
                fprintf(pf, " =======LSC Table %s=======", a[i]);
                for (int j = 0; j < m_closestEV0RawInfo.i32LSChannelLength[i]; j++) {
                    if (j % 17 == 0)
                        fprintf(pf, "\n");
                    fprintf(pf, "%.6f, ", m_closestEV0RawInfo.fLensShadingTable[i][j]);
                }
                fprintf(pf, "\n\n");
            }
        }
        fclose(pf);
    }
}

void ArcsoftSllPlugin::getDumpFileName(char *fileName, size_t size, const char *fileType,
                                       const char *timeBuf)
{
    char buf[128];
    memset(buf, 0, sizeof(buf));

    if (mkdir(g_RearNightDumpDirectory, 0777) != 0 && EEXIST != errno) {
        MLOGI(Mia2LogGroupPlugin, "Failed to create capture RearNightDump folder:%s, Error: %d",
              g_RearNightDumpDirectory, errno);
    }

    if (LENS_WIDE == m_cameraRoleID) {
        snprintf(buf, sizeof(buf), "W_");
    } else if (LENS_TELE == m_cameraRoleID) {
        snprintf(buf, sizeof(buf), "T_");
    } else if (LENS_ULTRA == m_cameraRoleID) {
        snprintf(buf, sizeof(buf), "U_");
    }

    if (m_snapshotMode == SNAPSHOTMODE::SM_SE_SAT) {
        strncat(buf, "SE", sizeof(buf) - strlen(buf) - 1);
    } else if (m_snapshotMode == SNAPSHOTMODE::SM_SE_BOKEN) {
        strncat(buf, "SE-Bokeh", sizeof(buf) - strlen(buf) - 1);
    } else {
        strncat(buf, "SN", sizeof(buf) - strlen(buf) - 1);
    }

    snprintf(fileName, size, "%sIMG_%s_%s_%s_%dx%d", g_RearNightDumpDirectory, timeBuf, buf,
             fileType, m_format.width, m_format.height);
}

bool ArcsoftSllPlugin::dumpToFile(const char *fileName, struct MiImageBuffer *imageBuffer,
                                  uint32_t index, const char *fileType)
{
    if (imageBuffer == NULL)
        return -1;
    char buf[128];
    snprintf(buf, sizeof(buf), "%s", fileName);
    android::String8 filePath(buf);
    const char *suffix = "RGGB";
    if (CAM_FORMAT_RAW10 == imageBuffer->format || CAM_FORMAT_RAW16 == imageBuffer->format) {
        switch (m_rawPattern) {
        case Y: ///< Monochrome pixel pattern.
            suffix = "Y";
            break;
        case YUYV: ///< YUYV pixel pattern.
            suffix = "YUYV";
            break;
        case YVYU: ///< YVYU pixel pattern.
            suffix = "YVYU";
            break;
        case UYVY: ///< UYVY pixel pattern.
            suffix = "UYVY";
            break;
        case VYUY: ///< VYUY pixel pattern.
            suffix = "VYUY";
            break;
        case RGGB: ///< RGGB pixel pattern.
            suffix = "RGGB";
            break;
        case GRBG: ///< GRBG pixel pattern.
            suffix = "GRBG";
            break;
        case GBRG: ///< GBRG pixel pattern.
            suffix = "GBRG";
            break;
        case BGGR: ///< BGGR pixel pattern.
            suffix = "BGGR";
            break;
        case RGB: ///< RGB pixel pattern.
            suffix = "RGB";
            break;
        default:
            suffix = "DATA";
            break;
        }
    }
    if (0 == strcmp(fileType, "input")) {
        snprintf(buf, sizeof(buf), "_%02d_EV[%d].%s", index, m_inputEV[m_InputIndex], suffix);
    } else {
        snprintf(buf, sizeof(buf), "_%d.%s", index, suffix);
    }
    MLOGD(Mia2LogGroupCore, "%s %s%s", __func__, filePath.string(), buf);

    filePath.append(buf);
    PluginUtils::miDumpBuffer(imageBuffer, filePath);
    return 0;
}

void ArcsoftSllPlugin::setNextStat(const PROCESSSTAT &stat)
{
    MLOGI(Mia2LogGroupPlugin, "set status %s --> %s ",
          SLLProcStateStrings[static_cast<uint32_t>(m_processStatus)],
          SLLProcStateStrings[static_cast<uint32_t>(stat)]);
    m_processStatus = stat;
}

bool ArcsoftSllPlugin::isEnabled(MiaParams settings)
{
    bool supernightEnable = false;
    void *pData = NULL;
    char prop[PROPERTY_VALUE_MAX];
    m_superNightProMode = DefaultMode;

    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.mivi.supernight.mode", &pData);
    if (NULL != pData) {
        m_superNightProMode = *static_cast<int32_t *>(pData);
        MLOGD(Mia2LogGroupPlugin, "get xiaomi.mivi.supernight mode = %d", m_superNightProMode);
        if (m_superNightProMode >= SLLHandMode && m_superNightProMode <= SLLLongTripodMode) {
            memset(prop, 0, sizeof(prop));
            property_get("persist.vendor.algoengine.sll.forcemode", prop, "0");
            int sllForceMode = atoi(prop);
            m_superNightProMode = (sllForceMode > 0) ? sllForceMode : m_superNightProMode;
        }
    }

    supernightEnable =
        ((m_superNightProMode == SLLHandMode) || (m_superNightProMode == SLLTripodMode) ||
         (m_superNightProMode == SLLLongHandMode) || (m_superNightProMode == SLLLongTripodMode));

    // ==== supernight + bokeh feature checker ====
    {
        void *pData =
            VendorMetadataParser::getTag(settings.metadata, "xiaomi.bokeh.superNightEnabled");
        if (NULL != pData) {
            uint8_t superNightBokehEnabled = *static_cast<uint8_t *>(pData);
            if (superNightBokehEnabled == 1) {
                MLOGD(Mia2LogGroupPlugin, "superNightBokehEnabled=%d", superNightBokehEnabled);
                supernightEnable = true;
            }
        }
    }

    MLOGI(Mia2LogGroupPlugin, "[ARCSOFT]: mivi supernightEnable %d m_superNightProMode %d",
          supernightEnable, m_superNightProMode);

    return supernightEnable;
}

int ArcsoftSllPlugin::flushRequest(FlushRequestInfo *pflushRequestInfo)
{
    int ret = 0;
    MLOGI(Mia2LogGroupPlugin, "flush isForced %d m_InputIndex %d", pflushRequestInfo->isForced,
          m_InputIndex.load());
    m_flushFlag = true;
    if (m_hSuperLowlightRaw && (m_InputIndex == m_algoInputNum - 1)) {
        m_hSuperLowlightRaw->setInitStatus(0);
        MLOGI(Mia2LogGroupPlugin, "set user cancel");
    }

    std::lock_guard<std::mutex> lock(m_flushMutex);
    if ((PROCESSEND != getCurrentStat()) && (PROCESSUINIT != getCurrentStat())) {
        setNextStat(PROCESSUINIT);
        processRequestStat();
    }
    m_flushFlag = false;

    MLOGI(Mia2LogGroupPlugin, "flush end %p", this);
    return ret;
}

void ArcsoftSllPlugin::destroy()
{
    MLOGI(Mia2LogGroupPlugin, "destroy %p", this);
    if (m_pFaceRects) {
        delete m_pFaceRects;
        m_pFaceRects = NULL;
        m_FaceParams.pFaceRects = NULL;
    }

    if (m_outputImgParam) {
        delete m_outputImgParam;
        m_outputImgParam = NULL;
    }

    if (m_normalInput) {
        delete m_normalInput;
        m_normalInput = NULL;
    }

    if (m_closestEV0Input) {
        delete m_closestEV0Input;
        m_closestEV0Input = NULL;
    }
}

#ifdef MCAM_SW_DIAG
bool ArcsoftSllPlugin::isLumaRatioMatched(float targetLumaRatio, float curLumaRatio)
{
    MLOGI(Mia2LogGroupPlugin, "[ParamDiag]: (C:T)LumaRatio[%d]:%f-%f", m_InputIndex.load(),
          targetLumaRatio, curLumaRatio);
    return (fabsf(targetLumaRatio - curLumaRatio) <= 0.1 * targetLumaRatio);
}

void ArcsoftSllPlugin::ParamDiag(camera_metadata_t *metaData)
{
    MiImageBuffer inputFrameBuf = {0};
    void *pData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.diag.mask", &pData);
    float curLumaRatio = .0;
    bool isTargetEvOkay = false;
    if (NULL != pData) {
        int32_t nightDiaginfo = *static_cast<int32_t *>(pData);
        if (NightCapture == (NightCapture & nightDiaginfo)) {
            bool crashTriggerNeeded = false;
            const EVINFODIAG *pExpectedEVParams = NULL;
            int count = sizeof((expectedEVInfo)) / sizeof((expectedEVInfo)[0]);
            for (uint16_t i = 0; i < count; i++) {
                if (expectedEVInfo[i].lensPos == m_cameraRoleID) {
                    pExpectedEVParams = &expectedEVInfo[i];
                    break;
                }
            }

            if (pExpectedEVParams != NULL) {
                if ((m_InputIndex >= 1) && (m_InputIndex < m_algoInputNum)) {
                    for (int32_t i = 0; i < maxSceneNum; i++) {
                        if (m_inputEV[m_InputIndex] ==
                            pExpectedEVParams->sceneInfo[i].targetEv[m_InputIndex]) {
                            isTargetEvOkay = true;
                            break;
                        }

                        if (true == isTargetEvOkay) {
                            float curLumaRatio =
                                (m_lumaValue[m_InputIndex] / m_lumaValue[m_InputIndex - 1]);
                            float targetLumaRatio = pow(
                                2, ((pExpectedEVParams->sceneInfo[i].targetEv[m_InputIndex] -
                                     pExpectedEVParams->sceneInfo[i].targetEv[m_InputIndex - 1]) /
                                    6.0));

                            if (false == isLumaRatioMatched(targetLumaRatio, curLumaRatio)) {
                                MLOGI(Mia2LogGroupPlugin,
                                      "[ParamDiag]: LumaRatio Mismatched  curLumaRatio[%d] = %f "
                                      "targetLumaRatio[%d] = %f",
                                      m_InputIndex.load(), curLumaRatio, m_InputIndex.load(),
                                      targetLumaRatio);
                                crashTriggerNeeded = false;
                            }
                        } else {
                            MLOGE(Mia2LogGroupPlugin,
                                  "[ParamDiag]: crash here because of no matched "
                                  "targetEv,cur-Ev[%d] = "
                                  "%d",
                                  m_InputIndex.load(), m_inputEV[m_InputIndex]);
                            crashTriggerNeeded = true;
                        }
                    }
                }
            }

            MLOGI(Mia2LogGroupPlugin, "[ParamDiag]: (C:T) m_InitRet:%d-0 m_camState:%d-3 ",
                  m_InitRet, m_camState);

            if (m_InitRet != 0) {
                MLOGE(Mia2LogGroupPlugin,
                      "[ParamDiag]: crash here because of m_InitRet:%d,expected 0", m_InitRet);
                crashTriggerNeeded = true;
            }

            if (true == crashTriggerNeeded) {
                MLOGE(Mia2LogGroupPlugin, "[ParamDiag]: currentStats:%d crash here for debug only!",
                      getCurrentStat());

                for (uint32_t i = 0; i < m_algoInputNum; i++) {
                    char fileNameBuf[128] = {0};
                    // input × mNumberOfInputImages
                    memset(fileNameBuf, 0, sizeof(fileNameBuf));
                    getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "input", m_dumpTimeBuf);
                    dumpToFile(fileNameBuf, &inputFrameBuf, m_InputIndex, "input");

                    // rawinfo × mNumberOfInputImages
                    memset(fileNameBuf, 0, sizeof(fileNameBuf));
                    getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "rawinfo", m_dumpTimeBuf);
                    dumpRawInfoExt((*m_curInputImgParam), m_InputIndex, m_algoInputNum,
                                   ARC_SN_SCENEMODE_LOWLIGHT, m_CamMode, &m_FaceParams,
                                   &m_rawInfoCurFrame, fileNameBuf);
                }
                // output × 1
                char fileNameBuf[128] = {0};
                int32_t i32ISOValue = 0;
                memset(fileNameBuf, 0, sizeof(fileNameBuf));
                getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "output", m_dumpTimeBuf);
                dumpToFile(fileNameBuf, &inputFrameBuf, m_InputIndex, "output");

                char isSochanged[256] = "";
                property_get("vendor.camera.sensor.logsystem", isSochanged, "0");
                // When the value of logsystem is 1 or 3, the send_message_to_mqs interface is
                // called When the value of logsystem is 2, it means that the exception needs to be
                // ignored
                if (atoi(isSochanged) == 2) {
                    abort();
                }
                if ((atoi(isSochanged) == 1) || (atoi(isSochanged) == 3)) {
                    char CamErrorString[256] = {0};
                    sprintf(CamErrorString,
                            "[SuperNight][ParamDiag-MIVI]: crash here for debug only!");
                    MLOGE(Mia2LogGroupPlugin, "[SuperNight][ParamDiag-MIVI] %s", CamErrorString);
                    camlog::send_message_to_mqs(CamErrorString, false);
                }
            }
        }
    }
}

#endif
