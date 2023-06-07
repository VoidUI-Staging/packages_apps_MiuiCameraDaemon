/************************************************************************************

Filename  : mialgo_resize.h
Content   :
Created   : Apr. 24， 2019
Author    : Xinpen Du (duxinpeng@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_RESIZE_H__
#define MIALGO_RESIZE_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"


/**
* @brief interpolation method for image resize.
*/
typedef enum MialgoResizeMethod
{
    MIALGO_RESIZE_INVALID = 0,
    MIALGO_RESIZE_NN,       /*!< nearest neighbor interpolation */
    MIALGO_RESIZE_BILINEAR, /*!< bilinear interpolation */
    MIALGO_RESIZE_CUBIC,    /*!< bicubic interpolation */
    MIALGO_RESIZE_AREA,     /*!< resampling using pixel area relation. It may be a preferred method for image decimation, as it gives moire’-free results. But when the image is zoomed, it is similar to the INTER_NEAREST method. */
    MIALGO_RESIZE_NUM,
} MialgoResizeMethod;


/**
* @brief Resizes an image.
* The function resize resizes the image src down to or up to the specified size. Note that the Initial dst type or size should be taken into account, the size and type are derived from the `src`.
* Supported MialgoMat data types are NN(U8C1/U8C3/U16C1/S16C1) BILINEAR(U8C1/U8C3/U16C1/S16C1/F32C1/F32C4) CUBIC(U8C1/F32C1) AREA(U8C1/U16C1/S16C1).
* MialgoMat data types supported by DSP are BILINEAR(U8C1) AREA(U8C1),and scaling ratio supports 4x/2x/0.5x/0.25x.
* @param[in] src        input MialgoMat/MialgoImg.
* @param[in] dst        output MialgoMat/MialgoImg, it must has the dsize and the type of dst is the same as of src.
* @param[in] method     interpolation method, @see MialgoResizeMethod.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoResize(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                        MialgoResizeMethod method);

/**
* @brief Resizes an image.
* The function resize resizes the image src down to or up to the specified size. Note that the Initial dst type or size should be taken into account, the size and type are derived from the `src`.
* Supported MialgoMat data types are NN(U8C1/U8C3/U16C1/S16C1) BILINEAR(U8C1/U8C3/U16C1/S16C1/F32C1/F32C4) CUBIC(U8C1/F32C1) AREA(U8C1/U16C1/S16C1).
* @param[in] src        input MialgoMat/MialgoImg.
* @param[in] dst        output MialgoMat/MialgoImg, it must has the dsize and the type of dst is the same as of src.
* @param[in] method     interpolation method, @see MialgoResizeMethod.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoResizeImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                        MialgoResizeMethod method, MialgoImpl impl);

/**
* @brief Resizes an image, special for yuv format image.
* The function resize resizes the image src down to or up to the specified size. Note that the Initial dst type or size should be taken into account, the size and type are derived from the `src`.
* Supported MialgoMat data types are NN(NV12/NV21 U8) BILINEAR(NV12/NV21/YUV444 U8).
* @param[in] src        input MialgoMat/MialgoImg.
* @param[in] dst        output MialgoMat/MialgoImg, it must has the dsize and the type of dst is the same as of src.
* @param[in] method     interpolation method, currently only supports NN and linear. see @MialgoResizeMethod.
* @param[in] format     image format, currently only supports @MIALGO_IMG_NV12 and @MIALGO_IMG_NV21 . @see MialgoImgFormat.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoResizeYUV(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                        MialgoResizeMethod method, MialgoImgFormat format);

/**
* @brief Resizes an image, special for yuv format image.
* The function resize resizes the image src down to or up to the specified size. Note that the Initial dst type or size should be taken into account, the size and type are derived from the `src`.
* Supported MialgoMat data types are NN(NV12/NV21 U8) BILINEAR(NV12/NV21/YUV444 U8).
* @param[in] src        input MialgoMat/MialgoImg.
* @param[in] dst        output MialgoMat/MialgoImg, it must has the dsize and the type of dst is the same as of src.
* @param[in] method     interpolation method, currently only supports NN and linear. see @MialgoResizeMethod.
* @param[in] format     image format, currently only supports @MIALGO_IMG_NV12 and @MIALGO_IMG_NV21 . @see MialgoImgFormat.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoResizeYUVImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                            MialgoResizeMethod method,
                            MialgoImgFormat format,
                            MialgoImpl impl);

/**
* @brief Resizes an image ROI, special for yuv format image.
* The function resize resizes the image src down to or up to the specified size. Note that the Initial dst type or size should be taken into account, the size and type are derived from the `src`.
* Supported MialgoMat data types are NN(NV12/NV21 U8) BILINEAR(NV12/NV21 U8).
* @param[in] src        input MialgoMat/MialgoImg.
* @param[in] dst        output MialgoMat/MialgoImg, it must has the dsize and the type of dst is the same as of src.
* @param[in] method     interpolation method, currently only supports NN and linear. see @MialgoResizeMethod.
* @param[in] format     image format, currently only supports @MIALGO_IMG_NV12 and @MIALGO_IMG_NV21 . @see MialgoImgFormat.
* @param[in] rect       the image ROI that needs to be resized, @see MialgoRect.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoResizeYUVRoi(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                                MialgoResizeMethod method,
                                MialgoImgFormat format,
                                MialgoRect rect);

/**
* @brief Resizes an image ROI, special for yuv format image.
* The function resize resizes the image src down to or up to the specified size. Note that the Initial dst type or size should be taken into account, the size and type are derived from the `src`.
* Supported MialgoMat data types are NN(NV12/NV21 U8) BILINEAR(NV12/NV21 U8).
* @param[in] src        input MialgoMat/MialgoImg.
* @param[in] dst        output MialgoMat/MialgoImg, it must has the dsize and the type of dst is the same as of src.
* @param[in] method     interpolation method, currently only supports NN and linear. see @MialgoResizeMethod.
* @param[in] format     image format, currently only supports @MIALGO_IMG_NV12 and @MIALGO_IMG_NV21 . @see MialgoImgFormat.
* @param[in] rect       the image ROI that needs to be resized, @see MialgoRect.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoResizeYUVRoiImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                                MialgoResizeMethod method,
                                MialgoImgFormat format,
                                MialgoRect rect,
                                MialgoImpl impl);

/**
* @brief Resizes an image and normalized into F32 from 0.0 to 1.0.
* The function resize resizes the image src down to or up to the specified size. Note that the Initial dst type or size should be taken into account, the size and type are derived from the `src`.
* Supported MialgoMat data types are NN(U8C3).
* @param[in] src        input MialgoMat/MialgoImg.
* @param[in] dst        output MialgoMat/MialgoImg, it must has the dsize and the type of dst is the same as of src.
* @param[in] method     interpolation method, @see MialgoResizeMethod. Only support MIALGO_RESIZE_NN.
* @param[in] norm       the max value of pixel value(ex. 255 for U8)
*
* @return
*       -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoResizeNorm(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                            MialgoResizeMethod method, MI_F32 norm);

/**
* @brief Resizes an image and normalized into F32 from 0.0 to 1.0
* The function resize resizes the image src down to or up to the specified size. Note that the Initial dst type or size should be taken into account, the size and type are derived from the `src`.
* Supported MialgoMat data types are NN(U8C3).
* @param[in] src        input MialgoMat/MialgoImg.
* @param[in] dst        output MialgoMat/MialgoImg, it must has the dsize and the type of dst is the same as of src.
* @param[in] method     interpolation method, @see MialgoResizeMethod. Only support MIALGO_RESIZE_NN.
* @param[in] norm       the max value of pixel value(ex. 255 for U8)
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*       -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoResizeNormImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                            MialgoResizeMethod method, MI_F32 norm,
                            MialgoImpl impl);


/**
* @brief interpolation method for image resize yuv norm.
*/
typedef enum MialgoResizeYuvNormMethod
{
    MIALGO_RESIZE_NN_NV12,       /*!< nearest neighbor interpolation NV12 */
    MIALGO_RESIZE_NN_NV21,       /*!< nearest neighbor interpolation NV21*/
} MialgoResizeYuvNormMethod;

/**
* @brief Resizes Yuv an image and normalized into F32 from 0.0 to 1.0.
* The function resize resizes the image src down to or up to the specified size. Note that the Initial dst type or size should be taken into account, the size and type are derived from the `src`.
* Supported MialgoMat data types are NN(NV12/NV21 U8).
* @param[in] src        input MialgoMat/MialgoImg.
* @param[in] dst        output MialgoMat/MialgoImg, it must has the dsize and the type of dst is the same as of src.
* @param[in] method     interpolation method, @see MialgoResizeYuvNormMethod. Only support MIALGO_RESIZE_NN.
* @param[in] norm       the max value of pixel value(ex. 255 for U8)
*
* @return
*       -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoResizeYuvNorm(MIALGO_ARRAY *srcY, MIALGO_ARRAY *srcUV, MIALGO_ARRAY *dst,
                            MialgoResizeYuvNormMethod method, MI_F32 norm);

/**
* @brief Resizes Yuv an image and normalized into F32 from 0.0 to 1.0
* The function resize resizes the image src down to or up to the specified size. Note that the Initial dst type or size should be taken into account, the size and type are derived from the `src`.
* Supported MialgoMat data types are NN(NV12/NV21 U8).
* @param[in] src        input MialgoMat/MialgoImg.
* @param[in] dst        output MialgoMat/MialgoImg, it must has the dsize and the type of dst is the same as of src.
* @param[in] method     interpolation method, @see MialgoResizeYuvNormMethod. Only support MIALGO_RESIZE_NN.
* @param[in] norm       the max value of pixel value(ex. 255 for U8)
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*       -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoResizeYuvNormImpl(MIALGO_ARRAY *srcY, MIALGO_ARRAY *srcUV, MIALGO_ARRAY *dst,
                                MialgoResizeYuvNormMethod method, MI_F32 norm,
                                MialgoImpl impl);

/**
* @brief Impl parameters
*/
typedef struct MialgoResizeImplParam
{
    MI_S32 threads_num;  // task number
} MialgoResizeImplParam;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_RESIZE_H__

