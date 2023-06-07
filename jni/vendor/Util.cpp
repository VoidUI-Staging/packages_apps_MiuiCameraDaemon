/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "include/Util.h"
#include <system/window.h>
#include <gui/Surface.h>
#include <gui/IProducerListener.h>
#include <stdio.h>

using namespace android;

/*
 * SurfaceWrapper: Encapsulate surface interface and manager the
 * surface buffer associated with MiaImage
 */
class SurfaceWrapper : public BnProducerListener {

#if defined(DETACHED_SURPPORT)
private:
    sp<Surface> mProducer;
#endif

public:
    // as a BufferProducer listener
    // Implementation of IProducerListener, used to notify that the consumer
    // has returned a buffer and it is ready for ImageWriter to dequeue.
    virtual void onBufferReleased() {
        // do nothing
    };

#if defined(DETACHED_SURPPORT)
    virtual void onBufferDetached(int slot) {
        if (mProducer) {
            mProducer->releaseSlot(slot);
        }
    }
    void setProducer(const sp<Surface>& producer) { mProducer = producer; }
#endif

};

/**
 * get ANativeWindow pointer with java surface object
 * @param env
 * @param surfaceObj
 * @return
 */
void* getANativeWindow(JNIEnv* env, jobject surfaceObj) {
    sp<Surface> sur;
    jclass clazz = env->FindClass("android/view/Surface");
    jobject res = env->NewGlobalRef(clazz);
    jclass surface_clazz = static_cast<jclass>(res);

    jfieldID mlock = env->GetFieldID(surface_clazz, "mLock", "Ljava/lang/Object;");
    jobject lock = env->GetObjectField(surfaceObj, mlock);

    if (env->MonitorEnter(lock) == JNI_OK) {
        jfieldID mNativeObject = env->GetFieldID(surface_clazz, "mNativeObject", "J");
        sur = reinterpret_cast<Surface *>(env->GetLongField(surfaceObj, mNativeObject));
        env->MonitorExit(lock);
    }
    env->DeleteLocalRef(lock);
    if (!sur) {
        return NULL;
    }
    sp<SurfaceWrapper> listener = new SurfaceWrapper();
#if defined(DETACHED_SURPPORT)
    if (listener) {
        listener->setProducer(sur);
    }
#endif
    sur->connect(NATIVE_WINDOW_API_CPU, listener.get(), /*reportBufferRemoval*/false);
    env->DeleteGlobalRef(static_cast<jobject>(surface_clazz));
    surface_clazz = NULL;
    ANativeWindow *window = sur.get();
    return (void *)window;
}
