#ifndef __IMAGEDUMPER_H__
#define __IMAGEDUMPER_H__

#include <SuperSensorServer/Dumper.h>
#include <SuperSensorServer/Size.h>

#include <cstdint>
#include <mutex>
#include <sstream>

namespace almalence {

class ImageDumper
{
private:
    const std::uint32_t minCropSize;
    const std::uint32_t maxCropSize;

    Dumper dumper;
    Dumper dumperOut;

    std::mutex mutex;
    bool dumpingEnabled;
    bool dumpingStop;
    std::string dumpingPath;

public:
    ImageDumper(const std::uint32_t width, const std::uint32_t height,
                const std::uint32_t sizeGuaranteeBorder);

    virtual ~ImageDumper();

    void start();

    void doDumpingInput(std::uint8_t *const yuv, const Size &size, const std::uint32_t index,
                        const int iso);

    void doDumpingOutput(std::uint8_t *const yuvOut, const std::uint32_t width,
                         const std::uint32_t height, const std::uint32_t stride,
                         const std::uint32_t frameCount, const std::uint32_t refIndex,
                         const bool superRes);

    bool finish(const bool onlyIfFull);
};

} // namespace almalence

#endif // __IMAGEDUMPER_H__
