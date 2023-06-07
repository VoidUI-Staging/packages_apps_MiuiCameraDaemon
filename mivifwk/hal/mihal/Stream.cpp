#include "Stream.h"

#include <chrono>
#include <sstream>

#include "Camera3Plus.h"
#include "CameraDevice.h"
#include "LogUtil.h"
#include "MiviSettings.h"

namespace mihal {

using namespace std::chrono_literals;

namespace {

std::map<const camera3_stream *, std::weak_ptr<Stream>> allStreams;
std::mutex streamMapMutex;

int registerStream(std::shared_ptr<Stream> stream)
{
    std::lock_guard<std::mutex> locker{streamMapMutex};
    if (allStreams.count(stream->toRaw()))
        return -EEXIST;

    allStreams.insert({stream->toRaw(), std::weak_ptr<Stream>{stream}});
    MLOGD(LogGroupHAL, "[Statistic]: register stream:%p, total %zu streams registered",
          stream->toRaw(), allStreams.size());
    return 0;
}

int unregisterStream(const camera3_stream *stream)
{
    std::lock_guard<std::mutex> locker{streamMapMutex};
    if (!allStreams.count(stream))
        return -ENOENT;

    allStreams.erase(stream);
    MLOGD(LogGroupHAL, "[Statistic]: unregister stream:%p, remain %zu streams left", stream,
          allStreams.size());
    return 0;
}

std::shared_ptr<Stream> getRegisteredStream(const camera3_stream *stream)
{
    std::lock_guard<std::mutex> locker{streamMapMutex};
    auto it = allStreams.find(stream);
    if (it == allStreams.end())
        return nullptr;

    auto sharedStream = it->second.lock();
    if (!sharedStream) {
        MLOGE(LogGroupHAL, "rich stream for raw %p has been destroied", stream);
        allStreams.erase(it);
    }

    return sharedStream;
}

} // namespace

Stream::Stream(CameraDevice *camera) : Stream{camera, 0} {}

Stream::Stream(CameraDevice *camera, uint32_t id) : Stream{camera, id, false, std::bitset<16>{}} {}
Stream::Stream(CameraDevice *camera, std::bitset<16> prop) : Stream{camera, 0, false, prop} {}
Stream::Stream(CameraDevice *camera, uint32_t id, bool isInternal, std::bitset<16> prop)
    : mIsInternal{isInternal},
      mRetired{false},
      mCamera{camera},
      mId{id},
      mPendingBuffersInVendor{0},
      mProp{prop},
      mBufferManager{std::make_unique<BufferManager>(*this)},
      mTimer{Timer::createTimer()},
      mHalStream{}
{
}

Stream::~Stream() = default;

std::shared_ptr<Stream> Stream::toRichStream(const camera3_stream *stream)
{
    return getRegisteredStream(stream);
}

std::unique_ptr<GraBuffer> Stream::requestGraBuffer()
{
    auto buffers = requestGraBuffers(1);

    if (buffers.empty())
        return nullptr;

    return std::move(buffers[0]);
}

std::vector<std::unique_ptr<GraBuffer>> Stream::requestGraBuffers(uint32_t num)
{
    std::unique_lock<std::recursive_mutex> locker{mMutex};
    MLOGI(LogGroupRT, "avail buffers:%d of stream:%p, request %d",
          mBufferManager->getAvailableBuffers(), toRaw(), num);

    std::chrono::milliseconds waitTime;
    MiviSettings::getVal("RequestGraBufferTimeOutMilliseconds", waitTime, 3000);
    bool ret = mCond.wait_for(
        locker, waitTime, [num, this]() { return mBufferManager->getAvailableBuffers() >= num; });
    if (!ret) {
        MLOGW(LogGroupHAL, "%p timeout for buffers", toRaw());
        return std::vector<std::unique_ptr<GraBuffer>>{};
    }

    std::vector<std::unique_ptr<GraBuffer>> graBuffers = mBufferManager->requestBuffers(num);
    locker.unlock();

    if (graBuffers.empty()) {
        MLOGW(LogGroupHAL, "failed to request graphic buffers");
        return std::vector<std::unique_ptr<GraBuffer>>{};
    }

    uint32_t reclaimWaitTime;
    MiviSettings::getVal("BufferReclaimWaitMilliseconds", reclaimWaitTime, 3000);
    std::weak_ptr<Stream> weakThis{shared_from_this()};
    mTimer->runAfter(reclaimWaitTime, [weakThis]() {
        std::shared_ptr<Stream> strongThis = weakThis.lock();
        if (!strongThis) {
            return;
        }

        std::lock_guard<std::recursive_mutex> locker{strongThis->mMutex};
        strongThis->mBufferManager->reclaim();
    });

    return graBuffers;
}

std::shared_ptr<StreamBuffer> Stream::requestBuffer()
{
    mVirtualBusyBufferCnt.fetch_add(1);
    mConsumeCnt.fetch_add(1);
    CameraDevice *camera = getCamera();
    std::unique_ptr<GraBuffer> graBuffer;
    if (!camera->supportBufferManagerAPI() && isInternal()) {
        graBuffer = requestGraBuffer();

        if (!graBuffer) {
            MLOGE(LogGroupHAL, "request buffer fail!!!");
            CameraDevice *camera = getCamera();
            if (!camera->supportBufferManagerAPI())
                MASSERT(false);

            mVirtualBusyBufferCnt.fetch_sub(1);
            mConsumeCnt.fetch_sub(1);
        }
    }

    return std::make_shared<LocalStreamBuffer>(shared_from_this(), std::move(graBuffer));
}

int Stream::requestBuffers(uint32_t num, std::vector<StreamBuffer *> &buffers)
{
    CameraDevice *camera = getCamera();
    if (!camera->supportBufferManagerAPI()) {
        MLOGE(LogGroupHAL, "HAL3 BufferManager API is not enabled for camera %s",
              camera->getId().c_str());
        MASSERT(false);
    }

    if (!isInternal()) {
        MLOGE(LogGroupHAL, "HAL3 BufferManager API could not be applied for stream %p", toRaw());
        MASSERT(false);
    }

    auto graBuffers = requestGraBuffers(num);
    if (graBuffers.empty()) {
        MLOGW(LogGroupHAL, "failed to request %d GraBuffers", num);
        return -ENOMEM;
    }

    std::lock_guard<std::recursive_mutex> locker{mMutex};
    for (auto &graBuffer : graBuffers) {
        buffer_handle_t handle = graBuffer->getHandle();
        auto streamBuffer =
            std::make_unique<LocalStreamBuffer>(shared_from_this(), std::move(graBuffer));
        buffers.push_back(streamBuffer.get());

        MLOGD(LogGroupHAL,
              "[RT][HalBufferMgr][Internal] request internal buffer successful, native_handle:%p, "
              "stream:%p",
              streamBuffer->getBuffer()->getHandle(), toRaw());
        mRequestedBuffers.insert({handle, std::move(streamBuffer)});
    }

    MLOGD(LogGroupHAL,
          "[RT][HalBufferMgr][Internal] requested %d buffers for stream %p, now total:%zu", num,
          toRaw(), mRequestedBuffers.size());
    return 0;
}

void Stream::releaseVirtualBuffer()
{
    std::shared_lock<std::shared_mutex> locker{mBufferListenerMutex};
    if (mVirtualBusyBufferCnt.load() > 0)
        mVirtualBusyBufferCnt.fetch_sub(1);

    if (mBufferListener)
        mBufferListener(true);
}

void Stream::releaseBuffer(std::unique_ptr<GraBuffer> buffer)
{
    std::string dump = buffer->toString().c_str();
    MLLOGI(LogGroupRT, dump, "stream: %p, streamId: %d released GraBuffer:", toRaw(),
           getStreamId());

    std::unique_lock<std::recursive_mutex> locker{mMutex};
    mBufferManager->releaseBuffer(std::move(buffer));
    locker.unlock();

    mCond.notify_one();
}

std::unique_ptr<StreamBuffer> Stream::getRequestedStreamBuffer(const StreamBuffer &buffer)
{
    CameraDevice *camera = getCamera();
    if (!camera->supportBufferManagerAPI()) {
        MLOGE(LogGroupHAL, "HAL3 BufferManager API is not enabled for camera %s",
              camera->getId().c_str());
        MASSERT(false);
    }

    std::lock_guard<std::recursive_mutex> locker{mMutex};
    auto it = mRequestedBuffers.find(buffer.getBuffer()->getHandle());
    if (it == mRequestedBuffers.end()) {
        MLOGE(LogGroupHAL,
              "[HalBufferMgr][Internal] failed to get requested buffer, buffer status:%d, "
              "native_handle:%p, stream:%p",
              buffer.getStatus(), buffer.getBuffer()->getHandle(), buffer.getStream()->toRaw());
        return nullptr;
    }

    std::unique_ptr<StreamBuffer> cached = std::move(it->second);
    mRequestedBuffers.erase(it);
    MLOGD(LogGroupHAL,
          "[RT][HalBufferMgr][Internal] get requested buffer, buffer status:%d, native_handle:%p, "
          "stream:%p",
          cached->getStatus(), cached->getBuffer()->getHandle(), cached->getStream()->toRaw());

    MLOGD(LogGroupHAL,
          "[RT][HalBufferMgr][Internal] remove one buffer from internal %p, after remove now "
          "total:%zu",
          toRaw(), mRequestedBuffers.size());

    return cached;
}

int Stream::returnInternalStreamBuffers()
{
    uint32_t size = mRequestedBuffers.size();
    if (!size) {
        return -ENOENT;
    }

    for (auto &p : mRequestedBuffers) {
        p.second->releaseBuffer();
    }

    MLOGI(LogGroupHAL, "[HalBufferMgr][Internal] returned %d buffers for internal:%p", size,
          toRaw());
    mRequestedBuffers.clear();

    return 0;
}

int Stream::returnInternalStreamBuffer(const camera3_stream_buffer &buffer)
{
    auto it = mRequestedBuffers.find(*buffer.buffer);
    if (it == mRequestedBuffers.end()) {
        MLOGW(LogGroupHAL,
              "[HalBufferMgr][Internal] buffer:%p not found, native_handle:%p, stream:%p, check if "
              "Vendor has invoked process_capture_result with the same handle",
              buffer.buffer, *buffer.buffer, toRaw());
        return -ENOENT;
    }

    it->second->releaseBuffer();
    mRequestedBuffers.erase(it);

    MLOGD(LogGroupHAL, "[RT][HalBufferMgr][Internal] after return, internal %p, now total:%zu",
          toRaw(), mRequestedBuffers.size());
    return 0;
}

uint32_t Stream::getRequestedBuffersSzie()
{
    std::lock_guard<std::recursive_mutex> locker{mMutex};
    return mRequestedBuffers.size();
}

int Stream::returnStreamBuffers()
{
    std::lock_guard<std::recursive_mutex> locker{mMutex};
    if (isInternal())
        return returnInternalStreamBuffers();

    return 0;
}

int Stream::returnStreamBuffer(const camera3_stream_buffer &buffer)
{
    std::lock_guard<std::recursive_mutex> locker{mMutex};
    if (isInternal())
        return returnInternalStreamBuffer(buffer);

    return 0;
}

void Stream::setBufferCapacity(uint32_t capacity)
{
    mBufferManager->setCapacity(capacity);
}

uint32_t Stream::getBufferCapacity() const
{
    std::lock_guard<std::recursive_mutex> locker{mMutex};
    return mBufferManager->getCapacity();
}

uint32_t Stream::getPendingBuffersInVendor() const
{
    std::lock_guard<std::recursive_mutex> locker{mMutex};
    return mPendingBuffersInVendor;
}

int Stream::registerListener(BufferListener listener)
{
    std::lock_guard<std::shared_mutex> locker{mBufferListenerMutex};
    mBufferListener = listener;

    return 0;
}

int Stream::unregisterListener()
{
    std::lock_guard<std::shared_mutex> locker{mBufferListenerMutex};
    mBufferListener = nullptr;

    return 0;
}

uint32_t Stream::getVirtualAvailableBuffers() const
{
    std::lock_guard<std::recursive_mutex> locker{mMutex};
    int capaity = mBufferManager->getCapacity();
    return capaity > mVirtualBusyBufferCnt.load() ? capaity - mVirtualBusyBufferCnt.load() : 0;
}

uint32_t Stream::getAvailableBuffers() const
{
    std::lock_guard<std::recursive_mutex> locker{mMutex};
    return mBufferManager->getAvailableBuffers();
}

std::string Stream::toString(int indent, bool full) const
{
    std::ostringstream str;

    const char *indentStr = getIndentation(indent);
    if (!full) {
        str << indentStr << "id=" << mId << " width=" << getWidth() << " height=" << getHeight()
            << " format=" << getFormat();

        return str.str();
    }

    std::string phyId{"null"};
    const char *phyIdCStr = getPhyId();
    if (phyIdCStr && strlen(phyIdCStr))
        phyId = phyIdCStr;

    str << "raw stream=" << toRaw() << "\n"
        << indentStr << "type=" << getType() << ", "
        << "WxH=" << getWidth() << "x" << getHeight() << ", "
        << "format=" << getFormat() << ", "
        << "usage=" << std::hex << std::showbase << getUsage() << ","
        << "stream_use_case=" << std::hex << std::showbase << getStreamUsecase() << "\n"
        << indentStr << "priv=" << getCookie() << ", "
        << "dataspace=" << getDataspace() << std::dec << ", "
        << "maxBuffers=" << getMaxBuffers() << ", "
        << "bufferCapacity=" << getBufferCapacity() << "\n"
        << indentStr << "phyId=" << phyId << ", "
        << "streamId=" << getStreamId() << ", "
        << "streamProp=" << mProp << "\n"
        << indentStr << "isInternal=" << isInternal() << ", "
        << "isSnapshot=" << isSnapshot() << ", "
        << "isPreview=" << isPreview() << ", "
        << "isRealtime=" << isRealtime() << ", "
        << "isQuickview=" << isQuickview();

    return str.str();
}

int Stream::setRetired()
{
    // NOTE: stream has been retired, mustn't retire twice because unregisterStream is not
    // re-entrant
    if (mRetired) {
        return -1;
    }

    mRetired = true;
    return unregisterStream(toRaw());
}

std::shared_ptr<Stream> RemoteStream::create(CameraDevice *camera, camera3_stream *stream,
                                             std::bitset<16> prop)
{
    std::shared_ptr<Stream> remote = getRegisteredStream(stream);
    if (remote)
        return remote;

    remote = std::shared_ptr<RemoteStream>{new RemoteStream{camera, stream, prop}};
    registerStream(remote);
    return remote;
}

RemoteStream::RemoteStream(CameraDevice *camera, camera3_stream *stream, std::bitset<16> prop)
    : Stream{camera, prop}, mStream{stream}
{
}

RemoteStream::~RemoteStream()
{
    if (!mRetired)
        unregisterStream(toRaw());
}

std::shared_ptr<Stream> LocalStream::create(CameraDevice *camera, uint32_t width, uint32_t height,
                                            int format, uint32_t usage, uint64_t stream_use_case,
                                            android_dataspace_t space, bool internal,
                                            const char *phyId, int type)
{
    auto local = std::shared_ptr<LocalStream>{new LocalStream{
        camera, width, height, format, usage, stream_use_case, space, internal, phyId, type}};
    registerStream(local);
    return local;
}

LocalStream::LocalStream(CameraDevice *camera, uint32_t width, uint32_t height, int format,
                         uint32_t usage, uint64_t stream_use_case, android_dataspace_t space,
                         bool internal, const char *phyId, int type)
    : Stream{camera},
      mStream{std::make_unique<RawStreamWrapper>(width, height, format, usage, stream_use_case,
                                                 space, phyId, type)}
{
    mIsInternal = internal;
}

LocalStream::~LocalStream()
{
    if (!mRetired)
        unregisterStream(toRaw());
}

LocalStream::RawStreamWrapper::RawStreamWrapper(uint32_t width, uint32_t height, int format,
                                                uint32_t usage, uint64_t stream_use_case,
                                                android_dataspace_t space, const char *phyId,
                                                int type)
    : camera3_stream{}
{
    camera3_stream::width = width;
    camera3_stream::height = height;
    camera3_stream::format = format;
    camera3_stream::usage = usage;
    camera3_stream::group_id = -1;
    camera3_stream::num_sensor_pixel_modes_used = 0;
    camera3_stream::sensor_pixel_modes = nullptr;
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 32) // Android-T or better
    camera3_stream::stream_use_case = stream_use_case;
    camera3_stream::dynamic_range_profile = 1;
#endif

    data_space = space;
    physical_camera_id = phyId ? strdup(phyId) : nullptr;
    stream_type = type;
}

LocalStream::RawStreamWrapper::~RawStreamWrapper()
{
    free(const_cast<char *>(physical_camera_id));
}

} // namespace mihal
