/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MI_UTIL__
#define __MI_UTIL__

#include <assert.h>
#include <cutils/properties.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <system/camera_metadata.h>
#include <utils/Condition.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>

#include "MiaFmt.h"
#include "MiaPluginUtils.h"
#include "MiaPostProcType.h"
#include "VendorMetadataParser.h"

using namespace android;
using namespace midebug;
namespace mialgo2 {

#define MAX_FACE_NUM               10
#define MAX_WATERMARK_SIZE         64
#define PAD_TO_SIZE(size, padding) (((size) + padding - 1) / padding * padding)

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static const char PathSeparator[] = "/";
static const char SharedLibraryExtension[] = "so";

#if defined(_LP64)
static const char VendorLibPath[] = "/vendor/lib64";
#else
static const char VendorLibPath[] = "/vendor/lib";
#endif // defined(_LP64)

typedef void *MIALIBRARYHANDLE;

#define PARAM_MAP_SIZE(MAP) (sizeof(MAP) / sizeof(MAP[0]))

typedef enum { MI_NO_BLOCK = 1, MI_BLOCK } mia_block_t;

typedef enum { MI_NO_WAKEUP = 1, MI_WAKEUP } mia_wake_t;

/// @brief Date time structure
struct MiaDateTime
{
    uint32_t seconds;                ///< Seconds after the minute 0-61*
    uint32_t microseconds;           ///< Microseconds 0-999999
    uint32_t minutes;                ///< Time in minutes 0-59
    uint32_t hours;                  ///< Time in hours 0-23
    uint32_t dayOfMonth;             ///< Day of month 1-31
    uint32_t month;                  ///< Months since January 0-11, where 0 is January
    uint32_t year;                   ///< Year since 1900
    uint32_t weekday;                ///< Day of week 0-6
    uint32_t yearday;                ///< Day of year 0-365
    uint32_t dayLightSavingTimeFlag; ///< Day light saving time flag
    int64_t gmtOffset;               ///< Offset from GMT
};
class MiaUtil
{
public:
    static CDKResult calcOffset(MiaImageFormat format, PaddingInfo *padding,
                                LenOffsetInfo *bufPlanes);

    static bool copyFrame(MiaFrame *input, MiaFrame *output);
    static double nowMSec();
    static uint64_t getAvailMemory();
    static bool isDumpVisible();

    static inline int sNPrintF(char *dst, size_t sizeDst, const char *format, ...)
    {
        int numCharWritten;
        va_list args;

        va_start(args, format);
        numCharWritten = vSNPrintF(dst, sizeDst, format, args);
        va_end(args);

        return numCharWritten;
    }

    static int vSNPrintF(char *dst, size_t sizeDst, const char *format, va_list argPtr);

    static MIALIBRARYHANDLE libMap(const char *libraryName, const char *libraryPath);

    static MIALIBRARYHANDLE libMap(const char *libraryName);

    static CDKResult libUnmap(MIALIBRARYHANDLE hLibrary);

    static MIALIBRARYHANDLE libGetAddr(MIALIBRARYHANDLE hLibrary, const char *procName);

    static uint16_t floatToHalf(float f);

    static void getDateTime(MiaDateTime *pDateTime);
};

template <typename T>
class Singleton
{
public:
    template <typename... Args>
    static T *Instance(Args &&... args)
    {
        if (m_pInstance == nullptr)
            m_pInstance = new T(std::forward<Args>(args)...);
        return m_pInstance;
    }

    static T *getInstance()
    {
        if (m_pInstance == nullptr) {
            MLOGE(Mia2LogGroupCore,
                  "the instance is not init,please initialize the instance first");
            return nullptr;
        }
        return m_pInstance;
    }

    static void DestroyInstance()
    {
        delete m_pInstance;
        m_pInstance = nullptr;
    }

private:
    Singleton(void);
    virtual ~Singleton(void);
    Singleton(const Singleton &);
    Singleton &operator=(const Singleton &);

private:
    static T *m_pInstance;
};

template <class T>
T *Singleton<T>::m_pInstance = nullptr;

} // namespace mialgo2
#endif // END define __MI_UTIL__
