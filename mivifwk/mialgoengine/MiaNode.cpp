/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaNode.h"

#include <errno.h>

#include <fstream>
#include <string>

#include "MiaImage.h"
#include "MiaUtil.h"
#include "ThreadPool.h"
#include "camlog.h"
#define ADVANCEMOONMODEON (35)

char s_AlgoPerfInfo[PERFLOGNUMBER][PERFLOGLONG];
namespace mialgo2 {
using namespace std;
using namespace midebug;

uint32_t mialgo2::MiaNode::m_uiLogNumber = 0;
MiaNode::MiaNode()
{
    mNodeMask = 0;
    mNodeCB = NULL;
    mInputFrameNum = 0; // current frame
    mPluginWraper = NULL;
    mPipeline = NULL;
    mProcStartTime = 0.0;
    mInPluginThreadNum = 0;
    m_currentTemp = 0;
    m_currentCpuFreq0 = 0;
    m_currentCpuFreq4 = 0;
    m_currentCpuFreq7 = 0;
    m_currentGpuFreq = 0;
    m_maxCpuFreq0 = 0;
    m_maxCpuFreq4 = 0;
    m_maxCpuFreq7 = 0;
    m_maxGpuFreq = 0;

    mOpenDump = MiaInternalProp::getInstance()->getBool(OpenEngineImageDump);
    mLastDumpFileName = "";

    mHasProcessThread = false;
    mIsMiHalBurstMode = false;
}

MiaNode::~MiaNode()
{
    deInit();
}

CDKResult MiaNode::init(struct NodeItem nodeItem, MiaCreateParams *params)
{
    mPipeline = NULL;
    mNodeMask = nodeItem.nodeMask;
    mOutputBufferNeedCheck = nodeItem.outputBufferNeedCheck;
    setNodeName(std::string(nodeItem.nodeName.c_str()));
    setNodeInstance(std::string(nodeItem.nodeInstance.c_str()));

    mPluginWraper = MPluginModule::getInstance()->getPluginWraperByPluginName(nodeItem.nodeName);
    if (mPluginWraper == NULL) {
        MLOGE(Mia2LogGroupCore, "getPluginWraperByPluginName failed");
        return MIAS_INVALID_PARAM;
    }

    if (params->operationMode == StreamConfigModeSATBurst) {
        mIsMiHalBurstMode = true;
    }

    double startTime = MiaUtil::nowMSec();
    CreateInfo creatInfo;
    creatInfo.frameworkCameraId = params->cameraMode;
    creatInfo.operationMode = params->operationMode;
    creatInfo.inputFormat.format = params->inputFormat[0].format;
    creatInfo.inputFormat.width = params->inputFormat[0].width;
    creatInfo.inputFormat.height = params->inputFormat[0].height;
    creatInfo.outputFormat.format = params->outputFormat[0].format;
    creatInfo.outputFormat.width = params->outputFormat[0].width;
    creatInfo.outputFormat.height = params->outputFormat[0].height;
    creatInfo.logicalCameraId = params->logicalCameraId;
    creatInfo.nodeInstance = mNodeInstance;
    creatInfo.nodePropertyId = nodeItem.nodeMask;
    creatInfo.roleId = -1;

    MiaNodeInterface nodeInterface;
    nodeInterface.owner = (void *)this;
    nodeInterface.pSetResultMetadata = setResultMetadata;
    nodeInterface.pSetOutputFormat = setOutputFormat;
    nodeInterface.pSetResultBuffer = setResultBuffer; // for signal Frame process plugin
    nodeInterface.pReleaseableUnneedBuffer = releaseableUnneedBuffer;
    mPluginWraper->initialize(&creatInfo, nodeInterface);
    MLOGI(Mia2LogGroupCore, "[%s][%p] Plugin initialize spend time %.2fms",
          nodeItem.nodeName.c_str(), this, MiaUtil::nowMSec() - startTime);

    return MIAS_OK;
}

CDKResult MiaNode::deInit()
{
    if (mPluginWraper != NULL) {
        mPluginWraper->destroy();
        delete mPluginWraper;
        mPluginWraper = NULL;
    }

    return MIAS_OK;
}

int MiaNode::preProcess(PostMiaPreProcParams *preParams)
{
    MiaParams settings = {0};
    settings.metadata = preParams->metadata->mPreMeta;

    if (mPluginWraper->needPreProcess(settings)) {
        MLOGD(Mia2LogGroupCore, "%s inputFormat: %d, num: %d, outputFormat: %d, num: %d",
              getNodeName(), preParams->inputFormats[0], preParams->inputFormats.size(),
              preParams->outputFormats[0], preParams->outputFormats.size());

        wp<MiaNode> weakThis(this);
        PreProcessInfo preProcessInfo{};
        for (auto &it : preParams->inputFormats) {
            ImageFormat format = {
                .format = it.second.format,
                .width = it.second.width,
                .height = it.second.height,
                .cameraId = it.second.cameraId,
            };
            preProcessInfo.inputFormats.insert({it.first, format});
        }
        for (auto &it : preParams->outputFormats) {
            ImageFormat format = {
                .format = it.second.format,
                .width = it.second.width,
                .height = it.second.height,
                .cameraId = it.second.cameraId,
            };
            preProcessInfo.outputFormats.insert({it.first, format});
        }
        preProcessInfo.metadata = preParams->metadata;

        Singleton<ThreadPool>::getInstance()->enqueue([weakThis, preProcessInfo]() {
            sp<MiaNode> node = weakThis.promote();
            if (node) {
                MLOGI(Mia2LogGroupCore, "run preProcess task for %s", node->getNodeName());
                node->mPluginWraper->preProcess(preProcessInfo);
            }
        });
    }

    return MIAS_OK;
}

bool MiaNode::isEnable(MiaParams settings)
{
    if (mPluginWraper != NULL) {
        return mPluginWraper->isEnabled(settings);
    }

    return false;
}

CDKResult MiaNode::getInputBuffer(sp<Pipeline> &pipeline,
                                  std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImages)
{
    return pipeline->getInputImage(mNodeInstance, mNodeMask, inputImages);
}

CDKResult MiaNode::getOutputBuffer(const sp<Pipeline> &pipeline,
                                   const std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImages,
                                   std::map<uint32_t, std::vector<sp<MiaImage>>> &outputImages)
{
    CDKResult result = MIAS_OK;
    if (pipeline == nullptr) {
        MLOGE(Mia2LogGroupCore, "unknow error!!");
        return MIAS_UNKNOWN_ERROR;
    }

    std::map<int, MiaImageFormat> imageFormats;
    for (auto &it : inputImages) {
        int inputPort = it.first;
        sp<MiaImage> image = it.second.at(0);
        imageFormats[inputPort] = image->getImageFormat();
    }

    result =
        pipeline->getOutputImage(mNodeInstance, mNodeMask, inputImages, imageFormats, outputImages);

    return result;
}

std::map<uint32_t, std::vector<ImageParams>> MiaNode::buildBufferParams(
    const std::map<uint32_t, std::vector<sp<MiaImage>>> &imageMap)
{
    std::map<uint32_t, std::vector<ImageParams>> imageBuffers{};

    auto praseImage = [&](ImageParams &dstImage, sp<MiaImage> srcImage) {
        dstImage.format.format = srcImage->getImageFormat().format;
        dstImage.format.width = srcImage->getImageFormat().width;
        dstImage.format.height = srcImage->getImageFormat().height;
        dstImage.planeStride = srcImage->getImageFormat().planeStride;
        dstImage.sliceheight = srcImage->getImageFormat().sliceheight;
        dstImage.cameraId = srcImage->getImageFormat().cameraId;
        dstImage.numberOfPlanes = srcImage->getMiaImageBuffer()->planes;
        dstImage.reserved = srcImage->getMiaImageBuffer()->reserved;
        if (srcImage->getMetadataWraper()) {
            dstImage.pMetadata = srcImage->getMetadataWraper()->getLogicalMetadata();
            dstImage.pPhyCamMetadata = srcImage->getMetadataWraper()->getPhysicalMetadata();
        } else {
            dstImage.pMetadata = NULL;
            dstImage.pPhyCamMetadata = NULL;
        }
        dstImage.fd[0] = srcImage->getMiaImageBuffer()->fd[0];
        dstImage.fd[1] = srcImage->getMiaImageBuffer()->fd[1];
        dstImage.pAddr[0] = srcImage->getMiaImageBuffer()->pAddr[0];
        dstImage.pAddr[1] = srcImage->getMiaImageBuffer()->pAddr[1];
        dstImage.bufferType = srcImage->getMiaImageBuffer()->bufferType;
        dstImage.bufferSize = srcImage->getMiaImageBuffer()->bufferSize;
        if (srcImage->getMiaImageBuffer()->phHandle != NULL) {
            dstImage.pNativeHandle = srcImage->getMiaImageBuffer()->phHandle;
        }
    };

    for (const auto &it : imageMap) {
        std::vector<ImageParams> buffers;
        uint32_t portId = it.first;
        for (auto frame : it.second) {
            ImageParams image;
            praseImage(image, frame);
            image.portId = portId;
            buffers.push_back(image);
        }
        imageBuffers.insert({portId, std::move(buffers)});
    }

    return std::move(imageBuffers);
}

CDKResult MiaNode::processRequest(std::map<uint32_t, std::vector<sp<MiaImage>>> &inputFrameMap,
                                  std::map<uint32_t, std::vector<sp<MiaImage>>> &outputFrameMap,
                                  int32_t &abnormal, int32_t &frame_num)
{
    ProcessRetStatus result = PROCSUCCESS;

    double processStartTime = MiaUtil::nowMSec();
    MICAM_TRACE_ASYNC_BEGIN_F(MialgoTraceProfile, 0, getNodeName());
    if (mNodeMask & CONCURRENCY_MODE) {
        int64_t timestamp = outputFrameMap.begin()->second[0]->getTimestamp();

        std::lock_guard<std::mutex> lock(mProcTimeLock);
        mProcStartTimes[timestamp] = processStartTime;
    } else {
        mProcStartTime = processStartTime;
    }

    sp<MiaImage> mainInputImage = inputFrameMap.begin()->second.at(0);
    std::string imageName = mainInputImage->getImageName();
    uint32_t mergeNum = mainInputImage->getMergeInputNumber();

    auto inputBuffersMap = buildBufferParams(inputFrameMap);
    auto outputBuffersMap = buildBufferParams(outputFrameMap);
    uint32_t inputPorts = inputBuffersMap.size();
    uint32_t outputPorts = outputBuffersMap.size();

    for (auto &it : inputBuffersMap) {
        uint32_t portId = it.first;
        MLOGI(Mia2LogGroupCore, "[%s::%s] get input portId: %d  inputNum: %d", getNodeName(),
              getNodeInstance().c_str(), portId, it.second.size());
    }

    for (auto &it : outputBuffersMap) {
        uint32_t portId = it.first;
        MLOGI(Mia2LogGroupCore, "[%s::%s] get output portId: %d  outputNum: %d", getNodeName(),
              getNodeInstance().c_str(), portId, it.second.size());
    }

    MLOGI(Mia2LogGroupCore, "[%s::%s] get inputPorts: %d  outputPorts: %d", getNodeName(),
          getNodeInstance().c_str(), inputPorts, outputPorts);

    uint32_t inputNum = inputBuffersMap.begin()->second.size();
    uint32_t outputNum = 0;
    if (outputPorts > 0) {
        outputNum = outputBuffersMap.begin()->second.size();
    }
    if (inputNum == 0 || (outputNum == 0 && !(mNodeMask & SIGFRAME_MODE))) {
        MLOGE(Mia2LogGroupCore, "[%s]input %d, or output %d, is null", getNodeName(), inputNum,
              outputNum);
        return MIAS_INVALID_PARAM;
    }

    ProcessRequestInfo requestInfo;
    requestInfo.frameNum = inputFrameMap.begin()->second[0]->getFrameNumber();
    MiaDateTime systemDateTime;
    MiaUtil::getDateTime(&systemDateTime);

    if (outputNum > 0) {
        requestInfo.timeStamp = outputFrameMap.begin()->second[0]->getTimestamp();
    }

    if (mOpenDump) {
        for (auto &it : inputBuffersMap) {
            uint32_t inputPort = it.first;
            dumpImageInfo(it.second, imageName, "input", inputPort);
        }
    }

    requestInfo.inputBuffersMap = inputBuffersMap;
    requestInfo.outputBuffersMap = outputBuffersMap;
    requestInfo.isFirstFrame = (mInputFrameNum == 1) ? true : false;
    requestInfo.maxConcurrentNum = MaxConcurrentNum;
    requestInfo.runningTaskNum = mNodeCB->getRunningTaskNum();

    if (mPluginWraper != NULL) {
        bool isProcessDone = false;
        std::condition_variable timeoutCond;
        std::mutex timeoutMux;
        uint32_t tid = syscall(SYS_gettid);
        std::thread detectThread([this, tid, &isProcessDone, &timeoutCond, &timeoutMux]() {
            timeoutDetection(tid, isProcessDone, timeoutCond, timeoutMux);
        });
        mInPluginThreadNum++;
        result = mPluginWraper->processRequest(&requestInfo);

        signalThead(isProcessDone, timeoutCond, timeoutMux);
        detectThread.join();
    } else {
        result = PROCFAILED;
    }

    {
        std::unique_lock<std::mutex> locker(mQuickFinishMux);
        mQuickFinishCond.wait(locker, [this]() {
            if (mInPluginThreadNum == 1) {
                --mInPluginThreadNum;
            }
            return mInPluginThreadNum == 0;
        });
    }

    if (mOpenDump) {
        for (auto &it : outputBuffersMap) {
            uint32_t outputPort = it.first;
            dumpImageInfo(it.second, imageName, "output", outputPort);
        }
    }

    int32_t errorCode = 0;
    if (outputNum > 0) {
        if (mNodeMask & SIGFRAME_MODE) {
            mAbnImageCheckForFrameByFrame.outputBuffersMap = outputBuffersMap;
            mAbnImageCheckForFrameByFrame.frameNum = requestInfo.frameNum;
            mAbnImageCheckForFrameByFrame.timeStamp = requestInfo.timeStamp;
        } else {
            errorCode = updateAbnImageExifDataV1(requestInfo);
            frame_num = requestInfo.frameNum;
            // abnormal = errorCode;
        }
    }
    if ((mNodeMask & SIGFRAME_MODE) && (mergeNum == mInputFrameNum)) {
        errorCode = updateAbnImageExifDataV1(mAbnImageCheckForFrameByFrame);
        frame_num = mAbnImageCheckForFrameByFrame.frameNum;
        // abnormal = errorCode;
    }

    double intervalTime = MiaUtil::nowMSec() - processStartTime;
    MLOGI(Mia2LogGroupCore,
          "[%s::%s][%p] Plugin processRequest spend time %.2fms, frameNumber %d, result: %d ",
          getNodeName(), mNodeInstance.c_str(), this, intervalTime, requestInfo.frameNum, result);
    MICAM_TRACE_ASYNC_END_F(MialgoTraceProfile, 0, getNodeName());

    if (intervalTime > MAX_THRESHOLD) {
        snprintf(s_AlgoPerfInfo[(m_uiLogNumber++) % PERFLOGNUMBER], sizeof(char) * PERFLOGLONG,
                 "%02d-%02d %02d:%02d:%02d:%03d Node %s  %.2fms, "
                 "frameNumber %d\n",
                 systemDateTime.month + 1, systemDateTime.dayOfMonth, systemDateTime.hours,
                 systemDateTime.minutes, systemDateTime.seconds, systemDateTime.microseconds / 1000,
                 getNodeName(), intervalTime, requestInfo.frameNum);
    }
    if (intervalTime >= PSI_PRINT_THRESHOLD_MS) {
        printPsiInfo();
        char isSochanged[256] = "";
        // When the value of logsystem is 1 or 3, the send_message_to_mqs interface is
        // called When the value of logsystem is 2, it means that the exception needs to be
        // ignored
        property_get("vendor.camera.sensor.logsystem", isSochanged, "0");
        if ((atoi(isSochanged) == 1) || (atoi(isSochanged) == 3)) {
            char CamErrorString[256] = {0};
            sprintf(CamErrorString, "[Perf][MIVI]%s::%s process cost %.2fms", getNodeName(),
                    mNodeInstance.c_str(), intervalTime);
            MLOGW(Mia2LogGroupCore, "send error to MQS: %s", CamErrorString);
            camlog::send_message_to_mqs(CamErrorString, false);
        }
    }

    return result == PROCSUCCESS ? MIAS_OK : MIAS_INVALID_PARAM;
}

CDKResult MiaNode::processRequest(std::map<uint32_t, std::vector<sp<MiaImage>>> &inputImageVecMap,
                                  int32_t &abnormal, int32_t &frame_num)
{
    if (inputImageVecMap.empty()) {
        return MIAS_INVALID_PARAM;
    }

    sp<MiaImage> mainInputImage = inputImageVecMap.begin()->second.at(0);
    std::string imageName = mainInputImage->getImageName();
    int64_t timeStamp = mainInputImage->getTimestamp();

    ProcessRequestInfoV2 requestInfo;
    double processStartTime = MiaUtil::nowMSec();
    if (mNodeMask & CONCURRENCY_MODE) {
        // assert: mainInputImage have the smallest timestamp in all inputsImage
        int64_t timestamp = mainInputImage->getTimestamp();

        std::lock_guard<std::mutex> lock(mProcTimeLock);
        mProcStartTimes[timestamp] = processStartTime;
    } else {
        mProcStartTime = processStartTime;
    }
    MiaDateTime systemDateTime;
    MiaUtil::getDateTime(&systemDateTime);

    std::map<uint32_t, std::vector<ImageParams>> inputBuffersMap =
        buildBufferParams(inputImageVecMap);
    uint32_t inputPorts = inputBuffersMap.size();
    uint32_t inputNum = inputBuffersMap.begin()->second.size();

    requestInfo.frameNum = mainInputImage->getFrameNumber();
    requestInfo.inputBuffersMap = inputBuffersMap;
    requestInfo.maxConcurrentNum = MaxConcurrentNum;
    requestInfo.runningTaskNum = mNodeCB->getRunningTaskNum();
    requestInfo.timeStamp = timeStamp;

    if (mOpenDump) {
        for (auto &it : inputBuffersMap) {
            uint32_t inputPort = it.first;
            dumpImageInfo(it.second, imageName, "input", inputPort);
        }
    }

    ProcessRetStatus result = PROCSUCCESS;
    MLOGI(Mia2LogGroupCore, "[%s] is inplace node, inputNum: %u", getNodeName(), inputNum);

    if (mPluginWraper != NULL) {
        bool isProcessDone = false;
        std::condition_variable timeoutCond;
        std::mutex timeoutMux;
        uint32_t tid = syscall(SYS_gettid);
        std::thread detectThread([this, tid, &isProcessDone, &timeoutCond, &timeoutMux]() {
            timeoutDetection(tid, isProcessDone, timeoutCond, timeoutMux);
        });
        mInPluginThreadNum++;
        result = mPluginWraper->processRequest(&requestInfo);

        signalThead(isProcessDone, timeoutCond, timeoutMux);
        detectThread.join();
    } else {
        return MIAS_INVALID_PARAM;
    }

    {
        std::unique_lock<std::mutex> locker(mQuickFinishMux);
        mQuickFinishCond.wait(locker, [this]() {
            if (mInPluginThreadNum == 1) {
                --mInPluginThreadNum;
            }
            return mInPluginThreadNum == 0;
        });
    }

    if (mOpenDump) {
        for (auto &it : inputBuffersMap) {
            uint32_t inputPort = it.first;
            dumpImageInfo(it.second, imageName, "output", inputPort);
        }
    }

    int32_t errorCode = updateAbnImageExifDataV2(requestInfo);
    // abnormal = errorCode;
    frame_num = requestInfo.frameNum;
    double intervalTime = MiaUtil::nowMSec() - processStartTime;

    if (intervalTime > MAX_THRESHOLD) {
        snprintf(s_AlgoPerfInfo[(m_uiLogNumber++) % PERFLOGNUMBER], sizeof(char) * PERFLOGLONG,
                 "%02d-%02d %02d:%02d:%02d:%03d Node %s %.2fms, "
                 "frameNumber %d\n",
                 systemDateTime.month + 1, systemDateTime.dayOfMonth, systemDateTime.hours,
                 systemDateTime.minutes, systemDateTime.seconds, systemDateTime.microseconds / 1000,
                 getNodeName(), intervalTime, requestInfo.frameNum);
    }

    MLOGI(Mia2LogGroupCore, "[%s::%s][%p] Plugin processRequest spend time %.2fms, frameNumber %d",
          getNodeName(), mNodeInstance.c_str(), this, intervalTime, requestInfo.frameNum);

    if (intervalTime >= PSI_PRINT_THRESHOLD_MS) {
        printPsiInfo();
        char isSochanged[256] = "";
        // When the value of logsystem is 1 or 3, the send_message_to_mqs interface is
        // called When the value of logsystem is 2, it means that the exception needs to be
        // ignored
        property_get("vendor.camera.sensor.logsystem", isSochanged, "0");
        if ((atoi(isSochanged) == 1) || (atoi(isSochanged) == 3)) {
            char CamErrorString[256] = {0};
            sprintf(CamErrorString, "[Perf][MIVI]%s::%s process cost %.2fms", getNodeName(),
                    mNodeInstance.c_str(), intervalTime);
            MLOGW(Mia2LogGroupCore, "send error to MQS: %s", CamErrorString);
            camlog::send_message_to_mqs(CamErrorString, false);
        }
    }

    return result == PROCSUCCESS ? MIAS_OK : MIAS_INVALID_PARAM;
}

void MiaNode::flush(bool isForced)
{
    FlushRequestInfo requestInfo = {0};
    requestInfo.isForced = isForced;
    mPluginWraper->flushRequest(&requestInfo);

    // must be executed once after plugin's flushRequest
    // reset plugin internal state
    mPluginWraper->reset();
}

void MiaNode::quickFinish(const std::string &pipelineName)
{
    auto isCurrentPipeline = [this](const std::string &currentPipelineName) {
        std::lock_guard<std::mutex> lock(mProcessLock);
        return strcmp(mPipeline->getPipelineName(), currentPipelineName.c_str()) == 0;
    };

    std::unique_lock<std::mutex> locker(mQuickFinishMux);
    if ((mInPluginThreadNum == 1) && isCurrentPipeline(pipelineName) &&
        !(mNodeMask & CONCURRENCY_MODE)) {
        MLOGI(Mia2LogGroupCore, "E node[%s] begin quickfinish", mNodeInstance.c_str());
        mInPluginThreadNum++;
        locker.unlock();

        std::thread quickFinishThread([this]() {
            flush(true);
            mInPluginThreadNum--;
            std::lock_guard<std::mutex> locker(mQuickFinishMux);
            mQuickFinishCond.notify_one();
        });
        quickFinishThread.detach();
    }
}

void MiaNode::getCurrentHWStatus()
{
    auto getDataFromPath = [](const char *path, int32_t &data) {
        int32_t fd = open(path, O_RDONLY);
        if (fd >= 0) {
            int32_t readSize = 0;
            char readBuffer[16] = {0};
            readSize = read(fd, &readBuffer[0], 15);
            if (readSize > 0) {
                readBuffer[readSize] = 0;
                data = atoi(&readBuffer[0]);
            } else {
                MLOGI(Mia2LogGroupCore, "cannot get HW status");
            }
            close(fd);
        } else {
            MLOGI(Mia2LogGroupCore, "cannot open HW status node, errno:%s", strerror(errno));
        }
    };

    std::string currentFilePath = "";
    getDataFromPath(
        MiaInternalProp::getInstance()->getString(CurrentTempFilePath, currentFilePath).c_str(),
        m_currentTemp);
    getDataFromPath("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", m_currentCpuFreq0);
    getDataFromPath("/sys/devices/system/cpu/cpu4/cpufreq/scaling_cur_freq", m_currentCpuFreq4);
    getDataFromPath("/sys/devices/system/cpu/cpu7/cpufreq/scaling_cur_freq", m_currentCpuFreq7);
    getDataFromPath("/sys/class/kgsl/kgsl-3d0/devfreq/cur_freq", m_currentGpuFreq);
    getDataFromPath("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", m_maxCpuFreq0);
    getDataFromPath("/sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq", m_maxCpuFreq4);
    getDataFromPath("/sys/devices/system/cpu/cpu7/cpufreq/scaling_max_freq", m_maxCpuFreq7);
    getDataFromPath("/sys/class/kgsl/kgsl-3d0/devfreq/max_freq", m_maxGpuFreq);
}

void MiaNode::printPsiInfo() const
{
    std::string sys_content{};
    std::ifstream psi_block_stream{};
    auto dump_psi_info = [&sys_content, &psi_block_stream](const std::string &block) {
        psi_block_stream.open(block);
        if (!psi_block_stream.is_open()) {
            MLOGI(Mia2LogGroupCore, "sys_psi: Dump PSI [%s] failed", block.c_str());
            return;
        }
        while (std::getline(psi_block_stream, sys_content)) {
            if (!sys_content.empty()) {
                MLOGI(Mia2LogGroupCore, "sys_psi: [%s] %s", block.c_str(), sys_content.c_str());
            }
        }
        psi_block_stream.close();
    };
    for (auto &pth : PSI_BLOCKS_PATH) {
        dump_psi_info(pth);
    }
}

void MiaNode::updateProcFreqExifData(sp<MiaImage> &image, sp<Pipeline> &pipeline,
                                     bool readyToExecute)
{
    getCurrentHWStatus();
    char buf[1024] = {0};
    if (readyToExecute) {
        snprintf(buf, sizeof(buf), "%s begin:{Temp:%d CpuFreq:%d,%d,%d GpuFreq:%d} ",
                 mNodeInstance.c_str(), m_currentTemp, m_currentCpuFreq0, m_currentCpuFreq4,
                 m_currentCpuFreq7, m_currentGpuFreq);

        if ((m_currentCpuFreq7 == m_maxCpuFreq7) &&
            (m_currentCpuFreq7 >= PERFLOCK_FREQ_THRE_CPU7)) {
            MLOGI(Mia2LogGroupCore,
                  "pipeline[%s] node[%s] process is going to start, temp:%d  cpu0:%d/%d  "
                  "cpu4:%d/%d  cpu7:%d/%d  gpu:%d/%d",
                  pipeline->getPipelineName(), mNodeInstance.c_str(), m_currentTemp,
                  m_currentCpuFreq0, m_maxCpuFreq0, m_currentCpuFreq4, m_maxCpuFreq4,
                  m_currentCpuFreq7, m_maxCpuFreq7, m_currentGpuFreq, m_maxGpuFreq);
        } else {
            MLOGI(Mia2LogGroupCore,
                  "pipeline[%s] node[%s] process unexpected cpu freq, temp:%d  cpu0:%d/%d  "
                  "cpu4:%d/%d  cpu7:%d/%d  gpu:%d/%d",
                  pipeline->getPipelineName(), mNodeInstance.c_str(), m_currentTemp,
                  m_currentCpuFreq0, m_maxCpuFreq0, m_currentCpuFreq4, m_maxCpuFreq4,
                  m_currentCpuFreq7, m_maxCpuFreq7, m_currentGpuFreq, m_maxGpuFreq);
        }
    } else {
        snprintf(buf, sizeof(buf), "%s end:{Temp:%d CpuFreq:%d,%d,%d GpuFreq:%d} ",
                 mNodeInstance.c_str(), m_currentTemp, m_currentCpuFreq0, m_currentCpuFreq4,
                 m_currentCpuFreq7, m_currentGpuFreq);

        MLOGI(Mia2LogGroupCore,
              "pipeline[%s] node[%s] process end, temp:%d  cpu0:%d/%d  cpu4:%d/%d  "
              "cpu7:%d/%d  gpu:%d/%d",
              pipeline->getPipelineName(), mNodeInstance.c_str(), m_currentTemp, m_currentCpuFreq0,
              m_maxCpuFreq0, m_currentCpuFreq4, m_maxCpuFreq4, m_currentCpuFreq7, m_maxCpuFreq7,
              m_currentGpuFreq, m_maxGpuFreq);
    }
    std::string results(buf);
    mNodeCB->onResultComing(MiaResultCbType::MIA_META_CALLBACK, image->getFrameNumber(),
                            image->getTimestamp(), results);
}

int32_t MiaNode::updateAbnImageExifDataV1(ProcessRequestInfo &requestInfo)
{
    MiAbnormalParam pAbnormalParam;
    memset(&pAbnormalParam, 0, sizeof(MiAbnormalParam));
    auto &outputBuffers = requestInfo.outputBuffersMap.begin()->second;

    pAbnormalParam.addr = outputBuffers.at(0).pAddr;
    pAbnormalParam.format = outputBuffers.at(0).format.format;
    pAbnormalParam.stride = outputBuffers.at(0).planeStride;
    pAbnormalParam.width = outputBuffers.at(0).format.width;
    pAbnormalParam.height = outputBuffers.at(0).format.height;
    pAbnormalParam.frameNum = requestInfo.frameNum;
    pAbnormalParam.timeStamp = requestInfo.timeStamp;
    pAbnormalParam.isSourceBuffer = false;
    bool isMoonMode = false;
    void *pData = NULL;
    if (requestInfo.inputBuffersMap.empty()) {
        VendorMetadataParser::getVTagValue(outputBuffers.at(0).pMetadata,
                                           "xiaomi.ai.asd.sceneApplied", &pData);
    } else {
        auto &inputBuffers = requestInfo.inputBuffersMap.begin()->second;
        VendorMetadataParser::getVTagValue(inputBuffers.at(0).pMetadata,
                                           "xiaomi.ai.asd.sceneApplied", &pData);
    }
    if (NULL != pData) {
        isMoonMode = *static_cast<int *>(pData) == ADVANCEMOONMODEON ? true : false;
    }

    MiAbnormalDetect abnDetect = PluginUtils::isAbnormalImage(
        pAbnormalParam.addr, pAbnormalParam.format, pAbnormalParam.stride, pAbnormalParam.width,
        pAbnormalParam.height);
    if ((0x7878 <= abnDetect.plane1_value) && (abnDetect.plane1_value <= 0x8282) &&
        (abnDetect.plane0_value <= 0x20) && isMoonMode) {
        abnDetect.isAbnormal = 0;
        abnDetect.level = 0;
    }
    abnDetect.level = updateAbnImageExifData(pAbnormalParam, abnDetect);

    return abnDetect.level;
}
int32_t MiaNode::updateAbnImageExifDataV2(ProcessRequestInfoV2 &requestInfo)
{
    MiAbnormalParam pAbnormalParam;
    memset(&pAbnormalParam, 0, sizeof(MiAbnormalParam));
    auto &inputBuffers = requestInfo.inputBuffersMap.begin()->second;

    pAbnormalParam.addr = inputBuffers.at(0).pAddr;
    pAbnormalParam.format = inputBuffers.at(0).format.format;
    pAbnormalParam.stride = inputBuffers.at(0).planeStride;
    pAbnormalParam.width = inputBuffers.at(0).format.width;
    pAbnormalParam.height = inputBuffers.at(0).format.height;
    pAbnormalParam.frameNum = requestInfo.frameNum;
    pAbnormalParam.timeStamp = requestInfo.timeStamp;

    MiAbnormalDetect abnDetect = PluginUtils::isAbnormalImage(
        pAbnormalParam.addr, pAbnormalParam.format, pAbnormalParam.stride, pAbnormalParam.width,
        pAbnormalParam.height);

    abnDetect.level = updateAbnImageExifData(pAbnormalParam, abnDetect);

    return abnDetect.level;
}

int32_t MiaNode::updateAbnImageExifData(MiAbnormalParam &pAbnormalParam,
                                        MiAbnormalDetect &abnDetect)
{
    if (abnDetect.isAbnormal && mOutputBufferNeedCheck) {
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf),
                 "[%s Abnormal:%d level:%d line:%d format:%d val0:0x%x val1:0x%x isSource:%d]",
                 getNodeInstance().c_str(), abnDetect.isAbnormal, abnDetect.level, abnDetect.line,
                 abnDetect.format, abnDetect.plane0_value, abnDetect.plane1_value,
                 pAbnormalParam.isSourceBuffer);
        std::string results(buf);
        setResultMetadata(this, pAbnormalParam.frameNum, pAbnormalParam.timeStamp, results);
        if (abnDetect.level && pAbnormalParam.format != CAM_FORMAT_RAW16) {
            if (sIsAbnorDumpMode) {
                struct MiImageBuffer inputFrame;
                inputFrame.format = pAbnormalParam.format;
                inputFrame.width = pAbnormalParam.width;
                inputFrame.height = pAbnormalParam.height;
                inputFrame.stride = pAbnormalParam.stride;
                inputFrame.plane[0] = pAbnormalParam.addr[0];
                if ((inputFrame.format == CAM_FORMAT_YUV_420_NV12) ||
                    (inputFrame.format == CAM_FORMAT_YUV_420_NV21)) {
                    inputFrame.plane[1] = pAbnormalParam.addr[1];
                }
                char abnorbuf[128];
                const char *nodeName = strrchr(getNodeName(), '.');
                if (NULL != nodeName) {
                    nodeName += 1;
                }
                snprintf(abnorbuf, sizeof(abnorbuf), "MIMQS_abnor%d_line%d_source%d_%s_%dx%d",
                         abnDetect.isAbnormal, abnDetect.line, pAbnormalParam.isSourceBuffer,
                         nodeName, pAbnormalParam.width, pAbnormalParam.height);
                PluginUtils::dumpToFile(abnorbuf, &inputFrame);
            }
            // add abnormal picture error trigger
            char CamErrorString[512] = {0};
            char isSochanged[256] = "";
            property_get("vendor.camera.sensor.logsystem", isSochanged, "0");
            // When the value of logsystem is 1 or 3, the send_message_to_mqs interface is called
            // When the value of logsystem is 2, it means that the exception needs to be ignored
            if ((atoi(isSochanged) == 1 || atoi(isSochanged) == 3)) {
                snprintf(CamErrorString, 512, "[%s]:abnormal picture", getNodeName());
                MLOGW(Mia2LogGroupCore,
                      "[CAM_LOG_SYSTEM][MIVI] send error to MQS CamErrorString:%s", CamErrorString);
                // Force to do the last time flush to file to ensure MQS get complete error log.
                // and continue flush log to file
                MiDebugInterface::getInstance()->flushLogToFileInterface(true);
                // false: abort() will not be triggered
                // true: abort() will be triggered
                camlog::send_message_to_mqs(CamErrorString, false);
            } else {
                MLOGW(Mia2LogGroupCore, "%s", buf);
            }
        } else {
            MLOGI(Mia2LogGroupCore, "%s", buf);
            return 0;
        }
    }
    return abnDetect.level;
}

void MiaNode::dumpImageInfo(const std::vector<ImageParams> &buffers, std::string imageName,
                            std::string type, uint32_t portId)
{
    if (imageName.size() == 0 || type.size() == 0) {
        return;
    }

    size_t suffixStartIndex = imageName.find_last_of('.');
    if (suffixStartIndex != std::string::npos) {
        // IMG_20210715_124242.jpg  -->  IMG_20210715_124242
        // ID_CARD_1.jpg --> ID_CARD_1
        imageName = imageName.substr(0, suffixStartIndex);
    }

    std::vector<MiImageBuffer> frames;
    for (int i = 0; i < buffers.size(); i++) {
        MiImageBuffer frame = {0};
        frame.format = buffers[i].format.format;
        frame.width = buffers[i].format.width;
        frame.height = buffers[i].format.height;
        frame.plane[0] = buffers[i].pAddr[0];
        frame.plane[1] = buffers[i].pAddr[1];
        frame.stride = buffers[i].planeStride;
        frame.scanline = buffers[i].sliceheight;

        frames.push_back(frame);
    }

    if (mNodeMask & CONCURRENCY_MODE) {
        for (int i = 0; i < frames.size(); i++) {
            int lastDumpIndex = i;
            std::string dumpFileName = imageName + "_" + mNodeInstance + "_" + type;

            dumpFileName = dumpFileName + "_" + std::to_string(portId);
            dumpFileName = dumpFileName + "_" + std::to_string(lastDumpIndex);
            dumpFileName = dumpFileName + "_" + std::to_string(frames[i].width);
            dumpFileName = dumpFileName + "_" + std::to_string(frames[i].height);

            PluginUtils::dumpToFile(&(frames[i]), dumpFileName.c_str());
        }
    } else {
        for (int i = 0; i < frames.size(); i++) {
            if (mLastDumpFileName.compare(imageName) != 0) {
                mLastDumpFileName = imageName;
                mLastDumpInputIndex = -1;
                mLastDumpOutputIndex = -1;
            }

            std::string dumpFileName = imageName + "_" + mNodeInstance + "_" + type;
            dumpFileName = dumpFileName + "_" + std::to_string(portId);

            if (type == "output") {
                ++mLastDumpInputIndex;
                dumpFileName = dumpFileName + "_" + std::to_string(mLastDumpInputIndex);
            } else {
                ++mLastDumpOutputIndex;
                dumpFileName = dumpFileName + "_" + std::to_string(mLastDumpOutputIndex);
            }

            dumpFileName = dumpFileName + "_" + std::to_string(frames[i].width);
            dumpFileName = dumpFileName + "_" + std::to_string(frames[i].height);

            PluginUtils::dumpToFile(&(frames[i]), dumpFileName.c_str());
        }
    }
}

void MiaNode::signalThead(bool &isProcessDone, std::condition_variable &timeoutCond,
                          std::mutex &timeoutMux)
{
    std::lock_guard<std::mutex> locker(timeoutMux);
    isProcessDone = true;
    timeoutCond.notify_one();
}

void MiaNode::timeoutDetection(uint32_t tid, bool &isProcessDone,
                               std::condition_variable &timeoutCond, std::mutex &timeoutMux)
{
    std::unique_lock<std::mutex> lck(timeoutMux);
    if (!timeoutCond.wait_for(lck, std::chrono::seconds(kProcessWaitTimeoutSec),
                              [&isProcessDone] { return isProcessDone; })) {
        midebug::dumpThreadCallStack(tid, 0, "process-timeout", ANDROID_LOG_INFO);
        MLOGE(Mia2LogGroupCore, "[%s][%p] process timeout, tid:%u, abort!", getNodeName(), this,
              tid);
        MASSERT(false);
    }
}

void MiaNode::setResultMetadata(void *owner, uint32_t frameNum, int64_t timeStamp, string &result)
{
    MiaNode *node = (MiaNode *)owner;
    if (node == NULL) {
        MLOGE(Mia2LogGroupCore, "owner is NULL");
        return;
    }
    MLOGI(Mia2LogGroupCore, "frameNum: %d timeStamp: %" PRIu64, frameNum, timeStamp);

    NodeCB *nodeCb = node->getNodeCB();
    MiaResultCbType resultType = MiaResultCbType::MIA_META_CALLBACK;

    if (!result.compare(0, 9, "quickview")) {
        resultType = MiaResultCbType::MIA_ANCHOR_CALLBACK;
        int64_t firstTs = node->mPipeline->getFirstFrameTimestamp();
        string firstTsStr = std::to_string(firstTs);
        nodeCb->onResultComing(resultType, frameNum /*requestFm*/, timeStamp /*AnchorTs*/,
                               firstTsStr /*FirtTimestamp*/);
        return;
    }

    double processStartTime = 0.0;
    if ((node->mNodeMask) & CONCURRENCY_MODE) {
        std::lock_guard<std::mutex> lock(node->mProcTimeLock);
        processStartTime = node->mProcStartTimes[timeStamp];
        node->mProcStartTimes.erase(timeStamp);
    } else {
        processStartTime = node->mProcStartTime;
    }

    stringstream sstr;
    std::string exifResult = result;
    if (result.length() > 0 && result[result.length() - 1] == '}') {
        sstr << " time:" << MiaUtil::nowMSec() - processStartTime << "}";
        exifResult.replace(result.length() - 1, 1, sstr.str());
    } else {
        sstr << " time:" << MiaUtil::nowMSec() - processStartTime;
        exifResult += sstr.str();
    }

    nodeCb->onResultComing(resultType, frameNum, timeStamp, exifResult);
}

void MiaNode::setResultBuffer(void *owner, uint32_t frameNum)
{
    MiaNode *node = (MiaNode *)owner;
    if (node == NULL) {
        MLOGE(Mia2LogGroupCore, "owner is NULL");
        return;
    }
}

void MiaNode::setOutputFormat(void *owner, ImageFormat imageFormat)
{
    (void)owner;
    (void)imageFormat;
}

void MiaNode::releaseableUnneedBuffer(void *owner, int portId, int releaseBufferNumber,
                                      PluginReleaseBuffer releaseBufferMode)
{
    MiaNode *node = (MiaNode *)owner;
    if (node == NULL) {
        MLOGE(Mia2LogGroupCore, "owner is NULL");
        return;
    }

    std::string nodeInstance = node->getNodeInstance();
    node->mPipeline->pluginReleaseUnneedBuffer(nodeInstance, portId, releaseBufferNumber,
                                               releaseBufferMode);
}

bool MiaNode::processTaskError(sp<Pipeline> &pipeline, NodeProcessInfo nodeProcessInfo,
                               int32_t &abnormal, int32_t &frame_num)
{
    MLOGE(Mia2LogGroupCore, "[%s] node[%s] process error because %s", pipeline->getPipelineName(),
          mNodeInstance.c_str(), NodeProcessInfoToChar[nodeProcessInfo]);

    pipeline->notifyNextNode(mNodeInstance, nodeProcessInfo, abnormal, frame_num);

    bool nextRequestReady = true;
    std::lock_guard<std::mutex> locker(mProcessLock);

    if (!mRequestQueue.empty()) {
        mPipeline = mRequestQueue.top().second;
        mRequestQueue.pop();
        MLOGD(Mia2LogGroupCore, "node[%s] mPipeline[%s] mRequestQueue.size()=%d",
              mNodeInstance.c_str(), mPipeline->getPipelineName(), mRequestQueue.size());
    } else {
        mPipeline = nullptr;
        mHasProcessThread = false;
        nextRequestReady = false;
    }
    return nextRequestReady;
}

// be called after all node's process finish,if necessary. For session reuse
void MiaNode::reInit()
{
    m_currentTemp = 0;
    m_currentCpuFreq0 = 0;
    m_currentCpuFreq4 = 0;
    m_currentCpuFreq7 = 0;
    m_currentGpuFreq = 0;
    m_maxCpuFreq0 = 0;
    m_maxCpuFreq4 = 0;
    m_maxCpuFreq7 = 0;
    m_maxGpuFreq = 0;
}

bool MiaNode::checkTaskEffectiveness(const sp<Pipeline> &pipeline, CDKResult &info,
                                     NodeProcessInfo &nodeProcessInfo)
{
    FlushStatus pipelieFlushStatus = pipeline->getFlushStatus();

    if (pipelieFlushStatus == ForcedFlush) {
        info = MIAS_ABNORMAL;
        nodeProcessInfo = FlushTask;
    }

    return true;
}

ConcurrencyNode::ConcurrencyNode()
{
    // Create a node for parallel processing, nodeMask = 2
    MLOGI(Mia2LogGroupCore, "now construct %p", this);
}

ConcurrencyNode::~ConcurrencyNode()
{
    MLOGI(Mia2LogGroupCore, "[%s] now distruct %p", mNodeInstance.c_str(), this);
}

void ConcurrencyNode::enqueueTask(sp<Pipeline> pipeline)
{
    // no LIFO/FIFO case
    int pipelinePriority = pipeline->getFIFOPriority(); // can also use getLIFOPriority

    std::lock_guard<std::mutex> lock(mProcessLock);
    mRequestQueue.push(std::pair<int, sp<Pipeline>>(pipelinePriority, pipeline));

    mNodeCB->increaseUsingThreadNum(1);
    Singleton<ThreadPool>::getInstance()->enqueue([this]() { processTask(); });
}

void ConcurrencyNode::processTask()
{
    CDKResult processResult = MIAS_OK;
    NodeProcessInfo nodeProcessInfo = ProcessOK;
    sp<Pipeline> pipeline = nullptr;
    {
        std::lock_guard<std::mutex> lock(mProcessLock);
        if (!mRequestQueue.empty()) {
            pipeline = mRequestQueue.top().second;
            mRequestQueue.pop();
        } else {
            MLOGE(Mia2LogGroupCore, "[%s] unknow error", mNodeInstance.c_str());
            return;
        }
    }

    std::map<uint32_t, std::vector<sp<MiaImage>>> inputImages;
    std::map<uint32_t, std::vector<sp<MiaImage>>> outputImages;
    processResult = getInputBuffer(pipeline, inputImages);
    (processResult == MIAS_OK) ?: (nodeProcessInfo = GetInputError);

    checkTaskEffectiveness(pipeline, processResult, nodeProcessInfo);

    if (processResult == MIAS_OK) {
        processResult = getOutputBuffer(pipeline, inputImages, outputImages);
        (processResult == MIAS_OK) ?: (nodeProcessInfo = GetOutputError);
    }

    int32_t frame_num = 0;
    int32_t abnormal = 0;

    if (processResult == MIAS_OK) {
        MLOGI(Mia2LogGroupCore, "pipeline[%s] node[%s] process start", pipeline->getPipelineName(),
              mNodeInstance.c_str());

        updateProcFreqExifData(inputImages.begin()->second.at(0), pipeline, true);

        processResult = processRequest(inputImages, outputImages, abnormal, frame_num);
        (processResult == MIAS_OK) ?: (nodeProcessInfo = NodeProcessError);

        updateProcFreqExifData(inputImages.begin()->second.at(0), pipeline, false);
    }

    pipeline->notifyNextNode(mNodeInstance, nodeProcessInfo, abnormal, frame_num);
    mNodeCB->increaseUsingThreadNum(-1);
}

SigframeNode::SigframeNode()
{
    // Create a node that processes frame by frame, nodeMask = 16
    MLOGI(Mia2LogGroupCore, "now construct %p", this);
}

SigframeNode::~SigframeNode()
{
    MLOGI(Mia2LogGroupCore, "[%s] now distruct %p", mNodeInstance.c_str(), this);
}

void SigframeNode::enqueueTask(sp<Pipeline> pipeline)
{
    int pipelinePriority = pipeline->getFIFOPriority();

    std::lock_guard<std::mutex> lock(mProcessLock);

    mRequestQueue.push(std::pair<int, sp<Pipeline>>(pipelinePriority, pipeline));

    // Unified LIFO/FIFO logic
    bool highestPriorityRequest =
        (mPipeline == nullptr &&
         (!mRequestQueue.empty() && (pipeline == mRequestQueue.top().second)));
    if (highestPriorityRequest) {
        mPipeline = mRequestQueue.top().second;
        mRequestQueue.pop();
    }

    MLOGD(Mia2LogGroupCore,
          "insert [%s] to [%s]'s requestQueue, highestPriorityRequest:%d haveThread:%d "
          "runningPipe[%s] mRequestQueueSize:%d",
          pipeline->getPipelineName(), mNodeInstance.c_str(), highestPriorityRequest,
          mHasProcessThread, mPipeline->getPipelineName(), mRequestQueue.size());
    if (!mHasProcessThread && highestPriorityRequest) {
        mNodeCB->increaseUsingThreadNum(1);
        Singleton<ThreadPool>::getInstance()->enqueue([this]() { processTask(); });
        mHasProcessThread = true;
    }
}

void SigframeNode::processTask()
{
    /* If the task comes fast enough, we hope that these tasks
     * can be executed continuously in the same thread.
     */
    while (true) {
        CDKResult processResult = MIAS_OK;
        NodeProcessInfo nodeProcessInfo = ProcessOK;
        int32_t frame_num = 0;
        int32_t abnormal = 0;
        bool thisRequestFinish = true;
        sp<Pipeline> pipeline = mPipeline;
        std::map<uint32_t, std::vector<sp<MiaImage>>> inputImages;
        std::map<uint32_t, std::vector<sp<MiaImage>>> outputImages;
        processResult = getInputBuffer(pipeline, inputImages);
        (processResult == MIAS_OK) ?: (nodeProcessInfo = GetInputError);

        bool isInterruptTask = checkTaskEffectiveness(pipeline, processResult, nodeProcessInfo);

        if (!isInterruptTask && processResult == MIAS_OK) {
            sp<MiaImage> &mainImage = inputImages.begin()->second.at(0);
            int mergeInputNum = mainImage->getMergeInputNumber();

            if (needGetOutBufferThisTime(mergeInputNum)) {
                processResult = getOutputBuffer(pipeline, inputImages, outputImages);
                (processResult == MIAS_OK) ?: (nodeProcessInfo = GetOutputError);
            }

            ++mInputFrameNum;

            if (processResult == MIAS_OK) {
                MLOGI(Mia2LogGroupCore,
                      "pipeline[%s] node[%s] process start, processing frames: %d/%d",
                      pipeline->getPipelineName(), mNodeInstance.c_str(), mInputFrameNum,
                      mergeInputNum);

                if (mNodeMask & INPLACE_MODE && outputImages.empty()) {
                    processResult = processRequest(inputImages, abnormal, frame_num);
                } else {
                    processResult = processRequest(inputImages, outputImages, abnormal, frame_num);
                }

                (processResult == MIAS_OK) ?: (nodeProcessInfo = NodeProcessError);
            }

            if (mInputFrameNum != mergeInputNum) {
                thisRequestFinish = false;
            }
        }

        if (processResult != MIAS_OK) {
            mInputFrameNum = 0;
            if (processTaskError(pipeline, nodeProcessInfo, abnormal, frame_num)) {
                continue; // next request is come
            } else {
                break;
            }
        }

        std::unique_lock<std::mutex> locker(mProcessLock);

        bool nextRequestReady = false;
        if (thisRequestFinish) {
            locker.unlock(); // non critical area unlock

            mPipeline->notifyNextNode(mNodeInstance, nodeProcessInfo, abnormal, frame_num);

            locker.lock();
            mPipeline = nullptr; // reset is very important
            mInputFrameNum = 0;  // reset is very important

            if (!mRequestQueue.empty()) {
                nextRequestReady = true;
            }
        } else {
            if (mNodeMask & SIGRELEASEINPUT_MODE) {
                mPipeline->releaseInputAndTryNotifyNextNode(mNodeInstance);
            }
            if (mNodeMask & SIGRELEASEOUTPUT_MODE) {
                mPipeline->releaseOutputAndTryNotifyNextNode(mNodeInstance);
            }

            continue; // go to get next frame
        }

        if (nextRequestReady) { // new request is come, process it
            mPipeline = mRequestQueue.top().second;
            mRequestQueue.pop();
            MLOGD(Mia2LogGroupCore, "node[%s] mPipeline[%s] mRequestQueue.size()=%d",
                  mNodeInstance.c_str(), mPipeline->getPipelineName(), mRequestQueue.size());
            continue;
        } else { // new request not come
            MLOGD(Mia2LogGroupCore, "No pending tasks, node[%s] give up current thread resources",
                  mNodeInstance.c_str());
            mHasProcessThread = false;
            break;
        }
    }
    mNodeCB->increaseUsingThreadNum(-1);
}

bool SigframeNode::needGetOutBufferThisTime(int mergeInputNum)
{
    bool result = false;

    if (mNodeMask & SIGGETOUTPUT_MODE) { // every time
        result = true;
    }
    /*else if (mNodeName == "...") { // customized
        if ((mInputFrameNum + 1) == mergeInputNum) { // in last
            result = true;
        }
    }*/
    else if ((mInputFrameNum + 1) == 1) { // in first
        result = true;
    }

    return result;
}

bool SigframeNode::checkTaskEffectiveness(const sp<Pipeline> &pipeline, CDKResult &info,
                                          NodeProcessInfo &nodeProcessInfo)
{
    FlushStatus pipelieFlushStatus = pipeline->getFlushStatus();
    bool isInterruptTask = true;

    if (pipelieFlushStatus == ForcedFlush) {
        // pipeline is in ForcedFlush state and no longer needs to be processed
        if (mInputFrameNum > 0) { // plugin's processRequest has been called
            flush(true);
        }
        info = MIAS_ABNORMAL;
        nodeProcessInfo = FlushTask;
    }

    if (info == MIAS_OK) { // getInputBuffer sucess
        isInterruptTask = false;
    } else if (pipelieFlushStatus == Expediting) {
        // getInputBuffer fail is caused by pipeline Expediting
        // pipeline is in Expediting state, and the final result needs to be generated
        if (mInputFrameNum > 0) { // plugin's processRequest has been called
            flush(false);
            info = MIAS_OK; // this case is considered normal
            nodeProcessInfo = ProcessOK;
        } else {
            MLOGE(Mia2LogGroupCore, "Not got any image ,this is serious error");
        }
    }

    return isInterruptTask;
}

OtherNode::OtherNode()
{
    // Create other processing types of nodes
    MLOGI(Mia2LogGroupCore, "now construct %p", this);
}

OtherNode::~OtherNode()
{
    MLOGI(Mia2LogGroupCore, "[%s] now distruct %p", mNodeInstance.c_str(), this);
}

void OtherNode::enqueueTask(sp<Pipeline> pipeline)
{
    int pipelinePriority = 0;
    if (MiaInternalProp::getInstance()->getBool(OpenLIFO) && mNodeCB->isLIFOSupportedOfSession()) {
        pipelinePriority = pipeline->getLIFOPriority();
    } else {
        pipelinePriority = pipeline->getFIFOPriority();
    }

    std::lock_guard<std::mutex> lock(mProcessLock);

    mRequestQueue.push(std::pair<int, sp<Pipeline>>(pipelinePriority, pipeline));

    // Unified LIFO/FIFO logic
    // mPipeline must not be nullptr during frame by frame algorithm processing
    bool highestPriorityRequest =
        (mPipeline == nullptr &&
         (!mRequestQueue.empty() && (pipeline == mRequestQueue.top().second)));
    if (highestPriorityRequest) {
        mPipeline = mRequestQueue.top().second;
        mRequestQueue.pop();
    }
    MLOGD(Mia2LogGroupCore,
          "insert [%s] to [%s]'s requestQueue, highestPriorityRequest:%d haveThread:%d "
          "runningPipe[%s] mRequestQueueSize:%d",
          pipeline->getPipelineName(), mNodeInstance.c_str(), highestPriorityRequest,
          mHasProcessThread, mPipeline->getPipelineName(), mRequestQueue.size());
    if (!mHasProcessThread && highestPriorityRequest) {
        mNodeCB->increaseUsingThreadNum(1);
        Singleton<ThreadPool>::getInstance()->enqueue([this]() { processTask(); });
        mHasProcessThread = true;
    }
}

void OtherNode::processTask()
{
    /* If the task comes fast enough, we hope that these tasks
     * can be executed continuously in the same thread.
     */
    while (true) {
        CDKResult processResult = MIAS_OK;
        NodeProcessInfo nodeProcessInfo = ProcessOK;
        sp<Pipeline> pipeline = mPipeline;
        std::map<uint32_t, std::vector<sp<MiaImage>>> inputImages;
        std::map<uint32_t, std::vector<sp<MiaImage>>> outputImages;
        processResult = getInputBuffer(pipeline, inputImages);
        (processResult == MIAS_OK) ?: (nodeProcessInfo = GetInputError);

        checkTaskEffectiveness(pipeline, processResult, nodeProcessInfo);

        if (processResult == MIAS_OK) {
            processResult = getOutputBuffer(pipeline, inputImages, outputImages);
            (processResult == MIAS_OK) ?: (nodeProcessInfo = GetOutputError);
        }
        int32_t frame_num = 0;
        int32_t abnormal = 0;
        if (processResult == MIAS_OK) {
            MLOGI(Mia2LogGroupCore, "pipeline[%s] node[%s] process start",
                  pipeline->getPipelineName(), mNodeInstance.c_str());

            if (!mIsMiHalBurstMode) {
                updateProcFreqExifData(inputImages.begin()->second.at(0), pipeline, true);
            }

            if (mNodeMask & INPLACE_MODE && outputImages.empty()) {
                processResult = processRequest(inputImages, abnormal, frame_num);
            } else {
                processResult = processRequest(inputImages, outputImages, abnormal, frame_num);
            }

            (processResult == MIAS_OK) ?: (nodeProcessInfo = NodeProcessError);

            if (!mIsMiHalBurstMode) {
                updateProcFreqExifData(inputImages.begin()->second.at(0), pipeline, false);
            }
        }

        if (processResult != MIAS_OK) { // process error , this task is no longer being processed
            if (processTaskError(pipeline, nodeProcessInfo, abnormal, frame_num)) {
                continue; // next request is come
            } else {
                break;
            }
        }

        pipeline->notifyNextNode(mNodeInstance, nodeProcessInfo, abnormal, frame_num);

        std::lock_guard<std::mutex> lock(mProcessLock);
        if (!mRequestQueue.empty()) { // new request is come, process it
            mPipeline = mRequestQueue.top().second;
            mRequestQueue.pop();
            MLOGD(Mia2LogGroupCore, "node[%s] mPipeline[%s] mRequestQueue.size()=%d",
                  mNodeInstance.c_str(), mPipeline->getPipelineName(), mRequestQueue.size());
            continue;
        } else {
            // case. the next request not ready
            mHasProcessThread = false;
            mPipeline = nullptr; // reset is very important
            inputImages.clear();
            outputImages.clear();
            MLOGD(Mia2LogGroupCore, "No pending tasks, node[%s] give up current thread resources",
                  mNodeInstance.c_str());
            break;
        }
    }
    mNodeCB->increaseUsingThreadNum(-1);
}

void VirtualNode::enqueueTask(sp<Pipeline> pipeline)
{
    // no LIFO/FIFO case
    int pipelinePriority = pipeline->getFIFOPriority(); // can also use getLIFOPriority

    std::lock_guard<std::mutex> lock(mProcessLock);
    mRequestQueue.push(std::pair<int, sp<Pipeline>>(pipelinePriority, pipeline));

    mNodeCB->increaseUsingThreadNum(1);
    Singleton<ThreadPool>::getInstance()->enqueue([this]() { processTask(); });
}

void VirtualNode::processTask()
{
    sp<Pipeline> pipeline = nullptr;
    {
        std::lock_guard<std::mutex> lock(mProcessLock);
        if (!mRequestQueue.empty()) {
            pipeline = mRequestQueue.top().second;
            mRequestQueue.pop();
        }
    }

    if (pipeline != nullptr) {
        NodeProcessInfo nodeProcessInfo = ProcessOK;
        if (mNodeMask & SIGFRAME_MODE) {
            int processTimes = 0;
            while (true) {
                std::map<uint32_t, std::vector<sp<MiaImage>>> inputImages;
                if (getInputBuffer(pipeline, inputImages) == MIAS_OK) {
                    ++processTimes;
                } else {
                    nodeProcessInfo = GetInputError;
                    break;
                }

                sp<MiaImage> &mainImage = inputImages.begin()->second.at(0);
                int mergeInputNum = mainImage->getMergeInputNumber();

                MLOGD(Mia2LogGroupCore,
                      "pipeline[%s] node[%s] process start, processing frames: %d/%d",
                      pipeline->getPipelineName(), mNodeInstance.c_str(), processTimes,
                      mergeInputNum);

                if (processTimes == mergeInputNum) {
                    break;
                }

                if (mNodeMask & SIGRELEASEINPUT_MODE) {
                    pipeline->releaseInputAndTryNotifyNextNode(mNodeInstance);
                }
            }
        } else {
            std::map<uint32_t, std::vector<sp<MiaImage>>> inputImages;
            getInputBuffer(pipeline, inputImages);
        }

        pipeline->notifyNextNode(mNodeInstance, nodeProcessInfo, 0, 0);
    } else {
        MLOGE(Mia2LogGroupCore, "running pipeline in Node[%s] is NULL", mNodeInstance.c_str());
    }

    mNodeCB->increaseUsingThreadNum(-1);
}

MiaNodeFactory::MiaNodeFactory()
{
    MLOGI(Mia2LogGroupCore, "now construct %p", this);
}

MiaNodeFactory::~MiaNodeFactory()
{
    MLOGI(Mia2LogGroupCore, "now distruct %p", this);
}

void MiaNodeFactory::CreateNodeInstance(const uint32_t mNodeMask, sp<MiaNode> &FactoryNode)
{
    MLOGI(Mia2LogGroupCore, "MiaNodeFactory CreateNodeInstance");
    if (mNodeMask & VIRTUAL_MODE) {
        FactoryNode = new VirtualNode();
    } else if (mNodeMask & CONCURRENCY_MODE) {
        FactoryNode = new ConcurrencyNode();
    } else if (mNodeMask & SIGFRAME_MODE) {
        FactoryNode = new SigframeNode();
    } else {
        FactoryNode = new OtherNode();
    }
}

} // namespace mialgo2
