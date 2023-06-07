/************************************************************************************

Filename  : mialgo_utils.h
Content   :
Created   : Nov. 20, 2019
Author    : guwei (guwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_UTILS_H__
#define MIALGO_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_basic.h"
#include "mialgo_mem.h"
#include "mialgo_cpu.h"

#include "math.h"

#ifdef ANDROID
#include "arm_neon.h"
#endif //ANDROID

#if defined(_MSC_VER) || defined(__CODEGEARC__)
#define SIMD_INLINE __forceinline
#elif defined(__GNUC__)
#define SIMD_INLINE inline __attribute__ ((always_inline))
#else
#error This platform is unsupported!
#endif

#define MIALGO_UNUSED(v)                (void)(v)

/**
* @brief Mialgo macro, Byte alignment, aligned to align bytes, (Mialgo mainly uses 32-byte alignment).
*/
#define MIALGO_ALIGN(x, align)          (((x) + (align) - 1) & -(align))

#if defined(_MSC_VER)

/**
* @brief Mialgo macro, swaps a and b.
*/
//#define MIALGO_SWAP(a, b)               ((a) = (a) + (b), (b) = (a) - (b), (a) = (a) - (b))
#define MIALGO_SWAP(a, b)	SwapGeneric(&(a), &(b), sizeof(a));

// swap - a generic swap-algorithmus
static inline MI_VOID SwapGeneric(MI_VOID *va /* vector object a */,
                                MI_VOID *vb /* vector object b */,
                                size_t i /* length of the objects */)
{
    MI_U8 temp;             // address for buffering one byte
    MI_U8 *a = (MI_U8*)va;  // bytewise moving pointer to va
    MI_U8 *b = (MI_U8*)vb;  // bytewise moving pointer to vb
    // Counting down from the length of the objects to 1
    // Reaching zero stops the while loop, all addresses
    // copied one to one.
    while (i--) temp = a[i], a[i] = b[i], b[i] = temp;
}

/**
* @brief Mialgo macro, finds the minimum & maximum.
*/
#define MIALGO_MIN(x, y)                ((x) < (y) ? (x) : (y))
#define MIALGO_MAX(x, y)                ((x) > (y) ? (x) : (y))

/**
* @brief Mialgo macro, gets absolute value.
*/
#define MIALGO_ABS(x)                   ((x) < 0 ? -(x) : (x))

/**
* @brief General-purpose saturation macros.
*/
#define MIALGO_CAST_U8(t)               (MI_U8)(!((t) & ~255) ? (t) : (t) > 0 ? 255 : 0)
#define MIALGO_CAST_S8(t)               (MI_S8)(!(((t)+128) & ~255) ? (t) : (t) > 0 ? 127 : -128)
#define MIALGO_CAST_U16(t)              (MI_U16)(!((t) & ~65535) ? (t) : (t) > 0 ? 65535 : 0)
#define MIALGO_CAST_S16(t)              (MI_S16)(!(((t)+32768) & ~65535) ? (t) : (t) > 0 ? 32767 : -32768)
#define MIALGO_CAST_S32(t)              (MI_S32)(t)
#define MIALGO_CAST_U32(t)              (MI_U32)(t)
#define MIALGO_CAST_F32(t)              (MI_F32)(t)
#define MIALGO_CAST_F64(t)              (MI_F64)(t)

/**
* @brief Cast t to between from and to.
* if (from <= t < to), keep t the same
* if (t < from), set t to from
* if (t >= to), set t to (to - 1)
*/
#define MIALGO_CAST_OP(t, from, to)     ((t) >= (from) ? ((t) < (to) ? (t) : (to) - 1) : (from))

/**
* @brief Mialgo macros, convert between types of datas.
* The macro basic format is :MIALGO_CAST_srctype_dsttype(t)
*
* How to use these macros, the key is distinguishing srctype and dsttype.
* @code
    MIALGO_CAST_F32_U8(-1.5)
* @endcode
*
*/
#define MIALGO_CAST_F32_U8(t)           (MI_U8)((t) >= 255.f ? 255 : (t) <= 0.f ? 0 : MialgoRound((t)))
#define MIALGO_CAST_F32_S8(t)           (MI_S8)((t) >= 127.f ? 127 : (t) <= -128.f ? -128 : MialgoRound((t)))
#define MIALGO_CAST_F32_U16(t)          (MI_U16)((t) >= 65535.f ? 65535 : (t) <= 0.f ? 0 : MialgoRound((t)))
#define MIALGO_CAST_F32_S16(t)          (MI_S16)((t) >= 32767.f ? 32767 : (t) <= -32768.f ? (-32768) : MialgoRound((t)))
#define MIALGO_CAST_F32_F32(t)          (t)
#define MIALGO_CAST_F32_S32(t)          (MI_S32)MialgoRound((t))
#define MIALGO_CAST_F32_U32(t)          (MI_U32)((t) <= 0.f ? 0 : MialgoRound((t)))
#define MIALGO_F32_NEARLY_EQUAL(a, b)   (((a) + 1e-5) > (b)) && (((a) - 1e-5) < (b))
#else /* _MSC_VER */
#define MIALGO_SWAP(a, b)               { typeof(a) __v = (a); (a) = (b); (b) = __v; }
#define MIALGO_MIN(x, y)                                        \
({                                                              \
    typeof(x) __min1 = (x); typeof(y) __min2 = (y);             \
    __min1 < __min2 ? __min1 : __min2;                          \
})
#define MIALGO_MAX(x, y)                                        \
({                                                              \
    typeof(x) __max1 = (x); typeof(y) __max2 = (y);             \
    __max1 > __max2 ? __max1 : __max2;                          \
})
#define MIALGO_ABS(x)                   ({typeof(x) __v = (x); (__v > 0 ? __v : -__v);})
/* general-purpose saturation macros */
#define MIALGO_CAST_U8(t)               ({typeof(t) __v = (t); (MI_U8)(!(__v & ~255) ? __v : __v > 0 ? 255 : 0);})
#define MIALGO_CAST_S8(t)               ({typeof(t) __v = (t); (MI_S8)(!((__v+128) & ~255) ? __v : __v > 0 ? 127 : -128);})
#define MIALGO_CAST_U16(t)              ({typeof(t) __v = (t); (MI_U16)(!(__v & ~65535) ? __v : __v > 0 ? 65535 : 0);})
#define MIALGO_CAST_S16(t)              ({typeof(t) __v = (t); (MI_S16)(!((__v+32768) & ~65535) ? __v : __v > 0 ? 32767 : -32768);})
#define MIALGO_CAST_S32(t)              ({typeof(t) __v = (t); (MI_S32)__v;})
#define MIALGO_CAST_U32(t)              ({typeof(t) __v = (t); (MI_U32)__v;})
#define MIALGO_CAST_F32(t)              ({typeof(t) __v = (t); (MI_F32)__v;})
#define MIALGO_CAST_F64(t)              ({typeof(t) __v = (t); (MI_F64)__v;})
#define MIALGO_CAST_OP(t, from, to)     ({typeof(t) __v = (t); (__v >= (from) ? (__v < (to) ? __v : (to) - 1) : (from));})
#define MIALGO_CAST_F32_U8(t)           ({typeof(t) __v = (t); (MI_U8)(__v >= 255.f ? 255 : __v <= 0.f ? 0 : MialgoRound(__v));})
#define MIALGO_CAST_F32_S8(t)           ({typeof(t) __v = (t); (MI_S8)(__v >= 127.f ? 127 : __v <= -128.f ? -128 : MialgoRound(__v));})
#define MIALGO_CAST_F32_U16(t)          ({typeof(t) __v = (t); (MI_U16)(__v >= 65535.f ? 65535 : __v <= 0.f ? 0 : MialgoRound(__v));})
#define MIALGO_CAST_F32_S16(t)          ({typeof(t) __v = (t); (MI_S16)(__v >= 32767.f ? 32767 : __v <= -32768.f ? (-32768) : MialgoRound(__v));})
#define MIALGO_CAST_F32_F32(t)          ({typeof(t) __v = (t); __v;})
#define MIALGO_CAST_F32_S32(t)          ({typeof(t) __v = (t); (MI_S32)MialgoRound(__v);})
#define MIALGO_CAST_F32_U32(t)          ({typeof(t) __v = (t); (MI_U32)(__v <= 0.f ? 0 : MialgoRound(__v));})
#define MIALGO_F32_NEARLY_EQUAL(a, b)   ({typeof(a) _a = a; typeof(b) _b = b; ((_a + 1e-5) > _b) && ((_a - 1e-5) < _b);})
#endif  /* _MSC_VER */

/**
* @brief Mialgo macro, Puts the a & b in the order of minimum and maximum.
*/
#define MIALGO_MINMAX(a, b)             { if ((a) > (b)) MIALGO_SWAP((a), (b)); }

/**
* @brief Mialgo macro, Puts the a & b in the order of maximum and minimum.
*/
#define MIALGO_MAXMIN(a, b)             { if ((a) < (b)) MIALGO_SWAP((a), (b)); }

/**
* @brief Mialgo macro, Initializes the time.
*/
#define MIALGO_TIME_INIT(info)                                  \
    MialgoTime info##_start, info##_end                         \

/**
* @brief Mialgo macro, Initializes the start time.
*/
#define MIALGO_TIME_START(info)                                 \
    do {                                                        \
        MialgoGetTime(&(info##_start));                         \
    } while (0)

/**
* @brief Mialgo macro, Log use time.
*/
#define MIALGO_TIME_END(tag, info)                              \
    do {                                                        \
        MialgoGetTime(&(info##_end));                           \
        MIALGO_LOGI(tag, "%s use %.2fms\n", #info,              \
                        (info##_end.time - info##_start.time)); \
    } while (0)

/**
* @brief Mialgo macro, Allocate memory error.
*/
#define MIALGO_ALLOCATE_ERROR                                   \
    mialgo_allocate_error

/**
* @brief Mialgo macro, Check allocate memory error.
*/
#define MIALGO_ALLOCATE_CHECK(p, size, type)                    \
    do {                                                        \
        if ((p = (type *)MialgoAllocate((size))) == NULL)       \
        {                                                       \
            MialgoLogError(MIALGO_NO_MEM, __LINE__,             \
                                __FUNCTION__, "no mem",         \
                                MIALGO_TRUE);                   \
            goto MIALGO_ALLOCATE_ERROR;                         \
        }                                                       \
    } while (0)

/**
* @brief Find the log base 2 of an N-bit integer.
* @param[in] v      @b v must be a power of 2
*
* @return
*        -<em>MI_U8</em> log2(v)
*/
static inline MI_U8 Log2Spec2_U32(const MI_U32 v)
{
    MI_U8 r = (v & 0xAAAAAAAA) != 0;
    r |= ((v & 0xFFFF0000) != 0) << 4;
    r |= ((v & 0xFF00FF00) != 0) << 3;
    r |= ((v & 0xF0F0F0F0) != 0) << 2;
    r |= ((v & 0xCCCCCCCC) != 0) << 1;
    return r;
}

/**
* @brief Find the log base 2 of an N-bit integer.
* @param[in] v      @b v must be a power of 2
*
* @return
*        -<em>MI_U8</em> log2(v)
*/
static inline MI_U8 Log2Spec2_U16(const MI_U16 v)
{
    MI_U8 r = (v & 0xAAAA) != 0;
    r |= ((v & 0xFF00) ? 8 : 0);
    r |= ((v & 0xF0F0) ? 4 : 0);
    r |= ((v & 0xCCCC) ? 2 : 0);
    return r;
}

/**
* @brief Find the log base 2 of an N-bit integer.
* @param[in] v      @b v must be a power of 2
*
* @return
*        -<em>MI_U8</em> log2(v)
*/
static inline MI_U8 Log2Spec2_U8(const MI_U8 v)
{
    MI_U8 r = (v & 0xAA) != 0;
    r |= ((v & 0xF0) ? 4 : 0);
    r |= ((v & 0xCC) ? 2 : 0);
    return r;
}

/**
* @brief Inline function for Rounding, (Input value type is MI_F32).
* @param[in] value       MI_F32 type, value to round
*
* @return
*        -<em>MI_S32</em> the value after rounding
*/
static inline MI_S32 MialgoRound(MI_F32 value)
{
#ifdef ANDROID
    #ifdef __aarch64__
        return vcvtns_s32_f32(value);
    #else
        MI_S32 res;
        MI_F32 temp;
        (void)temp;
        __asm__("vcvtr.s32.f32 %[temp], %[value]\n vmov %[res], %[temp]"
            : [res] "=r" (res), [temp] "=w" (temp) : [value] "w" (value));
        return res;
    #endif  //__aarch64__
#else
    MI_F64 intpart, fractpart;
    fractpart = modf(value, &intpart);
    if ((fabs(fractpart) != 0.5) || ((((MI_S32)intpart) & 1) != 0))
        return (MI_S32)(value + (value >= 0 ? 0.5 : -0.5));
    else
        return (MI_S32)intpart;
#endif //ANDROID
}

/**
* @brief Inline function for fractions are rounded down, (Input value type is MI_F32).
*
* @param[in]  value       MI_F32 type, value to round
* Unlike MialgoRound, MialgoFloor is directly to the left of the value closest to the desired value on the number line.
* That is, the integer value must be not greater than the maximum value required.
* @return
*        -<em>MI_S32</em> the value after rounding down.
*/
static inline MI_S32 MialgoFloor(MI_F32 value)
{
    MI_S32 temp = (MI_S32)value;
    MI_F32 diff = (MI_F32)(value - temp);
    return temp - (diff < 0);
}

/**
* @brief Inline function for fractions are rounded up, (Input value type is MI_F32).
* Returns the smallest integer greater than or equal to the specified value
*
* @param[in]  value       MI_F32 type, value to round.
*
* @return
*        -<em>MI_S32</em> the value after rounding.
*/
static inline MI_S32 MialgoCeil(MI_F32 value)
{
    MI_S32 temp = (MI_S32)value;
    MI_F32 diff = (MI_F32)(value - temp);
    return temp + (diff > 0);
}

/**
* @brief Inline function for Rounding, (Input value type is MI_F64).
* @param[in]  value       MI_F64 type, value to round
*
* @return
*        -<em>MI_S32</em> the value after rounding
*/
static inline MI_S32 MialgoRoundF64(MI_F64 value)
{
    MI_F64 intpart, fractpart;
    fractpart = modf(value, &intpart);
    if ((fabs(fractpart) != 0.5) || ((((MI_S32)intpart) & 1) != 0))
        return (MI_S32)(value + (value >= 0 ? 0.5 : -0.5));
    else
        return (MI_S32)intpart;
}

/**
* @brief Inline function for fractions are rounded down, (Input value type is MI_F64).
*
* @param[in]  value       MI_F64 type, value to round
* Unlike MialgoRound, MialgoFloor is directly to the left of the value closest to the desired value on the number line.
* That is, the integer value must be not greater than the maximum value required.
* @return
*        -<em>MI_S32</em> the value after rounding down.
*/
static inline MI_S32 MialgoFloorF64(MI_F64 value)
{
    MI_S32 temp = (MI_S32)value;
    MI_F64 diff = (MI_F64)(value - temp);
    return temp - (diff < 0);
}

/**
* @brief Inline function for fractions are rounded up, (Input value type is MI_F64).
* Returns the smallest integer greater than or equal to the specified value
*
* @param[in]  value       MI_F64 type, value to round.
*
* @return
*        -<em>MI_S32</em> the value after rounding.
*/
static inline MI_S32 MialgoCeilF64(MI_F64 value)
{
    MI_S32 temp = (MI_S32)value;
    MI_F64 diff = (MI_F64)(value - temp);
    return temp + (diff > 0);
}

#ifdef ANDROID

static SIMD_INLINE MI_VOID vminmaxq_u8(uint8x16_t& a, uint8x16_t& b)
{
    uint8x16_t t = a;
    a = vminq_u8(t, b);
    b = vmaxq_u8(t, b);
}

static SIMD_INLINE MI_VOID vminmaxq_f32(float32x4_t& a, float32x4_t& b)
{
    float32x4_t t = a;
    a = vminq_f32(t, b);
    b = vmaxq_f32(t, b);
}

static SIMD_INLINE MI_VOID vminmax_u8(uint8x8_t& a, uint8x8_t& b)
{
    uint8x8_t t = a;
    a = vmin_u8(t, b);
    b = vmax_u8(t, b);
}

static SIMD_INLINE MI_VOID vminmax_f32(float32x2_t& a, float32x2_t& b)
{
    float32x2_t t = a;
    a = vmin_f32(t, b);
    b = vmax_f32(t, b);
}

static inline int32x4_t MialgoRound_F32x4(float32x4_t value)
{
#ifdef __aarch64__
    return vcvtnq_s32_f32(value);
#else
    typedef union {
        float32x4_t v;
        MI_F32 f[4];
    } F;
    typedef union {
        int32x4_t v;
        MI_S32 i[4];
    } I;

    F u;
    I i;

    u.v = value;
    i.i[0] = MialgoRound(u.f[0]);
    i.i[1] = MialgoRound(u.f[1]);
    i.i[2] = MialgoRound(u.f[2]);
    i.i[3] = MialgoRound(u.f[3]);

    return i.v;
#endif
}

static inline int32x2_t MialgoRound_F32x2(float32x2_t value)
{
#ifdef __aarch64__
    return vcvtn_s32_f32(value);
#else
    typedef union {
        float32x2_t v;
        MI_F32 f[2];
    } F;
    typedef union {
        int32x2_t v;
        MI_S32 i[2];
    } I;

    F u;
    I i;

    u.v = value;
    i.i[0] = MialgoRound(u.f[0]);
    i.i[1] = MialgoRound(u.f[1]);

    return i.v;
#endif
}

#endif //ANDROID

/**
* @brief Data structure of statistic time.
*/
typedef struct MialgoTime
{
    MI_F64 time;                  /*!< MI_F64 type, Current exact time (the time from January 1, 1970 to present), in ms */
} MialgoTime;

/**
* @brief Mialgo Gets run time.
* This function gets the execution time of a specific function by the difference between the results of two MialgoGetTime operations.
* @param[in] time           MialgoTime type, A pointer to store the moment the function run.
*
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoGetTime(MialgoTime *time);

/**
* @brief Mialgo Retrieves textual description of the error given its code function.
* @param[in] error_no       MI_S32 type, Error code.
* @return
        --<em>MialgoEngine</em> const MI_CHAR* type value, the string that represents the error message @see mialgo_errorno.h.
*/
const MI_CHAR* MialgoErrorStr(MI_S32 error_no);

/**
* @brief Mialgo Record error information in the program function.
* The function can record the error messages in the program to form a backtrace.
* @param[in] error_no       MI_S32 type, Error code.
* @param[in] line           MI_S32 type, Line number where the error occurred.
* @param[in] func           const MI_CHAR type, Name of the function where the error occurred.
* @param[in] info           const MI_CHAR type, Error information.
* @param[in] root           MI_S32 type, Whether it is a root case, setting root flag will clear the backtrace.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h

* When an error occurs, instead of printing the error directly, the error is recorded, and the external call can print out the call stack through the function.
* Only the debug version logs errors.
*/
MI_S32 MialgoLogError(MI_S32 error_no, MI_S32 line, const MI_CHAR *func, const MI_CHAR *info, MI_S32 root);

/**
* @brief Mialgo Prints error backtrace.(return 0 means no error,)
* @param[in] error          MI_S32 type, Error code.
* @param[in] buf            MI_CHAR type, Saves the the error backtrace, or passing in NULL means saving nothing.
* @param[in] len            MI_S32 type, The total number of characters allowed to be saved.
* @return
*        --<em>MialgoEngine</em> MI_S32 type value, returns the total number of characters saved, excluding the null character appended to the end of the string, returns 0 meaning failed.
*
* This feature is enabled only in the debug version library
*/
MI_S32 MialgoErrorBacktrace(MI_S32 error, MI_CHAR *buf, MI_S32 len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_UTILS_H__

