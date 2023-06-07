#ifndef _PLUGINUTILS_
#define _PLUGINUTILS_

#include <cutils/properties.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <cstdio>
#include <list>
#include <string>
#include <utility>

#include "MiDebugUtils.h"

using namespace midebug;
namespace mialgo2 {

#define PAD_TO_SIZE(size, padding)  (((size) + padding - 1) / padding * padding)
#define DOWN_TO_SIZE(size, padding) (((size)-1) / (padding) * (padding))

#define DUMP_MQS_DATA_FILE_PATH "/data/vendor/camera/offlinelog/"
#define DUMP_SDCARD_FILE_PATH   "/sdcard/DCIM/Camera/"
#define DUMP_DATA_FILE_PATH     "/data/vendor/camera/"
#define FILE_PATH_LENGTH        256
#define MAX_FACE_NUM            10

enum CameraCombinationMode {
    CAM_COMBMODE_UNKNOWN = 0x0,
    CAM_COMBMODE_REAR_WIDE = 0x01,
    CAM_COMBMODE_REAR_TELE = 0x02,
    CAM_COMBMODE_REAR_ULTRA = 0x03,
    CAM_COMBMODE_REAR_MACRO = 0x04,
    CAM_COMBMODE_FRONT = 0x11,
    CAM_COMBMODE_FRONT_AUX = 0x12,
    CAM_COMBMODE_REAR_SAT_WT = 0x201,
    CAM_COMBMODE_REAR_SAT_WU = 0x202,
    CAM_COMBMODE_FRONT_SAT = 0x203,
    CAM_COMBMODE_REAR_SAT_T_UT = 0x204,
    CAM_COMBMODE_REAR_BOKEH_WT = 0x301,
    CAM_COMBMODE_REAR_BOKEH_WU = 0x302,
    CAM_COMBMODE_FRONT_BOKEH = 0x303,
    CAM_COMBMODE_REAR_SAT_UW_W = 0x304,
};

static const char PluginPathSeparator[] = "/";
static const char PluginSharedLibraryExtension[] = "so";

static const int32_t MaxPlanes = 3;
static const int32_t MaxFd = 4;

enum MiaPixelFormat {
    CAM_FORMAT_UNDEFINED = 0x0, // for pipeline check, not a useful format
    CAM_FORMAT_YUV_420_NV21 = 17,
    CAM_FORMAT_RAW16 = 32,
    CAM_FORMAT_BLOB = 33,
    CAM_FORMAT_IMPLEMENTATION_DEFINED = 34,
    CAM_FORMAT_YUV_420_NV12 = 35,
    CAM_FORMAT_RAW_OPAQUE = 36,
    CAM_FORMAT_RAW10 = 37,
    CAM_FORMAT_RAW12 = 38,
    CAM_FORMAT_Y16 = 540422489,
    CAM_FORMAT_YV12 = 842094169,
    CAM_FORMAT_Y8 = 0x20203859,
    CAM_FORMAT_P010 = 54, // ImageFormat's P010 0x3
    CAM_FORMAT_JPEG = 8960,
};

struct MiImageBuffer
{
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t scanline;
    uint32_t numberOfPlanes;
    uint8_t *plane[MaxPlanes];
    int fd[MaxFd];
    int planeSize[MaxPlanes];
};

struct MiAbnormalDetect
{
    uint32_t isAbnormal; // 0-normal,1-row abormal,2-col abormal
    uint32_t level;      // 0-low,1-high
    uint32_t line;
    uint32_t format;
    uint32_t plane0_value;
    uint32_t plane1_value;
};

struct MiAbnormalParam
{
    uint8_t **addr;
    uint32_t format;
    uint32_t stride;
    uint32_t width;
    uint32_t height;
    uint32_t frameNum;
    int64_t timeStamp;
    uint8_t isSourceBuffer;
};

/// @brief File Name Token
enum class FileNameToken {
    Com,      ///< com
    Vendor,   ///< vendor name
    Category, ///< category name
    Module,   ///< module name
    Extension,
    Max ///< Max tokens for file name
};

enum {
    MIA_RETURN_OK = 0,
    MIA_RETURN_UNKNOWN_ERROR = (-2147483647 - 1), // int32_t_MIN value
    MIA_RETURN_INVALID_PARAM,
    MIA_RETURN_NO_MEM,
    MIA_RETURN_MAP_FAILED,
    MIA_RETURN_UNMAP_FAILED,
    MIA_RETURN_OPEN_FAILED,
    MIA_RETURN_INVALID_CALL,
    MIA_RETURN_UNABLE_LOAD
};

typedef enum {
    GRALLOC_TYPE = 0,
    ION_TYPE,
} buffer_type;

typedef enum {
    PluginReleaseInputBuffer,
    PluginReleaseOutputBuffer,
} PluginReleaseBuffer;

#define PAD_TO_SIZE(size, padding) (((size) + padding - 1) / padding * padding)

class PluginUtils
{
public:
    static bool dumpToFile(const char *fileName, struct MiImageBuffer *miBuf);
    static void dumpToFile(struct MiImageBuffer *imageBuffer, const char *fileName);
    static bool dumpStrToFile(const char *fileName, const char *fmt, ...);
    static int miAllocBuffer(struct MiImageBuffer *buffer, int width, int height, int format);
    static void miFreeBuffer(struct MiImageBuffer *buffer);
    static void miDumpBuffer(struct MiImageBuffer *buffer, const char *file);
    static int miCopyBuffer(struct MiImageBuffer *hOutput, struct MiImageBuffer *hInput);
    static void miCopyPlane(unsigned char *dst, unsigned char *src, int width, int height,
                            int dstStride, int srcStride);
    static void miAllocCopyBuffer(struct MiImageBuffer *dstBuffer, struct MiImageBuffer *srcBuffer);
    static void miReSize(struct MiImageBuffer *dstImage, struct MiImageBuffer *srcImage);
    static double nowMSec();
    static bool isDumpVisible();
    static MiAbnormalDetect isAbnormalImage(uint8_t **addr, uint32_t format, uint32_t stride,
                                            uint32_t width, uint32_t height);
    template <typename T>
    static MiAbnormalDetect isAbnormalSampledImage(T *addr0, void *addr1, uint32_t step,
                                                   uint32_t stride, uint32_t width,
                                                   uint32_t height);
    template <typename T>
    static bool isAbnormalSampledPlane(T *addr, uint32_t step, uint32_t stride, uint32_t width,
                                       uint32_t height, uint32_t indexType, uint32_t start,
                                       uint32_t end);
    static unsigned int getFilesFromPath(const char *fileSearchPath, size_t maxFileNameLength,
                                         std::list<std::string> &fileNamesList,
                                         const char *vendorName, const char *categoryName,
                                         const char *moduleName, const char *extension);

    static int vSNPrintF(char *dst, size_t sizeDst, const char *format, va_list argPtr);
    static void *libMap(const char *libraryName, const char *libraryPath);
    static void *libMap(const char *libraryName);
    static int libUnmap(void *hLibrary);
    static void *libGetAddr(void *hLibrary, const char *procName);
    static uint16_t floatToHalf(float f);
    static int isLowCamLevel();
    static uint64_t getTotalRam();

    static const char *getDumpPath();

    static inline int sNPrintF(char *dst, size_t sizeDst, const char *format, ...)
    {
        int numCharWritten;
        va_list args;

        va_start(args, format);
        numCharWritten = vSNPrintF(dst, sizeDst, format, args);
        va_end(args);

        return numCharWritten;
    }
};

} // namespace mialgo2
#endif // _PLUGINUTILS_
