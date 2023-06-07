/************************************************************************************

Filename  : mialgo_norm.h
Content   :
Created   : Oct. 10, 2019
Author    : wuchenchen (wuchenchen@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/
#ifndef MIALGO_NORM_H__
#define MIALGO_NORM_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"

/**
* @brief Normalized method selection.
*/
typedef enum MialgoNormType
{
    MIALGO_NORM_TYPE_INVALID = 0,
    MIALGO_NORM_MINMAX,             /*!< MinMax method */
    MIALGO_NORM_L1,                 /*!< L1     method */
    MIALGO_NORM_L2,                 /*!< L2     method */
    MIALGO_NORM_INF,                /*!< Inf    method */
    MIALGO_NORM_TYPE_NUM,           /*!< Number of normalized methods */
} MialgoNormType;


/**
* @brief Normalized algorithm interface function.
* @param[in] src          Input array.
* @param[in] dst          Output array of the same size as src.
* @param[in] alpha        Norm value to normalize to or the lower range boundary in case of the range normalization.
* @param[in] beta         Upper range boundary in case of the range normalization; it is not used for the norm L1, L2 and Inf.
* @param[in] norm_type    Normalization type @see MialgoNormType.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoNorm(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_F32 alpha, MI_F32 beta, MI_S32 norm_type);


/**
* @brief Normalized algorithm interface function.
* @param[in] src          Input array.
* @param[in] dst          Output array of the same size as src.
* @param[in] alpha        Norm value to normalize to or the lower range boundary in case of the range normalization.
* @param[in] beta         Upper range boundary in case of the range normalization; it is not used for the norm L1, L2 and Inf.
* @param[in] norm_type    Normalization type @see MialgoNormType.
* @param[in] impl         algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoNormImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_F32 alpha, MI_F32 beta, MI_S32 norm_type,
                                MialgoImpl impl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_NORM_H__
