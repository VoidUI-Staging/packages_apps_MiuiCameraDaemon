/**
 * @file		dev_image.cpp
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

#include <stdlib.h>
#include <string.h>

#include "dev_type.h"
//#include "chinode.h"
// #include "miafmt.h"
// #include "miautil.h"
#include "MiaPluginUtils.h"
#include "MiaPluginWraper.h"
// #include "miainterface.h"
#include "dev_image.h"
#include "device.h"

S64 DevImage_Alloc(DEV_IMAGE_BUF *image, U32 width, U32 height, U32 offset, DEV_IMAGE_FORMAT format)
{
    DEV_IF_LOGE_RETURN_RET((image == NULL), RET_ERR_ARG, "buf addr err");
    char *addr = NULL;
    switch (format) {
    case DEV_IMAGE_FORMAT_YUV420NV12:
    case DEV_IMAGE_FORMAT_YUV420NV21:
        addr = (char *)malloc((width * height * 3 / 2) + offset);
        DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "malloc err");
        image->plane[0] = addr + offset;
        image->plane[1] = image->plane[0] + width * height;
        image->stride[0] = width;
        image->stride[1] = width;
        break;
    case DEV_IMAGE_FORMAT_YUV422NV16:
        addr = (char *)malloc((width * height * 2) + offset);
        DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "malloc err");
        image->plane[0] = addr + offset;
        image->plane[1] = image->plane[0] + width * height;
        image->stride[0] = width;
        image->stride[1] = width;
        break;
    case DEV_IMAGE_FORMAT_YUV420P010:
        width *= 2;
        addr = (char *)malloc((width * height * 3 / 2) + offset);
        DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "malloc err");
        image->plane[0] = addr + offset;
        image->plane[1] = image->plane[0] + width * height;
        image->stride[0] = width;
        image->stride[1] = width;
        break;
    case DEV_IMAGE_FORMAT_Y8:
        addr = (char *)malloc((width * height) + offset);
        DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "malloc err");
        image->plane[0] = addr + offset;
        image->plane[1] = NULL;
        image->stride[0] = width;
        image->stride[1] = width;
        break;
    case DEV_IMAGE_FORMAT_Y16:
        width *= 2;
        addr = (char *)malloc((width * height) + offset);
        DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "malloc err");
        image->plane[0] = addr + offset;
        image->plane[1] = NULL;
        image->stride[0] = width;
        image->stride[1] = width;
        break;
    case DEV_IMAGE_FORMAT_RGBA888:
        DEV_LOGE("format not support");
        return RET_ERR;
        break;
    default:
        DEV_LOGE("format not support");
        return RET_ERR;
        break;
    }
    image->format = format;
    image->width = width;
    image->height = height;
    image->offset = offset;
    return RET_OK;
}

S64 DevImage_Free(DEV_IMAGE_BUF *image)
{
    DEV_IF_LOGE_RETURN_RET((image == NULL), RET_ERR_ARG, "buf addr err");
    char *addr = image->plane[0] - image->offset;
    DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR_ARG, "no image buff");
    free(addr);
    addr = NULL;
    return RET_OK;
}

static S64 DevImage_CopyPlane(char *dst, char *src, int width, int height, int dst_stride,
                              int src_stride)
{
    DEV_IF_LOGE_RETURN_RET((dst == NULL), RET_ERR_ARG, "dst addr err");
    DEV_IF_LOGE_RETURN_RET((src == NULL), RET_ERR_ARG, "py addr err");
    for (int i = 0; i < height; i++) {
        memcpy(dst, src, width);
        dst += dst_stride;
        src += src_stride;
    }
    return RET_OK;
}

S64 DevImage_Copy(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage)
{
    DEV_IF_LOGE_RETURN_RET((dstImage == NULL), RET_ERR_ARG, "dst addr err");
    DEV_IF_LOGE_RETURN_RET((srcImage == NULL), RET_ERR_ARG, "py addr err");
    DEV_IF_LOGE_RETURN_RET((srcImage->format != dstImage->format), RET_ERR_ARG, "format err");
    DEV_IF_LOGE_RETURN_RET(
        ((dstImage->width != srcImage->width) || (dstImage->height != srcImage->height)) &&
            ((dstImage->width != srcImage->height) || (dstImage->height != srcImage->width)),
        RET_ERR_ARG, "size err");
    int width = srcImage->width;
    int height = srcImage->height;
    switch (srcImage->format) {
    case DEV_IMAGE_FORMAT_YUV420NV12:
    case DEV_IMAGE_FORMAT_YUV420NV21:
        DevImage_CopyPlane(dstImage->plane[0], srcImage->plane[0], width, height,
                           dstImage->stride[0], srcImage->stride[0]);
        DevImage_CopyPlane(dstImage->plane[1], srcImage->plane[1], width, height / 2,
                           dstImage->stride[1], srcImage->stride[1]);
        break;
    case DEV_IMAGE_FORMAT_YUV422NV16:
        DevImage_CopyPlane(dstImage->plane[0], srcImage->plane[0], width, height,
                           dstImage->stride[0], srcImage->stride[0]);
        DevImage_CopyPlane(dstImage->plane[1], srcImage->plane[1], width, height,
                           dstImage->stride[1], srcImage->stride[1]);
        break;
    case DEV_IMAGE_FORMAT_YUV420P010:
        width *= 2;
        DevImage_CopyPlane(dstImage->plane[0], srcImage->plane[0], width, height,
                           dstImage->stride[0], srcImage->stride[0]);
        DevImage_CopyPlane(dstImage->plane[1], srcImage->plane[1], width, height / 2,
                           dstImage->stride[1], srcImage->stride[1]);
        break;
    case DEV_IMAGE_FORMAT_Y8:
        DevImage_CopyPlane(dstImage->plane[0], srcImage->plane[0], width, height,
                           dstImage->stride[0], srcImage->stride[0]);
        break;
    case DEV_IMAGE_FORMAT_Y16:
        width *= 2;
        DevImage_CopyPlane(dstImage->plane[0], srcImage->plane[0], width, height,
                           dstImage->stride[0], srcImage->stride[0]);
        break;
    case DEV_IMAGE_FORMAT_RGBA888:
        DEV_LOGE("format not support");
        return RET_ERR;
        break;
    default:
        DEV_LOGE("format not support");
        return RET_ERR;
        break;
    }
    return RET_OK;
}

S64 DevImage_DumpImage(DEV_IMAGE_BUF *image, const char *nodeName, U32 id, const char *fileName)
{
    DEV_IF_LOGE_RETURN_RET((image == NULL), RET_ERR_ARG, "buf addr err");
    DEV_IF_LOGE_RETURN_RET((fileName == NULL), RET_ERR_ARG, "name err");
    DEV_IF_LOGE_RETURN_RET((image->width == 0), RET_ERR_ARG, "width err");
    DEV_IF_LOGE_RETURN_RET((image->height == 0), RET_ERR_ARG, "height err");
    char dirFileName[128];
    char timeStr[64];
    Device->time->GetTimeStr(timeStr);
    sprintf(dirFileName, "%s/%s_%s_%dx%d_%d_%s", mialgo2::PluginUtils::getDumpPath(), nodeName,
            timeStr, image->width, image->height, id, fileName);
    S64 fd = Device->file->Open(dirFileName);
    DEV_IF_LOGE_RETURN_RET((fd == 0), RET_ERR_ARG, "open file err");
    U32 offset = 0;
    char *addr = 0;
    int width = image->width;
    int height = image->height;
    switch (image->format) {
    case DEV_IMAGE_FORMAT_YUV420NV12:
    case DEV_IMAGE_FORMAT_YUV420NV21:
        offset = 0;
        addr = image->plane[0];
        for (int i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[0];
        }

        addr = image->plane[1];
        for (int i = 0; i < height / 2; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[1];
        }
        break;
    case DEV_IMAGE_FORMAT_YUV422NV16:
        offset = 0;
        addr = image->plane[0];
        for (int i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[0];
        }

        addr = image->plane[1];
        for (int i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[1];
        }
        break;
    case DEV_IMAGE_FORMAT_YUV420P010:
        width *= 2;
        offset = 0;
        addr = image->plane[0];
        for (int i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[0];
        }

        addr = image->plane[1];
        for (int i = 0; i < height / 2; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[1];
        }
        break;
    case DEV_IMAGE_FORMAT_Y8:
        offset = 0;
        addr = image->plane[0];
        for (int i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[0];
        }
        break;
    case DEV_IMAGE_FORMAT_Y16:
        width *= 2;
        offset = 0;
        addr = image->plane[0];
        for (int i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[0];
        }
        break;
    case DEV_IMAGE_FORMAT_RGBA888:
        DEV_LOGE("format not support");
        return RET_ERR;
        break;
    default:
        DEV_LOGE("format not support");
        return RET_ERR;
        break;
    }
    Device->file->Close(&fd);
    return RET_OK;
}

S64 DevImage_DumpData(void *buf, U32 size, const char *nodeName, U32 id, const char *fileName)
{
    DEV_IF_LOGE_RETURN_RET((buf == NULL), RET_ERR_ARG, "buf addr err");
    DEV_IF_LOGE_RETURN_RET((fileName == NULL), RET_ERR_ARG, "name err");
    char dirFileName[128];
    char timeStr[64];
    Device->time->GetTimeStr(timeStr);
    sprintf(dirFileName, "%s/%s_%s_%d_%s", mialgo2::PluginUtils::getDumpPath(), nodeName, timeStr,
            id, fileName);
    return Device->file->WriteOnce(dirFileName, buf, size, 0);
}

S64 DevImage_MatchToImage(void *srcData, DEV_IMAGE_BUF *dstImage)
{
    DEV_IF_LOGE_RETURN_RET((srcData == NULL), RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET((dstImage == NULL), RET_ERR_ARG, "arg err");
    ImageParams *hInput = (ImageParams *)srcData;
    switch (hInput->format.format) {
    case mialgo2::CAM_FORMAT_YUV_420_NV12:
        dstImage->format = DEV_IMAGE_FORMAT_YUV420NV12;
        break;
    case mialgo2::CAM_FORMAT_YUV_420_NV21:
        dstImage->format = DEV_IMAGE_FORMAT_YUV420NV21;
        break;
    case mialgo2::CAM_FORMAT_P010:
        dstImage->format = DEV_IMAGE_FORMAT_YUV420P010;
        break;
    case mialgo2::CAM_FORMAT_Y8:
        dstImage->format = DEV_IMAGE_FORMAT_Y8;
        break;
    case mialgo2::CAM_FORMAT_Y16:
        dstImage->format = DEV_IMAGE_FORMAT_Y16;
        break;
    default:
        DEV_LOGE("format not support=%d", hInput->format.format);
        return RET_ERR;
        break;
    }
    dstImage->width = hInput->format.width;
    dstImage->height = hInput->format.height;
    U32 m_numberOfPlanes = 0;
    if (hInput->numberOfPlanes > DEV_IMAGE_MAX_PLANE) {
        m_numberOfPlanes = DEV_IMAGE_MAX_PLANE;
        DEV_LOGE("MatchToImage numberOfPlanes %d>%d err", hInput->numberOfPlanes,
                 DEV_IMAGE_MAX_PLANE);
    } else {
        m_numberOfPlanes = hInput->numberOfPlanes;
    }
    for (UINT j = 0; j < m_numberOfPlanes; j++) {
        dstImage->stride[j] = hInput->planeStride;
        dstImage->sliceHeight[j] =
            hInput->format.height; // hInput->format.formatParams.yuvFormat[j].sliceHeight;
        dstImage->plane[j] = (char *)hInput->pAddr[j];
    }
    return RET_OK;
}

S64 DevImage_MatchToVoid(DEV_IMAGE_BUF *srcImage, void *dstData)
{
    // TODO
    return RET_ERR;
}

S64 DevImage_ReSize(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage)
{
    DEV_IF_LOGE_RETURN_RET((srcImage == NULL), RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET((dstImage == NULL), RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET((srcImage->format != dstImage->format), RET_ERR_ARG, "format err");
    DEV_IF_LOGE_RETURN_RET(((dstImage->width == 0) || (dstImage->height == 0)), RET_ERR_ARG,
                           "width height err");
    if ((srcImage->width == dstImage->width) && (srcImage->height == dstImage->height)) {
        return DevImage_Copy(dstImage, srcImage);
    }
    switch (srcImage->format) {
    case DEV_IMAGE_FORMAT_Y8: {
        U32 type = 1;
        if (type == 1) {
            unsigned int wrfloat_16 = (srcImage->width << 16) / dstImage->width + 1;
            unsigned int hrfloat_16 = (srcImage->height << 16) / dstImage->height + 1;
            unsigned char *pDstRow = (unsigned char *)dstImage->plane[0];
            unsigned char *pSrcRow = (unsigned char *)srcImage->plane[0];
            unsigned int srcy_16 = 0;
            for (int r = 0; r < (int)dstImage->height; r++) {
                pSrcRow =
                    (unsigned char *)srcImage->plane[0] + srcImage->stride[0] * (srcy_16 >> 16);
                unsigned int srcx_16 = 0;
                for (int c = 0; c < (int)dstImage->width; c++) {
                    pDstRow[c] = pSrcRow[srcx_16 >> 16];
                    srcx_16 += wrfloat_16;
                }
                srcy_16 += hrfloat_16;
                pDstRow += dstImage->stride[0];
            }
        } else if (type == 3) {
            unsigned int wrfloat_16_3 = ((srcImage->width << 16) / dstImage->width + 1);
            unsigned int hrfloat_16 = (srcImage->height << 16) / dstImage->height + 1;
            unsigned char *pDstRow = (unsigned char *)dstImage->plane[0];
            unsigned char *pSrcRow = (unsigned char *)srcImage->plane[0];
            unsigned int srcy_16 = 0;
            for (int r = 0; r < (int)dstImage->height; r++) {
                pSrcRow =
                    (unsigned char *)srcImage->plane[0] + srcImage->stride[0] * (srcy_16 >> 16);
                unsigned int srcx_16 = 0;
                for (int c = 0; c < (int)dstImage->width; c++) {
                    unsigned int tmppos = ((srcx_16) >> 16) * 3;
                    unsigned int tmppos1 = c * 3;
                    pDstRow[tmppos1] = pSrcRow[tmppos];
                    pDstRow[tmppos1 + 1] = pSrcRow[tmppos + 1];
                    pDstRow[tmppos1 + 2] = pSrcRow[tmppos + 2];
                    srcx_16 += (wrfloat_16_3);
                }
                srcy_16 += hrfloat_16;
                pDstRow += dstImage->stride[0];
            }
        }
    } break;
    case DEV_IMAGE_FORMAT_YUV420NV12:
    case DEV_IMAGE_FORMAT_YUV420NV21:
    case DEV_IMAGE_FORMAT_YUV422NV16:
    case DEV_IMAGE_FORMAT_YUV420P010:
    case DEV_IMAGE_FORMAT_Y16:
    case DEV_IMAGE_FORMAT_RGBA888:
    default:
        DEV_LOGE("format not support");
        return RET_ERR;
        break;
    }
    return RET_OK;
}

S64 DevImage_Yuv420sp2Rgba(char *py, char *puv, char *rgba, int stride_y, int stride_uv, int width,
                           int height)
{
    DEV_IF_LOGE_RETURN_RET((py == NULL), RET_ERR_ARG, "py addr err");
    DEV_IF_LOGE_RETURN_RET((puv == NULL), RET_ERR_ARG, "puv addr err");
    DEV_IF_LOGE_RETURN_RET((rgba == NULL), RET_ERR_ARG, "rgba addr err");
    DEV_IF_LOGE_RETURN_RET((width == 0), RET_ERR_ARG, "width err");
    DEV_IF_LOGE_RETURN_RET((height == 0), RET_ERR_ARG, "height err");
    char *yline, *uvline;
    int y, v, u, r, g, b;
    for (int i = 0; i < height; i++) {
        yline = py + i * stride_y;
        uvline = puv + i * stride_uv / 2;
        for (int j = 0; j < width; j++) {
            y = *yline++;
            v = uvline[0];
            u = uvline[1];
            b = (int)(y + 2.03211 * (u - 128));
            g = (int)(y - 0.39465 * (u - 128) - 0.5806 * (v - 128));
            r = (int)(y + 1.13983 * (v - 128));
            if (r < 0) {
                r = 0;
            }
            if (r > 255) {
                r = 255;
            }
            if (g < 0) {
                g = 0;
            }
            if (g > 255) {
                g = 255;
            }
            if (b < 0) {
                b = 0;
            }
            if (b > 255) {
                b = 255;
            }
            *rgba++ = r;
            *rgba++ = g;
            *rgba++ = b;
            *rgba++ = 0xff;
            if (j % 2 != 0) {
                uvline += 2;
            }
        }
    }
    return RET_OK;
}

S64 DevImage_Rgba2Yuv420sp(char *py, char *puv, char *rgba, int stride_y, int stride_uv, int width,
                           int height)
{
    DEV_IF_LOGE_RETURN_RET((py == NULL), RET_ERR_ARG, "py addr err");
    DEV_IF_LOGE_RETURN_RET((puv == NULL), RET_ERR_ARG, "puv addr err");
    DEV_IF_LOGE_RETURN_RET((rgba == NULL), RET_ERR_ARG, "rgba addr err");
    DEV_IF_LOGE_RETURN_RET((width == 0), RET_ERR_ARG, "width err");
    DEV_IF_LOGE_RETURN_RET((height == 0), RET_ERR_ARG, "height err");
    char *yline, *uvline;
    int y, v, u, r, g, b;
    for (int i = 0; i < height; i++) {
        yline = py + i * stride_y;
        uvline = puv + i * stride_uv / 2;
        for (int j = 0; j < width; j++) {
            r = *rgba++;
            g = *rgba++;
            b = *rgba++;
            rgba++;
            y = (int)(0.299 * r + 0.587 * g + 0.114 * b);
            if (y < 0) {
                y = 0;
            }
            if (y > 255) {
                y = 255;
            }
            *yline++ = y;
            if ((i % 2 == 0) && (j % 2 == 0)) {
                u = (int)(-0.14713 * r - 0.28886 * g + 0.436 * b) + 128;
                v = (int)(0.615 * r - 0.51499 * g - 0.10001 * b) + 128;
                if (u < 0) {
                    u = 0;
                }
                if (u > 255) {
                    u = 255;
                }
                if (v < 0) {
                    v = 0;
                }
                if (v > 255) {
                    v = 255;
                }
                *uvline++ = v;
                *uvline++ = u;
            }
        }
    }
    return RET_OK;
}

const Dev_Image m_dev_image = {
    .MatchToImage = DevImage_MatchToImage,
    .MatchToVoid = DevImage_MatchToVoid,
    .Copy = DevImage_Copy,
    .Alloc = DevImage_Alloc,
    .Free = DevImage_Free,
    .DumpImage = DevImage_DumpImage,
    .DumpData = DevImage_DumpData,
    .ReSize = DevImage_ReSize,
    .Yuv420sp2Rgba = DevImage_Yuv420sp2Rgba,
    .Rgba2Yuv420sp = DevImage_Rgba2Yuv420sp,
};
