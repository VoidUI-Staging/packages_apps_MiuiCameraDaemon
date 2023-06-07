#ifndef MIALGO_THRESHOLD_H__
#define MIALGO_THRESHOLD_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_mat.h"

/**
* @brief MialgoThresholdMethod supported method of threshold
*/
typedef enum MialgoThresholdMethod
{
    MIALGO_THRESH_BINARY     = 0,
    MIALGO_THRESH_BINARY_INV = 1,
    MIALGO_THRESH_TRUNC      = 2,
    MIALGO_THRESH_TOZERO     = 3,
    MIALGO_THRESH_TOZERO_INV = 4,
    MIALGO_THRESH_MASK       = 7,
    MIALGO_THRESH_OTSU       = 8,
    MIALGO_THRESH_TRIANGLE   = 16,
} MialgoThresholdMethod;

/**
* @brief the function interface of default implementation of threshold.
* Supported MialgoMat data types are @ref MIALGO_MAT_U8 and MIALGO_MAT_U16.
* Output image must have the same size and number of channels an input image.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] thresh         threshold value.
* @param[in] maxval         maximum value to use with the MIALGO_THRESH_BINARY and MIALGO_THRESH_BINARY_INV thresholding types.
* @param[in] method         threshold method, @see MialgoThresholdMethod
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoThreshold(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 thresh, 
                       MI_S32 maxval, MI_S32 method);

/**
* @brief the implementation function interface of threshold.
* Supported MialgoMat data types are @ref MIALGO_MAT_U8 and MIALGO_MAT_U16 for custom method and only MIALGO_MAT_U8 for adaptive method.
* Output image must have the same size and number of channels an input image.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] thresh         threshold value.
* @param[in] maxval         maximum value to use with the MIALGO_THRESH_BINARY and MIALGO_THRESH_BINARY_INV thresholding types.
* @param[in] method         threshold method, @see MialgoThresholdMethod
* @param[in] impl           implementation method, @see MialgoImpl
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoThresholdImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 thresh, 
                           MI_S32 maxval, MI_S32 method, MialgoImpl impl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_THRESHOLD_H__

