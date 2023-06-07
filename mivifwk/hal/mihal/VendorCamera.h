#ifndef __VENDOR_CAMERA_H__
#define __VENDOR_CAMERA_H__

#include <map>
#include <memory>

#include "CameraDevice.h"
#include "CameraModule.h"
#include "SdkUtils.h"

namespace mihal {

class VendorCamera : public CameraDevice
{
public:
    struct CallbackTransfer : public camera3_callback_ops
    {
        camera3_callback_ops *pSessionCallbacks;
        CameraDevice *pVendorCamera;
    };

    VendorCamera(int cameraId, const camera_info &info, CameraModule *module);
    ~VendorCamera() = default;

    int getCameraInfo(camera_info *info) const override;
    int setTorchMode(bool enabled) override;
    int turnOnTorchWithStrengthLevel(int32_t torchStrength) override;
    int getTorchStrengthLevel(int32_t *strengthLevel) override;
    int open() override;
    int close() override;
    int configureStreams(std::shared_ptr<StreamConfig> config) override;
    const camera_metadata *defaultSettings(int type) const override;
    int processCaptureRequest(std::shared_ptr<CaptureRequest> request) override;
    void dump(int fd) override;
    int flush() override;
    void signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams) override;
    void processCaptureResult(const CaptureResult *result) override;

    // NOTE: If there are still streambuffers in internal stream after vendorCamera flush, it needs
    // to signalStreamFlush
    void flushRemainingInternalStreams();

    virtual int initialize2Vendor(const camera3_callback_ops *callbacks);
    virtual int configureStreams2Vendor(StreamConfig *config);
    virtual int submitRequest2Vendor(CaptureRequest *request);
    virtual int flush2Vendor();
    virtual void signalStreamFlush2Vendor(uint32_t numStream,
                                          const camera3_stream_t *const *streams);

    MiHal3BufferMgr *getFwkStreamHal3BufMgr() override
    {
        MASSERT(mFwkHal3BufMgr);
        return mFwkHal3BufMgr.get();
    }

    std::string getLogPrefixWithFrame(uint32_t frame)
    {
        std::ostringstream str;
        str << getLogPrefix() << "[fwkFrame:" << frame << "]";
        return str.str();
    }

    std::string getName() const override { return "Vendor"; }

protected:
    CameraModule *mModule;

private:
    // NOTE: TODO: there are a lot of stuffs when determining the type of the created sessions:
    //      1. there are difference between sessions for MIUIApp and SDK/ThirdParty apps
    //      2. there are difference between sessions for individual hardware cameras: SAT
    //         Camera should creat 3/4 internal streams while Front Camera just need 2, etc.
    //         so shall we keep a cameraID to internal StreamConfigs mapping table to handle
    //         this difference? or we create sessions by the hardware camera's static info?
    //      3. for the above reasons, let's create base and derived Session classes to address it?
    int createSession(std::shared_ptr<StreamConfig> config);

    bool isAsyncAlgoStreamConfigMode(const StreamConfig *config);
    bool isSyncAlgoStreamConfigMode(StreamConfig *config,
                                    std::shared_ptr<EcologyAdapter> &ecoAdapter);

    int prepareReConfig(uint32_t &oldSessionFrameNum);
    int setupCallback2Vendor();
    int updateCallback2Vendor();
    int updateSessionData(uint32_t oldSessionFrameNum);
    int processConfig();
    int postConfig();

    const camera_info mRawInfo;
    std::unique_ptr<Metadata> mOverrideInfo;

    // camera3 device linking mihal and vendor hal(qcom/mtk/...)
    camera3_device *mDeviceImpl;

    std::shared_ptr<Session> mSession;
    std::shared_ptr<Session> mLastSession;

    // NOTE: transfer vendor callback to session, now we don't pass session callback to vendor
    // because we want to handle reconfig streams
    CallbackTransfer mCallbackTransfer;

    // NOTE: now configureStreams and signalStreamFlush maybe executed in a parallel manner by
    // Android, so we need to lock to pretend such thing from happening
    std::mutex mOpsLock;

    // NOTE: fwk streams hal3 buffer manager
    std::shared_ptr<MiHal3BufferMgr> mFwkHal3BufMgr;

    std::atomic<bool> mFlushed;
};

} // namespace mihal

#endif // __VENDOR_CAMERA_H__
