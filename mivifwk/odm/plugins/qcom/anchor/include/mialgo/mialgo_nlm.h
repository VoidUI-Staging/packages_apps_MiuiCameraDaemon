/************************************************************************************

Filename  : mialgo_nlm_denoise.h
Content   :
Created   : Nov. 4, 2020
Author    : jiaoweibin (jiaoweibin@xiaomi.com)
Copyright : Copyright 2020 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/
#ifndef MIALGO_NLMDENOISE_H__
#define MIALGO_NLMDENOISE_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_mat.h"

/**
* @brief Non-local-means algorithm interface function.
* @param[in] src           Input array, only suppose u8c1
* @param[in] dst           Output array of the same size as src, only suppose u8c1
* @param[in] lut           filter weight, only suppose u8c1, width must be 256, height must be 1
* @param[in] search_size   search size, only suppose 7x7
* @param[in] patch_size    neighborhood patch size, only suppose 5x5
* @param[in] type          pixel extrapolation method, @see MialgoBorderType, only suppose MIALGO_BORDER_REPLICATE
* @param[in] border_value  border value in case of constant border type
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoNlmDeNoise(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MIALGO_ARRAY *lut, MI_S32 search_size, 
                        MI_S32 patch_size, MialgoBorderType type, MialgoScalar border_value);


/**
* @brief Non-local-means algorithm interface function.
* @param[in] src           Input array, only suppose u8c1
* @param[in] dst           Output array of the same size as src, only suppose u8c1
* @param[in] lut           filter weight, only suppose u8c1, width must be 256, height must be 1
* @param[in] search_size   search size, only suppose 7x7
* @param[in] patch_size    neighborhood patch size, only suppose 5x5
* @param[in] type          pixel extrapolation method, @see MialgoBorderType, only suppose MIALGO_BORDER_REPLICATE
* @param[in] border_value  border value in case of constant border type
* @param[in] impl          algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoNlmDeNoiseImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MIALGO_ARRAY *lut, MI_S32 search_size,
                            MI_S32 patch_size, MialgoBorderType type, MialgoScalar border_value, MialgoImpl impl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_NORM_H__
