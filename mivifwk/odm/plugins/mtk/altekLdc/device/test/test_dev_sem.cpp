/**
 * @file		test_dev_sem.cpp
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
#ifdef __TEST_DEV_SEM__
#include "../device.h"

static S64 m_pthread_Id1;
static S64 m_pthread_Id2;
static S64 m_sem_Id;
static S64 add = 0;

static void *pthread_new_run1(void *arg)
{
    int a = 0;
    while (1) {
        Device->sem->Wait(m_sem_Id);
        add++;
        a++;
        if (a > 1000) {
            break;
        }
    }
    Device->system->SleepMs(10);
    DEV_LOGI("in new pthread 1 exit\n");
    Device->system->SleepMs(10);
    Device->pthread->Exit(0);
    return NULL;
}

static void *pthread_new_run2(void *arg)
{
    int a = 0;
    while (1) {
        Device->system->SleepUs(10);
        Device->sem->Post(m_sem_Id);
        a++;
        if (a > 1000) {
            break;
        }
    }
    Device->system->SleepMs(10);
    if (add == 1001) {
        DEV_LOGI("Semaphore test OK");
    } else {
        DEV_LOGE("Semaphore test ERR!!!!!!!!!!!!");
    }
    DEV_LOGI("in new pthread 2 exit\n");
    Device->system->SleepMs(10);
    Device->pthread->Exit(0);
    return NULL;
}

void test_dev_sem(void)
{
    DEV_LOGI("Semaphore Test start \n");
    m_pthread_Id1 = Device->pthread->Create(pthread_new_run1, NULL);
    m_pthread_Id2 = Device->pthread->Create(pthread_new_run2, NULL);
    m_sem_Id = Device->sem->Create();
}

#endif // __TEST_DEV_SEM__
