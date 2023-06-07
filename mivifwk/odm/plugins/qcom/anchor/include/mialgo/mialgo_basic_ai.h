/************************************************************************************

Filename  : mialgo_basic_ai.h
Content   :
Created   : Nov. 5, 2019
Author    : wuxiangwei (wuxiangwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_BASIC_AI_H__
#define MIALGO_BASIC_AI_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_mat.h"
#include "mialgo_cvtcolor.h"
#include "mialgo_raw.h"

/**
* @brief the implementation of softmax of 2 channel.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoSoftMax(MIALGO_ARRAY *src, MIALGO_ARRAY *dst);

/**
* @brief the implementation of softmax of 2 channel.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoSoftMaxImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoImpl impl);

/**
* @brief the implementation of interp of 2 channel.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoInterp(MIALGO_ARRAY *src, MIALGO_ARRAY *dst);

/**
* @brief the implementation of interp of 2 channel.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoInterpImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoImpl impl);



/**
* @brief CnnPreprocessMethod supported method of preprocess
*/
typedef enum 
{
    CNN_YUV_RES_CVT_ROT_DIV,
} CnnPreprocessMethod;

/**
* @brief CNN_YUV_RES_CVT_ROT_DIV preprocess method parameters
*/
typedef struct 
{
    MialgoCvtcolorMethod cvtmethod;
    MialgoRect rect;
    MI_S32 angle;
    MI_F32 coe;
} YuvResCvtRotDiv;

/**
* @brief the implementation of cnn yuv preprocess.
* @param[in] srcY       the source image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] srcUV      the source image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] method     Cnn Preprocess method, and only supported CNN_YUV_RES_CVT_ROT_DIV now。
* @param[in] param      the preprocess parameter pointer, and the supported void*.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoCnnYuvPreprocess(MIALGO_ARRAY *srcY, MIALGO_ARRAY *srcUV, MIALGO_ARRAY *dst,
                                CnnPreprocessMethod method, MI_VOID *param);

/**
* @brief the implementation of cnn yuv preprocess.
* @param[in] srcY       the source image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] srcUV      the source image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] method     Cnn Preprocess method, and only supported CNN_YUV_RES_CVT_ROT_DIV now。
* @param[in] param      the preprocess parameter pointer, and the supported void*.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoCnnYuvPreprocessImpl(MIALGO_ARRAY *srcY, MIALGO_ARRAY *srcUV, MIALGO_ARRAY *dst,
                                    CnnPreprocessMethod method, MI_VOID *param, MialgoImpl impl);


/**
* @brief CnnPostprocessMethod supported method of postprocess
*/
typedef enum 
{
    CNN_GARY_ROT_RES,
} CnnPostprocessMethod;

/**
* @brief CNN_GARY_ROT_RES postprocess method parameters
*/
typedef struct 
{
    MI_S32 angle;
    MialgoRect rect;
} GrayRotRes;

/**
* @brief the implementation of cnn gray postprocess.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] method     Cnn postprocess method, and only supported CNN_GARY_ROT_RES now。
* @param[in] param      the postprocess parameter pointer, and the supported void*.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoCnnGrayPostprocess(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                                CnnPostprocessMethod method, MI_VOID *param);

/**
* @brief the implementation of cnn gray postprocess.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] method     Cnn postprocess method, and only supported CNN_GARY_ROT_RES now。
* @param[in] param      the postprocess parameter pointer, and the supported void*.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoCnnGrayPostprocessImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                                    CnnPostprocessMethod method, MI_VOID *param, MialgoImpl impl);


/**
* @brief the implementation of cnn harr.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoHarr(MIALGO_ARRAY *src, MIALGO_ARRAY *dst);

/**
* @brief the implementation of cnn harr.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoHarrImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoImpl impl);

/**
* @brief the implementation of cnn reverse harr.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoRevHarr(MIALGO_ARRAY *src, MIALGO_ARRAY *dst);

/**
* @brief the implementation of cnn reverse harr.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoRevHarrImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoImpl impl);


/**
* @brief the implementation of harr.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstA       the result A image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstHVD     the result HVD image pointer, and the supported image & element types: MialgoImg or MialgoMat.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoHarrSep(MIALGO_ARRAY *src, MIALGO_ARRAY *dstA, MIALGO_ARRAY *dstHVD);

/**
* @brief the implementation of cnn harr.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstA       the result A image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstHVD     the result HVD image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoHarrSepImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dstA, MIALGO_ARRAY *dstHVD, MialgoImpl impl);

/**
* @brief the implementation of cnn reverse harr.
* @param[in] srcA       the source A image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] srcHVD     the source HVD image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dstHVD     the result HVD image pointer, and the supported image & element types: MialgoImg or MialgoMat.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoRevHarrSep(MIALGO_ARRAY *srcA, MIALGO_ARRAY *srcHVD, MIALGO_ARRAY *dst);

/**
* @brief the implementation of cnn reverse harr.
* @param[in] srcA       the source A image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] srcHVD     the source HVD image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoRevHarrSepImpl(MIALGO_ARRAY *srcA, MIALGO_ARRAY *srcHVD, MIALGO_ARRAY *dst, MialgoImpl impl);

/**
* @brief the implementation of cnn gray postprocess, neon request src is yuv format, opencl request src is rgb format.
* @param[in] src                        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat. only support u8c3
* @param[out] dst                       the result image pointer, and the supported image & element types: MialgoImg or MialgoMat. only support u8c3
* @param[in] coeffs                     Bilate Slice coeffs, float coeff[16][16][8][3][4]
* @param[in] ccm_mix_shift_slope        Bilate Slice other coeffs, it is made up of ccm/mix/shift/slope, float ccm[12]/mix[4]/shift[3][16]/slope[3][16]
* @param[in] param                      the postprocess parameter pointer, and the supported void*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoBilateSlice(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MIALGO_ARRAY *coeffs, MIALGO_ARRAY *ccm_mix_shift_slope);

/**
* @brief the implementation of cnn gray postprocess.
* @param[in] src                        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat. only support u8c3
* @param[out] dst                       the result image pointer, and the supported image & element types: MialgoImg or MialgoMat. only support u8c3
* @param[in] coeffs                     Bilate Slice coeffs, float coeff[16][16][8][3][4]
* @param[in] ccm_mix_shift_slope        Bilate Slice other coeffs, it is made up of ccm/mix/shift/slope, float ccm[12]/mix[4]/shift[3][16]/slope[3][16]
* @param[in] param                      the postprocess parameter pointer, and the supported void*
* @param[in] impl                       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoBilateSliceImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MIALGO_ARRAY *coeffs, MIALGO_ARRAY *ccm_mix_shift_slope, MialgoImpl impl);

/**
* @brief Bokeh style type.
*/
typedef enum MialgoAIBokehStyleType
{
    MIALGO_BOKEH_STYLE_INVALID = 0,
    MIALGO_BOKEH_STYLE_EXPAND,       /* expand method */
    MIALGO_BOKEH_STYLE_ROTATE,       /* rotate method */
    MIALGO_BOKEH_STYLE_SHRINK,       /* shrink method */
    MIALGO_BOKEH_STYLE_NUM
} MialgoAIBokehStyleType;

/**
* @brief Bokeh style parameters.
*/
typedef struct
{
    MI_F32 weight_cur;      /* weight of current point */
    MI_F32 center_x;        /* x coordinate of center point */
    MI_F32 center_y;        /* y coordinate of center point */
    MI_S32 add_num;         /* constant multiplier for calculating radius */
    MI_S32 init_num;        /* constant addend for calculating radius */
    MI_S32 interval;        /* step for selecting points*/
} MialgoAIBokehStyleParam;

/**
* @brief the implementation of bokeh with different style.
* @param[in] srcY        the source image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[in] srcUV       the source image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32,
*                        note that the input YUV image only support NV12/NV21 type.
* @param[out] dstY       the result image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U8.
* @param[out] dstUV      the result image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U8,
*                        note that the output YUV image also only support NV12/NV21 type.
* @param[in] mask        the source mask image pointer, and the supported image & element types: MialgoImg or MialgoMat, only support f32c1.
* @param[in] bokeh_type  the bokeh style method, @see MialgoAIBokehStyleType.
* @param[in] bokeh_param the bokeh style parameter pointer, @see MialgoAIBokehStyleParam.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoBokehStyle(MIALGO_ARRAY *srcY, MIALGO_ARRAY *srcUV, MIALGO_ARRAY *dstY, MIALGO_ARRAY *dstUV, MIALGO_ARRAY *mask,
                        MialgoAIBokehStyleType bokeh_type, MialgoAIBokehStyleParam *bokeh_param);

/**
* @brief the implementation of bokeh with different style.
* @param[in] srcY        the source image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[in] srcUV       the source image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32,
*                        note that the input YUV image only support NV12/NV21 type.
* @param[out] dstY       the result image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U8.
* @param[out] dstUV      the result image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U8,
*                        note that the output YUV image also only support NV12/NV21 type.
* @param[in] mask        the source mask image pointer, and the supported image & element types: MialgoImg or MialgoMat, only support f32c1.
* @param[in] bokeh_type  the bokeh style method, @see MialgoAIBokehStyleType.
* @param[in] bokeh_param the bokeh style parameter pointer, @see MialgoAIBokehStyleParam.
* @param[in] impl        algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoBokehStyleImpl(MIALGO_ARRAY *srcY, MIALGO_ARRAY *srcUV, MIALGO_ARRAY *dstY, MIALGO_ARRAY *dstUV, MIALGO_ARRAY *mask,
                            MialgoAIBokehStyleType bokeh_type, MialgoAIBokehStyleParam *bokeh_param, MialgoImpl impl);

/**
* @brief Impl parameters
*/
typedef struct MialgoAIBokehImplParam
{
    MI_S32 task_num;  // task number
} MialgoAIBokehImplParam;

/**
* @brief the implementation of the fusion of two images.
* @param[in] srcY    the source image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U8.
* @param[in] srcUV   the source image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U8.
*                    note that the input YUV image only support NV12 type.
* @param[in] blurY   the blurred image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[in] blurU   the blurred image(U channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[in] blurV   the blurred image(V channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[in] alpha   the weight image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[out] dstY   the result image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[out] dstUV  the result image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
*                    note that the output YUV image only support NV12 type.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoAIBokehImgFusion(MIALGO_ARRAY *srcY, MIALGO_ARRAY *srcUV, MIALGO_ARRAY *blurY, MIALGO_ARRAY *blurU, MIALGO_ARRAY *blurV,
                            MIALGO_ARRAY *alpha, MIALGO_ARRAY *dstY, MIALGO_ARRAY *dstUV);

/**
* @brief the implementation of the fusion of two images.
* @param[in] srcY    the source image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U8.
* @param[in] srcUV   the source image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U8.
*                    note that the input YUV image only support NV12 type.
* @param[in] blurY   the blurred image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[in] blurU   the blurred image(U channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[in] blurV   the blurred image(V channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[in] alpha   the weight image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[out] dstY   the result image(Y channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[out] dstUV  the result image(UV channel) pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
*                    note that the output YUV image only support NV12 type.
* @param[in] impl    the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoAIBokehImgFusionImpl(MIALGO_ARRAY *srcY, MIALGO_ARRAY *srcUV, MIALGO_ARRAY *blurY, MIALGO_ARRAY *blurU, MIALGO_ARRAY *blurV,
                                MIALGO_ARRAY *alpha, MIALGO_ARRAY *dstY, MIALGO_ARRAY *dstUV, MialgoImpl impl);

/**
* @brief the implementation of image mean blur When the integral image is known.
* @param[in] src         the source integral image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_F64.
*                        note that the height and width of integral image is bigger 1 than the destinate image.
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[in] ker_radius  filter kernel radius, ex ker_size = 3 means 7x7 kernel.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoAIBokehMeanBlur(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_radius);

/**
* @brief the implementation of image mean blur When the integral image is known.
* @param[in] src         the source integral image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_F64.
*                        note that the height and width of integral image is bigger 1 than the destinate image.
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[in] ker_radius  filter kernel radius, ex ker_size = 3 means 7x7 kernel.
* @param[in] impl        the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoAIBokehMeanBlurImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_radius, MialgoImpl impl);

/**
* @brief ideal coder type
*/
typedef enum
{
    MIALGO_IDEAL_INVALID = 0,
    MIALGO_IDEAL_DECODER,      /*!< decoder method */
    MIALGO_IDEAL_ENCODER,      /*!< encoder method */
} MialgoIdealCoderType;

/**
* @brief coordinate offset param  
*          Decoder MODE0: src_idx < 0, dst_idx < 0, input one/multi frame, output one/multi frame, calc all;
*          Decoder MODE1: src_idx >= 0 dst_idx >=0, input multi frame, output multi frame, calc the src_idx frame of src and save as the dst_idx frame of dst
*          Decoder MODE2: src_idx < 0  dst_idx >=0, input one frame, output multi-frame, clac src and save as the dst_idx frame of dst, dst_h /src_h should == frame_num
* @param bp                bayer pattern
* @param bl                coordinate offset
* @param src_idx           the index of source images, should be smaller than frame_num, if src_idx < 0, calc all images, if src_idx < 0 and dst_idx > 0, dst_h/src_h=frame_num
* @param dst_idx           the index of output images, should be samller than frame_num, if src_idx < 0, dst_idx should be smaller than 0 too;
* @param frame_num         the number of images
*/
typedef struct MialgoIdealCoderParam
{
    MialgoBayerPattern bp;
    MI_U16 bl[4];
    MI_S32 src_idx;
    MI_S32 dst_idx;
    MI_S32 frame_num;
} MialgoIdealCoderParam;

/**
* @brief the implementation of image decoder and image encoder.
* @param[in] src         the source image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.                    
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U16.
* @param[in] type        select MIALGO_IDEAL_DECODER or MIALGO_IDEAL_ENCODER.
* @param[in] param       the coordinate offset @see MialgoIdealCoderParam
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoIdealCoder(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoIdealCoderType type, MialgoIdealCoderParam param);

/**
* @brief the implementation of image decoder and image encoder.
* @param[in] src         the source image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.                    
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U16.
* @param[in] type        select MIALGO_IDEAL_DECODER or MIALGO_IDEAL_ENCODER.
* @param[in] param       the coordinate offset @see MialgoIdealCoderParam
* @param[in] impl        the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoIdealCoderImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoIdealCoderType type, MialgoIdealCoderParam param, MialgoImpl impl);

/**
* @brief the implementation of roi padding.
* @param[in] src         the source integral image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U16.
* @param[in] wmap        the threshold image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_S32.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoRoiPadding(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MIALGO_ARRAY *wmap);

/**
* @brief the implementation of roi padding.
* @param[in] src         the source integral image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U16.
* @param[in] wmap        the threshold image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_S32.
* @param[in] impl        the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoRoiPaddingImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MIALGO_ARRAY *wmap, MialgoImpl impl);

/**
* @brief partitionTf16 param
* @param box[4]          the coordinate of block
* @param offset[2]       the offset of block
* @param strides_src[4]  the stride of input data
* @param strides_dst[4]  the stride of output data
* @param num_n           the number of images
* @param num_patch_h     the height of block
* @param ratio           ratio param
* @param bl              
* @param wl
* @param step_zero       
* @param quant_ratio 
* @param task_num        thread number
* @param enable_v2       if enable_v2 != 0, partitiontf16 run with version 2, else run with old version 
*/
typedef struct MialgoPartitionTf16Param
{
    MI_S32 box[4];
    MI_S32 offset[2];
    MI_S32 strides_src[4];
    MI_S32 strides_dst[4];
    MI_S32 num_n;
    MI_S32 num_patch_h;
    MI_S32 num_patch_w;
    MI_F32 ratio;
    MI_S32 bl;
    MI_S32 wl;
    MI_S32 step_zero;
    MI_F32 quant_ratio;
    MI_S32 task_num;
    MI_S32 enable_v2;
    MI_S32 enable_input_multi;
} MialgoPartitionTf16Param;

/**
* @brief the implementation of images partition.
* @param[in] src         the source image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.                    
* @param[out] dst       the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U16.
* @param[in] param       Param about partitions, @see struct MialgoPartitionTf16Param
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoPartitionTf16(MIALGO_ARRAY **src, MIALGO_ARRAY *dst, MialgoPartitionTf16Param param);

/**
* @brief the implementation of images partition.
* @param[in] src         the source image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.                 
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U16.
* @param[in] param       Param about partitions, @see struct MialgoPartitionTf16Param
* @param[in] impl        the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoPartitionTf16Impl(MIALGO_ARRAY **src, MIALGO_ARRAY *dst, MialgoPartitionTf16Param param, MialgoImpl impl);

#define CROP_MAX_FRAMES 16

/**
* @brief CropAndQuantize param
* @param frame_num                       the num of input frames, max value is 10
* @param input_shape[4]                  the shape of src  (n,h,w,c=4)
* @param crop_shape[4]                   the shape of crop (1,h,w,n*c)
* @param size[2]                         the size of crop
* @param coor[2]                         the index of current crop
* @param ratio[CROP_MAX_FRAMES]          ratio param corresponding to each frame (max frame is 10)
* @param overlap[2]                      Used to calculate the starting coordinates of the current crop in the entire image
* @param white_level                     white_level value
* @param black_level[4]                  black level value of channel
* @param crop_offset                     quantize param      
* @param crop_scale                      quantize param
* @param task_num                        thread number
*/
typedef struct MialgoCropAndQuantizeParam
{
    MI_S32 frame_num;
    MI_S32 input_shape[4];
    MI_S32 crop_shape[4];            
    MI_S32 size[2];
    MI_S32 coor[2];
    
    MI_F32 ratio[CROP_MAX_FRAMES];
    MI_S32 overlap[2];
    MI_S32 white_level;
    MI_S32 black_level[4];
    MI_S32 crop_offset;
    MI_F32 crop_scale;
    MI_S32 task_num;
} MialgoCropAndQuantizeParam;

/**
* @brief the implementation of images CropAndQuantize.
* @param[in] src         the source image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.                    
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U16.
* @param[in] param       Param about CropAndQuantize, @see struct MialgoCropAndQuantizeParam
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoCropAndQuantize(MIALGO_ARRAY **src, MIALGO_ARRAY *dst, MialgoCropAndQuantizeParam param);

/**
* @brief the implementation of images CropAndQuantize.
* @param[in] src         the source image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.                 
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U16.
* @param[in] param       Param about CropAndQuantize, @see struct MialgoCropAndQuantizeParam
* @param[in] impl        the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoCropAndQuantizeImpl(MIALGO_ARRAY **src, MIALGO_ARRAY *dst, MialgoCropAndQuantizeParam param, MialgoImpl impl);

/**
* @brief mergeTf16  parameters.
*/
typedef struct MialgoMergeTf16Param
{
    MI_S32 valid[4];                 /*!< target box coordinates */
    MI_S32 coor[2];                  /*!< condition */
    MI_S32 size[2];                  /*!< condition */
    MI_S32 offset[2];                /*!< offset */
    MI_S32 strides_src[3];           /*!< stride of src */
    MI_S32 strides_dst[3];           /*!< stride of dst */
    MI_S32 num_patch_h;              /*!< height of input image */
    MI_S32 num_patch_w;              /*!< width of input image */
    MI_S32 bl;                       /*!< black level */
    MI_S32 wl;                       /*!< white level */
    MI_S32 step_zero;                /*!< step */
    MI_F32 dequant_ratio;            /*!< dequant_ratio */

    MI_F32 *mweights_row;            /*!< weight of row */
    MI_F32 *mweights_col;            /*!< weight of col */

    MI_S32 overlap_row;              /*!< overlap of row */
    MI_S32 overlap_col;              /*!< overlap of col */
} MialgoMergeTf16Param;

/**
* @brief the implementation of merge.
* @param[in] src         the source integral image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U16.
* @param[in] param       the merge parameter , @see MialgoMergeTf16Param.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoMergeTf16(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoMergeTf16Param param);

/**
* @brief the implementation of merge.
* @param[in] src         the source integral image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U16.
* @param[in] param       the merge parameter , @see MialgoMergeTf16Param.
* @param[in] impl        the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoMergeTf16Impl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoMergeTf16Param param, MialgoImpl impl);

/**
* @brief MergePlusTf16  parameters.
*/
typedef struct MialgoMergePlusTf16Param
{
    MI_S32 valid[4];                 /*!< target box coordinates */
    MI_S32 coor[2];                  /*!< condition */
    MI_S32 size[2];                  /*!< condition */
    MI_S32 offset[2];                /*!< offset */
    MI_S32 strides_src[3];           /*!< stride of src */
    MI_S32 strides_dst[3];           /*!< stride of dst */
    MI_S32 num_patch_h;              /*!< height of input image */
    MI_S32 num_patch_w;              /*!< width of input image */
    MI_S32 *bl;                      /*!< black level */
    MI_S32 wl;                       /*!< white level */
    MI_S32 step_zero;                /*!< step */
    MI_F32 *dequant_ratio;            /*!< dequant_ratio */

    MI_F32 *mweights_row;            /*!< weight of row */
    MI_F32 *mweights_col;            /*!< weight of col */

    MI_S32 overlap_row;              /*!< overlap of row */
    MI_S32 overlap_col;              /*!< overlap of col */
}MialgoMergePlusTf16Param;

/**
* @brief the implementation of merge.
* @param[in] src         the source integral image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U16.
* @param[in] param       the merge parameter , @see MialgoMergePlusTf16Param.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoMergePlusTf16(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoMergePlusTf16Param param);

/**
* @brief the implementation of merge.
* @param[in] src         the source integral image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.
* @param[out] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U16.
* @param[in] param       the merge parameter , @see MialgoMergePlusTf16Param.
* @param[in] impl        the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoMergePlusTf16Impl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoMergePlusTf16Param param, MialgoImpl impl);

/**
* @brief the implementation of MatMulSip.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] matrix     the matrix image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoMatMulSip(MIALGO_ARRAY *src, MIALGO_ARRAY *matrix, MIALGO_ARRAY *dst);

/**
* @brief the implementation of MatMulSip.
* @param[in] src        the source  image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] matrix     the matrix image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
**/
MI_S32 MialgoMatMulSipImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *matrix, MIALGO_ARRAY *dst, MialgoImpl impl);

/**
* @brief the param of aes
*/
#define AES_BLOCKLEN 16
#define AES_KEYLEN 16       /* key length in bytes */
#define AES_keyExpSize 96   /* roundkey length */ 
#define NB 4
#define NK 4                /* The number of 32 bit words in a key */
#define NR 5                /* The number of rounds in AES Cipher */

/**
* @brief the param of KeyIv
*/
typedef struct KeyIv
{
    MI_U8 key[AES_BLOCKLEN]; /* key of AES */
    MI_U8 iv[AES_BLOCKLEN];  /* iv of AES */
} KeyIv;

/**
* @brief the param of AesCtx
*/
typedef struct AesCtx
{
    MI_U8 round_key[AES_keyExpSize]; /* round key of AES */
    MI_U8 iv[AES_BLOCKLEN]; /* iv of AES */
} AesCtx;

/**
* @brief the implementation of MatMulSip.
* @param[in] ctx       the AesCtx pointer, @see AesCtx.
* @param[in] KeyIv     the KeyIv pointer, @see KeyIv.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoAesInitCtxIv(AesCtx *ctx, KeyIv *keyiv);

/**
* @brief the implementation of AesEcbDecryptImpl.
* @param[in] ctx        the AesCtx pointer, @see AesCtx.
* @param[in] src        the matrix image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoAesEcbDecryptImpl(AesCtx *ctx, MIALGO_ARRAY *src, MialgoImpl impl);

/**
* @brief the implementation of AesEcbDecrypt.
* @param[in] ctx        the AesCtx pointer, @see AesCtx.
* @param[in] src        the matrix image pointer, and the supported image & element types: MialgoImg or MialgoMat.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoAesEcbDecrypt(AesCtx *ctx, MIALGO_ARRAY *src);

/**
* @brief the implementation of AesEcbEncryptImpl.
* @param[in] ctx        the AesCtx pointer, @see AesCtx.
* @param[in] src        the matrix image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoAesEcbEncryptImpl(AesCtx *ctx, MIALGO_ARRAY *src, MialgoImpl impl);

/**
* @brief the implementation of AesEcbEncrypt.
* @param[in] ctx        the AesCtx pointer, @see AesCtx.
* @param[in] src        the matrix image pointer, and the supported image & element types: MialgoImg or MialgoMat.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoAesEcbEncrypt(AesCtx *ctx, MIALGO_ARRAY *src);

/**
* @brief Ai preprocess parameters.
*/
typedef struct MialgoBox
{
    MI_S32 height; /* height of box */
    MI_S32 width;  /* width of box */
    MI_S32 x;      /* x coordinate of upper left corner */
    MI_S32 y;      /* y coordinate of upper left corner */
} MialgoBox;

typedef struct MialgoAiPreproParam
{
    MialgoBox box; /* box */
    MI_S32 format; /* image format(RGB=0/RGBA=1) */
    MI_S32 angle;  /* angle (0,1,2,3->0/90/180/270) */
} MialgoAiPreproParam;

/**
* @brief the implementation of ai preprocess.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[out] dst       the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U8.
* @param[in] param      the ai preprocess parameter pointer, @see MialgoAiPreproParam.
* @param[in] impl       algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoSaliencyPreprocessImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoAiPreproParam *param, MialgoImpl impl);

/**
* @brief the implementation of ai preprocess.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_F32.
* @param[out] dst       the result image pointer, and the supported image & element types: MialgoImg or MialgoMat & MIALGO_MAT_U8.
* @param[in] param      the ai preprocess parameter pointer, @see MialgoAiPreproParam.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoSaliencyPreprocess(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoAiPreproParam *param);

/**
* @brief Lens shading parameters.
*/
typedef struct MialgoLensShadingParam
{
    MI_S32 lsc_table_h;          /* height of lsc table */
    MI_S32 lsc_table_w;          /* width of lsc table */
    MI_S32 height;               /* height of src image */
    MI_S32 width;                /* width of src image */
    MI_S32 bl;                   /* black level */
    MI_S32 wl;                   /* white level */
} MialgoLensShadingParam;

/**
* @brief the implementation of image lens shading.
* @param[in] src            the source integral image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.
* @param[in] lsc_table_gr   the lsc_table_gr image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_F32.
* @param[in] lsc_table_r    the lsc_table_r image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_F32.
* @param[in] lsc_table_gb   the lsc_table_gb image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_F32.
* @param[in] lsc_table_b    the lsc_table_b image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_F32.
* @param[in] param          the lens shading parameters pointer, @see MialgoLensShadingParam.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoLensShadingCorrect(MIALGO_ARRAY *src, MIALGO_ARRAY *lsc_table_gr, MIALGO_ARRAY *lsc_table_r, 
                                MIALGO_ARRAY *lsc_table_gb, MIALGO_ARRAY *lsc_table_b, MialgoLensShadingParam param);

/**
* @brief the implementation of image lens shading.
* @param[in] src            the source integral image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_U16.
* @param[in] lsc_table_gr   the lsc_table_gr image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_F32.
* @param[in] lsc_table_r    the lsc_table_r image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_F32.
* @param[in] lsc_table_gb   the lsc_table_gb image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_F32.
* @param[in] lsc_table_b    the lsc_table_b image pointer, and the supported image & element types: MialgoImg or MialgoMat, MIALGO_MAT_F32.
* @param[in] param          the lens shading parameters pointer, @see MialgoLensShadingParam.
* @param[in] impl           the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoLensShadingCorrectImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *lsc_table_gr, MIALGO_ARRAY *lsc_table_r,
                                    MIALGO_ARRAY *lsc_table_gb, MIALGO_ARRAY *lsc_table_b, MialgoLensShadingParam param, MialgoImpl impl);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_BASIC_AI_H__