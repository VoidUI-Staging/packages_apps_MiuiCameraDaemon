// Copyright 2017 XiaoMi. All Rights Reserved.
// Author: pengyongyi@xiaomi.com (Yongyi PENG)

#ifndef BOKEH_BOKEH_BASE_H_
#define BOKEH_BOKEH_BASE_H_

#include <math.h>
#include <stdio.h>
#include <time.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace bokeh {
static const uint8_t kMaskBits = 8;
static const uint16_t kMaskLevels = 1 << kMaskBits;
static const uint8_t kMaskMax = kMaskLevels - 1;
static const float kMaskMaxF = kMaskMax;

enum class PixelFormat {
    NV21 = 0,
    RGB888 = 1,
    YUYV = 2,
    NV12 = 3,
    YV12 = 4,
};

enum class Orientation : int32_t {
    NORMAL = 0,
    CLOCKWISE_90 = 1,
    CLOCKWISE_180 = 2,
    CLOCKWISE_270 = 3,
    ERROR = 4
};

enum class BlurSpotShape {
    CIRCLE = 0,
    HEART = 1,
    STAR = 2,
    MICKEY = 3,
};

typedef struct _MIBUFFER
{
    PixelFormat pixelArrayFormat;
    uint32_t width;
    uint32_t height;
    uint8_t *plane[4];
    uint32_t pitch[4];
} MiBuffer, *MiBufferPtr;

template <typename T, int N>
struct Pixel
{
    typedef T type;
    static const int n = N;
    T d_[N];

    Pixel() { memset(d_, 0, sizeof(T) * N); }

    template <typename T2>
    inline void add(const Pixel<T2, N> &rhs)
    {
        for (int i = 0; i < N; i++)
            d_[i] += rhs.d_[i];
    }

    template <typename T2>
    inline void subtract(const Pixel<T2, N> &rhs)
    {
        for (int i = 0; i < N; i++)
            d_[i] -= rhs.d_[i];
    }

    inline void multiply(float mul)
    {
        for (int i = 0; i < N; i++)
            d_[i] *= mul;
    }

    template <typename T2>
    inline void set(const Pixel<T2, N> &rhs)
    {
        for (int i = 0; i < N; i++)
            d_[i] = rhs.d_[i];
    }

    template <typename T2>
    inline void set(const Pixel<T2, N> &rhs, std::function<float(float)> trans)
    {
        for (int i = 0; i < N; i++)
            d_[i] = trans(rhs.d_[i]);
    }
};

typedef Pixel<uint8_t, 3> Pixel3i;
typedef Pixel<float, 3> Pixel3f;

struct Rect
{
    Rect() : h_(0), w_(0) {}
    Rect(int h, int w) : h_(h), w_(w) {}

    inline int At(int row, int col) const { return row * w_ + col; }

    inline int Row(int row) const { return row * w_; }

    inline int PixelCount() const { return h_ * w_; }

    Rect Transpose() const { return Rect(w_, h_); }

    bool IsPortrait() const { return w_ < h_; }

    void Scale(float scale)
    {
        w_ = int(roundf(w_ * scale)) / 4 * 4;
        h_ = int(roundf(h_ * scale)) / 4 * 4;
    }

    bool operator==(const Rect &rect) const { return h_ == rect.h_ && w_ == rect.w_; }
    bool operator!=(const Rect &rect) const { return !(*this == rect); }
    std::string ToString() const
    {
        char rectText[50];
        snprintf(rectText, 50, "Rect[w%d*h%d]", w_, h_);
        return std::string(rectText);
    }
    int h_;
    int w_;
};
} // namespace bokeh
#endif // BOKEH_BOKEH_BASE_H_
