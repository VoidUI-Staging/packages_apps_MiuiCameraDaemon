#include "MiHal3BufferMgr.h"

#include <sync/sync.h>

#include <algorithm>
#include <chrono>
#include <set>

#include "LogUtil.h"

namespace mihal {

using namespace std::chrono_literals;

MiHal3BufferMgr::MiHal3BufferMgr(CameraDevice *camera) : mCamera{camera}
{
    initialize();
}

MiHal3BufferMgr::~MiHal3BufferMgr()
{
    waitAndResetToInitStatus();

    {
        std::lock_guard<std::mutex> lg(mBufReqQueueMutex);
        mExitThread = true;
    }
    mWaitForBufReqCond.notify_all();

    if (mBufferRequestThread.joinable()) {
        mBufferRequestThread.join();
    }
}

void MiHal3BufferMgr::initialize()
{
    mBufferRequestThread = std::thread([this]() { bufferRequestLoop(); });
}

void MiHal3BufferMgr::configureStreams(camera3_stream_configuration *stream_list)
{
    MLOGD(LogGroupHAL, "[Hal3BufferMgr]: configureStreams E");

    // NOTE: waitAndResetToInitStatus to handle reconfig situation, but maybe we do not worry about
    // reconfig since camera service will promise that no capture request is being processing when
    // configuring streams, but i think we'd better double check it
    waitAndResetToInitStatus();

    // NOTE: register new stream infos
    std::lock_guard<std::mutex> lg(mStreamInfosLock);
    mStreamInfos.clear();
    for (uint32_t i = 0; i < stream_list->num_streams; ++i) {
        auto &stream = stream_list->streams[i];

        MASSERT(stream->max_buffers != 0);
        StreamControlBlock temp{};
        temp.mMaxBufferNum = stream->max_buffers;

        mStreamInfos.insert(std::pair{stream, temp});
        MLOGI(LogGroupHAL,
              "[Hal3BufferMgr]: register stream: %p to buffer mgr, detail: stream_type: %d, width: "
              "%u, height: %u, max_buffers: %u",
              stream, stream->stream_type, stream->width, stream->height, stream->max_buffers);
    }

    MLOGD(LogGroupHAL, "[Hal3BufferMgr]: configureStreams X");
}

void MiHal3BufferMgr::waitAndResetToInitStatus()
{
    std::unique_lock<std::mutex> lg(mStreamInfosLock);
    if (mStreamInfos.empty()) {
        return;
    }

    std::vector<const camera3_stream_t *> streams;
    for (auto &entry : mStreamInfos) {
        streams.push_back(entry.first);
    }

    lg.unlock();
    waitAndResetToInitStatus(streams);
}

void MiHal3BufferMgr::waitAndResetToInitStatus(std::vector<const camera3_stream_t *> &streams)
{
    std::unique_lock<std::mutex> lg(mStreamInfosLock);

    // It will directly return, if streams is empty.
    if (streams.empty()) {
        return;
    }

    std::vector<camera3_stream_buffer> freeBufs;
    for (auto &stream : streams) {
        if (!mStreamInfos.count(stream)) {
            MLOGE(LogGroupHAL, "[Hal3BufferMgr]: invalid stream");
            continue;
        }

        auto &info = mStreamInfos[stream];
        MLOGI(LogGroupHAL, "[Hal3BufferMgr]: start drain stream:%p, before drain, status:%s",
              stream, info.toString().c_str());

        // NOTE: wait all capture requests finish, so that we can make sure vendor will no longer
        // invoke getBuffer and buffer request thread has no new task
        auto ret = mWaitAllCaptureFinishedCond.wait_for(
            lg, 500ms, [&]() { return info.mBufferNeededNum <= 0; });
        if (!ret) {
            MLOGE(
                LogGroupHAL,
                "[Hal3BufferMgr]: FAIL to wait all capture finished in stream: %p, its status: %s",
                stream, info.toString().c_str());
        }

        // NOTE: wait all buffer requests related to this stream finish, so that we can make sure no
        // new free buffer will come
        ret = mWaitForFreeBufferCond.wait_for(lg, 500ms,
                                              [&]() { return info.mBufferIncomingNum <= 0; });
        if (!ret) {
            MLOGE(LogGroupHAL,
                  "[Hal3BufferMgr]: FAIL to wait all buffer requests finished in stream: %p, its "
                  "status: %s",
                  stream, info.toString().c_str());
        }

        // NOTE: collect all remaining free buffer and return them later
        MLOGI(LogGroupHAL, "[Hal3BufferMgr]: stream:%p has %zu buffers to drain", stream,
              info.mFreeBuffers.size());
        freeBufs.insert(freeBufs.end(), info.mFreeBuffers.begin(), info.mFreeBuffers.end());
        info.mFreeBuffers.clear();
    }

    lg.unlock();

    // If freeBufs is empty, return
    if (freeBufs.empty()) {
        return;
    }
    // NOTE: return free buffers to fwk
    returnStreamBuffersImpl(freeBufs);
}

void MiHal3BufferMgr::processCaptureRequest(const camera3_capture_request *request)
{
    MLOGD(LogGroupRT, "[Hal3BufferMgr]: processCaptureRequest E");

    // NOTE: bypass when hal3 buffer manager is off
    if (!mCamera->supportBufferManagerAPI()) {
        return;
    }

    // NOTE: record buffers
    std::lock_guard<std::mutex> lg(mStreamInfosLock);
    for (uint32_t i = 0; i < request->num_output_buffers; ++i) {
        auto &buffer = request->output_buffers[i];
        auto &stream = buffer.stream;
        if (stream == nullptr || !mStreamInfos.count(stream)) {
            MLOGE(LogGroupHAL, "[Hal3BufferMgr]: invalid capture request buffer stream");
            continue;
        }

        auto &info = mStreamInfos[stream];
        // NOTE: record all buffers in this capture request, no matter whether this buffer has
        // handle. Normally, when buffer manager is on, all buffers' handle should be null
        info.mBufferNeededNum += 1;
        // NOTE: if some buffers in this capture request have handle, then these buffers can be
        // treated as in-used buffer
        if (buffer.buffer != nullptr && *buffer.buffer != nullptr) {
            info.mBufferInUseNum += 1;
        }
        MLOGD(LogGroupRT, "[Hal3BufferMgr]: record buffer to stream: %p, status: %s", stream,
              info.toString().c_str());
    }

    // NOTE: no need to process input buffer since we only care about algoSession senario
    // where input buffer is always null

    MLOGD(LogGroupRT, "[Hal3BufferMgr]: processCaptureRequest X");
}

void MiHal3BufferMgr::processCaptureResult(const camera3_capture_result *result)
{
    MLOGD(LogGroupRT, "[Hal3BufferMgr]: processCaptureResult E");

    if (!mCamera->supportBufferManagerAPI()) {
        return;
    }

    // NOTE: just update some records, no need to process input buffer
    std::unique_lock<std::mutex> lg(mStreamInfosLock);
    for (uint32_t i = 0; i < result->num_output_buffers; ++i) {
        auto &buffer = result->output_buffers[i];
        if (buffer.stream == nullptr || !mStreamInfos.count(buffer.stream)) {
            MLOGE(LogGroupHAL, "[Hal3BufferMgr]: invalid buffer");
            continue;
        }

        auto &info = mStreamInfos[buffer.stream];
        // NOTE: we need to update mBufferNeededNum no matter whether this buffer has handle or not,
        // this matches processCaptureRequest
        info.mBufferNeededNum--;
        if (info.mBufferNeededNum < 0) {
            MLOGE(LogGroupHAL,
                  "[Hal3BufferMgr]: info.mBufferNeededNum < 0!! Abnormally, stream: %p, maybe a "
                  "preview request sneaks in when flush, force to set is as 0",
                  buffer.stream);
            info.mBufferNeededNum = 0;
        }
        if (buffer.buffer != nullptr && *buffer.buffer != nullptr) {
            if (info.mBufferInUseNum > 0) {
                info.mBufferInUseNum--;
            }
        }
        MLOGD(LogGroupRT, "[Hal3BufferMgr]: stream: %p buffer come back, after process, status: %s",
              buffer.stream, info.toString().c_str());

        if (info.mBufferNeededNum == 0) {
            mWaitAllCaptureFinishedCond.notify_all();
        }
    }

    MLOGD(LogGroupRT, "[Hal3BufferMgr]: processCaptureResult X");
}

void MiHal3BufferMgr::signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams)
{
    MLOGD(LogGroupHAL, "[Hal3BufferMgr]: signalStreamFlush E");

    std::vector<const camera3_stream_t *> targetStreams;
    for (uint32_t i = 0; i < numStream; ++i) {
        // It will continue, if the i-th stream is not in streams.
        if (!mStreamInfos.count(streams[i])) {
            continue;
        }
        targetStreams.push_back(streams[i]);
    }
    waitAndResetToInitStatus(targetStreams);

    MLOGD(LogGroupHAL, "[Hal3BufferMgr]: signalStreamFlush X");
}

void MiHal3BufferMgr::returnStreamBuffers(std::vector<camera3_stream_buffer> &retBufs)
{
    MLOGD(LogGroupRT, "[Hal3BufferMgr]: returnStreamBuffers E");

    // NOTE: first update some records
    {
        std::lock_guard<std::mutex> lg(mStreamInfosLock);
        for (auto &buffer : retBufs) {
            if (buffer.stream == nullptr || !mStreamInfos.count(buffer.stream)) {
                MLOGE(LogGroupHAL, "[Hal3BufferMgr]: invalid buffer");
                continue;
            }

            auto &info = mStreamInfos[buffer.stream];
            if (buffer.buffer != nullptr && *buffer.buffer != nullptr) {
                if (info.mBufferInUseNum > 0) {
                    info.mBufferInUseNum--;
                }
            } else {
                MLOGE(LogGroupHAL,
                      "[Hal3BufferMgr]: vendor returned buffer with null handle!! Abnormally");
                MASSERT(false);
            }
            MLOGD(LogGroupHAL,
                  "[Hal3BufferMgr][RT]: stream: %p buffer returned, after process, status: %s",
                  buffer.stream, info.toString().c_str());
        }
    }

    // NOTE: then return buffers to fwk
    returnStreamBuffersImpl(retBufs);

    MLOGD(LogGroupRT, "[Hal3BufferMgr]: returnStreamBuffers X");
}

void MiHal3BufferMgr::returnStreamBuffersImpl(std::vector<camera3_stream_buffer> &retBufs)
{
    // NOTE: filter invalid argument if any
    auto itr = std::remove_if(retBufs.begin(), retBufs.end(), [this](camera3_stream_buffer &buf) {
        return buf.stream == nullptr || !mStreamInfos.count(buf.stream);
    });
    retBufs.erase(itr, retBufs.end());

    uint32_t size = retBufs.size();
    if (!size) {
        return;
    }

    camera3_stream_buffer const **buffers = new const camera3_stream_buffer *[size];
    for (int i = 0; i < size; i++) {
        buffers[i] = &retBufs[i];
    }

    mCamera->returnStreamBuffers(size, buffers);

    delete[] buffers;
}

std::vector<camera3_stream_buffer> MiHal3BufferMgr::getBuffer(camera3_stream *targetStream,
                                                              uint32_t num)
{
    std::vector<camera3_stream_buffer> rets;

    std::unique_lock<std::mutex> lg(mStreamInfosLock);
    if (!mStreamInfos.count(targetStream)) {
        MLOGE(LogGroupHAL, "[Hal3BufferMgr]: unknown stream in getBuffer");
        return rets;
    }

    auto &info = mStreamInfos[targetStream];

    // NOTE: util function to move buffers from mFreeBuffers
    auto getBufferFromFreeList = [&](std::vector<camera3_stream_buffer> &out) {
        // NOTE: move num buffers from mFreeBuffers to out
        auto start = info.mFreeBuffers.end() - num;
        auto end = info.mFreeBuffers.end();
        std::move(start, end, std::back_inserter(out));
        info.mFreeBuffers.erase(start, end);

        info.mBufferInUseNum += num;
        MLOGD(LogGroupHAL, "[Hal3BufferMgr]: getBuffer from stream:%p, after get, status:%s",
              targetStream, info.toString().c_str());

        // NOTE: request more buffer asynchronously so that we can handle next getBuffer
        MLOGD(LogGroupHAL, "[Hal3BufferMgr]: getBuffer from stream:%p, after get, request more",
              targetStream);
        signalToRequestBufferLocked(targetStream);
    };

    // NOTE: if we have enough buffers, then we just return them
    if (info.mFreeBuffers.size() >= num) {
        getBufferFromFreeList(rets);
        return rets;
    }

    // NOTE: if this stream is been disabled by app, then it is needless to request buffer from it
    if (info.mStreamStatus == BROKEN) {
        MLOGW(LogGroupHAL, "[Hal3BufferMgr]: stream:%p is disabled, needless to request buffer",
              targetStream);
        return rets;
    }

    // NOTE: if we don't have enough buffer, then we try to request more buffers and try several
    // times again
    for (int tryNum = 1; tryNum <= GET_BUFFER_RETRY_NUM; ++tryNum) {
        // NOTE: to make sure that after requesting buffers, we have at least num buffers in this
        // stream
        MLOGD(LogGroupHAL,
              "[Hal3BufferMgr]: %d time(s) trying to reserve at least %d buffer for stream: %p, "
              "current status:%s",
              tryNum, num, targetStream, info.toString().c_str());
        signalToRequestBufferLocked(targetStream, num);

        auto status = mWaitForFreeBufferCond.wait_for(lg, 50ms, [&]() {
            return info.mFreeBuffers.size() >= num || info.mStreamStatus != NORMAL;
        });

        // NOTE: request buffer fail
        if (info.mStreamStatus != NORMAL) {
            // NOTE: fatal error, abort
            if (info.mStreamStatus == BROKEN) {
                MLOGW(LogGroupHAL,
                      "[Hal3BufferMgr]: FAIL to request buffer since stream:%p is disabled, no "
                      "need to try again",
                      targetStream);
                break;
            }

            // NOTE: not fatal error, try again
            if (info.mStreamStatus != NORMAL) {
                MLOGW(LogGroupHAL, "[Hal3BufferMgr]: stream:%p has insufficient buffers",
                      targetStream);
                info.mStreamStatus = NORMAL;
                continue;
            }
        }

        // NOTE: request buffer timeout, try again
        if (!status) {
            MLOGW(LogGroupHAL, "[Hal3BufferMgr]: get %d buffers from stream:%p TIMEOUT!!", num,
                  targetStream);
            continue;
        }

        // NOTE: request buffer succeed
        getBufferFromFreeList(rets);
        break;
    }

    if (rets.empty()) {
        MLOGW(LogGroupHAL, "[Hal3BufferMgr]: FAIL to get %d buffers from stream:%p !!", num,
              targetStream);
    }

    return rets;
}

void MiHal3BufferMgr::bufferRequestLoop()
{
    while (1) {
        // NOTE: first wait buffer request
        std::vector<StreamBufferRequest> tempBufReq;
        {
            std::unique_lock<std::mutex> lg(mBufReqQueueMutex);

            mWaitForBufReqCond.wait(
                lg, [this]() { return !mBufferRequestQueue.empty() || mExitThread; });

            if (mExitThread) {
                MLOGD(LogGroupHAL, "[Hal3BufferMgr]: buffer request thread exit.");
                return;
            }

            tempBufReq = std::move(mBufferRequestQueue);
            mBufferRequestQueue.clear();
        }

        // NOTE: then request buffers from fwk
        std::vector<StreamBufferReturn> bufRets;
        camera3_buffer_request_status status = requestBuffersImpl(tempBufReq, bufRets);

        // NOTE: then process returned buffers and notify threads waiting for free buffers
        processFwkReturnedBuffers(status, tempBufReq, bufRets);
        mWaitForFreeBufferCond.notify_all();
    }
}

camera3_buffer_request_status MiHal3BufferMgr::requestBuffersImpl(
    const std::vector<StreamBufferRequest> &bufReqs, std::vector<StreamBufferReturn> &bufRets)
{
    // NOTE: first convert to camera3 interface
    camera3_buffer_request *actualReqs = new camera3_buffer_request[bufReqs.size()];
    camera3_stream_buffer_ret *actualRets = new camera3_stream_buffer_ret[bufReqs.size()];
    for (size_t i = 0; i < bufReqs.size(); ++i) {
        actualReqs[i].stream = bufReqs[i].mStream;
        actualReqs[i].num_buffers_requested = bufReqs[i].mNumBufferRequested;
        MLOGD(LogGroupHAL, "request %d buffers for stream: %p", bufReqs[i].mNumBufferRequested,
              bufReqs[i].mStream);

        actualRets[i].output_buffers = new camera3_stream_buffer[bufReqs[i].mNumBufferRequested];
    }

    // NOTE: request buffers
    uint32_t actualReturnedNum = 0;
    camera3_buffer_request_status status =
        mCamera->requestStreamBuffers(bufReqs.size(), actualReqs, &actualReturnedNum, actualRets);

    // NOTE: convert camera3_stream_buffer_ret to StreamBufferReturn
    for (int i = 0; i < actualReturnedNum; ++i) {
        StreamBufferReturn temp;
        temp.mStatus = actualRets[i].status;
        temp.mStream = actualRets[i].stream;
        temp.mOutBufNum = actualRets[i].num_output_buffers;
        for (int j = 0; j < actualRets[i].num_output_buffers; ++j) {
            auto &buffer = actualRets[i].output_buffers[j].buffer;
            if (buffer != nullptr && *buffer != nullptr) {
                // NOTE: we should wait and close acquire fence, otherwise, acquire fence
                // leakage will happen
                waitAndCloseAcquireFence(actualRets[i].output_buffers[j]);
                temp.mOutputBuffers.push_back(actualRets[i].output_buffers[j]);
            }
        }

        bufRets.push_back(std::move(temp));
    }

    // NOTE: clear memory
    // XXX: not exception safety
    for (size_t i = 0; i < bufReqs.size(); ++i) {
        delete[] actualRets[i].output_buffers;
    }
    delete[] actualReqs;
    delete[] actualRets;

    return status;
}

int MiHal3BufferMgr::waitAndCloseAcquireFence(camera3_stream_buffer &buffer)
{
    int fd = buffer.acquire_fence;
    int ret = 0;
    // NOTE: -1 means we don't need to wait
    if (fd != -1) {
        ret = sync_wait(fd, 500);
        if (ret) {
            MLOGE(LogGroupHAL, "wait acquire fence %d failed:ret=%d, stream:%p", fd, ret,
                  buffer.stream);
        }
        close(fd);
    }
    // NOTE: set to -1 to avoid double close
    buffer.acquire_fence = -1;

    return ret;
}

void MiHal3BufferMgr::processFwkReturnedBuffers(camera3_buffer_request_status status,
                                                const std::vector<StreamBufferRequest> &bufReqs,
                                                const std::vector<StreamBufferReturn> &bufRets)
{
    std::lock_guard<std::mutex> lg(mStreamInfosLock);

    // NOTE: since fwk buffers now have come into mihal, we should update incoming buffer num record
    for (auto &entry : bufReqs) {
        auto &info = mStreamInfos[entry.mStream];

        if (info.mBufferIncomingNum < entry.mNumBufferRequested) {
            MLOGE(LogGroupHAL,
                  "[Hal3BufferMgr]: broken invariant: incoming buffer num < requested buffer num!");
            MASSERT(false);
        } else {
            info.mBufferIncomingNum -= entry.mNumBufferRequested;
        }
    }

    // NOTE: check overall status
    switch (status) {
    case CAMERA3_BUF_REQ_OK:
    case CAMERA3_BUF_REQ_FAILED_PARTIAL:
        break;
    case CAMERA3_BUF_REQ_FAILED_CONFIGURING:
    case CAMERA3_BUF_REQ_FAILED_ILLEGAL_ARGUMENTS:
    case CAMERA3_BUF_REQ_FAILED_UNKNOWN:
        MLOGW(LogGroupHAL,
              "[Hal3BufferMgr]: buffer request FAIL for all streams, overall buffer request "
              "status:%d",
              status);

        for (auto &entry : bufReqs) {
            auto &info = mStreamInfos[entry.mStream];
            info.mStreamStatus = BROKEN;
            MLOGW(LogGroupHAL,
                  "[Hal3BufferMgr]: set stream:%p status as BROKEN since "
                  "camera3_buffer_request_status is FATAL",
                  entry.mStream);
        }

        break;
    default:
        break;
    }

    // NOTE: check per stream status for each returned camera3_stream_buffer_ret_t, add buffers to
    // free list if any
    for (auto &entry : bufRets) {
        auto &info = mStreamInfos[entry.mStream];

        switch (entry.mStatus) {
        case CAMERA3_PS_BUF_REQ_OK:
            info.mFreeBuffers.insert(info.mFreeBuffers.end(), entry.mOutputBuffers.begin(),
                                     entry.mOutputBuffers.end());
            MLOGD(LogGroupHAL,
                  "[Hal3BufferMgr]: get %zu buffer for fwk stream:%p, after get print status:%s",
                  entry.mOutputBuffers.size(), entry.mStream, info.toString().c_str());

            // NOTE: for these successfully returned buffers' streams, we can reset them to
            // BATCH_MODE
            if (info.mPolicy != BATCH_MODE) {
                info.mPolicy = BATCH_MODE;
                MLOGD(
                    LogGroupHAL,
                    "[Hal3BufferMgr]: fwk stream:%p have enough buffer again, reset to BATCH_MODE",
                    entry.mStream);
            }
            if (info.mStreamStatus != NORMAL) {
                info.mStreamStatus = NORMAL;
                MLOGD(LogGroupHAL,
                      "[Hal3BufferMgr]: fwk stream:%p have enough buffer again, reset to NORMAL "
                      "status",
                      entry.mStream);
            }

            break;
        case CAMERA3_PS_BUF_REQ_NO_BUFFER_AVAILABLE:
        case CAMERA3_PS_BUF_REQ_MAX_BUFFER_EXCEEDED:
            MLOGW(LogGroupHAL, "[Hal3BufferMgr]: fwk stream:%p does not have enough buffer",
                  entry.mStream);
            info.mPolicy = ONE_BUFFER_MODE;
            info.mStreamStatus = DRAINED;
            break;
        case CAMERA3_PS_BUF_REQ_STREAM_DISCONNECTED:
        case CAMERA3_PS_BUF_REQ_UNKNOWN_ERROR:
            MLOGW(LogGroupHAL,
                  "[Hal3BufferMgr]: fwk stream:%p in fatal status, do not request buffer again",
                  entry.mStream);
            info.mPolicy = ONE_BUFFER_MODE;
            info.mStreamStatus = BROKEN;
            break;
        default:
            break;
        }
    }
}

void MiHal3BufferMgr::signalToRequestBufferLocked(camera3_stream *targetStream, int reserveNum)
{
    auto &info = mStreamInfos[targetStream];

    // NOTE: first we need to decide how many buffers we want to request. the policy is as follows:
    // normally, we only request mBatchNum buffers, that is batchChoice2. However, sometimes we are
    // forced to reserve at least reserveNum buffers in this stream, then we need to also promise to
    // request at least batchChoice1 buffers in this occasion.
    int batchChoice1 = 0;
    int batchChoice2 = 0;
    // NOTE: the number of buffers we need to reqeust if we are forced to reserve some buffers
    if (reserveNum > info.mBufferIncomingNum + info.mFreeBuffers.size()) {
        batchChoice1 = reserveNum - (info.mBufferIncomingNum + info.mFreeBuffers.size());
    }

    // NOTE: the number of buffers we need to request in normal occasions
    batchChoice2 = info.mPolicy == BATCH_MODE ? info.mBatchNum : 1;
    MASSERT(info.mMaxBufferNum > 0);
    int bufferNeededNum = std::min(info.mBufferNeededNum, info.mMaxBufferNum);
    if (bufferNeededNum <
        info.mBufferIncomingNum + info.mBufferInUseNum + info.mFreeBuffers.size() + batchChoice2) {
        // NOTE: if we have enough buffer to handle all capture requests, then we don't need to
        // request buffer
        batchChoice2 = 0;
    }

    int finalBatchNum = std::max(batchChoice1, batchChoice2);
    // NOTE: if we have enough buffer then we don't need to request more
    if (finalBatchNum <= 0) {
        return;
    }

    // NOTE: because we are going to request new buffers, so we need update incoming record to avoid
    // duplicate buffer request
    info.mBufferIncomingNum += finalBatchNum;
    MLOGD(LogGroupHAL,
          "[Hal3BufferMgr]: try to request %d buffers for stream: %p, now it's waiting %d incoming "
          "buffers",
          finalBatchNum, targetStream, info.mBufferIncomingNum);

    StreamBufferRequest req{targetStream, finalBatchNum};
    {
        std::lock_guard<std::mutex> lg(mBufReqQueueMutex);

        // NOTE: try to merge existing buffer requests to reduce IPC
        auto itr = std::find_if(mBufferRequestQueue.begin(), mBufferRequestQueue.end(),
                                [&](const StreamBufferRequest &reqInQueue) {
                                    return reqInQueue.mStream == req.mStream;
                                });
        if (itr != mBufferRequestQueue.end()) {
            itr->merge(req);
        } else {
            // NOTE: if we don't find one, then just push a new one buffer request
            mBufferRequestQueue.push_back(req);
        }
    }
    mWaitForBufReqCond.notify_all();
}

} // namespace mihal
