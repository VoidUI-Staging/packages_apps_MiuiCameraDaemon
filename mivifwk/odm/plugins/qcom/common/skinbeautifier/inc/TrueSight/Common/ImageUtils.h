#pragma once
#include <cstdlib>

#include <TrueSight/TrueSightDefine.hpp>
#include "Image.h"

namespace TrueSight {

class TrueSight_PUBLIC ImageUtils {

public:
    static int convertToGray(const Image &srcImage, Image &dstImage);
    static int convertToRGBA(const Image &srcImage, Image &dstImage);
    static int convertToBGRA(const Image &srcImage, Image &dstImage);
    static int convertToRGB(const Image &srcImage, Image &dstImage);
    static int convertToBGR(const Image &srcImage, Image &dstImage);
    static int convertToNV12(const Image &srcImage, Image &dstImage);
    static int convertToNV21(const Image &srcImage, Image &dstImage);
    static int convertToI420(const Image &srcImage, Image &dstImage);
    static int convertToYV12(const Image &srcImage, Image &dstImage);
    static int convertToI422(const Image &srcImage, Image &dstImage);
    static int convertToYUY2(const Image &srcImage, Image &dstImage);
    static int convertToYUYV(const Image &srcImage, Image &dstImage) {
        return convertToYUY2(srcImage, dstImage);
    }

    static int resize(const Image& src, Image &dst, int dstWidth, int dstHeight);
    static int convert(const Image& src, Image &dst, Image::ImageFormat format);
    /**
     * rotate from src.m_nOrientation to orientation
     * @param src
     * @param dst
     * @param orientation
     * @return
     */
    static int rotate(const Image& src, Image &dst, int orientation = 1);

public:
    static int warpAffineGray(const Image &srcImage, int oriWidth, int oriHeight, Image &dstImage, int dstWidth, int dstHeight, const float mouth_matrix[6]);

    /**
     * as cv::warpAffine, support gray, rgba/bgra, rgb/bgr, nv12/nv21, i420
     * @param srcImage
     * @param dstImage
     * @param width
     * @param height
     * @param matrix
     * @return
     */
    static int warpAffine(const Image &src, Image &dst, int width, int height, const float matrix[6]);
    static int warpPerspective(const Image &src, Image &dst, int width, int height, const float matrix[9]);
};

} // namespace TrueSight
