/************************************************************************************

Filename  : mialgo_homography.h
Content   :
Created   : Nov. 20, 2019
Author    : Qiongqiong Zhang (zhangqiongqiong@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_HOMOGRAPHY_H__
#define MIALGO_HOMOGRAPHY_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"
#include "mialgo_features2d.h"

/**
* @brief Mialgo Find Homography function, finds a perspective transformation between two planes used RANSAC method.
* @param[in] src_points         MialgoKPArray array pointer, coordinates of the keypoints in the original plane.
* @param[in] dst_points         MialgoKPArray array pointer, coordinates of the keypoints in the target plane.
* @param[out] p_dstH            MIALGO_ARRAY 3*3 matrix, stored the homography matrix, support F32 and F64 type.
* @param[in] reproj_threshold   Maximum allowed reprojection error to treat a point pair as an inlier, usually between 1 and 10.
* @param[in] max_iters          The maximum number of RANSAC iterations.
* @param[in] confidence         Confidence level, between 0 and 1.

* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoFindHomography(MialgoKPArray *src_points , MialgoKPArray *dst_points, MIALGO_ARRAY *p_dstH,
                            MI_F32 reproj_threshold, MI_S32 max_iters, MI_F32 confidence);

/**
* @brief Mialgo Find Homography function, finds a perspective transformation between two planes used RANSAC method.
* @param[in] src_points         MialgoKPArray array pointer, coordinates of the keypoints in the original plane.
* @param[in] dst_points         MialgoKPArray array pointer, coordinates of the keypoints in the target plane.
* @param[out] p_dstH            MIALGO_ARRAY 3*3 matrix, stored the homography matrix, support F32 and F64 type.
* @param[in] reproj_threshold   Maximum allowed reprojection error to treat a point pair as an inlier, usually between 1 and 10.
* @param[in] max_iters          The maximum number of RANSAC iterations.
* @param[in] confidence         Confidence level, between 0 and 1.
* @param[in] impl               algo running platform selection, @see MialgoImpl.

* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoFindHomographyImpl(MialgoKPArray *src_points , MialgoKPArray *dst_points, MIALGO_ARRAY *p_dstH,
                            MI_F32 reproj_threshold, MI_S32 max_iters, MI_F32 confidence, MialgoImpl impl);

/**
* @brief Mialgo Find Homography function, finds a perspective transformation between two planes used RANSAC method.
* @param[in] src_points         MialgoKPArray array pointer, coordinates of the keypoints in the original plane.
* @param[in] dst_points         MialgoKPArray array pointer, coordinates of the keypoints in the target plane.
* @param[out] p_dstH            MIALGO_ARRAY 3*3 matrix, stored the homography matrix, support F32 and F64 type.
* @param[out] p_mask            Optional output mask set by RANSAC, if a keypoint is inliners, mask equals 1, otherwise, mask equals 0.
* @param[in] reproj_threshold   Maximum allowed reprojection error to treat a point pair as an inlier, usually between 1 and 10.
* @param[in] max_iters          The maximum number of RANSAC iterations.
* @param[in] confidence         Confidence level, between 0 and 1.
* @param[in] alpha_x            horizontal direction wight in compute reproject error, if don't need, set up 1.0f.
* @param[in] alpha_y            vertical direction wight in compute reproject error, if don't need, set up 1.0f.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoFindHomographyMask(MialgoKPArray *src_points , MialgoKPArray *dst_points, MIALGO_ARRAY *p_dstH, MI_S32 *p_mask,
                                MI_F32 reproj_threshold, MI_S32 max_iters, MI_F32 confidence, MI_F32 alpha_x, MI_F32 alpha_y);

/**
* @brief Mialgo Find Homography function, finds a perspective transformation between two planes used RANSAC method.
* @param[in] src_points         MialgoKPArray array pointer, coordinates of the keypoints in the original plane.
* @param[in] dst_points         MialgoKPArray array pointer, coordinates of the keypoints in the target plane.
* @param[out] p_dstH            MIALGO_ARRAY 3*3 matrix, stored the homography matrix, support F32 and F64 type.
* @param[out] p_mask            Optional output mask set by RANSAC, if a keypoint is inliners, mask equals 1, otherwise, mask equals 0.
* @param[in] reproj_threshold   Maximum allowed reprojection error to treat a point pair as an inlier, usually between 1 and 10.
* @param[in] max_iters          The maximum number of RANSAC iterations.
* @param[in] confidence         Confidence level, between 0 and 1.
* @param[in] alpha_x            horizontal direction wight in compute reproject error, if don't need, set up 1.0f.
* @param[in] alpha_y            vertical direction wight in compute reproject error, if don't need, set up 1.0f.
* @param[in] impl               algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoFindHomographyMaskImpl(MialgoKPArray *src_points , MialgoKPArray *dst_points, MIALGO_ARRAY *p_dstH, MI_S32 *p_mask,
                                    MI_F32 reproj_threshold, MI_S32 max_iters, MI_F32 confidence, MI_F32 alpha_x, MI_F32 alpha_y, MialgoImpl impl);

/**
* @brief Mialgo calculate Homography function, finds a perspective transformation between matched points pair(points number no less than 4).
* @param[in] src_points         MialgoKPArray array pointer, coordinates of the keypoints in the original plane.
* @param[in] dst_points         MialgoKPArray array pointer, coordinates of the keypoints in the target plane.
* @param[out] p_dstH            MIALGO_ARRAY 3*3 matrix, stored the homography matrix, support F32 and F64 type.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoCalHomography(MialgoKPArray *src_points , MialgoKPArray *dst_points, MIALGO_ARRAY *p_dstH);

/**
* @brief Mialgo calculate Homography function implement, finds a perspective transformation between matched points pair(points number no less than 4).
* @param[in] src_points         MialgoKPArray array pointer, coordinates of the keypoints in the original plane.
* @param[in] dst_points         MialgoKPArray array pointer, coordinates of the keypoints in the target plane.
* @param[out] p_dstH            MIALGO_ARRAY 3*3 matrix, stored the homography matrix, support F32 and F64 type.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoCalHomographyImpl(MialgoKPArray *src_points , MialgoKPArray *dst_points, MIALGO_ARRAY *p_dstH, MialgoImpl impl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_HOMOGRAPHY_H__

