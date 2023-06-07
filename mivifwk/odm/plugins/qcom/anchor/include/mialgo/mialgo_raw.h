
/************************************************************************************

Filename  : mialgo_raw.h
Content   :
Created   : May. 27, 2019
Author    : shenhongyi (shenhongyi@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_RAW_H__
#define MIALGO_RAW_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"
/**
* @brief MialgoBayerPattern supported type of Raw
*/
typedef enum MialgoBayerPattern
{
    MIALGO_BAYER_RGGB,
    MIALGO_BAYER_GRBG,
    MIALGO_BAYER_BGGR,
    MIALGO_BAYER_GBRG
} MialgoBayerPattern;

/**
* @brief MialgoRawUnpackMethod supported method of rawunpack
*/
typedef enum MialgoRawUnpackMethod
{
    MIALGO_RAW_INVALID = 0,
    MIALGO_RAW_MIPI102UNPACK16,
    MIALGO_RAW_MIPI102UNPACK8,
} MialgoRawUnpackMethod;

/**
* @brief MialgoRawpackMethod supported method of rawpack
*/
typedef enum MialgoRawpackMethod
{
    MIALGO_RAWPACK_INVALID = 0,
    MIALGO_RAW_PACK16TOMIPI10,
} MialgoRawpackMethod;

/**
* @brief Mialgo Unpack MipiRaw function.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to unpack. (only for single channel mipi raw, width should be multiple of 5).
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of unpaked.(only for u8 and u16 gray).
* @param[in] method         The method of unpack raw. (refer to MialgoRawUnpackMethod).
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoRawUnpack(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoRawUnpackMethod method);

/**
* @brief Mialgo Unpack MipiRaw function.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to unpack. (only for single channel mipi raw, width should be multiple of 5).
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of unpaked.(only for u8 and u16 gray).
* @param[in] method         The method of unpack raw. (refer to MialgoRawUnpackMethod).
* @param[in] impl           algo running platform selection, @see MialgoImpl.

* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoRawUnpackImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoRawUnpackMethod method, MialgoImpl impl);

/**
* @brief Mialgo pack Mipi10 function.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to pack. (only for single channel raw(u16 gray), width should be multiple of 4).
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of unpaked.(only for u8 and u16 gray, width should be multiple of 5).
* @param[in] method         The method of unpack raw. (refer to MialgoRawUnpackMethod).
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoRawpack(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoRawpackMethod method);
/**
* @brief Mialgo pack mipi10 function.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to pack. (only for single channel raw(u16 gray), width should be multiple of 4).
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of mipi10 raw.(only for mipi10, width should be multiple of 5).
* @param[in] method         The method of unpack raw. (refer to MialgoRawpackMethod).
* @param[in] impl           algo running platform selection, @see MialgoImpl.

* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoRawpackImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoRawpackMethod method, MialgoImpl impl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_MIPIRAW_H__
