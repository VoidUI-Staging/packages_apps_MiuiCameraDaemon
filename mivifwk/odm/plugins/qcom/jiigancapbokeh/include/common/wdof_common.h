/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
#ifndef DOFSERVER_INCLUDE_COMMON_WDOF_COMMON_H_
#define DOFSERVER_INCLUDE_COMMON_WDOF_COMMON_H_

#include "common/wbokeh_common.h"
#include "common/wcommon.h"
#include "utils/base_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

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
  WFloat32 blur_level;        //< The intensity of blur, range [1f, 16f]
  WPoint2i focus;             //< The focus point to decide
                              //< which region should be clear
  WFaceInfo faces;            //< Human face info
  WOrientation device_angle;  //< Device rotation
  WBokehSpotType spot_type;   //< Bokeh spot type
  WBokehStyle bokeh_style;    //< Bokeh style
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
