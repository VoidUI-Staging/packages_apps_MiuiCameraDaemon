/************************************************************************************

Filename  : mialgo_remap.h
Content   :
Created   : May. 25, 2020
Author    : lutiantian (lutiantian@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/
#ifndef MIALGO_REMAP_H__
#define MIALGO_REMAP_H__

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
    MIALGO_REMAP_INTER_INVALID = 0,
    MIALGO_REMAP_INTER_NEAREST,                  /*!< nearest neighbor interpolation */
    MIALGO_REMAP_INTER_LINEAR,                   /*!< bilinear interpolation */

} MialgoRemapInterFlags;

/**
* @brief Impl parameters
*/
typedef struct MialgoRemapImplParam
{
    MI_S32 task_num;  // task number
} MialgoRemapImplParam;

/**
* @brief the implementation of remap, supported element types: MI_U8.
* @param[in]  src           the pointer to the source image; the supported image: MialgoImg or MialgoMat, the number of channel: 1.
* @param[out] dst           the pointer to the dst image; the supported image: MialgoImg or MialgoMat, the number of channel: 1.
* @param[in]  map0          the first map of either (x,y) points; the supported image: MialgoImg or MialgoMat, the type of MI_S16(C1).
* @param[in]  map1          The second map of y values having the type MI_U16(C1), or NULL(Nearest).
* @param[in]  inter_method  interpolation methods. 
* @param[in]  border_type   border types. 
* @param[in]  border_value  border values. 
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoRemap(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MIALGO_ARRAY *map0, MIALGO_ARRAY *map1, 
                        MialgoRemapInterFlags inter_method, MialgoBorderType border_type, MialgoScalar border_value);

/**
* @brief the implementation of remap, supported element types: MI_U8.
* @param[in]  src           the pointer to the source image; the supported image: MialgoImg or MialgoMat, the number of channel: 1.
* @param[out] dst           the pointer to the dst image; the supported image: MialgoImg or MialgoMat, the number of channel: 1.
* @param[in]  map0          the first map of either (x,y) points; the supported image: MialgoImg or MialgoMat, the type of MI_S16(C1).
* @param[in]  map1          The second map of y values having the type MI_U16(C1), or NULL(Nearest).
* @param[in]  inter_method  interpolation methods. 
* @param[in]  border_type   border types. 
* @param[in]  border_value  border values. 
* @param[in] impl           the function running platform configuration, @see MialgoImpl. 
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoRemapImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MIALGO_ARRAY *map0, MIALGO_ARRAY *map1, 
                       MialgoRemapInterFlags inter_method, MialgoBorderType border_type, 
                       MialgoScalar border_value, MialgoImpl impl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_REMAP_H__
