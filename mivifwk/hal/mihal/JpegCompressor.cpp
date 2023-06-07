#include "JpegCompressor.h"

#include "LogUtil.h"

namespace mihal {

JpegCompressor::JpegCompressor()
{
    result_buffer_size = 0;
    result_buffer = nullptr;
}

JpegCompressor::~JpegCompressor() {}

bool JpegCompressor::compressImage(const JpegCompressorParams &buffer, int quality,
                                   const void *app1Buffer, unsigned int app1Size)
{
    if (buffer.width % 8 != 0 || buffer.height % 2 != 0) {
        MLOGE(LogGroupHAL, "Image size can not be handled: %dx%d", buffer.width, buffer.height);
        return false;
    }

    if (!encode(buffer, quality, app1Buffer, app1Size)) {
        return false;
    }
    MLOGI(LogGroupHAL, "Compressed JPEG: [%dx%d] stride: %d, quality: %d", buffer.width,
          buffer.height, buffer.stride, quality);

    return true;
}

const void *JpegCompressor::getCompressedImagePtr()
{
    return result_buffer;
}

void JpegCompressor::setCompressedImage(uint8_t **outbuf, unsigned long size)
{
    result_buffer = outbuf;
    result_buffer_size = size;
}

unsigned long JpegCompressor::getCompressedImageSize()
{
    return result_buffer_size;
}

void JpegCompressor::outputErrorMessage(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];

    /* Create the message */
    (*cinfo->err->format_message)(cinfo, buffer);
    MLOGE(LogGroupHAL, "%s", buffer);
}

bool JpegCompressor::encode(const JpegCompressorParams &buffer, int jpegQuality,
                            const void *app1Buffer, unsigned int app1Size)
{
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    cinfo.err->output_message = &outputErrorMessage;

    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, result_buffer, &result_buffer_size);

    setJpegCompressStruct(buffer.width, buffer.height, jpegQuality, &cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    if (app1Buffer != nullptr && app1Size > 0) {
        jpeg_write_marker(&cinfo, JPEG_APP0 + 1, static_cast<const JOCTET *>(app1Buffer), app1Size);
    }

    uint32_t offsets[2]{0, (buffer.scanline * buffer.stride)};
    bool ret = compress(&cinfo, buffer.pAddr, offsets, buffer.stride);

    if (!ret) {
        MLOGE(LogGroupHAL, "compress fail");
        return false;
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    return true;
}

void JpegCompressor::setJpegCompressStruct(int width, int height, int quality,
                                           jpeg_compress_struct *cinfo)
{
    cinfo->image_width = width;
    cinfo->image_height = height;
    cinfo->input_components = 3;
    cinfo->in_color_space = JCS_YCbCr;
    jpeg_set_defaults(cinfo);

    jpeg_set_quality(cinfo, quality, TRUE);
    jpeg_set_colorspace(cinfo, JCS_YCbCr);
    cinfo->raw_data_in = TRUE;
    cinfo->dct_method = JDCT_IFAST;

    // Configure sampling factors. The sampling factor is JPEG subsampling 420
    // because the source format is YUV420.
    cinfo->comp_info[0].h_samp_factor = 2;
    cinfo->comp_info[0].v_samp_factor = 2;
    cinfo->comp_info[1].h_samp_factor = 1;
    cinfo->comp_info[1].v_samp_factor = 1;
    cinfo->comp_info[2].h_samp_factor = 1;
    cinfo->comp_info[2].v_samp_factor = 1;
}

bool JpegCompressor::compress(jpeg_compress_struct *cinfo, const uint8_t *yuv)
{
    JSAMPROW y[kCompressBatchSize];
    JSAMPROW cb[kCompressBatchSize / 2];
    JSAMPROW cr[kCompressBatchSize / 2];
    JSAMPARRAY planes[3]{y, cb, cr};

    size_t y_plane_size = cinfo->image_width * cinfo->image_height;
    size_t uv_plane_size = y_plane_size / 4;
    uint8_t *y_plane = const_cast<uint8_t *>(yuv);
    uint8_t *u_plane = const_cast<uint8_t *>(yuv + y_plane_size);
    uint8_t *v_plane = const_cast<uint8_t *>(yuv + y_plane_size + uv_plane_size);
    std::unique_ptr<uint8_t[]> empty(new uint8_t[cinfo->image_width]);
    memset(empty.get(), 0, cinfo->image_width);

    while (cinfo->next_scanline < cinfo->image_height) {
        for (int i = 0; i < kCompressBatchSize; ++i) {
            size_t scanline = cinfo->next_scanline + i;
            if (scanline < cinfo->image_height) {
                y[i] = y_plane + scanline * cinfo->image_width;
            } else {
                y[i] = empty.get();
            }
        }
        // cb, cr only have half scanlines
        for (int i = 0; i < kCompressBatchSize / 2; ++i) {
            size_t scanline = cinfo->next_scanline / 2 + i;
            if (scanline < cinfo->image_height / 2) {
                int offset = scanline * (cinfo->image_width / 2);
                cb[i] = u_plane + offset;
                cr[i] = v_plane + offset;
            } else {
                cb[i] = cr[i] = empty.get();
            }
        }

        int processed = jpeg_write_raw_data(cinfo, planes, kCompressBatchSize);
        if (processed != kCompressBatchSize) {
            MLOGE(LogGroupHAL, "Number of processed lines does not equal input lines.");
            return false;
        }
    }
    return true;
}

bool JpegCompressor::compress(jpeg_compress_struct *cinfo, uint8_t *yuv, uint32_t *offsets,
                              int stride)
{
    JSAMPROW y[kCompressBatchSize];
    JSAMPROW cb[kCompressBatchSize / 2];
    JSAMPROW cr[kCompressBatchSize / 2];
    JSAMPARRAY planes[3]{y, cb, cr};

    int width = cinfo->image_width;
    int height = cinfo->image_height;
    uint8_t *yPlanar = yuv + offsets[0];
    // TODO: Fix color reversal
    uint8_t *vuPlanar = yuv + offsets[1] + 1; // width * height;
    uint8_t *uRows = new uint8_t[8 * (width >> 1)];
    uint8_t *vRows = new uint8_t[8 * (width >> 1)];

    // process 16 lines of Y and 8 lines of U/V each time.
    while (cinfo->next_scanline < cinfo->image_height) {
        // deitnerleave u and v
        deinterleave(vuPlanar, uRows, vRows, cinfo->next_scanline, width, height, stride);

        // Jpeg library ignores the rows whose indices are greater than height.
        for (int i = 0; i < 16; i++) {
            // y row
            y[i] = yPlanar + (cinfo->next_scanline + i) * stride;

            // construct u row and v row
            if ((i & 1) == 0) {
                // height and width are both halved because of downsampling
                int offset = (i >> 1) * (width >> 1);
                cb[i / 2] = uRows + offset;
                cr[i / 2] = vRows + offset;
            }
        }

        int processed = jpeg_write_raw_data(cinfo, planes, kCompressBatchSize);
        if (processed != kCompressBatchSize) {
            MLOGE(LogGroupHAL, "Number of processed lines does not equal input lines.");
            return false;
        }
    }
    delete[] uRows;
    delete[] vRows;

    return true;
}

void JpegCompressor::deinterleave(uint8_t *vuPlanar, uint8_t *uRows, uint8_t *vRows, int rowIndex,
                                  int width, int height, int stride)
{
    int numRows = (height - rowIndex) / 2;
    if (numRows > 8)
        numRows = 8;
    for (int row = 0; row < numRows; ++row) {
        int offset = ((rowIndex >> 1) + row) * stride;
        uint8_t *vu = vuPlanar + offset;
        for (int i = 0; i < (width >> 1); ++i) {
            int index = row * (width >> 1) + i;
            uRows[index] = vu[1];
            vRows[index] = vu[0];
            vu += 2;
        }
    }
}

} // namespace mihal
