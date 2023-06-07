#ifndef __MI_BUF_H__
#define __MI_BUF_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define MAX_BUFFER_PLANE    4
#define FORMAT_YUV_420_NV21 0x01
#define FORMAT_YUV_420_NV12 0x02
#define FORMAT_RAW_16       0x03

typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned char uint8_t;
/*Define the image format space*/

namespace mihdr {
// struct MiImageBuffer
// {
//     int PixelArrayFormat;
//     int Width;
//     int Height;
//     int Pitch[MAX_BUFFER_PLANE];
//     unsigned char *Plane[MAX_BUFFER_PLANE];
// };

// int MiCopyBuffer(struct MiImageBuffer *dst, struct MiImageBuffer *src);
int MiAllocBuffer(struct MiImageBuffer *buffer, int width, int height, int stride, int format);
void MiFreeBuffer(struct MiImageBuffer *buffer);
int MiGetBufferStride(struct MiImageBuffer *buffer);
void MiAssignBuffer(struct MiImageBuffer *buffer, int width, int height, int stride,
                    unsigned char *data);
// void MiDumpBuffer(struct MiImageBuffer *buffer, const char* file);
void MiPrintBufferInfo(struct MiImageBuffer *buffer, char str[], const char *prefix);
void yuv420sp2rgba(unsigned char *py, unsigned char *puv, unsigned char *rgba, int stride_y,
                   int stride_uv, int width, int height);
void rgba2yuv420sp(unsigned char *py, unsigned char *puv, unsigned char *rgba, int stride_y,
                   int stride_uv, int width, int height);
void MiDumpRawBuffer(struct MiImageBuffer *buffer, const char *file);

} // namespace mihdr

using namespace mihdr;

#endif /*__MI_BUF_H__*/
