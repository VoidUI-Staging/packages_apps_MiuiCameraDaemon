/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */
#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>

#include "FrameDataUtil.h"
#include "NativeDebug.h"
#include "Util.h"
#include "mianodefactory.h"
#include "com_xiaomi_engine_MiaNodeJNI.h"

#undef LOG_TAG
#define LOG_TAG   "MIA_NODE_JNI"
using namespace mialgo;

static void printMiaFrame(MiaFrame *mia_frame) {
    LOGI("------------------------------------------------------------------");
    LOGI("[putFrameInQueue]: mia_frame->stream_index = %d", mia_frame->stream_index);
    LOGI("[putFrameInQueue]: mia_frame->frame_number = %llu", mia_frame->frame_number);
    LOGI("[putFrameInQueue]: mia_frame->format = %d * %d, %d",
         mia_frame->mibuffer.format.width,
         mia_frame->mibuffer.format.height,
         mia_frame->mibuffer.format.format);
    LOGI("[putFrameInQueue]: mia_frame->result_metadata = %p", mia_frame->result_metadata);
    for (uint32_t i = 0 ; i < mia_frame->mibuffer.numberOfPlanes; i++) {
        LOGI("[putFrameInQueue]: plane %d, pAddr %p, planeSize %zu", i,
             mia_frame->mibuffer.pImageList[0].pAddr[i],
             mia_frame->mibuffer.planeSize[i]);
    }
    LOGI("------------------------------------------------------------------");
}

/*
 * Free the previous transaction's resource
 */
static int releaseRequestRes (MiaNode *mianode, NodeProcessRequestInfo *pNodeProcReqInfo)
{
    LOGD("releaseRequestRes start ");

    /* 1. Free input & output buffers after taking advantage from them */
    for (uint32_t i = 0; i < pNodeProcReqInfo->inputNum; i++) {
        LOGI("free input buffer %d's space %p", i, pNodeProcReqInfo->phInputBuffer[i]);
        free(pNodeProcReqInfo->phInputBuffer[i]);
    }

    /* 2. reset buf cnt then we can prepare and initiate a whole new transaction */
    pNodeProcReqInfo->inputNum = 0;
    pNodeProcReqInfo->outputNum = 0;

    /* 3. Free metadata */
    mianode->FreeMetaData();

    free(pNodeProcReqInfo);
    LOGD("releaseRequestRes end ");
    return MIAS_OK;
}

//================================== JNI interface implement =======================================

JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiaNodeJNI_init(JNIEnv *env, jclass obj, jint mia_node_type)
{
    mia_node_type_t node_type = (mia_node_type_t)mia_node_type;

    LOGI("Init Java_com_xiaomi_engine_MiaNodeJNI_deInit");
    MiaNodeFactory::GetInstance()->Initialize(node_type);
    return MIAS_OK;
}

JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiaNodeJNI_deInit(JNIEnv *env, jclass obj, jint mia_node_type)
{
    mia_node_type_t node_type = (mia_node_type_t)mia_node_type;

    LOGI("Deinit Java_com_xiaomi_engine_MiaNodeJNI_deInit");
    MiaNodeFactory::GetInstance()->DeInit(node_type);
    return MIAS_OK;
}

JNIEXPORT jint JNICALL Java_com_xiaomi_engine_MiaNodeJNI_processRequest
        (JNIEnv *env, jclass obj, jobject frameDataList, jobject resultImage, jint mia_node_type, jboolean dualCamera)
{
    LOGD("ProcessRequest start >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    LOGD("PutFrameInQueue start %s : %d", __func__, __LINE__);
    int baseIndex = 0;
    // get FrameDataList
    jclass listArray = env->GetObjectClass(frameDataList); // get List class
    if (listArray == NULL) {
        LOGE("not find class");
        return baseIndex;
    }
    /* method in class List  */
    jmethodID list_get = env->GetMethodID(listArray, "get", "(I)Ljava/lang/Object;");
    jmethodID list_size = env->GetMethodID(listArray, "size", "()I");
    /* jni invoke list.get to get configurations count */
    int len = static_cast<int>(env->CallIntMethod(frameDataList, list_size));
    LOGI("get frameDataList: list size is %d", len);

    // check list size whether larger than MAX_INPUT_NUM
    if (len > MAX_INPUT_NUM) {
        LOGE("ERR! Overflow! We get input buffer cnt %d, Max is %d", len, MAX_INPUT_NUM);
        JNI_ThrowByName(env, "java/lang/RuntimeException", "We get input buffer overflow max size");
        return baseIndex;
    }

    // get pNodeProcReqInfo from MiaNodeFactory
    mia_node_type_t node_type = (mia_node_type_t)mia_node_type;
    MiaNode *mianode = MiaNodeFactory::GetInstance()->GetMiaNode(node_type);
    XM_CHECK_NULL_POINTER(mianode, MIAS_INVALID_CALL);

    NodeProcessRequestInfo *pNodeProcReqInfo =
            (NodeProcessRequestInfo *)malloc(sizeof(NodeProcessRequestInfo));
    memset(pNodeProcReqInfo, 0, sizeof(NodeProcessRequestInfo));

    XM_CHECK_NULL_POINTER(pNodeProcReqInfo, MIAS_INVALID_CALL);
    LOGI("pNodeProcReqInfo = %p", pNodeProcReqInfo);

    pNodeProcReqInfo->inputNum = static_cast<uint32_t>(len);
    if (dualCamera) {
        pNodeProcReqInfo->cropBoundaries = 0;
    } else {
        pNodeProcReqInfo->cropBoundaries = 1;
    }

    // add items to list
    for (int i = 0; i < len; i++) {
        /* get list the frameDataElement -- frameData */
        jobject frameDataElement = (jobject)(env->CallObjectMethod(frameDataList, list_get, i));
        if (frameDataElement == NULL) {
            JNI_ThrowByName(env, "java/lang/Exception", "fetch list frameDataElement failure!");
            for (int j = 0; j < i; j++) {
                LOGI("free input buffer %d's space %p", j, pNodeProcReqInfo->phInputBuffer[j]);
                free(pNodeProcReqInfo->phInputBuffer[j]);
            }
            free(pNodeProcReqInfo);
            return baseIndex;
        }

        // covert frameDataElement to mia_frame
        MiaFrame mia_frame;
        FrameDataUtil::getMiaFrame(env, &mia_frame, frameDataElement);
        // to debug
        // printMiaFrame(&mia_frame);

        // 此处申请memory的原因在于，mia_frame 在方法执行完之后，对应的结构体会被释放，
        // 导致后面赋值给pNodeProcReqInfo->phInputBuffer[i]时，在方法执行完后，对应的地址的值已经被覆盖，
        // 出现phInputBuffer指向的地址的值发生改变的情况。
        // TODO 后期可能需要将 NodeProcessRequestInfo 中的MiImageList_p改为MiImageList_t
        pNodeProcReqInfo->phInputBuffer[i] = (MiImageList_p)malloc(sizeof(MiImageList_t));
        if(pNodeProcReqInfo->phInputBuffer[i] == NULL){
            for (int j = 0; j < i; j++) {
                LOGI("free input buffer %d's space %p", j, pNodeProcReqInfo->phInputBuffer[j]);
                free(pNodeProcReqInfo->phInputBuffer[j]);
            }
            free(pNodeProcReqInfo);
            return baseIndex;
        }
        LOGI("malloced and initialized MiImageList_t %p, size %d for input buffer %d",
             pNodeProcReqInfo->phInputBuffer[i],
             sizeof(MiImageList_t), i);
        memcpy(pNodeProcReqInfo->phInputBuffer[i], &(mia_frame.mibuffer), sizeof(MiImageList_t));

        LOGI("phInputBuffer[%d]: %p, format is 0x%x", i,
            pNodeProcReqInfo->phInputBuffer[i],
            pNodeProcReqInfo->phInputBuffer[i]->format.format);
        pNodeProcReqInfo->meta = mia_frame.result_metadata;
    }

    //==============================================================================================

    // TODO: process will cost 330ms, the time is too long.
    // 1. prepare meta data
    mianode->PrePareMetaData(pNodeProcReqInfo->meta);

    // 2. prepare out frame buffer
    MiImageList_t miBuffer;
    FrameDataUtil::getMiImageList(env, &miBuffer, resultImage);
    pNodeProcReqInfo->phOutputBuffer[0] = &miBuffer; // current only use the first result
    pNodeProcReqInfo->outputNum = 1;

    LOGD("input buf cnt %d, output buf cnt %d, miBuffer addr %p, phOutputBuffer[0] %p",
        pNodeProcReqInfo->inputNum, pNodeProcReqInfo->outputNum, &miBuffer, pNodeProcReqInfo->phOutputBuffer[0]);

    mianode->ProcessRequest(pNodeProcReqInfo);
    baseIndex = pNodeProcReqInfo->baseFrameIndex;

    // release pNodeProcReqInfo
    releaseRequestRes(mianode, pNodeProcReqInfo);

    LOGD("ProcessRequest end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
    return baseIndex;
}

