/**
 * @file     morpho_image_data.h
 * @brief    Structure definition of the image data
 * @version  1.0.0
 * @date     2008-06-09
 *
 * Copyright (C) 2006 Morpho, Inc.
 */

#ifndef MORPHO_IMAGE_DATA_H
#define MORPHO_IMAGE_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

/** @struct morpho_ImageYuvPlanar
 *  @brief  Struct of Yuv Planar format.
 */
typedef struct
{
    void *y; /**< Head pointer of the Y image */
    void *u; /**< Head pointer of the U image */
    void *v; /**< Head pointer of the V image */
} morpho_ImageYuvPlanar;

/** @struct morpho_ImageYuvSemiPlanar
 *  @brief  Struct of Yuv SemiPlanar format.
 */
typedef struct
{
    void *y;  /**< Head pointer of the Y image */
    void *uv; /**< Head pointer of the UV image */
} morpho_ImageYuvSemiPlanar;

/** @struct morpho_ImageData
 *  @brief  Image Data.
 */
typedef struct
{
    int width;  /**< Width */
    int height; /**< Height */
    union {
        void *p;                               /**< Head pointer of the image */
        morpho_ImageYuvPlanar planar;          /**< Planar struct */
        morpho_ImageYuvSemiPlanar semi_planar; /**< Semiplanar struct */
    } dat;                                     /**< image data pointer */
} morpho_ImageData;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef MORPHO_IMAGE_DATA_H */
