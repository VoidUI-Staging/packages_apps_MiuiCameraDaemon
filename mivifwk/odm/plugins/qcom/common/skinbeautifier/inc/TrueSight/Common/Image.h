#pragma once
#include <cstdint>
#include <cstdlib>
#include <functional>

#include <TrueSight/TrueSightDefine.hpp>

namespace TrueSight {

/*
 * 封装图像的数据结构及相关函数
 */
struct TrueSight_PUBLIC Image {

    enum ImageFormat : int32_t {
        kImageFormatGray = 0,   // m_pData[0]

        // RGBX
        kImageFormatRGBA = 10,  // R: m_pData[0], G: m_pData[1], B: m_pData[2], A: m_pData[3]
        kImageFormatBGRA = 11,  // B: m_pData[0], G: m_pData[1], R: m_pData[2], A: m_pData[3]
        kImageFormatBGR  = 12,  // do not support yet
        kImageFormatRGB  = 13,  // do not support yet

        // YUV 420sp / 420p
        kImageFormatNV12 = 20,  // Y: m_pYData, VU: m_pUVData
        kImageFormatNV21 = 21,  // Y: m_pYData, UV: m_pUVData
        kImageFormatI420 = 22,  // Y: m_pYData, U: m_pUData, V: m_pVData
        kImageFormatYV12 = 23,  // Y: m_pYdata, V: m_pVData, U: m_pUData

        // YUV 422p
        kImageFormatI422 = 31, // Y: m_pYdata, V: m_pVData, U: m_pUData

        // YUV 422sp
        kImageFormatYUY2 = 41, // Y0 U0 Y1 V0; Y2 U2 Y3 V2; Y4 U4 Y5 V4
        kImageFormatYUYV = kImageFormatYUY2,
        //kImageFormatYVYU = 42, // TBD.
        //kImageFormatUYVY = 43, // TBD.
        //kImageFormatVYUV = 44, // non-existent
    };

    Image();
    Image(int nWidth, int nHeight, ImageFormat nFormat, int nOrientation = 1);
    Image(int nWidth, int nHeight, ImageFormat nFormat, int nOrientation, int nYStride, int nUStride = 0, int nVStride = 0);
    Image(const Image &rhs);
    Image &operator=(const Image &rhs);
    ~Image();

    size_t total() const;
    void release();
    bool empty() const;
    void create(int nWidth, int nHeight, ImageFormat _format, int _oriention = 1, int nYStride = 0, int nUStride = 0, int nVStride = 0);
    Image clone() const;
    void copyTo(Image& dst) const;

    union{
        unsigned char *m_pData;         //y通道数据、RGB类型数据、GRAY
        unsigned char *m_pYData;
        unsigned char *m_pYUYVData;
    };
    union {
        unsigned char *m_pUData;        //u通道数据
        unsigned char *m_pUVData;       //uv通道数据
        unsigned char *m_pVUData;       //vu通道数据
    };
    unsigned char *m_pVData;            //v通道数据
    union {
        int m_nStride;                  //RGB、GRAY stride
        int m_nYStride;                 //y通道stride
        int m_nYUYVStride;
    };
    union {
        int m_nUStride;                 //u通道的stride
        int m_nUVStride;                //uv通道的stride
        int m_nVUStride;                //vu通道的stride
    };
    int m_nVStride;                     //v通道的stride

    int m_nWidth;                       //图片宽度
    int m_nHeight;                      //图片高度

    ///////////////////////////////////////////////////////////////////////////////////////
    //                          image orientation示例
    //     1        2       3      4         5            6           7          8
    //
    //    888888  888888      88  88      8888888888  88                  88  8888888888
    //    88          88      88  88      88  88      88  88          88  88      88  88
    //    8888      8888    8888  8888    88          8888888888  8888888888          88
    //    88          88      88  88
    //    88          88  888888  888888
    ///////////////////////////////////////////////////////////////////////////////////////
    /**
     * Exif-Orientation : https://www.impulseadventure.com/photo/exif-orientation.html
     *                      (https://en.wikipedia.org/wiki/Exif#cite_note-23)
     * articles :
     *      https://magnushoff.com/articles/jpeg-orientation
     *      http://jpegclub.org/exif_orientation.html
     */
    int m_nOrientation;                 //旋转方向

    ImageFormat m_nPixelFormat;
    int* m_pRefcount;

private:
    class Deleter {
    public:
        explicit Deleter(std::function<void()> d) {
            deleter = std::move(d);
        }
        ~Deleter() {
            deleter();
        }
        std::function<void()> deleter;
    };
    std::shared_ptr<Deleter> deleter;

public:
    static Image create_gray(int nWidth, int nHeight, uint8_t* pData, int nOrientation = 1, int nStride = 0);
    static Image create_rgba(int nWidth, int nHeight, uint8_t* pData, int nOrientation = 1, int nStride = 0);
    static Image create_bgra(int nWidth, int nHeight, uint8_t* pData, int nOrientation = 1, int nStride = 0);
    static Image create_rgb(int nWidth, int nHeight, uint8_t* pData, int nOrientation = 1, int nStride = 0);
    static Image create_bgr(int nWidth, int nHeight, uint8_t* pData, int nOrientation = 1, int nStride = 0);
    static Image create_nv12(int nWidth, int nHeight, uint8_t* pYData, uint8_t* pUVData, int nOrientation = 1, int nYStride = 0, int nUVStride = 0);
    static Image create_nv21(int nWidth, int nHeight, uint8_t* pYData, uint8_t* pVUData, int nOrientation = 1, int nYStride = 0, int nVUStride = 0);
    static Image create_i420(int nWidth, int nHeight, uint8_t* pYData, uint8_t* pUData, uint8_t* pVData, int nOrientation = 1, int nYStride = 0, int nUStride = 0, int nVStride = 0);
    static Image create_yv12(int nWidth, int nHeight, uint8_t* pYData, uint8_t* pUData, uint8_t* pVData, int nOrientation = 1, int nYStride = 0, int nUStride = 0, int nVStride = 0);
    static Image create_i422(int nWidth, int nHeight, uint8_t* pYData, uint8_t* pUData, uint8_t* pVData, int nOrientation = 1, int nYStride = 0, int nUStride = 0, int nVStride = 0);
    static Image create_yuy2(int nWidth, int nHeight, uint8_t* pData, int nOrientation = 1, int nStride = 0);
    static Image create_yuyv(int nWidth, int nHeight, uint8_t* pData, int nOrientation = 1, int nStride = 0) {
        return create_yuy2(nWidth, nHeight, pData, nOrientation, nStride);
    }

public:
    static Image create_any(int nWidth, int nHeight, ImageFormat fmt, int nOrientation, uint8_t* pYData, int nYStride = 0, uint8_t* pUData = nullptr, int nUStride = 0, uint8_t* pVData = nullptr, int nVStride = 0);
    static Image create_any(int nWidth, int nHeight, ImageFormat fmt, int nOrientation, std::function<void()> deleter, uint8_t* pYData, int nYStride = 0, uint8_t* pUData = nullptr, int nUStride = 0, uint8_t* pVData = nullptr, int nVStride = 0);
};

} // namespace TrueSight