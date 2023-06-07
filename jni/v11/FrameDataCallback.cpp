/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "FrameDataCallback.h"

#include <assert.h>

#include "MiaInterface.h"
#include "NativeDebug.h"

#undef LOG_TAG
#define LOG_TAG "FrameDataCallback"

using namespace mialgo;

static JavaVM *jvm;

FrameDataCallback::FrameDataCallback(JNIEnv *env, jobject weakImage)
{
    mImageJObjectWeak = env->NewGlobalRef(weakImage);
    jint rs = env->GetJavaVM(&jvm);
    assert(rs == JNI_OK);
}

FrameDataCallback::~FrameDataCallback()
{
    if (mImageJObjectWeak != NULL) {
        JNIEnv *env;
        jint rs = jvm->AttachCurrentThread(&env, NULL);
        assert(rs == JNI_OK);
        env->DeleteGlobalRef(mImageJObjectWeak);
        mImageJObjectWeak = NULL;
    }
}

void FrameDataCallback::release(MiaFrame *frame)
{
    LOGD("%s: input frame: %p", __func__, frame);
    // call FrameData object release() method to release image
    if (mImageJObjectWeak != NULL) {
        JNIEnv *env;
        jint rs = jvm->AttachCurrentThread(&env, NULL);
        assert(rs == JNI_OK);
        jclass frameDataCls = env->GetObjectClass(mImageJObjectWeak);
        jmethodID get_buffer_list = env->GetMethodID(frameDataCls, "release", "()V");
        env->CallVoidMethod(mImageJObjectWeak, get_buffer_list);
    }
}