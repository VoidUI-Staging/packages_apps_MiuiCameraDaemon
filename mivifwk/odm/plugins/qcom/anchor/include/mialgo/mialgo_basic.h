/************************************************************************************

Filename  : mialgo_basic.h
Content   :
Created   : Nov. 21, 2019
Author    : guwei (guwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_BASIC_H__
#define MIALGO_BASIC_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mialgo_type.h"
#include "mialgo_errorno.h"

#define MIALGO_TRUE                 (1)
#define MIALGO_FALSE                (0)

/**
* @brief Data structure of log level, (mainly use the middle three levels).
* MIALGO_LOG_LV_ERROR mean that only the output of MIALGO_LOGE is shown
* MIALGO_LOG_LV_INFO mean that the output of MIALGO_LOGE and MIALGO_LOGI is shown
* MIALGO_LOG_LV_DEBUG mean that all output is shown
*/
typedef enum
{
    MIALGO_LOG_LV_INVALID = 0,
    MIALGO_LOG_LV_ERROR,                /*!< log error */
    MIALGO_LOG_LV_INFO,                 /*!< log information */
    MIALGO_LOG_LV_DEBUG,                /*!< log debug */
    MIALGO_LOG_LV_NUM,
} MialgoLogLv;

/**
* @brief Mialgo memory allocator, (using the internal default value when the memory allocator is not specified)
*/
typedef MI_VOID* (*MialgoAllocatorFunc)(size_t size);

/**
* @brief Mialgo memory deallocator.
*/
typedef MI_VOID (*MialgoDeAllocatorFunc)(MI_VOID *ptr);

/**
* @brief Mialgo handle to the algorithm example.
* All algorithm initialization functions (except the basic library) return a MialgoEngine
*/
typedef MI_VOID* MialgoEngine;

/**
* @brief algorithm implementation type.
*/
typedef enum
{
    MIALGO_IMPL_DEFAULT = 0,        /*!< implementation type is selected by mialgo engine */
    MIALGO_IMPL_NONE,               /*!< C implementation */
    MIALGO_IMPL_NEON,               /*!< neon implementation */
    MIALGO_IMPL_OPENCL,             /*!< opencl implementation */
    MIALGO_IMPL_CDSP,               /*!< cdsp implementation */
    MIALGO_IMPL_NUM,                /*!< implementation type num */
} MialgoImplType;

/**
* @brief opencl local work size.
*/
typedef struct
{
    MI_S32 param_len;               /*!< param len */
    MI_S32 dim;                     /*!< number of dimensions */
    MI_S32 size[3];                 /*!< dimensions of each dimension */
} MialgoImplClLws;

/**
* @brief Inline function of init MialgoImplClLws struct.
*
* @param[in] x              x size.
* @param[in] y              y size.
*
* @return
*         --<em>MialgoImpl</em>, @see MialgoImpl.
*/
static inline MialgoImplClLws mialgoImplClLws2D(MI_S32 x, MI_S32 y)
{
    MialgoImplClLws lws;

    lws.param_len = sizeof(MialgoImplClLws);
    lws.dim = 2;
    lws.size[0] = x;
    lws.size[1] = y;
    lws.size[2] = 0;

    return lws;
}

/**
* @brief Inline function of init MialgoImplClLws struct.
*
* @param[in] x              x size.
* @param[in] y              y size.
* @param[in] z              z size.
*
* @return
*         --<em>MialgoImpl</em>, @see MialgoImpl.
*/
static inline MialgoImplClLws mialgoImplClLws3D(MI_S32 x, MI_S32 y, MI_S32 z)
{
    MialgoImplClLws lws;

    lws.param_len = sizeof(MialgoImplClLws);
    lws.dim = 3;
    lws.size[0] = x;
    lws.size[1] = y;
    lws.size[2] = z;

    return lws;
}

/**
* @brief use to select the implementation type of the algorithm(c, neon, opencl, cdsp and so on).
*
* Description of type
*   @see MialgoImplType
*
* Description of param
*   implementation param, each algorithm can design its own parameters according to specific requirements.
*/
typedef struct
{
    MialgoImplType type;
    MI_VOID *param;
} MialgoImpl;

/**
* @brief Inline function of init MialgoImpl struct.
*
* @param[in] type           impl type.
* @param[in] param          impl param.
*
* @return
*         --<em>MialgoImpl</em>, @see MialgoImpl.
*/
static inline MialgoImpl mialgoImpl(MialgoImplType type, MI_VOID *param)
{
    MialgoImpl impl;

    impl.type = type;
    impl.param = param;

    return impl;
}

/**
* @brief work pool init param.
*/
typedef struct
{
    MI_S32 enable;                              /*!< enable flag */
    MI_S32 worker_num;                          /*!< worker num */
    MI_S32 affinity;                            /*!< cpu affinity */
} MialgoWorkPoolParam;

/**
* @brief Inline function of init MialgoWorkPoolParam struct.
*
* @param[in] enable             enable flag.
* @param[in] worker_num         worker num.
*
* @return
*         --<em>MialgoWorkPoolParam</em>, @see MialgoWorkPoolParam.
*/
static inline MialgoWorkPoolParam mialgoWorkPoolParam(MI_S32 enable, MI_S32 worker_num, MI_S32 affinity)
{
    MialgoWorkPoolParam param;

    param.enable = enable;
    param.worker_num = worker_num;
    param.affinity = affinity;

    return param;
}

/**
* @brief gpu performance level.
*/
typedef enum
{
    MIALGO_GPU_PERF_INVALID = 0,            /*!< invalid gpu performance level */
    MIALGO_GPU_PERF_LOW,                    /*!< low gpu performance level */
    MIALGO_GPU_PERF_NORMAL,                 /*!< normal gpu performance level */
    MIALGO_GPU_PERF_HIGH,                   /*!< high gpu performance level */
    MIALGO_GPU_PERF_NUM,
} MialgoGpuPerf;

/**
* @brief opencl init param.
*/
typedef struct
{
    MI_S32 enable;                          /*!< enable flag */
    MI_S32 profiling;                       /*!< profiling */
    MialgoGpuPerf gpu_perf;                 /*!< performance level */
    MI_CHAR cache_bin_prefix[64];           /*!< cache bin file prefix */
    MI_CHAR precompiled_bin_path[64];       /*!< full path of the precompiled bin file */
    MI_CHAR bin_version[64];                /*!< bin version string */
} MialgoOpenclParam;

/**
* @brief Inline function of init MialgoOpenclParam struct.
*
* @param[in] enable                     enable.
* @param[in] profiling                  profiling.
* @param[in] cache_bin_prefix           cache bin file prefix.
* @param[in] precompiled_bin_path       full path of the precompiled bin file.
*
* @return
*         --<em>MialgoImpl</em>, @see MialgoImpl.
*/
static inline MialgoOpenclParam mialgoOpenclParam(MI_S32 enable, MI_S32 profiling, MialgoGpuPerf gpu_perf,
                                                            const MI_CHAR *cache_bin_prefix, const MI_CHAR *precompiled_bin_path)
{
    MialgoOpenclParam param;
    memset(&param, 0, sizeof(param));

    param.enable = enable;
    param.profiling = profiling;
    param.gpu_perf = gpu_perf;

    if (cache_bin_prefix != NULL)
    {
        strncpy(param.cache_bin_prefix, cache_bin_prefix, sizeof(param.cache_bin_prefix));
    }

    if (precompiled_bin_path != NULL)
    {
        strncpy(param.precompiled_bin_path, precompiled_bin_path, sizeof(param.precompiled_bin_path));
    }

    return param;
}

/**
* @brief cdsp init param.
*
* Description of enable
*   set to FALSE mean the CDSP will not work.
*
* Description of signing
*   the default setting is TRUE, indicating that a signature is required.
*   if set to false, Unsigned PD will be enabled, CDSP has some limitations, so it is only necessary to set to false in debug mode.
*
*/
typedef struct
{
    MI_S32 enable;                          /*!< enable flag */
    MI_S32 signing;                         /*!< signing flag */
} MialgoCdspParam;

/**
* @brief Inline function of init MialgoCdspParam struct.
*
* @param[in] enable             enable flag.
*
* @return
*         --<em>MialgoCdspParam</em>, @see MialgoCdspParam.
*/
static inline MialgoCdspParam mialgoCdspParam(MI_S32 enable)
{
    MialgoCdspParam param;
    memset(&param, 0, sizeof(param));

    param.enable = enable;

    return param;
}

/**
* @brief Inline function of init MialgoCdspParam struct.
*
* @param[in] enable             enable flag.
* @param[in] signing            signing flag.
*
* @return
*         --<em>MialgoCdspParam</em>, @see MialgoCdspParam.
*/
static inline MialgoCdspParam mialgoCdspSigningParam(MI_S32 enable, MI_S32 signing)
{
    MialgoCdspParam param;
    memset(&param, 0, sizeof(param));

    param.enable = enable;
    param.signing = signing;

    return param;
}

/**
* @brief Mialgo structure of basic initialize.
*/
typedef struct MialgoBasicInit
{
    MialgoLogLv log;                            /*!< Log information printing level, @see MialgoLogLv*/
    MialgoAllocatorFunc allocator;              /*!< Memory allocator, passing in NULL when not specified*/
    MialgoDeAllocatorFunc deallocator;          /*!< Memory deallocator, passing in NULL when not specified*/
    MI_S32 addr_align;                          /*!< Address alignment, passing in NULL when not specified, 32 byte alignment by default*/
    MI_S32 num_threads;                         /*!< Number of parallelized threads*/
    const MI_CHAR *cache_path;                  /*!< cache path*/
    MialgoWorkPoolParam work_pool_param;        /*!< work pool init param*/
    MialgoOpenclParam opencl_param;             /*!< opencl init param*/
    MialgoCdspParam cdsp_param;                 /*!< cdsp init param*/
} MialgoBasicInit;

/**
* @brief Mialgo Initializes the base operator library function. 
* @param[in] p_init           MialgoBasicInit type, Basic operator library initialization configuration.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*
* MialgoBasicLibInit function can be used as following:
* @code
*    MI_S32 ret = MIALGO_OK;
*    MialgoBasicInit basic_init;
*
*    memset(&basic_init, 0, sizeof(basic_init));
*    basic_init.log = MIALGO_LOG_LV_ERROR;
*    basic_init.num_threads = 4;
*
*    if(ret =  MialgoBasicLibInit(&basic_init) != MIALGO_OK)
*    {
*        MIALGO_LOGE("fail with error(%d %s)\n", ret, MialgoErrorStr(ret));
*        return ret;
*    }
* @endcode
*
* the internal implementation has a reference count that allocates resources on the first call and can be initialized multiple times.
*/
MI_S32 MialgoBasicLibInit(MialgoBasicInit *p_init);

/**
* @brief Mialgo Destroys the base operator library function. 
* @param[in]                No.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h

* the internal implementation has a reference count, which is decremented by one each time.
* and the resource is released when the count is decremented to zero.
*/
MI_S32 MialgoBasicLibUnit();

/**
* @brief Mialgo Gets version information of base operator library function. 
* @param[in]                No.
* @return
*         --<em>MialgoEngine</em> const MI_CHAR* type, the version information of base operator library, or NULL identitying failed.

* version number rule.
* for weekly build is "mialgo_basic_$(platform)_${abi}_${build_type}_${project}_${version}.${git_hash}_${time}"
* for example: mialgo_basic_android_arm64_debug_none_1.0.0.6b0d412_20190819170331
*/
const MI_CHAR* MialgoBasicLibGetVer();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_BASIC_H__

