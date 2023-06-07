#ifndef __YUV161616_H__
#define __YUV161616_H__

#include <SuperSensorServer/Plane.h>
#include <string.h>

#include <cmath>

namespace almalence {

/**
 * Class representing YUV 161616 image in form of 3 separate planes.
 */
class Yuv161616
{
public:
    Plane<uint16_t> Y;
    Plane<uint16_t> U;
    Plane<uint16_t> V;

    /**
     * Creates empty Yuv161616 instance.
     */
    inline Yuv161616() {}

    /**
     * Creates Yuv161616 instance based on the provided Plane instances.
     */
    inline Yuv161616(const Plane<uint16_t> &Y, const Plane<uint16_t> &U, const Plane<uint16_t> &V)
        : Y(Y), U(U), V(V)
    {
    }

    /**
     * Copies Yuv161616 instance. Only copies pointers, not the data.
     * @param other Yuv161616 instance to be copied.
     */
    inline Yuv161616(const Yuv161616 &other) { *this = other; }

    virtual inline ~Yuv161616() {}

    /**
     * Basic assignment operator. Only copies pointers, not the data.
     * @param other Yuv161616 instance to be assigned from.
     * @return The assigned instance reference.
     */
    virtual inline Yuv161616 &operator=(const Yuv161616 &other)
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
    inline bool getLineUV(std::uint16_t *const destination, const uint32_t length,
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

#endif //__YUV161616_H__
