/**
 * @file        dev_system.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#ifndef __DEV_SYSTEM_H__
#define __DEV_SYSTEM_H__

typedef struct __Dev_System Dev_System;
struct __Dev_System {
    S32 (*Init)(void);
    S32 (*Deinit)(void);
    S32 (*Handler)(void);
    S64 (*SleepMs)(S64 ms);
    S64 (*SleepUs)(S64 us);
    S64 (*ReBoot)(void);
    S64 (*Abort)(void);
    S64 (*ShowStack)(U64 pid, U64 tid);
};

extern const Dev_System m_dev_system;

#endif //__DEV_SYSTEM_H__
