
/************************************************************************************

Filename  : mialgo_opticalflow.h
Content   :
Created   : Dec. 10, 2019
Author    : ltt (lutiantian@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_OPTICALFLOW_H__
#define MIALGO_OPTICALFLOW_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"
#include "mialgo_features2d.h"

// template, need to modify before using
typedef enum MialgoOpticalFlowMethod
{
    MIALGO_OPTICALFLOW = 0,
} MialgoOpticalFlowMethod;


typedef struct OpticalFlowWinSize
{
    MI_S32 width;                            /*!< width of the search window at each pyramid level */
    MI_S32 height;                           /*!< height of the search window at each pyramid level */
}OpticalFlowWinSize;


typedef struct OpticalFlowTermCriteria
{
    MI_S32 type;                             /*!< the type of iterations */
    MI_S32 maxcount;                         /*!< the specified maximum number of iterations */
    MI_F64 epsilon;                          /*!< the search window moves less than criteria.epsilon */
    MI_S32 bokeh_opt_flag;                   /*!< only used for bokeh adding select points judement, and other algorithms can ignore it, set it to 0*/
}OpticalFlowTermCriteria;


typedef enum OpticalFlowFlag
{
    MIALGOFLOW_USE_INITIAL_FLOW     = 4,     /*!< use the initial estimate, stored in nextpts; if the flag is not set, copy prevpts to nextpts and treat it as the initial estimate. */
    MIALGOFLOW_LK_GET_MIN_EIGENVALS = 8,     /*!< use the smallest eigenvalue as the error measurement */

} OpticalFlowFlag;


/**
* @brief the erode of images.
* @param[in] prev_img             first 8-bit input image or pyramid constructed by buildOpticalFlowPyramid.
* @param[in] next_img             second input image or pyramid of the same size and the same type as prev_mat.
* @param[in] prevpts              vector of 2D points for which the flow needs to be found; point coordinates must be single-precision floating-point numbers.
* @param[in] nextpts              output vector of 2D points (with single-precision floating-point coordinates) containing the calculated new positions of input features in the second image.
* @param[in] status               output status vector (of unsigned chars); each element of the vector is set to 1 if the flow for the corresponding features has been found, 
                                  otherwise, it is set to 0.
* @param[in] err                  output vector of errors; each element of the vector is set to an error for the corresponding feature, 
                                  type of the error measure can be set in flags parameter.
* @param[in] winsize              size of search window for each pyramid level.
* @param[in] maxlevel             layers of the pyramid.
* @param[in] termcrit             parameter specifying the termination condition of the iterative search algorithm.
* @param[in] flags                operation flags: OpticalFlowFlag.
* @param[in] mineigen_threshold   the algorithm calculates the minimum eigen value of a 2x2 normal matrix of optical flow equations.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/

MI_S32 MialgoOpticalFlow(MIALGO_ARRAY *prev_img, MIALGO_ARRAY *next_img, MialgoKPArray *prevpts, MialgoKPArray *nextpts, MI_U8 *status, MI_F32 *err, 
          OpticalFlowWinSize winsize, MI_S32 maxlevel, OpticalFlowTermCriteria termcrit, MI_S32 flags, MI_F32 mineigen_threshold);

/**
* @brief the erode of images.
* @param[in] prev_img             first 8-bit input image or pyramid constructed by buildOpticalFlowPyramid.
* @param[in] next_img             second input image or pyramid of the same size and the same type as prev_mat.
* @param[in] prevpts              vector of 2D points for which the flow needs to be found; point coordinates must be single-precision floating-point numbers.
* @param[in] nextpts              output vector of 2D points (with single-precision floating-point coordinates) containing the calculated new positions of input features in the second image.
* @param[in] status               output status vector (of unsigned chars); each element of the vector is set to 1 if the flow for the corresponding features has been found, 
                                  otherwise, it is set to 0.
* @param[in] err                  output vector of errors; each element of the vector is set to an error for the corresponding feature, 
                                  type of the error measure can be set in flags parameter.
* @param[in] winsize              size of search window for each pyramid level.
* @param[in] maxlevel             layers of the pyramid.
* @param[in] termcrit             parameter specifying the termination condition of the iterative search algorithm.
* @param[in] flags                operation flags: OpticalFlowFlag.
* @param[in] mineigen_threshold   the algorithm calculates the minimum eigen value of a 2x2 normal matrix of optical flow equations.
* @param[in] impl                 algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/

MI_S32 MialgoOpticalFlowImpl(MIALGO_ARRAY *prev_img, MIALGO_ARRAY *next_img, MialgoKPArray *prevpts, MialgoKPArray *nextpts, 
            MI_U8 *status, MI_F32 *err, OpticalFlowWinSize winsize, MI_S32 maxlevel, OpticalFlowTermCriteria termcrit, MI_S32 flags, 
            MI_F32 mineigen_threshold, MialgoImpl impl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_OPTICALFLOW_H__
