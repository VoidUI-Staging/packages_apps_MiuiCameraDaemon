/*
 * Copyright (c) 2021 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#include "BgServiceIntf.h"

#include <hidl/HidlTransportSupport.h>
#include <vendor/xiaomi/hardware/bgservice/1.0/IBGService.h>

#include "BGService.h"
#include "LogUtil.h"

using android::hardware::configureRpcThreadpool;
using vendor::xiaomi::hardware::bgservice::implementation::BGService;
using vendor::xiaomi::hardware::bgservice::V1_0::IBGService;

using namespace mihal;

///< Number of parallel threads needed in each session
const uint32_t NumberOfThreadsNeeded = 4;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RegisterIBGService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RegisterIBGService()
{
    static android::sp<IBGService> sIBGService = NULL;

    if (sIBGService == NULL) {
        MLOGI(LogGroupCore, "BGService register started");
        sIBGService = new BGService();

        if (sIBGService->registerAsService() != android::OK) {
            MLOGW(LogGroupCore, "BGService register failed");
        } else {
            // configureRpcThreadpool is already done as part of cameraService.
            // This is not needed to call from HIDL services bring up inside CameraService.
            MLOGI(LogGroupCore, "BGService register successfully");
        }
    }

    return;
}
