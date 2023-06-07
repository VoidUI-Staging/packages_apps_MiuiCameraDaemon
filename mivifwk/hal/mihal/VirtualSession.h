#ifndef __VIRTUAL_SESSION_H__
#define __VIRTUAL_SESSION_H__

#include <MiaPostProcIntf.h>

#include "VirtualCamera.h"

namespace mihal {

using namespace mialgo2;

class VirtualSession : public MiaPostProcSessionCb, public MiaFrameCb
{
public:
    enum ErrorState {
        ReprocessSuccess,
        ReprocessError,
    };

    VirtualSession(VirtualCamera *camera, const StreamConfig *config);
    ~VirtualSession();
    int processRequest(std::shared_ptr<CaptureRequest> request);
    int flush();

    void onSessionCallback(MiaResultCbType type, uint32_t frame, int64_t timeStamp,
                           void *msgData) override;
    void release(uint32_t frame, int64_t timeStampUs, MiaFrameType type, uint32_t streamIdx,
                 buffer_handle_t handle) override;
    int getBuffer(int64_t timeStampUs, OutSurfaceType type, buffer_handle_t *buffer) override;

    struct RequestRecord
    {
        RequestRecord(std::shared_ptr<CaptureRequest> request, int64_t timestamp);
        std::shared_ptr<CaptureRequest> mRequest;
        std::set<camera3_stream *> mMialgoPendingStreams;
        bool mMetaReturn;
        bool mShutterReturn;
        int64_t mTimestamp;
    };

private:
    int initialize(const StreamConfig *config);
    int destroy();
    void convert2MiaImage(MiaFrame &image, const StreamBuffer *streamBuffer);
    void updateRequestSettings(std::shared_ptr<CaptureRequest> request);
    void decideMialgoPort(std::shared_ptr<CaptureRequest> request);

    void *mPostProc;
    VirtualCamera *mCamera;
    std::map<uint32_t /* frame_number */, std::unique_ptr<RequestRecord>> mPendingRequests;
    std::mutex mMutex;
    uint8_t mHeicSnapshot;
    float mHeicRatio;

    std::atomic_flag mFlushFlag = ATOMIC_FLAG_INIT;
};

} // namespace mihal
#endif
