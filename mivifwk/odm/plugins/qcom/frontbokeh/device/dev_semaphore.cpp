/**
 * @file        dev_semaphore.cpp
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

#include "dev_stdlib.h"
#include "dev_type.h"
#include "dev_log.h"
#include <time.h>
#include <sys/time.h>
#include <semaphore.h>
#include "dev_semaphore.h"

#include <pthread.h>
static pthread_mutex_t m_mutex;

#define DEV_SEMAPHORE_NUM_MAX     128
static U32 init_f = FALSE;

typedef struct __DEV_SEMAPHORE_NODE {
    U32 en;
    S64 *pId;
    CHAR __fileName[FILE_NAME_LEN_MAX];
    CHAR __tagName[FILE_NAME_LEN_MAX];
    U32 __fileLine;
    sem_t semaphore;
    S32 post;
} Device_Semaphore_node;

static Device_Semaphore_node m_semaphore_list[DEV_SEMAPHORE_NUM_MAX];

static const char* DevSemaphore_NoDirFileName(const char *pFilePath) {
    if (pFilePath == NULL) {
        return NULL;
    }
    const char *pFileName = strrchr(pFilePath, '/');
    if (NULL != pFileName) {
        pFileName += 1;
    } else {
        pFileName = pFilePath;
    }
    return pFileName;
}
S64 DevSemaphore_Create(S64 *pId, MARK_TAG tagName) {
    pthread_mutex_lock(&m_mutex);
    S64 ret = RET_ERR;
    S64 m_id = 0;
    DEV_IF_LOGE_GOTO((init_f != TRUE), exit, "Semaphore Init err");
    for (S64 id = 1; id < DEV_SEMAPHORE_NUM_MAX; id++) {
        if (m_semaphore_list[id].pId != NULL) {
            if (m_semaphore_list[id].pId == pId) {
                if (m_semaphore_list[id].en == TRUE) {
                    m_id = id;
                    DEV_IF_LOGW_GOTO(m_id != 0, exit, "double Create %ld", m_id);
                }
                DEV_LOGE("not Destroy err=%ld", id);
            }
        }
    }
    for (S64 id = 1; id < DEV_SEMAPHORE_NUM_MAX; id++) {
        if (m_semaphore_list[id].en == FALSE) {
            ret = sem_init(&m_semaphore_list[id].semaphore, 0, 0);
            DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Semaphore Create  err");
            m_id = id;
            *pId = m_id;
            m_semaphore_list[id].pId = pId;
            m_semaphore_list[id].en = TRUE;
            m_semaphore_list[id].__fileLine = __fileLine;
            Dev_sprintf(m_semaphore_list[id].__fileName, "[%s]", DevSemaphore_NoDirFileName(__fileName));
            Dev_sprintf(m_semaphore_list[id].__tagName, "[%s]", tagName);
            break;
        }
    }
    ret = RET_OK;
    exit: pthread_mutex_unlock(&m_mutex);
    DEV_IF_LOGE_RETURN_RET((m_id == 0), RET_ERR, "Create ID err");
    return ret;
}

S64 DevSemaphore_Destroy(S64 *pId) {
    pthread_mutex_lock(&m_mutex);
    S64 ret = RET_OK;
    S64 m_id = 0;
    if (init_f != TRUE) {
        ret = RET_ERR;
        goto exit;
    }
    DEV_IF_LOGE_GOTO((pId==NULL), exit, "Semaphore id err");
    DEV_IF_LOGE_GOTO(((*pId>=DEV_SEMAPHORE_NUM_MAX)||(*pId <= 0)), exit, "Semaphore id err");
    if (m_semaphore_list[*pId].en != TRUE) {
        *pId = 0;
        ret = RET_ERR;
        goto exit;
    }
    if (m_semaphore_list[*pId].pId == NULL) {
        *pId = 0;
        ret = RET_ERR;
        goto exit;
    }
    m_id = *m_semaphore_list[*pId].pId;
    DEV_IF_LOGE_GOTO(((m_id>=DEV_SEMAPHORE_NUM_MAX)||(m_id <= 0)), exit, "Semaphore id err");
    sem_destroy(&m_semaphore_list[*pId].semaphore);
//    *m_sem_list[*pId].pId = 0;
    memset((void*) &m_semaphore_list[m_id], 0, sizeof(Device_Semaphore_node));
    *pId = 0;
    exit: pthread_mutex_unlock(&m_mutex);
    return ret;
}

S64 DevSemaphore_Wait(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Semaphore Init err");
    DEV_IF_LOGE_RETURN_RET(((id>=DEV_SEMAPHORE_NUM_MAX)||(id <= 0)), RET_ERR, "Semaphore id err %ld", id);
    DEV_IF_LOGE_RETURN_RET((m_semaphore_list[id].en != TRUE), RET_ERR, "Semaphore Destroy err");
    S64 ret = RET_OK;
    ret = sem_wait(&m_semaphore_list[id].semaphore);
    m_semaphore_list[id].post--;
    return ret;
}

S64 DevSemaphore_TimedWait(S64 id, U64 ms) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Semaphore Init err");
    DEV_IF_LOGE_RETURN_RET(((id>=DEV_SEMAPHORE_NUM_MAX)||(id <= 0)), RET_ERR, "Semaphore id err %ld", id);
    DEV_IF_LOGE_RETURN_RET((m_semaphore_list[id].en != TRUE), RET_ERR, "Semaphore Destroy err");
    S64 ret = RET_OK;
    UINT timeoutSeconds = (ms / 1000UL);
    UINT timeoutNanoseconds = (ms % 1000UL) * 1000000UL;
    struct timespec timeout = { 0 };
    // Calculate the timeout time
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeoutSeconds += (static_cast<UINT>(timeout.tv_nsec) + timeoutNanoseconds) / 1000000000UL;
    timeoutNanoseconds = (static_cast<UINT>(timeout.tv_nsec) + timeoutNanoseconds) % 1000000000UL;
    timeout.tv_sec += static_cast<INT>(timeoutSeconds);
    timeout.tv_nsec = static_cast<INT>(timeoutNanoseconds);
    ret = sem_timedwait(&m_semaphore_list[id].semaphore, &timeout);
    m_semaphore_list[id].post--;
    return ret;
}

S64 DevSemaphore_Post(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Semaphore Init err");
    DEV_IF_LOGE_RETURN_RET(((id>=DEV_SEMAPHORE_NUM_MAX)||(id <= 0)), RET_ERR, "Semaphore id err");
    DEV_IF_LOGE_RETURN_RET((m_semaphore_list[id].en != TRUE), RET_ERR, "Semaphore Destroy err");
    S64 ret = RET_OK;
    ret = sem_post(&m_semaphore_list[id].semaphore);
    m_semaphore_list[id].post++;
    return ret;
}

S64 DevSemaphore_Report(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_LOGI("[-------------------------------------SEMAPHORE REPORT SATRT--------------------------------------]");
    pthread_mutex_lock(&m_mutex);
    for (int i = 0; i < DEV_SEMAPHORE_NUM_MAX; i++) {
        if ((m_semaphore_list[i].pId != NULL) && (m_semaphore_list[i].en == TRUE)) {
            DEV_LOGI("[---SEMAPHORE [%d][%s][%s][%d]pId[%p]SEMAPHORE[%p]POST[%d]---]"
                    , i + 1
                    , m_semaphore_list[i].__tagName
                    , m_semaphore_list[i].__fileName
                    , m_semaphore_list[i].__fileLine
                    , m_semaphore_list[i].pId
                    , &m_semaphore_list[i].semaphore
                    , m_semaphore_list[i].post
                    );
        }
    }
    pthread_mutex_unlock(&m_mutex);
    DEV_LOGI("[-------------------------------------SEMAPHORE REPORT END  --------------------------------------]");
    return RET_OK;
}

S64 DevSemaphore_Init(void) {
    if (init_f == TRUE) {
        return RET_OK;
    }
    S32 ret = pthread_mutex_init(&m_mutex, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
    Dev_memset((void*) &m_semaphore_list, 0, sizeof(m_semaphore_list));
    init_f = TRUE;
    return RET_OK;
}

S64 DevSemaphore_Deinit(void) {
    if (init_f != TRUE) {
        return RET_OK;
    }

    S32 report = 0;
    for (int i = 0; i < DEV_SEMAPHORE_NUM_MAX; i++) {
        if ((m_semaphore_list[i].pId != NULL) && (m_semaphore_list[i].en == TRUE)) {
            report++;
        }
    }
    if (report > 0) {
        DevSemaphore_Report();
    }

    for (S64 id = 1; id < DEV_SEMAPHORE_NUM_MAX; id++) {
        S64 idp = id;
        DevSemaphore_Destroy(&idp);
    }
    init_f = FALSE;
    Dev_memset((void*) &m_semaphore_list, 0, sizeof(m_semaphore_list));
    pthread_mutex_destroy(&m_mutex);
    return RET_OK;
}

const Dev_Semaphore m_dev_semaphore = {
        .Init       = DevSemaphore_Init,
        .Deinit     = DevSemaphore_Deinit,
        .Create     = DevSemaphore_Create,
        .Destroy    = DevSemaphore_Destroy,
        .TimedWait  = DevSemaphore_TimedWait,
        .Wait       = DevSemaphore_Wait,
        .Post       = DevSemaphore_Post,
        .Report     = DevSemaphore_Report,
};
