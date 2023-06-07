#include <MiBuf.h>

static double nowMSec()
{
    struct timeval res;
    gettimeofday(&res, NULL);
    return 1000.0 * res.tv_sec + (double)res.tv_usec / 1e3;
}

template <typename T>
int miAllocBuffer(T buffer, int stride, int slice, int format)
{
    if (format == CAM_FORMAT_YUV_420_NV21 || format == CAM_FORMAT_YUV_420_NV12) {
        buffer->pAddr[0] = (unsigned char *)malloc(stride * slice * 3 / 2);
        if (NULL == buffer->pAddr[0])
            return false;
        buffer->pAddr[1] = buffer->pAddr[0] + stride * slice;
        MLOGI(Mia2LogGroupCore, "Alloc Buffer Addr1 %p, Addr2 %p.", buffer->pAddr[0],
              buffer->pAddr[1]);
    }
    return true;
}

bool miReadWritePlane(FILE *fp, unsigned char *plane, int width, int height, int pitch, bool read)
{
    if (fp && plane) {
        for (int i = 0; i < height; i++) {
            if (read)
                fread(plane, width, 1, fp);
            else
                fwrite(plane, width, 1, fp);
            plane += pitch;
        }
        return true;
    }
    return false;
}

ImageParams *initImageParams(const char *file, bool read)
{
    ImageParams *ret = (ImageParams *)malloc(sizeof(ImageParams));
    if (NULL == ret) {
        return NULL;
    }

    ret->format.width = gTestWidth;
    ret->format.height = gTestHeight;
    ret->format.format = gTestFormat;
    ret->planeStride = gTestStride;
    ret->sliceheight = gTestSlice;
    ret->pMetadata = NULL; // If need, change
    ret->pPhyCamMetadata = NULL;
    ret->numberOfPlanes = 2;
    ret->fd[0] = -1;
    ret->pNativeHandle = NULL;

    if (!miAllocBuffer(ret, gTestStride, gTestSlice, CAM_FORMAT_YUV_420_NV12)) {
        releaseImageParams(ret);
        return NULL;
    }
    if (read) {
        FILE *p;
        p = fopen(file, "rb");
        if (p == NULL) {
            releaseImageParams(ret);
            return NULL;
        }
        if ((!miReadWritePlane(p, ret->pAddr[0], gTestWidth, gTestHeight, gTestStride, 1)) ||
            (!miReadWritePlane(p, ret->pAddr[1], gTestWidth, gTestHeight / 2, gTestStride, 1))) {
            releaseImageParams(ret);
            return NULL;
        }
        fclose(p);
    }
    return ret;
}

MiaBatchedImage *initMiaBatchedImage(const char *file, bool read)
{
    MiaBatchedImage *image = (MiaBatchedImage *)malloc(sizeof(MiaBatchedImage));
    if (NULL == image) {
        return NULL;
    }

    image->format.width = gTestWidth;
    image->format.height = gTestHeight;
    image->format.format = gTestFormat;
    image->format.planeStride = gTestStride;
    image->format.sliceheight = gTestSlice;
    image->metaWraper = NULL; // If need, change
    image->timeStampUs = nowMSec();
    image->pBuffer.bufferSize = gTestStride * gTestSlice * 3 / 2;
    image->pBuffer.planes = 2;
    image->pBuffer.fd[0] = -1; // can't use for jpegencode and reprocess, If need, change
    image->pBuffer.pExtHandle = NULL;
    image->pBuffer.phHandle = NULL; // can't use for jpegencode and reprocess, If need, change

    if (!miAllocBuffer(&image->pBuffer, gTestStride, gTestSlice, CAM_FORMAT_YUV_420_NV12)) {
        releaseMiaBatchedImage(image);
        return NULL;
    }

    if (read) {
        FILE *p;
        p = fopen(file, "rb");
        if (p == NULL) {
            releaseMiaBatchedImage(image);
            return NULL;
        }

        bool rt =
            miReadWritePlane(p, image->pBuffer.pAddr[0], gTestWidth, gTestHeight, gTestStride, 1);
        rt = miReadWritePlane(p, image->pBuffer.pAddr[1], gTestWidth, gTestHeight / 2, gTestStride,
                              1);
        if (!rt) {
            releaseMiaBatchedImage(image);
            return NULL;
        }
        fclose(p);
    }
    return image;
}

void releaseImageParams(ImageParams *ret)
{
    if (!ret) {
        if (ret->pAddr[0]) {
            free(ret->pAddr[0]);
            ret->pAddr[0] = NULL;
        }
        free(ret);
        ret = NULL;
    }
}

void releaseMiaBatchedImage(MiaBatchedImage *image)
{
    if (!image) {
        if (image->pBuffer.pAddr[0]) {
            free(image->pBuffer.pAddr[0]);
            image->pBuffer.pAddr[0] = NULL;
        }
        free(image);
        image = NULL;
    }
}

int writeNV12(ImageParams buf, const char *file)
{
    FILE *p;
    p = fopen(file, "wb");
    if (p == NULL)
        return false;
    miReadWritePlane(p, buf.pAddr[0], buf.format.width, buf.format.height, buf.planeStride, 0);
    miReadWritePlane(p, buf.pAddr[1], buf.format.width, buf.format.height / 2, buf.planeStride, 0);
    fclose(p);
    printf("to %s \n", file);
    return true;
}
