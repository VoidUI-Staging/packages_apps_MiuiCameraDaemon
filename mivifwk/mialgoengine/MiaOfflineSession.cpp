/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaOfflineSession.h"

#include <hardware/gralloc1.h>

#include "MiaPluginModule.h"
#include "MiaSettings.h"
#include "ThreadPool.h"

static const int g_forceSEBokehAuxMfsr =
    property_get_int32("persist.vendor.camera.forceSEBokehAuxMfsr", 0);

namespace mialgo2 {
using namespace midebug;

BufferQueueWrapper *BufferQueueWrapper::create(MiaImageFormat &format, buffer_type type,
                                               uint32_t wrapperId, uint32_t numBuffers)
{
    BufferQueueWrapper *bufferQueueWrapper = new BufferQueueWrapper;
    if (NULL != bufferQueueWrapper) {
        if (MIAS_OK != bufferQueueWrapper->initialize(format, type, wrapperId, numBuffers)) {
            bufferQueueWrapper->destroy();
            bufferQueueWrapper = NULL;
        }
    }
    return bufferQueueWrapper;
}

sp<MiaImage> BufferQueueWrapper::dequeueBuffer()
{
    if (mBufferQueue.isEmpty()) {
        MLOGI(Mia2LogGroupCore, "Common image is all busy, need allocate more buffer");
        increaseImageQueue(1);
    }
    sp<MiaImage> image = nullptr;
    mBufferQueue.waitAndPop(image);
    if (image != nullptr) {
        image->increaseUseCount(); // useCount : from 0 to 1
    }
    MLOGD(Mia2LogGroupCore, "%p useCount:%u", image.get(), image->getUseCount());
    return image;
}

void BufferQueueWrapper::enqueueBuffer(sp<MiaImage> frame)
{
    MLOGD(Mia2LogGroupCore, "%p useCount:%u", frame.get(), frame->getUseCount());
    if (frame->decreaseUseCount() == 0) {
        mBufferQueue.push(frame);
    }
}

int BufferQueueWrapper::getFreeBufferQueueSize()
{
    return mBufferQueue.queueSize();
}

int BufferQueueWrapper::getTotalBufferSize()
{
    return mPbufferManager->getAllocatedBufferNum();
}

void BufferQueueWrapper::clearOutDefaultBuffer()
{
    while (mBufferQueue.queueSize() > DEFAULT_BUFFER_QUEUE_DEPTH) {
        sp<MiaImage> frame = nullptr;
        if (mBufferQueue.tryPop(frame)) {
            frame.clear();
            frame = nullptr;
        }
    }
}

void BufferQueueWrapper::clearOutThresholdBuffer()
{
    int totalBufferCnt = getTotalBufferSize();
    if (totalBufferCnt > mThresholdValue) {
        while (mBufferQueue.queueSize() > MINIMUM_BUFFER_QUEUE_DEPTH) {
            sp<MiaImage> frame = nullptr;
            if (mBufferQueue.tryPop(frame)) {
                frame.clear();
                frame = nullptr;
            }
        }
    }
}

void BufferQueueWrapper::setBufferThresholdValue(uint32_t bufferThresholdValue)
{
    mThresholdValue = bufferThresholdValue;
}

void BufferQueueWrapper::flush()
{
    mBufferQueue.releaseQueue();
}

void BufferQueueWrapper::destroy()
{
    MLOGD(Mia2LogGroupCore, "BufferQueueWrapper destroy queueSize: %d", mBufferQueue.queueSize());
    mBufferQueue.clearQueue();

    // release BufferManager
    if (mPbufferManager.get() != NULL) {
        checkBufferState();
        mPbufferManager->destroy();
        mPbufferManager.clear();
        mPbufferManager = NULL;
    }

    delete this;
}

bool BufferQueueWrapper::isMatchFormat(MiaImageFormat format)
{
    if (mBufferFormat.width == format.width && mBufferFormat.height == format.height &&
        mBufferFormat.format == format.format) {
        return true;
    } else {
        return false;
    }
}

CDKResult BufferQueueWrapper::initialize(MiaImageFormat &format, buffer_type type,
                                         uint32_t wrapperId, uint32_t numBuffers)
{
    CDKResult result = MIAS_OK;
    mBufferQueue.clearQueue();
    mBufferWrapperId = 0;
    mPbufferManager = NULL;

    mBufferFormat = format;
    mBufferWrapperId = wrapperId;
    BufferManagerCreateData createData;
    memset(&createData, 0, sizeof(BufferManagerCreateData));
    createData.width = format.width;
    createData.height = format.height;
    createData.format = format.format; // 35
    createData.bufferType = type;
    createData.usage = GRALLOC1_PRODUCER_USAGE_CAMERA | GRALLOC1_PRODUCER_USAGE_CPU_READ |
                       GRALLOC1_PRODUCER_USAGE_CPU_WRITE;
    createData.numBuffers = numBuffers;

    mPbufferManager = MiaBufferManager::create(&createData);
    if (!mPbufferManager.get()) {
        MLOGE(Mia2LogGroupCore, "error in creating BufferManager\n");
        return MIAS_NO_MEM;
    }

    uint32_t realBufferNum = mPbufferManager->getAllocatedBufferNum();
    for (uint32_t i = 0; i < realBufferNum; i++) {
        sp<MiaImage> image = new MiaImage(mPbufferManager, format);
        mBufferQueue.push(image);
    }

    MLOGI(Mia2LogGroupCore,
          "BufferQueueWrapper width: %d, height: %d, format: %d, wrapperId: %d created",
          format.width, format.height, format.format, wrapperId);
    return result;
}

void BufferQueueWrapper::checkBufferState()
{
    if (mPbufferManager.get() != NULL) {
        BufferList_t _l = mPbufferManager->getBuffersInFlight();
        BufferList_t::iterator it = _l.begin();
        for (; it != _l.end(); it++) {
            MLOGE(Mia2LogGroupCore, "Buffer:%p is going to be released but still in use", *it);
            // TODO: get the ownerID to show more information
        }
    }
}

CDKResult BufferQueueWrapper::increaseImageQueue(uint32_t numBuffers)
{
    CDKResult result = MIAS_OK;
    if (!mPbufferManager.get()) {
        MLOGE(Mia2LogGroupCore, "error increaseImageQueue");
        return MIAS_INVALID_PARAM;
    }

    uint32_t currentBuffers = mPbufferManager->getAllocatedBufferNum();
    if (currentBuffers >= MAX_BUFFER_QUEUE_DEPTH) {
        MLOGD(Mia2LogGroupCore, "Out of maxbuffers, current buffer num %d >= max buffer num %d",
              currentBuffers, MAX_BUFFER_QUEUE_DEPTH);
        return MIAS_NO_MEM;
    }

    result = mPbufferManager->allocateMoreBuffers(numBuffers);
    if (result != MIAS_OK) {
        MLOGE(Mia2LogGroupCore, "Out of memory, allocateMoreBuffers failed");
        return MIAS_NO_MEM;
    }

    for (int i = 0; i < numBuffers; i++) {
        sp<MiaImage> image = new MiaImage(mPbufferManager, mBufferFormat);
        MLOGD(Mia2LogGroupCore, "increaseImageQueue create new output Image %p", image.get());
        mBufferQueue.push(image);
    }

    return result;
}

MiaOfflineSession *MiaOfflineSession::create(MiaCreateParams *params)
{
    // used for check whether MIVI error trigger is effective
    char isSochanged[256] = "";
    property_get("vendor.logsystem.check", isSochanged, "0");
    if (atoi(isSochanged) == 1) {
        MLOGE(Mia2LogGroupCore, "check whether MIVI error trigger is effective");
    }

    MiaOfflineSession *session = new MiaOfflineSession;
    if (NULL != session) {
        // initialize Stream and TBM params
        CDKResult result = session->initialize(params);
        if (MIAS_OK != result) {
            MLOGE(Mia2LogGroupCore, "initialize failed, return NULL");
            delete session;
            session = NULL;
        }
    } else {
        MLOGE(Mia2LogGroupCore, "pEncoder is NULL, malloc failed");
    }

    return session;
}

MiaOfflineSession::MiaOfflineSession()
{
    mSinkImageMap.clear();
    mInitBufferQueueDepth = DEFAULT_BUFFER_QUEUE_DEPTH;
    mSessionCb = NULL;
    mFrameCb = NULL;
    mIdlePipeline = NULL;
    mSelectedPipelineName = "";
    mBufferQueueWrapperSet.clear();
    mBufferWraperCount = 0;
    mSessionRequestCnt = 0;
    MPluginModule::getInstance();
    Singleton<ThreadPool>::Instance(THREAD_POOL_NUMS);
    mNodes.clear();
    mAlgoResultInfo.clear();
    mSourcePortIdVec.clear();

    mReltime = 6000;
    mDefaultReclaimCnt = 3;
    mBufferThresholdValue = 3;
    mLIFOPipePriority = 0;
    mFIFOPipePriority = 0;
    mNodeThreadNum = 0;

    mTimeoutNode = "";
    mIsInitDone = false;
    mIsBurstMode = false;
    if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::QCOM) {
        mSupportLIFO = true;
    } else {
        mSupportLIFO = false;
    }
    MLOGI(Mia2LogGroupCore, "MiaOfflineSession construct %p", this);
}

MiaOfflineSession::~MiaOfflineSession()
{
    MLOGI(Mia2LogGroupCore, "disconstruct %p", this);
    destroy();

    mAlgoResultInfo.clear();

    MICAM_TRACE_ASYNC_END_F(MialgoTraceProfile, 0, "MIA_OFFLINE_SESSION");
}

CDKResult MiaOfflineSession::preProcess(MiaPreProcParams *preParams)
{
    MLOGD(Mia2LogGroupCore, "Session src ports: %d, sink ports: %d", preParams->inputFormats.size(),
          preParams->outputFormats.size());

    if (mPipelinePruner.get() == nullptr) {
        MLOGE(Mia2LogGroupCore, "Session %p mPipelinePruner is NULL, please check buildGraph!",
              this);
        return MIAS_INVALID_PARAM;
    }

    PipelineInfo *rawInfo = mPipelinePruner->getRawPipelineInfo();
    if ((rawInfo->nodes).empty() || (rawInfo->links).empty()) {
        MLOGE(Mia2LogGroupCore,
              "Session %p raw pipeline node info is empty %d, raw pipeline link info is empty %d, "
              "please check buildGraph!",
              this, rawInfo->nodes.empty(), rawInfo->links.empty());
        return MIAS_INVALID_PARAM;
    }

    std::shared_ptr<PreMetaProxy> metadata =
        std::make_shared<PreMetaProxy>(preParams->metadata->move());
    for (auto &nodeItem : rawInfo->nodes) {
        uint32_t inputPortnum = 0, outputPortnum = 0;
        std::map<int, uint32_t> nodeOutputFormat, nodeInputFormat;
        for (auto &link : rawInfo->links) {
            if (link.dstNodeInstance == nodeItem.first) {
                if (inputPortnum < (link.dstNodePortId + 1))
                    inputPortnum = link.dstNodePortId + 1;
                // There can be multiple formats on dstport, if you need to pass multiple formats,
                // you can modify them here
                nodeInputFormat[link.dstNodePortId] = link.dstPortFormat[0];
            }

            if (link.srcNodeInstance == nodeItem.first) {
                if (outputPortnum < (link.srcNodePortId + 1))
                    outputPortnum = link.srcNodePortId + 1;
                nodeOutputFormat[link.srcNodePortId] = link.srcPortFormat;
            }
        }

        PostMiaPreProcParams postPreParams{};
        for (int inPort = 0; inPort < inputPortnum; ++inPort) {
            postPreParams.inputFormats[inPort] = inPort < preParams->inputFormats.size()
                                                     ? preParams->inputFormats[inPort]
                                                     : preParams->inputFormats[0];
            // The port id of virtualsink is 2, there are no 0 and 1
            if (nodeInputFormat.find(inPort) != nodeInputFormat.end()) {
                postPreParams.inputFormats[inPort].format = nodeInputFormat[inPort];
            }
        }

        for (int outPort = 0; outPort < outputPortnum; ++outPort) {
            postPreParams.outputFormats[outPort] = outPort < preParams->outputFormats.size()
                                                       ? preParams->outputFormats[outPort]
                                                       : preParams->outputFormats[0];
            if (nodeOutputFormat.find(outPort) != nodeOutputFormat.end()) {
                postPreParams.outputFormats[outPort].format = nodeOutputFormat[outPort];
            }
        }

        postPreParams.metadata = metadata;

        if (mNodes.find(nodeItem.first) == mNodes.end()) {
            MLOGW(Mia2LogGroupCore, "mNodes cannot find %s!", nodeItem.first.c_str());
            continue;
        }

        mNodes[nodeItem.first]->preProcess(&postPreParams);
    }

    return MIAS_OK;
}

static void updateFovCropZoomRatio(camera_metadata_t *metadata, camera_metadata_t *phyMetadata)
{
    int cameraId = -1;
    void *pData = NULL;

    camera_metadata_t *inputMetadata = phyMetadata ? phyMetadata : metadata;
    VendorMetadataParser::getVTagValue(inputMetadata, "xiaomi.snapshot.fwkCameraId", &pData);
    if (NULL != pData) {
        cameraId = *static_cast<uint32_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "framework cameraId: %d", cameraId);

        bool enableFovcrop = false;
        uint32_t fovcropCameraIdMask = 0;
        float fovcropZoomRatio = 1.0;
        camera_metadata_t *staticMetadata =
            StaticMetadataWraper::getInstance()->getStaticMetadata(cameraId);
        VendorMetadataParser::getVTagValue(staticMetadata, "xiaomi.fovcrop.cameraIdMask", &pData);
        if (NULL != pData) {
            fovcropCameraIdMask = *(static_cast<uint32_t *>(pData));
            MLOGI(Mia2LogGroupCore, "fovcropCameraIdMask %x", fovcropCameraIdMask);
        }
        VendorMetadataParser::getVTagValue(staticMetadata, "xiaomi.fovcrop.zoomRatio", &pData);
        if (NULL != pData) {
            fovcropZoomRatio = *(static_cast<float *>(pData));
            MLOGI(Mia2LogGroupCore, "fovcropZoomRatio %.3f", fovcropZoomRatio);
        }
        enableFovcrop = (fovcropCameraIdMask & (1 << cameraId));
        MLOGI(Mia2LogGroupCore, "enableFovcrop %d", enableFovcrop);
        if (NULL != metadata && enableFovcrop && fovcropZoomRatio > 0.01) {
            camera_metadata_entry_t entry = {0};
            if (0 == find_camera_metadata_entry(metadata, ANDROID_CONTROL_ZOOM_RATIO, &entry)) {
                float controlZoomRatio = entry.data.f[0];
                // Here, we are updating metadata by modifying the array entries.
                entry.data.f[0] *= fovcropZoomRatio;
                MLOGI(Mia2LogGroupCore, "fovcrop ZOOM_RATIO %.3f --> %.3f", controlZoomRatio,
                      entry.data.f[0]);
            }
        }
    } else {
        MLOGW(Mia2LogGroupPlugin, "framework cameraId fail");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  MiaOfflineSession::postProcess
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaOfflineSession::postProcess(MiaProcessParams *sessionParams)
{
    CDKResult result = MIAS_OK;
    if (sessionParams == NULL) {
        MLOGE(Mia2LogGroupCore, "error invalid session params");
        return MIAS_INVALID_PARAM;
    }

    MLOGI(Mia2LogGroupCore, "frameNumber %u, input num %d, output num %d", sessionParams->frameNum,
          sessionParams->inputNum, sessionParams->outputNum);

    if ((sessionParams->outputNum > 0) && (sessionParams->outputFrame != NULL)) {
        sp<MiaImage> pOutputImage = new MiaImage(sessionParams->outputFrame, MI_IMG_SINK);
        if (pOutputImage == NULL) {
            MLOGE(Mia2LogGroupCore, "Fail to new output image");
            return MIAS_INVALID_PARAM;
        }
        std::unique_lock<std::mutex> locker(mSinkImageMux);
        mSinkImageMap.insert({pOutputImage->getTimestamp(), pOutputImage});
        locker.unlock();
    }

    if (!sessionParams->inputFrame) {
        MLOGW(Mia2LogGroupCore, "input buffer size is null");
        return MIAS_INVALID_PARAM;
    }

    sp<MiaImage> pInputImage = new MiaImage(sessionParams->inputFrame, MI_IMG_SOURCE);
    if (pInputImage == NULL) {
        MLOGE(Mia2LogGroupCore, "Fail to new input image\n");
        return MIAS_INVALID_PARAM;
    }

    camera_metadata_t *metadata = pInputImage->getMetadataWraper()->getLogicalMetadata();
    camera_metadata_t *phyMetadata = pInputImage->getMetadataWraper()->getPhysicalMetadata();

    if (MiaInternalProp::getInstance()->getBool(NeedUpdateFovCropZoomRatio)) {
        updateFovCropZoomRatio(metadata, phyMetadata);
    }

    MiaImageFormat inputImgFormat = pInputImage->getImageFormat();
    std::string requestName = pInputImage->getImageName();
    // At least ensure that the first frame of the request carries correct totalReqNum info
    uint32_t totalReqNum = pInputImage->getTotalReqNum();
    uint32_t sourcePort = sessionParams->inputFrame->streamId;
    MiaCustomization::reMapSourcePortId(sourcePort, metadata, phyMetadata, mSelectedPipelineName,
                                        pInputImage);
    bool newRequest = false;
    auto histroyPipe = mInputNotEnoughPipe.find(requestName);
    if (histroyPipe == mInputNotEnoughPipe.end()) {
        newRequest = true;

        if (totalReqNum == 0) {
            MLOGE(Mia2LogGroupCore, "xiaomi.burst.mialgoTotalReqNum tag info error");
            return MIAS_INVALID_PARAM;
        } else if (totalReqNum > 1) {
            // Single frame request need not be memorized
            // insert pipeline info
            mInputNotEnoughPipe[requestName].frameNumNotCome = totalReqNum - 1;
        }
    }

    sp<Pipeline> pipeline = nullptr;
    if (newRequest) {
        int64_t timestampUs = pInputImage->getTimestamp();
        pipeline =
            new Pipeline(metadata, &mCreateParams, sessionParams, timestampUs, mSourcePortIdVec);
        std::unique_lock<std::mutex> lock(mBusyLock);
        mBusyPipelines.insert(pipeline);
        int64_t totalPipelines = mBusyPipelines.size();
        lock.unlock();

        pipeline->setSession(this);
        pipeline->setPipelineName(pInputImage->getImageName());
        // FIFO: new request's priority is low
        pipeline->setFIFOPriority(atomic_fetch_sub(&mFIFOPipePriority, 1));
        // LIFO: new request's priority is high
        pipeline->setLIFOPriority(atomic_fetch_add(&mLIFOPipePriority, 1));

        MLOGI(Mia2LogGroupCore, "current pipeline[%s][%p] totals %d",
              pInputImage->getImageName().c_str(), pipeline.get(), totalPipelines);
        pipeline->start();
        // for exif of FaceRoi in QCOM
        if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::QCOM) {
            getFaceExifInfo(metadata, inputImgFormat.width, inputImgFormat.height,
                            pInputImage->getFrameNumber(), pInputImage->getTimestamp());
        }
        mSessionRequestCnt++;
    }

    auto forExif = [this, &pInputImage, &metadata](exifInfoOfRequest &exifInfoOfRequest) {
        if (!mIsBurstMode) {
            prepareForExifUpdate(pInputImage, metadata, exifInfoOfRequest);
            if (exifInfoOfRequest.mLastRequestSourceBufferNum ==
                pInputImage->getMergeInputNumber()) {
                updateExifInfo(metadata, exifInfoOfRequest);
            }
        }
    };

    if (newRequest && totalReqNum == 1) {
        exifInfoOfRequest exifInfoOfRequest;
        exifInfoOfRequest.mLastRequestSourceBufferNum++;
        forExif(exifInfoOfRequest);
    } else { // Multiframe request
        auto currentPipe = histroyPipe;
        if (newRequest) { // or use : if(histroyPipe == mInputNotEnoughPipe.end())
            currentPipe = mInputNotEnoughPipe.find(requestName);
        }
        requestInfo &requestInfo = currentPipe->second;
        exifInfoOfRequest &exifInfoOfRequest = requestInfo.exifInfoOfRequest;
        exifInfoOfRequest.mLastRequestSourceBufferNum++;
        forExif(exifInfoOfRequest);

        if (newRequest) {
            // Record pipeline address for subsequent frame
            requestInfo.pipeline = pipeline;
        } else {
            // Use the previously recorded pipeline address
            pipeline = requestInfo.pipeline;
            // The input is satisfied, and the related record information is deleted
            if (--requestInfo.frameNumNotCome == 0) {
                mInputNotEnoughPipe.erase(currentPipe);
            }
        }
    }

    if (!pipeline->enqueueSourceImage(pInputImage, sourcePort)) { // push sourceBuffer
        if (pInputImage->decreaseUseCount() == 0) {
            pInputImage->releaseExternalFrame();
        }
    }

    MLOGD(Mia2LogGroupCore, "[MemoryReclaim] mSessionRequestCnt = %u", mSessionRequestCnt);
    if (mSessionRequestCnt % mDefaultReclaimCnt == 0) {
        memoryMonitor();
    }

    MICAM_TRACE_ASYNC_BEGIN_F(MialgoTraceProfile, 0, "MIA_OFFLINE_SESSION");
    return result;
}

void MiaOfflineSession::destroy()
{
    MICAM_TRACE_SYNC_BEGIN_F(MialgoTraceProfile, "mialgo2::MiaOfflineSession::destroy");
    MLOGI(Mia2LogGroupCore, "destroy E");

    if (mIdlePipeline != NULL) {
        mIdlePipeline.clear();
        mIdlePipeline = NULL;
    }

    mPipelinePruner.clear();
    mNodes.clear();

    // stop memory reclaim timer
    mTimer.stop();

    for (auto &iter : mBufferQueueWrapperSet) {
        BufferQueueWrapper *bufferWrapper = iter;
        if (bufferWrapper != NULL) {
            bufferWrapper->flush();
        }
    }

    mSinkImageMap.clear();
    mBufferWraperCount = 0;

    for (auto iter = mBufferQueueWrapperSet.begin(); iter != mBufferQueueWrapperSet.end(); iter++) {
        BufferQueueWrapper *bufferWrapper = *iter;
        if (bufferWrapper != NULL) {
            bufferWrapper->destroy();
        }
    }

    MLOGI(Mia2LogGroupCore, "destroy X");
    MICAM_TRACE_SYNC_END(MialgoTraceProfile);
}

void MiaOfflineSession::memoryMonitor()
{
    Mutex::Autolock autoLock(mBufferReclaimMutex);
    for (auto iter = mBufferQueueWrapperSet.begin(); iter != mBufferQueueWrapperSet.end(); iter++) {
        BufferQueueWrapper *bufferQueueWrapper = *iter;
        if (bufferQueueWrapper != NULL) {
            bufferQueueWrapper->clearOutThresholdBuffer();
            MLOGD(Mia2LogGroupCore,
                  "[MemoryReclaim][%p] totalbuffer = %d, busybuffer = %d, freebuffer = %d", this,
                  bufferQueueWrapper->getTotalBufferSize(),
                  bufferQueueWrapper->getTotalBufferSize() -
                      bufferQueueWrapper->getFreeBufferQueueSize(),
                  bufferQueueWrapper->getFreeBufferQueueSize());
        }
    }
}

void MiaOfflineSession::initMemReclaim(MiaCreateParams *params)
{
    MLOGI(Mia2LogGroupCore, "operationMode: 0x%x, cameraMode: 0x%x", params->operationMode,
          params->cameraMode);
    bool isMatched = false;
    uint32_t paramsCnt =
        sizeof(g_MiaMemReclaimParamMappingTable) / sizeof(g_MiaMemReclaimParamMappingTable[0]);
    for (uint32_t index = 0; index < paramsCnt; index++) {
        if (params->operationMode == g_MiaMemReclaimParamMappingTable[index].operationMode &&
            params->cameraMode == g_MiaMemReclaimParamMappingTable[index].cameraMode) {
            mDefaultReclaimCnt =
                g_MiaMemReclaimParamMappingTable[index].memoryReclaimParams.defaultReclaimCnt;
            mBufferThresholdValue =
                g_MiaMemReclaimParamMappingTable[index].memoryReclaimParams.bufferThresholdValue;
            mReltime = g_MiaMemReclaimParamMappingTable[index].memoryReclaimParams.reltime;
            isMatched = true;
            if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::MTK) {
                mBufferThresholdValue *= 3;
            }
            break;
        }
    }

    if (!isMatched) {
        mDefaultReclaimCnt = 3;
        mBufferThresholdValue = 3;
        mReltime = 6000;
        if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::MTK) {
            mBufferThresholdValue = 12;
        }
    }

    MLOGD(Mia2LogGroupCore, "reclaimParams: defaultcnt = %u, threadValue = %u, reltime = %ds",
          mDefaultReclaimCnt, mBufferThresholdValue, (int)(mReltime / (nsecs_t)1000));

    auto func = [this]() { memoryMonitor(); };
    mTimer.start(mReltime, func);
}

CDKResult MiaOfflineSession::initialize(MiaCreateParams *params)
{
    if (params == NULL) {
        MLOGE(Mia2LogGroupCore, "error invalid params");
        return MIAS_INVALID_PARAM;
    }

    CDKResult result = MIAS_OK;
    mSessionCb = params->sessionCb;
    mFrameCb = params->frameCb;
    mCreateParams = *params;
    result = buildGraph(params);
    if (result != MIAS_OK) {
        return result;
    }
    initMemReclaim(params);
    return result;
}

CDKResult MiaOfflineSession::buildGraph(MiaCreateParams *params)
{
    CDKResult ret = MIAS_OK;
    PipelineInfo pipeline;
    char filename[64] = {0};

    MLOGI(Mia2LogGroupCore, "operationMode: 0x%x, cameraMode: 0x%x", params->operationMode,
          params->cameraMode);
    if (params->operationMode == StreamConfigModeBokeh) {
        if (params->cameraMode == CAM_COMBMODE_FRONT_BOKEH ||
            params->cameraMode == CAM_COMBMODE_FRONT)
            strcpy(filename, "/odm/etc/camera/xiaomi/frontbokehsnapshot.json");
        else {
            if (g_forceSEBokehAuxMfsr) {
                strcpy(filename, "/odm/etc/camera/xiaomi/dualbokehseauxmfsrsnapshot.json");
            } else {
                strcpy(filename, "/odm/etc/camera/xiaomi/dualbokehsnapshot.json");
                mSupportLIFO = false;
            }
        }
    } else if (params->operationMode == StreamConfigModeThirdPartyBokeh2x ||
               params->operationMode == StreamConfigModeThirdPartyBokeh) {
        if (params->cameraMode == CAM_COMBMODE_FRONT) {
            if (params->outputFormat[0].format == HAL_PIXEL_FORMAT_BLOB) {
                strcpy(filename, "/odm/etc/camera/xiaomi/thirdpartyjpegsnapshot.json");
            } else {
                strcpy(filename, "/odm/etc/camera/xiaomi/thirdpartysnapshot.json");
            }
        } else {
            if (params->outputFormat[0].format == HAL_PIXEL_FORMAT_BLOB) {
                strcpy(filename, "/odm/etc/camera/xiaomi/dualbokehjpegsnapshot.json");
            } else {
                strcpy(filename, "/odm/etc/camera/xiaomi/thirdpartydualbokehyuvsnapshot.json");
            }
        }
    } else if (params->operationMode == StreamConfigModeUltraPixelPhotography ||
               params->operationMode == StreamConfigModeAlgoQcfaMfnr) {
        strcpy(filename, "/odm/etc/camera/xiaomi/superhdsnapshot.json");
    } else if (params->operationMode == StreamConfigModeMiuiManual ||
               params->operationMode == StreamConfigModeAlgoManual) {
        // 108m Set a simple Graph
        if (params->outputFormat[0].width >= 6016 && params->outputFormat[0].height >= 4512) {
            strcpy(filename, "/odm/etc/camera/xiaomi/superhdsnapshot.json");
        } else {
            strcpy(filename, "/odm/etc/camera/xiaomi/manualsnapshot.json");
        }
    } else if (params->operationMode == StreamConfigModeSAT) {
        strcpy(filename, "/odm/etc/camera/xiaomi/satsnapshot.json");
    } else if (params->operationMode == StreamConfigModeThirdPartyAutoExtension) {
        strcpy(filename, "/odm/etc/camera/xiaomi/satsnapshotjpeg.json");
    } else if (params->operationMode == StreamConfigModeMiuiZslFront) {
        strcpy(filename, "/odm/etc/camera/xiaomi/frontsinglesnapshot.json");
    } else if (params->operationMode == StreamConfigModeThirdPartyNormal ||
               params->operationMode == StreamConfigModeThirdPartyMFNR) {
        if (params->outputFormat[0].format == HAL_PIXEL_FORMAT_BLOB)
            strcpy(filename, "/odm/etc/camera/xiaomi/thirdpartyjpegsnapshot.json");
        else
            strcpy(filename, "/odm/etc/camera/xiaomi/thirdpartysnapshot.json");
    } else if (params->operationMode == StreamConfigModeMiuiSuperNight) {
        if (params->cameraMode == CAM_COMBMODE_FRONT ||
            params->cameraMode == CAM_COMBMODE_FRONT_AUX) {
            strcpy(filename, "/odm/etc/camera/xiaomi/frontsupernightsnapshot.json");
        } else {
            strcpy(filename, "/odm/etc/camera/xiaomi/rearsupernightsnapshot.json");
        }
    } else if (params->operationMode == StreamConfigModeThirdPartySuperNight) {
        if (params->cameraMode == CAM_COMBMODE_FRONT ||
            params->cameraMode == CAM_COMBMODE_FRONT_AUX) {
            if (params->outputFormat[0].format == HAL_PIXEL_FORMAT_BLOB) {
                strcpy(filename, "/odm/etc/camera/xiaomi/frontsupernightsnapshotjpeg.json");
            } else {
                strcpy(filename, "/odm/etc/camera/xiaomi/frontsupernightsnapshot.json");
            }
        } else {
            bool isRawSupernight = false;
            bool isBlobSnapshot = false;
            for (auto iFormat : params->inputFormat) {
                if (iFormat.format == HAL_PIXEL_FORMAT_RAW16) {
                    isRawSupernight = true;
                }
            }
            for (auto oFormat : params->outputFormat) {
                if (oFormat.format == HAL_PIXEL_FORMAT_BLOB) {
                    isBlobSnapshot = true;
                }
            }
            if (isRawSupernight) {
                if (isBlobSnapshot) {
                    strcpy(filename, "/odm/etc/camera/xiaomi/thirdpartyrawsupernightjpeg.json");
                } else {
                    strcpy(filename, "/odm/etc/camera/xiaomi/thirdpartyrawsupernightyuv.json");
                }
            } else {
                if (isBlobSnapshot) {
                    strcpy(filename, "/odm/etc/camera/xiaomi/thirdpartyjpegsnapshot.json");
                } else {
                    strcpy(filename, "/odm/etc/camera/xiaomi/thirdpartysnapshot.json");
                }
            }
        }
    } else if (params->operationMode == StreamConfigModeSATBurst) {
        strcpy(filename, "/odm/etc/camera/xiaomi/satburstsnapshot.json");
        mIsBurstMode = true;
        mSupportLIFO = false;
    } else if (params->operationMode == StreamConfigModeMiuiCameraJpeg) {
        strcpy(filename, "/odm/etc/camera/xiaomi/miuicamerayuv2jpeg.json");
        mIsBurstMode = true;
        mSupportLIFO = false;
    } else if (params->operationMode == StreamConfigModeMiuiCameraHeic) {
        strcpy(filename, "/odm/etc/camera/xiaomi/miuicamerayuv2heic.json");
    } else {
        strcpy(filename, "/odm/etc/camera/xiaomi/normalsnapshot.json");
        mIsBurstMode = true;
        mSupportLIFO = false;
    }

    if (params->operationMode == StreamConfigModeTestMemcpy) {
        strcpy(filename, "/odm/etc/camera/xiaomi/testmemcpy.json");
    } else if (params->operationMode == StreamConfigModeTestCpyJpg) {
        strcpy(filename, "/odm/etc/camera/xiaomi/testcpyjpg.json");
    } else if (params->operationMode == StreamConfigModeTestReprocess) {
        strcpy(filename, "/odm/etc/camera/xiaomi/testreprocess.json");
    } else if (params->operationMode == StreamConfigModeTestBokeh) {
        strcpy(filename, "/odm/etc/camera/xiaomi/testbokeh.json");
    } else if (params->operationMode == StreamConfigModeTestMulti) {
        strcpy(filename, "/odm/etc/camera/xiaomi/testmulti.json");
    } else if (params->operationMode == StreamConfigModeTestR2Y) {
        strcpy(filename, "/odm/etc/camera/xiaomi/testraw2yuv.json");
    }

    if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::MTK) {
        std::string dir(filename);
        dir.replace(1, 3, "vendor");
        strcpy(filename, dir.c_str());
    }

    std::string jsonStr = MJsonUtils::readFile(filename);
    if (jsonStr.empty()) {
        MLOGE(Mia2LogGroupCore, "read  %s failed", filename);
        return MIAS_INVALID_PARAM;
    }
    MLOGI(Mia2LogGroupCore, "Opened filename: %s", filename);
    MJsonUtils::parseJson2Struct(jsonStr.c_str(), &pipeline);
    MLOGI(Mia2LogGroupCore, "Selected PipelineName: %s, node size: %lu",
          pipeline.pipelineName.c_str(), pipeline.nodes.size());

    // Detect whether node initialization timed out
    uint32_t tid = syscall(SYS_gettid);
    std::string initInfo = "";
    std::thread detectThread([this, tid, &initInfo]() { timeoutDetection(tid, initInfo); });
    sp<MiaNodeFactory> nodeFactory = new MiaNodeFactory();
    double initStartTime = MiaUtil::nowMSec();

    /*
     * When the pipeline is pruned, source->sink will appear, and memcpy needs to be added in
     * the middle to make it run normally. That is,"source->sink" becomes
     * "source->newMemcpyInstance->sink".So we create bridges here and use them.
     */
    std::string nodeName = "com.xiaomi.plugin.memcpy";
    std::string nodeInstance = "newMemcpyInstance";
    mTimeoutNode = nodeInstance;
    struct NodeItem nodeItem = {nodeName, nodeInstance, 0, true, OneBuffer};
    sp<MiaNode> node = nullptr;
    nodeFactory->CreateNodeInstance(0, node);
    if (node.get() == NULL) {
        MLOGE(Mia2LogGroupCore, "fail in create node");
        ret = MIAS_NO_MEM;
    } else {
        ret = node->init(nodeItem, params);
        if (ret != MIAS_OK) {
            MLOGE(Mia2LogGroupCore, "node:%s Init failed", nodeName.c_str());
        } else {
            node->setNodeCB(this);
            mNodes[nodeInstance] = node;
        }
    }

    initInfo = initInfo + " " + mTimeoutNode + ":" +
               std::to_string(MiaUtil::nowMSec() - initStartTime) + "ms";
    initStartTime = MiaUtil::nowMSec();

    // create and initialize all needed nodes
    auto mapIt = pipeline.nodes.begin();
    for (; mapIt != pipeline.nodes.end(); mapIt++) {
        uint32_t nodeProcessMask = static_cast<uint32_t>(mapIt->second.nodeMask);
        sp<MiaNode> node = nullptr;
        nodeFactory->CreateNodeInstance(nodeProcessMask, node);
        if (node.get() == NULL) {
            MLOGE(Mia2LogGroupCore, "fail in create node");
            ret = MIAS_NO_MEM;
            break;
        } else {
            mTimeoutNode = mapIt->second.nodeInstance.c_str();
            ret = node->init(mapIt->second, params);
            if (ret != MIAS_OK) {
                MLOGE(Mia2LogGroupCore, "node:%s Init failed", mapIt->second.nodeName.c_str());
                break;
            } else {
                node->setNodeCB(this);
                node->setNodeMask(nodeProcessMask);
                mNodes[mapIt->second.nodeInstance] = node; // push
            }
        }
        initInfo = initInfo + " " + mTimeoutNode + ":" +
                   std::to_string(MiaUtil::nowMSec() - initStartTime) + "ms";
        initStartTime = MiaUtil::nowMSec();
    }
    pipeline.nodes.insert(std::make_pair(nodeInstance, nodeItem));

    signalThread();
    if (detectThread.joinable()) {
        detectThread.join();
    }
    if (ret != MIAS_OK) {
        return ret;
    }

    for (int index = 0; index < pipeline.links.size(); index++) {
        const std::string &linkSrcInstance = pipeline.links[index].srcNodeInstance;
        bool source = strstr(linkSrcInstance.c_str(), "SourceBuffer") != NULL ? true : false;
        if (source) {
            mSourcePortIdVec.push_back(pipeline.links[index].srcNodePortId);
        }
    }

    mSelectedPipelineName = pipeline.pipelineName;
    mPipelinePruner = new PipelinePruner(mNodes, pipeline);

    return ret;
}

void MiaOfflineSession::onResultComing(MiaResultCbType type, uint32_t frameNum, int64_t timeStamp,
                                       std::string &data)
{
    if (type == MiaResultCbType::MIA_ANCHOR_CALLBACK) {
        MLOGI(Mia2LogGroupCore, "Anchor Callback: frameNum:%llu timeStamp:%" PRIu64 " data:%s",
              frameNum, timeStamp, data.c_str());
        mSessionCb->onSessionCallback(MiaResultCbType::MIA_ANCHOR_CALLBACK,
                                      frameNum /*request frame*/, timeStamp /*Anchor ts*/,
                                      &data /*first ts*/);
        return;
    }

    if (type == MiaResultCbType::MIA_META_CALLBACK) {
        MLOGI(Mia2LogGroupCore, "frameNum: %llu timeStamp:  %" PRIu64 " data: %s", frameNum,
              timeStamp, data.c_str());
        if (timeStamp <= 0) {
            return;
        }

        Mutex::Autolock autoLock(mAlgoResultLock);
        auto iter = mAlgoResultInfo.find(timeStamp);
        if (iter != mAlgoResultInfo.end()) {
            std::vector<std::string> &allResult = iter->second;
            allResult.push_back(data);
        } else {
            std::vector<std::string> newResult;
            newResult.push_back(data);
            mAlgoResultInfo.insert(std::make_pair(timeStamp, newResult));
        }
    }

    if (type == MiaResultCbType::MIA_ABNORMAL_CALLBACK) {
        MLOGI(Mia2LogGroupCore, " frameNum: %llu timeStamp:  %" PRIu64, frameNum, timeStamp);
        mSessionCb->onSessionCallback(MiaResultCbType::MIA_ABNORMAL_CALLBACK, frameNum, timeStamp,
                                      &data);
        return;
    }
}

sp<MiaNode> MiaOfflineSession::getProcessNode(std::string instance)
{
    sp<MiaNode> processNode = nullptr;

    auto it = mNodes.find(instance);
    if (it != mNodes.end()) {
        processNode = it->second;
    }

    return processNode;
}

void MiaOfflineSession::moveToIdleQueue(sp<Pipeline> pipeline)
{
    if (pipeline == NULL) {
        return;
    }

    {
        Mutex::Autolock autoLock(mIdleLock);
        if (mIdlePipeline != NULL) {
            mIdlePipeline.clear();
        }
        mIdlePipeline = pipeline;
    }

    MLOGI(Mia2LogGroupCore, "pipeline[%s] move to IdleQueue finish", pipeline->getPipelineName());

    {
        std::lock_guard<std::mutex> lock(mBusyLock);

        auto it = mBusyPipelines.find(pipeline);
        if (it != mBusyPipelines.end()) {
            mBusyPipelines.erase(it);
        }

        if (mBusyPipelines.empty()) {
            MLOGI(Mia2LogGroupCore, "session[%p] all request process finish in the current", this);
            /*
             * Here is the reason for locking: make sure that this signal is sent after
             * the wait in the "flush(bool isForced)" to prevent flush from not exiting
             */
            mBusyCond.notify_one();
        }
    }
}

void MiaOfflineSession::increaseUsingThreadNum(int increment)
{
    std::lock_guard<std::mutex> lock(mAllNodeFinishLock);
    mNodeThreadNum += increment;
    if (mNodeThreadNum == 0) {
        mAllNodeFinishCond.notify_one();
    }
}

sp<MiaImage> MiaOfflineSession::acquireOutputBuffer(MiaImageFormat format, int64_t timestamp,
                                                    uint32_t sinkPortId, MiaImageType newImageType)
{
    MLOGI(Mia2LogGroupCore,
          "format: 0x%x, width: %d, height: %d, timestamp: %" PRIu64
          ",sinkPortId: %d, newImageType:%d",
          format.format, format.width, format.height, timestamp, sinkPortId, newImageType);

    sp<MiaImage> outputImage = NULL;
    if (newImageType != MI_IMG_INTERNAL) {
        std::unique_lock<std::mutex> locker{mSinkImageMux};
        auto it = mSinkImageMap.equal_range(timestamp);
        while (it.first != it.second) {
            auto image = it.first->second;
            MLOGI(Mia2LogGroupCore, "PortId: %d", image->getPortId());
            if (image->getPortId() == sinkPortId) {
                outputImage = image;
                it.first = mSinkImageMap.erase(it.first);
                break;
            }
            it.first++;
        }
        locker.unlock();

        if (!outputImage) {
            outputImage = new MiaImage(format, timestamp, sinkPortId, mFrameCb, newImageType);
        }
    } else {
        std::lock_guard<std::mutex> lock(mAcquireBufferMutex);

        for (auto iter = mBufferQueueWrapperSet.begin(); iter != mBufferQueueWrapperSet.end();
             iter++) {
            BufferQueueWrapper *bufferQueueWrapper = *iter;
            if (bufferQueueWrapper != NULL && bufferQueueWrapper->isMatchFormat(format)) {
                outputImage = bufferQueueWrapper->dequeueBuffer();
            }
        }
        if (!outputImage) {
            buffer_type type = ION_TYPE;
            BufferQueueWrapper *bufferWraper = BufferQueueWrapper::create(
                format, type, mBufferWraperCount++, mInitBufferQueueDepth);
            if (bufferWraper != NULL) {
                bufferWraper->setBufferThresholdValue(mBufferThresholdValue);
                mBufferQueueWrapperSet.insert(bufferWraper);
                outputImage = bufferWraper->dequeueBuffer();
            }
        }
    }

    if (!outputImage) {
        MLOGE(Mia2LogGroupCore, "Drop  processImage: because of outputImage is null");
        return NULL;
    } else if (outputImage->getImageState() == MI_IMG_STATE_UNINITIALED) {
        MLOGE(Mia2LogGroupCore, "ERR! miaImage is not init state");
        return NULL;
    }

    MLOGD(Mia2LogGroupCore, "End timestamp: %" PRIu64, timestamp);
    return outputImage;
}

/*
 * Nodes to release intermediate buffers, the endPoint
 * just callback this buffer without "release"
 */
void MiaOfflineSession::releaseBuffer(sp<MiaImage> frame, bool requesetComplete)
{
    MLOGD(Mia2LogGroupCore, "%p getImageType %s", frame.get(),
          frame->getImageTypeName(frame->getImageType()));
    if (frame->getImageType() == MI_IMG_INTERNAL) {
        bool isMatch = false;
        Mutex::Autolock autoLock(mBufferQueueLock);
        for (auto iter = mBufferQueueWrapperSet.begin(); iter != mBufferQueueWrapperSet.end();
             iter++) {
            BufferQueueWrapper *bufferQueueWrapper = *iter;
            if (bufferQueueWrapper != NULL &&
                bufferQueueWrapper->isMatchFormat(frame->getImageFormat())) {
                bufferQueueWrapper->enqueueBuffer(frame);
                isMatch = true;
                break;
            }
        }
        if (isMatch == false)
            MLOGE(Mia2LogGroupCore,
                  "enqueue internalBuffer failed because of image format mismatch");
    } else if (frame->getImageType() == MI_IMG_SOURCE) {
        if (frame->decreaseUseCount() == 0) {
            frame->releaseExternalFrame();
        }
    } else if (frame->getImageType() == MI_IMG_SINK ||
               frame->getImageType() == MI_IMG_VIRTUAL_SINK) {
        MLOGI(Mia2LogGroupCore,
              "onFrameComing return frameNum:%llu, portId:%u, timestamp: %" PRIu64,
              frame->getFrameNumber(), frame->getPortId(), frame->getTimestamp());

        // return plugin's exif info to Camera App
        if (requesetComplete) {
            std::vector<std::string> allResult;
            bool haveExifInfo = false;
            {
                Mutex::Autolock autoLock(mAlgoResultLock);

                auto iter = mAlgoResultInfo.find(frame->getTimestamp());
                if (iter != mAlgoResultInfo.end()) {
                    allResult = iter->second;
                    haveExifInfo = true;
                    mAlgoResultInfo.erase(iter);
                }
            }

            if (haveExifInfo) {
                mSessionCb->onSessionCallback(MiaResultCbType::MIA_META_CALLBACK,
                                              frame->getFrameNumber(), frame->getTimestamp(),
                                              &allResult);
            }
        }

        if (frame->decreaseUseCount() == 0) {
            frame->releaseExternalFrame();
        }
    } else {
        MLOGE(Mia2LogGroupCore, "ImageType %d is not supported!", frame->getImageType());
    }
}

int MiaOfflineSession::getRunningTaskNum()
{
    std::lock_guard<std::mutex> lock(mBusyLock);
    return mBusyPipelines.size();
}

// flush return after all requests have ended
CDKResult MiaOfflineSession::flush(bool isForced)
{
    std::lock_guard<std::mutex> flushLock(mFlushLock);
    {
        MLOGI(Mia2LogGroupCore, "session[%p] flush(%d) begin", this, isForced);

        std::unique_lock<std::mutex> locker(mBusyLock);
        auto cloneBusyPipelines = mBusyPipelines;
        for (auto &pipeline : cloneBusyPipelines) {
            pipeline->flush(isForced); // flush pipeline
        }

        MLOGI(Mia2LogGroupCore, "session[%p] wait all pipeline finish", this);
        mBusyCond.wait(locker, [this]() { return mBusyPipelines.empty(); });
        MLOGI(Mia2LogGroupCore, "session[%p] all pipeline flush end", this);
    }
    {
        MLOGI(Mia2LogGroupCore, "session[%p] wait all node finish", this);
        std::unique_lock<std::mutex> lock(mAllNodeFinishLock);
        mAllNodeFinishCond.wait(lock, [this]() { return mNodeThreadNum == 0; });
        MLOGI(Mia2LogGroupCore, "session[%p] all node finish", this);
    }

    MLOGI(Mia2LogGroupCore, "session[%p] flush(%d) end", this, isForced);

    return MIAS_OK;
}

QFResult MiaOfflineSession::quickFinish(QuickFinishMessageInfo &messageInfo)
{
    QFResult result = pipelineNotFound;
    const char *fileName = messageInfo.fileName.c_str();

    std::lock_guard<std::mutex> lock(mBusyLock);
    for (auto &pipeline : mBusyPipelines) {
        const char *pipeName = pipeline->getPipelineName();
        if (strncmp(pipeName, fileName, std::min(strlen(pipeName), strlen(fileName))) == 0) {
            pipeline->quickFinish(messageInfo.needImageResult);
            result = messageSentFinish;
            break;
        }
    }

    return result;
}

void MiaOfflineSession::prepareForExifUpdate(sp<MiaImage> frame, camera_metadata_t *metadata,
                                             exifInfoOfRequest &exifInfoOfRequest)
{
    CDKResult ret = MIAS_OK;
    void *tagData = nullptr;
    if (1 == exifInfoOfRequest.mLastRequestSourceBufferNum) {
        exifInfoOfRequest.mFirstTimestamp = frame->getTimestamp();
        exifInfoOfRequest.mFirstFrameNum = frame->getFrameNumber();
        ret = VendorMetadataParser::getVTagValue(
            metadata, "xiaomi.performanceInfo.halRequestTimestamp", &tagData);
        if (MIAS_OK == ret) {
            exifInfoOfRequest.mGetFrameStartTimestamp = *static_cast<u_int64_t *>(tagData);
        }
        ret = VendorMetadataParser::getVTagValue(
            metadata, "xiaomi.performanceInfo.getFrameEndTimestamp", &tagData);
        if (MIAS_OK == ret) {
            exifInfoOfRequest.mGetFrameEndTimestamp = *static_cast<u_int64_t *>(tagData);
        }
    }
    ret =
        VendorMetadataParser::getVTagValue(metadata, "xiaomi.performanceInfo.shutterLag", &tagData);
    if (MIAS_OK == ret) {
        exifInfoOfRequest.mShutterLag = *static_cast<int32_t *>(tagData);
    }
    ret = VendorMetadataParser::getVTagValue(metadata, "xiaomi.performanceInfo.B2YCost", &tagData);
    if (MIAS_OK == ret) {
        exifInfoOfRequest.mB2YCount++;
        exifInfoOfRequest.mB2YCostTotal += *static_cast<double *>(tagData);
    }
    ret = VendorMetadataParser::getVTagValue(metadata, "xiaomi.performanceInfo.halRequestTimestamp",
                                             &tagData);
    if (MIAS_OK == ret) {
        u_int64_t halRequestTimestamp = *static_cast<u_int64_t *>(tagData);
        exifInfoOfRequest.mGetFrameStartTimestamp =
            (exifInfoOfRequest.mGetFrameStartTimestamp > halRequestTimestamp)
                ? halRequestTimestamp
                : exifInfoOfRequest.mGetFrameStartTimestamp;
    }
    ret = VendorMetadataParser::getVTagValue(
        metadata, "xiaomi.performanceInfo.getFrameEndTimestamp", &tagData);
    if (MIAS_OK == ret) {
        u_int64_t getFrameEndTimestamp = *static_cast<u_int64_t *>(tagData);
        exifInfoOfRequest.mGetFrameEndTimestamp =
            (exifInfoOfRequest.mGetFrameEndTimestamp > getFrameEndTimestamp)
                ? exifInfoOfRequest.mGetFrameEndTimestamp
                : getFrameEndTimestamp;
    }
    // for gyro
    ret =
        VendorMetadataParser::getVTagValue(metadata, "org.quic.camera.qtimer.timestamp", &tagData);
    if (MIAS_OK == ret) {
        u_int64_t timestamp = NanoToMicro(*static_cast<u_int64_t *>(tagData));
        exifInfoOfRequest.mTimestamp[exifInfoOfRequest.mLastRequestSourceBufferNum - 1] = timestamp;
    }
    ret = VendorMetadataParser::getVTagValue(metadata, "xiaomi.superResolution.pickAnchorIndex",
                                             &tagData);
    if (MIAS_OK == ret) {
        exifInfoOfRequest.mAnchorFrameIndex = *static_cast<u_int32_t *>(tagData);
    }
}

void MiaOfflineSession::updateExifInfo(camera_metadata_t *metadata,
                                       exifInfoOfRequest &exifInfoOfRequest)
{
    void *tagData = nullptr;
    CDKResult ret = MIAS_OK;
    std::string exifResult;
    uint64_t anchorFrameTimestamp =
        exifInfoOfRequest.mTimestamp[exifInfoOfRequest.mAnchorFrameIndex];
    ret = VendorMetadataParser::getVTagValue(metadata, "xiaomi.mivi.ellc.gyro", &tagData);
    if (MIAS_OK == ret) {
        ChiFrameGyro *gyro = static_cast<ChiFrameGyro *>(tagData);
        uint32_t count = gyro->gcount;
        if ((0 < count) && (count <= MAX_SAMPLES_BUF_SIZE)) {
            for (uint32_t i = 0; i < count; i++) {
                uint64_t timestamp = NanoToMicro(QtimerTicksToQtimerNano(gyro->t[i]));
                if (timestamp >= anchorFrameTimestamp) {
                    char gyroData[40] = {0};
                    snprintf(gyroData, sizeof(gyroData), "{x:%f, y:%f, z:%f}", gyro->x[i],
                             gyro->y[i], gyro->z[i]);
                    MLOGI(Mia2LogGroupCore, "Snapshot gyro info: x:%f, y:%f, z:%f", gyro->x[i],
                          gyro->y[i], gyro->z[i]);
                    exifResult = exifResult + " gyro:" + gyroData;
                    break;
                }
            }
        }
    }
    if (exifInfoOfRequest.mB2YCostTotal && exifInfoOfRequest.mB2YCount) {
        exifResult = exifResult + " b2yAvgCost:";
        char b2yAvgCost[16] = {0};
        snprintf(b2yAvgCost, sizeof(b2yAvgCost), "%0.3f",
                 exifInfoOfRequest.mB2YCostTotal / exifInfoOfRequest.mB2YCount);
        exifResult = exifResult + b2yAvgCost;
        exifInfoOfRequest.mB2YCount = 0;
        exifInfoOfRequest.mB2YCostTotal = 0.0;
    }
    if (exifInfoOfRequest.mShutterLag) {
        exifResult = exifResult + " shutterLag:";
        char shutterLag[16] = {0};
        snprintf(shutterLag, sizeof(shutterLag), "%d", exifInfoOfRequest.mShutterLag);
        exifResult = exifResult + shutterLag;
        exifInfoOfRequest.mShutterLag = 0;
    }
    if (exifInfoOfRequest.mGetFrameStartTimestamp && exifInfoOfRequest.mGetFrameEndTimestamp) {
        exifResult = exifResult + " getFrameCost:";
        char getFrameCost[16] = {0};
        snprintf(
            getFrameCost, sizeof(getFrameCost), "%0.3f",
            (exifInfoOfRequest.mGetFrameEndTimestamp - exifInfoOfRequest.mGetFrameStartTimestamp) /
                1000000.0);
        exifResult = exifResult + getFrameCost;
        MLOGI(
            Mia2LogGroupCore, "Process get frame cost time:%0.3fms",
            (exifInfoOfRequest.mGetFrameEndTimestamp - exifInfoOfRequest.mGetFrameStartTimestamp) /
                1000000.0);
        exifInfoOfRequest.mGetFrameStartTimestamp = 0;
        exifInfoOfRequest.mGetFrameEndTimestamp = 0;
    }
    onResultComing(MiaResultCbType::MIA_META_CALLBACK, exifInfoOfRequest.mFirstFrameNum,
                   exifInfoOfRequest.mFirstTimestamp, exifResult);
    exifInfoOfRequest.mFirstTimestamp = 0;
    exifInfoOfRequest.mFirstFrameNum = 0;
}

bool MiaOfflineSession::rebuildLinks(MiaParams &settings, PipelineInfo &pipeline,
                                     uint32_t firstFrameInputFormat,
                                     std::vector<MiaImageFormat> &sinkBufferFormat)
{
    return mPipelinePruner->rebuildLinks(settings, pipeline, firstFrameInputFormat,
                                         sinkBufferFormat);
}

void MiaOfflineSession::signalThread()
{
    std::lock_guard<std::mutex> locker(mTimeoutMux);
    mIsInitDone = true;
    mTimeoutCond.notify_one();
}

void MiaOfflineSession::timeoutDetection(uint32_t tid, std::string &initInfo)
{
    std::unique_lock<std::mutex> lck(mTimeoutMux);
    if (!mTimeoutCond.wait_for(lck, std::chrono::seconds(kInitWaitTimeoutSec),
                               [this] { return mIsInitDone; })) {
        MLOGI(Mia2LogGroupCore,
              "[%p] init timeout duing [%s] init, initInfo:%s, tid:%u. callstack ===>", this,
              mTimeoutNode.c_str(), initInfo.c_str(), tid);
        midebug::dumpCurProcessCallStack("init-timeout", ANDROID_LOG_INFO);

        MLOGE(Mia2LogGroupCore, "[%p] init timeout duing [%s] init, initInfo:%s, tid:%u, abort!",
              this, mTimeoutNode.c_str(), initInfo.c_str(), tid);
        MASSERT(false);
    }
}

void MiaOfflineSession::getFaceExifInfo(camera_metadata_t *metaData, uint32_t w, uint32_t h,
                                        uint32_t frameNum, int64_t timeStamp)
{
    MiFaceInfo faceInfo[16] = {};
    MiaRect cropRegion;
    memset(&cropRegion, 0, sizeof(MiaRect));
    int faceNum = 0;
    std::string exifResult = "";

    void *data = NULL;
    float zoomRatio = 1.0;

    const char *cropInfo = "xiaomi.snapshot.cropRegion";
    int isAndroidRegion = VendorMetadataParser::getVTagValue(metaData, cropInfo, &data);
    if (data != NULL) {
        cropRegion = *reinterpret_cast<MIARECT *>(data);
        MLOGI(Mia2LogGroupCore, "get CropRegion begin [%d, %d, %d, %d]", cropRegion.left,
              cropRegion.top, cropRegion.width, cropRegion.height);
    } else {
        MLOGI(Mia2LogGroupCore, "get crop failed, try ANDROID_SCALER_CROP_REGION");
        VendorMetadataParser::getTagValue(metaData, ANDROID_SCALER_CROP_REGION, &data);
        if (data != NULL) {
            cropRegion = *reinterpret_cast<MIARECT *>(data);
            MLOGI(Mia2LogGroupCore, "get CropRegion begin [%d, %d, %d, %d]", cropRegion.left,
                  cropRegion.top, cropRegion.width, cropRegion.height);
        } else {
            MLOGI(Mia2LogGroupCore, "get CropRegion failed");
            cropRegion.left = 0;
            cropRegion.top = 0;
            cropRegion.width = w;
            cropRegion.height = h;
        }
    }

    if (cropRegion.width == 0 || cropRegion.height == 0) {
        cropRegion.left = 0;
        cropRegion.top = 0;
        cropRegion.width = w;
        cropRegion.height = h;
    }

    if (cropRegion.width != 0)
        zoomRatio = (float)w / cropRegion.width;

    float scaleWidth = 0.0;
    float scaleHeight = 0.0;
    if (w != 0 && h != 0) {
        scaleWidth = (float)cropRegion.width / w;
        scaleHeight = (float)cropRegion.height / h;
    } else {
        MLOGW(Mia2LogGroupCore, "get inputimage's width failed");
    }
    const float AspectRatioTolerance = 0.01f;

    if (scaleHeight > scaleWidth + AspectRatioTolerance) {
        // 16:9 case run to here, it means height is cropped.
        int32_t deltacropHeight = (int32_t)((float)h * scaleWidth);
        cropRegion.top = cropRegion.top + (cropRegion.height - deltacropHeight) / 2;
    } else if (scaleWidth > scaleHeight + AspectRatioTolerance) {
        // 1:1 width is cropped.
        int32_t deltacropWidth = (int32_t)((float)w * scaleHeight);
        cropRegion.left = cropRegion.left + (cropRegion.width - deltacropWidth) / 2;
        if (cropRegion.height != 0) {
            zoomRatio = (float)h / cropRegion.height;
        } else {
            MLOGW(Mia2LogGroupCore, "get cropRegion or inputimage's width failed");
        }
    }

    data = NULL;
    const char *faceRect = "xiaomi.snapshot.faceRect";
    VendorMetadataParser::getVTagValue(metaData, faceRect, &data);
    if (data != NULL) {
        exifResult = "FaceRoi: size:";
        MiFaceResult *tmpData = reinterpret_cast<MiFaceResult *>(data);
        faceNum = tmpData->faceNumber;

        int faces = (min(16, faceNum));
        char faceNumber[40] = {0};
        snprintf(faceNumber, sizeof(faceNumber), "%d ", faces);
        exifResult = exifResult + faceNumber;

        for (int index = 0; index < (min(16, faceNum)); index++) {
            MiRectEXT curFaceInfo = tmpData->miFaceInfo[index];
            int32_t xMin = (curFaceInfo.left - static_cast<int32_t>(cropRegion.left)) * zoomRatio;
            int32_t xMax = (curFaceInfo.right - static_cast<int32_t>(cropRegion.left)) * zoomRatio;
            int32_t yMin = (curFaceInfo.bottom - static_cast<int32_t>(cropRegion.top)) * zoomRatio;
            int32_t yMax = (curFaceInfo.top - static_cast<int32_t>(cropRegion.top)) * zoomRatio;

            xMin = max(min(xMin, (int32_t)w - 2), 0);
            xMax = max(min(xMax, (int32_t)w - 2), 0);
            yMin = max(min(yMin, (int32_t)h - 2), 0);
            yMax = max(min(yMax, (int32_t)h - 2), 0);

            faceInfo[index].left = xMin;
            faceInfo[index].top = yMin;
            faceInfo[index].right = xMax;
            faceInfo[index].bottom = yMax;

            char faceData[40] = {0};
            snprintf(faceData, sizeof(faceData), "[%d, %d, %d, %d]", faceInfo[index].left,
                     faceInfo[index].top, faceInfo[index].right, faceInfo[index].bottom);
            exifResult = exifResult + faceData;
            MLOGI(Mia2LogGroupCore, "index:%d faceRect[%d,%d,%d,%d] cvt [%d,%d,%d,%d]", index,
                  curFaceInfo.left, curFaceInfo.bottom, curFaceInfo.right, curFaceInfo.top,
                  faceInfo[index].left, faceInfo[index].top, faceInfo[index].right,
                  faceInfo[index].bottom);
        }
    } else {
        MLOGW(Mia2LogGroupCore, "faces are not found");
    }
    onResultComing(MiaResultCbType::MIA_META_CALLBACK, frameNum, timeStamp, exifResult);
}

} // namespace mialgo2
