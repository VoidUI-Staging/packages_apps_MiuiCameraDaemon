/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __CAM_GENERAL_DEFINITIONS_H__
#define __CAM_GENERAL_DEFINITIONS_H__

namespace mihal {

enum HALPixelFormat {
    HALPixelFormatYCbCr420_SP =
        0x00000109, ///< YCbCr420_SP is mapped to HAL_PIXEL_FORMAT_YCbCr_420_SP, used in Dewarp
    HALPixelFormatYCrCb420_SP =
        0x00000113, ///< YCrCb420_SP is mapped to HAL_PIXEL_FORMAT_YCbCr420_888 with ZSL flags
    HALPixelFormatRaw16 = 0x00000020, ///< Blob format
    HALPixelFormatBlob =
        0x00000021, ///< Carries data which does not have a standard image structure (e.g. JPEG)
    HALPixelFormatImplDefined =
        0x00000022, ///< Format is up to the device-specific Gralloc implementation.
    HALPixelFormatYCbCr420_888 =
        0x00000023, ///< Efficient YCbCr/YCrCb 4:2:0 buffer layout, layout-independent
    HALPixelFormatRawOpaque = 0x00000024,      ///< Raw Opaque
    HALPixelFormatRaw10 = 0x00000025,          ///< Raw 10
    HALPixelFormatRaw12 = 0x00000026,          ///< Raw 12
    HALPixelFormatRaw64 = 0x00000027,          ///< Blob format
    HALPixelFormatUBWCNV124R = 0x00000028,     ///< UBWCNV12-4R  ---MFSR PrefilterÊä³ö---
    HALPixelFormatNV12Encodeable = 0x00000102, ///< YUV Format also used during HEIF
    HALPixelFormatNV12HEIF = 0x00000116,       ///< HEIF video YUV420 format
    HALPixelFormatNV12YUVFLEX = 0x00000125,    ///< Flex NV12 YUV format with 1 batch
    HALPixelFormatNV12UBWCFLEX = 0x00000126,   ///< Flex NV12 UBWC format with 16 batch7FA30C09
    HALPixelFormatNV12UBWCFLEX2 = 0x00000128,  ///< Flex NV12 UBWC format with 2 batch
    HALPixelFormatNV12UBWCFLEX4 = 0x00000129,  ///< Flex NV12 UBWC format with 4 batch
    HALPixelFormatNV12UBWCFLEX8 = 0x00000130,  ///< Flex NV12 UBWC format with 8 batch

    HALPixelFormatY8 = 0x20203859,       ///< Y 8
    HALPixelFormatY16 = 0x20363159,      ///< Y 16
    HALPixelFormatP010 = 0x7FA30C0A,     ///< P010 ---MFSR Prefilter FullÊä³ö---
    HALPixelFormatUBWCTP10 = 0x7FA30C09, ///< UBWCTP10 ---MFSR Prefilter 1/4 1/16 1/64Êä³ö---
    HALPixelFormatUBWCNV12 = 0x7FA30C06, ///< UBWCNV12
    HALPixelFormatPD10 = 0x7FA30C08,     ///< PD10
    HALPixelFormatUndefined = 0x00000404,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// operation mode define
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum StreamConfigMode {
    StreamConfigModeNormal = 0x0000, // Normal stream configuration operation mode
    StreamConfigModeConstrainedHighSpeed =
        0x0001, // Special constrained high speed operation mode for devices that can
                //  not support high speed output in NORMAL mode
    StreamConfigModeVendorStart =
        0x8000,                     // First value for vendor-defined stream configuration modes
    StreamConfigModeSAT = 0x8001,   // Xiaomi SAT
    StreamConfigModeBokeh = 0x8002, // Xiaomi Bokeh
    StreamConfigModeMiuiManual = 0x8003,               // Xiaomi Manual mode
    StreamConfigModeMiuiVideo = 0x8004,                // Xiaomi Video mode
    StreamConfigModeMiuiZslFront = 0x8005,             // Xiaomi zsl for front camera
    StreamConfigModeMiuiFaceUnlock = 0x8006,           // Xiaomi faceunlock for front camera
    StreamConfigModeMiuiQcfaFront = 0x8007,            // Xiaomi Qcfa for front camera
    StreamConfigModeMiuiPanorama = 0x8008,             // Xiaomi Panorama mode
    StreamConfigModeMiuiVideoBeauty = 0x8009,          // Xiaomi video beautifier
    StreamConfigModeMiuiVideoBeautyWithEIS = 0x8019,   // Xiaomi video beautifier with eis
    StreamConfigModeMiuiVideoAI = 0x8029,              // Xiaomi video beautifier with AI
    StreamConfigModeMiuiSuperNight = 0x800A,           // Xiaomi super night
    StreamConfigModeMiuiMimoji = 0x800B,               // Xiaomi Mimoji mode
    StreamConfigModeMiuiVlog = 0x800C,                 // Xiaomi Vlog
    StreamConfigModeMiuiSUPEREISVideo = 0x800d,        // Xiaomi super eis Video mode
    StreamConfigModeMiuiSUPEREISPROVideo = 0x800F,     // Xiaomi super eis Video mode
    StreamConfigMode3SAT = 0x8011,                     // Xiaomi 3SAT
    StreamConfigModeMiuiVideo8k = 0x801d,              // xiaomi eis mode 8k
    StreamConfigModeMiuiHdr10VideoWithoutEis = 0x8024, // Xiaomi hdr10  no Eis
    StreamConfigModeMiuiShortVideo = 0x8030,           // Xiaomi short video mode
    StreamConfigModeMiuiVideoNight = 0x8031,           // Xiaomi Video Night
    StreamConfigModeHFR60FPS = 0x803C,                 // Xiaomi HFR 60FPS
    StreamConfigModeHFR120FPS = 0x8078,                // Xiaomi HFR 120fps
    StreamConfigModeHFR240FPS = 0x80F0,                // Xiaomi HFR 240fps
    StreamConfigModeFrontBokeh = 0x80F1,               // Xiaomi front Bokeh
    StreamConfigModeSWRemosaicRaw = 0x80F2,            // Xiaomi sw remosaic raw used in Cit
    StreamConfigModeMiuiZslRear = 0x80F3,       // Xiaomi zsl for rear qcfa camera, only used for
                                                // upscale binning size to full size
    StreamConfigModeLiveAutoZoom = 0x80F4,      // Xiaomi Live Auto Zoom
    StreamConfigModeMiuiQcfaRear = 0x80F5,      // Xiaomi Qcfa for rear camera
    StreamConfigModeSecureCameraPay = 0x80F6,   // Xiaomi SecureCamera Pay Plan
    StreamConfigModeMagicSnapshot = 0x80F8,     // Xiaomi Magic Snapshot
    StreamConfigModeAlgoDual = 0x9000,          // Xiaomi App AlgoUp Dual Camera mode
    StreamConfigModeAlgoDualSAT = 0x9002,       // Xiaomi App AlgoUp Dual Camera mode
    StreamConfigModeQCFALite = 0x9001,          // Xiaomi App AlgoUp QCFA Camera mode
    StreamConfigModeAlgoQCFAMFNR = 0x9004,      // Xiaomi App AlgoUp QCFA + MFNR mode
    StreamConfigModeAlgoSingleRTB = 0x9003,     // Xiaomi App AlgoUp Single Camera mode
    StreamConfigModeAlgoNormal = 0x9005,        // Xiaomi App AlgoUp Single back Camera mode
    StreamConfigModeDualCalib = 0x9006,         // Xiaomi Dual Sensor Calibration
    StreamConfigModeAlgoManualSuperHD = 0x9007, // Xiaomi App AlgoUp Manual Camera mode
    StreamConfigModeAlgoManual = 0x9008,        // Xiaomi App AlgoUp Manual Camera mode
    StreamConfigModeAlgoQCFAMFNRFront = 0x9009, // Xiaomi App AlgoUp Manual Camera mode

    StreamConfigModeThirdCamera = 0x9010,             // Xiaomi Third camera Normal JPEG
    StreamConfigModeThirdCameraNormalYUV = 0x9100,    // Xiaomi Third camera Normal YUV
    StreamConfigModeThirdCameraDualYUV = 0x9101,      // Xiaomi Third camera Bokeh YUV
    StreamConfigModeThirdCameraVideoEIS = 0x9200,     // Xiaomi Third camera VideoEIS
    StreamConfigModeThirdCameraSuperNight = 0x9201,   // Xiaomi Third camera SuperNight
    StreamConfigModeThirdCameraSnapshotEIS = 0x9202,  // Xiaomi Third camera SnapshotEIS
    StreamConfigModeThirdCameraVideoNight = 0x9203,   // Xiaomi Third camera Video SuperNight
    StreamConfigModeThirdCameraVideoHDR = 0x9204,     // Xiaomi Third camera Video HDR
    StreamConfigAmbiLight = 0x9300,                   // Xiaomi Long expo Camera mode
    StreamConfigModePureShot = 0x9400,                // Xiaomi Long expo Camera mode
    StreamConfigFactoryTest0 = 0x9500,                // Xiaomi Factory Test mode
    StreamConfigFactoryTest1 = 0x9501,                // Xiaomi Factory Test mode
    StreamConfigFactoryTest2 = 0x9502,                // Xiaomi Factory Test mode
    StreamConfigModeMovieLensCameraVideoEIS = 0x800e, // Xiaomi movie camera VideoEIS
    StreamConfigModeVendorEnd = 0xEFFF, // End value for vendor-defined stream configuration modes
    StreamConfigModeQTIStart = 0xF000,  // First value for QTI-defined stream configuration modes
    StreamConfigModeQTITorchWidget = 0xF001,  // Operation mode for Torch Widget.
    StreamConfigModeVideoHdr = 0xF002,        // Video HDR On
    StreamConfigModeQTIEISRealTime = 0xF004,  // Operation mode for Real-Time EIS recording usecase.
    StreamConfigModeQTIEISLookAhead = 0xF008, // Operation mode for Look Ahead EIS recording usecase
    StreamConfigModeQTIFOVC = 0xF010,         // Field of View compensation in recording usecase.
    StreamConfigModeQTIFAEC = 0xF020,         // Fast AEC mode
    StreamConfigModeFastShutter = 0xF040,     // FastShutter mode
    StreamConfigModeSuperSlowMotionFRC = 0xF080, // Super Slow Motion with FRC interpolation usecase
    StreamConfigModeSensorMode = 0xF0000,        // Sensor Mode selection for >=60fps
                                                 // require 4 bits for sensor mode index
};

typedef struct
{
    int32_t x;      // x coordinates of the top left corner of the rectangle
    int32_t y;      // y coordinates of the top left corner of the rectangle
    int32_t width;  // width in pixels */
    int32_t height; // height in pixels
} SensorActiveSize;

typedef struct
{
    int32_t width;  // width in pixels
    int32_t height; // height in pixels
} SensorDimension;

} // namespace mihal

#endif // __COMMON_H__ END
