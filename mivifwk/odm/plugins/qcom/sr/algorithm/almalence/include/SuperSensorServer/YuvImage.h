#ifndef __ALMALENCE_YUVIMAGE_H__
#define __ALMALENCE_YUVIMAGE_H__

#include <SuperSensorServer/Exposure.h>
#include <SuperSensorServer/Yuv161616.h>
#include <SuperSensorServer/Yuv888.h>

namespace almalence {

/**
 * Template class representing a composition of YUV and Exposure instances.
 */
template <class YUV>
class YuvImage
{
private:
    YUV image;
    Exposure exposure;

public:
    /**
     * Creates empty YuvImage instance.
     */
    YuvImage() {}

    /**
     * Copies YuvImage instance. Only copies pointers, not the data.
     * @param other YuvImage instance to be copied.
     */
    YuvImage(const YuvImage &other) { *this = other; }
    /**
     * Creates YuvImage instance based on the provided YUV and Exposure instances.
     * @param image
     * @param exposure
     */
    YuvImage(const YUV &image, const Exposure &exposure) : image(image), exposure(exposure) {}

    ~YuvImage() {}

    /**
     * Basic assignment operator. Only copies pointers, not the data.
     * @param other YuvImage instance to be assigned from.
     * @return The assigned instance reference.
     */
    YuvImage &operator=(const YuvImage &other)
    {
        this->image = other.image;
        this->exposure = other.exposure;
        return *this;
    }
    /**
     * @return YUV instance.
     */
    const YUV &getImage() const { return this->image; }

    /**
     * @return Exposures instance.
     */
    const Exposure &getExposure() const;

    /**
     * @return Sensitivity (ISO) of the image.
     */
    std::uint32_t getSensitivity() const;

    /**
     * @return Exposure time of the image.
     */
    std::uint64_t getExposureTime() const;
};

} // namespace almalence

#endif
