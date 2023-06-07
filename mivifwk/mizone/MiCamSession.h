/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _MIZONE_MICAMESESSION_H_
#define _MIZONE_MICAMESESSION_H_

#include <mtkcam-halif/common/types.h>
#include <mtkcam-halif/device/1.x/IMtkcamDeviceCallback.h>
#include <mtkcam-halif/device/1.x/IMtkcamDeviceSession.h>
#include <mtkcam-halif/device/1.x/types.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "MiCamDevCallback.h"
#include "MiZoneTypes.h"

namespace mizone {

class MiVendorCamera;
class MiCamMode;

class MiCamSession : public mcam::IMtkcamDeviceSession, public mcam::IMtkcamDeviceCallback
{
public:
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  Definitions.
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    struct CreationInfo
    {
        int32_t intanceId;
        NSCam::IMetadata staticMetadata;
        bool supportCoreHBM = false;
        bool supportAndroidHBM = false;
        std::map<int32_t, NSCam::IMetadata> physicalMetadataMap;
    };
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  Operations.
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ~MiCamSession() override;
    explicit MiCamSession(const CreationInfo &info);
    static auto create(const CreationInfo &info) -> MiCamSession *;

    virtual int open(const std::shared_ptr<mcam::IMtkcamDeviceCallback> &callback);

    void setCameraDeviceSession(const std::shared_ptr<mcam::IMtkcamDeviceSession> &session);

    void setCusCameraDeviceSession(const std::shared_ptr<mcam::IMtkcamDeviceSession> &session);
    void setCusCameraDeviceCallback(const std::shared_ptr<mcam::IMtkcamDeviceCallback> &callback);

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  ICameraDeviceSession Interfaces.
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    int constructDefaultRequestSettings(const mcam::RequestTemplate type,
                                        NSCam::IMetadata &requestTemplate) override;
    int configureStreams(const mcam::StreamConfiguration &requestedConfiguration,
                         mcam::HalStreamConfiguration &halConfiguration) override;
    int processCaptureRequest(const std::vector<mcam::CaptureRequest> &requests,
                              uint32_t &numRequestProcessed) override;
    int flush() override;
    int close() override;
    // ::android::hardware::camera::device::V3_5
    void signalStreamFlush(const std::vector<int32_t> &streamIds,
                           uint32_t streamConfigCounter) override;
    int isReconfigurationRequired(const NSCam::IMetadata &oldSessionParams,
                                  const NSCam::IMetadata &newSessionParams,
                                  bool &reconfigurationNeeded) const override;

    // ::android::hardware::camera::device::V3_6
    int switchToOffline(const std::vector<int64_t> &streamsToKeep,
                        mcam::CameraOfflineSessionInfo &offlineSessionInfo,
                        std::shared_ptr<mcam::IMtkcamOfflineSession> &offlineSession) override;

    int queryFeatureSetting(const IMetadata &info __unused, uint32_t &frameCount __unused,
                            std::vector<IMetadata> &settings __unused) override
    {
        return static_cast<int>(mcam::Status::OPERATION_NOT_SUPPORTED);
    }
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#if 1
    // TODO: copy from MTK, should be removed later
    // WARNING: It should be removed after custom zone moved down.
    int getConfigurationResults(const uint32_t streamConfigCounter,
                                mcam::ExtConfigurationResults &ConfigurationResults) override;
#endif

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  ::android::hardware::camera::device::V3_5::ICameraDeviceCallback
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    void notify(const std::vector<mcam::NotifyMsg> &msgs) override;

    void processCaptureResult(const std::vector<mcam::CaptureResult> &results) override;

    void requestStreamBuffers(const std::vector<mcam::BufferRequest> &bufReqs,
                              requestStreamBuffers_cb cb) override;

    void returnStreamBuffers(const std::vector<mcam::StreamBuffer> &buffers) override;
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

private:
    std::shared_ptr<mcam::IMtkcamDeviceSession> mSession = nullptr;
    std::shared_ptr<MiCamDevCallback> mCameraDeviceCallback = nullptr;

    std::unique_ptr<MiCamMode> mCamMode;

protected:
    int32_t mCameraId = -1;
    NSCam::IMetadata mStaticInfo;
    bool mSupportCoreHBM = false;
    bool mSupportAndroidHBM = false;
    std::map<int32_t, NSCam::IMetadata> mPhysicalMetadataMap;
    bool mUsingMode = false;
    std::shared_ptr<mcam::IMtkcamDeviceSession> mCusSession;
    std::shared_ptr<mcam::IMtkcamDeviceCallback> mCusDeviceCallbacks;
};

} // namespace mizone

#endif // _MIZONE_MICAMESESSION_H_
