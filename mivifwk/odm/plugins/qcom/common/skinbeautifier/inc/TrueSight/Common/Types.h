#pragma once
#include <TrueSight/TrueSightDefine.hpp>

namespace TrueSight {

/*
* TrueSight基础数据类型
*/
template <typename T> struct TrueSight_PUBLIC IPoint_ {
    IPoint_() {}
    IPoint_(T xx, T yy) : x(xx), y(yy) {}

    T x;
    T y;
};

typedef IPoint_<float> Point2f;

template <typename T> struct TrueSight_PUBLIC Rect_ {
    Rect_() {}
    Rect_(T xx, T yy, T ww, T hh) : x(xx), y(yy), width(ww), height(hh) {}

    T x;
    T y;
    T width;
    T height;
};

typedef Rect_<float> Rect2f;

} // namespace TrueSight