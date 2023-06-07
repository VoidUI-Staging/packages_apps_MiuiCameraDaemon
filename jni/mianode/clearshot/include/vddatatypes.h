/*VDDualCameraClearShot v1.0.0 (Mon Oct 15 11:33:53 EEST 2018, 21441c820da76711b9b9797667a486a4fd475b81)*/

/**
 * @file VDDataTypes.h
 * @brief Contains type definitions shared between multiple Visidon API Interface
 *
 * Copyright (C) 2018 Visidon Oy
 */

#ifndef __H_VDDUALCAMERA_DATATYPES_H__
#define __H_VDDUALCAMERA_DATATYPES_H__


/**
 * @brief Image formats supported by various algorithms
 *
 * Note to devs:
 * - Remember to update VDImageFormatStr() when modifying VDImageFormat enum in
 *   VDDataTypes.cpp!
 */
typedef enum VDImageFormat_e
{
    VDImageFormatRawGray = 1, /**< 8-bit grayscale image*/
    VDImageFormatYUV422,/**< YUV422*/
    VDImageFormatRGB565, /**< RGB565 */
    VDImageFormatRGBA8888, /**< 32-bit RGB+A*/
    VDImageFormatRGB888, /**< 24-bit RGB */
    VDImageFormatJPEG, /**< JPEG compression */
    VDImageFormatYUV_NV21, /**< YUV NV21 */
    VDImageFormatYUV_NV12, /**< YUV NV12 */
    VDImageFormatYUV_NV16, /**< YUV NV16 */
    VDImageFormatYUV_NV41, /**< YUV NV41 */
    VDImageFormatYUV_420P, /**< YUV 420P (planar format Y....U..V.. */
    VDImageFormatYUV_YUYV, /** < YUV YUYV (YUYVYUYV...YUYV */
    VDImageFormatYUV_UYVY, /** < YUV UYVY (UYVYUYVY...UYVY */
    VDImageFormatYUV_YV12, /**< 8 bit Y plane followed by 8 bit 2x2 subsampled V and U planes (Y VU can be at different buffers) **/
    VDImageFormatYUV_NV21_10BIT, /**< YUV NV21 10-bit per pixel*/
    VDImageFormatYUV_NV12_10BIT, /**< YUV NV12 10-bit per pixel*/
    VDImageFormatYUV_P010_10BIT /**< YUV P010 10-bit per pixel*/
} VDImageFormat;

/**
 * @brief Structure for general multi-plane yuv image to be used for
 *   for many Visidon algorithms
 */
typedef struct VDYUVMultiPlaneImage_s
{
    VDImageFormat type;
    int width;
    int height;
    int yPixelStride;
    int yRowStride;
    int yColumnStride;
    int cbPixelStride;
    int cbRowStride;
    int crPixelStride;
    int crRowStride;
    unsigned char * yPtr;
    unsigned char * cbPtr;
    unsigned char * crPtr;
} VDYUVMultiPlaneImage;

/**
 * @brief Generic error types for various algorithms
 *
 * Note to devs:
 * - Remember to update VDErrorCodeStr() when modifying VDErrorCode enum in
 *   VDDataTypes.cpp!
 * - VDErrorCodeOK MUST always be 0
 * - All Visidon algorithm error codes MUST be negative
 * - Positive error codes are treated as system (errno.h) errors
 */
typedef enum VDErrorCode_e
{
	VDErrorCodeOK = 0, /**< OK (no error) */
	VDErrorCodeNOK = -1, /**< Not OK */
	VDErrorCodeLicense = -2, /**< License check failed */
	VDErrorCodeInputParam = -3 /**< Invalid input params */
} VDErrorCode;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return string representation of given image format
 * @param aFormat Format to convert to string
 * @return Return string representation of given image format
 */
const char *
VDImageFormatStr(VDImageFormat aFormat);

/**
 * @brief Return string representation of given error code
 * @param aError Error to convert to string
 * @return Return string representation of given error code
 */
const char *
VDErrorCodeStr(VDErrorCode aError);

#ifdef __cplusplus
}
#endif
#endif
