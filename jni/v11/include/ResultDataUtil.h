/*
 * Copyright (c) 2019. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef MIUICAMERADAEMON_RESULTDATAUTIL_H
#define MIUICAMERADAEMON_RESULTDATAUTIL_H

#include <android-base/macros.h>
#include <jni.h>
#include <stdlib.h>

#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include "MiaInterface.h"
#include "MiaInterface_V10.h"

using namespace mialgo;

template <typename T>
using JavaRef = std::unique_ptr<typename std::remove_pointer<T>::type, std::function<void(T)>>;

template <typename T>
JavaRef<T> makeJavaref(JNIEnv *env, T ref)
{
    return JavaRef<T>(ref, [env](T ref) {
        if (env && ref) {
            env->DeleteLocalRef(ref);
        }
    });
}

JavaRef<jstring> makeJavastr(JNIEnv *env, const std::string &str);

class ResultDataUtil
{
public:
    static int setResultData(JNIEnv *env, MiaResult *result, jobject jResultData);
    static int setResultDataV10(JNIEnv *env, MiaResultV10 *result, jobject jResultData);
};

#endif // MIUICAMERADAEMON_RESULTDATAUTIL_H
