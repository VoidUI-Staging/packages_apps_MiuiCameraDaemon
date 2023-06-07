/* Copyright

*/

#ifndef _MI_BUFFER_MANAGER_H_
#define _MI_BUFFER_MANAGER_H_

#include <functional>
//#include <unordered_map>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "MiAlgoCamMode.h"
#include "MiCamDevCallback.h"
#include "MiDebugUtils.h"
#include "MiImageBuffer.h"
#include "decoupleutil/MiZoneTypes.h"

#define STREAM_USED_BY_CUSTOM  (1 << 0)
#define STREAM_USED_BY_HAL     (1 << 1)
#define STREAM_OWNED_FRAMEWORK ((1 << 0) << 16)
#define STREAM_OWNED_VENDOR    ((1 << 1) << 16)

#define STREAM_ID_VENDOR_START (20011)

namespace mizone {

using requestStreamBuffers_cb =
    std::function<void(BufferRequestStatus status, const std::vector<StreamBufferRet> &bufferRets)>;

enum HBMStrategy { COMPATIBLE, IMMEDIATELY, CACHED, INVALID };

class MiBufferManager
{
public:
    // map<streamId, MemUsed>
    typedef std::map<int32_t, int32_t> MemoryInfoMap;
    using MemoryChangedCallback = std::function<void(MemoryInfoMap &)>;
    struct BufferMgrCreateInfo
    {
        StreamConfiguration fwkConfig;
        StreamConfiguration vendorConfig;
        // CameraInfo            camInfo;
        bool supportCoreHBM = false;
        bool supportAndroidHBM = false;
        MiAlgoCamMode *camMode;
        MemoryChangedCallback callback;
        std::shared_ptr<MiCamDevCallback> devCallback;
    };
    struct MiBufferRequestParam
    {
        int32_t streamId;
        int32_t bufferNum;
    };

    static std::shared_ptr<MiBufferManager> create(const BufferMgrCreateInfo &info);
    explicit MiBufferManager(const BufferMgrCreateInfo &info);
    virtual ~MiBufferManager();

    // Interfaces of alloc/free buffers for users(but not really allocate or free)
    // 1. CaptureRequest with framework buffers, we will convert the buffers to MiImageBuffer and
    // return.
    // 2. list of requestParam, we will "allocate" buffers internal and return.
    // NOTEs: 1. will get image info such as format and size from stream info saved at create stage.
    //       2. always ion buffer //TODO
    // end

    bool requestBuffers(const std::vector<MiBufferRequestParam> &requestParam,
                        std::map<int32_t, std::vector<StreamBuffer>> &streamBufferMap);
    void releaseBuffers(std::vector<StreamBuffer> &streamBuffers);

    // Android Buffer Management API impl
    void requestStreamBuffers(const std::vector<BufferRequest> &bufReqs,
                              requestStreamBuffers_cb cb);
    void returnStreamBuffers(const std::vector<StreamBuffer> &buffers);
    void signalStreamFlush(const std::vector<int32_t> &streamIds, uint32_t streamConfigCounter);

    // after switchToOffline, need requestStreamBuffer from new callback
    void pauseCallback();
    void setCallback(std::shared_ptr<MiCamDevCallback> deviceCallback);

    // to query memory used of specified stream
    uint32_t queryMemoryUsedByStream(int32_t streamId);

private:
    struct StreamInfo
    {
        Stream stream;
        int32_t streamId;
        uint32_t streamProp;
        int32_t capacity = 0;
        HBMStrategy strategy;
    };
    struct BufferItem
    {
        StreamBuffer buffer;
        uint32_t requestNumber;
    };

    void threadLoop();

    // internal used utils
    virtual bool registerFwkBuffers(const std::vector<MiBufferRequestParam> &requestParam,
                                    std::map<int32_t, std::vector<StreamBuffer>> &streamBufferMap);
    virtual bool registerVndBuffers(const std::vector<MiBufferRequestParam> &requestParam,
                                    std::map<int32_t, std::vector<StreamBuffer>> &streamBufferMap);
    virtual bool allocateVndBuffer(int32_t streamId, StreamBuffer &streamBuffer);
    virtual BufferRequestStatus requestFwkStreamBuffers(std::vector<BufferRequest> &bufReqs,
                                                        std::vector<StreamBufferRet> &bufferRets);

    virtual void onMemoryChanged();
    virtual bool isCapcityExceed(int32_t streamId);
    virtual void moveToIdle(StreamBuffer &streamBuffer);
    virtual void moveToBusy(StreamBuffer &streamBuffer);
    bool getBufferFromIdle(int32_t streamId, StreamBuffer &streamBuffer);

    virtual void buildStreamInfo(const Stream &);
    virtual uint32_t getStreamProp(const Stream &stream);
    virtual bool isPreviewStream(const Stream &stream);

    bool isQuickViewCacheStream(const Stream &stream);
    std::pair<bool, StreamBuffer> findOutputBufferByStreamId(const CaptureRequest &request,
                                                             const int32_t &streamId);
    uint32_t getTotalMemoryUsedByStream(int32_t streamId);

    bool waitForCallback(std::unique_lock<std::mutex> &lock);

    MiAlgoCamMode *mpCamMode;
    // for calling framework callback
    std::shared_ptr<MiCamDevCallback> mDeviceCallback; // TODO
    //<streamId, StreamInfo>
    std::map<int32_t, StreamInfo> mStreamInfoMap;

    //<<streamId,
    //    <bufferId, BufferItem>>
    std::map<uint32_t, std::map<uint64_t, BufferItem>> mBusyBufferMap;
    std::map<uint32_t, std::map<uint64_t, BufferItem>> mIdleBufferMap;
    std::mutex mMapMutex;

    bool mSupportCoreHBM = false;
    bool mSupportAndroidHBM = false;
    int64_t mTotalUsed = 0; // in byte
    // should release some idle buffers if reach this threshold
    int64_t mThreshold;
    MemoryInfoMap mMemInfo;
    MemoryChangedCallback statusCallback = nullptr;
    // std::unique_ptr<MiMemoryMonitor> mHalMemMonitor;

    std::thread mThread;
    std::atomic<int> mThreadLoopStatus = 0;
    std::mutex mMutex;

    bool mPauseCallback = false;
    std::mutex mPauseCallbackMutex;
    std::condition_variable mPauseCallbackCond;
};

} // namespace mizone
#endif //_MI_BUFFER_MANAGER_H_
