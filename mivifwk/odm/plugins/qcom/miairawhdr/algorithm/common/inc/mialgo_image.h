#ifndef MIALGO_IMAGE_H__
#define MIALGO_IMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief max number of channels in image.
 */
#define MIALGO_IMAGE_MAX_PLANE (4)

/**
 * @brief uncached memory flag.
 */
#define MIALGO_IMAGE_MEM_UNCACHED (0)

/**
 * @brief cached memory flag.
 */
#define MIALGO_IMAGE_MEM_CACHED (1)

/**
 * @brief Memory type.
 *
 * Memory usage description
 *   MIALGO_IMAGE_MEM_HEAP:
 *       Windows sysytem
 *       Ubuntu sysytem
 *       Android sytem : A small piece of memory on the CPU
 *
 *   MIALGO_IMAGE_MEM_ION: only used for Android sytem
 *       A large block memory for storing images
 *       memory used for HVX and GPU
 */
typedef enum {
    MIALGO_IMAGE_MEM_INVALID = 0, /*!< invalid mem type */
    MIALGO_IMAGE_MEM_HEAP,        /*!< heap mem */
    MIALGO_IMAGE_MEM_ION,         /*!< ion mem */
} MialgoImageMem;

/**
 * @brief image format.
 */
typedef enum {
    MIALGO_IMAGE_FMT_INVALID = 0,

    /*********** GRAY **********/
    MIALGO_IMAGE_FMT_GRAY = 1000, /*!< gray image format */

    /*********** YUV ***********/

    // yuv 420
    MIALGO_IMAGE_FMT_NV12 = 2000, /*!< nv12 image format */
    MIALGO_IMAGE_FMT_NV21,        /*!< nv21 image format */

    /*********** RGB ***********/

    /*********** RAW ***********/
} MialgoImageFmt;

/**
 * @brief represents image data
 */
typedef struct
{
    MialgoImageFmt fmt;                /*!< image format, @see MialgoImageFmt */
    int width;                         /*!< image width in pixels */
    int height;                        /*!< image hight in pixels */
    int pitch[MIALGO_IMAGE_MAX_PLANE]; /*!< stored four elements which are the size of aligned image
                                          row in bytes */
    unsigned char *plane[MIALGO_IMAGE_MAX_PLANE]; /*!< stored image data header address */

    int fd;             /*!< ion memory fd */
    int size;           /*!< memory total size */
    MialgoImageMem mem; /*!< memory type, @see MialgoImageMem */
} MialgoImage;

/**
 * @brief Mialgo handle to the algorithm instance.
 */
typedef void *MialgoHandle;

/**
 * @brief Inline function of assignment of Gray MialgoImage quickly.
 *
 * @return
 *         --<em>MialgoImage</em> , @see MialgoImage.
 */
static inline MialgoImage MialgoInitGray(int width, int height, int stride, unsigned char *buffer,
                                         int fd, MialgoImageMem mem)
{
    MialgoImage image;

    memset(&image, 0, sizeof(image));
    image.fmt = MIALGO_IMAGE_FMT_GRAY;
    image.width = width;
    image.height = height;
    image.pitch[0] = stride;
    image.plane[0] = buffer;

    image.fd = fd;
    image.size = stride * image.height;
    image.mem = mem;

    return image;
}

/**
 * @brief Inline function of assignment of NV12 MialgoImage quickly.
 *
 * @return
 *         --<em>MialgoImage</em> , @see MialgoImage.
 */
static inline MialgoImage MialgoInitNV12(int width, int height, int stride, unsigned char *buffer,
                                         int fd, MialgoImageMem mem)
{
    MialgoImage image;

    memset(&image, 0, sizeof(image));
    image.fmt = MIALGO_IMAGE_FMT_NV12;
    image.width = width;
    image.height = height;
    image.pitch[0] = image.pitch[1] = stride;
    image.plane[0] = buffer;
    image.plane[1] = image.plane[0] + (stride * image.height);

    image.fd = fd;
    image.size = stride * image.height * 3 / 2;
    image.mem = mem;

    return image;
}

/**
 * @brief Inline function of assignment of NV21 MialgoImage quickly.
 *
 * @return
 *         --<em>MialgoImage</em> , @see MialgoImage.
 */
static inline MialgoImage MialgoInitNV21(int width, int height, int stride, unsigned char *buffer,
                                         int fd, MialgoImageMem mem)
{
    MialgoImage image;

    memset(&image, 0, sizeof(image));
    image.fmt = MIALGO_IMAGE_FMT_NV21;
    image.width = width;
    image.height = height;
    image.pitch[0] = image.pitch[1] = stride;
    image.plane[0] = buffer;
    image.plane[1] = image.plane[0] + (stride * image.height);

    image.fd = fd;
    image.size = stride * image.height * 3 / 2;
    image.mem = mem;

    return image;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_IMAGE_H__
