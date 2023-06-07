/*
 * Copyright (c) 2018 AnC Technology Co., Ltd. All rights reserved.
 */
#ifndef LIBCUTILS_INCLUDE_UTILS_BASE_DEFS_H_
#define LIBCUTILS_INCLUDE_UTILS_BASE_DEFS_H_

#include <stddef.h> // for NULL, size_t

#if defined(COMPILER_MSVC) && (COMPILER_MSVC_VER < 1600)
#include <sys/types.h> // for uintptr_t on x86
#else
// stdint.h is part of C99 but MSVC doesn't have it.
#include <stdint.h> // for uintptr_t
#endif

/**
 * @brief basic types
 */
#ifndef WA_BASIC_TYPES
// #define __wsigned signed
#define __wsigned

typedef __wsigned char WInt8;
typedef signed short WInt16; // NOLINT
typedef int WInt32;
#if defined(__LP64__) && !defined(__APPLE__) && !defined(__OpenBSD__)
typedef long WInt64; // NOLINT
#else
typedef long long WInt64;           // NOLINT
#endif
typedef unsigned char WUInt8;
typedef unsigned short WUInt16; // NOLINT
typedef unsigned int WUInt32;
#if defined(__LP64__) && !defined(__APPLE__) && !defined(__OpenBSD__)
typedef unsigned long WUInt64; // NOLINT
#else
typedef unsigned long long WUInt64; // NOLINT
#endif

typedef float WFloat32;
typedef double WFloat64;
typedef unsigned char WByte;
typedef enum {
    WFalse = 0, ///<  false
    WTrue = 1,  ///<  true
} WBool;
// typedef int                  WBool;

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif // __cplusplus
#endif // NULL

#define WA_BASIC_TYPES
#endif // WA_BASIC_TYPES

#ifdef __cplusplus
extern "C" {
#endif

typedef WInt32 WRetStatus;

// define common status code
#define WOK          0 // Successful
#define WFAILED      1 // General error
#define WUNSUPPORTED 2 // Unsupport function

#define WERR_INVALID_BASE        100
#define WERR_INVALID_PARAMS      WERR_INVALID_BASE       // Invalid parameters
#define WERR_INVALID_HANDLE      (WERR_INVALID_BASE + 1) // Invalid handle
#define WERR_INVALID_POINTER     (WERR_INVALID_BASE + 2) // Invalid poniter
#define WERR_INVALID_CALI_DATA   (WERR_INVALID_BASE + 3) // Invalid cali data
#define WERR_INVALID_MODULE_DATA (WERR_INVALID_BASE + 4) // Invalid module data
#define WERR_INVALID_MECP_DATA   (WERR_INVALID_BASE + 5) // Invalid mecp data

#define WERR_FILE_BASE              200
#define WERR_FILE_PERMISSION_DENIED WERR_FILE_BASE // Incorrect permission for file

#define WERR_MEM_BASE   300
#define WERR_MEM_MALLOC WERR_MEM_BASE // Memory allocation error

#define WERR_IMAGE_BASE  400
#define WERR_IMAGE_EMPTY WERR_IMAGE_BASE // Image empty error

#define WSDK_BASE 1000 // SDK base status code

#if defined(__cplusplus)
}
#endif

#endif // LIBCUTILS_INCLUDE_UTILS_BASE_DEFS_H_
