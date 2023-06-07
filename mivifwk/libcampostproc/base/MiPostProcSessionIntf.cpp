/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiPostProcSessionIntf.h"

#include <android/hardware_buffer_jni.h>
#include <android_media_Utils.h>
#include <cutils/native_handle.h>
#include <hardware/gralloc.h>
#include <system/camera_metadata.h>
#include <ui/GraphicBuffer.h>
#include <ui/Rect.h>

#include "MiPostProcServiceIntf.h"
#include "VendorMetadataParser.h"
#include "gr_priv_handle.h"

// Whether to get the output buffer by getOutputBuffer callback
#define USE_GETOUTPUT_BYCALLBACK 1

/** buffer is used as a camera HAL output */
const uint64_t CAMERA_OUTPUT = (1ULL << 17);
/** buffer is used as a camera HAL input */
const uint64_t CAMERA_INPUT = (1ULL << 18);

namespace mialgo {

extern sp<IMiPostProcService> gRemoteService;
extern uint8_t bServiceDied;

static void convertToHidl(const camera_metadata_t *src, MiCameraMetadata *dst)
{
    if (src == nullptr) {
        return;
    }
    size_t size = get_camera_metadata_size(src);
    dst->setToExternal((uint8_t *)src, size);
}

uint32_t getBufferSize(buffer_handle_t bufferHandle)
{
    uint32_t size = 0;
    struct private_handle_t *handle = NULL;

    if (NULL != bufferHandle) {
        handle =
            reinterpret_cast<struct private_handle_t *>(const_cast<native_handle *>(bufferHandle));
        if (NULL != handle) {
            size = handle->size;
        }
    }
    return size;
}

void *getCPUAddress(buffer_handle_t bufferHandle, int size)
{
    void *virtualAddress = NULL;
    if (NULL != bufferHandle) {
        // bufferHandle is a gralloc handle
        MLOGI("bufferHandle = %p, fd = %d", bufferHandle,
              reinterpret_cast<const native_handle_t *>(bufferHandle)->data[0]);

        virtualAddress = mmap(NULL, size, (PROT_READ | PROT_WRITE), MAP_SHARED,
                              reinterpret_cast<const native_handle_t *>(bufferHandle)->data[0], 0);

        if ((NULL == virtualAddress) || (reinterpret_cast<void *>(-1) == virtualAddress)) {
            MLOGW("MIA Failed in getting virtual address, bufferHandle=%p, virtualAddress = %p",
                  bufferHandle, virtualAddress);

            // in case of mmap failures, it will return -1, not NULL. So, set to NULL so that
            // callers just validate on NULL
            virtualAddress = NULL;
        }
    } else {
        MLOGE("Buffer handle is NULL, bufferHandle=%p", bufferHandle);
    }

    return virtualAddress;
}

void putCPUAddress(buffer_handle_t bufferHandle, void *virtualAddress, int size)
{
    if ((bufferHandle != NULL) && (virtualAddress != NULL)) {
        munmap(virtualAddress, size);
    } else {
        MLOGE("virtualAddress is NULL");
    }
}

SurfaceWrapper::SurfaceWrapper(ANativeWindow *window, SurfaceType type)
    : mANWindow(window), mWidth(0), mHeight(0), mFormat(0), mType(type)
{
    // mANWindowBufferQ.clear();
    mANWindowBufferQs.clear();
}

SurfaceWrapper::~SurfaceWrapper()
{
    // TODO: ensure all surface buffer returned
    mANWindowBufferQs.clear();
}

CDKResult SurfaceWrapper::setup()
{
    int res;

    if (!mANWindow) {
        MLOGE("invalid surface");
        return MIAS_INVALID_PARAM;
    }

    // setup, usage force to this
    uint64_t usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN |
                     CAMERA_OUTPUT | CAMERA_INPUT;
    res = ANativeWindow_setUsage(mANWindow, usage);
    if (res != MIAS_OK) {
        MLOGE("Unable to configure usage %lx", usage);
        return res;
    }

    // determine buffer reservation in MIA.
    int cBuffNum;
    res = ANativeWindow_query(mANWindow, ANATIVEWINDOW_QUERY_MIN_UNDEQUEUED_BUFFERS, &cBuffNum);
    if (res != MIAS_OK) {
        MLOGE("Unable to query consumer undequeued buffer count");
        return res;
    }
    MLOGD("Consumer holds %d buffers", cBuffNum);
    res = ANativeWindow_setBufferCount(mANWindow, cBuffNum + 2);
    if (res != MIAS_OK) {
        MLOGE("Unable to set buffer count");
        return res;
    }

    // get default native window properties wxh format
    res = ANativeWindow_query(mANWindow, ANATIVEWINDOW_QUERY_DEFAULT_WIDTH, (int *)&mWidth);
    res = ANativeWindow_query(mANWindow, ANATIVEWINDOW_QUERY_DEFAULT_HEIGHT, (int *)&mHeight);
    if (res != MIAS_OK) {
        MLOGE("Unable to query window properties %dx%d", mWidth, mHeight);
        return MIAS_INVALID_PARAM;
    }

    // default format query is not supported in vndk interface. MIA determines it.
    if (mType == SURFACE_TYPE_SNAPSHOT_MAIN || mType == SURFACE_TYPE_SNAPSHOT_AUX) {
        mFormat = AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420; // 0x23 NV12
    } else if (mType == SURFACE_TYPE_SNAPSHOT_DEPTH) {
        mFormat = AHARDWAREBUFFER_FORMAT_Y16; // Y16
    }
    res = ANativeWindow_setBuffersFormat(mANWindow, mFormat);
    ANativeWindow_setDequeueTimeout(mANWindow, milliseconds_to_nanoseconds(4000));

    MLOGD("X window format:0x%x wxh:%dx%d", mFormat, mWidth, mHeight);
    return MIAS_OK;
}

const native_handle_t *SurfaceWrapper::dequeueBuffer(int64_t timestamp)
{
    // int res;
    int fenceFd = -1, res;
    ANativeWindowBuffer *anb = NULL;

    // dequeue an native window buffer and make it to a MiaImage
    res = ANativeWindow_dequeueBuffer(mANWindow, &anb, &fenceFd);
    if (res != MIAS_OK || anb == NULL) {
        MLOGE("Can't dequeue next output buffer: %s (%d)", strerror(-res), res);
        return NULL;
    }
    MLOGD("[%d]dequeueBuffer got anb: %p, %dx%d format:%x fenceFd:%d, timestamp: %ld", mType, anb,
          anb->width, anb->height, anb->format, fenceFd, timestamp);
    if (fenceFd != -1) {
        MLOGW("fence to be waited and closed, fixme!!!");
    }

    Mutex::Autolock autoLock(mLock);
    // check if it's a known buffer
    std::map<ANativeWindowBuffer *, int64_t>::iterator it;
    it = mANWindowBufferQs.find(anb);
    if (it == mANWindowBufferQs.end()) {
        mANWindowBufferQs[anb] = timestamp;
    } else {
        anb = it->first;
    }

    const native_handle_t *nativeHandle = nullptr;
    AHardwareBuffer *hardwareBuffer = ANativeWindowBuffer_getHardwareBuffer(anb);
    if (hardwareBuffer != nullptr) {
        nativeHandle = AHardwareBuffer_getNativeHandle(hardwareBuffer);
    }
    if (nativeHandle == nullptr) {
        MLOGE("error nativeHandle is null");
        return NULL;
    }

#if 0
    sp<GraphicBuffer> gBuffer = reinterpret_cast<GraphicBuffer*>(hardwareBuffer);
    const Rect rect { 0, 0, static_cast<int32_t>(gBuffer->getWidth()), static_cast<int32_t>(gBuffer->getHeight()) };
    android::LockedImage lockedImg = android::LockedImage();
    res = lockImageFromBuffer(gBuffer, GRALLOC_USAGE_SW_WRITE_OFTEN, rect, fenceFd, &lockedImg);
    if (res != OK) {
        MLOGE("lock buffer failed for format 0x%x", gBuffer->getPixelFormat());
        return NULL;
    }
#endif

    return nativeHandle;
}

CDKResult SurfaceWrapper::enqueueBuffer(int64_t timestamp)
{
    Mutex::Autolock autoLock(mLock);
    // locate the ANativeWindowBuffer using MiaImage. This can be optimized
    std::map<ANativeWindowBuffer *, int64_t>::iterator it;
    for (it = mANWindowBufferQs.begin(); it != mANWindowBufferQs.end(); it++) {
        if (it->second == timestamp)
            break;
    }
    if (it == mANWindowBufferQs.end()) {
        MLOGE("didn't find corresponding anb for timestamp:%ld", timestamp);
        return MIAS_UNKNOWN_ERROR;
    }

    ANativeWindow_setBuffersTimestamp(mANWindow, timestamp);
    // enqueue this buffer to window, no fence after release
    int res = ANativeWindow_queueBuffer(mANWindow, it->first, -1);
    if (res != MIAS_OK) {
        MLOGE("queueBuffer error:%s", strerror(-res));
        return MIAS_UNKNOWN_ERROR;
    }
    MLOGD("[%d]enqueueBuffer done WindowBuffer %p, timestamp %ld", mType, it->first, timestamp);
    mANWindowBufferQs.erase(it);

    return MIAS_OK;
}

MiPostProcSessionIntf::MiPostProcSessionIntf()
    : mAcitveSurfaceCnt(0),
      mSessionCb(0),
      mCameraMode(0),
      mOperationMode(0),
      mEngineBypass(false),
      mIsBokeh(0),
      mNeedInputNumber(0),
      mInputTimes(0),
      mLastRequestName("")
{
    MLOGD("MiaSession construct");
    mProcessStatusError = false;
    CLEAR(mANWindows);
}

MiPostProcSessionIntf::~MiPostProcSessionIntf()
{
    MLOGD("MiPostProcSessionIntf disconstruct");
    if (!bServiceDied) {
        mPostSessionsProxy.clear();
        mPostSessionsProxy = NULL;
    }
}

// postproc interface complete
CDKResult MiPostProcSessionIntf::initialize(GraphDescriptor *gd, SessionOutput *output,
                                            MiaSessionCb *cb)
{
    if (gd == NULL || output == NULL || gd == NULL) {
        MLOGE("invalid argument");
        return MIAS_INVALID_PARAM;
    }
    MLOGD("### cameraMode 0x%x, opMode 0x%x", gd->camera_mode, gd->operation_mode);

    CDKResult ret = buildSurface(output->surfaces);
    if (ret != MIAS_OK) {
        MLOGE("build surface failed");
        return ret;
    }

    mSessionCb = cb;
    mOperationMode = gd->operation_mode;
    mProcessedFrameWrpMap.clear();

    CreateSessionParams createParams;
    createParams.cameraMode = mCameraMode = gd->camera_mode;
    // workaround for no need modify app code
    if (gd->operation_mode != 0) {
        createParams.operationMode = gd->operation_mode;
    } else if (gd->camera_mode == CAM_COMBMODE_REAR_SAT_WT ||
               gd->camera_mode == CAM_COMBMODE_REAR_ULTRA ||
               gd->camera_mode == CAM_COMBMODE_REAR_MACRO ||
               gd->camera_mode == CAM_COMBMODE_REAR_WIDE ||
               gd->camera_mode == CAM_COMBMODE_REAR_SAT_WU ||
               gd->camera_mode == CAM_COMBMODE_REAR_SAT_T_UT) {
        createParams.operationMode = mOperationMode = StreamConfigModeSAT;
    } else if ((gd->camera_mode == CAM_COMBMODE_FRONT ||
                gd->camera_mode == CAM_COMBMODE_FRONT_AUX) &&
               gd->operation_mode == StreamConfigModeNormal) {
        createParams.operationMode = mOperationMode = StreamConfigModeMiuiZslFront;
    }

    while (!mTimestampQueue.empty()) {
        mTimestampQueue.pop();
    }
    while (!mStreamIdxQueue.empty()) {
        mStreamIdxQueue.pop();
    }
    mInputTimes = 0;
    mNeedInputNumber = 0;
    mIsBokeh = 0;
    if (mOperationMode == StreamConfigModeBokeh) {
        if (mCameraMode == CAM_COMBMODE_FRONT_BOKEH || mCameraMode == CAM_COMBMODE_FRONT) {
            mIsBokeh = 1;
        } else {
            mIsBokeh = 2;
        }
    }

    createParams.inputParams.resize(1);
    createParams.inputParams[0].format = output->fInfo.format;
    createParams.inputParams[0].width = output->fInfo.width;
    createParams.inputParams[0].height = output->fInfo.height;

    createParams.outputParams.resize(mAcitveSurfaceCnt);
    for (int i = 0; i < mAcitveSurfaceCnt; i++) {
        SurfaceWrapper *activeSurface = mANWindows[i];
        if (activeSurface == NULL) {
            MLOGE("ERROR! mANWindows[%d] is null", i);
            return MIAS_INVALID_PARAM;
        }
        createParams.outputParams[i].format = activeSurface->getFormat();
        createParams.outputParams[i].width = activeSurface->getWidth();
        createParams.outputParams[i].height = activeSurface->getHeight();
    }

    if (mEngineBypass) {
        mPostSessionsProxy = NULL;
    } else {
        sp<MiPostProcCallBacks> callbacks = new MiPostProcCallBacks();
        callbacks->setSessionHandle(this);
        sp<IMiPostProcSession> postprocessor =
            gRemoteService->createPostProcessor(createParams, callbacks);
        if (postprocessor.get() == NULL) {
            MLOGE("createPostProcessor failed");
            return MIAS_INVALID_PARAM;
        }
        mPostSessionsProxy = postprocessor;

        std::shared_ptr<MetadataQueue> &logicQueue = mLogicMetadataQueue;
        auto logicQueueRet =
            mPostSessionsProxy->getLogicMetadataQueue([&logicQueue](const auto &descriptor) {
                logicQueue = std::make_shared<MetadataQueue>(descriptor);
                if (!logicQueue->isValid() || logicQueue->availableToWrite() <= 0) {
                    MLOGE("MiPostProcService returns empty request metadata fmq, not use it");
                    logicQueue = nullptr;
                    // don't use the logicQueue onwards.
                }
            });
        if (!logicQueueRet.isOk()) {
            MLOGE("Transaction error when getting request metadata fmq: %s, not use it",
                  logicQueueRet.description().c_str());
            return MIAS_INVALID_PARAM;
        }

        std::shared_ptr<MetadataQueue> &phyQueue = mPhysicalMetadataQueue;
        auto phyQueueRet =
            mPostSessionsProxy->getPhysicalMetadataQueue([&phyQueue](const auto &descriptor) {
                phyQueue = std::make_shared<MetadataQueue>(descriptor);
                if (!phyQueue->isValid() || phyQueue->availableToWrite() <= 0) {
                    MLOGE("MiPostProcService returns empty request metadata fmq, not use it");
                    phyQueue = nullptr;
                    // don't use the phyQueue onwards.
                }
            });
        if (!phyQueueRet.isOk()) {
            MLOGE("Transaction error when getting request metadata fmq: %s, not use it",
                  phyQueueRet.description().c_str());
            return MIAS_INVALID_PARAM;
        }

        // for preprocess
        std::shared_ptr<MetadataQueue> &metaQueue = mMetadataQueue;
        auto metaQueueRet =
            mPostSessionsProxy->getMetadataQueue([&metaQueue](const auto &descriptor) {
                metaQueue = std::make_shared<MetadataQueue>(descriptor);
                if (!metaQueue->isValid() || metaQueue->availableToWrite() <= 0) {
                    MLOGE("MiPostProcService returns empty request metadata fmq, not use it");
                    metaQueue = nullptr;
                    // don't use the metaQueue onwards.
                }
            });
        if (!metaQueueRet.isOk()) {
            MLOGE("Transaction error when getting request metadata fmq: %s, not use it",
                  metaQueueRet.description().c_str());
            return MIAS_INVALID_PARAM;
        }
    }

    return MIAS_OK;
}

void MiPostProcSessionIntf::bypassProcess()
{
    int64_t timestamp = 0;
    uint32_t streamIdx = 0;
    if (!mTimestampQueue.empty() && !mStreamIdxQueue.empty()) {
        timestamp = mTimestampQueue.front();
        streamIdx = mStreamIdxQueue.front();
    }

    sp<MiaFrameWrapper> firstInputFrameWrapper = NULL;
    auto itCb = mProcessedFrameWrpMap.find(timestamp + streamIdx);
    if (itCb != mProcessedFrameWrpMap.end()) {
        firstInputFrameWrapper = itCb->second;
    }
    MiaFrame *frame = firstInputFrameWrapper->getMiaFrame();

    const native_handle_t *outputHandle = NULL;
    // get all output buffer
    if (mIsBokeh == 0) {
        outputHandle = dequeuBuffer((SurfaceType)0, timestamp);
    } else if (mIsBokeh == 1) {
        outputHandle = dequeuBuffer((SurfaceType)0, timestamp);
        dequeuBuffer((SurfaceType)1, timestamp);
        dequeuBuffer((SurfaceType)2, timestamp);
    } else if (mIsBokeh == 2) {
        outputHandle = dequeuBuffer((SurfaceType)0, timestamp);
        dequeuBuffer((SurfaceType)1, timestamp);
        dequeuBuffer((SurfaceType)2, timestamp);
    }

    const native_handle_t *inputHandle =
        reinterpret_cast<const native_handle_t *>(frame->mibuffer.nativeHandle);
    uint32_t inputBufferSize = getBufferSize(inputHandle);
    void *inputAddress = getCPUAddress(inputHandle, inputBufferSize);

    uint32_t outputBufferSize = getBufferSize(outputHandle);
    void *outputAddress = getCPUAddress(outputHandle, outputBufferSize);
    if ((inputAddress != NULL) && (outputAddress != NULL)) { // Just copy the first frame
        uint32_t copySize = inputBufferSize > outputBufferSize ? outputBufferSize : inputBufferSize;
        memcpy(outputAddress, inputAddress, copySize);
    }

    putCPUAddress(inputHandle, inputAddress, inputBufferSize);
    putCPUAddress(outputHandle, outputAddress, outputBufferSize);
    inputAddress = outputAddress = NULL;
    inputHandle = outputHandle = NULL;

    // release all input buffer
    for (int i = 0; i < mNeedInputNumber; ++i) {
        if (!mTimestampQueue.empty() && !mStreamIdxQueue.empty()) {
            timestamp = mTimestampQueue.front();
            mTimestampQueue.pop();
            streamIdx = mStreamIdxQueue.front();
            mStreamIdxQueue.pop();
            enqueueBuffer(CallbackType::SOURCE_CALLBACK, timestamp, streamIdx);
        }
    }
    if (mIsBokeh == 2) {
        if (!mTimestampQueue.empty() && !mStreamIdxQueue.empty()) {
            enqueueBuffer(CallbackType::SOURCE_CALLBACK, mTimestampQueue.front(),
                          mStreamIdxQueue.front());
            mTimestampQueue.pop();
            mStreamIdxQueue.pop();
        } else {
            MLOGE("mialgoengine bypass dualBokeh input error!");
        }
    }
    // release and return all output buffer
    if (mIsBokeh == 0) {
        enqueueBuffer(CallbackType::SINK_CALLBACK, timestamp, 0);
    } else if (mIsBokeh == 1) {
        enqueueBuffer(CallbackType::SINK_CALLBACK, timestamp, 0);
        enqueueBuffer(CallbackType::SINK_CALLBACK, timestamp, 1);
        enqueueBuffer(CallbackType::SINK_CALLBACK, timestamp, 2);
    } else if (mIsBokeh == 2) {
        enqueueBuffer(CallbackType::SINK_CALLBACK, timestamp, 0);
        enqueueBuffer(CallbackType::SINK_CALLBACK, timestamp, 1);
        enqueueBuffer(CallbackType::SINK_CALLBACK, timestamp, 2);
    }
}

void MiPostProcSessionIntf::bypass(const sp<MiaFrameWrapper> &frameWrp)
{
    bool doBypass = true;

    if (mNeedInputNumber == frameWrp->getMergeInputNumber()) {
        if (mIsBokeh == 2) {
            if ((mInputTimes % 2) == 1) {
                doBypass = false;
            }
        }
        if (doBypass) {
            bypassProcess();
        }
        mNeedInputNumber = 0;
    }
}

CDKResult MiPostProcSessionIntf::processFrame(MiaFrame *frame)
{
    if (frame == NULL) {
        MLOGE("NULL frame");
        return MIAS_INVALID_PARAM;
    }

    sp<MiaFrameWrapper> miaFrameWrp = new MiaFrameWrapper(frame);
    uint32_t streamIdx = frame->stream_index;
    int64_t timeStamp = miaFrameWrp->getTimestampUs();

    MLOGI(
        "frame_number %lu, requestId %d W*H: %d*%d, stride*scanline: %d*%d, format:%d, "
        "streamIdx: %d, timestamp: %" PRIu64,
        frame->frame_number, frame->requestId, frame->mibuffer.format.width,
        frame->mibuffer.format.height, frame->mibuffer.planeStride, frame->mibuffer.sliceHeight,
        frame->mibuffer.format.format, frame->stream_index, timeStamp);

    SessionProcessParams encParams = {0};

    HandleParams inFrame = {0};
    MiaFrame *newframe = miaFrameWrp->getMiaFrame();
    inFrame.format = newframe->mibuffer.format.format;
    inFrame.width = newframe->mibuffer.format.width;
    inFrame.height = newframe->mibuffer.format.height;
    const hidl_handle in_hidl(
        static_cast<const native_handle_t *>(newframe->mibuffer.nativeHandle));
    inFrame.bufHandle = in_hidl;
    if (mProcessStatusError) {
        MLOGE("service is dead");
        return MIAS_INVALID_PARAM;
    }

    MiCameraMetadata settings, phySettings;

    // Write metadata to FMQ.
    if (newframe->result_metadata != nullptr) {
        size_t settingsSize = get_camera_metadata_size(
            static_cast<const camera_metadata_t *>(newframe->result_metadata));
        if (mLogicMetadataQueue != nullptr &&
            mLogicMetadataQueue->write(reinterpret_cast<const uint8_t *>(newframe->result_metadata),
                                       settingsSize)) {
            inFrame.metadata.resize(0);
            inFrame.fmqSettingsSize = settingsSize;
        } else {
            if (mLogicMetadataQueue != nullptr) {
                MLOGW("couldn't utilize fmq, fallback to hwbinder");
            }

            convertToHidl(static_cast<const camera_metadata_t *>(newframe->result_metadata),
                          &inFrame.metadata);
            inFrame.fmqSettingsSize = 0u;
        }
    } else {
        // A null metadata maps to a size-0 MiCameraMetadata
        inFrame.metadata.resize(0);
        inFrame.fmqSettingsSize = 0u;
    }

    if (newframe->phycam_metadata != nullptr) {
        size_t phySettingsSize = get_camera_metadata_size(
            static_cast<const camera_metadata_t *>(newframe->phycam_metadata));
        if (mPhysicalMetadataQueue != nullptr &&
            mPhysicalMetadataQueue->write(
                reinterpret_cast<const uint8_t *>(newframe->phycam_metadata), phySettingsSize)) {
            inFrame.phycamMetadata.resize(0);
            inFrame.fmqPhySettingsSize = phySettingsSize;
        } else {
            if (mPhysicalMetadataQueue != nullptr) {
                MLOGW("couldn't utilize fmq, fallback to hwbinder");
            }
            convertToHidl(static_cast<const camera_metadata_t *>(newframe->phycam_metadata),
                          &inFrame.phycamMetadata);
            inFrame.fmqPhySettingsSize = 0u;
        }
    } else {
        // A null phycamMetadata maps to a size-0 MiCameraMetadata
        inFrame.phycamMetadata.resize(0);
        inFrame.fmqPhySettingsSize = 0u;
    }

    encParams.input = inFrame;
    {
        bool newRequest = false;
        if (mLastRequestName != miaFrameWrp->getFileName()) {
            newRequest = true;
        }

        Mutex::Autolock autoLock(mTaskLock);

        auto itCb = mProcessedFrameWrpMap.find(timeStamp + streamIdx);
        if (itCb != mProcessedFrameWrpMap.end()) {
            MLOGE("ERROR! it has been processed, timestamp: %" PRIu64 ", streamIdx: %d", timeStamp,
                  streamIdx);
            return MIAS_INVALID_PARAM;
        } else {
            mProcessedFrameWrpMap[timeStamp + streamIdx] = miaFrameWrp;
        }

        if (newRequest) {
            mLastRequestName = miaFrameWrp->getFileName();
            mFirstTimestampsOfRequests[timeStamp] = mLastRequestName;
        }
    }

    if (mEngineBypass) {
        Mutex::Autolock autoLock(mBypassLock);

        ++mInputTimes;
        ++mNeedInputNumber;
        mTimestampQueue.push(timeStamp);
        mStreamIdxQueue.push(streamIdx);

        bypass(miaFrameWrp);
    } else {
        encParams.streamId = newframe->stream_index;
        encParams.frameNum = newframe->frame_number;
        encParams.timeStampUs = timeStamp;

        Error error = Error::INVALID_HANDLE;
        uint32_t requestId = 0;
        auto hidl_cb = [&](const auto reqId, const auto tmpError) {
            if (Error::NONE != tmpError) {
                error = tmpError;
            } else {
                error = Error::NONE;
                requestId = reqId;
            }
        };

        Return<void> ret = mPostSessionsProxy->process(encParams, hidl_cb);
        if (!ret.isOk()) {
            MLOGE("hidl process failed");
            return MIAS_INVALID_PARAM;
        }
    }

    return MIAS_OK;
}

CDKResult MiPostProcSessionIntf::preProcessFrame(PreProcessInfo *params)
{
    if (params == NULL) {
        MLOGE("NULL params");
        return MIAS_INVALID_PARAM;
    }
    PreProcessParams encParams = {0};
    camera_metadata_t *metadata = NULL;

    if (params->meta) {
        metadata = VendorMetadataParser::allocateCopyMetadata(params->meta);
    } else {
        MLOGW("preProcess metadata is null");
    }
    if (mProcessStatusError) {
        MLOGE("service is dead");
        return MIAS_INVALID_PARAM;
    }

    MiCameraMetadata settings;
    // Write metadata to FMQ.
    if (metadata != nullptr) {
        size_t settingsSize =
            get_camera_metadata_size(static_cast<const camera_metadata_t *>(metadata));
        if (mMetadataQueue != nullptr &&
            mMetadataQueue->write(reinterpret_cast<const uint8_t *>(metadata), settingsSize)) {
            encParams.meta.resize(0);
            encParams.fmqSettingsSize = settingsSize;
        } else {
            if (mMetadataQueue != nullptr) {
                MLOGW("couldn't utilize fmq, fallback to hwbinder");
            }
            convertToHidl(static_cast<const camera_metadata_t *>(metadata), &encParams.meta);
            encParams.fmqSettingsSize = 0u;
        }
    } else {
        // A null metadata maps to a size-0 MiCameraMetadata
        encParams.meta.resize(0);
        encParams.fmqSettingsSize = 0u;
    }

    encParams.width = params->width;
    encParams.height = params->height;
    encParams.format = params->format;

    Return<void> ret = mPostSessionsProxy->preProcess(encParams);
    if (!ret.isOk()) {
        MLOGE("hidl process failed");
        return MIAS_INVALID_PARAM;
    }
    if (NULL != metadata) {
        VendorMetadataParser::freeMetadata(metadata);
        metadata = NULL;
    }
    return MIAS_OK;
}

CDKResult MiPostProcSessionIntf::processFrameWithSync(ProcessRequestInfo *processRequestInfo)
{
    CDKResult ret = MIAS_OK;
    MLOGE("processFrameWithSync not supported");
    return ret;
}

void MiPostProcSessionIntf::setProcessStatus()
{
    mProcessStatusError = true;
}

CDKResult MiPostProcSessionIntf::destory()
{
    MLOGD("E %p bServiceDied %d", this, bServiceDied);

    if ((!bServiceDied) && (mPostSessionsProxy != NULL)) {
        Return<Error> ret = mPostSessionsProxy->flush(true);
        if (ret != Error::NONE) {
            MLOGE("Forced flush session failed!");
        }
    }

    for (int i = 0; i < SURFACE_TYPE_COUNT; i++) {
        if (mANWindows[i] != NULL) {
            delete mANWindows[i];
            mANWindows[i] = NULL;
        }
    }

    if (mSessionCb) {
        delete mSessionCb;
        mSessionCb = NULL;
    }

    return MIAS_OK;
}

void MiPostProcSessionIntf::flush()
{
    if (bServiceDied) {
        return;
    }

    MLOGD("E %p", this);
    if (mPostSessionsProxy != NULL) {
        Return<Error> ret = mPostSessionsProxy->flush(false);
        if (ret != Error::NONE) {
            MLOGE("Flush session failed!");
        }
    }

    MLOGD("X %p", this);
}

CDKResult MiPostProcSessionIntf::quickFinish(int64_t timeStamp, bool needImageResult)
{
    CDKResult result = MIAS_OK;
    if (bServiceDied) {
        result = MIAS_INVALID_CALL;
    }

    std::string fileName = "";
    {
        Mutex::Autolock autoLock(mTaskLock);
        auto it = mFirstTimestampsOfRequests.find(timeStamp);
        if (it != mFirstTimestampsOfRequests.end()) {
            fileName = it->second;
        }
    }

    MLOGD("E %p", this);
    if (mPostSessionsProxy != NULL && fileName != "") {
        mPostSessionsProxy->quickFinish(fileName, needImageResult);
    } else {
        result = MIAS_INVALID_CALL;
    }

    MLOGD("X %p", this);
    return result;
}

void MiPostProcSessionIntf::returnOutImageToSurface(int64_t timeStamp, uint32_t stremIdx)
{
    if (stremIdx >= SURFACE_TYPE_COUNT || SURFACE_TYPE_COUNT < 0) {
        MLOGE("invalid stremIdx: %d", stremIdx);
        return;
    }

    SurfaceWrapper *activeSurface = NULL;
    if (!mANWindows[stremIdx]) {
        MLOGE("mANWindows[%d] is null", stremIdx);
        return;
    } else {
        activeSurface = mANWindows[stremIdx];
    }
    activeSurface->enqueueBuffer(timeStamp);
    MLOGD("Return streamId:%d buffer timestamp: %" PRIu64, stremIdx, timeStamp);
}

CDKResult MiPostProcSessionIntf::buildSurface(void **surfaces)
{
    uint32_t i;
    CDKResult ret = MIAS_OK;
    SurfaceWrapper *activeSurface = NULL;
    mAcitveSurfaceCnt = 0;

    for (i = 0; i < SURFACE_TYPE_COUNT; i++) {
        ANativeWindow *s = reinterpret_cast<ANativeWindow *>(surfaces[i]);
        // create wrapper for all valid surfaces
        if (surfaces[i]) {
            mANWindows[i] = new SurfaceWrapper(s, (SurfaceType)i);
            if (!mANWindows[i]) {
                MLOGE("Error in creating SurfaceWrapper");
                return MIAS_NO_MEM;
            }
            if (mANWindows[i]->setup()) {
                MLOGE("Error in setup surface:%d", i);
                return MIAS_UNKNOWN_ERROR;
            }

            mAcitveSurfaceCnt++;
            // bind to correct node and port in this graph
            MLOGI("Created SurfaceWrapper for type:%d", i);
        }
    }

    if (mANWindows[SURFACE_TYPE_SNAPSHOT_MAIN]) {
        activeSurface = mANWindows[SURFACE_TYPE_SNAPSHOT_MAIN];
    } else {
        MLOGE("No valid output surface for initializing pipeline");
        return MIAS_INVALID_PARAM;
    }

    if (activeSurface != NULL) {
        MLOGD("Get frame info from %d type surface: %dx%d format:%d", activeSurface->getType(),
              activeSurface->getWidth(), activeSurface->getHeight(), activeSurface->getFormat());
    }

    return ret;
}

void MiPostProcSessionIntf::enqueueBuffer(CallbackType type, int64_t timeStampUs, uint32_t stremIdx)
{
    if (type == CallbackType::SOURCE_CALLBACK) {
        Mutex::Autolock autoLock(mTaskLock);
        auto iter = mProcessedFrameWrpMap.find(timeStampUs + stremIdx);
        if (iter != mProcessedFrameWrpMap.end()) {
            sp<MiaFrameWrapper> miaFrameWrapperVec = iter->second;
            mProcessedFrameWrpMap.erase(iter);
            miaFrameWrapperVec.clear();
        } else {
            MLOGE("can't found timestamp: %" PRIu64 " input frame", timeStampUs);
        }
    } else if (type == CallbackType::SINK_CALLBACK) {
        returnOutImageToSurface(timeStampUs, stremIdx);

        Mutex::Autolock autoLock(mTaskLock);
        auto it = mFirstTimestampsOfRequests.find(timeStampUs);
        if (it != mFirstTimestampsOfRequests.end()) {
            mFirstTimestampsOfRequests.erase(it);
        } else {
            MLOGE("can't found timestamp: %" PRIu64 " input frame", timeStampUs);
        }
    }
}

const native_handle_t *MiPostProcSessionIntf::dequeuBuffer(SurfaceType type, int64_t timeStampUs)
{
    MLOGI("getOutputBuffer type: %d, timestamp: %" PRIu64, type, timeStampUs);
    const native_handle_t *nativeHandle = nullptr;
    if (mANWindows[type]) {
        nativeHandle = mANWindows[type]->dequeueBuffer(timeStampUs);
    } else {
        MLOGE("mANWindows type: %d is null", type);
    }

    return nativeHandle;
}

void MiPostProcSessionIntf::onSessionCallback(ResultType type, int64_t timeStampUs, void *msgData)
{
    if (mSessionCb == NULL) {
        MLOGE("mSessionCb is null");
        return;
    }
    if (type == MIA_RESULT_METADATA) {
        mSessionCb->onSessionResultCb(MIA_MSG_PO_RESULT, msgData);
    } else if (type == MIA_SERVICE_DIED) {
        mSessionCb->onSessionResultCb(MIA_MSG_SERIVCE_DIED, NULL);
    } else if (type == MIA_ABNORMAL_CALLBACK) {
        mSessionCb->onSessionResultCb(MIA_MSG_ABNORMAL_PROCESS, msgData);
    }
}

} // namespace mialgo
