
/************************************************************************************

Filename  : mialgo_morph.h
Content   :
Created   : Oct. 14, 2019
Author    : ltt (lutiantian@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_MORPH_H__
#define MIALGO_MORPH_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"

typedef enum MialgoKerShape
{
    MIALGO_RECT = 0,
    MIALGO_ELLIPSE,
    MIALGO_CROSS,

} MialgoKerShape;

typedef struct
{
    MI_S32 x;
    MI_S32 y;
} MialgoAnchor;

typedef enum MialgoMorphMethod
{
    MIALGO_DILATE = 0,
    MIALGO_ERODE,

} MialgoMorphMethod;

/**
* @brief the erode of images.
* @param[in] src               the input image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] dst               dst output image of the same size and type as src
* @param[in] ker_size          ksize Size of the structuring element.
* @param[in] ker_shape         shape Element shape that could be one of #MialgoKerShape
* @param[in] anchor            anchor position of the anchor within the element; default value (-1, -1) means that the
                               anchor is at the element center.
* @param[in] iteration         iterations number of times erosion is applied.
* @param[in] border_type       borderType pixel extrapolation method, see #MialgoBorderType
* @param[in] border_value      borderValue border value in case of a constant border
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/

MI_S32 MialgoErode(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size, 
                                MialgoKerShape ker_shape, MialgoAnchor anchor, MI_S32 iteration, MialgoBorderType border_type,
                                MialgoScalar border_value);

/**
* @brief the dilate of images.
* @param[in] src               the input image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] dst               dst output image of the same size and type as src
* @param[in] ker_size          ksize Size of the structuring element.
* @param[in] ker_shape         shape Element shape that could be one of #MialgoKerShape
* @param[in] anchor            anchor position of the anchor within the element; default value (-1, -1) means that the
                               anchor is at the element center.
* @param[in] iteration         iterations number of times erosion is applied.
* @param[in] border_type       borderType pixel extrapolation method, see #MialgoBorderType
* @param[in] border_value      borderValue border value in case of a constant border
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/

MI_S32 MialgoDilate(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size, 
                                MialgoKerShape ker_shape, MialgoAnchor anchor, MI_S32 iteration, MialgoBorderType border_type,
                                MialgoScalar border_value);

/**
* @brief the dilate of images.
* @param[in] src               the input image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] dst               dst output image of the same size and type as src
* @param[in] ker_size          ksize Size of the structuring element.
* @param[in] ker_shape         shape Element shape that could be one of #MialgoKerShape(MialgoKerShape must be MIALGO_RECT in dsp version).
* @param[in] anchor            anchor position of the anchor within the element; default value (-1, -1) means that the
                               anchor is at the element center.
* @param[in] iteration         iterations number of times erosion is applied(iterations number of times must be 1 in dsp version).
* @param[in] border_type       borderType pixel extrapolation method, see #MialgoBorderType
* @param[in] border_value      borderValue border value in case of a constant border
* @param[in] impl              algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/

MI_S32 MialgoMorphImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoMorphMethod morph_method, MI_S32 ker_size, 
                                MialgoKerShape ker_shape, MialgoAnchor anchor, MI_S32 iteration, MialgoBorderType border_type,
                                MialgoScalar border_value, MialgoImpl impl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_MORPH_H__
