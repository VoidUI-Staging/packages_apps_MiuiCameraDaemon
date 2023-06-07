#include "HWJpegPlugin.h"

#include "MiZoneTypes.h"
#include "MiaPluginUtils.h"
#include "MiaPostProcType.h"

using namespace mizone::mipostprocinterface;
using mizone::PostProcParams;

// static std::map<std::string, CapFeatureType> gNodeInstance2ProcessType = {
//     {"Raw2YuvInstance", CapFeatureType::FEATURE_R2Y},
//     {"Raw2YuvInstance0", CapFeatureType::FEATURE_R2Y},
//     {"Raw2YuvInstance1", CapFeatureType::FEATURE_R2Y},
//     {"MfnrInstance", CapFeatureType::FEATURE_MFNR},     // maybe AINR;
//     {"Raw2RawInstance", CapFeatureType::FEATURE_R2R},
//     {""}
//     // TODO:
// };

static const int32_t sIsDumpData =
    property_get_int32("persist.vendor.camera.algoengine.hwjpeg.dump", 0);

HWJpegPlugin::~HWJpegPlugin()
{
    MLOGI(Mia2LogGroupPlugin, "%p", this);
}

int HWJpegPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    MLOGI(Mia2LogGroupPlugin, "+");
    MLOGI(Mia2LogGroupPlugin, "this:%p", this);

    mNodeInstanceName = createInfo->nodeInstance;
    mOperationMode = createInfo->operationMode;
    mCameraMode = createInfo->frameworkCameraId;

    mProcAdp = MiPluginAdaptor::create(mCreateInfo);

    mResultInfo = PROCFAILED;

    mNodeInterface = nodeInterface;
    MLOGI(Mia2LogGroupPlugin, "-");
    return 0;
}

void dumpProcessRequestInfo(ProcessRequestInfo *processInfo)
{
    MLOGD(LogGroupHAL,
          "frameNum = %d, timeStamp = %ld, "
          "isFirstFrame = %d, maxConcurrentNum = %d, runningTaskNum = %d"
          "inputBuffersMap.size = %lu, outputBuffersMap.size = %lu, ",
          processInfo->frameNum, processInfo->timeStamp, processInfo->isFirstFrame,
          processInfo->maxConcurrentNum, processInfo->runningTaskNum,
          processInfo->inputBuffersMap.size(), processInfo->outputBuffersMap.size());

    for (auto &&[port, phInputBuffer] : processInfo->inputBuffersMap) {
        MLOGD(LogGroupHAL, "inputBuffers: port = %d, phInputBuffer.size = %lu", port,
              phInputBuffer.size());
        for (auto &&buffer : phInputBuffer) {
            MLOGD(LogGroupHAL,
                  "    "
                  "format = %d, "
                  "width = %d, "
                  "height = %d, "
                  "planeStride = %d, "
                  "sliceheight = %d, "
                  "pMetadata = %p, "
                  "pPhyCamMetadata = %p, "
                  "numberOfPlanes = %d, "
                  "fd[0] = %d, "
                  "pAddr[0] = %p, "
                  "pNativeHandle = %p, "
                  "bufferType = %d, "
                  "cameraId = %d, "
                  "roleId = %d, ",
                  buffer.format.format, buffer.format.width, buffer.format.height,
                  buffer.planeStride, buffer.sliceheight, buffer.pMetadata, buffer.pPhyCamMetadata,
                  buffer.numberOfPlanes, buffer.fd[0], buffer.pAddr[0], buffer.pNativeHandle,
                  buffer.bufferType, buffer.cameraId, buffer.roleId);
            if (buffer.reserved != NULL) {
                auto pInPostProcParams = static_cast<PostProcParams *>(buffer.reserved);
                MLOGD(LogGroupHAL,
                      "    batchedSupplementary: "
                      "sessionParams.count = %d, "
                      "bufferId = %lu, bufferSettings.count = %d, halSettings.count = %d, usage = "
                      "%lu",
                      pInPostProcParams->sessionParams.count(), pInPostProcParams->bufferId,
                      pInPostProcParams->bufferSettings.count(),
                      pInPostProcParams->halSettings.count(), pInPostProcParams->usage);
            }
        }
    }

    for (auto &&[port, phOutputBuffer] : processInfo->outputBuffersMap) {
        MLOGD(LogGroupHAL, "outputBuffers: port = %d, phOutputBuffer.size = %lu", port,
              phOutputBuffer.size());
        for (auto &&buffer : phOutputBuffer) {
            MLOGD(LogGroupHAL,
                  "    "
                  "format = %d, "
                  "width = %d, "
                  "height = %d, "
                  "planeStride = %d, "
                  "sliceheight = %d, "
                  "pMetadata = %p, "
                  "pPhyCamMetadata = %p, "
                  "numberOfPlanes = %d, "
                  "fd[0] = %d, "
                  "pAddr[0] = %p, "
                  "pNativeHandle = %p, "
                  "bufferType = %d, "
                  "cameraId = %d, "
                  "roleId = %d, ",
                  buffer.format.format, buffer.format.width, buffer.format.height,
                  buffer.planeStride, buffer.sliceheight, buffer.pMetadata, buffer.pPhyCamMetadata,
                  buffer.numberOfPlanes, buffer.fd[0], buffer.pAddr[0], buffer.pNativeHandle,
                  buffer.bufferType, buffer.cameraId, buffer.roleId);
        }
    }
}

ProcessRetStatus HWJpegPlugin::processRequest(ProcessRequestInfo *processInfo)
{
    MLOGI(Mia2LogGroupPlugin, "+");
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.algoengine.hwjpeg.dump", prop, "0");
    bool needDump = (int32_t)atoi(prop) == 1;

    if (needDump) {
        for (auto &&[port, phInputBuffer] : processInfo->inputBuffersMap) {
            for (size_t i = 0; i < phInputBuffer.size(); ++i) {
                const auto &imageParam = phInputBuffer[i];
                struct MiImageBuffer inputFrame;
                inputFrame.format = imageParam.format.format;
                inputFrame.width = imageParam.format.width;
                inputFrame.height = imageParam.format.height;
                inputFrame.plane[0] = imageParam.pAddr[0];
                inputFrame.plane[1] = imageParam.pAddr[1];
                inputFrame.stride = imageParam.planeStride;
                inputFrame.scanline = imageParam.sliceheight;
                inputFrame.numberOfPlanes = imageParam.numberOfPlanes;

                std::string fileName = "HWJpeg_" + mNodeInstanceName;
                fileName += "_input_" + std::to_string(i) + "_" + std::to_string(inputFrame.width) +
                            "x" + std::to_string(inputFrame.height) + "_port_" +
                            std::to_string(port);
                PluginUtils::dumpToFile(fileName.c_str(), &inputFrame);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(mIsProcessDoneMutex);
        mIsProcessDone = false;
    }

    dumpProcessRequestInfo(processInfo);
    for (auto &&[port, phOutputBuffer] : processInfo->outputBuffersMap) {
        for (int i = 0; i < phOutputBuffer.size(); i++) {
            if (phOutputBuffer[i].format.format == CAM_FORMAT_JPEG) {
                phOutputBuffer[i].format.width = processInfo->inputBuffersMap.at(i)[0].format.width;
                phOutputBuffer[i].format.height =
                    processInfo->inputBuffersMap.at(i)[0].format.height;
            }
        }
    }

    mProcReqInfo = processInfo;
    ProcessInfos pInfo;
    pInfo.cb = this;

    pInfo.capFeatureType = CapFeatureType::FEATURE_Y2J;
    pInfo.multiFrameNum = processInfo->inputBuffersMap.at(0).size();

    MLOGI(Mia2LogGroupPlugin, "send to postProcAdaptor.");
    mProcAdp->process(pInfo, mProcReqInfo);

    {
        std::unique_lock<std::mutex> lock(mIsProcessDoneMutex);
        mIsProcessDoneCond.wait(lock, [this]() { return mIsProcessDone; });
    }
#if 0
    // for offline session test, to simulate long time process
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
#endif

    if (needDump) {
        for (auto &&[port, phOutputBuffer] : processInfo->outputBuffersMap) {
            for (size_t i = 0; i < phOutputBuffer.size(); ++i) {
                const auto &imageParam = phOutputBuffer[i];
                struct MiImageBuffer outputFrame;
                outputFrame.format = imageParam.format.format;
                outputFrame.width = imageParam.format.width;
                outputFrame.height = imageParam.format.height;
                outputFrame.plane[0] = imageParam.pAddr[0];
                outputFrame.plane[1] = imageParam.pAddr[1];
                outputFrame.stride = imageParam.planeStride;
                outputFrame.scanline = imageParam.sliceheight;
                outputFrame.numberOfPlanes = imageParam.numberOfPlanes;

                std::string fileName = "HWJpeg_" + mNodeInstanceName;
                fileName += "_output_" + std::to_string(i) + "_" +
                            std::to_string(outputFrame.width) + "x" +
                            std::to_string(outputFrame.height) + "_port_" + std::to_string(port);
                PluginUtils::dumpToFile(fileName.c_str(), &outputFrame);
            }
        }
    }

    MLOGI(Mia2LogGroupPlugin, "-");
    return mResultInfo;
}

ProcessRetStatus HWJpegPlugin::processRequest(ProcessRequestInfoV2 *processInfo2)
{
    // bypass this inplace-func;
    ProcessRetStatus resultInfo = PROCSUCCESS;

    return resultInfo;
}

int HWJpegPlugin::flushRequest(FlushRequestInfo *flushInfo)
{
    mProcAdp->flush();
    MLOGI(Mia2LogGroupPlugin, "flush.");
    return 0;
}

void HWJpegPlugin::destroy()
{
    MLOGI(Mia2LogGroupPlugin, "+");

    mProcAdp->destroy();
    mProcAdp = nullptr;
    MLOGI(Mia2LogGroupPlugin, "destroy:%p", this);
    MLOGI(Mia2LogGroupPlugin, "-");
}

void HWJpegPlugin::processResultDone(CallbackInfos &callbackInfos)
{
    MLOGI(Mia2LogGroupPlugin, "+");

    uint32_t frameNum = mProcReqInfo->frameNum;
    std::string results = callbackInfos.settings; //++++
    mResultInfo = callbackInfos.msg == ResultStatus::OK ? PROCSUCCESS : PROCFAILED;
    if (mResultInfo == PROCSUCCESS) {
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, frameNum, mTimeStamp, results);
    } else {
        MLOGI(Mia2LogGroupPlugin, "Error!");
    }

    {
        std::lock_guard<std::mutex> lock(mIsProcessDoneMutex);
        mIsProcessDone = true;
    }
    mIsProcessDoneCond.notify_one();

    MLOGI(Mia2LogGroupPlugin, "-");
}

bool HWJpegPlugin::isEnabled(MiaParams settings)
{
    MLOGD(Mia2LogGroupPlugin, "mOperationMode: 0x%x", mOperationMode);

    // TODO:
    return true;
}