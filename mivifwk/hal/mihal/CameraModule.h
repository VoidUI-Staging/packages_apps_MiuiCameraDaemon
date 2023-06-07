#ifndef __CAMERA_MODULE_H__
#define __CAMERA_MODULE_H__

#include <errno.h>
#include <hardware/camera3.h>

#include <set>
#include <string>

#include "CameraDevice.h"
#include "LogUtil.h"
#include "Metadata.h"

namespace mihal {

// It's a wrapper class interface to provider the same api to the upper camera manager
// to hide the difference among the Vendor/Mock/PostProc camera devices
class CameraModule
{
public:
    CameraModule(int startId = 0) : mStartId{startId} {}
    virtual ~CameraModule() = default;

    static bool isLogicalMultiCamera(const Metadata &info,
                                     std::set<std::string> *physicalCameraIds);

    int getNumberOfCameras(void) const { return mCameras.size(); }
    int getCameraInfo(int cameraId, camera_info *info);
    virtual int getPhysicalCameraInfo(int physicalCameraId, const Metadata **info)
    {
        return -ENOEXEC;
    }
    // int getDeviceVersion(int cameraId) const;
    virtual int open(const char *id, camera3_device **device) = 0;
    virtual int setCallbacks(const camera_module_callbacks *callbacks) = 0;
    virtual void getVendorTagOps(vendor_tag_ops *ops) = 0;
    virtual bool isSetTorchModeSupported() const { return false; }
    virtual int setTorchMode(const char *cameraId, bool enable) { return -ENOEXEC; }
    virtual int turnOnTorchWithStrengthLevel(const char *cameraId, int32_t torchStrength)
    {
        return -ENOEXEC;
    }
    virtual int getTorchStrengthLevel(const char *cameraId, int32_t *strengthLevel)
    {
        return -ENOEXEC;
    }
    virtual uint16_t getModuleApiVersion() const = 0;
    virtual const char *getModuleName() const = 0;
    virtual uint16_t getHalApiVersion() const = 0;
    virtual const char *getModuleAuthor() const = 0;
    virtual void notifyDeviceStateChange(uint64_t deviceState) = 0;
    virtual int isStreamCombinationSupported(int cameraId,
                                             const camera_stream_combination *streams) const
    {
        return false;
    }

    virtual int getConcurrentStreamingCameraIds(uint32_t *pConcCamArrayLength,
                                                concurrent_camera_combination **ppConcCamArray)
    {
        return -EINVAL;
    }

    virtual int isConcurrentStreamCombinationSupported(
        uint32_t numCombinations, const cameraid_stream_combination_t *pStreamComb)
    {
        return -EINVAL;
    }
    // virtual void notifyDeviceStateChange(uint64_t deviceState);

    virtual void dumpPerfInfo(int fd) {}
    CameraDevice *getCameraDevice(int id);

protected:
    const int mStartId;

    // mCameras stores all the cameras and the corresponding ID
    // NOTE: reason why we use unique_ptr to manage the lifecyle:
    // CameraModule hold the lifecyle of all the CameraDevice and lives as long as
    // provider manager, using unique_ptr just to easy the dctor
    std::vector<std::unique_ptr<CameraDevice>> mCameras;
};

} // namespace mihal

#endif // __CAMERA_MODULE_H__
