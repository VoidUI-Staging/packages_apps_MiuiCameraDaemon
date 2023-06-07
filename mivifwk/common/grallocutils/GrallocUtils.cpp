#include "GrallocUtils.h"

#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <utils/StrongPointer.h>

using android::GraphicBuffer;
using android::GraphicBufferMapper;
using android::sp;

namespace {
GraphicBufferMapper &mapper = GraphicBufferMapper::getInstance();
}

struct GraphicBufferWrapper
{
    GraphicBufferWrapper(sp<GraphicBuffer> b);
    sp<GraphicBuffer> buffer;
};

GraphicBufferWrapper::GraphicBufferWrapper(sp<GraphicBuffer> b) : buffer(b) {}

sp<GraphicBuffer> allocateGraphicBuffer(uint32_t width, uint32_t height, int format, uint64_t usage)
{
    return new GraphicBuffer(width, height, format, 1, usage);
}

sp<GraphicBuffer> createGraphicBufferFromHandle(uint32_t width, uint32_t height, int format,
                                                uint64_t usage, uint32_t stride,
                                                const buffer_handle_t handle)
{
    return new GraphicBuffer(handle, GraphicBuffer::WRAP_HANDLE, width, height, format, 0, usage,
                             stride);
}

void GrallocUtils::allocateGrallocBuffer(uint32_t width, uint32_t height, int format,
                                         uint64_t usage, GrallocBufHandle *handle, uint32_t *stride)
{
    sp<GraphicBuffer> gb = allocateGraphicBuffer(width, height, format, usage);
    handle->backend = new GraphicBufferWrapper(gb);
    handle->nativeHandle = gb->getNativeBuffer()->handle;
    *stride = gb->getStride();
}

void GrallocUtils::createGrallocBuffer(uint32_t width, uint32_t height, int format, uint64_t usage,
                                       uint32_t stride, const buffer_handle_t buffer,
                                       GrallocBufHandle *handle)
{
    MI_ASSERT(handle);
    sp<GraphicBuffer> gb =
        createGraphicBufferFromHandle(width, height, format, usage, stride, buffer);
    handle->backend = new GraphicBufferWrapper(gb);
    handle->nativeHandle = gb->getNativeBuffer()->handle;
}

void GrallocUtils::freeGrallocBuffer(GrallocBufHandle *handle)
{
    MI_ASSERT(handle);
    MI_ASSERT(handle->backend);
    delete static_cast<GraphicBufferWrapper *>(handle->backend);
    handle->backend = NULL;
}

void GrallocUtils::lockGrallocBuffer(GrallocBufHandle *handle, uint64_t usage, void **vaddr)
{
    MI_ASSERT(handle);
    MI_ASSERT(handle->backend);
    GraphicBufferWrapper *wrapper = static_cast<GraphicBufferWrapper *>(handle->backend);
    wrapper->buffer->lock(usage, vaddr);
}

void GrallocUtils::GrallocUtils::unlockGrallocBuffer(GrallocBufHandle *handle)
{
    MI_ASSERT(handle);
    MI_ASSERT(handle->backend);
    GraphicBufferWrapper *wrapper = static_cast<GraphicBufferWrapper *>(handle->backend);
    wrapper->buffer->unlock();
}

void GrallocUtils::getGrallocBufferSize(buffer_handle_t bufferHandle, uint64_t *outAllocationSize)
{
    MI_ASSERT(bufferHandle);
    mapper.getAllocationSize(bufferHandle, outAllocationSize);
}
