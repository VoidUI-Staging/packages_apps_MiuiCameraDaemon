#include "BufferManager.h"

#include <sstream>

#include "GraBuffer.h"
#include "LogUtil.h"
#include "Stream.h"

namespace mihal {

BufferManager::BufferManager(const Stream &stream)
    : mReclaim{false}, mStream{stream}, mBufferCapacity{}
{
}

uint32_t BufferManager::adjustedCap() const
{
    return mBufferCapacity ? mBufferCapacity : mStream.getMaxBuffers();
}

void BufferManager::setCapacity(uint32_t capacity)
{
    mBufferCapacity = capacity;
    MLOGI(LogGroupHAL, "new capacity:%d for streamID:%d", adjustedCap(), mStream.getStreamId());

    if (getIdleBuffers() > adjustedCap()) {
        auto destroyNum = getIdleBuffers() - adjustedCap();
        auto it = std::prev(mIdleBuffers.end(), destroyNum);
        mIdleBuffers.erase(it, mIdleBuffers.end());
    }
}

uint32_t BufferManager::getAvailableBuffers() const
{
    uint32_t cap = adjustedCap();
    return cap > mBusyBuffers.size() ? cap - mBusyBuffers.size() : 0;
}

uint32_t BufferManager::getIdleBuffers() const
{
    return mIdleBuffers.size();
}

uint32_t BufferManager::getBusyBuffers() const
{
    return mBusyBuffers.size();
}

std::vector<std::unique_ptr<GraBuffer>> BufferManager::requestBuffers(uint32_t num)
{
    std::vector<std::unique_ptr<GraBuffer>> buffers;

    if (getAvailableBuffers() < num)
        return buffers;

    for (int i = 0; i < num; i++) {
        auto buffer = requestBuffer();
        if (buffer == nullptr) {
            MLOGW(LogGroupHAL, "request grahic buffer failed, return empty buffers directly!");
            return std::vector<std::unique_ptr<GraBuffer>>{};
        }
        buffers.push_back(std::move(buffer));
    }

    return buffers;
}

std::unique_ptr<GraBuffer> BufferManager::requestBuffer()
{
    uint32_t cap = adjustedCap();

    if (mIdleBuffers.size()) {
        auto it = mIdleBuffers.begin();
        std::unique_ptr<GraBuffer> buffer = std::move(it->second);
        mBusyBuffers.insert(it->first);
        mIdleBuffers.erase(it);

        mReclaim = false;
        return buffer;
    }

    if (mBusyBuffers.size() >= cap) {
        MLOGE(LogGroupHAL, "busybuffer size %zu is greater than capacity %d", mBusyBuffers.size(),
              cap);
        return nullptr;
    }

    auto buffer = std::make_unique<GraBuffer>(&mStream, "mihal");
    if (buffer->initCheck()) {
        MLOGE(LogGroupHAL, "failed to create a graphic buffer");
        return nullptr;
    }

    mBusyBuffers.insert(buffer->getId());

    mReclaim = false;
    return buffer;
}

// TODO: if the new cap is less than the older one, we need to free the buffers in need
int BufferManager::releaseBuffer(std::unique_ptr<GraBuffer> buffer)
{
    uint64_t id = buffer->getId();
    // sanity check: the buffer must be in the busy and not in free
    if (mIdleBuffers.count(id)) {
        return -EEXIST;
    }

    if (!mBusyBuffers.count(id)) {
        MLOGW(LogGroupHAL, "not found in Busy list: bufferId %" PRIu64 " @stream:\n\t%s", id,
              mStream.toString(0).c_str());
        return -ENOENT;
    }

    mBusyBuffers.erase(id);
    if (mIdleBuffers.size() < adjustedCap()) {
        mIdleBuffers.insert({id, std::move(buffer)});
    }

    return reclaimImpl();
}

int BufferManager::reclaim()
{
    mReclaim = true;

    MLOGI(LogGroupHAL, "reclaim for stream %d, buffers before reclaim:\n%s", mStream.getStreamId(),
          toString().c_str());

    return reclaimImpl();
}

int BufferManager::reclaimImpl()
{
    if (!mReclaim)
        return 0;

    mIdleBuffers.clear();

    return 0;
}

std::string BufferManager::toString() const
{
    std::ostringstream str;

    str << "BufferCapacity: " << adjustedCap()
        << ", Total Buffers: " << mIdleBuffers.size() + mBusyBuffers.size()
        << "\n\tBusy=" << mBusyBuffers.size() << ":";

    for (const auto &id : mBusyBuffers)
        str << " " << (id & UINT32_MAX);

    str << "\n\tIdle=" << mIdleBuffers.size() << ":";
    for (const auto &pair : mIdleBuffers)
        str << " " << (pair.first & UINT32_MAX);

    return str.str();
}

} // namespace mihal
