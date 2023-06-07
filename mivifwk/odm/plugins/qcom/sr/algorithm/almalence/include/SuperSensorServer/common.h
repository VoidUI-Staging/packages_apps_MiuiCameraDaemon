#ifndef __REALTIMESUPERSENSOR_COMMON_H__
#define __REALTIMESUPERSENSOR_COMMON_H__

#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#define TAG     "Almalence"
#define VERSION 3

#if defined(__ANDROID__)
#include <android/log.h>
#else
enum {
    ANDROID_LOG_UNKNOWN = 0,
    ANDROID_LOG_DEFAULT,
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
    ANDROID_LOG_FATAL,
    ANDROID_LOG_SILENT,
};
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

namespace almalence {

extern int logLevel;
extern char logTag[32];

void setLogLevel(int level);
void setLogTag(const char *tag);
int getLogLevel();

template <int P>
void Log(const char *const format, ...)
{
#if defined(PRINT_LOGS_ADB) || defined(PRINT_LOGS_STDOUT)
    va_list args;
    va_start(args, format);
#endif

#ifdef PRINT_LOGS_ADB
    if (P >= logLevel) {
        __android_log_vprint(P, logTag, format, args);
    }
#endif

#ifdef PRINT_LOGS_STDOUT
    if (P >= logLevel) {
        vprintf(format, args);
        printf("\n");
    }
#endif

#if defined(PRINT_LOGS_ADB) || defined(PRINT_LOGS_STDOUT)
    va_end(args);
#endif
}
#pragma clang diagnostic pop

} // namespace almalence

#if defined(__ANDROID__)
namespace std {
template <typename T>
inline static std::string to_string(const T value)
{
    std::ostringstream os;
    os << value;
    return os.str();
}
} // namespace std
#endif

namespace almalence {

inline static void setCurrentThreadAffinityMask(int mask)
{
#ifdef __ANDROID__
    uint32_t mask_old[2];
    syscall(__NR_sched_getaffinity, gettid(), sizeof(mask_old), mask_old);
    Log<ANDROID_LOG_INFO>("pID: %d, Affinity mask: 0x%x", gettid(), mask_old[0]);

    int err, syscallres;
    pid_t pid = gettid();
    syscallres = syscall(__NR_sched_setaffinity, pid, sizeof(mask), &mask);
    if (syscallres) {
        err = errno;
        Log<ANDROID_LOG_ERROR>("Error in the syscall setaffinity: mask=%d=0x%x err=%d=0x%x", mask,
                               mask, err, err);
    }

    uint32_t mask_new[2];
    syscall(__NR_sched_getaffinity, gettid(), sizeof(mask_new), mask_new);
    Log<ANDROID_LOG_INFO>("Affinity mask: 0x%x", mask_new[0]);
#endif
}

inline static unsigned long long currentTimeInMilliseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (((unsigned long)tv.tv_sec * (unsigned)1000) +
            ((unsigned long)tv.tv_usec / (unsigned)1000));
}

static inline int64_t get_time_usec()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec * (int64_t)1000000 + time.tv_usec;
}

template <uint32_t origin, bool up, typename T>
inline static T adjustdim(const T x)
{
    return (x / origin + (up ? 1 : -1) * (x % origin != 0)) * origin;
}

} // namespace almalence

#endif //__REALTIMESUPERSENSOR_COMMON_H__
