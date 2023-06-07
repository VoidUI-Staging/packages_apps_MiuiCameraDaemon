#ifndef __YUV888_H__
#define __YUV888_H__

#include <SuperSensorServer/Plane.h>
#include <string.h>

#include <cmath>

namespace almalence {

/**
 * Class representing YUV 888 image in form of 3 separate planes.
 */
class Yuv888
{
public:
    Plane<uint8_t> Y;
    Plane<uint8_t> U;
    Plane<uint8_t> V;

    /**
     * Creates empty Yuv888 instance.
     */
    inline Yuv888() {}

    /**
     * Creates Yuv888 instance based on the provided Plane instances.
     */
    inline Yuv888(const Plane<uint8_t> &Y, const Plane<uint8_t> &U, const Plane<uint8_t> &V)
        : Y(Y), U(U), V(V)
    {
    }

    /**
     * Copies Yuv888 instance. Only copies pointers, not the data.
     * @param other Yuv888 instance to be copied.
     */
    inline Yuv888(const Yuv888 &other) { *this = other; }

    virtual inline ~Yuv888() {}

    /**
     * Basic assignment operator. Only copies pointers, not the data.
     * @param other Yuv888 instance to be assigned from.
     * @return The assigned instance reference.
     */
    virtual inline Yuv888 &operator=(const Yuv888 &other)
    {
        this->Y = other.Y;
        this->U = other.U;
        this->V = other.V;
        return *this;
    }

    /**
     * Checks if U and V planes are interleaved.
     * @return True if U and V planes are interleaved and False otherwise.
     */
    inline bool isUvInterleaved() const
    {
        return this->U.itemStride == 2 && this->V.itemStride == 2 &&
               std::max(std::uint64_t(this->V.data), std::uint64_t(this->U.data)) -
                       std::min(std::uint64_t(this->V.data), std::uint64_t(this->U.data)) ==
                   1;
    }

    /**
     * Copies row from UV planes to the destination buffer.
     * @param destination Destination address.
     * @param length Item count.
     * @param sourceX Source X coordinate.
     * @param sourceY Source Y coordinate.
     * @return True is succeeded and False otherwise.
     */
    inline bool getLineUV(std::uint8_t *const destination, const uint32_t length,
                          const uint32_t sourceX, const uint32_t sourceY) const
    {
        if (this->isUvInterleaved()) {
            memcpy(destination,
                   std::min(this->U.data, this->V.data) + sourceY * this->U.rowStride + sourceX,
                   length * 2);
            return true;
        } else {
            return false;
        }
    }
};

} // namespace almalence

#endif //__YUV888_H__
