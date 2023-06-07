/**
 * @file		dev_mutex.h
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

#ifndef __DEV_MUTEX_H__
#define __DEV_MUTEX_H__
#include "dev_type.h"

typedef struct __Dev_Mutex Dev_Mutex;
struct __Dev_Mutex
{
    S64 (*Init)(void);
    S64 (*Uninit)(void);
    S64 (*Create)(void);
    S64 (*Destroy)(S64 *id);
    S64 (*Lock)(S64 id);
    S64 (*Unlock)(S64 id);
    S64 (*Trylock)(S64 id);
};

extern const Dev_Mutex m_dev_mutex;

#endif //__DEV_MUTEX_H__
