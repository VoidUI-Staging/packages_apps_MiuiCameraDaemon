/**
 * @file        test_dev_detector.cpp
 * @brief
 * @details
 * @author
 * @date        2021.06.03
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */
#include "../device.h"

static S32 m_Result = 0;

#define TEST_MAX_TIMES      5

static S64 m_pthread_Id;
static S64 g_detectorTestId_1 = 0;
static S64 g_detectorTestId_2 = 0;

static void* pthread_new_run(void *arg) {
    for (int w = 0; w < TEST_MAX_TIMES * 5; w++) {
        S64 detector = 0;
        m_Result |= Device->detector->Begin(g_detectorTestId_1);
        m_Result |= Device->system->SleepMs(10);
        detector = Device->detector->End(g_detectorTestId_1, "detector TEST1-->");
        DEV_LOGI("detector TEST1--> %ld MS",detector);
        m_Result |= Device->detector->Begin(g_detectorTestId_2);
        m_Result |= Device->system->SleepMs(13);
        detector = Device->detector->End(g_detectorTestId_2, "detector TEST2-->");
        DEV_LOGI("detector TEST2--> %ld MS",detector);
    }
    m_Result |= Device->system->SleepMs(1);
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

S32 test_dev_detector(void) {
    m_Result = 0;
    m_Result |= Device->detector->Create(&g_detectorTestId_1, MARK("g_detectorTestId_1"));
    m_Result |= Device->detector->Create(&g_detectorTestId_2, MARK("g_detectorTestId_2"));
//    m_Result |= Device->detector->Report();
    m_Result |= Device->pthread->Create(&m_pthread_Id, pthread_new_run, NULL, MARK("m_pthread_Id"));
    m_Result |= Device->pthread->Join(m_pthread_Id, NULL);
    m_Result |= Device->detector->Destroy(&g_detectorTestId_1);
    m_Result |= Device->detector->Destroy(&g_detectorTestId_2);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id);
    return m_Result;
}
