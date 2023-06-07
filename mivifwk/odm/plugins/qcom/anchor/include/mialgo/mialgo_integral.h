
/************************************************************************************

Filename  : mialgo_integral.h
Content   :
Created   : Jun. 06, 2019
Author    : shenhongyi (shenhongyi@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_INTEGRAL_H__
#define MIALGO_INTEGRAL_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"

/**
* @brief MialgoIntegralMethod supported method of integral
*/
typedef enum MialgoIntegralMethod
{
    MIALGO_INTEGRAL_INVALID = 0,

    MIALGO_INTEGRAL_SUM,
    // ...
} MialgoIntegralMethod;

/**
* @brief Mialgo Integral function.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to get integral. (only for u8 gray image, total pixal no larger than 16843009).
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of Integral.(only for u32 gray image).
* @param[in] method         The method of Integral. (refer to MialgoIntegralMethod).
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoIntegral(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoIntegralMethod method);

/**
* @brief Mialgo Integral function.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to get integral. (only for u8 gray image, total pixal no larger than 16843009).
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of Integral.(only for u32 gray image).
* @param[in] method         The method of Integral. (refer to MialgoIntegralMethod).
* @param[in] impl           algo running platform selection, @see MialgoImpl.

* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoIntegralImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoIntegralMethod method, MialgoImpl impl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_INTEGRAL_H__
