/************************************************************************************

Filename  : mialgo_features2d.h
Content   :
Created   : Aug. 21， 2019
Author    : Xinpeng Du (duxinpeng@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_FEATURES2D_H__
#define MIALGO_FEATURES2D_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"

/**
* @brief Data structure of single KeyPoint.
*/
typedef struct
{
    MI_F32 x;           /*!< coordinates of the keypoints */
    MI_F32 y;           /*!< coordinates of the keypoints */
    MI_F32 size;        /*!< diameter of the meaningful keypoint neighborhood */
    MI_F32 angle;       /*!< computed orientation of the keypoint (-1 if not applicable); it's in [0, 360) degrees and measured relative to image coordinate system, ie in clockwise. */
    MI_F32 response;    /*!< the response by which the most strong keypoints have been selected. Can be used for the further sorting or subsampling */
    MI_S32 octave;      /*!< octave (pyramid layer) from which the keypoint has been extracted */
    MI_S32 class_id;    /*!< object class (if the keypoints need to be clustered by an object they belong to) */
} MialgoKeyPoint;

/**
* @brief Data structure for KeyPoint detectors.
*/
typedef struct
{
    MialgoKeyPoint *p_arr;  /*!< Keypoints obtained from any feature detection algorithm */
    MI_U32 size;            /*!< the number of Keypoints  */
    MI_U32 capacity;        /*!< the number of Keypoints that can be held in currently allocated storage */
} MialgoKeyPointArray;

typedef MialgoKeyPointArray MialgoKPArray;

/**
* @brief Using to allocate memory for KeyPoints.
* @param[in] size        the number of Keypoints
*
* @return
*        -<em>MialgoKPArray*ngine</em> MialgoKPArray* type value, the allocated storage memory for MialgoKPArray.
*/
MialgoKPArray* MialgoAllocateKeyPointArray(const MI_U32 size);

/**
* @brief Using to allocate memory for KeyPoints.
* @param[in] size        the number of Keypoints
*
* @return
*        -<em>MialgoKPArray*ngine</em> MialgoKPArray* type value, the allocated storage memory for MialgoKPArray.
*/
MialgoKPArray* MialgoReallocateKeyPointArray(MialgoKPArray* ptr, const MI_U32 size);


/**
* @brief Using to deallocate memory for KeyPoints.
* @param[in] pptr        the pointer of MialgoKPArray
*
* @return null
*/
MI_VOID MialgoDeallocateKeyPointArray(MialgoKPArray **pptr);

/**
* @brief FAST Feature Detector Type.
*/
typedef enum MialgoFastMethod
{
    MIALGO_FAST_5_8 = 0,
    MIALGO_FAST_7_12 = 1,
    MIALGO_FAST_9_16 = 2
} MialgoFastMethod;

/**
* @brief FAST corners detector
* @param[in] p_src                  grayscale image where keypoints (corners) are detected.
* @param[in] pp_keypoints           keypoints detected on the image.
* @param[in] threshold              threshold on difference between intensity of the central pixel and pixels of a circle around this pixel.
* @param[in] nonmax_suppression     if 1, non-maximum suppression is applied to detected corners (keypoints).
* @param[in] method                 type one of the three neighborhoods as defined in the @MialgoFastMethod
*
* @return
*        -<em>MialgoEngine</em> void* type value, mainly containing algorithm running context information.
*/
MI_S32 MialgoFast(MIALGO_ARRAY *p_src, MialgoKPArray** pp_keypoints,
        MI_S32 threshold, const MI_U8 nonmax_suppression,
        const MialgoFastMethod method);

/**
* @brief FAST corners detector
* @param[in] p_src                  grayscale image where keypoints (corners) are detected.
* @param[in] pp_keypoints           keypoints detected on the image.
* @param[in] threshold              threshold on difference between intensity of the central pixel and pixels of a circle around this pixel.
* @param[in] nonmax_suppression     if 1, non-maximum suppression is applied to detected corners (keypoints).
* @param[in] method                 type one of the three neighborhoods as defined in the @MialgoFastMethod
* @param[in] impl                   algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MialgoEngine</em> void* type value, mainly containing algorithm running context information.
*/
MI_S32 MialgoFastImpl(MIALGO_ARRAY *p_src, MialgoKPArray** pp_keypoints,
        MI_S32 threshold, const MI_U8 nonmax_suppression,
        const MialgoFastMethod method, MialgoImpl impl);

/**
* @brief Orb Desciptor Extractor for ORB extractor algorithm
* @param[in] p_src                  grayscale image where keypoints (corners) are detected.
* @param[in] p_keypoints            keypoints detected on the image. note:between the positons of keypoints and border must greater 4 pixels
* @param[in] offset                 The offset of feature point coordinates relative to the image.
*                                   ex: the coordinates of feature points mapped to image are (kp.x + offset.x, kp.y + offset.y)
* @param[out] p_desc                the resulting descriptors, param must be elem_type=U8, size={1, pp_keypoints->size, 32}
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoOrbDescriptorExtractor(MIALGO_ARRAY *p_src,
                                    MialgoKPArray *p_keypoints,
                                    const MialgoPoint2D32f offset,
                                    MIALGO_ARRAY *p_desc);

/**
* @brief Orb Desciptor Extractor for ORB extractor algorithm
* @param[in] p_src                  grayscale image where keypoints (corners) are detected.
* @param[in] p_keypoints            keypoints detected on the image. note:between the positons of keypoints and border must greater 4 pixels
* @param[in] offset                 The offset of feature point coordinates relative to the image.
*                                   ex: the coordinates of feature points mapped to image are (kp.x + offset.x, kp.y + offset.y)
* @param[in] impl                   algo running platform selection, @see MialgoImpl.
* @param[out] p_desc                the resulting descriptors, param must be elem_type=U8, size={1, pp_keypoints->size, 32}
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoOrbDescriptorExtractorImpl(MIALGO_ARRAY *p_src,
                                        MialgoKPArray *p_keypoints,
                                        const MialgoPoint2D32f offset,
                                        MIALGO_ARRAY *p_desc, 
                                        MialgoImpl impl);

/**
* @brief shi-tomasi corners detector
* @param[in] p_src                   Input image where keypoints (corners) are detected, currently only supports MI_U8, single-channel image.
* @param[in] corners                 Output vector of detected corners.
* @param[in] max_corners             Maxinum number of corners to return, if there are more corners than are found, the strongest of them are returned.
* @param[in] quality_level           Parameter characterizing the minimal accepted quality of image corners.
* @param[in] min_distance            Minimum possible Euclidean distance between the returned corners.
* @param[in] block_size              Size of an average block for computing a derivative covariation matrix over each pixel neighborhood.
* @param[in] gradient_size           Aperture parameter for the Sobel operator.
* @param[in] use_harris              Indicating whether to use a Harris detector.
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoGoodFeatureToTrack(MIALGO_ARRAY *p_src, MialgoKPArray** corners,
                                 MI_S32 max_corners, MI_F64 quality_level, MI_F32 min_distance,
                                 MI_S32 block_size, MI_S32 gradient_size, bool use_harris);

/**
* @brief shi-tomasi corners detector
* @param[in] p_src                   Input image where keypoints (corners) are detected, currently only supports MI_U8, single-channel image.
* @param[in] corners                 Output vector of detected corners.
* @param[in] max_corners             Maxinum number of corners to return, if there are more corners than are found, the strongest of them are returned.
* @param[in] quality_level           Parameter characterizing the minimal accepted quality of image corners.
* @param[in] min_distance            Minimum possible Euclidean distance between the returned corners.
* @param[in] block_size              Size of an average block for computing a derivative covariation matrix over each pixel neighborhood.
* @param[in] gradient_size           Aperture parameter for the Sobel operator.
* @param[in] use_harris              Indicating whether to use a Harris detector.
* @param[in] impl                    algo running platform selection, @see MialgoImpl.
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoGoodFeatureToTrackImpl(MIALGO_ARRAY *p_src, MialgoKPArray** corners,
                                     MI_S32 max_corners, MI_F64 quality_level, MI_F32 min_distance,
                                     MI_S32 block_size, MI_S32 gradient_size, bool use_harris, MialgoImpl impl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_FEATURES2D_H__
