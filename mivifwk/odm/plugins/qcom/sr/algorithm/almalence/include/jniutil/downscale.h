
#ifndef __DOWNSCALE_H__
#define __DOWNSCALE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int rescaleNV21_rect(uint8_t *in, int x, int y, int w, int h, uint8_t *temp,
                                   uint8_t *out, uint8_t *outUV, int sx, int sy, int stride,
                                   int sxo, int syo, int ostride,
                                   int (*func)(uint8_t *in, uint8_t *inUV, uint8_t *temp,
                                               uint8_t *out, uint8_t *outUV, int sx, int sy,
                                               int stride, int sxo, int syo, int ostride))
{
    return func(in + y * stride + x, in + (sy + (y >> 1)) * stride + (x & -2), temp, out, outUV, w,
                h, stride, sxo, syo, ostride);
}

void rescaleNV21(uint8_t *in, uint8_t *inUV, uint8_t *out, uint8_t *outUV, int sx, int sy,
                 int stride, int sxo, int syo, int ostride);
int rescaleNV21_vec(uint8_t *in, uint8_t *inUV, uint8_t *temp, uint8_t *out, uint8_t *outUV, int sx,
                    int sy, int stride, int sxo, int syo, int ostride);
int fast_rescaleNV21_vec(uint8_t *in, uint8_t *inUV, uint8_t *temp, uint8_t *out, uint8_t *outUV,
                         int sx, int sy, int stride, int sxo, int syo, int ostride);

#ifdef __cplusplus
}
#endif

#endif
