/**
 * @file        dev_image.cpp
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

#include <stdlib.h>
#include <string.h>
#include "dev_type.h"
#include "device.h"
#include "dev_image.h"

#if defined(MIVIFWK_CDK_OS)  //MIMV
#include "MiaPluginWraper.h"
#include "MiaPluginUtils.h"
#include <cutils/native_handle.h>
#include <system/camera_metadata.h>
#endif
#if defined(CHI_CDK_OS)   //CHI
#include "chinode.h"
#endif


#define DEV_IMAGE_NUM_MAX     128

typedef struct __DEV_IMAGE_NODE {
    U32 en;
    CHAR __fileName[FILE_NAME_LEN_MAX];
    CHAR __tagName[FILE_NAME_LEN_MAX];
    U32 __fileLine;
    DEV_IMAGE_BUF *image;
} DEV_IMAGE_NODE;

static DEV_IMAGE_NODE m_image_list[DEV_IMAGE_NUM_MAX] = { { 0 } };
static U32 init_f = FALSE;
static S64 m_mutex_Id;

static const char* DevImage_NoDirFileName(const char *pFilePath) {
    if (pFilePath == NULL) {
        return NULL;
    }
    const char *pFileName = strrchr(pFilePath, '/');
    if (NULL != pFileName) {
        pFileName += 1;
    } else {
        pFileName = pFilePath;
    }
    return pFileName;
}

S64 DevImage_Alloc(DEV_IMAGE_BUF *image, U32 width, U32 stride, U32 height, U32 sliceHeight, U32 offset, DEV_IMAGE_FORMAT format, MARK_TAG tagName) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((image==NULL), RET_ERR_ARG, "buf addr err");
    DEV_IF_LOGE_RETURN_RET((stride < width), RET_ERR_ARG, "stride err");
    DEV_IF_LOGE_RETURN_RET((sliceHeight < height), RET_ERR_ARG, "sliceHeight err");
    char *addr = NULL;
    U32 tempStride = 0;
    U32 tempWidth = 0;
    memset(image, 0, sizeof(DEV_IMAGE_BUF));
    U32 size = 0;
    char imageTagName[FILE_NAME_LEN_MAX + 32] = { 0 };
    Dev_snprintf(imageTagName, FILE_NAME_LEN_MAX + 32, "imageAlloc_%s", tagName);
    switch (format) {
    case DEV_IMAGE_FORMAT_YUV420NV12:
    case DEV_IMAGE_FORMAT_YUV420NV21:
        size = (stride * sliceHeight * 3 / 2) + offset;
        //addr = (char*) malloc(size);
        addr = (char*) Device->memoryPool->Malloc(size, __fileName, __fileLine, __pid(),__tid(), imageTagName);
        DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "malloc err");
        image->plane[0] = addr + offset;
        image->planeSize[0] = width * height;
        image->stride[0] = stride;
        image->sliceHeight[0] = sliceHeight;
        image->plane[1] = image->plane[0] + stride * sliceHeight;
        image->planeSize[1] = (width * height) / 2;
        image->stride[1] = stride;
        image->sliceHeight[1] = sliceHeight;
        image->planeCount = 2;
        break;
    case DEV_IMAGE_FORMAT_YUV422NV16:
        size = (stride * sliceHeight * 2) + offset;
        //addr = (char*) malloc(size);
        addr = (char*) Device->memoryPool->Malloc(size, __fileName, __fileLine, __pid(),__tid(), imageTagName);
        DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "malloc err");
        image->plane[0] = addr + offset;
        image->planeSize[0] = width * height;
        image->stride[0] = stride;
        image->sliceHeight[0] = sliceHeight;
        image->plane[1] = image->plane[0] + stride * sliceHeight;
        image->planeSize[1] = width * height;
        image->stride[1] = stride;
        image->sliceHeight[1] = sliceHeight;
        image->planeCount = 2;
        break;
    case DEV_IMAGE_FORMAT_YUV420P010:
        tempStride = stride * 2;
        tempWidth = width * 2;
        size = (tempStride * sliceHeight * 3 / 2) + offset;
        //addr = (char*) malloc(size);
        addr = (char*) Device->memoryPool->Malloc(size, __fileName, __fileLine, __pid(),__tid(), imageTagName);
        DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "malloc err");
        image->plane[0] = addr + offset;
        image->planeSize[0] = tempWidth * height;
        image->stride[0] = tempStride / 2;
        image->sliceHeight[0] = sliceHeight;
        image->plane[1] = image->plane[0] + tempStride * sliceHeight;
        image->planeSize[1] = (tempWidth * height) / 2;
        image->stride[1] = tempStride / 2;
        image->sliceHeight[1] = sliceHeight;
        image->planeCount = 2;
        break;
    case DEV_IMAGE_FORMAT_Y8:
        size = (stride * sliceHeight) + offset;
        //addr = (char*) malloc(size);
        addr = (char*) Device->memoryPool->Malloc(size, __fileName, __fileLine, __pid(),__tid(), imageTagName);
        DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "malloc err");
        image->plane[0] = addr + offset;
        image->planeSize[0] = width * height;
        image->stride[0] = stride;
        image->sliceHeight[0] = sliceHeight;
        image->planeCount = 1;
        break;
    case DEV_IMAGE_FORMAT_RAWPLAIN16:
    case DEV_IMAGE_FORMAT_Y16:
        tempStride = stride * 2;
        tempWidth = width * 2;
        size = (tempStride * sliceHeight) + offset;
        //addr = (char*) malloc(size);
        addr = (char*) Device->memoryPool->Malloc(size, __fileName, __fileLine, __pid(),__tid(), imageTagName);
        DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "malloc err");
        image->plane[0] = addr + offset;
        image->planeSize[0] = tempWidth * height;
        image->stride[0] = tempStride / 2;
        image->sliceHeight[0] = sliceHeight;
        image->planeCount = 1;
        break;
    case DEV_IMAGE_FORMAT_RAWPLAIN10:
        size = (((stride * sliceHeight * 5) / 4)) + offset;
        //addr = (char*) malloc(size);
        addr = (char*) Device->memoryPool->Malloc(size, __fileName, __fileLine, __pid(),__tid(), imageTagName);
        DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "malloc err");
        image->plane[0] = addr + offset;
        image->planeSize[0] = ((width * height * 5) / 4);
        image->stride[0] = stride;
        image->sliceHeight[0] = sliceHeight;
        image->planeCount = 1;
        break;
    case DEV_IMAGE_FORMAT_RGBA888:
    default:
        DEV_LOGE("format not support");
        return RET_ERR;
        break;
    }
    image->format = format;
    image->width = width;
    image->height = height;
    image->offset = offset;
    /********************************/
    //记录申请的image
    Device->mutex->Lock(m_mutex_Id);
    S64 ret = RET_ERR;
    S64 m_id = 0;
    for (S64 id = 1; id < DEV_IMAGE_NUM_MAX; id++) {
        if (m_image_list[id].image != NULL) {
            if (m_image_list[id].image == image) {
                m_id = id;
                DEV_IF_LOGW_GOTO(m_image_list[id].en == TRUE, exit, "double Create %p", image);
            }
        }
    }
    m_id = 0;
    for (S64 id = 1; id < DEV_IMAGE_NUM_MAX; id++) {
        if (m_image_list[id].en == FALSE) {
            m_image_list[id].image = image;
            m_image_list[id].en = TRUE;
            m_image_list[id].__fileLine = __fileLine;
            Dev_sprintf(m_image_list[id].__fileName, "%s", DevImage_NoDirFileName(__fileName));
            Dev_sprintf(m_image_list[id].__tagName, "%s", tagName);
            m_id = id;
            break;
        }
    }
    ret = RET_OK;
    exit:
    Device->mutex->Unlock(m_mutex_Id);
    DEV_IF_LOGE((m_image_list[m_id].image==NULL),
            "The number of image pools is not enough, the number of threads needs to be increased DEV_IMAGE_NUM_MAX");
    /********************************/
    return ret;
}

S64 DevImage_Free(DEV_IMAGE_BUF *image) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((image==NULL), RET_ERR_ARG, "buf addr err");
    DEV_IF_LOGE_RETURN_RET((image->plane[0]==NULL), RET_ERR_ARG, "no image buff");
    char *addr = image->plane[0] - image->offset;
    DEV_IF_LOGE_RETURN_RET((addr==NULL), RET_ERR_ARG, "no image buff");
//    free(addr);
    Device->memoryPool->Free(addr);
    addr = NULL;
    /********************************/
    //删除记录申请的image
    Device->mutex->Lock(m_mutex_Id);
    for (S64 id = 1; id < DEV_IMAGE_NUM_MAX; id++) {
        if (m_image_list[id].image != NULL) {
            if (m_image_list[id].image == image) {
                if (m_image_list[id].en == TRUE) {
                    memset((void*) &m_image_list[id], 0, sizeof(DEV_IMAGE_NODE));
                }
            }
        }
    }
    Device->mutex->Unlock(m_mutex_Id);
    /********************************/
    return RET_OK;
}

static S64 DevImage_CopyPlane(char *dst, char *src, U32 width, U32 height, U32 dst_stride, U32 src_stride) {
    DEV_IF_LOGE_RETURN_RET((dst==NULL), RET_ERR_ARG, "dst addr err");
    DEV_IF_LOGE_RETURN_RET((src==NULL), RET_ERR_ARG, "py addr err");
    for (U32 i = 0; i < height; i++) {
        memcpy(dst, src, width);
        dst += dst_stride;
        src += src_stride;
    }
    return RET_OK;
}

S64 DevImage_Copy(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage) {
    DEV_IF_LOGE_RETURN_RET((dstImage==NULL), RET_ERR_ARG, "dst addr err");
    DEV_IF_LOGE_RETURN_RET((srcImage==NULL), RET_ERR_ARG, "py addr err");
    DEV_IF_LOGE_RETURN_RET(
            ((dstImage->width != srcImage->width) || (dstImage->height != srcImage->height))
                    && ((dstImage->width != srcImage->height) || (dstImage->height != srcImage->width)), RET_ERR_ARG, "size err");
    U32 width = srcImage->width;
    U32 height = srcImage->height;
    switch (srcImage->format) {
    case DEV_IMAGE_FORMAT_YUV420NV12:
    case DEV_IMAGE_FORMAT_YUV420NV21:
        DEV_IF_LOGE_RETURN_RET((srcImage->format != dstImage->format), RET_ERR_ARG, "format err[%d:%d]",srcImage->format, dstImage->format);
        DevImage_CopyPlane(dstImage->plane[0], srcImage->plane[0], width, height, dstImage->stride[0], srcImage->stride[0]);
        DevImage_CopyPlane(dstImage->plane[1], srcImage->plane[1], width, height / 2, dstImage->stride[1], srcImage->stride[1]);
        break;
    case DEV_IMAGE_FORMAT_YUV422NV16:
        DEV_IF_LOGE_RETURN_RET((srcImage->format != dstImage->format), RET_ERR_ARG, "format err");
        DevImage_CopyPlane(dstImage->plane[0], srcImage->plane[0], width, height, dstImage->stride[0], srcImage->stride[0]);
        DevImage_CopyPlane(dstImage->plane[1], srcImage->plane[1], width, height, dstImage->stride[1], srcImage->stride[1]);
        break;
    case DEV_IMAGE_FORMAT_YUV420P010:
        DEV_IF_LOGE_RETURN_RET((srcImage->format != dstImage->format), RET_ERR_ARG, "format err");
        width *= 2;
        DevImage_CopyPlane(dstImage->plane[0], srcImage->plane[0], width, height, dstImage->stride[0], srcImage->stride[0]);
        DevImage_CopyPlane(dstImage->plane[1], srcImage->plane[1], width, height / 2, dstImage->stride[1], srcImage->stride[1]);
        break;
    case DEV_IMAGE_FORMAT_RAWPLAIN10:
        if (dstImage->format == DEV_IMAGE_FORMAT_RAWPLAIN10) {
            DevImage_CopyPlane(dstImage->plane[0], srcImage->plane[0], ((width * 5) / 4), height, dstImage->stride[0], srcImage->stride[0]);
        } else if (dstImage->format == DEV_IMAGE_FORMAT_RAWPLAIN16) {
            DEV_LOGI("Quad CFA dummy unpacking (mipi10 --> plain16)");
            for (U32 j = 0; j < dstImage->planeCount; j++) {
                U8* pIn = (U8*) srcImage->plane[j];
                U8* pOut = (U8*) dstImage->plane[j];
                U32 count = (srcImage->width * srcImage->height * 5) / 4;
                for (U32 i = 0; i < count; i = i + 5) {
                    *pOut = ((*(pIn + 4)) & 0b00000011) | ((*pIn & 0b00111111) << 2);
                    pOut++;
                    *pOut = ((*pIn) & 0b11000000) >> 6;
                    pOut++;

                    *pOut = (((*(pIn + 4)) & 0b00001100) >> 2) | ((*(pIn + 1) & 0b00111111) << 2);
                    pOut++;
                    *pOut = ((*(pIn + 1)) & 0b11000000) >> 6;
                    pOut++;

                    *pOut = (((*(pIn + 4)) & 0b00110000) >> 4) | ((*(pIn + 2) & 0b00111111) << 2);
                    pOut++;
                    *pOut = ((*(pIn + 2)) & 0b11000000) >> 6;
                    pOut++;

                    *pOut = (((*(pIn + 4)) & 0b11000000) >> 6) | ((*(pIn + 3) & 0b00111111) << 2);
                    pOut++;
                    *pOut = ((*(pIn + 3)) & 0b11000000) >> 6;
                    pOut++;
                    pIn = pIn + 5;
                }
            }
        } else {
            DEV_IF_LOGE_RETURN_RET((srcImage->format != dstImage->format), RET_ERR_ARG, "format err");
        }
        break;
    case DEV_IMAGE_FORMAT_Y8:
        DEV_IF_LOGE_RETURN_RET((srcImage->format != dstImage->format), RET_ERR_ARG, "format err");
        DevImage_CopyPlane(dstImage->plane[0], srcImage->plane[0], width, height, dstImage->stride[0], srcImage->stride[0]);
        break;
    case DEV_IMAGE_FORMAT_RAWPLAIN16:
    case DEV_IMAGE_FORMAT_Y16:
        DEV_IF_LOGE_RETURN_RET((srcImage->format != dstImage->format), RET_ERR_ARG, "format err");
        width *= 2;
        DevImage_CopyPlane(dstImage->plane[0], srcImage->plane[0], width, height, dstImage->stride[0], srcImage->stride[0]);
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

S64 DevImage_Clone(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage, MARK_TAG tagName) {
    DEV_IF_LOGE_RETURN_RET((dstImage==NULL), RET_ERR_ARG, "dst addr err");
    DEV_IF_LOGE_RETURN_RET((srcImage==NULL), RET_ERR_ARG, "py addr err");
    S64 ret = DevImage_Alloc(dstImage, srcImage->width, srcImage->stride[0], srcImage->height, srcImage->sliceHeight[0], srcImage->offset,
            (DEV_IMAGE_FORMAT) srcImage->format, __fileName, __fileLine, __pid(),__tid(), tagName);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR_ARG, "Alloc err");
    return DevImage_Copy(dstImage, srcImage);
}

S64 DevImage_DumpImage(DEV_IMAGE_BUF *image, const char* nodeName, U32 id, const char *fileName) {
    DEV_IF_LOGE_RETURN_RET((image==NULL), RET_ERR_ARG, "buf addr err");
    DEV_IF_LOGE_RETURN_RET((fileName==NULL), RET_ERR_ARG, "name err");
    DEV_IF_LOGE_RETURN_RET((image->width == 0), RET_ERR_ARG, "width err");
    DEV_IF_LOGE_RETURN_RET((image->height == 0), RET_ERR_ARG, "height err");
    char dirFileName[128];
    char timeStr[64];
    Device->time->GetTimeStr(timeStr);
    sprintf(dirFileName, "%s/%s_%s_%dx%d_FORMAT_%d_%d_%s", DEV_IMAGE_DUMP_DIR, nodeName, timeStr, image->width, image->height, image->format, id, fileName);
    S64 fd = 0;
    Device->file->Open(&fd, dirFileName, MARK("dirFileName"));
    DEV_IF_LOGE_RETURN_RET((fd == 0), RET_ERR_ARG, "open file err");
    U32 offset = 0;
    char *addr = 0;
    U32 width = image->width;
    U32 height = image->height;
    switch (image->format) {
    case DEV_IMAGE_FORMAT_YUV420NV12:
    case DEV_IMAGE_FORMAT_YUV420NV21:
        offset = 0;
        addr = image->plane[0];
        for (U32 i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[0];
        }

        addr = image->plane[1];
        for (U32 i = 0; i < height / 2; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[1];
        }
        break;
    case DEV_IMAGE_FORMAT_YUV422NV16:
        offset = 0;
        addr = image->plane[0];
        for (U32 i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[0];
        }

        addr = image->plane[1];
        for (U32 i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[1];
        }
        break;
    case DEV_IMAGE_FORMAT_YUV420P010:
        width *= 2;
        offset = 0;
        addr = image->plane[0];
        for (U32 i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[0];
        }

        addr = image->plane[1];
        for (U32 i = 0; i < height / 2; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[1];
        }
        break;
    case DEV_IMAGE_FORMAT_Y8:
        offset = 0;
        addr = image->plane[0];
        for (U32 i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[0];
        }
        break;
    case DEV_IMAGE_FORMAT_RAWPLAIN10:
        width = ((width * 5) / 4);
        offset = 0;
        addr = image->plane[0];
        for (U32 i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[0];
        }
        break;
    case DEV_IMAGE_FORMAT_RAWPLAIN16:
    case DEV_IMAGE_FORMAT_Y16:
        width *= 2;
        offset = 0;
        addr = image->plane[0];
        for (U32 i = 0; i < height; i++) {
            Device->file->Write(fd, addr, width, offset);
            offset += width;
            addr += image->stride[0];
        }
        break;
    case DEV_IMAGE_FORMAT_RGBA888:
    default:
        DEV_LOGE("format not support");
        return RET_ERR;
        break;
    }
    Device->file->Close(&fd);
    DEV_LOGI("DUMP IMAGE:[%s]", dirFileName);
    return RET_OK;
}

S64 DevImage_DumpData(void *buf, U32 size, const char* nodeName, U32 id, const char *fileName) {
    DEV_IF_LOGE_RETURN_RET((buf==NULL), RET_ERR_ARG, "buf addr err");
    DEV_IF_LOGE_RETURN_RET((fileName==NULL), RET_ERR_ARG, "name err");
    char dirFileName[128];
    char timeStr[64];
    Device->time->GetTimeStr(timeStr);
    sprintf(dirFileName, "%s/%s_%s_%d_%s", DEV_IMAGE_DUMP_DIR, nodeName, timeStr, id, fileName);
    return Device->file->WriteOnce(dirFileName, buf, size, 0);
}

S64 DevImage_MatchToImage(void *srcData, DEV_IMAGE_BUF *dstImage) {
    DEV_IF_LOGE_RETURN_RET((srcData==NULL), RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET((dstImage==NULL), RET_ERR_ARG, "arg err");
#if defined(CHI_CDK_OS)   //CHI
    CHINODEBUFFERHANDLE hInput = (CHINODEBUFFERHANDLE) srcData;
    dstImage->width = hInput->format.width;
    dstImage->height = hInput->format.height;
    if (hInput->numberOfPlanes > DEV_IMAGE_MAX_PLANE) {
        dstImage->planeCount = DEV_IMAGE_MAX_PLANE;
        DEV_LOGE("MatchToImage numberOfPlanes %d>%d err", hInput->numberOfPlanes, DEV_IMAGE_MAX_PLANE);
    } else {
        dstImage->planeCount = hInput->numberOfPlanes;
    }
    switch (hInput->format.format) {
    case YUV420NV12:
        dstImage->format = DEV_IMAGE_FORMAT_YUV420NV12;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->format.formatParams.yuvFormat[j].planeStride;
            dstImage->sliceHeight[j] = /*hInput->format.height;*/hInput->format.formatParams.yuvFormat[j].sliceHeight;
            dstImage->planeSize[j] = hInput->planeSize[j];
            dstImage->plane[j] = (char *) hInput->pImageList[0].pAddr[j];
            dstImage->fd[j] = hInput->pImageList[0].fd[j];
        }
        break;
    case YUV420NV21:
        dstImage->format = DEV_IMAGE_FORMAT_YUV420NV21;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->format.formatParams.yuvFormat[j].planeStride;
            dstImage->sliceHeight[j] = /*hInput->format.height;*/hInput->format.formatParams.yuvFormat[j].sliceHeight;
            dstImage->planeSize[j] = hInput->planeSize[j];
            dstImage->plane[j] = (char *) hInput->pImageList[0].pAddr[j];
            dstImage->fd[j] = hInput->pImageList[0].fd[j];
        }
        break;
    case YUV422NV16:
        dstImage->format = DEV_IMAGE_FORMAT_YUV422NV16;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->format.formatParams.yuvFormat[j].planeStride;
            dstImage->sliceHeight[j] = /*hInput->format.height;*/hInput->format.formatParams.yuvFormat[j].sliceHeight;
            dstImage->planeSize[j] = hInput->planeSize[j];
            dstImage->plane[j] = (char *) hInput->pImageList[0].pAddr[j];
        }
        break;
    case P010:
        dstImage->format = DEV_IMAGE_FORMAT_YUV420P010;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->format.formatParams.yuvFormat[j].planeStride;
            dstImage->sliceHeight[j] = /*hInput->format.height;*/hInput->format.formatParams.yuvFormat[j].sliceHeight;
            dstImage->planeSize[j] = hInput->planeSize[j];
            dstImage->plane[j] = (char *) hInput->pImageList[0].pAddr[j];
        }
        break;
    case Y8:
        dstImage->format = DEV_IMAGE_FORMAT_Y8;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->format.formatParams.yuvFormat[j].planeStride;
            dstImage->sliceHeight[j] = /*hInput->format.height;*/hInput->format.formatParams.yuvFormat[j].sliceHeight;
            dstImage->planeSize[j] = hInput->planeSize[j];
            dstImage->plane[j] = (char *) hInput->pImageList[0].pAddr[j];
        }
        break;
    case Y16:
        dstImage->format = DEV_IMAGE_FORMAT_Y16;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->format.formatParams.yuvFormat[j].planeStride;
            dstImage->sliceHeight[j] = /*hInput->format.height;*/hInput->format.formatParams.yuvFormat[j].sliceHeight;
            dstImage->planeSize[j] = hInput->planeSize[j];
            dstImage->plane[j] = (char *) hInput->pImageList[0].pAddr[j];
        }
        break;
    case RawMIPI:
        if (hInput->format.formatParams.rawFormat.bitsPerPixel == 10) {
            dstImage->format = DEV_IMAGE_FORMAT_RAWPLAIN10;
            for (UINT j = 0; j < dstImage->planeCount; j++) {
                dstImage->stride[j] = hInput->format.formatParams.rawFormat.stride;
                dstImage->sliceHeight[j] = /*hInput->format.height;*/hInput->format.formatParams.rawFormat.sliceHeight;
                dstImage->planeSize[j] = hInput->planeSize[j];
                dstImage->plane[j] = (char *) hInput->pImageList[0].pAddr[j];
            }
        } else {
            DEV_LOGE("format not support=%d bpp=%d", hInput->format.format, hInput->format.formatParams.rawFormat.bitsPerPixel);
        }
        break;
    case RawPlain16:
        dstImage->format = DEV_IMAGE_FORMAT_RAWPLAIN16;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->format.formatParams.rawFormat.stride;
            dstImage->sliceHeight[j] = /*hInput->format.height;*/hInput->format.formatParams.rawFormat.sliceHeight;
            dstImage->planeSize[j] = hInput->planeSize[j];
            dstImage->plane[j] = (char *) hInput->pImageList[0].pAddr[j];
        }
        break;
//    case XXX://TODO
//        dstImage->format = DEV_IMAGE_FORMAT_YUYV;
    default:
        DEV_LOGE("format not support=%d", hInput->format.format);
        return RET_ERR;
        break;
    }
    return RET_OK;
#endif

#if defined(MIVIFWK_CDK_OS)  //MIMV
#if 0
    /*******************************************************************************************************************/
    static const int32_t MaxPlanes = 3;

    enum {
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
        CAM_FORMAT_P010 = 0x7FA30C0A, // ImageFormat's P010
    };

    struct ImageFormat
    {
        uint32_t format; ///< format
        uint32_t width;  ///< width
        uint32_t height; ///< height
    };

    struct ImageParams
    {
        ImageFormat format;
        U32 planeStride;               // stride
        U32 sliceheight;               // scanline
        camera_metadata_t *pMetadata;       ///< logical camera metadata
        camera_metadata_t *pPhyCamMetadata; ///< physical camera metadata
        U32 numberOfPlanes;
        int fd[MaxPlanes];           ///< File descriptor, used by node to map the buffer to FD
        U8 *pAddr[MaxPlanes];   ///< Starting virtual address of the allocated buffer.
        const native_handle_t *pNativeHandle; ///< Native handle
        S32 bufferType;
        U32 cameraId;
        S32 roleId;
    };
    /*******************************************************************************************************************/
#endif
    ImageParams *hInput = (ImageParams*) srcData;
    dstImage->width = hInput->format.width;
    dstImage->height = hInput->format.height;
    if (hInput->numberOfPlanes > DEV_IMAGE_MAX_PLANE) {
        dstImage->planeCount = DEV_IMAGE_MAX_PLANE;
        DEV_LOGE("MatchToImage numberOfPlanes %ld>%ld err", hInput->numberOfPlanes, DEV_IMAGE_MAX_PLANE);
    } else {
        dstImage->planeCount = hInput->numberOfPlanes;
    }
    switch (hInput->format.format) {
    case CAM_FORMAT_YUV_420_NV12:
        dstImage->format = DEV_IMAGE_FORMAT_YUV420NV12;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->planeStride;
            dstImage->sliceHeight[j] = hInput->sliceheight;
            dstImage->planeSize[j] = hInput->planeStride * hInput->sliceheight;
            dstImage->plane[j] = (char*) hInput->pAddr[j];
            dstImage->fd[j] = hInput->fd[j];
        }
        break;
    case CAM_FORMAT_YUV_420_NV21:
        dstImage->format = DEV_IMAGE_FORMAT_YUV420NV21;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->planeStride;
            dstImage->sliceHeight[j] = hInput->sliceheight;
            dstImage->planeSize[j] = hInput->planeStride * hInput->sliceheight;
            dstImage->plane[j] = (char*) hInput->pAddr[j];
            dstImage->fd[j] = hInput->fd[j];
        }
        break;
/*
    case mialgo2::YUV422NV16:
        dstImage->format = DEV_IMAGE_FORMAT_YUV422NV16;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->planeStride;
            dstImage->sliceHeight[j] = hInput->sliceheight;
            dstImage->planeSize[j] = hInput->planeStride * hInput->sliceheight;
            dstImage->plane[j] = (char*) hInput->pAddr[j];
            dstImage->fd[j] = hInput->fd[j];
        }
        break;
    case mialgo2::P010:
        dstImage->format = DEV_IMAGE_FORMAT_YUV420P010;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->planeStride;
            dstImage->sliceHeight[j] = hInput->sliceheight;
            dstImage->planeSize[j] = hInput->planeStride * hInput->sliceheight;
            dstImage->plane[j] = (char*) hInput->pAddr[j];
            dstImage->fd[j] = hInput->fd[j];
        }
        break;
    case mialgo2::Y8:
        dstImage->format = DEV_IMAGE_FORMAT_Y8;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->planeStride;
            dstImage->sliceHeight[j] = hInput->sliceheight;
            dstImage->planeSize[j] = hInput->planeStride * hInput->sliceheight;
            dstImage->plane[j] = (char*) hInput->pAddr[j];
            dstImage->fd[j] = hInput->fd[j];
        }
        break;
*/
    case CAM_FORMAT_Y16:
        dstImage->format = DEV_IMAGE_FORMAT_Y16;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->planeStride;
            dstImage->sliceHeight[j] = hInput->sliceheight;
            dstImage->planeSize[j] = hInput->planeStride * hInput->sliceheight;
            dstImage->plane[j] = (char*) hInput->pAddr[j];
            dstImage->fd[j] = hInput->fd[j];
        }
        break;
    case CAM_FORMAT_RAW10:
        dstImage->format = DEV_IMAGE_FORMAT_RAWPLAIN10;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->planeStride;
            dstImage->sliceHeight[j] = hInput->sliceheight;
            dstImage->planeSize[j] = hInput->planeStride * hInput->sliceheight;
            dstImage->plane[j] = (char*) hInput->pAddr[j];
            dstImage->fd[j] = hInput->fd[j];
        }
        break;
    case CAM_FORMAT_RAW16:
        dstImage->format = DEV_IMAGE_FORMAT_RAWPLAIN16;
        for (UINT j = 0; j < dstImage->planeCount; j++) {
            dstImage->stride[j] = hInput->planeStride;
            dstImage->sliceHeight[j] = hInput->sliceheight;
            dstImage->planeSize[j] = hInput->planeStride * hInput->sliceheight;
            dstImage->plane[j] = (char*) hInput->pAddr[j];
            dstImage->fd[j] = hInput->fd[j];
        }
        break;
//    case XXX://TODO
//        dstImage->format = DEV_IMAGE_FORMAT_YUYV;
    default:
        DEV_LOGE("format not support=%ld", hInput->format.format);
        return RET_ERR;
        break;
    }
#endif
    return RET_OK;
}

S64 DevImage_MatchToVoid(DEV_IMAGE_BUF *srcImage, void *dstData) {
    return RET_ERR;
}

S64 DevImage_ReSize(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage) {
    DEV_IF_LOGE_RETURN_RET((srcImage==NULL), RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET((dstImage==NULL), RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET((srcImage->format != dstImage->format), RET_ERR_ARG, "format err");
    DEV_IF_LOGE_RETURN_RET(((dstImage->width == 0) || (dstImage->height == 0)), RET_ERR_ARG, "width height err");
    if ((srcImage->width == dstImage->width) && (srcImage->height == dstImage->height)) {
        return DevImage_Copy(dstImage, srcImage);
    }
    switch (srcImage->format) {
    case DEV_IMAGE_FORMAT_Y8: {
        unsigned int wrfloat_16 = (srcImage->width << 16) / dstImage->width + 1;
        unsigned int hrfloat_16 = (srcImage->height << 16) / dstImage->height + 1;
        unsigned char *pDstRow = (unsigned char*)dstImage->plane[0];
        unsigned char *pSrcRow = (unsigned char*)srcImage->plane[0];
        unsigned int srcy_16 = 0;
        for (int r = 0; r < (int)dstImage->height; r++) {
            pSrcRow = (unsigned char*)srcImage->plane[0] + srcImage->stride[0] * (srcy_16 >> 16);
            unsigned int srcx_16 = 0;
            for (int c = 0; c < (int)dstImage->width; c++) {
                pDstRow[c] = pSrcRow[srcx_16 >> 16];
                srcx_16 += wrfloat_16;
            }
            srcy_16 += hrfloat_16;
            pDstRow += dstImage->stride[0];
        }
    }
        break;
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

S64 DevImage_Merge(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage, S32 downsize) {
    DEV_IF_LOGE_RETURN_RET((srcImage==NULL), RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET((dstImage==NULL), RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET((srcImage->format != dstImage->format), RET_ERR_ARG, "format err");
    DEV_IF_LOGE_RETURN_RET(((dstImage->width == 0) || (dstImage->height == 0)), RET_ERR_ARG, "width height err");
    DEV_IF_LOGE_RETURN_RET(((srcImage->width == 0) || (srcImage->height == 0)), RET_ERR_ARG, "width height err");
#if 0
    if ((srcImage->width == dstImage->width) && (srcImage->height == dstImage->height)) {
        return DevImage_Copy(dstImage, srcImage);
    }
#endif
    if (downsize <= 0) {
        return DevImage_Copy(dstImage, srcImage);
    }
    switch (srcImage->format) {
    case DEV_IMAGE_FORMAT_YUV420NV12:
    case DEV_IMAGE_FORMAT_YUV420NV21: {
        U32 i, j, index;
        char *l_pBase = NULL;
        char *l_pOver = NULL;

        l_pBase = dstImage->plane[0];
        l_pOver = srcImage->plane[0];
        for (i = 0; i < srcImage->height;) {
            for (j = 0, index = 0; j < srcImage->width; index++, j += downsize) {
                *(l_pBase + index) = *(l_pOver + j);
            }
            l_pBase += dstImage->stride[0];
            l_pOver += srcImage->stride[0] * downsize;
            i += downsize;
        }

        l_pBase = dstImage->plane[1];
        l_pOver = srcImage->plane[1];
        for (i = 0; i < srcImage->height / 2;) {
            for (j = 0, index = 0; j < srcImage->width / 2; index++, j += downsize) {
                *(l_pBase + 2 * index) = *(l_pOver + 2 * j);
                *(l_pBase + 2 * index + 1) = *(l_pOver + 2 * j + 1);
            }

            l_pBase += dstImage->stride[0];
            l_pOver += srcImage->stride[0] * downsize;
            i += downsize;
        }
    }
        break;
    case DEV_IMAGE_FORMAT_Y8:
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

S64 DevImage_Convert(DEV_IMAGE_BUF *dstImage, DEV_IMAGE_BUF *srcImage) {
    DEV_IF_LOGE_RETURN_RET((srcImage==NULL), RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET((dstImage==NULL), RET_ERR_ARG, "arg err");
    DEV_IF_LOGE_RETURN_RET((srcImage->format == srcImage->format), RET_ERR_ARG, "The format is the same, use image Copy");
    switch (srcImage->format) {
    case DEV_IMAGE_FORMAT_YUV420NV12:
    case DEV_IMAGE_FORMAT_YUV420NV21: {
        switch (dstImage->format) {
        case DEV_IMAGE_FORMAT_RGBA888: {
            char *py = srcImage->plane[0];
            char *puv = srcImage->plane[1];
            char *rgba = dstImage->plane[0];
            int stride_y = srcImage->stride[0];
            int stride_uv = srcImage->stride[0];
            int width = srcImage->width;
            int height = srcImage->height;
            DEV_IF_LOGE_RETURN_RET((py==NULL), RET_ERR_ARG, "py addr err");
            DEV_IF_LOGE_RETURN_RET((puv==NULL), RET_ERR_ARG, "puv addr err");
            DEV_IF_LOGE_RETURN_RET((rgba==NULL), RET_ERR_ARG, "rgba addr err");
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
                    b = (int) (y + 2.03211 * (u - 128));
                    g = (int) (y - 0.39465 * (u - 128) - 0.5806 * (v - 128));
                    r = (int) (y + 1.13983 * (v - 128));
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
        }
            break;
        default:
            DEV_LOGE("format not support");
            return RET_ERR;
            break;
        }
    }
        break;
    case DEV_IMAGE_FORMAT_RGBA888: {
        switch (dstImage->format) {
        case DEV_IMAGE_FORMAT_YUV420NV12:
        case DEV_IMAGE_FORMAT_YUV420NV21: {
            char *py = dstImage->plane[0];
            char *puv = dstImage->plane[1];
            char *rgba = srcImage->plane[0];
            int stride_y = dstImage->stride[0];
            int stride_uv = dstImage->stride[0];
            int width = srcImage->width;
            int height = srcImage->height;
            DEV_IF_LOGE_RETURN_RET((py==NULL), RET_ERR_ARG, "py addr err");
            DEV_IF_LOGE_RETURN_RET((puv==NULL), RET_ERR_ARG, "puv addr err");
            DEV_IF_LOGE_RETURN_RET((rgba==NULL), RET_ERR_ARG, "rgba addr err");
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
                    y = (int) (0.299 * r + 0.587 * g + 0.114 * b);
                    if (y < 0) {
                        y = 0;
                    }
                    if (y > 255) {
                        y = 255;
                    }
                    *yline++ = y;
                    if ((i % 2 == 0) && (j % 2 == 0)) {
                        u = (int) (-0.14713 * r - 0.28886 * g + 0.436 * b) + 128;
                        v = (int) (0.615 * r - 0.51499 * g - 0.10001 * b) + 128;
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
        }
            break;
        default:
            DEV_LOGE("format not support");
            return RET_ERR;
            break;
        }
    }
        break;
    default:
        DEV_LOGE("format not support");
        return RET_ERR;
        break;
    }

    return RET_OK;
}

typedef struct DevRectEXT {
    U32 left;                           ///< x coordinate for top-left point
    U32 top;                            ///< y coordinate for top-left point
    U32 right;                          ///< x coordinate for bottom-right point
    U32 bottom;                         ///< y coordinate for bottom-right point
} DEVRECTEXT;

static S64 DevImage_DrawPoint(DEV_IMAGE_BUF* image, DEV_IMAGE_POINT xPoint, S32 nPointWidth, S32 iType) {
    DEV_IF_LOGE_RETURN_RET((image==NULL), RET_ERR_ARG, "py addr err");
    S32 nPoints = 10;
    nPoints = (nPoints < nPointWidth) ? nPointWidth : nPoints;
    int nStartX = xPoint.x - (nPoints / 2);
    int nStartY = xPoint.y - (nPoints / 2);
    nStartX = nStartX < 0 ? 0 : nStartX;
    nStartY = nStartY < 0 ? 0 : nStartY;
    if (nStartX > (int)image->width) {
        nStartX = image->width - nPoints;
    }
    if (nStartY > (int)image->height) {
        nStartY = image->height - nPoints;
    }
    if (image->format == DEV_IMAGE_FORMAT_YUV420NV21 || DEV_IMAGE_FORMAT_YUV420NV12 == image->format) {
        char* pByteYTop = image->plane[0] + nStartY * image->stride[0] + nStartX;
        char* pByteVTop = image->plane[1] + (nStartY / 2) * (image->stride[1]) + nStartX;
        for (int i = 0; i < nPoints; i++) {
            if (0 == iType) {
                Dev_memset(pByteYTop, 255, nPoints);
                Dev_memset(pByteVTop, 0, nPoints);
            } else {
                Dev_memset(pByteYTop, 81, nPoints);
                Dev_memset(pByteVTop, 238, nPoints);
            }
            pByteYTop += image->stride[0];
            if (i % 2 == 1)
                pByteVTop += (image->stride[1]);
        }
    } else {
        DEV_LOGE("format not support");
    }
    return RET_OK;
}

static DEVRECTEXT DevImage_intersection(const DEVRECTEXT& a, const DEVRECTEXT& d) {
#define MATHL_MAX(a,b) ((a) > (b) ? (a) : (b))
#define MATH_MIN(a,b) ((a) < (b) ? (a) : (b))
    U32 l = MATHL_MAX(a.left, d.left);
    U32 t = MATHL_MAX(a.top, d.top);
    U32 r = MATH_MIN(a.right, d.right);
    U32 b = MATH_MIN(a.bottom, d.bottom);
    if (l >= r || t >= b) {
        l = 0;
        t = 0;
        r = 0;
        b = 0;
    }
    DEVRECTEXT c = { l, t, r, b };
    return c;
}

static S64 DevImage_DrawRect(DEV_IMAGE_BUF *image, DEV_IMAGE_RECT rt, S32 nLineWidth, S32 iType) {
    DEV_IF_LOGE_RETURN_RET((image==NULL), RET_ERR_ARG, "py addr err");
    U32 l = rt.left;
    U32 t = rt.top;
    U32 r = rt.left + rt.width;
    U32 b = rt.top + rt.height;
    DEVRECTEXT RectDraw = { l, t, r, b };
    nLineWidth = (nLineWidth + 1) / 2 * 2;
    S32 nPoints = nLineWidth <= 0 ? 1 : nLineWidth;
    DEVRECTEXT RectImage = { 0, 0, image->width, image->height };
    DEVRECTEXT rectIn = DevImage_intersection(RectDraw, RectImage);
    S32 nLeft = rectIn.left;
    S32 nTop = rectIn.top;
    S32 nRight = rectIn.right;
    S32 nBottom = rectIn.bottom;
    //U8* pByteY = image->plane[0];
    int nRectW = nRight - nLeft;
    int nRecrH = nBottom - nTop;
    if (nTop + nPoints + nRecrH > (int)image->height) {
        nTop = image->height - nPoints - nRecrH;
    }
    if (nTop < nPoints / 2) {
        nTop = nPoints / 2;
    }
    if (nBottom - nPoints < 0) {
        nBottom = nPoints;
    }
    if (nBottom + nPoints / 2 > (int)image->height) {
        nBottom = image->height - nPoints / 2;
    }
    if (image->format == DEV_IMAGE_FORMAT_YUYV) {
        nLeft = (nLeft + 1) / 2 * 2;
        nRight = (nRight + 1) / 2 * 2;
    }
    if (nLeft + nPoints + nRectW > (int)image->width) {
        nLeft = image->width - nPoints - nRectW;
    }
    if (nLeft < nPoints / 2) {
        nLeft = nPoints / 2;
    }

    if (nRight - nPoints < 0) {
        nRight = nPoints;
    }
    if (nRight + nPoints / 2 > (int)image->width) {
        nRight = image->width - nPoints / 2;
    }
    nRectW = nRight - nLeft;
    nRecrH = nBottom - nTop;
    if (image->format == DEV_IMAGE_FORMAT_YUV420NV21 || DEV_IMAGE_FORMAT_YUV420NV12 == image->format) {
        //draw the top line
        char* pByteYTop = image->plane[0] + (nTop - nPoints / 2) * image->stride[0] + nLeft - nPoints / 2;
        char* pByteVTop = image->plane[1] + ((nTop - nPoints / 2) / 2) * image->stride[1] + nLeft - nPoints / 2;
        char* pByteYBottom = image->plane[0] + (nTop + nRecrH - nPoints / 2) * image->stride[0] + nLeft - nPoints / 2;
        char* pByteVBottom = image->plane[1] + ((nTop + nRecrH - nPoints / 2) / 2) * image->stride[1] + nLeft - nPoints / 2;
        for (int i = 0; i < nPoints; i++) {
            pByteVTop = image->plane[1] + ((nTop + i - nPoints / 2) / 2) * image->stride[1] + nLeft - nPoints / 2;
            pByteVBottom = image->plane[1] + ((nTop + i + nRecrH - nPoints / 2) / 2) * image->stride[1] + nLeft - nPoints / 2;
            Dev_memset(pByteYTop, 255, nRectW + nPoints);
            Dev_memset(pByteYBottom, 255, nRectW + nPoints);
            pByteYTop += image->stride[0];
            pByteYBottom += image->stride[0];
            Dev_memset(pByteVTop, 60 * iType, nRectW + nPoints);
            Dev_memset(pByteVBottom, 60 * iType, nRectW + nPoints);
        }
        for (int i = 0; i < nRecrH; i++) {
            char* pByteYLeft = image->plane[0] + (nTop + i) * image->stride[0] + nLeft - nPoints / 2;
            char* pByteYRight = image->plane[0] + (nTop + i) * image->stride[0] + nLeft + nRectW - nPoints / 2;
            char* pByteVLeft = image->plane[1] + (((nTop + i) / 2) * image->stride[1]) + nLeft - nPoints / 2;
            char* pByteVRight = image->plane[1] + (((nTop + i) / 2) * image->stride[1]) + nLeft + nRectW - nPoints / 2;
            Dev_memset(pByteYLeft, 255, nPoints);
            Dev_memset(pByteYRight, 255, nPoints);
            Dev_memset(pByteVLeft, 60 * iType, nPoints);
            Dev_memset(pByteVRight, 60 * iType, nPoints);
        }
    } else {
        DEV_LOGE("format not support");
    }
    return RET_OK;
}

static S64 DevImage_Flip(DEV_IMAGE_BUF *image, U32 orient) {
    DEV_IF_LOGE_RETURN_RET((image==NULL), RET_ERR_ARG, "py addr err");
    DEV_IMAGE_BUF tempImage = { 0 };
    DevImage_Clone(&tempImage, image, MARK("tempImage"));
    switch (image->format) {
    case DEV_IMAGE_FORMAT_YUV420NV12:
    case DEV_IMAGE_FORMAT_YUV420NV21: {
        if ((0 == orient) || (180 == orient)) {
            int idx = 0;
            for (int i = 0; i < image->height * image->stride[0]; i += image->stride[0]) {
                for (int j = 0; j < image->width; j++) {
                    tempImage.plane[0][idx++] = *(image->plane[0] + i + image->width - 1 - j);
                }
                idx += (image->stride[0] - image->width);
            }

            // UV
            idx = 0;
            for (int i = 0; i < image->height * image->stride[0] / 2; i += image->stride[0]) {
                for (int j = 0; j < image->width; j += 2) {
                    tempImage.plane[1][idx++] = *(image->plane[1] + i + image->width - 2 - j);
                    tempImage.plane[1][idx++] = *(image->plane[1] + i + image->width - 1 - j);
                }
                idx += (image->stride[0] - image->width);
            }
        } else {
            int idx = 0;
            for (int i = 0; i < image->height; i++) {
                for (int j = 0; j < image->stride[0]; j += 2) {
                    tempImage.plane[0][idx++] = *(image->plane[0] + image->stride[0] * (image->height - 1 - i) + j);
                    tempImage.plane[0][idx++] = *(image->plane[0] + image->stride[0] * (image->height - 1 - i) + j + 1);
                }
            }

            // UV
            idx = 0;
            for (int i = 0; i < image->height / 2; i++) {
                for (int j = 0; j < image->stride[0]; j += 2) {
                    tempImage.plane[1][idx++] = *(image->plane[1] + image->stride[0] * (image->height / 2 - 1 - i) + j);
                    tempImage.plane[1][idx++] = *(image->plane[1] + image->stride[0] * (image->height / 2 - 1 - i) + j + 1);
                }
            }
        }

    }
        break;
    case DEV_IMAGE_FORMAT_Y8:
    case DEV_IMAGE_FORMAT_YUV422NV16:
    case DEV_IMAGE_FORMAT_YUV420P010:
    case DEV_IMAGE_FORMAT_Y16:
    case DEV_IMAGE_FORMAT_RGBA888:
    default:
        DEV_LOGE("format not support");
        return RET_ERR;
        break;
    }
    DevImage_Copy(image, &tempImage);
    DevImage_Free(&tempImage);
    return RET_OK;
}

char* DevImage_FormatString(U32 format) {
    DEV_IMAGE_FORMAT i_format = (DEV_IMAGE_FORMAT) format;
    const char *pString = "UNKNOWN";
    switch (i_format) {
    case DEV_IMAGE_FORMAT_Y8:
        pString = "Y8";
        break;
    case DEV_IMAGE_FORMAT_Y16:
        pString = "Y16";
        break;
    case DEV_IMAGE_FORMAT_YUV420NV12:
        pString = "YUV420NV12";
        break;
    case DEV_IMAGE_FORMAT_YUV420NV21:
        pString = "YUV420NV21";
        break;
    case DEV_IMAGE_FORMAT_YUV422NV16:
        pString = "YUV422NV16";
        break;
    case DEV_IMAGE_FORMAT_YUYV:
        pString = "YUYV";
        break;
    case DEV_IMAGE_FORMAT_RAWPLAIN10:
        pString = "RAWPLAIN10";
        break;
    case DEV_IMAGE_FORMAT_RAWPLAIN16:
        pString = "RAWPLAIN16";
        break;
    case DEV_IMAGE_FORMAT_YUV420P010:
        pString = "YUV420P010";
        break;
    case DEV_IMAGE_FORMAT_RGBA888:
        pString = "RGBA888";
        break;
    case DEV_IMAGE_FORMAT_END:
    default:
        pString = "UNKNOWN";
        break;
    }
    return (char*) pString;
}

S64 DevImage_Report(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_LOGI("[-------------------------------------IMAGE REPORT SATRT--------------------------------------]");
    Device->mutex->Lock(m_mutex_Id);
    for (int i = 0; i < DEV_IMAGE_NUM_MAX; i++) {
        if ((m_image_list[i].image != NULL) && (m_image_list[i].en == TRUE)) {
            DEV_LOGI("[---IMAGE [%d][%s][%s][%d]IMAGE[%p]WxH[%dx%d]FORMAT[%s]OFFSET[%d]P1[%p][%dx%d]P2[%p][%dx%d]P3[%p][%dx%d]---]"
                    , i
                    , m_image_list[i].__tagName
                    , m_image_list[i].__fileName
                    , m_image_list[i].__fileLine
                    , m_image_list[i].image
                    , m_image_list[i].image->width
                    , m_image_list[i].image->height
                    , DevImage_FormatString(m_image_list[i].image->format)
                    , m_image_list[i].image->offset
                    , m_image_list[i].image->plane[0]
                    , m_image_list[i].image->stride[0]
                    , m_image_list[i].image->sliceHeight[0]
                    , m_image_list[i].image->plane[1]
                    , m_image_list[i].image->stride[1]
                    , m_image_list[i].image->sliceHeight[1]
                    , m_image_list[i].image->plane[2]
                    , m_image_list[i].image->stride[2]
                    , m_image_list[i].image->sliceHeight[2]
                    );
        }
    }
    Device->mutex->Unlock(m_mutex_Id);
    DEV_LOGI("[-------------------------------------IMAGE REPORT END  --------------------------------------]");
    return RET_OK;
}

S64 DevImage_Init(void) {
    if (init_f == TRUE) {
        return RET_OK;
    }
    Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
    memset((void*) &m_image_list, 0, sizeof(m_image_list));
    init_f = TRUE;
    return RET_OK;
}

S64 DevImage_Deinit(void) {
    if (init_f != TRUE) {
        return RET_OK;
    }
    S32 report = 0;
    for (int i = 0; i < DEV_IMAGE_NUM_MAX; i++) {
        if ((m_image_list[i].image != NULL) && (m_image_list[i].en == TRUE)) {
            report++;
        }
    }
    if (report > 0) {
        DevImage_Report();
    }
    for (S64 id = 1; id < DEV_IMAGE_NUM_MAX; id++) {
        if ((m_image_list[id].image != NULL) && (m_image_list[id].en == TRUE)) {
            DEV_IMAGE_BUF *image = m_image_list[id].image;
            DevImage_Free(image);
        }
    }
    init_f = FALSE;
    memset((void*) &m_image_list, 0, sizeof(m_image_list));
    Device->mutex->Destroy(&m_mutex_Id);
    return RET_OK;
}

const Dev_Image m_dev_image = {
        .Init           = DevImage_Init,
        .Deinit         = DevImage_Deinit,
        .MatchToImage   = DevImage_MatchToImage,
        .MatchToVoid    = DevImage_MatchToVoid,
        .Copy           = DevImage_Copy,
        .Alloc          = DevImage_Alloc,
        .Clone          = DevImage_Clone,
        .Free           = DevImage_Free,
        .DumpImage      = DevImage_DumpImage,
        .DumpData       = DevImage_DumpData,
        .ReSize         = DevImage_ReSize,
        .Merge          = DevImage_Merge,
        .Convert        = DevImage_Convert,
        .DrawPoint      = DevImage_DrawPoint,
        .DrawRect       = DevImage_DrawRect,
        .Flip           = DevImage_Flip,
        .Report         = DevImage_Report,
        .FormatString   = DevImage_FormatString,
};

