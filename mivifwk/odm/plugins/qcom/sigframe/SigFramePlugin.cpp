#include "SigFramePlugin.h"

using namespace mialgo2;

static const int32_t sIsDumpData =
    property_get_int32("persist.vendor.camera.algoengine.sigframe.dump", 0);

SigFramePlugin::~SigFramePlugin()
{
    MLOGI(Mia2LogGroupPlugin, "%p", this);
}

int SigFramePlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    MLOGI(Mia2LogGroupPlugin, "%p", this);
    mNodeInterface = nodeInterface;
    mStatus = INIT;
    pMultiNum = 1; // for test 3
    mProcThread = thread([this]() { loopFunc(); });
    return 0;
}

int SigFramePlugin::processFrame(uint32_t inputFrameNum)
{
    int ret = 0;

    ImageQueueWrapper inputQueueWrapper = mInputQWrapperQueue.front();
    ImageQueue inputImageQueue = inputQueueWrapper.imageQueue;
    uint32_t frameNum = inputQueueWrapper.frameNum;
    mInputQWrapperQueue.pop();

    ImageQueue outputImageQueue;
    auto it = mOutputQueueMap.find(frameNum);
    if (it == mOutputQueueMap.end()) {
        MLOGE(Mia2LogGroupPlugin, "Can find outputImageQueue for framenum: %u", frameNum);
        ret = -1;
    } else {
        outputImageQueue = it->second;
        mOutputQueueMap.erase(frameNum);
    }

    shared_ptr<MiImageBuffer> inputFrame = nullptr, outputFrame = nullptr;

    shared_ptr<MiImageBuffer> mainFrame(new MiImageBuffer);
    inputFrame = inputImageQueue.front();
    cpyFrame(mainFrame.get(), inputFrame.get());

    while (inputImageQueue.size() && outputImageQueue.size()) {
        inputFrame = inputImageQueue.front();
        inputImageQueue.pop();
        outputFrame = outputImageQueue.front();
        outputImageQueue.pop();

        ret = PluginUtils::miCopyBuffer(outputFrame.get(), inputFrame.get());
        PluginUtils::miFreeBuffer(inputFrame.get());
        inputFrame = nullptr;
        outputFrame = nullptr;
        if (ret == -1) {
            MLOGE(Mia2LogGroupPlugin, "Cpy input frame %u to output frame failed!", frameNum);
        }
    }

    while (!outputImageQueue.empty()) {
        outputFrame = outputImageQueue.front();
        outputImageQueue.pop();
        ret = PluginUtils::miCopyBuffer(outputFrame.get(), mainFrame.get());
        outputFrame = nullptr;
        if (ret == -1) {
            MLOGE(Mia2LogGroupPlugin, "Cpy input frame %u to output frame failed!", frameNum);
        }
    }
    PluginUtils::miFreeBuffer(mainFrame.get());
    mainFrame = nullptr;
    // notify setResultBuffer
    mNodeInterface.pSetResultBuffer(mNodeInterface.owner, frameNum);

    while (!inputImageQueue.empty()) {
        inputFrame = inputImageQueue.front();
        inputImageQueue.pop();
        PluginUtils::miFreeBuffer(inputFrame.get());
        inputFrame = nullptr;
    }

    // pop remaining input buffer
    while (--inputFrameNum) {
        inputQueueWrapper = mInputQWrapperQueue.front();
        mInputQWrapperQueue.pop();
        inputImageQueue = inputQueueWrapper.imageQueue;
        while (!inputImageQueue.empty()) {
            inputFrame = inputImageQueue.front();
            inputImageQueue.pop();
            PluginUtils::miFreeBuffer(inputFrame.get());
            inputFrame = nullptr;
        }
    }
    return ret;
}

void SigFramePlugin::loopFunc()
{
    while (true) {
        // one by one
        unique_lock<std::mutex> threadLocker(mMutex);
        mCond.wait(threadLocker);

        lock_guard<std::mutex> autoLocker(mImageMutex);
        MLOGI(Mia2LogGroupPlugin, "inputFrame count:%d outputFrame count:%d",
              mInputQWrapperQueue.size(), mOutputQueueMap.size());

        if (mStatus == NEEDPROC && mInputQWrapperQueue.size() == pMultiNum) {
            if (!processFrame(pMultiNum)) {
                MLOGI(Mia2LogGroupPlugin, "Process Request finish");
            }
            mStatus = PROCESSED;

        } else if (mStatus == NEEDFLUSH && !mInputQWrapperQueue.empty()) {
            uint32_t index = mInputQWrapperQueue.size() % pMultiNum;
            index = (index > 0) ? index : pMultiNum;
            if (!processFrame(index)) {
                MLOGI(Mia2LogGroupPlugin, "Flush Request finish");
            }
            mStatus = FLUSHD;
        } else if (mStatus == END) {
            break;
        }
    }
}

int SigFramePlugin::cpyFrame(MiImageBuffer *dstFrame, MiImageBuffer *srcFrame)
{
    dstFrame->format = srcFrame->format;
    dstFrame->width = srcFrame->width;
    dstFrame->height = srcFrame->height;
    int ret =
        PluginUtils::miAllocBuffer(dstFrame, srcFrame->width, srcFrame->height, srcFrame->format);
    if (!ret) {
        MLOGE(Mia2LogGroupPlugin, "inputFrame memory alloc failed!");
        return false;
    }
    dstFrame->stride = srcFrame->stride;
    dstFrame->scanline = srcFrame->scanline;
    dstFrame->numberOfPlanes = srcFrame->numberOfPlanes;

    ret = PluginUtils::miCopyBuffer(dstFrame, srcFrame);
    if (ret == -1) {
        PluginUtils::miFreeBuffer(dstFrame);
        MLOGE(Mia2LogGroupPlugin, "inputFrame buffer cpy failed!");
        return false;
    }

    return true;
}

ProcessRetStatus SigFramePlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    int ret = 0;
    uint32_t frameNum = processRequestInfo->frameNum;

    int32_t iIsDumpData = sIsDumpData;

    uint32_t inNum = processRequestInfo->phInputBuffer.size();
    uint32_t outNum = processRequestInfo->phOutputBuffer.size();

    if (!inNum) {
        MLOGE(Mia2LogGroupPlugin, "Frame %d stream %d input buffer is none!",
              processRequestInfo->frameNum, processRequestInfo->streamId);
        resultInfo = PROCFAILED;
        return resultInfo;
    }

    void *data = nullptr;
    camera_metadata_t *metadata = processRequestInfo->phInputBuffer[0].pMetadata;
    VendorMetadataParser::getVTagValue(metadata, "xiaomi.multiframe.inputNum", &data);
    if (data != nullptr) {
        pMultiNum = *static_cast<uint32_t *>(data);
    }

    if (processRequestInfo->isFirstFrame && !outNum) {
        MLOGE(Mia2LogGroupPlugin, "Frame %d stream %d as first frame, output buffer is none!",
              processRequestInfo->frameNum, processRequestInfo->streamId);

        resultInfo = PROCFAILED;
        return resultInfo;
    }

    ImageQueueWrapper inputQueueWrapper;
    MiImageBuffer inputFrame;
    inputQueueWrapper.frameNum = frameNum;
    for (uint32_t i = 0; i < inNum; ++i) {
        inputFrame.format = processRequestInfo->phInputBuffer[i].format.format;
        inputFrame.width = processRequestInfo->phInputBuffer[i].format.width;
        inputFrame.height = processRequestInfo->phInputBuffer[i].format.height;
        inputFrame.plane[0] = processRequestInfo->phInputBuffer[i].pAddr[0];
        inputFrame.plane[1] = processRequestInfo->phInputBuffer[i].pAddr[1];
        inputFrame.stride = processRequestInfo->phInputBuffer[i].planeStride;
        inputFrame.scanline = processRequestInfo->phInputBuffer[i].sliceheight;
        inputFrame.numberOfPlanes = processRequestInfo->phInputBuffer[i].numberOfPlanes;

        shared_ptr<MiImageBuffer> storeFrame(new MiImageBuffer);
        ret = cpyFrame(storeFrame.get(), &inputFrame);
        if (!ret) {
            MLOGE(Mia2LogGroupPlugin, "Frame %d stream %d push input frame failed!",
                  processRequestInfo->frameNum, processRequestInfo->streamId);
            resultInfo = PROCBADSTATE;
            return resultInfo;
        }
        inputQueueWrapper.imageQueue.push(storeFrame);
    }

    MLOGI(Mia2LogGroupPlugin,
          "Frame %d, input width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          processRequestInfo->frameNum, inputFrame.width, inputFrame.height, inputFrame.stride,
          inputFrame.scanline, inputFrame.format);

    ImageQueue outputImageQueue;
    for (uint32_t i = 0; i < outNum; ++i) {
        shared_ptr<MiImageBuffer> outputFrame(new MiImageBuffer);
        outputFrame->format = processRequestInfo->phOutputBuffer[i].format.format;
        outputFrame->width = processRequestInfo->phOutputBuffer[i].format.width;
        outputFrame->height = processRequestInfo->phOutputBuffer[i].format.height;
        outputFrame->plane[0] = processRequestInfo->phOutputBuffer[i].pAddr[0];
        outputFrame->plane[1] = processRequestInfo->phOutputBuffer[i].pAddr[1];
        outputFrame->stride = processRequestInfo->phOutputBuffer[i].planeStride;
        outputFrame->scanline = processRequestInfo->phOutputBuffer[i].sliceheight;
        outputFrame->numberOfPlanes = processRequestInfo->phOutputBuffer[i].numberOfPlanes;
        outputImageQueue.push(outputFrame);
    }

    {
        lock_guard<std::mutex> autoLock(mImageMutex);
        mInputQWrapperQueue.push(inputQueueWrapper);
        if (!outputImageQueue.empty()) {
            mOutputQueueMap[frameNum] = outputImageQueue;
        }
        lock_guard<std::mutex> threadLocker(mMutex);
        mStatus = NEEDPROC;
        mCond.notify_one();
    }

    if (iIsDumpData) {
        char inputBuf[128];
        snprintf(inputBuf, sizeof(inputBuf), "sigframe_input_%dx%d", inputFrame.width,
                 inputFrame.height);
        PluginUtils::dumpToFile(inputBuf, &inputFrame);
    }

    MLOGI(Mia2LogGroupPlugin, "Got Process Request , test!");
    return resultInfo;
}

ProcessRetStatus SigFramePlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int SigFramePlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    int rc = 0;
    lock_guard<std::mutex> autoLocker(mImageMutex);
    if (!mInputQWrapperQueue.empty() && !mOutputQueueMap.empty()) {
        lock_guard<std::mutex> locker(mMutex);
        mStatus = NEEDFLUSH;
        MLOGD(Mia2LogGroupPlugin, "flush notify!");
        mCond.notify_one();
    } else if (!mInputQWrapperQueue.empty()) {
        ImageQueueWrapper inputQueueWrapper;
        ImageQueue inputImageQueue;
        shared_ptr<MiImageBuffer> inputFrame = nullptr;

        while (!mInputQWrapperQueue.empty()) {
            inputQueueWrapper = mInputQWrapperQueue.front();
            mInputQWrapperQueue.pop();
            inputImageQueue = inputQueueWrapper.imageQueue;
            while (!inputImageQueue.empty()) {
                inputFrame = inputImageQueue.front();
                inputImageQueue.pop();
                PluginUtils::miFreeBuffer(inputFrame.get());
                inputFrame = nullptr;
            }
        }
        MLOGE(Mia2LogGroupPlugin, "Output Image Queue is empty");
        rc = -1;
    } else if (!mOutputQueueMap.empty()) {
        ImageQueue outputImageQueue;
        for (auto iter : mOutputQueueMap) {
            outputImageQueue = iter.second;
            while (!outputImageQueue.empty()) {
                outputImageQueue.pop();
            }
        }
        mOutputQueueMap.clear();
        MLOGE(Mia2LogGroupPlugin, "Input Image Queue is empty");
        rc = -1;
    }

    return rc;
}

void SigFramePlugin::destroy()
{
    {
        lock_guard<std::mutex> threadLocker(mMutex);
        mStatus = END;
        mCond.notify_one();
    }
    mProcThread.join();

    lock_guard<std::mutex> autoLocker(mImageMutex);
    if (!mInputQWrapperQueue.empty()) {
        ImageQueueWrapper inputQueueWrapper;
        ImageQueue inputImageQueue;
        shared_ptr<MiImageBuffer> inputFrame = nullptr;

        while (!mInputQWrapperQueue.empty()) {
            inputQueueWrapper = mInputQWrapperQueue.front();
            mInputQWrapperQueue.pop();
            inputImageQueue = inputQueueWrapper.imageQueue;
            while (!inputImageQueue.empty()) {
                inputFrame = inputImageQueue.front();
                inputImageQueue.pop();
                PluginUtils::miFreeBuffer(inputFrame.get());
                inputFrame = nullptr;
            }
        }
    }
    if (!mOutputQueueMap.empty()) {
        ImageQueue outputImageQueue;
        for (auto iter : mOutputQueueMap) {
            outputImageQueue = iter.second;
            while (!outputImageQueue.empty()) {
                outputImageQueue.pop();
            }
        }
        mOutputQueueMap.clear();
    }

    MLOGE(Mia2LogGroupPlugin, "destroy %p.", this);
}
