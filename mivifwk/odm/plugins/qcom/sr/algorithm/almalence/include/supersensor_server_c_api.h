#ifndef ALMALENCE_SUPERSENSOR_SERVER_C_API_H
#define ALMALENCE_SUPERSENSOR_SERVER_C_API_H

#include <almashot/hvx/superzoom.h>

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#if defined(__ANDROID__)
#include <android/log.h>
#else
enum {
    ANDROID_LOG_UNKNOWN = 0,
    ANDROID_LOG_DEFAULT,
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
    ANDROID_LOG_FATAL,
    ANDROID_LOG_SILENT,
};
#endif

#ifndef SUPERSENSOR_DEFAULT_SIZE_GUARANTEE_BORDER
#define SUPERSENSOR_DEFAULT_SIZE_GUARANTEE_BORDER 64
#endif

#ifndef SUPERSENSOR_DEFAULT_MAX_STAB
#define SUPERSENSOR_DEFAULT_MAX_STAB 48
#endif

typedef void *supersensor_AlmaShotHolder_t;
typedef void *supersensor_Rect_t;
typedef void *supersensor_SuperSensorProcessor_t;
typedef void *supersensor_SuperSensorServer_t;
typedef void *supersensor_Yuv888_t;
typedef void *supersensor_YuvImage_t;

EXTERNC supersensor_Yuv888_t supersensor_Yuv888_init();
EXTERNC void supersensor_Yuv888_setPlanes(supersensor_Yuv888_t Yuv888, uint8_t *Yptr, uint8_t *Uptr,
                                          uint8_t *Vptr, uint32_t stride);
EXTERNC void supersensor_Yuv888_destroy(supersensor_Yuv888_t Yuv888);

EXTERNC supersensor_AlmaShotHolder_t supersensor_AlmaShotHolder_init();
EXTERNC void supersensor_AlmaShotHolder_destroy(supersensor_AlmaShotHolder_t AlmaShotHolder);

EXTERNC supersensor_YuvImage_t supersensor_YuvImage_init(uint8_t *Yptr, uint8_t *Uptr,
                                                         uint8_t *Vptr, uint32_t stride,
                                                         uint32_t scanline, uint32_t iso,
                                                         uint64_t exposure_time);
EXTERNC void supersensor_YuvImage_destroy(supersensor_YuvImage_t YuvImage);

EXTERNC supersensor_Rect_t supersensor_Rect_init(int left, int top, uint32_t width,
                                                 uint32_t height);
EXTERNC void supersensor_Rect_destroy(supersensor_Rect_t Rect);

EXTERNC supersensor_SuperSensorProcessor_t supersensor_SuperSensorProcessor_init(
    int camera_index, uint32_t sensor_width, uint32_t sensor_height, uint32_t output_width,
    uint32_t output_height, uint32_t output_stride, uint32_t output_scanline, uint32_t frames_max,
    alma_config_t *config);
EXTERNC supersensor_SuperSensorProcessor_t supersensor_SuperSensorProcessor_custom_bordered_init(
    int camera_index, uint32_t sensor_width, uint32_t sensor_height, uint32_t output_width,
    uint32_t output_height, uint32_t output_stride, uint32_t output_scanline, uint32_t frames_max,
    alma_config_t *config, uint32_t border);
EXTERNC void supersensor_SuperSensorProcessor_destroy(
    supersensor_SuperSensorProcessor_t SuperSensorProcessor);
EXTERNC void supersensor_SuperSensorProcessor_process(supersensor_SuperSensorProcessor_t self,
                                                      uint8_t *out, supersensor_YuvImage_t *input,
                                                      uint32_t count, double zoom,
                                                      alma_config_t *config);
EXTERNC void supersensor_SuperSensorProcessor_process_from_rect(
    supersensor_SuperSensorProcessor_t self, uint8_t *out, supersensor_YuvImage_t *input,
    uint32_t count, supersensor_Rect_t rect, alma_config_t *config);
EXTERNC void supersensor_SuperSensorProcessor_process8to10(supersensor_SuperSensorProcessor_t self,
                                                           uint16_t *out,
                                                           supersensor_YuvImage_t *input,
                                                           uint32_t count, double zoom,
                                                           alma_config_t *config);
EXTERNC void supersensor_SuperSensorProcessor_process8to10_from_rect(
    supersensor_SuperSensorProcessor_t self, uint16_t *out, supersensor_YuvImage_t *input,
    uint32_t count, supersensor_Rect_t rect, alma_config_t *config);

EXTERNC void supersensor_setLogLevel(int level);

EXTERNC supersensor_SuperSensorServer_t supersensor_superSensorServer_init(
    uint32_t sensor_width, uint32_t sensor_height, uint32_t output_width, uint32_t output_height,
    int camera_index, uint32_t (*frame_number_function)(uint32_t), alma_config_t *config_init,
    uint32_t max_stab, uint32_t border);
EXTERNC void supersensor_SuperSensorServer_destroy(
    supersensor_SuperSensorServer_t SuperSensorServer);
EXTERNC void supersensor_superSensorServer_queue(supersensor_SuperSensorServer_t self, int iso,
                                                 bool scaled,
                                                 bool processing_enabled, // processingEnabled
                                                 float gamma,
                                                 bool stabilization, // stabilization
                                                 float zoom, uint32_t width, uint32_t height,
                                                 supersensor_Yuv888_t yuv888,
                                                 alma_config_t *config);

EXTERNC int supersensor_superSensorServer_getLastProcessedImage(
    supersensor_SuperSensorServer_t self, bool next, uint8_t *out, char *errorMessage);

EXTERNC uint32_t SUPERSENSOR_DEFAULT_FRAMENUMBER_FUNCTION(uint32_t size);

#undef EXTERNC

#endif // ALMALENCE_SUPERSENSOR_SERVER_C_API_H
