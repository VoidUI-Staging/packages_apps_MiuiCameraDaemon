#ifndef __SUPER_SENSOR_UTIL_H__
#define __SUPER_SENSOR_UTIL_H__

#include <SuperSensorServer/Rect.h>
#include <SuperSensorServer/Size.h>
#include <SuperSensorServer/Yuv888.h>
#include <SuperSensorServer/common.h>
#include <almashot/hvx/superzoom.h>
#include <android/log.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>

#ifndef SIZE_GUARANTEE_BORDER
#define SIZE_GUARANTEE_BORDER 64
#endif

#define MAX_STAB        48
#define MAX_CONFIG_SIZE 100

#define GET_IMGSIZE(stride, sy) ((((stride * sy * 3) >> 1) + 0xfff) & -0x1000)
#define STRIDE(sx)              (((sx) + 127) & -128)

namespace almalence {

const std::uint32_t SUPERSENSOR_RT_FRAMES_SKIP = std::uint32_t(2);

using FrameNumberFunction = std::function<std::uint32_t(std::uint32_t)>;

const FrameNumberFunction DEFAULT_FRAME_NUMBER_FUNCTION = [](std::uint32_t size) -> std::uint32_t {
    if (size >= 1440 * 1080) {
        return 4U;
    }

    return std::min(12U, 1U + 4947968U / size);
};

static inline std::uint32_t bordered(const std::uint32_t x, const std::uint32_t sizeGuaranteeBorder)
{
    return x + 2 * sizeGuaranteeBorder;
}

static inline Size bordered(const Size size, const std::uint32_t sizeGuaranteeBorder)
{
    return Size(bordered(size.width(), sizeGuaranteeBorder),
                bordered(size.height(), sizeGuaranteeBorder));
}

const std::uint32_t CROP_MIN = 64;

std::uint32_t getFrameBufferSize(const Size &size, const std::uint32_t count);

std::uint32_t getFrameLengthWithStride(const Size &sizeMax, const std::uint32_t count);

/*
 * This function is intended to calculate maximum size of required circullar buffer size at any
 * given moment. To acomplish that we have to iterate over all image counts and sizes and find the
 * largest multiplication: FRAME_COUNT * FRAME_SIZE.
 */
std::uint32_t solveCircularBufferSize(Size sizeMax, FrameNumberFunction frameNumberFunction);

double getExposureTimeDiff(uint64_t expRef, uint64_t expTest);

double getISODiff(uint32_t isoRef, uint32_t isoTest);

void cropYuv888(const Yuv888 image, uint8_t *__restrict dstY, uint8_t *__restrict dstUV,
                unsigned crop_x, unsigned crop_y, unsigned sxo, unsigned syo, unsigned ostride);

Rect calculateBorderedCrop(const Size inputSizeMax, const Rect inputCrop,
                           const std::uint32_t sizeGuaranteeBorder);

void extend_bordersNV21(uint8_t *in, int sx, int sy, unsigned stride, int sizeOfBorder);

int getConfigLength(const alma_config_t *config);
void copyConfig(const alma_config_t *srcConfig, alma_config_t *dstConfig, int dstOffset);
void releaseCopiedConfigArryas(const alma_config_t *config);
} // namespace almalence

#endif // __SUPER_SENSOR_UTIL_H__
