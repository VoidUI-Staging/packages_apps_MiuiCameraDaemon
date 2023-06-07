/**
 * @file		dev_time.h
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

#ifndef __DEV_TIME_H__
#define __DEV_TIME_H__
#include "dev_type.h"

typedef struct __DevTimeBuf
{
    S32 year;
    S32 month;
    S32 day;
    S32 hour;
    S32 minute;
    S32 second;
    S32 timezone;
} DEV_TIME_BUF;

typedef struct __Dev_Time Dev_Time;
struct __Dev_Time
{
    S64 (*SetTime)(DEV_TIME_BUF *dateTime);
    S64 (*GetTime)(DEV_TIME_BUF *dateTime);
    S64 (*GetTimeStr)(char *string);
    U64 (*GetTimestamp)(void);
};

extern const Dev_Time m_dev_time;

#endif // __DEV_TIME_H__
