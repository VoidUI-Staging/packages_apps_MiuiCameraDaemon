/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef MIUICAMERADAEMON_UTIL_H
#define MIUICAMERADAEMON_UTIL_H

#include <jni.h>
/**
 * covert a surface object to ANativeWindow
 * @param surface_sp object
 * @return ANativeWindow object
 */
void *getANativeWindow(JNIEnv *env, jobject surfaceObj);

#endif // MIUICAMERADAEMON_UTIL_H
