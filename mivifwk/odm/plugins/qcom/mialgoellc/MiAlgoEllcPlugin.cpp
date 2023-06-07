#include "MiAlgoEllcPlugin.h"

#undef LOG_TAG
#define LOG_TAG                "MIA_ELLC"
#define FRAME_MAX_INPUT_NUMBER 8

using namespace mialgo2;

// Eg: adb shell setprop persist.vendor.algoengine.ellc.mode 0x1000  >>>  enable dump
// Eg: adb shell setprop persist.vendor.algoengine.ellc.mode 0x0100  >>>  bypass plugin
// Eg: adb shell setprop persist.vendor.algoengine.ellc.mode 0x0050  >>>  forcemode 5
// Eg: adb shell setprop persist.vendor.algoengine.ellc.mode 0x0004  >>>  set black level 1024 =
// 1020 + 4
union PROP {
    struct
    {
        uint32_t black : 4;  // 0x0000000F
        uint32_t mode : 4;   // 0x000000F0
        uint32_t bypass : 4; // 0x00000F00
        uint32_t dump : 1;   // 0x00001000
        uint32_t : 19;       // reserved
    } u;

    uint32_t data;
};
const static uint32_t s_prop = property_get_int32("persist.vendor.algoengine.ellc.mode", 0);

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

MiAlgoEllcPlugin::~MiAlgoEllcPlugin()
{
    MLOGI(Mia2LogGroupPlugin, "%p", this);
}

int MiAlgoEllcPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    MLOGI(Mia2LogGroupPlugin, "%p", this);
    mNodeInterface = nodeInterface;
    initMialgoAiEllcParas(createInfo);

    return 0;
}

void MiAlgoEllcPlugin::initMialgoAiEllcParas(CreateInfo *createInfo)
{
    setNextStat(PROCESSSTAT::INVALID);
    mp_ellc_handle = nullptr;
    m_ellc_version = MialgoAi_ellcGetVersion();
    MLOGI(Mia2LogGroupPlugin, "ellc version {%d.%d.%d.%d}", m_ellc_version.major,
          m_ellc_version.minor, m_ellc_version.patch, m_ellc_version.revision);

    m_ellc_burstinfo.num_frames = ELLC_SUPPORT_FRAMES;
    m_ellc_burstinfo.width = createInfo->inputFormat.width;
    m_ellc_burstinfo.height = createInfo->inputFormat.height;
    m_ellc_burstinfo.raw_type = IDEAL_RAW_14BIT; // createInfo->inputFormat.format;
    m_ellc_burstinfo.bayer_pattern = BAYER_BGGR; // createInfo->inputFormat.format;
    m_ellc_initparams.path_assets = "/vendor/etc/camera/ellc.bin";
    m_ellc_initparams.path_backend = "/vendor/lib64/libQnnHtpStub.so";

    m_ellc_cost = .0;
    m_frameIndex = 0;
    m_outFrameIdx = 0;
    m_outFrameTimeStamp = 0;
    m_forceMode = 0;
    m_needDump = 0;
    m_needBypass = 0;
    m_forceBlackLevel = 1029;
    m_flushDone = false;
    m_exitDone = false;
    parseProp();

    MLOGI(Mia2LogGroupPlugin, "ellc input size {%dx%d} num_frames %d. needDump: %d, needBypass: %d",
          m_ellc_burstinfo.width, m_ellc_burstinfo.height, m_ellc_burstinfo.num_frames, m_needDump,
          m_needBypass);
}

void MiAlgoEllcPlugin::parseProp()
{
    PROP prop;
    prop.data = s_prop;

    m_needDump = prop.u.dump;
    m_needBypass = prop.u.bypass;
    m_forceMode = prop.u.mode;
    m_forceBlackLevel = (prop.u.black > 0) ? (prop.u.black + 1020) : m_forceBlackLevel;

    MLOGI(Mia2LogGroupPlugin, "ellc prop 0x%x {dump: 0x%x, bypass: 0x%x, mode: 0x%x, black: 0x%x}",
          prop.data, prop.u.dump, prop.u.bypass, prop.u.mode, prop.u.black);
}

bool MiAlgoEllcPlugin::isEnabled(MiaParams settings)
{
    void *pData = nullptr;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.mivi.supernight.mode", &pData);
    m_process_mode = pData ? *static_cast<int *>(pData) : 0;
    MLOGI(Mia2LogGroupPlugin, "ellc xiaomi.mivi.supernight.mode %d", m_process_mode);

    pData = nullptr;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.ai.misd.StateScene", &pData);
    uint32_t is_tripod = 0;
    if (!pData) {
        MLOGE(Mia2LogGroupPlugin, "ellc xiaomi.ai.misd.StateScene: pData is null");
    }
    is_tripod = pData ? static_cast<AiMisdScene *>(pData)->value : 0;
    MLOGI(Mia2LogGroupPlugin, "ellc xiaomi.ai.misd.StateScene %d", is_tripod);

    if (is_tripod) {
        if (m_process_mode == ELLCMODE_HANDLE) {
            m_process_mode = ELLCMODE_TRIPOD;
        } else if (m_process_mode == ELLCMODE_HANDLE_LONG) {
            m_process_mode = ELLCMODE_TRIPOD_LONG;
        }
    }

    m_process_mode = (m_forceMode > 0) ? m_forceMode : m_process_mode;
    updateEllcMode();

    bool is_ellc_enable =
        ((m_process_mode == ELLCMODE_HANDLE) || (m_process_mode == ELLCMODE_TRIPOD) ||
         (m_process_mode == ELLCMODE_HANDLE_LONG) || (m_process_mode == ELLCMODE_TRIPOD_LONG));

    return is_ellc_enable;
}

void MiAlgoEllcPlugin::updateEllcMode()
{
    switch (m_process_mode) {
    case ELLCMODE_HANDLE: // handle ellc
        m_ellc_mode = GENERAL;
        break;
    case ELLCMODE_TRIPOD: // tripod ellc
        m_ellc_mode = TRIPOD;
        break;
    case ELLCMODE_HANDLE_LONG: // long handle ellc
        m_ellc_mode = GENERAL;
        break;
    case ELLCMODE_TRIPOD_LONG: // long tripod ellc
        m_ellc_mode = TRIPOD;
        break;
    default: // default
        m_ellc_mode = GENERAL;
        break;
    }
    // m_ellc_initparams.mode = TRIPOD;
    m_ellc_initparams.mode = m_ellc_mode;
    MLOGI(Mia2LogGroupPlugin, "ellc mode %s", [&]() -> const char * {
        switch (m_ellc_mode) {
        case TRIPOD:
            return "TRIPOD";
        case GENERAL:
            return "GENERAL";
        }
    }());
}

static const uint32_t GYRO_SAMPLES_BUF_SIZE = 512;
ProcessRetStatus MiAlgoEllcPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    m_ellc_ret = OK;

    struct MiImageBuffer inputFrame[FRAME_MAX_INPUT_NUMBER];
    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;
    uint32_t inputNum = inputBuffers.size();
    uint32_t outputNum = outputBuffers.size();
    if (inputNum > FRAME_MAX_INPUT_NUMBER) {
        inputNum = FRAME_MAX_INPUT_NUMBER;
    }
    MLOGI(Mia2LogGroupPlugin, "ellc input frame nums %d", inputNum);

    auto override_buffer = [&](const char *str, MiImageBuffer &mFrame, const ImageParams &rFrame) {
        mFrame.format = rFrame.format.format;
        mFrame.width = rFrame.format.width;
        mFrame.height = rFrame.format.height;
        mFrame.plane[0] = rFrame.pAddr[0];
        mFrame.fd[0] = rFrame.fd[0];
        mFrame.stride = rFrame.planeStride;
        mFrame.scanline = rFrame.sliceheight;
        mFrame.numberOfPlanes = rFrame.numberOfPlanes;
        MLOGI(Mia2LogGroupPlugin,
              "%s buffer info {handle: %p, fd: %d, width: %d, height: %d, stride: %d, format: %d}",
              str, mFrame.plane[0], mFrame.fd[0], mFrame.width, mFrame.height, mFrame.stride,
              mFrame.format);
    };

    if ((processRequestInfo->isFirstFrame && (outputNum > 0)) ||
        (inputNum == m_ellc_burstinfo.num_frames)) {
        m_flushDone = false;
        m_exitDone = false;
        m_frameIndex = 0;
        m_outFrameIdx = processRequestInfo->frameNum;
        m_outFrameTimeStamp = processRequestInfo->timeStamp;
        setNextStat(PROCESSSTAT::PROCESSINIT);
        override_buffer("ouput", m_outputFrame, outputBuffers[0]);
        mp_outputMeta = (outputBuffers[0].pPhyCamMetadata == NULL)
                            ? outputBuffers[0].pMetadata
                            : outputBuffers[0].pPhyCamMetadata;
    }

    // Notice: Set flushDone at the end of flushProcess
    // Case 1: If flushDone is setted and the next request start,
    // the latter request will be cancaled here.
    // Case 2: If current request is the first and flushDone is setted without reset,
    // the state will be changed to INVALID, but flushDone is true still.
    // Case 3: If current request is the first and flushDone is reset by the upper code,
    // the request will not cancel soon, but state is EXIT still.
    if (m_flushDone) {
        MLOGI(Mia2LogGroupPlugin, "fast exit as flush done settted.");
        // reset state
        setNextStat(PROCESSSTAT::INVALID);
        return resultInfo;
    }

    for (int i = 0; i < inputNum; i++) {
        override_buffer("input", m_inputFrame, inputBuffers[i]);
        mp_inputMeta = (inputBuffers[i].pPhyCamMetadata == NULL) ? inputBuffers[i].pMetadata
                                                                 : inputBuffers[i].pPhyCamMetadata;
        m_ellc_burstinfo.stride = m_inputFrame.stride;
        m_ellc_burstinfo.width = m_inputFrame.width;
        m_ellc_burstinfo.height = m_inputFrame.height;

        if (m_needBypass ||
            !matchSize(m_ellc_burstinfo.width, m_ellc_burstinfo.height, m_ellc_burstinfo.stride) ||
            !matchFormat(m_inputFrame.format)) {
            setNextStat(PROCESSSTAT::PROCESSERROR);
            MLOGI(Mia2LogGroupPlugin,
                  "skip process by check {bypass: %d, size: %ux%u, format: %u}, goto PROCESSERROR",
                  m_needBypass, m_ellc_burstinfo.width, m_ellc_burstinfo.height,
                  m_inputFrame.format);
        }

        if (processRequest(m_frameIndex++) == PROCESSSTAT::EXIT) {
            break;
        }
    }

    MLOGI(Mia2LogGroupPlugin, "process index[%d] done", m_frameIndex - 1);
    return resultInfo;
}

PROCESSSTAT MiAlgoEllcPlugin::processRequest(const int &index)
{
    const auto statLog = [=](const int &index, const PROCESSSTAT &process_stat) {
        MialgoAi_EllcStatus process_ret = m_ellc_ret;
        MLOGI(Mia2LogGroupPlugin, "processing index[%d] stat %s next %s, ret %s", index,
              statStr(process_stat).c_str(), statStr(getCurrentStat()).c_str(),
              statStr(process_ret).c_str());
    };

    do {
        switch (getCurrentStat()) {
        case PROCESSSTAT::PROCESSINIT:
            m_ellc_ret = processInit(m_inputFrame);
            statLog(index, PROCESSSTAT::PROCESSINIT);
            break;

        case PROCESSSTAT::PROCESSPRE:
            m_ellc_ret = processBuffer(m_inputFrame, index);
            statLog(index, PROCESSSTAT::PROCESSPRE);
            break;

        case PROCESSSTAT::PROCESSRUN:
            m_ellc_ret = processAlgo(m_outputFrame);
            statLog(index, PROCESSSTAT::PROCESSRUN);
            break;

        case PROCESSSTAT::PROCESSUINIT:
            m_ellc_ret = processUinit();
            statLog(index, PROCESSSTAT::PROCESSUINIT);
            break;

        case PROCESSSTAT::PROCESSERROR:
            m_ellc_ret = processError();
            statLog(index, PROCESSSTAT::PROCESSERROR);
            break;

        case PROCESSSTAT::PROCESSFLUSH:
            m_ellc_ret = processFlush();
            statLog(index, PROCESSSTAT::PROCESSFLUSH);
            break;

        case PROCESSSTAT::PROCESSEXIT:
            m_ellc_ret = processExit();
            statLog(index, PROCESSSTAT::PROCESSEXIT);
            break;

        case PROCESSSTAT::PROCESSPREWAIT:
            setNextStat(PROCESSSTAT::PROCESSPRE);
            statLog(index, PROCESSSTAT::PROCESSPREWAIT);
            break;

        case PROCESSSTAT::EXIT:
            MLOGI(Mia2LogGroupPlugin, "processing index[%d] stat %s", index,
                  statStr(getCurrentStat()).c_str());
            break;

        default:
            setNextStat(PROCESSSTAT::PROCESSERROR);
            statLog(index, PROCESSSTAT::INVALID);
            break;
        }
    } while ((PROCESSSTAT::PROCESSUINIT == getCurrentStat()) ||
             (PROCESSSTAT::PROCESSRUN == getCurrentStat()) ||
             (PROCESSSTAT::PROCESSPRE == getCurrentStat()) ||
             (PROCESSSTAT::PROCESSERROR == getCurrentStat()) ||
             (PROCESSSTAT::PROCESSFLUSH == getCurrentStat()) ||
             (PROCESSSTAT::PROCESSEXIT == getCurrentStat()));

    return getCurrentStat();
}

MialgoAi_EllcStatus MiAlgoEllcPlugin::processInit(MiImageBuffer &frame)
{
    // terminate is able to called parallel
    // but terminate/init/uinit need called squnsense
    std::lock_guard<std::mutex> lock_tmd(m_terminate_lock);
    // only one process called onetime
    std::lock_guard<std::mutex> lock(m_process_lock);
    if (checkSuccess() && getCurrentStat() == PROCESSSTAT::PROCESSINIT) {
        getInitInfo(mp_inputMeta);
        m_ellc_initparams.raw_burst_info = m_ellc_burstinfo;

        MialgoAi_ellc_SetLogLevel(1, printAndroidAndOfflineLog);
        MLOGI(Mia2LogGroupPlugin, "set MIALGO_ELLC offlinelog enable");
        double cost = .0;
        std::tie(cost, m_ellc_ret) =
            costTimeMs(MialgoAi_ellcInit, mp_ellc_handle, m_ellc_initparams);
        MLOGI(Mia2LogGroupPlugin, "PROCESSINIT cost %lfms, ellc handle %p", cost, mp_ellc_handle);
        setNextStat(PROCESSSTAT::PROCESSPRE);
    } else {
        setNextStat(PROCESSSTAT::PROCESSERROR);
    }

    return m_ellc_ret;
}

MialgoAi_EllcStatus MiAlgoEllcPlugin::processBuffer(MiImageBuffer &frame, const int &index)
{
    // only one process called onetime
    std::lock_guard<std::mutex> lock(m_process_lock);
    if (checkSuccess() && getCurrentStat() == PROCESSSTAT::PROCESSPRE) {
        getPrepInfo(mp_inputMeta);
        m_ellc_gaininfo.shutter_capt = (float)m_ellc_frametime.exp_dur;
        m_ellc_preprocessparams.gyro_data = m_ellc_gyrodata;
        m_ellc_preprocessparams.raw_data = frame.plane[0];
        m_ellc_preprocessparams.fd = frame.fd[0];
        m_ellc_preprocessparams.gain_info = m_ellc_gaininfo;

        if (m_needDump) {
            char inputbuf[256];
#ifdef THOR_CAM
            snprintf(
                inputbuf, sizeof(inputbuf),
                "mialgo_ellc_input[%d]_%dx%d_cluma[%f]_tluma[%f]_rtexp[%f]_capexp[%f]_rtispdg[%"
                "f]_cispdg[%f]_ctotalg[%f]_luxidx[%f]",
                index, m_inputFrame.width, m_inputFrame.height, m_ellc_gaininfo.cur_luma,
                m_ellc_gaininfo.tar_luma, m_ellc_gaininfo.shutter_prev,
                m_ellc_gaininfo.shutter_capt, m_ellc_gaininfo.isp_gain_prev,
                m_ellc_gaininfo.isp_gain_capt, m_ellc_gaininfo.total_gain_capt,
                m_ellc_gaininfo.lux_index);
#else
            snprintf(inputbuf, sizeof(inputbuf),
                     "mialgo_ellc_input[%d]_%dx%d_cluma[%f]_tluma[%f]_rtexp[%f]_capexp[%f]_ispdg[%"
                     "f]_luxidx[%f]",
                     index, m_inputFrame.width, m_inputFrame.height, m_ellc_gaininfo.cur_luma,
                     m_ellc_gaininfo.tar_luma, m_ellc_gaininfo.shutter_prev,
                     m_ellc_gaininfo.shutter_capt, m_ellc_gaininfo.isp_gain,
                     m_ellc_gaininfo.lux_index);
#endif
            PluginUtils::dumpToFile(inputbuf, &m_inputFrame);
        }

        double cost = .0;
        std::tie(cost, m_ellc_ret) =
            costTimeMs(MialgoAi_ellcPreprocess, mp_ellc_handle, m_ellc_preprocessparams);
        MLOGI(Mia2LogGroupPlugin, "PROCESSPRE cost %lfms, ellc handle %p", cost, mp_ellc_handle);
        setNextStat(PROCESSSTAT::PROCESSPREWAIT);
        if (index == (m_ellc_burstinfo.num_frames - 1)) {
            setNextStat(PROCESSSTAT::PROCESSRUN);
        }
    } else {
        setNextStat(PROCESSSTAT::PROCESSERROR);
    }

    return m_ellc_ret;
}

MialgoAi_EllcStatus MiAlgoEllcPlugin::processAlgo(MiImageBuffer &frame)
{
    // only one process called onetime
    std::lock_guard<std::mutex> lock(m_process_lock);
    if (checkSuccess() && getCurrentStat() == PROCESSSTAT::PROCESSRUN) {
        m_ellc_runparams.raw_data = frame.plane[0];
        m_ellc_runparams.fd = frame.fd[0];
        m_ellc_cost = .0;
        std::tie(m_ellc_cost, m_ellc_ret) =
            costTimeMs(MialgoAi_ellcRun, mp_ellc_handle, m_ellc_runparams);
        MLOGI(Mia2LogGroupPlugin, "PROCESSRUN cost %lfms, out buffer handle %p, ellc handle %p",
              m_ellc_cost, m_ellc_runparams.raw_data, mp_ellc_handle);
        setNextStat(PROCESSSTAT::PROCESSUINIT);
    } else {
        setNextStat(PROCESSSTAT::PROCESSERROR);
    }

    return m_ellc_ret;
}

MialgoAi_EllcStatus MiAlgoEllcPlugin::processUinit()
{
    // terminate is able to called parallel
    // but terminate/init/uinit need called squnsense
    std::lock_guard<std::mutex> lock_tmd(m_terminate_lock);
    // only one process called onetime
    std::lock_guard<std::mutex> lock(m_process_lock);
    if (mp_ellc_handle && getCurrentStat() == PROCESSSTAT::PROCESSUINIT) {
        double cost = .0;
        std::tie(cost, m_ellc_ret) =
            costTimeMs(MialgoAi_ellcGetExecLog, mp_ellc_handle, m_ellc_execlog);
        MLOGI(Mia2LogGroupPlugin, "ellcGetExecLog cost %lfms, ellc handle %p", cost,
              mp_ellc_handle);
        std::tie(cost, m_ellc_ret) = costTimeMs(MialgoAi_ellcUnit, mp_ellc_handle);
        MLOGI(Mia2LogGroupPlugin, "PROCESSUINIT unit cost %lfms, ellc handle %p", cost,
              mp_ellc_handle);
    } else {
        MLOGE(Mia2LogGroupPlugin, "free null handle");
    }
    setNextStat(PROCESSSTAT::PROCESSEXIT, true);
    return m_ellc_ret;
}

MialgoAi_EllcStatus MiAlgoEllcPlugin::processError()
{
    // only one process called onetime
    std::lock_guard<std::mutex> lock(m_process_lock);
    if (getCurrentStat() == PROCESSSTAT::PROCESSERROR) {
        double cost = .0;
        std::tie(cost, std::ignore) =
            costTimeMs(PluginUtils::miCopyBuffer, &m_outputFrame, &m_inputFrame);
        MLOGI(Mia2LogGroupPlugin, "PROCESSERROR cost %lfms", cost);
        if (m_needBypass ||
            !matchSize(m_ellc_burstinfo.width, m_ellc_burstinfo.height, m_ellc_burstinfo.stride) ||
            !matchFormat(m_inputFrame.format)) {
            if (FRAME_MAX_INPUT_NUMBER == m_frameIndex) {
                setNextStat(PROCESSSTAT::PROCESSUINIT);
            } else {
                setNextStat(PROCESSSTAT::PROCESSBUFWAIT);
            }
        } else {
            setNextStat(PROCESSSTAT::PROCESSUINIT);
        }
    }

    return m_ellc_ret;
}

MialgoAi_EllcStatus MiAlgoEllcPlugin::processFlush()
{
    // terminate is able to called parallel
    // but terminate/init/uinit need called squnsense
    std::lock_guard<std::mutex> lock_tmd(m_terminate_lock);
    if (getCurrentStat() == PROCESSSTAT::PROCESSFLUSH) {
        double cost = .0;
        MLOGI(Mia2LogGroupPlugin, "processing stat %s ", statStr(getCurrentStat()).c_str());

        // terminate will not changed/free the handle
        if (mp_ellc_handle) {
            std::tie(cost, m_ellc_ret) = costTimeMs(MialgoAi_ellcTerminate, mp_ellc_handle, 500);
            MLOGI(Mia2LogGroupPlugin, "PROCESSFLUSH cost %lfms", cost);
            setNextStat(PROCESSSTAT::PROCESSUINIT, true);
        } else {
            MLOGI(Mia2LogGroupPlugin, "PROCESSFLUSH nothing to free");
            setNextStat(PROCESSSTAT::PROCESSEXIT, true);
        }
    }
    return m_ellc_ret;
}

MialgoAi_EllcStatus MiAlgoEllcPlugin::processExit()
{
    // terminate is able to called parallel
    // but terminate/init/uinit need called squnsense
    // the process stat after terminate need to be Protected
    // as that will process repeatly if not
    std::lock_guard<std::mutex> lock_tmd(m_terminate_lock);
    // only one process called onetime
    std::lock_guard<std::mutex> lock(m_process_lock);
    if (getCurrentStat() == PROCESSSTAT::PROCESSEXIT) {
        setNextStat(PROCESSSTAT::EXIT, true);
        MLOGI(Mia2LogGroupPlugin, "m_exitDone %d", m_exitDone);
        if (m_exitDone) {
            return m_ellc_ret;
        }

        setMetaInfo();
        if (m_needDump && m_outputFrame.plane[0]) {
            char outputbuf[128];
            snprintf(outputbuf, sizeof(outputbuf), "mialgo_ellc_output_%dx%d", m_outputFrame.width,
                     m_outputFrame.height);
            PluginUtils::dumpToFile(outputbuf, &m_outputFrame);
        }
        MialgoAi_ellc_SetLogLevel(0, printAndroidAndOfflineLog);
        MLOGI(Mia2LogGroupPlugin, "set MIALGO_ELLC offlinelog disable");
        // HardCode: merge 8th meta to 1st meta
        // Populate debugdata all meta
        if (mp_inputMeta != NULL && mp_outputMeta != NULL) {
            MLOGI(Mia2LogGroupPlugin, "merge inputMeta and outputMeta");
            VendorMetadataParser::mergeMetadata(mp_outputMeta, mp_inputMeta);
        }
        // clear meta point
        mp_inputMeta = nullptr;
        mp_outputMeta = nullptr;
        m_exitDone = true;
    }

    return m_ellc_ret;
}

void MiAlgoEllcPlugin::getGyroInterval(camera_metadata_t *pMetaData,
                                       MialgoAi_EllcFrameTime &frametime)
{
    if (nullptr == pMetaData) {
        return;
    }

    void *pData;
    CHITIMESTAMPINFO timestampInfo = {0};
    uint64_t frameDuration = 0;
    uint64_t frameExposureTime = 0;
    uint64_t frameRollingShutterSkew = 0;

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pMetaData, "org.quic.camera.qtimer.timestamp", &pData);
    if (pData) {
        timestampInfo = *static_cast<CHITIMESTAMPINFO *>(pData);
    } else {
        MLOGE(Mia2LogGroupPlugin, "get metadata error 'org.quic.camera.qtimer.timestamp'");
    }

    pData = nullptr;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_SENSOR_FRAME_DURATION, &pData);
    if (pData) {
        frameDuration = *static_cast<int64_t *>(pData);
    } else {
        MLOGE(Mia2LogGroupPlugin, "get metadata error 'ANDROID_SENSOR_FRAME_DURATION'");
    }

    pData = nullptr;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_SENSOR_EXPOSURE_TIME, &pData);
    if (pData) {
        frameExposureTime = *static_cast<int64_t *>(pData);
    } else {
        MLOGE(Mia2LogGroupPlugin, "get metadata error 'ANDROID_SENSOR_EXPOSURE_TIME'");
    }

    pData = nullptr;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_SENSOR_ROLLING_SHUTTER_SKEW, &pData);
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

    frametime.exp_beg = sof - frameExposureTime;
    frametime.exp_end = sof + frameRollingShutterSkew;
    frametime.exp_dur = frameExposureTime;

    MLOGI(Mia2LogGroupPlugin, "ellc frame time {beigin:%lu, end:%lu, dur:%lu}", frametime.exp_beg,
          frametime.exp_end, frametime.exp_dur);
}

void MiAlgoEllcPlugin::fillGyroData(camera_metadata_t *pMetaData, MialgoAi_EllcGyroElement *data,
                                    MialgoAi_EllcFrameTime &frametime, uint32_t &count)
{
    if (nullptr == pMetaData) {
        return;
    }

    getGyroInterval(pMetaData, frametime);
    void *pData;

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.mivi.ellc.gyro", &pData);
    if (pData != nullptr) {
        ChiFrameGyro *gyro = static_cast<ChiFrameGyro *>(pData);
        count = gyro->gcount;

        if ((0 < count) && (count <= MAX_SAMPLES_BUF_SIZE)) {
            frametime.exp_beg = NanoToMicro(QtimerTicksToQtimerNano(gyro->ts));
            frametime.exp_end = NanoToMicro(QtimerTicksToQtimerNano(gyro->te));

            for (uint32_t i = 0; i < count; i++) {
                data[i].shift_x = gyro->x[i];
                data[i].shift_y = gyro->y[i];
                data[i].shift_z = gyro->z[i];
                data[i].timestamp = NanoToMicro(QtimerTicksToQtimerNano(gyro->t[i]));
            }

            //[begin, end)
            std::sort(data, data + count,
                      [](const MialgoAi_EllcGyroElement &d1, const MialgoAi_EllcGyroElement &d2) {
                          return d1.timestamp < d2.timestamp;
                      });

            // if count > 0 , check data range
            bool frame_ok = (frametime.exp_beg < frametime.exp_end);
            bool gyro_ok = (data[0].timestamp < data[count - 1].timestamp);
            bool start_ok = (frametime.exp_beg > data[0].timestamp) ||
                            (data[0].timestamp < (frametime.exp_beg + SMAPLE_TIME));
            bool end_ok = (frametime.exp_end < data[count - 1].timestamp) ||
                          (frametime.exp_end < (data[count - 1].timestamp + SMAPLE_TIME));
            bool range_ok = frame_ok && gyro_ok && start_ok && end_ok;

            MLOGI(Mia2LogGroupPlugin,
                  "frame start from %lu to %lu, gyro data start from %lu to %lu, range ok (%d, %d, "
                  "%d, "
                  "%d, %d)",
                  frametime.exp_beg, frametime.exp_end, data[0].timestamp,
                  data[count - 1].timestamp, frame_ok, gyro_ok, start_ok, end_ok, range_ok);
        } else {
            count = 200;
            memset(data, 0, sizeof(MialgoAi_EllcGyroElement) * MAX_SAMPLES_BUF_SIZE);
            MLOGE(Mia2LogGroupPlugin, "gyro data count zero");
        }
    } else {
        count = 200;
        memset(data, 0, sizeof(MialgoAi_EllcGyroElement) * MAX_SAMPLES_BUF_SIZE);
        MLOGE(Mia2LogGroupPlugin, "get gyro meta falied");
    }
    MLOGI(Mia2LogGroupPlugin,
          "ellc frame time after gyro {beigin:%lu, end:%lu, dur:%lu, gyrocount: %u}",
          frametime.exp_beg, frametime.exp_end, frametime.exp_dur, count);
}

void MiAlgoEllcPlugin::getInitInfo(camera_metadata_t *pMetaData)
{
    if (nullptr == pMetaData) {
        return;
    }
    void *pData;

    pData = nullptr;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_SENSOR_DYNAMIC_BLACK_LEVEL, &pData);
    if (pData != nullptr) {
        float black_level[4] = {0};
        memcpy(black_level, static_cast<float *>(pData), 4 * sizeof(float));
        m_ellc_burstinfo.black_level[0] = (int32_t)black_level[0];
        m_ellc_burstinfo.black_level[1] = (int32_t)black_level[1];
        m_ellc_burstinfo.black_level[2] = (int32_t)black_level[2];
        m_ellc_burstinfo.black_level[3] = (int32_t)black_level[3];
    } else {
        m_ellc_burstinfo.black_level[0] = 1029;
        m_ellc_burstinfo.black_level[1] = 1029;
        m_ellc_burstinfo.black_level[2] = 1029;
        m_ellc_burstinfo.black_level[3] = 1029;
        MLOGE(Mia2LogGroupPlugin, "failed load black level, force %d",
              m_ellc_burstinfo.black_level[0]);
    }

    pData = nullptr;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_SENSOR_DYNAMIC_WHITE_LEVEL, &pData);
    if (pData != nullptr) {
        m_ellc_burstinfo.white_level = *static_cast<int32_t *>(pData);
    } else {
        m_ellc_burstinfo.white_level = 16384;
        MLOGE(Mia2LogGroupPlugin, "failed load white level, force %d",
              m_ellc_burstinfo.white_level);
    }

    pData = nullptr;
    float luxidx = .0;
    VendorMetadataParser::getVTagValue(pMetaData, "org.quic.camera2.statsconfigs.AECFrameControl",
                                       &pData);
    if (pData != nullptr) {
        luxidx = static_cast<AECFrameControl *>(pData)->luxIndex;
    }

    //读到的值不对, 先强制设, 要的是14bit
    // m_ellc_burstinfo.black_level =
    //     (luxidx > 530.0) ? (m_forceBlackLevel + 2)
    //                      : (luxidx > 520.0) ? (m_forceBlackLevel + 1) : m_forceBlackLevel;
    // if (m_ellc_mode == TRIPOD) {
    //     m_ellc_burstinfo.black_level = m_forceBlackLevel;
    // }
    m_ellc_burstinfo.black_level[0] = m_forceBlackLevel;
    m_ellc_burstinfo.black_level[1] = m_ellc_burstinfo.black_level[0];
    m_ellc_burstinfo.black_level[2] = m_ellc_burstinfo.black_level[0];
    m_ellc_burstinfo.black_level[3] = m_ellc_burstinfo.black_level[0];

    m_ellc_burstinfo.white_level = 16383;
    MLOGI(Mia2LogGroupPlugin, "get metadata {luxidx: %f, black-level: %d, white-level: %d}", luxidx,
          m_ellc_burstinfo.black_level[0], m_ellc_burstinfo.white_level);

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pMetaData, "org.quic.camera2.tuning.mode.TuningMode",
                                       &pData);
    if (pData != nullptr) {
        memcpy(&m_tuningParam, pData, sizeof(ChiTuningModeParameter));
    }
}

void MiAlgoEllcPlugin::getPrepInfo(camera_metadata_t *pMetaData)
{
    if (nullptr == pMetaData) {
        return;
    }
    void *pData;

    // get gyro data
    uint32_t gyro_count = 0;
    fillGyroData(pMetaData, m_ellc_gyrosample, m_ellc_frametime, gyro_count);
    m_ellc_gyrodata.count = gyro_count;
    m_ellc_gyrodata.gyro_eles = m_ellc_gyrosample;
    m_ellc_gyrodata.frame_time = m_ellc_frametime;

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.mivi.ellc.frameluma", &pData);
    if (pData != nullptr) {
        m_ellc_gaininfo.cur_luma = *static_cast<float *>(pData);
    } else {
        m_ellc_gaininfo.cur_luma = 20.0;
        MLOGE(Mia2LogGroupPlugin, "failed load frameluma, force %f", m_ellc_gaininfo.cur_luma);
    }

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.mivi.ellc.targetluma", &pData);
    if (pData != nullptr) {
        m_ellc_gaininfo.tar_luma = *static_cast<float *>(pData);
    } else {
        m_ellc_gaininfo.tar_luma = 20.0;
        MLOGE(Mia2LogGroupPlugin, "failed load targetluma, force %f", m_ellc_gaininfo.tar_luma);
    }
    MLOGI(Mia2LogGroupPlugin, "get metadata {current-luma: %f, target-luma: %f}",
          m_ellc_gaininfo.cur_luma, m_ellc_gaininfo.tar_luma);

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.mivi.ellc.rtisp", &pData);
#ifdef THOR_CAM
    if (pData != nullptr) {
        m_ellc_gaininfo.isp_gain_prev = *static_cast<float *>(pData);
    } else {
        m_ellc_gaininfo.isp_gain_prev = 1.0f;
        MLOGE(Mia2LogGroupPlugin, "failed load isp_gain_prev, force %f",
              m_ellc_gaininfo.isp_gain_prev);
    }

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pMetaData, "com.qti.sensorbps.gain", &pData);
    if (pData != nullptr) {
        m_ellc_gaininfo.isp_gain_capt = *static_cast<float *>(pData);
    } else {
        m_ellc_gaininfo.isp_gain_capt = 1.0f;
        MLOGE(Mia2LogGroupPlugin, "failed load isp_gain_capt, force %f",
              m_ellc_gaininfo.isp_gain_capt);
    }
#else
    if (pData != nullptr) {
        m_ellc_gaininfo.isp_gain = *static_cast<float *>(pData);
    } else {
        m_ellc_gaininfo.isp_gain = 1.0f;
        MLOGE(Mia2LogGroupPlugin, "failed load isp_gain, force %f", m_ellc_gaininfo.isp_gain);
    }

#endif

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.mivi.ellc.rtexp", &pData);
    if (pData != nullptr) {
        m_ellc_gaininfo.shutter_prev = NanoToMicro(*static_cast<float *>(pData));
    } else {
        m_ellc_gaininfo.shutter_prev = 30000.0f;
        MLOGE(Mia2LogGroupPlugin, "failed load shutter_prev, force %f",
              m_ellc_gaininfo.shutter_prev);
    }

#ifdef THOR_CAM
    MLOGI(Mia2LogGroupPlugin, "get metadata {rtdg: %f, rtexp: %f, cptdg: %f}",
          m_ellc_gaininfo.isp_gain_prev, m_ellc_gaininfo.shutter_prev,
          m_ellc_gaininfo.isp_gain_capt);
#else
    MLOGI(Mia2LogGroupPlugin, "get metadata {rtdg: %f, rtexp: %f}", m_ellc_gaininfo.isp_gain,
          m_ellc_gaininfo.shutter_prev);
#endif

    pData = nullptr;
    VendorMetadataParser::getVTagValue(pMetaData, "org.quic.camera2.statsconfigs.AECFrameControl",
                                       &pData);
    if (pData != nullptr) {
        m_ellc_gaininfo.lux_index = static_cast<AECFrameControl *>(pData)->luxIndex;
#ifdef THOR_CAM
        m_ellc_gaininfo.total_gain_capt =
            static_cast<AECFrameControl *>(pData)->exposureInfo[ExposureIndexSafe].linearGain;
        MLOGI(Mia2LogGroupPlugin, "get metadata {total_gain_capt: %f}",
              m_ellc_gaininfo.total_gain_capt);
#endif
    }

    pData = nullptr;
    int32_t perFrameEv;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &pData);
    if (pData != nullptr) {
        perFrameEv = *static_cast<int32_t *>(pData);
    }
    MLOGI(Mia2LogGroupPlugin, "get metadata {luxIndex: %f, EV: %d}", m_ellc_gaininfo.lux_index,
          perFrameEv);
}

void MiAlgoEllcPlugin::setExifInfo()
{
    auto version = [&]() {
        return toStr(m_ellc_version.major) + "." + toStr(m_ellc_version.minor) + "." +
               toStr(m_ellc_version.patch) + "." + toStr(m_ellc_version.revision);
    };
    uint32_t frame_index = m_frameIndex;

    std::string ellc_exif_info = "ellc:{on:1";
    ellc_exif_info += (", version:" + version());                            // algo version
    ellc_exif_info += (", luidx:" + toStr(m_ellc_gaininfo.lux_index, 3));    // capture lux index
    ellc_exif_info += (", cluma:" + toStr(m_ellc_gaininfo.cur_luma, 3));     // current luma
    ellc_exif_info += (", tluma:" + toStr(m_ellc_gaininfo.tar_luma, 3));     // target luma
    ellc_exif_info += (", pexpt:" + toStr(m_ellc_gaininfo.shutter_prev, 1)); // preview exp time
    ellc_exif_info += (", cexpt:" + toStr(m_ellc_gaininfo.shutter_capt, 1)); // capture exp time
#ifdef THOR_CAM
    ellc_exif_info += (", pigain:" + toStr(m_ellc_gaininfo.isp_gain_prev));   // preivew isp gain
    ellc_exif_info += (", cigain:" + toStr(m_ellc_gaininfo.isp_gain_capt));   // capture isp gain
    ellc_exif_info += (", ctgain:" + toStr(m_ellc_gaininfo.total_gain_capt)); // capture total gain
#else
    ellc_exif_info += (", igain:" + toStr(m_ellc_gaininfo.isp_gain)); // preivew isp gain
#endif
    ellc_exif_info += (", blevl:" + toStr(m_ellc_burstinfo.black_level[0])); // black level
    ellc_exif_info += (", wlevl:" + toStr(m_ellc_burstinfo.white_level));    // white level
    MialgoAi_EllcStatus alert = m_ellc_ret;
    ellc_exif_info += (", alret:" + toStr(alert));          // algo return status
    ellc_exif_info += (", fmidx:" + toStr(frame_index));    // frame index
    ellc_exif_info += (", almod:" + toStr(m_ellc_mode));    // algo mode
    ellc_exif_info += (", acost:" + toStr(m_ellc_cost, 2)); // algo cost
    ellc_exif_info +=
        (", Exec:{ratio:" + toStr(m_ellc_execlog.ratio) +                 // algo exec ratio
         ", output_type:" + toStr(m_ellc_execlog.output_type) +           // algo exec output_type
         ", frame_abandon:" + toStr(m_ellc_execlog.frame_abandon) + "}"); // algo exec frame_abandon
    ellc_exif_info +=
        (", tning:{DE:" + toStr(m_tuningParam.TuningMode[0].subMode.value) + // default
         ", SM:" + toStr(m_tuningParam.TuningMode[1].subMode.value) +        // sensor mdoe
         ", UC:" + toStr(m_tuningParam.TuningMode[2].subMode.usecase) +      // usecase
         ", F1:" + toStr(m_tuningParam.TuningMode[3].subMode.feature1) +     // feature 1
         ", F2:" + toStr(m_tuningParam.TuningMode[4].subMode.feature2) +     // feature 2
         ", SC:" + toStr(m_tuningParam.TuningMode[5].subMode.scene) +        // scene
         ", EF:" + toStr(m_tuningParam.TuningMode[6].subMode.effect) + "}"); // effect
    ellc_exif_info += "}";

    MLOGI(Mia2LogGroupPlugin, "update ellc exif info: %s", ellc_exif_info.c_str());
    if (mNodeInterface.pSetResultMetadata != NULL) {
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, m_outFrameIdx, m_outFrameTimeStamp,
                                          ellc_exif_info);
    }
}

void MiAlgoEllcPlugin::setMetaInfo()
{
    setExifInfo();
}

ProcessRetStatus MiAlgoEllcPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int MiAlgoEllcPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    MLOGI(Mia2LogGroupPlugin, "%p", this);
    {
        // protect stats, ensure uinit will be called
        std::lock_guard<std::mutex> lock_tmd(m_terminate_lock);
        setNextStat(PROCESSSTAT::PROCESSFLUSH, true);
    }
    processRequest(m_frameIndex);
    m_flushDone = true;
    return 0;
}

void MiAlgoEllcPlugin::reset()
{
    setNextStat(PROCESSSTAT::INVALID);
    m_ellc_cost = .0;
    m_frameIndex = 0;
    m_outFrameIdx = 0;
    m_outFrameTimeStamp = 0;
    m_flushDone = false;
    m_exitDone = false;
}

void MiAlgoEllcPlugin::destroy()
{
    MLOGI(Mia2LogGroupPlugin, "%p", this);
    // as mivi promise destroy will called after flush and process
    // no parallel situations here
    // check if ellc handle had been uinit
    if (mp_ellc_handle) {
        double cost = .0;
        std::tie(cost, std::ignore) = costTimeMs(MialgoAi_ellcUnit, mp_ellc_handle);
        MLOGI(Mia2LogGroupPlugin,
              "destroy check ellc handle not been uinit, uinit cost %lfms, ellc handle %p", cost,
              mp_ellc_handle);
    }
}
