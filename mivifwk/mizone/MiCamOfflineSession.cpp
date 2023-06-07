/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiCamOfflineSession.h"

#include <MiDebugUtils.h>

#include <memory>
#include <vector>

#include "MiCamDevCallback.h"
#include "MiCamMode.h"

using namespace midebug;

#define MZ_LOGV(group, fmt, arg...) MLOGV(group, "[%d]" fmt, mCameraId, ##arg)

#define MZ_LOGD(group, fmt, arg...) MLOGD(group, "[%d]" fmt, mCameraId, ##arg)

#define MZ_LOGI(group, fmt, arg...) MLOGI(group, "[%d]" fmt, mCameraId, ##arg)

#define MZ_LOGW(group, fmt, arg...) MLOGW(group, "[%d]" fmt, mCameraId, ##arg)

#define MZ_LOGE(group, fmt, arg...) MLOGE(group, "[%d]" fmt, mCameraId, ##arg)

#define MZ_LOGF(group, fmt, arg...) MLOGF(group, "[%d]" fmt, mCameraId, ##arg)

namespace mizone {

MiCamOfflineSession::MiCamOfflineSession(int32_t cameraId, std::unique_ptr<MiCamMode> mode)
    : mCameraId(cameraId), mCameraDeviceCallback(nullptr), mCamMode(std::move(mode))
{
    MZ_LOGI(LogGroupCore, "ctor");
    MiDebugInterface::getInstance()->startOfflineLogger();
}

MiCamOfflineSession::~MiCamOfflineSession()
{
    MZ_LOGI(LogGroupCore, "dtor");
    MiDebugInterface::getInstance()->stopOfflineLogger();
}

MiCamOfflineSession *MiCamOfflineSession::create(int32_t cameraId, std::unique_ptr<MiCamMode> mode)
{
    auto *pInstance = new MiCamOfflineSession(cameraId, std::move(mode));

    if (pInstance == nullptr) {
        MLOGE(LogGroupCore, "[%d]Create MiCamOfflineSession failed!", cameraId);
        delete pInstance;
        return nullptr;
    }

    return pInstance;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICameraOfflineSession  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void MiCamOfflineSession::setCallback(const std::shared_ptr<mcam::IMtkcamDeviceCallback> &callback)
{
    MZ_LOGI(LogGroupCore, "+");

    if (mCameraDeviceCallback != nullptr) {
        MZ_LOGE(LogGroupCore, "Offline session's callback can only be once, old: %p, new: %p!",
                mCameraDeviceCallback.get(), callback.get());
        return;
    }

    // create MiCamDevCallback
    auto *pCusDeviceCallback = MiCamDevCallback::create(mCameraId);
    if (pCusDeviceCallback == nullptr) {
        MZ_LOGE(LogGroupCore, "MiCamDevCallback create failed!");
        return;
    }

    auto customCallback = std::shared_ptr<mizone::MiCamDevCallback>(pCusDeviceCallback);
    customCallback->setCustomSessionCallback(callback); // set callback in MiCamDevCallback

    mCameraDeviceCallback = customCallback;
    mCamMode->setCallback(mCameraDeviceCallback);
    MZ_LOGI(LogGroupCore, "-");
}

int MiCamOfflineSession::close()
{
    MZ_LOGI(LogGroupCore, "+");
    auto status = mCamMode->close();
    MZ_LOGI(LogGroupCore, "-");
    return status;
}

}; // namespace mizone
