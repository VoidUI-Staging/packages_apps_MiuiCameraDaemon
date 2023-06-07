/************************************************************************************

Filename  : mialgo_pyramid.h
Content   :
Created   : Aug. 27, 2019
Author    : Hongyi Shen (shenhongyi@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_PYRAMID_H__
#define MIALGO_PYRAMID_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"

/**
* @brief Mialgo Pyramid Up function.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to pyramid up. (only for u8_gray & u16_gray).
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of pyramid up image.(same type as src, width and height are half of src)
* @param[in] type           MialgoBorderType to describe the way to make copy border. (support all MialgoBorderType, refer to mialgo_mat.h )
* @param[in] kernal_size    The kernal size of Gaussian filter, only support 3 and 5(3 for u16 image and 5 for u8 image), opencl only support 5 for u8 image
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoPyramidUp(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoBorderType type, MialgoScalar border_value, MI_S32 kernal_size);

/**
* @brief Mialgo Pyramid Up function.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to pyramid up. (only for u8_gray & u16_gray).
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of pyramid up image.(same type as src, width and height are half of src)
* @param[in] type           MialgoBorderType to describe the way to make copy border. (support all MialgoBorderType, refer to mialgo_mat.h )
* @param[in] kernal_size    The kernal size of Gaussian filter, only support 3 and 5(3 for u16 image and 5 for u8 image), opencl only support 5 for u8 image
* @param[in] impl           algo running platform selection, @see MialgoImpl.

* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoPyramidUpImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoBorderType type, MialgoScalar border_value, MI_S32 kernal_size, MialgoImpl impl);

/**
* @brief Mialgo Pyramid Dwon function.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to pyramid down. (only for u8_gray & u16_gray).
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of pyramid down image.(same type as src, width and height are half of src)
* @param[in] type           MialgoBorderType to describe the way to make copy border. (ONLY support MIALGO_BORDER_REFLECT_101)
* @param[in] kernal_size    The kernal size of Gaussian filter, only support 3 and 5(3 for u16 image and 5 for u8 image), opencl only support 5 for u8 image
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoPyramidDown(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoBorderType type, MialgoScalar border_value, MI_S32 kernal_size);

/**
* @brief Mialgo Pyramid Dwon function.
* @param[in] src            MIALGO_ARRAY array pointer, stored image data to pyramid down. (only for u8_gray & u16_gray).
* @param[out] dst           MIALGO_ARRAY array pointer, stored result of pyramid down image.(same type as src, width and height are half of src)
* @param[in] type           MialgoBorderType to describe the way to make copy border. (ONLY support MIALGO_BORDER_REFLECT_101)
* @param[in] kernal_size    The kernal size of Gaussian filter, only support 3 and 5(3 for u16 image and 5 for u8 image), opencl only support 5 for u8 image
* @param[in] impl           algo running platform selection, @see MialgoImpl.

* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoPyramidDownImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoBorderType type, MialgoScalar border_value, MI_S32 kernal_size, MialgoImpl impl);

typedef enum
{
    MIALGO_PYR_MAT,
    MIALGO_PYR_IMG
} MialgoPyramidType;

/**
* @brief Structure representing a pyramid image
*
* Description of octaves
*   Number of octaves. Image size is halved between adjacent octaves.
*
* Description of layers
*   Number of layers per octave. This is included in octave
*
* Description of scale_for_octave
*   Scaling parameters for each octave.
*
* Description of scale_for_layer
*   Scaling parameters for each layer in octave.
*
* Description of pyri
*   Pointer to the image structure array.
*/
typedef struct MialgoPyramid
{
    MI_S32 octaves;
    MI_S32 layers;
    MI_F32 scale_for_octave;
    MI_F32 scale_for_layer;
    MI_S32 total_levels;    // octaves * layers

    MialgoPyramidType type;
    union upyr {
        MialgoImg **pyri;       // only support 'MIALGO_IMG_NUMERIC' format
        MialgoMat **pyrm;
    } pyr;

    /***********************
    ex:
    octaves = m;
    layers  = n;

    octave 0:
        layer 0      -> pyr[0]
        layer 1      -> pyr[1]
        ...
        layer n-1    -> pyr[n-1]

    octave 1:
        layer 0      -> pyr[n]
        layer 1      -> pyr[n+1]
        ...
        layer n-1    -> pyr[2n-1]
    ...

    octave m-1:
        layer 0      -> pyr[m*n]
        layer 1      -> pyr[m*n+1]
        ...
        layer n-1    -> pyr[m*n-1]
    ************************/
} MialgoPyramid;

/**
* @brief Mialgo Creates Pyramid.
* @param[in] size              MialgoSize3D type, Size information of image, including width, height and depth.
* @param[in] info              MialgoImgInfo type, Image information. only support 'MIALGO_IMG_NUMERIC' format
* @param[in] type              MI_S32 type, Mat element type. ex: MIALGO_MAT_U8
* @param[in] octaves           @see MialgoPyramid
* @param[in] layers            @see MialgoPyramid
* @param[in] scale_for_octave  @see MialgoPyramid
* @param[in] scale_for_layer   @see MialgoPyramid
*
* @return
*        --<em>MialgoPyramid *</em> MialgoPyramid* type value, the MialgoPyramid pointer created, or NULL identitying creation failed.
*
* Note: MialgoCreatePyramidImg using to create buffer stored in MialgoPyramid.pyr.pyri
*       MialgoCreatePyramidMat using to create buffer stored in MialgoPyramid.pyr.pyrm
*/
MialgoPyramid* MialgoCreatePyramidImg(MialgoSize3D size, MialgoImgInfo info,
                        MI_S32 octaves, MI_S32 layers,
                        MI_F32 scale_for_octave, MI_F32 scale_for_layer);

MialgoPyramid* MialgoCreatePyramidMat(MI_S32 *sizes, MI_S32 type,
                        MI_S32 octaves, MI_S32 layers,
                        MI_F32 scale_for_octave, MI_F32 scale_for_layer);

/**
* @brief Mialgo Deallocates the MialgoPyramid.
* @param[in] pp_pyr           MialgoPyramid type, pyramid to be deleted, double pointer to the matrix.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoDeletePyramid(MialgoPyramid **pp_pyr);

/**
* @brief Constructs the Gaussian pyramid for an image.
*
* The function constructs a vector of images and builds the Gaussian pyramid
*     by recursively applying pyrDown to the previously built pyramid layers,
*     starting from dst[0]==src.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] pyr            output MialgoPyramid. octaves should be 1, and scale_for_layer eq 2
* @param[in] ksize          filter kernel size, ex ker_size = 3 means 3x3 kernel, Supported 3/5/7.
* @param[in] sigma          Gaussian kernel standard deviation
* @param[in] type           pixel extrapolation method, @see MialgoBorderType
* @param[in] border_value   border value in case of constant border type
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoBuildGaussPyramid(MIALGO_ARRAY *src, MialgoPyramid *pyr,
                        MI_S32 ksize, MI_F32 sigma, MialgoBorderType type,
                        MialgoScalar border_value);
/**
* @brief Constructs the Gaussian pyramid for an image.
*
* The function constructs a vector of images and builds the Gaussian pyramid
*     by recursively applying pyrDown to the previously built pyramid layers,
*     starting from dst[0]==src.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] pyr            output MialgoPyramid. octaves should be 1, and scale_for_layer eq 2
* @param[in] ksize          filter kernel size, ex ker_size = 3 means 3x3 kernel, Supported 3/5/7.
* @param[in] sigma          Gaussian kernel standard deviation
* @param[in] type           pixel extrapolation method, @see MialgoBorderType
* @param[in] border_value   border value in case of constant border type
* @param[in] impl           algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoBuildGaussPyramidImpl(MIALGO_ARRAY *src, MialgoPyramid *pyr,
                        MI_S32 ksize, MI_F32 sigma, MialgoBorderType type,
                        MialgoScalar border_value, MialgoImpl impl);
/**
* @brief Constructs the Gaussian pyramid for an image.
*
* The function constructs a vector of images and builds the Gaussian pyramid
*     by recursively applying pyrDown to the previously built pyramid layers,
*     starting from dst[0]==src.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] pyr            output MialgoPyramid. octaves should be 1, and scale_for_layer eq 2
* @param[in] ksize          filter kernel size, ex ker_size = 3 means 3x3 kernel, Supported 3/5/7.
* @param[in] sigma          Gaussian kernel standard deviation
* @param[in] type           pixel extrapolation method, @see MialgoBorderType
* @param[in] border_value   border value in case of constant border type
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/

MI_S32 MialgoBuildLaplacianPyramid(MIALGO_ARRAY *src, MialgoPyramid *pyr,
                        MI_S32 ksize, MI_F32 sigma, MialgoBorderType type,
                        MialgoScalar border_value);
/**
* @brief Constructs the Gaussian pyramid for an image.
*
* The function constructs a vector of images and builds the Gaussian pyramid
*     by recursively applying pyrDown to the previously built pyramid layers,
*     starting from dst[0]==src.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] pyr            output MialgoPyramid. octaves should be 1, and scale_for_layer eq 2
* @param[in] ksize          filter kernel size, ex ker_size = 3 means 3x3 kernel, Supported 3/5/7.
* @param[in] sigma          Gaussian kernel standard deviation
* @param[in] type           pixel extrapolation method, @see MialgoBorderType
* @param[in] border_value   border value in case of constant border type
* @param[in] impl           algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoBuildLaplacianPyramidImpl(MIALGO_ARRAY *src, MialgoPyramid *pyr,
                        MI_S32 ksize, MI_F32 sigma, MialgoBorderType type,
                        MialgoScalar border_value, MialgoImpl impl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_PYRAMID_H__

