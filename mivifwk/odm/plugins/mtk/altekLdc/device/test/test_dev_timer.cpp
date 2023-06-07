/**
 * @file		test_dev_timer.cpp
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
#include "test.h"
#ifdef __TEST_DEV_TIMER__
#include "../device.h"

static S64 m_timer_Id;
static S64 a = 0;

static void timer_new_run(S64 id, void *arg)
{
    a++;
    DEV_LOGI("TIMER in new timer=[%" PRIi64 "][%" PRIi64 "]\n", id, a);
    if (a == 40) {
        Device->timer->Stop(m_timer_Id);
        DEV_LOGI("TIMER Test Stop \n");
    }
}

void test_dev_timer(void)
{
    DEV_LOGI("TIMER Test start \n");
    m_timer_Id = Device->timer->Create(timer_new_run, NULL);
    Device->timer->Start(m_timer_Id, TIMER_ONE_SECOND, 1);
    DEV_LOGI("TIMER Test start \n");
}

#endif // __TEST_DEV_TIMER__
