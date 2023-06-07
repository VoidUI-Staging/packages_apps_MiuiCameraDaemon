/*
 *   @author:   zhuangxj
 *   @mail:     zhuangxiaojian@morphochina.com
 *   @date:     2019/05/30
 *   @version:  v1.0.0
 */

#ifndef MORPHO_LOG_DEBUG_H
#define MORPHO_LOG_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ANDROID_LOG
#ifdef ANDROID_LOG

#ifndef LOG_TAG
#define LOG_TAG "MORPHO"
#endif

#include <android/log.h>
#define XLOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define XLOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define XLOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#else

#define XLOGI(fmt, args...) printf(LOG_TAG "(%s)[%d] " fmt, __func__, __LINE__, ##args)
#define XLOGD(fmt, args...) printf(LOG_TAG "(%s)[%d] " fmt, __func__, __LINE__, ##args)
#define XLOGE(fmt, args...) printf(LOG_TAG "(%s)[%d] " fmt, __func__, __LINE__, ##args)

#define LOGI(...) XLOGI(__VA_ARGS__)
#define LOGD(...) XLOGD(__VA_ARGS__)
#define LOGE(...) XLOGE(__VA_ARGS__)
#endif

#define LOGI_IF_VALID(cond, ...) \
    do {                         \
        if ((cond)) {            \
            XLOGI(__VA_ARGS__);  \
        }                        \
    } while (0)
#define LOGD_IF_VALID(cond, ...) \
    do {                         \
        if ((cond)) {            \
            XLOGD(__VA_ARGS__);  \
        }                        \
    } while (0)
#define LOGE_IF_VALID(cond, ...) \
    do {                         \
        if ((cond)) {            \
            XLOGE(__VA_ARGS__);  \
        }                        \
    } while (0)

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* MORPHO_LOG_DEBUG_H */
