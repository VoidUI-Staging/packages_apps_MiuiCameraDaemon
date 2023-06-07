/**
 * @file		dev_type.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par			History:
 *
 */

#ifndef __DEV_TYPE_H__
#define __DEV_TYPE_H__

#include "stdint.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef char CHAR;
typedef unsigned char UCHAR;
typedef int INT;
typedef unsigned int UINT;
typedef float FLOAT;

// typedef unsigned char BOOL;
typedef uint8_t U8;
typedef int8_t S8;
typedef uint16_t U16;
typedef int16_t S16;
typedef uint32_t U32;
typedef int32_t S32;
typedef uint64_t U64;
typedef int64_t S64;
typedef float FP32;
#define ALIGNED(x) __attribute__((aligned(x)))

/**
 * @brief
 * @details
 * @author
 * @date		2016.05.17
 * @version		0.1
 * @note
 * @warning
 * @par			History:
 *
 */
typedef enum __DevTypeErr {
    RET_OK = 0,
    RET_ERR = (-2147483647 - 1), // int32_t_MIN value
    RET_ERR_ARG,
    RET_BUSY,
    RET_TIMEOUT,
    RET_CONTINUE,
} DevTypeErr;

#endif // __DEV_TYPE_H__
