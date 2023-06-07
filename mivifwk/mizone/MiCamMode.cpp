/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiCamMode.h"

#include <MiDebugUtils.h>

#include "MiAlgoCamMode.h"
#include "MiNonAlgoCamMode.h"
#include "xiaomi/MiCommonTypes.h"

using namespace midebug;

namespace mizone {

std::unique_ptr<MiCamMode> MiCamMode::createMode(const CreateInfo &info)
{
    MLOGD(LogGroupCore, "operationMode = 0x%x", info.config.operationMode);

    if (info.deviceSession == nullptr) {
        MLOGE(LogGroupCore, "create MiCamMode failed: info.deviceSession is nullptr!");
        return nullptr;
    }

    if (info.deviceCallback == nullptr) {
        MLOGE(LogGroupCore, "create MiCamMode failed: info.deviceCallback is nullptr!");
        return nullptr;
    }

    bool isAlgoMode = false;

    switch (static_cast<int32_t>(info.config.operationMode)) {
    // MiuiCamera
    case StreamConfigModeAlgoupDualBokeh:
    case StreamConfigModeAlgoupSingleBokeh:
    case StreamConfigModeAlgoupHD:
    case StreamConfigModeAlgoupNormal:
    case StreamConfigModeMiuiSuperNight:
    case StreamConfigModeAlgoupManualUltraPixel:
    case StreamConfigModeAlgoupManual: {
        isAlgoMode = true;
        break;
    }

    // 3rd-party camera / MiuiCamera vedio
    default: {
        isAlgoMode = false;
        break;
    }
    }

    if (isAlgoMode) {
        return std::make_unique<MiAlgoCamMode>(info);
    }
    return std::make_unique<MiNonAlgoCamMode>(info);
}

} // namespace mizone
