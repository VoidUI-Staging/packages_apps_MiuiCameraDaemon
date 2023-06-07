/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaUtil.h"

namespace mialgo2 {

CDKResult MiaUtil::calcOffset(MiaImageFormat format, PaddingInfo *padding, LenOffsetInfo *bufPlanes)
{
    CDKResult rc = MIAS_OK;

    if ((NULL == padding) || (NULL == bufPlanes)) {
        MLOGE(Mia2LogGroupCore, "Fail to : invalid parameters\n");
        return MIAS_INVALID_PARAM;
    }

    int32_t stride = PAD_TO_SIZE(format.width, (int32_t)padding->width_padding);
    int32_t scanline = PAD_TO_SIZE(format.height, (int32_t)padding->height_padding);

    MLOGD(Mia2LogGroupCore, "format:%d, w:h->(%d:%d) stride:scanline->(%d:%d)", format.format,
          format.width, format.height, stride, scanline);

    switch (format.format) {
    case CAM_FORMAT_YUV_420_NV21:
    case CAM_FORMAT_YUV_420_NV12:
    case CAM_FORMAT_IMPLEMENTATION_DEFINED:
    case CAM_FORMAT_BLOB:
        /* 2 planes: Y + CbCr */
        bufPlanes->num_planes = 2;
        bufPlanes->mp[0].len = PAD_TO_SIZE((uint32_t)(stride * scanline), padding->plane_padding);
        bufPlanes->mp[0].offset = 0;
        bufPlanes->mp[0].offset_x = 0;
        bufPlanes->mp[0].offset_y = 0;
        bufPlanes->mp[0].stride = stride;
        bufPlanes->mp[0].stride_in_bytes = stride;
        bufPlanes->mp[0].scanline = scanline;
        bufPlanes->mp[0].width = format.width;
        bufPlanes->mp[0].height = format.height;

        scanline = PAD_TO_SIZE((uint32_t)(scanline / 2), padding->plane_padding);
        bufPlanes->mp[1].len = PAD_TO_SIZE((uint32_t)(stride * scanline), padding->plane_padding);
        bufPlanes->mp[1].offset = 0;
        bufPlanes->mp[1].offset_x = 0;
        bufPlanes->mp[1].offset_y = 0;
        bufPlanes->mp[1].stride = stride;
        bufPlanes->mp[1].stride_in_bytes = stride;
        bufPlanes->mp[1].scanline = scanline;
        bufPlanes->mp[1].width = format.width;
        bufPlanes->mp[1].height = format.height / 2;
        bufPlanes->frame_len =
            PAD_TO_SIZE(bufPlanes->mp[0].len + bufPlanes->mp[1].len, MI_PAD_TO_4K);
        break;
    // TODO
    case CAM_FORMAT_RAW10:
        MLOGI(Mia2LogGroupCore, "get RAW10 %d\n", format.format);
        stride = PAD_TO_SIZE((format.width * 10) / 8, 80); // flow qcom formatutil external
                                                           // bsp dump may need optim
        scanline = format.height;
        bufPlanes->num_planes = 1;
        bufPlanes->mp[0].offset = 0;
        bufPlanes->mp[0].offset_x = 0;
        bufPlanes->mp[0].offset_y = 0;
        bufPlanes->mp[0].stride = stride;
        bufPlanes->mp[0].stride_in_bytes = stride;
        bufPlanes->mp[0].scanline = scanline;
        bufPlanes->mp[0].width = format.width;
        bufPlanes->mp[0].height = format.height;
        bufPlanes->mp[0].len = bufPlanes->frame_len =
            PAD_TO_SIZE((uint32_t)(stride * scanline), MI_PAD_TO_4K);
        break;
    case CAM_FORMAT_RAW16:
        MLOGI(Mia2LogGroupCore, "get RAW16 %d\n", format.format);
        stride =
            PAD_TO_SIZE((uint32_t)(format.width * 2), MI_PAD_TO_32); // BPS raw2yuv need 32 align
        scanline = format.height;
        bufPlanes->num_planes = 1;
        bufPlanes->mp[0].offset = 0;
        bufPlanes->mp[0].offset_x = 0;
        bufPlanes->mp[0].offset_y = 0;
        bufPlanes->mp[0].stride = stride;
        bufPlanes->mp[0].stride_in_bytes = stride;
        bufPlanes->mp[0].scanline = scanline;
        bufPlanes->mp[0].width = format.width;
        bufPlanes->mp[0].height = format.height;
        bufPlanes->mp[0].len = bufPlanes->frame_len =
            PAD_TO_SIZE((uint32_t)(stride * scanline), MI_PAD_TO_4K);
        break;
    case CAM_FORMAT_RAW_OPAQUE:
        MLOGI(Mia2LogGroupCore, "get RAWOPAQUE format %d\n", format.format);
        bufPlanes->num_planes = 1;
        bufPlanes->frame_len = bufPlanes->mp[0].len =
            static_cast<uint32_t>(format.width * format.height * 1);
        bufPlanes->mp[0].offset = 0;
        bufPlanes->mp[0].offset_x = 0;
        bufPlanes->mp[0].offset_y = 0;
        bufPlanes->mp[0].stride = stride;
        bufPlanes->mp[0].stride_in_bytes = stride;
        bufPlanes->mp[0].scanline = scanline;
        bufPlanes->mp[0].width = format.width;
        bufPlanes->mp[0].height = format.height;
        break;
    case CAM_FORMAT_Y16:
        MLOGI(Mia2LogGroupCore, "get CAM_FORMAT_Y16 %d\n", format.format);
        bufPlanes->num_planes = 1;
        bufPlanes->mp[0].len = PAD_TO_SIZE((uint32_t)(stride * scanline), padding->plane_padding);

        bufPlanes->mp[0].offset = 0;
        bufPlanes->mp[0].offset_x = 0;
        bufPlanes->mp[0].offset_y = 0;
        bufPlanes->mp[0].stride = stride;
        bufPlanes->mp[0].stride_in_bytes = stride;
        bufPlanes->mp[0].scanline = scanline;
        bufPlanes->mp[0].width = format.width;
        bufPlanes->mp[0].height = format.height;
        break;
    case CAM_FORMAT_P010:
        // 2 planes: Y + CbCr
        // P010 is 2 bytes per pixel where the least significant 6 bits are essentially ignored
        // Y_Stride : Width * 2 aligned to 256
        // UV_Stride : Width * 2 aligned to 256
        // Y_Scanlines: Height aligned to 32
        // UV_Scanlines: Height/2 aligned to 16
        // Total size = align(Y_Stride * Y_Scanlines
        //          + UV_Stride * UV_Scanlines, 4096)
        // requirements from msm_media_info.h
        stride = PAD_TO_SIZE(format.width * 2, MI_PAD_TO_256);
        scanline = PAD_TO_SIZE(format.height, MI_PAD_TO_32);
        bufPlanes->num_planes = 2;
        // TODO fix may import failed
        bufPlanes->mp[0].len = PAD_TO_SIZE((uint32_t)(stride * scanline), padding->plane_padding);
        bufPlanes->mp[0].offset = 0;
        bufPlanes->mp[0].offset_x = 0;
        bufPlanes->mp[0].offset_y = 0;
        bufPlanes->mp[0].stride = stride;
        bufPlanes->mp[0].stride_in_bytes = stride;
        bufPlanes->mp[0].scanline = scanline;
        bufPlanes->mp[0].width = format.width;
        bufPlanes->mp[0].height = format.height;

        scanline = PAD_TO_SIZE((uint32_t)(format.height / 2), MI_PAD_TO_16);
        // TODO fix may import failed
        bufPlanes->mp[1].len = PAD_TO_SIZE((uint32_t)(stride * scanline), padding->plane_padding);
        bufPlanes->mp[1].offset = 0;
        bufPlanes->mp[1].offset_x = 0;
        bufPlanes->mp[1].offset_y = 0;
        bufPlanes->mp[1].stride = stride;
        bufPlanes->mp[1].stride_in_bytes = stride;
        bufPlanes->mp[1].scanline = scanline;
        bufPlanes->mp[1].width = format.width;
        bufPlanes->mp[1].height = format.height / 2;
        bufPlanes->frame_len =
            PAD_TO_SIZE(bufPlanes->mp[0].len + bufPlanes->mp[1].len, MI_PAD_TO_4K);
        break;
    default:
        MLOGE(Mia2LogGroupCore, ": Invalid cam_format %d for raw stream\n", format.format);
        rc = MIAS_INVALID_PARAM;
        break;
    }
    return rc;
}

bool MiaUtil::copyFrame(MiaFrame *input, MiaFrame *output)
{
    if ((input == NULL) || (output == NULL)) {
        return false;
    }

    uint32_t outPlaneStride = output->format.planeStride;
    uint32_t outHeight = output->format.height;
    uint32_t outSliceheight = output->format.sliceheight;
    uint32_t inPlaneStride = input->format.planeStride;
    uint32_t inHeight = input->format.height;
    uint32_t inSliceheight = input->format.sliceheight;
    uint32_t copyPlaneStride = outPlaneStride > inPlaneStride ? inPlaneStride : outPlaneStride;
    uint32_t copyHeight = outHeight > inHeight ? inHeight : outHeight;

    MLOGI(Mia2LogGroupCore, "copyFrame input %d, %d, %d, output %d, %d, %d, format 0x%x 0x%x",
          inPlaneStride, inHeight, inSliceheight, outPlaneStride, outHeight, outSliceheight,
          input->format.format, output->format.format);

    if ((output->format.format == CAM_FORMAT_YUV_420_NV21) ||
        (output->format.format == CAM_FORMAT_YUV_420_NV12) ||
        (output->format.format == CAM_FORMAT_IMPLEMENTATION_DEFINED)) {
        for (uint32_t m = 0; m < copyHeight; m++) {
            memcpy((output->pBuffer.pAddr[0] + outPlaneStride * m),
                   (input->pBuffer.pAddr[0] + inPlaneStride * m), copyPlaneStride);
        }
        for (uint32_t m = 0; m < (copyHeight >> 1); m++) {
            memcpy((output->pBuffer.pAddr[1] + outPlaneStride * m),
                   (input->pBuffer.pAddr[1] + inPlaneStride * m), copyPlaneStride);
        }
    } else {
        MLOGE(Mia2LogGroupCore, "unknow format: %d", output->format.format);
        return false;
    }

    return true;
}

int MiaUtil::vSNPrintF(char *dst, size_t sizeDst, const char *format, va_list argPtr)
{
    int numCharWritten = 0;

    numCharWritten = vsnprintf(dst, sizeDst, format, argPtr);

    if ((numCharWritten >= static_cast<int>(sizeDst)) && (sizeDst > 0)) {
        numCharWritten = -1;
    }
    return numCharWritten;
}

MIALIBRARYHANDLE MiaUtil::libMap(const char *libraryName, const char *libraryPath)
{
    MIALIBRARYHANDLE hLibrary = NULL;
    char libFilename[FILENAME_MAX];
    int numCharWritten = 0;

    numCharWritten = sNPrintF(libFilename, FILENAME_MAX, "%s%s.%s", libraryPath, libraryName,
                              SharedLibraryExtension);

    const unsigned int bindFlags = RTLD_NOW | RTLD_LOCAL;
    hLibrary = dlopen(libFilename, bindFlags);

    if (NULL == hLibrary) {
        MLOGE(Mia2LogGroupCore, "Error Loading Library: %s, dlopen err %s", libFilename, dlerror());
    }
    return hLibrary;
}

MIALIBRARYHANDLE MiaUtil::libMap(const char *libraryName)
{
    MIALIBRARYHANDLE hLibrary = NULL;

    const unsigned int bindFlags = RTLD_NOW | RTLD_LOCAL;
    hLibrary = dlopen(libraryName, bindFlags);

    if (NULL == hLibrary) {
        MLOGE(Mia2LogGroupCore, "Error Loading Library: %s, dlopen err %s", libraryName, dlerror());
    }
    return hLibrary;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaUtil::libUnmap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaUtil::libUnmap(MIALIBRARYHANDLE hLibrary)
{
    CDKResult result = MIAS_OK;

    if (NULL != hLibrary) {
        if (0 != dlclose(hLibrary)) {
            result = MIAS_UNKNOWN_ERROR;
        }
    } else {
        result = MIAS_INVALID_PARAM;
    }

    if (MIAS_OK != result) {
        MLOGE(Mia2LogGroupCore, "Failed to close MIA node Library %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaUtil::libGetAddr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void *MiaUtil::libGetAddr(MIALIBRARYHANDLE hLibrary, const char *procName)
{
    void *procAddr = NULL;

    if (NULL != hLibrary) {
        procAddr = dlsym(hLibrary, procName);
    }

    return procAddr;
}

uint16_t MiaUtil::floatToHalf(float f)
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

double MiaUtil::nowMSec()
{
    struct timeval res;
    gettimeofday(&res, NULL);
    return 1000.0 * res.tv_sec + (double)res.tv_usec / 1e3;
}

// get the available memory in kb, return 0 if get failed
uint64_t MiaUtil::getAvailMemory()
{
    uint64_t availMem = 0;
    int memInfoFile = open("/proc/meminfo", O_RDONLY);
    if (memInfoFile < 0) {
        MLOGE(Mia2LogGroupCore, "open /proc/meminfo failed");
        return availMem;
    }

    char buffer[256];
    const int len = read(memInfoFile, buffer, sizeof(buffer) - 1);
    close(memInfoFile);
    if (len < 0) {
        return availMem;
    }
    buffer[len] = 0;
    int numFound = 0;
    static const char *const sums[] = {"MemFree:", "Cached:", NULL};
    static const uint64_t sumsLen[] = {strlen("MemFree:"), strlen("Cached:"), 0};
    char *p = buffer;
    while (*p && numFound < 2) {
        int i = 0;
        while (sums[i]) {
            if (strncmp(p, sums[i], sumsLen[i]) == 0) {
                p += sumsLen[i];
                while (*p == ' ')
                    p++;
                char *num = p;
                while (*p >= '0' && *p <= '9')
                    p++;
                if (*p != 0) {
                    *p = 0;
                    p++;
                    if (*p == 0)
                        p--;
                }
                availMem += atoll(num);
                numFound++;
                break;
            }
            i++;
        }
        p++;
    }

    MLOGI(Mia2LogGroupCore, "get availMem %llu", availMem);
    return availMem;
}

bool MiaUtil::isDumpVisible()
{
    if (!access("/sdcard/DCIM/Camera/algoup_dump_images", 0))
        return true;
    else
        return false;
}

void MiaUtil::getDateTime(MiaDateTime *pDateTime)
{
    struct timeval timeValue;
    struct tm *pTimeInfo = NULL;
    int result = 0;

    result = gettimeofday(&timeValue, NULL);
    if (0 == result) {
        pTimeInfo = localtime(&timeValue.tv_sec);
        if (NULL != pTimeInfo) {
            pDateTime->seconds = static_cast<uint32_t>(pTimeInfo->tm_sec);
            pDateTime->microseconds = static_cast<uint32_t>(timeValue.tv_usec);
            pDateTime->minutes = static_cast<uint32_t>(pTimeInfo->tm_min);
            pDateTime->hours = static_cast<uint32_t>(pTimeInfo->tm_hour);
            pDateTime->dayOfMonth = static_cast<uint32_t>(pTimeInfo->tm_mday);
            pDateTime->month = static_cast<uint32_t>(pTimeInfo->tm_mon);
            pDateTime->year = static_cast<uint32_t>(pTimeInfo->tm_year);
            pDateTime->weekday = static_cast<uint32_t>(pTimeInfo->tm_wday);
            pDateTime->yearday = static_cast<uint32_t>(pTimeInfo->tm_yday);
            pDateTime->dayLightSavingTimeFlag = static_cast<uint32_t>(pTimeInfo->tm_isdst);
            pDateTime->gmtOffset = pTimeInfo->tm_gmtoff;
        }
    }
}
} // end namespace mialgo2
