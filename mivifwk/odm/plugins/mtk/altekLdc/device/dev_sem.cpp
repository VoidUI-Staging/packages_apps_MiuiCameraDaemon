/**
 * @file		dev_sem.cpp
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par
 *
 */

#include "dev_sem.h"

#include <semaphore.h>
#include <sys/time.h>
#include <time.h>

#include "config.h"
#include "dev_log.h"
#include "dev_type.h"

//#define CONFIG_PTHREAD_SUPPORT       (1)
//#undef CONFIG_PTHREAD_SUPPORT

#ifdef CONFIG_PTHREAD_SUPPORT
#include <pthread.h>
static pthread_mutex_t m_mutex;
#endif

#define DEV_SEM_NUM_MAX 128
static U32 init_f = FALSE;

typedef struct __DEV_SEM_NODE
{
    U32 en;
    sem_t sem;
} Device_Sem_node;

static Device_Sem_node m_sem_list[DEV_SEM_NUM_MAX];

S64 DevSem_Create(void)
{
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_lock(&m_mutex);
#endif
    S64 m_id = 0;
    DEV_IF_LOGE_GOTO((init_f != TRUE), exit, "Semaphore Init err");
    for (S64 id = 1; id < DEV_SEM_NUM_MAX; id++) {
        if (m_sem_list[id].en == FALSE) {
            m_sem_list[id].en = TRUE;
            S32 ret = sem_init(&m_sem_list[id].sem, 0, 0);
            DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Semaphore Create  err");
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

S64 DevSem_Destroy(S64 *id)
{
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_lock(&m_mutex);
#endif
    if (init_f != TRUE) {
        goto exit;
    }
    DEV_IF_LOGE_GOTO((id == NULL), exit, "Semaphore id err");
    DEV_IF_LOGE_GOTO(((*id >= DEV_SEM_NUM_MAX) || (*id <= 0)), exit, "Semaphore id err");
    if (m_sem_list[*id].en != TRUE) {
        *id = 0;
        goto exit;
    }
    m_sem_list[*id].en = FALSE;
    sem_destroy(&m_sem_list[*id].sem);
    *id = 0;
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return RET_OK;
}

S64 DevSem_Wait(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Semaphore Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DEV_SEM_NUM_MAX) || (id <= 0)), RET_ERR, "Semaphore id err");
    DEV_IF_LOGE_RETURN_RET((m_sem_list[id].en != TRUE), RET_ERR, "Semaphore Destroy err");
    return sem_wait(&m_sem_list[id].sem);
}

S64 DevSem_Post(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Semaphore Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DEV_SEM_NUM_MAX) || (id <= 0)), RET_ERR, "Semaphore id err");
    DEV_IF_LOGE_RETURN_RET((m_sem_list[id].en != TRUE), RET_ERR, "Semaphore Destroy err");
    return sem_post(&m_sem_list[id].sem);
}

S64 DevSem_Value(S64 id, S64 *value)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Semaphore Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DEV_SEM_NUM_MAX) || (id <= 0)), RET_ERR, "Semaphore id err");
    DEV_IF_LOGE_RETURN_RET((m_sem_list[id].en != TRUE), RET_ERR, "Semaphore Destroy err");
    return sem_getvalue(&m_sem_list[id].sem, (int *)value);
}

S64 DevSem_Trywait(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Semaphore Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DEV_SEM_NUM_MAX) || (id <= 0)), RET_ERR, "Semaphore id err");
    DEV_IF_LOGE_RETURN_RET((m_sem_list[id].en != TRUE), RET_ERR, "Semaphore Destroy err");
    return sem_trywait(&m_sem_list[id].sem);
}

S64 DevSem_WaitTimeout(S64 id, S64 ms)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Semaphore Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DEV_SEM_NUM_MAX) || (id <= 0)), RET_ERR, "Semaphore id err");
    DEV_IF_LOGE_RETURN_RET((m_sem_list[id].en != TRUE), RET_ERR, "Semaphore Destroy err");
    struct timespec abs_to;
    struct timeval now;
    U64 us;
    gettimeofday(&now, NULL);
    us = (U64)now.tv_sec * 1000000 + (U64)now.tv_usec + (U64)ms * 1000;
    abs_to.tv_sec = us / 1000000;
    abs_to.tv_nsec = (us % 1000000) * 1000;
    return sem_timedwait(&m_sem_list[id].sem, &abs_to);
}

S64 DevSem_Init(void)
{
    if (init_f == TRUE) {
        return RET_OK;
    }
#ifdef CONFIG_PTHREAD_SUPPORT
    S32 ret = pthread_mutex_init(&m_mutex, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
#endif
    memset((void *)&m_sem_list, 0, sizeof(m_sem_list));
    init_f = TRUE;
    return RET_OK;
}

S64 DevSem_Uninit(void)
{
    if (init_f != TRUE) {
        return RET_OK;
    }
    for (S64 id = 1; id < DEV_SEM_NUM_MAX; id++) {
        S64 idp = id;
        DevSem_Destroy(&idp);
    }
    memset((void *)&m_sem_list, 0, sizeof(m_sem_list));
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_destroy(&m_mutex);
#endif
    init_f = FALSE;
    return RET_OK;
}

const Dev_Sem m_dev_sem = {
    .Init = DevSem_Init,
    .Uninit = DevSem_Uninit,
    .Create = DevSem_Create,
    .Destroy = DevSem_Destroy,
    .Wait = DevSem_Wait,
    .Post = DevSem_Post,
    .Value = DevSem_Value,
    .Trywait = DevSem_Trywait,
    .WaitTimeout = DevSem_WaitTimeout,
};
