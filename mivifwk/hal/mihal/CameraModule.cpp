
#include "CameraModule.h"

#include "LogUtil.h"
#include "Metadata.h"

namespace mihal {

CameraDevice *CameraModule::getCameraDevice(int id)
{
    if (id < mStartId || id >= (mStartId + mCameras.size()))
        return nullptr;
    else
        return mCameras[id - mStartId].get();
}

int CameraModule::getCameraInfo(int cameraId, camera_info *info)
{
    auto device = getCameraDevice(cameraId);
    if (!device) {
        MLOGE(LogGroupHAL, "invalid camera id %d for this module", cameraId);
        return -EINVAL;
    }

    return device->getCameraInfo(info);
}

bool CameraModule::isLogicalMultiCamera(const Metadata &info,
                                        std::set<std::string> *physicalCameraIds)
{
    if (physicalCameraIds == nullptr) {
        MLOGE(LogGroupHAL, "physicalCameraIds must not be null");
        return false;
    }

    bool isLogicalMultiCamera = false;
    camera_metadata_ro_entry capabilities = info.find(ANDROID_REQUEST_AVAILABLE_CAPABILITIES);
    for (size_t i = 0; i < capabilities.count; i++) {
        if (capabilities.data.u8[i] ==
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA) {
            isLogicalMultiCamera = true;
            break;
        }
    }

    if (isLogicalMultiCamera) {
        camera_metadata_ro_entry entry = info.find(ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS);
        const uint8_t *ids = entry.data.u8;
        size_t start = 0;
        for (size_t i = 0; i < entry.count; ++i) {
            if (ids[i] == '\0') {
                if (start != i) {
                    const char *physicalId = reinterpret_cast<const char *>(ids + start);
                    physicalCameraIds->emplace(physicalId);
                }
                start = i + 1;
            }
        }
    }

    return isLogicalMultiCamera;
}

} // namespace mihal
