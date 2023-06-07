/************************************************************************************

Filename  : mialgo_mat.h
Content   :
Created   : Nov. 19, 2019
Author    : guwei (guwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_MAT_H__
#define MIALGO_MAT_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_mem.h"

/**
* mat magic macro, Internal implementation use, do not recommend using directly.
*/
#define MIALGO_MAT_MAGIC_MASK           (0xff)
#define MIALGO_MAT_MAGIC_SHIFT          (12)
#define MIALGO_MAT_MAGIC_VAL            (0x12)
#define MIALGO_MAT_MAGIC                (MIALGO_MAT_MAGIC_VAL << MIALGO_MAT_MAGIC_SHIFT)

/**
* @brief mat magic macro, Internal implementation use, do not recommend using directly.
*/
#define MIALGO_MAT_CH_MASK              (0x3)
#define MIALGO_MAT_CH_SHIFT             (0)
#define MIALGO_MAT_CH_FIRST_VAL         (0x0)
#define MIALGO_MAT_CH_LAST_VAL          (0x1)

/**
* @brief mat type macro, Internal implementation use, do not recommend using directly.
*/
#define MIALGO_MAT_TYPE_MASK            (0x7)
#define MIALGO_MAT_TYPE_SHIFT           (2)
#define MIALGO_MAT_TYPE_IMG_VAL         (0x0)
#define MIALGO_MAT_TYPE_NUMERIC_VAL     (0x1)

/**
* @brief mat type macro, Internal implementation use, do not recommend using directly.
*/
#define MIALGO_MAT_MEM_MASK             (0x3)
#define MIALGO_MAT_MEM_SHIFT            (5)
#define MIALGO_MAT_DEFAULT_MEM_VAL      (0x0)
#define MIALGO_MAT_CL_MEM_VAL           (0x1)
#define MIALGO_MAT_HEAP_MEM_VAL         (0x2)

/**
* @brief flag of plane image(rrr...ggg...bbb...).
*/
#define MIALGO_MAT_FLAG_CH_FIRST        (MIALGO_MAT_CH_FIRST_VAL << MIALGO_MAT_CH_SHIFT)

/**
* @brief flag of nonplanar image(rgbrgbrgb...).
*/
#define MIALGO_MAT_FLAG_CH_LAST         (MIALGO_MAT_CH_LAST_VAL << MIALGO_MAT_CH_SHIFT)

/**
* @brief flag of image mat.
*/
#define MIALGO_MAT_FLAG_IMG_MAT         (MIALGO_MAT_TYPE_IMG_VAL << MIALGO_MAT_TYPE_SHIFT)

/**
* @brief not in use currently.
*/
#define MIALGO_MAT_FLAG_NUMERIC_MAT     (MIALGO_MAT_TYPE_NUMERIC_VAL << MIALGO_MAT_TYPE_SHIFT)

/**
* @brief not in use currently.
*/
#define MIALGO_MAT_FLAG_CL_MEM          (MIALGO_MAT_CL_MEM_VAL << MIALGO_MAT_MEM_SHIFT)
#define MIALGO_MAT_FLAG_HEAP_MEM        (MIALGO_MAT_HEAP_MEM_VAL << MIALGO_MAT_MEM_SHIFT)

/**
* @brief max number of channels in MialgoImg.
*/
#define MIALGO_IMG_MAX_PLANE            (4)

/**
* @brief Internal implementation use, do not recommend using directly.
*/
#define MIALGO_IMG_MASK                 (0xff)
#define MIALGO_IMG_SHIFT                (12)
#define MIALGO_IMG_VAL                  (0x11)
#define MIALGO_IMG_MAGIC                (MIALGO_IMG_VAL << MIALGO_IMG_SHIFT)

/**
* @brief the element type in the MialgoMat and MialgoImg.
*/
#define MIALGO_MAT_U8                   (0)             /*!< element type is u8 */
#define MIALGO_MAT_S8                   (1)             /*!< element type is s8 */
#define MIALGO_MAT_U16                  (2)             /*!< element type is u16 */
#define MIALGO_MAT_S16                  (3)             /*!< element type is s16 */
#define MIALGO_MAT_U32                  (4)             /*!< element type is u32 */
#define MIALGO_MAT_S32                  (5)             /*!< element type is s32 */
#define MIALGO_MAT_U64                  (6)             /*!< element type is u64 */
#define MIALGO_MAT_S64                  (7)             /*!< element type is s64 */
#define MIALGO_MAT_F32                  (8)             /*!< element type is f32 */
#define MIALGO_MAT_F64                  (9)             /*!< element type is f64 */

/**
* @brief MIALGO_ARRAY can be used to point to any object.
*/
typedef void                            MIALGO_ARRAY;

/**
* @brief The following macros for internal implementation using, do not recommend using directly.
*/
#define MIALGO_MAT_CH_ORDER(flag)       (((flag) >> MIALGO_MAT_CH_SHIFT) & MIALGO_MAT_CH_MASK)
#define MIALGO_MAT_TYPE(flag)           (((flag) >> MIALGO_MAT_TYPE_SHIFT) & MIALGO_MAT_TYPE_MASK)
#define MIALGO_CHECK_MAT_MAGIC(flag)    ((((flag) >> MIALGO_MAT_MAGIC_SHIFT) & MIALGO_MAT_MAGIC_MASK) == MIALGO_MAT_MAGIC_VAL)

#define MIALGO_CHECK_IMG_MAGIC(flag)    ((((flag) >> MIALGO_IMG_SHIFT) & MIALGO_IMG_MASK) == MIALGO_IMG_VAL)

#define MIALGO_IS_MAT(mat)              (((mat) != NULL) && MIALGO_CHECK_MAT_MAGIC(((MialgoMat *)(mat))->flag))
#define MIALGO_IS_IMG(img)              (((img) != NULL) && MIALGO_CHECK_IMG_MAGIC(((MialgoImg *)(img))->format))

#define MIALGO_MAT_IMG                  (MIALGO_MAT_FLAG_IMG_MAT | MIALGO_MAT_FLAG_CH_LAST)
#define MIALGO_MAT_PLANE_IMG            (MIALGO_MAT_FLAG_IMG_MAT | MIALGO_MAT_FLAG_CH_FIRST)

#define MIALGO_MAT_MEM_TYPE(flag)       (((flag) >> MIALGO_MAT_MEM_SHIFT) & MIALGO_MAT_MEM_MASK)

/**
* @brief image format.
*
* There are two image formats: nonnumeric and numeric
* A nonnumeric image is a generic image such as a grayscale image
* A numeric images are non-generic image formats, such as a 256X256 single-channel image, each pixel of type U32
*/
typedef enum
{
    MIALGO_IMG_INVALID = 0,                         /*!< invalid image format */

    // numeric
    MIALGO_IMG_NUMERIC = MIALGO_IMG_MAGIC + 100,    /*!< numeric image format */

    // yuv
    MIALGO_IMG_GRAY = MIALGO_IMG_MAGIC + 200,       /*!< gray image format */
    MIALGO_IMG_NV12,                                /*!< nv12 image format */
    MIALGO_IMG_NV21,                                /*!< nv21 image format */
    MIALGO_IMG_I420,                                /*!< i420 image format */
    MIALGO_IMG_NV12_16,                             /*!< nv12 16bit image format */
    MIALGO_IMG_NV21_16,                             /*!< nv21 16bit image format */
    MIALGO_IMG_I420_16,                             /*!< i420 16bit image format */
    MIALGO_IMG_P010,                                /*!< p010 image format */

    // ch last rgb
    MIALGO_IMG_RGB = MIALGO_IMG_MAGIC + 300,        /*!< nonplanar rgb image format */

    // ch first rgb
    MIALGO_IMG_PLANE_RGB = MIALGO_IMG_MAGIC + 400,  /*!< planar rgb image format */

    // raw
    MIALGO_IMG_RAW = MIALGO_IMG_MAGIC + 500,        /*!< raw image format */
    MIALGO_IMG_MIPIRAWPACK10,                       /*!< mipi raw pack 10bit image format */
    MIALGO_IMG_MIPIRAWUNPACK8,                      /*!< mipi raw unpack 8bit image format */
    MIALGO_IMG_MIPIRAWUNPACK16,                     /*!< mipi raw unpack 16bit image format */

    MIALGO_IMG_YUV444 = MIALGO_IMG_PLANE_RGB,       /*!< same as MIALGO_IMG_PLANE_RGB */
    MIALGO_IMG_NUM,
} MialgoImgFormat;

/**
* @brief make border type.
*/
typedef enum MialgoBorderType
{
    MIALGO_BORDER_TYPE_INVALID = 0,                 /*!< invalid type */
    MIALGO_BORDER_CONSTANT,                         /*!< constant make border type(`iiiiii|abcdefgh|iiiiiii`) */
    MIALGO_BORDER_REPLICATE,                        /*!< replicate make border type(`aaaaaa|abcdefgh|hhhhhhh`) */
    MIALGO_BORDER_REFLECT_101,                      /*!< replicate_101 make border type(`gfedcb|abcdefgh|gfedcba`) */
    MIALGO_BORDER_TYPE_NUM,
} MialgoBorderType;

/**
* @brief the shape of the image mat.
* An image mat is a 3dims data
*/
typedef struct
{
    MI_S32 c;                                   /*!< channels of image, most of Mialgo functions support 1,2,3 or 4 channels */
    MI_S32 h;                                   /*!< image hight in pixels */
    MI_S32 w;                                   /*!< image width in pixels */
    MI_S32 pitch;                               /*!< size of aligned image row in bytes */
    MI_S32 c_step;                              /*!< the number of bytes of the channel, which is meaningful in row-major order */
} MialgoShapeImg;

/**
* @brief represents image data, similar to mat in OpenCV, but much simpler than mat in OpenCV.
* use @see MialgoCreateMat to create a new MialgoMat
* use @see MialgoInitMat to init a MialgoMat with external buffer
* use @see MialgoDeleteMat to delete a MialgoMat
*
* How to create or init a new MialgoMat, the key is to set the dimensions, flags, and element types
* @code
    MI_S32 flag = MIALGO_MAT_FLAG_IMG_MAT | MIALGO_MAT_FLAG_CH_FIRST;
    MI_S32 sizes[3] = {1, 480, 640};
    MialgoMat *mat = MialgoCreateMat(3, sizes, MIALGO_MAT_U8, NULL, flag);
* @endcode
*
* Description of flag
*   MIALGO_MAT_FLAG_IMG_MAT is for image mat and the only supported flag currently
*   MIALGO_MAT_FLAG_CH_FIRST is for plane image image such as rrr...ggg...bbb...
*
* Description of sizes
*   arranged in order of (c, h, w)
*
* Description of mem_info
*   for the implementation of HVX, We have to know the fd of ion buf, mem_info is used to record this information.
*   If the MialgoMat is created by MialgoCreateMat, the mem_info will be filled by MialgoCreateMat.
*   If the MialgoMat is init by MialgoInitMat and will be used in HVX, must use MialgoMatSetMemInfo to set mem_info.
*
* Description of MialgoCreateMat
*   3 is Dimension of image and the only supported Dimension currently
*   MIALGO_MAT_U8 is element types, each pixel in the image is of type u8
*   NULL is alignment, passing NULL when alignment is not used
*/
typedef struct MialgoMat
{
    MI_S32 flag;                                /*!< identifier*/

    union
    {
        MialgoShapeImg img;
    } shape;                                    /*!< shape information*/

    MI_S32 dims;                                /*!< dimension*/
    MI_S32 elem_type;                           /*!< the element type*/

    MI_VOID *data;                              /*!< buffer pointer of mat*/
    MI_S64 data_bytes;                          /*!< the size of data buffer*/
    MI_VOID *raw;                               /*!< buffer pointer of raw data*/
    MI_S64 raw_bytes;                           /*!< the original size of mat's buffer, data bytes are equal to data_bytes plus reference count pointer size*/
    MI_S32 *ref_count;                          /*!< reference count pointer of mat*/
    MialgoMemInfo mem_info;                     /*!< memory info*/
} MialgoMat;

/**
* @brief represents image data, compared with MialgoMat, it is more intuitive and clearly explains the image format, width, height, pitch and pointer of each plane.
* use @see MialgoCreateImg to create a new MialgoImg
* use @see MialgoInitImg to init a MialgoImg with external buffer
* use @see MialgoDeleteImg to delete a MialgoImg
*
* How to create or init a new nonnumeric MialgoImg
* @code
    MI_S32 img_w = 600;
    MI_S32 img_h = 400;
    MialgoImg *img = MialgoCreateImg(mialgoSize(img_w, img_h), imgFmt(MIALGO_IMG_GRAY), mialgoScalarAll(0));
* @endcode
*
* How to create or init a new numeric MialgoImg
* @code
    MI_S32 img_w = 600;
    MI_S32 img_h = 400;
    MialgoImg *img = MialgoCreateImg(mialgoSize3D(img_w, img_h, 3), imgInfo(MIALGO_MAT_S16, 0), mialgoScalarAll(0));
* @endcode
*
* Description of mem_info
*   for the implementation of HVX, We have to know the fd of ion buf, mem_info is used to record this information.
*   If the MialgoImg is created by MialgoCreateImg, the mem_info will be filled by MialgoCreateImg.
*   If the MialgoMat is init by MialgoInitImg and will be used in HVX, must use MialgoImgSetMemInfo to set mem_info.
*
*/
typedef struct
{
    MialgoImgFormat format;                     /*!< image format, @see MialgoImgFormat*/
    MI_S32 w;                                   /*!< image width in pixels */
    MI_S32 h;                                   /*!< image hight in pixels */
    MI_S32 pitch[MIALGO_IMG_MAX_PLANE];         /*!< MI_S32 an array, stored four elements which are the size of aligned image row in bytes */
    MI_S64 plane[MIALGO_IMG_MAX_PLANE];         /*!< MI_S32 an array, stored four elements which are image plane*/
    MI_VOID *data[MIALGO_IMG_MAX_PLANE];        /*!< MI_VOID an array pointer, stored image data header address */
    MialgoMemInfo mem_info;                     /*!< memory info*/
} MialgoImg;

/**
* @brief image info, used at @see MialgoCreateImg and @see MialgoInitImg.
*
* Description of format
*   @see MialgoImgFormat
*
* Description of type
*   the element type in MialgoImg, for example, MIALGO_MAT_U8 or MIALGO_MAT_U32.
*
* Description of plane
*   0 is for nonplanar image(rgbrgbrgb...)
*   1 is for plane image(rrr...ggg...bbb...)
*
* for nonnumeric MialgoImg
*   use @see imgFmt to init MialgoImgInfo
*
* for numeric MialgoImg
*   use @see imgInfo to init MialgoImgInfo
*/
typedef struct
{
    MialgoImgFormat format;                     /*!< image format, @see MialgoImgFormat*/
    MI_S32 type;                                /*!< image type*/
    MI_S32 plane;                               /*!< image plane*/
} MialgoImgInfo;

/**
* @brief inline function for creating MialgoImgInfo quickly.
* @param[in] format     @see MialgoImgFormat
*
* @return
*        -<em>MialgoImgInfo</em> mimage info @see MialgoImgFormat.
*
* imgFmt can be only used for nonnumeric MialgoImg
* @code
    MI_S32 img_w = 600;
    MI_S32 img_h = 400;
    MialgoImg *img = MialgoCreateImg(mialgoSize(img_w, img_h), imgFmt(MIALGO_IMG_GRAY), mialgoScalarAll(0));
* @endcode
*/
static inline MialgoImgInfo imgFmt(MialgoImgFormat format)
{
    MialgoImgInfo info;

    info.format = format;
    info.type = -1;
    info.plane = -1;

    return info;
}

/**
* @brief inline function for creating MialgoImgInfo quickly.
* @param[in] type       @see MialgoImgInfo
* @param[in] plane      @see MialgoImgInfo
*
* @return
*        -<em>MialgoImgInfo</em> mimage info @see MialgoImgFormat.
*
* imgInfo can be only used for numeric MialgoImg
* @code
    MI_S32 img_w = 600;
    MI_S32 img_h = 400;
    MialgoImg *img = MialgoCreateImg(mialgoSize3D(img_w, img_h, 3), imgInfo(MIALGO_MAT_S16, 0), mialgoScalarAll(0));
* @endcode
*/
static inline MialgoImgInfo imgInfo(MI_S32 type, MI_S32 plane)
{
    MialgoImgInfo info;

    info.format = MIALGO_IMG_NUMERIC;
    info.type = type;
    info.plane = plane;

    return info;
}

/**
* @brief get the address of the specified channel for the MialgoMat.
* @param[in] mat        point to MialgoMat
* @param[in] ch         channel number
*
* @return
*        -<em>MI_VOID *</em> the address of the specified channel for the MialgoMat.
*
* MIALGO_IMG_MAT_CH_PTR only for plane image
* @code
    MI_U8 *src_head = (MI_U8 *)MIALGO_IMG_MAT_CH_PTR(src, c)
* @endcode
*
* Opencv provides a variety of methods to access image elements, such as mat.at<uchar>(row,col).
* Instead of doing what OpenCV does, we just provide three macros that get the address of the element.
* MIALGO_IMG_MAT_CH_PTR is the first way for plane image to get the address of specified channel
*/
#define MIALGO_IMG_MAT_CH_PTR(mat, ch)                      \
    ((MI_VOID *)((MI_U8 *)(mat)->data + (mat)->shape.img.c_step * (ch)))

/**
* @brief get the address of the specified row for the MialgoMat.
* @param[in] ch_ptr     the address of specified channel
* @param[in] pitch      image pitch
* @param[in] row        row number
*
* @return
*        -<em>MI_VOID *</em> the address of the specified row for the MialgoMat.
*
* MIALGO_IMG_MAT_CH_ROW_PTR only for plane image and must get the channel address firstly
*
* MIALGO_IMG_MAT_CH_ROW_PTR is the second way for plane image to get the address of specified row
*/
#define MIALGO_IMG_MAT_CH_ROW_PTR(ch_ptr, pitch, row)       \
    ((MI_VOID *)((MI_U8 *)(ch_ptr) + (pitch) * (row)))

/**
* @brief get the address of the specified row for the MialgoMat.
* @param[in] mat        point to MialgoMat
* @param[in] row        row number
*
* @return
*        -<em>MI_VOID *</em> the address of the specified row for the MialgoMat.
*
* MIALGO_IMG_MAT_ROW_PTR only for nonplane image

* MIALGO_IMG_MAT_ROW_PTR is the third way for plane image to get the address of specified row
*/
#define MIALGO_IMG_MAT_ROW_PTR(mat, row)                    \
    ((MI_VOID *)((MI_U8 *)(mat)->data + (mat)->shape.img.pitch * (row)))

/**
* @brief Mialgo Creates a Mat header and allocates the data.
* @param[in] dims           MI_S32 type, Dimension information of image, such as RGB image dimension 3. (currently supports dims of 3).
* @param[in] sizes          MI_S32 type, size information of each dimension. (arranged in order of (c, h, w)).
* @param[in] type           MI_S32 type, Mat element type.
* @param[in] strides        MI_S32 type, The stride of each dimension in bytes. (arranged in the order of (c, h, w), passing in NULL when strides are not used.)
* @param[in] flag           Identification, used in combination with | , such as channel priority.

* @return
*        --<em>MialgoMat *</em> MialgoMat* type value, the mat pointer created, or NULL identitying creation failed.
*
* How to create a new MialgoMat, the key is to set the dimensions, flags, and element types
* @code
    MI_S32 flag = MIALGO_MAT_FLAG_IMG_MAT | MIALGO_MAT_FLAG_CH_FIRST;
    MI_S32 sizes[3] = {1, 480, 640};
    MialgoMat *mat = MialgoCreateMat(3, sizes, MIALGO_MAT_U8, NULL, flag);
* @endcode
*
* Description of dims
*   3 is Dimension of image and currently the only supported Dimension
*
* Description of sizes
*   arranged in order of (c, h, w),
*
* Description of flag
*   MIALGO_MAT_FLAG_IMG_MAT is for image mat and currently the only supported flag
*   MIALGO_MAT_FLAG_CH_FIRST is for plane image such as rrr...ggg...bbb...
*
* Description of strides
*   passing NULL when strides is not used.
*   arranged in the order of (c, h, w), currently, only the stride of W is supported, while the stride of C and H will be supported later
*   the stride of each dimension is in bytes.
*       for nonplane mat, strides[2] = align_w * c * sizeof(data_type)
*       for plane mat, strides[2] = align_w * sizeof(data_type)
*   For a certain dimension, the stride can be set to 0, mialgo engine can calculate the actual strideaccording to the size of the image.
*/
MialgoMat* MialgoCreateMat(MI_S32 dims, MI_S32 *sizes, MI_S32 type, MI_S32 *strides, MI_S32 flag);

/**
* @brief Mialgo init a Mat header and allocates the data. (The difference from mialgo_create_mat is that the external buffer is used)
* @param[in] mat            MI_S32 type, Mat to be initialized.
* @param[in] dims           MI_S32 type, Dimension information of image, such as RGB image dimension 3. (currently supports dims of 3).
* @param[in] sizes          MI_S32 type, size information of each dimension. (arranged in order of (c, h, w)).
* @param[in] type           MI_S32 type, Mat element type.
* @param[in] strides        MI_S32 type, The stride of each dimension in bytes. (arranged in the order of (c, h, w), passing in NULL when strides are not used.)
* @param[in] flag           MI_S32 type, Identification, used in combination with | , such as channel priority.
* @param[in] data           MI_VOID type, External buffer.
*
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*
* How to init a new MialgoMat, the key is to set the dimensions, flags, and element types
* @code
    MialgoMat mat;
    MI_U8 *data = external_ptr;

    MI_S32 flag = MIALGO_MAT_FLAG_IMG_MAT | MIALGO_MAT_FLAG_CH_FIRST;
    MI_S32 sizes[3] = {1, 480, 640};

    MialgoInitMat(&mat, 3, sizes, MIALGO_MAT_U8, NULL, flag, data);
* @endcode
*
* Description of mat
*   mat can be a local variable, MialgoInitMat olny fill the mat with correct information
*
* Description of dims
*   3 is Dimension of image and currently the only supported Dimension
*
* Description of sizes
*   arranged in order of (c, h, w),
*
* Description of flag
*   MIALGO_MAT_FLAG_IMG_MAT is for image mat and currently the only supported flag
*   MIALGO_MAT_FLAG_CH_FIRST is for planar image such as rrr...ggg...bbb...
*
* Description of strides
*   passing NULL when strides is not used.
*   arranged in the order of (c, h, w), currently, only the stride of W is supported, while the stride of C and H will be supported later
*   the stride of each dimension is in bytes.
*       for nonplane mat, strides[2] = align_w * c * sizeof(data_type)
*       for plane mat, strides[2] = align_w * sizeof(data_type)
*   For a certain dimension, the stride can be set to 0, mialgo engine can calculate the actual stride according to the size of the image.
*   the align must match the actual memory layout of the external buffer.
*/
MI_S32 MialgoInitMat(MialgoMat *mat, MI_S32 dims, MI_S32 *sizes, MI_S32 type, MI_S32 *strides, MI_S32 flag, MI_VOID *data);

/**
* @brief Mialgo Delete mat function.
* The function decrements the matrix data reference counter by one and releases matrix header, If the reference counter reaches 0, it also deallocates the data.
* @param[in] mat           MialgoMat type, Mat to be deallocated, double pointer to the matrix.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoDeleteMat(MialgoMat **mat);

/**
* @brief Mialgo Creates an exact copy of the input mat function. (support for shallow-copy and deep-copy)
* @param[in] src           MialgoMat type, source mat.
* @param[in] deep_clone    MI_S32 type, identification of deep copy, setting deep_clone = MIALGO_TRUE means deep copy, setting deep_clone = MIALGO_FALSE means shallow copy.
* Both deep copy and shallow copy require calling MialgoDeleteMat() to free the memory of the header or data segment.
* The deep-copy will copy the buffer, the shallow-copy will copy the mat head, with sharing the buffer with the source mat, and reference count plusing one
* @return
*         --<em>MialgoMat *</em> MialgoMat* type value, the MialgoMat pointer created, or NULL identitying creation failed.
*/
MialgoMat* MialgoCloneMat(MialgoMat *src, MI_S32 deep_clone);

/**
* @brief Mialgo Crops image function.
* @param[in] src           MialgoMat type, source mat.
* @param[in] deep_crop    MI_S32 type, identification of deep crop.
* The deep-crop will copy the buffer, the shallow-crop will copy the mat head, with sharing the buffer with the source mat, and reference count plusing one
* @param[in] crop_rect     MialgoRect type, clipping region, sunch as crop_rect(1, 1, 480, 360).
* @return
*         --<em>MialgoMat *</em> MialgoMat* type value, the MialgoMat pointer created, or NULL identitying creation failed.
*/
MialgoMat* MialgoCropMat(MialgoMat *src, MI_S32 deep_crop, MialgoRect crop_rect);

/**
* @brief Mialgo copy mat function.
* @param[in] src           MialgoMat type, src mat.
* @param[in] dst           MialgoMat type, dst mat.
* @param[in] pt            MialgoPoint type, top left start copy point
* @return
*         <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoCopyMat(MialgoMat *src, MialgoMat *dst, MialgoPoint pt);

/**
* @brief Mialgo Get the mat element bytes.
* @param[in] mat           MialgoMat type.
* @return
*        <em>MI_S32</em>, the mat element bytes, 1 for U8, 2 for S16, and so on.
*/
MI_S32 MialgoGetMatElemBytes(MialgoMat *mat);

/**
* @brief Set the mem_info of some MialgoMat.
* @param[in] mat_list           MialgoMat list.
* @param[in] info_list          MialgoMemInfo list.
* @param[in] num                MialgoMat num.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*
* More description
*   for the implementation of HVX, We have to know the fd of ion buf, mem_info is used to record this information.
*   If the MialgoMat is created by MialgoCreateMat, the mem_info will be filled by MialgoCreateMat.
*   If the MialgoMat is init by MialgoInitMat and will be used in HVX, must use MialgoMatSetMemInfo to set mem_info.
*
*/
MI_S32 MialgoMatSetMemInfo(MialgoMat *mat_list, MialgoMemInfo *info_list, MI_S32 num);

/**
* @brief Mialgo Creates a MialgoImg header and allocates the image data.
* @param[in] size           MialgoSize3D type, Size information of image, including width, height and depth.
* @param[in] info           MialgoImgInfo type, Image information.
* @param[in] stride         MialgoScalar type, The stride of each dimension in bytes.

* @return
*        --<em>MialgoImg *</em> MialgoImg* type value, the MialgoImg pointer created, or NULL identitying creation failed.
*
* How to create a new nonnumeric MialgoImg
* @code
    MI_S32 img_w = 600;
    MI_S32 img_h = 400;
    MialgoImg *img = MialgoCreateImg(mialgoSize(img_w, img_h), imgFmt(MIALGO_IMG_GRAY), mialgoScalarAll(0));
* @endcode
*
* How to create or init a new numeric MialgoImg
* @code
    MI_S32 img_w = 600;
    MI_S32 img_h = 400;
    MialgoImg *img = MialgoCreateImg(mialgoSize3D(img_w, img_h, 3), imgInfo(MIALGO_MAT_S16, 0), mialgoScalarAll(0));
* @endcode
*
* Description of size
*   arranged in order of (w, h, c), can use mialgoSize or mialgoSize3D to init a MialgoSize3D
*
* Description of MialgoImgInfo
*   can use imgFmt or imgInfo to init a MialgoImgInfo
*   imgFmt can be only used for nonnumeric MialgoImg
*   imgInfo can be only used for numeric MialgoImg
*
* Description of stride
*   passing mialgoScalarAll(0) when stride is not used.
*   arranged in the order of (w, h, c), currently, only the stride of W is supported, while the stride of C and H will be supported later
*   the stride of each dimension is in bytes.
*       for nonplane img, scalar.val[0] = align_w * c * sizeof(data_type)
*       for plane img, scalar.val[0] = align_w * sizeof(data_type)
*   For a certain dimension, the stride can be set to 0, mialgo engine can calculate the actual stride according to the size of the image.
*/
MialgoImg* MialgoCreateImg(MialgoSize3D size, MialgoImgInfo info, MialgoScalar stride);

/**
* @brief Mialgo Initializes image function.
* @param[in] img           MialgoImg type, Image to be initialized.
* @param[in] size          MialgoSize3D type, Size information of image, including width, height and depth.
* @param[in] info          MialgoImgInfo type, Image information.
* @param[in] stride        MialgoScalar type, The stride of each dimension in bytes.
* @param[in] data          MI_VOID type, External buffer.
* @return
*         --<em>MI_S32</em> @see mialgo_errorno.h
*
* How to init a new nonnumeric MialgoImg
* @code
    MialgoImg img;
    MI_U8 *data = external_ptr;

    MI_S32 img_w = 600;
    MI_S32 img_h = 400;
    MialgoInitImg(&img, mialgoSize(w, h), imgFmt(MIALGO_IMG_NV12), mialgoScalarAll(0), data);
* @endcode
*
* How to create or init a new numeric MialgoImg
* @code
    MialgoImg img;
    MI_U8 *data = external_ptr;

    MI_S32 img_w = 600;
    MI_S32 img_h = 400;

    MialgoInitImg(&img, mialgoSize(w, h), imgInfo(MIALGO_MAT_U32, 0), mialgoScalarAll(0), data);
* @endcode
*
* Description of size
*   arranged in order of (w, h, c), can use mialgoSize or mialgoSize3D to init a MialgoSize3D
*
* Description of MialgoImgInfo
*   can use imgFmt or imgInfo to init a MialgoImgInfo
*   imgFmt can be only used for nonnumeric MialgoImg
*   imgInfo can be only used for numeric MialgoImg
*
* Description of stride
*   passing mialgoScalarAll(0) when stride is not used.
*   arranged in the order of (w, h, c), currently, only the stride of W is supported, while the stride of C and H will be supported later
*   the stride of each dimension is in bytes.
*       for nonplane img, scalar.val[0] = align_w * c * sizeof(data_type)
*       for plane img, scalar.val[0] = align_w * sizeof(data_type)
*   For a certain dimension, the stride can be set to 0, mialgo engine can calculate the actual stride according to the size of the image.
*   the stride must match the actual memory layout of the external buffer.
*/
MI_S32 MialgoInitImg(MialgoImg *img, MialgoSize3D size, MialgoImgInfo info, MialgoScalar stride, MI_VOID *data);

/**
* @brief Mialgo Creates an exact copy of the input image function.
* @param[in] src           MialgoImg type, source image.
* @return
*         --<em>MialgoImg *</em> MialgoImg* type value, the MialgoImg pointer created, or NULL identitying creation failed.
*/
MialgoImg* MialgoCloneImg(MialgoImg *src);

/**
* @brief Mialgo Deallocates the image header and the image data.
* @param[in] img           MialgoImg type, image to be deleted, double pointer to the matrix.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoDeleteImg(MialgoImg **img);

/**
* @brief Set the mem_info of some MialgoImg.
* @param[in] img_list           MialgoImg list.
* @param[in] info_list          MialgoMemInfo list.
* @param[in] num                MialgoImg num.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*
* More description
*   For the implementation of HVX, We have to know the fd of ion buf.
*   If the MialgoImg is created by MialgoCreateImg, the mem_info will be filled by MialgoCreateImg.
*   If the MialgoMat is init by MialgoInitImg and will be used in HVX, must use MialgoImgSetMemInfo to set mem_info.
*
*/
MI_S32 MialgoImgSetMemInfo(MialgoImg *img_list, MialgoMemInfo *info_list, MI_S32 num);

/**
* @brief Mialgo Allocates memory for image.
* @param[in] img         MialgoImg type, Image to be allocated the memory.
* @param[in] size        MialgoSize3D type, Size information of image, including width, height and depth.
* @param[in] info        MialgoImgInfo type, Image information.
* @param[in] stride      MialgoScalar type, The alignment information of each dimension.
* @return
*       <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoImgAllocData(MialgoImg *img, MialgoSize3D size, MialgoImgInfo info, MialgoScalar stride);

/**
* @brief Mialgo Releases the image data.
* @param[in] img           MialgoImg type, Image to release data.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoImgReleaseData(MialgoImg *img);

/**
* @brief Mialgo Get the image type.
* @param[in] img           MialgoImg type, Image to be operated.
* @return
*        <em>MI_S32</em>, for example MIALGO_MAT_U8, MIALGO_MAT_S8 and so on
*/
MI_S32 MialgoGetImgType(MialgoImg *img);

/**
* @brief Mialgo Get the image plane.
* @param[in] img           MialgoImg type, Image to be operated.
* @return
*        <em>MI_S32</em>, 0 means nonplane image, 1 meas plane image
*/
MI_S32 MialgoGetImgPlane(MialgoImg *img);

/**
* @brief Mialgo Get the image size information.
* @param[in] img           MialgoImg type, Image to be operated.
* @return
*          --<em>MialgoEngine</em>MialgoSize3D type value, the size information, including width, height and depth of image.
*/
MialgoSize3D MialgoGetImgSize(MialgoImg *img);

// utils

/**
* @brief Mialgo Returns MialgoMat matrix header for the input array that can be a matrix(MialgoMat type), an image (MialgoImg type), or multi-dimensional array.
* @param[in] array         MIALGO_ARRAY array pointer, Input array.
* @param[in] mat           MialgoMat type, Mat buffer.
* @return
*         --<em>MialgoEngine</em> MialgoMat* type value, the MialgoImg pointer created, or NULL identitying creation failed.
*/
MialgoMat* MialgoGetMat(MIALGO_ARRAY *array, MialgoMat *mat);

/**
* @brief Mialgo Returns MialgoImg matrix header for the input array that can be a matrix(MialgoMat type), or an image (MialgoImg type).
* @param[in] array         MIALGO_ARRAY array pointer, Input array.
* @param[in] mat           MialgoImg type, Image buffer.
* @return
*         --<em>MialgoEngine</em> MialgoImg* type value, the MialgoImg pointer created, or NULL identitying creation failed.
*/
MialgoImg* MialgoGetImg(MIALGO_ARRAY *array, MialgoImg *img, MialgoImgFormat format);

/**
* @brief Mialgo Adjust the size of mat function without copying the data.
* @param[in] mat           MialgoMat type, source mat.
* @param[in] sizes         MI_S32 type, New dimensions, in order by c, h, w. (when channel is preferred, guarantee the w constant after reshaping, but not support reshape when the channel is in the end).
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoReshapeMat(MialgoMat *mat, MI_S32 *sizes);

/**
* @brief Mialgo Fill the mat function.
* @param[in] mat           MialgoMat type, Source mat.
* @param[in] val           MI_S32 type, The value that filling the channel with.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoFillMat(MialgoMat *mat, MI_VOID *val);

/**
* @brief Mialgo Boundary expansion function. (only support Gray and Rgb format)
* @param[in] src           MIALGO_ARRAY array pointer, stored image data.
* @param[out] dst          MIALGO_ARRAY array pointer, stored result of image data after boundary expansion.
* @param[in] top           MI_S32 type, Top number of boundary expansion.
* @param[in] bottom        MI_S32 type, Bottom number of boundary expansion.
* @param[in] left          MI_S32 type, Left number of boundary expansion.
* @param[in] right         MI_S32 type, Right number of boundary expansion.
* @param[in] border_type   MI_S32 type, Type of boundary expansion.
* @param[in] border_value  MialgoScalar type, Value of boundary expansion.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoMakeBorder(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 top,
                                                MI_S32 bottom, MI_S32 left, MI_S32 right, MialgoBorderType border_type, MialgoScalar border_value);

/**
* @brief the implementation of conversion just like convertTo() in OpenCV, supported element types: MI_U8, MI_U16, MI_U32, MI_S8, MI_S16, MI_S32, MI_F32.
* @param[in]  src           the pointer to the source image; the supported image: MialgoImg or MialgoMat, the number of channel: 1 or 3.
* @param[out] dst           the pointer to the dst image; the supported image: MialgoImg or MialgoMat, the number of channel: 1 or 3.
* @param[in]  alpha         the float type multiplier, dst[i] = alpha * src[i] + beta.
* @param[in]  beta          the float type additon.
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoConvertTo(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_F32 alpha, MI_F32 beta);

/**
* @brief the implementation of conversion just like convertTo() in OpenCV, supported element types: MI_U8, MI_U16, MI_U32, MI_S8, MI_S16, MI_S32, MI_F32.
* @param[in]  src           the pointer to the source image; the supported image: MialgoImg or MialgoMat, the number of channel: 1 or 3.
* @param[out] dst           the pointer to the dst image; the supported image: MialgoImg or MialgoMat, the number of channel: 1 or 3.
* @param[in]  alpha         the float type multiplier, dst[i] = alpha * src[i] + beta.
* @param[in]  beta          the float type additon.
* @param[in]  impl          the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoConvertToImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_F32 alpha, MI_F32 beta, MialgoImpl impl);

/**
* @brief the implementation of mat(U8, 1/3/4 channel ) rotation.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] angle      The angle should be clockwise rotation angle, such as 90, 180, and 270.
* @param[out] dst       the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoRotation(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 angle);

/**
* @brief the implementation of mat(U8, 1/3/4 channel ) rotation.
* @param[in] src        the source image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] angle      The angle should be clockwise rotation angle, such as 90, 180, and 270.
* @param[in] dst        the result image pointer, and the supported image & element types: MialgoImg or MialgoMat.
* @param[in] impl       the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoRotationImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MI_S32 angle, MialgoImpl impl);

/**
* @brief the implementation of matrix transpositon; supported element types: MI_U8, MI_S16.
* @param[in]  src           the pointer to the source image; the supported image: MialgoImg or MialgoMat, the number of channel: 1.
* @param[out] dst           the pointer to the dst image; the supported image: MialgoImg or MialgoMat, the number of channel: 1.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoTranspose(MIALGO_ARRAY *src, MIALGO_ARRAY *dst);

/**
* @brief the implementation of matrix transpositon; supported element types: MI_U8, MI_S16.
* @param[in] src            the pointer to the source image; the supported image: MialgoImg or MialgoMat, the number of channel: 1.
* @param[in] dst            the pointer to the dst image; the supported image: MialgoImg or MialgoMat, the number of channel: 1.
* @param[in] impl           the function running platform configuration, @see MialgoImpl.
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoTransposeImpl(MIALGO_ARRAY *src, MIALGO_ARRAY *dst, MialgoImpl impl);

// debug
/**
* @brief Mialgo Prints the mat function.
* @param[in] mat           MialgoMat type, Source mat.
* @param[in]info           const MI_CHAR type, Tag information for printing.
* @param[in]data_num       MI_S32 type, The number of buffer data in mat for printing, when no printing data, set it to 0 when no .
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoPrintMat(MialgoMat *mat, const MI_CHAR *info, MI_S32 data_num);

/**
* @brief Mialgo Copy mat information and move it somewhere to store it function.
* @param[in] mat           MialgoMat type, Source mat.
* @param[in] file_name     const MI_CHAR type, The filename of data dump.
* @param[in] binary        MI_S32 type, Set to 1 for binary dump and 0 for text dump.
* @param[in] with_pad      MI_S32 type, Whether dump aligned data, setting to 1 means dumping aligned data, setting to 0 means not dumping aligned data
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoDumpMat(MialgoMat *mat, const MI_CHAR *file_name, MI_S32 binary, MI_S32 with_pad);

/**
* @brief Mialgo Writes mat formatted data to a string function.
* @param[in] mat           MialgoMat type, Source mat.
* @param[in] info          MI_S32 type, The information to write.
* @param[in] len           MI_S32 type, The maximum number of characters to write.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoMatString(MialgoMat *mat, MI_CHAR *info, MI_S32 len);

/**
* @brief Mialgo Prints the image function.
* @param[in] mat           MialgoImg type, Source mat.
* @param[in]info           const MI_CHAR type, Tag information for printing.
* @param[in]data_num       MI_S32 type, The number of buffer data in mat for printing, when no printing data, set it to 0 when no .
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoPrintImg(MialgoImg *img, const MI_CHAR *info, MI_S32 data_num);

/**
* @brief Mialgo Creates a Mat from a file.
* @param[in] fname          const MI_CHAR *, file name.
* @param[in] sizes          @see MialgoReadCreateMat.
* @param[in] type           @see MialgoReadCreateMat.
* @param[in] flag           @see MialgoReadCreateMat.

* @return
*        @see MialgoMat.
*/
MialgoMat* MialgoReadCreateMat(const MI_CHAR *file_name, MI_S32 sizes[3], MI_S32 type, MI_S32 flag);

/**
* @brief The function of add corresponding elements of two mats, only support U16 element type currently.
* @param[in] src0     One source single channel mat
* @param[in] src1     Another source single channel mats
* @param[out] dst     The destination single channel mat
*
* @return             <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoAddMat(MIALGO_ARRAY *src0, MIALGO_ARRAY *src1, MIALGO_ARRAY *dst);

/**
* @brief The function of add corresponding elements of two mats, only support U16 element type currently.
* @param[in] src0     One source single channel mat
* @param[in] src1     Another source single channel mats
* @param[out] dst     The destination single channel mat
* @param[in] impl     algo running platform selection, @see MialgoImpl.
*
* @return             <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoAddMatImpl(MIALGO_ARRAY *src0, MIALGO_ARRAY *src1, MIALGO_ARRAY *dst, MialgoImpl impl);

/**
* @brief The function of subtract the elements in the second mat from the corresponding elements in the first mat, only support U16 element type currently.
* @param[in] src0     The first single channel mat
* @param[in] src1     The Second single channel mat
* @param[out] dst     The destination single channel mat
*
* @return             <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoSubMat(MIALGO_ARRAY *src0, MIALGO_ARRAY *src1, MIALGO_ARRAY *dst);

/**
* @brief The function of subtract the elements in the second mat from the corresponding elements in the first mat, only support U16 element type currently.
* @param[in] src0     The first single channel mat
* @param[in] src1     The Second single channel mat
* @param[out] dst     The destination single channel mat
* @param[in] impl     algo running platform selection, @see MialgoImpl.
*
* @return             <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoSubMatImpl(MIALGO_ARRAY *src0, MIALGO_ARRAY *src1, MIALGO_ARRAY *dst, MialgoImpl impl);

/**
* @brief The function of multiplie corresponding elements in two mats, only support U16 element type currently.
* @param[in] src0     One source single channel mat
* @param[in] src1     Another source single channel mat
* @param[out] dst     The destination single channel mat
*
* @return             <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoMulMat(MIALGO_ARRAY *src0, MIALGO_ARRAY *src1, MIALGO_ARRAY *dst);

/**
* @brief The function of multiplie corresponding elements in two mats, only support U16 element type currently.
* @param[in] src0     One source single channel mat
* @param[in] src1     Another source single channel mat
* @param[out] dst     The destination single channel mat
* @param[in] impl     algo running platform selection, @see MialgoImpl.
*
* @return             <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoMulMatImpl(MIALGO_ARRAY *src0, MIALGO_ARRAY *src1, MIALGO_ARRAY *dst, MialgoImpl impl);

/**
* @brief The function of multiplie one mat by a scalar, only support U16 element type currently.
* @param[in] src0     The source single channel mat
* @param[in] scalar   MI_U16 type, the multiplier scalar
* @param[out] dst    The destination single channel mat
*
* @return             <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoMulScalar(MIALGO_ARRAY *src0, MI_U16 scalar, MIALGO_ARRAY *dst);

/**
* @brief The function of multiplie one mat by a scalar, only support U16 element type currently.
* @param[in] src0     The source single channel mat
* @param[in] scalar   MI_U16 type, the multiplier scalar
* @param[out] dst    The destination single channel mat
* @param[in] impl     algo running platform selection, @see MialgoImpl.
*
* @return             <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoMulScalarImpl(MIALGO_ARRAY *src0, MI_U16 scalar, MIALGO_ARRAY *dst, MialgoImpl impl);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_MAT_H__

