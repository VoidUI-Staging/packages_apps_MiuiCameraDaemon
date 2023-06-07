/**
 * @file        dev_timer.c
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

#include <stdio.h>
#include <time.h>
#include <signal.h>
#include "dev_type.h"
#include "dev_log.h"
#include "device.h"
#include "dev_timer.h"

#define DEV_TIMER_NUM_MAX           (128)
#define SYSTEM_POLLING_TIMER        (1)

static S64 init_f = FALSE;
static S64 m_mutex_Id;
static S64 timer_Start_All_f = TRUE;
static timer_t m_Timer_ID;

typedef struct __TIMER_NODE {
    S64 en;
    S64 *pId;
    char __fileName[FILE_NAME_LEN_MAX];
    char __tagName[FILE_NAME_LEN_MAX];
    U32 __fileLine;
    S64 start;
    void * param;
    Dev_Timer_CallBack callback;
    S64 setTime;
    S64 overflowTimestamp;
    S64 repeat;
    S32 trigger;
} Timer_node;

static Timer_node m_timer_list[DEV_TIMER_NUM_MAX];

static void Dev_Timer_handler(union sigval v);

static const char* Dev_Timer_NoDirFileName(const char *pFilePath) {
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

static S64 DevTimer_InitSystemTimer(void) {
    S32 ret = RET_OK;
    struct sigevent evp = { {0}};
    evp.sigev_value.sival_int = 111;
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = Dev_Timer_handler;
    ret = timer_create(CLOCK_REALTIME, &evp, &m_Timer_ID);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Timer Init err");
    return ret;
}

static S64 DevTimer_DeinitSystemTimer(void) {
    timer_delete(m_Timer_ID);
    return RET_OK;
}

static S64 DevTimer_StartSystemTimer(U64 ms) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Timer Init err");
    DEV_IF_LOGE_RETURN_RET((ms <= 0), RET_ERR, "Timer Init err");
    S64 ret = RET_OK;
    struct itimerspec its = { {0}};
    its.it_value.tv_sec = ms / 1000;
    its.it_value.tv_nsec = 1000000 * (ms % 1000);
    its.it_interval.tv_sec = ms / 1000;
    its.it_interval.tv_nsec = 1000000 * (ms % 1000);
    ret = timer_settime(m_Timer_ID, 0, &its, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Timer Init err[%ld]", ret);
    return RET_OK;
}

static S64 DevTimer_StopSystemTimer(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Timer Init err");
    S64 ret = RET_OK;
    struct itimerspec its = { {0}};
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    ret = timer_settime(m_Timer_ID, 0, &its, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Timer Stop err");
    return RET_OK;
}

static S64 DevTimer_GetMinOverflowTime(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), -1, "Timer Init err");
    S64 checking = 0;
    S64 minOverflowTime = 0x7FFFFFFFFFFFFFFF;
    for (S64 i = 1; i < DEV_TIMER_NUM_MAX; i++) {
        if (m_timer_list[i].start == TRUE) {
            checking++;
            if ((S64) (m_timer_list[i].overflowTimestamp - (S64) Device->time->GetTimestampMs()) <= minOverflowTime) {
                minOverflowTime = (S64)(m_timer_list[i].overflowTimestamp - Device->time->GetTimestampMs());
            }
        }
    }
    if (minOverflowTime <= 0) {
        minOverflowTime = 1;
    }
    if (checking == 0) {
        minOverflowTime = -1;
    }
    return minOverflowTime;
}

static S64 DevTimer_SetMinOverflowTime(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), -1, "Timer Init err");
    for (S64 i = 1; i < DEV_TIMER_NUM_MAX; i++) {
        if (m_timer_list[i].start == TRUE) {
            if ((m_timer_list[i].overflowTimestamp - (S64) Device->time->GetTimestampMs()) <= 0) {
                if (m_timer_list[i].repeat == TRUE) {
                    m_timer_list[i].overflowTimestamp = Device->time->GetTimestampMs() + m_timer_list[i].setTime; //重新装载定时器
                }
                m_timer_list[i].trigger = TRUE; //定时器满足了
            }
        }
    }
    return RET_OK;
}

static S64 DevTimer_SetAction(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), -1, "Timer Init err");
    for (S64 i = 1; i < DEV_TIMER_NUM_MAX; i++) {
        if (m_timer_list[i].start == TRUE) {
            if (m_timer_list[i].trigger == TRUE) {
                m_timer_list[i].trigger = FALSE;
                if (m_timer_list[i].repeat != TRUE) {
                    m_timer_list[i].start = FALSE;
                }
                if (m_timer_list[i].callback != NULL) {

                    DEV_LOGI("[---TIMER ACTION START [%d][%s][%s][%d]pId[%p]START[%ld]ARG[%p]FUNCTION[%p]SET[%ldMS]OVERFLOW[%ld]REPEAT[%d]---]"
                            ,i+1
                            ,m_timer_list[i].__tagName
                            ,m_timer_list[i].__fileName
                            ,m_timer_list[i].__fileLine
                            ,m_timer_list[i].pId
                            ,m_timer_list[i].start
                            ,m_timer_list[i].param
                           , m_timer_list[i].callback
                           , m_timer_list[i].setTime
                           , m_timer_list[i].overflowTimestamp
                           , m_timer_list[i].repeat
                           );

                    m_timer_list[i].callback(i, m_timer_list[i].param);

                    DEV_LOGD("[---TIMER ACTION END  [%d][%s][%s][%d]pId[%p]START[%ld]ARG[%p]FUNCTION[%p]SET[%ldMS]OVERFLOW[%ld]REPEAT[%d]---]"
                            ,i+1
                            ,m_timer_list[i].__tagName
                            ,m_timer_list[i].__fileName
                            ,m_timer_list[i].__fileLine
                            ,m_timer_list[i].pId
                            ,m_timer_list[i].start
                            ,m_timer_list[i].param
                           , m_timer_list[i].callback
                           , m_timer_list[i].setTime
                           , m_timer_list[i].overflowTimestamp
                           , m_timer_list[i].repeat
                           );
                }
            }
        }
    }
    return RET_OK;
}

static void Dev_Timer_handler(union sigval v) {
    S64 ret = -1;
    DevTimer_SetMinOverflowTime();
    DevTimer_SetAction();
    DevTimer_StopSystemTimer();
    ret = DevTimer_GetMinOverflowTime();
    if (timer_Start_All_f == TRUE) {
        if (ret > 0) {
            DevTimer_StartSystemTimer(ret);
        }
    }
}

S64 DevTimer_Start_All(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Timer Init err");
    S64 ret = RET_OK;
    if (timer_Start_All_f != TRUE) {
        ret = DevTimer_StartSystemTimer(SYSTEM_POLLING_TIMER);
        DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Timer Init err");
        timer_Start_All_f = TRUE;
        union sigval s;
        Dev_Timer_handler(s);
    }
    return RET_OK;
}

S64 DevTimer_Stop_All(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Timer Init err");
    S64 ret = RET_OK;
    if (timer_Start_All_f != FALSE) {
        ret = DevTimer_StopSystemTimer();
        DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Timer Stop err");
        timer_Start_All_f = FALSE;
    }
    return RET_OK;
}

S64 DevTimer_Create(S64 *pId, Dev_Timer_CallBack callBack, void *arg, MARK_TAG tagName) {
    Device->mutex->Lock(m_mutex_Id);
    S64 ret = RET_ERR;
    S64 m_id = 0;
    S64 id = 0;
    DEV_IF_LOGE_GOTO((init_f != TRUE), exit, "Timer Init err");
    DEV_IF_LOGE_GOTO((callBack == NULL), exit, "Timer callback err");
    for (id = 1; id < DEV_TIMER_NUM_MAX; id++) {
        if (m_timer_list[id].pId != NULL) {
            if (m_timer_list[id].pId == pId) {
                if (m_timer_list[id].en == TRUE) {
                    m_id = id;
                    DEV_IF_LOGW_GOTO(m_id != 0, exit, "double Create %ld", m_id);
                }
                DEV_LOGE("not Destroy err=%ld", id);
            }
        }
    }
    id = 0;
    for (id = 1; id < DEV_TIMER_NUM_MAX; id++) {
        if (m_timer_list[id].en != TRUE) {
            m_timer_list[id].en = TRUE;
            m_timer_list[id].callback = callBack;
            m_timer_list[id].trigger = FALSE;
            m_timer_list[id].param = arg;
            m_id = id;
            *pId = m_id;
            m_timer_list[id].pId = pId;
            Dev_snprintf((char*) m_timer_list[id].__fileName, FILE_NAME_LEN_MAX, "%s", Dev_Timer_NoDirFileName(__fileName));
            Dev_snprintf((char*) m_timer_list[id].__tagName, FILE_NAME_LEN_MAX, "%s", tagName);
            m_timer_list[id].__fileLine = __fileLine;
            break;
        }
    }
    DEV_IF_LOGE_GOTO((id >= DEV_TIMER_NUM_MAX), exit,
            "The number of timer is not enough, the number of threads needs to be increased DEV_TIMER_NUM_MAX");
    ret = RET_OK;
    exit:
    Device->mutex->Unlock(m_mutex_Id);
    DEV_IF_LOGE_RETURN_RET(((m_id == 0)||(m_id >=DEV_TIMER_NUM_MAX)), RET_ERR, "Create ID err");
    return ret;
}

S64 DevTimer_Destroy(S64 *pId) {
    Device->mutex->Lock(m_mutex_Id);
    S64 ret = RET_OK;
    S64 m_id = 0;
    if (init_f != TRUE) {
        goto exit;
    }
    DEV_IF_LOGE_GOTO((pId==NULL), exit, "Timer id err");
    DEV_IF_LOGE_GOTO(((*pId>=DEV_TIMER_NUM_MAX)||(*pId <= 0)), exit, "Timer id err");
    if (m_timer_list[*pId].en != TRUE) {
        *pId = 0;
        ret = RET_ERR;
        goto exit;
    }
    if (m_timer_list[*pId].pId == NULL) {
        *pId = 0;
        ret = RET_ERR;
        goto exit;
    }
    m_id = *m_timer_list[*pId].pId;
    DEV_IF_LOGE_GOTO(((m_id>=DEV_TIMER_NUM_MAX)||(m_id <= 0)), exit, "Timer id err");
//    *m_timer_list[*pId].pId = 0;
    memset((void*) &m_timer_list[m_id], 0, sizeof(Timer_node));
    *pId = 0;
    exit:
    Device->mutex->Unlock(m_mutex_Id);
    return ret;
}

S64 DevTimer_Stop(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Timer Init err");
    DEV_IF_LOGE_RETURN_RET(((id>=DEV_TIMER_NUM_MAX)||(id <= 0)), RET_ERR, "Timer id err");
    DEV_IF_LOGE_RETURN_RET((m_timer_list[id].en != TRUE), RET_ERR, "Timer en err");
    m_timer_list[id].trigger = FALSE;
    m_timer_list[id].start = FALSE;
    union sigval s;
    Dev_Timer_handler(s);
    return RET_OK;
}

S64 DevTimer_Start(S64 id, S64 ms, S64 Repeat) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Timer Init err");
    DEV_IF_LOGE_RETURN_RET(((id>=DEV_TIMER_NUM_MAX)||(id <= 0)), RET_ERR, "Timer id err %ld", id);
    DEV_IF_LOGE_RETURN_RET((m_timer_list[id].en != TRUE), RET_ERR, "Timer en err");
    DEV_IF_LOGE_RETURN_RET((m_timer_list[id].callback == NULL), RET_ERR, "Timer callback err");
    m_timer_list[id].setTime = ms;
    m_timer_list[id].overflowTimestamp = Device->time->GetTimestampMs() + ms;
    m_timer_list[id].repeat = Repeat;
    m_timer_list[id].start = TRUE;
    m_timer_list[id].trigger = FALSE;

    DEV_LOGD("[---TIMER SET START [%d][%s][%s][%d]pId[%p]START[%ld]ARG[%p]FUNCTION[%p]SET[%ldMS]OVERFLOW[%ld]REPEAT[%d]---]"
            ,id+1
            ,m_timer_list[id].__tagName
            ,m_timer_list[id].__fileName
            ,m_timer_list[id].__fileLine
            ,m_timer_list[id].pId
            ,m_timer_list[id].start
            ,m_timer_list[id].param
           , m_timer_list[id].callback
           , m_timer_list[id].setTime
           , m_timer_list[id].overflowTimestamp
           , m_timer_list[id].repeat
           );

    union sigval s;
    Dev_Timer_handler(s);
    return RET_OK;
}

S64 DevTimer_Report(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_LOGI("[-------------------------------------TIMER REPORT SATRT--------------------------------------]");
    Device->mutex->Lock(m_mutex_Id);
    for (int i = 0; i < DEV_TIMER_NUM_MAX; i++) {
        if ((m_timer_list[i].en == TRUE) && (m_timer_list[i].pId != NULL)) {
            DEV_LOGI("[---[%d][%s][%s][%d]pId[%p]START[%ld]ARG[%p]FUNCTION[%p]SET[%ldMS]OVERFLOW[%ld]REPEAT[%d]---]"
                    ,i+1
                    ,m_timer_list[i].__tagName
                    ,m_timer_list[i].__fileName
                    ,m_timer_list[i].__fileLine
                    ,m_timer_list[i].pId
                    ,m_timer_list[i].start
                    ,m_timer_list[i].param
                   , m_timer_list[i].callback
                   , m_timer_list[i].setTime
                   , m_timer_list[i].overflowTimestamp
                   , m_timer_list[i].repeat
                   );
        }
    }
    Device->mutex->Unlock(m_mutex_Id);
    DEV_LOGI("[-------------------------------------TIMER REPORT END  --------------------------------------]");
    return RET_OK;
}

S64 DevTimer_Init(void) {
    if (init_f == TRUE) {
        return RET_OK;
    }
    S64 ret = RET_OK;
     ret = Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
    Dev_memset(m_timer_list, 0, sizeof(m_timer_list));
    ret = (S64)DevTimer_InitSystemTimer();
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Timer Init err");
    init_f = TRUE;
    DevTimer_Start_All();
    return RET_OK;
}
S64 DevTimer_Deinit(void) {
    if (init_f != TRUE) {
        return RET_OK;
    }
    S32 report = 0;
    for (int i = 0; i < DEV_TIMER_NUM_MAX; i++) {
        if ((m_timer_list[i].en == TRUE) && (m_timer_list[i].pId != NULL)) {
            report++;
        }
    }
    if (report > 0) {
        DevTimer_Report();
    }
    for (S64 id = 1; id < DEV_TIMER_NUM_MAX; id++) {
        S64 idp = id;
        DevTimer_Destroy(&idp);
    }
    Device->mutex->Destroy(&m_mutex_Id);
    DevTimer_DeinitSystemTimer();
    init_f = FALSE;
    Dev_memset(m_timer_list, 0, sizeof(m_timer_list));
    return RET_OK;
}

const Dev_Timer m_dev_timer = {
        .Init           =DevTimer_Init,
        .Deinit         =DevTimer_Deinit,
        .Create         =DevTimer_Create,
        .Destroy        =DevTimer_Destroy,
        .Start          =DevTimer_Start,
        .Stop           =DevTimer_Stop,
        .Stop_All       =DevTimer_Stop_All,
        .Start_All      =DevTimer_Start_All,
        .Report         =DevTimer_Report,
};

