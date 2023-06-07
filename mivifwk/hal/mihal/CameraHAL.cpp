#include "AppSnapshotController.h"
#include "BgServiceIntf.h"
#include "CameraManager.h"
#include "LogUtil.h"
#include "MiGrallocMapper.h"
#include "MihalMonitor.h"
#include "MiviSettings.h"

using namespace mihal;

namespace {

CameraManager *cameraManager = nullptr;

int halOpen(const hw_module_t *module, const char *name, hw_device_t **device)
{
    int id = atoi(name);

    MLOGI(LogGroupHAL, "Open camera %s", name);

    int ret;
    CameraDevice *camera;
    std::tie(ret, camera) = cameraManager->open(id);
    if (!camera) {
        MLOGE(LogGroupHAL, "Failed to open camera %s", name);
        return ret;
    }

    // TODO: do not forget here
    *device = camera->getCommonHwDevice();
    (*device)->module = const_cast<hw_module_t *>(module);

    return 0;
}

struct hw_module_methods_t methods = {
    .open = halOpen,
};

int getNumberofCameras()
{
    return cameraManager->getNumberOfCameras();
}

int getCameraInfo(int id, struct camera_info *info)
{
    return cameraManager->getCameraInfo(id, info);
}

int setCallbacks(const camera_module_callbacks *callbacks)
{
    return cameraManager->setCallbacks(callbacks);
}

void getVendorTagOps(vendor_tag_ops *ops)
{
    cameraManager->getVendorTagOps(ops);
}

int setTorchMode(const char *id, bool enabled)
{
    return cameraManager->setTorchMode(id, enabled);
}

int turnOnTorchWithStrengthLevel(const char *id, int32_t torchStrength)
{
    return cameraManager->turnOnTorchWithStrengthLevel(id, torchStrength);
}

int getTorchStrengthLevel(const char *id, int32_t *strengthLevel)
{
    return cameraManager->getTorchStrengthLevel(id, strengthLevel);
}

void dumpPerfInfo(int fd)
{
    cameraManager->dumpPerfInfo(fd);
}

int init()
{
    MiDebugInterface::getInstance();
    RegisterIBGService();

    MiviSettings::init();
    AppSnapshotController::getInstance();
    PsiMemMonitor::getInstance();
    MiGrallocMapper::getInstance();
    property_set("persist.vendor.camera.mialgoengine.openlifo", "1");
    property_set("persist.vendor.camera.mivi.version", "2");
    cameraManager = CameraManager::getInstance();
    if (!cameraManager) {
        MLOGE(LogGroupHAL, "Camera HAL init failed!!");
        return -ENODEV;
    }

    return 0;
}

int getPhysicalCameraInfo(int id, camera_metadata **metadata)
{
    return cameraManager->getPhysicalCameraInfo(id, metadata);
}

int isStreamCombinationSupported(int id, const camera_stream_combination *streams)
{
    return cameraManager->isStreamCombinationSupported(id, streams);
}

void notifyDeviceStateChange(uint64_t deviceState)
{
    cameraManager->notifyDeviceStateChange(deviceState);
}

int getConcurrentStreamingCameraIds(uint32_t *pConcCamArrayLength,
                                    concurrent_camera_combination **ppConcCamArray)
{
    return cameraManager->getConcurrentStreamingCameraIds(pConcCamArrayLength, ppConcCamArray);
}

int isConcurrentStreamCombinationSupported(uint32_t numCombinations,
                                           const cameraid_stream_combination_t *pStreamComb)
{
    return cameraManager->isConcurrentStreamCombinationSupported(numCombinations, pStreamComb);
}

} // namespace

camera_module HAL_MODULE_INFO_SYM = {
    .common =
        {
            .tag = HARDWARE_MODULE_TAG,
            .module_api_version = CAMERA_MODULE_API_VERSION_CURRENT,
            .hal_api_version = HARDWARE_HAL_API_VERSION,
            .id = CAMERA_HARDWARE_MODULE_ID,
            .name = "Xiaomi Camera HAL",
            .author = "Xiaomi Technologies, Inc.",
            .methods = &methods,
            .dso = nullptr,
            .reserved = {},
        },
    .get_number_of_cameras = getNumberofCameras,
    .get_camera_info = getCameraInfo,
    .set_callbacks = setCallbacks,
    .get_vendor_tag_ops = getVendorTagOps,
    .open_legacy = nullptr,
    .set_torch_mode = setTorchMode,
    .turn_on_torch_with_strength_level = turnOnTorchWithStrengthLevel,
    .get_torch_strength_level = getTorchStrengthLevel,
    .init = init,
    .get_physical_camera_info = getPhysicalCameraInfo,
    .is_stream_combination_supported = isStreamCombinationSupported,
    .notify_device_state_change = notifyDeviceStateChange,
    .get_camera_device_version = nullptr,
    .get_concurrent_streaming_camera_ids = getConcurrentStreamingCameraIds,
    .is_concurrent_stream_combination_supported = isConcurrentStreamCombinationSupported,
    .reserved[0] = (void *)dumpPerfInfo,
};
