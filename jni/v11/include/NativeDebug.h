#include <android/log.h>

#undef LOG_TAG
#define LOG_TAG   "MiCamAlgoJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ASSERT(cond, fmt, ...)                                    \
    if (!(cond)) {                                                \
        __android_log_assert(#cond, LOG_TAG, fmt, ##__VA_ARGS__); \
    }

/*
 * Throw a java Exception with Exception class name and string message
 * @param env jni env
 * @param jClassName Exception name, such as java/lang/Exception
 * @param msg error message string
 */
static void JNI_ThrowByName(JNIEnv *env, const char *jClassName, const char *msg)
{
    jclass cls = env->FindClass(jClassName);
    if (cls != NULL) {
        (*env).ThrowNew(cls, msg);
    }
    env->DeleteLocalRef(cls);
}
