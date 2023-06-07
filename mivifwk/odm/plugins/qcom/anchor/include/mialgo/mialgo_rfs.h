#ifndef MIALGO_RFS_H__
#define MIALGO_RFS_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_basic.h"
#include "mialgo_mat.h"
#include "mialgo_raw.h"


#define MIALGO_RFS_MAX_FACE_NUM  10
#define MIALGO_RFS_MAX_FRAME_NUM 12

/**
* @brief Raw frame selection algorithm running platform options
*/
typedef enum
{
    MIALGO_RFS_TARGET_NONE = 0,     /*!< No platform dependency */
    MIALGO_RFS_TARGET_ARM,          /*!< ARM architecture */
    MIALGO_RFS_TARGET_QCOM_SM8250,  /*!< DSP architecture */
    MIALGO_RFS_TARGET_INVALID,
} MialgoRFSTargetType;

typedef enum
{
    MIALGO_RFS_DEVICE_NONE = 0, /*!< No device dependency */
    MIALGO_RFS_DEVICE_INVALID,
} MialgoRFSDeviceType;

/**
* @brief Raw frame selection algorithm initialization configuration parameters.
* For PC (windows or linux OS): 
*           target = MIALGO_RFS_TARGET_NONE;
*           device = MIALGO_RFS_DEVICE_NONE;
* For Android or Arm devices:
*           target = MIALGO_RFS_TARGET_ARM;  // or other options(TODO)
*           device = MIALGO_RFS_DEVICE_NONE; // or other options(TODO)
* For Android or Cdsp devices:
*           target = MIALGO_RFS_TARGET_DSP;  // or other options(TODO)
*           device = MIALGO_RFS_DEVICE_NONE; // or other options(TODO)
*/
typedef struct MialgoRFSInitParam
{
    MI_S32              num_threads;    /*!< Parallelized thread number parameter */
    MialgoRFSTargetType target;         /*!< Running platform options */
    MialgoRFSDeviceType device;         /*!< Algorithm platform dependency */
} MialgoRFSInitParam;

/**
* @brief Raw frame selection algorithm face area parameter.
*/
typedef struct MialgoRFSFaceRoi
{
    MI_S32 face_num;                                /*!< The number of face areas in the frame */
    MI_F32 angle[MIALGO_RFS_MAX_FACE_NUM];          /*!< Mobile phone shooting attitude - angle */
    MialgoRect face_roi[MIALGO_RFS_MAX_FACE_NUM];   /*!< Face area location */
} MialgoRFSFaceRoi;

/**
* @brief Raw frame selection algorithm running configuration parameters.
*/
typedef struct MialgoRFSConfig
{
    MialgoBayerPattern bayer_pattern;   /*!< MIPI raw bayer Pattern */
    MI_S32 way_select;                  /*!< Raw frame selection algorithm selection -0/1 */
    MI_S32 xratio;                      /*!< Downsampling coefficient (internal use), must be 2/4/8 */
    MI_S32 yratio;                      /*!< Downsampling coefficient (internal use), must be 2/4/8 */
    MI_U8  face_roi_enable;             /*!< Enable face area processing */
    MialgoRFSFaceRoi face_roi[MIALGO_RFS_MAX_FRAME_NUM];          /*!< Face area information */
    MI_F32 face_angle_shift;            /*!< Face angle shift */
    MI_S32 min_face_num;                /*!< min face number */
    MI_S32 bit;                         /*!< MIPI raw bit, must be 10/12/16  10->MIPIRAW10BIT, 12->RawPlain16LSB12bit, 16->MIPIRAW12BIT 18->RawPlain16LSB10bit,*/
    MI_U8  mean_enable;                 /*!< enable clac resize_mat mean*/
    MI_F64 frame0_mean_luma;            /*!< first frame mean luma */
    MI_S32 frame_idx;                   /*!< frame index  */
    MI_U8  roi_enable;                  /*!< enable roi */
    MialgoRect roi_rect;                /*!< roi rect, roi_rect.x and roi_rect.width should be multipe of 4, roi_rect.y and roi_rect.height should be multipe of 2*/
    MI_S32 face_weight;                 /*!< face weight */
} MialgoRFSConfig;

/**
* @brief Raw frame selection algorithm initialization function.
* @param[in] param      @see MialgoRFSInitParam
* @param[in] error      @see mialgo_errorno.h
*
* @return
*        -<em>MialgoEngine</em> void* type value, mainly containing algorithm running context information.
*/
MialgoEngine MialgoRFSInit(const MialgoRFSInitParam *param, MI_S32 *error);

/**
* @brief Raw frame selection algorithm deinit function.
* @param[in] algo      Algorithm running context pointer.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoRFSUnit(MialgoEngine *algo);

/**
* @brief Raw frame selection algorithm sharpness calculation function, for single frame sharpness calculation. 
*        this func cannot use config->mean_enable, because the param need multi-frames.
* @param[in] algo       Algorithm running context pointer.
* @param[in] src        MIALGO_ARRAY pointer, stored single frame image data. (MIPIRaw10Bit)
* @param[in] sharpness  MI_U64 type, Image sharpness.
* @param[in] config     @see MialgoRFSConfig
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoRFSCalSharpness(MialgoEngine algo, MIALGO_ARRAY *src, MI_U64 *sharpness,
                            MialgoRFSConfig *config);

/**
* @brief The Raw frame selection algorithm selects the frame with the highest sharpness value, for multi-frame screening.
* @param[in] algo       Algorithm running context pointer.
* @param[in] src        MIALGO_ARRAY array pointer, stored multi-frame frame image data. (MIPIRaw10Bit)
* @param[in] frame_num  MI_S32 type, the number of images to be screening.
* @param[in] index      MI_S32 type, the array index for one frame with the largest sharpness value.
* @param[in] config     @see MialgoRFSConfig
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoRFSGetIndex(MialgoEngine algo, MIALGO_ARRAY **src, MI_S32 frame_num,
                            MI_S32 *index, MialgoRFSConfig *config);

/**
* @brief The Raw frame selection algorithm version number acquisition function.
* @param[in] algo       Algorithm running context pointer
*
* @return
*        -<em>MI_CHAR*</em> Version number information string.
*/
const MI_CHAR* MialgoRFSGetVer(MialgoEngine algo);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* MIALGO_RFS_H__ */
