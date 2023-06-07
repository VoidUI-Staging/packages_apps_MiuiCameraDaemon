/*----------------------------------------------------------------------------------------------
*
* This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary and
* confidential information.
*
* The information and code contained in this file is only for authorized ArcSoft employees
* to design, create, modify, or review.
*
* DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER AUTHORIZATION.
*
* If you are not an intended recipient of this file, you must not copy, distribute, modify,
* or take any action in reliance on it.
*
* If you have received this file in error, please immediately notify ArcSoft and
* permanently delete the original and any copy of any file and any printout thereof.
*
*-------------------------------------------------------------------------------------------------*/


#ifndef __DEBUG_LOGS_H__
#define __DEBUG_LOGS_H__

#include <cutils/log.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include <string.h>
#include "asvloffscreen.h"
#define ARCSOFT_LOG_TAG "ARCSOFT_DUAL_CAMERA"

static int arcLogLevel;

static void arcInitCommonConfig()
{
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.arcsoft.log", prop, "0");
    arcLogLevel = atoi(prop);
    ALOGD("arcLogLevel %d %s", arcLogLevel, prop);
}

static void arcInitCommonConfigSpec(const char *property_str)
{
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get(property_str, prop, "0");
    arcLogLevel = atoi(prop);
}

static void __attribute__((unused)) LogV(const char *fmt, ...)
{
    if(4 > arcLogLevel)
    {
        return;
    }

    va_list ap;
    char buf[512] = { 0 };

    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    __android_log_write(ANDROID_LOG_VERBOSE, ARCSOFT_LOG_TAG, buf);
}

static void __attribute__((unused)) LogD(const char *fmt, ...)
{
    if(3 > arcLogLevel)
    {
        return;
    }

    va_list ap;
    char buf[512] = { 0 };

    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    __android_log_write(ANDROID_LOG_DEBUG, ARCSOFT_LOG_TAG, buf);
}

static void __attribute__((unused)) LogI(const char *fmt, ...)
{
    if(2 > arcLogLevel)
    {
        return;
    }

    va_list ap;
    char buf[512] = { 0 };

    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    __android_log_write(ANDROID_LOG_INFO, ARCSOFT_LOG_TAG, buf);
}

static void __attribute__((unused)) LogW(const char *fmt, ...)
{
    if(1 > arcLogLevel)
    {
        return;
    }

    va_list ap;
    char buf[512] = { 0 };

    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    __android_log_write(ANDROID_LOG_WARN, ARCSOFT_LOG_TAG, buf);
}

static void __attribute__((unused)) LogE(const char *fmt, ...)
{
    if(0 > arcLogLevel)
    {
        return;
    }

    va_list ap;
    char buf[512] = { 0 };

    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    __android_log_write(ANDROID_LOG_ERROR, ARCSOFT_LOG_TAG, buf);
}


#define  ARC_LOGV(fmt,...) LogV("%s():%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define  ARC_LOGD(fmt,...) LogD("%s():%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define  ARC_LOGI(fmt,...) LogI("%s():%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define  ARC_LOGW(fmt,...) LogW("%s():%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define  ARC_LOGE(fmt,...) LogE("%s():%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)

static inline
void dumpImageAppend(const char* fileName, ASVLOFFSCREEN *img)
{
    FILE *f = fopen(fileName, "ab");
    if (f) {
        MUInt8 *y = img->ppu8Plane[0];
        MUInt8 *u = img->ppu8Plane[1];
        for (int i=0; i<img->i32Height; i++) {
            fwrite(y, img->i32Width, 1, f);
            y += img->pi32Pitch[0];
        }
        for (int i=0; i<img->i32Height / 2; i++) {
            fwrite(u, img->i32Width, 1, f);
            u += img->pi32Pitch[1];
        }
        fclose(f);
    }
}

#endif //__DEBUG_LOGS_H__
