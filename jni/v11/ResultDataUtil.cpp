/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "ResultDataUtil.h"

#include <jni.h>

#include "FrameDataCallback.h"
#include "NativeDebug.h"

using namespace mialgo;
#undef LOG_TAG
#define LOG_TAG "ResultDataUtil"

JavaRef<jstring> makeJavastr(JNIEnv *env, const std::string &str)
{
    return makeJavaref(env, env->NewStringUTF(str.c_str()));
}

/**
 *
 * @param env jni environment printer
 * @param FlawResult struct printer
 * @param jResultData {@link com.xiaomi.engine.ResultData}
 * @return result code
 */
int ResultDataUtil::setResultData(JNIEnv *env, MiaResult *result, jobject jResultData)
{
    LOGI("====================== <setResultData> ==============================");
    jclass frameCls = env->GetObjectClass(jResultData);

    LOGI("resultId  %d, timeStamp %ld", result->resultId, result->timeStamp);
    jmethodID set_result_id = env->GetMethodID(frameCls, "setResultId", "(I)V");
    env->CallVoidMethod(jResultData, set_result_id, result->resultId);

    jmethodID set_timestamp = env->GetMethodID(frameCls, "setTimeStamp", "(J)V");
    env->CallVoidMethod(jResultData, set_timestamp, result->timeStamp);

    if (MIA_RESULT_METADATA == result->type) {
        LOGI("set metaData count %d", result->metaData.size());
        auto hashMapClass = env->FindClass("java/util/HashMap");
        jclass hashMapClazz = static_cast<jclass>(env->NewGlobalRef(hashMapClass));
        jmethodID hashMapMethod = env->GetMethodID(hashMapClass, "<init>", "()V");
        auto jMetadataMap = makeJavaref(env, env->NewObject(hashMapClazz, hashMapMethod));

        auto mapClass = env->FindClass("java/util/Map");
        jmethodID mapPutMethod = env->GetMethodID(
            mapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

        for (auto &it : result->metaData) {
            LOGI("get map key:%s, value:%s", it.first.c_str(), it.second.c_str());
            auto jKey = makeJavastr(env, it.first);
            auto jValue = makeJavastr(env, it.second);
            env->CallObjectMethod(jMetadataMap.get(), mapPutMethod, jKey.get(), jValue.get());
        }

        jmethodID set_meta_result =
            env->GetMethodID(frameCls, "setMetaResult", "(Ljava/util/HashMap;)V");
        if (set_meta_result != NULL) {
            env->CallVoidMethod(jResultData, set_meta_result, jMetadataMap.get());
        } else {
            env->ExceptionClear(); // clear OOM
        }

        env->DeleteGlobalRef(static_cast<jobject>(hashMapClazz));
    }
    return 0;
}

/**
 *
 * @param env jni environment printer
 * @param FlawResult struct printer
 * @param jResultData {@link com.xiaomi.engine.ResultData}
 * @return result code
 */
int ResultDataUtil::setResultDataV10(JNIEnv *env, MiaResultV10 *result, jobject jResultData) {
    FlawResult *flawResult = result->flawData;

    jclass frameCls = env->GetObjectClass(jResultData);
    LOGI("====================== <setResultData> ==============================");

    LOGI("set_result_id  %d", result->resultId);
    jmethodID set_result_id = env->GetMethodID(frameCls, "setResultId", "(I)V");
    env->CallVoidMethod(jResultData, set_result_id, result->resultId);

    if (result->type == (ResultType)ResultType_V10::MIA_RESULT_FLAW) {
        LOGI("set_flaw_result %f", flawResult->flaw_result);
        jmethodID set_flaw_result = env->GetMethodID(frameCls, "setFlawResult", "(I)V");
        env->CallVoidMethod(jResultData, set_flaw_result, flawResult->flaw_result);
    }
    return 0;
}
