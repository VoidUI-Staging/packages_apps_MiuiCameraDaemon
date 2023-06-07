/**
 * @file		dev_system.h
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

#ifndef __DEV_SYSTEM_H__
#define __DEV_SYSTEM_H__
#include "dev_type.h"

typedef struct __Dev_System Dev_System;
struct __Dev_System
{
    S64 (*SleepMs)(S64 ms);
    S64 (*SleepUs)(S64 us);
    S64 (*Reboot)(S64 ms);
};

extern const Dev_System m_dev_system;

#endif //__DEV_SYSTEM_H__
