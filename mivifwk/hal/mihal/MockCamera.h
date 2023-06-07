#ifndef __MOCK_CAMERA_H__
#define __MOCK_CAMERA_H__

#include <memory>

#include "CameraDevice.h"
#include "CameraModule.h"
#include "Reprocess.h"

namespace mihal {

class AsyncAlgoSession;
class SessionManager;
class MockCamera : public CameraDevice
{
public:
    MockCamera(int cameraId, const camera_info *info, const Metadata *defaultSettings,
               CameraModule *module);
    ~MockCamera() = default;

    std::string getLogPrefixWithFrame(uint32_t frame)
    {
        std::ostringstream str;
        str << getLogPrefix() << "[mockFrame:" << frame << "]";
        return str.str();
    }

    std::string getName() const override { return "Mock"; }

    int getCameraInfo(camera_info *info) const override;
    int open() override;
    int close() override;
    const camera_metadata *defaultSettings(int) const override;
    int configureStreams(std::shared_ptr<StreamConfig> config) override;
    int processCaptureRequest(std::shared_ptr<CaptureRequest> request) override;
    void processCaptureResult(const CaptureResult *result) override;
    int flush() override;
    int flushPendingRequests(const CaptureResult *result) override;
    void signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams) override;
    bool isMockCamera() const override { return true; }
    void processMockPendingFrame(uint32_t frame) override;

    bool hasResultHijacked(uint32_t frame);

private:
    const camera_info *mInfo;
    const Metadata *mDefaultSettings;
    std::unique_ptr<StreamConfig, std::function<void(StreamConfig *)>> mConfig;
    std::weak_ptr<AsyncAlgoSession> mSession;
    SessionManager *mSessionManager;

    mutable std::recursive_mutex mMutex;
    mutable std::condition_variable mCond;
    struct MockRequestRecord
    {
        MockRequestRecord() = delete;
        MockRequestRecord(std::shared_ptr<CaptureRequest> request);
        int flush();

        std::shared_ptr<CaptureRequest> mMockRequest;
        std::set<camera3_stream *> mPendingStreams;
        uint32_t mPartialResult;
    };

    std::map<uint32_t /* frameNumber */, std::unique_ptr<MockRequestRecord>> mPendingMockRecords;
    int updatePendingMockRecords(std::unique_ptr<MockRequestRecord> &record,
                                 const CaptureResult *result);

    // NOTE: for VTS test where the test script will send many strange stream config
    int checkIsConfigValid(std::shared_ptr<StreamConfig> config);

    void asyncErrorResultHandler();
    void processMockPendingFrameImpl(uint32_t frame);

    uint32_t mFwkStreamMaxBufferQueueSize;

    std::mutex mMapMutex;
    std::map<uint32_t, std::shared_ptr<CaptureResult>> mHijackedResultMap;
    std::atomic<bool> mFlushed;
};

} // namespace mihal

#endif // __MOCK_CAMERA_H__
