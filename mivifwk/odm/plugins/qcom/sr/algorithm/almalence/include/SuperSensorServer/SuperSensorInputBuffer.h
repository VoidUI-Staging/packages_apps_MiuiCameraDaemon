#ifndef __SUPERSENSOR_INPUT_BUFFER_H__
#define __SUPERSENSOR_INPUT_BUFFER_H__

#include <SuperSensorServer/Exposure.h>
#include <SuperSensorServer/Rect.h>
#include <SuperSensorServer/Size.h>
#include <SuperSensorServer/YuvImage.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace almalence {

/**
 * SuperSensor input buffer class.
 * It's responsible for allocating and managing a buffer for max amount of frames.
 *
 * Note: zoom and base exposure can only be modified when the buffer is empty or exception
 * will be thrown.
 */
class SuperSensorInputBuffer
{
private:
    const std::function<void *(std::uint32_t)> allocator;
    const std::function<void(void *)> deleter;
    const std::function<std::uint32_t(std::uint32_t)> functionStride;
    const std::function<std::uint32_t(Size)> functionImageSize;
    const std::function<std::uint32_t(Size, std::uint32_t)> functionBufferSize;
    const Size sensorSize;
    const std::uint32_t capacity;
    const bool externalAllowed;

    const std::unique_ptr<uint8_t[], std::function<void(void *)>> buffer;
    std::uint32_t bufferCount;

    Rect crop;         // crop on original image - allways inside - numbers are positive
    Rect cropBordered; // crop with outer border (64) - upper left side can be negative
    Exposure exposure;
    const std::uint32_t sizeGuaranteeBorder;

    std::vector<std::pair<std::uint8_t *, std::uint8_t *>> bufferPointers;

public:
    SuperSensorInputBuffer(
        const std::function<void *(std::uint32_t)> allocator,
        const std::function<void(void *)> deleter,
        const std::function<std::uint32_t(std::uint32_t)> functionStride,
        const std::function<std::uint32_t(Size)> functionImageSize,
        const std::function<std::uint32_t(Size, std::uint32_t)> functionBufferSize,
        const Size sensorSize, const std::uint32_t capacity, const bool externalAllowed,
        const std::uint32_t sizeGuaranteeBorder);

    virtual ~SuperSensorInputBuffer();

    std::uint32_t getCapacity() const;

    bool isEmpty() const;

    bool isFull() const;

    void setCropRect(const Rect cropRect);

    void setZoom(const double zoom);

    void setBaseExposure(const Exposure &exposure);

    std::tuple<std::vector<std::pair<uint8_t *, uint8_t *>>, Rect, Exposure> flush();

    void addFrame(const YuvImage<Yuv888> &image);

    void addFrame(const YuvImage<Yuv161616> &image);
};

} // namespace almalence

#endif
