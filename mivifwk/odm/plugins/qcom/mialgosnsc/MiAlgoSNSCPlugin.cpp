#include "MiAlgoSNSCPlugin.h"

#include <utils/String8.h>

#undef LOG_TAG
#define LOG_TAG "MIA_SNSC"

using namespace mialgo2;
using namespace tinyxml2;

static const int numSNSCFrames = 8;
static int64_t mstRef = 0;
static std::mutex mstMutex;
MialgoAi_DIPS_Handle MiAlgoSNSCPlugin::m_dipsHandle = nullptr;
MST::Result<MialgoAi_DIPS_Status> MiAlgoSNSCPlugin::m_async_result;

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

// for algo to dump offline log
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

MiAlgoSNSCPlugin::MiAlgoSNSCPlugin() : m_realEnable(false)
{
    m_SNSCState = STATE_IDLE;
    MLOGI(Mia2LogGroupPlugin, "construct %p status %d", this, m_SNSCState);
}

MiAlgoSNSCPlugin::~MiAlgoSNSCPlugin()
{
    MLOGI(Mia2LogGroupPlugin, "distruct %p is realEnable %d", this, m_realEnable);
    if (m_realEnable) {
        std::lock_guard<std::mutex> lock(mstMutex);
        MLOGI(Mia2LogGroupPlugin, "%p mstRef %ld", this, mstRef);
        mstRef--;
        if (mstRef <= 0) {
            MLOGI(Mia2LogGroupPlugin, "doInitModel Unregisted with Signal %d Start", MST::SIG_SNSC);
            MST::disconnect(MST::SIG_SNSC);
            MLOGI(Mia2LogGroupPlugin, "doInitModel Unregisted with Signal %d End", MST::SIG_SNSC);

            if (nullptr != m_dipsHandle) {
                MialgoAi_DIPS_Unit_Model(m_dipsHandle);
            }
            m_dipsHandle = nullptr;
        }
    }

    MLOGI(Mia2LogGroupPlugin, "~MiAlgoSNSCPlugin");
}

int MiAlgoSNSCPlugin::safeIndex(int idx)
{
    int min_idx = 0;
    int max_idx = std::min(m_supportFrameSize, SNSC_SUPPORT_FRAME_SIZE);
    if (min_idx >= max_idx) {
        MLOGI(Mia2LogGroupPlugin, "ERROR !!! Invalid Index range %d to %d", min_idx, max_idx);
        return 0;
    }
    if (idx < min_idx) {
        MLOGI(Mia2LogGroupPlugin, "ERROR !!! Index %d out of range %d to %d", idx, min_idx,
              max_idx);
        return min_idx;
    }
    if (idx >= max_idx) {
        MLOGI(Mia2LogGroupPlugin, "ERROR !!! Index %d out of range %d to %d", idx, min_idx,
              max_idx);
        return max_idx;
    }
    return idx;
}

MialgoAi_DIPS_Status MiAlgoSNSCPlugin::doInitModel()
{
    MLOGI(Mia2LogGroupPlugin, "doInitModel Task Processed start");
    if (nullptr != m_dipsHandle) {
        MialgoAi_DIPS_Unit_Model(m_dipsHandle);
    }
    m_dipsHandle = nullptr;

    MialgoAi_DIPS_InitModelParams initParams;
    initParams.dips_type = MIALGO_SNSC;
    initParams.num_frames = numSNSCFrames;
    initParams.path_assets = "/odm/etc/camera/snsc.bin";
    initParams.path_backend = "/odm/lib64/libQnnHtpStub.so";
    auto status = MialgoAi_DIPS_Init_Model(m_dipsHandle, initParams);
    MLOGI(Mia2LogGroupPlugin, "doInitModel Task Processed end");
    return status;
}

int MiAlgoSNSCPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    MLOGI(Mia2LogGroupPlugin, "initialize %p", this);
    // m_realEnable = (createInfo->operationMode == StreamConfigModeSAT) &&
    //                (createInfo->frameworkCameraId == CAM_COMBMODE_REAR_SAT_WT);
    m_realEnable = true;
    MLOGI(Mia2LogGroupPlugin, "createInfo operationMode %d, frameworkCameraId %d is realEnable %d",
          createInfo->operationMode, createInfo->frameworkCameraId, m_realEnable);

    if (m_realEnable) {
        std::lock_guard<std::mutex> lock(mstMutex);
        MLOGI(Mia2LogGroupPlugin, "%p mstRef %ld", this, mstRef);
        if (mstRef == 0) {
            MLOGI(Mia2LogGroupPlugin, "doInitModel Task Registed with Signal %d Start",
                  MST::SIG_SNSC);
            m_async_result = MST::connect(MST::SIG_SNSC, &MiAlgoSNSCPlugin::doInitModel, this);
            MLOGI(Mia2LogGroupPlugin, "doInitModel Task Registed with Signal %d %s", MST::SIG_SNSC,
                  m_async_result.valid() ? "End" : "Failed");
        }
        mstRef++;
    }

    m_nodeInterface = nodeInterface;

    return triggerCmd(CMD_INITIALIZE, createInfo);
}

ProcessRetStatus MiAlgoSNSCPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    MLOGI(Mia2LogGroupPlugin, "processRequestInfo frameNum ---------------");

    ProcessRetStatus processStatus = PROCSUCCESS;

    if (triggerCmd(CMD_PROCESS, processRequestInfo) != OK) {
        processStatus = PROCFAILED;
    }

    return processStatus;
}

ProcessRetStatus MiAlgoSNSCPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    MLOGI(Mia2LogGroupPlugin, "processRequest v2");

    ProcessRetStatus resultInfo = PROCSUCCESS;

    return resultInfo;
}

int MiAlgoSNSCPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    MLOGI(Mia2LogGroupPlugin, "flushRequest");

    return triggerCmd(CMD_FLUSH, flushRequestInfo);
}

void MiAlgoSNSCPlugin::destroy()
{
    MLOGI(Mia2LogGroupPlugin, "destroy");

    triggerCmd(CMD_DESTROY);
}

bool MiAlgoSNSCPlugin::isEnabled(MiaParams settings)
{
    MLOGI(Mia2LogGroupPlugin, "isEnabled");

    void *pData = nullptr;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.nightmotioncapture.mode", &pData);
    int mode = pData ? *static_cast<int *>(pData) : 0;

    bool is_snsc_enable = (0 != mode);

    MLOGI(Mia2LogGroupPlugin, "isEnabled:%d", is_snsc_enable);

    m_dumpMask = property_get_int32("persist.vendor.algoengine.snsc.dump", 0);

    return is_snsc_enable;
}

int MiAlgoSNSCPlugin::triggerCmd(int cmd, void *arg)
{
    m_lock.lock();
    MialgoAi_DIPS_Status status;
    MLOGI(Mia2LogGroupPlugin, "cmd:%d", cmd);
    switch (cmd) {
    case CMD_INITIALIZE: {
        status = doInit((CreateInfo *)arg);
        break;
    }
    case CMD_PROCESS: {
        status = doProcess((ProcessRequestInfo *)arg);
        break;
    }
    case CMD_FLUSH: {
        status = doFlush((FlushRequestInfo *)arg);
        break;
    }
    case CMD_DESTROY: {
        status = doDestroy();
        break;
    }
    default: {
        status = BAD_OP;
        break;
    }
    }

    MLOGI(Mia2LogGroupPlugin, "state:%d", m_SNSCState);
    m_lock.unlock();

    return status;
}

MialgoAi_DIPS_Status MiAlgoSNSCPlugin::doInit(CreateInfo *createInfo)
{
    m_SNSCExifInfo = "snsc:{on:1";

    MialgoAi_DIPS_Verison version = MialgoAi_DIPS_GetVersion();
    MLOGI(Mia2LogGroupPlugin, "version(%d.%d.%d.%d)", version.major, version.minor, version.patch,
          version.revision);
    MialgoAi_DIPS_SetLogLevel(1, printAndroidAndOfflineLog);

    /*auto version = [&]() {
        return toStr(m_ellc_version.major) + "." + toStr(m_ellc_version.minor) + "." +
               toStr(m_ellc_version.patch) + "." + toStr(m_ellc_version.revision);
    };*/
    m_SNSCExifInfo += (", version:" + toStr(version.major) + "." + toStr(version.minor) + "." +
                       toStr(version.patch) + "." + toStr(version.revision));

    memset(m_gyroElement, 0, sizeof(MialgoAi_DIPS_GyroElement) * MAX_GYRO_ELEMENT_SIZE);

    m_supportFrameSize = 0;
    m_processedFrameSize = 0;

    m_baseFrameIndex = 0;

    return OK;
}

MialgoAi_DIPS_Status MiAlgoSNSCPlugin::doProcess(ProcessRequestInfo *processRequestInfo)
{
    MialgoAi_DIPS_Status processStatus = OK;

    if (processRequestInfo == nullptr) {
        MLOGE(Mia2LogGroupPlugin, "processRequestInfo is nullptr!!!");
        processStatus = NULL_PTR;
        return processStatus;
    }

    uint32_t inNum = processRequestInfo->inputBuffersMap.size();
    uint32_t outNum = processRequestInfo->outputBuffersMap.size();

    MLOGI(Mia2LogGroupPlugin, "processRequestInfo frameNum %d inNum %d outNum %d isFirstFrame %d ",
          processRequestInfo->frameNum, inNum, outNum, processRequestInfo->isFirstFrame);

    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;

    if (processRequestInfo->isFirstFrame) {
        /***************************************for dump****************************************/
        if (m_dumpMask > 0) {
            int size = sizeof(m_dumpPrefixs);
            memset(m_dumpPrefixs, 0, size);
            char str[size];
            time_t currentTime;
            struct tm *timeInfo;
            time(&currentTime);
            timeInfo = localtime(&currentTime);
            strftime(str, size, "%Y%m%d_%H%M%S", timeInfo);
            struct timeval time;
            gettimeofday(&time, NULL);
            snprintf(m_dumpPrefixs, size, "IMG_%s_%03d", str, (time.tv_usec / 1000));
            MLOGI(Mia2LogGroupPlugin, "m_dumpPrefixs:%s", m_dumpPrefixs);

            XMLDocument xml;
            XMLDeclaration *declaration = xml.NewDeclaration();
            xml.InsertFirstChild(declaration);
            XMLElement *root = xml.NewElement("META");
            xml.InsertEndChild(root);
            char fileName[128];
            memset(fileName, 0, sizeof(fileName));
            snprintf(fileName, size, "/data/vendor/camera/%s_meta.xml", m_dumpPrefixs);
            xml.SaveFile(fileName);
        }
        /***************************************for dump****************************************/

        if (m_SNSCState == STATE_IDLE) {
            doInit(nullptr);
        }

        auto override_buffer = [&](const char *str, MiImageBuffer &mFrame,
                                   const ImageParams &rFrame) {
            mFrame.format = rFrame.format.format;
            mFrame.width = rFrame.format.width;
            mFrame.height = rFrame.format.height;
            mFrame.plane[0] = rFrame.pAddr[0];
            mFrame.fd[0] = rFrame.fd[0];
            mFrame.stride = rFrame.planeStride;
            mFrame.scanline = rFrame.sliceheight;
            mFrame.numberOfPlanes = rFrame.numberOfPlanes;
            MLOGI(Mia2LogGroupPlugin,
                  "%s buffer info {handle: %p, fd: %d, width: %d, height: %d, stride: %d, format: "
                  "%d}",
                  str, mFrame.plane[0], mFrame.fd[0], mFrame.width, mFrame.height, mFrame.stride,
                  mFrame.format);
        };

        auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;
        override_buffer("output", m_outputImageBuffer, outputBuffers[0]);
        mp_outputMeta = (outputBuffers[0].pPhyCamMetadata == NULL)
                            ? outputBuffers[0].pMetadata
                            : outputBuffers[0].pPhyCamMetadata;

        override_buffer("input", m_inputImageBuffer, inputBuffers[0]);

        if (m_dumpMask > 0) {
            char fileNameBuf[128] = {0};
            char dumpTimeBuf[128];
            memset(fileNameBuf, 0, sizeof(fileNameBuf));
            gettimestr(dumpTimeBuf, sizeof(dumpTimeBuf));

            getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "input", dumpTimeBuf,
                            m_inputImageBuffer.width, m_inputImageBuffer.height);
            dumpToFile(fileNameBuf, &m_inputImageBuffer, processRequestInfo->frameNum, "input");
        }

        MLOGI(Mia2LogGroupPlugin, "before init m_dipsHandle:%p", m_dipsHandle);

        processStatus = processSNSCInit(processRequestInfo);

        m_outFrameIdx = processRequestInfo->frameNum;
        m_outFrameTimeStamp = processRequestInfo->timeStamp;

        MLOGI(Mia2LogGroupPlugin, "after init m_dipsHandle:%p", m_dipsHandle);
    }

    m_processedFrameSize = safeIndex(m_processedFrameSize);
    mp_inputMeta[m_processedFrameSize] = (inputBuffers[0].pPhyCamMetadata == NULL)
                                             ? inputBuffers[0].pMetadata
                                             : inputBuffers[0].pPhyCamMetadata;
    MLOGI(Mia2LogGroupPlugin, "set inputmeta index : %d", m_processedFrameSize);

    auto timestamp = [](auto pMetadata) {
        void *pData = NULL;
        VendorMetadataParser::getTagValue(pMetadata, ANDROID_SENSOR_TIMESTAMP, &pData);
        if (nullptr != pData) {
            return *static_cast<int64_t *>(pData);
        } else {
            MLOGE(Mia2LogGroupPlugin, "Invalid timestamp from metadata %p", pMetadata);
            return static_cast<int64_t>(0);
        }
    };
    m_frameTimestamps[m_processedFrameSize] = timestamp(mp_inputMeta[m_processedFrameSize]);

    if (processStatus == OK) {
        processStatus = processSNSCProcess(processRequestInfo);
        MLOGI(Mia2LogGroupPlugin, "after process m_dipsHandle:%p", m_dipsHandle);
    }

    if (processStatus == OK) {
        m_processedFrameSize++;
        MLOGI(Mia2LogGroupPlugin, "m_processedFrameSize:%d m_supportFrameSize:%d",
              m_processedFrameSize, m_supportFrameSize);

        if (m_processedFrameSize == m_supportFrameSize) {
            int64_t anchorTimestamp = m_frameTimestamps[m_baseFrameIndex];
            std::string result{"quickview"};
            MLOGI(Mia2LogGroupPlugin, "send m_baseFrameIndex:%d ts %" PRIu64, m_baseFrameIndex,
                  anchorTimestamp);
            m_nodeInterface.pSetResultMetadata(m_nodeInterface.owner, processRequestInfo->frameNum,
                                               anchorTimestamp, result);

            auto get_time_us = []() -> double {
                struct timeval time;
                gettimeofday(&time, NULL);
                double ms = time.tv_sec * 1000.0 + time.tv_usec / 1000.0;
                return ms;
            };

            double start = get_time_us();
            processStatus = processSNSCRun(processRequestInfo);
            double end = get_time_us();
            double cost = end - start;

            // TODO:may delete after debug
            MiAbnormalDetect abnDetect = PluginUtils::isAbnormalImage(
                m_outputImageBuffer.plane, m_outputImageBuffer.format, m_outputImageBuffer.stride,
                m_outputImageBuffer.width, m_outputImageBuffer.height);

            if (m_dumpMask > 0 || abnDetect.isAbnormal) {
                char fileNameBuf[128] = {0};
                char dumpTimeBuf[128];
                memset(fileNameBuf, 0, sizeof(fileNameBuf));
                gettimestr(dumpTimeBuf, sizeof(dumpTimeBuf));

                MLOGE(Mia2LogGroupPlugin,
                      "MiAlgoSNSCPlugin------>dumpToFile<------isAbnormal = %d!",
                      abnDetect.isAbnormal);

                getDumpFileName(fileNameBuf, sizeof(fileNameBuf), "output", dumpTimeBuf,
                                m_inputImageBuffer.width, m_inputImageBuffer.height);
                dumpToFile(fileNameBuf, &m_outputImageBuffer, processRequestInfo->frameNum,
                           "output");
            }

            MLOGI(Mia2LogGroupPlugin, "after run m_dipsHandle:%p cost:%lf", m_dipsHandle, cost);

            m_SNSCExifInfo += (", alret:" + toStr(processStatus));
            m_SNSCExifInfo += (", fmidx:" + toStr(m_processedFrameSize));
            m_SNSCExifInfo += (", acost:" + toStr(cost, 2));

            // PluginUtils::miCopyBuffer(&m_outputImageBuffer, &m_inputImageBuffer);
        }
    }

    if (processStatus == OK && m_SNSCState == STATE_COMPLETED) {
        processStatus = processSNSCUnit(false);
    }

    return processStatus;
}

MialgoAi_DIPS_Status MiAlgoSNSCPlugin::doFlush(FlushRequestInfo *flushRequestInfo)
{
    MialgoAi_DIPS_Status flushStatus = OK;

    if (m_SNSCState != STATE_ERROR && m_SNSCState != STATE_IDLE) {
        flushStatus = processSNSCUnit(true);
    }

    return flushStatus;
}

MialgoAi_DIPS_Status MiAlgoSNSCPlugin::doDestroy()
{
    MialgoAi_DIPS_Status destroyStatus = OK;

    return destroyStatus;
}

MialgoAi_DIPS_Status MiAlgoSNSCPlugin::processSNSCInit(ProcessRequestInfo *processRequestInfo)
{
    MialgoAi_DIPS_Status initStatus = OK;
    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    camera_metadata_t *pInputMeta = (inputBuffers[0].pPhyCamMetadata == NULL)
                                        ? inputBuffers[0].pMetadata
                                        : inputBuffers[0].pPhyCamMetadata;

    void *pData = nullptr;
    NightMotionCaptureInfo *nmCapInfo = nullptr;
    VendorMetadataParser::getVTagValue(pInputMeta,
                                       "xiaomi.nightmotioncapture.nightmotioncaptureInfo", &pData);
    if (pData != nullptr) {
        nmCapInfo = static_cast<NightMotionCaptureInfo *>(pData);
        if (nmCapInfo != nullptr) {
            MLOGI(Mia2LogGroupPlugin, "nmCapInfo burstCount:%u burstIndex:%u",
                  nmCapInfo->burstCount, nmCapInfo->burstIndex);

            // for stg&non-zsl mode
            if (nmCapInfo->burstCount == 2) {
                m_supportFrameSize = 1;
            } else {
                m_supportFrameSize = nmCapInfo->burstCount;
            }
        }
    } else {
        MLOGE(Mia2LogGroupPlugin, "get meta nightmotioncaptureInfo failed");
    }

    MialgoAi_DIPS_InitParams initParams;

    // fill raw burst info
    initParams.raw_burst_info.raw_type = IDEAL_RAW_14BIT;
    initParams.raw_burst_info.num_frames = m_supportFrameSize;
    initParams.raw_burst_info.height = inputBuffers[0].format.height;
    initParams.raw_burst_info.width = inputBuffers[0].format.width;
    initParams.raw_burst_info.stride = inputBuffers[0].planeStride;

    // fill black level
    pData = nullptr;
    VendorMetadataParser::getTagValue(pInputMeta, ANDROID_SENSOR_DYNAMIC_BLACK_LEVEL, &pData);
    if (pData != nullptr) {
        float black_level[4] = {0};
        memcpy(black_level, static_cast<float *>(pData), 4 * sizeof(float));
        initParams.raw_burst_info.black_level[0] = (int32_t)black_level[0];
        initParams.raw_burst_info.black_level[1] = (int32_t)black_level[1];
        initParams.raw_burst_info.black_level[2] = (int32_t)black_level[2];
        initParams.raw_burst_info.black_level[3] = (int32_t)black_level[3];
        MLOGI(Mia2LogGroupPlugin, "bl0:%d bl1:%d bl2:%d bl3:%d",
              initParams.raw_burst_info.black_level[0], initParams.raw_burst_info.black_level[1],
              initParams.raw_burst_info.black_level[2], initParams.raw_burst_info.black_level[3]);
    } else {
        initParams.raw_burst_info.black_level[0] = 1029;
        initParams.raw_burst_info.black_level[1] = 1029;
        initParams.raw_burst_info.black_level[2] = 1029;
        initParams.raw_burst_info.black_level[3] = 1029;
        MLOGE(Mia2LogGroupPlugin, "failed load black level, force %d",
              initParams.raw_burst_info.black_level[0]);
    }

    // fill white level
    pData = nullptr;
    VendorMetadataParser::getTagValue(pInputMeta, ANDROID_SENSOR_DYNAMIC_WHITE_LEVEL, &pData);
    if (pData != nullptr) {
        initParams.raw_burst_info.white_level = *static_cast<int32_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "wl:%d", initParams.raw_burst_info.white_level);
    } else {
        initParams.raw_burst_info.white_level = 16384;
        MLOGE(Mia2LogGroupPlugin, "failed load white level, force %d",
              initParams.raw_burst_info.white_level);
    }

    int overrideBlevl = property_get_int32("persist.vendor.algoengine.snsc.blevl", 0);
    int overrideWlevl = property_get_int32("persist.vendor.algoengine.snsc.wlevl", 0);

    if (overrideBlevl != 0) {
        initParams.raw_burst_info.black_level[0] = overrideBlevl;
        initParams.raw_burst_info.black_level[1] = overrideBlevl;
        initParams.raw_burst_info.black_level[2] = overrideBlevl;
        initParams.raw_burst_info.black_level[3] = overrideBlevl;
        MLOGI(Mia2LogGroupPlugin, "override blevl to %d", overrideBlevl);
    } else {
        initParams.raw_burst_info.black_level[0] = 1024;
        initParams.raw_burst_info.black_level[1] = 1024;
        initParams.raw_burst_info.black_level[2] = 1024;
        initParams.raw_burst_info.black_level[3] = 1024;
    }

    if (overrideWlevl != 0) {
        initParams.raw_burst_info.white_level = overrideWlevl;
        MLOGI(Mia2LogGroupPlugin, "override wlevl to %d", overrideWlevl);
    } else {
        initParams.raw_burst_info.white_level = 16383;
    }

    // update exif info
    m_SNSCExifInfo += (", blevl:" + toStr(initParams.raw_burst_info.black_level[0]));
    m_SNSCExifInfo += (", wlevl:" + toStr(initParams.raw_burst_info.white_level));

    pData = nullptr;
    ChiTuningModeParameter tuningParam;
    memset(&tuningParam, 0, sizeof(ChiTuningModeParameter));
    VendorMetadataParser::getVTagValue(pInputMeta, "org.quic.camera2.tuning.mode.TuningMode",
                                       &pData);
    if (pData != nullptr) {
        memcpy(&tuningParam, pData, sizeof(ChiTuningModeParameter));
    }
    m_SNSCExifInfo += (", tning:{DE:" + toStr(tuningParam.TuningMode[0].subMode.value) + // default
                       ",SM:" + toStr(tuningParam.TuningMode[1].subMode.value) +    // sensor mdoe
                       ",UC:" + toStr(tuningParam.TuningMode[2].subMode.usecase) +  // usecase
                       ",F1:" + toStr(tuningParam.TuningMode[3].subMode.feature1) + // feature 1
                       ",F2:" + toStr(tuningParam.TuningMode[4].subMode.feature2) + // feature 2
                       ",SC:" + toStr(tuningParam.TuningMode[5].subMode.scene) +    // scene
                       ",EF:" + toStr(tuningParam.TuningMode[6].subMode.effect) + "}"); // effect

#if defined(NUWA_CAM)
    initParams.raw_burst_info.bayer_pattern = BAYER_RGGB;
#else
    initParams.raw_burst_info.bayer_pattern = BAYER_BGGR;
#endif

    //
    initParams.mode = GENERAL;
    pData = nullptr;
    VendorMetadataParser::getVTagValue(pInputMeta, "xiaomi.ai.misd.StateScene", &pData);
    if (pData != nullptr) {
        if (static_cast<AiMisdScene *>(pData)->value) {
            initParams.mode = TRIPOD;
        }
    }

    m_SNSCExifInfo += (", almod:" + toStr(initParams.mode));

    //
    initParams.dips_type = MIALGO_SNSC;

    //
    initParams.path_assets = "/odm/etc/camera/snsc.bin";
    initParams.path_backend = "/odm/lib64/libQnnHtpStub.so";

    // do init
    MialgoAi_DIPS_Status status{NULL_HANDLE};
    MST::signal(MST::SIG_SNSC);
    if (m_async_result.wait(5000)) {
        status = m_async_result.get();
    } else {
        MLOGI(Mia2LogGroupPlugin, "ERROR !!! async model timeout 5000ms check result %s",
              m_async_result.valid() ? "valid" : "invalid");

        // the async task may still runing here
        // the task bind to MST::SIG_SNSC will not do after disconnect
        MST::disconnect(MST::SIG_SNSC);

        // m_dipsHandle is a global static handle
        // doInitModel will uninit m_dipsHandle first if it's not null
        status = doInitModel();
    }
    initStatus = status;

    MLOGI(Mia2LogGroupPlugin, "snsc init model result:%d", initStatus);

    initStatus = MialgoAi_DIPS_Init(m_dipsHandle, initParams);
    MLOGI(Mia2LogGroupPlugin, "snsc init result:%d", initStatus);

    if (initStatus == OK) {
        m_SNSCState = STATE_INITIALIZED;
    } else {
        m_SNSCState = STATE_ERROR;
    }

    return initStatus;
}

MialgoAi_DIPS_Status MiAlgoSNSCPlugin::processSNSCProcess(ProcessRequestInfo *processRequestInfo)
{
    if (m_dipsHandle == nullptr) {
        MLOGE(Mia2LogGroupPlugin, "m_dipsHandle is nullptr");
        return NULL_HANDLE;
    }

    m_SNSCState = STATE_PROCESSING;

    MialgoAi_DIPS_Status processStatus = OK;

    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    camera_metadata_t *pInputMeta = (inputBuffers[0].pPhyCamMetadata == NULL)
                                        ? inputBuffers[0].pMetadata
                                        : inputBuffers[0].pPhyCamMetadata;

    MialgoAi_DIPS_PreprocessParams processParams{0};
    processParams.raw_data = inputBuffers[0].pAddr[0];
    processParams.fd = inputBuffers[0].fd[0];
    // fill base_index only when last frame preprocess done
    processParams.base_index = &m_baseFrameIndex;

    // fill gyro data
    fillGyroData(pInputMeta, processParams.gyro_data);

    // fill gain info
    fillGainInfo(pInputMeta, processParams.gain_info,
                 (float)processParams.gyro_data.frame_time.exp_dur, processParams.banding);

    // fill motion info
    fillMotionInfo(pInputMeta, processParams.motion_max);

    processStatus = MialgoAi_DIPS_Preprocess(m_dipsHandle, processParams);
    m_baseFrameIndex = safeIndex(m_baseFrameIndex);
    MLOGI(Mia2LogGroupPlugin, "snsc process result:%d", processStatus);
    if (processStatus != OK) {
        m_SNSCState = STATE_ERROR;
    }

    return processStatus;
}

MialgoAi_DIPS_Status MiAlgoSNSCPlugin::processSNSCRun(ProcessRequestInfo *processRequestInfo)
{
    if (m_dipsHandle == nullptr) {
        MLOGE(Mia2LogGroupPlugin, "m_dipsHandle is nullptr");
        return NULL_HANDLE;
    }

    m_SNSCState = STATE_RUNNING;

    MialgoAi_DIPS_Status runStatus = OK;

    MialgoAi_DIPS_RunParams runParams;

    runParams.raw_data = m_outputImageBuffer.plane[0];
    runParams.fd = m_outputImageBuffer.fd[0];

    runStatus = MialgoAi_DIPS_Run(m_dipsHandle, runParams);
    MLOGI(Mia2LogGroupPlugin, "snsc run result:%d baseFrameINdex:%d", runStatus, m_baseFrameIndex);

    if (runStatus == OK) {
        m_SNSCState = STATE_COMPLETED;
    } else {
        m_SNSCState = STATE_ERROR;
    }

    return runStatus;
}

MialgoAi_DIPS_Status MiAlgoSNSCPlugin::processSNSCUnit(bool isFlush)
{
    if (m_dipsHandle == nullptr) {
        MLOGE(Mia2LogGroupPlugin, "m_dipsHandle is nullptr");
        return NULL_HANDLE;
    }

    MialgoAi_DIPS_Status uninitStatus = OK;

    MialgoAi_DIPS_ExecLog execLog;
    MialgoAi_DIPS_GetExecLog(m_dipsHandle, execLog);

    m_SNSCExifInfo += (", Exec:{ratio:" + toStr(execLog.ratio) +          // algo exec ratio
                       ",output_type:" + toStr(execLog.output_type) +     // algo exec output_type
                       ",frame_abandon:" + toStr(execLog.frame_abandon) + // algo exec frame_abandon
                       ",base:" + toStr(m_baseFrameIndex) + "}");         // algo selected frame
    m_SNSCExifInfo += mExifGainInfo[m_baseFrameIndex] + mExifMotionInfo + mExifLPInfo;
    m_SNSCExifInfo += "}";
    MLOGI(Mia2LogGroupPlugin, "update snsc exif info: %s", m_SNSCExifInfo.c_str());

    if (m_nodeInterface.pSetResultMetadata != NULL) {
        m_nodeInterface.pSetResultMetadata(m_nodeInterface.owner, m_outFrameIdx,
                                           m_outFrameTimeStamp, m_SNSCExifInfo);
    }

    // erase exif strings
    mExifMotionInfo.erase();
    for (int i = 0; i < SNSC_SUPPORT_FRAME_SIZE; i++) {
        mExifGainInfo[i].erase();
    }
    m_SNSCExifInfo.erase();
    mExifLPInfo.erase();

    MLOGI(Mia2LogGroupPlugin, "before unint m_dipsHandle:%p", m_dipsHandle);
    if (isFlush) {
        uninitStatus = MialgoAi_DIPS_Terminate(m_dipsHandle, 100);
    } else {
        uninitStatus = MialgoAi_DIPS_Unit(m_dipsHandle);
    }

    if (uninitStatus != OK) {
        m_SNSCState = STATE_ERROR;
    } else {
        m_SNSCState = STATE_IDLE;
    }
    MLOGI(Mia2LogGroupPlugin, "uninit result:%d", uninitStatus);
    // !!! DO NOT set to null on model init version
    // m_dipsHandle = nullptr;
    MLOGI(Mia2LogGroupPlugin, "after unint m_dipsHandle:%p", m_dipsHandle);

    MialgoAi_DIPS_SetLogLevel(0, printAndroidAndOfflineLog);

    if (mp_inputMeta[m_baseFrameIndex] != NULL && mp_outputMeta != NULL && (isFlush == false)) {
        VendorMetadataParser::mergeMetadata(mp_outputMeta, mp_inputMeta[m_baseFrameIndex]);
    }

    mp_outputMeta = nullptr;
    for (int i = 0; i < 8; i++) {
        mp_inputMeta[i] = nullptr;
    }

    return uninitStatus;
}

void MiAlgoSNSCPlugin::fillGyroData(camera_metadata_t *pInputMeta, MialgoAi_DIPS_GyroData &gyroData)
{
    if (pInputMeta == nullptr) {
        MLOGE(Mia2LogGroupPlugin, "fillGyroData fail cause pInputMeta is nullptr");
    }

    void *pData = nullptr;
    CHITIMESTAMPINFO timestampInfo = {0};
    uint64_t frameDuration = 0;
    uint64_t frameExposureTime = 0;
    uint64_t frameRollingShutterSkew = 0;

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pInputMeta, "org.quic.camera.qtimer.timestamp", &pData);
    if (pData) {
        timestampInfo = *static_cast<CHITIMESTAMPINFO *>(pData);
    } else {
        MLOGE(Mia2LogGroupPlugin, "get metadata error 'org.quic.camera.qtimer.timestamp'");
    }

    pData = nullptr;
    VendorMetadataParser::getTagValue(pInputMeta, ANDROID_SENSOR_FRAME_DURATION, &pData);
    if (pData) {
        frameDuration = *static_cast<int64_t *>(pData);
    } else {
        MLOGE(Mia2LogGroupPlugin, "get metadata error 'ANDROID_SENSOR_FRAME_DURATION'");
    }

    pData = nullptr;
    VendorMetadataParser::getTagValue(pInputMeta, ANDROID_SENSOR_EXPOSURE_TIME, &pData);
    if (pData) {
        frameExposureTime = *static_cast<int64_t *>(pData);
    } else {
        MLOGE(Mia2LogGroupPlugin, "get metadata error 'ANDROID_SENSOR_EXPOSURE_TIME'");
    }

    pData = nullptr;
    VendorMetadataParser::getTagValue(pInputMeta, ANDROID_SENSOR_ROLLING_SHUTTER_SKEW, &pData);
    if (pData) {
        frameRollingShutterSkew = *static_cast<int64_t *>(pData);
    } else {
        MLOGE(Mia2LogGroupPlugin, "get metadata error 'ANDROID_SENSOR_ROLLING_SHUTTER_SKEW'");
    }

    MLOGI(Mia2LogGroupPlugin, "ellc time info {sof:%llu, dur:%lld, exp:%lld, shutter-skew:%lld}",
          timestampInfo.timestamp, frameDuration, frameExposureTime, frameRollingShutterSkew);

    uint64_t sof = static_cast<uint64_t>(NanoToMicro(timestampInfo.timestamp));
    frameDuration = static_cast<uint64_t>(NanoToMicro(frameDuration));
    frameExposureTime = static_cast<uint64_t>(NanoToMicro(frameExposureTime));
    frameRollingShutterSkew = static_cast<uint64_t>(NanoToMicro(frameRollingShutterSkew));

    gyroData.frame_time.exp_beg = sof - frameExposureTime;
    gyroData.frame_time.exp_end = sof + frameRollingShutterSkew;
    gyroData.frame_time.exp_dur = frameExposureTime;

    MLOGI(Mia2LogGroupPlugin, "ellc frame time {beigin:%lu, end:%lu, dur:%lu}",
          gyroData.frame_time.exp_beg, gyroData.frame_time.exp_end, gyroData.frame_time.exp_dur);

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pInputMeta, "xiaomi.mivi.ellc.gyro", &pData);
    if (pData != nullptr) {
        ChiFrameGyro *gyro = static_cast<ChiFrameGyro *>(pData);
        gyroData.count = gyro->gcount;
        if (gyro->gcount > 0) {
            gyroData.frame_time.exp_beg = NanoToMicro(QtimerTicksToQtimerNano(gyro->ts));
            gyroData.frame_time.exp_end = NanoToMicro(QtimerTicksToQtimerNano(gyro->te));

            for (int i = 0; i < gyro->gcount; i++) {
                m_gyroElement[i].shift_x = gyro->x[i];
                m_gyroElement[i].shift_y = gyro->y[i];
                m_gyroElement[i].shift_z = gyro->z[i];
                m_gyroElement[i].timestamp = NanoToMicro(QtimerTicksToQtimerNano(gyro->t[i]));
            }

            //[begin, end)
            std::sort(m_gyroElement, m_gyroElement + gyroData.count,
                      [](const MialgoAi_DIPS_GyroElement &d1, const MialgoAi_DIPS_GyroElement &d2) {
                          return d1.timestamp < d2.timestamp;
                      });
        } else {
            gyroData.count = 200;
            memset(m_gyroElement, 0, sizeof(MialgoAi_DIPS_GyroElement) * MAX_GYRO_ELEMENT_SIZE);
            MLOGE(Mia2LogGroupPlugin, "gyro data count zero");

            gyroData.frame_time.exp_beg = 0;
            gyroData.frame_time.exp_end = 0;
        }

        gyroData.gyro_eles = m_gyroElement;
    }
}

void MiAlgoSNSCPlugin::fillGainInfo(camera_metadata_t *pInputMeta, MialgoAi_DIPS_GainInfo &gainInfo,
                                    float shutterCapt, int &banding)
{
    if (pInputMeta == nullptr) {
        MLOGE(Mia2LogGroupPlugin, "fillGainInfo fail cause pInputMeta is nullptr");
        return;
    }

    UINT64 finalExposureTime = 0;
    float finalLinearGain = 0.0f;

    gainInfo.shutter_capt = shutterCapt;

    void *pData = nullptr;
    AECFrameControl *pAecFrameControl = nullptr;
    VendorMetadataParser::getVTagValue(pInputMeta, "org.quic.camera2.statsconfigs.AECFrameControl",
                                       &pData);
    if (pData != nullptr) {
        pAecFrameControl = static_cast<AECFrameControl *>(pData);
    }

    if (pAecFrameControl != nullptr) {
        gainInfo.shutter_prev =
            NanoToMicro(pAecFrameControl->exposureInfo[ExposureIndexSafe].exposureTime);
        gainInfo.cur_luma = pAecFrameControl->exposureInfo[ExposureIndexShort].exposureTime *
                            pAecFrameControl->exposureInfo[ExposureIndexShort].linearGain;
        gainInfo.lux_index = pAecFrameControl->luxIndex;
        gainInfo.adrc_gain = pAecFrameControl->compenADRCGain;

        finalExposureTime = pAecFrameControl->exposureInfo[ExposureIndexShort].exposureTime;
        finalLinearGain = pAecFrameControl->exposureInfo[ExposureIndexShort].linearGain;
    } else {
        gainInfo.shutter_prev = 30000.0f;
        gainInfo.cur_luma = 20.0f;
        gainInfo.adrc_gain = 1.0f;
    }

    // fill isp gain
    pData = nullptr;
    VendorMetadataParser::getVTagValue(pInputMeta, "com.qti.sensorbps.gain", &pData);
    if (pData != nullptr) {
        gainInfo.isp_gain = *static_cast<float *>(pData);
    } else {
        gainInfo.isp_gain = 1.0f;
        MLOGE(Mia2LogGroupPlugin, "failed load isp_gain, force %f", gainInfo.isp_gain);
    }
    // override isp gain to 1.0f, for bps open demux module.
    float overrideISPGain = 1.0f;
    VendorMetadataParser::setVTagValue(pInputMeta, "com.qti.sensorbps.gain", &overrideISPGain, 1);

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pInputMeta, "com.qti.chi.statsaec.meterTargetExp", &pData);
    if (pData != nullptr) {
        gainInfo.tar_luma = reinterpret_cast<UINT64 *>(pData)[AECXExposureOutSafe];
    } else {
        gainInfo.tar_luma = 20.0f;
        MLOGE(Mia2LogGroupPlugin, "failed load targetluma, force %f", gainInfo.tar_luma);
    }

    MLOGI(Mia2LogGroupPlugin, "get metadata {current-luma: %f, target-luma: %f}", gainInfo.cur_luma,
          gainInfo.tar_luma);
    MLOGI(Mia2LogGroupPlugin, "get metadata {rtdg: %f, rtexp: %f, lux_index:%f adrc_gain:%f}",
          gainInfo.isp_gain, gainInfo.shutter_prev, gainInfo.lux_index, gainInfo.adrc_gain);

    pData = nullptr;
    AWBFrameControl *awbControl = nullptr;
    VendorMetadataParser::getVTagValue(pInputMeta, "org.quic.camera2.statsconfigs.AWBFrameControl",
                                       &pData);
    if (pData != nullptr) {
        awbControl = (AWBFrameControl *)pData;
    }
    if (awbControl != nullptr) {
        gainInfo.blue_gain = awbControl->AWBGains.bGain;
        gainInfo.red_gain = awbControl->AWBGains.rGain;
    } else {
        gainInfo.blue_gain = 1.0f;
        gainInfo.red_gain = 1.0f;
    }
    MLOGI(Mia2LogGroupPlugin, "get metadata {bgian: %f, rgain: %f}", gainInfo.blue_gain,
          gainInfo.red_gain);

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pInputMeta, "xiaomi.ai.misd.motionCaptureType", &pData);
    float factor = 1.0f;
    if (pData != nullptr) {
        UINT64 motionCaptureType = *static_cast<UINT64 *>(pData);
        factor = (float)((motionCaptureType >> 8) & 0xFF) / 10.0f;
        MLOGI(Mia2LogGroupPlugin, "factor:%f", factor);

        int type = motionCaptureType & 0xFF;
        MLOGI(Mia2LogGroupPlugin, "type:%d", type);

        if (type == MotionCaptureTypeValues::NightMotionFlicker ||
            type == MotionCaptureTypeValues::NightMotionHeavyFlicker) {
            banding = 1;
        }
    }

    UINT64 origExposureTime = (UINT64)((float)finalExposureTime * factor);
    float origLinearGain = finalLinearGain / factor;
    MLOGI(Mia2LogGroupPlugin, "fexpt:%llu fgain:%f factor:%f oexpt:%llu ogain:%f",
          finalExposureTime, finalLinearGain, factor, origExposureTime, origLinearGain);

    mExifGainInfo[m_processedFrameSize] += (", luidx:" + toStr(gainInfo.lux_index, 3));
    mExifGainInfo[m_processedFrameSize] += (", cluma:" + toStr(gainInfo.cur_luma, 3));
    mExifGainInfo[m_processedFrameSize] += (", tluma:" + toStr(gainInfo.tar_luma, 3));
    mExifGainInfo[m_processedFrameSize] += (", pexpt:" + toStr(gainInfo.shutter_prev, 1));
    mExifGainInfo[m_processedFrameSize] += (", cexpt:" + toStr(gainInfo.shutter_capt, 1));
    mExifGainInfo[m_processedFrameSize] += (", igain:" + toStr(gainInfo.isp_gain));

    mExifGainInfo[m_processedFrameSize] += (", fexpt:" + toStr(finalExposureTime));
    mExifGainInfo[m_processedFrameSize] += (", fgain:" + toStr(finalLinearGain, 3));
    mExifGainInfo[m_processedFrameSize] += (", factor:" + toStr(factor, 3));
    mExifGainInfo[m_processedFrameSize] += (", oexpt:" + toStr(origExposureTime));
    mExifGainInfo[m_processedFrameSize] += (", ogain:" + toStr(origLinearGain));
    MLOGI(Mia2LogGroupPlugin, "mExifGainInfo:%s", mExifGainInfo[m_processedFrameSize].c_str());
}

void MiAlgoSNSCPlugin::fillMotionInfo(camera_metadata_t *pInputMeta, float &motion)
{
    void *pData = nullptr;
    VendorMetadataParser::getVTagValue(pInputMeta, "xiaomi.ai.misd.motionVelocity", &pData);
    if (pData != nullptr) {
        MotionVelocity *pVelocity = static_cast<MotionVelocity *>(pData);

        MLOGI(Mia2LogGroupPlugin,
              "GET miviMotionVelocity(velocityStaticRatio:%f velocitySlowRatio:%f "
              "velocityMiddleRatio:%f velocityFastRatio:%f velocityMax:%f velocityMean:%f)",
              pVelocity->velocityStaticRatio, pVelocity->velocitySlowRatio,
              pVelocity->velocityMiddleRatio, pVelocity->velocityFastRatio, pVelocity->velocityMax,
              pVelocity->velocityMean);

        motion = pVelocity->velocityMax;

        // dump
        char fileName[128];
        memset(fileName, 0, sizeof(fileName));
        snprintf(fileName, sizeof(fileName), "/data/vendor/camera/%s_meta.xml", m_dumpPrefixs);
        XMLDocument xml;
        int res = xml.LoadFile(fileName);
        if (res == 0) {
            XMLElement *root = xml.RootElement();

            XMLElement *motionNode = xml.NewElement("Motion");
            motionNode->SetAttribute("index", m_processedFrameSize);
            root->InsertEndChild(motionNode);

            char maxString[128];
            memset(maxString, 0, sizeof(maxString));
            snprintf(maxString, sizeof(maxString), "%f", pVelocity->velocityMax);

            XMLElement *max = xml.NewElement("Max");
            XMLText *maxText = xml.NewText(maxString);
            max->InsertEndChild(maxText);
            motionNode->InsertEndChild(max);

            xml.SaveFile(fileName);
        } else {
            MLOGW(Mia2LogGroupPlugin, "load xml file failed");
        }
    } else {
        motion = 0.0f;
        MLOGE(Mia2LogGroupPlugin, "failed to get motion data");
    }

    if (m_processedFrameSize == 0) {
        mExifMotionInfo += (", motion{" + toStr(motion, 3));
    } else {
        mExifMotionInfo += ("," + toStr(motion, 3));
    }
    if (m_processedFrameSize == m_supportFrameSize - 1) {
        mExifMotionInfo += "}";
    }

    // write af info to exif
    pData = nullptr;
    int32_t lensPos = -1;
    VendorMetadataParser::getVTagValue(pInputMeta, "xiaomi.exifInfo.lenstargetposition", &pData);
    if (pData != nullptr) {
        lensPos = *static_cast<int32_t *>(pData);
    }

    if (m_processedFrameSize == 0) {
        mExifLPInfo += (", lenspos{" + toStr(lensPos));
    } else {
        mExifLPInfo += ("," + toStr(lensPos));
    }
    if (m_processedFrameSize == m_supportFrameSize - 1) {
        mExifLPInfo += "}";
    }
}

void MiAlgoSNSCPlugin::getDumpFileName(char *fileName, size_t size, const char *fileType,
                                       const char *timeBuf, int width, int height)
{
    char buf[128];
    memset(buf, 0, sizeof(buf));

    strncat(buf, "SNSC", sizeof(buf) - strlen(buf) - 1);

    snprintf(fileName, size, "/data/vendor/camera/IMG_%s_%s_%s_%dx%d", timeBuf, buf, fileType,
             width, height);
}

bool MiAlgoSNSCPlugin::dumpToFile(const char *fileName, struct MiImageBuffer *imageBuffer,
                                  uint32_t index, const char *fileType)
{
    if (imageBuffer == NULL)
        return -1;
    char buf[128];
    snprintf(buf, sizeof(buf), "%s", fileName);
    android::String8 filePath(buf);
    const char *suffix = "RGGB";

    snprintf(buf, sizeof(buf), "_%d.%s", index, suffix);

    MLOGE(Mia2LogGroupPlugin, "MiAlgoSNSCPlugin----->%s %s%s", __func__, filePath.string(), buf);

    filePath.append(buf);
    PluginUtils::miDumpBuffer(imageBuffer, filePath);
    return 0;
}
