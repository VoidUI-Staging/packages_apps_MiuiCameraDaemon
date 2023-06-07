/************************************************************************************

Filename  : mialgo_cpu.h
Content   :
Created   : Nov. 21, 2019
Author    : guwei (guwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_CPU_H__
#define MIALGO_CPU_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_basic.h"
#include "mialgo_type.h"

/**
* @brief Mialgo macro, All kernel.
*/
#define MIALGO_CPU_ALL                      (0x20)

/**
* @brief Mialgo macro, Big kernel.
*/
#define MIALGO_CPU_BIG                      (0x40)

/**
* @brief Mialgo macro, Little kernel.
*/
#define MIALGO_CPU_LITTLE                   (0x80)

/**
* @brief Data structure of cpu frequency.
*/
typedef struct MialgoCpuFreq
{
    MI_S32 cpu_id;                       /*!< ID number of cpu*/
    MI_S32 max_freq;                     /*!< Maximum frequency of cpu*/
    MI_S32 min_freq;                     /*!< Minimum frequency of cpu*/
    MI_S32 cur_freq;                     /*!< Current frequency of cpu*/
} MialgoCpuFreq;

/**
* @brief Mialgo Gets the count of CPU-Die function. 
* @param[in]                No.
* @return
*         --<em>MialgoEngine</em> MI_S32 type, the count of CPU-Die.
*/
MI_S32 MialgoGetCpuCount();

/**
* @brief Mialgo Gets the frequency of CPU function.
* @param[in] cpu_id           MI_S32 type, ID number of cpu.
* @param[in] cpu_freq         MialgoCpuFreq type, Frequency of cpu.
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoGetCpuFreq(MI_S32 cpu_id, MialgoCpuFreq *cpu_freq);

/**
* @brief Mialgo Sets the affinity of CPU. 
* @param[in] mode           MI_S32 type, Setted as MIALGO_CPU_BIG means runing at big cpu big kernel or MIALGO_CPU_LITTLE means running at little kernel.
*
* MialgoSetCpuAffinity function can be used as following:
* @code
* // run at big cpu
*    MialgoSetCpuAffinity(MIALGO_CPU_BIG);
* @endcode
*
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoSetCpuAffinity(MI_S32 mode);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_CPU_H__

