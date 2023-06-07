/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MI_POSTPROC_CALLBACKS__
#define __MI_POSTPROC_CALLBACKS__

#include <system/camera_metadata.h>
#include <vendor/xiaomi/hardware/campostproc/1.0/IMiPostProcService.h>
#include <vendor/xiaomi/hardware/campostproc/1.0/IMiPostProcSession.h>
#include <vendor/xiaomi/hardware/campostproc/1.0/types.h>
#include <vndk/hardware_buffer.h>
#include <vndk/window.h>

#include <algorithm>
#include <map>

#include "LogUtil.h"
#include "MiaFrameWrapper.h"
#include "hidl/MQDescriptor.h"
#include "hidl/Status.h"
#include "vendor/xiaomi/hardware/campostproc/1.0/IMiPostProcCallBacks.h"

using android::hardware::hidl_death_recipient;
using android::hardware::hidl_handle;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hidl::base::V1_0::IBase;
using vendor::xiaomi::hardware::campostproc::V1_0::BufferParams;
using vendor::xiaomi::hardware::campostproc::V1_0::CreateSessionParams;
using vendor::xiaomi::hardware::campostproc::V1_0::Error;
using vendor::xiaomi::hardware::campostproc::V1_0::HandleParams;
using vendor::xiaomi::hardware::campostproc::V1_0::IMiPostProcCallBacks;
using vendor::xiaomi::hardware::campostproc::V1_0::IMiPostProcService;
using vendor::xiaomi::hardware::campostproc::V1_0::IMiPostProcSession;
using vendor::xiaomi::hardware::campostproc::V1_0::NotifyType;
using vendor::xiaomi::hardware::campostproc::V1_0::OutSurfaceType;
using vendor::xiaomi::hardware::campostproc::V1_0::PostProcResult;
using vendor::xiaomi::hardware::campostproc::V1_0::PreProcessParams;
using vendor::xiaomi::hardware::campostproc::V1_0::SessionProcessParams;
using MiCameraMetadata = vendor::xiaomi::hardware::campostproc::V1_0::CameraMetadata;

using namespace std;
namespace mialgo {

class MiPostProcCallBacks : public IMiPostProcCallBacks
{
public:
    MiPostProcCallBacks();
    virtual ~MiPostProcCallBacks();

    Return<void> notifyCallback(const PostProcResult &result) override;

    Return<void> getOutputBuffer(OutSurfaceType type, int64_t timeStampUs,
                                 getOutputBuffer_cb hidlCb) override;

    void setSessionHandle(void *session) { mSessionIntf = session; };

private:
    void *mSessionIntf;
};

extern "C" IMiPostProcCallBacks *HIDL_FETCH_IMiPostProcCallBacks(const char *name);

} // namespace mialgo

#endif // __MI_POSTPROC_CALLBACKS__
