#include "HWProcPlugin.h"

#include <MiaMetadataUtils.h>

#include "MiZoneTypes.h"
#include "MiaPluginUtils.h"
#include "MiaPostProcType.h"

using namespace mizone::mipostprocinterface;
using mizone::PostProcParams;

static std::map<std::string, CapFeatureType> gNodeInstance2ProcessType = {
    {"Raw2YuvInstance", CapFeatureType::FEATURE_R2Y},
    {"Raw2YuvInstance0", CapFeatureType::FEATURE_R2Y},
    {"Raw2YuvInstance1", CapFeatureType::FEATURE_R2Y},
    {"MfnrInstance", CapFeatureType::FEATURE_MFNR}, // maybe AINR;
    {"MfnrInstance0", CapFeatureType::FEATURE_MFNR},
    {"MfnrInstance1", CapFeatureType::FEATURE_MFNR},
    {"Raw2RawInstance", CapFeatureType::FEATURE_R2R},
    {"Raw2YuvSelectionInstance", CapFeatureType::FEATURE_R2Y},
    {"Yuv2YuvScaleInstance", CapFeatureType::FEATURE_Y2Y},
    {"Yuv2YuvInstance", CapFeatureType::FEATURE_Y2Y}
    // TODO:
};

static const int32_t sIsDumpData =
    property_get_int32("persist.vendor.camera.algoengine.hwproc.dump", 0);

HWProcPlugin::~HWProcPlugin()
{
    MLOGI(Mia2LogGroupPlugin, "%p", this);
}

int HWProcPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
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
          "frameNum = %d timeStamp = %ld, "
          "isFirstFrame = %d, maxConcurrentNum = %d, runningTaskNum = %d"
          "phInputBuffer.portCount = %lu, phOutputBuffer.portCount = %lu",
          processInfo->frameNum, processInfo->timeStamp, processInfo->isFirstFrame,
          processInfo->maxConcurrentNum, processInfo->runningTaskNum,
          processInfo->inputBuffersMap.size(), processInfo->outputBuffersMap.size());

    MLOGD(LogGroupHAL, "outputBuffers: ");
    for (auto &&[port, imageVec] : processInfo->outputBuffersMap) {
        for (size_t i = 0; i < imageVec.size(); ++i) {
            auto &&buffer = imageVec[i];
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
                auto pInPostProcParames = static_cast<PostProcParams *>(buffer.reserved);
                MLOGD(LogGroupHAL,
                      "    batchedSupplementary: "
                      "sessionParams.count = %d, "
                      "bufferId = %lu, bufferSettings.count = %d, halSettings.count = %d, usage = "
                      "%lu",
                      pInPostProcParames->sessionParams.count(), pInPostProcParames->bufferId,
                      pInPostProcParames->bufferSettings.count(),
                      pInPostProcParames->halSettings.count(), pInPostProcParames->usage);
            }
        }
    }

    MLOGD(LogGroupHAL, "inputBuffers: ");
    for (auto &&[port, imageVec] : processInfo->inputBuffersMap) {
        for (size_t i = 0; i < imageVec.size(); ++i) {
            auto &&buffer = imageVec[i];
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

ProcessRetStatus HWProcPlugin::processRequest(ProcessRequestInfo *processInfo)
{
    MLOGI(Mia2LogGroupPlugin, "+");
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.algoengine.hwproc.dump", prop, "0");
    bool needDump = (int32_t)atoi(prop) == 1;

    if (needDump) {
        for (auto &&[port, imageVec] : processInfo->inputBuffersMap) {
            for (size_t i = 0; i < imageVec.size(); ++i) {
                const auto &imageParam = imageVec[i];
                struct MiImageBuffer outputFrame;
                outputFrame.format = imageParam.format.format;
                outputFrame.width = imageParam.format.width;
                outputFrame.height = imageParam.format.height;
                outputFrame.plane[0] = imageParam.pAddr[0];
                outputFrame.plane[1] = imageParam.pAddr[1];
                outputFrame.stride = imageParam.planeStride;
                outputFrame.scanline = imageParam.sliceheight;
                outputFrame.numberOfPlanes = imageParam.numberOfPlanes;

                std::string fileName = "HWProc_" + mNodeInstanceName;
                fileName += "_input_" + std::to_string(i) + "_" +
                            std::to_string(outputFrame.width) + "x" +
                            std::to_string(outputFrame.height);
                PluginUtils::dumpToFile(fileName.c_str(), &outputFrame);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(mIsProcessDoneMutex);
        mIsProcessDone = false;
    }

    // if input contain physical stream, set all output stream's physical camera id to the same one
    int32_t phyId = -1;
    for (auto &&[port, imageVec] : processInfo->inputBuffersMap) {
        for (size_t i = 0; i < imageVec.size(); ++i) {
            if (imageVec[i].reserved != NULL) {
                phyId = static_cast<PostProcParams *>(imageVec[i].reserved)
                            ->physicalCameraSettings.physicalCameraId;
                break;
            }
        }
    }
    if (phyId != -1) {
        for (auto &&[port, imageVec] : processInfo->outputBuffersMap) {
            for (size_t i = 0; i < imageVec.size(); ++i) {
                auto pOutPostProcParams = static_cast<PostProcParams *>(imageVec[i].reserved);
                pOutPostProcParams->physicalCameraSettings.physicalCameraId = phyId;
            }
        }
    }

    dumpProcessRequestInfo(processInfo);

    mProcReqInfo = processInfo;
    mFwkTimeStamp = processInfo->timeStamp;
    ProcessInfos pInfo;
    pInfo.cb = this;

    pInfo.capFeatureType = CapFeatureType::FEATURE_R2Y;
    pInfo.multiFrameNum = processInfo->inputBuffersMap.begin()->second.size();

    void *pData = NULL;
    bool mfnrEnable = false;
    auto settingMeta = processInfo->inputBuffersMap.begin()->second[0].pMetadata;

    int32_t feature = 0;
    VendorMetadataParser::getTagValue(settingMeta, MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_FEATURE,
                                      &pData);
    if (NULL != pData) {
        feature = *static_cast<int32_t *>(pData);
    }

    if (mNodeInstanceName == "MfnrInstance") {
        if (feature == MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AINR) {
            pInfo.capFeatureType = CapFeatureType::FEATURE_AINR;
        } else if (feature == MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AISHUTTER1) {
            pInfo.capFeatureType = CapFeatureType::FEATURE_AISHUTTER1;
        } else if (feature == MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AISHUTTER2) {
            pInfo.capFeatureType = CapFeatureType::FEATURE_AISHUTTER2;
        } else if (feature == MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_MFNR) {
            pInfo.capFeatureType = CapFeatureType::FEATURE_MFNR;
        } else {
            MLOGI(Mia2LogGroupCore, "shouldn't be here!");
        }
    } else if (mNodeInstanceName == "Raw2YuvSelectionInstance") {
        pInfo.capFeatureType = gNodeInstance2ProcessType[mNodeInstanceName];
        int gloden_index = 0;
        pData = NULL;
        VendorMetadataParser::getTagValue(settingMeta, MTK_MFNR_FEATURE_BSS_GOLDEN_INDEX, &pData);
        if (NULL != pData) {
            gloden_index = *static_cast<int32_t *>(pData);
        }
        MLOGD(Mia2LogGroupPlugin, "hwplugin gloden_index is %d", gloden_index);
        int count = 0;
        ImageParams tmp;
        for (auto &&[prot, imgVec] : mProcReqInfo->inputBuffersMap) {
            for (int i = 0; i < imgVec.size(); i++) {
                if (count == gloden_index) {
                    tmp = imgVec[i];
                }
                count++;
            }
        }

        auto setting = tmp.pMetadata;
        if (mOperationMode == StreamConfigModeBokeh) {
            void *pData = NULL;
            bool rawSnEnable = false;
            VendorMetadataParser::getTagValue(setting, XIAOMI_SuperNight_FEATURE_ENABLED, &pData);
            if (NULL != pData) {
                rawSnEnable = *static_cast<int *>(pData);
            }

            pData = NULL;
            uint8_t hdrFlag = false;
            VendorMetadataParser::getTagValue(setting, XIAOMI_HDR_FEATURE_ENABLED, &pData);
            if (NULL != pData) {
                hdrFlag = *static_cast<uint8_t *>(pData);
            }
            if (rawSnEnable) {
                // bokehse: enable slaver's OB module in isp
                int32_t value = -1;
                int ret = VendorMetadataParser::setTagValue(
                    setting, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, &value, 1);
                MLOGD(Mia2LogGroupCore, "bokeh se: enable slaver's OB module for slaver,ret: %d",
                      ret);
            } else if (hdrFlag) {
                // bokehhdr
                // set tuning hint for slaver, beause master rewrite the meta when returing on left
                // stage
                int32_t bss = 0;
                int retMf1 = VendorMetadataParser::setTagValue(
                    setting, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, &bss, 1);

                int32_t hdrHint = XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_HDR;
                int retHint = VendorMetadataParser::setTagValue(
                    setting, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, &hdrHint, 1);

                MLOGD(Mia2LogGroupCore, "bokeh hdr: disable mfnr for slaver, retMf1:%d, retHint:%d",
                      retMf1, retHint);
            }
        }

        int port = mProcReqInfo->inputBuffersMap.begin()->first;
        mProcReqInfo->inputBuffersMap.clear();
        mProcReqInfo->inputBuffersMap[port] = {tmp};

    } else {
        pInfo.capFeatureType = gNodeInstance2ProcessType[mNodeInstanceName];
    }

    pData = NULL;
    camera_metadata_t *metadata = processInfo->inputBuffersMap.begin()->second[0].pMetadata;
    VendorMetadataParser::getTagValue(metadata, XIAOMI_MULTIFEAME_INPUTNUM, &pData);
    // VendorMetadataParser::getVTagValue(metadata, "xiaomi.multiframe.inputNum", &pData);
    if (NULL != pData) {
        uint32_t inputNum = *static_cast<unsigned int *>(pData);
        MLOGI(Mia2LogGroupCore, "get multiframe inputNum = %d", inputNum);
        pInfo.multiFrameNum = inputNum;
    }

    pData = NULL;
    int srEnable = false;
    // const char *SrEnable = "xiaomi.superResolution.enabled";
    VendorMetadataParser::getTagValue(metadata, XIAOMI_SR_ENABLED, &pData);
    if (NULL != pData) {
        srEnable = *static_cast<int *>(pData);
    }

    if (srEnable && mNodeInstanceName == "Yuv2YuvInstance") {
        int singleNR = 1;
        VendorMetadataParser::setTagValue(metadata, MTK_CONTROL_CAPTURE_SINGLE_YUV_NR, &singleNR,
                                          1);

        pData = NULL;
        MIRECT cropRegion = {};
        float zoomRatio = 1.f;
        MINT32 hitMFSR = XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_MFSR;
        // VendorMetadataParser::getVTagValue(metadata, "xiaomi.superResolution.cropRegionMtk",
        // &pData);
        VendorMetadataParser::getTagValue(metadata, XIAOMI_SR_CROP_REOGION_MTK, &pData);
        if (NULL != pData) {
            cropRegion = *static_cast<MIRECT *>(pData);
            zoomRatio = (float)(cropRegion.left * 2 + cropRegion.width) / cropRegion.width;
            VendorMetadataParser::setTagValue(metadata, ANDROID_CONTROL_ZOOM_RATIO, &zoomRatio, 1);
            VendorMetadataParser::setTagValue(metadata, MTK_POSTPROCDEV_PROCESSED_CROP_INFO,
                                              &cropRegion, 4);
            VendorMetadataParser::setTagValue(metadata, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                              &hitMFSR, 1);
            MLOGD(Mia2LogGroupPlugin,
                  "SR get crop region from SAT cropregion: [%d %d %d %d] zoomRatio: %f",
                  cropRegion.left, cropRegion.top, cropRegion.width, cropRegion.height, zoomRatio);
        }
    }

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
        for (auto &&[port, imageVec] : processInfo->outputBuffersMap) {
            for (size_t i = 0; i < imageVec.size(); ++i) {
                const auto &imageParam = imageVec[i];
                struct MiImageBuffer outputFrame;
                outputFrame.format = imageParam.format.format;
                outputFrame.width = imageParam.format.width;
                outputFrame.height = imageParam.format.height;
                outputFrame.plane[0] = imageParam.pAddr[0];
                outputFrame.plane[1] = imageParam.pAddr[1];
                outputFrame.stride = imageParam.planeStride;
                outputFrame.scanline = imageParam.sliceheight;
                outputFrame.numberOfPlanes = imageParam.numberOfPlanes;

                std::string fileName = "HWProc_" + mNodeInstanceName;
                fileName += "_output_" + std::to_string(i) + "_" +
                            std::to_string(outputFrame.width) + "x" +
                            std::to_string(outputFrame.height);
                PluginUtils::dumpToFile(fileName.c_str(), &outputFrame);
            }
        }
    }

    MLOGI(Mia2LogGroupPlugin, "-");
    return mResultInfo;
}

ProcessRetStatus HWProcPlugin::processRequest(ProcessRequestInfoV2 *processInfo2)
{
    // bypass this inplace-func;
    ProcessRetStatus resultInfo = PROCSUCCESS;

    return resultInfo;
}

int HWProcPlugin::flushRequest(FlushRequestInfo *flushInfo)
{
    mProcAdp->flush();
    MLOGI(Mia2LogGroupPlugin, "flush.");
    return 0;
}

void HWProcPlugin::destroy()
{
    MLOGI(Mia2LogGroupPlugin, "+");
    mProcAdp->destroy();
    mProcAdp = nullptr;
    MLOGI(Mia2LogGroupPlugin, "destroy:%p", this);
    MLOGI(Mia2LogGroupPlugin, "-");
}

void HWProcPlugin::processResultDone(CallbackInfos &callbackInfos)
{
    MLOGI(Mia2LogGroupPlugin, "+");

    uint32_t frameNum = mProcReqInfo->frameNum;
    std::string results = callbackInfos.settings; //++++
    mResultInfo = callbackInfos.msg == ResultStatus::OK ? PROCSUCCESS : PROCFAILED;
    if (mResultInfo == PROCSUCCESS) {
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, frameNum, mFwkTimeStamp, results);
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

bool HWProcPlugin::isEnabled(MiaParams settings)
{
    void *pData = NULL;
    bool hdrEnable = false;
    // const char *hdrEnablePtr = "xiaomi.hdr.enabled";
    // VendorMetadataParser::getVTagValue(settings.metadata, hdrEnablePtr, &pData);
    VendorMetadataParser::getTagValue(settings.metadata, XIAOMI_HDR_FEATURE_ENABLED, &pData);
    if (NULL != pData) {
        hdrEnable = *static_cast<uint8_t *>(pData);
    }

    pData = NULL;
    bool hdrsrEnable = false;
    VendorMetadataParser::getTagValue(settings.metadata, XIAOMI_HDR_SR_FEATURE_ENABLED, &pData);
    if (NULL != pData) {
        hdrsrEnable = *static_cast<int *>(pData);
    }

    pData = NULL;
    bool miMfnrEnable = false;
    VendorMetadataParser::getTagValue(settings.metadata, XIAOMI_MFNR_ENABLED, &pData);
    if (NULL != pData) {
        miMfnrEnable = *static_cast<int *>(pData);
    }

    pData = NULL;
    bool mfnrEnable = false;
    bool ainrEnable = false;
    int32_t feature = 0;
    VendorMetadataParser::getTagValue(settings.metadata,
                                      MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_FEATURE, &pData);
    if (NULL != pData) {
        feature = *static_cast<int32_t *>(pData);
    }
    if (feature == MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AINR ||
        feature == MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AISHUTTER1 ||
        feature == MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AISHUTTER2) {
        ainrEnable = true;
    } else if (feature == MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_MFNR || miMfnrEnable) {
        mfnrEnable = true;
    }

    bool mfnrOrAinr = ainrEnable | mfnrEnable;

    bool mfnrHdrEnable = hdrEnable && mfnrEnable;

    pData = NULL;
    int srEnable = false;
    // const char *SrEnable = "xiaomi.superResolution.enabled";
    VendorMetadataParser::getTagValue(settings.metadata, XIAOMI_SR_ENABLED, &pData);
    if (NULL != pData) {
        srEnable = *static_cast<int *>(pData);
    }

    pData = NULL;
    bool rawSnEnable = false;
    VendorMetadataParser::getTagValue(settings.metadata, XIAOMI_SuperNight_FEATURE_ENABLED, &pData);
    if (NULL != pData) {
        rawSnEnable = *static_cast<int *>(pData);
    }

    pData = NULL;
    bool remosaicEnable = false;
    VendorMetadataParser::getTagValue(settings.metadata, XIAOMI_REMOSAIC_ENABLED, &pData);
    if (NULL != pData) {
        remosaicEnable = *static_cast<int *>(pData);
    }

    MLOGI(Mia2LogGroupPlugin,
          "OperationMode:0x%x, hdr:%d, mfnr:%d, ainr:%d, sr:%d, rawsn:%d, remosaic:%d, mfnrHdr:%d, "
          "hdsr:%d",
          mOperationMode, hdrEnable, mfnrEnable, ainrEnable, srEnable, rawSnEnable, remosaicEnable,
          mfnrHdrEnable, hdrsrEnable);

    if (mOperationMode == StreamConfigModeSAT) {
        if (mfnrHdrEnable) {
            return mNodeInstanceName == "MfnrInstance" || mNodeInstanceName == "Raw2YuvInstance0";
        } else if (hdrsrEnable) {
            return mNodeInstanceName == "Raw2YuvInstance1" ||
                   mNodeInstanceName == "Raw2YuvInstance0" ||
                   mNodeInstanceName == "Yuv2YuvInstance";
        } else if (srEnable) {
            return mNodeInstanceName == "Raw2YuvInstance" || mNodeInstanceName == "Yuv2YuvInstance";
        } else if (rawSnEnable) {
            return mNodeInstanceName == "Raw2YuvInstance";
        } else if (mfnrOrAinr) {
            return mNodeInstanceName == "MfnrInstance";
        } else {
            return mNodeInstanceName == "Raw2YuvInstance1";
        }
    }

    if (mOperationMode == StreamConfigModeBokeh) {
        if (mCameraMode == CAM_COMBMODE_FRONT_BOKEH || mCameraMode == CAM_COMBMODE_FRONT) {
            if (mfnrOrAinr) {
                return mNodeInstanceName == "MfnrInstance";
            } else {
                return mNodeInstanceName == "Raw2YuvInstance";
            }
        } else {
            if (rawSnEnable) {
                return mNodeInstanceName == "Raw2YuvInstance" ||
                       mNodeInstanceName == "Raw2YuvSelectionInstance" ||
                       mNodeInstanceName == "PortraitRepairInstance";
            } else if (mfnrEnable && !hdrEnable) {
                return mNodeInstanceName == "MfnrInstance0" ||
                       mNodeInstanceName == "Raw2YuvSelectionInstance" ||
                       mNodeInstanceName == "PortraitRepairInstance";
            } else if (mfnrHdrEnable) {
                return mNodeInstanceName == "MfnrInstance0" ||
                       mNodeInstanceName == "Raw2YuvSelectionInstance" ||
                       mNodeInstanceName == "Raw2YuvInstance0" ||
                       mNodeInstanceName == "Raw2YuvInstance1" ||
                       mNodeInstanceName == "PortraitRepairInstance";
            }
            return true;
        }
    }

    if (mOperationMode == StreamConfigModeMiuiZslFront) {
        if (mfnrHdrEnable) {
            return mNodeInstanceName == "MfnrInstance" || mNodeInstanceName == "Raw2YuvInstance0" ||
                   mNodeInstanceName == "Yuv2YuvScaleInstance";
        } else if (mfnrOrAinr) {
            return mNodeInstanceName == "MfnrInstance" ||
                   mNodeInstanceName == "Yuv2YuvScaleInstance";
        }
        return mNodeInstanceName == "Raw2YuvInstance" ||
               mNodeInstanceName == "Yuv2YuvScaleInstance";
    }

    if (mOperationMode == StreamConfigModeMiuiManual ||
        (mOperationMode == StreamConfigModeMiuiSuperNight && mCameraMode == CAM_COMBMODE_FRONT)) {
        if (mfnrOrAinr) {
            return mNodeInstanceName == "MfnrInstance";
        }
        if (rawSnEnable) {
            return mNodeInstanceName == "Raw2YuvInstance";
        }
        return mNodeInstanceName == "Yuv2YuvScaleInstance";
    }

    if (mOperationMode == StreamConfigModeUltraPixelPhotography) {
        if (remosaicEnable) {
            return mNodeInstanceName == "Raw2YuvInstance";
        } else if (mfnrOrAinr) {
            return mNodeInstanceName == "MfnrInstance";
        }
        return mNodeInstanceName == "Raw2YuvInstance";
    }

    // TODO:
    return true;
}
