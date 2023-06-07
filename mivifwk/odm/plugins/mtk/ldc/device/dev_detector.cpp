/**
 * @file		dev_detector.cpp
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

#include "dev_detector.h"

#include <stdlib.h>
#include <string.h>

#include "device.h"

//#define CONFIG_PTHREAD_SUPPORT       (1)
//#undef CONFIG_PTHREAD_SUPPORT

#ifdef CONFIG_PTHREAD_SUPPORT
#include <pthread.h>
static pthread_mutex_t m_mutex;
#endif

#define DETECTOR_REPORT_MAX   64
#define DETECTOR_NAME_LEN_MAX 128
static U32 init_f = FALSE;

typedef struct _DETECTOR_NODE
{
    /* fps start*/
    char fpsStr[DETECTOR_NAME_LEN_MAX];
    U64 fps_start_time;
    U32 fps_add;
    U32 fps_run_flag;
    /*fps end*/
    /* run time start*/
    char consumeStr[DETECTOR_NAME_LEN_MAX];
    U64 run_begin_time;
    U64 run_end_time;
    U64 run_all_time;
    U64 run_max_time;
    U64 run_min_time;
    U32 run_add;
    /* run time end*/
    U32 en;
} Detector_node;

static Detector_node m_detector_list[DETECTOR_REPORT_MAX];

S64 DevDetector_Create(void)
{
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_lock(&m_mutex);
#endif
    S64 m_id = 0;
    DEV_IF_LOGE_GOTO((init_f != TRUE), exit, "DETECTOR Init err");
    for (S64 id = 1; id < DETECTOR_REPORT_MAX; id++) {
        if (m_detector_list[id].en == FALSE) {
            m_detector_list[id].en = TRUE;
            // run time
            m_detector_list[id].run_begin_time = Device->time->GetTimestamp();
            m_detector_list[id].run_end_time = Device->time->GetTimestamp();
            m_detector_list[id].run_all_time = 0;
            m_detector_list[id].run_add = 0;
            m_detector_list[id].run_min_time = 0xffffffffffffffff;
            m_detector_list[id].run_max_time = 0;
            // fps
            m_detector_list[id].fps_start_time = Device->time->GetTimestamp();
            m_detector_list[id].fps_add = 0;
            m_detector_list[id].fps_run_flag = 0;
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

S64 DevDetector_Destroy(S64 *id)
{
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_lock(&m_mutex);
#endif
    if (init_f != TRUE) {
        goto exit;
    }
    DEV_IF_LOGE_GOTO((id == NULL), exit, "Detector id err");
    DEV_IF_LOGE_GOTO(((*id >= DETECTOR_REPORT_MAX) || (*id <= 0)), exit,
                     "DETECTOR id err%" PRIi64 "", *id);
    if (m_detector_list[*id].en != TRUE) {
        *id = 0;
        goto exit;
    }
    m_detector_list[*id].en = FALSE;
    m_detector_list[*id].run_add = 0;
    m_detector_list[*id].run_all_time = 0;
    m_detector_list[*id].fps_run_flag = 0;
    *id = 0;
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return RET_OK;
}

static S64 DevDetector_Fps(S64 id, const char *str)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "DETECTOR Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DETECTOR_REPORT_MAX) || (id <= 0)), RET_ERR, "DETECTOR id err");
    DEV_IF_LOGE_RETURN_RET((m_detector_list[id].en != TRUE), RET_ERR, "DETECTOR Fps err");
    DEV_IF_LOGE_RETURN_RET((str == NULL), 0, "DETECTOR arg err");
    if (m_detector_list[id].fps_add == 0) {
        m_detector_list[id].fps_start_time = Device->time->GetTimestamp();
    }
    m_detector_list[id].fps_add++;
    if ((strlen(str) <= DETECTOR_NAME_LEN_MAX) && (strlen(str) > 0)) {
        sprintf(m_detector_list[id].fpsStr, "%s", str);
    }
    if (m_detector_list[id].fps_add == 60) {
        DEV_LOGI("[%s]FPS: %f ", m_detector_list[id].fpsStr,
                 (double)(((double)(m_detector_list[id].fps_add) * 1000) /
                          ((double)((double)Device->time->GetTimestamp() -
                                    (double)m_detector_list[id].fps_start_time))));
        m_detector_list[id].fps_add = 0;
    }
    return RET_OK;
}

S64 __DevDetector_Fps(S64 id, const char *str)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "DETECTOR Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DETECTOR_REPORT_MAX) || (id <= 0)), RET_ERR, "DETECTOR id err");
    DEV_IF_LOGE_RETURN_RET((m_detector_list[id].en != TRUE), RET_ERR, "DETECTOR Fps err");
    DEV_IF_LOGE_RETURN_RET((str == NULL), 0, "DETECTOR arg err");
    m_detector_list[id].fps_run_flag = 1;
    return DevDetector_Fps(id, str);
}

S64 DevDetector_Reset(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "DETECTOR Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DETECTOR_REPORT_MAX) || (id <= 0)), RET_ERR, "DETECTOR id err");
    DEV_IF_LOGE_RETURN_RET((m_detector_list[id].en != TRUE), RET_ERR, "DETECTOR Reset err");
    m_detector_list[id].run_all_time = 0;
    m_detector_list[id].run_add = 0;
    // fps
    m_detector_list[id].fps_start_time = Device->time->GetTimestamp();
    m_detector_list[id].fps_add = 0;
    return RET_OK;
}

S64 DevDetector_Begin(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "DETECTOR Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DETECTOR_REPORT_MAX) || (id <= 0)), RET_ERR, "DETECTOR id err");
    DEV_IF_LOGE_RETURN_RET((m_detector_list[id].en != TRUE), RET_ERR, "DETECTOR Begin err");
    m_detector_list[id].run_begin_time = Device->time->GetTimestamp();
    return RET_OK;
}

S64 DevDetector_End(S64 id, const char *str)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "DETECTOR Init err");
    DEV_IF_LOGE_RETURN_RET(((id >= DETECTOR_REPORT_MAX) || (id <= 0)), RET_ERR, "DETECTOR id err");
    DEV_IF_LOGE_RETURN_RET((m_detector_list[id].en != TRUE), RET_ERR, "DETECTOR End err");
    DEV_IF_LOGE_RETURN_RET((str == NULL), 0, "DETECTOR arg err");
    // run time
    m_detector_list[id].run_end_time = Device->time->GetTimestamp();
    m_detector_list[id].run_all_time +=
        m_detector_list[id].run_end_time - m_detector_list[id].run_begin_time;
    m_detector_list[id].run_add++;
    double currentTime =
        (double)(m_detector_list[id].run_end_time - m_detector_list[id].run_begin_time);
    if (m_detector_list[id].run_max_time < currentTime) {
        m_detector_list[id].run_max_time = (U64)currentTime;
    }
    if (m_detector_list[id].run_min_time > currentTime) {
        m_detector_list[id].run_min_time = (U64)currentTime;
    }
    if ((strlen(str) <= DETECTOR_NAME_LEN_MAX) && (strlen(str) > 0)) {
        sprintf(m_detector_list[id].consumeStr, "%s", str);
    }
    DEV_LOGI("[%s] TIME PERFORMANCE ONCE: [%f MS] AVERAGE: [%f MS] MAX: [%f MS] MIN: [%f MS]",
             m_detector_list[id].consumeStr, currentTime,
             (double)((double)(m_detector_list[id].run_all_time) /
                      (double)(m_detector_list[id].run_add)),
             (double)m_detector_list[id].run_max_time, (double)m_detector_list[id].run_min_time);
    if (m_detector_list[id].run_all_time >= 0xffffffffff000000) {
        DevDetector_Reset(id);
    }
    // fps
    if (m_detector_list[id].fps_run_flag == 0) {
        DevDetector_Fps(id, str);
    }
    return RET_OK;
}

S64 DevDetector_Init(void)
{
    if (init_f == TRUE) {
        return RET_OK;
    }
#ifdef CONFIG_PTHREAD_SUPPORT
    S32 ret = pthread_mutex_init(&m_mutex, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
#endif
    memset((void *)&m_detector_list, 0, sizeof(m_detector_list));
    init_f = TRUE;
    return RET_OK;
}

S64 DevDetector_Uninit(void)
{
    if (init_f != TRUE) {
        return RET_OK;
    }
    for (S64 id = 1; id < DETECTOR_REPORT_MAX; id++) {
        S64 idp = id;
        DevDetector_Destroy(&idp);
    }
    memset((void *)&m_detector_list, 0, sizeof(m_detector_list));
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_destroy(&m_mutex);
#endif
    init_f = FALSE;
    return RET_OK;
}

const Dev_Detector m_dev_detector = {
    .Init = DevDetector_Init,
    .Uninit = DevDetector_Uninit,
    .Create = DevDetector_Create,
    .Destroy = DevDetector_Destroy,
    .Begin = DevDetector_Begin,
    .End = DevDetector_End,
    .Reset = DevDetector_Reset,
    .Fps = __DevDetector_Fps,
};
