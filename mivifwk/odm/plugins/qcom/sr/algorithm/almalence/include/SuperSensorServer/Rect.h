#ifndef __ALMALENCE_RECT__
#define __ALMALENCE_RECT__

#include <SuperSensorServer/Size.h>

#include <stdexcept>

namespace almalence {

class Rect : public Size
{
private:
    int _left;
    int _top;

    using Size::set;

public:
    inline Rect(const int left, const int top, const std::uint32_t width,
                const std::uint32_t height)
        : Size(width, height), _left(left), _top(top)
    {
    }

    inline Rect(const Size &size, const int left, const int top)
        : Size(size), _left(left), _top(top)
    {
    }

    inline Rect() : Rect(0, 0, 0, 0) {}

    inline Rect(const Rect &other)
    {
        this->set(other._left, other._top, other.width(), other.height());
    }

    virtual inline ~Rect() {}

    inline int left() const { return this->_left; }

    inline int top() const { return this->_top; }

    inline int right() const { return this->_left + (int)this->width(); }

    inline int bottom() const { return this->_top + (int)this->height(); }

    inline void setLeft(const int left) { this->_left = left; }

    inline void setTop(const int top) { this->_top = top; }

    inline void setRight(const int right)
    {
        if (right < this->_left) {
            this->_left = right;
            this->setWidth(0);
        } else {
            this->setWidth(std::uint32_t(right - this->_left));
        }
    }

    inline void setBottom(const int bottom)
    {
        if (bottom < this->_top) {
            this->_top = bottom;
            this->setHeight(0);
        } else {
            this->setHeight(std::uint32_t(bottom - this->_top));
        }
    }

    virtual inline void set(const int x, const int y, const std::uint32_t w, const std::uint32_t h)
    {
        this->setLeft(x);
        this->setTop(y);
        this->setWidth(w);
        this->setHeight(h);
    }

    inline bool checkBounds(const std::uint32_t width, const std::uint32_t height) const
    {
        return this->left() >= 0 && this->top() >= 0 && std::uint32_t(this->right()) <= width &&
               std::uint32_t(this->bottom()) <= height;
    }

    inline bool checkBounds(const Size &size) const
    {
        return checkBounds(size.width(), size.height());
    }

    virtual inline bool operator==(const Rect &other) const
    {
        return other.left() == this->left() && other.top() == this->top() &&
               other.width() == this->width() && other.height() == this->height();
    }

    virtual inline bool operator==(const Rect &other)
    {
        return other.left() == this->left() && other.top() == this->top() &&
               other.width() == this->width() && other.height() == this->height();
    }

    virtual inline bool operator!=(const Rect &other) const { return !(other == *this); }

    virtual inline bool operator!=(const Rect &other) { return !(other == *this); }
};

} // namespace almalence

#endif
