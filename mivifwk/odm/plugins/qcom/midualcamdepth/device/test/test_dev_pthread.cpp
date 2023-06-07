/**
 * @file        test_dev_pthread.cpp
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

static S64 m_pthread_Id;
static S32 testAdd = 0;
#define TEST_ADD_OK     (33)

static void* pthread_new_run(void *arg) {
    while (1) {
        testAdd++;
        m_Result |= Device->system->SleepMs(1);
//        DEV_LOGI("in new pthread=%ld", testAdd);
        if (testAdd == TEST_ADD_OK) {
            break;
        }
    }
//    DEV_LOGI("in new pthread exit");
    m_Result |= Device->system->SleepMs(10);
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

S32 test_dev_pthread(void) {
    m_Result = RET_OK;
    testAdd = 0;
    m_Result |= Device->pthread->Create(&m_pthread_Id, pthread_new_run, NULL, MARK("m_pthread_Id"));
    m_Result |= Device->pthread->Report();
    m_Result |= Device->pthread->Join(m_pthread_Id, NULL);
    if (testAdd != TEST_ADD_OK) {
        DEV_LOGE("PTHREAD TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    m_Result |= Device->pthread->Destroy(&m_pthread_Id);
    return m_Result;
}
