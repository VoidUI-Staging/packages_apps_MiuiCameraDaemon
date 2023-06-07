#ifndef MI_TYPE_H
#define MI_TYPE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace AlgoMultiCams
{

/****************************** MiImage Definition ***********************************/
/**
* @brief fov information.
*/
enum ALGO_CAPTURE_FOV
{
    CAPTURE_FOV_2X = 0,
    CAPTURE_FOV_1X = 1,
    CAPTURE_FOV_1_4X = 2,
    CAPTURE_FOV_3_75X = 3,
    CAPTURE_FOV_3_26X = 4,
    CAPTURE_FOV_3X = 5,
};

/**
* @brief image frame information.
*/
enum ALGO_CAPTURE_FRAME
{
    CAPTURE_FRAME_4_3 = 0,
    CAPTURE_FRAME_16_9 = 1,
    CAPTURE_FRAME_FULL_FRAME = 2
};

/**
* @brief sensor combination information.
*/
enum ALGO_CAPTURE_SENSOR_COMBINATION
{
    ALGO_T_W = 0,
    ALGO_W_UW = 1,
};

/**
* @brief sensor mode information.
*/
enum ALGO_CAPTURE_SENSOR_MODE
{
    ALGO_BINING_MODE = 0,
    ALGO_REMOSAIC_MODE = 1,
};

/**
* @brief max number of channels in image.
*/
enum
{
    MI_IMAGE_MAX_PLANE = 4
};

/**
* @brief image format.
* ONLY 8 bit is allowed for software integration.
*/
typedef enum
{
    MI_IMAGE_FMT_INVALID = 0,

    /*********** GRAY **********/
    MI_IMAGE_FMT_GRAY = 1000, /*!< gray 8bit image format */

    /*********** YUV ***********/
    MI_IMAGE_FMT_NV12 = 2000, /*!< nv12 8bit image format YYYY UV */
    MI_IMAGE_FMT_NV21,        /*!< nv21 8bit image format YYYY VU */
    /*********** RGB ***********/
    MI_IMAGE_FMT_RGB = 3000, /*!< rgb 8bit 3 channel image format */
    MI_IMAGE_FMT_PLANE_RGB
    /********** Others **********/
} MiImageFmt;

/**
* @brief represents image data
*/
typedef struct
{
    MiImageFmt fmt; /*!< image format, @see MiImageFmt */
    int width;      /*!< image width in pixels */
    int height;     /*!< image hight in pixels */
    // Plane num. Gray, RGB has 1 plane, NV12/21 has two planes, PlaneRGB has 3 planes.
    unsigned int plane_count;
    // stored four elements which are the size of aligned image row in bytes
    unsigned int pitch[MI_IMAGE_MAX_PLANE];
    unsigned int slice_height[MI_IMAGE_MAX_PLANE]; // Unused!
    // stored plane size in bytes
    unsigned int plane_size[MI_IMAGE_MAX_PLANE];
    unsigned char *plane[MI_IMAGE_MAX_PLANE]; /*!< stored image data header address */
    int fd[MI_IMAGE_MAX_PLANE];               /*!< ion memory fd */
} MiImage;

/*************************** Point Definition ****************************************/

/**
 * @brief represents int Point2D data
 *
 */
typedef struct
{
    int x;
    int y;
} MiPoint2i;

/**
 * @brief represents float Point2D data
 *
 */
typedef struct
{
    float x;
    float y;
} MiPoint2f;

/**
 * @brief represents int Point3D data
 *
 */
typedef struct
{
    int x;
    int y;
    int z;
} MiPoint3i;

/**
 * @brief represents float Point3D data
 *
 */
typedef struct
{
    float x;
    float y;
    float z;
} MiPoint3f;

/************************** MiSize Definition **********************************/

/**
 * @brief represents Size2D
 *
 */
typedef struct
{
    int width;
    int height;
} MiSize2D;

/**
 * @brief represents Size3D
 *
 */
typedef struct
{
    int width;
    int height;
    int depth;
} MiSize3D;

/************************* MiRect Definition *************************************/

/**
 * @brief Data struct of rect2d
 *
 */
typedef struct
{
    int x;
    int y;
    int width;
    int height;
} MiRect;

/**
 * @brief Data struct of faceRect
 *
 */
typedef struct
{
    int x;
    int y;
    int width;
    int height;
    int angle;
} MiFaceRect;

} // namespace AlgoMultiCams

#endif // MI_TYPE_H
