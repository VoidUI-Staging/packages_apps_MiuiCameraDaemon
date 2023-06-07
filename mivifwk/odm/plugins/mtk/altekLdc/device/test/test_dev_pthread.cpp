/**
 * @file		test_dev_pthread.cpp
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par			History:
 *
 */
#include "test.h"
#ifdef __TEST_DEV_PTHREAD__
#include "../device.h"

static S64 m_pthread_Id;

static void *pthread_new_run(void *arg)
{
    int a = 0;
    while (1) {
        a++;
        Device->system->SleepMs(1);
        DEV_LOGI("in new pthread=%d\n", a);
        if (a > 1000) {
            break;
        }
    }
    Device->system->SleepMs(10);
    DEV_LOGI("in new pthread exit\n");
    Device->system->SleepMs(10);
    Device->pthread->Exit(0);
    return NULL;
}

void test_dev_pthread(void)
{
    DEV_LOGI("PTHREAD Test start \n");
    m_pthread_Id = Device->pthread->Create(pthread_new_run, NULL);
    DEV_LOGI("PTHREAD Test start \n");
}

#endif // __TEST_DEV_PTHREAD__
