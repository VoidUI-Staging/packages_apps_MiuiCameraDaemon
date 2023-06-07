#ifndef __VIRTUAL_CAMERA_H__
#define __VIRTUAL_CAMERA_H__

#include <memory>

#include "CameraDevice.h"
#include "CameraModule.h"

namespace mihal {

class VirtualSession;
class VirtualCamera : public CameraDevice
{
public:
    VirtualCamera(int cameraId, const camera_info &info);
    ~VirtualCamera();

    std::string getLogPrefixWithFrame(uint32_t frame)
    {
        std::ostringstream str;
        str << getLogPrefix() << "[virtualFrame:" << frame << "]";
        return str.str();
    }

    std::string getName() const override { return "Virtual"; }
    int getCameraInfo(camera_info *info) const override;
    int open() override;
    int close() override;
    const camera_metadata *defaultSettings(int) const override;
    int configureStreams(std::shared_ptr<StreamConfig> config) override;
    int processCaptureRequest(std::shared_ptr<CaptureRequest> request) override;
    void processCaptureResult(const CaptureResult *result) override;
    void notify(const camera3_notify_msg *msg) override;
    int flush() override;
    int flushPendingRequests(const CaptureResult *result) override;
    void signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams) override;

    int getVTCameraMaxJpegSize() const { return mMaxJpegSize; }

private:
    void processConfig();

    const camera_info mRawInfo;
    std::unique_ptr<Metadata> mOverrideInfo;
    Metadata mDefaultSettings;
    std::unique_ptr<StreamConfig, std::function<void(StreamConfig *)>> mConfig;
    std::unique_ptr<VirtualSession> mSession;
    std::atomic<bool> mFlush;

    mutable std::recursive_mutex mMutex;
    struct VirtualRequestRecord
    {
        VirtualRequestRecord() = delete;
        VirtualRequestRecord(std::shared_ptr<CaptureRequest> request);
        int flush();

        std::shared_ptr<CaptureRequest> mVirtualRequest;
        std::set<camera3_stream *> mPendingStreams;
        uint32_t mPartialResult;
    };

    std::map<uint32_t /* frameNumber */, std::unique_ptr<VirtualRequestRecord>>
        mPendingVirtualRecords;
    int updatePendingVirtualRecords(const CaptureResult *result);

    // NOTE: for VTS test where the test script will send many strange stream config
    int checkIsConfigValid(std::shared_ptr<StreamConfig> config);

    void initDefaultSettings();

    // NOTE: fwk streams hal3 buffer manager
    std::shared_ptr<MiHal3BufferMgr> mFwkHal3BufMgr;
    int32_t mMaxJpegSize;
};

} // namespace mihal

#endif // __VIRTUAL_CAMERA_H__