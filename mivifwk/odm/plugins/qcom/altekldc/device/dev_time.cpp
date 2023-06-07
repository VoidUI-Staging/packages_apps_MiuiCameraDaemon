/**
 * @file        dev_time.c
 * @brief
 * @details
 * @author
 * @date        2016.05.17
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#include <time.h>
#include "sys/time.h"
#include <stdio.h>
#include "dev_type.h"
#include "dev_log.h"
#include "dev_time.h"

static U8 init_f = FALSE;
S64 DevTime_GetTime(DEV_TIME_BUF* dateTime) {
    DEV_IF_LOGE_RETURN_RET((dateTime==NULL), RET_ERR, "ARG ERR");
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    struct tm *timenow = localtime((time_t*) &spec.tv_sec);
#if 0
    struct tm *timenow;
    time_t now;
    time(&now);
    timenow = localtime(&now);
#endif
    dateTime->year = timenow->tm_year + 1900;
    dateTime->month = timenow->tm_mon + 1;
    dateTime->day = timenow->tm_mday;
    dateTime->hour = timenow->tm_hour;
    dateTime->minute = timenow->tm_min;
    dateTime->second = timenow->tm_sec;
    dateTime->nsecond = spec.tv_nsec;
    dateTime->timezone = 0; //TODO    timenow->tm_zone
    return RET_OK;
}

S64 DevTime_SetTime(DEV_TIME_BUF* dateTime) {
    DEV_IF_LOGE_RETURN_RET((dateTime==NULL), RET_ERR, "ARG ERR");
    return RET_OK;
}

U64 DevTime_GetTimestampMs(void) {
    struct timeval start;
    gettimeofday(&start, 0);
    return (start.tv_sec * 1000) + (start.tv_usec / 1000);
}

S64 DevTime_GetTimeStr(char* string) {
    DEV_IF_LOGE_RETURN_RET((string == NULL), RET_ERR_ARG, "arg err");
    DEV_TIME_BUF dateTime;
    DevTime_GetTime(&dateTime);
    sprintf((char *) string, "%d-%d-%d-%d-%d-%d", dateTime.year, dateTime.month, dateTime.day, dateTime.hour, dateTime.minute, dateTime.second);
    return RET_OK;
}

S32 DevTime_Init(void) {
    init_f = TRUE;
    return RET_OK;
}

S32 DevTime_Deinit(void) {
    return RET_OK;
}

const Dev_Time m_dev_time = {
        .Init = DevTime_Init,
        .Deinit = DevTime_Deinit,
        .SetTime = DevTime_SetTime,
        .GetTime = DevTime_GetTime,
        .GetTimeStr = DevTime_GetTimeStr,
        .GetTimestampMs = DevTime_GetTimestampMs,
};
