/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
#ifndef OPTICALZOOMFUSIONSERVER_INCLUDE_COMMON_V1_WOPZ_FUSION_COMMON_H_
#define OPTICALZOOMFUSIONSERVER_INCLUDE_COMMON_V1_WOPZ_FUSION_COMMON_H_

#include "common/wcommon.h"
#include "utils/base_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialization data structure
 */
typedef struct {
  WDataBlob mecpBlob;  // Config data blob
} WOpZFusionConfig;

typedef struct {
  WFloat32 zoomFactor;  // user zoom level
  WFloat32 mainScale;   // main camera scale level
  WFloat32 auxScale;    // aux camera scale level
} WOpZFusionScaleParams;

typedef struct {
  WFloat32 distance;  // af distance in mm
  WFloat32 luma;      // environment luma
  WBool isAfStable;   // auto focus stable
  WRect cropRect;     // crop rect on sensor size domain
} WOpZFusionCameraMetaData;

typedef struct {
  WOpZFusionScaleParams scale;    // scale params
  WOpZFusionCameraMetaData main;  // main camera metadata
  WOpZFusionCameraMetaData aux;   // aux camera metadata
} WOpZFusionParams;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPTICALZOOMFUSIONSERVER_INCLUDE_COMMON_V1_WOPZ_FUSION_COMMON_H_
