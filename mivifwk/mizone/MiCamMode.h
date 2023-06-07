/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _MIZONE_MICAMMODE_H_
#define _MIZONE_MICAMMODE_H_

#include "MiZoneTypes.h"

namespace mizone {

class MiCamDevCallback;
class MiVendorCamera;

class MiCamMode
{
public:
    struct CreateInfo
    {
        int32_t cameraId;
        MiMetadata staticInfo;
        std::shared_ptr<MiCamDevCallback> deviceCallback;
        std::shared_ptr<MiVendorCamera> deviceSession;
        bool supportCoreHBM;
        bool supportAndroidHBM;
        //     <phyCamId, staticPhyMeta>
        std::map<int32_t, MiMetadata> physicalMetadataMap;
        StreamConfiguration config;
    };

    enum MiCamModeType { AlgoMode, NonAlgoMode, BaseMode };

    static std::unique_ptr<MiCamMode> createMode(const CreateInfo &info);
    MiCamMode() = default;
    virtual ~MiCamMode() = default;
    MiCamMode(const MiCamMode &) = delete;
    MiCamMode &operator=(const MiCamMode &) = delete;

public:
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // External Interface (for MiCamSession)
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    virtual int configureStreams(const StreamConfiguration &requestedConfiguration,
                                 HalStreamConfiguration &halConfiguration) = 0;
    virtual int processCaptureRequest(const std::vector<CaptureRequest> &requests,
                                      uint32_t &numRequestProcessed) = 0;
    virtual void processCaptureResult(const std::vector<CaptureResult> &results) = 0;
    virtual void notify(const std::vector<NotifyMsg> &msgs) = 0;
    virtual int flush() = 0;
    virtual int close() = 0;

    virtual MiCamModeType getModeType() { return BaseMode; };

    // for HAL Buffer Manage (HBM)
    using requestStreamBuffers_cb = std::function<void(
        BufferRequestStatus status, const std::vector<StreamBufferRet> &bufferRets)>;
    virtual void signalStreamFlush(const std::vector<int32_t> &streamIds,
                                   uint32_t streamConfigCounter) = 0;
    virtual void requestStreamBuffers(const std::vector<BufferRequest> &bufReqs,
                                      requestStreamBuffers_cb cb) = 0;
    virtual void returnStreamBuffers(const std::vector<StreamBuffer> &buffers) = 0;

    // for Offline Session
    virtual int switchToOffline(const std::vector<int64_t> &streamsToKeep,
                                CameraOfflineSessionInfo &offlineSessionInfo) = 0;
    virtual void setCallback(std::shared_ptr<MiCamDevCallback> deviceCallback) = 0;
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
};

} // namespace mizone

#endif //_MIZONE_MICAMMODE_H_
