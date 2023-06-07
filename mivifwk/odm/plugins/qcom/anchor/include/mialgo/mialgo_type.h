/************************************************************************************

Filename  : mialgo_type.h
Content   :
Created   : Nov. 21, 2019
Author    : guwei (guwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_TYPE_H__
#define MIALGO_TYPE_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
* @brief Redefinition of the C basic data type.
*/
#ifndef MI_U8_DEFINED
#define MI_U8_DEFINED
typedef unsigned char               MI_U8;
#endif // MI_U8_DEFINED

#ifndef MI_S8_DEFINED
#define MI_S8_DEFINED
typedef signed char                 MI_S8;
#endif // MI_S8_DEFINED

#ifndef MI_U16_DEFINED
#define MI_U16_DEFINED
typedef unsigned short              MI_U16;
#endif // MI_U16_DEFINED

#ifndef MI_S16_DEFINED
#define MI_S16_DEFINED
typedef signed short                MI_S16;
#endif // MI_S16_DEFINED

#ifndef MI_U32_DEFINED
#define MI_U32_DEFINED
typedef unsigned int                MI_U32;
#endif // MI_U32_DEFINED

#ifndef MI_S32_DEFINED
#define MI_S32_DEFINED
typedef signed int                  MI_S32;
#endif // MI_S32_DEFINED

#ifndef MI_U64_DEFINED
#define MI_U64_DEFINED
typedef unsigned long long          MI_U64;
#endif // MI_U64_DEFINED

#ifndef MI_S64_DEFINED
#define MI_S64_DEFINED
typedef signed long long            MI_S64;
#endif // MI_S64_DEFINED

#ifndef MI_F32_DEFINED
#define MI_F32_DEFINED
typedef float                       MI_F32;
#endif // MI_F32_DEFINED

#ifndef MI_F64_DEFINED
#define MI_F64_DEFINED
typedef double                      MI_F64;
#endif // MI_F64_DEFINED

#ifndef MI_CHAR_DEFINED
#define MI_CHAR_DEFINED
typedef char                        MI_CHAR;
#endif // MI_CHAR_DEFINED

#ifndef MI_VOID_DEFINED
#define MI_VOID_DEFINED
typedef void                        MI_VOID;
#endif // MI_VOID_DEFINED

/**
* @brief Data structure of two dimensional size, types for point, rect, size....
*/
typedef struct
{
    MI_S32 width;                                  /*!< width in pixels */
    MI_S32 height;                                 /*!< height in pixels */
} MialgoSize2D;

/**
* @brief Inline function of assignment of the two-dimensional size quickly.
*
* @return
*         --<em>MialgoEngine</em> MialgoSize2D type, @see MialgoSize2D.
*/
static inline MialgoSize2D mialgoSize2D(MI_S32 width, MI_S32 height)
{
    MialgoSize2D size;

    size.width = width;
    size.height = height;

    return size;
}

/**
* @brief Data structure of three dimensional size.
*/
typedef struct
{
    MI_S32 width;                                  /*!< width in pixels */
    MI_S32 height;                                 /*!< height in pixels */
    MI_S32 depth;                                  /*!< depth in pixels */
} MialgoSize3D;


/**
* @brief Inline function of assignment for the three-dimensional size quickly, depth setted to 1 by default.
*
* @return
*         --<em>MialgoEngine</em> MialgoSize3D type, @see MialgoSize3D.
*
* mialgoSize can be only used for nonnumeric MialgoImg:
* @code
    MI_S32 img_w = 600;
    MI_S32 img_h = 400;
    MialgoImg *img = MialgoCreateImg(mialgoSize(img_w, img_h), imgFmt(MIALGO_IMG_GRAY), mialgoScalarAll(0));
* @endcode
*/
static inline MialgoSize3D mialgoSize(MI_S32 width, MI_S32 height)
{
    MialgoSize3D size;

    size.width = width;
    size.height = height;
    size.depth = 1;

    return size;
}

/**
* @brief Inline function of assignment for the three-dimensional size quickly.
* The difference between mialgoSize is the need of setting depth value.
* @return
*         --<em>MialgoEngine</em> MialgoSize3D type, @see MialgoSize3D.
*
* mialgoSize3D can be used for numeric MialgoImg:
* @code
    MI_S32 img_w = 600;
    MI_S32 img_h = 400;
    MialgoImg *img = MialgoCreateImg(mialgoSize3D(img_w, img_h, 3), imgFmt(MIALGO_IMG_GRAY), mialgoScalarAll(0));
* @endcode
*/
static inline MialgoSize3D mialgoSize3D(MI_S32 width, MI_S32 height, MI_S32 depth)
{
    MialgoSize3D size;

    size.width = width;
    size.height = height;
    size.depth = depth;

    return size;
}

/**
* @brief Data structure of rectangle size
* The function is to determine a rectangle by defining x, y, width and height.
*/
typedef struct
{
    MI_S32 x;                                      /*!< x coordinates of the top left corner of the rectangle */
    MI_S32 y;                                      /*!< y coordinates of the top left corner of the rectangle */
    MI_S32 width;                                  /*!< width in pixels */
    MI_S32 height;                                 /*!< height in pixels */
} MialgoRect;

/**
* @brief Inline function of definiting a rectangle by setting the values of x, y, width and height.
*
* @return
*         --<em>MialgoEngine</em> MialgoRect type, @see MialgoRect.
*
*/
static inline MialgoRect mialgoRect(MI_S32 x, MI_S32 y, MI_S32 width, MI_S32 height)
{
    MialgoRect rect;

    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;

    return rect;
}

/**
* @brief Data structure of representing a two-dimensional point.
* Noted that x and coordinates are integers, the MI_S32 type.
*/
typedef struct
{
    MI_S32 x;                                      /*!< x coordinates, Usually based on 0 */
    MI_S32 y;                                      /*!< y coordinates, Usually based on 0 */
} MialgoPoint;

/**
* @brief Inline function of definiting a two-dimensional point whose coordinates are integers.
*
* @return
*         --<em>MialgoEngine</em> MialgoPoint type, @see MialgoPoint.
*
*/
static inline MialgoPoint mialgoPoint(MI_S32 x, MI_S32 y)
{
    MialgoPoint point;

    point.x = x;
    point.y = y;

    return point;
}

/**
* @brief Data structure of representing a two-dimensional point.
* Noted that x and coordinates are MI_F32 type.
*/
typedef struct
{
    MI_F32 x;
    MI_F32 y;
} MialgoPoint2D32f;

/**
* @brief Inline function of definiting a two-dimensional point whose coordinates are float, the MI_F32 type.
*
* @return
*         --<em>MialgoEngine</em> MialgoPoint2D32f type, @see MialgoPoint2D32f.
*
*/
static inline MialgoPoint2D32f mialgoPoint2D32f(MI_F32 x, MI_F32 y)
{
    MialgoPoint2D32f point;

    point.x = x;
    point.y = y;

    return point;
}

/**
* @brief Data structure of representing a 3-dimensional point.
* Noted that x and coordinates are integers, the MI_S32 type.
*/
typedef struct
{
    MI_S32 x;                                      /*!< x coordinates, Usually based on 0 */
    MI_S32 y;                                      /*!< y coordinates, Usually based on 0 */
    MI_S32 z;                                      /*!< z coordinates, Usually based on 0 */
} MialgoPoint3D;

/**
* @brief Inline function of definiting a 3-dimensional point whose coordinates are integers.
*
* @return
*         --<em>MialgoEngine</em> MialgoPoint type, @see MialgoPoint3D.
*
*/
static inline MialgoPoint3D mialgoPoint3D(MI_S32 x, MI_S32 y, MI_S32 z)
{
    MialgoPoint3D point;

    point.x = x;
    point.y = y;
    point.z = z;

    return point;
}

/**
* @brief Data structure of representing a 3-dimensional point.
* Noted that x and coordinates are integers, the MI_F32 type.
*/
typedef struct
{
    MI_F32 x;
    MI_F32 y;
    MI_F32 z;
} MialgoPoint3D32f;

/**
* @brief Inline function of definiting a 3-dimensional point whose coordinates are float type.
*
* @return
*         --<em>MialgoEngine</em> MialgoPoint type, @see MialgoPoint3D32f.
*
*/
static inline MialgoPoint3D32f mialgoPoint3D32f(MI_F32 x, MI_F32 y, MI_F32 z)
{
    MialgoPoint3D32f point;

    point.x = x;
    point.y = y;
    point.z = z;

    return point;
}

/**
* @brief Data structure used to hold an array of four float values, which are MI_F32 type.
*/
typedef struct
{
    MI_F32 val[4];
} MialgoScalar;

/**
* @brief Inline function Stores pixels in a single channel image.
*
* The inline function can be used as following:
* @code
     mialgoScalar1(255.0);
* @endcode
*
* @return
*         --<em>MialgoEngine</em> MialgoScalar type, @see MialgoScalar.
*
*/
static inline MialgoScalar mialgoScalar1(MI_F32 val0)
{
    MialgoScalar scalar;

    scalar.val[0] = val0;
    scalar.val[1] = 0;
    scalar.val[2] = 0;
    scalar.val[3] = 0;

    return scalar;
}

/**
* @brief Inline function Stores pixels in a two-channel image.
*
* @return
*         --<em>MialgoEngine</em> MialgoScalar type, @see MialgoScalar.
*
*/
static inline MialgoScalar mialgoScalar2(MI_F32 val0, MI_F32 val1)
{
    MialgoScalar scalar;

    scalar.val[0] = val0;
    scalar.val[1] = val1;
    scalar.val[2] = 0;
    scalar.val[3] = 0;

    return scalar;
}

/**
* @brief Inline function Stores pixels in a three-channel image.
*
* The inline function can be used as following:
* @code
     mialgoScalar3(255.0, 255.0, 255.0);
* @endcode
*
* @return
*         --<em>MialgoEngine</em> MialgoScalar type, @see MialgoScalar.
*
*/
static inline MialgoScalar mialgoScalar3(MI_F32 val0, MI_F32 val1, MI_F32 val2)
{
    MialgoScalar scalar;

    scalar.val[0] = val0;
    scalar.val[1] = val1;
    scalar.val[2] = val2;
    scalar.val[3] = 0;

    return scalar;
}

/**
* @brief Inline function Stores pixels in a four-channel image.
*
* @return
*         --<em>MialgoEngine</em> MialgoScalar type, @see MialgoScalar.
*
*/
static inline MialgoScalar mialgoScalar4(MI_F32 val0, MI_F32 val1, MI_F32 val2, MI_F32 val3)
{
    MialgoScalar scalar;

    scalar.val[0] = val0;
    scalar.val[1] = val1;
    scalar.val[2] = val2;
    scalar.val[3] = val3;

    return scalar;
}

/**
* @brief Inline function Stores pixels in a four-channel image used the same value.
*
* The inline function can be used as following:
* @code
     mialgoScalarAll(255.0);
* @endcode
* 
* @return
*         --<em>MialgoEngine</em> MialgoScalar type, @see MialgoScalar.
*
*/
static inline MialgoScalar mialgoScalarAll(MI_F32 val)
{
    MialgoScalar scalar;

    scalar.val[0] = val;
    scalar.val[1] = val;
    scalar.val[2] = val;
    scalar.val[3] = val;

    return scalar;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_TYPE_H__

