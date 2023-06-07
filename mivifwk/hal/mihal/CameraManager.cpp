
#include "CameraManager.h"

#include <cutils/properties.h>
#include <hardware/hardware.h>
#include <math.h>

#include <algorithm>
#include <chrono>

#include "LogUtil.h"
#include "MiviSettings.h"
#include "MockModule.h"
#include "Session.h"
#include "VendorModule.h"

namespace mihal {

using namespace std::chrono_literals;

namespace {

extern "C" {
int mihal_hw_get_module(const char *id, const struct hw_module_t **module);
}

std::unique_ptr<CameraModule> CreateVendorModule(int startId)
{
    std::unique_ptr<CameraModule> module{};
    int err;

    camera_module *rawModule;
    err = mihal_hw_get_module(CAMERA_HARDWARE_MODULE_ID, (const hw_module_t **)&rawModule);
    if (err) {
        MLOGE(LogGroupHAL, "failed to load hardware camera module %d", err);
        return module;
    }
    module = std::make_unique<VendorModule>(rawModule, startId);

    return module;
}

} // namespace

CameraManager *CameraManager::getInstance()
{
    static CameraManager manager;

    if (!manager.inited())
        return nullptr;

    return &manager;
}

CameraManager::CameraManager() : mInit{false}, mVendorModule{nullptr}, mMockModule{nullptr}
{
    mVendorModule = CreateVendorModule(0);
    if (!mVendorModule) {
        MLOGE(LogGroupHAL, "fatal error and now exit the constructor");
        return;
    }

    int vendorCameraNum = mVendorModule->getNumberOfCameras();
    MergedVendorCameraStaticInfo mergedInfo{};
    for (int i = 0; i < vendorCameraNum; i++) {
        camera_info info;
        mVendorModule->getCameraInfo(i, &info);
        const Metadata meta{info.static_camera_characteristics};
        camera_metadata_ro_entry entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
        if (entry.count) {
            for (int j = 0; j < entry.count; j += 4) {
                int32_t format = entry.data.i32[j];
                if (format != HAL_PIXEL_FORMAT_BLOB)
                    continue;

                int32_t width = entry.data.i32[j + 1];
                int32_t height = entry.data.i32[j + 2];
                int32_t dir = entry.data.i32[j + 3];

                mergedInfo.streamSet.insert(std::make_tuple(format, width, height, dir));
            }
        }

        entry = meta.find(MI_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
        if (entry.count) {
            for (int j = 0; j < entry.count; j += 4) {
                int32_t format = entry.data.i32[j];
                if (format != HAL_PIXEL_FORMAT_BLOB)
                    continue;

                int32_t width = entry.data.i32[j + 1];
                int32_t height = entry.data.i32[j + 2];
                int32_t dir = entry.data.i32[j + 3];

                mergedInfo.xiaomiStreamSet.insert(std::make_tuple(format, width, height, dir));
            }
        }

        entry = meta.find(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES);
        if (entry.count) {
            for (int j = 0; j < entry.count; j += 2) {
                int32_t width = entry.data.i32[j];
                int32_t height = entry.data.i32[j + 1];

                mergedInfo.thumbnailSet.insert(std::make_pair(width, height));
            }
        }

        entry = meta.find(ANDROID_JPEG_MAX_SIZE);
        if (entry.count) {
            int maxSize = *entry.data.i32;
            mergedInfo.maxJpegSize = std::max(maxSize, mergedInfo.maxJpegSize);
        }

        entry = meta.find(QCOM_JPEG_DEBUG_DATA);
        if (entry.count) {
            int jpegDebugSize = *entry.data.i32;
            mergedInfo.jpegDebugSize = std::max(jpegDebugSize, mergedInfo.jpegDebugSize);
        }
    }

    mMockMaxJpegSize = mergedInfo.maxJpegSize;
    mMockModule = std::make_unique<MockModule>(vendorCameraNum, mergedInfo);
    if (!mMockModule) {
        MLOGW(LogGroupHAL, "VirutalModule create failed");
    }

    mInit = true;

    MLOGI(LogGroupHAL, "CameraManager created: total %d CameraDevice enumerated",
          getNumberOfCameras());
}

CameraDevice *CameraManager::getCameraDevice(int id) const
{
    return mVendorModule->getCameraDevice(id) ?: mMockModule->getCameraDevice(id);
}

std::tuple<int /* result */, CameraDevice *> CameraManager::open(int id)
{
    auto device = getCameraDevice(id);
    if (!device) {
        MLOGE(LogGroupHAL, "invalid camera id:%d", id);
        return std::make_tuple(-ENODEV, nullptr);
    }

    int err = device->open();
    if (err) {
        MLOGE(LogGroupHAL, "failed to open camera :%d, err=%d", id, err);
        return std::make_tuple(err, nullptr);
    }

    MLOGI(LogGroupHAL, "camera %d opened", id);
    return std::make_tuple(0, device);
}

int CameraManager::getNumberOfCameras() const
{
    {
        // Here is a trick implement for gsi/vts test. because the tests
        // will replace system.img which google providing, then ro.build.product
        // property value also change as "generic_arm64".
        char prop[PROPERTY_VALUE_MAX] = {0};
        if (property_get("ro.build.product", prop, NULL) > 0) {
            if (strcasecmp(prop, "generic_arm64") == 0)
                return (mVendorModule->getNumberOfCameras() < 2) ?: 2;
        }
    }
    return mVendorModule->getNumberOfCameras() + mMockModule->getNumberOfCameras();
}

int CameraManager::getCameraInfo(int id, struct camera_info *info) const
{
    auto device = getCameraDevice(id);
    if (!device) {
        MLOGE(LogGroupHAL, "invalid camera id:%d", id);
        return -ENODEV;
    }

    return device->getCameraInfo(info);
}

int CameraManager::setCallbacks(const camera_module_callbacks *callbacks)
{
    // FIXME: now just use the VendorModule's ops for test
    return mVendorModule->setCallbacks(callbacks);
}

// TODO: vendor tags:
// we should call get_vendor_tag_ops() one by one for all types of camera modules
// and then store all the tags together to the framework
void CameraManager::getVendorTagOps(vendor_tag_ops *ops)
{
    // FIXME: now just use the VendorModule's ops for test
    mVendorModule->getVendorTagOps(ops);
}

int CameraManager::setTorchMode(const char *id, bool enabled)
{
    int index = atoi(id);
    auto device = getCameraDevice(index);
    if (!device) {
        MLOGE(LogGroupHAL, "invalid camera id:%s", id);
        return -ENODEV;
    }

    MLOGI(LogGroupHAL, "set torch for camera %s to %s", id, enabled ? "on" : "off");
    return device->setTorchMode(enabled);
}

int CameraManager::turnOnTorchWithStrengthLevel(const char *id, int32_t torchStrength)
{
    int index = atoi(id);
    auto device = getCameraDevice(index);
    if (!device) {
        MLOGE(LogGroupHAL, "invalid camera id:%s", id);
        return -ENODEV;
    }

    MLOGI(LogGroupHAL, "Turn on torch for camera %s with strength-level %d", id, torchStrength);
    return device->turnOnTorchWithStrengthLevel(torchStrength);
}

int CameraManager::getTorchStrengthLevel(const char *id, int32_t *strengthLevel)
{
    int index = atoi(id);
    auto device = getCameraDevice(index);
    if (!device) {
        MLOGE(LogGroupHAL, "invalid camera id:%s", id);
        return -ENODEV;
    }

    MLOGI(LogGroupHAL, "get torch strength-level for camera %s", id);
    return device->getTorchStrengthLevel(strengthLevel);
}

int CameraManager::getPhysicalCameraInfo(int id, camera_metadata **metadata) const
{
    const Metadata *meta;
    int err = mVendorModule->getPhysicalCameraInfo(id, &meta);
    *metadata = const_cast<camera_metadata *>(meta->peek());

    return err;
}

int CameraManager::isStreamCombinationSupported(int id,
                                                const camera_stream_combination *streams) const
{
    return mVendorModule->isStreamCombinationSupported(id, streams);
}

void CameraManager::notifyDeviceStateChange(uint64_t deviceState)
{
    mVendorModule->notifyDeviceStateChange(deviceState);
}

int CameraManager::getConcurrentStreamingCameraIds(uint32_t *pConcCamArrayLength,
                                                   concurrent_camera_combination **ppConcCamArray)
{
    return mVendorModule->getConcurrentStreamingCameraIds(pConcCamArrayLength, ppConcCamArray);
}

int CameraManager::isConcurrentStreamCombinationSupported(
    uint32_t numCombinations, const cameraid_stream_combination_t *pStreamComb)
{
    return mVendorModule->isConcurrentStreamCombinationSupported(numCombinations, pStreamComb);
}

bool CameraManager::isFovCropFeatureSupported(const Metadata &meta)
{
    // NOTE: Refer to the following documents for the fovCrop feature:
    // https://xiaomi.f.mioffice.cn/docs/dock4ATVQNwuVTBGq8HfEePaSmb
    bool isSupport = false;
    camera_metadata_ro_entry entry = meta.find(MI_SUPPORT_FEATURE_FOV_CROP);
    if (entry.count) {
        // NOTE: Only when isSupport is 1, we think the fovCrop of capture is on. The other
        // values of fovCrop express the other features.
        uint8_t fovCrop = entry.data.u8[0];
        if (fovCrop == 1)
            isSupport = true;
    }

    return isSupport;
}

std::pair<int, int> CameraManager::getMaxYUVResolution(int32_t cameraId, float ratio,
                                                       bool isUpscale)
{
    //       /*cameraId*/,                 /*width*/ /*height*/
    std::map<uint32_t, std::vector<std::pair<uint32_t, uint32_t>>> maxYUVResolution;
    float currentFrameRatio = ratio;
    std::pair<int, int> resolution = {};
    std::vector<float> frameRatio;
    MiviSettings::getVal(
        "InternalSnapshotStreamProperty.YuvStreamResolutionOptions.AspectRatioList", frameRatio,
        std::vector<float>{1, 1.33333, 1.77777, 2.22222});
    float tolerance;
    MiviSettings::getVal("InternalSnapshotStreamProperty.YuvStreamResolutionOptions.Tolerance",
                         tolerance, 0.05);

    auto isFloatEqual = [](float src, float dst, float delta) {
        return (src - delta <= dst && src + delta >= dst);
    };

    auto getCurrentRatio = [=](float &ratio) -> int {
        int index = 0;
        for (auto item : frameRatio) {
            if (isFloatEqual(item, ratio, tolerance)) {
                ratio = item;
                return index;
            }
            index++;
        }
        return -1;
    };

    int index = getCurrentRatio(currentFrameRatio);
    if (index == -1) {
        MLOGE(LogGroupHAL, "can't find Can't find the right frame ratio");
        return resolution;
    }
    MLOGI(LogGroupHAL, "index = %d currentFrameRatio = %f", index, currentFrameRatio);

    if (maxYUVResolution.find(cameraId) == maxYUVResolution.end()) {
        camera_info info;
        mVendorModule->getCameraInfo(cameraId, &info);
        const Metadata meta{info.static_camera_characteristics};
        camera_metadata_ro_entry entry = meta.find(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE);
        if (isFovCropFeatureSupported(meta)) {
            entry = meta.find(MI_SENSOR_INFO_ACTIVE_ARRAY_SIZE);
            MLOGI(LogGroupHAL,
                  "FovCrop feature is on! Try get value from xiaomi.sensor.info.activeArraySize");
        }
        SensorActiveSize sensorActiveSize = {};
        if (entry.count) {
            sensorActiveSize = *reinterpret_cast<const SensorActiveSize *>(entry.data.i32);
            if (isUpscale) {
                int upScaleFactor;
                MiviSettings::getVal("InternalSnapshotStreamProperty.UpScaleSizeFactor",
                                     upScaleFactor, 2);
                sensorActiveSize.width *= upScaleFactor;
                sensorActiveSize.height *= upScaleFactor;
            }
        }
        entry = meta.find(MI_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
        std::vector<std::pair<uint32_t, uint32_t>> maxSizes(frameRatio.size(), {0, 0});
        if (entry.count) {
            for (int i = 0; i < entry.count; i += 4) {
                int32_t format = entry.data.i32[i];
                if (format != HAL_PIXEL_FORMAT_YCBCR_420_888)
                    continue;

                int32_t width = entry.data.i32[i + 1];
                int32_t height = entry.data.i32[i + 2];
                int32_t dir = entry.data.i32[i + 3];

                if (dir == 0) {
                    float currentRatio = width * 1.0f / height;
                    int index = getCurrentRatio(currentRatio);
                    if (index == -1) {
                        continue;
                    }
                    auto imageSize = maxSizes[index];
                    if (width >= imageSize.first && height >= imageSize.second) {
                        if (sensorActiveSize.height != 0 && sensorActiveSize.width != 0) {
                            if (width <= sensorActiveSize.width &&
                                height <= sensorActiveSize.height) {
                                maxSizes[index] = {width, height};
                            }
                        } else {
                            maxSizes[index] = {width, height};
                        }
                    }
                }
            }
        }
        maxYUVResolution.insert(std::make_pair(cameraId, maxSizes));
    }

    auto resolutions = maxYUVResolution[cameraId];
    resolution = resolutions[index];
    MLOGI(LogGroupHAL, "width = %d height = %d", resolution.first, resolution.second);
    if (!isFloatEqual((resolution.first * 1.0 / resolution.second), ratio, tolerance)) {
        resolution = {};
        for (auto item : resolutions) {
            if (isFloatEqual((item.first * 1.0 / item.second), ratio, tolerance)) {
                resolution = item;
                MLOGI(LogGroupHAL, "width = %d height = %d ratio = %f", resolution.first,
                      resolution.second, ratio);
            }
        }
    }
    return resolution;
}

std::pair<int, int> CameraManager::getMaxRawResolution(int32_t cameraId, float ratio, int rFormat)
{
    //       /*cameraId*/,                 /*width*/ /*height*/
    std::map<uint32_t, std::vector<std::pair<uint32_t, uint32_t>>> maxRawResolution;
    float currentFrameRatio = ratio;
    std::pair<int, int> resolution = {};
    std::vector<float> frameRatio;
    MiviSettings::getVal(
        "InternalSnapshotStreamProperty.RawStreamResolutionOptions.AspectRatioList", frameRatio,
        std::vector<float>{1, 1.33, 1.77, 2.22});
    float tolerance;
    MiviSettings::getVal("InternalSnapshotStreamProperty.RawStreamResolutionOptions.Tolerance",
                         tolerance, 0.3);

    auto getBestRatio = [=](float &ratio) -> int {
        int count = 0;
        int dIndex = -1;
        float diff = tolerance;
        for (auto item : frameRatio) {
            if (fabsf(item - ratio) <= diff) {
                diff = fabsf(item - ratio);
                dIndex = count;
            }
            count++;
        }
        return dIndex;
    };

    int index = getBestRatio(currentFrameRatio);
    if (index == -1) {
        MLOGE(LogGroupHAL, "can't find Can't find the right frame ratio");
        return resolution;
    }

    if (maxRawResolution.find(cameraId) == maxRawResolution.end()) {
        camera_info info;
        mVendorModule->getCameraInfo(cameraId, &info);
        const Metadata meta{info.static_camera_characteristics};
        camera_metadata_ro_entry entry = meta.find(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE);
        if (isFovCropFeatureSupported(meta)) {
            entry = meta.find(MI_SENSOR_INFO_ACTIVE_ARRAY_SIZE);
        }

        // NOTE: physical limit of this sensor, the size found below should be within this range
        SensorActiveSize sensorActiveSize = {};
        if (entry.count) {
            sensorActiveSize = *reinterpret_cast<const SensorActiveSize *>(entry.data.i32);
        }

        entry = meta.find(MI_SCALER_AVAILABLE_LIMIT_STREAM_CONFIGURATIONS);
        if (!entry.count) {
            MLOGE(LogGroupHAL, "Failed to get vendor tag %s",
                  MI_SCALER_AVAILABLE_LIMIT_STREAM_CONFIGURATIONS);
            return resolution;
        }

        std::vector<std::pair<uint32_t, uint32_t>> maxSizes(frameRatio.size(), {0, 0});
        for (int i = 0; i < entry.count; i += 4) {
            int32_t format = entry.data.i32[i];
            if (format != rFormat) {
                continue;
            }

            int32_t width = entry.data.i32[i + 1];
            int32_t height = entry.data.i32[i + 2];

            float currentRatio = width * 1.0f / height;
            int currentIndex = getBestRatio(currentRatio);
            if (currentIndex == -1) {
                continue;
            }

            auto imageSize = maxSizes[currentIndex];
            if (width >= imageSize.first && height >= imageSize.second) {
                if (sensorActiveSize.height != 0 && sensorActiveSize.width != 0) {
                    if (width <= sensorActiveSize.width && height <= sensorActiveSize.height) {
                        maxSizes[currentIndex] = {width, height};
                    }
                } else {
                    maxSizes[currentIndex] = {width, height};
                }
            }
        }
        maxRawResolution.insert(std::make_pair(cameraId, maxSizes));
    }

    auto resolutions = maxRawResolution[cameraId];
    resolution = resolutions[index];

    // NOTE: if current resolution is {0, 0}, then we look for the resolution whose value is not {0,
    // 0} and whose aspect ratio is the cloest to the taget frame ratio
    if (resolution.first == 0 || resolution.second == 0) {
        MLOGW(LogGroupHAL, "find no resolution at target ratio: %f, try nearest one ", ratio);

        // NOTE: sort
        std::vector<float> sortedRatio = frameRatio;
        std::sort(sortedRatio.begin(), sortedRatio.end(),
                  [ratio](float a, float b) { return fabsf(a - ratio) < fabsf(b - ratio); });
        uint32_t bestRatio = -1;
        uint32_t maxNum = resolutions.size();
        for (auto ratioEntry : sortedRatio) {
            MLOGI(LogGroupHAL, "try ratio: %f", ratioEntry);
            bestRatio = getBestRatio(ratioEntry);
            if (bestRatio >= maxNum) {
                continue;
            }
            resolution = resolutions[bestRatio];
            if (resolution.first != 0 && resolution.second != 0) {
                MLOGI(LogGroupHAL, "choose resolution in ratio:%f", ratioEntry);
                break;
            }
        }
    }

    if (resolution.first == 0 || resolution.second == 0) {
        MLOGE(LogGroupHAL, "no resolution found in every ratio, invalid static meta");
        MASSERT(false);
    }

    MLOGI(LogGroupHAL, "index: %d, ratio: %f width = %d height = %d cameraId = %d", index, ratio,
          resolution.first, resolution.second, cameraId);

    return resolution;
}

void CameraManager::dumpPerfInfo(int fd)
{
    mVendorModule->dumpPerfInfo(fd);
}
} // namespace mihal
