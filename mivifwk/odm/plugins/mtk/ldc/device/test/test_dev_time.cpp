/**
 * @file        test_dev_time.cpp
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

#include "test.h"

#ifdef __TEST_DEV_TIME__
#include "../device.h"

void test_dev_time(void)
{
    DEV_LOGI("Device->time->GetTimestamp=%" PRIu64, Device->time->GetTimestamp());
    char timeStr[128];
    Device->time->GetTimeStr(timeStr);
    DEV_LOGI("Device->time->GetTimeStr=%s", timeStr);
}

#endif //__TEST_DEV_TIME__
