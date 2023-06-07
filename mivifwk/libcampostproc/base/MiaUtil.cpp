/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaUtil.h"

namespace mialgo {

static void MiDumpPlane(FILE *fp, unsigned char *plane, int width, int height, int pitch)
{
    for (int i = 0; i < height; i++) {
        fwrite(plane, width, 1, fp);
        plane += pitch;
    }
}

CDKResult MiaUtil::calcOffset(MiImageFormat format, mia_padding_info_p padding,
                              MiaFrame_len_offset_p bufPlanes)
{
    CDKResult rc = MIAS_OK;

    if ((NULL == padding) || (NULL == bufPlanes)) {
        MLOGE("Fail to : invalid parameters\n");
        return MIAS_INVALID_PARAM;
    }

    int32_t stride = PAD_TO_SIZE(format.width, (int32_t)padding->width_padding);
    int32_t scanline = PAD_TO_SIZE(format.height, (int32_t)padding->height_padding);

    MLOGI("format %d, dim->width ,dim->height (%d:%d) \n", format.format, format.width,
          format.height);
    MLOGI("padding->width_padding, padding->height_padding, padding->plane_padding (%d:%d:%d) \n",
          padding->width_padding, padding->height_padding, padding->plane_padding);
    MLOGI("stride ,scanline (%d:%d), multi result:%d\n", stride, scanline, stride * scanline);

    switch (format.format) {
    case CAM_FORMAT_YUV_420_NV21:
    case CAM_FORMAT_YUV_420_NV12:
    case CAM_FORMAT_JPEG:
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
    case CAM_FORMAT_Y16:
        MLOGE("get CAM_FORMAT_Y16 %d\n", format.format);
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
    case 37: // CAM_FORMAT_RAW10:
        MLOGI("get Format RAW10  %d\n", format.format);
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
        bufPlanes->mp[0].len = bufPlanes->frame_len = PAD_TO_SIZE(stride * scanline, MI_PAD_TO_4K);
        break;
    case 32: // CAM_FORMAT_RAW16:
        MLOGI("get Format RAW16  %d\n", format.format);
        stride = format.width * 2;
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
        bufPlanes->mp[0].len = bufPlanes->frame_len = PAD_TO_SIZE(stride * scanline, MI_PAD_TO_4K);
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
        MLOGE(": Invalid cam_format %d for raw stream\n", format.format);
        rc = MIAS_INVALID_PARAM;
        break;
    }
    return rc;
}

CDKResult MiaUtil::dumpMemToFile(char *fileName, char *buffer, uint32_t bufferLen)
{
    if (fileName == NULL || buffer == NULL || bufferLen <= 0) {
        MLOGE("Failed: invalid parameters");
        return MIAS_INVALID_PARAM;
    }
    char dumpFile[100];
    sprintf(dumpFile, "/data/%s", fileName);
    int fileFd = open(dumpFile, O_RDWR | O_CREAT, 0777);
    if (fileFd <= 0) {
        MLOGE("Failed: open fileName %s", fileName);
        return MIAS_OPEN_FAILED;
    }
    write(fileFd, buffer, bufferLen);
    close(fileFd);
    return MIAS_OK;
}

bool MiaUtil::dumpToFile(const char *fileName, MiImageList_p miBuf)
{
    if (miBuf == NULL)
        return MIAS_INVALID_PARAM;
    char buf[128];
    char timeBuf[128];
    time_t currentTime;
    struct tm *timeinfo;
    static String8 lastFileName("");
    static int k = 0;

    time(&currentTime);
    timeinfo = localtime(&currentTime);
    memset(buf, 0, sizeof(buf));
    strftime(timeBuf, sizeof(timeBuf), "IMG_%Y%m%d%H%M%S", timeinfo);
    snprintf(buf, sizeof(buf), "%s%s_%s", DUMP_FILE_PATH, timeBuf, fileName);
    String8 filePath(buf);
    const char *suffix = "yuv";
    int fileFd = -1;
    uint8_t *frame0 = (uint8_t *)miBuf->pImageList[0].pAddr[0];
    uint8_t *frame1 = (uint8_t *)miBuf->pImageList[0].pAddr[1];
    int fmt = miBuf->format.format;
    int writtenLen = 0;
    if (!filePath.compare(lastFileName)) {
        snprintf(buf, sizeof(buf), "_%d.%s", k++, suffix);
        MLOGD("%s%s", filePath.string(), buf);
    } else {
        MLOGD("diff %s %s", filePath.string(), lastFileName.string());
        k = 0;
        lastFileName = filePath;
        snprintf(buf, sizeof(buf), ".%s", suffix);
    }

    filePath.append(buf);
    fileFd = open(filePath.string(), O_RDWR | O_CREAT, 0777);
    if (fileFd > 0) {
        if (fmt == CAM_FORMAT_YUV_420_NV12 || fmt == CAM_FORMAT_YUV_420_NV21) {
            for (int i = 0; i < miBuf->format.height; i++) {
                writtenLen += write(fileFd, frame0, miBuf->planeStride);
                frame0 += miBuf->planeStride;
            }
            for (int i = 0; i < miBuf->format.height / 2; i++) {
                writtenLen += write(fileFd, frame1, miBuf->planeStride);
                frame1 += miBuf->planeStride;
            }
            MLOGD("write from %p, %p, planeSize[0]:%zu, planeSize[1]:%zu", frame0, frame1,
                  miBuf->planeSize[0], miBuf->planeSize[1]);
        } else if (fmt == CAM_FORMAT_Y16) {
            writtenLen += write(fileFd, frame0, miBuf->planeSize[0]);
        }
        MLOGD("Success written number of bytes %d ,planeSize %lu, %s, fmt:0x%x", writtenLen,
              miBuf->planeSize[0] + miBuf->planeSize[1], filePath.string(), fmt);
        close(fileFd);
    } else {
        MLOGI("Fail t open file for image dumping %s: %s\n", filePath.string(), strerror(errno));
    }
    return MIAS_OK;
}

bool MiaUtil::dumpToFile(const char *fileName, struct MiImageBuffer *miBuf)
{
    if (miBuf == NULL)
        return MIAS_INVALID_PARAM;

    char buf[128];
    char timeBuf[128];
    time_t currentTime;
    struct tm *timeinfo;
    static String8 lastFileName("");
    static int k = 0;

    time(&currentTime);
    timeinfo = localtime(&currentTime);
    memset(buf, 0, sizeof(buf));
    strftime(timeBuf, sizeof(timeBuf), "IMG_%Y%m%d%H%M%S", timeinfo);
    snprintf(buf, sizeof(buf), "%s%s_%s", DUMP_FILE_PATH, timeBuf, fileName);
    String8 filePath(buf);
    const char *suffix = "yuv";
    if (!filePath.compare(lastFileName)) {
        snprintf(buf, sizeof(buf), "_%d.%s", k++, suffix);
        MLOGD("%s%s", filePath.string(), buf);
    } else {
        MLOGD("diff %s %s", filePath.string(), lastFileName.string());
        k = 0;
        lastFileName = filePath;
        snprintf(buf, sizeof(buf), ".%s", suffix);
    }

    filePath.append(buf);
    MiaUtil::miDumpBuffer(miBuf, filePath);
    return MIAS_OK;
}

void MiaUtil::CopyImage(MiImageList_p hInput, MiImageList_p hOutput)
{
    uint32_t outPlaneStride = hOutput->planeStride;
    uint32_t outHeight = hOutput->format.height;
    uint32_t outSliceHeight = hOutput->sliceHeight;
    uint32_t inPlaneStride = hInput->planeStride;
    uint32_t inHeight = hInput->format.height;
    uint32_t inSliceHeight = hInput->sliceHeight;
    uint32_t copyPlaneStride = outPlaneStride > inPlaneStride ? inPlaneStride : outPlaneStride;
    uint32_t copyHeight = outHeight > inHeight ? inHeight : outHeight;

    MLOGD("CopyImage input %d, %d, %d, output %d, %d, %d, format 0x%x 0x%x, numberOfPlanes: %d",
          inPlaneStride, inHeight, inSliceHeight, outPlaneStride, outHeight, outSliceHeight,
          hInput->format.format, hOutput->format.format, hInput->numberOfPlanes);

    for (uint32_t i = 0; i < hOutput->imageCount; i++) {
        if (((outPlaneStride != inPlaneStride) || (outSliceHeight != inSliceHeight)) &&
            ((hOutput->format.format == CAM_FORMAT_YUV_420_NV12) ||
             (hOutput->format.format == CAM_FORMAT_YUV_420_NV21))) {
            for (uint32_t m = 0; m < copyHeight; m++) {
                memcpy((hOutput->pImageList[i].pAddr[0] + outPlaneStride * m),
                       (hInput->pImageList[i].pAddr[0] + inPlaneStride * m), copyPlaneStride);
            }
            for (uint32_t m = 0; m < (copyHeight >> 1); m++) {
                memcpy((hOutput->pImageList[i].pAddr[1] + outPlaneStride * m),
                       (hInput->pImageList[i].pAddr[1] + inPlaneStride * m), copyPlaneStride);
            }
        } else {
            for (uint32_t j = 0; j < hOutput->numberOfPlanes; j++) {
                memcpy(hOutput->pImageList[i].pAddr[j], hInput->pImageList[i].pAddr[j],
                       hInput->planeSize[j]);
            }
        }
    }
}

int MiaUtil::miAllocBuffer(MiImageList_p buffer, int width, int height, MiaFormat format)
{
    if (format == CAM_FORMAT_YUV_420_NV21 || format == CAM_FORMAT_YUV_420_NV12) {
        buffer->pImageList[0].pAddr[0] = (unsigned char *)malloc(width * height * 3 / 2);
        if (NULL == buffer->pImageList[0].pAddr[0])
            return false;
        buffer->pImageList[0].pAddr[1] = buffer->pImageList[0].pAddr[0] + width * height;
        buffer->planeStride = width;
        buffer->imageCount = 1;
        buffer->format.format = format;
        buffer->format.width = width;
        buffer->format.height = height;
        return true;
    }
    return false;
}

int MiaUtil::miAllocBuffer(struct MiImageBuffer *buffer, int width, int height, int format)
{
    if (format == CAM_FORMAT_YUV_420_NV21 || format == CAM_FORMAT_YUV_420_NV12) {
        buffer->Plane[0] = (unsigned char *)malloc(width * height * 3 / 2);
        if (NULL == buffer->Plane[0])
            return false;
        buffer->Plane[1] = buffer->Plane[0] + width * height;
        buffer->Pitch[0] = buffer->Pitch[1] = width;
        buffer->PixelArrayFormat = format;
        buffer->Width = width;
        buffer->Height = height;
        return true;
    } else if (format == CAM_FORMAT_Y16) {
        buffer->Plane[0] = (unsigned char *)malloc(width * height);
        if (NULL == buffer->Plane[0])
            return false;
        buffer->Plane[1] = NULL;
        buffer->Pitch[0] = buffer->Pitch[1] = width;
        buffer->PixelArrayFormat = format;
        buffer->Width = width;
        buffer->Height = height;
        return true;
    }
    return false;
}

void MiaUtil::miFreeBuffer(MiImageList_p buffer)
{
    if (buffer->pImageList[0].pAddr[0]) {
        free(buffer->pImageList[0].pAddr[0]);
        buffer->pImageList[0].pAddr[0] = NULL;
    }
}

void MiaUtil::miFreeBuffer(struct MiImageBuffer *buffer)
{
    if (buffer->Plane[0]) {
        free(buffer->Plane[0]);
        buffer->Plane[0] = NULL;
    }
}

void MiaUtil::miDumpBuffer(MiImageList_p buffer, const char *file)
{
    FILE *p = fopen(file, "wb+");
    if (p == NULL) {
        int width, height;
        if (buffer->format.format == CAM_FORMAT_YUV_420_NV21 ||
            buffer->format.format == CAM_FORMAT_YUV_420_NV12) {
            width = buffer->format.width;
            height = buffer->format.height;
            MiDumpPlane(p, buffer->pImageList[0].pAddr[0], width, height, buffer->planeStride);
            MiDumpPlane(p, buffer->pImageList[0].pAddr[1], width, height / 2, buffer->planeStride);
            MLOGD("MiDump %s width:height = %d * %d\n", file, width, height);
        }
        fclose(p);
    }
}

void MiaUtil::miDumpBuffer(struct MiImageBuffer *buffer, const char *file)
{
    FILE *p = fopen(file, "wb+");
    if (p != NULL) {
        int width, height;
        if (buffer->PixelArrayFormat == CAM_FORMAT_YUV_420_NV21 ||
            buffer->PixelArrayFormat == CAM_FORMAT_YUV_420_NV12) {
            width = buffer->Width;
            height = buffer->Height;
            MiDumpPlane(p, buffer->Plane[0], width, height, buffer->Pitch[0]);
            MiDumpPlane(p, buffer->Plane[1], width, height / 2, buffer->Pitch[1]);
            MLOGD("MiDump %s width:height = %d * %d\n", file, width, height);
        }
        if (buffer->PixelArrayFormat == CAM_FORMAT_Y16) {
            width = buffer->Width;
            height = buffer->Height;
            MiDumpPlane(p, buffer->Plane[0], width, height, buffer->Pitch[0]);
            MLOGD("MiDump %s width:height = %d * %d\n", file, width, height);
        }
        fclose(p);
    }
}

void MiaUtil::miCopyPlane(unsigned char *dst, unsigned char *src, int width, int height,
                          int dstStride, int srcStride)
{
    int i;
    for (i = 0; i < height; i++) {
        memcpy(dst, src, width);
        dst += dstStride;
        src += srcStride;
    }
}

int MiaUtil::miCopyBuffer(struct MiImageBuffer *dst, struct MiImageBuffer *src)
{
    if (src->PixelArrayFormat == CAM_FORMAT_YUV_420_NV21 ||
        dst->PixelArrayFormat == CAM_FORMAT_YUV_420_NV12) {
        if (((dst->Width == src->Width) && (dst->Height == src->Height)) ||
            ((dst->Width == src->Height) && (dst->Height == src->Width))) {
            miCopyPlane(dst->Plane[0], src->Plane[0], src->Width, src->Height, dst->Pitch[0],
                        src->Pitch[0]);
            miCopyPlane(dst->Plane[1], src->Plane[1], src->Width, src->Height / 2, dst->Pitch[1],
                        src->Pitch[1]);
            return true;
        }
    }
    if (src->PixelArrayFormat == CAM_FORMAT_Y16) {
        if (((dst->Width == src->Width) && (dst->Height == src->Height)) ||
            ((dst->Width == src->Height) && (dst->Height == src->Width))) {
            miCopyPlane(dst->Plane[0], src->Plane[0], src->Width, src->Height, dst->Pitch[0],
                        src->Pitch[0]);
            return true;
        }
    }
    return false;
}

void MiaUtil::miAllocCopyBuffer(struct MiImageBuffer *dstBuffer, struct MiImageBuffer *srcBuffer)
{
    memcpy(dstBuffer, srcBuffer, sizeof(struct MiImageBuffer));
    dstBuffer->Plane[0] = (unsigned char *)malloc(dstBuffer->Height * dstBuffer->Pitch[0] * 3 / 2);
    dstBuffer->Plane[1] = dstBuffer->Plane[0] + dstBuffer->Height * dstBuffer->Pitch[0];

    unsigned char *srcTemp = srcBuffer->Plane[0];
    unsigned char *dstTemp = dstBuffer->Plane[0];
    for (int h = 0; h < srcBuffer->Height; h++) {
        memcpy(dstTemp, srcTemp, srcBuffer->Width);
        srcTemp += srcBuffer->Pitch[0];
        dstTemp += dstBuffer->Pitch[0];
    }

    srcTemp = srcBuffer->Plane[1];
    dstTemp = dstBuffer->Plane[1];
    for (int h = 0; h < srcBuffer->Height / 2; h++) {
        memcpy(dstTemp, srcTemp, srcBuffer->Width);
        srcTemp += srcBuffer->Pitch[1];
        dstTemp += dstBuffer->Pitch[1];
    }
}

void MiaUtil::miReSize(struct MiImageBuffer *dstImage, struct MiImageBuffer *srcImage)
{
    if ((srcImage->Width == dstImage->Width) && (srcImage->Height == dstImage->Height)) {
        miCopyBuffer(dstImage, srcImage);
        return;
    }
    if (srcImage->PixelArrayFormat == CAM_FORMAT_Y16) {
        unsigned int wrfloat16 = (srcImage->Width << 16) / dstImage->Width + 1;
        unsigned int hrfloat16 = (srcImage->Height << 16) / dstImage->Height + 1;
        unsigned char *dstRow = (unsigned char *)dstImage->Plane[0];
        unsigned char *srcRow = (unsigned char *)srcImage->Plane[0];
        unsigned int srcy16 = 0;
        for (int r = 0; r < (int)dstImage->Height; r++) {
            srcRow = (unsigned char *)srcImage->Plane[0] + srcImage->Pitch[0] * (srcy16 >> 16);
            unsigned int srcx16 = 0;
            for (int c = 0; c < (int)dstImage->Width; c++) {
                dstRow[c] = srcRow[srcx16 >> 16];
                srcx16 += wrfloat16;
            }
            srcy16 += hrfloat16;
            dstRow += dstImage->Pitch[0];
        }
    } else {
        MLOGE("format not support");
    }
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
        MLOGE("Error Loading Library: %s, dlopen err %s", libFilename, dlerror());
    }
    return hLibrary;
}

MIALIBRARYHANDLE MiaUtil::libMap(const char *libraryName)
{
    MIALIBRARYHANDLE hLibrary = NULL;

    const unsigned int bindFlags = RTLD_NOW | RTLD_LOCAL;
    hLibrary = dlopen(libraryName, bindFlags);

    if (NULL == hLibrary) {
        MLOGE("Error Loading Library: %s, dlopen err %s", libraryName, dlerror());
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
        MLOGE("Failed to close MIA node Library %d", result);
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
    int32_t exponent_32 = (static_cast<int32_t>(hex32 >> 23)) - 127;
    mantissa32 = ((mantissa32 >> (23 - 10 - 1)) + 1) >> 1; // shift with rounding to yield 10 MSBs

    if (mantissa32 & 0x00000400) {
        mantissa32 >>= 1; // rounding resulted in overflow, so adjust mantissa and exponent
        exponent_32++;
    }

    // Assume exponent fits with 4 bits for exponent with half

    // compose
    uint16_t sign16 = static_cast<uint16_t>(sign32 >> 16);
    uint16_t biasedExponent16 = static_cast<uint16_t>((exponent_32 + 15) << 10);
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
        MLOGE("open /proc/meminfo failed");
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

    MLOGI("get availMem %ld", availMem);
    return availMem;
}

bool MiaUtil::isDumpVisible()
{
    if (!access("/sdcard/DCIM/Camera/algoup_dump_images", 0))
        return true;
    else
        return false;
}

void MiaUtil::addMovieEffect(MiImageList_p input, int blackWidth)
{
    if (input == NULL || input->pImageList[0].pAddr[0] == NULL) {
        MLOGE("input is null");
        return;
    }
    double startTime = MiaUtil::nowMSec();
    uint8_t *y = input->pImageList[0].pAddr[0];
    uint8_t *uv = input->pImageList[0].pAddr[1];
    // y top
    memset(y, 0, blackWidth * input->planeStride);
    // y bottom
    memset(y + (input->format.height - blackWidth) * input->planeStride, 0,
           blackWidth * input->planeStride);
    // uv top
    memset(uv, 128, blackWidth * input->planeStride / 2);
    // uv bottom
    memset(uv + (input->format.height - blackWidth) * input->planeStride / 2, 128,
           blackWidth * input->planeStride / 2);
    MLOGI("addMovieEffect process costTime = %.2fms ", MiaUtil::nowMSec() - startTime);
}

} // end namespace mialgo
