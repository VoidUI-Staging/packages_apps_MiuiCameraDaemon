#include "MiaPluginUtils.h"

#include <dirent.h>
#include <math.h>
#include <sys/sysinfo.h>
#include <utils/String8.h>

namespace mialgo2 {

static const int32_t sIsDebugMode = property_get_int32("persist.vendor.camera.algoengine.debug", 1);

static void MiDumpPlane(FILE *fp, unsigned char *plane, int width, int height, int pitch)
{
    for (int i = 0; i < height; i++) {
        fwrite(plane, width, 1, fp);
        plane += pitch;
    }
}

int PluginUtils::isLowCamLevel()
{
    static int g_isLowCamLevel = -1;
    if (g_isLowCamLevel == -1) {
        g_isLowCamLevel = property_get_int32("ro.boot.camera.config", 0); // 0--pro 1--low
    }
    return g_isLowCamLevel;
}

uint64_t PluginUtils::getTotalRam()
{
    uint64_t g_totalRam = 0;
    if (!g_totalRam) {
        struct sysinfo pInfo;
        int ccode = sysinfo(&pInfo);
        if (0 != ccode) {
            MLOGE(Mia2LogGroupCore, "sysinfo failed!");
            return 0;
        }
        g_totalRam = (uint64_t)pInfo.totalram * pInfo.mem_unit;
        MLOGD(Mia2LogGroupCore, "totalram = %ld, freeram = %ld mem_unit = %d totalRamSize %" PRIu64,
              pInfo.totalram, pInfo.freeram, pInfo.mem_unit, g_totalRam);
    }
    return g_totalRam;
}

const char *PluginUtils::getDumpPath()
{
    if (!access(DUMP_SDCARD_FILE_PATH, F_OK)) {
        return DUMP_SDCARD_FILE_PATH;
    } else {
        return DUMP_DATA_FILE_PATH;
    }
}

bool PluginUtils::dumpToFile(const char *fileName, struct MiImageBuffer *imageBuffer)
{
    if (imageBuffer == NULL)
        return -1;

    char buf[256];
    char timeBuf[256];
    time_t currentTime;
    struct tm *timeInfo;
    static android::String8 lastFileName("");
    static int k = 0;

    time(&currentTime);
    timeInfo = localtime(&currentTime);
    memset(buf, 0, sizeof(buf));
    strftime(timeBuf, sizeof(timeBuf), "IMG_%Y%m%d%H%M%S", timeInfo);
    if ((strstr(fileName, "MIMQS") != NULL) && (!access(DUMP_MQS_DATA_FILE_PATH, F_OK))) {
        snprintf(buf, sizeof(buf), "%s%s_%s", DUMP_MQS_DATA_FILE_PATH, timeBuf, fileName);
    } else {
        snprintf(buf, sizeof(buf), "%s%s_%s", getDumpPath(), timeBuf, fileName);
    }
    android::String8 filePath(buf);
    const char *suffix = "yuv";
    if (imageBuffer->format == CAM_FORMAT_RAW10 || imageBuffer->format == CAM_FORMAT_RAW16) {
        suffix = "raw";
    } else if (imageBuffer->format == CAM_FORMAT_BLOB) {
        suffix = "jpg";
    }
    if (!filePath.compare(lastFileName)) {
        snprintf(buf, sizeof(buf), "_%d.%s", k++, suffix);
        MLOGD(Mia2LogGroupCore, "%s %s%s", __func__, filePath.string(), buf);
    } else {
        MLOGD(Mia2LogGroupCore, "%s diff %s %s", __func__, filePath.string(),
              lastFileName.string());
        k = 0;
        lastFileName = filePath;
        snprintf(buf, sizeof(buf), ".%s", suffix);
    }

    filePath.append(buf);
    PluginUtils::miDumpBuffer(imageBuffer, filePath);
    return 0;
}

void PluginUtils::dumpToFile(struct MiImageBuffer *imageBuffer, const char *fileName)
{
    if (imageBuffer == NULL)
        return;

    char buf[256];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "/data/vendor/camera/MIVI_%s", fileName);

    android::String8 filePath(buf);
    const char *suffix = "yuv";
    if (imageBuffer->format == CAM_FORMAT_RAW10 || imageBuffer->format == CAM_FORMAT_RAW16) {
        suffix = "raw";
    }
    if (imageBuffer->format == CAM_FORMAT_BLOB) {
        suffix = "jpg";
    }

    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), ".%s", suffix);

    filePath.append(buf);
    PluginUtils::miDumpBuffer(imageBuffer, filePath);
}

bool PluginUtils::dumpStrToFile(const char *fileName, const char *fmt, ...)
{
    char buf[128];
    char timeBuf[128];
    time_t currentTime;
    struct tm *timeInfo;
    static android::String8 lastFileName("");
    static int k = 0;

    time(&currentTime);
    timeInfo = localtime(&currentTime);
    memset(buf, 0, sizeof(buf));
    strftime(timeBuf, sizeof(timeBuf), "IMG_%Y%m%d%H%M%S", timeInfo);
    snprintf(buf, sizeof(buf), "%s%s_%s", getDumpPath(), timeBuf, fileName);
    android::String8 filePath(buf);
    const char *suffix = "log";
    if (!filePath.compare(lastFileName)) {
        snprintf(buf, sizeof(buf), "_%d.%s", k++, suffix);
        MLOGD(Mia2LogGroupCore, "%s %s%s", __func__, filePath.string(), buf);
    } else {
        MLOGD(Mia2LogGroupCore, "%s diff %s %s", __func__, filePath.string(),
              lastFileName.string());
        k = 0;
        lastFileName = filePath;
        snprintf(buf, sizeof(buf), ".%s", suffix);
    }

    filePath.append(buf);
    FILE *pf = fopen(filePath.c_str(), "wb");
    if (pf) {
        va_list ap;
        char buf[PROPERTY_VALUE_MAX] = {0};
        va_start(ap, fmt);
        vsnprintf(buf, PROPERTY_VALUE_MAX, fmt, ap);
        va_end(ap);
        fwrite(buf, strlen(buf) * sizeof(char), sizeof(char), pf);
        fclose(pf);
    }
    return 0;
}

int PluginUtils::miAllocBuffer(struct MiImageBuffer *buffer, int width, int height, int format)
{
    if (format == CAM_FORMAT_YUV_420_NV21 || format == CAM_FORMAT_YUV_420_NV12) {
        buffer->plane[0] = (unsigned char *)malloc(width * height * 3 / 2);
        if (NULL == buffer->plane[0])
            return false;
        buffer->plane[1] = buffer->plane[0] + width * height;
        buffer->stride = width;
        buffer->format = format;
        buffer->width = width;
        buffer->height = height;
        return true;
    } else if (format == CAM_FORMAT_Y16) {
        buffer->plane[0] = (unsigned char *)malloc(width * height);
        if (NULL == buffer->plane[0])
            return false;
        buffer->plane[1] = NULL;
        buffer->stride = width;
        buffer->format = format;
        buffer->width = width;
        buffer->height = height;
        return true;
    }
    return false;
}

void PluginUtils::miFreeBuffer(struct MiImageBuffer *buffer)
{
    if (buffer->plane[0]) {
        free(buffer->plane[0]);
        buffer->plane[0] = NULL;
    }
}

void PluginUtils::miDumpBuffer(struct MiImageBuffer *buffer, const char *file)
{
    FILE *p = fopen(file, "wb+");
    chmod(file, 0666);
    if (p != NULL) {
        int width, height;
        if ((buffer->format == CAM_FORMAT_YUV_420_NV21) ||
            (buffer->format == CAM_FORMAT_YUV_420_NV12) ||
            (buffer->format == CAM_FORMAT_IMPLEMENTATION_DEFINED)) {
            width = buffer->width;
            height = buffer->height;
            MiDumpPlane(p, buffer->plane[0], width, height, buffer->stride);
            MiDumpPlane(p, buffer->plane[1], width, height / 2, buffer->stride);
            MLOGD(Mia2LogGroupCore, "MiDump %s width:height = %d * %d\n", file, width, height);
        } else if (buffer->format == CAM_FORMAT_P010) {
            width = buffer->width;
            height = buffer->height;
            MiDumpPlane(p, buffer->plane[0], width * 2, height, buffer->stride);
            MiDumpPlane(p, buffer->plane[1], width * 2, height / 2, buffer->stride);
            MLOGD(Mia2LogGroupCore, "MiDump %s width:height = %d * %d\n", file, width, height);
        } else if (buffer->format == CAM_FORMAT_Y16) {
            width = buffer->width;
            height = buffer->height;
            MLOGD(Mia2LogGroupCore, "MiDump %s width:height = %d * %d\n", file, width, height);
            MiDumpPlane(p, buffer->plane[0], width, height, buffer->stride);
        } else if (buffer->format == CAM_FORMAT_RAW16) {
            width = buffer->width;
            height = buffer->height;
            MLOGD(Mia2LogGroupCore, "MiDump %s width:height = %d * %d stride: %d", file, width,
                  height, buffer->stride);
            MiDumpPlane(p, buffer->plane[0], width * 2, height, buffer->stride);
        } else if (buffer->format == CAM_FORMAT_RAW10) {
            width = buffer->width;
            height = buffer->height;
            MLOGD(Mia2LogGroupCore, "MiDump %s width:height = %d * %d stride: %d", file, width,
                  height, buffer->stride);
            MiDumpPlane(p, buffer->plane[0], (width * 10) / 8, height, buffer->stride);
        } else if (buffer->format == CAM_FORMAT_BLOB) {
            width = buffer->width;
            height = buffer->height;
            MLOGD(Mia2LogGroupCore, "MiDump %s width:height = %d*%d size: %d", file, width, height,
                  buffer->planeSize[0]);
            fwrite(buffer->plane[0], width * height * 2, 1, p);
        } else {
            MLOGE(Mia2LogGroupCore, "%s:%d unknow format: %d", __func__, __LINE__, buffer->format);
        }

        fclose(p);
    }
}

void PluginUtils::miCopyPlane(unsigned char *dst, unsigned char *src, int width, int height,
                              int dstStride, int srcStride)
{
    int i;
    for (i = 0; i < height; i++) {
        memcpy(dst, src, width);
        dst += dstStride;
        src += srcStride;
    }
}

int PluginUtils::miCopyBuffer(struct MiImageBuffer *hOutput, struct MiImageBuffer *hInput)
{
    int ret = 0;
    uint32_t outPlaneStride = hOutput->stride;
    uint32_t outHeight = hOutput->height;
    uint32_t outSliceheight = hOutput->height;
    uint32_t inPlaneStride = hInput->stride;
    uint32_t inHeight = hInput->height;

    uint32_t inSliceheight = hInput->height;
    uint32_t copyPlaneStride = outPlaneStride > inPlaneStride ? inPlaneStride : outPlaneStride;
    uint32_t copyHeight = outHeight > inHeight ? inHeight : outHeight;

    MLOGI(Mia2LogGroupCore,
          "CopyImage input %d, %d, %d, output %d, %d, %d, format 0x%x 0x%x, numberOfPlanes: %d",
          inPlaneStride, inHeight, inSliceheight, outPlaneStride, outHeight, outSliceheight,
          hInput->format, hOutput->format, hInput->numberOfPlanes);

    if ((hOutput->format == CAM_FORMAT_YUV_420_NV21) ||
        (hOutput->format == CAM_FORMAT_YUV_420_NV12) ||
        (hOutput->format == CAM_FORMAT_IMPLEMENTATION_DEFINED) ||
        (hOutput->format == CAM_FORMAT_P010)) {
        for (uint32_t m = 0; m < copyHeight; m++) {
            memcpy((hOutput->plane[0] + outPlaneStride * m), (hInput->plane[0] + inPlaneStride * m),
                   copyPlaneStride);
        }
        for (uint32_t m = 0; m < (copyHeight >> 1); m++) {
            memcpy((hOutput->plane[1] + outPlaneStride * m), (hInput->plane[1] + inPlaneStride * m),
                   copyPlaneStride);
        }
    } else if (hOutput->format == CAM_FORMAT_Y16) {
        for (uint32_t m = 0; m < copyHeight; m++) {
            memcpy((hOutput->plane[0] + outPlaneStride * m), (hInput->plane[0] + inPlaneStride * m),
                   copyPlaneStride);
        }
    } else if (hOutput->format == CAM_FORMAT_RAW10) {
        for (uint32_t m = 0; m < copyHeight; m++) {
            memcpy((hOutput->plane[0] + outPlaneStride * m), (hInput->plane[0] + inPlaneStride * m),
                   copyPlaneStride);
        }
    } else if (hOutput->format == CAM_FORMAT_RAW16) {
        for (uint32_t m = 0; m < copyHeight; m++) {
            memcpy((hOutput->plane[0] + outPlaneStride * m), (hInput->plane[0] + inPlaneStride * m),
                   copyPlaneStride);
        }
    } else {
        MLOGE(Mia2LogGroupCore, "%s:%d unknow format: %d", __func__, __LINE__, hOutput->format);
        ret = -1;
    }

    return ret;
}

void PluginUtils::miAllocCopyBuffer(struct MiImageBuffer *dstBuffer,
                                    struct MiImageBuffer *srcBuffer)
{
    memcpy(dstBuffer, srcBuffer, sizeof(struct MiImageBuffer));
    dstBuffer->plane[0] = (unsigned char *)malloc(dstBuffer->height * dstBuffer->stride * 3 / 2);
    dstBuffer->plane[1] = dstBuffer->plane[0] + dstBuffer->height * dstBuffer->stride;

    unsigned char *src = srcBuffer->plane[0];
    unsigned char *dst = dstBuffer->plane[0];
    for (int h = 0; h < srcBuffer->height; h++) {
        memcpy(dst, src, srcBuffer->width);
        src += srcBuffer->stride;
        dst += dstBuffer->stride;
    }

    src = srcBuffer->plane[1];
    dst = dstBuffer->plane[1];
    for (int h = 0; h < srcBuffer->height / 2; h++) {
        memcpy(dst, src, srcBuffer->width);
        src += srcBuffer->stride;
        dst += dstBuffer->stride;
    }
}

void PluginUtils::miReSize(struct MiImageBuffer *dstImage, struct MiImageBuffer *srcImage)
{
    if ((srcImage->width == dstImage->width) && (srcImage->height == dstImage->height)) {
        miCopyBuffer(dstImage, srcImage);
        return;
    }
    if (srcImage->format == CAM_FORMAT_Y16) {
        unsigned int wrFloat16 = (srcImage->width << 16) / dstImage->width + 1;
        unsigned int hrFloat16 = (srcImage->height << 16) / dstImage->height + 1;
        unsigned char *dstRow = (unsigned char *)dstImage->plane[0];
        unsigned char *srcRow = (unsigned char *)srcImage->plane[0];
        unsigned int srcy16 = 0;
        for (int r = 0; r < (int)dstImage->height; r++) {
            srcRow = (unsigned char *)srcImage->plane[0] + srcImage->stride * (srcy16 >> 16);
            unsigned int srcx16 = 0;
            for (int c = 0; c < (int)dstImage->width; c++) {
                dstRow[c] = srcRow[srcx16 >> 16];
                srcx16 += wrFloat16;
            }
            srcy16 += hrFloat16;
            dstRow += dstImage->stride;
        }
    } else {
        MLOGE(Mia2LogGroupCore, "format not support");
    }
}

double PluginUtils::nowMSec()
{
    struct timeval res;
    gettimeofday(&res, NULL);
    return 1000.0 * res.tv_sec + (double)res.tv_usec / 1e3;
}

bool PluginUtils::isDumpVisible()
{
    if (!access("/sdcard/DCIM/Camera/algoup_dump_images", 0))
        return true;
    else
        return false;
}

MiAbnormalDetect PluginUtils::isAbnormalImage(uint8_t **addr, uint32_t format, uint32_t stride,
                                              uint32_t width, uint32_t height)
{
    MiAbnormalDetect abnDetect;
    memset(&abnDetect, 0, sizeof(MiAbnormalDetect));
    int32_t iIsDumpData = sIsDebugMode;
    if (iIsDumpData == 0) {
        return abnDetect;
    }
    uint32_t step = width > 6016 ? 40 : 20;
    if ((format == CAM_FORMAT_YUV_420_NV12) || (format == CAM_FORMAT_YUV_420_NV21)) {
        abnDetect = isAbnormalSampledImage(addr[0], addr[1], step, stride, width, height);
    } else if (format == CAM_FORMAT_RAW16) {
        if (!(stride % 4) && !(width % 2)) {
            abnDetect = isAbnormalSampledImage((uint32_t *)addr[0], NULL, PAD_TO_SIZE(step, 2),
                                               stride / 4, width / 2, height);
        }
    }
    abnDetect.format = format;
    return abnDetect;
}

template <typename T>
MiAbnormalDetect PluginUtils::isAbnormalSampledImage(T *addr0, void *addr1, uint32_t step,
                                                     uint32_t stride, uint32_t width,
                                                     uint32_t height)
{
    MiAbnormalDetect abnDetect;
    memset(&abnDetect, 0, sizeof(MiAbnormalDetect));
    int row = 0, col = 0;
    if (stride < width) {
        stride = width;
    }
    if (width <= step || height <= step) {
        return abnDetect;
    }
    if (!addr1) {
        MLOGI(Mia2LogGroupCore, "addr0:%p, step:%d, stride:%d, width:%d, height:%d", addr0, step,
              stride, width, height);
    }

    auto isGreenPicture = [&](uint32_t value) {
        const int checkPixel = 12;
        uint8_t buffer[checkPixel];
        memset(buffer, value, checkPixel * sizeof(uint8_t));
        if (!memcmp(buffer, addr0, checkPixel)) {
            if (!memcmp(buffer, addr0 + stride * (height / 2) + width - checkPixel, checkPixel)) {
                return true;
            }
        }
        return false;
    };

    int detectCount = 0;
    const int maxDetectCount = 2;
    uint32_t WH[2] = {width, height};
    uint32_t coeff[2] = {1, stride};
    for (int i = 0; i < 2; ++i) {
        uint32_t coeff1 = coeff[i];
        uint32_t coeff2 = coeff[1 - i];
        uint32_t boundary = DOWN_TO_SIZE(WH[1 - i], step);
        for (row = 0; row < WH[1 - i]; row += step) {
            uint32_t tmp = addr0[row * coeff2];
            for (col = 0; col < WH[i]; col += step) {
                if (addr0[row * coeff2 + col * coeff1] != tmp) {
                    break;
                }
            }
            if (col >= WH[i]) {
                abnDetect.isAbnormal = i + 1;
                abnDetect.line = row + 1;
                abnDetect.plane0_value = tmp;
                bool isAbnormalPlane0 = false, isAbnormalPlane1 = false;
                uint32_t start = row - 2 < 0 ? 0 : row - 2;
                uint32_t end = row + 2 >= WH[1 - i] - 2 ? WH[1 - i] - 2 : row + 2;
                isAbnormalPlane0 =
                    isAbnormalSampledPlane(addr0, step, stride, width, height, i, start, end);
                if (isAbnormalPlane0 && (addr1 != NULL)) {
                    isAbnormalPlane1 =
                        isAbnormalSampledPlane((uint16_t *)addr1, step / 2, stride / 2, width / 2,
                                               height / 2, i, start / 2, end / 2);
                    uint32_t tmp1 =
                        ((uint16_t *)addr1)[row / 2 * (int32_t)(pow(stride / 2, 1 - i))];
                    abnDetect.plane1_value = tmp1;
                    if (isAbnormalPlane1 && (tmp > 250) && (tmp1 >= 0x7f7f) && (tmp1 <= 0x8282)) {
                        isAbnormalPlane1 = isGreenPicture(tmp);
                    }
                } else if (isAbnormalPlane0) {
                    abnDetect.level = isGreenPicture(tmp);
                    return abnDetect;
                }
                if (isAbnormalPlane0 && isAbnormalPlane1) {
                    abnDetect.level = 1;
                    return abnDetect;
                }
                if (++detectCount == maxDetectCount) {
                    return abnDetect;
                }
            }
            if (row == boundary && (row != WH[1 - i] - 1)) {
                row = WH[1 - i] - step - 1;
            }
        }
    }
    return abnDetect;
}

template <typename T>
bool PluginUtils::isAbnormalSampledPlane(T *addr, uint32_t step, uint32_t stride, uint32_t width,
                                         uint32_t height, uint32_t indexType, uint32_t start,
                                         uint32_t end)
{
    uint32_t WH[2] = {width, height};
    if ((start > end) || (stride < width) || ((indexType != 0) && (indexType != 1)) ||
        (WH[indexType] <= end) || (step <= 0)) {
        return false;
    }
    uint32_t coeff[2] = {1, stride};
    uint32_t coeff1 = coeff[indexType];
    uint32_t coeff2 = coeff[1 - indexType];
    uint32_t tmp = addr[start * coeff2];
    for (int j = start; j <= end; ++j) {
        for (int i = 0; i < WH[indexType]; i += step) {
            if (addr[j * coeff2 + i * coeff1] != tmp) {
                return false;
            }
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// getFilesFromPath
///
/// @brief  Reads the directory and returns the binary file names
///
/// @param  fileSearchPath   Path to the directory
/// @param  maxFileNameLength Maximum length of file name
/// @param  fileNamesList        Pointer to store an list of full file names at the return of
/// function
/// @param  vendorName       Vendor name
/// @param  moduleName       Name of the module
///
/// @return Number of binary files in the directory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int PluginUtils::getFilesFromPath(const char *fileSearchPath, size_t maxFileNameLength,
                                           std::list<std::string> &fileNamesList,
                                           const char *vendorName, const char *categoryName,
                                           const char *moduleName, const char *extension)
{
    unsigned int fileCount = 0;
    DIR *directory = NULL;
    dirent *dirent = NULL;
    const char *token = NULL;
    char *context = NULL;
    unsigned int tokenCount = 0;
    bool isValid = true;
    const char *com = static_cast<const char *>("com");
    uint8_t maxFileNameToken = static_cast<uint8_t>(FileNameToken::Max);
    const char *tokens[maxFileNameToken];
    char fileName[maxFileNameLength];

    MLOGI(Mia2LogGroupCore, "fileSearchPath: %s", fileSearchPath);
    directory = opendir(fileSearchPath);
    if (NULL != directory) {
        while (NULL != (dirent = readdir(directory))) {
            strncpy(fileName, dirent->d_name, maxFileNameLength);
            tokenCount = 0;
            token = strtok_r(fileName, ".", &context);
            isValid = true;
            for (int i = 0; i < maxFileNameToken; i++) {
                tokens[i] = NULL;
            }

            // The binary name is of format com.<vendor>.<category>.<module>.<extension>
            // Read all the tokens
            while ((NULL != token) && (tokenCount < maxFileNameToken)) {
                tokens[tokenCount++] = token;
                token = strtok_r(NULL, ".", &context);
            }

            // token validation
            if ((NULL == token) && (maxFileNameToken == tokenCount)) {
                for (int i = 0; (i < maxFileNameToken) && (isValid == true); i++) {
                    switch (static_cast<FileNameToken>(i)) {
                    case FileNameToken::Com:
                        token = com;
                        break;
                    case FileNameToken::Vendor:
                        token = vendorName;
                        break;
                    case FileNameToken::Category:
                        token = categoryName;
                        break;
                    case FileNameToken::Module:
                        token = moduleName;
                        break;
                    case FileNameToken::Extension:
                        token = extension;
                        break;
                    default:
                        break;
                    }

                    if (0 != strcmp(token, "*")) {
                        if (0 == strcmp(token, tokens[i])) {
                            isValid = true;
                        } else {
                            isValid = false;
                        }
                    }
                }
                if (true == isValid) {
                    MLOGD(Mia2LogGroupCore, "valid fileName: %s", dirent->d_name);
                    char tempFileName[maxFileNameLength];
                    snprintf(tempFileName, maxFileNameLength, "%s%s%s", fileSearchPath, "/",
                             dirent->d_name);
                    std::string name(tempFileName);
                    fileNamesList.push_back(name);
                    fileCount++;
                }
            }
        }
        closedir(directory);
    }
    return fileCount;
}

int PluginUtils::vSNPrintF(char *dst, size_t sizeDst, const char *format, va_list argPtr)
{
    int numCharWritten = 0;

    numCharWritten = vsnprintf(dst, sizeDst, format, argPtr);

    if ((numCharWritten >= static_cast<int>(sizeDst)) && (sizeDst > 0)) {
        numCharWritten = -1;
    }
    return numCharWritten;
}

void *PluginUtils::libMap(const char *libraryName, const char *libraryPath)
{
    void *hLibrary = NULL;
    char libFilename[FILENAME_MAX];
    int numCharWritten = 0;

    numCharWritten = sNPrintF(libFilename, FILENAME_MAX, "%s%s.%s", libraryPath, libraryName,
                              PluginSharedLibraryExtension);

    const unsigned int bindFlags = RTLD_NOW | RTLD_LOCAL;
    hLibrary = dlopen(libFilename, bindFlags);

    if (NULL == hLibrary) {
        MLOGE(Mia2LogGroupCore, "Error Loading Library: %s, dlopen err %s", libFilename, dlerror());
    }
    return hLibrary;
}

void *PluginUtils::libMap(const char *libraryName)
{
    void *hLibrary = NULL;

    const unsigned int bindFlags = RTLD_NOW | RTLD_LOCAL;
    hLibrary = dlopen(libraryName, bindFlags);

    if (NULL == hLibrary) {
        MLOGE(Mia2LogGroupCore, "Error Loading Library: %s, dlopen err %s", libraryName, dlerror());
    }
    return hLibrary;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PluginUtils::libUnmap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int PluginUtils::libUnmap(void *hLibrary)
{
    int result = MIA_RETURN_OK;

    if (NULL != hLibrary) {
        if (0 != dlclose(hLibrary)) {
            result = MIA_RETURN_UNKNOWN_ERROR;
        }
    } else {
        result = MIA_RETURN_INVALID_PARAM;
    }

    if (MIA_RETURN_OK != result) {
        MLOGE(Mia2LogGroupCore, "Failed to close MIA node Library %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PluginUtils::libGetAddr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void *PluginUtils::libGetAddr(void *hLibrary, const char *procName)
{
    void *procAddr = NULL;

    if (NULL != hLibrary) {
        procAddr = dlsym(hLibrary, procName);
    }

    return procAddr;
}

uint16_t PluginUtils::floatToHalf(float f)
{
    // For IEEE-754 float representation, the sign is in bit 31, biased exponent is in bits 23-30
    // and mantissa is in bits 0-22. Using hex representation, extract sign, biased exponent and
    // mantissa.
    uint32_t hex32 = *(reinterpret_cast<uint32_t *>(&f));
    uint32_t sign32 = hex32 & (0x80000000);
    uint32_t biasedExponent32 = hex32 & (0x7F800000);
    uint32_t mantissa32 = hex32 & (0x007FFFFF);

    // special case: 0 or denorm
    if (biasedExponent32 == 0) {
        return 0;
    }

    // The exponent is stored in the range 1 .. 254 (0 and 255 have special meanings), and is biased
    // by subtracting 127 to get an exponent value in the range -126 .. +127. remove exp bias,
    // adjust mantissa
    int32_t exponent32 = (static_cast<int32_t>(hex32 >> 23)) - 127;
    mantissa32 = ((mantissa32 >> (23 - 10 - 1)) + 1) >> 1; // shift with rounding to yield 10 MSBs

    if (mantissa32 & 0x00000400) {
        mantissa32 >>= 1; // rounding resulted in overflow, so adjust mantissa and exponent
        exponent32++;
    }

    // Assume exponent fits with 4 bits for exponent with half

    // compose
    uint16_t sign16 = static_cast<uint16_t>(sign32 >> 16);
    uint16_t biasedExponent16 = static_cast<uint16_t>((exponent32 + 15) << 10);
    uint16_t mantissa16 = static_cast<uint16_t>(mantissa32);

    return (sign16 | biasedExponent16 | mantissa16);
}

} // end namespace mialgo2
