/**
 * @file		dev_time.cpp
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par			History:
 *
 */

#include "dev_time.h"

#include <stdio.h>
#include <time.h>

#include "dev_log.h"
#include "dev_type.h"
#include "sys/time.h"

S64 DevTime_GetTime(DEV_TIME_BUF *dateTime)
{
    struct tm *timenow;
    time_t now;
    time(&now);
    timenow = localtime(&now);
    dateTime->year = timenow->tm_year + 1900;
    dateTime->month = timenow->tm_mon + 1;
    dateTime->day = timenow->tm_mday;
    dateTime->hour = timenow->tm_hour;
    dateTime->minute = timenow->tm_min;
    dateTime->second = timenow->tm_sec;
    dateTime->timezone = 0; // TODO	timenow->tm_zone
    return RET_OK;
}

S64 DevTime_SetTime(DEV_TIME_BUF *dateTime)
{
    // TODO
    return RET_ERR;
}

U64 DevTime_GetTimestamp(void)
{
    struct timeval start;
    gettimeofday(&start, 0);
    return (start.tv_sec * 1000) + (start.tv_usec / 1000);
}

S64 DevTime_GetTimeStr(char *string)
{
    DEV_IF_LOGE_RETURN_RET((string == NULL), RET_ERR_ARG, "arg err");
    DEV_TIME_BUF dateTime;
    DevTime_GetTime(&dateTime);
    sprintf((char *)string, "%d-%d-%d-%d-%d-%d", dateTime.year, dateTime.month, dateTime.day,
            dateTime.hour, dateTime.minute, dateTime.second);
    return RET_OK;
}

const Dev_Time m_dev_time = {
    .SetTime = DevTime_SetTime,
    .GetTime = DevTime_GetTime,
    .GetTimeStr = DevTime_GetTimeStr,
    .GetTimestamp = DevTime_GetTimestamp,
};
