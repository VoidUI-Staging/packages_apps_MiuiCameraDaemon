#ifndef __LOG_UTIL__
#define __LOG_UTIL__

#include <assert.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
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
#include <utils/String8.h>
#include <utils/Vector.h>

#include "MiaInterface.h"

using namespace android;

namespace mialgo {

#define MAX_FACE_NUM               10
#define MAX_WATERMARK_SIZE         64
#define PAD_TO_SIZE(size, padding) (((size) + padding - 1) / padding * padding)

#define CLEAR(x) memset(&(x), 0, sizeof(x))

typedef void *MIALIBRARYHANDLE;

#undef LOG_TAG
#define LOG_TAG "MiaInterface"

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

// MIA LOG Utils
extern uint32_t gMiaLogLevel;
#define MLOGD(fmt, args...)                                                              \
    ({                                                                                   \
        if (gMiaLogLevel <= 3)                                                           \
            ALOGD("%s:%d %s():" fmt, GetFileName(__FILE__), __LINE__, __func__, ##args); \
    })
#define MLOGI(fmt, args...)                                                              \
    {                                                                                    \
        if (gMiaLogLevel <= 4)                                                           \
            ALOGI("%s:%d %s():" fmt, GetFileName(__FILE__), __LINE__, __func__, ##args); \
    }
#define MLOGW(fmt, args...)                                                              \
    ({                                                                                   \
        if (gMiaLogLevel <= 5)                                                           \
            ALOGW("%s:%d %s():" fmt, GetFileName(__FILE__), __LINE__, __func__, ##args); \
    })
#define MLOGE(fmt, args...)                                                              \
    ({                                                                                   \
        if (gMiaLogLevel <= 6)                                                           \
            ALOGE("%s:%d %s():" fmt, GetFileName(__FILE__), __LINE__, __func__, ##args); \
    })

#define MIA_IF_LOGE_RETURN_RET(cond, ret, fmt, args...)                                  \
    do {                                                                                 \
        if (cond) {                                                                      \
            ALOGE("%s:%d %s():" fmt, GetFileName(__FILE__), __LINE__, __func__, ##args); \
            return ret;                                                                  \
        }                                                                                \
    } while (0)

#define XM_CHECK_NULL_POINTER(condition, val)                                          \
    do {                                                                               \
        if (!(condition)) {                                                            \
            ALOGE("[%s:%d]: ERR! DO NOTHING DUE TO NULL POINTER", __func__, __LINE__); \
            return val;                                                                \
        }                                                                              \
    } while (0)

#define PARAM_MAP_SIZE(MAP) (sizeof(MAP) / sizeof(MAP[0]))

#define DUMP_FILE_PATH   "/sdcard/DCIM/Camera/"
#define FILE_PATH_LENGTH 256

} // namespace mialgo
#endif // END define __LOG_UTIL__