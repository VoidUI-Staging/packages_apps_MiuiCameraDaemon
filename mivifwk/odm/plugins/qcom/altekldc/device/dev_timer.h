/**
 * @file        dev_timer.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version     0.1
 * @note
 * @warning
 * @par
 *
 */

#ifndef __DEV_TIMER_H__
#define __DEV_TIMER_H__

#define TIMER_ONE_MILLISECOND  (1)
#define TIMER_ONE_SECOND       (1000*TIMER_ONE_MILLISECOND)
#define TIMER_ONE_MINUTE       (60*TIMER_ONE_SECOND)
#define TIMER_ONE_HOUR         (60*TIMER_ONE_MINUTE)
#define TIMER_TIME_ALL         (0xFFFFFFFF)

typedef void (*Dev_Timer_CallBack)(S64 id, void* arg);

typedef struct __Dev_Timer Dev_Timer;
struct __Dev_Timer {
    S64 (*Init)(void);
    S64 (*Deinit)(void);
    S64 (*Create)(S64 *pId, Dev_Timer_CallBack callBack, void *arg, MARK_TAG tagName);
    S64 (*Destroy)(S64 *pId);
    S64 (*Start)(S64 id, S64 ms, S64 Repeat);
    S64 (*Stop)(S64 id);
    S64 (*Stop_All)(void);
    S64 (*Start_All)(void);
    S64 (*Report)(void);
};

extern const Dev_Timer m_dev_timer;

#endif  // __DEV_TIMER_H__
