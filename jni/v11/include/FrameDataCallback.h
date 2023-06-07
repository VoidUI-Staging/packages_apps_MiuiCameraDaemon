/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef MIUICAMERADAEMON_FRAMEDATACALLBACK_H
#define MIUICAMERADAEMON_FRAMEDATACALLBACK_H

#include <jni.h>
/* Header for class com_xiaomi_engine_MiCamAlgoInterfaceJNI */
#include <android/log.h>

#include "MiaInterface.h"
using namespace mialgo;

class FrameDataCallback : public MiaFrameCb
{
public:
    FrameDataCallback(JNIEnv *env, jobject weakImage);
    virtual ~FrameDataCallback();
    virtual void release(MiaFrame *frame);

    jobject mImageJObjectWeak;
};

#endif // MIUICAMERADAEMON_FRAMEDATACALLBACK_H
