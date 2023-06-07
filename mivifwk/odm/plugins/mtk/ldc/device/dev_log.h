/**
 * @file		dev_log.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par
 *
 */

#ifndef __DEV_LOG_H__
#define __DEV_LOG_H__

#include <inttypes.h>
#include <log/log.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#undef LOG_TAG
#define LOG_TAG PROJECT_NAME

#define DEV_LOGHEX_MAX_BUFF (128)

#define DEV_LOGV(fmt, args...) ALOGV("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args)
#define DEV_LOGI(fmt, args...) ALOGI("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args)
#define DEV_LOGW(fmt, args...) ALOGW("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args)
#define DEV_LOGE(fmt, args...) ALOGE("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args)
#define DEV_IF_LOGI(cond, fmt, args...) \
    ALOGI_IF(cond, "[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args)
#define DEV_IF_LOGW(cond, fmt, args...) \
    ALOGW_IF(cond, "[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args)
#define DEV_IF_LOGE(cond, fmt, args...) \
    ALOGE_IF(cond, "[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args)

#define DEV_IF_LOGI_RETURN_RET(cond, ret, fmt, args...)                 \
    do {                                                                \
        if (cond) {                                                     \
            ALOGI("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args); \
            return ret;                                                 \
        }                                                               \
    } while (0)

#define DEV_IF_LOGW_RETURN_RET(cond, ret, fmt, args...)                 \
    do {                                                                \
        if (cond) {                                                     \
            ALOGW("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args); \
            return ret;                                                 \
        }                                                               \
    } while (0)

#define DEV_IF_LOGE_RETURN_RET(cond, ret, fmt, args...)                 \
    do {                                                                \
        if (cond) {                                                     \
            ALOGE("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args); \
            return ret;                                                 \
        }                                                               \
    } while (0)

#define DEV_IF_LOGI_RETURN(cond, fmt, args...)                          \
    do {                                                                \
        if (cond) {                                                     \
            ALOGI("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args); \
            return;                                                     \
        }                                                               \
    } while (0)

#define DEV_IF_LOGW_RETURN(cond, fmt, args...)                          \
    do {                                                                \
        if (cond) {                                                     \
            ALOGW("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args); \
            return;                                                     \
        }                                                               \
    } while (0)

#define DEV_IF_LOGE_RETURN(cond, fmt, args...)                          \
    do {                                                                \
        if (cond) {                                                     \
            ALOGE("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args); \
            return;                                                     \
        }                                                               \
    } while (0)

#define DEV_IF_LOGI_GOTO(cond, tag, fmt, args...)                       \
    do {                                                                \
        if (cond) {                                                     \
            ALOGI("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args); \
            goto tag;                                                   \
        }                                                               \
    } while (0)

#define DEV_IF_LOGW_GOTO(cond, tag, fmt, args...)                       \
    do {                                                                \
        if (cond) {                                                     \
            ALOGW("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args); \
            goto tag;                                                   \
        }                                                               \
    } while (0)

#define DEV_IF_LOGE_GOTO(cond, tag, fmt, args...)                       \
    do {                                                                \
        if (cond) {                                                     \
            ALOGE("[%s()]:[%d] " fmt "\n", __func__, __LINE__, ##args); \
            goto tag;                                                   \
        }                                                               \
    } while (0)

#define DEV_LOG_HEX(pData, size)                                                                \
    do {                                                                                        \
        if (size <= DEV_LOGHEX_MAX_BUFF) {                                                      \
            char __pData__[(size * 6) + ((size / 8) * 8)];                                      \
            memset(__pData__, 0, sizeof(__pData__));                                            \
            UINT __i__ = 0;                                                                     \
            UINT __add__ = 0;                                                                   \
            sprintf(__pData__ + __add__, "%c", '|');                                            \
            __add__ = strlen(__pData__);                                                        \
            for (__i__ = 0; __i__ < size; __i__++) {                                            \
                if ((__i__ % 8 == 0) && (__i__ != 0)) {                                         \
                    sprintf(__pData__ + __add__, "|<=%d\r\n|", __i__);                          \
                    __add__ = strlen(__pData__);                                                \
                }                                                                               \
                sprintf(__pData__ + __add__, "0X%02X ", pData[__i__]);                          \
                __add__ = strlen(__pData__);                                                    \
            }                                                                                   \
            sprintf(__pData__ + __add__, "|<=%d\r\n", __i__);                                   \
            __add__ = strlen(__pData__);                                                        \
            ALOGI("[%s()]:[%d] \n%s", __func__, __LINE__, __pData__);                           \
        } else {                                                                                \
            DEV_LOGE("DEV_LOGHEX_MAX_BUFF=[%d] BUT SIZE=[%d] ERR!", DEV_LOGHEX_MAX_BUFF, size); \
        }                                                                                       \
    } while (0)

#endif //__DEV_LOG_H__
