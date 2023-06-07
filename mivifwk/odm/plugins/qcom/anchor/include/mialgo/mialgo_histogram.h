/************************************************************************************

* Filename : mialgo_histogram.h
* Content : 
* Created : 8, 24, 2020
* Authors : Jie Zhou (zhoujie6@xiaomi.com)
* Copyright : Copyright 2020 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_HISTOGRAM_H__
#define MIALGO_HISTOGRAM_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"

/**
* @brief Mialgo histogram Impl parameters for mutilple threads
*/
typedef struct MialgoHistogramImplParam
{
    MI_S32 enable_threads; // flag for using mutilple threads
    MI_S32 threads_num;
} MialgoHistogramImplParam;

/**
* @brief Mialgo calculate histogram 1D function, calculate on one channel.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to histogram calculation.
* @param[out] hist          MI_U32 pointer, stored result of histogram calculation.
* @param[in] hist_size      MI_S32 histogram size
* @param[in] ranges         MI_F32 counted pixel value range, counted pixel val: low <= val < high counts
* @param[in] channel        MI_S32 image channel to calculate histogram
* @param[in] accumulate     MI_S32 set MIALGO_TRUE when needing accumulate
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoCalHist1D(MIALGO_ARRAY *src, MI_U32 *hist, MI_S32 hist_size, MI_S32 *ranges, MI_S32 channel,
                        MI_S32 accumulate);

/**
* @brief Mialgo calculate histogram 1D function, calculate on one channel.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to histogram calculation.
* @param[out] hist          MI_U32 pointer, stored result of histogram calculation.
* @param[in] hist_size      MI_S32 histogram size
* @param[in] ranges         MI_F32 counted pixel value range, counted pixel val: low <= val < high counts
* @param[in] channel        MI_S32 image channel to calculate histogram
* @param[in] accumulate     MI_S32 set MIALGO_TRUE when needing accumulate
* @param[in] impl           algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoCalHist1DImpl(MIALGO_ARRAY *src, MI_U32 *hist, MI_S32 hist_size, MI_S32 *ranges, MI_S32 channel,
                        MI_S32 accumulate, MialgoImpl impl);

// ***********************************************************************************************************

/**
* @brief Mialgo histogram equalization function, only surport gray image.
* @param[in] src            MIALGO_ARRAY array pointer, stored gray image data to histogram equalization.
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of histogram equalization.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoEqualizeHist(MIALGO_ARRAY *src, MIALGO_ARRAY *dst);

/**
* @brief Mialgo gray histogram equalization function, only surport gray image.
* @param[in] src            MIALGO_ARRAY array pointer, stored gray image data to histogram equalization.
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of histogram equalization.
* @param[in] impl           algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoEqualizeHistImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoImpl impl);

#ifdef __cplusplus
}
#endif

#endif // MIALGO_HISTOGRAM_H__