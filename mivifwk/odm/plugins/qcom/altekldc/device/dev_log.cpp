/**
 * @file        dev_log.c
 * @brief
 * @details
 * @author
 * @date        2020.02.17
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#include <log/log.h>
#include "dev_type.h"
#include "dev_stdlib.h"
#include "device.h"
#include "dev_log.h"
#include <stdarg.h>

#if defined(MIVIFWK_CDK_OS)  //MIMV
#include "MiDebugUtils.h"
#endif

#if defined(WINDOWS_OS)
#include "windows.h"
#elif defined(LINUX_OS)
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

U32 m_DEV_LOG_level = DEV_LOG_LEVEL_INFO;

U32 m_DEV_LOG_group = Settings->SDK_LOG_MASK.value.int_value;

static char m_DEV_LOG_group_string[][16] = DEV_LOG_GROUP_STRING;

static const U32 MaxLogLength = 1024;
static U32 m_OfflineEnable = TRUE;

const char* DevLogGroupToString(DEV_LOG_GROUP group) {
    const char *pString = NULL;
    if (group >= GROUP_MAX) {
        pString = "DRIFT";
        return pString;
    }
    switch (group) {
    case 1 << 0:
        pString = m_DEV_LOG_group_string[0];
        break;
    case 1 << 1:
        pString = m_DEV_LOG_group_string[1];
        break;
    case 1 << 2:
        pString = m_DEV_LOG_group_string[2];
        break;
    case 1 << 3:
        pString = m_DEV_LOG_group_string[3];
        break;
    case 1 << 4:
        pString = m_DEV_LOG_group_string[4];
        break;
    case 1 << 5:
        pString = m_DEV_LOG_group_string[5];//TODO optimum length
        break;
    default:
        pString = "DRIFT";
        break;
    }
    return pString;
}

const char* __DevLog_NoDirFileName(const char *pFilePath) {
    if (pFilePath == NULL) {
        return NULL;
    }
    const char *pFileName = strrchr(pFilePath, '/');
    if (NULL != pFileName) {
        pFileName += 1;
    } else {
        pFileName = pFilePath;
    }
    return pFileName;
}

S32 __DevLog_PintOffline(U32 level, U32 group, const char *log_tag, const char *format, ...) {
    if (m_OfflineEnable == 0) {
        return RET_OK;
    }
    if (m_DEV_LOG_level > level) {
        return RET_OK;
    }
    if ((m_DEV_LOG_group & group) == 0) {
        return RET_OK;
    }
    char strBuffer[MaxLogLength];
    char levelBuffer[2] = { 0 };
    U32 logLen = 0;
    U32 timestampLen = 0;
    DEV_TIME_BUF time_temp;
    switch (level) {
    case DEV_LOG_LEVEL_VERBOSE:
        levelBuffer[0] = 'V';
        break;
    case DEV_LOG_LEVEL_DEBUG:
        levelBuffer[0] = 'D';
        break;
    case DEV_LOG_LEVEL_INFO:
        levelBuffer[0] = 'I';
        break;
    case DEV_LOG_LEVEL_WARNING:
        levelBuffer[0] = 'W';
        break;
    case DEV_LOG_LEVEL_ERROR:
        levelBuffer[0] = 'E';
        break;
    default:
        levelBuffer[0] = 'W';
        break;
    }
    Device->time->GetTime(&time_temp);
    U32 pid = 0;
    U32 tid = 0;

#if defined(WINDOWS_OS)
    pid = GetCurrentProcessId();
    tid = GetCurrentThreadId();
#elif defined(LINUX_OS)
    pid = getpid();
    tid = gettid();
#endif

    timestampLen = Dev_snprintf((char*)strBuffer, sizeof(strBuffer) - 1, "%02d-%02d %02d:%02d:%02d:%09.0f  %4d  %4d %s ", time_temp.month,
            time_temp.day, time_temp.hour, time_temp.minute, time_temp.second, (double)time_temp.nsecond, pid, tid, levelBuffer);
    va_list args;
    va_start(args, format);
    logLen = Dev_snprintf(strBuffer + timestampLen, sizeof(strBuffer) - timestampLen, "[%s][%s]", DevLogGroupToString(DEV_LOG_GROUP(group)), log_tag);
    logLen += vsnprintf(strBuffer + timestampLen + logLen, sizeof(strBuffer) - timestampLen - logLen, format, args);
    va_end(args);
    logLen = ((timestampLen + logLen) > (MaxLogLength - 3)) ? MaxLogLength - 3 : timestampLen + logLen;
    strBuffer[logLen] = '\n';
    strBuffer[logLen + 1] = '\0';
#if defined(MIVIFWK_CDK_OS)  //MIMV
    if (level >= DEV_LOG_LEVEL_ERROR) {
        midebug::MiDebugInterface::getInstance()->setFailedKeyInfoInterface(strBuffer, logLen + 1);
    }
    midebug::MiDebugInterface::getInstance()->addLogInterface(strBuffer, logLen + 1);
#endif
    return RET_OK;
}

S32 __DevLog_PintOnline(U32 level, U32 group, const char *log_tag, const char *format, ...) {
    if (m_DEV_LOG_level > level) {
        return RET_OK;
    }
    if ((m_DEV_LOG_group & group) == 0) {
        return RET_OK;
    }
    char strBuffer[MaxLogLength];
    U32 logLen = 0;
    va_list args;
    va_start(args, format);
    logLen = Dev_snprintf(strBuffer, sizeof(strBuffer), "[%s]", DevLogGroupToString(DEV_LOG_GROUP(group)));
    logLen += vsnprintf(strBuffer + logLen, sizeof(strBuffer) - logLen, format, args);
    va_end(args);
    logLen = ((logLen) > (MaxLogLength - 3)) ? MaxLogLength - 3 : logLen;
    strBuffer[logLen] = '\n';
    strBuffer[logLen + 1] = '\0';
    switch (level) {
    case DEV_LOG_LEVEL_VERBOSE:
        ALOG(LOG_VERBOSE, log_tag, "%s", strBuffer);
        break;
    case DEV_LOG_LEVEL_DEBUG:
        ALOG(LOG_DEBUG, log_tag, "%s", strBuffer);
        break;
    case DEV_LOG_LEVEL_INFO:
        ALOG(LOG_INFO, log_tag, "%s", strBuffer);
        break;
    case DEV_LOG_LEVEL_WARNING:
        ALOG(LOG_WARN, log_tag, "%s", strBuffer);
        break;
    case DEV_LOG_LEVEL_ERROR:
        ALOG(LOG_ERROR, log_tag, "%s", strBuffer);
        break;
    default:
        ALOG(LOG_WARN, log_tag, "%s", strBuffer);
        break;
    }
    return RET_OK;
}

S32 DevLog_Register(U32 uart, Callback_LOG_Notify callback) {
    return RET_OK;
}

S32 DevLog_UnRegister(U32 uart, Callback_LOG_Notify callback) {
    return RET_OK;
}

S64 DevLog_Level(U32 level) {
    m_DEV_LOG_level = level;
    return m_DEV_LOG_level;
}

S64 DevLog_Group(U32 group) {
    m_DEV_LOG_group = group;
    return m_DEV_LOG_group;
}

S64 DevLog_OfflineEnable(U32 en) {
    m_OfflineEnable = en;
    return RET_OK;
}

S64 DevLog_Init(void) {
    return RET_OK;
}

S64 DevLog_Deinit(void) {
    return RET_OK;
}

const Dev_Log m_dev_log = {
        .Init       = DevLog_Init,
        .Deinit     = DevLog_Deinit,
        .Level      = DevLog_Level,
        .Group      = DevLog_Group,
        .OfflineEnable  = DevLog_OfflineEnable,
        .Register   = DevLog_Register,
        .UnRegister = DevLog_UnRegister,
};
