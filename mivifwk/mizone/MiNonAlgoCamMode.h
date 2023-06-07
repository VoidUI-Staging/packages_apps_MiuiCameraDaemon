/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _MIZONE_MINONALGOCAMMODE_H_
#define _MIZONE_MINONALGOCAMMODE_H_

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <thread>

#include "MiCamMode.h"
#include "MiImageBuffer.h"
#include "MiZoneTypes.h"

namespace mizone {

class MiCamDevCallback;
class MiVendorCamera;

class MiNonAlgoCamMode : public MiCamMode
{
public:
    MiNonAlgoCamMode(const CreateInfo &info);
    ~MiNonAlgoCamMode() override;
    MiNonAlgoCamMode(const MiNonAlgoCamMode &) = delete;
    MiNonAlgoCamMode &operator=(const MiNonAlgoCamMode &) = delete;

public:
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // External Interface (for MiCamSession)
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    int configureStreams(const StreamConfiguration &requestedConfiguration,
                         HalStreamConfiguration &halConfiguration) override;
    int processCaptureRequest(const std::vector<CaptureRequest> &requests,
                              uint32_t &numRequestProcessed) override;
    void processCaptureResult(const std::vector<CaptureResult> &results) override;
    void notify(const std::vector<NotifyMsg> &msgs) override;
    int flush() override;
    int close() override;

    MiCamModeType getModeType() override { return NonAlgoMode; };

    // for HAL Buffer Manage (HBM)
    void signalStreamFlush(const std::vector<int32_t> &streamIds,
                           uint32_t streamConfigCounter) override;
    void requestStreamBuffers(const std::vector<BufferRequest> &bufReqs,
                              requestStreamBuffers_cb cb) override;
    void returnStreamBuffers(const std::vector<StreamBuffer> &buffers) override;

    // for Offline Session
    int switchToOffline(const std::vector<int64_t> &streamsToKeep,
                        CameraOfflineSessionInfo &offlineSessionInfo) override;
    void setCallback(std::shared_ptr<MiCamDevCallback> deviceCallback) override;
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // Create info
    CameraInfo mCameraInfo;
    MiMetadata mStaticInfo;
    std::shared_ptr<MiCamDevCallback> mDeviceCallback;
    std::shared_ptr<MiVendorCamera> mDeviceSession;
    bool mSupportCoreHBM;
    bool mSupportAndroidHBM;
    std::map<int32_t, MiMetadata> mPhysicalMetadataMap;
};

} // namespace mizone

#endif //_MIZONE_MINONALGOCAMMODE_H_
