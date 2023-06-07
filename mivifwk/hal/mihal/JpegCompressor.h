#ifndef _JPEG_COMPRESSOR_H_
#define _JPEG_COMPRESSOR_H_

#include <cstdio>
#include <vector>

// We must include cstdio before jpeglib.h. It is a requirement of libjpeg.
#include <inttypes.h>
#include <jerror.h>
#include <jpeglib.h>

namespace mihal {

static const int32_t MaxPlanes = 3;

struct JpegCompressorParams
{
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t scanline;
    uint8_t *pAddr;
    size_t size;
};

// Encapsulates a converter from YU12 to JPEG format. This class is not
// thread-safe.
class JpegCompressor
{
public:
    JpegCompressor();
    ~JpegCompressor();

    // Compresses NV12 image to JPEG format. After calling this method, call
    // getCompressedImagePtr() to get the image. |quality| is the resulted jpeg
    // image quality. It ranges from 1 (poorest quality) to 100 (highest quality).
    // |app1Buffer| is the buffer of APP1 segment (exif) which will be added to
    // the compressed image. Returns false if errors occur during compression.
    bool compressImage(const JpegCompressorParams &buffer, int quality, const void *app1Buffer,
                       unsigned int app1Size);

    // Returns the compressed JPEG buffer pointer. This method must be called only
    // after calling compressImage().
    const void *getCompressedImagePtr();

    void setCompressedImage(uint8_t **outbuf, unsigned long size);

    // Returns the compressed JPEG buffer size. This method must be called only
    // after calling compressImage().
    unsigned long getCompressedImageSize();

private:
    static void outputErrorMessage(j_common_ptr cinfo);

    // Returns false if errors occur.
    bool encode(const JpegCompressorParams &buffer, int jpegQuality, const void *app1Buffer,
                unsigned int app1Size);

    void setJpegCompressStruct(int width, int height, int quality, jpeg_compress_struct *cinfo);

    // Returns false if errors occur.
    bool compress(jpeg_compress_struct *cinfo, const uint8_t *yuv);

    bool compress(jpeg_compress_struct *cinfo, uint8_t *yuv, uint32_t *offsets, int stride);

    void deinterleave(uint8_t *vuPlanar, uint8_t *uRows, uint8_t *vRows, int rowIndex, int width,
                      int height, int stride);

    // Process 16 lines of Y and 16 lines of U/V each time.
    // We must pass at least 16 scanlines according to libjpeg documentation.
    static const int kCompressBatchSize = 16;

    uint8_t **result_buffer;
    unsigned long result_buffer_size;
};

} // namespace mihal

#endif //_JPEG_COMPRESSOR_H_