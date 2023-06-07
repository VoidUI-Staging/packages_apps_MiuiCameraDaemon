/************************************************************************************

Filename  : mialgo_basic_multicam.h
Content   :
Created   : Oct. 14, 2019
Author    : zhengbao (zhengbao@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_BASIC_MULTICAM_H__
#define MIALGO_BASIC_MULTICAM_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"

/**
* @brief the implementation of the fusion of two images.
* @param[in] sharp_img      the source image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] blur_img       the blurred image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] alpha          the gamma-transformed image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] dst_img        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoVideoBokehImgFusion(MIALGO_ARRAY *sharp_img,  MIALGO_ARRAY *blur_img,
                        MIALGO_ARRAY *alpha, MIALGO_ARRAY *dst_img);

/**
* @brief the implementation of the fusion of two images.
* @param[in] sharp_img      the source image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] blur_img       the blurred image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] alpha          the gamma-transformed image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] dst_img        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] impl           the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoVideoBokehImgFusionImpl(MIALGO_ARRAY *sharp_img,  MIALGO_ARRAY *blur_img,
                        MIALGO_ARRAY *alpha, MIALGO_ARRAY *dst_img, MialgoImpl impl);

/**
* @brief the implementation of gamma transformation or correction.
* @param[in] src    the source image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] exp    the exponent parameter.
* @param[in] dst    the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoVideoBokehGammaTrans(MIALGO_ARRAY *src, MI_F32 exp, MIALGO_ARRAY *dst);

/**
* @brief the implementation of gamma transformation or correction.
* @param[in] src    the source image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] exp    the exponent parameter.
* @param[in] dst    the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] impl   the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoVideoBokehGammaTransImpl(MIALGO_ARRAY *src, MI_F32 exp, MIALGO_ARRAY *dst, MialgoImpl impl);

/**
* @brief the implementation of division and image fusion.
* @param[in] d_blury_backgr    the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] bin_mask_blur     the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] d_y               the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] d_alpha           the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] dst_img           the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoVideoBokehDivideAndImgFusion(MIALGO_ARRAY *d_blury_backgr, MIALGO_ARRAY *bin_mask_blur,
                        MIALGO_ARRAY *d_y, MIALGO_ARRAY *d_alpha, MIALGO_ARRAY *dst_img);

/**
* @brief the implementation of division and image fusion.
* @param[in] d_blury_backgr    the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] bin_mask_blur     the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] d_y               the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] d_alpha           the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] dst_img           the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] impl              the function running platform configuration, @see MialgoImpl.
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoVideoBokehDivideAndImgFusionImpl(MIALGO_ARRAY *d_blury_backgr, MIALGO_ARRAY *bin_mask_blur,
                        MIALGO_ARRAY *d_y, MIALGO_ARRAY *d_alpha, MIALGO_ARRAY *dst_img, MialgoImpl impl);

/**
* @brief the implementation of multiplation.
* @param[in] d_alpha           the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] d_y               the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] bin_mask          the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] d_y_backgr        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoVideoBokehMaskBinar(MIALGO_ARRAY *d_alpha, MIALGO_ARRAY *d_y,
                        MIALGO_ARRAY *bin_mask, MIALGO_ARRAY *d_y_backgr);

/**
* @brief the implementation of multiplation.
* @param[in] d_alpha           the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] d_y               the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] bin_mask          the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] d_y_backgr        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MI_U8.
* @param[in] impl              the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoVideoBokehMaskBinarImpl(MIALGO_ARRAY *d_alpha, MIALGO_ARRAY *d_y,
                        MIALGO_ARRAY *bin_mask, MIALGO_ARRAY *d_y_backgr, MialgoImpl impl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_BASIC_MULTICAM_H__
