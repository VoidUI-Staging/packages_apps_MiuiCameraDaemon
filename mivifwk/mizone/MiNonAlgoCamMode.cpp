/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiNonAlgoCamMode.h"

#include <MiDebugUtils.h>

#include <thread>

#include "MiVendorCamera.h"

using namespace midebug;

namespace mizone {

MiNonAlgoCamMode::MiNonAlgoCamMode(const CreateInfo &info)
    : mStaticInfo(info.staticInfo),
      mDeviceCallback(info.deviceCallback),
      mDeviceSession(info.deviceSession),
      mSupportCoreHBM(info.supportCoreHBM),
      mSupportAndroidHBM(info.supportAndroidHBM),
      mPhysicalMetadataMap(info.physicalMetadataMap)
{
    MLOGI(LogGroupCore, "ctor");

    MLOGD(LogGroupCore, "mSupportCoreHBM = %d, mSupportAndroidHBM = %d", mSupportCoreHBM,
          mSupportAndroidHBM);
}

MiNonAlgoCamMode::~MiNonAlgoCamMode()
{
    MLOGI(LogGroupCore, "dtor");
}

int MiNonAlgoCamMode::configureStreams(const StreamConfiguration &requestedConfiguration,
                                       HalStreamConfiguration &halConfiguration)
{
    return mDeviceSession->configureStreams(requestedConfiguration, halConfiguration);
}

int MiNonAlgoCamMode::processCaptureRequest(const std::vector<CaptureRequest> &requests,
                                            uint32_t &numRequestProcessed)
{
    return mDeviceSession->processCaptureRequest(requests, numRequestProcessed);
}

void MiNonAlgoCamMode::processCaptureResult(const std::vector<CaptureResult> &results)
{
    mDeviceCallback->processCaptureResult(results);
}

void MiNonAlgoCamMode::notify(const std::vector<NotifyMsg> &msgs)
{
    mDeviceCallback->notify(msgs);
}

int MiNonAlgoCamMode::flush()
{
    return mDeviceSession->flush();
}

int MiNonAlgoCamMode::close()
{
    return mDeviceSession->close();
}

// for HAL Buffer Manage (HBM)
void MiNonAlgoCamMode::signalStreamFlush(const std::vector<int32_t> &streamIds,
                                         uint32_t streamConfigCounter)
{
    return mDeviceSession->signalStreamFlush(streamIds, streamConfigCounter);
}

void MiNonAlgoCamMode::requestStreamBuffers(const std::vector<BufferRequest> &bufReqs,
                                            requestStreamBuffers_cb cb)
{
    mDeviceCallback->requestStreamBuffers(bufReqs, cb);
}

void MiNonAlgoCamMode::returnStreamBuffers(const std::vector<StreamBuffer> &buffers)
{
    mDeviceCallback->returnStreamBuffers(buffers);
}

// for Offline Session
int MiNonAlgoCamMode::switchToOffline(const std::vector<int64_t> &streamsToKeep,
                                      CameraOfflineSessionInfo &offlineSessionInfo)
{
    MLOGE(LogGroupCore, "MiNonAlgoCamMode does not support switchToOffline!");
    offlineSessionInfo.offlineRequests.clear();
    offlineSessionInfo.offlineStreams.clear();
    return Status::INTERNAL_ERROR;
}

void MiNonAlgoCamMode::setCallback(std::shared_ptr<MiCamDevCallback> deviceCallback)
{
    MLOGE(LogGroupCore, "MiNonAlgoCamMode does not support setCallback!");
}

} // namespace mizone
