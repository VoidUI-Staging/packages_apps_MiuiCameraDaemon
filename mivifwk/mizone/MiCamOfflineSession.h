/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _MIZONE_MICAMOFFLINESESSION_H_
#define _MIZONE_MICAMOFFLINESESSION_H_

#include <mtkcam-halif/device/1.x/IMtkcamOfflineSession.h>

#include <memory>
#include <string>
#include <vector>

using mcam::IMtkcamOfflineSession;

namespace mizone {

class MiCamMode;
class MiCamDevCallback;

class MiCamOfflineSession : public mcam::IMtkcamOfflineSession
{
public:
    ~MiCamOfflineSession() override;
    MiCamOfflineSession(int32_t cameraId, std::unique_ptr<MiCamMode> mode);
    static MiCamOfflineSession *create(int32_t cameraId, std::unique_ptr<MiCamMode> mode);

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  ICameraOfflineSession Interfaces.
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    void setCallback(const std::shared_ptr<mcam::IMtkcamDeviceCallback> &callback) override;
    int close() override;

protected:
    int32_t mCameraId = -1;
    std::shared_ptr<mizone::MiCamDevCallback> mCameraDeviceCallback = nullptr;
    std::unique_ptr<MiCamMode> mCamMode;
};

}; // namespace mizone

#endif // _MIZONE_MICAMOFFLINESESSION_H_
