/**
 * @file        dev_detector.cpp
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
#include "device.h"
#include "dev_detector.h"

static S64 m_mutex_Id;
static S64 m_timer_Id;
static S64 m_mutexTimeout_Id;
static S64 g_traceId;

#define DETECTOR_NUM_MAX         64
#define DETECTOR_NAME_LEN_MAX    128
static U32 init_f = FALSE;

typedef struct _DETECTOR_NODE {
    S64 *pId;
    char __fileNameCreate[FILE_NAME_LEN_MAX];
    char __tagNameCreate[FILE_NAME_LEN_MAX];
    U32 __fileLineCreate;
    /* fps start*/
    char fpsStr[DETECTOR_NAME_LEN_MAX];
    U64 fps_start_time;
    U32 fps_add;
    U32 fps_run_flag;
    /*fps end*/
    /* run time start*/
    char consumeStr[DETECTOR_NAME_LEN_MAX];
    U64 pid;
    U64 tid;
    char __fileNamePrint[FILE_NAME_LEN_MAX];
    U32 __fileLinePrint;
    S32 run;
    U64 run_begin_time;
    char run_begin_time_str[32];
    U64 run_end_time;
    U64 run_all_time;
    U64 run_max_time;
    U64 run_min_time;
    U64 run_timeoutMsTimestamp;
    U64 run_timeout;
    U32 run_add;
    /* run time end*/
    U32 en;
} Detector_node;

static Detector_node m_detector_list[DETECTOR_NUM_MAX] = { { 0 } };

typedef struct _DETECTOR_ID_TIMEOUT {
    S64 id;
} Detector_id_timeout;
static S32 m_detector_id_timeout_num = 0;
static Detector_id_timeout m_detector_id_timeout_list[DETECTOR_NUM_MAX] = { { 0 } };

static const char* DevDetector_NoDirFileName(const char *pFilePath) {
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

S64 DevDetector_Create(S64 *pId, MARK_TAG tagName) {
    Device->mutex->Lock(m_mutex_Id);
    S64 m_id = 0;
    DEV_IF_LOGE_GOTO((init_f != TRUE), exit, "DETECTOR Init err");
    for (S64 id = 1; id < DETECTOR_NUM_MAX; id++) {
        if (m_detector_list[id].pId != NULL) {
            if (m_detector_list[id].pId == pId) {
                if (m_detector_list[id].en == TRUE) {
                    m_id = id;
                    DEV_IF_LOGW_GOTO(m_id != 0, exit, "DETECTOR double Create %ld", m_id);
                }
                DEV_LOGE("DETECTOR not Destroy err=%ld", id);
            }
        }
    }
    for (S64 id = 1; id < DETECTOR_NUM_MAX; id++) {
        if (m_detector_list[id].en == FALSE) {
            // run time
            m_detector_list[id].run_begin_time = Device->time->GetTimestampMs();
            Device->time->GetTimeStr(m_detector_list[id].run_begin_time_str);
            m_detector_list[id].run_end_time = Device->time->GetTimestampMs();
            m_detector_list[id].run_all_time = 0;
            m_detector_list[id].run_add = 0;
            m_detector_list[id].run_min_time = 0xffffffffffffffff;
            m_detector_list[id].run_max_time = 0;
            m_detector_list[id].run_timeoutMsTimestamp = 0;
            m_detector_list[id].run_timeout = 0;
            //fps
            m_detector_list[id].fps_start_time = Device->time->GetTimestampMs();
            m_detector_list[id].fps_add = 0;
            m_detector_list[id].fps_run_flag = 0;
            m_id = id;
            *pId = m_id;
            m_detector_list[id].pId = pId;
            m_detector_list[id].en = TRUE;
            Dev_snprintf((char*) m_detector_list[id].__fileNameCreate, FILE_NAME_LEN_MAX, "%s", DevDetector_NoDirFileName(__fileName));
            Dev_snprintf((char*) m_detector_list[id].__tagNameCreate, FILE_NAME_LEN_MAX, "%s", tagName);
            m_detector_list[id].__fileLineCreate = __fileLine;
            break;
        }
    }
    exit:
    Device->mutex->Unlock(m_mutex_Id);
    DEV_IF_LOGE_RETURN_RET((m_id == 0), RET_ERR, "Create ID err");
    return RET_OK;
}

S64 DevDetector_Destroy(S64 *pId) {
    Device->mutex->Lock(m_mutex_Id);
    S64 m_id = 0;
    if (init_f != TRUE) {
        goto exit;
    }
    DEV_IF_LOGE_GOTO((pId == NULL), exit, "Detector id err");
    DEV_IF_LOGE_GOTO(((*pId >= DETECTOR_NUM_MAX) || (*pId <= 0)), exit, "DETECTOR id err%0.f", (double ) *pId);
    if (m_detector_list[*pId].pId == NULL) {
        goto exit;
    }
    m_id = *m_detector_list[*pId].pId;
    DEV_IF_LOGE_GOTO(((m_id >= DETECTOR_NUM_MAX) || (m_id <= 0)), exit, "DETECTOR id err%0.f", (double ) *pId);
//    *m_detector_list[*pId].pId = 0;
    memset((void*)&m_detector_list[m_id], 0, sizeof(Detector_node));
    *pId = 0;
    exit:
    Device->mutex->Unlock(m_mutex_Id);
    return RET_OK;
}

static S64 DevDetector_Fps(S64 id, const char *str) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "DETECTOR Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DETECTOR_NUM_MAX) || (id <= 0)), RET_ERR, "DETECTOR id err");
    DEV_IF_LOGE_RETURN_RET((m_detector_list[id].en != TRUE), RET_ERR, "DETECTOR Fps err");
    DEV_IF_LOGE_RETURN_RET((str == NULL), 0, "DETECTOR arg err");
    if (m_detector_list[id].fps_add == 0) {
        m_detector_list[id].fps_start_time = Device->time->GetTimestampMs();
    }
    m_detector_list[id].fps_add++;
    if ((Dev_strlen(str) <= DETECTOR_NAME_LEN_MAX) && (Dev_strlen(str) > 0)) {
        Dev_sprintf(m_detector_list[id].fpsStr, "%s", str);
    }
    if (m_detector_list[id].fps_add == 60) {
        DEV_LOGI("[%s]FPS: %f ", m_detector_list[id].fpsStr,
                (double) (((double) (m_detector_list[id].fps_add) * 1000) / ((double) ((double) Device->time->GetTimestampMs() - (double) m_detector_list[id].fps_start_time))));
        m_detector_list[id].fps_add = 0;
    }
    return RET_OK;
}

S64 __DevDetector_Fps(S64 id, const char *str) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "DETECTOR Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DETECTOR_NUM_MAX) || (id <= 0)), RET_ERR, "DETECTOR id err");
    DEV_IF_LOGE_RETURN_RET((m_detector_list[id].en != TRUE), RET_ERR, "DETECTOR Fps err");
    DEV_IF_LOGE_RETURN_RET((str == NULL), 0, "DETECTOR arg err");
    m_detector_list[id].fps_run_flag = 1;
    return DevDetector_Fps(id, str);
}

S64 DevDetector_Reset(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "DETECTOR Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DETECTOR_NUM_MAX) || (id <= 0)), RET_ERR, "DETECTOR id err");
    DEV_IF_LOGE_RETURN_RET((m_detector_list[id].en != TRUE), RET_ERR, "DETECTOR Reset err");
    m_detector_list[id].run_all_time = 0;
    m_detector_list[id].run_add = 0;
    m_detector_list[id].run_timeoutMsTimestamp = 0;
    m_detector_list[id].run_timeout = 0;
    //fps
    m_detector_list[id].fps_start_time = Device->time->GetTimestampMs();
    m_detector_list[id].fps_add = 0;
    return RET_OK;
}

static S64 DevDetector_StartCountdown(S64 id) {
    DEV_IF_LOGE_RETURN_RET((id<0||id>=DETECTOR_NUM_MAX), RET_ERR, "DETECTOR ID err");
    Device->mutex->Lock(m_mutexTimeout_Id);
    Detector_id_timeout insertNode = { .id = id };
    S32 usedNum = m_detector_id_timeout_num;
    S32 front = 0;
    S32 rear = usedNum - 1;
    while (front <= rear) {
        if (m_detector_list[id].run_timeoutMsTimestamp
                > m_detector_list[m_detector_id_timeout_list[((front + rear) / 2)].id].run_timeoutMsTimestamp) {
            front = (front + rear) / 2 + 1;
        } else {
            rear = (front + rear) / 2 - 1;
        }
    }
    for (int j = usedNum - 1; j >= front; j--) {
        m_detector_id_timeout_list[j + 1] = m_detector_id_timeout_list[j];
    }
    if (front >= DETECTOR_NUM_MAX) {
        DEV_LOGE("front err =[%d:%d]", front, DETECTOR_NUM_MAX);
        front = DETECTOR_NUM_MAX - 1;
    }
    m_detector_id_timeout_list[front] = insertNode;
    m_detector_id_timeout_num++;
    if (m_detector_id_timeout_num >= DETECTOR_NUM_MAX) {
        DEV_LOGE("m_detector_id_timeout_num ID err =[%d:%d]", m_detector_id_timeout_num, DETECTOR_NUM_MAX);
        m_detector_id_timeout_num = DETECTOR_NUM_MAX - 1;
    }
    Device->mutex->Unlock(m_mutexTimeout_Id);
    if (m_detector_id_timeout_list[0].id > 0 && m_detector_id_timeout_list[0].id < DETECTOR_NUM_MAX) {
        if (m_detector_list[m_detector_id_timeout_list[0].id].run_timeoutMsTimestamp != 0) {
            S64 timeOut = 0;
            if (m_detector_list[m_detector_id_timeout_list[0].id].run_timeoutMsTimestamp > Device->time->GetTimestampMs()) {
                timeOut = m_detector_list[m_detector_id_timeout_list[0].id].run_timeoutMsTimestamp - Device->time->GetTimestampMs();
            }
            if (timeOut > 0) {
                Device->timer->Stop(m_timer_Id);
                Device->timer->Start(m_timer_Id, timeOut, 0);
            } else {
                Device->timer->Stop(m_timer_Id);
                Device->timer->Start(m_timer_Id, 1, 0);
            }
        }
    } else {
        DEV_LOGE("DETECTOR ID err %ld", m_detector_id_timeout_list[0].id);
    }
    return RET_OK;
}

static void DevDetector_TimerHandle(S64 id, void *arg) {
    Device->mutex->Lock(m_mutexTimeout_Id);
    S64 minTimeOut = 0x0FFFFFFFFFFFFFFF;
    for (int i = 0; i < m_detector_id_timeout_num; i++) {
        S64 detectorId = m_detector_id_timeout_list[i].id;
        if (detectorId < 0 || detectorId >= DETECTOR_NUM_MAX) {
            continue;
            DEV_LOGE("DETECTOR ID err");
        }
        if (m_detector_list[detectorId].run_timeoutMsTimestamp == 0 || m_detector_list[detectorId].run_timeout == 0) {
            continue;
        }
        if (m_detector_list[detectorId].run_timeoutMsTimestamp <= m_detector_list[detectorId].run_begin_time) {
            continue;
        }
        if (Device->time->GetTimestampMs() >= m_detector_list[detectorId].run_timeoutMsTimestamp) {
            DEV_LOGE_S(m_detector_list[detectorId].__fileNamePrint, "", m_detector_list[detectorId].__fileLinePrint,
                    "PERFORMANCE TIME OUT ERROR %4d  %4d[START][%s]FORECAST[%ld]EXCEED[%ld]MS[%s]", m_detector_list[detectorId].pid,
                    m_detector_list[detectorId].tid, m_detector_list[detectorId].run_begin_time_str, m_detector_list[detectorId].run_timeout,
                    Device->time->GetTimestampMs() - m_detector_list[detectorId].run_begin_time, m_detector_list[detectorId].consumeStr);
            Device->system->ShowStack(m_detector_list[detectorId].pid, m_detector_list[detectorId].tid);
            char buffer[DETECTOR_NAME_LEN_MAX + 32] = { 0 };
            Dev_snprintf(buffer, sizeof(buffer), "%s%s[%lu:%lu]", m_detector_list[id].consumeStr, "TIME OUT ERROR",
                    m_detector_list[detectorId].run_timeout, Device->time->GetTimestampMs() - m_detector_list[detectorId].run_begin_time);
            Device->trace->Message(g_traceId, m_detector_list[id].__fileNamePrint, m_detector_list[id].__fileLinePrint, m_detector_list[id].pid,
                    m_detector_list[id].tid, buffer);
            // 清除超时信息。
            for (int j = i; j < m_detector_id_timeout_num - 1; j++) {
                m_detector_id_timeout_list[j] = m_detector_id_timeout_list[j + 1];
            }
            memset(&m_detector_id_timeout_list[m_detector_id_timeout_num], 0, sizeof(Detector_id_timeout));
            m_detector_list[detectorId].run_timeoutMsTimestamp = 0;
            m_detector_list[detectorId].run_timeout = 0;
            m_detector_id_timeout_num--;
            if (m_detector_id_timeout_num < 0) {
                m_detector_id_timeout_num = 0;
            }
        } else {
            if (m_detector_list[detectorId].run_timeoutMsTimestamp >= Device->time->GetTimestampMs()) {
                S64 timeOut = m_detector_list[detectorId].run_timeoutMsTimestamp - Device->time->GetTimestampMs();
                if (timeOut < minTimeOut) {
                    minTimeOut = timeOut;
                }
            }
        }
    }
    Device->mutex->Unlock(m_mutexTimeout_Id);
    if (minTimeOut > 0 && minTimeOut < 0x0FFFFFFFFFFFFFFF) {
        Device->timer->Stop(m_timer_Id);
        Device->timer->Start(m_timer_Id, minTimeOut, 0);
    } else {
        Device->timer->Stop(m_timer_Id);
    }
}

S64 DevDetector_Begin(S64 id, MARK_TAG outStr, U64 timeoutMs) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "DETECTOR Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DETECTOR_NUM_MAX) || (id <= 0)), RET_ERR, "DETECTOR id err[%ld]", id);
    DEV_IF_LOGE_RETURN_RET((m_detector_list[id].en != TRUE), RET_ERR, "DETECTOR Begin err[%ld]", id);
    DEV_IF_LOGE_RETURN_RET((outStr == NULL), 0, "DETECTOR arg err");
    m_detector_list[id].pid = __pid;
    m_detector_list[id].tid = __tid;
    Dev_snprintf((char*) m_detector_list[id].__fileNamePrint, FILE_NAME_LEN_MAX, "%s", DevDetector_NoDirFileName(__fileName));
    m_detector_list[id].__fileLinePrint = __fileLine;

    if ((Dev_strlen(outStr) <= DETECTOR_NAME_LEN_MAX) && (Dev_strlen(outStr) > 0)) {
        Dev_sprintf(m_detector_list[id].consumeStr, "%s", outStr);
    }
    m_detector_list[id].run_begin_time = Device->time->GetTimestampMs();
    Device->time->GetTimeStr(m_detector_list[id].run_begin_time_str);
    if (timeoutMs > 0) {
        m_detector_list[id].run_timeoutMsTimestamp = m_detector_list[id].run_begin_time + timeoutMs;
    } else {
        m_detector_list[id].run_timeoutMsTimestamp = 0;
    }
    m_detector_list[id].run_timeout = timeoutMs;
    m_detector_list[id].run = TRUE;
    DEV_LOGI_S(m_detector_list[id].__fileNamePrint, "", m_detector_list[id].__fileLinePrint, "PERFORMANCE START %4d  %4d[%s]FORECAST[%ld]MS[%s]",
            m_detector_list[id].pid, m_detector_list[id].tid, m_detector_list[id].run_begin_time_str, m_detector_list[id].run_timeout,
            m_detector_list[id].consumeStr);
    /* For trace */
    Device->trace->Message(g_traceId, m_detector_list[id].__fileNamePrint, m_detector_list[id].__fileLinePrint, m_detector_list[id].pid,
            m_detector_list[id].tid, m_detector_list[id].consumeStr);
    Device->trace->AsyncBegin(g_traceId, m_detector_list[id].__fileNamePrint, m_detector_list[id].__fileLinePrint, m_detector_list[id].pid,
            m_detector_list[id].tid, m_detector_list[id].consumeStr, m_detector_list[id].tid);
    /* For trace */
    DevDetector_StartCountdown(id);
    return RET_OK;
}

S64 DevDetector_End(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "DETECTOR Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DETECTOR_NUM_MAX) || (id <= 0)), 0, "DETECTOR id err[%ld]", id);
    DEV_IF_LOGE_RETURN_RET((m_detector_list[id].en != TRUE), 0, "DETECTOR End err[%ld]", id);
    //run time
    m_detector_list[id].run_end_time = Device->time->GetTimestampMs();
    m_detector_list[id].run_all_time += m_detector_list[id].run_end_time - m_detector_list[id].run_begin_time;
    m_detector_list[id].run_add++;
    double currentTime = (double)(m_detector_list[id].run_end_time - m_detector_list[id].run_begin_time);
    if (m_detector_list[id].run_max_time < (U64)currentTime) {
        m_detector_list[id].run_max_time = (U64)currentTime;
    }
    if (m_detector_list[id].run_min_time > (U64)currentTime) {
        m_detector_list[id].run_min_time = (U64)currentTime;
    }
    DEV_LOGI_S(m_detector_list[id].__fileNamePrint, "", m_detector_list[id].__fileLinePrint,
            "PERFORMANCE END   %4d  %4d[%s]ACTUAL[%.2f] AVG:[%.2f] MAX:[%.2f] MIN:[%.2f][%s]", m_detector_list[id].pid, m_detector_list[id].tid,
            m_detector_list[id].run_begin_time_str, currentTime,
            (double ) ((double ) (m_detector_list[id].run_all_time) / (double ) (m_detector_list[id].run_add)),
            (double ) m_detector_list[id].run_max_time, (double ) m_detector_list[id].run_min_time, m_detector_list[id].consumeStr);
    /* For trace */
    Device->trace->AsyncEnd(g_traceId, m_detector_list[id].__fileNamePrint, m_detector_list[id].__fileLinePrint, m_detector_list[id].pid,
            m_detector_list[id].tid, m_detector_list[id].consumeStr, m_detector_list[id].tid);
    /* For trace */
    if (m_detector_list[id].run_all_time >= 0xffffffffff000000) {
        DevDetector_Reset(id);
    }
    Device->mutex->Lock(m_mutexTimeout_Id);
    // 清除超时信息。
    for (int i = 0; i < m_detector_id_timeout_num; i++) {
        if (m_detector_id_timeout_list[i].id == id) {
            for (int j = i; j < m_detector_id_timeout_num - 1; j++) {
                m_detector_id_timeout_list[j] = m_detector_id_timeout_list[j + 1];
            }
            memset(&m_detector_id_timeout_list[m_detector_id_timeout_num], 0, sizeof(Detector_id_timeout));
            m_detector_id_timeout_num--;
            if (m_detector_id_timeout_num < 0) {
                DEV_LOGE("m_detector_id_timeout_num ID err =[%d:%d]", m_detector_id_timeout_num, DETECTOR_NUM_MAX);
                m_detector_id_timeout_num = 0;
            }
        }
    }
    Device->mutex->Unlock(m_mutexTimeout_Id);
    m_detector_list[id].run_timeoutMsTimestamp = 0;
    m_detector_list[id].run_timeout = 0;
    m_detector_list[id].run = FALSE;
    //fps
    if (m_detector_list[id].fps_run_flag == 0) {
        DevDetector_Fps(id, m_detector_list[id].consumeStr);
    }
    return (S64) currentTime;
}

S64 DevDetector_Report(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_LOGI("[-------------------------------------DETECTOR REPORT SATRT--------------------------------------]");
    Device->mutex->Lock(m_mutex_Id);
    for (int i = 0; i < DETECTOR_NUM_MAX; i++) {
        if ((m_detector_list[i].pId != NULL) && (m_detector_list[i].en == TRUE)) {
            DEV_LOGI("[---DETECTOR [%d][%s][%s][%d]pId[%p][%s]MAX[%ldMS]MIN[%ldMS]AVG[%ldMS]RUN[%ld]FPS NAME[%s]FPS EN[%d]---]"
                    , i + 1
                    , m_detector_list[i].__tagNameCreate
                    , m_detector_list[i].__fileNameCreate
                    , m_detector_list[i].__fileLineCreate
                    , m_detector_list[i].pId
                    , m_detector_list[i].consumeStr
                    , m_detector_list[i].run_max_time
                    , m_detector_list[i].run_min_time
                    , m_detector_list[i].run_all_time/(m_detector_list[i].run_add == 0 ? 1 : m_detector_list[i].run_add)
                    , m_detector_list[i].run
                    , m_detector_list[i].fpsStr
                    , m_detector_list[i].fps_run_flag
                    );
        }
    }
    Device->mutex->Unlock(m_mutex_Id);
    DEV_LOGI("[-------------------------------------DETECTOR REPORT END  --------------------------------------]");
    return RET_OK;
}

S64 DevDetector_Init(void) {
    if (init_f == TRUE) {
        return RET_OK;
    }
    S32 ret = Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
    ret |= Device->mutex->Create(&m_mutexTimeout_Id, MARK("m_mutexTimeout_Id"));
    ret |= Device->timer->Create(&m_timer_Id, DevDetector_TimerHandle, NULL, MARK("m_timer_Id"));
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
    Dev_memset((void*)&m_detector_list, 0, sizeof(m_detector_list));
    Dev_memset((void*)&m_detector_id_timeout_list, 0, sizeof(m_detector_id_timeout_list));
    m_detector_id_timeout_num = 0;
    Device->trace->Create(&g_traceId, MARK("g_traceId"));
    init_f = TRUE;
    return RET_OK;
}

S64 DevDetector_Deinit(void) {
    if (init_f != TRUE) {
        return RET_OK;
    }
    S32 report = 0;
    for (int i = 0; i < DETECTOR_NUM_MAX; i++) {
        if ((m_detector_list[i].pId != NULL) && (m_detector_list[i].en == TRUE)) {
            report++;
        }
    }
    if (report > 0) {
        DevDetector_Report();
    }
    for (S64 id = 1; id < DETECTOR_NUM_MAX; id++) {
        S64 idp = id;
        DevDetector_Destroy(&idp);
    }
    init_f = FALSE;
    m_detector_id_timeout_num = 0;
    Dev_memset((void*)&m_detector_list, 0, sizeof(m_detector_list));
    Dev_memset((void*)&m_detector_id_timeout_list, 0, sizeof(m_detector_id_timeout_list));
    Device->trace->Destroy(&g_traceId);
    Device->timer->Destroy(&m_timer_Id);
    Device->mutex->Destroy(&m_mutexTimeout_Id);
    Device->mutex->Destroy(&m_mutex_Id);
    return RET_OK;
}

const Dev_Detector m_dev_detector = {
        .Init       = DevDetector_Init,
        .Deinit     = DevDetector_Deinit,
        .Create     = DevDetector_Create,
        .Destroy    = DevDetector_Destroy,
        .Begin      = DevDetector_Begin,
        .End        = DevDetector_End,
        .Reset      = DevDetector_Reset,
        .Fps        = __DevDetector_Fps,
        .Report     = DevDetector_Report,
};
