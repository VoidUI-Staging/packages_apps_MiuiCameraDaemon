////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camlog.cpp
/// @brief General OS specific utility class implementation for Linux
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camlog.h"

#include <android/log.h>       // Logcat logging functions
#include <cutils/properties.h> // Android properties
#include <dlfcn.h>             // dynamic linking
#include <endian.h>
#include <errno.h> // errno
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h> // posix_memalign, free
#include <string.h> // strlcat
#include <sync/sync.h>
#include <sys/prctl.h>

#include "log/log.h"

static const UINT32 MaxStringLength256 = 256;
static CamlogOfflineLoggerType m_offlineLoggertype = CamlogOfflineLoggerType::NUM_OF_TYPE;
static CAMLOGPFNCHIFLUSHLOG m_offlineLoggerflush = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// camlog::GetTime
///
/// @brief  Gets the current time
///
/// @param  pTime Output time structure
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID camlog::GetTime(CameraTime *pTime)
{
    timespec time = {0};

    clock_gettime(CLOCK_BOOTTIME, &time);
    if (NULL != pTime) {
        pTime->seconds = static_cast<UINT32>(time.tv_sec);
        pTime->nanoSeconds = static_cast<UINT32>(time.tv_nsec);
    }
}

INT32 camlog::GetCurrentTemperature()
{
    INT32 readSize = 0;
    char readBuffer[16] = {0};
    INT32 currentTemp = 0;
    INT32 thermalFd = open("/sys/class/thermal/thermal_message/board_sensor_temp", O_RDONLY);

    if (thermalFd >= 0) {
        memset(&readBuffer[0], 0, 16);
        readSize = read(thermalFd, &readBuffer[0], 7);

        if (readSize != 0) {
            readBuffer[readSize + 1] = 0;
            currentTemp = atoi(&readBuffer[0]);
            currentTemp = currentTemp / 1000;
        }
        close(thermalFd);
    }

    return currentTemp;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// camlog::TimeToMillis
///
/// @brief  Gets the current time
///
/// @param  pTime Output time structure
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 camlog::TimeToMillis(CameraTime *pTime)
{
    return (pTime == NULL) ? 0 : ((1000 * pTime->seconds) + ((pTime->nanoSeconds) / 1000000L));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// camlog::send_camerr_toqms
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int camlog::send_message_to_mqs(CHAR *buf, BOOL isForceSIGABRT)
{
    int ret = 0;
    int fd = 0;
    CameraTime eTime = {0};
    static UINT32 lastTime = 0;
    UINT32 nowTime = 0;
    INT32 currentTemp = 0;
    char cMqsBuf[MaxStringLength256] = {0};

    GetTime(&eTime);
    nowTime = TimeToMillis(&eTime);
    // first time or interval time longer than 30s
    if (0 != lastTime && (fabs(nowTime - lastTime) < 30000)) {
        return ret;
    }

    if (NULL == buf) {
        return ret;
    }

    CHAR *substr = strstr(buf, "[Power]");
    if (substr != NULL) {
        strlcpy(cMqsBuf, buf, MaxStringLength256);
    } else {
        sprintf(cMqsBuf, "[Stability] %s", buf);
    }

    if (m_offlineLoggerflush != NULL) {
        m_offlineLoggerflush(m_offlineLoggertype, 0);
    }

    fd = open("/dev/camlog", O_RDWR);
    if (fd >= 0) {
        lastTime = nowTime;
        ret = write(fd, cMqsBuf, strlen(cMqsBuf));
        close(fd);
    }

    if (isForceSIGABRT) {
        raise(SIGABRT);
    }
    return ret;
}

void camlog::setQCOMOfflinelog(CamlogOfflineLoggerType type, CAMLOGPFNCHIFLUSHLOG offlineflush)
{
    m_offlineLoggertype = type;
    m_offlineLoggerflush = offlineflush;
}
