#pragma once

#include <TrueSight/TrueSightDefine.hpp>
#include <TrueSight/Common/Image.h>

namespace TrueSight {

class TrueSight_PUBLIC ImageCodecs {

public:
    enum Type : int32_t {
        Default = 0x0,
        PNG = 1,
        JPG = 2,
        BMP = 3
    };

public:
    /*
     * default load rgba format data
     */
    static Image DecodeFromFile(const char* fileName);
    static Image DecodeFromBuffer(const unsigned char* pBuffer, int size);

    static bool EncodeToFile(const char* fileName, const Image &image, Type type = Default, int jpeg_quality = 75);
    static unsigned char* EncodeToBuffer(const Image &image, int &dstSize, Type type = Default, int jpeg_quality = 75);

public:
    /**
     * deprecated
     */
    static unsigned char* DecodeFromFile(const char* fileName, int &width, int &height);
    static unsigned char* DecodeFromBuffer(const unsigned char* pBuffer, unsigned long size, int &width, int &height, bool flip = false);

    static bool EncodeToFile(const char* fileName, const unsigned char* pSrcData, int width, int height, int channel = 4, Type type = PNG);
    static unsigned char* EncodeToBuffer(const unsigned char* pSrcData, int width, int height, unsigned long &dstSize, int channel = 4, Type type = PNG);
};

} // namespace TrueSight