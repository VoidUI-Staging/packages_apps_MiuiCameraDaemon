/**
 * @file VDClearShotAPI.h
 * @brief Contains API Interface for Visidon VDClearShot.
 *
 * Copyright (C) 2018 Visidon Oy
 * This file is part of Visidon VDClearShot SDK.
 */

#ifndef __VD_CLEAR_SHOT_API_H__
#define __VD_CLEAR_SHOT_API_H__

#ifdef __cplusplus
extern "C" {
#endif


#ifndef __ErrorType__DEF__
#define __ErrorType__DEF__
/*
 * General error codes for Visidon API
 */
typedef enum _VDErrorType {
    VD_OK = 0, /**< Operation ok*/
    VD_NOK /**< Operation not ok*/
} VDErrorType;
#endif

#ifndef __Datatype__DEF__
#define __Datatype__DEF__
/*
 * General data types for Visidon API. Note that only VD_YUV_NV21, VD_YUV_NV12,
 * YUV YUYV and VD_YUV_420P are suppported by VDClearShot Algorithms.
 */
typedef enum _VDDatatype {
    VD_RawGray = 1,    /**< 8-bit grayscale image*/
    VD_YUV422,         /**< YUV422*/
    VD_RGB565,         /**< RGB565 */
    VD_RGBA8888,       /**< 32-bit RGB+A*/
    VD_RawBayer,       /**< Specific Bayer pattern format*/
    VD_RGB888,         /**< 24-bit RGB */
    VD_JPEG,           /**< JPEG compression */
    VD_YUV_NV21,       /**< YUV NV21 */
    VD_YUV_NV12,       /**< YUV NV12 */
    VD_YUV_NV16,       /**< YUV NV16 */
    VD_YUV_420P,       /**< YUV 420P (planar format Y....U..V.. */
    VD_YUV_YUYV,       /** < YUV YUYV */
    VD_YUV_UYVY,       /** < YUV UYVY (UYVYUYVY...UYVY */
    VD_BAYER10,        /** 10 bit raw bayer, 8 bytes per pixel */
    VD_YUV_NV41,       /**< YUV NV41 */
    VD_YUV_YV12,       /**< planar Y ½V ½U (Y ½VU can be at different buffers) **/
    VD_YUV_NV21_10BIT, /**< YUV NV21 10 bit */
    VD_YUV_NV12_10BIT, /**< YUV NV12 10 bit */
    VD_YUV_P010_10BIT  /**< YUV P010 10 bit */
} VDDatatype;
#endif

/*
 * Structure for general multi-plane yuv image to be used for
 * for many Visidon algorithms
 */
#ifndef __VDYUVPlaneImage__DEF__
#define __VDYUVPlaneImage__DEF__
typedef struct _VDYUVMultiPlaneImage {
    VDDatatype type;
    int width;
    int height;
    int yPixelStride;
    int yRowStride;
    int yColumnStride;
    int cbPixelStride;
    int cbRowStride;
    int crPixelStride;
    int crRowStride;
    unsigned char *yPtr;
    unsigned char *cbPtr;
    unsigned char *crPtr;
} VDYUVMultiPlaneImage;
#endif

/*
 * Structure for clearshot algorithm initialization parameters
 */
typedef struct _VDClearShotInitParameters {
    VDDatatype type;    /**< Input image format (supported formats VD_YUV_NV21 / VD_YUV_NV12 / VD_YUV_NV16) */
    int width;          /**< Input image width in pixels (note: all input frames need to be same size). */
    int height;         /**< Input image height in pixels (note: all input frames need to be same size). */
    int yRowStride;     /**< Length of one row (may be larger than width if there is padding) */
    int yColumnStride;  /**< Length of one column (may be larger than height if there is padding) */
    int numberOfFrames; /**< Maximum number of input images to be used with an algorithm. */
} VDClearShotInitParameters;

/**
* Structure for clearshot algorithm processing parameters
**/
typedef struct _VDClearShotRuntimeParams
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
    int useTripodMode; /**< Flag defining if inputs were captured with tripod (=1), or by hand (=0) */
    float movingPixelsRejectionThreshold; /**< Percentage to define how much there can be moving pixels in one image before dropping the frame from fusion. Suggested default value = 0.15 */
    int deghostingStrength; /**< Additional strength for deghosting. (0-100) 60=default, 0 = off, 1-30 => loose (easily merges moving targets), 70-100 => tight (larger value --> more easily moving) */
} VDClearShotRuntimeParams;


/* Added by qudao1@xiaomi.com */
typedef struct vdclearshotops {
    const char *(*GetVersion)(void);

    VDErrorType (*Initialize)(VDClearShotInitParameters, int, int, void **);

    int (*SetNRForBrightness)(int, float, void *);

    int (*Process)(VDYUVMultiPlaneImage *, VDYUVMultiPlaneImage *, VDClearShotRuntimeParams, int *,
                   void *);

    VDErrorType (*Release)(void **);
} VDClearshotOps_t;
int VDClearShot_APIOps_Init(VDClearshotOps_t *VCS_Ops);
int VDClearShot_APIOps_DeInit(void);
/* End of Added by qudao1@xiaomi.com */

/*
 * Initialize clear shot engine with specific numbers of processing cores
 * @param initparams Engine paramters to be used with the algorithm
 * @param numberOfCores Number of processor cores to be used by engine
 * @param activeCPUMask Bitmask to define which CPU cores are tried to be used for processing
 * @param engine Engine instance pointer
 * @return Error code indicating if initialization was succeed (VD_OK) or failed (VD_NOK)
 */
VDErrorType
VDClearShot_Initialize(VDClearShotInitParameters initparams, int numberOfCores, int activeCPUMask,
                       void **engine);

/*
 * Release clear shot engine
 * @param engine Engine instance pointer
 * @return Error code indicating if initialization was succeed (VD_OK) or failed (VD_NOK)
 */
VDErrorType VDClearShot_Release(void **engine);

/*
 * Run clear shot engine with specific processing params
 * @param params Processing parameters
 * @param inputImages Input images (VDClearShotInitParameters.numberOfFrames images)
 * @param output Output image
 * @param params Run-time parameters
 * @param selectedBaseFrame Number indicating which input frame from the array inputImages was selected as base frame
 * @param engine Engine instance pointer
 * @return Number of effective frames used for fusion
 */
int VDClearShot_Process(VDYUVMultiPlaneImage *inputImages, VDYUVMultiPlaneImage *output,
                        VDClearShotRuntimeParams params, int *selectedBaseFrame, void *engine);

/*
 * Add face roi data for an input images
 * @param frameIndex Index for the input image, e.g. 0, 1, 2, ..., numberOfFrames
 * @param numberOfFaces The number of faces detected in this image (frameIndex)
 * @param faceCoords Pointer to face roi data that is one dimensional array with length of 4*numberOfFaces,
 * e.g. faceCoords = [xmin1, ymin1, width1, height1, ..., xminN, yminN, widthN, heightN];
 * @param faceWeight Additional sharpness / brightness weight for face regions
 *     (total weight will be original weight (=distance to image center) + faceWeight).
 *     If faceWeight >= 1.0, only face region will be considered in calculation.
 * @param engine Engine instance pointer
 * @return Error code indicating if set was successful (VD_OK) or failed (VD_NOK)
 */
VDErrorType
VDClearShot_AddFaceROI(int frameIndex, int numberOfFaces, int *faceCoords, float faceWeight,
                       void *engine);

/*
 * Get sharpness and brighntess of the input frame.
 * @param input Pointer to YUV image
 * @param frameIndex Index for the input image, e.g. 0, 1, 2, ..., numberOfFrames
 * @param brightness Calculated brightness level
 * @param faceCoords Face coordinates in array of Nx4 [xmin, xmax, width, height,...]
 * @param faceCount Number of faces in the faceCoords table
 * @param faceWeight Additional sharpness / brightness weight for face regions
 *     (total weight will be original weight (=distance to image center) + faceWeight).
 *     If faceWeight >= 1.0, only face region will be considered in calculation.
 * @param engine Engine instance pointer
 * @return Calculated sharpness level of input image
 */
float VDClearShot_GetSharpness(VDYUVMultiPlaneImage *input, int frameIndex, float *brightness,
                               int *faceCoords, int faceCount, float faceWeight, void *engine);

/*
 * Set brightness based noise reduction strength. Brightness (=luminance value) is divided into 64
 * equally spaced grades: 0-3, 4-7, 8-11, ... 252-255.
 * @param brightnessIndex Brightness grade index to be adjusted (0-63)
 * @param strength Noise reduction strength for this brightness grades
 * @param engine Engine instance pointer
 * @return 0 If parameter set was succeed, otherwise -1
 */
int VDClearShot_SetNRForBrightness(int brightnessIndex, float strength, void *engine);

/*
 * Get VDClearShot library version string.
 * @return Version string
 */
const char *VDClearShot_GetVersion(void);

#ifdef __cplusplus
}
#endif

#endif /* __VD_CLEAR_SHOT_API_H__ */
