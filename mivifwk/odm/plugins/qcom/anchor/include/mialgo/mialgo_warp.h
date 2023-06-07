/************************************************************************************

Filename  : mialgo_warp.h
Content   :
Created   : Jul. 30, 2019
Author    : zhengbao (zhengbao@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/
#ifndef MIALGO_WARP_H__
#define MIALGO_WARP_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"

/**
* @brief interpolation methods
*/
typedef enum
{
    MIALGO_WARP_INTER_INVALID = 0,
    MIALGO_WARP_INTER_NEAREST,                  /*!< nearest neighbor interpolation */
    MIALGO_WARP_INTER_LINEAR,                   /*!< bilinear interpolation */

} MialgoWarpInterFlags;

/**
* @brief the implementation of affine transformation, supported element types: MI_U8.
* @param[in]  src           the pointer to the source image; the supported image: MialgoImg or MialgoMat, the number of channel: 1 ,3.
* @param[out] dst           the pointer to the dst image; the supported image: MialgoImg or MialgoMat, the number of channel: 1 ,3.
* @param[in]  matrix        the pointer to the homography matrix, the size of matrix should be 2x3.
* @param[in]  inter_method  interpolation methods. 
* @param[in]  border_type   border types. 
* @param[in]  border_value  border values. 
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoWarpAffine(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_F32 *matrix, MialgoWarpInterFlags inter_method,
                        MialgoBorderType border_type, MialgoScalar border_value);

/**
* @brief the implementation of affine transformation, supported element types: MI_U8.
* @param[in]  src           the pointer to the source image; the supported image: MialgoImg or MialgoMat, the number of channel: 1 ,3.
* @param[out] dst           the pointer to the dst image; the supported image: MialgoImg or MialgoMat, the number of channel: 1 ,3.
* @param[in]  matrix        the pointer to the homography matrix, the size of matrix should be 2x3.
* @param[in]  inter_method  interpolation methods. 
* @param[in]  border_type   border types. 
* @param[in]  border_value  border values. 
* @param[in] impl           the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoWarpAffineImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_F32 *matrix, MialgoWarpInterFlags inter_method,
                        MialgoBorderType border_type, MialgoScalar border_value, MialgoImpl impl);

/**
* @brief the implementation of perspective transformation, supported element types: MI_U8.
* @param[in]  src           the pointer to the source image; the supported image: MialgoImg or MialgoMat, the number of channel: 1 ,3.
* @param[out] dst           the pointer to the dst image; the supported image: MialgoImg or MialgoMat, the number of channel: 1 ,3.
* @param[in]  matrix        the pointer to the homography matrix, the size of matrix should be 2x3.
* @param[in]  inter_method  interpolation methods. 
* @param[in]  border_type   border types. 
* @param[in]  border_value  border values. 
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoWarpPerspective(MIALGO_ARRAY *src,  MIALGO_ARRAY *dst, MI_F32 *matrix, MialgoWarpInterFlags inter_method,
                             MialgoBorderType border_type, MialgoScalar border_value);

/**
* @brief the implementation of perspective transformation, supported element types: MI_U8.
* @param[in]  src           the pointer to the source image; the supported image: MialgoImg or MialgoMat, the number of channel: 1 ,3.
* @param[out] dst           the pointer to the dst image; the supported image: MialgoImg or MialgoMat, the number of channel: 1 ,3.
* @param[in]  matrix        the pointer to the homography matrix, the size of matrix should be 2x3.
* @param[in]  inter_method  interpolation methods. 
* @param[in]  border_type   border types. 
* @param[in]  border_value  border values.
* @param[in] impl           the function running platform configuration, @see MialgoImpl. 
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoWarpPerspectiveImpl(MIALGO_ARRAY *src,  MIALGO_ARRAY *dst, MI_F32 *matrix, MialgoWarpInterFlags inter_method,
                             MialgoBorderType border_type, MialgoScalar border_value, MialgoImpl impl);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_WARP_H__
