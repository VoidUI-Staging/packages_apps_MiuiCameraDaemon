/**
 * @file        test.c
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

#include "device.h"
#include "dev_type.h"
#include "dev_log.h"
#include "dev_test.h"
#include "settings.h"
#include "test/test.h"

Dev_TestNode __m_devTestsTable__[DEV_TEST_NUM_MAX] = { {0}};
S32 __devTestIndex__ = 0;

S32 DevTest_Report(void) {
    S32 m_Result = 0;
    if (Settings->SDK_PROJECT_RELEASE.value.bool_value != TRUE) {
        DEV_LOGI("DEVICE TEST REPORT START[------------------------------CASE:%d-------------------------]", __devTestIndex__);
        for (int i = 0; i < __devTestIndex__; i++) {
            if (__m_devTestsTable__[i].function != NULL) {
                m_Result |= __m_devTestsTable__[i].status;
                if (__m_devTestsTable__[i].status == RET_OK) {
                    DEV_LOGI("DEVICE TEST REPORT CASE [% 3d/%d] [%-25s] USE TIME[% 6dMS] [OK]", i + 1, __devTestIndex__, __m_devTestsTable__[i].name,
                            __m_devTestsTable__[i].useTime);
                } else {
                    DEV_LOGI("DEVICE TEST REPORT CASE [% 3d/%d] [%-25s] USE TIME[% 6dMS] [FAILURE!]", i + 1, __devTestIndex__,
                            __m_devTestsTable__[i].name, __m_devTestsTable__[i].useTime);
                }
            }
        }
        if (m_Result != 0) {
            DEV_LOGI("DEVICE TEST REPORT END  [--------------------FAILURE! FAILURE! FAILURE!---------------]");
        } else {
            DEV_LOGI("DEVICE TEST REPORT END  [-------------------------------OK!---------------------------]");
        }
    }
    return m_Result;
}

void DevTest_Init(void) {
    if (Settings->SDK_PROJECT_RELEASE.value.bool_value != TRUE) {
        __devTestIndex__ = 0;
        DevTest_Assembly();
        for (int i = 0; i < __devTestIndex__; i++) {
            if (__m_devTestsTable__[i].function != NULL) {
                DEV_LOGI("DEVICE TEST REPORT CASE SATRT %s", __m_devTestsTable__[i].name);
                S64 testStartTime = Device->time->GetTimestampMs();
                __m_devTestsTable__[i].status |= __m_devTestsTable__[i].function();
                DEV_LOGI("DEVICE TEST REPORT CASE END   %s", __m_devTestsTable__[i].name);
                __m_devTestsTable__[i].useTime = Device->time->GetTimestampMs() - testStartTime;
            }
        }
        if (__devTestIndex__ > 0) {
            DevTest_Report();
        }
        memset((void*)&__m_devTestsTable__, 0, sizeof(__m_devTestsTable__));
        __devTestIndex__ = 0;
    }
}

void DevTest_Handler(void) {
    if (Settings->SDK_PROJECT_RELEASE.value.bool_value != TRUE) {
    }
}

void DevTest_Deinit(void) {
    if (Settings->SDK_PROJECT_RELEASE.value.bool_value != TRUE) {
    }
}

const Dev_Test m_dev_test = {
        .Init       = DevTest_Init,
        .Handler    = DevTest_Handler,
        .Report     = DevTest_Report,
        .Deinit     = DevTest_Deinit,
};
