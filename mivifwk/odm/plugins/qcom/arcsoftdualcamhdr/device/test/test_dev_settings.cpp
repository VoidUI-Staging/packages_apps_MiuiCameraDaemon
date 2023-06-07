/**
 * @file        test_dev_settings.c
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
#include "settings.h"

static S32 m_Result = 0;

S32 test_dev_settings(void) {
    m_Result = 0;
    m_Result |= Device->settings->Report();
    if (Settings->SDK_PROJECT_TEST.value.int_value != 0x8992) {
        DEV_LOGI("SETTINGS TEST name  =%s", Settings->SDK_PROJECT_TEST.name);
        DEV_LOGI("SETTINGS TEST value =%d", Settings->SDK_PROJECT_TEST.value.int_value);
        m_Result |= RET_ERR;
    }
    return m_Result;
}
