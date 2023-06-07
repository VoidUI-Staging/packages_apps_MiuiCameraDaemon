/************************************************************************************

Filename  : mialgo_cdsp.h
Content   :
Created   : Feb. 04, 2020
Author    : guwei (guwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_UTILS_CDSP_H__
#define MIALGO_UTILS_CDSP_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_basic.h"
#include "mialgo_type.h"

/**
* @brief cdsp clk level.
*/
typedef enum
{
    MIALGO_CDSP_CLK_INVALID = 0,                /*!< invalid cdsp clk level */
    MIALGO_CDSP_CLK_STANDBY,                    /*!< standby clk level */
    MIALGO_CDSP_CLK_LOW_POWER,                  /*!< low power clk level */
    MIALGO_CDSP_CLK_NORMAL,                     /*!< normal clk level */
    MIALGO_CDSP_CLK_TURBO,                      /*!< turbo clk level */
    MIALGO_CDSP_CLK_NUM,
} MialgoCdspClk;

/**
* @brief Mialgo sets the cdsp clk.
*   Before calling any cdsp algorithm, must call the MialgoSetCdspClk to set the cdsp clk level.
*   And it doesn't need to be called multiple times.
*   DSP can automatically go into standby mode, so you don't have to set standby mode after processing
*
* @param[in] algo               @see MialgoEngine, use NULL for default
* @param[in] cdsp_clk           @see MialgoCdspClk
*
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoSetCdspClk(MialgoEngine algo, MialgoCdspClk cdsp_clk);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_UTILS_CDSP_H__

