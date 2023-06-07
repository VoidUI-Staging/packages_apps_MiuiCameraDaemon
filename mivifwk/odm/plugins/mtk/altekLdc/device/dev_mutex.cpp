/**
 * @file		dev_mutex.cpp
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

#include "dev_mutex.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "dev_log.h"
#include "dev_type.h"

//#define CONFIG_PTHREAD_SUPPORT       (1)
//#undef CONFIG_PTHREAD_SUPPORT

#ifdef CONFIG_PTHREAD_SUPPORT
#include <pthread.h>
static pthread_mutex_t m_mutex;
#endif

#define DEV_MUTEX_NUM_MAX 128
static U32 init_f = FALSE;

typedef struct __DEV_MUTEX_NODE
{
    U32 en;
    pthread_mutex_t mutex;
} Device_Mutex_node;

static Device_Mutex_node m_mutex_list[DEV_MUTEX_NUM_MAX];

S64 DevMutex_Create(void)
{
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_lock(&m_mutex);
#endif
    S64 m_id = 0;
    DEV_IF_LOGE_GOTO((init_f != TRUE), exit, "Mutex Init err");
    for (S64 id = 1; id < DEV_MUTEX_NUM_MAX; id++) {
        if (m_mutex_list[id].en == FALSE) {
            m_mutex_list[id].en = TRUE;
            S32 ret = pthread_mutex_init(&m_mutex_list[id].mutex, NULL);
            DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
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

S64 DevMutex_Destroy(S64 *id)
{
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_lock(&m_mutex);
#endif
    if (init_f != TRUE) {
        goto exit;
    }
    DEV_IF_LOGE_GOTO((id == NULL), exit, "Mutex id err");
    DEV_IF_LOGE_GOTO(((*id >= DEV_MUTEX_NUM_MAX) || (*id <= 0)), exit, "Mutex id err");
    if (m_mutex_list[*id].en != TRUE) {
        *id = 0;
        return RET_OK;
    }
    m_mutex_list[*id].en = FALSE;
    pthread_mutex_destroy(&m_mutex_list[*id].mutex);
    *id = 0;
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return RET_OK;
}

S64 DevMutex_Lock(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Mutex Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DEV_MUTEX_NUM_MAX) || (id <= 0)), RET_ERR, "Mutex id err");
    DEV_IF_LOGE_RETURN_RET((m_mutex_list[id].en != TRUE), RET_ERR, "Mutex En err=%" PRIi64, id);
    return pthread_mutex_lock(&m_mutex_list[id].mutex);
}
S64 DevMutex_Unlock(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Mutex Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DEV_MUTEX_NUM_MAX) || (id <= 0)), RET_ERR, "Mutex id err");
    DEV_IF_LOGE_RETURN_RET((m_mutex_list[id].en != TRUE), RET_ERR, "Mutex En err=%" PRIi64, id);
    return pthread_mutex_unlock(&m_mutex_list[id].mutex);
}

S64 DevMutex_Trylock(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Mutex Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DEV_MUTEX_NUM_MAX) || (id <= 0)), RET_ERR, "Mutex id err");
    DEV_IF_LOGE_RETURN_RET((m_mutex_list[id].en != TRUE), RET_ERR, "Mutex En err");
    return pthread_mutex_trylock(&m_mutex_list[id].mutex);
}

S64 DevMutex_Init(void)
{
    if (init_f == TRUE) {
        return RET_OK;
    }
#ifdef CONFIG_PTHREAD_SUPPORT
    S32 ret = pthread_mutex_init(&m_mutex, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
#endif
    memset((void *)&m_mutex_list, 0, sizeof(m_mutex_list));
    init_f = TRUE;
    return RET_OK;
}

S64 DevMutex_Uninit(void)
{
    if (init_f != TRUE) {
        return RET_OK;
    }
    for (S64 id = 1; id < DEV_MUTEX_NUM_MAX; id++) {
        S64 idp = id;
        DevMutex_Destroy(&idp);
    }
    memset((void *)&m_mutex_list, 0, sizeof(m_mutex_list));
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_destroy(&m_mutex);
#endif
    init_f = FALSE;
    return RET_OK;
}

const Dev_Mutex m_dev_mutex = {
    .Init = DevMutex_Init,
    .Uninit = DevMutex_Uninit,
    .Create = DevMutex_Create,
    .Destroy = DevMutex_Destroy,
    .Lock = DevMutex_Lock,
    .Unlock = DevMutex_Unlock,
    .Trylock = DevMutex_Trylock,
};
