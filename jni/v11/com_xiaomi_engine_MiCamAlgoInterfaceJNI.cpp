/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */
#include "com_xiaomi_engine_MiCamAlgoInterfaceJNI.h"

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <vndksupport/linker.h>

#include <string>

#include "FrameDataUtil.h"
#include "MiaInterface_V10.h"
#include "NativeDebug.h"
#include "ResultDataUtil.h"
#include "VendorMetadataParser.h"
#include "Util.h"

#undef LOG_TAG
#define LOG_TAG "MI_Algo_JNI"

using namespace mialgo;

static void *gHandle;
static MiaInterface *mCAI;
static MiaInterface_v10 *mCAI_v10;
int (*mGetApiVersion)() = NULL;
static MiaExtInterface *mExtCAI;
static int mExtApiVersion;
static jclass gResultData_class;
static jmethodID gResultData_constructorMethodID;
static jobject mResultProvider;

/*
 * Save a pointer to JavaVm to attach/detach threads executing
 * callback methods that need to make JNI calls.
 */
static JavaVM *gJavaVM;

struct mialgo::MiaMetaUtilCb MIA_META_UTILS = {
        VendorMetadataParser::getVTagValue,
        VendorMetadataParser::getTagValue,
        VendorMetadataParser::setVTagValue,
        VendorMetadataParser::setTagValue,
        VendorMetadataParser::allocateCopyMetadata,
        VendorMetadataParser::allocateCopyInterMetadata,
        VendorMetadataParser::freeMetadata,
        VendorMetadataParser::getVTagValueExt,
        VendorMetadataParser::getTagValueExt,
};

JNIEnv *getEnv(int *attach)
{
    if (gJavaVM == NULL)
        return NULL;

    *attach = 0;
    JNIEnv *jni_env = NULL;

    int status = gJavaVM->GetEnv((void **)&jni_env, JNI_VERSION_1_6);
    if (status == JNI_EDETACHED || jni_env == NULL) {
        status = gJavaVM->AttachCurrentThread(&jni_env, NULL);
        if (status < 0) {
            jni_env = NULL;
        } else {
            *attach = 1;
        }
    }
    return jni_env;
}

void delEnv()
{
    gJavaVM->DetachCurrentThread();
}

jint initGlobalJniVariables(JavaVM *jvm)
{
    if (gJavaVM != nullptr) {
        LOGE("%s: JVM is not NULL, jvm = %p", __FUNCTION__, jvm);
    }

    gJavaVM = jvm;

    if (gJavaVM == nullptr) {
        LOGE("%s: JVM is NULL", __FUNCTION__);
    }

    JNIEnv *jni = nullptr;
    if (jvm->GetEnv(reinterpret_cast<void **>(&jni), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_6;
}

char *jstringTochar(JNIEnv *env, jstring jstr)
{
    char *rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("utf-8");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte *ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char *)malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->DeleteLocalRef(clsstring);
    env->DeleteLocalRef(strencode);
    env->ReleaseByteArrayElements(barr, ba, 0);
    env->DeleteLocalRef(barr);
    return rtn;
}

/*
 * java session callback defined in {@link TaskSession#SessionStatusCallback}
 *
 * This Functional Interface callback has only one method, who has 3 prams:
 *    public abstract void onSessionCallback(int, java.lang.String, java.lang.Object);
 *    descriptor: (ILjava/lang/String;Ljava/lang/Object;)V
 * </br>
 * @param resultCode a integer value indicate result code
 * @param message a String value indicate some error reason
 * @param result a object array indicate indefinite parameters
 */
class MySessionCb : public MiaSessionCb
{
public:
    MySessionCb(jobject obj);
    virtual ~MySessionCb();
    virtual void onSessionResultCb(uint32_t msgType, void *data);

private:
    void callSessionCallback(JNIEnv *env, uint32_t resultCode, const char *message, void *data);
    jobject mCb;
};

MySessionCb::MySessionCb(jobject obj)
{
    mCb = obj;
    LOGD("MySessionCb has been constructed.");
}

MySessionCb::~MySessionCb() {
    LOGD("MySessionCb has been deconstructed.");

    int attach = 0;
    JNIEnv *env = getEnv(&attach);
    if (mCb != NULL) {
        env->DeleteGlobalRef(mCb);
        mCb = NULL;
    }

    if (attach == 1) {
        delEnv();
    }
};


/*
 * A callback static method to receive algorithm lib status
 *
 * @param msgType message type defined MiaInterface.h#MsgType
 * @param data message data with any type
 */
void MySessionCb::onSessionResultCb(uint32_t msgType, void *data)
{
    int attach = 0;
    JNIEnv *env = getEnv(&attach);

    switch (msgType) {
    case MsgType::MIA_MSG_PO_RESULT: { // pipeline output result, cast the data to MiaResult
        if (data == NULL) {
            LOGE("result msg type data is NULL");
            break;
        }
        MiaResult *po_result = (MiaResult *)data;
        LOGD("po_result = %p", po_result);
        LOGD("return resultId : %d", po_result->resultId);
        callSessionCallback(env, msgType, "result notify", po_result);
        break;
    }
    case MsgType::MIA_MSG_SERVICE_DIED: {
        LOGI("received service died msgType");
        callSessionCallback(env, msgType, "service died", NULL);
        break;
    }
    case MsgType::MIA_MSG_PP_ERROR:
    case MsgType::MIA_MSG_PP_TIMEOUT:
    case MsgType::MIA_MSG_PP_BUFOVERFLOW:
    case MsgType::MIA_MSG_ABNORMAL_PROCESSING: {
        MiaResult *abnormal = (MiaResult *)data;
        LOGE("return resultId : %d", abnormal->resultId);
        LOGE("return timeStamp : %llu", abnormal->timeStamp);
        callSessionCallback(env, msgType, "abnormal_process", abnormal);
        break;
    }
    default:
        LOGE("unsupported msgType:%d", msgType);
        break;
    }

    if (attach == 1) {
        delEnv();
    }
}

void MySessionCb::callSessionCallback(JNIEnv *env, uint32_t resultCode, const char *message,
                                      void *data)
{
    LOGI("Received a session callback: resultcode: %d, message: %s, data: %p", resultCode, message,
         data);
    if (mCb != NULL) {
        jclass SessionCallbackClazz = env->GetObjectClass(mCb);
        jmethodID onSessionCallbackMethod = env->GetMethodID(
            SessionCallbackClazz, "onSessionCallback", "(ILjava/lang/String;Ljava/lang/Object;)V");
        jstring resultMessage = env->NewStringUTF(message);

        if ((resultCode == MsgType::MIA_MSG_PO_RESULT) && (data != NULL)) {
            // todo miaResult to jobject
            MiaResult *miaResult = static_cast<MiaResult *>(data);
            if ((miaResult->type == ResultType::MIA_RESULT_METADATA) && (miaResult->metaData.size() > 0)) {
                LOGI("callSessionCallback output metadata result %p resultId %d mapsize %d",
                     miaResult, miaResult->resultId, miaResult->metaData.size());
                ResultDataUtil::setResultData(env, miaResult, mResultProvider);
                env->CallVoidMethod(mCb, onSessionCallbackMethod, 3, resultMessage,
                                    mResultProvider);
            } else if (miaResult->type == ResultType::MIA_ANCHOR_CALLBACK) {
                LOGI("callSessionCallback output quickview result %p resultId %d timestamp%llu ",
                     miaResult, miaResult->resultId, miaResult->timeStamp);
                ResultDataUtil::setResultData(env, miaResult, mResultProvider);
                env->CallVoidMethod(mCb, onSessionCallbackMethod, 10, resultMessage,
                                    mResultProvider);
            }
        } else if (resultCode == MsgType::MIA_MSG_SERVICE_DIED) {
            // app resultCode 1 is used to return crop region to app in MTK platform .
            env->CallVoidMethod(mCb, onSessionCallbackMethod, 2, resultMessage, NULL);
        } else if (resultCode == MsgType::MIA_MSG_ABNORMAL_PROCESSING) {
            MiaResult *miaResult = static_cast<MiaResult *>(data);
            LOGI("callSessionCallback output abnormal processing resultId %d timestamp%llu ",
                  miaResult->resultId, miaResult->timeStamp);
            ResultDataUtil::setResultData(env, miaResult, mResultProvider);
            env->CallVoidMethod(mCb, onSessionCallbackMethod, 20, resultMessage,
                                    mResultProvider);
        }
    } else {
        LOGE("callSessionCallback is a NullPointer %s %d", __func__, __LINE__);
    }
}

/*
* java session callback defined in {@link TaskSession#SessionStatusCallback}
*
* This Functional Interface callback has only one method, who has 3 prams:
*    public abstract void onSessionCallback(int, java.lang.String, java.lang.Object);
*    descriptor: (ILjava/lang/String;Ljava/lang/Object;)V
* </br>
* @param resultCode a integer value indicate result code
* @param message a String value indicate some error reason
* @param result a object array indicate indefinite parameters
*/
class MySessionCb_V10 : public MiaSessionCb_V10 {
public:
  MySessionCb_V10(jobject obj);
  virtual ~MySessionCb_V10();
  virtual void onSessionStatusChanged(uint32_t msg, void* msgData, void* priv);

private:
  void callSessionCallback(JNIEnv *env, uint32_t resultCode, const char *message, void *data);
  jobject mCb;
};

MySessionCb_V10::MySessionCb_V10(jobject obj) {
    mCb = obj;
    LOGD("MySessionCb has been constructed.");
}

MySessionCb_V10::~MySessionCb_V10() {
    LOGD("MySessionCb has been deconstructed .");

    int attach = 0;
    JNIEnv *env = getEnv(&attach);
    if (mCb != NULL) {
        LOGD("MySessionCb has DeleteGlobalRef .");
        env->DeleteGlobalRef(mCb);
        mCb = NULL;
    }

    if (attach == 1) {
        delEnv();
    }
};

/*
 * A callback static method to receive algorithm lib status
 * @param msg message type defined miainterface.h#MsgType
 * @param msgData message data with any type
 * @param priv
 */
void MySessionCb_V10::onSessionStatusChanged(uint32_t msg, void* msgData, void* priv) {
    MiaMsgDataV10 *data = (MiaMsgDataV10 *) msgData;
    LOGD("onSessionStatusChanged: msg = %d, msgData = %p, priv = %p", msg, msgData, priv);

    int attach = 0;
    JNIEnv *env = getEnv(&attach);

    assert (res == JNI_OK);

    switch (msg) {
        // pipeline output frame, cast the msgData to MiaMsgDataV10.frame
        case MsgType_V10::MIA_MSG_PO_FRAME_V10: {
            MiaFrame *po_frame = data->frame;
            callSessionCallback(env, msg, "capture successful", po_frame);
            // release this frame
            if (NULL != po_frame->callback) {
                po_frame->callback->release(po_frame);
                LOGD("onSessionStatusChanged: release frame %p", po_frame);
            }
            break;
        }
        // pipeline output result, cast the msgData to MiaMsgDataV10.result
        case MsgType_V10::MIA_MSG_PO_RESULT_V10: {
            MiaResultV10 *po_result = data->result;
            LOGD("onSessionStatusChanged: po_result = %p, resultId = %d", po_result, po_result->resultId);
            callSessionCallback(env, msg, "result successful", po_result);
            break;
        }
        // pipeline input frame release, cast the msgData to MiaMsgDataV10.frame
        case MsgType_V10::MIA_MSG_PI_FRAME_V10: {
            MiaFrame *pi_frame = data->frame;
            LOGD("onSessionStatusChanged: input frame %p %llu can be released", pi_frame, pi_frame->frame_number);
            break;
        }
        // pipeline notifications, cast the msgData to uint32_t (node ID)
        case MsgType_V10::MIA_MSG_PP_ERROR_V10: {
            uint32_t *error_code = reinterpret_cast<uint32_t *>(msgData);
            LOGD("onSessionStatusChanged: error_code = %d", *error_code);
            break;
        }
        // pipeline notifications, cast the msgData to uint32_t
        case MsgType_V10::MIA_MSG_PP_TIMEOUT_V10: {
            uint32_t *result_code = reinterpret_cast<uint32_t *>(msgData);
            LOGD("onSessionStatusChanged: timeout result_code = %d", *result_code);
            break;
        }
        // pipeline notifications, cast the msgData to uint32_t
        case MsgType_V10::MIA_MSG_PP_BUFOVERFLOW_V10: {
            uint32_t *error_node = reinterpret_cast<uint32_t *>(msgData);
            LOGD("onSessionStatusChanged: receive msg: %d from node: %d", msg, *error_node);
            break;
        }
        default: {
            LOGE("onSessionStatusChanged: unsupported msg:%d", msg);
            break;
        }
    }

    if (attach == 1) {
        delEnv();
    }
}

void MySessionCb_V10::callSessionCallback(JNIEnv *env, uint32_t resultCode, const char *message, void *data) {
    LOGI("sessionCallback: resultCode = %d, message = %s ", resultCode, message);
    if (mCb != NULL) {
        jclass SessionCallbackClazz = env->GetObjectClass(mCb);
        jmethodID onSessionCallbackMethod
            = env->GetMethodID(SessionCallbackClazz, "onSessionCallback",
                    "(ILjava/lang/String;Ljava/lang/Object;)V");
        jstring resultMessage = env->NewStringUTF(message);

        if (resultCode == MsgType_V10::MIA_MSG_PO_FRAME_V10) {
            // todo miaFrame to jobject
            MiaFrame *miaFrame = static_cast<MiaFrame *>(data);
            LOGD("sessionCallback: got output frame %p %llu", miaFrame, miaFrame->frame_number);
            LOGD("sessionCallback: return buffer size: %d", miaFrame->mibuffer.pImageList[0].size);
            LOGD("sessionCallback: return stream_index: %d", miaFrame->stream_index);
        } else if (resultCode == MsgType_V10::MIA_MSG_PO_RESULT_V10) {
            // todo miaResult to jobject
            MiaResultV10 *miaResult = static_cast<MiaResultV10 *>(data);
            if (miaResult != NULL && (miaResult->type == (ResultType)ResultType_V10::MIA_RESULT_FLAW || \
                miaResult->type == (ResultType)ResultType_V10::MIA_RESULT_RAWSN)) {
                LOGD("sessionCallback: get output result %p resultId %d", miaResult, miaResult->resultId);
                ResultDataUtil::setResultDataV10(env, miaResult, mResultProvider);
                env->CallVoidMethod(mCb, onSessionCallbackMethod, miaResult->type, resultMessage, mResultProvider);
            }
        }
    } else {
        LOGE("sessionCallback: null session");
    }
}

static GraphDescriptor getGraphDescriptor(JNIEnv *env, jobject jGraphDescriptor)
{
    if (mCAI_v10 == NULL && mCAI == NULL) {
        LOGI("gHandle %p mCAI %p,%p !", gHandle, mCAI_v10, mCAI);
        JNI_ThrowByName(env, "java/lang/RuntimeException", "algo lib has not been init!");
    }
    LOGI("Before pInit in %s %d mCAI %p,%p", __func__, __LINE__, mCAI_v10, mCAI);
    GraphDescriptor gd;
    jclass graphDescriptorClazz = env->GetObjectClass(jGraphDescriptor);
    // getOperationModeID
    jmethodID getOperationModeIDMethod =
        env->GetMethodID(graphDescriptorClazz, "getOperationModeID", "()I");
    jint operationMode = env->CallIntMethod(jGraphDescriptor, getOperationModeIDMethod);
    gd.operation_mode = static_cast<uint32_t>(operationMode);
    // getStreamNumber
    jmethodID getStreamNumberMethod =
        env->GetMethodID(graphDescriptorClazz, "getStreamNumber", "()I");
    jint streamNum = env->CallIntMethod(jGraphDescriptor, getStreamNumberMethod);
    gd.num_streams = static_cast<uint32_t>(streamNum);
    // isSnapshot
    jmethodID isSnapshotMethod = env->GetMethodID(graphDescriptorClazz, "isSnapshot", "()Z");
    jboolean isSnapshot = env->CallBooleanMethod(jGraphDescriptor, isSnapshotMethod);
    gd.is_snapshot = isSnapshot;
    // getCameraCombinationMode
    jmethodID getCameraCombinationModeMethod =
        env->GetMethodID(graphDescriptorClazz, "getCameraCombinationMode", "()I");
    jint combinationMode = env->CallIntMethod(jGraphDescriptor, getCameraCombinationModeMethod);
    gd.camera_mode = combinationMode;

    LOGI(
        "received graphId descriptor: operation_mode: %d, num_streams: %d, is_snapshot: %d, "
        "camera_id: %d",
        gd.operation_mode, streamNum, gd.is_snapshot, gd.camera_mode);
    return gd;
}

//================================== JNI interface implement =======================================

CDKResult Jni_Init_V10(JNIEnv *env, jclass obj, jstring filePath)
{
    static const char * libName = "/vendor/lib64/libmialgoengine.so";
    void *handle = android_load_sphal_library(libName, RTLD_NOW);
    if (!handle) {
        LOGE("init: open %s error: %s", libName, dlerror());
        return -1;
    }
    LOGI("init: open %s with handle: %p", libName, handle);

    gResultData_class = env->FindClass("com/xiaomi/engine/ResultData");
    gResultData_constructorMethodID = env->GetMethodID(gResultData_class, "<init>", "()V");
    jobject result = env->NewObject(gResultData_class, gResultData_constructorMethodID);
    mResultProvider = env->NewGlobalRef(result);

    void *(*MI_open_lib)(void) = NULL;
    *(void **)&MI_open_lib = dlsym(handle, "MI_open_lib");
    LOGI("init: open function pointer: %p", MI_open_lib);
    mCAI_v10 = (mialgo::MiaInterface_v10*)MI_open_lib();
    LOGI("init: mCAI = %p", mCAI_v10);

    int (*GetApiVersion)() = NULL;
    *(int **)&GetApiVersion = (int *)dlsym(handle, "MIAGetApiVersion");
    LOGI("init: MIAGetApiVersion pointer: %p", GetApiVersion);
    mGetApiVersion = GetApiVersion;

    if (mCAI_v10 == NULL) {
        LOGW("init: gHandle = %p, mCAI = %p", gHandle, mCAI_v10);
    } else {
        const char* utf_string = env->GetStringUTFChars(filePath, NULL);
        if (utf_string == NULL) {
            /* OutOfMemoryError already thrown. In some way indicate there has been an error */
            LOGE("init: null calFilePath");
        }
        char android_files_dir[128];
        strcpy(android_files_dir, utf_string); //value in android_files_dir available here

        env->ReleaseStringUTFChars(filePath, utf_string);
        MiaInitParams params;
        params.calFilePath = android_files_dir;
        params.calFilePathLen = 128;
        LOGI("init: calFilePath = %s", params.calFilePath);
        return mCAI_v10->Init( MIA_META_UTILS , &params);
    }
    return MIAS_UNKNOWN_ERROR;
}

CDKResult Jni_Init_V11(JNIEnv *env, jclass obj, jstring filePath)
{
    static const char *libName = "/system/lib64/libmicampostproc_client.so";
    // normally we should use the "android_load_sphal_library()" method to open lib
    void *handle = dlopen(libName, RTLD_NOW);
    if (!handle) {
        LOGE("Open %s Error :%s", libName, dlerror());
        JNI_ThrowByName(env, "java/lang/RuntimeException", "open libmialgoengine.so failed!");
        return -1;
    }
    LOGI("###Open %s with handle:%p", libName, handle);

    gResultData_class = env->FindClass("com/xiaomi/engine/ResultData");
    gResultData_constructorMethodID = env->GetMethodID(gResultData_class, "<init>", "()V");
    jobject result = env->NewObject(gResultData_class, gResultData_constructorMethodID);
    mResultProvider = env->NewGlobalRef(result);

    void *(*miOpenLib)(void) = NULL;
    *(void **)&miOpenLib = dlsym(handle, "miOpenLib");
    LOGI("Got the function pointer:%p ", miOpenLib);
    mCAI = (mialgo::MiaInterface *)miOpenLib();
    LOGI("Got the MiaInterface pointer:%p ", mCAI);

    mExtCAI = NULL;
    void *(*getExtInterafce)() = NULL;
    *(void **)&getExtInterafce = dlsym(handle, "GetExtInterafce");
    LOGI("getExtInterafce pointer: %p", getExtInterafce);
    if (getExtInterafce != NULL) {
        mExtCAI = (mialgo::MiaExtInterface *)getExtInterafce();

        int (*getExtVersion)() = NULL;
        *(int **)&getExtVersion = (int *)dlsym(handle, "GetExtVersion");
        if (getExtVersion != NULL) {
            mExtApiVersion = getExtVersion();
        }
        LOGI("GetExtVersion pointer: %p, extVersion: %d", getExtVersion, mExtApiVersion);
    }

    if (mCAI == NULL) {
        LOGI("gHandle %p mCAI %p !", gHandle, mCAI);
    } else {
        return mCAI->init(NULL);
    }
    return MIAS_UNKNOWN_ERROR;
}

JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_init(JNIEnv *env, jclass obj,
                                                                         jstring filePath)
{
    CDKResult res = Jni_Init_V10(env, obj, filePath);

    if (res == -1) {
        return Jni_Init_V11(env, obj, filePath);
    } else {
        return res;
    }
}

JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_setMiViInfo(JNIEnv *env,
                                                                                jclass obj,
                                                                                jstring info)
{
    if (mCAI_v10 != NULL) {
        (void)info;
        return MIAS_INVALID_CALL;
    } else {
        if (mCAI == NULL) {
            LOGI("gHandle %p mCAI %p !", gHandle, mCAI);
            return -1;
        }
        char *chardata = jstringTochar(env, info);
        std::string miviInfo = chardata;
        LOGI("huangxin get miviInfo: %s", chardata);

        return mCAI->setMiViInfo(miviInfo);
    }

    return -1;
}

JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_deInit(JNIEnv *env, jclass obj)
{
    CDKResult res = MIAS_OK;
    if (mCAI_v10 == NULL && mCAI == NULL) {
        LOGI("gHandle %p mCAI %p,%p !", gHandle, mCAI_v10, mCAI);
        return -1;
    }

    if (mResultProvider != NULL) {
        env->DeleteGlobalRef(mResultProvider);
        mResultProvider = NULL;
    }

    LOGI("Before deinit in %s %d mCAI %p,%p", __func__, __LINE__, mCAI_v10, mCAI);

    if (mCAI_v10 != NULL) {
        res = mCAI_v10->Deinit();
    } else {
        res = mCAI->deinit();
    }
    return res;
}

/*
 *
 * @param bufferFormat BufferFormat
 * @param surfaceList List<Surface> (Ljava/util/List;)V
 * @param callback TaskSession.SessionStatusCallback
 * @return session handle
 */
JNIEXPORT jlong JNICALL Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_createSessionWithSurfaces(
    JNIEnv *env, jclass obj, jobject bufferFormat, jobject surfaceList, jobject callback)
{
    if (mCAI_v10 == NULL && mCAI == NULL) {
        LOGI("gHandle %p mCAI %p,%p !", gHandle, mCAI_v10, mCAI);
        return MIAS_UNKNOWN_ERROR;
    }

    // get buffer width
    jclass frameCls = env->GetObjectClass(bufferFormat);
    jfieldID frame_width = env->GetFieldID(frameCls, "mBufferWidth", "I");
    jint width = env->GetIntField(bufferFormat, frame_width);

    // get buffer height
    jfieldID frame_height = env->GetFieldID(frameCls, "mBufferHeight", "I");
    jint height = env->GetIntField(bufferFormat, frame_height);

    // get buffer format
    jfieldID frame_format = env->GetFieldID(frameCls, "mBufferFormat", "I");
    jint format = env->GetIntField(bufferFormat, frame_format);
    FrameInfo frame;
    frame.width = (unsigned)width;
    frame.height = (unsigned)height;
    frame.format = (MiaFormat)format;
    SessionOutput outputConf;
    outputConf.type = SESSION_OUT_SURFACE;
    outputConf.fInfo = frame;

    // set sink surfaces ===========================================================================
    jclass listArray = env->GetObjectClass(surfaceList); // get List class
    if (listArray == NULL) {
        LOGE("not find class");
    }
    /* method in class List  */
    jmethodID list_get = env->GetMethodID(listArray, "get", "(I)Ljava/lang/Object;");
    jmethodID list_size = env->GetMethodID(listArray, "size", "()I");
    /* jni invoke list.get to get points count */
    int len = static_cast<int>(env->CallIntMethod(surfaceList, list_size));
    LOGD("[createSessionWithSurfaces]: get surfaces: list size is: %d", len);
    for (int i = 0; i < len; i++) {
        /* get list the element -- Surface */
        jobject element = (jobject)(env->CallObjectMethod(surfaceList, list_get, i));
        if (element == NULL) {
            JNI_ThrowByName(env, "java/lang/Exception", "fetch list element Surface failure!");
        }

        // Assume there is only 1 surface here
        void *mANativeWindowPointer = getANativeWindow(env, element);
        outputConf.surfaces[SURFACE_TYPE_VIEWFINDER] = mANativeWindowPointer;
        LOGD(
            "[createSessionWithSurfaces]: get surface: ANativeWindow pointer = %ld, "
            "group id = %d",
            mANativeWindowPointer, SURFACE_TYPE_VIEWFINDER);
        env->DeleteLocalRef(element);
    }

    // get buffer GraphDescriptor object
    jfieldID graphDescriptorField =
        env->GetFieldID(frameCls, "mGraphDescriptor", "Lcom/xiaomi/engine/GraphDescriptorBean;");
    jobject graphDescriptor = env->GetObjectField(bufferFormat, graphDescriptorField);
    GraphDescriptor descriptor = getGraphDescriptor(env, graphDescriptor);

    jlong sessionHandle = 0;
    if (mCAI_v10 != NULL) {
        MySessionCb_V10 *mCb = new MySessionCb_V10(env->NewGlobalRef(callback));
        LOGI("Before createSession in %s %d mCAI: %p, descriptor: %p", __func__, __LINE__, mCAI_v10,
             &descriptor);
        sessionHandle = (long)mCAI_v10->CreateSession(&descriptor, &outputConf, (MiaSessionCb_V10 *)mCb);
    } else {
        MySessionCb *mCb = new MySessionCb(env->NewGlobalRef(callback));
        LOGI("Before createSession in %s %d mCAI: %p, descriptor: %p", __func__, __LINE__, mCAI,
             &descriptor);
        sessionHandle = (long)mCAI->createSession(&descriptor, &outputConf, (MiaSessionCb *)mCb);
    }
    return sessionHandle;
}

/*
 *
 * @param bufferFormat
 * @param configurationList
 * @param callback
 * @return
 */
JNIEXPORT jlong JNICALL
Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_createSessionByOutputConfigurations(
    JNIEnv *env, jclass obj, jobject bufferFormat, jobject configurationList, jobject callback)
{
    if (mCAI_v10 == NULL && mCAI == NULL) {
        LOGI("gHandle %p mCAI %p,%p !", gHandle, mCAI_v10, mCAI);
        return MIAS_UNKNOWN_ERROR;
    }

    // get buffer width
    jclass frameCls = env->GetObjectClass(bufferFormat);
    jfieldID frame_width = env->GetFieldID(frameCls, "mBufferWidth", "I");
    jint width = env->GetIntField(bufferFormat, frame_width);

    // get buffer height
    jfieldID frame_height = env->GetFieldID(frameCls, "mBufferHeight", "I");
    jint height = env->GetIntField(bufferFormat, frame_height);

    // get buffer format
    jfieldID frame_format = env->GetFieldID(frameCls, "mBufferFormat", "I");
    jint format = env->GetIntField(bufferFormat, frame_format);
    FrameInfo frame;
    frame.width = (unsigned)width;
    frame.height = (unsigned)height;
    frame.format = (MiaFormat)format;
    SessionOutput outputConf;
    outputConf.type = SESSION_OUT_SURFACE;
    outputConf.fInfo = frame;

    // set sink OutputConfiguration ================================================================
    jclass listArray = env->GetObjectClass(configurationList); // get List class
    if (listArray == NULL) {
        LOGE("not find class");
    }
    /* method in class List  */
    jmethodID list_get = env->GetMethodID(listArray, "get", "(I)Ljava/lang/Object;");
    jmethodID list_size = env->GetMethodID(listArray, "size", "()I");
    /* jni invoke list.get to get configurations count */
    int len = static_cast<int>(env->CallIntMethod(configurationList, list_size));
    LOGI("[createSessionByOutputConfigurations]: get configurations: list size is %d callback %p",
         len, callback);
    for (int i = 0; i < len; i++) {
        /* get list the outputConfigurationElement -- OutputConfiguration */
        jobject outputConfigurationElement =
            (jobject)(env->CallObjectMethod(configurationList, list_get, i));
        if (outputConfigurationElement == NULL) {
            JNI_ThrowByName(env, "java/lang/Exception",
                            "fetch list outputConfigurationElement OutputConfiguration failure!");
        }

        // outputConfiguration.getSurface()
        jclass outputConfigurationClazz = env->GetObjectClass(outputConfigurationElement);
        jmethodID getSurfaceMethod =
            env->GetMethodID(outputConfigurationClazz, "getSurface", "()Landroid/view/Surface;");
        jobject surfaceObj = env->CallObjectMethod(outputConfigurationElement, getSurfaceMethod);
        // outputConfiguration.getSurfaceGroupId()
        jmethodID getSurfaceGroupIdMethod =
            env->GetMethodID(outputConfigurationClazz, "getSurfaceGroupId", "()I");
        jint groupId = env->CallIntMethod(outputConfigurationElement, getSurfaceGroupIdMethod);

        void *mANativeWindowPointer = getANativeWindow(env, surfaceObj);
        outputConf.surfaces[groupId] = mANativeWindowPointer;
        LOGD(
            "[createSessionByOutputConfigurations]: get configuration: ANativeWindow pointer = "
            "%ld, "
            "group id = %d",
            mANativeWindowPointer, SURFACE_TYPE_VIEWFINDER);

        env->DeleteLocalRef(outputConfigurationElement);
    }

    // get buffer GraphDescriptor object
    jfieldID graphDescriptorField =
        env->GetFieldID(frameCls, "mGraphDescriptor", "Lcom/xiaomi/engine/GraphDescriptorBean;");
    jobject graphDescriptor = env->GetObjectField(bufferFormat, graphDescriptorField);

    GraphDescriptor descriptor = getGraphDescriptor(env, graphDescriptor);

    LOGI(
        "[createSessionByOutputConfigurations]: createSession in %s + %d; mCAI: %p,%p, descriptor: %p",
        __func__, __LINE__, mCAI_v10, mCAI, &descriptor);

    jlong sessionHandle = 0;
    if (mCAI_v10 != NULL) {
        MySessionCb_V10 *mCb = new MySessionCb_V10(env->NewGlobalRef(callback));
        sessionHandle = (long)mCAI_v10->CreateSession(&descriptor, &outputConf, (MySessionCb_V10 *)mCb);
    } else {
        MySessionCb *mCb = new MySessionCb(env->NewGlobalRef(callback));
        sessionHandle = (long)mCAI->createSession(&descriptor, &outputConf, (MiaSessionCb *)mCb);
    }
    return sessionHandle;
}

JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_processFrame(
    JNIEnv *env, jclass obj, jlong sessionID, jobject frameData, jobject jRequestCallback)
{
    CDKResult res = MIAS_OK;
    if (mCAI_v10 == NULL && mCAI == NULL) {
        LOGI("gHandle %p mCAI %p,%p !", gHandle, mCAI_v10, mCAI);
        return MIAS_UNKNOWN_ERROR;
    }

    MiaFrame mia_frame;
    FrameDataUtil::getMiaFrame(env, &mia_frame, frameData);
    LOGI("[ProcessFrame]: Before ProcessFrame in %s %d mCAI %p,%p", __func__, __LINE__, mCAI_v10, mCAI);
    LOGI("------------------------------------------------------------------");
    LOGI("[ProcessFrame]: stream_index = %d, frame_number = %llu, format = %d * %d, %d",
         mia_frame.stream_index, mia_frame.frame_number, mia_frame.mibuffer.format.width,
         mia_frame.mibuffer.format.height, mia_frame.mibuffer.format.format);
    LOGI("[ProcessFrame]: result_metadata = %p, phycam_metadata = %p", mia_frame.result_metadata,
         mia_frame.phycam_metadata);
    LOGI("------------------------------------------------------------------");
    if (mCAI_v10 != NULL) {
        res = mCAI_v10->ProcessFrame((SessionHandle)sessionID, &mia_frame);
    } else {
        res = mCAI->processFrame((SessionHandle)sessionID, &mia_frame);
    }
    return res;
}

CDKResult Jni_ProcessFrameWithSync_V10(
    JNIEnv *env, jclass obj, jlong sessionID, jobject frameDataList, jobject resultImage,
    jint algo_type)
{
    if (mCAI_v10 == NULL) {
        LOGW("processFrameWithSync: gHandle = %p, mCAI = %p", gHandle, mCAI_v10);
        return MIAS_UNKNOWN_ERROR;
    }
    // get FrameDataList
    jclass listArray = env->GetObjectClass(frameDataList); // get List class
    if(listArray == NULL){
        LOGE("processFrameWithSync: null frame data list");
    }
    /* method in class List  */
    jmethodID list_get = env->GetMethodID(listArray, "get", "(I)Ljava/lang/Object;");
    jmethodID list_size = env->GetMethodID(listArray, "size", "()I");
    /* jni invoke list.get to get configurations count */
    int len = static_cast<int>(env->CallIntMethod(frameDataList, list_size));
    LOGI("processFrameWithSync: list size is %d", len);

    // check list size whether larger than MAX_INPUT_NUM
    if (len > MiMaxInputNum) {
        LOGE("processFrameWithSync: ERR! Overflow! input buffer cnt %d, max is %d", len, MiMaxInputNum);
        JNI_ThrowByName(env, "java/lang/RuntimeException", "We get input buffer overflow max size");
    }

    ProcessRequestInfo *pProcReqInfo =
        (ProcessRequestInfo *)malloc(sizeof(ProcessRequestInfo));
    if (pProcReqInfo == NULL) {
        LOGE("processFrameWithSync: ProcessRequestInfo malloc failed");
        return MIAS_NO_MEM;
    }
    memset(pProcReqInfo, 0, sizeof(ProcessRequestInfo));
    LOGI("processFrameWithSync: pProcReqInfo = %p", pProcReqInfo);

    pProcReqInfo->inputNum = static_cast<uint32_t>(len);
    ProcessAlgoType algType = (ProcessAlgoType)algo_type;
    pProcReqInfo->algoType = algType;

    // add items to list
    for (int i = 0; i < len; i++) {
        /* get list the frameDataElement -- frameData */
        jobject frameDataElement = (jobject)(env->CallObjectMethod(frameDataList, list_get, i));
        if(frameDataElement == NULL) {
            JNI_ThrowByName(env, "java/lang/Exception", "fetch list frameDataElement failure!");
        }

        // covert frameDataElement to mia_frame
        MiaFrame mia_frame;
        FrameDataUtil::getMiaFrame(env, &mia_frame, frameDataElement);

        pProcReqInfo->phInputBuffer[i] = (MiImageList_p)malloc(sizeof(MiImageList_t));
        if (pProcReqInfo->phInputBuffer[i] == NULL) {
            LOGE("processFrameWithSync: phInputBuffer[%d] malloc failed", i);
            return MIAS_NO_MEM;
        }
        //XM_CHECK_NULL_POINTER(pProcReqInfo->phInputBuffer[i], MIAS_NO_MEM);
        LOGI("processFrameWithSync: malloced and initialized MiImageList_t %p, size %d for input buffer %d",
             pProcReqInfo->phInputBuffer[i],
             sizeof(MiImageList_t), i);
        memcpy(pProcReqInfo->phInputBuffer[i], &(mia_frame.mibuffer), sizeof(MiImageList_t));

        LOGI("processFrameWithSync: phInputBuffer[%d] = %p, format = 0x%x, meta = 0x%p",
            i, pProcReqInfo->phInputBuffer[i], pProcReqInfo->phInputBuffer[i]->format.format,
            mia_frame.result_metadata);
        pProcReqInfo->meta[i] = mia_frame.result_metadata;
        if (mia_frame.callback != NULL) {
            delete mia_frame.callback;
        }
    }

    //==============================================================================================

    // 2. prepare out frame buffer
    MiImageList_t miBuffer;
    FrameDataUtil::getMiImageList(env, &miBuffer, resultImage);
    pProcReqInfo->phOutputBuffer[0] = &miBuffer; // current only use the first result
    pProcReqInfo->outputNum = 1;

    LOGD("processFrameWithSync: inputBufCnt = %d, outputBufCnt = %d, miBuffer.addr = %p, phOutputBuffer[0] = %p",
        pProcReqInfo->inputNum, pProcReqInfo->outputNum, &miBuffer, pProcReqInfo->phOutputBuffer[0]);

    CDKResult result = MIAS_OK;
    if(mGetApiVersion) {
        int version = mGetApiVersion();
        LOGI("processFrameWithSync: apiVersion = %d", version);
        if(version >= 2) {
            result = mCAI_v10->ProcessFrameWithSync((SessionHandle) sessionID,(void *)pProcReqInfo);
        }
    } else { //default version, the very first version: v1, struct defined as ProcessRequestInfo_V1
        LOGI("processFrameWithSync: default version v1");
        ProcessRequestInfo_V1 ProcReqInfo_V1;
        ProcReqInfo_V1.frameNum = pProcReqInfo->frameNum;
        ProcReqInfo_V1.algoType = pProcReqInfo->algoType;
        ProcReqInfo_V1.meta = pProcReqInfo->meta[0];      //v1 struct has only one meta ptr
        ProcReqInfo_V1.inputNum = pProcReqInfo->inputNum; //v1 struct has a maxinputnum = 10
        for(int i = 0; i < 10; i++) {
            ProcReqInfo_V1.phInputBuffer[i] = pProcReqInfo->phInputBuffer[i];
        }
        ProcReqInfo_V1.outputNum = pProcReqInfo->outputNum;
        ProcReqInfo_V1.phOutputBuffer[0] = pProcReqInfo->phOutputBuffer[0];
        ProcReqInfo_V1.baseFrameIndex = pProcReqInfo->baseFrameIndex;

        result = mCAI_v10->ProcessFrameWithSync((SessionHandle) sessionID,(void *)&ProcReqInfo_V1);
    }

    /* 1. Free input & output buffers after taking advantage from them */
    for (uint32_t i = 0; i < pProcReqInfo->inputNum; i++) {
        LOGI("processFrameWithSync: free phInputBuffer[%d] %p", i, pProcReqInfo->phInputBuffer[i]);
        free(pProcReqInfo->phInputBuffer[i]);
    }
    int baseFrameIndex = pProcReqInfo->baseFrameIndex;
    LOGI("processFrameWithSync: result = %d, baseFrameIndex = %d", result, baseFrameIndex);

    /* 2. reset buf cnt then we can prepare and initiate a whole new transaction */
    pProcReqInfo->inputNum = 0;
    pProcReqInfo->outputNum = 0;
    free(pProcReqInfo);

    return baseFrameIndex;
}

CDKResult Jni_ProcessFrameWithSync_V11(
    JNIEnv *env, jclass obj, jlong sessionID, jobject frameDataList, jobject resultImage,
    jint algo_type)
{
    if (mCAI == NULL) {
        LOGI("gHandle %p mCAI %p !", gHandle, mCAI);
        return MIAS_UNKNOWN_ERROR;
    }
    LOGD("PutFrameInQueue start %s : %d", __func__, __LINE__);
    // get FrameDataList
    jclass listArray = env->GetObjectClass(frameDataList); // get List class
    if (listArray == NULL) {
        LOGE("not find class");
    }
    /* method in class List  */
    jmethodID list_get = env->GetMethodID(listArray, "get", "(I)Ljava/lang/Object;");
    jmethodID list_size = env->GetMethodID(listArray, "size", "()I");
    /* jni invoke list.get to get configurations count */
    int len = static_cast<int>(env->CallIntMethod(frameDataList, list_size));
    LOGI("get frameDataList: list size is %d", len);

    // check list size whether larger than MAX_INPUT_NUM
    if (len > MiMaxInputNum) {
        LOGE("ERR! Overflow! We get input buffer cnt %d, Max is %d", len, MiMaxInputNum);
        JNI_ThrowByName(env, "java/lang/RuntimeException", "We get input buffer overflow max size");
    }

    ProcessRequestInfo *pProcReqInfo = (ProcessRequestInfo *)malloc(sizeof(ProcessRequestInfo));
    if (pProcReqInfo == NULL) {
        LOGE("pProcReqInfo malloc failed");
        return MIAS_NO_MEM;
    }
    memset(pProcReqInfo, 0, sizeof(ProcessRequestInfo));
    LOGI("pProcReqInfo = %p", pProcReqInfo);

    pProcReqInfo->inputNum = static_cast<uint32_t>(len);
    ProcessAlgoType algType = (ProcessAlgoType)algo_type;
    pProcReqInfo->algoType = algType;

    // add items to list
    for (int i = 0; i < len; i++) {
        /* get list the frameDataElement -- frameData */
        jobject frameDataElement = (jobject)(env->CallObjectMethod(frameDataList, list_get, i));
        if (frameDataElement == NULL) {
            JNI_ThrowByName(env, "java/lang/Exception", "fetch list frameDataElement failure!");
        }

        // covert frameDataElement to mia_frame
        MiaFrame mia_frame;
        FrameDataUtil::getMiaFrame(env, &mia_frame, frameDataElement);

        pProcReqInfo->phInputBuffer[i] = (MiImageList_p)malloc(sizeof(MiImageList_t));
        if (pProcReqInfo->phInputBuffer[i] == NULL) {
            LOGE("phInputBuffer malloc failed");
            return MIAS_NO_MEM;
        }
        LOGI("malloced and initialized MiImageList_t %p, size %d for input buffer %d",
             pProcReqInfo->phInputBuffer[i], sizeof(MiImageList_t), i);
        memcpy(pProcReqInfo->phInputBuffer[i], &(mia_frame.mibuffer), sizeof(MiImageList_t));

        LOGI("phInputBuffer[%d]: %p, format is 0x%x, meta 0x%p", i, pProcReqInfo->phInputBuffer[i],
             pProcReqInfo->phInputBuffer[i]->format.format, mia_frame.result_metadata);
        pProcReqInfo->meta[i] = mia_frame.result_metadata;
        if (mia_frame.callback != NULL) {
            delete mia_frame.callback;
        }
    }

    //==============================================================================================

    // 2. prepare out frame buffer
    MiImageList_t miBuffer;
    FrameDataUtil::getMiImageList(env, &miBuffer, resultImage);
    pProcReqInfo->phOutputBuffer[0] = &miBuffer; // current only use the first result
    pProcReqInfo->outputNum = 1;

    LOGD("input buf cnt %d, output buf cnt %d, miBuffer addr %p, phOutputBuffer[0] %p",
         pProcReqInfo->inputNum, pProcReqInfo->outputNum, &miBuffer,
         pProcReqInfo->phOutputBuffer[0]);

    CDKResult result = mCAI->processFrameWithSync((SessionHandle)sessionID, pProcReqInfo);

    /* 1. Free input & output buffers after taking advantage from them */
    for (uint32_t i = 0; i < pProcReqInfo->inputNum; i++) {
        LOGI("free input buffer %d's space %p", i, pProcReqInfo->phInputBuffer[i]);
        free(pProcReqInfo->phInputBuffer[i]);
    }
    int baseFrameIndex = pProcReqInfo->baseFrameIndex;
    LOGI("%s:%d result: %d, baseFrameIndex: %d", __func__, __LINE__, result, baseFrameIndex);

    /* 2. reset buf cnt then we can prepare and initiate a whole new transaction */
    pProcReqInfo->inputNum = 0;
    pProcReqInfo->outputNum = 0;
    free(pProcReqInfo);

    return baseFrameIndex;
}


// ProcessFrameWithSync will be abandoned
JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_processFrameWithSync(
    JNIEnv *env, jclass obj, jlong sessionID, jobject frameDataList, jobject resultImage,
    jint algo_type)
{
    if (mCAI_v10 == NULL && mCAI == NULL) {
        LOGI("gHandle %p mCAI %p,%p !", gHandle, mCAI_v10, mCAI);
        return MIAS_UNKNOWN_ERROR;
    }

    if (mCAI_v10 != NULL) {
        return Jni_ProcessFrameWithSync_V10(env, obj, sessionID, frameDataList, resultImage, algo_type);
    } else {
        return Jni_ProcessFrameWithSync_V11(env, obj, sessionID, frameDataList, resultImage, algo_type);
    }
}

JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_preProcess(
    JNIEnv *env, jclass obj, jlong sessionId, jobject data)
{
    if (mExtCAI == NULL || mExtCAI->preProcess == NULL) {
        LOGI("preProcess unsupported");
        return 0;
    }

    PreProcessInfo preProcessInfo;
    FrameDataUtil::getPreProcessInfo(env, &preProcessInfo, data);
    return mExtCAI->preProcess((SessionHandle)sessionId, &preProcessInfo);
}

JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_destroySession(JNIEnv *env,
                                                                                   jclass obj,
                                                                                   jlong sessionId)
{
    CDKResult res = MIAS_OK;
    if (mCAI_v10 == NULL && mCAI == NULL) {
        LOGI("gHandle %p mCAI %p,%p !", gHandle, mCAI_v10, mCAI);
        return MIAS_UNKNOWN_ERROR;
    }

    LOGI("destroySession in %s %d mCAI %p,%p", __func__, __LINE__, mCAI_v10, mCAI);
    if (mCAI_v10 != NULL) {
        res = mCAI_v10->DestroySession((SessionHandle) sessionId);
    } else {
        res = mCAI->destroySession((SessionHandle)sessionId);
    }
    return res;
}

JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_flush(JNIEnv *env, jclass obj,
                                                                          jlong sessionId)
{
    CDKResult res = MIAS_OK;
    if (mCAI_v10 == NULL && mCAI == NULL) {
        LOGI("gHandle %p mCAI %p,%p !", gHandle, mCAI_v10, mCAI);
        return MIAS_UNKNOWN_ERROR;
    }
    LOGI("flush in %s %d mCAI %p,%p", __func__, __LINE__, mCAI_v10, mCAI);
    if (mCAI_v10 != NULL) {
        res = mCAI_v10->Flush((SessionHandle) sessionId);
    } else {
        res = mCAI->flush((SessionHandle)sessionId);
    }
    return res;
}

JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_quickFinish
    (JNIEnv *env, jclass obj, jlong sessionId, jlong timestamp, jboolean needResult) {
    if (mCAI_v10 != NULL) {
        //v10框架没有增加quickFinish， 所以V10的JNI更新到202204120这个版本之后，就可能会调用到这个方法，
        //所以在V10上，该方法直接回退到flush
        LOGW("not support quickFinish, use flush");
        return Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_flush(env, obj, sessionId);
    } else {
        if (mCAI == NULL) {
            LOGI("gHandle %p mCAI %p!", gHandle, mCAI);
            return MIAS_UNKNOWN_ERROR;
        }
        bool needImageResult = (needResult == JNI_TRUE) ? true : false;
        if (mExtApiVersion >= 202204120) {
            //框架和JNI的版本号都大于202204120， 正常调用quickFinish方法
            LOGI("do quickFinish");
            return mCAI->quickFinish((SessionHandle)sessionId, (int64_t)timestamp, needImageResult);
        } else {
            //框架小于202204120， 但是JNI更新的版本大于202204120， 这个时候回退到flush方法
            LOGI("version %d not support quickFinish", mExtApiVersion);
            return mCAI->flush((SessionHandle)sessionId);
        }
    }
    return MIAS_UNKNOWN_ERROR;
}

/*
 * The version code of libcamera_algoup_jni.xiaomi.so, which is in the form of yyMMddX.
 * yyMMdd is the date on which the library was updated.
 * X is the sub version. Its initial value is 0, then it increases with versions if more
 * than one versions are released on that day.
 *
 * More detailed information please refers to:
 *     https://wiki.n.miui.com/pages/viewpage.action?pageId=561367433
 */
static const int kVersionCode = 202204120;

JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiCamAlgoInterfaceJNI_getVersionCode(JNIEnv *env,
                                                                                   jclass obj)
{
    return kVersionCode;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved __unused)
{
    jint status = -1;
    status = initGlobalJniVariables(jvm);
    if (status < 0) {
        goto bail;
    }

    status = JNI_VERSION_1_6;

bail:
    return status;
}
