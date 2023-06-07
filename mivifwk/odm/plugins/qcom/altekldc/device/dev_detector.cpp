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

#define DETECTOR_NUM_MAX         64
#define DETECTOR_NAME_LEN_MAX    128
static U32 init_f = FALSE;

typedef struct _DETECTOR_NODE {
    S64 *pId;
    char __fileName[FILE_NAME_LEN_MAX];
    char __tagName[FILE_NAME_LEN_MAX];
    U32 __fileLine;
    /* fps start*/
    char fpsStr[DETECTOR_NAME_LEN_MAX];
    U64 fps_start_time;
    U32 fps_add;
    U32 fps_run_flag;
    /*fps end*/
    /* run time start*/
    char consumeStr[DETECTOR_NAME_LEN_MAX];
    S32 run;
    U64 run_begin_time;
    U64 run_end_time;
    U64 run_all_time;
    U64 run_max_time;
    U64 run_min_time;
    U32 run_add;
    /* run time end*/
    U32 en;
} Detector_node;

static Detector_node m_detector_list[DETECTOR_NUM_MAX] = { {0}};

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
            m_detector_list[id].run_end_time = Device->time->GetTimestampMs();
            m_detector_list[id].run_all_time = 0;
            m_detector_list[id].run_add = 0;
            m_detector_list[id].run_min_time = 0xffffffffffffffff;
            m_detector_list[id].run_max_time = 0;
            //fps
            m_detector_list[id].fps_start_time = Device->time->GetTimestampMs();
            m_detector_list[id].fps_add = 0;
            m_detector_list[id].fps_run_flag = 0;
            m_id = id;
            *pId = m_id;
            m_detector_list[id].pId = pId;
            m_detector_list[id].en = TRUE;
            Dev_snprintf((char*) m_detector_list[id].__fileName, FILE_NAME_LEN_MAX, "%s", DevDetector_NoDirFileName(__fileName));
            Dev_snprintf((char*) m_detector_list[id].__tagName, FILE_NAME_LEN_MAX, "%s", tagName);
            m_detector_list[id].__fileLine = __fileLine;
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
    //fps
    m_detector_list[id].fps_start_time = Device->time->GetTimestampMs();
    m_detector_list[id].fps_add = 0;
    return RET_OK;
}

S64 DevDetector_Begin(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "DETECTOR Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DETECTOR_NUM_MAX) || (id <= 0)), RET_ERR, "DETECTOR id err[%ld]", id);
    DEV_IF_LOGE_RETURN_RET((m_detector_list[id].en != TRUE), RET_ERR, "DETECTOR Begin err[%ld]", id);
    m_detector_list[id].run_begin_time = Device->time->GetTimestampMs();
    m_detector_list[id].run = TRUE;
    return RET_OK;
}

S64 DevDetector_End(S64 id, const char *str) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "DETECTOR Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DETECTOR_NUM_MAX) || (id <= 0)), 0, "DETECTOR id err[%ld]", id);
    DEV_IF_LOGE_RETURN_RET((m_detector_list[id].en != TRUE), 0, "DETECTOR End err[%ld]", id);
    DEV_IF_LOGE_RETURN_RET((str == NULL), 0, "DETECTOR arg err");
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
    if ((Dev_strlen(str) <= DETECTOR_NAME_LEN_MAX) && (Dev_strlen(str) > 0)) {
        Dev_sprintf(m_detector_list[id].consumeStr, "%s", str);
    }
    DEV_LOGI("[%s] PERFORMANCE[MS]:[%.2f] AVG:[%.2f] MAX:[%.2f] MIN:[%.2f]", m_detector_list[id].consumeStr, currentTime,
            (double ) ((double ) (m_detector_list[id].run_all_time) / (double ) (m_detector_list[id].run_add)),
            (double ) m_detector_list[id].run_max_time, (double ) m_detector_list[id].run_min_time);
    if (m_detector_list[id].run_all_time >= 0xffffffffff000000) {
        DevDetector_Reset(id);
    }
    m_detector_list[id].run = FALSE;
    //fps
    if (m_detector_list[id].fps_run_flag == 0) {
        DevDetector_Fps(id, str);
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
                    , m_detector_list[i].__tagName
                    , m_detector_list[i].__fileName
                    , m_detector_list[i].__fileLine
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
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
    Dev_memset((void*)&m_detector_list, 0, sizeof(m_detector_list));
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
    Dev_memset((void*)&m_detector_list, 0, sizeof(m_detector_list));
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
