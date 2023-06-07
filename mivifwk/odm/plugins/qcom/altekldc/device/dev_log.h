/**
 * @file        dev_log.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version        0.1
 * @note
 * @warning
 * @par
 *
 */

#ifndef __DEV_LOG_H__
#define __DEV_LOG_H__

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "settings.h"

#undef LOG_TAG
#define LOG_TAG  Settings->SDK_PROJECT_NAME.value.char_value

#undef LOG_GROUP
#define LOG_GROUP GROUP_MAX

#define DEV_LOGHEX_MAX_BUFF        (128)

#define DEV_LOG_GROUP_STRING {{"CORE"},{"PLUGIN"},{"META"},{"SERVICE"},{"ALGORITHM"},{"TEST"},{"DRIFT"}}

typedef enum Dev_Log_group{
    GROUP_CORE = (1 << 0),
    GROUP_PLUGIN = (1 << 1),
    GROUP_META = (1 << 2),
    GROUP_SERVICE = (1 << 3),
    GROUP_ALGORITHM = (1 << 4),
    GROUP_TEST = (1 << 5),
    GROUP_MAX = (1 << 6),
} DEV_LOG_GROUP;

extern U32 m_DEV_LOG_level;

typedef enum Dev_Log_level{
    DEV_LOG_LEVEL_VERBOSE = 0,
    DEV_LOG_LEVEL_DEBUG = 1,
    DEV_LOG_LEVEL_INFO = 2,
    DEV_LOG_LEVEL_WARNING = 3,
    DEV_LOG_LEVEL_ERROR = 4,
} DEV_LOG_LEVEL;

const char* __DevLog_NoDirFileName(const char *pFilePath);
S32 __DevLog_PintOffline(U32 level, U32 group, const char *log_tag, const char *format, ...);
S32 __DevLog_PintOnline(U32 level, U32 group, const char *log_tag, const char *format, ...);

#define DEV_LOGV(fmt, args...)                                          \
    do{                                                                 \
        __DevLog_PintOnline (DEV_LOG_LEVEL_VERBOSE,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
        __DevLog_PintOffline(DEV_LOG_LEVEL_VERBOSE,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
    }while(0)

#define DEV_LOGD(fmt, args...)                                          \
    do{                                                                 \
        __DevLog_PintOnline (DEV_LOG_LEVEL_DEBUG,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
        __DevLog_PintOffline(DEV_LOG_LEVEL_DEBUG,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
    }while(0)

#define DEV_LOGI(fmt, args...)                                          \
    do{                                                                 \
        __DevLog_PintOnline (DEV_LOG_LEVEL_INFO,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
        __DevLog_PintOffline(DEV_LOG_LEVEL_INFO,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
    }while(0)

#define DEV_LOGW(fmt, args...)                                          \
    do{                                                                 \
        __DevLog_PintOnline (DEV_LOG_LEVEL_WARNING,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
        __DevLog_PintOffline(DEV_LOG_LEVEL_WARNING,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
    }while(0)

#define DEV_LOGE(fmt, args...)                                          \
    do{                                                                 \
        __DevLog_PintOnline (DEV_LOG_LEVEL_ERROR,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
        __DevLog_PintOffline(DEV_LOG_LEVEL_ERROR,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
    }while(0)

#define DEV_IF_LOGV(cond, fmt, args...)                                 \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_VERBOSE,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_VERBOSE,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
        }                                                               \
    }while(0)

#define DEV_IF_LOGD(cond, fmt, args...)                                 \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_DEBUG,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_DEBUG,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
        }                                                               \
    }while(0)

#define DEV_IF_LOGI(cond, fmt, args...)                                 \
    do{                                                                 \
        if (cond) {            \
            __DevLog_PintOnline (DEV_LOG_LEVEL_INFO,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_INFO,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
        }                                                               \
    }while(0)

#define DEV_IF_LOGW(cond, fmt, args...)                                 \
    do{                                                                 \
        if (cond) {         \
            __DevLog_PintOnline (DEV_LOG_LEVEL_WARNING,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_WARNING,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
        }                                                               \
    }while(0)

#define DEV_IF_LOGE(cond, fmt, args...)                                 \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_ERROR,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_ERROR,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
        }                                                               \
    }while(0)

#define DEV_IF_LOGV_RETURN_RET(cond, ret, fmt, args...)                 \
    do{                                                                 \
        if (cond) {         \
            __DevLog_PintOnline (DEV_LOG_LEVEL_VERBOSE,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_VERBOSE,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            return ret;                                                 \
        }                                                               \
    }while(0)

#define DEV_IF_LOGD_RETURN_RET(cond, ret, fmt, args...)                 \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_DEBUG,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_DEBUG,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            return ret;                                                 \
        }                                                               \
    }while(0)

#define DEV_IF_LOGI_RETURN_RET(cond, ret, fmt, args...)                 \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_INFO,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_INFO,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            return ret;                                                 \
        }                                                               \
    }while(0)

#define DEV_IF_LOGW_RETURN_RET(cond, ret, fmt, args...)                 \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_WARNING,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_WARNING,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            return ret;                                                 \
        }                                                               \
    }while(0)

#define DEV_IF_LOGE_RETURN_RET(cond, ret, fmt, args...)                 \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_ERROR,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_ERROR,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            return ret;                                                 \
        }                                                               \
    }while(0)

#define DEV_IF_LOGV_RETURN(cond, fmt, args...)                          \
    do{                                                                 \
        if (cond) {         \
            __DevLog_PintOnline (DEV_LOG_LEVEL_VERBOSE,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_VERBOSE,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            return ;                                                    \
        }                                                               \
    }while(0)

#define DEV_IF_LOGD_RETURN(cond, fmt, args...)                          \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_DEBUG,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_DEBUG,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            return ;                                                    \
        }                                                               \
    }while(0)

#define DEV_IF_LOGI_RETURN(cond, fmt, args...)                          \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_INFO,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_INFO,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            return ;                                                    \
        }                                                               \
    }while(0)

#define DEV_IF_LOGW_RETURN(cond, fmt, args...)                          \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_WARNING,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_WARNING,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            return ;                                                    \
        }                                                               \
    }while(0)

#define DEV_IF_LOGE_RETURN(cond, fmt, args...)                          \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_ERROR,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_ERROR,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            return ;                                                    \
        }                                                                \
    }while(0)

#define DEV_IF_LOGV_GOTO(cond, tag, fmt, args...)                       \
    do{                                                                 \
        if (cond) {         \
            __DevLog_PintOnline (DEV_LOG_LEVEL_VERBOSE,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_VERBOSE,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            goto tag;                                                   \
        }                                                               \
    }while(0)

#define DEV_IF_LOGD_GOTO(cond, tag, fmt, args...)                       \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_DEBUG,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_DEBUG,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            goto tag;                                                   \
        }                                                               \
    }while(0)

#define DEV_IF_LOGI_GOTO(cond, tag, fmt, args...)                       \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_INFO,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_INFO,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            goto tag;                                                   \
        }                                                               \
    }while(0)

#define DEV_IF_LOGW_GOTO(cond, tag, fmt, args...)                       \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_WARNING,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_WARNING,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            goto tag;                                                   \
        }                                                               \
    }while(0)

#define DEV_IF_LOGE_GOTO(cond, tag, fmt, args...)                       \
    do{                                                                 \
        if (cond) {                                                     \
            __DevLog_PintOnline (DEV_LOG_LEVEL_ERROR,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            __DevLog_PintOffline(DEV_LOG_LEVEL_ERROR,LOG_GROUP,LOG_TAG,"[%s][%s()]:[%d] " fmt,__DevLog_NoDirFileName(__FILE__), __func__, __LINE__, ##args); \
            goto tag;                                                   \
        }                                                               \
    }while(0)

#define DEV_LOG_HEX(string,pData,size)                                  \
    do {                                                                \
        if (size <= DEV_LOGHEX_MAX_BUFF) {                              \
            char __pData__[(size * 6) + ((size / 8) * 8)];              \
            memset(__pData__, 0, sizeof(__pData__));                    \
            UINT __i__ = 0;                                             \
            UINT __add__ = 0;                                           \
            sprintf(__pData__ + __add__, "%c", '|');                    \
            __add__ = strlen(__pData__);                                \
            for (__i__ = 0; __i__ < size; __i__++) {                    \
                if ((__i__ % 8 == 0) && (__i__ != 0)) {                 \
                    sprintf(__pData__ + __add__, "|<=%ld\r\n|", __i__);  \
                    __add__ = strlen(__pData__);                        \
                }                                                       \
                sprintf(__pData__ + __add__, "0X%02X ", pData[__i__]);  \
                __add__ = strlen(__pData__);                            \
            }                                                           \
            sprintf(__pData__ + __add__, "|<=%ld\r\n", __i__);           \
            __add__ = strlen(__pData__);                                \
        __DevLog_PintOnline (DEV_LOG_LEVEL_INFO,LOG_GROUP,LOG_TAG,"[%s()]:[%d]%s \n%s", __func__, __LINE__, (const char*)string,__pData__); \
        __DevLog_PintOffline(DEV_LOG_LEVEL_INFO,LOG_GROUP,LOG_TAG,"[%s()]:[%d]%s \n%s", __func__, __LINE__, (const char*)string,__pData__); \
        } else {                                                        \
            DEV_LOGE("DEV_LOGHEX_MAX_BUFF=[%d] BUT SIZE=[%d] ERR!", DEV_LOGHEX_MAX_BUFF, size);    \
        }                                                               \
    } while(0)

typedef void (*Callback_LOG_Notify)(U32 uart, U8 *data, U32 len);

typedef struct __Dev_Log Dev_Log;
struct __Dev_Log {
    S64 (*Init)(void);
    S64 (*Deinit)(void);
    S64 (*Level)(U32 level);
    S64 (*Group)(U32 group);
    S64 (*OfflineEnable)(U32 en);
    S32 (*Register)(U32 uart, Callback_LOG_Notify callback);
    S32 (*UnRegister)(U32 uart, Callback_LOG_Notify callback);
};

extern const Dev_Log m_dev_log;
#endif //__DEV_LOG_H__
