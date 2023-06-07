/************************************************************************************

Filename  : mialgo_errorno.h
Content   :
Created   : Nov. 21, 2019
Author    : guwei (guwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_ERRORNO_H__
#define MIALGO_ERRORNO_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define MIALGO_OK                   (0)         // everithing is ok
#define MIALGO_ERROR                (-1)        // unknown/unspecified error
#define MIALGO_IO_ERROR             (-2)        // io error
#define MIALGO_NO_MEM               (-3)        // insufficient memory
#define MIALGO_NULL_PTR             (-4)        // null pointer
#define MIALGO_BAD_ARG              (-5)        // arg/param is bad
#define MIALGO_BAD_OPT              (-6)        // bad operation

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_ERRORNO_H__

