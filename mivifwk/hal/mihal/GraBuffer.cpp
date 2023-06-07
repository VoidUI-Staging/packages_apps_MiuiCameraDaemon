#include "GraBuffer.h"

#include <gralloctypes/Gralloc4.h>
#include <ui/GraphicBuffer.h>

#include <algorithm>

#include "LogUtil.h"
#include "MiGrallocMapper.h"
#include "Stream.h"

using ::aidl::android::hardware::graphics::common::ExtendableType;
using aidl::android::hardware::graphics::common::PlaneLayout;
using android::GraphicBuffer;
using android::GraphicBufferMapper;

namespace mihal {

namespace {
GraphicBufferMapper &mapper = GraphicBufferMapper::getInstance();
}

buffer_handle_t GraBuffer::mEmptyBuffer = nullptr;
uint64_t GraBuffer::mTotalIonMem = 0;
std::mutex GraBuffer::mMutex;

GraBuffer::GraBuffer(uint32_t width, uint32_t height, int format, uint64_t usage, std::string name)
    : mImpl{new GraphicBuffer{width, height, format, 1, usage, name}}, mAllocated{true}
{
    buffer_handle_t handle = getHandle();
    MASSERT(handle);

    uint64_t allocSize;
    mapper.getAllocationSize(handle, &allocSize);
    mAllocSize = allocSize;

    std::unique_lock<std::mutex> locker{mMutex};
    mTotalIonMem += allocSize;
    locker.unlock();

    MLOGI(LogGroupRT,
          "[GraphicBufferInfo]: totalSize: %llu M, allocSize:%llu M, w*h : %d * %d, format : 0x%x",
          mTotalIonMem / 1024 / 1024, mAllocSize / 1024 / 1024, width, height, format);
    MICAM_TRACE_INT64_F(MialgoTraceMemProfile, mTotalIonMem, "%s", "mihal_Ion_mem");
}

GraBuffer::GraBuffer(const Stream *stream, std::string name)
    : mImpl{new GraphicBuffer{stream->getWidth(), stream->getHeight(), stream->getFormat(), 1,
                              stream->getUsage(), name}},
      mAllocated{true}
{
    buffer_handle_t handle = getHandle();
    MASSERT(handle);

    uint64_t allocSize;
    mapper.getAllocationSize(handle, &allocSize);
    mAllocSize = allocSize;

    std::unique_lock<std::mutex> locker{mMutex};
    mTotalIonMem += allocSize;
    locker.unlock();
    MLOGI(LogGroupRT,
          "[GraphicBufferInfo]: totalSize: %llu M, allocSize:%llu M, w*h : %d * %d ,format : 0x%x",
          mTotalIonMem / 1024 / 1024, mAllocSize / 1024 / 1024, stream->getWidth(),
          stream->getHeight(), stream->getFormat());
    MICAM_TRACE_INT64_F(MialgoTraceMemProfile, mTotalIonMem, "%s", "mihal_ion_mem");
}

GraBuffer::GraBuffer(uint32_t width, uint32_t height, int format, uint64_t usage,
                     const buffer_handle_t handle)
    : mImpl{new GraphicBuffer{handle, GraphicBuffer::WRAP_HANDLE, width, height, format, 1, usage,
                              0}},
      mAllocated{false},
      mAllocSize{0}
{
}

GraBuffer::GraBuffer(const Stream *stream, const buffer_handle_t handle)
    : mImpl{new GraphicBuffer{handle, GraphicBuffer::WRAP_HANDLE, stream->getWidth(),
                              stream->getHeight(), stream->getFormat(), 1, stream->getUsage(), 0}},
      mAllocated{false},
      mAllocSize{0}
{
}

GraBuffer::~GraBuffer()
{
    if (mAllocated) {
        std::unique_lock<std::mutex> locker{mMutex};
        mTotalIonMem -= mAllocSize;
        locker.unlock();
        MLOGI(LogGroupRT, "[GraphicBufferInfo]: realse buffer size: %llu M, total size:%llu M",
              mAllocSize / 1024 / 1024, mTotalIonMem / 1024 / 1024);
        MICAM_TRACE_INT64_F(MialgoTraceMemProfile, mTotalIonMem, "%s", "mihal_ion_mem");
    }

    mAllocSize = 0;
}

int GraBuffer::initCheck() const
{
    if (mImpl)
        return mImpl->initCheck();
    else
        return -ENOMEM;
}

buffer_handle_t GraBuffer::getHandle() const
{
    if (!mImpl)
        return nullptr;

    return mImpl->getNativeBuffer()->handle;
}

buffer_handle_t *GraBuffer::getHandlePtr() const
{
    if (!mImpl)
        return nullptr;

    // XXX: is this right? do we need to save the ptr in the ctor?
    return &mImpl->getNativeBuffer()->handle;
}

int GraBuffer::getFormat() const
{
    return mImpl->getPixelFormat();
}

uint32_t GraBuffer::getWidth() const
{
    return mImpl->getWidth();
}

uint32_t GraBuffer::getHeight() const
{
    return mImpl->getHeight();
}

uint32_t GraBuffer::getStride() const
{
    return mImpl->getStride();
}

uint64_t GraBuffer::getUsage() const
{
    return mImpl->getUsage();
}

uint64_t GraBuffer::getId() const
{
    return mImpl->getId();
}

bool GraBuffer::operator<(const GraBuffer &other)
{
    return getId() < other.getId();
}

int GraBuffer::processData(AccessFunc func)
{
    buffer_handle_t handle = getHandle();
    MASSERT(handle);

    uint64_t w, h;
    mapper.getWidth(handle, &w);
    mapper.getHeight(handle, &h);

    // FIXME:NOTE: the lenght should be coverted according to the buffer format, jpeg/yuv/...
    int format = getFormat();
    size_t len = w * h;

    switch (format) {
    case HAL_PIXEL_FORMAT_YCBCR_420_888:
    case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
        uint64_t allocSize;
        mapper.getAllocationSize(handle, &allocSize);
        len = allocSize;
        break;
    case HAL_PIXEL_FORMAT_BLOB:
        len = w * h;
        break;
    default:
        break;
    }

    int fd = handle->data[0];
    uint8_t *data =
        static_cast<uint8_t *>(mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (!data)
        return -ENOMEM;

    int ret = func(data, len);
    munmap(data, len);

    return ret;
}

std::string GraBuffer::toString() const
{
    std::string str;
    // mapper.dumpBuffer(getHandle(), str, true);

    return str;
}

// NOTE: util function
void getPlaneWithSpecificType(const std::vector<PlaneLayout> &planeLayouts,
                              ExtendableType targetType, size_t &index)
{
    auto ptr =
        std::find_if(planeLayouts.begin(), planeLayouts.end(), [&](const PlaneLayout &planeLayout) {
            for (auto &component : planeLayout.components) {
                if (component.type == targetType) {
                    return true;
                }
            }
            return false;
        });

    index = ptr - planeLayouts.begin();
}

uint64_t GraBuffer::getYplaneOffset()
{
    if (mPlaneLayouts.empty()) {
        getYUVplaneInfo();
    }

    size_t index;
    getPlaneWithSpecificType(mPlaneLayouts, android::gralloc4::PlaneLayoutComponentType_Y, index);

    if (index == mPlaneLayouts.size()) {
        MLOGE(LogGroupHAL, "getYplane fail!");
        return -1;
    }

    return mPlaneLayouts[index].offsetInBytes;
}

uint64_t GraBuffer::getUVplaneOffset()
{
    if (mPlaneLayouts.empty()) {
        getYUVplaneInfo();
    }

    size_t index;
    getPlaneWithSpecificType(mPlaneLayouts, android::gralloc4::PlaneLayoutComponentType_CB, index);

    if (index == mPlaneLayouts.size()) {
        MLOGE(LogGroupHAL, "getUVplane fail!");
        return -1;
    }

    return mPlaneLayouts[index].offsetInBytes;
}

uint32_t GraBuffer::getYstrideBytes()
{
    if (mPlaneLayouts.empty()) {
        getYUVplaneInfo();
    }

    size_t index;
    getPlaneWithSpecificType(mPlaneLayouts, android::gralloc4::PlaneLayoutComponentType_Y, index);

    if (index == mPlaneLayouts.size()) {
        MLOGE(LogGroupHAL, "getYplane fail!");
        return -1;
    }

    return mPlaneLayouts[index].strideInBytes;
}

uint32_t GraBuffer::getUVstrideBytes()
{
    if (mPlaneLayouts.empty()) {
        getYUVplaneInfo();
    }

    size_t index;
    getPlaneWithSpecificType(mPlaneLayouts, android::gralloc4::PlaneLayoutComponentType_CB, index);

    if (index == mPlaneLayouts.size()) {
        MLOGE(LogGroupHAL, "getUVplane fail!");
        return -1;
    }

    return mPlaneLayouts[index].strideInBytes;
}

int GraBuffer::getYUVplaneInfo()
{
    if (mPlaneLayouts.empty()) {
        auto status = mapper.getPlaneLayouts(getHandle(), &mPlaneLayouts);

        if (status != 0) {
            MLOGE(LogGroupHAL, "get YUV plane info fail!");
            return -EINVAL;
        }
    }

    return 0;
}

} // namespace mihal
