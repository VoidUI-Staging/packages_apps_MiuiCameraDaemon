/************************************************************************************

Filename  : mialgo_cvtcolor.h
Content   :
Created   : May. 13, 2019
Author    : guwei (guwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_CVTCOLOR_H__
#define MIALGO_CVTCOLOR_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"

/**
* @brief MialgoCvtcolorMethod supported method of cvtcolor
*/
typedef enum MialgoCvtcolorMethod
{
    MIALGO_CVTCOLOR_INVALID = 0,

    MIALGO_CVTCOLOR_BGR2YUV = 100,
    MIALGO_CVTCOLOR_BGR2GRAY,
    MIALGO_CVTCOLOR_BGR2NV12,
    MIALGO_CVTCOLOR_BGR2NV21,
    MIALGO_CVTCOLOR_BGR2I420,
    MIALGO_CVTCOLOR_RGB2NV12,
    MIALGO_CVTCOLOR_RGB2NV21,
    MIALGO_CVTCOLOR_YUV4442NV12,
    MIALGO_CVTCOLOR_YUV4442NV21,
    MIALGO_CVTCOLOR_NV122YUV444,
    MIALGO_CVTCOLOR_NV212YUV444,

    MIALGO_CVTCOLOR_YUV2BGR = 200,
    MIALGO_CVTCOLOR_GRAY2BGR,
    MIALGO_CVTCOLOR_NV122BGR,
    MIALGO_CVTCOLOR_NV212BGR,
    MIALGO_CVTCOLOR_I4202BGR,
    MIALGO_CVTCOLOR_NV122RGB,
    MIALGO_CVTCOLOR_NV212RGB,

    MIALGO_CVTCOLOR_SPECIAL = 300,
    MIALGO_CVTCOLOR_NV212BGRY,
    MIALGO_CVTCOLOR_NV122BGRY,
    MIALGO_CVTCOLOR_NV122BGRGRAY,
    MIALGO_CVTCOLOR_NV212BGRGRAY,

    MIALGO_CVTCOLOR_NV122RGB_601 = 400,
    MIALGO_CVTCOLOR_NV212RGB_601,
    MIALGO_CVTCOLOR_RGB2NV12_601,
    MIALGO_CVTCOLOR_RGB2NV21_601,

    MIALGO_CVTCOLOR_NV122BGR_601 = 500,
    MIALGO_CVTCOLOR_NV212BGR_601,
    MIALGO_CVTCOLOR_NV122BGRGRAY_601,
    MIALGO_CVTCOLOR_NV212BGRGRAY_601,

    MIALGO_CVTCOLOR_I4202P010 = 600,
    MIALGO_CVTCOLOR_P0102NV12,

    MIALGO_CVTCOLOR_UBWCTP102P010 = 700,

    MIALGO_CVTCOLOR_NUM,
} MialgoCvtcolorMethod;

/**
* @brief Mialgo Convert color space function.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to convert color. (only for u8 data type).
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of convert color image.(only for u8 data type).
* @param[in] method         The method of color convert. (refer to MialgoCvtcolorMethod).
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoCvtcolor(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                        MialgoCvtcolorMethod method);

/**
* @brief Mialgo Convert color space function.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to convert color. (only for u8 data type).
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of convert color image.(only for u8 data type).
* @param[in] method         The method of color convert. (refer to MialgoCvtcolorMethod).
* @param[in] impl           algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoCvtcolorImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                        MialgoCvtcolorMethod method, MialgoImpl impl);

/**
* @brief Mialgo Convert color space function(YUV -> RGB, YUV separate).
* @param[in] srcY       the source image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] srcUV      the source image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] method     The method of color convert. (refer to MialgoCvtcolorMethod, and only supported YUV to RGB/BGR/BGRGRAY).
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoCvtcolorYUVToRGB(MIALGO_ARRAY *srcY, MIALGO_ARRAY *srcUV, MIALGO_ARRAY *dst,
                                MialgoCvtcolorMethod method);

/**
* @brief Mialgo Convert color space function(YUV -> RGB, YUV separate).
* @param[in] srcY       the source image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] srcUV      the source image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] method     The method of color convert. (refer to MialgoCvtcolorMethod, and only supported YUV to RGB/BGR/BGRGRAY).
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoCvtcolorYUVToRGBImpl(MIALGO_ARRAY *srcY, MIALGO_ARRAY *srcUV, MIALGO_ARRAY *dst,
                                    MialgoCvtcolorMethod method, MialgoImpl impl);

/**
* @brief Mialgo Convert color space function(RGB -> YUV, YUV separate).
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstY       the result image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstUV      the result image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] method     The method of color convert. (refer to MialgoCvtcolorMethod, and only supported RGB to YUV).
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoCvtcolorRGBToYUV(MIALGO_ARRAY *src, MIALGO_ARRAY *dstY, MIALGO_ARRAY *dstUV,
                                MialgoCvtcolorMethod method);

/**
* @brief Mialgo Convert color space function(RGB -> YUV, YUV separate).
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstY       the result image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstUV      the result image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] method     The method of color convert. (refer to MialgoCvtcolorMethod, and only supported RGB to YUV).
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoCvtcolorRGBToYUVImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dstY, MIALGO_ARRAY *dstUV, 
                                    MialgoCvtcolorMethod method, MialgoImpl impl);



/**
* @brief Mialgo Convert color space function(YUV -> YUV, YUV separate).
* @param[in] srcY       the source image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] srcUV      the source image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstY       the result image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstUV      the result image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] method     The method of color convert. (refer to MialgoCvtcolorMethod, and only supported MIALGO_CVTCOLOR_I4202P010).
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoCvtcolorYUVToYUV(MIALGO_ARRAY *srcY,  MIALGO_ARRAY *srcUV, MIALGO_ARRAY *dstY, MIALGO_ARRAY *dstUV, 
                                MialgoCvtcolorMethod method);

/**
* @brief Mialgo Convert color space function(YUV -> YUV, YUV separate).
* @param[in] srcY       the source image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] srcUV      the source image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstY       the result image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstUV      the result image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] method     The method of color convert. (refer to MialgoCvtcolorMethod, and only supported MIALGO_CVTCOLOR_I4202P010).
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoCvtcolorYUVToYUVImpl(MIALGO_ARRAY *srcY,  MIALGO_ARRAY *srcUV, MIALGO_ARRAY *dstY, MIALGO_ARRAY *dstUV, 
                                    MialgoCvtcolorMethod method, MialgoImpl impl);



#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_CVTCOLOR_H__
