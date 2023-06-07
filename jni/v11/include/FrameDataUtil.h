/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef MIUICAMERADAEMON_FRAMEDATAUTIL_H
#define MIUICAMERADAEMON_FRAMEDATAUTIL_H

#include <jni.h>
#include <stdlib.h>

#include "MiaInterface.h"

using namespace mialgo;

int get_api_version();

class FrameDataUtil
{
public:
    static int getMiImageList(JNIEnv *env, /*out*/ MiImageList_t *miImage, /*in*/ jobject jImage);
    static int getMiaFrame(JNIEnv *env, MiaFrame *frame, jobject jFrameData);
    static int getPreProcessInfo(JNIEnv *env, PreProcessInfo *info, jobject jFrameData);
};

#endif // MIUICAMERADAEMON_FRAMEDATAUTIL_H
