/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _MIZONE_MIVENDORCAMERA_H_
#define _MIZONE_MIVENDORCAMERA_H_

#include <mtkcam-android/main/hal/core/device/3.x/device/types.h>
#include <mtkcam-android/utils/metastore/IMetadataProvider.h>
#include <mtkcam-halif/common/types.h>
#include <mtkcam-halif/device/1.x/IMtkcamDevice.h>
#include <mtkcam-halif/device/1.x/IMtkcamDeviceCallback.h>
#include <mtkcam-halif/device/1.x/IMtkcamDeviceSession.h>
#include <mtkcam-halif/device/1.x/types.h>
#include <mtkcam-halif/provider/1.x/IMtkcamProvider.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "MiCamDevCallback.h"
#include "MiZoneTypes.h"

namespace mizone {

class MiVendorCamera
{
private:
    explicit MiVendorCamera(std::shared_ptr<mcam::IMtkcamDeviceSession> session);

public:
    ~MiVendorCamera();
    static std::unique_ptr<MiVendorCamera> create(
        const std::shared_ptr<mcam::IMtkcamDeviceSession> &session);

    int constructDefaultRequestSettings(const RequestTemplate type, MiMetadata &requestTemplate);
    int configureStreams(const StreamConfiguration &requestedConfiguration,
                         HalStreamConfiguration &halConfiguration);
    int processCaptureRequest(const std::vector<CaptureRequest> &requests,
                              uint32_t &numRequestProcessed);

    int queryFeatureSetting(const MiMetadata &info, uint32_t &frameCount,
                            std::vector<MiMetadata> &settings);

    int flush();
    int close();

    void signalStreamFlush(const std::vector<int32_t> &streamIds, uint32_t streamConfigCounter);
    int isReconfigurationRequired(const MiMetadata &oldSessionParams,
                                  const MiMetadata &newSessionParams,
                                  bool &reconfigurationNeeded) const;

private:
    std::shared_ptr<mcam::IMtkcamDeviceSession> mSession = nullptr;
    std::shared_ptr<mcam::IMtkcamProvider> mPostProcProvider = nullptr;
    std::shared_ptr<mcam::IMtkcamDeviceSession> mPostProcSession = nullptr;
};

} // namespace mizone

#endif // _MIZONE_MIVENDORCAMERA_H_
