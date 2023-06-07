/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "FrameDataUtil.h"

#include <android/hardware_buffer_jni.h>
#include <android_media_Utils.h>
#include <camera/CameraMetadata.h>
#include <cutils/native_handle.h>
#include <cutils/properties.h>
#include <hardware/gralloc.h>
#include <jni.h>
#include <ui/GraphicBuffer.h>
#include <ui/Rect.h>
#include <utils/Log.h>
#include <vndk/hardware_buffer.h>

#include "FrameDataCallback.h"
#include "NativeDebug.h"

using namespace android;
using namespace mialgo;
using ::android::LockedImage;

#undef LOG_TAG
#define LOG_TAG "FrameDataUtil"

static int CameraMetadata_getNativeMetadata(JNIEnv *env, /*out*/ int64_t *metadata,
                                            /*in*/ jobject jCameraMetadataNative);

/**
 *
 * @param env jni environment printer
 * @param frame mia frame struct printer
 * @param jFrameData {@link com.xiaomi.engine.FrameData}
 * @return result code
 */
int FrameDataUtil::getMiaFrame(JNIEnv *env, MiaFrame *frame, jobject jFrameData)
{
    jclass frameCls = env->GetObjectClass(jFrameData);
    LOGI("=======================================================");
    // int streamIndex = FrameData.getImageFlag()
    jmethodID get_stream_index = env->GetMethodID(frameCls, "getImageFlag", "()I");
    jint index = env->CallIntMethod(jFrameData, get_stream_index);
    frame->stream_index = static_cast<uint32_t>(index);
    LOGI("frame->stream_index = %u", frame->stream_index);
    // long frameNumber = FrameData.getFrameNumber()
    jmethodID get_frame_num = env->GetMethodID(frameCls, "getFrameNumber", "()J");
    jlong num = env->CallLongMethod(jFrameData, get_frame_num);
    frame->frame_number = static_cast<uint64_t>(num);
    LOGI("frame->frame_number = %lld", frame->frame_number);

    // int sequenceId = FrameData.getSequenceId();
    jmethodID get_sequence_id = env->GetMethodID(frameCls, "getSequenceId", "()I");
    jint sequenceId = env->CallIntMethod(jFrameData, get_sequence_id);
    frame->requestId = static_cast<uint32_t>(sequenceId);
    LOGI("frame->requestId = %d", frame->requestId);

    // Parcelable cameraMetadataNative = getCaptureResultMetaDataNative()
    jmethodID get_metadata =
        env->GetMethodID(frameCls, "getCaptureResultMetaDataNative", "()Landroid/os/Parcelable;");
    jobject jCameraMetadataNative = env->CallObjectMethod(jFrameData, get_metadata);
    // get private long mMetadataPtr;
    int64_t result;
    CameraMetadata_getNativeMetadata(env, &result, jCameraMetadataNative);
    frame->result_metadata = reinterpret_cast<void *>(result);
    LOGI("frame->result_metadata point addr == 0x%llx", result);

    // Parcelable cameraMetadataNative = getCaptureRequestMetaDataNative()
    jmethodID get_request_metadata =
        env->GetMethodID(frameCls, "getCaptureRequestMetaDataNative", "()Landroid/os/Parcelable;");
    jobject jCameraMetadataNativeRequest = env->CallObjectMethod(jFrameData, get_request_metadata);
    // get private long mMetadataPtr;
    int64_t request;
    CameraMetadata_getNativeMetadata(env, &request, jCameraMetadataNativeRequest);
    frame->request_metadata = reinterpret_cast<void *>(request);
    LOGI("frame->meta point addr == 0x%llx", request);

    // Parcelable cameraMetadataNative = getPhysicalResultMetadata()
    jmethodID get_physical_metadata =
        env->GetMethodID(frameCls, "getPhysicalResultMetadata", "()Landroid/os/Parcelable;");
    jobject jCameraMetadataNativePhyResult =
        env->CallObjectMethod(jFrameData, get_physical_metadata);
    // get private long mMetadataPtr;
    int64_t phsical_result;
    CameraMetadata_getNativeMetadata(env, &phsical_result, jCameraMetadataNativePhyResult);
    frame->phycam_metadata = reinterpret_cast<void *>(phsical_result);
    LOGI("frame->phycam_metadata point addr == 0x%llx", phsical_result);

    // Image image = FrameData.getBufferImage()
    jmethodID get_buffer_image =
        env->GetMethodID(frameCls, "getBufferImage", "()Landroid/media/Image;");
    jobject buffer_image = env->CallObjectMethod(jFrameData, get_buffer_image);
    getMiImageList(env, &frame->mibuffer, buffer_image);

    // set frame callback
    frame->callback = new FrameDataCallback(env, jFrameData);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @param env
 * @param jImage pointer of MiImageList
 * @param jImage Image object
 * @return
 */
int FrameDataUtil::getMiImageList(JNIEnv *env, MiImageList_t *miImage /*out*/,
                                  jobject jImage /*Image*/)
{
    jclass imageCls = env->GetObjectClass(jImage);
    LOGI("=======================================================");
    jmethodID get_width = env->GetMethodID(imageCls, "getWidth", "()I");
    jint image_width = env->CallIntMethod(jImage, get_width);
    miImage->format.width = static_cast<uint32_t>(image_width);
    LOGI("miImage->format.width = %d", miImage->format.width);
    jmethodID get_height = env->GetMethodID(imageCls, "getHeight", "()I");
    jint image_height = env->CallIntMethod(jImage, get_height);
    miImage->format.height = static_cast<uint32_t>(image_height);
    LOGI("miImage->format.height = %d", miImage->format.height);
    jmethodID get_format = env->GetMethodID(imageCls, "getFormat", "()I");
    jint image_format = env->CallIntMethod(jImage, get_format);
    miImage->format.format = static_cast<MiaFormat>(image_format);
    LOGI("miImage->format.format = %d", miImage->format.format);
    // currently there is only one jImage in a frame data.
    miImage->imageCount = 1;

    jmethodID get_planes =
        env->GetMethodID(imageCls, "getPlanes", "()[Landroid/media/Image$Plane;");
    jobjectArray planes = (jobjectArray)env->CallObjectMethod(jImage, get_planes);
    jsize plane_count = env->GetArrayLength(planes);
    /**
     * 只给algo传入两个plane即可，因为对于 Yuv_420_888 而言，取出U的首地址，即可获取到V的首地址，
     * 因为他们之间只差1个字节。
     */
    plane_count = plane_count > 2 ? 2 : plane_count;
    miImage->numberOfPlanes = static_cast<uint32_t>(plane_count);
    LOGI("miImage->numberOfPlanes = %d", miImage->numberOfPlanes);
    for (int i = 0; i < plane_count; i++) {
        // Plane plane = jImage.getPlanes()[0]
        jobject plane = env->GetObjectArrayElement(planes, i);

        // ByteBuffer buffer = plane.getBuffer();
        jclass planeCls = env->GetObjectClass(plane);
        jmethodID get_buffer = env->GetMethodID(planeCls, "getBuffer", "()Ljava/nio/ByteBuffer;");
        jobject byte_buffer = env->CallObjectMethod(plane, get_buffer);

        // int length = buffer.remaining()
        jclass byteBufferCls = env->GetObjectClass(byte_buffer);
        jmethodID call_remaining = env->GetMethodID(byteBufferCls, "remaining", "()I");
        jint bufferLength = env->CallIntMethod(byte_buffer, call_remaining);
        LOGI("plane %d size is : %d", i, bufferLength);
        // get DirectBufferAddress
        miImage->pImageList->pAddr[i] =
            static_cast<uint8_t *>(env->GetDirectBufferAddress(byte_buffer));
        miImage->planeSize[i] = static_cast<size_t>(bufferLength); // plane size;
        miImage->pImageList->fd[i] = 0; // 传入buffer指针或者fd即可，上面传了buffer地址，则无需再传入;
        LOGI("miImage->planeSize[%d] = %zu; addr = %p", i, miImage->planeSize[i],
             miImage->pImageList->pAddr[i]);
    }
    // buffer size, unknown as lib, needn't set
    miImage->pImageList->size = 0;

    // plane width 64bit;
    miImage->planeStride = static_cast<uint32_t>(image_width);
    // y plane height 64bit;
    miImage->sliceHeight = static_cast<uint32_t>(image_height);

    jmethodID get_hardwarebuffer =
        env->GetMethodID(imageCls, "getHardwareBuffer", "()Landroid/hardware/HardwareBuffer;");
    jobject hardwareBufferObj = env->CallObjectMethod(jImage, get_hardwarebuffer);
    const native_handle_t *nativeHandle = nullptr;
    AHardwareBuffer *hardwareBuffer = AHardwareBuffer_fromHardwareBuffer(env, hardwareBufferObj);
    if (hardwareBuffer != nullptr) {
        nativeHandle = AHardwareBuffer_getNativeHandle(hardwareBuffer);
    }
    if (nativeHandle == nullptr) {
        LOGE("%s:%d error nativeHandle is null", __func__, __LINE__);
        return -1;
    } else {
        miImage->nativeHandle = (void *)nativeHandle;
    }

    sp<GraphicBuffer> gBuffer = reinterpret_cast<GraphicBuffer *>(hardwareBuffer);
    if (gBuffer == nullptr){
        LOGE("gBuffer is null");
        return -1;
    }
    const Rect rect{0, 0, static_cast<int32_t>(gBuffer->getWidth()),
                    static_cast<int32_t>(gBuffer->getHeight())};
    int fenceFd = -1;
    LockedImage lockedImg = LockedImage();
    status_t res =
        lockImageFromBuffer(gBuffer, GRALLOC_USAGE_SW_WRITE_OFTEN, rect, fenceFd, &lockedImg);
    if (res != OK) {
        // jniThrowExceptionFmt(env, "java/lang/RuntimeException",
        LOGE("lock buffer failed for format 0x%x", gBuffer->getPixelFormat());
        return -1;
    }

    return 0;
}

/**
 *
 * @param env jni environment printer
 * @param PreProcessInfo struct printer
 * @param jFrameData {@link com.xiaomi.engine.PreProcessData}
 * @return result code
 */
int FrameDataUtil::getPreProcessInfo(JNIEnv *env, PreProcessInfo *info, jobject jFrameData)
{
    jclass frameCls = env->GetObjectClass(jFrameData);

    jmethodID get_cameraId = env->GetMethodID(frameCls, "getCameraId", "()I");
    jint cameraId = env->CallIntMethod(jFrameData, get_cameraId);
    info->cameraId = static_cast<uint32_t>(cameraId);

    jmethodID get_width = env->GetMethodID(frameCls, "getWidth", "()I");
    jint width = env->CallIntMethod(jFrameData, get_width);
    info->width = static_cast<uint32_t>(width);

    jmethodID get_height = env->GetMethodID(frameCls, "getHeight", "()I");
    jint height = env->CallIntMethod(jFrameData, get_height);
    info->height = static_cast<uint32_t>(height);

    jmethodID get_format = env->GetMethodID(frameCls, "getFormat", "()I");
    jint format = env->CallIntMethod(jFrameData, get_format);
    info->format = static_cast<uint32_t>(format);

    jmethodID get_request_metadata =
        env->GetMethodID(frameCls, "getCaptureRequestMetaDataNative", "()Landroid/os/Parcelable;");
    jobject jCameraMetadataNativeRequest = env->CallObjectMethod(jFrameData, get_request_metadata);
    // get private long mMetadataPtr;
    int64_t request;
    CameraMetadata_getNativeMetadata(env, &request, jCameraMetadataNativeRequest);
    info->meta = reinterpret_cast<void *>(request);

    return 0;
}

int CameraMetadata_getNativeMetadata(JNIEnv *env, /*out*/ int64_t *metadata,
                                     jobject jCameraMetadataNative)
{
    if (!jCameraMetadataNative) {
        LOGE("%s: Invalid java metadata object.", __FUNCTION__);
        return -1;
    }
    jclass cameraMetadataClazz = env->GetObjectClass(jCameraMetadataNative);
    jfieldID res = env->GetFieldID(cameraMetadataClazz, "mMetadataPtr", "J");
    jlong metadataPtr = env->GetLongField(jCameraMetadataNative, res);
    auto nativePtr = reinterpret_cast<std::shared_ptr<android::CameraMetadata> *>(metadataPtr);
    *metadata = reinterpret_cast<jlong>(nativePtr->get());

    return 0;
}

int get_api_version()
{
    static int version = 0;
    if (version == 0) {
        char sdk_ver[128];
        memset(sdk_ver, 0, sizeof(sdk_ver));
        property_get("ro.build.version.sdk", sdk_ver, "0");
        version = atoi(sdk_ver);
    }
    return version;
}
