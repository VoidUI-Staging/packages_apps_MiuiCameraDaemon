/**
 * @file        dev_type.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#ifndef __DEV_TYPE_H__
#define __DEV_TYPE_H__

#include "stdint.h"

#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
    #define WINDOWS_OS  1
#elif defined(UNIX) || defined(LINUX) || defined(__linux__)
    #define LINUX_OS    1
    #if defined(ANDROID) || defined(_ANDROID_)
    #define ANDROID_OS  1
    #undef LINUX_OS
    #endif
#endif

//#define CHI_CDK_OS  1
#define MIVIFWK_CDK_OS  1

#ifndef FALSE
#define FALSE    0
#endif

#ifndef TRUE
#define TRUE     1
#endif

#ifndef NULL
#define NULL    ((void *) 0)
#endif

typedef char CHAR;
typedef unsigned char UCHAR;
typedef int INT;
typedef unsigned int UINT;
typedef float FLOAT;

//typedef unsigned char BOOL;
typedef uint8_t U8;
typedef int8_t S8;
typedef uint16_t U16;
typedef int16_t S16;
typedef uint32_t U32;
typedef int32_t S32;
typedef uint64_t U64;
typedef int64_t S64;
typedef float FP32;

#define ALIGNED(x)          __attribute__((aligned(x)))
#define BSS                 __attribute__((section(".bss")))

#define MARK(__tag)         __FILE__,__LINE__,__tag
#define MARK_TAG            const char *__fileName, U32 __fileLine,const char *
#define FILE_NAME_LEN_MAX   256

#ifdef __cplusplus
#define DEV_PUBLIC_ENTRY extern "C" __attribute__ ((visibility ("default")))
#else
#define DEV_PUBLIC_ENTRY __attribute__ ((visibility ("default")))
#endif

/**
 * @brief
 * @details
 * @author
 * @date        2016.05.17
 * @version        0.1
 * @note
 * @warning
 * @par            History:
 *
 */
typedef enum __DevTypeErr {
    RET_OPEN = 1,
    RET_CLOSE = 0,
    RET_OK = 0,
    RET_ERR = -1,
    RET_ERR_ARG = -2,
    RET_BUSY = -3,
    RET_TIMEOUT = -4,
    RET_CONTINUE = -5,
    RET_WOULDBLOCK = -6
} DevTypeErr;

#endif  // __DEV_TYPE_H__
