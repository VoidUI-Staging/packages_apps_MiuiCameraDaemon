/*VDDualCameraClearShot v1.0.0 (Mon Oct 15 11:33:53 EEST 2018, 21441c820da76711b9b9797667a486a4fd475b81)*/

/**
 * @file VDDualCameraClearShotAPI.h
 * @brief Contains API Interface for Visidon VDDualCameraClearShot.
 *
 * Copyright (C) 2018 Visidon
 * This file is part of Visidon VDDualCameraClearShot SDK.
 * <br />
 * See example usage in simple.c: @include simple.c
 * @example simple.c
 */
#ifndef __H_VDDUALCAMERASHOTAPI_H__
#define __H_VDDUALCAMERASHOTAPI_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "vddatatypes.h"

/**
* Structure for dual camera clearshot algorithm initialization parameters
**/
typedef struct VDDualCameraClearShotInitParameters_s
{
    VDImageFormat type; /**< Input image format */
    int width; /**< Input image width in pixels (note: all input frames need to be same size). */
    int height; /**< Input image height in pixels (note: all input frames need to be same size). */
    int yRowStride; /**< Length of one row (may be larger than width if there is padding) */
    int yColumnStride; /**< Length of one column (may be larger than height if there is padding) */
    int numberOfFrames;	/**< Maximum number of input images to be used with an algorithm. */
    int numberOfCores; /**< Number of processor cores */
} VDDualCameraClearShotInitParameters_t;

/**
* Structure for dual camera clearshot algorithm processing parameters
**/
typedef struct VDDualCameraClearShotParameters_s
{
    float luminanceNoiseReduction; /**< Luma noise reduction filter strength (0-10), 0 lowest level (no filtering), 10 highest filtering. */
    float chromaNoiseReduction; /**< Chroma noise reduction filter stregth (0-10), 0 off, 10 highest filtering */
    float sharpen; /**< Sharpening strenght (0-10), 0 lowest sharpening (no sharpening), 10 highest sharpening */
    int brightening; /**< Non-linear intensity adjustment to reveal low-intensity details (brighten image with more weight in dark pixels) (0-10), 0=off, 10=full strength */
    int colorBoost;	/**< Color boosting strength to increase color saturation. Supported values 0-10: 0=off, 10 = maximum. */
    int movingObjectNoiseRemoval; /**< Additional removal of noise from moving objects. 0 = off, 1 = low, 2 = medium, and 3 = high */
    int inputSharpnessThreshold; /**< Threshold for input frame sharpness check, i.e., how much sharpness can deviate[%] from the reference frame sharpness(0 - 100), e.g. 10 for rear camera and 40 for front camera */
    int allocate; /**< Allocate memory for input frame copies to keep original inputs unmodified(1 = allocate, 0 = do not allocate(use input image memory)) */
    int noiseReductionAdjustment[6]; /**< Amount of adjustment based on effective frames after fusion. Value can be between 0-10, 0 means no adjustment, 5 means default adjustment, 10 means maximum adjustment. Element [0] correspond to effective frames = 1, element [1] correspond to effective frames = 2, and so on. Supports 1-6 effective frames. If more than 6 frames are used, the last value will be used for the rest of the effectives. */
    float inputBrightnessThreshold; /**< How much input frames can vary in brightness (percentage 0 - 1). If larger than 0, check first if input frames are having too different brightness and select the most similar ones for processing (recommeded esp. with FRONT camera mode with the value 0.02) */
    int baseFrame; /**< Use this base frame (0 - numberOfFrames -1). If baseFrame == -1 --> will use automatic selection */
    int effectiveFrames; /**< Maximum number of frames to be used (consider only the N sharpest frames, must be between [1 - numberOfFrames] ) */
} VDDualCameraClearShotParameters_t;

/* Added by qudao1@xiaomi.com */
typedef struct vddclearshotops {
    const char *(*GetVersion)(void);

    VDErrorCode (*Initialize)(VDDualCameraClearShotInitParameters_t, void **);

    VDErrorCode (*SetNRForBrightness)(int, float, void *);

    int (*Process)(VDYUVMultiPlaneImage *, VDYUVMultiPlaneImage *, VDDualCameraClearShotParameters_t, int *, void *);

    VDErrorCode (*Release)(void **);

} VDDClearshotOps_t;
int VDDClearShot_APIOps_Init(VDDClearshotOps_t *VCS_Ops);
int VDDClearShot_APIOps_DeInit(void);
/* End of Added by qudao1@xiaomi.com */

/**
 * @brief Get VDDualCameraClearShot library version string
 *
 * @return version string
 */
const char * VDDualCameraClearShotVersion(void);

/**
 * @brief Initialize VDDualCameraClearShot engine
 *
 * Always check return value to catch initialization errors!
 *
 * @param aParams Algorithm parameters
 * @param aEngine VDDualCameraClearShot instance
 * @return VDErrorCodeOK on success, VDErrorCodeNOK on failure
 */
VDErrorCode VDDualCameraClearShotInitialize(VDDualCameraClearShotInitParameters_t aParams, void ** aEngine);

/**
 * @brief Release VDDualCameraClearShot engine resources
 *
 * @param aEngine VDDualCameraClearShot instance
 * @return VDErrorCodeOK on success, VDErrorCodeNOK on failure
 */
VDErrorCode VDDualCameraClearShotRelease(void ** aEngine);

/**
 * @brief Process VDDualCameraClearShot
 *
 * @param aInputImages Input images
 * @param aOutput Output image
 * @param aParams Run-time parameters
 * @param aSelectedBaseFrame VDDualCameraClearShot instance
 * @param aEngine VDDualCameraClearShot instance
 * @return Number of input frames used for output
 */
int VDDualCameraClearShotProcess(VDYUVMultiPlaneImage * aInputImages, VDYUVMultiPlaneImage * aOutput, VDDualCameraClearShotParameters_t aParams, int * aSelectedBaseFrame, void * aEngine);

/**
 * @brief Add face roi data for an input images
 *
 * @param aFrameIndex Index for the input image, e.g. 0, 1, 2, ..., numberOfFrames
 * @param aNumberOfFaces The number of faces detected in this image (frameIndex)
 * @param aFaceCoords Pointer to face roi data that is one dimensional array with length of 4*numberOfFaces, e.g. faceCoords = [xmin1, ymin1, width1, height1, ..., xminN, yminN, widthN, heightN];
 * @param aFaceWeight Additional sharpness / brightness weight for face regions (total weight will be original weight (=distance to image center) + faceWeight). If faceWeight >= 1.0, only face region will be considered in calculation.
 * @param aEngine VDDualCameraClearShot instance
 * @return VDErrorCodeOK on success, VDErrorCodeNOK on failure
*/
VDErrorCode VDDualCameraClearShotAddFaceROI(int aFrameIndex, int aNumberOfFaces, int * aFaceCoords, float aFaceWeight, void * aEngine);

/**
 * @brief Get sharpness and brighntess of the input frame.
 *
 * @param aInput Pointer to YUV image
 * @param aFrameIndex Index for the input image, e.g. 0, 1, 2, ..., numberOfFrames
 * @param aBrightness Calculated brightness level
 * @param aFaceCoords Face coordinates in array of Nx4 [xmin, xmax, width, height,...]
 * @param aFaceCount Number of faces in the faceCoords table
 * @param aFaceWeight Additional sharpness / brightness weight for face regions (total weight will be original weight (=distance to image center) + faceWeight). If faceWeight >= 1.0, only face region will be considered in calculation.
 * @param aEngine VDDualCameraClearShot instance
 * @return Calculated sharpness level of input image
*/
float VDDualCameraClearShotGetSharpness(VDYUVMultiPlaneImage * aInput, int aFrameIndex, float * aBrightness, int * aFaceCoords, int aFaceCount, float aFaceWeight, void * aEngine);

/**
 * @brief Set brightness based noise reduction strength. Brightness (=luminance value) is divided into 64 equally spaced grades: 0-3, 4-7, 8-11, ... 252-255.
 *
 * @param aBrightnessIndex Brightness grade index to be adjusted (0-63)
 * @param aNoiseReductionStrength Noise reduction strength for this brightness grades
 * @param aEngine VDDualCameraClearShot instance
 * @return VDErrorCodeOK on success, VDErrorCodeNOK on failure
 */
VDErrorCode VDDualCameraClearShotSetNRForBrightness(int aBrightnessIndex, float aNoiseReductionStrength, void * aEngine);

#ifdef __cplusplus
}
#endif
#endif
