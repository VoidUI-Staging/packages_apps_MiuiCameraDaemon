#ifndef __BUFFER_MANAGER_H__
#define __BUFFER_MANAGER_H__

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace mihal {

class GraBuffer;
class Stream;

class BufferManager
{
public:
    BufferManager(const Stream &stream);
    BufferManager(const BufferManager &other) = delete;
    ~BufferManager() = default;

    void setCapacity(uint32_t capacity);

    uint32_t getCapacity() const { return adjustedCap(); }
    uint32_t getAvailableBuffers() const;
    uint32_t getIdleBuffers() const;
    uint32_t getBusyBuffers() const;

    std::vector<std::unique_ptr<GraBuffer>> requestBuffers(uint32_t num);
    int releaseBuffer(std::unique_ptr<GraBuffer> buffer);
    int reclaim();

    std::string toString() const;

private:
    std::unique_ptr<GraBuffer> requestBuffer();
    uint32_t adjustedCap() const;
    int reclaimImpl();

    bool mReclaim;
    // NOTE: now we assume the BufferManager must be a member of a Stream,
    // so the reference is always alive with the BufferManager object
    const Stream &mStream;

    uint32_t mBufferCapacity;
    std::set<uint64_t> mBusyBuffers;
    std::map<uint64_t, std::unique_ptr<GraBuffer>> mIdleBuffers;
};

} // namespace mihal

#endif // __BUFFER_MANAGER_H__
