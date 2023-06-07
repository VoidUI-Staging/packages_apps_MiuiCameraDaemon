/**
 * @file        dev_mutex.cpp
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

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "dev_type.h"
#include "dev_stdlib.h"
#include "dev_log.h"
#include "dev_mutex.h"

#include <pthread.h>
static pthread_mutex_t m_mutex;

#define DEV_MUTEX_NUM_MAX     256
static U32 init_f = FALSE;

typedef struct __DEV_MUTEX_NODE {
    U32 en;
    S64 *pId;
    CHAR __fileName[FILE_NAME_LEN_MAX];
    CHAR __tagName[FILE_NAME_LEN_MAX];
    U32 __fileLine;
    S32 locked;
    pthread_mutex_t mutex;
} Device_Mutex_node;

static Device_Mutex_node m_mutex_list[DEV_MUTEX_NUM_MAX] = { { 0 } };

static const char* DevMutex_NoDirFileName(const char *pFilePath) {
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

S64 DevMutex_Create(S64 *pId, MARK_TAG tagName) {
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value != TRUE) {
        return RET_OK;
    }
    pthread_mutex_lock(&m_mutex);
    S64 ret = RET_ERR;
    S64 m_id = 0;
    DEV_IF_LOGE_GOTO((init_f != TRUE), exit, "Mutex Init err");
    for (S64 id = 1; id < DEV_MUTEX_NUM_MAX; id++) {
        if (m_mutex_list[id].pId != NULL) {
            if (m_mutex_list[id].pId == pId) {
                if (m_mutex_list[id].en == TRUE) {
                    m_id = id;
                    DEV_IF_LOGW_GOTO(m_id != 0, exit, "double Create %ld", m_id);
                }
                DEV_LOGE("not Destroy err=%ld", id);
            }
        }
    }
    for (S64 id = 1; id < DEV_MUTEX_NUM_MAX; id++) {
        if (m_mutex_list[id].en == FALSE) {
            ret = pthread_mutex_init(&m_mutex_list[id].mutex, NULL);
            DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Mutex Create  err");
            m_id = id;
            *pId = m_id;
            m_mutex_list[id].pId = pId;
            m_mutex_list[id].en = TRUE;
            m_mutex_list[id].__fileLine = __fileLine;
            Dev_sprintf(m_mutex_list[id].__fileName, "%s", DevMutex_NoDirFileName(__fileName));
            Dev_sprintf(m_mutex_list[id].__tagName, "%s", tagName);
            break;
        }
    }
    ret = RET_OK;
    exit:
    pthread_mutex_unlock(&m_mutex);
    DEV_IF_LOGE((m_id == 0), "The number of locks is not enough, please increase DEV_MUTEX_NUM_MAX");
    return ret;
}

S64 DevMutex_Destroy(S64 *pId) {
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value != TRUE) {
        return RET_OK;
    }
    pthread_mutex_lock(&m_mutex);
    S64 m_id = 0;
    DEV_IF_LOGE_GOTO((init_f != TRUE), exit, "Mutex Init err");
    DEV_IF_LOGE_GOTO((pId==NULL), exit, "Mutex id err");
    DEV_IF_LOGE_GOTO(((*pId >= DEV_MUTEX_NUM_MAX) || (*pId <= 0)), exit, "Mutex id err");
    if (m_mutex_list[*pId].pId == NULL) {
        goto exit;
    }
    m_id = *m_mutex_list[*pId].pId;
    DEV_IF_LOGE_GOTO(((m_id >= DEV_MUTEX_NUM_MAX) || (m_id <= 0)), exit, "Mutex id err");
//    *m_mutex_list[*pId].pId = 0;
    pthread_mutex_destroy(&m_mutex_list[m_id].mutex);
    memset((void*) &m_mutex_list[m_id], 0, sizeof(Device_Mutex_node));
    *pId = 0;
    exit:
    pthread_mutex_unlock(&m_mutex);
    return RET_OK;
}

S64 DevMutex_Lock(S64 id) {
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value != TRUE) {
        return RET_OK;
    }
    S64 ret = RET_OK;
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Mutex Init err");
    DEV_IF_LOGE_RETURN_RET(((id>=DEV_MUTEX_NUM_MAX)||(id <= 0)), RET_ERR, "Mutex id err");
    DEV_IF_LOGE_RETURN_RET((m_mutex_list[id].en != TRUE), RET_ERR, "Mutex En err=%" PRIi64, id);
    ret = pthread_mutex_lock(&m_mutex_list[id].mutex);
    m_mutex_list[id].locked = TRUE;
    return ret;
}

S64 DevMutex_Unlock(S64 id) {
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value != TRUE) {
        return RET_OK;
    }
    S64 ret = RET_OK;
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Mutex Init err");
    DEV_IF_LOGE_RETURN_RET(((id>=DEV_MUTEX_NUM_MAX)||(id <= 0)), RET_ERR, "Mutex id err");
    DEV_IF_LOGE_RETURN_RET((m_mutex_list[id].en != TRUE), RET_ERR, "Mutex En err=%" PRIi64, id);
    ret = pthread_mutex_unlock(&m_mutex_list[id].mutex);
    m_mutex_list[id].locked = FALSE;
    return ret;
}

S64 DevMutex_Trylock(S64 id) {
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value != TRUE) {
        return RET_OK;
    }
    S64 ret = RET_OK;
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Mutex Init err");
    DEV_IF_LOGE_RETURN_RET(((id>=DEV_MUTEX_NUM_MAX)||(id <= 0)), RET_ERR, "Mutex id err");
    DEV_IF_LOGE_RETURN_RET((m_mutex_list[id].en != TRUE), RET_ERR, "Mutex En err");
    ret = pthread_mutex_trylock(&m_mutex_list[id].mutex);
    if (ret == RET_OK) {
        m_mutex_list[id].locked = TRUE;
    }
    return ret;
}

pthread_mutex_t* DevMutex_Geter(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), NULL, "Mutex Init err");
    DEV_IF_LOGE_RETURN_RET(((id>=DEV_MUTEX_NUM_MAX)||(id <= 0)), NULL, "Mutex id err");
    DEV_IF_LOGE_RETURN_RET((m_mutex_list[id].en != TRUE), NULL, "Mutex En err");
    return &m_mutex_list[id].mutex;
}

S64 DevMutex_Report(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_LOGI("[-------------------------------------MUTEX REPORT SATRT--------------------------------------]");
    pthread_mutex_lock(&m_mutex);
    for (int i = 0; i < DEV_MUTEX_NUM_MAX; i++) {
        if ((m_mutex_list[i].pId != NULL) && (m_mutex_list[i].en == TRUE)) {
            DEV_LOGI("[---MUTEX [%d][%s][%s][%d]pId[%p]MUTEX[%p]LOCKED[%d]---]"
                    , i + 1
                    , m_mutex_list[i].__tagName
                    , m_mutex_list[i].__fileName
                    , m_mutex_list[i].__fileLine
                    , m_mutex_list[i].pId
                    , &m_mutex_list[i].mutex
                    , m_mutex_list[i].locked
                    );
        }
    }
    pthread_mutex_unlock(&m_mutex);
    DEV_LOGI("[-------------------------------------MUTEX REPORT END  --------------------------------------]");
    return RET_OK;
}

S64 DevMutex_Init(void) {
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value != TRUE) {
        return RET_OK;
    }
    if (init_f == TRUE) {
        return RET_OK;
    }
    S32 ret = pthread_mutex_init(&m_mutex, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
    memset((void*) &m_mutex_list, 0, sizeof(m_mutex_list));
    init_f = TRUE;
    return RET_OK;
}

S64 DevMutex_Deinit(void) {
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value != TRUE) {
        return RET_OK;
    }
    if (init_f != TRUE) {
        return RET_OK;
    }

    S32 report = 0;
    for (int i = 0; i < DEV_MUTEX_NUM_MAX; i++) {
        if ((m_mutex_list[i].pId != NULL) && (m_mutex_list[i].en == TRUE)) {
            report++;
        }
    }
    if (report > 0) {
        DevMutex_Report();
    }

    for (S64 id = 1; id < DEV_MUTEX_NUM_MAX; id++) {
        S64 idp = id;
        DevMutex_Destroy(&idp);
    }
    init_f = FALSE;
    memset((void*) &m_mutex_list, 0, sizeof(m_mutex_list));
    pthread_mutex_destroy(&m_mutex);
    return RET_OK;
}

const Dev_Mutex m_dev_mutex = {
    .Init       = DevMutex_Init,
    .Deinit     = DevMutex_Deinit,
    .Create     = DevMutex_Create,
    .Destroy    = DevMutex_Destroy,
    .Lock       = DevMutex_Lock,
    .Unlock     = DevMutex_Unlock,
    .Trylock    = DevMutex_Trylock,
    .Report     = DevMutex_Report,
    .Geter      = DevMutex_Geter,
};
