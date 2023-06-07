#ifndef __VENDOR_MODULE_H__
#define __VENDOR_MODULE_H__

#include <map>

#include "CameraModule.h"

namespace mihal {

class VendorModule : public CameraModule
{
public:
    VendorModule(camera_module *module, int startId);
    ~VendorModule();

    int getPhysicalCameraInfo(int physicalId, const Metadata **info) override;
    int open(const char *id, camera3_device **device) override;
    int setCallbacks(const camera_module_callbacks *callbacks) override;
    void getVendorTagOps(vendor_tag_ops *ops) override;
    bool isSetTorchModeSupported() const override;
    int setTorchMode(const char *cameraId, bool enable) override;
    int turnOnTorchWithStrengthLevel(const char *cameraId, int32_t torchStrength) override;
    int getTorchStrengthLevel(const char *cameraId, int32_t *strengthLevel) override;
    uint16_t getModuleApiVersion() const override { return mModuleImpl->common.module_api_version; }
    const char *getModuleName() const override { return mModuleImpl->common.name; }
    uint16_t getHalApiVersion() const override { return mModuleImpl->common.hal_api_version; }
    const char *getModuleAuthor() const override { return mModuleImpl->common.author; }
    int isStreamCombinationSupported(int cameraId,
                                     const camera_stream_combination *streams) const override;
    void notifyDeviceStateChange(uint64_t deviceState) override;
    int getConcurrentStreamingCameraIds(uint32_t *pConcCamArrayLength,
                                        concurrent_camera_combination **ppConcCamArray) override;
    int isConcurrentStreamCombinationSupported(
        uint32_t numCombinations, const cameraid_stream_combination_t *pStreamComb) override;
    void dumpPerfInfo(int fd);

private:
    int init();

    std::map<int, Metadata> mPhysicalMetas;
    camera_module *mModuleImpl;
};

} // namespace mihal

#endif // __VENDOR_MODULE_H__
