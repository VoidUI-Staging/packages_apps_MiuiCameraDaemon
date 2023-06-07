/**
 * @file        test_dev_time.c
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

#define TEST_START_TEMPSTAMP_MS (1624355234594)  //当前时间戳  2021-06-22 17:47:14
#define TEST_START_YEAR         (2021)
#define TEST_START_MONTH        (06)
#define TEST_START_DAY          (22)

S32 test_dev_time(void) {
    m_Result = RET_OK;
    DEV_TIME_BUF time;
    time.year = 2017;
    time.month = 7;
    time.day = 5;
    time.hour = 12;
    time.minute = 0;
    time.second = 0;
    time.timezone = 8;
    m_Result |= Device->time->SetTime(&time);
//    DEV_LOGI("SetTime:%ld.%02d.%02d %02d:%02d:%02d zone=%ld", time.year, time.month, time.day, time.hour, time.minute, time.second, time.timezone);
    char timerstr[128];
    m_Result |= Device->time->GetTimeStr(timerstr);
//    DEV_LOGI("TimeStr:[%s][%ld]", timerstr);
    DEV_TIME_BUF time_temp;
    U64 temp = Device->time->GetTimestampMs();
//    DEV_LOGI("TimestampMs=%ld", temp);
    m_Result |= Device->time->GetTime(&time_temp);
    if (temp <= TEST_START_TEMPSTAMP_MS) {
        DEV_LOGE("TIME TEST ERR!!!!!!!!!!!!!!!!![%ld:%ld]", temp, (U64)TEST_START_TEMPSTAMP_MS);
        m_Result |= RET_ERR;
    }
    if ((time_temp.year * 365) + (time_temp.month * 30) + (time_temp.day) < ((TEST_START_YEAR * 365) + (TEST_START_MONTH * 30) + (TEST_START_DAY))) {
        DEV_LOGI("GetTime:%i.%i.%i %i:%i:%i zone=%i", (time_temp.year), time_temp.month, time_temp.day, time_temp.hour, time_temp.minute,
                time_temp.second, time_temp.timezone);
        DEV_LOGE("TIME TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    return m_Result;
}
