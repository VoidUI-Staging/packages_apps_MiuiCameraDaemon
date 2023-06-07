//********************************************************************************************
//  Filename    : pureView_api.h
//  Date        : Mar 25, 2020
//  Author      : Guoqing Zheng(zhengguoqing@xiaomi.com)
//  Desciption  :
//  others      :
//********************************************************************************************
//      Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.
//********************************************************************************************

#ifndef __PUREVIEW_API__H__
#define __PUREVIEW_API__H__

#include "pureView_basic.h"

// this version should be changed for each time
typedef struct PV_LIB_VERSION_ST
{
    char versionName[32];
    char majorVersion[4];
    char minorVersion[8];
    char gitCodeVersion[8];
    char buildDataTime[128];
} PV_LIB_VERSION;

/**
 * @brief This function does initialization for miAi fucntion. this function should be called
 * when entering front camera in camera app
 * @param[in]  NONE
 *
 * @return  PV_INT32
 *
 */
PV_INT32 pureViewInitMiAiLib();

/**
 * @brief The PUREVIEW algorithm initialization function.
 * @param[in] pConfig      @see PV_CONFIG
 *
 * @return
 *        PV_INT
 */
// PV_INT32 pureViewInit(PV_CONFIG *pConfig);

/**
 * @brief The PUREVIEW algorithm preInitialization function.
 * @param[in] pConfig      @see PV_CONFIG
 *
 * @return
 *        PV_INT
 */
PV_INT32 pureViewPreInit(PV_CONFIG *pureViewAlgo);

/**
 * @brief The PUREVIEW algorithm preDeInitialization function.
 * @param[in] pConfig      @see PV_CONFIG
 *
 * @return
 *        PV_INT
 */
PV_INT32 pureViewPreDeInit(PV_CONFIG *pureViewAlgo);

/**
 * @brief The PUREVIEW algorithm version number acquisition function.
 * @param[in] algo       Algorithm running context pointer
 *
 * @return
 *        -<em>char*</em> Version number information string. only work on Android platform
 */
PV_INT32 pureViewGetVersion(char *pPsLibVersion);

/**
 * @brief The pureView algorithm main processing function.

 * @param[in] pCurrentFrame      AMBT_IMG pointer, stored single frame image data. (NV12)
 * @param[in] pConfig            @see PV_CONFIG
 * @param[in] algo               Algorithm running context pointer.
 * @param[in] pImgOut            output image of pureView
 *
 * @return
 *        -<em>MI_S32</em> @see mialgo_errorno.h
 */
PV_INT32 pureViewProc(PV_IMG *pImgIn, PV_CONFIG *pConfig, PV_IMG *pImgOut);

/**
 * @brief The AMBT algorithm initialization function.
 * @param[in] algo       Algorithm running context pointer.
 *
 * @return
 *        -<em>MI_S32</em> @see mialgo_errorno.h
 */
// PV_INT32 pureViewDestroy(PV_CONFIG *pConfig);

/**
 * @brief camera app flush API to quickly release PureView resource.
 * @param[in] pConfig      see PV_CONFIG.
 *
 * @return
 *        -<em>MI_S32</em> @see mialgo_errorno.h
 */
PV_INT32 pureViewFlush(PV_CONFIG *pConfig);

/**
 * @brief This function does de-initialization for miAi fucntion. this function should be called
 * when exit front camera in camera app
 * @param[in] pConfig      @see AMBT_CONFIG
 *
 * @return
 *
 */
PV_VOID pureViewDeInitMiAiLib();

#endif // !AMBT_API__
