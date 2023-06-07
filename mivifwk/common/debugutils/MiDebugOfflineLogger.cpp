/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiDebugOfflineLogger.h"

#include <cutils/properties.h>
#include <dirent.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>

#include <ctime>

namespace midebug {

static struct sigaction gDefaultsigsegvhndlr;
static struct sigaction gDefaultsigabrthndlr;
static struct sigaction gDefaultsigfpehndlr;
static struct sigaction gDefaultsigillhndlr;
static struct sigaction gDefaultsigbushndlr;
static struct sigaction gDefaultsigsyshndlr;
static struct sigaction gDefaultsigtraphndlr;

static const uint32_t MaxPreAllocateQueue = 16;    // Total memory buckets    : 16
static const int32_t MaxFileSize = 10485760;       // Max size of one log fize: 10 (MB)
static const int32_t MaxLocalBufferSize = 1048576; // Each memory bucket size : 1  (MB)
static const uint32_t MinBufferNumFlushCriteria =
    MaxPreAllocateQueue / 2;                           // Exceed this mem buckets will trigger flush
static const int32_t MaxSupportCrashHeaderSize = 1024; // Max crash message array size for header
static const int32_t MaxSupportCrashContentSize = 4096; // Max crash message array size for content
static const int32_t MaxSupportCrashLnSize = 1024;    // Max crash message array size for each line
static const int32_t MaxSupportTimestampSize = 100;   // Max crash message array size for timestamp
static const uint32_t SkipTopBacktraceDepth = 2;      // Skip the top N backtrace
static const float MinRequiredAvailabSpace = 3000.0f; // Minium required space   : 3000 (MB)
static const float ConversionToMByte = 1048576.0f;    // Covert to Mega Byte Factor

thread_local uint8_t *MiDegbugOfflineLogger::sThreadLocalLogBuffer = nullptr;
thread_local int32_t MiDegbugOfflineLogger::sThreadLocalQueuePoolId = 0;
thread_local uint32_t MiDegbugOfflineLogger::sThreadLocalSessionCnt = 0;
thread_local uint32_t MiDegbugOfflineLogger::sThreadLocalLogSize = 0;
thread_local uint32_t MiDegbugOfflineLogger::sThreadLocalLogCurPos = 0;
thread_local bool MiDegbugOfflineLogger::sThreadLocalFlushDone = false;
thread_local LifetimeThreadInfo MiDegbugOfflineLogger::sThreadLocalLifeTime;

recursive_mutex MiDegbugOfflineLogger::mOfflineLoggerLock;
pthread_rwlock_t MiDegbugOfflineLogger::mOfflineLoggerRWLock;
mutex MiDegbugOfflineLogger::mQueueLoadLock;

OFFLINELOG_CFI_NO_SANITIZE void InvokeDefaultHndlr(struct sigaction *pAction, int32_t sigNum,
                                                   siginfo_t *pSigInfo, void *pContext)
{
    if (pAction->sa_flags & SA_SIGINFO) {
        pAction->sa_sigaction(sigNum, pSigInfo, pContext);
    } else {
        pAction->sa_handler(sigNum);
    }
}

OFFLINELOG_CFI_NO_SANITIZE void SignalCatcher(int32_t sigNum, siginfo_t *pSigInfo, void *pContext)
{
    char crashMsgHead[MaxSupportCrashHeaderSize];
    char crashMsgStackOri[MaxSupportCrashContentSize];
    char crashMsgStackFinal[MaxSupportCrashLnSize];
    char timestampText[MaxSupportTimestampSize];

    struct timespec spec;
    memset(&spec, 0, sizeof(struct timespec));
    clock_gettime(CLOCK_REALTIME, &spec);
    bool flag = false;
    struct tm pCurrentTime = {};
    if (NULL != localtime_r((time_t *)&spec.tv_sec, &pCurrentTime)) {
        flag = true;
        snprintf(timestampText, MaxSupportTimestampSize, "%02d-%02d %02d:%02d:%02d:%06ld  %4d  %4d",
                 (pCurrentTime.tm_mon + 1), pCurrentTime.tm_mday, pCurrentTime.tm_hour,
                 pCurrentTime.tm_min, pCurrentTime.tm_sec, spec.tv_nsec, getpid(), gettid());

        snprintf(crashMsgHead, MaxSupportCrashHeaderSize,
                 "%s F DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***\n"
                 "%s F DEBUG   : pid: %d, tid: %d\n"
                 "%s F DEBUG   : signal %d, code %d, fault addr %p\n"
                 "%s F DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***\n",
                 timestampText, timestampText, getpid(), gettid(), timestampText,
                 pSigInfo->si_signo, pSigInfo->si_code, pSigInfo->si_addr, timestampText);

        MiDebugInterface::getInstance()->addLogInterface(crashMsgHead, strlen(crashMsgHead));
    }

    // Using Android::CallStack to generate backtrace
    android::CallStack stack;
    stack.update();

    // Convert Android string to char
    android::String8 stackString;
    stackString = stack.toString();
    strlcpy(crashMsgStackOri, stackString.c_str(), sizeof(crashMsgStackOri));

    // Split the long backtrace string to single line and add timestamp
    char delim[] = "#";
    char *pSavePtr = NULL;
    char *pSplitChar = strtok_r(crashMsgStackOri, delim, &pSavePtr);

    uint32_t skipDepth = SkipTopBacktraceDepth;
    while (NULL != pSplitChar) {
        // Output backtrace to char format and push to offlinelogger
        // Top 1 and 2 from backtrace stack is the offlinelogger itself
        // not come from crash thread so we ignore it and start loop from 2
        if (0 == skipDepth) {
            if (flag) {
                snprintf(crashMsgStackFinal, MaxSupportCrashLnSize, "%s F DEBUG   : #%s",
                         timestampText, pSplitChar);
            } else {
                snprintf(crashMsgStackFinal, MaxSupportCrashLnSize, "F DEBUG   : #%s", pSplitChar);
            }

            MiDebugInterface::getInstance()->addLogInterface(crashMsgStackFinal,
                                                             strlen(crashMsgStackFinal));
        } else {
            skipDepth--;
        }

        pSplitChar = strtok_r(NULL, delim, &pSavePtr);
    }

    // Force to do the last time flush to file to ensure we get complete error log.
    MiDebugInterface::getInstance()->flushLogToFileInterface(true);

    switch (sigNum) {
    case SIGSEGV:
        InvokeDefaultHndlr(&gDefaultsigsegvhndlr, sigNum, pSigInfo, pContext);
        break;
    case SIGABRT:
        InvokeDefaultHndlr(&gDefaultsigabrthndlr, sigNum, pSigInfo, pContext);
        break;
    case SIGFPE:
        InvokeDefaultHndlr(&gDefaultsigfpehndlr, sigNum, pSigInfo, pContext);
        break;
    case SIGBUS:
        InvokeDefaultHndlr(&gDefaultsigbushndlr, sigNum, pSigInfo, pContext);
        break;
    case SIGSYS:
        InvokeDefaultHndlr(&gDefaultsigsyshndlr, sigNum, pSigInfo, pContext);
        break;
    case SIGTRAP:
        InvokeDefaultHndlr(&gDefaultsigtraphndlr, sigNum, pSigInfo, pContext);
        break;
    case SIGILL:
        InvokeDefaultHndlr(&gDefaultsigillhndlr, sigNum, pSigInfo, pContext);
        break;
    default:
        OFFLINELOG_ERROR("Signal %d received, No action matching", sigNum);
        break;
    }
}

OFFLINELOG_CFI_NO_SANITIZE void RegisterSignalHandlers()
{
    struct sigaction newSigAction;

    memset(&newSigAction, '\0', sizeof(struct sigaction));
    if (sigaction(SIGSEGV, NULL, &gDefaultsigsegvhndlr) < 0) {
        OFFLINELOG_ERROR("Failed to get signal handler for SIGSEGV");
    }
    newSigAction = gDefaultsigsegvhndlr;
    newSigAction.sa_flags |= SA_SIGINFO;
    newSigAction.sa_sigaction = SignalCatcher;
    if (sigaction(SIGSEGV, &newSigAction, NULL) < 0) {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGSEGV");
    }
    if (sigaction(SIGABRT, &newSigAction, &gDefaultsigabrthndlr) < 0) {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGABRT");
    }
    if (sigaction(SIGBUS, &newSigAction, &gDefaultsigbushndlr) < 0) {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGBUS");
    }
    if (sigaction(SIGFPE, &newSigAction, &gDefaultsigfpehndlr) < 0) {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGFPE");
    }
    if (sigaction(SIGILL, &newSigAction, &gDefaultsigillhndlr) < 0) {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGILL");
    }
    if (sigaction(SIGSYS, &newSigAction, &gDefaultsigsyshndlr) < 0) {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGSYS");
    }
    if (sigaction(SIGTRAP, &newSigAction, &gDefaultsigtraphndlr) < 0) {
        OFFLINELOG_ERROR("Failed to register signal handler for SIGTRAP");
    }
}

MiDegbugOfflineLogger::MiDegbugOfflineLogger()
{
    mOfflinelogHasPreFileName = false;
    mOfflinelogPreFileName = "";
    mOfflinelogStatus = OfflineLoggerStatus::DISABLE;
    mSessionCount = 1;
    mForceFlushDone = false;
    mDeleteLocalLogDone = false;
    mOfflineLogNumber = MaxOfflineLogNums;
    mOfflinelogPreLogTypeName = "Debug";
    mMemoryLogPool.clear();
    mSignalRegistered = false;
    mNeedFlushThreadCount = 0;
    mTotalFlushLogSize = 0;
    mTotalWritedByte = 0;
    mSignalFlushTriggerThread = NULL;
    pthread_rwlock_init(&mOfflineLoggerRWLock, NULL);
}

MiDegbugOfflineLogger::~MiDegbugOfflineLogger()
{
    // Clear the element in our log memory pool
    for (auto buffer : mMemoryLogPool) {
        free(buffer);
    }
    mMemoryLogPool.clear();
}

bool MiDegbugOfflineLogger::isEnableOfflinelogger()
{
    bool result = true;

    if (OfflineLoggerStatus::ACTIVE != mOfflinelogStatus) {
        result = false;
    }

    return result;
}

bool MiDegbugOfflineLogger::setFlushTriggerLink(PFNSIGNALOFFLINETHREAD pSignalFunc)
{
    bool result = true;

    if (NULL == pSignalFunc) {
        result = false;
    } else {
        mSignalFlushTriggerThread = pSignalFunc;
    }

    return result;
}

int MiDegbugOfflineLogger::start()
{
    int result = 0;

    // read local directory gz file name into mLogFileNameQueue
    if (false == mDeleteLocalLogDone) {
        string fileName;
        char dumpFilename[FILENAME_MAX];

        struct dirent *entry = NULL;
        DIR *pDir = opendir(midebug::ConfigFileDirectory);
        priority_queue<string, vector<string>, greater<string> > pri;
        bool hasSameFile = false;

        while ((entry = readdir(pDir)) != NULL) {
            fileName = entry->d_name;
            if (fileName.find(mOfflinelogPreLogTypeName.c_str()) != string::npos) {
                snprintf(dumpFilename, sizeof(dumpFilename), "%s/%s", midebug::ConfigFileDirectory,
                         fileName.c_str());
                pri.push(dumpFilename);
            }
        }
        closedir(pDir);

        while (true != pri.empty()) {
            queue<string> tmp = mLogFileNameQueue;
            while (true != tmp.empty()) {
                if (tmp.front().c_str() == pri.top().c_str()) {
                    OFFLINELOG_ERROR("mi-debug OfflineLogger has same file");
                    hasSameFile = true;
                    break;
                }

                tmp.pop();
            }

            if (false == hasSameFile) {
                OFFLINELOG_INFO("List local debug file: %s", pri.top().c_str());
                mLogFileNameQueue.push(pri.top().c_str());
            }

            hasSameFile = false;
            pri.pop();
        }
        mDeleteLocalLogDone = true;
    }

    // Check system has enough space left when camera server initializes
    float availableSpace = getAvailableSpace(midebug::ConfigFileDirectory);
    if (availableSpace >= 0.0f && availableSpace < MinRequiredAvailabSpace) {
        result = -1;

        FILE *pFile = NULL;
        // Write a warning file to system to inform disk space insufficient
        char dumpFilename[FILENAME_MAX];

        int ret = snprintf(dumpFilename, sizeof(dumpFilename), "%s/MiCam_%s_ErrorMsg.txt",
                           midebug::ConfigFileDirectory, mOfflinelogPreLogTypeName.c_str());
        if (ret > 0) {
            pFile = fopen(dumpFilename, "a+");
        }

        if (NULL != pFile) {
            fprintf(pFile, "OfflineLogger detect file system has no space left");
            fclose(pFile);
        }

        OFFLINELOG_ERROR("OfflineLogger detect file system has no space left!");
    } else if (availableSpace < 0.0f) {
        result = -1;
        OFFLINELOG_ERROR("OfflineLogger query system space failed!");
    }

    if (0 == result) {
        // If feature is enable then we can pre-allocate memory/set logger status as active/register
        // signal handler

        // Allocate Log queue pool
        mOfflineLoggerLock.lock();
        result = allocateLogQueueMemPool();
        mOfflineLoggerLock.unlock();

        // If we allocate memory success then we can set logger status as active
        if (0 == result) {
            mOfflinelogStatus = OfflineLoggerStatus::ACTIVE;
        }

        // Signal handler should be registered only one time if feature enable,
        // otherwise the signal cannot get back to parent process
#ifndef USERDEBUG_CAM
        if (false == mSignalRegistered && 0 == result) {
            // RegisterSignalHandlers();
            mSignalRegistered = true;
        }
#endif
    } else {
        result = -1;
        OFFLINELOG_ERROR("OfflineLogger get override setting failed");
    }

    return result;
}

void MiDegbugOfflineLogger::stop()
{
    OFFLINELOG_INFO("E");
    mOfflinelogStatus = OfflineLoggerStatus::CAMERA_CLOSING;
    flushLogToFile(true);
    mOfflinelogStatus = OfflineLoggerStatus::DISABLE;

    setAllToDefaultValue();

    mForceFlushDone = false;
    mSessionCount++;

    // Empty the available memory pool
    while (!mAvailableQueueIdPool.empty()) {
        mAvailableQueueIdPool.pop();
    }

    // Clear the element in our log memory pool
    for (auto &buffer : mMemoryLogPool) {
        free(buffer);
        buffer = NULL;
    }
    mMemoryLogPool.clear();

    OFFLINELOG_INFO("X mSessionCount %d", mSessionCount);
}

void MiDegbugOfflineLogger::activeOfflineLogger()
{
    mOfflinelogStatus = OfflineLoggerStatus::ACTIVE;
}

int MiDegbugOfflineLogger::allocateLogQueueMemPool()
{
    int result = 0;

    for (uint32_t i = 0; i < MaxPreAllocateQueue; i++) {
        uint8_t *pData = (uint8_t *)malloc(MaxLocalBufferSize);

        if (NULL != pData) {
            memset(pData, 0, MaxLocalBufferSize);
            mMemoryLogPool.push_back(pData);
            mAvailableQueueIdPool.push(static_cast<int32_t>(mMemoryLogPool.size() - 1));
        } else {
            result = -1;
            OFFLINELOG_ERROR("OfflineLogger allocate memory failed");
        }
    }

    OFFLINELOG_INFO("mMemoryLogPool: %zu", mMemoryLogPool.size());
    return result;
}

void MiDegbugOfflineLogger::requestFlushLogQueue(int32_t registerId)
{
    mOfflineLoggerLock.lock();

    if (registerId >= mPerLogInfoPool.size()) {
        OFFLINELOG_ERROR("MiDegbugOfflineLogger registerId %d mPerLogInfoPool %d", registerId,
                         (int)mPerLogInfoPool.size());
        mOfflineLoggerLock.unlock();
        return;
    }

    mPerLogInfoPool[registerId].bNeedFlush = true;

    // Increment count of threads needing flush
    mNeedFlushThreadCount++;
    // Push in registerID to queue to track buffer flush order
    mRegisterIdFlushOrderQueue.push(registerId);
    // Buffer is full, signal flush thread to trigger a flush
    mOfflineLoggerLock.unlock();
    if (NULL != mSignalFlushTriggerThread) {
        OFFLINELOG_INFO(
            "OfflineLogger requestFlushLogQueue Signaled!!"
            "Signal = %p, mNeedFlushThreadCount:%d",
            mSignalFlushTriggerThread, mNeedFlushThreadCount);
        mSignalFlushTriggerThread();
    }
}

LifetimeThreadInfo::~LifetimeThreadInfo()
{
    // Trigger flush while thread being killed
    MiDegbugOfflineLogger *pOfflineLogger = MiDegbugOfflineLogger::getInstance();
    pOfflineLogger->requestFlushLogQueue(registerID);
}

int32_t MiDegbugOfflineLogger::requestNewLogQueue(uint8_t *&rpNewQueue, uint32_t tid)
{
    mOfflineLoggerLock.lock();

    int32_t memoryPoolId = -1;
    int32_t registerId = -1;
    uint32_t sessionCount = sThreadLocalSessionCnt;

    if (false == mAvailableQueueIdPool.empty()) {
        memoryPoolId = mAvailableQueueIdPool.front();
        mAvailableQueueIdPool.pop();

        if (false == mAvailableRegisterIdPool.empty()) {
            registerId = mAvailableRegisterIdPool.front();
            mAvailableRegisterIdPool.pop();
            mPerLogInfoPool[registerId].bNeedFlush = false;
            mPerLogInfoPool[registerId].bHasFileName = false;
            mPerLogInfoPool[registerId].registerID = memoryPoolId;
            mPerLogInfoPool[registerId].threadID = tid;
            mPerLogInfoPool[registerId].sessionCount = sessionCount;
            mPerLogInfoPool[registerId].currentLogSize = 0;
            mPerLogInfoPool[registerId].numByteToWrite = 0;
        } else {
            PerLogThreadInfo info;
            info.bNeedFlush = false;
            info.bHasFileName = false;
            info.registerID = memoryPoolId;
            info.threadID = tid;
            info.sessionCount = sessionCount;
            info.currentLogSize = 0;
            info.numByteToWrite = 0;
            mPerLogInfoPool.push_back(info);
            registerId = mPerLogInfoPool.size() - 1;
        }
    } else {
        // We don`t have enough available memory so need to expand it
        allocateLogQueueMemPool();

        if (false == mAvailableQueueIdPool.empty()) {
            memoryPoolId = mAvailableQueueIdPool.front();
            mAvailableQueueIdPool.pop();

            PerLogThreadInfo info;
            info.bNeedFlush = false;
            info.bHasFileName = false;
            info.registerID = memoryPoolId;
            info.threadID = tid;
            info.sessionCount = sessionCount;
            info.currentLogSize = 0;
            info.numByteToWrite = 0;
            mPerLogInfoPool.push_back(info);
            registerId = mPerLogInfoPool.size() - 1;
        }
    }

    if (registerId == -1) {
        // registerId equal to default value, it must be a serious failure
        OFFLINELOG_ERROR("Offlinelog failed to request new container");
        rpNewQueue = NULL;
    } else {
        rpNewQueue = mMemoryLogPool.at(memoryPoolId);
    }

    sThreadLocalLifeTime.threadID = tid;
    sThreadLocalLifeTime.registerID = registerId;

    mOfflineLoggerLock.unlock();

    return registerId;
}

MiDegbugOfflineLogger *MiDegbugOfflineLogger::getInstance()
{
    static MiDegbugOfflineLogger sMiDegbugOfflineLogger;

    return &sMiDegbugOfflineLogger;
}

void MiDegbugOfflineLogger::addLog(const char *pLog, uint32_t logSize)
{
    if (!isEnableOfflinelogger()) {
        return;
    }

    pthread_rwlock_rdlock(&mOfflineLoggerRWLock);

    // Check the mOfflinelogStatus again
    if (!isEnableOfflinelogger()) {
        pthread_rwlock_unlock(&mOfflineLoggerRWLock);
        return;
    }

    // If the camera is reopen, the thread local session count will minus one than offlinelogger
    uint8_t *memoryAddressToWrite = NULL;
    if (sThreadLocalSessionCnt != mSessionCount) {
        sThreadLocalSessionCnt = mSessionCount;

        // Reset the file size
        sThreadLocalLogSize = 0;
        sThreadLocalLogCurPos = 0;
        sThreadLocalFlushDone = false;

        // Done reset, request a new buffer to add log
        sThreadLocalQueuePoolId = requestNewLogQueue(sThreadLocalLogBuffer, gettid());

        if (NULL != sThreadLocalLogBuffer) {
            memoryAddressToWrite = &(sThreadLocalLogBuffer[sThreadLocalLogCurPos]);
            sThreadLocalLogCurPos += logSize;
            sThreadLocalLogSize += logSize;

            mPerLogInfoPool.at(sThreadLocalQueuePoolId).numByteToWrite = sThreadLocalLogCurPos;
            mPerLogInfoPool.at(sThreadLocalQueuePoolId).currentLogSize = sThreadLocalLogSize;
        }
    } else {
        if (sThreadLocalQueuePoolId >= mPerLogInfoPool.size()) {
            OFFLINELOG_ERROR("sThreadLocalQueuePoolId : %d mPerLogInfoPool size: %d",
                             sThreadLocalQueuePoolId, (int)mPerLogInfoPool.size());
            pthread_rwlock_unlock(&mOfflineLoggerRWLock);
            return;
        }

        if (sThreadLocalLogBuffer !=
            mMemoryLogPool.at(mPerLogInfoPool.at(sThreadLocalQueuePoolId).registerID)) {
            OFFLINELOG_ERROR(
                "OFFLINELOG:%p != %p %d %d", sThreadLocalLogBuffer,
                mMemoryLogPool.at(mPerLogInfoPool.at(sThreadLocalQueuePoolId).registerID),
                sThreadLocalQueuePoolId, mPerLogInfoPool.at(sThreadLocalQueuePoolId).registerID);
            sThreadLocalLogBuffer =
                mMemoryLogPool.at(mPerLogInfoPool.at(sThreadLocalQueuePoolId).registerID);
        }

        if (NULL != sThreadLocalLogBuffer) {
            // If we find local flush flag != central flush flag means
            // force flash happened in central and flush all the container done
            // so need to reset local log size and cursor pos
            if (sThreadLocalFlushDone != mForceFlushDone && sThreadLocalFlushDone == false) {
                sThreadLocalFlushDone = mForceFlushDone;
                sThreadLocalLogSize = 0;
                sThreadLocalLogCurPos = 0;

                if (sThreadLocalQueuePoolId < mPerLogInfoPool.size()) {
                    mPerLogInfoPool.at(sThreadLocalQueuePoolId).numByteToWrite = 0;
                }
            }

            if (sThreadLocalLogCurPos + logSize > MaxLocalBufferSize) {
                // request offlinelog to flush the current container
                requestFlushLogQueue(sThreadLocalQueuePoolId);

                // request new queue for thread local container
                sThreadLocalQueuePoolId = requestNewLogQueue(sThreadLocalLogBuffer, gettid());
                sThreadLocalLogCurPos = 0;

                if (NULL != sThreadLocalLogBuffer) {
                    memoryAddressToWrite = &(sThreadLocalLogBuffer[sThreadLocalLogCurPos]);
                    sThreadLocalLogCurPos += logSize;
                    sThreadLocalLogSize += logSize;

                    mPerLogInfoPool.at(sThreadLocalQueuePoolId).numByteToWrite =
                        sThreadLocalLogCurPos;
                    mPerLogInfoPool.at(sThreadLocalQueuePoolId).currentLogSize =
                        sThreadLocalLogSize;
                }
            } else if (sThreadLocalLogSize > MaxFileSize) {
                sThreadLocalLogSize = 0;

                requestFlushLogQueue(sThreadLocalQueuePoolId);

                sThreadLocalQueuePoolId = requestNewLogQueue(sThreadLocalLogBuffer, gettid());
                sThreadLocalLogCurPos = 0;

                if (NULL != sThreadLocalLogBuffer) {
                    memoryAddressToWrite = &(sThreadLocalLogBuffer[sThreadLocalLogCurPos]);
                    sThreadLocalLogCurPos += logSize;
                    sThreadLocalLogSize += logSize;

                    mPerLogInfoPool.at(sThreadLocalQueuePoolId).numByteToWrite =
                        sThreadLocalLogCurPos;
                    mPerLogInfoPool.at(sThreadLocalQueuePoolId).currentLogSize =
                        sThreadLocalLogSize;
                }
            } else {
                if (sThreadLocalQueuePoolId >= mPerLogInfoPool.size()) {
                    OFFLINELOG_ERROR("sThreadLocalQueuePoolId : %d mPerLogInfoPool size: %d",
                                     sThreadLocalQueuePoolId, (int)mPerLogInfoPool.size());
                    pthread_rwlock_unlock(&mOfflineLoggerRWLock);
                    return;
                }

                memoryAddressToWrite = &(sThreadLocalLogBuffer[sThreadLocalLogCurPos]);
                sThreadLocalLogCurPos += logSize;
                sThreadLocalLogSize += logSize;

                mPerLogInfoPool.at(sThreadLocalQueuePoolId).numByteToWrite = sThreadLocalLogCurPos;
                mPerLogInfoPool.at(sThreadLocalQueuePoolId).currentLogSize = sThreadLocalLogSize;
            }
        }
    }

    if (memoryAddressToWrite && pLog) {
        memcpy(memoryAddressToWrite, pLog, logSize);
    } else {
        OFFLINELOG_ERROR("memoryAddressToWrite is %p inputlog address is %p!", memoryAddressToWrite,
                         pLog);
    }
    pthread_rwlock_unlock(&mOfflineLoggerRWLock);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MiDegbugOfflineLogger::flushLogToFile
// 1. write all log of every thread to one file instead of creating file for every thread.
// 2. If log file size > MaxFileSize, create a new log file.
// 3. write log into gizp or txt file directly.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MiDegbugOfflineLogger::flushLogToFile(bool lastFlush)
{
    int result = 0;

    OFFLINELOG_INFO("flushLogToFile lastFlush: %d, status: %d, mNeedFlushThreadCount: %d",
                    lastFlush, mOfflinelogStatus, mNeedFlushThreadCount);

    // The offline logger is disable
    if (OfflineLoggerStatus::DISABLE == mOfflinelogStatus) {
        result = -1;
    }

    if (0 == result) {
        // While we do the last flush means we flush all the container we have
        // so we need to mark we are in the closing status and each thread can deal with the special
        // case
        pthread_rwlock_wrlock(&mOfflineLoggerRWLock);
        mOfflineLoggerLock.lock();

        if (true == lastFlush) {
            mOfflinelogStatus = OfflineLoggerStatus::CAMERA_CLOSING;

            // Push all valid registerIDs to queue so they will be force flushed
            for (uint32_t i = 0; i < mPerLogInfoPool.size(); i++) {
                mRegisterIdFlushOrderQueue.push(i);
            }
        }

        // Only continue flush if at least half of buffers full or this is a force flush.
        // Otherwise skip below operations for performance
        if (mNeedFlushThreadCount >= MinBufferNumFlushCriteria || lastFlush) {
            mQueueLoadLock.lock();
            // Flush in order, or if lastFlush = true, flush all
            while (true != mRegisterIdFlushOrderQueue.empty()) {
                if (mRegisterIdFlushOrderQueue.front() >= mPerLogInfoPool.size()) {
                    mRegisterIdFlushOrderQueue.pop();
                    continue;
                }
                auto it = std::next(mPerLogInfoPool.begin(), mRegisterIdFlushOrderQueue.front());
                // Get Pre-fileName
                if (false == mOfflinelogHasPreFileName) {
                    getPrefixFilename();
                    mOfflinelogFileName = mOfflinelogPreFileName + ".txt";
                    mLogFileNameQueue.push(mOfflinelogFileName);
                }
                // Already has a filename, so flush out the log to file
                // or if got to do last flush in a session, we flush out all log to files
                OFFLINELOG_INFO("bNeedFlush: %d, lastFlush: %d, fileName: %s", (*it).bNeedFlush,
                                lastFlush, mOfflinelogFileName.c_str());
                if ((true == (*it).bNeedFlush) ||
                    (false == (*it).bNeedFlush &&
                     (true == lastFlush && (*it).numByteToWrite > 0))) {
                    FILE *pFile = NULL;
                    pFile = fopen(mOfflinelogFileName.c_str(), "a+");
                    if (NULL != pFile) {
                        fchmod(fileno(pFile),
                               S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                        fwrite(mMemoryLogPool[(*it).registerID], sizeof(char), (*it).numByteToWrite,
                               pFile);
                        memset(mMemoryLogPool[(*it).registerID], 0, MaxLocalBufferSize);
                        fclose(pFile);
                    } else {
                        OFFLINELOG_ERROR("mi-debug TXT Offlinelog failed to open file and write");
                    }

                    mTotalWritedByte += (*it).numByteToWrite;
                    if (mTotalWritedByte > MaxFileSize) {
                        // set mOfflinelogHasPreFileName to false for creating a new log file
                        mOfflinelogHasPreFileName = false;
                        mOfflinelogPreFileName = "";
                        mTotalWritedByte = 0;
                    }

                    (*it).bNeedFlush = false;
                    (*it).numByteToWrite = 0;

                    if (mNeedFlushThreadCount >= 1) {
                        mNeedFlushThreadCount--;
                    }

                    // Push available id to pool and remove flush job from job pool
                    mAvailableQueueIdPool.push((*it).registerID);
                    mAvailableRegisterIdPool.push(mRegisterIdFlushOrderQueue.front());
                }
                mRegisterIdFlushOrderQueue.pop();
            }

            while (uint32_t(mLogFileNameQueue.size()) > mOfflineLogNumber) {
                int ret = 0;
                ret = remove(mLogFileNameQueue.front().c_str());
                if (ret < 0) {
                    OFFLINELOG_ERROR("remove OfflineLog failed: %s",
                                     mLogFileNameQueue.front().c_str());
                }
                mLogFileNameQueue.pop();
            }
            mQueueLoadLock.unlock();
        }

        if (true == lastFlush) {
            mForceFlushDone = true;
        }
        mOfflineLoggerLock.unlock();
        pthread_rwlock_unlock(&mOfflineLoggerRWLock);
    }

    return result;
}

void MiDegbugOfflineLogger::setAllToDefaultValue()
{
    pthread_rwlock_wrlock(&mOfflineLoggerRWLock);
    mOfflineLoggerLock.lock();

    // mOfflinelogHasPreFileName = false;
    // mOfflinelogPreFileName = "";

    // Empty the available memory pool
    while (!mAvailableQueueIdPool.empty()) {
        mAvailableQueueIdPool.pop();
    }

    // Mark all the memory queue pool as available
    for (size_t i = 0; i < mMemoryLogPool.size(); i++) {
        mAvailableQueueIdPool.push(static_cast<int32_t>(i));
    }

    // Empty the available register pool
    while (!mAvailableRegisterIdPool.empty()) {
        mAvailableRegisterIdPool.pop();
    }

    // Empty the flush order queue
    while (!mRegisterIdFlushOrderQueue.empty()) {
        mRegisterIdFlushOrderQueue.pop();
    }

    // Empty per thread info pool
    mPerLogInfoPool.clear();

    mOfflineLoggerLock.unlock();
    pthread_rwlock_unlock(&mOfflineLoggerRWLock);
}

string MiDegbugOfflineLogger::getPrefixFilename()
{
    char dumpFilename[FILENAME_MAX];
    char timeStamp[MaxTimeStampLength];

    struct timeval tv;
    memset(&tv, 0, sizeof(struct timeval));
    struct timezone tz;
    memset(&tz, 0, sizeof(struct timezone));
    gettimeofday(&tv, &tz);
    struct tm pCurrentTime = {};
    if (localtime_r((time_t *)&tv.tv_sec, &pCurrentTime) != NULL) {
        snprintf(timeStamp, MaxTimeStampLength, "%2d-%02d-%02d--%02d-%02d-%02d-%03ld",
                 (pCurrentTime.tm_year + 1900), (pCurrentTime.tm_mon + 1), pCurrentTime.tm_mday,
                 pCurrentTime.tm_hour, pCurrentTime.tm_min, pCurrentTime.tm_sec, tv.tv_usec);
    } else {
        timeStamp[0] = '\0';
    }

    snprintf(dumpFilename, sizeof(dumpFilename), "%s/MiCam_%s_%s", midebug::ConfigFileDirectory,
             mOfflinelogPreLogTypeName.c_str(), timeStamp);

    mOfflinelogPreFileName = dumpFilename;
    mOfflinelogHasPreFileName = true;

    return dumpFilename;
}

float MiDegbugOfflineLogger::getAvailableSpace(const char *pPath)
{
    float result = 0.0f;
    struct statvfs vfs;

    if (statvfs(pPath, &vfs) != 0) {
        // Error to get free storage space, it must be serious system fault
        mOfflinelogStatus = OfflineLoggerStatus::DISABLE;
        result = -1.0f;
        OFFLINELOG_ERROR("Offlinelog failed to get file system information!");
    } else {
        // the available size is f_bsize * f_bavail, the return unit is "MEGA uint8_t"
        result = (static_cast<float>(vfs.f_bsize * vfs.f_bavail) / ConversionToMByte);
    }
    return result;
}

string MiDegbugOfflineLogger::getCurrentTime()
{
    char timeStamp[MaxTimeStampLength];

    struct timeval tv;
    memset(&tv, 0, sizeof(struct timeval));
    struct timezone tz;
    memset(&tz, 0, sizeof(struct timezone));
    gettimeofday(&tv, &tz);
    struct tm pCurrentTime = {};
    if (localtime_r((time_t *)&tv.tv_sec, &pCurrentTime) != NULL) {
        snprintf(timeStamp, MaxTimeStampLength, "%2d-%02d-%02d--%02d-%02d-%02d-%09ld",
                 (pCurrentTime.tm_year + 1900), (pCurrentTime.tm_mon + 1), pCurrentTime.tm_mday,
                 pCurrentTime.tm_hour, pCurrentTime.tm_min, pCurrentTime.tm_sec, tv.tv_usec);
    } else {
        timeStamp[0] = '\0';
    }

    string timeStr(timeStamp);
    return timeStr;
}

} // namespace midebug
