#include "AlgoEngine.h"

#include <MiaPostProcIntf.h>
#include <xiaomi/CameraPlatformInfoXiaomi.h>
#include <xiaomi/MiCommonTypes.h>

#include <queue>

#include "MiAlgoCamMode.h"
#include "MiBufferManager.h"
#include "MiCamTrace.h"
#include "VendorMetadataParser.h"
#include "mtkcam-halif/utils/metadata_tag/1.x/custom/custom_metadata_tag.h"
#include "utils.h"

using namespace midebug;

namespace mizone {

class BufferCallback : public MiaFrameCb
{
public:
    BufferCallback(AlgoEngine *algoEngine) : mEngine(algoEngine) {}
    void release(uint32_t frame, int64_t timeStampUs, MiaFrameType type, uint32_t portIdx,
                 buffer_handle_t handle) override;
    int getBuffer(int64_t timeStampUs, OutSurfaceType type, buffer_handle_t *buffer) override;

private:
    int outputBufferErrorHandler(int64_t timeStamp);
    AlgoEngine *mEngine;
};

class PostProcCallback : public MiaPostProcSessionCb
{
public:
    PostProcCallback(AlgoEngine *algoEngine) : mEngine(algoEngine) {}
    void onSessionCallback(MiaResultCbType type, uint32_t frame, int64_t timeStamp,
                           void *msgData) override;

private:
    AlgoEngine *mEngine;
};

int BufferCallback::outputBufferErrorHandler(int64_t timeStamp)
{
    MLOGE(LogGroupHAL, " nothing!");
    return 0;
}

void BufferCallback::release(uint32_t frame, int64_t timeStampUs, MiaFrameType type,
                             uint32_t portIdx, buffer_handle_t handle)
{
    MLOGI(LogGroupHAL, "+");
    MLOGD(LogGroupHAL, "frameNum = %d, timeStamp = %ld, type = %d, portIdx = %d, buffer = %p",
          frame, timeStampUs, type, portIdx, handle);

    if (type == MiaFrameType::MI_FRAME_TYPE_OUTPUT) {
        // note that the output buffer release, means the process task is completed
        mEngine->releaseOutputBuffer(frame, timeStampUs, portIdx);
    } else {
        MLOGI(LogGroupHAL, "release the input buffer %d:%d", frame, portIdx);
        mEngine->releaseInputBuffer(frame, timeStampUs, portIdx);
    }
    MLOGI(LogGroupHAL, "-");
}

int BufferCallback::getBuffer(int64_t timeStampUs, OutSurfaceType type, buffer_handle_t *buffer)
{
    MLOGI(LogGroupHAL, "+");

    mEngine->getOutputBuffer(timeStampUs, static_cast<int32_t>(type), *buffer);

    MLOGI(LogGroupHAL, "-");
    return 0;
}

void PostProcCallback::onSessionCallback(MiaResultCbType type, uint32_t frame, int64_t timeStamp,
                                         void *msgData)
{
    MLOGI(LogGroupHAL, "+");
    MLOGD(LogGroupHAL, "type = %d, frameNum = %d, timeStamp = %ld", type, frame, timeStamp);
    // call back to MiCamMode::onReprocessCompleted(uint32_t frameNum, const std::string &exifInfo);

    if (type == MiaResultCbType::MIA_META_CALLBACK) {
        std::vector<std::string> &rawResult = *static_cast<std::vector<std::string> *>(msgData);
        if (rawResult.empty()) {
            MLOGW(LogGroupHAL, "rawResult is empty");
            return;
        }

        std::string resultValue = "MiviFwkResult:{";
        for (int i = 0; i < rawResult.size(); i++) {
            resultValue += "";
            resultValue += rawResult[i];
        }
        resultValue += "}";
        MLOGD(LogGroupHAL, "frame: %d timeStamp: %" PRId64 ", resultValue: %s", frame, timeStamp,
              resultValue.c_str());

        mEngine->onMetedataRecieved(frame, resultValue);
    }
    MLOGI(LogGroupHAL, "-");
}

std::unique_ptr<AlgoEngine> AlgoEngine::create(const CreateInfo &info)
{
    if (info.mode == nullptr) {
        MLOGE(LogGroupHAL, "info.mode is nullptr");
        return nullptr;
    }
    return std::unique_ptr<AlgoEngine>(new AlgoEngine(info));
}

AlgoEngine::AlgoEngine(const CreateInfo &info)
    : mMode(info.mode),
      mCameraInfo(info.cameraInfo),
      mAlgoEngineSession(nullptr),
      mBufferManager(info.mode->getBufferManager()),
      mConfigCompleted(false)
{
    MLOGI(LogGroupHAL, "ctor +");
    mPostProcCallback = std::make_unique<PostProcCallback>(this);
    mBufferCallback = std::make_unique<BufferCallback>(this);
    camera_metadata_t *staticMetadata = nullptr;
    DecoupleUtil::convertMetadata(info.cameraInfo.staticInfo, staticMetadata);
    StaticMetadataWraper::getInstance()->setStaticMetadata(staticMetadata,
                                                           info.cameraInfo.cameraId);
    MLOGI(LogGroupHAL, "ctor -");
}

AlgoEngine::~AlgoEngine()
{
    MLOGI(LogGroupHAL, "dtor +");
    if (mAlgoEngineSession != nullptr) {
        MiaPostProc_Destroy(mAlgoEngineSession);
        mAlgoEngineSession = nullptr;
    }
    MLOGI(LogGroupHAL, "dtor -");
}

bool AlgoEngine::configure(std::shared_ptr<MiConfig> fwkConfig, std::shared_ptr<MiConfig> vndConfig)
{
    MLOGI(LogGroupHAL, "+");
    MICAM_TRACE_SYNC_BEGIN(MialgoTraceCapture, "AlgoEngine::configure");

    if (fwkConfig == nullptr || vndConfig == nullptr) {
        MLOGE(LogGroupHAL, "configure failed: fwkConfig(%p) or vndConfig(%p) is nullptr",
              fwkConfig.get(), vndConfig.get());
        return false;
    }

    if (mAlgoEngineSession != nullptr) {
        MLOGE(LogGroupHAL, "mAlgoEngineSession (%p) is not nullptr", mAlgoEngineSession);
        MiaPostProc_Destroy(mAlgoEngineSession);
        mAlgoEngineSession = nullptr;
    }

    mFwkConfig = fwkConfig;
    mVndConfig = vndConfig;

    mSessionParams = mVndConfig->rawConfig.sessionParams;
    // for postproc to get cameraId
    MiMetadata::setEntry(&mSessionParams, MTK_CONFIGURE_ISP_CAMERA_ID, mCameraInfo.cameraId);

    MiaCreateParams createParams;
    createParams.processMode = MiaPostProcMode::OfflineSnapshot;

    createParams.cameraMode = getCameraMode();
    createParams.operationMode = getOperationMode(createParams.cameraMode);

    for (auto &&[streamId, stream] : mVndConfig->streams) {
        // only snapshot stream will send to algoengine for reprocess
        if (stream->usageType != StreamUsageType::SnapshotStream) {
            continue;
        }

        createParams.inputFormat.emplace_back();
        auto &format = createParams.inputFormat.back();
        MiaPixelFormat tmpFormat;
        DecoupleUtil::convertFormat(stream->rawStream.format, tmpFormat);
        format.format = tmpFormat;
        format.width = stream->rawStream.width;
        format.height = stream->rawStream.height;
    }
    for (auto &&[streamId, stream] : mFwkConfig->streams) {
        // only snapshot stream will send to algoengine for reprocess
        if (stream->usageType == StreamUsageType::SnapshotStream) {
            createParams.outputFormat.emplace_back();
            auto &format = createParams.outputFormat.back();
            format.format = stream->rawStream.format;
            format.width = stream->rawStream.width;
            format.height = stream->rawStream.height;
        }
    }

    if (createParams.inputFormat.empty() || createParams.outputFormat.empty()) {
        MLOGE(LogGroupHAL, "inputFormat or outputFormat is empty, abort creating mialgoengine");
        MLOGI(LogGroupHAL, "-");
        return true;
    }

    createParams.sessionCb = mPostProcCallback.get();
    createParams.frameCb = mBufferCallback.get();
    createParams.logicalCameraId = mCameraInfo.cameraId;

    mAlgoEngineSession = MiaPostProc_Create(&createParams);
    if (mAlgoEngineSession == nullptr) {
        MLOGE(LogGroupHAL, "mAlgoEngineSession create failed!");
        return false;
    }
    mConfigCompleted = true;
    MICAM_TRACE_SYNC_END(MialgoTraceCapture);
    MLOGI(LogGroupHAL, "-");
    return true;
}

uint32_t AlgoEngine::getOperationMode(const uint32_t cameraMode)
{
    // NOTE: For now, some operationMode configured from MiuiCamera with value from 0x9000 to
    //       0x9008, which are for Algo Up, and not defined in mialgoengine.
    //       These Algo up operationMode may not need now, however, for compatiblity,
    //       we just convert them here now.

    // TODO: need full implenmentation.

    MLOGD(LogGroupHAL, "before convert: operationMode = 0x%x", mFwkConfig->rawConfig.operationMode);

    uint32_t operationMode = mialgo2::StreamConfigModeNormal;

    switch (static_cast<uint32_t>(mFwkConfig->rawConfig.operationMode)) {
    case ::StreamConfigModeAlgoupDualBokeh:
    case ::StreamConfigModeAlgoupSingleBokeh:
        operationMode = mialgo2::StreamConfigModeBokeh;
        break;
    case ::StreamConfigModeAlgoupNormal:
        if (cameraMode == CAM_COMBMODE_FRONT) {
            operationMode = mialgo2::StreamConfigModeMiuiZslFront;
        } else {
            operationMode = mialgo2::StreamConfigModeSAT;
        }
        break;
    case ::StreamConfigModeMiuiSuperNight:
        operationMode = mialgo2::StreamConfigModeMiuiSuperNight;
        break;
    case ::StreamConfigModeAlgoupManual:
        operationMode = mialgo2::StreamConfigModeMiuiManual;
        break;
    case ::StreamConfigModeAlgoupHD:
    case ::StreamConfigModeAlgoupManualUltraPixel:
        operationMode = mialgo2::StreamConfigModeUltraPixelPhotography;
        break;
    default:
        operationMode = mialgo2::StreamConfigModeNormal;
    }

    MLOGD(LogGroupHAL, "after convert: operationMode = 0x%x", operationMode);

    return operationMode;
}

uint32_t AlgoEngine::getCameraMode()
{
    MLOGD(LogGroupHAL, "roleId = %d", mCameraInfo.roleId);

    uint32_t cameraMode = CAM_COMBMODE_UNKNOWN;

    switch (mCameraInfo.roleId) {
    case RoleIdRearWide:
        cameraMode = CAM_COMBMODE_REAR_WIDE;
        break;
    case RoleIdFront:
        cameraMode = CAM_COMBMODE_FRONT;
        break;
    case RoleIdRearTele:
    case RoleIdRearTele4x:
        cameraMode = CAM_COMBMODE_REAR_TELE;
        break;
    case RoleIdRearUltra:
        cameraMode = CAM_COMBMODE_REAR_ULTRA;
        break;
    case RoleIdRearMacro:
        cameraMode = CAM_COMBMODE_REAR_MACRO;
        break;
    case RoleIdFrontAux:
        cameraMode = CAM_COMBMODE_FRONT_AUX;
        break;
    case RoleIdRearSat:
    case RoleIdRearSatUWT:
    case RoleIdRearSatUWTT4x:
    case RoleIdParallelVt:
        cameraMode = CAM_COMBMODE_REAR_SAT_WU;
        break;
    case RoleIdRearBokeh:
    case RoleIdRearBokeh2:
    case RoleIdRearBokeh3:
        cameraMode = CAM_COMBMODE_REAR_BOKEH_WU;
        break;
    default:
        MLOGW(LogGroupHAL, "roleId %d is not supported", mCameraInfo.roleId);
        cameraMode = CAM_COMBMODE_UNKNOWN;
        break;
    }

    MLOGD(LogGroupHAL, "cameraMode = 0x%x", cameraMode);

    return cameraMode;
}

int AlgoEngine::process(const CaptureRequest &fwkReq,
                        const std::map<uint32_t, std::shared_ptr<MiRequest>> &miRequests)
{
    MLOGI(LogGroupHAL, "+");
    CDKResult status = MIAS_OK;
    PortsRule portsRule = Normal;
    uint8_t hdrEnable = 0;
    uint8_t mfnrEnable = 0;
    uint8_t hdrsrEnable = 0;

    MiMetadata::getEntry<MUINT8>(&miRequests.begin()->second->result, XIAOMI_HDR_FEATURE_ENABLED,
                                 hdrEnable);
    MiMetadata::getEntry<MUINT8>(&miRequests.begin()->second->result, XIAOMI_MFNR_ENABLED,
                                 mfnrEnable);
    MiMetadata::getEntry<MUINT8>(&miRequests.begin()->second->result, XIAOMI_HDR_SR_FEATURE_ENABLED,
                                 hdrsrEnable);
    if (mfnrEnable && hdrEnable &&
        static_cast<uint32_t>(mFwkConfig->rawConfig.operationMode) ==
            ::StreamConfigModeAlgoupNormal) {
        portsRule = MfnrHdr;
    } else if (mfnrEnable && hdrEnable &&
               static_cast<uint32_t>(mFwkConfig->rawConfig.operationMode) ==
                   ::StreamConfigModeAlgoupDualBokeh) {
        portsRule = BokehHdr;
    } else if (hdrsrEnable && static_cast<uint32_t>(mFwkConfig->rawConfig.operationMode) ==
                                  ::StreamConfigModeAlgoupNormal) {
        portsRule = HdrSr;
    }

    // FIX ME: TAG XIAOMI_SNAPSHOT_IMAGENAME need set by MiuiCamera and copy from fwk req here,
    //         check and write here just as a work-around solution
    auto inflightRequest = std::make_shared<InflightRequest>();
    if (fwkReq.frameNumber != 0) {
        auto entry = fwkReq.settings.entryFor(XIAOMI_SNAPSHOT_IMAGENAME);
        if (entry.isEmpty()) {
            std::string imageName("SDK ");
            imageName += std::to_string(fwkReq.frameNumber);
            MiMetadata::MiEntry imageNameEntry;
            imageNameEntry.push_back((uint8_t *)imageName.c_str(), imageName.length() + 1,
                                     Type2Type<uint8_t>());
            const_cast<CaptureRequest &>(fwkReq).settings.update(XIAOMI_SNAPSHOT_IMAGENAME,
                                                                 imageNameEntry);
        }
        mFwkReq = fwkReq;
    }
    inflightRequest->fwkReq = fwkReq;
    inflightRequest->requests = miRequests;

    // prepare inflightRequest;
    prepareInflight(fwkReq, miRequests, portsRule, inflightRequest);

    {
        std::lock_guard<std::mutex> lock(mInflightRequestsMutex);
        int32_t frameNumber = mFwkReq.frameNumber;
        if (mInflightRequests.find(frameNumber) != mInflightRequests.end()) {
            // int the case of frame by frame transmission, expand inflightRequest.
            auto inflightReq = mInflightRequests[frameNumber];

            inflightReq->requests.insert(inflightRequest->requests.begin(),
                                         inflightRequest->requests.end());
            inflightReq->outputBuffers.insert(inflightRequest->outputBuffers.begin(),
                                              inflightRequest->outputBuffers.end());
            inflightReq->tsMapReqs.insert(inflightRequest->tsMapReqs.begin(),
                                          inflightRequest->tsMapReqs.end());
            for (auto &&[portId, buffer] : inflightRequest->inputBuffers) {
                inflightReq->inputBuffers[portId].insert(inflightReq->inputBuffers[portId].end(),
                                                         buffer.begin(), buffer.end());
            }
            mInflightRequests[frameNumber] = inflightReq;

        } else {
            mInflightRequests.emplace(fwkReq.frameNumber, inflightRequest);
        }
    }

    // build and send output:
    for (auto &&portMap : inflightRequest->outputBuffers) {
        uint32_t portId = portMap.first;
        for (auto &&[ts, buffer] : portMap.second) {
            MiaProcessParams sessionParam;
            MiaFrame outFrame;
            sessionParam.outputFrame = &outFrame;
            buildOutputSessionParams(inflightRequest->fwkReq, buffer, portId, sessionParam);
            auto status = MiaPostProc_Process(mAlgoEngineSession, &sessionParam);
            if (status != MIAS_OK) {
                MLOGE(LogGroupHAL, "call MiaPostProc_Process failed: status = %d", status);
                // return status;
            }
        }
    }

    // build and send input:
    std::vector<MiaProcessParams> sesParams;
    std::vector<MiaFrame> inFrames;
    std::vector<std::shared_ptr<DynamicMetadataWraper>> metaWs;
    inFrames.resize(miRequests.size() * 2);
    metaWs.resize(inFrames.size());
    std::vector<MiaFrame>::iterator frameIt = inFrames.begin();
    auto metaW = metaWs.begin();
    for (auto &&portMap : inflightRequest->inputBuffers) {
        uint32_t portId = portMap.first;
        for (auto &&[ts, buffer] : portMap.second) {
            MiaProcessParams sessionParam;
            sessionParam.inputFrame = &*frameIt;
            buildInputSessionParams(mFwkReq, inflightRequest->tsMapReqs[ts], buffer, portId,
                                    sessionParam);
            frameIt++;
            metaW++;
            sesParams.push_back(sessionParam);
        }
    }
    for (auto &&param : sesParams) {
        auto status = MiaPostProc_Process(mAlgoEngineSession, &param);
        if (status != MIAS_OK) {
            MLOGE(LogGroupHAL, "call MiaPostProc_Process failed: status = %d", status);
            return status;
        }
    }

    MLOGI(LogGroupHAL, "-");
    return status;
}

int AlgoEngine::prepareInflight(const CaptureRequest &fwkReq,
                                const std::map<uint32_t, std::shared_ptr<MiRequest>> &miRequests,
                                PortsRule portsRule,
                                std::shared_ptr<InflightRequest> inflightRequest)
{
    int64_t fwkTimestamp = static_cast<int64_t>(miRequests.begin()->second->shutter);
    MiMetadata::setEntry(&inflightRequest->fwkReq.settings, ANDROID_SENSOR_TIMESTAMP,
                         static_cast<int64_t>(fwkTimestamp));
    inflightRequest->timestamp = fwkTimestamp;

    int port = 0;
    auto normalOut = [&] {
        for (auto &&buffer : fwkReq.outputBuffers) {
            const auto &stream = mFwkConfig->streams[buffer.streamId];
            if (stream->usageType == StreamUsageType::SnapshotStream &&
                stream->rawStream.format != EImageFormat::eImgFmt_RAW16) {
                inflightRequest->outputBuffers[port++][fwkTimestamp] = buffer;
            }
        }
        port = 0;
    };

    switch (portsRule) {
    case Normal: {
        // output:
        normalOut();

        // input:
        for (auto &&[vndFrameNum, miReq] : miRequests) {
            int inPortId = 0;
            uint64_t timeStamp = miReq->shutter;
            inflightRequest->tsMapReqs[timeStamp] = miReq;
            for (auto &&buffer : miReq->vndRequest.outputBuffers) {
                if (mVndConfig->streams[buffer.streamId]->usageType ==
                    StreamUsageType::SnapshotStream) {
                    inflightRequest->inputBuffers[inPortId].emplace_back(timeStamp, buffer);
                    inPortId++;
                }
            }
        }

        break;
    }
    case MfnrHdr: {
        uint32_t mfnrNum = 0;
        uint32_t hdrNum = 0;
        for (auto &&[vndFrameNum, miReq] : miRequests) {
            uint8_t mfnrEnable = 0;
            MiMetadata::getEntry<uint8_t>(&miReq->result, XIAOMI_MFNR_ENABLED, mfnrEnable);
            if (mfnrEnable) {
                mfnrNum++;
            } else {
                hdrNum++;
            }
        }
        MLOGD(LogGroupHAL, "MFNR + HDR situation, mfnrNum:%u, hdrNum:%u, totalNum:%lu", mfnrNum,
              hdrNum, miRequests.size());
        // output:
        normalOut();

        // input:
        int inBufferIndex = 1;
        for (auto &&[vndFrameNum, miReq] : miRequests) {
            int inPortId = 0;
            if (inBufferIndex++ > mfnrNum) {
                // frames from HDR should only have one inBuffer;
                // frames from MFNR could have R3 inBuffer;
                inPortId = 2;
            }
            uint64_t timeStamp = miReq->shutter;
            MLOGD(LogGroupHAL, "timeStamp:%lu, miReq:%u.", timeStamp, miReq->frameNum);
            inflightRequest->tsMapReqs[timeStamp] = miReq;
            for (auto &&buffer : miReq->vndRequest.outputBuffers) {
                if (mVndConfig->streams[buffer.streamId]->usageType ==
                    StreamUsageType::SnapshotStream) {
                    inflightRequest->inputBuffers[inPortId].emplace_back(timeStamp, buffer);
                    inPortId++;
                }
            }
        }
        break;
    }
    case BokehHdr: {
        // output:
        normalOut();

        // input:
        int ev0Count = 4;
        int ev_Count = 1;
        if (miRequests.size() != (ev0Count + ev_Count)) {
            MLOGD(LogGroupHAL, "requestCount error! ev0Count:%d, ev_count:%d, allCount:%lu.",
                  ev0Count, ev_Count, miRequests.size());
        }
        int index = 0;
        int portId = 0;
        for (auto &&[vndFrameNum, miReq] : miRequests) {
            if (index++ < ev0Count) {
                portId = 0;
            } else {
                portId = 2;
            }
            inflightRequest->tsMapReqs[miReq->shutter] = miReq;
            for (auto &&buffer : miReq->vndRequest.outputBuffers) {
                if (mVndConfig->streams[buffer.streamId]->usageType ==
                    StreamUsageType::SnapshotStream) {
                    inflightRequest->inputBuffers[portId].emplace_back(miReq->shutter, buffer);
                    portId++;
                }
            }
        }
        break;
    }
    case HdrSr: {
        uint32_t srNum = 0;
        uint32_t hdrNum = 0;
        for (auto &&[vndFrameNum, miReq] : miRequests) {
            MINT32 ev_value = 0;
            MiMetadata::getEntry<MINT32>(&miReq->result, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                         ev_value);
            if (ev_value == 0) {
                srNum++;
            } else {
                hdrNum++;
            }
        }

        MLOGD(LogGroupHAL, "SR + HDR situation, srNum:%u, hdrNum:%u, totalNum:%lu", srNum, hdrNum,
              miRequests.size());
        // output:
        normalOut();

        // input:
        int inBufferIndex = 1;
        for (auto &&[vndFrameNum, miReq] : miRequests) {
            int inPortId = 0;
            if (inBufferIndex++ > srNum) {
                // frames from HDR should only have one inBuffer;
                // frames from MFNR could have R3 inBuffer;
                inPortId = 2;
            }
            uint64_t timeStamp = miReq->shutter;
            MLOGD(LogGroupHAL, "timeStamp:%lu, miReq:%u.", timeStamp, miReq->frameNum);
            inflightRequest->tsMapReqs[timeStamp] = miReq;
            for (auto &&buffer : miReq->vndRequest.outputBuffers) {
                if (mVndConfig->streams[buffer.streamId]->usageType ==
                    StreamUsageType::SnapshotStream) {
                    inflightRequest->inputBuffers[inPortId].emplace_back(
                        std::make_pair(timeStamp, buffer));
                    inPortId++;
                }
            }
        }
        break;
    }
    default:
        MLOGE(LogGroupHAL, "portsRule:%d error!", portsRule);
        return MIAS_INVALID_PARAM;
    }
    auto dumpInflightInfo = [&](std::shared_ptr<InflightRequest> inflight) -> int {
        std::string str;
        str += "frameNum:" + std::to_string(mFwkReq.frameNumber) + ", ";
        str += "timestamp:" + std::to_string(inflight->timestamp) + ", ";
        for (auto &&[port, vec] : inflight->inputBuffers) {
            str += "{";
            str += "InPort:" + std::to_string(port);
            for (auto &&[ts, sb] : vec) {
                str += "[ts:" + std::to_string(ts) + "]";
            }
            str += "}";
        }
        MLOGD(LogGroupHAL, "inflight info: %s", str.c_str());
        return 0;
    }(inflightRequest);

    return MIAS_OK;
}

int AlgoEngine::buildOutputSessionParams(CaptureRequest &fwkRequest, StreamBuffer buffer,
                                         int32_t algoPortId,
                                         MiaProcessParams &sessionParams /* output */)
{
    sessionParams.frameNum = fwkRequest.frameNumber;

    camera_metadata_t *meta = nullptr;
    camera_metadata_t *phyMeta = nullptr;
    DecoupleUtil::convertMetadata(fwkRequest.settings, meta);

    DecoupleUtil::convert2MiaImage(buffer, *sessionParams.outputFrame);

    sessionParams.outputNum = 1;
    sessionParams.inputNum = 0;

    sessionParams.outputFrame->frameNumber = fwkRequest.frameNumber;
    sessionParams.outputFrame->streamId = algoPortId;
    sessionParams.outputFrame->callback = mBufferCallback.get();
    sessionParams.outputFrame->metaWraper = std::make_shared<DynamicMetadataWraper>(meta, phyMeta);
    MLOGD(LogGroupHAL, "meta = %p, phyMeta = %p, outputFrame.metaWraper = %p", meta, phyMeta,
          sessionParams.outputFrame->metaWraper.get());
    return 0;
}

int AlgoEngine::buildInputSessionParams(const CaptureRequest &fwkRequest,
                                        const std::shared_ptr<MiRequest> &miReq,
                                        const StreamBuffer &buffer, int32_t algoPortId,
                                        MiaProcessParams &sessionParams /* output */)
{
    sessionParams.frameNum = fwkRequest.frameNumber;
    sessionParams.outputNum = 0;
    sessionParams.inputNum = 1;
    sessionParams.inputFrame->callback = mBufferCallback.get();
    sessionParams.inputFrame->streamId = algoPortId;
    sessionParams.inputFrame->frameNumber = fwkRequest.frameNumber;

    auto &stream = mVndConfig->streams[buffer.streamId];
    PostProcParams *pInPostProcParams = NULL;
    int32_t phyId = -1;
    auto it = miReq->physicalCameraMetadata.find(stream->rawStream.physicalCameraId);
    if (!miReq->physicalCameraMetadata.empty() && it != miReq->physicalCameraMetadata.end()) {
        phyId = it->second.physicalCameraId;
    }
    for (auto &&pData : miReq->vPostProcData) {
        if (pData->physicalCameraId == phyId) {
            pInPostProcParams = pData->pPostProcParams;
        }
    }
    DecoupleUtil::convertMetadata(mSessionParams, pInPostProcParams->sessionParams);
    pInPostProcParams->bufferId = buffer.bufferId;
    DecoupleUtil::convertMetadata(buffer.bufferSettings, pInPostProcParams->bufferSettings);
    DecoupleUtil::convertMetadata(miReq->halResult, pInPostProcParams->halSettings);
    pInPostProcParams->usage = stream->rawStream.usage;

    sessionParams.inputFrame->timeStampUs = static_cast<int64_t>(miReq->shutter);

    camera_metadata_t *meta = nullptr;
    camera_metadata_t *phyMeta = nullptr;
    DecoupleUtil::convertMetadata(miReq->result, meta);
    if (!miReq->physicalCameraMetadata.empty() && it != miReq->physicalCameraMetadata.end()) {
        DecoupleUtil::convertMetadata(it->second.metadata, phyMeta);

        pInPostProcParams->physicalCameraSettings.physicalCameraId = it->second.physicalCameraId;
        DecoupleUtil::convertMetadata(it->second.halMetadata,
                                      pInPostProcParams->physicalCameraSettings.halSettings);
        DecoupleUtil::convertMetadata(it->second.metadata,
                                      pInPostProcParams->physicalCameraSettings.settings);
    }

    sessionParams.inputFrame->metaWraper = std::make_shared<DynamicMetadataWraper>(meta, phyMeta);
    MLOGD(LogGroupHAL, "meta = %p, phyMeta = %p, inputFrame.metaWraper = %p", meta, phyMeta,
          sessionParams.inputFrame->metaWraper.get());
    if (mVndConfig->streams[buffer.streamId]->rawStream.format == eImgFmt_RAW16 &&
        buffer.buffer->getImgFormat() == eImgFmt_BLOB) {
        // need to modify inputBuffer format eImgFmt_BLOB to eImgFmt_RAW16
        DecoupleUtil::convert2MiaImage(buffer, *sessionParams.inputFrame, &stream->rawStream);
    } else {
        DecoupleUtil::convert2MiaImage(buffer, *sessionParams.inputFrame);
    }
    sessionParams.inputFrame->pBuffer.reserved = pInPostProcParams;
    return 0;
}

void AlgoEngine::onMetedataRecieved(uint32_t fwkFrameNum, const std::string &exifInfo)
{
    MLOGI(LogGroupHAL, "+ fwkframeNum = %d", fwkFrameNum);
    MLOGD(LogGroupHAL, "exifInfo = %s", exifInfo.c_str());

    {
        std::lock_guard<std::mutex> lock(mInflightRequestsMutex);
        auto inflightRequest = getInflightRequestLocked(fwkFrameNum);
        if (inflightRequest == nullptr) {
            MLOGE(LogGroupHAL, "inflightRequest is nullptr");
            return;
        }

        inflightRequest->exifInfo = exifInfo;
    }

    CaptureResult result;
    // fill halresult, and tuningBuffer will be filled by AlgoMode;
    result.frameNumber = fwkFrameNum;
    result.bLastPartialResult = true; // need mark as last partial result
    MiMetadata::MiEntry exifEntry(XIAOMI_MIVI2_EXIF);
    exifEntry.push_back((uint8_t *)exifInfo.c_str(), exifInfo.length() + 1, Type2Type<uint8_t>());
    result.result.update(exifEntry);

    mMode->onReprocessResultComing(result);

    MLOGI(LogGroupHAL, "-");
}

bool AlgoEngine::getOutputBuffer(int64_t timestamp, int32_t portId, buffer_handle_t &bufferHandle)
{
    MLOGI(LogGroupHAL, "+");

    std::shared_ptr<InflightRequest> inflightRequest(nullptr);
    std::lock_guard<std::mutex> lock(mInflightRequestsMutex);
    auto it = std::find_if(mInflightRequests.begin(), mInflightRequests.end(),
                           [timestamp](auto elem) -> bool {
                               auto &inflight = elem.second;
                               return inflight->timestamp == timestamp;
                           });
    if (it != mInflightRequests.end()) {
        inflightRequest = it->second;
    } else {
        MLOGE(LogGroupHAL, "inflightRequest for timestamp(%ld) not found!", timestamp);
        return false;
    }

    auto buffer = inflightRequest->outputBuffers[portId][timestamp];
    auto streamId = buffer.streamId;
    StreamBuffer bufferOut;
    bool ok = true;
    if (buffer.bufferId == 0 || buffer.buffer == nullptr) {
        ok = mMode->getFwkBuffer(inflightRequest->fwkReq, streamId, bufferOut);
        if (!ok) {
            MLOGE(LogGroupHAL, "fwkRequest(%d) getFwkBuffer failed for stream(%d)",
                  inflightRequest->fwkReq.frameNumber, streamId);
            return false;
        }

        // update inflightRequest->outputBuffers
        MLOGD(LogGroupHAL, "%s", utils::toString(bufferOut).c_str());
        inflightRequest->outputBuffers[portId][timestamp] = bufferOut;
    } else {
        // may already requested after switchToOffline, in this case, just return buffer stored in
        // inflightRequest
        MLOGI(LogGroupHAL, "buffer already requested: bufferId=%lu", buffer.bufferId);
        bufferOut = buffer;
    }

    auto *handlePtr = bufferOut.buffer->getBufferHandlePtr();
    if (handlePtr == nullptr) {
        MLOGE(LogGroupHAL, "getBufferHandlePtr failed!");
        bufferHandle = nullptr;
        return false;
    }
    bufferHandle = *handlePtr;

    MLOGI(LogGroupHAL, "-");
    return ok;
}

void AlgoEngine::switchToOffline()
{
    MLOGI(LogGroupHAL, "+");

    std::lock_guard<std::mutex> lock(mInflightRequestsMutex);
    for (auto &&[frameNum, inflightRequest] : mInflightRequests) {
        for (auto &&[portId, outputBuffers] : inflightRequest->outputBuffers) {
            for (auto &&[timestamp, buffer] : outputBuffers) {
                StreamBuffer bufferOut;
                // if null buffer exist, request here
                if (buffer.bufferId == 0 || buffer.buffer == nullptr) {
                    auto streamId = buffer.streamId;
                    auto ok = mMode->getFwkBuffer(inflightRequest->fwkReq, streamId, bufferOut);
                    if (!ok) {
                        MLOGE(LogGroupHAL, "fwkRequest(%d) getFwkBuffer failed for stream(%d)",
                              inflightRequest->fwkReq.frameNumber, streamId);
                        continue;
                    }

                    // update inflightRequest->outputBuffers
                    MLOGD(LogGroupHAL, "%s", utils::toString(bufferOut).c_str());
                    buffer = bufferOut;
                }
            }
        }
    }

    MLOGI(LogGroupHAL, "-");
}

void AlgoEngine::releaseOutputBuffer(uint32_t fwkFrameNum, int64_t timeStamp, uint32_t portId)
{
    MLOGI(LogGroupHAL, "+");
    MLOGD(LogGroupHAL, "frameNum = %d, portId = %d", fwkFrameNum, portId);

    std::lock_guard<std::mutex> lock(mInflightRequestsMutex);
    auto inflightRequest = getInflightRequestLocked(fwkFrameNum);
    if (inflightRequest == nullptr) {
        MLOGE(LogGroupHAL, "inflightRequest is nullptr");
        return;
    }

    auto &outputBuffers = inflightRequest->outputBuffers;

    if (outputBuffers.find(portId) == outputBuffers.end()) {
        MLOGE(LogGroupHAL, "portId (%d) not found in inflightRequest", portId);
        return;
    }

    if (outputBuffers[portId].find(timeStamp) == outputBuffers[portId].end()) {
        MLOGE(LogGroupHAL, "timeStamp (%ld) not found in inflightRequest", timeStamp);
        return;
    }

    CaptureResult result{
        .frameNumber = inflightRequest->fwkReq.frameNumber,
        .outputBuffers = {outputBuffers[portId][timeStamp]},
    };
    mMode->onReprocessResultComing(result);

    outputBuffers[portId].erase(timeStamp);
    if (outputBuffers[portId].empty()) {
        outputBuffers.erase(portId);
    }

    if (outputBuffers.empty()) {
        MLOGD(LogGroupHAL,
              "process task (fwkFrameNum = %d) completed, erase from mInflightRequests",
              fwkFrameNum);
        mInflightRequests.erase(fwkFrameNum);

        mMode->onReprocessCompleted(fwkFrameNum);
    }

    MLOGI(LogGroupHAL, "-");
}

void AlgoEngine::releaseInputBuffer(uint32_t fwkFrameNum, int64_t timeStamp, uint32_t portId)
{
    MLOGI(LogGroupHAL, "+");
    MLOGD(LogGroupHAL, "frameNum = %d, timeStamp = %ld, portId = %d", fwkFrameNum, timeStamp,
          portId);

    if (mBufferManager == nullptr) {
        MLOGE(LogGroupHAL, "mBufferManager is nullptr");
    }

    std::lock_guard<std::mutex> lock(mInflightRequestsMutex);
    auto inflightRequest = getInflightRequestLocked(fwkFrameNum);
    if (inflightRequest == nullptr) {
        MLOGE(LogGroupHAL, "inflightRequest is nullptr");
        return;
    }

    auto &inputBuffers = inflightRequest->inputBuffers;

    if (inputBuffers.find(portId) == inputBuffers.end()) {
        MLOGE(LogGroupHAL, "portId (%d) not found in inflightRequest", portId);
        return;
    }

    auto it = std::find_if(
        inputBuffers[portId].begin(), inputBuffers[portId].end(),
        [&](const std::pair<int64_t, StreamBuffer> pair) { return pair.first == timeStamp; });

    if (it == inputBuffers[portId].end()) {
        MLOGE(LogGroupHAL, "timeStamp (%ld) not found in inflightRequest", timeStamp);
        return;
    }

    std::vector<StreamBuffer> streamBuffers;
    streamBuffers.push_back(it->second);
    mBufferManager->releaseBuffers(streamBuffers);

    inputBuffers[portId].erase(it);
    if (inputBuffers[portId].empty()) {
        inputBuffers.erase(portId);
    }

    MLOGI(LogGroupHAL, "-");
}

int AlgoEngine::flush(bool isForced)
{
    MLOGI(LogGroupHAL, "+");
    CDKResult status = MIAS_OK;
    status = MiaPostProc_Flush(mAlgoEngineSession, isForced);

    if (mBufferManager == nullptr) {
        MLOGE(LogGroupHAL, "mBufferManager is nullptr");
    }

    std::map<int, std::vector<StreamBuffer>> fwkStreamBufferMap;
    std::lock_guard<std::mutex> lock(mInflightRequestsMutex);

    MLOGI(LogGroupHAL, "mInflightRequests.size = %zu.", mInflightRequests.size());

    for (auto &&[fwkFrameNum, inflightRequest] : mInflightRequests) {
        // release input buffers
        std::vector<StreamBuffer> outStreamBuffers;
        for (auto &&[portId, portBuffers] : inflightRequest->outputBuffers) {
            for (auto &&[timeStamp, buffer] : portBuffers) {
                outStreamBuffers.emplace_back(buffer);
            }
        }
        inflightRequest->outputBuffers.clear();
        if (!outStreamBuffers.empty()) {
            NotifyMsg notifyErr = {
                .type = MsgType::ERROR,
                .msg.error.frameNumber = inflightRequest->fwkReq.frameNumber,
                .msg.error.errorStreamId = NO_STREAM,
                .msg.error.errorCode = ErrorCode::ERROR_REQUEST,
            };
            mMode->forwardNotifyToFwk(notifyErr);

            CaptureResult result{
                .frameNumber = fwkFrameNum,
                .outputBuffers = outStreamBuffers,
                .bLastPartialResult = true,
            };
            mMode->onReprocessResultComing(result);
            mMode->onReprocessCompleted(fwkFrameNum);
        }

        // release input buffers
        std::vector<StreamBuffer> inStreamBuffers;
        for (auto &&[portId, portBuffers] : inflightRequest->inputBuffers) {
            for (auto &&[timeStamp, buffer] : portBuffers) {
                inStreamBuffers.emplace_back(buffer);
            }
        }
        inflightRequest->inputBuffers.clear();
        if (!inStreamBuffers.empty()) {
            mBufferManager->releaseBuffers(inStreamBuffers);
        }
    }
    mInflightRequests.clear();

    MLOGI(LogGroupHAL, "-");
    return status;
}

int AlgoEngine::quickFinish(std::string pipelineName)
{
    MLOGI(LogGroupHAL, "+");
    int ret = 0;
    QuickFinishMessageInfo messageInfo{pipelineName, true /*isNeedImageResult*/};

    ret = MiaPostProc_QuickFinish(mAlgoEngineSession, messageInfo); // CDKResult

    MLOGD(LogGroupHAL, "[Snapshot][QuickFinishTask] MiaPostProc_QuickFinish ret %d", ret);
    MLOGI(LogGroupHAL, "-");

    return ret;
}

std::shared_ptr<AlgoEngine::InflightRequest> AlgoEngine::getInflightRequestLocked(
    uint32_t fwkFrameNum)
{
    auto it = mInflightRequests.find(fwkFrameNum);
    if (it == mInflightRequests.end()) {
        MLOGE(LogGroupHAL, "not found (frameNum = %d) in mInflightRequests, size:%lu", fwkFrameNum,
              mInflightRequests.size());
        return nullptr;
    }

    return it->second;
}

} // namespace mizone
