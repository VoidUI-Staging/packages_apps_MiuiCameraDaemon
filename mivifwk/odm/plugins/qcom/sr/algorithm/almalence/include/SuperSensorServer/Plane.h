#ifndef __PLANE_H__
#define __PLANE_H__

#include <SuperSensorServer/Rect.h>
#include <SuperSensorServer/Size.h>

namespace almalence {

/**
 * Universal plane class storing data pointer and item & row strides.
 */
template <typename T>
struct Plane
{
    T *data;

    std::uint32_t itemStride;
    std::uint32_t rowStride;

    /**
     * Creates an empty Plane instance.
     */
    Plane() {}

    /**
     * Creates Plane instance based on the provided data pointer and item & row strides.
     * @param data Data pointer.
     * @param itemStride Item stride.
     * @param rowStride Row stride.
     */
    Plane(T *const data, const std::uint32_t itemStride, const std::uint32_t rowStride)
        : data(data), itemStride(itemStride), rowStride(rowStride)
    {
    }

    /**
     * Copies the provided Plane instance. Only copies pointers, not the data.
     * @param other Plane to be copied.
     */
    Plane(const Plane &other) { *this = other; }

    /**
     * Assigns values of the provided Plane to this.
     * @param other Plane values of which shall be assigned to this.
     * @return Plane instance being assigned to.
     */
    Plane &operator=(const Plane &other)
    {
        this->data = other.data;
        this->itemStride = other.itemStride;
        this->rowStride = other.rowStride;
        return *this;
    }

    virtual ~Plane() {}

    /**
     * @return Item stride.
     */
    inline std::uint32_t getItemStride() const { return this->itemStride; }

    /**
     * @return Row stride.
     */
    inline std::uint32_t getRowStride() const { return this->rowStride; }

    /**
     * @return Direct data pointer.
     */
    inline T *getData() const { return this->data; }

    /**
     * @param index Item index.
     * @return Constant item reference at the specified index.
     */
    inline const T operator[](const std::uint32_t index) const { return this->data[index]; }

    /**
     * @param index Item index.
     * @return Item reference at the specified index.
     */
    inline T &operator[](const std::uint32_t index) { return this->data[index]; }

    /**
     * @param x Coordinate X.
     * @param y Coordinate Y.
     * @return Constant item reference at the specified coordinates.
     */
    inline const T &at(const std::uint32_t x, const std::uint32_t y) const
    {
        return this->data[y * this->rowStride + x * this->itemStride];
    }

    /**
     * @param x Coordinate X.
     * @param y Coordinate Y.
     * @return Item reference at the specified coordinates.
     */
    inline T &at(const std::uint32_t x, const std::uint32_t y)
    {
        return this->data[y * this->rowStride + x * this->itemStride];
    }

    /**
     * Copies a row of data from the plane to the destination buffer.
     * @param destination Destination address.
     * @param destinationItemStride Destination item stride.
     * @param length Item count.
     * @param sourceX Source X coordinate.
     * @param sourceY Source Y coordinate.
     */
    inline void getLine(std::uint8_t *const destination, const std::uint32_t destinationItemStride,
                        const std::uint32_t length, const std::uint32_t sourceX,
                        const std::uint32_t sourceY) const
    {
        if (this->itemStride == 1 && destinationItemStride == 1) {
            memcpy(destination, this->data + sourceY * this->rowStride + sourceX * this->itemStride,
                   this->itemStride * length);
        } else {
            for (std::uint32_t i = 0; i < length; i++) {
                destination[i * destinationItemStride] =
                    this->data[sourceY * this->rowStride + (sourceX + i) * this->itemStride];
            }
        }
    }
};

} // namespace almalence

#endif
