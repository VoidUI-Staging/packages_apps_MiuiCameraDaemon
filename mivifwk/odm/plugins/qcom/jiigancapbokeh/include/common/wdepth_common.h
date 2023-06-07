/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
// NOLINT
#ifndef DEPTHSERVER_INCLUDE_COMMON_WDEPTH_COMMON_H_
#define DEPTHSERVER_INCLUDE_COMMON_WDEPTH_COMMON_H_

#include "common/wcommon.h"
#include "utils/base_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Algo initialization data structure
 */
typedef struct {
  WDataBlob mecp;             //< Config data blob
  WDataBlob calib;            //< Calib data blob
  WModelInfo model;           //< Model data blob
} WDepthConfig;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DEPTHSERVER_INCLUDE_COMMON_WDEPTH_COMMON_H_
