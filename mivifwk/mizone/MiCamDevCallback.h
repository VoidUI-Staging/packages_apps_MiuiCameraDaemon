/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _MIZONE_MICAMDEVCALLBACK_H_
#define _MIZONE_MICAMDEVCALLBACK_H_

#include <mtkcam-halif/common/types.h>
#include <mtkcam-halif/device/1.x/IMtkcamDeviceCallback.h>
#include <mtkcam-halif/device/1.x/types.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "MiZoneTypes.h"

namespace mizone {

class MiCamDevCallback
{
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  Operations.
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public: ////        Instantiation.
    virtual ~MiCamDevCallback();
    explicit MiCamDevCallback(int32_t cameraId);
    static MiCamDevCallback *create(int32_t cameraId);

    ////        Operations.
    virtual void setCustomSessionCallback(
        const std::shared_ptr<mcam::IMtkcamDeviceCallback> callback);

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  ::android::hardware::camera::device::V3_5::ICameraDeviceCallback
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    void processCaptureResult(const std::vector<CaptureResult> &results);

    void notify(const std::vector<NotifyMsg> &msgs);

    using requestStreamBuffers_cb = std::function<void(
        BufferRequestStatus status, const std::vector<StreamBufferRet> &bufferRets)>;
    void requestStreamBuffers(const std::vector<BufferRequest> &bufferRequests,
                              requestStreamBuffers_cb callback);

    void returnStreamBuffers(const std::vector<StreamBuffer> &buffers);

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:
    std::weak_ptr<mcam::IMtkcamDeviceCallback> mCustomCameraDeviceCallback;
    int32_t mCameraId = -1;
};

}; // namespace mizone

#endif // _MIZONE_MICAMDEVCALLBACK_H_
