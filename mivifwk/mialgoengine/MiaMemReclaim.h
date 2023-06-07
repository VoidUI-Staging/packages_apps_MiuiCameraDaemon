/*
 * Copyright (c) 2020-2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MI_MEMORY_RECLAIM__
#define __MI_MEMORY_RECLAIM__

#include "MiaPluginUtils.h"
#include "MiaPostProcType.h"

namespace mialgo2 {

typedef struct
{
    uint32_t defaultReclaimCnt;
    uint32_t bufferThresholdValue;
    int reltime;
} MiaMemoryReclaimParams;

typedef struct
{
    uint32_t operationMode;
    uint32_t cameraMode;
    MiaMemoryReclaimParams memoryReclaimParams;
} MiaMemReclaimParamMappingTable;

static const MiaMemReclaimParamMappingTable g_MiaMemReclaimParamMappingTable[] = {
    {
        StreamConfigModeBokeh,
        CAM_COMBMODE_FRONT_BOKEH,
        {3, 3, 6000},
    },
    {
        StreamConfigModeBokeh,
        CAM_COMBMODE_FRONT,
        {3, 3, 6000},
    },
    {
        StreamConfigModeBokeh,
        CAM_COMBMODE_REAR_BOKEH_WT,
        {4, 3, 6000},
    },
    {
        StreamConfigModeMiuiSuperNight,
        CAM_COMBMODE_FRONT,
        {3, 2, 6000},
    },
    {
        StreamConfigModeUltraPixelPhotography,
        CAM_COMBMODE_REAR_WIDE,
        {4, 2, 10000},
    },
    {
        StreamConfigModeMiuiManual,
        CAM_COMBMODE_REAR_WIDE,
        {1, 1, 1000},
    },
    {
        StreamConfigModeMiuiManual,
        CAM_COMBMODE_REAR_TELE,
        {3, 3, 6000},
    },
    {
        StreamConfigModeMiuiManual,
        CAM_COMBMODE_REAR_ULTRA,
        {3, 3, 6000},
    },
    {
        StreamConfigModeMiuiManual,
        CAM_COMBMODE_REAR_MACRO,
        {3, 3, 6000},
    },
    {
        StreamConfigModeSAT,
        CAM_COMBMODE_REAR_SAT_WT,
        {3, 3, 6000},
    },
    {
        StreamConfigModeMiuiZslFront,
        CAM_COMBMODE_FRONT,
        {4, 2, 6000},
    },
    {
        StreamConfigModeThirdPartyNormal,
        CAM_COMBMODE_REAR_WIDE,
        {3, 2, 8000},
    },
    {
        StreamConfigModeThirdPartyNormal,
        CAM_COMBMODE_FRONT,
        {3, 2, 8000},
    },
    {
        StreamConfigModeThirdPartyMFNR,
        CAM_COMBMODE_REAR_WIDE,
        {3, 2, 8000},
    },
    {
        StreamConfigModeThirdPartyMFNR,
        CAM_COMBMODE_FRONT,
        {3, 2, 8000},
    },
    {
        StreamConfigModeThirdPartySmartEngine,
        CAM_COMBMODE_REAR_WIDE,
        {3, 2, 8000},
    },
    {
        StreamConfigModeThirdPartySmartEngine,
        CAM_COMBMODE_FRONT,
        {3, 2, 8000},
    },
    {
        StreamConfigModeThirdPartyFastMode,
        CAM_COMBMODE_REAR_WIDE,
        {3, 2, 8000},
    },
    {
        StreamConfigModeThirdPartyFastMode,
        CAM_COMBMODE_FRONT,
        {3, 2, 8000},
    },
    {
        StreamConfigModeNormal,
        CAM_COMBMODE_REAR_WIDE,
        {4, 4, 6000},
    },
    {
        StreamConfigModeNormal,
        CAM_COMBMODE_FRONT,
        {4, 4, 6000},
    }};

} // namespace mialgo2

#endif // end define __MI_MEMORY_RECLAIM__
