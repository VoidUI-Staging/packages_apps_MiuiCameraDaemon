/**
 * @file        dev_pthread.cpp
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

#include "dev_type.h"
#include "dev_log.h"
#include "device.h"
#include <pthread.h>
#include "dev_pthread.h"

#define DEV_PTHREAD_NUM_MAX     128

typedef struct __DEV_PTHREAD_NODE {
    U32 en;
    S64 *pId;
    CHAR __fileName[FILE_NAME_LEN_MAX];
    CHAR __tagName[FILE_NAME_LEN_MAX];
    U32 __fileLine;
    pthread_t pthread;
    pthread_attr_t attr;
} DEV_PTHREAD_NODE;

static DEV_PTHREAD_NODE m_pthread_list[DEV_PTHREAD_NUM_MAX] = { { 0 } };
static U32 init_f = FALSE;
static S64 m_mutex_Id;

static const char* DevPthread_NoDirFileName(const char *pFilePath) {
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

S64 DevPthread_Create(S64 *pId, void* (*function)(void*), void *arg, MARK_TAG tagName) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    Device->mutex->Lock(m_mutex_Id);
    S64 ret = RET_ERR;
    S64 m_id = 0;
    DEV_IF_LOGE_GOTO((init_f != TRUE), exit, "Mutex Init err");
    for (S64 id = 1; id < DEV_PTHREAD_NUM_MAX; id++) {
        if (m_pthread_list[id].pId != NULL) {
            if (m_pthread_list[id].pId == pId) {
                if (m_pthread_list[id].en == TRUE) {
                    m_id = id;
                    DEV_IF_LOGW_GOTO(m_id != 0, exit, "double Create %ld", m_id);
                }
                DEV_LOGE("not Destroy err=%ld", id);
            }
        }
    }
    for (S64 id = 1; id < DEV_PTHREAD_NUM_MAX; id++) {
        if (m_pthread_list[id].en == FALSE) {
            ret = pthread_attr_init(&m_pthread_list[id].attr);
            //    pthread_attr_setscope (&m_pthread_list[id].attr, PTHREAD_SCOPE_SYSTEM);
            //    pthread_attr_setdetachstate (&m_pthread_list[id].attr, PTHREAD_CREATE_DETACHED);
            DEV_IF_LOGE(ret, "pthread_attr_init");
            ret = pthread_create(&m_pthread_list[id].pthread, &m_pthread_list[id].attr, function, arg);
            DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Create ID err");
            DEV_IF_LOGE_RETURN_RET(((S64 ) m_pthread_list[id].pthread == 0), RET_ERR, "Create ID err");
            m_id = id;
//            m_id = (S64) m_pthread_list[id].pthread;
            *pId = m_id;
            m_pthread_list[id].pId = pId;
            m_pthread_list[id].en = TRUE;
            m_pthread_list[id].__fileLine = __fileLine;
            Dev_sprintf(m_pthread_list[id].__fileName, "%s", DevPthread_NoDirFileName(__fileName));
            Dev_sprintf(m_pthread_list[id].__tagName, "%s", tagName);
            break;
        }
    }
    ret = RET_OK;
    exit:
    Device->mutex->Unlock(m_mutex_Id);
    DEV_IF_LOGE((m_id == 0), "Create ID err");
    return ret;
}

S64 DevPthread_Destroy(S64 *pId) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    Device->mutex->Lock(m_mutex_Id);
    S64 m_id = 0;
    DEV_IF_LOGE_GOTO((pId==NULL), exit, "pthread id err");
    DEV_IF_LOGE_GOTO(((*pId >= DEV_PTHREAD_NUM_MAX) || (*pId <= 0)), exit, "pthread id err");
    if (m_pthread_list[*pId].pId == NULL) {
        goto exit;
    }
    m_id = *m_pthread_list[*pId].pId;
    DEV_IF_LOGE_GOTO(((m_id >= DEV_PTHREAD_NUM_MAX) || (m_id <= 0)), exit, "pthread id err");
//    *m_pthread_list[*pId].pId = 0;
    pthread_attr_destroy(&m_pthread_list[m_id].attr);
    memset((void*) &m_pthread_list[m_id], 0, sizeof(DEV_PTHREAD_NODE));
    *pId = 0;
    exit:
    Device->mutex->Unlock(m_mutex_Id);
    return RET_OK;
}

S64 DevPthread_Exit(void *value) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    pthread_exit(value);
    return RET_OK;
}

S64 DevPthread_Join(S64 id, void *value) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET(((id>=DEV_PTHREAD_NUM_MAX)||(id <= 0)), RET_ERR, "pthread id err");
    DEV_IF_LOGE_RETURN_RET((m_pthread_list[id].en != TRUE), RET_ERR, "pthread En err=%" PRIi64, id);
    S64 ret = pthread_join(m_pthread_list[id].pthread, &value);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "pthread_join err");
    return ret;
}

S64 DevPthread_Detach(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET(((id>=DEV_PTHREAD_NUM_MAX)||(id <= 0)), RET_ERR, "pthread id err");
    DEV_IF_LOGE_RETURN_RET((m_pthread_list[id].en != TRUE), RET_ERR, "pthread En err=%" PRIi64, id);
    return pthread_detach(m_pthread_list[id].pthread);
}

S64 DevPthread_Equal(S64 id1, S64 id2) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET(((id1>=DEV_PTHREAD_NUM_MAX)||(id1 <= 0)), RET_ERR, "pthread id err");
    DEV_IF_LOGE_RETURN_RET((m_pthread_list[id1].en != TRUE), RET_ERR, "pthread En err=%" PRIi64, id1);
    DEV_IF_LOGE_RETURN_RET(((id2>=DEV_PTHREAD_NUM_MAX)||(id2 <= 0)), RET_ERR, "pthread id err");
    DEV_IF_LOGE_RETURN_RET((m_pthread_list[id2].en != TRUE), RET_ERR, "pthread En err=%" PRIi64, id2);
    return pthread_equal(m_pthread_list[id1].pthread, m_pthread_list[id2].pthread);
}

S64 DevPthread_Self(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    S64 m_id = 0;
    pthread_t pthread = pthread_self();
    for (int i = 0; i < DEV_PTHREAD_NUM_MAX; i++) {
        if ((m_pthread_list[i].pId != NULL) && (m_pthread_list[i].en == TRUE)) {
            if (m_pthread_list[i].pthread == pthread) {
                m_id = i;
            }
        }
    }
    return m_id;
}

S64 DevPthread_Event_Create(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    return RET_ERR;
}

S64 DevPthread_Event_Send(S64 id, U32 flag) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    return RET_ERR;
}

S64 DevPthread_Event_Wait(S64 id, U32 flag, S64 timeout) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    return RET_ERR;
}

S64 DevPthread_Report(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_LOGI("[-------------------------------------PTHREAD REPORT SATRT--------------------------------------]");
    Device->mutex->Lock(m_mutex_Id);
    for (int i = 0; i < DEV_PTHREAD_NUM_MAX; i++) {
        if ((m_pthread_list[i].pId != NULL) && (m_pthread_list[i].en == TRUE)) {
            DEV_LOGI("[---PTHREAD [%d][%s][%s][%d]pId[%p]PTHREAD[%p]ATTR[%p]---]"
                    , i + 1
                    , m_pthread_list[i].__tagName
                    , m_pthread_list[i].__fileName
                    , m_pthread_list[i].__fileLine
                    , m_pthread_list[i].pId
                    , (void*)m_pthread_list[i].pthread
                    , (void*)&m_pthread_list[i].attr
                    );
        }
    }
    Device->mutex->Unlock(m_mutex_Id);
    DEV_LOGI("[-------------------------------------PTHREAD REPORT END  --------------------------------------]");
    return RET_OK;
}

S64 DevPthread_Init(void) {
    if (init_f == TRUE) {
        return RET_OK;
    }
    Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
    memset((void*) &m_pthread_list, 0, sizeof(m_pthread_list));
    init_f = TRUE;
    return RET_OK;
}

S64 DevPthread_Deinit(void) {
    if (init_f != TRUE) {
        return RET_OK;
    }
    S32 report = 0;
    for (int i = 0; i < DEV_PTHREAD_NUM_MAX; i++) {
        if ((m_pthread_list[i].pId != NULL) && (m_pthread_list[i].en == TRUE)) {
            report++;
        }
    }
    if (report > 0) {
        DevPthread_Report();
    }
    for (S64 id = 1; id < DEV_PTHREAD_NUM_MAX; id++) {
        S64 idp = id;
        DevPthread_Destroy(&idp);
    }
    init_f = FALSE;
    memset((void*) &m_pthread_list, 0, sizeof(m_pthread_list));
    Device->mutex->Destroy(&m_mutex_Id);
    return RET_OK;
}

const Dev_Pthread m_dev_pthread = {
        .Init           =DevPthread_Init,
        .Deinit         =DevPthread_Deinit,
        .Create         =DevPthread_Create,
        .Destroy        =DevPthread_Destroy,
        .Exit           =DevPthread_Exit,
        .Join           =DevPthread_Join,
        .Detach         =DevPthread_Detach,
        .Equal          =DevPthread_Equal,
        .Self           =DevPthread_Self,
        .Event_Create   =DevPthread_Event_Create,
        .Event_Send     =DevPthread_Event_Send,
        .Event_Wait     =DevPthread_Event_Wait,
        .Report         =DevPthread_Report,
};
