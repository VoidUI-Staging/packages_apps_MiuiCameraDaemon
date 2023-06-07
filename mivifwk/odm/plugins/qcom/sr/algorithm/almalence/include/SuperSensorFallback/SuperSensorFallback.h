#ifndef __SUPERSENSORSERVER_SUPERSENSOR_H__
#define __SUPERSENSORSERVER_SUPERSENSOR_H__

#include <almashot/reference/almashot.h>
#include <almashot/reference/superzoom.h>

namespace almalence {

int Super_ProcessNewRef(uint8_t **in, uint8_t *out, int sx, int sy, int sxo, int syo, int nFrames,
                        int iso, int cameraIndex, alma_config_t *config, int logLevel,
                        char *logTag);

int convertYUV10_to_NV21CPU(uint16_t *y10, uint16_t *uv10, uint8_t *nv21, uint8_t *nv21uv, int cx,
                            int cy, int width, int height, int stride, int sy, int ostride);

int convertNV21_to_YUV10CPU(uint8_t *nv21, uint8_t *nv21uv, uint16_t *y10, uint16_t *uv10, int cx,
                            int cy, int width, int height, int stride, int sy, int ostride);

} // namespace almalence
#endif
