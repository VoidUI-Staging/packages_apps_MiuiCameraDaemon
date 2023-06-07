/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
#ifndef REFOCUSSERVER_INCLUDE_COMMON_WREFOCUS_COMMON_H_
#define REFOCUSSERVER_INCLUDE_COMMON_WREFOCUS_COMMON_H_

#include "common/wbokeh_common.h"
#include "common/wcommon.h"
#include "utils/base_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Algo initialization data structure
 */
typedef struct {
  WDataBlob mecpBlob;  //< param data blob
} WRefocusConfig;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // REFOCUSSERVER_INCLUDE_COMMON_WREFOCUS_COMMON_H_
