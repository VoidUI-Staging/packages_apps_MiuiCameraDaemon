/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MIDEBUG_UTILS__
#define __MIDEBUG_UTILS__

#include <cutils/properties.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utils/Log.h>

#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <sstream>
#include <thread>

#include "MiCallStackUtils.h"
#include "MiCamTrace.h"

#undef LOG_TAG
#define LOG_TAG "MiAlgoEngine"

namespace midebug {
using namespace std;

// The group tag for a given debug print message
typedef uint32_t Mia2LogGroup;

static const Mia2LogGroup Mia2LogGroupCore = (1 << 0);      ///< algofwk Core
static const Mia2LogGroup Mia2LogGroupPlugin = (1 << 1);    ///< algofwk plugin
static const Mia2LogGroup Mia2LogGroupMeta = (1 << 2);      ///< metadata
static const Mia2LogGroup Mia2LogGroupService = (1 << 3);   ///< service
static const Mia2LogGroup Mia2LogGroupDebug = (1 << 4);     ///< dedug
static const Mia2LogGroup LogGroupCore = (1 << 5);          ///< mihal core
static const Mia2LogGroup LogGroupHAL = (1 << 6);           ///< mihal
static const Mia2LogGroup LogGroupRT = (1 << 7);            ///< mihal realtime
static const Mia2LogGroup Mia2LogGroupAlgorithm = (1 << 8); //< algofwk algorithm
static const Mia2LogGroup Mia2LogGroupCallStack = (1 << 9); ///< callStack dump

extern uint32_t gMiCamLogLevel;
extern uint32_t gMiCamLogGroup;
extern uint32_t gMiCamDebugMask;

inline const char *getIndentation(int indentation)
{
    static const char *indents[] = {"", "\t", "\t\t", "\t\t\t"};

    int index = (indentation + 3) / 4;
    index = index > 3 ? 3 : index;

    return indents[index];
}

class Log
{
public:
    static const uint32_t MaxLogLength = 1024;

    static const char *getFileName(const char *pFilePath);
    static void updateLogInfo();
    static const char *miaGroupToString(Mia2LogGroup group);
    static const char *miaGroupToTag(Mia2LogGroup group);
    static void logSystem(uint32_t group, const char *level, const char *pFileName,
                          const char *pFuncName, int32_t lineNum, const char *format, ...);

    static void logSystem(uint32_t group, const char *level, const char *format, ...);

    template <typename... Args>
    static void writeLogToFile(Mia2LogGroup group, const char *level, const char *format,
                               Args &&... args)
    {
        Log::logSystem(group, level, format, args...);

        if (group & gMiCamLogGroup) {
            char strBuffer[midebug::Log::MaxLogLength];
            std::stringstream sstream;
            size_t loglen = 0;

            loglen = snprintf(strBuffer, midebug::Log::MaxLogLength - 1, "%s ",
                              Log::miaGroupToString(group));
            std::snprintf(strBuffer + loglen, midebug::Log::MaxLogLength - loglen, format, args...);
            std::string logStreamBuf(strBuffer);
            sstream << logStreamBuf;

            if (!strcmp(level, "E") && (gMiCamLogLevel <= 6)) {
                __android_log_print(ANDROID_LOG_ERROR, Log::miaGroupToString(group),
                                    sstream.str().c_str());
            } else if (!strcmp(level, "W") && (gMiCamLogLevel <= 5)) {
                __android_log_print(ANDROID_LOG_WARN, Log::miaGroupToString(group),
                                    sstream.str().c_str());
            } else if (!strcmp(level, "I") && (gMiCamLogLevel <= 4)) {
                __android_log_print(ANDROID_LOG_INFO, Log::miaGroupToString(group),
                                    sstream.str().c_str());
            } else if (!strcmp(level, "D") && (gMiCamLogLevel <= 3)) {
                __android_log_print(ANDROID_LOG_DEBUG, Log::miaGroupToString(group),
                                    sstream.str().c_str());
            } else if (!strcmp(level, "V") && (gMiCamLogLevel <= 2)) {
                __android_log_print(ANDROID_LOG_VERBOSE, Log::miaGroupToString(group),
                                    sstream.str().c_str());
            }
        }
    }

private:
    Log() = default;
    Log(const Log &) = delete;
    Log &operator=(const Log &) = delete;
};

class MiDebugInterface;

#define MASSERT(condition)                                        \
    ({                                                            \
        if (!(condition)) {                                       \
            MiDebugInterface::getInstance()->stopOfflineLogger(); \
            std::abort();                                         \
        }                                                         \
    })

#define LOG_SYSTEM(group, loglevel, fmt, args...)                                                \
    ({                                                                                           \
        if (gMiCamDebugMask > 0)                                                                 \
            Log::logSystem(group, loglevel, Log::getFileName(__FILE__), __func__, __LINE__, fmt, \
                           ##args);                                                              \
        else if (!strcmp(loglevel, "E"))                                                         \
            Log::logSystem(group, loglevel, Log::getFileName(__FILE__), __func__, __LINE__, fmt, \
                           ##args);                                                              \
    })

#define MLOGV(group, fmt, args...)                                                               \
    ({                                                                                           \
        if (gMiCamLogLevel <= 2 && (group & gMiCamLogGroup))                                     \
            ALOGD("%s %s:%d %s()" fmt, Log::miaGroupToString(group), Log::getFileName(__FILE__), \
                  __LINE__, __func__, ##args);                                                   \
        if (group & gMiCamLogGroup)                                                              \
            LOG_SYSTEM(group, "D", fmt, ##args);                                                 \
    })

#define MLLOGV(group, str, fmt, args...)                                                           \
    ({                                                                                             \
        if (gMiCamLogLevel <= 2 && (group & gMiCamLogGroup)) {                                     \
            ALOGD("%s %s:%d %s()> " fmt, Log::miaGroupToString(group), Log::getFileName(__FILE__), \
                  __LINE__, __func__, ##args);                                                     \
            for (int start = 0; start < str.size();) {                                             \
                size_t pos = str.find('\n', start + 500);                                          \
                pos = (pos == str.npos ? start + 500 : pos);                                       \
                ALOGD("%s", str.substr(start, pos - start).c_str());                               \
                start = pos + 1;                                                                   \
            }                                                                                      \
        }                                                                                          \
    })

#define MLOGD(group, fmt, args...)                                                               \
    ({                                                                                           \
        if (gMiCamLogLevel <= 3 && (group & gMiCamLogGroup))                                     \
            ALOGD("%s %s:%d %s()" fmt, Log::miaGroupToString(group), Log::getFileName(__FILE__), \
                  __LINE__, __func__, ##args);                                                   \
        if (group & gMiCamLogGroup)                                                              \
            LOG_SYSTEM(group, "D", fmt, ##args);                                                 \
    })

#define MLLOGD(group, str, fmt, args...)                                                           \
    ({                                                                                             \
        if (gMiCamLogLevel <= 3 && (group & gMiCamLogGroup)) {                                     \
            ALOGD("%s %s:%d %s()> " fmt, Log::miaGroupToString(group), Log::getFileName(__FILE__), \
                  __LINE__, __func__, ##args);                                                     \
            for (int start = 0; start < str.size();) {                                             \
                size_t pos = str.find('\n', start + 500);                                          \
                pos = (pos == str.npos ? start + 500 : pos);                                       \
                ALOGD("%s", str.substr(start, pos - start).c_str());                               \
                start = pos + 1;                                                                   \
            }                                                                                      \
        }                                                                                          \
    })

#define MLOGI(group, fmt, args...)                                                               \
    ({                                                                                           \
        if (gMiCamLogLevel <= 4 && (group & gMiCamLogGroup))                                     \
            ALOGI("%s %s:%d %s()" fmt, Log::miaGroupToString(group), Log::getFileName(__FILE__), \
                  __LINE__, __func__, ##args);                                                   \
        if (group & gMiCamLogGroup)                                                              \
            LOG_SYSTEM(group, "I", fmt, ##args);                                                 \
    })

#define MLLOGI(group, str, fmt, args...)                                                           \
    ({                                                                                             \
        if (gMiCamLogLevel <= 4 && (group & gMiCamLogGroup)) {                                     \
            ALOGI("%s %s:%d %s()> " fmt, Log::miaGroupToString(group), Log::getFileName(__FILE__), \
                  __LINE__, __func__, ##args);                                                     \
            for (int start = 0; start < str.size();) {                                             \
                size_t pos = str.find('\n', start + 500);                                          \
                pos = (pos == str.npos ? start + 500 : pos);                                       \
                ALOGI("%s", str.substr(start, pos - start).c_str());                               \
                start = pos + 1;                                                                   \
            }                                                                                      \
        }                                                                                          \
    })

#define MLOGW(group, fmt, args...)                                                               \
    ({                                                                                           \
        if (gMiCamLogLevel <= 5 && (group & gMiCamLogGroup))                                     \
            ALOGW("%s %s:%d %s()" fmt, Log::miaGroupToString(group), Log::getFileName(__FILE__), \
                  __LINE__, __func__, ##args);                                                   \
        if (group & gMiCamLogGroup)                                                              \
            LOG_SYSTEM(group, "W", fmt, ##args);                                                 \
    })

#define MLOGE(group, fmt, args...)                                                               \
    ({                                                                                           \
        if (gMiCamLogLevel <= 6 && (group & gMiCamLogGroup))                                     \
            ALOGE("%s %s:%d %s()" fmt, Log::miaGroupToString(group), Log::getFileName(__FILE__), \
                  __LINE__, __func__, ##args);                                                   \
        if (group & gMiCamLogGroup)                                                              \
            LOG_SYSTEM(group, "E", fmt, ##args);                                                 \
    })

static constexpr char *LogLevelToString[] = {
    "D", // ANDROID_LOG_UNKNOWN = 0,
    "D", // ANDROID_LOG_DEFAULT,
    "D", // ANDROID_LOG_VERBOSE,
    "D", // ANDROID_LOG_DEBUG,
    "I", // ANDROID_LOG_INFO,
    "W", // ANDROID_LOG_WARN,
    "E", // ANDROID_LOG_ERROR,
    "E", // ANDROID_LOG_FATAL,
    "E", // ANDROID_LOG_SILENT,
};
// example: MLOG_PRI(ANDROID_LOG_INFO, Mia2LogGroupDebug, "test");
#define MLOG_PRI(level, group, fmt, args...)                                       \
    do {                                                                           \
        LOG_PRI(level, LOG_TAG, "%s %s:%d %s()" fmt, Log::miaGroupToString(group), \
                Log::getFileName(__FILE__), __LINE__, __func__, ##args);           \
        LOG_SYSTEM(group, LogLevelToString[level], fmt, ##args);                   \
    } while (0)

#define XM_IF_LOGE_RETURN_RET(cond, ret, fmt, args...)                                     \
    do {                                                                                   \
        if (cond) {                                                                        \
            ALOGE("[%s()]:[%d] " fmt "\n", Log::getFileName(__FILE__), __LINE__, __func__, \
                  ##args);                                                                 \
            return ret;                                                                    \
        }                                                                                  \
    } while (0)

#define XM_CHECK_NULL_POINTER(condition, val)                                         \
    do {                                                                              \
        if (!(condition)) {                                                           \
            ALOGE("[%s:%d] ERR! DO NOTHING DUE TO NULL POINTER", __func__, __LINE__); \
            return val;                                                               \
        }                                                                             \
    } while (0)

class MiDebugInterface
{
public:
    static MiDebugInterface *getInstance();

    int startOfflineLogger();

    void stopOfflineLogger();

    void signalOfflineLoggerThread();

    void addLogInterface(const char *log, uint32_t logSize);

    void flushLogToFileInterface(bool lastFlush);

    void notifyPluginError(std::string imageName, std::string pluginName, int errorCode);

    void dumpErrorInfoToFile(const char *errorInfo);

private:
    MiDebugInterface();

    ~MiDebugInterface();

    void requestOfflineLogThreadProcessing();

    bool mStarted;
    bool mOfflineLoggerEnabled;
    bool mAbortEnable;

    bool mTerminateOfflineLogThread;
    std::thread mOfflineLoggerThread;         ///< Offlinelog thread;
    mutable std::mutex mFlushNeededMutex;     ///< If flush needed mutex
    std::condition_variable mFlushNeededCond; ///< OfflineLogger condition variable array
    mutable std::recursive_mutex mOfflineLoggerMutex;
    mutable std::mutex dumpMiviErrorInfoMutex;
};

} // namespace midebug

#endif //__MIDEBUG_UTILS__
