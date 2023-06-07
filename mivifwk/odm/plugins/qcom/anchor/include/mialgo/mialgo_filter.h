#ifndef MIALGO_FILTER_H__
#define MIALGO_FILTER_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_mat.h"

/**
* @brief Filter an image with the kernel.
* The function applies an arbitrary linear filter to an image. When the aperture is partially outside the image, the function interpolates outlier pixel values according to the specified border mode.
* The function does actually compute correlation, not the convolution.
* Supported MialgoMat data types are @ref MIALGO_MAT_U8.
* Output image must have the same size and number of channels an input image.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] ker_size       filter kernel size, ex ker_size = 3 means 3x3 kernel, Supported 3/5/7.
* @param[in] ker_data       1-dim kernel array.
* @param[in] type           pixel extrapolation method, @see MialgoBorderType
* @param[in] border_value   border value in case of constant border type
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoFilter2D(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size,
                        MI_F32 *ker_data, MialgoBorderType type,
                        MialgoScalar border_value);

/**
* @brief Filter an image with the kernel.
* The function applies an arbitrary linear filter to an image. When the aperture is partially outside the image, the function interpolates outlier pixel values according to the specified border mode.
* The function does actually compute correlation, not the convolution.
* Supported MialgoMat data types are @ref MIALGO_MAT_U8.
* Output image must have the same size and number of channels an input image.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] ker_size       filter kernel size, ex ker_size = 3 means 3x3 kernel, Supported 3/5/7.
* @param[in] ker_data       1-dim kernel array.
* @param[in] type           pixel extrapolation method, @see MialgoBorderType
* @param[in] border_value   border value in case of constant border type
* @param[in] impl           algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoFilter2DImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size,
                            MI_F32 *ker_data, MialgoBorderType type,
                            MialgoScalar border_value, MialgoImpl impl);

/**
* @brief Blurs an image using a Gaussian filter.
* The function filter2Ds the source image with the specified Gaussian kernel.
* Output image must have the same type and number of channels an input image.
* Supported MialgoMat data types are @ref MIALGO_MAT_U8.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] ker_size       gaussian kernel size, ex ker_size = 3 meaning 3x3 kernel,  Supported 3/5/7.
* @param[in] sigmaX         Gaussian kernel standard deviation in X direction.
* @param[in] sigmaY         Gaussian kernel standard deviation in Y direction.
* @param[in] type           pixel extrapolation method, @see MialgoBorderType
* @param[in] border_value   border value in case of constant border type
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoGaussianBlur(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size,
                            MI_F32 sigmaX, MI_F32 sigmaY,
                            MialgoBorderType type, MialgoScalar border_value);

/**
* @brief Blurs an image using a Gaussian filter.
* The function filter2Ds the source image with the specified Gaussian kernel.
* Output image must have the same type and number of channels an input image.
* Supported MialgoMat data types are @ref MIALGO_MAT_U8.
* @param[in] src            input MialgoMat/MialgoImg.Support U8c1/U8c3.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] ker_size       gaussian kernel size, ex ker_size = 3 meaning 3x3 kernel,  Supported 3/5/7.
* @param[in] sigmaX         Gaussian kernel standard deviation in X direction.
* @param[in] sigmaY         Gaussian kernel standard deviation in Y direction.
* @param[in] type           pixel extrapolation method, @see MialgoBorderType.For c3, it only supports CONSTANT.
* @param[in] border_value   border value in case of constant border type
* @param[in] impl           algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoGaussianBlurImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size,
                            MI_F32 sigmaX, MI_F32 sigmaY,
                            MialgoBorderType type,
                            MialgoScalar border_value,
                            MialgoImpl impl);

/**
* @brief Blurs an image using the box filter.
* The function smooths an image using the kernel:
* \f[\texttt{K} =  \alpha \begin{bmatrix} 1 & 1 & 1 &  \cdots & 1 & 1  \\ 1 & 1 & 1 &  \cdots & 1 & 1  \\ \hdotsfor{6} \\ 1 & 1 & 1 &  \cdots & 1 & 1 \end{bmatrix}\f]
* Output image must have the same type and number of channels an input image.
* Supported MialgoMat data types are @ref MIALGO_MAT_U8.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] ker_size       filter kernel size, ex ker_size = 3 means 3x3 kernel,  Supported 3/5/7.
* @param[in] type           pixel extrapolation method, @see MialgoBorderType
* @param[in] border_value   border value in case of constant border type
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoBoxFilter(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size,
                            MialgoBorderType type, MialgoScalar border_value);

/**
* @brief Blurs an image using the box filter.
* The function smooths an image using the kernel:
* \f[\texttt{K} =  \alpha \begin{bmatrix} 1 & 1 & 1 &  \cdots & 1 & 1  \\ 1 & 1 & 1 &  \cdots & 1 & 1  \\ \hdotsfor{6} \\ 1 & 1 & 1 &  \cdots & 1 & 1 \end{bmatrix}\f]
* Output image must have the same type and number of channels an input image.
* Supported MialgoMat data types are @ref MIALGO_MAT_U8.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] ker_size       filter kernel size, ex ker_size = 3 means 3x3 kernel,  Supported 3/5/7.
* @param[in] type           pixel extrapolation method, @see MialgoBorderType
* @param[in] border_value   border value in case of constant border type
* @param[in] impl           algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoBoxFilterImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size,
                            MialgoBorderType type,
                            MialgoScalar border_value,
                            MialgoImpl impl);

/**
* @brief Calculates the Laplacian of an image.
* The function calculates the Laplacian of the source image by adding up the second x and y derivatives calculated using the Sobel operator:
* \f[\texttt{dst} =  \Delta \texttt{src} =  \frac{\partial^2 \texttt{src}}{\partial x^2} +  \frac{\partial^2 \texttt{src}}{\partial y^2}\f]
* This is done when `ksize > 1`. When `ksize == 1`, the Laplacian is computed by filtering the image with the following \f$3 \times 3\f$ aperture:
* Supported MialgoMat data types are @ref MIALGO_MAT_U8.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] ker_size       filter kernel size, ex ker_size = 3 means 3x3 kernel,  Supported 1/3/5/7.
* @param[in] with_scale_abs means Convert the output from the Laplacian operator to a MIALGO_MAT_U8 image.
*                           if with_scale_abs 0, the size of the kernel corresponds to the output image format as follows: 1/3/5 -> MIALGO_MAT_S16, 7 -> MIALGO_MAT_S32
* @param[in] type           pixel extrapolation method, @see MialgoBorderType
* @param[in] border_value   border value in case of constant border type
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoLaplacian(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size,
                            MI_U8 with_scale_abs, MialgoBorderType type,
                            MialgoScalar border_value);

/**
* @brief Calculates the Laplacian of an image.
* The function calculates the Laplacian of the source image by adding up the second x and y derivatives calculated using the Sobel operator:
* \f[\texttt{dst} =  \Delta \texttt{src} =  \frac{\partial^2 \texttt{src}}{\partial x^2} +  \frac{\partial^2 \texttt{src}}{\partial y^2}\f]
* This is done when `ksize > 1`. When `ksize == 1`, the Laplacian is computed by filtering the image with the following \f$3 \times 3\f$ aperture:
* Supported MialgoMat data types are @ref MIALGO_MAT_U8.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] ker_size       filter kernel size, ex ker_size = 3 means 3x3 kernel,  Supported 1/3/5/7.
* @param[in] with_scale_abs means Convert the output from the Laplacian operator to a MIALGO_MAT_U8 image.
*                           if with_scale_abs 0, the size of the kernel corresponds to the output image format as follows: 1/3/5 -> MIALGO_MAT_S16, 7 -> MIALGO_MAT_S32
* @param[in] type           pixel extrapolation method, @see MialgoBorderType
* @param[in] border_value   border value in case of constant border type
* @param[in] impl           algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoLaplacianImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size,
                            MI_U8 with_scale_abs, MialgoBorderType type,
                            MialgoScalar border_value,
                            MialgoImpl impl);

/**
* @brief Blurs an image using the median filter.
* The function smoothes an image using the median filter with the \f$\texttt{ksize} \times \texttt{ksize}\f$ aperture.
* Output image must have the same type, size, and number of channels as the input image.
* The median filter uses @MIALGO_BORDER_REPLICATE internally to cope with border pixels, @see MialgoBorderType
* Supported MialgoMat data types are @ref MIALGO_MAT_U8.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] ker_size       filter kernel size, ex ker_size = 3 means 3x3 kernel,  Supported 3/5/7.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoMedianFilter(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size);

/**
* @brief Blurs an image using the median filter.
* The function smoothes an image using the median filter with the \f$\texttt{ksize} \times \texttt{ksize}\f$ aperture.
* Output image must have the same type, size, and number of channels as the input image.
* The median filter uses @MIALGO_BORDER_REPLICATE internally to cope with border pixels, @see MialgoBorderType
* Supported MialgoMat data types are @ref MIALGO_MAT_U8.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] ker_size       filter kernel size, ex ker_size = 3 means 3x3 kernel,  Supported 3/5/7.
* @param[in] impl           algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoMedianFilterImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst,
                                MI_S32 ker_size, MialgoImpl impl);

/**
* @brief Calculates the image derivatives using an extended Sobel operator for the x-derivative, or the y-derivative.
* The function is called with ( xorder = 1, yorder = 0, ksize = 1, 3, support input type --output type(u8--s16, u8--f32, f32--f32))
* or ( xorder = 0, yorder = 1, ksize = 1, 3, support input type --output type(u8--s16, u8--f32, f32--f32)) to calculate the first x- or y- image derivative.
* The sobel filter uses @MIALGO_BORDER_REPLICATE internally to cope with border pixels, @see MialgoBorderType
** ch_order only support MIALGO_MAT_CH_LAST_VAL
* @param[in] src            input MialgoMat.
* @param[in] dst            output MialgoMat.
* @param[in] dx             order of the derivative x.
* @param[in] dy             order of the derivative y.
* @param[in] ker_size       filter kernel size, current supports 1, 3.
* @param[in] scale          optional scale factor for the computed derivative values.
* @param[in] type           pixel extrapolation method, @see MialgoBorderType.
* @param[in] border_value   border value in case of constant border type.
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoSobelFilter(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 dx, MI_S32 dy,
                            MI_S32 ker_size, MI_F32 scale, MialgoBorderType type,
                            MialgoScalar border_value);

/**
* @brief Calculates the image derivatives using an extended Sobel operator for the x-derivative, or the y-derivative.
* The function is called with ( xorder = 1, yorder = 0, ksize = 1, 3, support input type --output type(u8--s16, u8--f32, f32--f32)).
* or ( xorder = 0, yorder = 1, ksize = 1, 3, (u8--s16, u8--f32, f32--f32)) to calculate the first x- or y- image derivative.
* The sobel filter uses @MIALGO_BORDER_REPLICATE internally to cope with border pixels, @see MialgoBorderType
* ch_order only support MIALGO_MAT_CH_LAST_VAL
* @param[in] src            input MialgoMat.
* @param[in] dst            output MialgoMat.
* @param[in] dx             order of the derivative x.
* @param[in] dy             order of the derivative y.
* @param[in] ker_size       filter kernel size, current supports 1, 3.
* @param[in] scale          optional scale factor for the computed derivative values.
* @param[in] type           pixel extrapolation method, @see MialgoBorderType.
* @param[in] border_value   border value in case of constant border type.
* @param[in] impl           algo running platform selection, @see MialgoImpl.
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoSobelFilterImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 dx, MI_S32 dy,
                            MI_S32 ker_size, MI_F32 scale, MialgoBorderType type,
                            MialgoScalar border_value, MialgoImpl impl);

/**
* @brief Blurs an image using the median filter.
* The function smoothes an image using the median filter with the \f$\texttt{ksize} \times \texttt{ksize}\f$ aperture.
* Output image must have the same type, size, and number of channels as the input image.
* The median filter uses @MIALGO_BORDER_REPLICATE internally to cope with border pixels, @see MialgoBorderType
* Supported MialgoMat data types are @ref MIALGO_MAT_U8.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] ker_size       filter kernel size, ex ker_size = 3 means 3x3 kernel,  Supported 3/5/7.
* @param[in] format         image format, currently only supports @MIALGO_IMG_NV12 and @MIALGO_IMG_NV21 . @see MialgoImgFormat.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoMedianFilterYUV(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size, MialgoImgFormat format);


/**
* @brief Blurs an image using the median filter.
* The function smoothes an image using the median filter with the \f$\texttt{ksize} \times \texttt{ksize}\f$ aperture.
* Output image must have the same type, size, and number of channels as the input image.
* The median filter uses @MIALGO_BORDER_REPLICATE internally to cope with border pixels, @see MialgoBorderType
* Supported MialgoMat data types are @ref MIALGO_MAT_U8.
* @param[in] src            input MialgoMat/MialgoImg.
* @param[in] dst            output MialgoMat/MialgoImg.
* @param[in] ker_size       filter kernel size, ex ker_size = 3 means 3x3 kernel,  Supported 3/5/7.
* @param[in] format         image format, currently only supports @MIALGO_IMG_NV12 and @MIALGO_IMG_NV21 . @see MialgoImgFormat.
* @param[in] impl           algo running platform selection, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoMedianFilterYUVImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 ker_size, MialgoImgFormat format, MialgoImpl impl);

/**
* @brief Impl parameters
*/
typedef struct MialgoGaussianBlurImplParam
{
    MI_S32 task_num;  // task number
} MialgoGaussianBlurImplParam;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_FILTER_H__

