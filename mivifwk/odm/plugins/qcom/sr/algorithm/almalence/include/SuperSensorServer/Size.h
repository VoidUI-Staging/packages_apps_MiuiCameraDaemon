#ifndef __ALMALENCE_SIZE__
#define __ALMALENCE_SIZE__

#include <cstdint>
#include <string>

namespace almalence {

/**
 * Class representing composition of width and height.
 */
class Size
{
private:
    std::uint32_t _width;
    std::uint32_t _height;

public:
    /**
     * Creates Size instance based on the provided width and height.
     * @param width The width.
     * @param height The height.
     */
    inline Size(const std::uint32_t width, const std::uint32_t height)
        : _width(width), _height(height)
    {
    }

    /**
     * Creates an empty Size instance.
     */
    inline Size() : Size(0, 0) {}

    /**
     * Copies the provided Size instance.
     * @param other Size instance to be copied.
     */
    inline Size(const Size &other) { this->set(other.width(), other.height()); }

    virtual inline ~Size() {}

    /**
     * @return The width.
     */
    inline std::uint32_t width() const { return this->_width; }

    /**
     * @return The height.
     */
    inline std::uint32_t height() const { return this->_height; }

    /**
     * @return Composition of width and height.
     */
    inline std::uint32_t size() const { return this->width() * this->height(); }

    /**
     * Sets width.
     * @param width Width value to be set.
     */
    inline void setWidth(const std::uint32_t width) { this->_width = width; }

    /**
     * Sets height.
     * @param height Height value to be set.
     */
    inline void setHeight(const std::uint32_t height) { this->_height = height; }

    /**
     * Sets width and height.
     * @param width Width value to be set.
     * @param height Height value to be set.
     */
    virtual inline void set(const std::uint32_t width, const std::uint32_t height)
    {
        this->_width = width;
        this->_height = height;
    }

    /**
     * Checks if this Size instance is equal to the provided one.
     */
    virtual inline bool operator==(const Size &other) const
    {
        return other.width() == this->_width && other.height() == this->_height;
    }

    /**
     * Checks if this Size instance is unequal to the provided one.
     */
    virtual inline bool operator!=(const Size &other) const { return !(other == *this); }
};

} // namespace almalence

#endif
