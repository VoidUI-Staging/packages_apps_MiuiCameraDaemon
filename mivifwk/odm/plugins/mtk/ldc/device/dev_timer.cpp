/**
 * @file        dev_timer.cpp
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

#include "dev_timer.h"

#include <signal.h>
#include <stdio.h>
#include <time.h>

#include "dev_log.h"
#include "dev_type.h"

//#define CONFIG_PTHREAD_SUPPORT       (1)
//#undef CONFIG_PTHREAD_SUPPORT

#ifdef CONFIG_PTHREAD_SUPPORT
#include <pthread.h>
static pthread_mutex_t m_mutex;
#endif

#define DEV_TIMER_NUM_MAX    (128)
#define SYSTEM_POLLING_TIMER (1)

static S64 init_f = FALSE;
static S64 timer_Start_All_f = FALSE;
static timer_t timer;

typedef struct __TIMER_NODE
{
    S64 en;
    S64 start;
    Dev_Timer_CallBack callback;
    void *param;
    S64 accumulator;
    S64 interval;
    S64 repeat;
} Timer_node;

static Timer_node m_timer_list[DEV_TIMER_NUM_MAX];

S64 DevTimer_Stop_All(void);
S64 DevTimer_Start_All(void);

static void Dev_Timer_handler(int arg)
{
    S64 checking = 0;
    for (S64 i = 1; i < DEV_TIMER_NUM_MAX; i++) {
        if (m_timer_list[i].start == TRUE) {
            checking++;
            m_timer_list[i].accumulator++;
            if (m_timer_list[i].accumulator >= m_timer_list[i].interval) {
                m_timer_list[i].accumulator = 0;
                if (m_timer_list[i].repeat != TRUE) {
                    m_timer_list[i].start = FALSE;
                    checking--;
                }
                if (m_timer_list[i].callback != NULL) {
                    m_timer_list[i].callback(i, m_timer_list[i].param);
                }
            }
        }
    }
    if (checking == 0) {
        DevTimer_Stop_All();
    }
}

S64 DevTimer_Start_All(void)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Timer Init err");
    S64 ret = RET_OK;
    if (timer_Start_All_f != TRUE) {
        struct itimerspec its;
        its.it_value.tv_sec = 0;
        its.it_value.tv_nsec = 1000000 * SYSTEM_POLLING_TIMER;
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 1000000 * SYSTEM_POLLING_TIMER;
        ret = timer_settime(timer, 0, &its, NULL);
        DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Timer Init err");
        timer_Start_All_f = TRUE;
    }
    return RET_OK;
}

S64 DevTimer_Stop_All(void)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Timer Init err");
    S64 ret = RET_OK;
    if (timer_Start_All_f != FALSE) {
        struct itimerspec its;
        its.it_value.tv_sec = 0;
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        ret = timer_settime(timer, 0, &its, NULL);
        DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Timer Stop err");
        timer_Start_All_f = FALSE;
    }
    return RET_OK;
}

S64 DevTimer_Init(void)
{
    if (init_f == TRUE) {
        return RET_OK;
    }
    S32 ret = RET_OK;
#ifdef CONFIG_PTHREAD_SUPPORT
    ret = pthread_mutex_init(&m_mutex, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
#endif
    memset(m_timer_list, 0, sizeof(m_timer_list));
    struct sigevent evp;
    evp.sigev_value.sival_ptr = &timer;
    evp.sigev_notify = SIGEV_SIGNAL;
    evp.sigev_signo = SIGUSR1;
    signal(SIGUSR1, Dev_Timer_handler);
    ret = timer_create(CLOCK_MONOTONIC, &evp, &timer);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Timer Init err");
    init_f = TRUE;
    DevTimer_Start_All();
    return RET_OK;
}

S64 DevTimer_Create(Dev_Timer_CallBack callBack, void *arg)
{
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_lock(&m_mutex);
#endif
    S64 m_id = 0;
    S64 id = 0;
    DEV_IF_LOGE_GOTO((init_f != TRUE), exit, "Timer Init err");
    DEV_IF_LOGE_GOTO((callBack == NULL), exit, "Timer callback err");
    for (id = 1; id < DEV_TIMER_NUM_MAX; id++) {
        if (m_timer_list[id].en != TRUE) {
            m_timer_list[id].en = TRUE;
            m_timer_list[id].callback = callBack;
            m_timer_list[id].accumulator = 0;
            m_timer_list[id].param = arg;
            m_id = id;
            break;
        }
    }
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    DEV_IF_LOGE((m_id == 0), "Create ID err");
    return m_id;
}

S64 DevTimer_Destroy(S64 *id)
{
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_lock(&m_mutex);
#endif
    if (init_f != TRUE) {
        goto exit;
    }
    DEV_IF_LOGE_GOTO((id == NULL), exit, "Timer id err");
    DEV_IF_LOGE_GOTO(((*id >= DEV_TIMER_NUM_MAX) || (*id <= 0)), exit, "Timer id err");
    if (m_timer_list[*id].en != TRUE) {
        *id = 0;
        goto exit;
    }
    m_timer_list[*id].en = FALSE;
    m_timer_list[*id].accumulator = 0;
    m_timer_list[*id].start = FALSE;
    *id = 0;
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return RET_OK;
}

S64 DevTimer_Stop(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Timer Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DEV_TIMER_NUM_MAX) || (id <= 0)), RET_ERR, "Timer id err");
    DEV_IF_LOGE_RETURN_RET((m_timer_list[id].en != TRUE), RET_ERR, "Timer en err");
    m_timer_list[id].accumulator = 0;
    m_timer_list[id].start = FALSE;
    return RET_OK;
}

S64 DevTimer_Start(S64 id, S64 ms, S64 Repeat)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Timer Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DEV_TIMER_NUM_MAX) || (id <= 0)), RET_ERR, "Timer id err");
    DEV_IF_LOGE_RETURN_RET((m_timer_list[id].en != TRUE), RET_ERR, "Timer en err");
    DEV_IF_LOGE_RETURN_RET((m_timer_list[id].callback == NULL), RET_ERR, "Timer callback err");
    S64 temp = ms / SYSTEM_POLLING_TIMER;
    if (temp == 0) {
        temp = 1;
    }
    m_timer_list[id].interval = temp;
    m_timer_list[id].repeat = Repeat;
    m_timer_list[id].start = TRUE;
    m_timer_list[id].accumulator = 0;
    DevTimer_Start_All();
    return RET_OK;
}

S64 DevTimer_Uninit(void)
{
    if (init_f != TRUE) {
        return RET_OK;
    }
    for (S64 id = 1; id < DEV_TIMER_NUM_MAX; id++) {
        S64 idp = id;
        DevTimer_Destroy(&idp);
    }
    memset(m_timer_list, 0, sizeof(m_timer_list));
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_destroy(&m_mutex);
#endif
    timer_delete(timer);
    init_f = FALSE;
    return RET_OK;
}

const Dev_Timer m_dev_timer = {
    .Init = DevTimer_Init,
    .Uninit = DevTimer_Uninit,
    .Create = DevTimer_Create,
    .Destroy = DevTimer_Destroy,
    .Start = DevTimer_Start,
    .Stop = DevTimer_Stop,
    .Stop_All = DevTimer_Stop_All,
    .Start_All = DevTimer_Start_All,
};
