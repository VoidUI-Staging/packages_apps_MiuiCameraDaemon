/**
 * @file        test_dev_timer.c
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
#include "../device.h"

static S32 m_Result = 0;

#define TEST_MAX_TIMES      50

static S64 m_timer_Id1;
static S64 a1 = 0;

static S64 m_timer_Id2;
static S64 a2 = 0;

static S64 m_pthread_Id1;
static S64 m_semaphore_Id = 0;
static S64 m_timerStartTime = 0;

#define TIMER1_INTERVAL (1 * TIMER_ONE_SECOND / 100)
#define TIMER2_INTERVAL (3 * TIMER_ONE_SECOND / 100)

static void* pthread_new_run1(void *arg) {
    m_Result |= Device->semaphore->TimedWait(m_semaphore_Id, ((TIMER1_INTERVAL * TEST_MAX_TIMES) + (TIMER2_INTERVAL * TEST_MAX_TIMES)) + 300);
    if ((Device->time->GetTimestampMs() - m_timerStartTime) <= ((TIMER1_INTERVAL * TEST_MAX_TIMES) + (TIMER2_INTERVAL * TEST_MAX_TIMES))) {
        DEV_LOGE("TIMER TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    m_timerStartTime = 0;
    m_Result |= Device->system->SleepMs(1);
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void timer_new_run2(S64 id, void *arg) {
    a2++;
//    DEV_LOGI("TIMER in new timer=[%0.f][%0.f]", (double )id, (double )a2);
    if (a2 == TEST_MAX_TIMES) {
        m_Result |= Device->timer->Stop(m_timer_Id2);
        a2 = 0;
        m_Result |= Device->semaphore->Post(m_semaphore_Id);
        m_Result |= Device->timer->Stop_All();
//        m_Result |= Device->timer->Start_All();
    }
}

static void timer_new_run1(S64 id, void *arg) {
    a1++;
//    DEV_LOGI("TIMER in new timer=[%0.f][%0.f]", (double )id, (double )a1);
    if (a1 == TEST_MAX_TIMES) {
        m_Result |= Device->timer->Stop(m_timer_Id1);
        m_Result |= Device->timer->Start(m_timer_Id2, TIMER2_INTERVAL, 1);
        a1 = 0;
    }
}

S32 test_dev_timer(void) {
    m_Result = RET_OK;
    m_Result |= Device->semaphore->Create(&m_semaphore_Id, MARK("m_semaphore_Id"));
    m_Result |= Device->timer->Create(&m_timer_Id1, timer_new_run1, NULL, MARK("m_timer_Id1"));
    m_Result |= Device->timer->Create(&m_timer_Id2, timer_new_run2, NULL, MARK("m_timer_Id2"));
    m_Result |= Device->timer->Report();
    m_timerStartTime = Device->time->GetTimestampMs();
    m_Result |= Device->timer->Start(m_timer_Id1, TIMER1_INTERVAL, 1);
//    m_Result |= Device->timer->Report();
    m_Result |= Device->pthread->Create(&m_pthread_Id1, pthread_new_run1, NULL, MARK("m_pthread_Id1"));
    m_Result |= Device->pthread->Join(m_pthread_Id1, NULL);
    m_Result |= Device->semaphore->Destroy(&m_semaphore_Id);
    m_Result |= Device->timer->Destroy(&m_timer_Id1);
    m_Result |= Device->timer->Destroy(&m_timer_Id2);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id1);
    return m_Result;
}

