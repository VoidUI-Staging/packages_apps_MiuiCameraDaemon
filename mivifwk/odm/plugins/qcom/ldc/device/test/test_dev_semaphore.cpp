/**
 * @file       test_dev_semaphore.cpp
 * @brief
 * @details
 * @author
 * @date       2019.07.03
 * @version    0.1
 * @note
 * @warning
 * @par        History:
 *
 */
#include "../device.h"

static S32 m_Result = 0;

#define TEST_MAX_TIMES      99

static S64 m_pthread_Id1;
static S64 m_pthread_Id2;
static S64 m_pthread_Id3;
static S64 m_semaphore_Id = 0;
static S64 m_semaphore_Id2 = 0;

static void* pthread_new_run3(void *arg) {
    m_Result |= Device->semaphore->TimedWait(m_semaphore_Id2, ((TEST_MAX_TIMES / 10) + 50));
    if (m_Result != RET_OK) {
        DEV_LOGE("SEMAPHORE TEST ERR!!!!!!!!!!!!!!!!!%d", m_Result);
        m_Result |= RET_ERR;
    }
    m_Result |= Device->system->SleepMs(1);
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run1(void *arg) {
    int add = 0;
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        m_Result |= Device->semaphore->Wait(m_semaphore_Id);
        add++;
    }
    if (add != TEST_MAX_TIMES) {
        DEV_LOGE("SEMAPHORE TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    m_Result |= Device->semaphore->Post(m_semaphore_Id2);
    m_Result |= Device->system->SleepMs(1);
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run2(void *arg) {
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
//        m_Result |= Device->system->SleepUs(1);
        m_Result |= Device->semaphore->Post(m_semaphore_Id);
    }
    m_Result |= Device->system->SleepMs(1);
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

S32 test_dev_semaphore(void) {
    m_Result = RET_OK;
    m_Result |= Device->semaphore->Create(&m_semaphore_Id, MARK("m_semaphore_Id"));
    m_Result |= Device->semaphore->Create(&m_semaphore_Id2, MARK("m_semaphore_Id"));
    m_Result |= Device->semaphore->Report();
    m_Result |= Device->pthread->Create(&m_pthread_Id1, pthread_new_run1, NULL, MARK("m_pthread_Id1"));
    m_Result |= Device->pthread->Create(&m_pthread_Id2, pthread_new_run2, NULL, MARK("m_pthread_Id2"));
    m_Result |= Device->pthread->Create(&m_pthread_Id3, pthread_new_run3, NULL, MARK("m_pthread_Id3"));
    m_Result |= Device->pthread->Join(m_pthread_Id1, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id2, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id3, NULL);
    m_Result |= Device->semaphore->Destroy(&m_semaphore_Id);
    m_Result |= Device->semaphore->Destroy(&m_semaphore_Id2);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id1);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id2);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id3);
    return m_Result;
}
