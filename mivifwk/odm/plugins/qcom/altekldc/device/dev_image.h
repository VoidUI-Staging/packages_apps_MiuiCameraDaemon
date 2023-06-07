/**
 * @file        dev_image.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version     0.1
 * @note
 * @warning
 * @par
 *
 */

#ifndef __DEV_IMAGE_H__
#define __DEV_IMAGE_H__

#include "dev_type.h"

#if defined(WINDOWS_OS)
#define DEV_IMAGE_DUMP_DIR  "output"
#elif defined(LINUX_OS)
#define DEV_IMAGE_DUMP_DIR  "output"
#elif defined(ANDROID_OS)
#define DEV_IMAGE_DUMP_DIR  "/data/vendor/camera"
#endif

#define DEV_IMAGE_MAX_PLANE 3

typedef enum DevImage_Format {
    DEV_IMAGE_FORMAT_Y8 = 1,//Y8,
    DEV_IMAGE_FORMAT_Y16 = 540422489,//Y16,
    DEV_IMAGE_FORMAT_YUV420NV12 = 35,//YUV420NV12,
    DEV_IMAGE_FORMAT_YUV420NV21 = 17,//YUV420NV21,
    DEV_IMAGE_FORMAT_YUV422NV16 = 5,//YUV422NV16,
    DEV_IMAGE_FORMAT_YUYV = 6,//YUYV,
    DEV_IMAGE_FORMAT_RAWPLAIN10 = 37,//RAW Plain16 ,
    DEV_IMAGE_FORMAT_RAWPLAIN12 = 38,//RAW Plain16 ,
    DEV_IMAGE_FORMAT_RAWPLAIN16 = 32,//RAW Plain16 ,
    DEV_IMAGE_FORMAT_YUV420P010 = 0x7FA30C0A,//P010,
    DEV_IMAGE_FORMAT_YV12 = 842094169,//YV12,
    DEV_IMAGE_FORMAT_BLOB = 33,//BLOB
    DEV_IMAGE_FORMAT_RAW_OPAQUE = 36,//RAW_OPAQUE,
    DEV_IMAGE_FORMAT_IMPLEMENTATION_DEFINED = 34,//IMPLEMENTATION_DEFINED,
    DEV_IMAGE_FORMAT_RGBA888 = 0xff,
    DEV_IMAGE_FORMAT_END = 0xFFFFFFFF,
} DEV_IMAGE_FORMAT;

typedef struct __DevImage_point {
    S32 x;
    S32 y;
} DEV_IMAGE_POINT;

typedef struct __DevImage_Rect {
    U32 left;                           ///< x coordinate for top-left point
    U32 top;                            ///< y coordinate for top-left point
    U32 width;                          ///< width
    U32 height;                         ///< height
} DEV_IMAGE_RECT;

typedef struct DevImageBuf {
    U32 format;
    U32 width;
    U32 height;
    U32 offset = 0;
    U32 planeCount;
    U32 stride[DEV_IMAGE_MAX_PLANE];
    U32 sliceHeight[DEV_IMAGE_MAX_PLANE];
    U32 planeSize[DEV_IMAGE_MAX_PLANE];
    char *plane[DEV_IMAGE_MAX_PLANE];
    int fd[DEV_IMAGE_MAX_PLANE];
} DEV_IMAGE_BUF;

typedef struct __Dev_Image Dev_Image;
struct __Dev_Image {
    S64 (*Init)(void);
    S64 (*Deinit)(void);
    S64 (*MatchToImage)(void *srcData, DEV_IMAGE_BUF *dstImage);
    S64 (*MatchToVoid)(DEV_IMAGE_BUF *srcImage, void *dstData);
    S64 (*Copy)(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage);
    S64 (*Alloc)(DEV_IMAGE_BUF *image, U32 width, U32 stride, U32 height, U32 sliceHeight, U32 offset, DEV_IMAGE_FORMAT format, MARK_TAG tagName);
    S64 (*Clone)(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage, MARK_TAG tagName);
    S64 (*Free)(DEV_IMAGE_BUF *image);
    S64 (*DumpImage)(DEV_IMAGE_BUF *image, const char* nodeName, U32 id, const char *fileName);
    S64 (*DumpData)(void *buf, U32 size, const char* nodeName, U32 id, const char *fileName);
    S64 (*ReSize)(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage);
    S64 (*Merge)(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage, S32 downsize);
    S64 (*Yuv420sp2Rgba)(char *py, char *puv, char *rgba, int stride_y, int stride_uv, int width, int height);
    S64 (*Rgba2Yuv420sp)(char *py, char *puv, char *rgba, int stride_y, int stride_uv, int width, int height);
    S64 (*DrawPoint)(DEV_IMAGE_BUF* image, DEV_IMAGE_POINT xPoint, S32 nPointWidth, S32 iType);
    S64 (*DrawRect)(DEV_IMAGE_BUF *image, DEV_IMAGE_RECT rt, S32 nLineWidth, S32 iType);
    S64 (*Report)(void);
    char* (*FormatString)(U32 format);
};

extern const Dev_Image m_dev_image;

#endif // __DEV_IMAGE_H__
