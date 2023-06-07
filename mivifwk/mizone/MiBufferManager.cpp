/* Copyright

*/

#include "MiBufferManager.h"

#include <cutils/properties.h>

#include <mutex>

#include "MiZoneTypes.h"

#define BUF_USAGE  (eBUFFER_USAGE_SW_MASK | eBUFFER_USAGE_HW_CAMERA_READWRITE)
#define MGR_BUF_GB (1024 * 1024 * 1024)

#define PREVIEW_WIDTH  1440
#define PREVIEW_HEIGHT 1080

using namespace midebug;
namespace mizone {

// large number to avoid bufferId conflict with framework buffers
const uint64_t kBufferIdOffset = 0x98000000;
std::atomic<uint64_t> gStreamBufferId = kBufferIdOffset;

std::shared_ptr<MiBufferManager> MiBufferManager::create(const BufferMgrCreateInfo &info)
{
    StreamConfiguration fwkCfg = info.fwkConfig;
    StreamConfiguration vndCfg = info.vendorConfig;

    if (fwkCfg.streams.empty() && vndCfg.streams.empty()) {
        MLOGE(LogGroupHAL, "invalid configuration");
        return nullptr;
    }

    if (info.callback == nullptr) {
        MLOGW(LogGroupHAL, "invalid callback");
    }

    if (info.camMode == nullptr) {
        MLOGW(LogGroupHAL, "invalid CamMode");
    }

    return std::make_shared<MiBufferManager>(info);
}

MiBufferManager::MiBufferManager(const BufferMgrCreateInfo &info)
{
    MLOGI(LogGroupHAL, "ctor start");

    statusCallback = info.callback;
    mSupportAndroidHBM = info.supportAndroidHBM;
    mSupportCoreHBM = info.supportCoreHBM;

    mpCamMode = info.camMode;
    mDeviceCallback = info.devCallback;
    if (mDeviceCallback == nullptr) {
        mSupportAndroidHBM = false;
        mSupportCoreHBM = false;
    }

    MLOGD(LogGroupHAL, "framework stream size %lu", info.fwkConfig.streams.size());
    for (auto &stream : info.fwkConfig.streams) {
        if (stream.streamType == StreamType::OUTPUT) {
            buildStreamInfo(stream);
        }
    }
    MLOGD(LogGroupHAL, "vendor stream size %lu", info.vendorConfig.streams.size());
    for (const auto &stream : info.vendorConfig.streams) {
        auto it = mStreamInfoMap.find(stream.id);
        if (it != mStreamInfoMap.end()) {
            MLOGW(LogGroupHAL, "the same stream %d in map, it's framework stream", it->first);
            continue;
        }
        if (stream.streamType == StreamType::OUTPUT) {
            buildStreamInfo(stream);
        }
    }

    for (auto &streamInfo : mStreamInfoMap) {
        mMemInfo[streamInfo.second.streamId] = 0;
    }

    mThreshold = ::property_get_int64("vendor.debug.camera.mizone.bufMgrThr", 4LL * MGR_BUF_GB);

    mThread = std::thread([this] { threadLoop(); });

    MLOGI(LogGroupHAL, "ctor end");
}

MiBufferManager::~MiBufferManager()
{
    {
        std::lock_guard<std::mutex> lock(mMapMutex);
        mBusyBufferMap.clear();
        mIdleBufferMap.clear();
        // onMemoryChanged();
    }
    mMemInfo.clear();
    mStreamInfoMap.clear();

    mThread.join();
}

bool MiBufferManager::requestBuffers(const std::vector<MiBufferRequestParam> &requestParam,
                                     std::map<int32_t, std::vector<StreamBuffer>> &streamBufferMap)
{
    bool ret = true;
    MLOGD(LogGroupHAL, " requestParam size %lu", requestParam.size());

    streamBufferMap.clear();
    std::vector<MiBufferRequestParam> fwkReqs;
    std::vector<MiBufferRequestParam> vndReqs;
    if (mSupportAndroidHBM) {
        for (const auto &bufRequest : requestParam) {
            if (mStreamInfoMap.find(bufRequest.streamId) == mStreamInfoMap.end()) {
                continue;
            }
            uint32_t prop = mStreamInfoMap.at(bufRequest.streamId).streamProp;
            if ((prop & STREAM_OWNED_FRAMEWORK) != 0) {
                if ((prop & STREAM_USED_BY_HAL) != 0) {
                    MLOGE(LogGroupHAL, " stream %d used by Core should not requestBuffer here",
                          bufRequest.streamId);
                    continue;
                }
                fwkReqs.push_back(bufRequest);
            } else {
                vndReqs.push_back(bufRequest);
            }
        }
    } else {
        vndReqs = requestParam;
    }

    // for fwk streams
    std::map<int32_t, std::vector<StreamBuffer>> fwkBufMap;
    if (!fwkReqs.empty()) {
        registerFwkBuffers(fwkReqs, fwkBufMap);
    }

    // for vendor streams
    std::map<int32_t, std::vector<StreamBuffer>> vndBufMap;
    if (!vndReqs.empty()) {
        registerVndBuffers(vndReqs, vndBufMap);
    }

    // append all buffers to result map
    streamBufferMap.insert(fwkBufMap.begin(), fwkBufMap.end());
    MLOGD(LogGroupHAL, "fwk streamBufferMap size %lu", streamBufferMap.size());
    streamBufferMap.insert(vndBufMap.begin(), vndBufMap.end());
    MLOGD(LogGroupHAL, "vnd streamBufferMap size %lu", streamBufferMap.size());

    if (streamBufferMap.empty()) {
        ret = false;
    }
    return ret;
}

void MiBufferManager::releaseBuffers(std::vector<StreamBuffer> &streamBuffers)
{
    std::vector<StreamBuffer> fwkBufs;
    std::vector<StreamBuffer> vndBufs;
    if (mSupportAndroidHBM) {
        for (auto &streamBuffer : streamBuffers) {
            if (mStreamInfoMap[streamBuffer.streamId].streamProp & STREAM_OWNED_FRAMEWORK) {
                fwkBufs.push_back(streamBuffer);
            } else {
                vndBufs.push_back(streamBuffer);
            }
        }
    } else {
        vndBufs = streamBuffers;
    }

    // framework buffers, call returnStreamBuffers cb
    if (!fwkBufs.empty()) {
        mDeviceCallback->returnStreamBuffers(fwkBufs);
    }

    // vendor buffers, release local
    std::lock_guard<std::mutex> lock(mMapMutex);
    for (auto &streamBuffer : vndBufs) {
        auto it = mBusyBufferMap.find(streamBuffer.streamId);
        if (it == mBusyBufferMap.end() ||
            it->second.find(streamBuffer.bufferId) == it->second.end()) {
            MLOGE(LogGroupHAL, "invalid stream %d or buffer %lu", streamBuffer.streamId,
                  streamBuffer.bufferId);
            continue;
        }
        if (isCapcityExceed(streamBuffer.streamId)) {
            (*it).second.erase(streamBuffer.bufferId);
        } else {
            moveToIdle(streamBuffer);
        }
        onMemoryChanged();
    }
}

bool MiBufferManager::registerFwkBuffers(
    const std::vector<MiBufferRequestParam> &requestParam,
    std::map<int32_t, std::vector<StreamBuffer>> &streamBufferMap)
{
    BufferRequestStatus status;
    std::vector<StreamBufferRet> bufferRets;
    std::vector<BufferRequest> streamBufReqs;
    for (const auto &param : requestParam) {
        streamBufReqs.push_back({param.streamId, static_cast<uint32_t>(param.bufferNum)});
    }

    status = requestFwkStreamBuffers(streamBufReqs, bufferRets);

    for (auto &ret : bufferRets) {
        StreamBuffersVal val = ret.val;
        if (val.error == StreamBufferRequestError::NO_ERROR) {
            for (auto &buf : val.buffers) {
                streamBufferMap[ret.streamId].push_back(buf);
            }
        } else {
            MLOGE(LogGroupHAL, " requestStreamBuffers failed for stream %d", ret.streamId);
        }
    }

    return !streamBufferMap.empty();
}

bool MiBufferManager::registerVndBuffers(
    const std::vector<MiBufferRequestParam> &requestParam,
    std::map<int32_t, std::vector<StreamBuffer>> &streamBufferMap)
{
    for (const auto &bufRequest : requestParam) {
        MLOGD(LogGroupHAL, " alloc buffer for stream %d with num %d", bufRequest.streamId,
              bufRequest.bufferNum);
        auto it = mStreamInfoMap.find(bufRequest.streamId);
        if (it == mStreamInfoMap.end()) {
            MLOGE(LogGroupHAL, "can not find stream of id %d", bufRequest.streamId);
            continue;
        }
        std::vector<StreamBuffer> streamBuffers;
        for (int i = 0; i < bufRequest.bufferNum; i++) {
            StreamBuffer newstreamBuffer;
            if (getBufferFromIdle(bufRequest.streamId, newstreamBuffer)) {
                std::lock_guard<std::mutex> lock(mMapMutex);
                moveToBusy(newstreamBuffer);
                streamBuffers.push_back(newstreamBuffer);
            } else if (allocateVndBuffer(bufRequest.streamId, newstreamBuffer)) {
                streamBuffers.push_back(newstreamBuffer);
            }
        }
        streamBufferMap.insert(std::make_pair(bufRequest.streamId, streamBuffers));
    }

    return !streamBufferMap.empty();
}

bool MiBufferManager::allocateVndBuffer(int32_t streamId, StreamBuffer &streamBuffer)
{
    auto it = mStreamInfoMap.find(streamId);
    Stream &stream = it->second.stream;
    streamBuffer.streamId = stream.id;
    streamBuffer.bufferId = (gStreamBufferId++);
    ImgParam param{stream.format, (int32_t)stream.width, (int32_t)stream.height, stream.bufferSize,
                   BUF_USAGE};
    streamBuffer.buffer = MiImageBuffer::createBuffer("MiBufferManager", "vendor", param, 1);
    if (streamBuffer.buffer == nullptr) {
        MLOGE(LogGroupHAL, "create MiImageBuffer failed for stream %d", stream.id);
        return false;
    }
    BufferItem bufferItem{streamBuffer, 0};
    {
        std::lock_guard<std::mutex> lock(mMapMutex);
        mBusyBufferMap[streamBuffer.streamId][streamBuffer.bufferId] = bufferItem;
        onMemoryChanged();
    }
    return true;
}

void MiBufferManager::requestStreamBuffers(const std::vector<BufferRequest> &bufReqs,
                                           requestStreamBuffers_cb cb)
{
    BufferRequestStatus status = STATUS_OK;
    if (mDeviceCallback == nullptr) {
        MLOGE(LogGroupHAL, "null mDeviceCallback");
        cb(status, {});
        return;
    }

    std::vector<StreamBufferRet> bufferRets;
    std::vector<BufferRequest> fwkStreamBufReqs;
    std::vector<BufferRequest> vendorStreamBufReqs;

    // distinguish streams
    for (auto request : bufReqs) {
        if (mStreamInfoMap[request.streamId].streamProp & STREAM_OWNED_FRAMEWORK) {
            fwkStreamBufReqs.push_back(request);
        } else {
            vendorStreamBufReqs.push_back(request);
        }
    }

    if (!mSupportAndroidHBM && !fwkStreamBufReqs.empty()) {
        MLOGE(LogGroupHAL, "not support request fwk buffer. mSupportAndroidHBM %d",
              mSupportAndroidHBM);
        status = FAILED_ILLEGAL_ARGUMENTS;
        cb(status, {});
    }

    if (!mSupportCoreHBM && !vendorStreamBufReqs.empty()) {
        MLOGE(LogGroupHAL, "not support request vnd buffer. mSupportCoreHBM %d", mSupportCoreHBM);
        status = FAILED_ILLEGAL_ARGUMENTS;
        cb(status, {});
        return;
    }

    // for vendor streams, do allocate
    std::map<int32_t, std::vector<StreamBuffer>> streamBufferMap;
    if (!vendorStreamBufReqs.empty()) {
        std::vector<MiBufferRequestParam> requestParam;
        for (auto req : vendorStreamBufReqs) {
            requestParam.push_back({req.streamId, static_cast<int32_t>(req.numBuffersRequested)});
        }
        if (!registerVndBuffers(requestParam, streamBufferMap)) {
            MLOGE(LogGroupHAL, "registerVndBuffers failed");
            status = FAILED_PARTIAL;
            cb(status, bufferRets);
            return;
        }
    }

    // for framework streams, call framework callback
    if (!fwkStreamBufReqs.empty()) {
        status = requestFwkStreamBuffers(fwkStreamBufReqs, bufferRets);
    }

    // append vendor stream buffers
    for (auto streamBuffer : streamBufferMap) {
        bufferRets.push_back(
            {streamBuffer.first, {StreamBufferRequestError::NO_ERROR, streamBuffer.second}});
    }

    cb(status, bufferRets);
}

BufferRequestStatus MiBufferManager::requestFwkStreamBuffers(
    std::vector<BufferRequest> &bufReqs, std::vector<StreamBufferRet> &bufferRets)
{
    // if callback is paused by switchToOffline, wait until new callbak set by setCallback
    std::unique_lock<std::mutex> lock(mPauseCallbackMutex);
    if (!waitForCallback(lock)) {
        return FAILED_CONFIGURING;
    }

    BufferRequestStatus status = FAILED_ILLEGAL_ARGUMENTS;
    auto myCallback = [&status, &bufferRets](BufferRequestStatus retStatus,
                                             const std::vector<StreamBufferRet> &retBufferRets) {
        status = retStatus;
        bufferRets = std::move(retBufferRets);
    };

    do {
        mDeviceCallback->requestStreamBuffers(bufReqs, myCallback);
        if (status != STATUS_OK && status != FAILED_PARTIAL && status != FAILED_CONFIGURING) {
            MLOGE(LogGroupHAL, " failed with status %u", status);
            break;
        }
        if (status == FAILED_CONFIGURING) {
            MLOGW(LogGroupHAL, " FAILED_CONFIGURING, will try again");
            usleep(10 * 1000);
        }
    } while (status == FAILED_CONFIGURING);

    return status;
}

void MiBufferManager::returnStreamBuffers(const std::vector<StreamBuffer> &buffers)
{
    MLOGD(LogGroupHAL, " buffers num %lu", buffers.size());

    if (mDeviceCallback == nullptr) {
        MLOGE(LogGroupHAL, "null mDeviceCallback");
        return;
    }

    // if callback is paused by switchToOffline, wait until new callbak set by setCallback
    std::unique_lock<std::mutex> lock(mPauseCallbackMutex);
    if (!waitForCallback(lock)) {
        return;
    }

    std::vector<StreamBuffer> fwkBuffers;
    std::vector<StreamBuffer> vendorBuffers;
    for (const auto &buf : buffers) {
        if ((mStreamInfoMap[buf.streamId].streamProp & STREAM_OWNED_FRAMEWORK) != 0) {
            fwkBuffers.push_back(buf);
        } else {
            vendorBuffers.push_back(buf);
        }
    }

    releaseBuffers(vendorBuffers);
    mDeviceCallback->returnStreamBuffers(fwkBuffers);
}

void MiBufferManager::signalStreamFlush(const std::vector<int32_t> &streamIds,
                                        uint32_t streamConfigCounter)
{
    MLOGE(LogGroupHAL, "MiBufferManager will do nothing");
}

bool MiBufferManager::waitForCallback(std::unique_lock<std::mutex> &lock)
{
    auto ok = mPauseCallbackCond.wait_for(lock, std::chrono::milliseconds(500),
                                          [this]() -> bool { return !mPauseCallback; });
    if (!ok) {
        MLOGE(LogGroupHAL, "wait callback timeout (%d ms)!", 500);
    }
    return ok;
}

void MiBufferManager::pauseCallback()
{
    std::lock_guard<std::mutex> lock(mPauseCallbackMutex);
    mPauseCallback = true;
}

void MiBufferManager::setCallback(std::shared_ptr<MiCamDevCallback> deviceCallback)
{
    if (deviceCallback == nullptr) {
        MLOGE(LogGroupCore, "deviceCallback is nullptr!");
        return;
    }
    mDeviceCallback = deviceCallback;

    // notify to continue callback
    {
        std::lock_guard<std::mutex> lock(mPauseCallbackMutex);
        mPauseCallback = false;
    }
    mPauseCallbackCond.notify_all();
}

uint32_t MiBufferManager::queryMemoryUsedByStream(int32_t streamId)
{
    if ((mStreamInfoMap.at(streamId).streamProp & (STREAM_OWNED_FRAMEWORK)) != 0) {
        MLOGE(LogGroupHAL, "not controled stream %d", streamId);
        return 0;
    } else {
        std::lock_guard<std::mutex> lock(mMapMutex);
        return getTotalMemoryUsedByStream(streamId);
    }
}

uint32_t MiBufferManager::getTotalMemoryUsedByStream(int32_t streamId)
{
    uint32_t size = 0;

    // used
    for (auto &pair : mBusyBufferMap) {
        if (pair.first == streamId) {
            for (auto &subPair : pair.second) {
                StreamBuffer streamBuffer = subPair.second.buffer;
                size += streamBuffer.buffer->getBufSize();
            }
        }
    }

    // MLOGV(LogGroupHAL, "buffer used %u bytes by stream %d", size, streamId);
    return size;
}

void MiBufferManager::onMemoryChanged()
{
    mTotalUsed = 0;
    for (auto &streamInfo : mStreamInfoMap) {
        if (mMemInfo.find(streamInfo.first) != mMemInfo.end()) {
            auto &streamMemUsed = mMemInfo.at(streamInfo.first);
            streamMemUsed = getTotalMemoryUsedByStream(streamInfo.first);
            mTotalUsed += streamMemUsed;
        }
    }

    if (mTotalUsed > mThreshold) {
        MLOGW(LogGroupHAL, "total used %ld exceed threshold %ld", mTotalUsed, mThreshold);
        // TODO: need to release idle buffers?
    }

    if (statusCallback != nullptr) {
        statusCallback(mMemInfo);
    } else {
        MLOGE(LogGroupHAL, "nullptr");
    }
}

bool MiBufferManager::isCapcityExceed(int32_t streamId)
{
    // user should consider the concurrency and data validity
    int32_t bufferCount = 0;
    auto it = mBusyBufferMap.find(streamId);
    if (it != mBusyBufferMap.end()) {
        bufferCount += it->second.size();
    }
    it = mIdleBufferMap.find(streamId);
    if (it != mIdleBufferMap.end()) {
        bufferCount += it->second.size();
    }

    return bufferCount > mStreamInfoMap.at(streamId).capacity;
}

void MiBufferManager::moveToIdle(StreamBuffer &streamBuffer)
{
    // user should consider the concurrency and data validity
    auto bufItem = mBusyBufferMap[streamBuffer.streamId][streamBuffer.bufferId];
    mIdleBufferMap[streamBuffer.streamId][streamBuffer.bufferId] = bufItem;
    mBusyBufferMap[streamBuffer.streamId].erase(streamBuffer.bufferId);
}

void MiBufferManager::moveToBusy(StreamBuffer &streamBuffer)
{
    // user should consider the concurrency and data validity
    auto bufItem = mIdleBufferMap[streamBuffer.streamId][streamBuffer.bufferId];
    mBusyBufferMap[streamBuffer.streamId][streamBuffer.bufferId] = bufItem;
    mIdleBufferMap[streamBuffer.streamId].erase(streamBuffer.bufferId);
}

bool MiBufferManager::getBufferFromIdle(int32_t streamId, StreamBuffer &streamBuffer)
{
    std::lock_guard<std::mutex> lock(mMapMutex);
    auto it = mIdleBufferMap.find(streamId);
    if (it != mIdleBufferMap.end()) {
        auto subIt = it->second.begin();
        if (subIt != it->second.end()) {
            streamBuffer = subIt->second.buffer;
            return true;
        }
        return false;
    }
    return false;
}

std::pair<bool, StreamBuffer> MiBufferManager::findOutputBufferByStreamId(
    const CaptureRequest &request, const int32_t &streamId)
{
    for (const auto &outputBuffer : request.outputBuffers) {
        if (outputBuffer.streamId == streamId) {
            return std::make_pair(true, outputBuffer);
        }
    }
    return std::make_pair(false, StreamBuffer());
}

void MiBufferManager::buildStreamInfo(const Stream &stream)
{
    StreamInfo curStreamInfo;
    curStreamInfo.stream = stream;
    curStreamInfo.streamId = stream.id;
    curStreamInfo.capacity =
        ::property_get_int32("vendor.debug.camera.mizone.bufMgrCap", 8); // TODO

    if (isQuickViewCacheStream(stream)) {
        // QuickViewCache mantain 20 cache buffer now
        // idle queue will left 2~3 quickview cache buffer.
        curStreamInfo.capacity = 30;
    }
    curStreamInfo.strategy = IMMEDIATELY; // TODO
    curStreamInfo.streamProp = getStreamProp(stream);

    mStreamInfoMap.insert(std::make_pair(stream.id, curStreamInfo));
}

// hardcode for get stream prop
uint32_t MiBufferManager::getStreamProp(const Stream &stream)
{
    uint32_t prop = 0;

    // decide stream owner
    if (stream.id >= STREAM_ID_VENDOR_START) {
        prop |= (STREAM_OWNED_VENDOR | STREAM_USED_BY_HAL);
    } else {
        prop |= STREAM_OWNED_FRAMEWORK;
        if (isPreviewStream(stream)) {
            prop |= STREAM_USED_BY_HAL;
        } else {
            prop |= STREAM_USED_BY_CUSTOM;
        }
    }
    return prop;
}

bool MiBufferManager::isPreviewStream(const Stream &stream)
{
    auto usage = static_cast<uint64_t>(stream.usage);

    if (stream.streamType == StreamType::OUTPUT && stream.format == EImageFormat::eImgFmt_NV21 &&
        ((usage & eBUFFER_USAGE_HW_TEXTURE) || (usage & eBUFFER_USAGE_HW_COMPOSER))) {
        return true;
    }
    return false;
}

bool MiBufferManager::isQuickViewCacheStream(const Stream &stream)
{
    auto usage = static_cast<uint64_t>(stream.usage);
    if (stream.streamType == StreamType::OUTPUT &&
        stream.format == NSCam::EImageFormat::eImgFmt_NV21 && stream.width == PREVIEW_WIDTH &&
        stream.height == PREVIEW_HEIGHT && ((usage & mcam::eBUFFER_USAGE_HW_TEXTURE)) == 0 &&
        ((usage & mcam::eBUFFER_USAGE_HW_COMPOSER)) == 0 &&
        ((usage ^ (mcam::eBUFFER_USAGE_SW_READ_OFTEN | mcam::eBUFFER_USAGE_SW_WRITE_OFTEN |
                   mcam::eBUFFER_USAGE_HW_CAMERA_WRITE)) == 0)) {
        return true;
    }
    return false;
}

void MiBufferManager::threadLoop()
{
    MLOGE(LogGroupHAL, "not implement yet");
}

} // namespace mizone
