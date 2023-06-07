#ifndef __GRA_BUFFER_H__
#define __GRA_BUFFER_H__

#include <cutils/native_handle.h>
#include <ui/GraphicBuffer.h>
#include <utils/StrongPointer.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace android {
class GraphicBuffer;
}

namespace mihal {

class Stream;
class GraBuffer
{
public:
    using AccessFunc = std::function<int(uint8_t *data, size_t size)>;

    GraBuffer() = delete;
    GraBuffer(const GraBuffer &other) = delete;
    GraBuffer(const GraBuffer &&other) = delete;
    GraBuffer &operator=(const GraBuffer &rhs) = delete;
    GraBuffer &operator=(const GraBuffer &&rhs) = delete;

    GraBuffer(uint32_t width, uint32_t height, int format, uint64_t usage, std::string name);
    GraBuffer(const Stream *stream, std::string name);
    GraBuffer(uint32_t width, uint32_t height, int format, uint64_t usage,
              const buffer_handle_t handle);
    GraBuffer(const Stream *stream, const buffer_handle_t handle);
    ~GraBuffer();

    int initCheck() const;
    buffer_handle_t getHandle() const;
    buffer_handle_t *getHandlePtr() const;
    static buffer_handle_t *getEmptyHandlePtr() { return &mEmptyBuffer; }

    int getFormat() const;
    uint32_t getWidth() const;
    uint32_t getHeight() const;
    uint32_t getStride() const;
    uint64_t getUsage() const;
    uint64_t getId() const;
    int processData(AccessFunc func);

    std::string toString() const;
    // void *getVirtualAddress() const;

    bool operator<(const GraBuffer &other);

    // NOTE: get the start point offset of Y plane
    uint64_t getYplaneOffset();
    // NOTE: get the start point offset of UV plane
    uint64_t getUVplaneOffset();
    // NOTE: don't mix strideBytes with stride, basically strideBytes is aligned stride
    uint32_t getYstrideBytes();
    uint32_t getUVstrideBytes();

private:
    int getYUVplaneInfo();

    static buffer_handle_t mEmptyBuffer;
    android::sp<android::GraphicBuffer> mImpl;
    bool mAllocated;
    uint64_t mAllocSize;

    static uint64_t mTotalIonMem;
    static std::mutex mMutex;

    // NOTE: yuv layout info or RGB layout info
    std::vector<aidl::android::hardware::graphics::common::PlaneLayout> mPlaneLayouts;
};

} // namespace mihal

#endif // __GRA_BUFFER_H__
