#ifndef MIAI_BUFFER_H_
#define MIAI_BUFFER_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MIFORMATSMAXPLANES 3
typedef enum {
    CAM_FORMAT_JPEG = 0x100,        // ImageFormat's JPG
    CAM_FORMAT_YUV_420_NV12 = 0x23, // ImageFormat's YUV_420_888
    CAM_FORMAT_YUV_420_NV21 = 0x11, // ImageFormat's NV21
    CAM_FORMAT_Y16 = 0x20363159,    // ImageFormat's Y16
    CAM_FORMAT_Y8 = 0x20203859,     // ImageFormat's Y8
    CAM_FORMAT_P010 = 0x7FA30C0A,   // ImageFormat's P010
    CAM_FORMAT_MAX
} MiaFormat;

struct MIAIImageBuffer
{
    int PixelArrayFormat;
    int Width;
    int Height;
    int Pitch[MIFORMATSMAXPLANES];
    int Scanline[MIFORMATSMAXPLANES];
    unsigned char *Plane[MIFORMATSMAXPLANES];
    int fd[MIFORMATSMAXPLANES];
};

int MIAICopyBuffer(struct MIAIImageBuffer *dst, struct MIAIImageBuffer *src);
void MIAIDumpBuffer(struct MIAIImageBuffer *buffer, const char *file);
int MIAIAllocBuffer(struct MIAIImageBuffer *buffer, int width, int height, int format);
void MIAIFreeBuffer(struct MIAIImageBuffer *buffer);

#endif // MIAI_BUFFER_H_
