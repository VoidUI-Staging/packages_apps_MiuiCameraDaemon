/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
// clang-format off
#ifndef DOFSERVER_INCLUDE_COMMON_WDOF_COMMON_H_
#define DOFSERVER_INCLUDE_COMMON_WDOF_COMMON_H_

#include "common/wcommon.h"
#include "utils/base_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief REFOCUS spot value type.
 *
 * - WRF_SPOT_SHARPCIRCLE: the shape of the spot is sharp circle.
 * - WRF_SPOT_DONUTCIRCLE: the shape of the spot is donut circle.
 * - WRF_SPOT_CONCENTRICCIRCLE: he shape of the spot is concentric circle.
 * - WRF_SPOT_HEXAGON: the shape of the spot is hexagon.
 * - WRF_SPOT_HEART: the shape of the spot is heart.
 * - WRF_SPOT_STAR: the shape of the spot is star.
 * - WRF_SPOT_DIAMOND: the shape of the spot is diamond.
 */
typedef enum _WBokehSpotType {
  WRF_SPOT_SHARPCIRCLE = 1,   ///< Default spot type (sharp circle)
  WRF_SPOT_DONUTCIRCLE,       ///< The shape of the spot is donut circle.
  WRF_SPOT_CONCENTRICCIRCLE,  ///< The shape of the spot is concentric circle.
  WRF_SPOT_HEXAGON,           ///< The shape of the spot is hexagon.
  WRF_SPOT_HEART,             ///< The shape of the spot is heart.
  WRF_SPOT_STAR,              ///< The shape of the spot is star.
  WRF_SPOT_DIAMOND            ///< The shape of the spot is diamond.
} WBokehSpotType;

/**
 * @brief Initialization data structure
 */
typedef struct {
  WDataBlob mecp;             //< Config data blob
  WDataBlob calib;            //< Calib data blob
  WModelInfo model;           //< Model data blob
} WDofConfig;

/**
 * @brief Runtime parameters
 */
typedef struct {
  WFloat32  blur_level;       //< The intensity of blur, range [1f, 16f]
  WPoint2i  focus;            //< The focus point to decide
                              //< which region should be clear
  WFaceInfo faces;            //< Human face info
  WOrientation device_angle;  //< Device rotation
  WBokehSpotType spot_type;   //< Bokeh spot type
} WDofParam;

/**
 * @brief Depth in/out struct
 */
typedef struct {
  WGDepthInfo ginfo;          //< GDepth info if enabled
  WDepthInfo info;            //< Depth basic info
  WInt32 total;               //< Depth buffer size
  void* data;                 //< Depth buffer
} WDepth;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DOFSERVER_INCLUDE_COMMON_WDOF_COMMON_H_
// clang-format on
