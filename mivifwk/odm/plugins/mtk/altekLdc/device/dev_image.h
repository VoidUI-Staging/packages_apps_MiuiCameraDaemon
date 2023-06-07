/**
 * @file		dev_image.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par
 *
 */

#ifndef __DEV_IMAGE_H__
#define __DEV_IMAGE_H__

#include "dev_type.h"

#define DEV_IMAGE_DUMP_DIR  "/sdcard/DCIM/Camera"
#define DEV_IMAGE_MAX_PLANE 3 // ChiNodeFormatsMaxPlanes

typedef enum DevImage_Format {
    DEV_IMAGE_FORMAT_YUV420NV12 = 0x23,       // YUV420NV12,
    DEV_IMAGE_FORMAT_YUV420NV21 = 0x11,       // YUV420NV21,
    DEV_IMAGE_FORMAT_YUV422NV16 = 5,          // YUV422NV16,
    DEV_IMAGE_FORMAT_YUV420P010 = 0x7FA30C0A, // P010,
    DEV_IMAGE_FORMAT_Y8 = 0x20203859,         // Y8,
    DEV_IMAGE_FORMAT_Y16 = 0x20363159,        // Y16,
    DEV_IMAGE_FORMAT_RGBA888 = 0xff,
    DEV_IMAGE_FORMAT_END = 0xff + 1,
} DEV_IMAGE_FORMAT;

typedef struct DevImageBuf
{
    U32 format;
    U32 width;
    U32 height;
    U32 offset = 0;
    U32 stride[DEV_IMAGE_MAX_PLANE];
    U32 sliceHeight[DEV_IMAGE_MAX_PLANE];
    char *plane[DEV_IMAGE_MAX_PLANE];
} DEV_IMAGE_BUF;

typedef struct __Dev_Image Dev_Image;
struct __Dev_Image
{
    S64 (*MatchToImage)(void *srcData, DEV_IMAGE_BUF *dstImage);
    S64 (*MatchToVoid)(DEV_IMAGE_BUF *srcImage, void *dstData);
    S64 (*Copy)(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage);
    S64 (*Alloc)(DEV_IMAGE_BUF *image, U32 width, U32 height, U32 offset, DEV_IMAGE_FORMAT format);
    S64 (*Free)(DEV_IMAGE_BUF *image);
    S64 (*DumpImage)(DEV_IMAGE_BUF *image, const char *nodeName, U32 id, const char *fileName);
    S64 (*DumpData)(void *buf, U32 size, const char *nodeName, U32 id, const char *fileName);
    S64 (*ReSize)(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage);
    S64(*Yuv420sp2Rgba)
    (char *py, char *puv, char *rgba, int stride_y, int stride_uv, int width, int height);
    S64(*Rgba2Yuv420sp)
    (char *py, char *puv, char *rgba, int stride_y, int stride_uv, int width, int height);
};

extern const Dev_Image m_dev_image;

#endif // __DEV_IMAGE_H__
