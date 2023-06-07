/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiDebugUtils.h"

#include "MiDebugOfflineLogger.h"
#include "MiPraseSettings.h"
#include "camlog.h"

namespace midebug {

uint32_t gMiCamLogLevel = 0;
uint32_t gMiCamLogGroup = 0xFF;
uint32_t gMiCamDebugMask = 0;

static void signalOfflineLoggerThreadWrapper()
{
    // Allow caller to signal corresponding Offline logger thread to trigger flush
    (void)MiDebugInterface::getInstance()->signalOfflineLoggerThread();
}

const char *Log::getFileName(const char *pFilePath)
{
    const char *pFileName = strrchr(pFilePath, '/');

    if (NULL != pFileName) {
        pFileName += 1;
    } else {
        pFileName = pFilePath;
    }

    return pFileName;
}

void Log::updateLogInfo()
{
    gMiCamLogLevel = property_get_int32("persist.vendor.camera.mivi.loglevel", 3);

    gMiCamLogGroup = property_get_int32("persist.vendor.camera.mivi.groupsEnable", 0x7f);

    gMiCamDebugMask = MiPraseSettings::getInstance()->getIntFromSettingInfo(
        offlinedebugmask, 0, "persist.vendor.camera.offlinedebug.mask");

    g_traceInfo.groupsEnable = property_get_int32("persist.vendor.camera.mialgo.tracegroups", 0);
    g_traceInfo.traceErrorLogEnable = 0;
}

const char *Log::miaGroupToString(Mia2LogGroup group)
{
    const char *pString = NULL;
    switch (group) {
    case Mia2LogGroupCore:
        pString = "[AlgoFwk]";
        break;
    case Mia2LogGroupPlugin:
        pString = "[Plugin]";
        break;
    case Mia2LogGroupMeta:
        pString = "[Meta]";
        break;
    case Mia2LogGroupService:
        pString = "[Service]";
        break;
    case Mia2LogGroupDebug:
        pString = "[Debug]";
        break;
    case LogGroupCore:
        pString = "[Core]";
        break;
    case LogGroupHAL:
        pString = "[HAL ]";
        break;
    case LogGroupRT:
        pString = "[RT  ]";
        break;
    case Mia2LogGroupAlgorithm:
        pString = "[Algorithm]";
        break;
    default:
        pString = "[UNKNOWN]";
        break;
    }
    return pString;
}

const char *Log::miaGroupToTag(Mia2LogGroup group)
{
    const char *pString = NULL;
    switch (group) {
    case Mia2LogGroupCore:
    case Mia2LogGroupPlugin:
    case Mia2LogGroupMeta:
    case Mia2LogGroupAlgorithm:
        pString = "MiAlgoEngine";
        break;
    case Mia2LogGroupService:
        pString = "MiCamService";
        break;
    case Mia2LogGroupDebug:
        pString = "MiCamDebug";
        break;
    case LogGroupCore:
    case LogGroupHAL:
    case LogGroupRT:
        pString = "MiCamHAL";
        break;
    default:
        pString = "[UNKNOWN]";
        break;
    }
    return pString;
}

void Log::logSystem(uint32_t group, const char *level, const char *pFileName, const char *pFuncName,
                    int32_t lineNum, const char *format, ...)
{
    char CamErrorString[512] = {0};
    char strBuffer[MaxLogLength];
    size_t timestampLen = 0;
    size_t logLen = 0;
    struct timespec spec;
    memset(&spec, 0, sizeof(struct timespec));
    clock_gettime(CLOCK_REALTIME, &spec);
    struct tm pCurrentTime = {};
    if (NULL != localtime_r((time_t *)&spec.tv_sec, &pCurrentTime)) {
        timestampLen = snprintf(
            strBuffer, MaxLogLength - 1, "%02d-%02d %02d:%02d:%02d:%09ld  %4d  %4d %s ",
            (pCurrentTime.tm_mon + 1), pCurrentTime.tm_mday, pCurrentTime.tm_hour,
            pCurrentTime.tm_min, pCurrentTime.tm_sec, spec.tv_nsec, getpid(), gettid(), level);
    }

    // Generate output string
    va_list args;
    va_start(args, format);
    logLen =
        snprintf(strBuffer + timestampLen, sizeof(strBuffer) - timestampLen, "%s %s %s:%d %s() ",
                 miaGroupToTag(group), miaGroupToString(group), pFileName, lineNum, pFuncName);
    logLen += vsnprintf(strBuffer + timestampLen + logLen,
                        sizeof(strBuffer) - timestampLen - logLen, format, args);
    va_end(args);

    size_t len =
        ((timestampLen + logLen) > (MaxLogLength - 3)) ? MaxLogLength - 3 : timestampLen + logLen;
    strBuffer[len] = '\n';
    strBuffer[len + 1] = '\0';

    if (!strcmp(level, "E")) {
        MiDebugInterface::getInstance()->dumpErrorInfoToFile(strBuffer);
    }

    if (gMiCamDebugMask > 0) {
        MiDebugInterface::getInstance()->addLogInterface(strBuffer, len + 1);

        if (!strcmp(level, "E")) {
            // add MIVI error trigger
            char isSochanged[256] = "";
            property_get("vendor.camera.sensor.logsystem", isSochanged, "0");

            // When the value of logsystem is 1 or 3, the send_message_to_mqs interface is called
            // When the value of logsystem is 2, it means that the exception needs to be ignored
            if (atoi(isSochanged) == 1 || atoi(isSochanged) == 3) {
                snprintf(CamErrorString, 512, "%s:%s %s:%d fail", miaGroupToTag(group),
                         miaGroupToString(group), pFileName, lineNum);
                OFFLINELOG_INFO("[CAM_LOG_SYSTEM][MIVI] send error to MQS CamErrorString: %s",
                                CamErrorString);
                // Force to do the last time flush to file to ensure MQS get complete error log.
                // and continue flush log to file
                MiDebugInterface::getInstance()->flushLogToFileInterface(true);
                // false: abort() will not be triggered
                // true: abort() will be triggered
                camlog::send_message_to_mqs(CamErrorString, false);
            }
        }
    }
}

void Log::logSystem(uint32_t group, const char *level, const char *format, ...)
{
    char CamErrorString[512] = {0};
    char strBuffer[MaxLogLength];
    size_t timestampLen = 0;
    size_t logLen = 0;
    struct timespec spec;
    memset(&spec, 0, sizeof(struct timespec));
    clock_gettime(CLOCK_REALTIME, &spec);
    struct tm pCurrentTime = {};
    if (NULL != localtime_r((time_t *)&spec.tv_sec, &pCurrentTime)) {
        timestampLen = snprintf(
            strBuffer, MaxLogLength - 1, "%02d-%02d %02d:%02d:%02d:%09ld  %4d  %4d %s ",
            (pCurrentTime.tm_mon + 1), pCurrentTime.tm_mday, pCurrentTime.tm_hour,
            pCurrentTime.tm_min, pCurrentTime.tm_sec, spec.tv_nsec, getpid(), gettid(), level);
    }

    // Generate output string
    va_list args;
    va_start(args, format);
    logLen = snprintf(strBuffer + timestampLen, sizeof(strBuffer) - timestampLen, "%s %s ",
                      miaGroupToTag(group), miaGroupToString(group));
    logLen += vsnprintf(strBuffer + timestampLen + logLen,
                        sizeof(strBuffer) - timestampLen - logLen, format, args);
    va_end(args);

    size_t len =
        ((timestampLen + logLen) > (MaxLogLength - 3)) ? MaxLogLength - 3 : timestampLen + logLen;
    strBuffer[len] = '\n';
    strBuffer[len + 1] = '\0';

    MiDebugInterface::getInstance()->addLogInterface(strBuffer, len + 1);

    if (!strcmp(level, "E")) {
        // add MIVI error trigger
        char isSochanged[256] = "";
        property_get("vendor.camera.sensor.logsystem", isSochanged, "0");

        // When the value of logsystem is 1 or 3, the send_message_to_mqs interface is called
        // When the value of logsystem is 2, it means that the exception needs to be ignored
        if (atoi(isSochanged) == 1 || atoi(isSochanged) == 3) {
            int copyLen = len - timestampLen;
            copyLen = copyLen > 512 ? 512 : copyLen;
            memcpy(CamErrorString, strBuffer + timestampLen, copyLen);
            OFFLINELOG_INFO("[CAM_LOG_SYSTEM][MIVI] send error to MQS CamErrorString: %s",
                            CamErrorString);
            // Force to do the last time flush to file to ensure MQS get complete error log.
            // and continue flush log to file
            MiDebugInterface::getInstance()->flushLogToFileInterface(true);
            // false: abort() will not be triggered
            // true: abort() will be triggered
            camlog::send_message_to_mqs(CamErrorString, false);
        }
    }
}

MiDebugInterface *MiDebugInterface::getInstance()
{
    static MiDebugInterface sMiDebugInterface;

    return &sMiDebugInterface;
}

MiDebugInterface::MiDebugInterface()
{
    mTerminateOfflineLogThread = false;
    mStarted = false;
    mOfflineLoggerEnabled = false;
    mAbortEnable = false;

    Log::updateLogInfo();
    if (gMiCamDebugMask >= 1) {
        mOfflineLoggerEnabled = true;
    }
    if (gMiCamDebugMask >= 3) {
        mAbortEnable = true;
    }

    if (mOfflineLoggerEnabled) {
        // Create offlinelogger logger thread for flush
        mOfflineLoggerThread = thread([this]() { requestOfflineLogThreadProcessing(); });

        // Pass in OfflineLogger thread signal function pointer into OfflineLogger
        MiDegbugOfflineLogger::getInstance()->setFlushTriggerLink(
            &signalOfflineLoggerThreadWrapper);
    }
}

MiDebugInterface::~MiDebugInterface()
{
    mTerminateOfflineLogThread = true;
}

int MiDebugInterface::startOfflineLogger()
{
    int rc = 0;
    if (!mOfflineLoggerEnabled) {
        return rc;
    }

    std::unique_lock<std::recursive_mutex> lock(mOfflineLoggerMutex);
    OFFLINELOG_INFO("Begin mStarted: %d", mStarted);

    if (mStarted)
        return rc;

    rc = MiDegbugOfflineLogger::getInstance()->start();
    if (rc != 0) {
        OFFLINELOG_ERROR("OfflineLogger Initialize failed!");
        return rc;
    }

    mStarted = true;

    OFFLINELOG_INFO("End");
    return rc;
}

void MiDebugInterface::stopOfflineLogger()
{
    if (!mOfflineLoggerEnabled) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(mOfflineLoggerMutex);
    if (!mStarted)
        return;

    OFFLINELOG_INFO("Begin");
    MiDegbugOfflineLogger::getInstance()->stop();
    mStarted = false;
    OFFLINELOG_INFO("End");
}

void MiDebugInterface::signalOfflineLoggerThread()
{
    // Allow caller to signal Offline logger thread to trigger flush
    mFlushNeededCond.notify_all();
}

void MiDebugInterface::requestOfflineLogThreadProcessing()
{
    while (true) {
        if (false == mTerminateOfflineLogThread) {
            // Wait for flush trigger logic to be satisfied before executing
            std::unique_lock<std::mutex> lock(mFlushNeededMutex);
            mFlushNeededCond.wait(lock);
        } else {
            // Terminate thread
            OFFLINELOG_INFO("OfflineLogger Thread is now terminated!");
            break;
        }

        if (false == mTerminateOfflineLogThread) {
            MiDegbugOfflineLogger::getInstance()->flushLogToFile(false);
        }
    }
}

void MiDebugInterface::addLogInterface(const char *log, uint32_t logSize)
{
    if (!mOfflineLoggerEnabled) {
        return;
    }

    MiDegbugOfflineLogger::getInstance()->addLog(log, logSize);
}

void MiDebugInterface::flushLogToFileInterface(bool lastFlush)
{
    if (!mOfflineLoggerEnabled) {
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(mOfflineLoggerMutex);
    MiDegbugOfflineLogger::getInstance()->flushLogToFile(lastFlush);
    MiDegbugOfflineLogger::getInstance()->activeOfflineLogger();
}

void MiDebugInterface::dumpErrorInfoToFile(const char *errorInfo)
{
    static const char *dumpFilePath =
        "data/vendor/camera/offlinelog/MiCam_Debug_MiviErrorInfoDump.txt";
    static const off_t MAX_MiviErrorInfoDumpFile_SIZE = 512 * 1024; // 512k
    std::lock_guard<std::mutex> locker(dumpMiviErrorInfoMutex);

    struct stat myStat;
    bool clearFile = false;
    bool isFirstOpenDumpFile = false;
    if (!stat(dumpFilePath, &myStat)) { // file exists
        if (myStat.st_size > MAX_MiviErrorInfoDumpFile_SIZE) {
            clearFile = true;
        }
    } else {
        isFirstOpenDumpFile = true;
    }

    FILE *pf = NULL;
    if (clearFile) {
        pf = fopen(dumpFilePath, "wb+"); // clear file and add a new line
    } else {
        pf = fopen(dumpFilePath, "ab+"); // add a new line
    }

    if (isFirstOpenDumpFile) {
        chmod(dumpFilePath, 0666);
    }

    if (pf != NULL) {
        fprintf(pf, "%s\n", errorInfo);
        fclose(pf);
    }
}

} // namespace midebug
