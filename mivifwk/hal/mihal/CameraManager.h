#ifndef __CAMERA_MANAGER_H__
#define __CAMERA_MANAGER_H__

#include <hardware/camera_common.h>

#include <memory>
#include <mutex>
#include <tuple>
#include <vector>

#include "CameraDevice.h"
#include "CameraModule.h"

namespace mihal {
class Session;
class CameraManager
{
public:
    static CameraManager *getInstance();
    bool inited() const { return mInit; }
    std::tuple<int, CameraDevice *> open(int id);

    int getNumberOfCameras() const;
    int getCameraInfo(int id, struct camera_info *info) const;
    int setCallbacks(const camera_module_callbacks *callbacks);
    void getVendorTagOps(vendor_tag_ops *ops);
    int setTorchMode(const char *id, bool enabled);
    int turnOnTorchWithStrengthLevel(const char *id, int32_t torchStrength);
    int getTorchStrengthLevel(const char *id, int32_t *strengthLevel);
    int getPhysicalCameraInfo(int id, camera_metadata **metadata) const;
    int isStreamCombinationSupported(int id, const camera_stream_combination *streams) const;
    void notifyDeviceStateChange(uint64_t deviceState);
    int getConcurrentStreamingCameraIds(uint32_t *pConcCamArrayLength,
                                        concurrent_camera_combination **ppConcCamArray);
    int isConcurrentStreamCombinationSupported(uint32_t numCombinations,
                                               const cameraid_stream_combination_t *pStreamComb);
    CameraDevice *getCameraDevice(int id) const;
    std::pair<int, int> getMaxYUVResolution(int32_t cameraId, float ratio, bool isUpscale = false);
    std::pair<int, int> getMaxRawResolution(int32_t cameraId, float ratio, int rFormat);
    int getMockCameraMaxJpegSize() const { return mMockMaxJpegSize; }
    bool isFovCropFeatureSupported(const Metadata &meta);
    void dumpPerfInfo(int fd);

private:
    CameraManager();
    ~CameraManager() = default;

    CameraManager(const CameraManager &other) = delete;
    CameraManager &operator=(const CameraManager &other) = delete;

    bool mInit;

    // NOTE: why we use unique_ptr here?
    // these 2 members keep the modules whole lifetime, and the CameraManager it self
    // will live as long as the provider process. usage of unique_ptr will ease the
    // dctor. but regard to the owner mamangement, there will be some other object that
    // borrow the modules
    std::unique_ptr<CameraModule> mVendorModule;
    std::unique_ptr<CameraModule> mMockModule;
    int mMockMaxJpegSize;
};

} // namespace mihal

#endif // __CAMERA_MANAGER_H__
