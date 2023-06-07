/**
 * @file     morpho_image_data_ex.h
 * @brief    Structure definition for image data
 * @version  1.0.0
 * @date     2010-03-30
 *
 * Copyright (C) 2010 Morpho, Inc.
 */

#ifndef MORPHO_IMAGE_DATA_EX_H
#define MORPHO_IMAGE_DATA_EX_H

#include "morpho_image_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @struct morpho_ImageYuvPlanarPitch
 *  @brief  Pitch value of Yuv Planar format.
 */
typedef struct
{
    int y; /**< Y data. Number of bytes from head of line to the head of next line */
    int u; /**< U data. Number of bytes from head of line to the head of next line */
    int v; /**< V data. Number of bytes from head of line to the head of next line */
} morpho_ImageYuvPlanarPitch;

/** @struct morpho_ImageYuvSemiPlanarPitch
 *  @brief  Pitch value of Yuv SemiPlanar format.
 */
typedef struct
{
    int y;  /**< Y data. Number of bytes from head of line to the head of next line */
    int uv; /**< UV data. Number of bytes from head of line to the head of next line */
} morpho_ImageYuvSemiPlanarPitch;

/** @struct morpho_ImageDataEx
 *  @brief  Image Data.
 */
typedef struct
{
    int width;  /**< Width */
    int height; /**< Height */
    union {
        void *p;                               /**< Head pointer of the image data */
        morpho_ImageYuvPlanar planar;          /**< Planar struct */
        morpho_ImageYuvSemiPlanar semi_planar; /**< Semiplanar struct */
    } dat;                                     /**< image data pointer */
    union {
        int p; /**< Number of bytes from head of line to the head of next line */
        morpho_ImageYuvPlanarPitch planar;          /**< Planar pitch struct */
        morpho_ImageYuvSemiPlanarPitch semi_planar; /**< Semiplanar pitch struct */
    } pitch;                                        /**< image pitch union */
} morpho_ImageDataEx;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef MORPHO_IMAGE_DATA_EX_H */
