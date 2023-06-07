#ifndef __MOCK_MODULE_H__
#define __MOCK_MODULE_H__

#include <map>
#include <set>
#include <tuple>

#include "CameraDevice.h"
#include "CameraModule.h"

namespace mihal {

using StreamTuple = std::set<std::tuple<int32_t, int32_t, int32_t, int32_t>>;
using Thumbnails = std::set<std::pair<int32_t, int32_t>>;
struct MergedVendorCameraStaticInfo
{
    StreamTuple streamSet;
    StreamTuple xiaomiStreamSet;
    Thumbnails thumbnailSet;
    int32_t maxJpegSize;
    int32_t jpegDebugSize;
};

class MockModule : public CameraModule
{
public:
    MockModule(int startId, const MergedVendorCameraStaticInfo &mergedInfo);
    ~MockModule() = default;

    int open(const char *id, camera3_device **device) override;
    int setCallbacks(const camera_module_callbacks *callbacks) override;
    void getVendorTagOps(vendor_tag_ops *ops) override;
    uint16_t getModuleApiVersion() const { return CAMERA_MODULE_API_VERSION_CURRENT; }
    const char *getModuleName() const { return "Xiaomi Mock Camera"; }
    uint16_t getHalApiVersion() const { return HARDWARE_HAL_API_VERSION; }
    const char *getModuleAuthor() const { return "Xiaomi"; }
    void notifyDeviceStateChange(uint64_t deviceState) override;

private:
    void initCameraInfo(const MergedVendorCameraStaticInfo &mergedInfo);
    void initDefaultSettings();

    // all the mock cameras share the same camera info, so we put they here
    camera_info mInfo;
    Metadata mStaticMeta;
    Metadata mDefaultSettings;

    camera_module_callbacks mCallbacks;
};

} // namespace mihal

#endif // __MOCK_MODULE_H__
