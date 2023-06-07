#ifndef __H_MI_UTIL_H__
#define __H_MI_UTIL_H__

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/List.h>
#include <dlfcn.h>
#include "MiaInterface.h"

using namespace android;

namespace mialgo {

#define DUMP_FILE_PATH "/sdcard/DCIM/Camera/"
#define PAD_TO_SIZE(size, padding)  (((size) + padding - 1) / padding * padding)

#define XM_CHECK_NULL_POINTER(condition, val) do            \
    {                                                       \
       if (!(condition))                                    \
       {                                                    \
           ALOGE("[%s:%d]: ERR! DO NOTHING DUE TO NULL POINTER", __func__, __LINE__);    \
           return val;                                      \
       }                                                    \
    } while (0)

#define XM_CHECK_NULL_POINTER_AND_RELEASE_RESOURCE(condition, handle, val) do            \
    {                                                       \
       if (!(condition))                                    \
       {                                                    \
           dlclose(handle);                                 \
           handle = NULL;                                   \
           ALOGE("[%s:%d]: ERR! DO NOTHING DUE TO NULL POINTER", __func__, __LINE__);    \
           return val;                                      \
       }                                                    \
    } while (0)

struct MiImageBuffer
{
    int PixelArrayFormat;
    int Width;
    int Height;
    int Pitch[MiFormatsMaxPlanes];
    unsigned char *Plane[MiFormatsMaxPlanes];
};

class MiaUtil {
public:
    static void MiDumpBuffer(MiImageList_p buffer, const char *file);
    static void MiDumpBuffer(struct MiImageBuffer *buffer, const char *file);
    static bool DumpToFile(struct MiImageBuffer *buffer, const char*  file_name);
};

}

#endif //END define __H_MI_UTIL_H__
