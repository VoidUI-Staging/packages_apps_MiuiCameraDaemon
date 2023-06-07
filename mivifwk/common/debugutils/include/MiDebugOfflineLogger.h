/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef MIDEBUG_OFFLINELOGGER_H
#define MIDEBUG_OFFLINELOGGER_H

#include <log/log.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#define OFFLINELOG_ERROR(fmt, args...) \
    ALOGE("%s %s():%d " fmt "\n", GetFileName(__FILE__), __func__, __LINE__, ##args)
#define OFFLINELOG_INFO(fmt, args...) \
    ALOGI("%s %s():%d " fmt "\n", GetFileName(__FILE__), __func__, __LINE__, ##args)

#if defined(__clang__) && defined(__has_attribute) && __has_attribute(no_sanitize)
#define OFFLINELOG_CFI_NO_SANITIZE __attribute__((no_sanitize("cfi")))
#else
#define OFFLINELOG_CFI_NO_SANITIZE
#endif // defined(__clang__) && defined(__has_attribute) && __has_attribute(no_sanitize)

#include <time.h>
#include <unistd.h>
#include <utils/CallStack.h>

#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>

#include "MiDebugUtils.h"

#undef LOG_TAG
#define LOG_TAG "MiCamDebug"

namespace midebug {

static const char *GetFileName(const char *pFilePath)
{
    const char *pFileName = strrchr(pFilePath, '/');

    if (NULL != pFileName) {
        pFileName += 1;
    } else {
        pFileName = pFilePath;
    }

    return pFileName;
}

enum class OfflineLoggerStatus {
    ACTIVE,         ///< Offlinelogger is active mode
    CAMERA_CLOSING, ///< Offlinelogger is a special mode while camera close.
    DISABLE         ///< Offlinelogger is disable
};

/// @brief Per thread queue data structure
///        being passed into offlinelogger container
struct PerLogThreadInfo
{
    bool bNeedFlush;           ///< When queue reaches flush threshold, set this flag to true
    bool bHasFileName;         ///< If this thread had already created file successfully
    uint32_t threadID;         ///< Thread ID this info structure associated with
    int32_t registerID;        ///< Queue ID registered within memory pool
    uint32_t sessionCount;     ///< Session count corresponding to this buffer
    uint32_t numByteToWrite;   ///< The buffer size need to be written to file
    string offlinelogFileName; ///< Thread's unique filename for filesystem operation
    uint32_t currentLogSize;   ///< The log file size for the current local thread
};

/// @brief Per thread information data structure
///        being passed for tracking thread lifetime in offlinelogger
struct LifetimeThreadInfo
{
    uint32_t threadID;  ///< Thread ID this info structure associated with
    int32_t registerID; ///< Memory container ID registered within offlinelogger

    ~LifetimeThreadInfo();
};

static const char ConfigFileDirectory[] = "/data/vendor/camera/offlinelog";
static const uint32_t MaxOfflineLogNums = 10;

typedef void (*PFNSIGNALOFFLINETHREAD)();

class MiDegbugOfflineLogger
{
public:
    static const uint32_t MaxTimeStampLength = 50;

    static const uint32_t MaxPathLength = 128;

    static const uint32_t ReFlushRateToFile = 200;

    static MiDegbugOfflineLogger *getInstance();

    void addLog(const char *pLog, uint32_t logSize);

    int flushLogToFile(bool forceFlush = false);

    int32_t requestNewLogQueue(uint8_t *&newQueue, uint32_t tid);

    void requestFlushLogQueue(int32_t registerId);

    int start();

    void stop();

    bool isEnableOfflinelogger();

    bool setFlushTriggerLink(PFNSIGNALOFFLINETHREAD pConditionVar);

    void activeOfflineLogger();

private:
    friend class MiDebugInterface; // for accessing private constructor

    /// Constructor
    MiDegbugOfflineLogger();
    /// Destructor
    ~MiDegbugOfflineLogger();

    string getPrefixFilename();

    void setAllToDefaultValue();

    int allocateLogQueueMemPool();

    float getAvailableSpace(const char *path);

    string getCurrentTime();

    /// Do not support the copy constructor or assignment operator
    MiDegbugOfflineLogger(const MiDegbugOfflineLogger &rOfflineLogger) = delete;
    MiDegbugOfflineLogger &operator=(const MiDegbugOfflineLogger &rOfflineLogger) = delete;

    static thread_local uint8_t *sThreadLocalLogBuffer;  ///< Log buffer container
    static thread_local int32_t sThreadLocalQueuePoolId; ///< Buffer container register id
    static thread_local uint32_t sThreadLocalSessionCnt; ///< Camera reopen count
    static thread_local uint32_t sThreadLocalLogSize;    ///< Log size for current thread
    static thread_local uint32_t sThreadLocalLogCurPos;  ///< Current buffer cursor location
    static thread_local bool sThreadLocalFlushDone;      ///< To store if force flush happened
    static thread_local LifetimeThreadInfo sThreadLocalLifeTime; ///< Tracking for thread lifetime

    vector<uint8_t *> mMemoryLogPool;         ///< log container memory pool
    vector<PerLogThreadInfo> mPerLogInfoPool; ///< Each thread register pool
    queue<int32_t> mAvailableQueueIdPool;     ///< Available memory id pool
    queue<int32_t> mAvailableRegisterIdPool;  ///< Available registered id for log container
    queue<int32_t>
        mRegisterIdFlushOrderQueue; ///< Queue to track flush order, so timestamp will be sorted

    string mOfflinelogPreFileName;         ///< Pre-filename for offlinelog
    string mOfflinelogPreLogTypeName;      ///< Pre-logType for offlinelog, eg:ASCII/Binary
    bool mOfflinelogHasPreFileName;        ///< The flag for pre-filename created
    OfflineLoggerStatus mOfflinelogStatus; ///< Offlinelogger status
    uint32_t mSessionCount;                ///< Camera reopen count
    bool mForceFlushDone;                  ///< Force flush done flag
    uint32_t mNeedFlushThreadCount;        ///< Count of threads needing flush
    float mTotalFlushLogSize; ///< Total log size flush to file since boot-up (Unit: MB)
    static pthread_rwlock_t mOfflineLoggerRWLock;
    static recursive_mutex mOfflineLoggerLock; ///< Mutex to protect queue internal thread state
    static mutex mQueueLoadLock;               ///< Mutex to protect queue load thread state

    queue<string> mLogFileNameQueue; ///< Queue to store Log file which is ready to zip
    string mOfflinelogFileName;      ///< Offline log file name
    uint32_t mTotalWritedByte;       ///< Total byte of total thread writed into log file
    uint32_t mOfflineLogNumber;      ///< Total log files
    bool mDeleteLocalLogDone;        ///< Delete local log flag

    PFNSIGNALOFFLINETHREAD
    mSignalFlushTriggerThread; ///< Function pointer to signal flush trigger thread

    bool mSignalRegistered;
    string mImageInfoFileName;
};

} // namespace midebug

#endif
