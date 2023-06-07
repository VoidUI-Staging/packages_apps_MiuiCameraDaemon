////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camlog.h
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMLOG_H
#define CAMLOG_H

#include <ctype.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef __cplusplus

typedef char CHAR;
typedef uint32_t UINT32;
typedef int BOOL;
typedef void VOID;
typedef int32_t INT32;

#define CAMLOG_VISIBILITY_PUBLIC __attribute__((visibility("default")))

/// @brief Time structure
struct CameraTime
{
    UINT32 seconds;     ///< Time in whole seconds
    UINT32 nanoSeconds; ///< Fractional nanoseconds
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIFLUSHLOG
///
/// @brief  Fuction pointer to let CHI flush the offline log into android file system
///
/// @return VOID
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum class CamlogOfflineLoggerType {
    ASCII,      ///< Offlinelogger type for ASCII
    BINARY,     ///< Offlinelogger type for Binary
    NUM_OF_TYPE ///< Calculate total number of offlinelogger type, do not remove it
};

typedef VOID (*CAMLOGPFNCHIFLUSHLOG)(CamlogOfflineLoggerType type, BOOL lastFlush);
class camlog
{
public:
    CAMLOG_VISIBILITY_PUBLIC static int send_message_to_mqs(CHAR *buf, BOOL isForceSIGABRT);
    CAMLOG_VISIBILITY_PUBLIC static void setQCOMOfflinelog(CamlogOfflineLoggerType type,
                                                           CAMLOGPFNCHIFLUSHLOG offlineflush);
    CAMLOG_VISIBILITY_PUBLIC static INT32 GetCurrentTemperature();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTime
    ///
    /// @brief  Gets the current time
    ///
    /// @param  pTime Output time structure
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetTime(CameraTime *pTime);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TimeToMillis
    ///
    /// @brief  Translates CamxTime to time in milliseconds
    ///
    /// @param  pTime   Pointer to CamxTime structure
    ///
    /// @return Return milliseconds on success or 0 on error
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 TimeToMillis(CameraTime *pTime);
};

#endif // __cplusplus
#endif // CAMXOSUTILS_H
