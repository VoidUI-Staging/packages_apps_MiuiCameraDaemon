/**
 * @file        dev_pthreadPool.cpp
 * @brief
 * @details
 * @author
 * @date        2021.07.03
 * @version     0.1
 * @note
 * @warning
 * @par
 *
 */
//TODO LIST
//增加FLUSH超时报警
//FLUSH前回调通知接口

#include "device.h"
#include <pthread.h>
#include "dev_pthreadPool.h"

#define PTHREAD_POOL_MAX_TODO           (PTHREAD_POOL_MAX_WOKER*4)
#define DEV_ENABLE_WORKER_CLEANUP       1

static U32 init_f = FALSE;
static S64 m_mutex_Id;
static pthread_cond_t mCondNotFull;
static S64 m_semaphore_Id = 0;

typedef enum {
    WORKER_STATE_IDLE = 0, WORKER_STATE_READY, WORKER_STATE_RUN, WORKER_STATE_END, WORKER_STATE_FLUSH,
} Enum_WORKER_STATE;

typedef struct __DEV_PTHREADPOOL_NODE {
    DEV_PTHREAD_POOL_P_FUNCTION function;
    void *arg1;
    void *arg2;
    char __fileName[FILE_NAME_LEN_MAX];
    char __tagName[FILE_NAME_LEN_MAX];
    U32 __fileLine;
    S64 postTimestamp;
    S64 flightTimestamp;
    U32 flight;
    U32 flush;
} DEV_PTHREADPOOL_NODE;

static DEV_PTHREADPOOL_NODE m_pthreadPool_list[PTHREAD_POOL_MAX_TODO] = { { 0 } };
static S32 m_pthreadPool_counter = 0;

typedef struct __DEV_PTHREADPOOL_WORKER {
    U32 index;
    S64 m_pthread_Id;
    Enum_WORKER_STATE state;
    S64 mutex_Id;
    pthread_cond_t mCondNotEmpty;
    DEV_PTHREADPOOL_NODE pthreadPool_node;
} DEV_PTHREADPOOL_WORKER;

static DEV_PTHREADPOOL_WORKER m_pthreadPool_workerList[PTHREAD_POOL_MAX_WOKER] = { { 0 } };

static const char* DevPthreadPool_NoDirFileName(const char *pFilePath) {
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

#ifdef DEV_ENABLE_WORKER_CLEANUP
static void DevPthreadPool_Worker_Cleanup(void *arg) {
    DEV_LOGW("DevPthreadPool_Worker EXIT ERR");
    Device->mutex->Unlock(m_mutex_Id);
}
#endif

static void* DevPthreadPool_Worker(void *arg) {
#ifdef DEV_ENABLE_WORKER_CLEANUP
    pthread_cleanup_push(DevPthreadPool_Worker_Cleanup,NULL);
#endif
        DEV_IF_LOGE_RETURN_RET((arg == NULL), NULL, "arg ERR");
        U32 index = *(U32*) arg;
        S64 ret = RET_OK;
        do {
            DEV_IF_LOGW_RETURN_RET((init_f != TRUE), NULL, "NOT INIT");
            ret = Device->mutex->Lock(m_pthreadPool_workerList[index].mutex_Id);
            DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "LOCK ERR");
            while (m_pthreadPool_counter == 0) { // m_pthreadPool_counter LOCK
                pthread_mutex_t *mutex = Device->mutex->Geter(m_pthreadPool_workerList[index].mutex_Id);
                DEV_IF_LOGE_GOTO((mutex == NULL), exit, "mutex err[%ld]", ret);
                m_pthreadPool_workerList[index].state = WORKER_STATE_IDLE;
                Device->semaphore->Post(m_semaphore_Id);//thread to be ready
                /* Last worker to go to sleep. */
                ret = pthread_cond_wait(&m_pthreadPool_workerList[index].mCondNotEmpty, mutex);
                DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "cond wait[%ld]", ret);
                if (init_f != TRUE) { //PthreadPool exit
                    goto exit;
                }
            }
            if (m_pthreadPool_workerList[index].state != WORKER_STATE_FLUSH) {
                m_pthreadPool_workerList[index].state = WORKER_STATE_RUN;
            }
            ret |= Device->mutex->Unlock(m_pthreadPool_workerList[index].mutex_Id);

            ret |= Device->mutex->Lock(m_mutex_Id);
            if (m_pthreadPool_counter == 0) {
                m_pthreadPool_workerList[index].state = WORKER_STATE_FLUSH;
                DEV_LOGE("m_pthreadPool_counter ERR! May need a lock");
            }
            DEV_PTHREADPOOL_NODE pthreadPool_node = m_pthreadPool_list[0]; // m_pthreadPool_list read LOCK
            ret |= Device->mutex->Unlock(m_mutex_Id);

            ret = Device->mutex->Lock(m_pthreadPool_workerList[index].mutex_Id);
            memset((void*) &m_pthreadPool_workerList[index].pthreadPool_node, 0, sizeof(m_pthreadPool_workerList[index].pthreadPool_node));
            m_pthreadPool_workerList[index].pthreadPool_node = pthreadPool_node;
            ret |= Device->mutex->Unlock(m_pthreadPool_workerList[index].mutex_Id);

            ret |= Device->mutex->Lock(m_mutex_Id);
            for (int i = 0; i < m_pthreadPool_counter - 1; i++) {
                m_pthreadPool_list[i] = m_pthreadPool_list[i + 1];
            }
            m_pthreadPool_counter--;
            if (m_pthreadPool_counter <= 0) {
                m_pthreadPool_counter = 0;
            }
            ret |= Device->mutex->Unlock(m_mutex_Id);
            ret |= Device->mutex->Lock(m_pthreadPool_workerList[index].mutex_Id);
            m_pthreadPool_workerList[index].pthreadPool_node.flight = 1;
            DEV_LOGI("[---THREAD FLIGHT START [%d][%s][%s][%d]FLIGHT[%d]FLUSH[%d]FUNCTION[%p]ARG1[%p]ARG2[%p]TIME[%ld]WAIT[%ld MS]RUN[0 MS]STATE[%d]---]"
                    , index
                    ,m_pthreadPool_workerList[index].pthreadPool_node.__tagName
                    ,m_pthreadPool_workerList[index].pthreadPool_node.__fileName
                    ,m_pthreadPool_workerList[index].pthreadPool_node.__fileLine
                    ,m_pthreadPool_workerList[index].pthreadPool_node.flight
                    ,m_pthreadPool_workerList[index].pthreadPool_node.flush
                    ,m_pthreadPool_workerList[index].pthreadPool_node.function
                    ,m_pthreadPool_workerList[index].pthreadPool_node.arg1
                   , m_pthreadPool_workerList[index].pthreadPool_node.arg2
                   , m_pthreadPool_workerList[index].pthreadPool_node.postTimestamp
                   , Device->time->GetTimestampMs()-m_pthreadPool_workerList[index].pthreadPool_node.postTimestamp
                   , m_pthreadPool_workerList[index].state
                   );
            m_pthreadPool_workerList[index].pthreadPool_node.flightTimestamp = (S64)Device->time->GetTimestampMs();
            ret = Device->mutex->Unlock(m_pthreadPool_workerList[index].mutex_Id);

            DEV_IF_LOGE_RETURN_RET((ret != RET_OK), NULL, "LOCK ERR");
            if ((m_pthreadPool_workerList[index].pthreadPool_node.function != NULL)
                    && (m_pthreadPool_workerList[index].state != WORKER_STATE_FLUSH)) {
                m_pthreadPool_workerList[index].pthreadPool_node.function(m_pthreadPool_workerList[index].pthreadPool_node.arg1,
                        m_pthreadPool_workerList[index].pthreadPool_node.arg2);
            }
            ret |= Device->mutex->Lock(m_pthreadPool_workerList[index].mutex_Id);
            m_pthreadPool_workerList[index].pthreadPool_node.flight = 0;
            m_pthreadPool_workerList[index].state = WORKER_STATE_END;
            DEV_LOGI("[---THREAD FLIGHT END [%d][%s][%s][%d]FLIGHT[%d]FLUSH[%d]FUNCTION[%p]ARG1[%p]ARG2[%p]TIME[%ld]WAIT[%ld MS]RUN[%ld MS]STATE[%d]---]"
                    , index
                    ,m_pthreadPool_workerList[index].pthreadPool_node.__tagName
                    ,m_pthreadPool_workerList[index].pthreadPool_node.__fileName
                    ,m_pthreadPool_workerList[index].pthreadPool_node.__fileLine
                    ,m_pthreadPool_workerList[index].pthreadPool_node.flight
                    ,m_pthreadPool_workerList[index].pthreadPool_node.flush
                    ,m_pthreadPool_workerList[index].pthreadPool_node.function
                    ,m_pthreadPool_workerList[index].pthreadPool_node.arg1
                   , m_pthreadPool_workerList[index].pthreadPool_node.arg2
                   , m_pthreadPool_workerList[index].pthreadPool_node.postTimestamp
                   , m_pthreadPool_workerList[index].pthreadPool_node.flightTimestamp-m_pthreadPool_workerList[index].pthreadPool_node.postTimestamp
                   , Device->time->GetTimestampMs()-m_pthreadPool_workerList[index].pthreadPool_node.flightTimestamp
                   , m_pthreadPool_workerList[index].state
                   );
            ret |= Device->mutex->Unlock(m_pthreadPool_workerList[index].mutex_Id);
            ret |= pthread_cond_signal(&mCondNotFull); //Can receive new thread msg
            DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "cond signal[%ld]", ret);
        } while (1);
exit:
        Device->mutex->Unlock(m_mutex_Id);
        Device->mutex->Unlock(m_pthreadPool_workerList[index].mutex_Id);
#ifdef DEV_ENABLE_WORKER_CLEANUP
    pthread_cleanup_pop(0);
#endif
    Device->pthread->Exit(NULL);
    return NULL;
}

S64 DevPthreadPool_Post(DEV_PTHREAD_POOL_P_FUNCTION function, void *arg1, void *arg2, MARK_TAG tagName) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((function == NULL), RET_ERR, "ARG ERR");
    S64 ret = RET_ERR;
    ret = Device->mutex->Lock(m_mutex_Id);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), ret, "mutex lock err");
    DEV_PTHREADPOOL_NODE pthreadPool_node = {0};
    pthreadPool_node.function = function;
    pthreadPool_node.arg1 = (void*)arg1;
    pthreadPool_node.arg2 = (void*)arg2;
    pthreadPool_node.__fileLine = __fileLine;
    pthreadPool_node.postTimestamp = (S64)Device->time->GetTimestampMs();
    Dev_snprintf(pthreadPool_node.__fileName, FILE_NAME_LEN_MAX, "%s", DevPthreadPool_NoDirFileName(__fileName));
    Dev_snprintf(pthreadPool_node.__tagName, FILE_NAME_LEN_MAX, "%s", tagName);
    pthread_mutex_t *mutex = Device->mutex->Geter(m_mutex_Id);
    DEV_IF_LOGE_GOTO((mutex == NULL), exit, "mutex Geter[%ld]", ret);
    if (m_pthreadPool_counter >= PTHREAD_POOL_MAX_TODO) {
        DEV_LOGW("The number of thread pools is not enough, the number of threads needs to be increased PTHREAD_POOL_MAX_WOKER");
        ret = pthread_cond_wait(&mCondNotFull, mutex); //Wait for other threads to finish running
        DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "cond wait[%ld]", ret);
        DEV_IF_LOGE_GOTO((m_pthreadPool_counter >= PTHREAD_POOL_MAX_TODO), exit, "The number of thread pools is not enough, the number of threads needs to be increased PTHREAD_POOL_MAX_WOKER");
    }
    m_pthreadPool_list[m_pthreadPool_counter] = pthreadPool_node;
    m_pthreadPool_counter++;
    Device->mutex->Unlock(m_mutex_Id);

    for (int i = 0; i < PTHREAD_POOL_MAX_WOKER; i++) {
        if (m_pthreadPool_workerList[i].state == WORKER_STATE_IDLE) {
            ret = Device->mutex->Lock(m_pthreadPool_workerList[i].mutex_Id);
            DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "LOCK ERR");
            m_pthreadPool_workerList[i].state = WORKER_STATE_READY;
            ret = Device->mutex->Unlock(m_pthreadPool_workerList[i].mutex_Id);
            DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "LOCK ERR");
            ret = (S64) pthread_cond_signal(&m_pthreadPool_workerList[i].mCondNotEmpty);
            break;
        }
    }
    exit: ret |= Device->mutex->Unlock(m_mutex_Id);
    return ret;
}

S64 DevPthreadPool_Report(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_LOGI("[-------------------------------------PTHREAD POOL REPORT SATRT--------------------------------------]");
    DEV_LOGI("[-------------------------------------PTHREAD POOL READY TODO %d--------------------------------------]", m_pthreadPool_counter);
    Device->mutex->Lock(m_mutex_Id);
    for (int i = 0; i < m_pthreadPool_counter; i++) {
        DEV_LOGI("[---[%d/%d][%s][%s][%d]FLIGHT[%d]FLUSH[%d]FUNCTION[%p]ARG1[%p]ARG2[%p]TIME[%ld]WAIT[%ld MS]RUN[0 MS]---]"
                ,i+1
                ,m_pthreadPool_counter
                , m_pthreadPool_list[i].__tagName
                ,m_pthreadPool_list[i].__fileName
                ,m_pthreadPool_list[i].__fileLine
                ,m_pthreadPool_list[i].flight
                ,m_pthreadPool_list[i].flush
                ,m_pthreadPool_list[i].function
                ,m_pthreadPool_list[i].arg1
               , m_pthreadPool_list[i].arg2
               , m_pthreadPool_list[i].postTimestamp
               , Device->time->GetTimestampMs()-m_pthreadPool_list[i].postTimestamp
               );
    }
    Device->mutex->Unlock(m_mutex_Id);
    DEV_LOGI("[----------------------------------------------------------------------------------------------------]");

    U32 toFlightNum = 0;
    for (int i = 0; i < PTHREAD_POOL_MAX_WOKER; i++) {
        Device->mutex->Lock(m_pthreadPool_workerList[i].mutex_Id);
        if (m_pthreadPool_workerList[i].state != WORKER_STATE_IDLE) {
            toFlightNum++;
        }
        Device->mutex->Unlock(m_pthreadPool_workerList[i].mutex_Id);
    }
    DEV_LOGI("[-------------------------------------PTHREAD POOL READY FLIGHT %d------------------------------------]", toFlightNum);
    for (int i = 0; i < PTHREAD_POOL_MAX_WOKER; i++) {
        Device->mutex->Lock(m_pthreadPool_workerList[i].mutex_Id);
        if (m_pthreadPool_workerList[i].state != WORKER_STATE_IDLE) {
        DEV_LOGI("[---[%d/%d][%s][%s][%d]FLIGHT[%d]FLUSH[%d]FUNCTION[%p]ARG1[%p]ARG2[%p]TIME[%ld]WAIT[%ld MS]RUN[%ld MS]STATE[%d]---]"
                ,i+1
                ,toFlightNum
                ,m_pthreadPool_workerList[i].pthreadPool_node.__tagName
                ,m_pthreadPool_workerList[i].pthreadPool_node.__fileName
                ,m_pthreadPool_workerList[i].pthreadPool_node.__fileLine
                ,m_pthreadPool_workerList[i].pthreadPool_node.flight
                ,m_pthreadPool_workerList[i].pthreadPool_node.flush
                ,m_pthreadPool_workerList[i].pthreadPool_node.function
                ,m_pthreadPool_workerList[i].pthreadPool_node.arg1
               , m_pthreadPool_workerList[i].pthreadPool_node.arg2
               , m_pthreadPool_workerList[i].pthreadPool_node.postTimestamp
               , m_pthreadPool_workerList[i].pthreadPool_node.flightTimestamp-m_pthreadPool_workerList[i].pthreadPool_node.postTimestamp
               , Device->time->GetTimestampMs()-m_pthreadPool_workerList[i].pthreadPool_node.flightTimestamp
               , m_pthreadPool_workerList[i].state
               );
        }
        Device->mutex->Unlock(m_pthreadPool_workerList[i].mutex_Id);
    }
    DEV_LOGI("[----------------------------------------------------------------------------------------------------]");
    DEV_LOGI("[-------------------------------------PTHREAD POOL REPORT END  --------------------------------------]");
    return RET_OK;
}

S64 DevPthreadPool_Flush(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    S64 ret = RET_OK;
    DEV_LOGI("[-------------------------------------PTHREAD POOL FLUSH SATRT--------------------------------------]");
    DEV_LOGI("[-------------------------------------PTHREAD POOL FLUSH READY TODO %d------------------------------]", m_pthreadPool_counter);
    Device->mutex->Lock(m_mutex_Id);
    U32 flushCounter = m_pthreadPool_counter;
    for (int i = flushCounter - 1; i >= 0; i--) {
        DEV_LOGI("[---FLUSH TODO [%d/%d][%s][%s][%d]FLIGHT[%d]FLUSH[%d]FUNCTION[%p]ARG1[%p]ARG2[%p]TIME[%ld]WAIT[%ld MS]RUN[0 MS]---]"
                ,i+1
                ,m_pthreadPool_counter
                , m_pthreadPool_list[i].__tagName
                ,m_pthreadPool_list[i].__fileName
                ,m_pthreadPool_list[i].__fileLine
                ,m_pthreadPool_list[i].flight
                ,m_pthreadPool_list[i].flush
                ,m_pthreadPool_list[i].function
                ,m_pthreadPool_list[i].arg1
               , m_pthreadPool_list[i].arg2
               , m_pthreadPool_list[i].postTimestamp
               , Device->time->GetTimestampMs()-m_pthreadPool_list[i].postTimestamp
               );
        memset((void*) &m_pthreadPool_list[i], 0, sizeof(DEV_PTHREADPOOL_NODE));
        m_pthreadPool_counter--;
        if (m_pthreadPool_counter <= 0) {
            m_pthreadPool_counter = 0;
        }
    }
    Device->mutex->Unlock(m_mutex_Id);
    DEV_LOGI("[----------------------------------------------------------------------------------------------------]");
    DEV_LOGI("[-------------------------------------PTHREAD POOL FLUSH READY FLIGHT   ------------------------------]");
    for (int i = 0; i < PTHREAD_POOL_MAX_WOKER; i++) {
        Device->mutex->Lock(m_pthreadPool_workerList[i].mutex_Id);
        if ((m_pthreadPool_workerList[i].state != WORKER_STATE_IDLE) && (m_pthreadPool_workerList[i].state != WORKER_STATE_FLUSH)) {
            m_pthreadPool_workerList[i].pthreadPool_node.flush = 1;
            m_pthreadPool_workerList[i].state = WORKER_STATE_FLUSH;
            ret = (S64) pthread_cond_signal(&m_pthreadPool_workerList[i].mCondNotEmpty);
        DEV_LOGI("[---FLUSH FLIGHT [%d][%s][%s][%d]FLIGHT[%d]FLUSH[%d]FUNCTION[%p]ARG1[%p]ARG2[%p]TIME[%ld]WAIT[%ld MS]RUN[%ld MS]STATE[%d]---]"
                ,i+1
                ,m_pthreadPool_workerList[i].pthreadPool_node.__tagName
                ,m_pthreadPool_workerList[i].pthreadPool_node.__fileName
                ,m_pthreadPool_workerList[i].pthreadPool_node.__fileLine
                ,m_pthreadPool_workerList[i].pthreadPool_node.flight
                ,m_pthreadPool_workerList[i].pthreadPool_node.flush
                ,m_pthreadPool_workerList[i].pthreadPool_node.function
                ,m_pthreadPool_workerList[i].pthreadPool_node.arg1
               , m_pthreadPool_workerList[i].pthreadPool_node.arg2
               , m_pthreadPool_workerList[i].pthreadPool_node.postTimestamp
               ,((m_pthreadPool_workerList[i].pthreadPool_node.flightTimestamp-m_pthreadPool_workerList[i].pthreadPool_node.postTimestamp)>0?(m_pthreadPool_workerList[i].pthreadPool_node.flightTimestamp-m_pthreadPool_workerList[i].pthreadPool_node.postTimestamp):0)
               ,((Device->time->GetTimestampMs()-m_pthreadPool_workerList[i].pthreadPool_node.flightTimestamp)>0?(Device->time->GetTimestampMs()-m_pthreadPool_workerList[i].pthreadPool_node.flightTimestamp):0)
               , m_pthreadPool_workerList[i].state
               );
        }
        Device->mutex->Unlock(m_pthreadPool_workerList[i].mutex_Id);
    }
    DEV_LOGI("[----------------------------------------------------------------------------------------------------]");
    DEV_LOGI("[-------------------------------------PTHREAD POOL FLUSH END  --------------------------------------]");

    return ret;
}

S64 DevPthreadPool_Init(void) {
    if (init_f == TRUE) {
        return RET_OK;
    }
    S64 ret = RET_OK;
    memset((void*) m_pthreadPool_workerList, 0, sizeof(m_pthreadPool_workerList));
    ret |= Device->semaphore->Create(&m_semaphore_Id, MARK("m_semaphore_Id"));
    ret |= Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Create mutex ID err");
    for (int i = 0; i < PTHREAD_POOL_MAX_WOKER; i++) {
        m_pthreadPool_workerList[i].index = i;
        ret |= (S64) pthread_cond_init(&m_pthreadPool_workerList[i].mCondNotEmpty, NULL);
        ret |= Device->mutex->Create(&m_pthreadPool_workerList[i].mutex_Id, MARK("m_mutex_Id"));
        DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Create cond ID err");
    }
    ret |= (S64) pthread_cond_init(&mCondNotFull, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Create cond ID err");
    memset((void*) m_pthreadPool_list, 0, sizeof(m_pthreadPool_list));
    m_pthreadPool_counter = 0;
    init_f = TRUE;
    for (U32 i = 0; i < PTHREAD_POOL_MAX_WOKER; i++) {
        ret |= Device->pthread->Create(&m_pthreadPool_workerList[i].m_pthread_Id, DevPthreadPool_Worker, &m_pthreadPool_workerList[i].index,
                MARK("m_pthread_Id"));
    }

    for (U32 i = 0; i < PTHREAD_POOL_MAX_WOKER; i++) {
        Device->semaphore->TimedWait(m_semaphore_Id, 500); //Wait for all threads to be ready
    }

    if (ret != RET_OK) {
        init_f = FALSE;
    }
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "PthreadPool Init err[%ld]", ret);
    return ret;
}

S64 DevPthreadPool_Deinit(void) {
    if (init_f != TRUE) {
        return RET_OK;
    }
    S64 ret = RET_OK;
    U32 toFlightNum = 0;
    for (int i = 0; i < PTHREAD_POOL_MAX_WOKER; i++) {
        if (m_pthreadPool_workerList[i].state != WORKER_STATE_IDLE) {
            toFlightNum++;
        }
    }
    if (m_pthreadPool_counter > 0 || toFlightNum != 0) {
        DevPthreadPool_Report();
    }
    init_f = FALSE;
    for (int i = 0; i < PTHREAD_POOL_MAX_WOKER; i++) {
        ret |= (S64) pthread_cond_broadcast(&m_pthreadPool_workerList[i].mCondNotEmpty);
    }
    ret |= (S64) pthread_cond_broadcast(&mCondNotFull);
    for (int i = 0; i < PTHREAD_POOL_MAX_WOKER; i++) {
        ret |= Device->pthread->Join(m_pthreadPool_workerList[i].m_pthread_Id, NULL);
        ret |= Device->pthread->Destroy(&m_pthreadPool_workerList[i].m_pthread_Id);
    }
    for (int i = 0; i < PTHREAD_POOL_MAX_WOKER; i++) {
        ret |= pthread_cond_destroy(&m_pthreadPool_workerList[i].mCondNotEmpty);
        ret |= Device->mutex->Destroy(&m_pthreadPool_workerList[i].mutex_Id);
    }
    ret |= pthread_cond_destroy(&mCondNotFull);
    ret |= Device->mutex->Destroy(&m_mutex_Id);
    Device->semaphore->Destroy(&m_semaphore_Id);
    memset((void*) m_pthreadPool_list, 0, sizeof(m_pthreadPool_list));
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "PthreadPool Deinit err[%ld]", ret);
    return ret;
}

const Dev_PthreadPool m_dev_pthreadPool = {
        .Init           =DevPthreadPool_Init,
        .Deinit         =DevPthreadPool_Deinit,
        .Post           =DevPthreadPool_Post,
        .Report         =DevPthreadPool_Report,
        .Flush          =DevPthreadPool_Flush,
};
