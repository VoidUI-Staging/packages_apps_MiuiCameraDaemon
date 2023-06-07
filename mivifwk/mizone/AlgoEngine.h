#ifndef _MIZONE_ALGOENGINE_H_
#define _MIZONE_ALGOENGINE_H_

#include <map>
#include <set>

#include "DecoupleUtil.h"
#include "MiImageBuffer.h"
#include "MiZoneTypes.h"

namespace mizone {

class MiAlgoCamMode;
class MiBufferManager;

class PostProcCallback;
class BufferCallback;

class AlgoEngine
{
public:
    struct CreateInfo
    {
        MiAlgoCamMode *mode;
        CameraInfo cameraInfo;
    };

    enum PortsRule { Normal, MfnrHdr, BokehHdr, HdrSr };

    static std::unique_ptr<AlgoEngine> create(const CreateInfo &info);

    ~AlgoEngine();
    AlgoEngine(const AlgoEngine &) = delete;
    AlgoEngine &operator=(const AlgoEngine &) = delete;

    bool isConfigCompleted() { return mConfigCompleted; }
    bool configure(std::shared_ptr<MiConfig> fwkConfig, std::shared_ptr<MiConfig> vndConfig);
    int process(const CaptureRequest &fwkReq,
                const std::map<uint32_t, std::shared_ptr<MiRequest>> &requests);

    void onMetedataRecieved(uint32_t fwkFrameNum, const std::string &exifInfo);

    bool getOutputBuffer(int64_t timestamp, int32_t portId, buffer_handle_t &bufferHandle);
    void releaseOutputBuffer(uint32_t fwkFrameNum, int64_t timeStamp, uint32_t portId);
    void releaseInputBuffer(uint32_t fwkFrameNum, int64_t timeStamp, uint32_t portId);

    // NOTE: if HBM enabled, HAL should request all buffers needed through requestStreamBuffers
    //       during switchToOffline, this interface just signal AlgoEngine to request buffers.
    void switchToOffline();

    int flush(bool isForced = true);
    int quickFinish(std::string pipelineName);

private:
    struct InflightRequest
    {
        CaptureRequest fwkReq;
        int64_t timestamp;
        std::map<uint32_t, std::shared_ptr<MiRequest>> requests;
        std::string exifInfo;
        //      < portId,        vec<timestamp, buffer>  >
        std::map<uint32_t, std::vector<std::pair<int64_t, StreamBuffer>>> inputBuffers;
        std::map<uint32_t, std::map<int64_t, StreamBuffer>> outputBuffers;
        //      < timestamp,  miRequest>
        std::map<int64_t, std::shared_ptr<MiRequest>> tsMapReqs;
    };

    AlgoEngine(const CreateInfo &info);

    std::shared_ptr<InflightRequest> getInflightRequestLocked(uint32_t fwkFrameNum);

    int prepareInflight(const CaptureRequest &fwkReq,
                        const std::map<uint32_t, std::shared_ptr<MiRequest>> &requests,
                        PortsRule portsRule, std::shared_ptr<InflightRequest> inflightRequest);

    int buildOutputSessionParams(CaptureRequest &fwkRequest, StreamBuffer buffer,
                                 int32_t algoPortId, MiaProcessParams &sessionParams /* output */);
    int buildInputSessionParams(const CaptureRequest &fwkRequest,
                                const std::shared_ptr<MiRequest> &miReq, const StreamBuffer &buffer,
                                int32_t algoPortId, MiaProcessParams &sessionParams /* output */);

    uint32_t getOperationMode(const uint32_t cameraMode);
    uint32_t getCameraMode();

    MiAlgoCamMode *mMode;
    CameraInfo mCameraInfo;
    std::shared_ptr<MiConfig> mFwkConfig;
    std::shared_ptr<MiConfig> mVndConfig;

    // mialgoengine session handle
    void *mAlgoEngineSession;

    MiMetadata mSessionParams;
    // std::vector<uint32_t> mVenFrameNums;
    std::shared_ptr<MiBufferManager> mBufferManager;
    //      <fwkFrameNum, InFlightRequest>
    std::map<uint32_t, std::shared_ptr<InflightRequest>> mInflightRequests;
    std::mutex mInflightRequestsMutex;

    std::unique_ptr<PostProcCallback> mPostProcCallback;
    std::unique_ptr<BufferCallback> mBufferCallback;
    CaptureRequest mFwkReq;
    bool mConfigCompleted;
};

} // namespace mizone

#endif // _MIZONE_ALGOENGINE_H_
