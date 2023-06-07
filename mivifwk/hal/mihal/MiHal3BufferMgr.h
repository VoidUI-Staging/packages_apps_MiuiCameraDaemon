#ifndef __MI_HAL3_BUFFER_MGR_H__
#define __MI_HAL3_BUFFER_MGR_H__

#include <hardware/camera3.h>

#include <condition_variable>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "CameraDevice.h"

#define BUFFER_REQUEST_BATCH_NUM 2
#define GET_BUFFER_RETRY_NUM     2

namespace mihal {

// NOTE: MiHal3BufferMgr is used to only manager fwk streams when HAL3 buffer manager is on.
// Reference: vendor/qcom/proprietary/chi-cdk/core/chiframework/chxstreambuffermanager.h
class MiHal3BufferMgr
{
public:
    MiHal3BufferMgr(CameraDevice *camera);
    ~MiHal3BufferMgr();

    // @param:
    // 'targetStream' is the stream you want to get buffer from
    // 'num' is the number of buffers you want to get
    // @return:
    // the returned vector is either be empty or its size() == num, there's no such case where user
    // wants 3 buffers and we only return 2 buffers. This policy just follows Google's logic, see:
    // frameworks/av/services/camera/libcameraservice/device3/Camera3OutputUtils.cpp/requestStreamBuffers
    // Actually, Qcom can handle this case where it wants 3 buffers and we only return 1 or 2
    // buffers, so maybe we can change the code logic in the future
    std::vector<camera3_stream_buffer> getBuffer(camera3_stream *targetStream, uint32_t num);

    void configureStreams(camera3_stream_configuration *stream_list);

    // NOTE: should be called before session's processCaptureRequest. This function will record
    // buffers which have null handle, these buffers need to be getBuffers later
    void processCaptureRequest(const camera3_capture_request *request);

    // NOTE: should be called after we forward result to fwk.
    void processCaptureResult(const camera3_capture_result *result);

    // NOTE: hal3 interface, called from Qcom
    void returnStreamBuffers(std::vector<camera3_stream_buffer> &retBufs);

    // NOTE: when fwk signal stream flush, wait and return all free buffers
    void signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams);

    // NOTE: reset all StreamControlBlock, wait for all
    void waitAndResetToInitStatus();

private:
    enum BufferRequestPolicy {
        // NOTE: normally, for each stream we will request more than one buffers in a buffer request
        // to reduce IPC nums
        BATCH_MODE,
        // NOTE: if we failed to request buffers in BATCH_MODE, then we switch to ONE_BUFFER_MODE.
        // If we succeed to request buffer in ONE_BUFFER_MODE, then we switch back to BATCH_MODE.
        // This is the same policy taken by Qcom
        ONE_BUFFER_MODE
    };

    enum StreamStatus {
        NORMAL,
        // NOTE: if a stream is DRAINED, it means it does not have enough buffer
        DRAINED,
        // NOTE: if app disables this stream (e.g. during flash and close), then the stream becomes
        // BROKEN
        BROKEN
    };

    // NOTE: a struct to record some buffer info of stream, with which we can decide whether we
    // should request more buffers for a stream
    struct StreamControlBlock
    {
        // NOTE: all buffers in captur requests. When buffer manager is on, these buffers handle
        // will be null
        int mBufferNeededNum = 0;
        // NOTE: mBufferIncomingNum is the num of buffers which mBufferRequestThread is requesting
        // but haven't been returned by fwk
        int mBufferIncomingNum = 0;
        int mBufferInUseNum = 0;

        // NOTE: fwk's max num of buffers
        int mMaxBufferNum = 8;

        // NOTE: free buffers requested from fwk
        std::vector<camera3_stream_buffer> mFreeBuffers = {};

        // NOTE: use BATCH_MODE as default to reduce IPC
        BufferRequestPolicy mPolicy = BATCH_MODE;

        // NOTE: buffer requested in BATCH_MODE
        int mBatchNum = BUFFER_REQUEST_BATCH_NUM;

        StreamStatus mStreamStatus = NORMAL;

        std::string toString()
        {
            std::ostringstream str;
            str << "[Hal3BufferMgr]:"
                << "mBufferNeededNum: " << mBufferNeededNum << ", "
                << "mBufferIncomingNum: " << mBufferIncomingNum << ", "
                << "mBufferInUseNum: " << mBufferInUseNum << ", mFreeBuffers: "
                << "size: " << mFreeBuffers.size() << ", "
                << "mPolicy: " << (mPolicy == BATCH_MODE ? "BATCH_MODE" : "ONE_BUFFER_MODE") << ", "
                << "mStreamStatus: " << mStreamStatus;

            return str.str();
        }
    };

    struct StreamBufferRequest
    {
        camera3_stream *mStream;
        int mNumBufferRequested;

        void merge(const StreamBufferRequest &other)
        {
            if (mStream != other.mStream) {
                return;
            }

            mNumBufferRequested += other.mNumBufferRequested;
        }
    };

    // NOTE: wrap camera3_stream_buffer_ret so that C++ STL can help do memory management
    struct StreamBufferReturn
    {
        camera3_stream *mStream;
        camera3_stream_buffer_req_status_t mStatus;
        uint32_t mOutBufNum;
        std::vector<camera3_stream_buffer> mOutputBuffers;
    };

    // NOTE: boot up thread
    void initialize();

    // NOTE: thread loop function to request buffers
    void bufferRequestLoop();

    // @brief: this function generate buffer request and notify dedicated thread to request buffer
    // @param:
    // 'targetStream' is the stream you want to get buffer from
    // 'reserveNum' is the min number of buffers we want to reserve in a stream even though
    // the capture requests may not have such many buffers. If it's 0, then we can request buffers
    // on our need
    void signalToRequestBufferLocked(camera3_stream *targetStream, int reserveNum = 0);

    // NOTE: the actual implementation where we use hidl interface to request buffers from fwk
    camera3_buffer_request_status requestBuffersImpl(
        const std::vector<StreamBufferRequest> &bufReqs, std::vector<StreamBufferReturn> &bufRets);
    int waitAndCloseAcquireFence(camera3_stream_buffer &buffer);
    void processFwkReturnedBuffers(camera3_buffer_request_status status,
                                   const std::vector<StreamBufferRequest> &bufReqs,
                                   const std::vector<StreamBufferReturn> &bufRets);

    // NOTE: the actual implementation where we use hidl interface to return buffers from fwk
    void returnStreamBuffersImpl(std::vector<camera3_stream_buffer> &retBufs);

    // NOTE: reset to init status means that StreamControlBlock returned to init status and all free
    // buffers it retains will be returned and so that this data struct can be safely deleted later
    void waitAndResetToInitStatus(std::vector<const camera3_stream_t *> &streams);

    std::map<const camera3_stream *, StreamControlBlock> mStreamInfos;
    // NOTE: locks to protect mStreamInfos
    std::mutex mStreamInfosLock;
    // NOTE: condition variable used to notify when it comes free buffers
    std::condition_variable mWaitForFreeBufferCond;
    // NOTE: condition variable used to notify when a stream's all capture has been finished
    // processing
    std::condition_variable mWaitAllCaptureFinishedCond;

    // NOTE: mBufferRequestThread is used to request stream buffers
    std::thread mBufferRequestThread;
    bool mExitThread = false;

    // NOTE: mBufferRequestThread contains all buffer requests and buffer request thread will
    // monitor this queue
    std::vector<StreamBufferRequest> mBufferRequestQueue;
    std::mutex mBufReqQueueMutex;
    // NOTE: condition variable used to notify thread that it comes new buffer requests
    std::condition_variable mWaitForBufReqCond;

    // NOTE: the vendor camera this buffer manager is bind to
    CameraDevice *mCamera;
};

} // namespace mihal

#endif