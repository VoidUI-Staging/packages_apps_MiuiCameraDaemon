/**
 * @file        test_dev_mutex.cpp
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */
#include "../device.h"

static S32 m_Result = 0;

#define USE_MUTEX_LOCK  1

#define TEST_MAX_TIMES      500

static S64 m_pthread_Id1;
static S64 m_pthread_Id2;
static S64 m_mutex_Id;
static S64 add = 0;

void run_mutex(S64 id) {
#if (USE_MUTEX_LOCK==1)
    m_Result |= Device->mutex->Lock(m_mutex_Id);
#endif
    if (add != 0) {
        DEV_LOGE("MUTEX LOCK TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    add++;
    m_Result |= Device->system->SleepUs(100);
    add--;
#if (USE_MUTEX_LOCK==1)
    m_Result |= Device->mutex->Unlock(m_mutex_Id);
#endif
}

static void *pthread_new_run1(void *arg) {
    int a = 0;
    while (1) {
        a++;
        run_mutex(m_pthread_Id1);
        if (a > TEST_MAX_TIMES) {
            break;
        }
        if (m_Result != RET_OK) {
            m_Result |= Device->pthread->Exit(0);
        }
    }
//    DEV_LOGI("pthread 1 exit");
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void *pthread_new_run2(void *arg) {
    int a = 0;
    while (1) {
        a++;
        run_mutex(m_pthread_Id2);
        if (a > TEST_MAX_TIMES) {
            break;
        }
        if (m_Result != RET_OK) {
            m_Result |= Device->pthread->Exit(0);
        }
    }
//    DEV_LOGI("pthread 2 exit");
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

S32 test_dev_mutex(void) {
    m_Result = RET_OK;
    m_Result |= Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
//    m_Result |= Device->mutex->Report();
    m_Result |= Device->pthread->Create(&m_pthread_Id1, pthread_new_run1, NULL, MARK("m_pthread_Id1"));
    m_Result |= Device->pthread->Create(&m_pthread_Id2, pthread_new_run2, NULL, MARK("m_pthread_Id2"));
    m_Result |= Device->pthread->Join(m_pthread_Id1, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id2, NULL);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id1);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id2);
    m_Result |= Device->mutex->Destroy(&m_mutex_Id);
    return m_Result;
}
