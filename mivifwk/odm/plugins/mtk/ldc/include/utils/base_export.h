/*
 * Copyright (c) 2018 AnC Technology Co., Ltd. All rights reserved.
 */
#ifndef LIBCUTILS_INCLUDE_UTILS_BASE_EXPORT_H_
#define LIBCUTILS_INCLUDE_UTILS_BASE_EXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)

#if defined(BASE_IMPLEMENTATION)
#define BASE_EXPORT __declspec(dllexport)
#else
#define BASE_EXPORT __declspec(dllimport)
#endif // defined(BASE_IMPLEMENTATION)

#else // defined(WIN32)
#define BASE_EXPORT __attribute__((visibility("default")))
#endif // defined(WIN32)

#if defined(__cplusplus)
}
#endif

#endif // LIBCUTILS_INCLUDE_UTILS_BASE_EXPORT_H_
