#include "AnchorsyncPlugin.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <vector>

#include "string.h"
using namespace mialgo2;

AnchorsyncPlugin::~AnchorsyncPlugin()
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);
}

int AnchorsyncPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    m_interface = nodeInterface;
    m_pSharpnessMiAlgo = nullptr;
    m_dumpData = property_get_int32("persist.vendor.algoengine.anchor.dump", 0);
    m_dumpFaceInfo = property_get_int32("persist.vendor.algoengine.anchor.dumpFaceInfo", 0);

    return 0;
}

int AnchorsyncPlugin::preProcess(PreProcessInfo preProcessInfo)
{
    if (m_pSharpnessMiAlgo == nullptr) {
        m_pSharpnessMiAlgo = SharpnessMiAlgo::GetInstance();
        m_pSharpnessMiAlgo->AddRef();
        MLOGD(Mia2LogGroupPlugin, "get Algo point");
    }
    return 0;
}

bool isSupperNightBokehEnabled(camera_metadata_t *meta)
{
    uint8_t superNightBokehEnabled = 0;
    void *pData = VendorMetadataParser::getTag(meta, "xiaomi.bokeh.superNightEnabled");
    if (NULL != pData) {
        superNightBokehEnabled = *static_cast<uint8_t *>(pData);
    }
    return superNightBokehEnabled;
}

bool isMiviRawSrEnabled(camera_metadata_t *meta)
{
    uint8_t isSrEnable = 0;
    uint8_t isMiviRawCb = 0;

    void *pData = VendorMetadataParser::getTag(meta, "xiaomi.superResolution.enabled");
    if (NULL != pData) {
        isSrEnable = *static_cast<uint8_t *>(pData);
    }

    pData = NULL;
    pData = VendorMetadataParser::getTag(meta, "com.xiaomi.mivi2.rawzsl.enable");
    if (NULL != pData) {
        isMiviRawCb = *static_cast<uint8_t *>(pData);
    }

    MLOGI(Mia2LogGroupPlugin, "isSrEnable: %d isMiviRawCb: %d", isSrEnable, isMiviRawCb);

    return isSrEnable && isMiviRawCb;
}

bool AnchorsyncPlugin::isEnabled(MiaParams settings)
{
    bool anchorEnable = false;
    uint8_t mfnrTagValue = 0;
    VOID *pData = NULL;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.mfnr.enabled",
                                       &pData); // todo make a newtag or use mfnrtag
    if (pData) {
        mfnrTagValue = *static_cast<uint8_t *>(pData);
    }

    pData = NULL;
    const char *hdrEnablePtr = "xiaomi.hdr.enabled";
    VendorMetadataParser::getVTagValue(settings.metadata, hdrEnablePtr, &pData);

    // NOTE: disable chioffline mfnr plugins when mfnr+hdr snapshot
    bool hdrEnable = false;
    if (NULL != pData) {
        hdrEnable = *static_cast<uint8_t *>(pData);
    }
    if (!hdrEnable && (mfnrTagValue == 2)) {
        anchorEnable = true;
    }

    if (isSupperNightBokehEnabled(settings.metadata)) {
        anchorEnable = true;
    }

    if (isMiviRawSrEnabled(settings.metadata)) {
        anchorEnable = true;
    }

    return anchorEnable;
}

ProcessRetStatus AnchorsyncPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    return PROCSUCCESS;
}

struct MotionVelocity
{
    float velocityStaticRatio;
    float velocitySlowRatio;
    float velocityMiddleRatio;
    float velocityFastRatio;
    float velocityMax;
    float velocityMean;
};
#define MOTION_VELOCITY_LATENCY_CONTROL_THRESHOLD 35
#define TOLERANCE_THRESHOLD_RATE                  0.2
void AnchorsyncPlugin::AnchorPickLatencyBased(AnchorParamInfo *pAnchorParamInfo)
{
    UINT64 minLatency = ~0;
    INT32 minLatencyIndex = 0;
    int32_t numOfSequence = pAnchorParamInfo->inputNum;
    uint64_t sensorTimeStamp = 0;
    uint64_t anchorTimeStamp = 0;
    std::vector<int> factorVect;
    std::vector<int> factorStatistics;
    UINT32 motionCaptureType = 0;

    AnchorPickSharpnessBased(pAnchorParamInfo);

    std::vector<ImageParams> &imageparam = *(pAnchorParamInfo->pBufferInfo);

    // 3. Get the frame Id whose sensorTimeStamp is closest to anchorTimeStamp.
    for (int i = 0; i < pAnchorParamInfo->inputNum; i++) {
        camera_metadata_t *pMetadata = imageparam[i].pMetadata;
        void *pData;
        if (i == 0) {
            VendorMetadataParser::getVTagValue((camera_metadata_t *)pMetadata,
                                               "com.xiaomi.mivi2.preferredAnchorTime", &pData);
            if (NULL != pData) {
                uint64_t *pTimeStamp = static_cast<uint64_t *>(pData);
                anchorTimeStamp = *pTimeStamp;
                MLOGD(Mia2LogGroupPlugin, "anchorTimestamp %" PRIu64, anchorTimeStamp);
            }
        }

        VendorMetadataParser::getVTagValue((camera_metadata_t *)pMetadata,
                                           "xiaomi.ai.misd.motionCaptureType", &pData);
        if (NULL != pData) {
            uint32_t *pMotionCaptureType = static_cast<uint32_t *>(pData);
            motionCaptureType = *pMotionCaptureType;
            MLOGD(Mia2LogGroupPlugin, "motionCaptureType %x", *pMotionCaptureType);
        }

        VendorMetadataParser::getTagValue((camera_metadata_t *)pMetadata, ANDROID_SENSOR_TIMESTAMP,
                                          &pData);
        if (NULL != pData) {
            uint64_t *ptimestamp = static_cast<uint64_t *>(pData);
            sensorTimeStamp = *ptimestamp;
            MLOGD(Mia2LogGroupPlugin, "sensorTimestamp %" PRIu64, *ptimestamp);
        }

        factorVect.push_back((0x7f00 & motionCaptureType) >> 8);

        UINT64 latency = (sensorTimeStamp > anchorTimeStamp) ? (sensorTimeStamp - anchorTimeStamp)
                                                             : (anchorTimeStamp - sensorTimeStamp);

        if (latency < minLatency) {
            minLatencyIndex = i;
            minLatency = latency;
        }
        MLOGD(Mia2LogGroupPlugin,
              "anchorTimeStamp:%ld, sensorTimeStamp:%ld, minLatency:%ld, minLatencyIndex:%d",
              anchorTimeStamp, sensorTimeStamp, minLatency, minLatencyIndex);
    }

    MLOGD(Mia2LogGroupPlugin, "origin minLatencyIndex: %d", minLatencyIndex);
    // 4. update the minLatencyIndex according to the buffers' factor info
    // 4.1 统计下factor的信息
    if (numOfSequence == factorVect.size()) {
        int factorSeriers = -1;
        int tempFactor = 0;
        for (auto factor : factorVect) {
            if ((abs(tempFactor - factor) < tempFactor * TOLERANCE_THRESHOLD_RATE)) {
                if (-1 != factorSeriers)
                    factorStatistics[factorSeriers]++;
            } else {
                factorSeriers++;
                factorStatistics.push_back(1);
                tempFactor = factor;
            }
        }
    }
    if (0 < factorStatistics.size()) {
        // 4.2 获取factor相同或相近的最长的段的脚标
        int maxFactorSection = 0;
        for (int i = 0; i < factorStatistics.size(); i++) {
            if (factorStatistics[maxFactorSection] < factorStatistics[i])
                maxFactorSection = i;
        }
        // 4.3 获取最长factor段的最小，最大，和中间的Index值（脚标）
        int maxFactorSecMidIndex = 0;
        int maxFactorSecMinIndex = 0;
        int maxFactorSecMaxIndex = 0;
        for (int j = 0; j < maxFactorSection; j++)
            maxFactorSecMinIndex += factorStatistics[j];
        maxFactorSecMidIndex = maxFactorSecMinIndex + (factorStatistics[maxFactorSection] - 1) / 2;
        maxFactorSecMaxIndex = maxFactorSecMinIndex + factorStatistics[maxFactorSection] - 1;

        MLOGD(
            Mia2LogGroupPlugin,
            "maxFactorSection:%d, maxFactorSecMinID:%d, maxFactorSecMidID:%d, maxFactorSecMaxID:%d",
            maxFactorSection, maxFactorSecMinIndex, maxFactorSecMidIndex, maxFactorSecMaxIndex);

        // 4.4 update minLatencyIndex if need
        //原始基准帧不在最长序列中，直接换
        if ((maxFactorSecMinIndex > minLatencyIndex) || (maxFactorSecMaxIndex < minLatencyIndex)) {
            minLatencyIndex = maxFactorSecMidIndex;
        } else {
            if (factorStatistics[maxFactorSection] >= 3) {
                //取满足要求的最靠近原来基准帧位置的帧
                if (maxFactorSecMinIndex == minLatencyIndex)
                    minLatencyIndex = maxFactorSecMinIndex + 1;
                else if (maxFactorSecMaxIndex == minLatencyIndex)
                    minLatencyIndex = maxFactorSecMaxIndex - 1;
            }
        }
        MLOGD(Mia2LogGroupPlugin, "updated minLatencyIndex: %d", minLatencyIndex);
        minLatencyIndex = minLatencyIndex < 0 ? 0 : minLatencyIndex;
        minLatencyIndex = minLatencyIndex >= numOfSequence ? numOfSequence - 1 : minLatencyIndex;
    }

    // 5. Pick up the sharpest frame between minLatencyIndeyx-1, minLatencyIndeyx,
    // minLatencyIndeyx+1
    //   As outputOrder is already sorted, so just pick up the first one being iterated.
    //   Then update the minLatencyIndey as the final anchor frame index.
    for (int i = 0; i < numOfSequence; i++) {
        MLOGD(Mia2LogGroupPlugin, "outputOrder:%d, index:%d", pAnchorParamInfo->outputOrder[i], i);
        if ((pAnchorParamInfo->outputOrder[i] == (minLatencyIndex > 0 ? minLatencyIndex - 1 : 0)) ||
            (pAnchorParamInfo->outputOrder[i] == minLatencyIndex) ||
            (pAnchorParamInfo->outputOrder[i] == (minLatencyIndex + 1))) {
            minLatencyIndex = pAnchorParamInfo->outputOrder[i];
            break;
        }
    }

    // 6. Update the order.
    //   Put the anchor frame Id to the top. And sort the rest frames by sharpness.
    for (int i = 0; i < numOfSequence; i++) {
        MLOGD(Mia2LogGroupPlugin, "outputOrder:%d, index:%d", pAnchorParamInfo->outputOrder[i], i);
        if (pAnchorParamInfo->outputOrder[i] == minLatencyIndex) {
            if (0 != i) {
                for (int j = i; j > 0; j--) {
                    pAnchorParamInfo->outputOrder[j] = pAnchorParamInfo->outputOrder[j - 1];
                }
            }
            pAnchorParamInfo->outputOrder[0] = minLatencyIndex;
            break;
        }
    }
    return;
}

static const UINT32 NumIHistBins = 256;
typedef struct
{
    UINT16 YCCHistogram[NumIHistBins];   ///< Array containing the either the Y, Cb or Cr histogram
                                         ///< values
    UINT16 greenHistogram[NumIHistBins]; ///< Array containing the green histogram values
    UINT16 blueHistogram[NumIHistBins];  ///< Array containing the blue histogram values
    UINT16 redHistogram[NumIHistBins];   ///< Array containing the red histogram values
} IHistStatsOutput;

typedef struct
{
    IHistStatsOutput imageHistogram; ///< Image histogram statistics data
    UINT32 numBins;                  ///< Number of bins per channel
} ParsedIHistStatsOutput;

void AnchorsyncPlugin::AnchorPickSharpnessAndLuma(AnchorParamInfo *pAnchorParamInfo)
{
    std::vector<ImageParams> &imageparam = *(pAnchorParamInfo->pBufferInfo);
    void *pData;
    std::vector<float> &lumas = pAnchorParamInfo->luma;
    for (int i = 0; i < pAnchorParamInfo->inputNum; i++) {
        camera_metadata_t *pMetadata = imageparam[i].pMetadata;
        VendorMetadataParser::getTagValue((camera_metadata_t *)pMetadata, ANDROID_SENSOR_TIMESTAMP,
                                          &pData);
        if (NULL != pData) {
            ParsedIHistStatsOutput *phist = static_cast<ParsedIHistStatsOutput *>(pData);
            INT32 lumaSum = 0;
            INT32 count = 0;
            float luma = 0;
            for (int j = 0; j < NumIHistBins; j++) {
                lumaSum += phist->imageHistogram.YCCHistogram[j] * j;
                count += phist->imageHistogram.YCCHistogram[j];
            }
            luma = (float)lumaSum / count;
            MLOGD(Mia2LogGroupPlugin, "luma %d, %f", i, luma);
            lumas.push_back(luma);
        }
    }

    LumaScreen screen;
    std::vector<int> indexs = screen(lumas);
    std::vector<int> &groupId = pAnchorParamInfo->groupId;
    groupId = std::vector<int>(pAnchorParamInfo->inputNum, 0);
    std::for_each(indexs.begin(), indexs.end(), [&](int i) { groupId[i] = 1; });
    AnchorPickSharpnessBased(pAnchorParamInfo);

    static int dump = property_get_int32("persist.vendor.camera.anchorlumadump", 0);
    ;
    if (dump) {
        const char *path = "/data/vendor/camera";
        FILE *fp = nullptr;
        char timeBuf[128];
        time_t current_time;
        struct tm *timeinfo;
        time(&current_time);
        timeinfo = localtime(&current_time);
        strftime(timeBuf, sizeof(timeBuf), "%Y%m%d%H%M%S", timeinfo);
        char imageName[256];
        sprintf(imageName, "%s/anchorinfo_%s", path, timeinfo);
        std::ofstream fout;
        fout.open(imageName, std::ios::out);

        std::stringstream ss;
        ss << "outindex,sharpness,lumabyraw,group" << std::endl;
        std::for_each(pAnchorParamInfo->outputOrder.begin(), pAnchorParamInfo->outputOrder.end(),
                      [&](int i) {
                          ss << i << "," << std::fixed << std::setprecision(0)
                             << pAnchorParamInfo->sharpness[i] << "," << std::setprecision(2)
                             << lumas[i] << "," << pAnchorParamInfo->groupId[i] << std::endl;
                      });
        fout << ss.str();
        fout.close();
    }
    return;
}

void AnchorsyncPlugin::AnchorPickSharpnessBased(AnchorParamInfo *pAnchorParamInfo)
{
    std::vector<int> &sharpness = pAnchorParamInfo->sharpness;
    std::vector<int> &groupId = pAnchorParamInfo->groupId;
    std::vector<int> &outputOrder = pAnchorParamInfo->outputOrder;
    if (groupId.size() == 0) {
        for (int i = 0; i < pAnchorParamInfo->inputNum; i++) {
            groupId.push_back(0);
        }
    }
    for (int i = 0; i < pAnchorParamInfo->inputNum; i++) {
        sharpness.push_back(0);
        outputOrder.push_back(i);
    }

    if (nullptr != m_pSharpnessMiAlgo) {
        m_pSharpnessMiAlgo->Calculate(pAnchorParamInfo);
    }

    auto compare = [&](int i1, int i2) {
        if (groupId[i1] != groupId[i2]) {
            return groupId[i1] > groupId[i2];
        } else {
            return sharpness[i1] > sharpness[i2];
        }
    };
    std::sort(pAnchorParamInfo->outputOrder.begin(), pAnchorParamInfo->outputOrder.end(), compare);

    std::stringstream ss;
    ss << "anchorout:" << std::fixed << std::setprecision(0);
    std::for_each(
        pAnchorParamInfo->outputOrder.begin(), pAnchorParamInfo->outputOrder.end(),
        [&](int i) { ss << "[" << i << "|" << groupId[i] << "|" << sharpness[i] << "] "; });
    MLOGD(Mia2LogGroupPlugin, "%s", ss.str().c_str());

    return;
}

void AnchorsyncPlugin::PrepareAnchorInfo(ProcessRequestInfoV2 *pProcessRequestInfo,
                                         AnchorParamInfo *pAnchorParamInfo)
{
    auto &inputBuffers = pProcessRequestInfo->inputBuffersMap.begin()->second;
    int inputNum = inputBuffers.size();
    pAnchorParamInfo->inputNum = inputNum;
    pAnchorParamInfo->pBufferInfo = &inputBuffers;
    pAnchorParamInfo->lumaEnable = true;
    void *pData;
    for (int index = 0; index < inputNum; index++) {
        camera_metadata_t *pMetadata = inputBuffers[index].pMetadata;

        if (index == 0) {
            VendorMetadataParser::getVTagValue((camera_metadata_t *)pMetadata,
                                               "xiaomi.ai.misd.motionCaptureType", &pData);

            if (NULL != pData) {
                uint32_t *pMotionCaptureType = static_cast<uint32_t *>(pData);
                if (0 != (*pMotionCaptureType & 0xff)) {
                    pAnchorParamInfo->motionEnable = TRUE;
                }
                MLOGD(Mia2LogGroupPlugin, "motionCaptureType %x", *pMotionCaptureType);
            }
            if (pAnchorParamInfo->motionEnable == FALSE) {
                VendorMetadataParser::getVTagValue((camera_metadata_t *)pMetadata,
                                                   "xiaomi.ai.misd.motionVelocity", &pData);
                if (NULL != pData) {
                    struct MotionVelocity *pVelocity = static_cast<struct MotionVelocity *>(pData);
                    if (MOTION_VELOCITY_LATENCY_CONTROL_THRESHOLD < pVelocity->velocityMax) {
                        pAnchorParamInfo->motionEnable = TRUE;
                    }
                    MLOGD(Mia2LogGroupPlugin, "MotionVelocity %f", pVelocity->velocityMax);
                }
            }

            // aecLux
            pData = nullptr;
            VendorMetadataParser::getVTagValue(pMetadata, "com.qti.chi.statsaec.AecLux", &pData);
            if (nullptr != pData) {
                pAnchorParamInfo->aecLux = *static_cast<float *>(pData);
                MLOGD(Mia2LogGroupPlugin, "AecLux: %f", pAnchorParamInfo->aecLux);
            } else {
                pAnchorParamInfo->aecLux = 0.0;
                MLOGW(Mia2LogGroupPlugin, "failed load AecLux, force: %f",
                      pAnchorParamInfo->aecLux);
            }

            // crop region
            pData = nullptr;
            VendorMetadataParser::getTagValue(pMetadata, ANDROID_SCALER_CROP_REGION, &pData);
            if (NULL != pData) {
                pAnchorParamInfo->cropRegion = *reinterpret_cast<MiCropRect *>(pData);

                MLOGD(Mia2LogGroupPlugin, "get CropRegion begin [%d, %d, %d, %d]",
                      pAnchorParamInfo->cropRegion.x, pAnchorParamInfo->cropRegion.y,
                      pAnchorParamInfo->cropRegion.width, pAnchorParamInfo->cropRegion.height);
            } else {
                pAnchorParamInfo->cropRegion.x = 0;
                pAnchorParamInfo->cropRegion.y = 0;
                pAnchorParamInfo->cropRegion.width = 0;
                pAnchorParamInfo->cropRegion.height = 0;
                MLOGW(Mia2LogGroupPlugin, "failed load CropRegion, force:[%d, %d, %d, %d]",
                      pAnchorParamInfo->cropRegion.x, pAnchorParamInfo->cropRegion.y,
                      pAnchorParamInfo->cropRegion.width, pAnchorParamInfo->cropRegion.height);
            }

            // face rect
            pData = nullptr;
            VendorMetadataParser::getTagValue(pMetadata, ANDROID_STATISTICS_FACE_RECTANGLES,
                                              &pData);
            if (nullptr != pData) {
                camera_metadata_entry_t entry = {0};
                if (0 == find_camera_metadata_entry(pMetadata, ANDROID_STATISTICS_FACE_RECTANGLES,
                                                    &entry)) {
                    uint32_t numElemRect = sizeof(CHIRECT) / sizeof(uint32_t);
                    pAnchorParamInfo->faceInfo.faceNum = entry.count / numElemRect;
                    pAnchorParamInfo->faceInfo.faceRectInfo =
                        *reinterpret_cast<MiFaceRect *>(pData);
                    MLOGD(Mia2LogGroupPlugin, "faceNum: %f, faceRectInfo begin [%d, %d, %d, %d]",
                          pAnchorParamInfo->faceInfo.faceNum,
                          pAnchorParamInfo->faceInfo.faceRectInfo.x,
                          pAnchorParamInfo->faceInfo.faceRectInfo.y,
                          pAnchorParamInfo->faceInfo.faceRectInfo.width,
                          pAnchorParamInfo->faceInfo.faceRectInfo.height);
                }
            }

            // bayerPattern
            int32_t cameraId = 0;
            pData = nullptr;
            VendorMetadataParser::getVTagValue(pMetadata, "xiaomi.snapshot.fwkCameraId", &pData);
            if (nullptr != pData) {
                cameraId = *static_cast<int32_t *>(pData);
            }

            pData = nullptr;
            camera_metadata_t *staticMeta =
                StaticMetadataWraper::getInstance()->getStaticMetadata(cameraId);
            VendorMetadataParser::getTagValue(staticMeta,
                                              ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT, &pData);
            if (nullptr != pData) {
                pAnchorParamInfo->bayerPattern = *static_cast<int32_t *>(pData);
                MLOGD(Mia2LogGroupPlugin, "bayerPattern: %d", pAnchorParamInfo->bayerPattern);
            }

            // qcfa sensor
            pData = nullptr;
            VendorMetadataParser::getVTagValue(
                staticMeta, "org.codeaurora.qcamera3.quadra_cfa.is_qcfa_sensor", &pData);
            if (nullptr != pData) {
                pAnchorParamInfo->isQuadCFASensor = *static_cast<bool *>(pData);
                MLOGD(Mia2LogGroupPlugin, "isQuadCFASensor: %d", pAnchorParamInfo->isQuadCFASensor);
            } else {
                MLOGW(Mia2LogGroupPlugin, "get is_qcfa_sensor failed !!");
            }
        }
    }
}

bool isInputBufNeeded(camera_metadata_t *metadata, int bufferCount, int currentIndex)
{
    bool isNeeded = true;
    bool superNightBokehEnabled = isSupperNightBokehEnabled(metadata);
    if (superNightBokehEnabled) {
        int32_t ev = 0;
        void *pData =
            VendorMetadataParser::getTag(metadata, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
        if (NULL != pData) {
            ev = *static_cast<int32_t *>(pData);
        }
        // aux use b2y plugin when sebokeh
        if (bufferCount < 3 || currentIndex == 2) {
            isNeeded = true;
        } else {
            isNeeded = false;
        }
        // aux use mfsr plugin when sebokeh
        // if (ev != 0) {
        //     isNeeded = false;
        // }
    }
    return isNeeded;
}

ProcessRetStatus AnchorsyncPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    AnchorParamInfo anchorParamInfo{};
    auto &inputBuffers = pProcessRequestInfo->inputBuffersMap.begin()->second;
    int inputNum = inputBuffers.size();
    if (inputNum > MIALGO_RFS_MAX_FRAME_NUM) {
        inputNum = MIALGO_RFS_MAX_FRAME_NUM;
        MLOGW(Mia2LogGroupPlugin, "anchor input frame nums force set:%d", inputNum);
    }
    anchorParamInfo.inputNum = inputNum;
    PrepareAnchorInfo(pProcessRequestInfo, &anchorParamInfo);

    double CalculateStartTime = PluginUtils::nowMSec();
    if (anchorParamInfo.motionEnable == true) {
        AnchorPickLatencyBased(&anchorParamInfo);
    } else if (anchorParamInfo.lumaEnable == true) {
        AnchorPickSharpnessAndLuma(&anchorParamInfo);
    } else {
        AnchorPickSharpnessBased(&anchorParamInfo);
    }
    MLOGI(Mia2LogGroupPlugin, "anchor Calculate costTime=%.2f inputnum",
          PluginUtils::nowMSec() - CalculateStartTime, inputNum);

    camera_metadata_t *pMetaData = inputBuffers[0].pMetadata;
    if (isSupperNightBokehEnabled(pMetaData)) {
        std::vector<int32_t> seBokehAnchorFrames = {};
        for (int index = 0; index < inputBuffers.size(); index++) {
            if (isInputBufNeeded(inputBuffers[index].pMetadata, inputBuffers.size(), index)) {
                seBokehAnchorFrames.push_back(index);
            }
        }
        int seBokehAnchorFramesNum = seBokehAnchorFrames.size();
        int32_t seBokehAnchorFramesResult[seBokehAnchorFramesNum];
        for (int i = 0; i < seBokehAnchorFramesNum; i++) {
            seBokehAnchorFramesResult[i] = seBokehAnchorFrames[i];
        }
        VendorMetadataParser::updateVTagValue(pMetaData, "xiaomi.bokeh.AuxAnchorFrame",
                                              seBokehAnchorFramesResult, seBokehAnchorFramesNum,
                                              NULL);
    } else {
        int32_t outputOrder[inputNum];
        for (int i = 0; i < anchorParamInfo.outputOrder.size(); i++) {
            outputOrder[i] = anchorParamInfo.outputOrder[i];
        }
        VendorMetadataParser::setVTagValue(pMetaData, "com.xiaomi.mivi2.anchorOutputOrder",
                                           outputOrder, inputNum);

        int32_t anchorFrameIndex = outputOrder[0];
        VendorMetadataParser::setVTagValue(pMetaData, "xiaomi.superResolution.pickAnchorIndex",
                                           &anchorFrameIndex, 1);
    }

    //[Quickview] send timestamp back to APP.
    if (m_interface.pSetResultMetadata != NULL) {
        int anchorIndex = anchorParamInfo.outputOrder[0];
        camera_metadata_t *pMetadata = inputBuffers[anchorIndex].pMetadata;

        void *pData = NULL;
        int64_t anchorTimestamp = 0;
        VendorMetadataParser::getTagValue((camera_metadata_t *)pMetadata, ANDROID_SENSOR_TIMESTAMP,
                                          &pData);
        if (NULL != pData) {
            anchorTimestamp = *static_cast<int64_t *>(pData);
        }
        std::string results("quickview");
        MLOGI(Mia2LogGroupPlugin, "send ts %" PRIu64, anchorTimestamp);
        m_interface.pSetResultMetadata(m_interface.owner, pProcessRequestInfo->frameNum,
                                       anchorTimestamp, results);
    }

    if (m_dumpData) {
        for (int i = 0; i < inputNum; i++) {
            auto override_buffer = [&](MiImageBuffer &mFrame, const ImageParams &rFrame) {
                mFrame.format = rFrame.format.format;
                mFrame.width = rFrame.format.width;
                mFrame.height = rFrame.format.height;
                mFrame.plane[0] = rFrame.pAddr[0];
                mFrame.fd[0] = rFrame.fd[0];
                mFrame.stride = rFrame.planeStride;
                mFrame.scanline = rFrame.sliceheight;
                mFrame.numberOfPlanes = rFrame.numberOfPlanes;
            };
            MiImageBuffer inputFrame{};
            override_buffer(inputFrame, inputBuffers[i]);
            char inputbuf[128];
            snprintf(inputbuf, sizeof(inputbuf), "anchor_input_[%d]_%dx%d", i, inputFrame.width,
                     inputFrame.height);

            PluginUtils::dumpToFile(inputbuf, &inputFrame);
        }
    }

    if (m_dumpFaceInfo) {
        DumpFaceInfo(&anchorParamInfo);
    }

    return PROCSUCCESS;
}

int AnchorsyncPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

void AnchorsyncPlugin::destroy()
{
    if (nullptr != m_pSharpnessMiAlgo) {
        MI_S32 ret = MIALGO_OK;
        ret = m_pSharpnessMiAlgo->ReleaseRef();
        if (MIALGO_OK == ret) {
            MLOGD(Mia2LogGroupPlugin, "Release Ref lib succeed");
        } else {
            MLOGW(Mia2LogGroupPlugin, "Release Ref lib failed");
        }
        m_pSharpnessMiAlgo = nullptr;
    }
}

void AnchorsyncPlugin::DumpFaceInfo(AnchorParamInfo *pAnchorParamInfo) const
{
    std::vector<ImageParams> &imageparam = *pAnchorParamInfo->pBufferInfo;
    int scale = pAnchorParamInfo->isQuadCFASensor ? 2 : 1;
    char str[256] = {0};
    const char *path = "/data/vendor/camera";
    FILE *fp = nullptr;
    char timeBuf[128];
    time_t current_time;
    struct tm *timeinfo;
    time(&current_time);
    timeinfo = localtime(&current_time);
    strftime(timeBuf, sizeof(timeBuf), "%Y%m%d%H%M%S", timeinfo);
    for (int i = 0; i < pAnchorParamInfo->inputNum; i++) {
        camera_metadata_t *pMetaData = imageparam[i].pMetadata;
        if (nullptr != pMetaData) {
            sprintf(str, "%s/IMG_w%d_h%d_s[%d].txt", path, imageparam[i].format.width,
                    imageparam[i].format.height, i);
            fp = fopen(str, "wb");
            if (nullptr == fp) {
                MLOGI(Mia2LogGroupPlugin, "Open image %s return error !\n", str);
            } else {
                void *pData;
                // face rect
                pData = nullptr;
                VendorMetadataParser::getTagValue(pMetaData, ANDROID_STATISTICS_FACE_RECTANGLES,
                                                  &pData);
                if (nullptr != pData) {
                    camera_metadata_entry_t entry = {0};
                    if (0 == find_camera_metadata_entry(
                                 pMetaData, ANDROID_STATISTICS_FACE_RECTANGLES, &entry)) {
                        void *pTmpData = nullptr;
                        VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.device.orientation",
                                                           &pTmpData);
                        int *pAngle = static_cast<int *>(pTmpData);
                        int orientation = 0;
                        if (nullptr != pAngle) {
                            orientation = *pAngle;
                        }
                        uint32_t numElemRect = sizeof(CHIRECT) / sizeof(uint32_t);
                        int numFaces = entry.count / numElemRect;
                        MiFaceRect faceRectnfo = *reinterpret_cast<MiFaceRect *>(pData);
                        fprintf(fp, "Face:%d", numFaces);
                        if (numFaces > 0) {
                            int x, y, h, w;
                            for (int i = 0; i < numFaces; i++) {
                                x = faceRectnfo.x / scale;
                                y = faceRectnfo.y / scale;
                                int x2 = faceRectnfo.width / scale;
                                int y2 = faceRectnfo.height / scale;
                                w = x2 - x + 1;
                                h = y2 - y + 1;
                                fprintf(fp, ",face%d(%d,%d,%d,%d)", i, x, y, w, h);
                                fprintf(fp, ",orientation(%d)", orientation);
                            }
                        }
                    }
                }
                fclose(fp);
            }
        }
    }
}
