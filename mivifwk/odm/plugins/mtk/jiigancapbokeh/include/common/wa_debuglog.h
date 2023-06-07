// clang-format off
#ifndef __DEBUG_LOGS_H__
#define __DEBUG_LOGS_H__

#include <cutils/log.h>
#include <cutils/properties.h>
#include <errno.h>

#define WA_LOG_TAG "WA_CHI"
#define CHECK_PERSIST FALSE

static void __attribute__((unused)) LogV(const char *fmt, ...)
{
#if CHECK_PERSIST
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("debug.camera.anc.log", prop, "0");
    int log = atoi(prop);

    if(log < 1)
    {
        return;
    }
#endif
    va_list ap;
    char buf[512] = { 0 };

    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    __android_log_write(ANDROID_LOG_VERBOSE, WA_LOG_TAG, buf);
}

static void __attribute__((unused)) LogI(const char *fmt, ...)
{
#if CHECK_PERSIST
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("debug.camera.anc.log", prop, "2");
    int log = atoi(prop);

    if(log < 2)
    {
        return;
    }
#endif
    va_list ap;
    char buf[512] = { 0 };

    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    __android_log_write(ANDROID_LOG_INFO, WA_LOG_TAG, buf);
}

static void __attribute__((unused)) LogD(const char *fmt, ...)
{
#if CHECK_PERSIST
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("debug.camera.anc.log", prop, "0");
    int log = atoi(prop);

    if(log < 3)
    {
        return;
    }
#endif
    va_list ap;
    char buf[512] = { 0 };

    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    __android_log_write(ANDROID_LOG_DEBUG, WA_LOG_TAG, buf);
}

static void __attribute__((unused)) LogE(const char *fmt, ...)
{
#if CHECK_PERSIST
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("debug.camera.anc.log", prop, "4");
    int log = atoi(prop);

    if(log < 4)
    {
        return;
    }
#endif
    va_list ap;
    char buf[512] = { 0 };

    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    __android_log_write(ANDROID_LOG_ERROR, WA_LOG_TAG, buf);
}


#define  WA_LOGV(fmt,...) LogV(fmt,##__VA_ARGS__)
#define  WA_LOGD(fmt,...) LogD(fmt,##__VA_ARGS__)
#define  WA_LOGI(fmt,...) LogI(fmt,##__VA_ARGS__)
#define  WA_LOGE(fmt,...) LogE(fmt,##__VA_ARGS__)



#endif //__DEBUG_LOGS_H__
// clang-format on
