/**
 * @file		dev_sem.h
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

#ifndef __DEV_SEM_H__
#define __DEV_SEM_H__
#include "dev_type.h"

typedef struct __Dev_Sem Dev_Sem;
struct __Dev_Sem
{
    S64 (*Init)(void);
    S64 (*Uninit)(void);
    S64 (*Create)(void);
    S64 (*Destroy)(S64 *id);
    S64 (*Wait)(S64 id);
    S64 (*Post)(S64 id);
    S64 (*Value)(S64 id, S64 *value);
    S64 (*Trywait)(S64 id);
    S64 (*WaitTimeout)(S64 id, S64 ms);
};

extern const Dev_Sem m_dev_sem;

#endif //__DEV_SEM_H__
