#include "MiaPipeline.h"

namespace mialgo2 {
const char *NodeProcessInfoToChar[] = {"ProcessOK", "GetInputError", "GetOutputError", "FlushTask",
                                       "NodeProcessError"};

Pipeline::Pipeline(camera_metadata_t *metadata, MiaCreateParams *params,
                   MiaProcessParams *processParams, int64_t timestampUs,
                   const std::vector<uint32_t> &sourcePortIdVec)
{
    mSettings.metadata = metadata;
    mSinkBufferFormats = params->outputFormat;
    mSessionCB = NULL;
    mRunningNodeNum = 0;
    mHasProcessError = false;
    mSinkNumToReturn = 0;
    mFIFOPriority = 0;
    mLIFOPriority = 0;
    mMiviProcessStartTime = 0;
    mFlushStatus = NoFlush;
    mIsCallExternal = false;
    mTimestampUs = timestampUs;
    mFirstFrameInputFormat = processParams->inputFrame->format.format;

    std::lock_guard<std::mutex> locker(mSourceBufferLock);
    for (auto &sourcePortId : sourcePortIdVec) { // create all sourceBufferQueue
        if (mSourceBufferQueueMap.find(sourcePortId) == mSourceBufferQueueMap.end()) {
            mSourceBufferQueueMap[sourcePortId].clearQueue(); // new a empty queue
        }
    }
}

void Pipeline::setSession(PipelineCB *session)
{
    if (session != NULL) {
        mSessionCB = session;
    }
}

void Pipeline::start()
{
    Singleton<ThreadPool>::getInstance()->enqueue([this]() { postProcTask(); });
}

bool Pipeline::rebuildLinks()
{
    double startTime = MiaUtil::nowMSec();

    bool result = mSessionCB->rebuildLinks(mSettings, mPipelineInfo, mFirstFrameInputFormat,
                                           mSinkBufferFormats);

    char processTime[16] = {0};
    snprintf(processTime, sizeof(processTime), "%0.2fms", MiaUtil::nowMSec() - startTime);
    MLOGI(Mia2LogGroupCore, "rebuildLink use %s", processTime);

    for (int i = 0; i < mPipelineInfo.links.size(); i++) {
        if (mPipelineInfo.links[i].type == SinkLink) {
            mSinkNumToReturn++;
        }
    }

    for (int i = 0; i < mPipelineInfo.links.size(); i++) {
        LinkItem link = mPipelineInfo.links[i];
        MLOGI(Mia2LogGroupCore,
              "pruned Link[%d] [%s:outport %d] -----> [%s:inport %d] ,linkType:%d", i,
              link.srcNodeInstance.c_str(), link.srcNodePortId, link.dstNodeInstance.c_str(),
              link.dstNodePortId, link.type);
    }

    return result;
}

void Pipeline::postProcTask()
{
    mMiviProcessStartTime = MiaUtil::nowMSec();
    CDKResult rc;
    MLOGI(Mia2LogGroupCore, "pipeline[%s][%p] is running", getPipelineName(), this);
    if (mFlushStatus == ForcedFlush) {
        releaseUnusedBuffer(); // release unused sourceBuffer
        mSessionCB->moveToIdleQueue(this);
        return;
    }

    if (!rebuildLinks()) {
        std::string data = "";
        mSessionCB->onResultComing(MiaResultCbType::MIA_ABNORMAL_CALLBACK, 0, mTimestampUs, data);
        releaseUnusedBuffer();
        mSessionCB->moveToIdleQueue(this);
        MLOGE(Mia2LogGroupCore, "pipeline[%s] rebuildlink error", getPipelineName());
        return;
    }
    std::vector<LinkItem> &pipelineLinks = mPipelineInfo.links;
    std::set<std::string> sourceNodes;
    std::vector<std::string> performNodes;

    for (int index = 0; index < pipelineLinks.size(); index++) {
        LinkItem &link = pipelineLinks[index];
        std::string &linkSrcInstance = link.srcNodeInstance;
        std::string &linkDstInstance = link.dstNodeInstance;

        bool isSinkBuffer = strstr(linkDstInstance.c_str(), "SinkBuffer") != NULL ? true : false;
        if (!isSinkBuffer && mDependenceNum.find(linkDstInstance) == mDependenceNum.end()) {
            mDependenceNum[linkDstInstance] = 0;
        }

        if (link.type == SourceLink) {
            mInputLinkMap[linkDstInstance].push_back(index);

            sourceNodes.insert(linkDstInstance);
        } else if (link.type == SinkLink) {
            mOutputLinkMap[linkSrcInstance].push_back(index);
        } else {
            // the current link is the inputLink of currentLink's DstNodeInstance
            mInputLinkMap[linkDstInstance].push_back(index);
            // the current link is the outputLink of currentLink's SrcNodeInstance
            mOutputLinkMap[linkSrcInstance].push_back(index);

            if (!isSinkBuffer) {
                ++mDependenceNum[linkDstInstance];
            }
        }

        mBuffers[linkSrcInstance].outputBuffers[link.srcNodePortId].clearQueue();
        mBuffers[linkDstInstance].inputBuffers[link.dstNodePortId].clearQueue();
        mBuffers[linkDstInstance].usingInputBuffers[link.dstNodePortId].clearQueue();
    }

    for (auto it : mInputLinkMap) {
        int currentNodeMask = mPipelineInfo.nodes.at(it.first).nodeMask;
        if ((currentNodeMask & SIGFRAME_MODE) && it.second.size() > 1) {
            MLOGE(Mia2LogGroupCore,
                  "pipeline[%s] topology error, frame-by-frame node:%s have more inputLink",
                  getPipelineName(), it.first.c_str(), currentNodeMask);
            releaseUnusedBuffer();
            mSessionCB->moveToIdleQueue(this);
            return;
        }
    }

    for (auto it = mDependenceNum.begin(); it != mDependenceNum.end();) {
        if (it->second == 0) {
            performNodes.push_back(it->first);
            mDependenceNum.erase(it++);
        } else {
            ++it;
        }
    }
    // Executable nodes are sorted in ascending order of SourcePortId at initialization
    // The input order of mialgoengine is from small to large according to the SourcePortId
    for (auto &instance : performNodes) {
        if (sourceNodes.find(instance) != sourceNodes.end()) {
            mRunSequence.push(instance);
        }
    }
    // Free memory that will not be used again
    sourceNodes.clear();
    performNodes.clear();

    MLOGI(Mia2LogGroupCore,
          "pipeline[%s] the preparatory work has been completed and is ready for formal processing",
          getPipelineName());

    while (!mRunSequence.empty()) {
        std::unique_lock<std::mutex> locker(mLock);

        std::string processInstance = "";
        if (!mRunSequence.empty()) {
            processInstance = mRunSequence.front();
            std::lock_guard<std::mutex> recordlock(mRecordProcessNodeLock);
            mCurrentProcessNodes.insert(processInstance);
            mRunSequence.pop();
        }

        locker.unlock();
        CDKResult processResult = postProcess(processInstance);
        locker.lock();

        if (processResult == MIAS_OK) {
            mRunningNodeNum += 1;
        }

        if (mRunSequence.empty()) {
            MLOGD(Mia2LogGroupCore, "pipeline[%s] mRunSequenceEmpty:%d mRunningNodeNum:%d",
                  getPipelineName(), mRunSequence.empty(), mRunningNodeNum);
            mCond.wait(locker,
                       [this]() { return (!mRunSequence.empty() || mRunningNodeNum == 0); });
        }
    }

    MLOGI(Mia2LogGroupCore, "pipeline:[%s] process finish", getPipelineName());

    releaseUnusedBuffer();
    mSessionCB->moveToIdleQueue(this);
}

CDKResult Pipeline::postProcess(std::string instance)
{
    CDKResult rc = MIAS_OK;

    sp<MiaNode> processNode = mSessionCB->getProcessNode(instance);
    if (processNode != NULL) {
        processNode->enqueueTask(this);
    } else {
        rc = MIAS_OPEN_FAILED;
        MLOGE(Mia2LogGroupCore, "pipeline[%s] not find this node, nodeInstance[%s]",
              getPipelineName(), instance.c_str());
    }

    return rc;
}

bool Pipeline::checkInputFormat(sp<MiaImage> inputImage, const LinkItem &linkItem, int portId)
{
    if (MiaInternalProp::getInstance()->getBool(OpenPrunerAbnormalDump)) {
        uint32_t inputFormat = inputImage->getImageFormat().format;
        const std::vector<MiaPixelFormat> &dstFormatVec = linkItem.dstPortFormat;

        if (find(dstFormatVec.begin(), dstFormatVec.end(), inputFormat) == dstFormatVec.end()) {
            if (dstFormatVec[0] != CAM_FORMAT_UNDEFINED &&
                linkItem.srcPortFormat != CAM_FORMAT_UNDEFINED) {
                std::ostringstream dstFormats;
                for (auto i : dstFormatVec) {
                    dstFormats << i << ' ';
                }
                MLOGE(Mia2LogGroupCore,
                      "[%s] input format %d is different from expected format %s from Port %d",
                      linkItem.dstNodeInstance.c_str(), inputFormat, dstFormats.str().c_str(),
                      portId);
                return false;
            }
        }
    }
    return true;
}

CDKResult Pipeline::getInputImage(const std::string &instance, int nodeMask,
                                  std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImages)
{
    MLOGI(Mia2LogGroupCore, "pipeline[%s] instance[%s] get inputImages begin", getPipelineName(),
          instance.c_str());
    CDKResult rc = MIAS_OK;

    const std::vector<int> &inputLinks = mInputLinkMap[instance];

    for (int i = 0; i < inputLinks.size(); i++) {
        sp<MiaImage> inputImage = nullptr;
        int linkIndex = inputLinks[i];
        const LinkItem &linkItem = mPipelineInfo.links[linkIndex];
        int inputPort = linkItem.dstNodePortId;

        if (linkItem.type == SourceLink) {
            int sourcePortId = linkItem.srcNodePortId;
            inputImage = getSourceImage(sourcePortId);

            if (inputImage == nullptr) {
                return MIAS_NO_MEM;
            }
            mBuffers.at(instance).usingInputBuffers.at(inputPort).push(inputImage);
            inputImages[inputPort].push_back(inputImage);

            // Used to check whether the source port format is correct,It needs to be placed after
            // the usingInputBuffers push operation to prevent memory leaks
            if (!checkInputFormat(inputImage, linkItem, sourcePortId)) {
                return MIAS_ABNORMAL;
            }

            int inputTotalNum = inputImage->getMergeInputNumber();

            if (!(nodeMask & SIGFRAME_MODE)) {
                int numberOfResidualInput = inputTotalNum - 1;
                MLOGD(Mia2LogGroupCore, "pipeline[%s] get sourceImage:%p 1/%d of sourcePort:%d",
                      getPipelineName(), inputImage.get(), inputTotalNum, sourcePortId);
                for (int j = 0; j < numberOfResidualInput; j++) {
                    inputImage = getSourceImage(sourcePortId);
                    MLOGD(Mia2LogGroupCore,
                          "pipeline[%s] get sourceImage:%p %d/%d of "
                          "sourcePort:%d",
                          getPipelineName(), inputImage.get(), j + 2, inputTotalNum, sourcePortId);

                    if (inputImage == nullptr) {
                        return MIAS_NO_MEM;
                    }

                    inputImages[inputPort].push_back(inputImage);
                    mBuffers.at(instance).usingInputBuffers.at(inputPort).push(inputImage);
                }
            }

            MLOGI(Mia2LogGroupCore, "pipeline[%s] total sourceImage number:%d  of sourcePort:%d",
                  getPipelineName(), inputTotalNum, sourcePortId);
        } else if (linkItem.type == InternalLink) {
            MiaImageQueue &currentInputImageQueue =
                mBuffers.at(instance).inputBuffers.at(inputPort);
            MiaImageQueue &currentUsingInputImageQueue =
                mBuffers.at(instance).usingInputBuffers.at(inputPort);

            sp<MiaImage> inputImage = nullptr;
            if (nodeMask & SIGFRAME_MODE) {
                /* must use waitAndPop !!!
                   A->B , A and B are frame-by-frame node ,
                   B When the first frame is processed, A has not processed the second frame
                */
                if (currentInputImageQueue.waitAndPop(inputImage)) {
                    MLOGD(Mia2LogGroupCore, "pipeline[%s] %s:inputPort:%d get image:%p",
                          getPipelineName(), instance.c_str(), inputPort, inputImage.get());

                    currentUsingInputImageQueue.push(inputImage);
                    inputImages[inputPort].push_back(inputImage);

                    if (!checkInputFormat(inputImage, linkItem, inputPort)) {
                        return MIAS_ABNORMAL;
                    }
                    inputImage = nullptr;
                } else {
                    MLOGE(Mia2LogGroupCore, "pipeline[%s] %s:inputPort:%d get image fail",
                          getPipelineName(), instance.c_str(), inputPort);
                    return MIAS_NO_MEM;
                }
            } else {
                while (currentInputImageQueue.tryPop(inputImage)) {
                    MLOGD(Mia2LogGroupCore, "pipeline[%s] %s:inputPort:%d get image:%p",
                          getPipelineName(), instance.c_str(), inputPort, inputImage.get());
                    currentUsingInputImageQueue.push(inputImage);
                    inputImages[inputPort].push_back(inputImage);

                    if (!checkInputFormat(inputImage, linkItem, inputPort)) {
                        return MIAS_ABNORMAL;
                    }

                    inputImage = nullptr;
                }
            }
        }
    }

    MLOGI(Mia2LogGroupCore, "pipeline[%s] instance[%s] get inputImages end", getPipelineName(),
          instance.c_str());
    return rc;
}

CDKResult Pipeline::releaseInputBuffers(std::string instance, bool releaseOneInputBuffer)
{
    MLOGI(Mia2LogGroupCore, "pipeline[%s] releaseInputBuffers begin", getPipelineName());

    int currentNodeMask = mPipelineInfo.nodes.at(instance).nodeMask;
    if (isInplaceNode(instance, currentNodeMask) || (currentNodeMask & VIRTUAL_MODE)) {
        /*
         * 1. inplaceNode has multiple input ports and only one output port. It is necessary to send
         *    the images in all inputPorts of inplaceNode to the childNode corresponding to the
         *    inplaceNode's output port.
         * 2. virtualNode has multiple output ports and only one input port. It is necessary to send
         *    the images in inputPort of virtualNode to all childNodes corresponding to the
         *    virtualNode's output ports.
         */
        const std::vector<int> outputLinks = mOutputLinkMap[instance];
        auto &usingInputBufferMap = mBuffers.at(instance).usingInputBuffers;

        for (auto &it : usingInputBufferMap) {
            sp<MiaImage> image = nullptr;
            auto func = [this, &image, &outputLinks, &instance]() {
                // need move this image to all outputPort of current Node
                for (int i = 0; i < outputLinks.size(); i++) {
                    int outputLinkIndex = outputLinks[i];
                    int childInputPortId = mPipelineInfo.links[outputLinkIndex].dstNodePortId;

                    const std::string &childInstance =
                        mPipelineInfo.links[outputLinkIndex].dstNodeInstance;

                    image->increaseUseCount(); // this iamge is required by more nodes

                    // move buffer to ChildNodeInstance's inputPort
                    mBuffers.at(childInstance).inputBuffers.at(childInputPortId).push(image);

                    MLOGD(Mia2LogGroupCore,
                          "pipeline[%s] %s move inputImage[%p] to child inPort:%d",
                          getPipelineName(), instance.c_str(), image.get(), childInputPortId);
                }

                mSessionCB->releaseBuffer(image); // current node need release inputBuffer
                image = nullptr;
            };

            if (releaseOneInputBuffer) {
                if (it.second.tryPop(image)) {
                    func();
                }
            } else {
                while (it.second.tryPop(image)) {
                    func();
                }
            }
        }
    } else {
        auto &usingInputBuffers = mBuffers.at(instance).usingInputBuffers;

        for (auto &itMap : usingInputBuffers) {
            sp<MiaImage> image = nullptr;
            if (releaseOneInputBuffer) {
                if (itMap.second.tryPop(image)) {
                    mSessionCB->releaseBuffer(image);
                    MLOGD(Mia2LogGroupCore, "pipeline[%s] release inputImage[%p]",
                          getPipelineName(), image.get());
                    image = nullptr;
                } else {
                    MLOGE(Mia2LogGroupCore, "pipeline[%s] release inputImage error");
                }
            } else {
                while (itMap.second.tryPop(image)) {
                    mSessionCB->releaseBuffer(image);
                    MLOGD(Mia2LogGroupCore, "pipeline[%s] release inputImage[%p]",
                          getPipelineName(), image.get());
                    image = nullptr;
                }
            }
        }
    }

    MLOGI(Mia2LogGroupCore, "pipeline[%s] releaseInputBuffers end", getPipelineName());
    return MIAS_OK;
}

CDKResult Pipeline::pluginReleaseUnneedBuffer(const std::string &instance, int portId,
                                              int releaseNumber,
                                              PluginReleaseBuffer releaseBufferMode)
{
    switch (releaseBufferMode) {
    case PluginReleaseInputBuffer: {
        auto &usingInputBufferQueue = mBuffers.at(instance).usingInputBuffers.at(portId);
        if (usingInputBufferQueue.queueSize() < releaseNumber) {
            MLOGE(Mia2LogGroupCore, "plugin[%s] release inputImage number:%d > bufferqueue size:%d",
                  instance.c_str(), releaseNumber, usingInputBufferQueue.queueSize());
            return MIAS_INVALID_PARAM;
        }

        for (int i = 0; i < releaseNumber; i++) {
            sp<MiaImage> image = nullptr;

            if (usingInputBufferQueue.tryPop(image)) {
                mSessionCB->releaseBuffer(image);
                MLOGD(Mia2LogGroupCore, "pipeline[%s] plugin[%s] release unneed inputImage[%p]",
                      getPipelineName(), instance.c_str(), image.get());
                image = nullptr;
            } else {
                MLOGE(Mia2LogGroupCore, "pipeline[%s] plugin[%s] release unneed inputImage error",
                      getPipelineName(), instance.c_str());
                image = nullptr;
            }
        }
        break;
    }
    case PluginReleaseOutputBuffer: {
        const std::vector<int> outputLinks = mOutputLinkMap.at(instance);

        for (auto linkIndex : outputLinks) {
            LinkItem link = mPipelineInfo.links[linkIndex];
            if (link.srcNodePortId == portId) {
                MiaImageQueue &curentOutQueue = mBuffers.at(instance).outputBuffers.at(portId);
                if (curentOutQueue.queueSize() < releaseNumber) {
                    MLOGE(Mia2LogGroupCore,
                          "plugin[%s] release outputImage number:%d > bufferqueue size:%d",
                          instance.c_str(), releaseNumber, curentOutQueue.queueSize());
                    return MIAS_INVALID_PARAM;
                }

                if (link.type == SinkLink) {
                    releaseOutBuffers(linkIndex, SinkLink);
                } else {
                    for (int i = 0; i < releaseNumber; i++) {
                        releaseOutBuffers(linkIndex, InternalLink, true);
                    }
                }
            }
        }
        break;
    }
    }

    return MIAS_OK;
}

CDKResult Pipeline::getOutputImage(const std::string &nodeInstance, int nodeMask,
                                   const std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImages,
                                   const std::map<int, MiaImageFormat> &imageFormats,
                                   std::map<uint32_t, std::vector<sp<MiaImage>>> &outputImages)
{
    CDKResult rc = MIAS_OK;
    const std::vector<int> &outputLinks = mOutputLinkMap[nodeInstance];

    sp<MiaImage> mainInputImage = inputImages.begin()->second.at(0);
    int64_t timestampUs = mainInputImage->getTimestamp();

    auto itInstance = mPipelineInfo.nodes.find(nodeInstance);
    if (itInstance != mPipelineInfo.nodes.end()) {
        /* When node obtains the output buffer, it may not follow the nodemask configured in
         * json file . In a specific scenario, it may be expected that the value is different from
         * that originally configured
         */
        int newNodeMask = itInstance->second.nodeMask;
        if (nodeMask != newNodeMask) {
            MLOGD(Mia2LogGroupCore,
                  "pipeline[%s] node[%s] temporarily use nodeMask:%d, the original nodeMask:%d",
                  getPipelineName(), nodeInstance.c_str(), newNodeMask, nodeMask);
            nodeMask = newNodeMask;
        }
    }

    if (isInplaceNode(nodeInstance, nodeMask)) {
        return rc;
    }

    MLOGI(Mia2LogGroupCore, "pipeline[%s] instance[%s] get outputImages begin", getPipelineName(),
          nodeInstance.c_str());
    const std::vector<LinkItem> &links = mPipelineInfo.links;

    std::vector<MiaImageFormat> inputFormats;
    for (auto &it : imageFormats) {
        // Store the inputImageFormat in the order of inputPortId from small to large
        inputFormats.push_back(it.second);
    }

    std::map<int, int> dynamicBufferNum; // outPortId <--> outportBufferNum
    countDynamicOutBufferNum(dynamicBufferNum, nodeInstance, inputImages);

    for (int i = 0; i < outputLinks.size(); i++) {
        MiaImageFormat outputImageFormat;
        memset(&outputImageFormat, 0, sizeof(outputImageFormat));

        int index = outputLinks[i];
        LinkItem outputLink = links[index];
        int outputPortId = outputLink.srcNodePortId;

        MiaImageType newImageType = MI_IMG_INTERNAL;
        newImageType = (outputLink.type == SinkLink) ? MI_IMG_SINK : newImageType;
        auto it = mPipelineInfo.nodes.find(outputLink.dstNodeInstance);
        if (it != mPipelineInfo.nodes.end()) {
            int childNodeMask = it->second.nodeMask;
            newImageType = (childNodeMask & VIRTUAL_SINKNODE) ? MI_IMG_VIRTUAL_SINK : newImageType;
        }

        if (newImageType != MI_IMG_INTERNAL) {
            int sinkPortId = outputLink.dstNodePortId;
            try {
                outputImageFormat = mSinkBufferFormats.at(sinkPortId);
            } catch (...) {
                MLOGE(Mia2LogGroupCore,
                      "sinkPortId:%d > mSinkBufferFormatSize:%d, sinkLinkNumber:%d", sinkPortId,
                      mSinkBufferFormats.size(), mSinkNumToReturn);
                MASSERT(false);
            }
        } else {
            outputImageFormat =
                (i >= inputFormats.size()) ? inputFormats.at(0) : inputFormats.at(i);

            if (outputLink.srcPortFormat != CAM_FORMAT_UNDEFINED) {
                outputImageFormat.format = outputLink.srcPortFormat;
            }
            if (nodeMask & SCALE_MODE) {
                outputImageFormat.width = mSinkBufferFormats.at(0).width;
                outputImageFormat.height = mSinkBufferFormats.at(0).height;
            }
        }

        int outportBufferNum = dynamicBufferNum[outputPortId];
        for (int j = 0; j < outportBufferNum; j++) {
            if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::MTK &&
                mPipelineInfo.nodes.at(nodeInstance).outputType == FollowInput) {
                mainInputImage = inputImages.begin()->second[j];
                timestampUs = mainInputImage->getTimestamp();
            }
            sp<MiaImage> outputImage = mSessionCB->acquireOutputBuffer(
                outputImageFormat, timestampUs, outputLink.dstNodePortId, newImageType);

            MLOGD(Mia2LogGroupCore,
                  "pipeline[%s] get outputImage[%p]  of outputPort:%d NodeMask:%d format:%d",
                  getPipelineName(), outputImage.get(), outputPortId, nodeMask,
                  outputImageFormat.format);
            if (outputImage == NULL) {
                MLOGE(Mia2LogGroupCore, "pipeline[%s] get outputImage failed", getPipelineName());
                return MIAS_INVALID_PARAM;
            }

            outputImage->copyParams(mainInputImage, timestampUs, outportBufferNum);
            outputImages[outputPortId].push_back(outputImage);
            mBuffers.at(nodeInstance).outputBuffers.at(outputPortId).push(outputImage);
            if (nodeMask & SIGGETOUTPUT_MODE) {
                break;
            }
        }
        int type = mPipelineInfo.nodes[nodeInstance].outputType;
        MLOGD(Mia2LogGroupCore,
              "pipeline[%s] node[%s] type[%d] outputPortId[%d] get output buffer num:%d",
              getPipelineName(), nodeInstance.c_str(), type, outputPortId, outportBufferNum);
    }

    MLOGI(Mia2LogGroupCore, "pipeline[%s] instance[%s] get outputImages end", getPipelineName(),
          nodeInstance.c_str());
    return rc;
}

CDKResult Pipeline::releaseOutBuffers(int linkIndex, LinkType linkType, bool releaseOneOutputBuffer)
{
    MLOGI(Mia2LogGroupCore, "pipeline[%s] releaseOutBuffers begin", getPipelineName());

    const std::string &currentInstance = mPipelineInfo.links[linkIndex].srcNodeInstance;
    int currentOutputPort = mPipelineInfo.links[linkIndex].srcNodePortId;
    MiaImageQueue &curentOutQueue =
        mBuffers.at(currentInstance).outputBuffers.at(currentOutputPort);

    if (linkType == InternalLink) {
        const std::string &childInstance = mPipelineInfo.links[linkIndex].dstNodeInstance;
        int childNodeInputPort = mPipelineInfo.links[linkIndex].dstNodePortId;
        MiaImageQueue &childNodeInputQueue =
            mBuffers.at(childInstance).inputBuffers.at(childNodeInputPort);

        sp<MiaImage> image = nullptr;
        if (releaseOneOutputBuffer) {
            if (curentOutQueue.tryPop(image)) {
                MLOGD(Mia2LogGroupCore, "pipeline[%s] %s release outImage:%p to %s:inputPort:%d",
                      getPipelineName(), currentInstance.c_str(), image.get(),
                      childInstance.c_str(), childNodeInputPort);
                childNodeInputQueue.push(image);
                image = nullptr;
            }
        } else {
            while (curentOutQueue.tryPop(image)) {
                MLOGD(Mia2LogGroupCore, "pipeline[%s] %s release outImage:%p to %s:inputPort:%d",
                      getPipelineName(), currentInstance.c_str(), image.get(),
                      childInstance.c_str(), childNodeInputPort);
                childNodeInputQueue.push(image);
                image = nullptr;
            }
        }

    } else {
        --mSinkNumToReturn;

        sp<MiaImage> image = nullptr;

        while (curentOutQueue.tryPop(image)) {
            MLOGD(Mia2LogGroupCore, "pipeline[%s] will release sinkImage:%p", getPipelineName(),
                  image.get());
            if ((mSinkNumToReturn == 0) && (curentOutQueue.queueSize() == 0)) {
                updateMiviProcessTime2Exif(image);
                // The current image is the last SinkImage of the pipelines
                mSessionCB->releaseBuffer(image, true);
            } else {
                mSessionCB->releaseBuffer(image);
            }
            image = nullptr;
        }
    }

    MLOGI(Mia2LogGroupCore, "pipeline[%s] releaseOutBuffers end", getPipelineName());
    return MIAS_OK;
}

bool Pipeline::enqueueSourceImage(sp<MiaImage> image, uint32_t sourcePortId)
{
    std::lock_guard<std::mutex> lock(mSourceBufferLock);
    if (mSourceBufferQueueMap[sourcePortId].push(image)) { // push image
        MLOGI(Mia2LogGroupCore, "pipeline[%s] sourcePort:%d enqueue image:%p", getPipelineName(),
              sourcePortId, image.get());

        return true;
    }
    return false;
}

sp<MiaImage> Pipeline::getSourceImage(uint32_t srcPortId)
{
    sp<MiaImage> sourceImage = nullptr;

    // "it" definitely not equal to "mSourceBufferQueueMap.end()"
    auto it = mSourceBufferQueueMap.find(srcPortId);
    if (it == mSourceBufferQueueMap.end()) {
        MLOGE(Mia2LogGroupCore, "unknow error,pipeline[%s] get sourceImage from sourcePort:%d",
              getPipelineName(), srcPortId);
        return nullptr;
    }
    it->second.waitAndPop(sourceImage);

    MLOGI(Mia2LogGroupCore, "pipeline[%s] get sourceImage:%p from sourcePort:%d", getPipelineName(),
          sourceImage.get(), srcPortId);
    return sourceImage;
}

void Pipeline::releaseUnusedBuffer()
{
    /*
     * Some sources have been removed from the "mPipeline.Links" in the clipping stage, so the
     * Sourcebuffers corresponding to these sources are not used in the execution process. These
     * Sourcebuffers need to be release to the app manually, otherwise the app will face the
     * situation of no input buffers available
     */
    sp<MiaImage> image = nullptr;

    for (auto it = mSourceBufferQueueMap.begin(); it != mSourceBufferQueueMap.end(); it++) {
        it->second.releaseQueue();
        while (it->second.tryPop(image)) {
            mSessionCB->releaseBuffer(image);
            image = nullptr;
        }
    }

    for (auto &it : mBuffers) {
        auto &inputBufferMap = it.second.inputBuffers;
        auto &usingInputBufferMap = it.second.usingInputBuffers;
        auto &outputBufferMap = it.second.outputBuffers;

        for (auto &imageQueueIt : inputBufferMap) {
            while (imageQueueIt.second.tryPop(image)) {
                mSessionCB->releaseBuffer(image);
                image = nullptr;
            }
        }

        for (auto &imageQueueIt : usingInputBufferMap) {
            while (imageQueueIt.second.tryPop(image)) {
                mSessionCB->releaseBuffer(image);
                image = nullptr;
            }
        }

        for (auto &imageQueueIt : outputBufferMap) {
            while (imageQueueIt.second.tryPop(image)) {
                mSessionCB->releaseBuffer(image);
                image = nullptr;
            }
        }
    }
}

void Pipeline::flush(bool isForced)
{
    quickFinish(false);
}

void Pipeline::quickFinish(bool needImageResult)
{
    MLOGI(Mia2LogGroupCore, "E pipeline[%s] quickFinish(%d)", getPipelineName(), needImageResult);
    std::lock_guard<std::mutex> locker(mQuickFinishLock);
    if (needImageResult) {
        mFlushStatus = Expediting;
    } else {
        mFlushStatus = ForcedFlush;
    }

    for (auto it = mSourceBufferQueueMap.begin(); it != mSourceBufferQueueMap.end(); it++) {
        it->second.releaseQueue();
    }

    if (!needImageResult) { // When the picture is not needed, call the flush of the node
        std::lock_guard<std::mutex> recordlock(mRecordProcessNodeLock);
        for (auto currentProcessNode : mCurrentProcessNodes) {
            // frame-by-frame internal node needs this line when current pipeline quickFinish
            mBuffers.at(currentProcessNode).inputBuffers.begin()->second.releaseQueue();

            sp<MiaNode> processNode = mSessionCB->getProcessNode(currentProcessNode);
            processNode->quickFinish(mName);
        }
    }

    MLOGI(Mia2LogGroupCore, "X pipeline[%s] quickFinish(%d)", getPipelineName(), needImageResult);
}

void Pipeline::notifyNextNode(std::string instance, NodeProcessInfo nodeProcessInfo, int abnormal,
                              int32_t frame_num)
{
    std::unique_lock<std::mutex> recordlock(mRecordProcessNodeLock);
    auto currentProcessNode = mCurrentProcessNodes.find(instance);
    if (currentProcessNode != mCurrentProcessNodes.end()) {
        mCurrentProcessNodes.erase(currentProcessNode);
    } else {
        MLOGE(Mia2LogGroupCore, "The currently processed node was not found");
    }
    recordlock.unlock();

    const std::vector<int> outputLinks = mOutputLinkMap[instance];
    if ((nodeProcessInfo != ProcessOK) || (abnormal)) {
        std::string data = "";
        {
            std::lock_guard<std::mutex> locker(mIsCallExternalLock);
            if (!mIsCallExternal) {
                mIsCallExternal = true;
                mSessionCB->onResultComing(MiaResultCbType::MIA_ABNORMAL_CALLBACK, frame_num,
                                           mTimestampUs, data);
            }
        }
        for (auto linkIndex : outputLinks) {
            if (mPipelineInfo.links[linkIndex].type == SinkLink) {
                releaseOutBuffers(linkIndex, SinkLink);
            }
        }

        std::lock_guard<std::mutex> locker(mLock);
        mHasProcessError = true;
        mRunningNodeNum -= 1;
        MLOGE(Mia2LogGroupCore,
              "pipeline[%s] instance:%s process error because %s!  mRunningPathNum:%d "
              "mRunSequence.size()=%d",
              getPipelineName(), instance.c_str(), NodeProcessInfoToChar[nodeProcessInfo],
              mRunningNodeNum, mRunSequence.size());
        mCond.notify_one();
        return;
    }

    MLOGI(Mia2LogGroupCore, "pipeline[%s] instance[%s] process finish!", getPipelineName(),
          instance.c_str());
    releaseInputBuffers(instance);

    for (auto linkIndex : outputLinks) {
        LinkItem link = mPipelineInfo.links[linkIndex];
        if (link.type == SinkLink) {
            releaseOutBuffers(linkIndex, SinkLink);
        } else {
            releaseOutBuffers(linkIndex, InternalLink);

            const std::string &nextNode = link.dstNodeInstance;
            std::lock_guard<std::mutex> lock(mLock);
            if (!mHasProcessError && --mDependenceNum.at(nextNode) == 0) {
                MLOGD(Mia2LogGroupCore, "pipeline[%s] instance[%s] will notify [%s] start process",
                      getPipelineName(), instance.c_str(), nextNode.c_str());

                mRunSequence.push(nextNode);
            }
        }
    }

    std::lock_guard<std::mutex> lock(mLock);
    mRunningNodeNum -= 1;
    mCond.notify_one();
}

void Pipeline::releaseInputAndTryNotifyNextNode(const std::string &instance)
{
    releaseInputBuffers(instance, true);

    // If the current node is inplaceNode or virtualNode, we need to
    // determine whether the childNode can start running now
    int currentNodeMask = mPipelineInfo.nodes.at(instance).nodeMask;
    if (isInplaceNode(instance, currentNodeMask) || (currentNodeMask & VIRTUAL_MODE)) {
        const std::vector<int> outputLinks = mOutputLinkMap[instance];
        for (int i = 0; i < outputLinks.size(); i++) {
            const LinkItem &link = mPipelineInfo.links[outputLinks[i]];
            const std::string &childInstance = link.dstNodeInstance;

            // childNode is frame-by-frame node and not sinkBuffer
            if ((link.type != SinkLink) &&
                (mPipelineInfo.nodes.at(childInstance).nodeMask & SIGFRAME_MODE)) {
                std::lock_guard<std::mutex> lock(mLock);
                if (!mHasProcessError && --mDependenceNum.at(childInstance) == 0) {
                    MLOGD(Mia2LogGroupCore,
                          "pipeline[%s] instance[%s] will notify [%s] start process",
                          getPipelineName(), instance.c_str(), childInstance.c_str());

                    mRunSequence.push(childInstance);
                    mCond.notify_one();
                }
            }
        }
    }
}

void Pipeline::releaseOutputAndTryNotifyNextNode(const std::string &instance)
{
    int currentNodeMask = mPipelineInfo.nodes.at(instance).nodeMask;
    if (isInplaceNode(instance, currentNodeMask) || (currentNodeMask & VIRTUAL_MODE)) {
        // inplace and virtual node no outputBuffer
        return;
    }

    const std::vector<int> outputLinks = mOutputLinkMap[instance];

    int outputLinkIndex = outputLinks[0];
    const LinkItem &link = mPipelineInfo.links[outputLinkIndex];
    if (link.type == SinkLink) {
        releaseOutBuffers(outputLinkIndex, SinkLink, true);
    } else {
        releaseOutBuffers(outputLinkIndex, InternalLink, true);

        const std::string &nextNode = link.dstNodeInstance;
        // ChildNode is frame-by-frame node
        if (mPipelineInfo.nodes.at(nextNode).nodeMask & SIGFRAME_MODE) {
            std::lock_guard<std::mutex> lock(mLock);
            if (!mHasProcessError && --mDependenceNum.at(nextNode) == 0) {
                MLOGD(Mia2LogGroupCore, "pipeline[%s] instance[%s] will notify [%s] start process",
                      getPipelineName(), instance.c_str(), nextNode.c_str());

                mRunSequence.push(nextNode);
                mCond.notify_one();
            }
        }
    }
}

bool Pipeline::isInplaceNode(const std::string &nodeInstance, int nodeMask)
{
    bool isInplaceNode = false;
    // The inplace node must have only one outlink, and it's childNode
    // cannot be a virtualSink or a SinkBuffer

    const std::vector<int> &outputLinks = mOutputLinkMap[nodeInstance];
    if ((nodeMask & INPLACE_MODE) && (outputLinks.size() == 1)) {
        if (mPipelineInfo.links[outputLinks[0]].type != SinkLink) {
            std::string dstNodeInstance = mPipelineInfo.links[outputLinks[0]].dstNodeInstance;
            if (mPipelineInfo.nodes.at(dstNodeInstance).nodeMask & VIRTUAL_SINKNODE) {
                // nodeInstance's childNode is virtualSink

                // isInplaceNode = false;
            } else {
                isInplaceNode = true;
            }
        } /*else { // nodeInstance's childNode is SinkBuffer
            isInplaceNode = false;
        }*/
    }

    return isInplaceNode;
}

void Pipeline::updateMiviProcessTime2Exif(sp<MiaImage> &image)
{
    double miviProcessTime = MiaUtil::nowMSec() - mMiviProcessStartTime;
    MLOGI(Mia2LogGroupCore, "mivi process cost:%0.3fms", miviProcessTime);
    std::string exifResult = " miviProcessCost:";
    char miviProcessCost[16] = {0};
    snprintf(miviProcessCost, sizeof(miviProcessCost), "%0.3f", miviProcessTime);
    exifResult = exifResult + miviProcessCost;
    mSessionCB->onResultComing(MiaResultCbType::MIA_META_CALLBACK, image->getFrameNumber(),
                               image->getTimestamp(), exifResult);
}

void Pipeline::countDynamicOutBufferNum(
    std::map<int, int> &dynamicBufferNum, const std::string &instance,
    const std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImages)
{
    OutputType type = mPipelineInfo.nodes[instance].outputType;

    std::map<int, int> inportBufferNum; // inputPortId <--> inputPortBufferNum
    std::vector<int> outports;          // the output portId of the node

    const std::vector<int> &inputLinks = mInputLinkMap[instance];
    const std::vector<int> &outputLinks = mOutputLinkMap[instance];

    for (int i = 0; i < inputLinks.size(); i++) {
        int inLinkIndex = inputLinks[i];
        const LinkItem &inputLinkItem = mPipelineInfo.links[inLinkIndex];
        int inputPortId = inputLinkItem.dstNodePortId;
        int mergeInputNum = inputImages.at(inputPortId).at(0)->getMergeInputNumber();
        inportBufferNum[inputPortId] = mergeInputNum;
    }

    for (int i = 0; i < outputLinks.size(); i++) {
        int outlinkIndex = outputLinks[i];
        const LinkItem &outLinkItem = mPipelineInfo.links[outlinkIndex];
        int outputPortId = outLinkItem.srcNodePortId;
        outports.push_back(outputPortId);
    }

    if (type == OneBuffer) {
        for (auto outputPortId : outports) {
            dynamicBufferNum[outputPortId] = 1;
        }
    } else if (inputLinks.size() == 1 && outputLinks.size() == 1 && type == FollowInput) {
        dynamicBufferNum[inportBufferNum.begin()->first] = inportBufferNum.begin()->second;
    } else if (type == Custom) {
        customOutportBufferNum(dynamicBufferNum, instance, inputImages, inportBufferNum, outports);
    }
}

void Pipeline::customOutportBufferNum(
    std::map<int, int> &dynamicBufferNum, const std::string &instance,
    const std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImages,
    const std::map<int, int> &inportBufferNum, std::vector<int> &outports)
{
    // Since there is no actual application scenario, only initialize here
    for (auto outputPortId : outports) {
        dynamicBufferNum[outputPortId] = 1;
    }

    //clang-format off
    /* for example:
       nodeInstance0 has 1 input port and 2 output port

            |---------------|0-->
        -->0| nodeInstance0 |
            |---------------|1-->

       if(instance == "nodeInstance0"){
           inportBufferNum[0] = 10;

           //The node sets the number of buffers for each outport according to different case
           //different case (include different pipeline, different project...)

           if(case1){
               dynamicBufferNum[0] = 2;  //outport[0] has 2 buffer
               dynamicBufferNum[1] = 8;  //outport[1] has 8 buffer
           }else if(case2){
               dynamicBufferNum[0] = 3;
               dynamicBufferNum[1] = 7;
           }
       }
    */
    //clang-format on
}

} // namespace mialgo2
