/**
 * @file        test_dev_system.c
 * @brief
 * @details
 * @author
 * @date        2021.06.22
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#include "../device.h"

static S64 m_Result = 0;

S32 test_dev_system(void) {
    m_Result = RET_OK;
    Device->system->SleepMs(1);
    Device->system->SleepUs(1);
    return m_Result;
}
