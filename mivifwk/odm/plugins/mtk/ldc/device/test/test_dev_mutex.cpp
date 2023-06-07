/**
 * @file		test_dev_mutex.cpp
 * @brief
 * @details
 * @author
 * @date		2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par			History:
 *
 */
#include "test.h"
#ifdef __TEST_DEV_MUTEX__
#include "../device.h"

#define USE_MUTEX_LOCK 1
static S64 m_pthread_Id1;
static S64 m_pthread_Id2;
static S64 m_mutex_Id;
static S64 add = 0;

void run_mutex(S64 id)
{
#if (USE_MUTEX_LOCK == 1)
    Device->mutex->Lock(m_mutex_Id);
#endif
    if (add != 0) {
        DEV_LOGE("MUTEX LOCK TEST ERR!!!!!!!!!!!!!!!!!");
    }
    add++;
    Device->system->SleepMs(1);
    add--;
#if (USE_MUTEX_LOCK == 1)
    Device->mutex->Unlock(m_mutex_Id);
#endif
}

static void *pthread_new_run1(void *arg)
{
    int a = 0;
    while (1) {
        a++;
        run_mutex(m_pthread_Id1);
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

static void *pthread_new_run2(void *arg)
{
    int a = 0;
    while (1) {
        a++;
        run_mutex(m_pthread_Id2);
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

void test_dev_mutex(void)
{
    DEV_LOGI("mutex Test start \n");
    m_mutex_Id = Device->mutex->Create();
    m_pthread_Id1 = Device->pthread->Create(pthread_new_run1, NULL);
    m_pthread_Id2 = Device->pthread->Create(pthread_new_run2, NULL);
}

#endif // __TEST_DEV_MUTEX__
