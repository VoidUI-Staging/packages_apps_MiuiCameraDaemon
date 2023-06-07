/************************************************************************************

Filename  : mialgo_cl.h
Content   :
Created   : Feb. 04, 2020
Author    : guwei (guwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_UTILS_OPENCL_H__
#define MIALGO_UTILS_OPENCL_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_basic.h"
#include "mialgo_type.h"

/**
* @brief Mialgo load opencl bin string.
*
* @param[in] data               opencl bin string
* @param[in] len                opencl bin string len
*
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoLoadClString(MI_U8 *data, MI_S32 len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_UTILS_OPENCL_H__

