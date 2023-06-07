/**
 * @file		test_dev_create.cpp
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
#ifdef __TEST_DEV_CREATE__
#include "../device.h"

//#define USE_MUTEX_LOCK  1

#define TEST_MAX_TIMES 10000

static S64 m_mutex_Id = 0;
static S64 m_pthread_Id1;
static S64 m_pthread_Id2;
static S64 m_create_Id1 = 0;
static S64 m_create_Id2 = 0;

static void test_CallBack(S64 id, void *arg) {}

void run_Create1()
{
#if (USE_MUTEX_LOCK == 1)
    Device->mutex->Lock(m_mutex_Id);
#endif
    m_create_Id1 = Device->sem->Create(/*MSG_TYPE_DATA,64,test_CallBack, NULL*/);
    DEV_IF_LOGE((m_create_Id1 == m_create_Id2),
                "CREATE ID LOCK TEST ERR!!!!!!!!!!!!!!!!!%" PRIi64 "", m_create_Id1);
    Device->system->SleepMs(1);
    Device->sem->Destroy(&m_create_Id1);
#if (USE_MUTEX_LOCK == 1)
    Device->mutex->Unlock(m_mutex_Id);
#endif
}

void run_Create2()
{
#if (USE_MUTEX_LOCK == 1)
    Device->mutex->Lock(m_mutex_Id);
#endif
    m_create_Id2 = Device->sem->Create(/*MSG_TYPE_DATA,64,test_CallBack, NULL*/);
    DEV_IF_LOGE((m_create_Id1 == m_create_Id2),
                "CREATE ID LOCK TEST ERR!!!!!!!!!!!!!!!!!%" PRIi64 "", m_create_Id2);
    Device->system->SleepMs(3);
    Device->sem->Destroy(&m_create_Id2);
#if (USE_MUTEX_LOCK == 1)
    Device->mutex->Unlock(m_mutex_Id);
#endif
}

static void *pthread_new_run1(void *arg)
{
    int a = 0;
    while (1) {
        a++;
        Device->system->SleepMs(2);
        //        DEV_LOGI("in pthread run 1");
        run_Create1();
        if (a > TEST_MAX_TIMES) {
            break;
        }
    }
    DEV_LOGI("in new pthread exit");
    Device->system->SleepMs(10);
    Device->pthread->Exit(0);
    return NULL;
}

static void *pthread_new_run2(void *arg)
{
    int a = 0;
    while (1) {
        a++;
        Device->system->SleepMs(5);
        //        DEV_LOGI("in pthread run 2");
        run_Create2();
        if (a > TEST_MAX_TIMES) {
            break;
        }
    }
    DEV_LOGI("in new pthread exit");
    Device->system->SleepMs(10);
    Device->pthread->Exit(0);
    return NULL;
}

void test_dev_create(void)
{
    DEV_LOGI("create Test start");
    m_mutex_Id = Device->mutex->Create();
    m_pthread_Id1 = Device->pthread->Create(pthread_new_run1, NULL);
    m_pthread_Id2 = Device->pthread->Create(pthread_new_run2, NULL);
    DEV_LOGI("create Test start m_mutex_Id=%" PRIi64 "", m_mutex_Id);
}

#endif // __TEST_DEV_CREATE__
