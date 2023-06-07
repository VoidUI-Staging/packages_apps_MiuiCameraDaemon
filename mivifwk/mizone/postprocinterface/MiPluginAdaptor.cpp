#include "MiPluginAdaptor.h"

#include "DecoupleUtil.h"

namespace mizone {
namespace mipostprocinterface {

auto MiPluginAdaptor::create(CreationInfos &rInfos) -> std::shared_ptr<MiPluginAdaptor>
{
    MLOGI(LogGroupType, "+");

    auto rSession = std::make_shared<MiPluginAdaptor>(rInfos);
    if (rSession == nullptr) {
        MLOGE(LogGroupType, "session is nullptr, create MiPluginAdaptor fail!");
    }

    MLOGI(LogGroupType, "-");
    return rSession;
}

MiPluginAdaptor::MiPluginAdaptor(CreationInfos &rInfos)
{
    MLOGD(LogGroupType, "MiPluginAdaptor");
    mPostProcDevice = MiPostProcDevice::create(rInfos);
}

MiPluginAdaptor::~MiPluginAdaptor()
{
    MLOGD(LogGroupType, "~MiPluginAdaptor");
}

int MiPluginAdaptor::process(ProcessInfos &rInfos, ProcessRequestInfo *pluginInfo)
{
    MLOGI(LogGroupType, "+");
    auto &phInputBuffer = pluginInfo->inputBuffersMap.begin()->second;
    if (phInputBuffer.size()) {
        if (phInputBuffer[0].reserved != NULL) {
            auto pInPostProcParams = static_cast<PostProcParams *>(phInputBuffer[0].reserved);
            IMetadata::getEntry(&pInPostProcParams->sessionParams, MTK_CONFIGURE_ISP_CAMERA_ID,
                                mCamId);
            // set first frame vndRequest number to PostProcDevice request number
            mPluginReqNo = pInPostProcParams->vndRequestNo;
        }
    }
    MLOGD(LogGroupType, "mCamId = %d, FrameNum = %d, fwkReqNum = %d, vndReqNum = %d", mCamId,
          rInfos.multiFrameNum, pluginInfo->frameNum, mPluginReqNo);
    multiFrameCount = rInfos.multiFrameNum;
    mPluginInfo = pluginInfo;
    mPluginCb = rInfos.cb;
    rInfos.adaptorCb = this;
    mPostProcDevice->process(rInfos);

    if (multiFrameCount > 0) {
        if (rInfos.capFeatureType == CapFeatureType::FEATURE_R2Y ||
            rInfos.capFeatureType == CapFeatureType::FEATURE_Y2Y ||
            rInfos.capFeatureType == CapFeatureType::FEATURE_Y2J ||
            rInfos.capFeatureType == CapFeatureType::FEATURE_R2R ||
            rInfos.capFeatureType == CapFeatureType::FEATURE_MFNR ||
            rInfos.capFeatureType == CapFeatureType::FEATURE_AINR ||
            rInfos.capFeatureType == CapFeatureType::FEATURE_AISHUTTER1 ||
            rInfos.capFeatureType == CapFeatureType::FEATURE_AISHUTTER2) {
            AdaptorParams mParams;
            mParams.capFeatureType = rInfos.capFeatureType;

            // NOTE：keep repeating configuration mode. Currently using configMode ONCE.
            if (rInfos.configMode == ConfigMode::REPEATED) {
                mMapStream.clear();
            }

            convertRequestInfos(mParams, pluginInfo);
            configureIfNeed(mParams, pluginInfo);
            mPostProcDevice->request(mParams.requestsIn);
        } else {
            MLOGE(LogGroupType, "do not support feature type:%d", rInfos.capFeatureType);
        }
    }

    MLOGI(LogGroupType, "-");
    return 0;
}

void MiPluginAdaptor::composeStream(const ImageParams &phBuffer,
                                    std::vector<StreamBuffer> &streamBuffers, StreamInfos &rInfos)
{
    for (auto &streamBuffer : streamBuffers) {
        Stream stream;
        if (rInfos.streamType == StreamType::INPUT) {
            stream.streamType = StreamType::INPUT;
        } else if (rInfos.streamType == StreamType::OUTPUT) {
            stream.streamType = StreamType::OUTPUT;
        }

        if (phBuffer.format.format == eImgFmt_RAW16) {
            stream.width = phBuffer.format.width;
            stream.height = phBuffer.format.height;
            stream.format = static_cast<EImageFormat>(phBuffer.format.format);
        } else {
            stream.width = streamBuffer.buffer->getImgSize().w;
            stream.height = streamBuffer.buffer->getImgSize().h;
            stream.format = static_cast<EImageFormat>(streamBuffer.buffer->getImgFormat());
        }

        // check map and add buffer streamId
        auto ret = mMapStream.equal_range(stream.format);
        bool flag = false;
        for (auto it = ret.first; it != ret.second; ++it) {
            if (it->second.streamType == stream.streamType && it->second.width == stream.width &&
                it->second.height == stream.height) {
                streamBuffer.streamId = it->second.stream.id;
                flag = true;
                break;
            }
        }

        if (flag) {
            continue;
        }

        rStreamIdStart++;
        stream.id = rStreamIdStart;
        stream.usage = rInfos.pPostProcParams->usage;
        stream.dataSpace = Dataspace::UNKNOWN;
        stream.transform = NSCam::eTransform_None;
        stream.physicalCameraId = rInfos.pPostProcParams->physicalCameraSettings.physicalCameraId;
        if (stream.physicalCameraId == -1) {
            stream.physicalCameraId = mCamId;
        }
        stream.bufPlanes.count = streamBuffer.buffer->getPlaneCount();
        // stream.bufferSize = streamBuffer.buffer->getBufSizeInBytes(stream.bufPlanes.count);

        streamBuffer.streamId = stream.id;

        rInfos.width = stream.width;
        rInfos.height = stream.height;
        rInfos.stream = stream;

        MLOGD(LogGroupType, "add stream format:%d w:%d, h:%d type:%d", stream.format, stream.width,
              stream.height, stream.streamType);
        mMapStream.emplace(stream.format, rInfos);
        mIsNeedRecfg = true;
    }
}

void MiPluginAdaptor::configureIfNeed(AdaptorParams &rParams, ProcessRequestInfo *pluginInfo)
{
    auto &phInputBuffer = pluginInfo->inputBuffersMap.begin()->second;
    auto pInPostProcParams = static_cast<PostProcParams *>(phInputBuffer[0].reserved);
    auto &streamCfg = rParams.streamCfgIn;
    streamCfg.streamConfigCounter = 1;
    streamCfg.operationMode = StreamConfigurationMode::NORMAL_MODE;
    streamCfg.sessionParams = pInPostProcParams->sessionParams;

    if (mIsNeedRecfg) {
        MLOGI(LogGroupType, "need to reconfigure");
        // replace streams in mMapStream with streams in rParams.streamCfgIn
        for (auto it = mMapStream.begin(); it != mMapStream.end(); ++it) {
            streamCfg.streams.emplace_back(it->second.stream);
        }
        mPostProcDevice->configure(streamCfg);
        mIsNeedRecfg = false;
    } else {
        MLOGI(LogGroupType, "no need to reconfigure");
        std::stringstream ss;
        ss << "{ dump mMapStream: ";
        for (auto it = mMapStream.begin(); it != mMapStream.end(); ++it) {
            ss << "[ ";
            ss << dumpStream(it->second.stream) << ", ";
            ss << "], ";
        }
        ss << "}";
        MLOGD(LogGroupType, "%s", ss.str().c_str());
    }
}

void MiPluginAdaptor::composeStreamBuffer(const ImageParams &phParams,
                                          std::vector<StreamBuffer> &streamBuffers,
                                          StreamInfos &rInfos)
{
    StreamBuffer streamBuffer;
    DecoupleUtil::buildStreamBuffer(phParams, streamBuffer);
    if (rInfos.streamType == StreamType::INPUT) {
        streamBuffer.bufferId = rInfos.pPostProcParams->bufferId;
    } else if (rInfos.streamType == StreamType::OUTPUT) {
        rBufferIdx++;
        streamBuffer.bufferId = rBufferIdx;
    }
    streamBuffer.status = BufferStatus::OK;
    streamBuffer.acquireFenceFd = -1;
    streamBuffer.releaseFenceFd = -1;
    streamBuffer.bufferSettings = rInfos.pPostProcParams->bufferSettings;
    streamBuffers.emplace_back(streamBuffer);
}

void MiPluginAdaptor::composeMetadata(const ImageParams &phParams, CaptureRequest &request,
                                      StreamInfos &rInfos)
{
    DecoupleUtil::convertMetadata(phParams.pMetadata, request.settings);
    request.halSettings = rInfos.pPostProcParams->halSettings;
    PhysicalCameraSetting phyCamSettings;
    phyCamSettings.physicalCameraId =
        rInfos.pPostProcParams->physicalCameraSettings.physicalCameraId;
    phyCamSettings.settings = rInfos.pPostProcParams->physicalCameraSettings.settings;
    phyCamSettings.halSettings = rInfos.pPostProcParams->physicalCameraSettings.halSettings;
    request.physicalCameraSettings.emplace_back(phyCamSettings);
    // record request settings <vndReqNo, settings>
    mMapReqSettings.emplace(request.frameNumber, request.settings);
}

void MiPluginAdaptor::composeMetadata(const ImageParams &phParams, SubRequest &request,
                                      StreamInfos &rInfos)
{
    DecoupleUtil::convertMetadata(phParams.pMetadata, request.settings);
    request.halSettings = rInfos.pPostProcParams->halSettings;
    PhysicalCameraSetting phyCamSettings;
    phyCamSettings.physicalCameraId =
        rInfos.pPostProcParams->physicalCameraSettings.physicalCameraId;
    phyCamSettings.settings = rInfos.pPostProcParams->physicalCameraSettings.settings;
    phyCamSettings.halSettings = rInfos.pPostProcParams->physicalCameraSettings.halSettings;
    request.physicalCameraSettings.emplace_back(phyCamSettings);
}

void MiPluginAdaptor::convertRequestInfos(AdaptorParams &rParams, ProcessRequestInfo *pluginInfo)
{
    if (!pluginInfo) {
        MLOGE(LogGroupType, "pluginInfo is null!");
        return;
    }

    auto &phInputBuffer = pluginInfo->inputBuffersMap.begin()->second;
    auto &phOutputBuffer = pluginInfo->outputBuffersMap.begin()->second;
    if (!(phInputBuffer.size() || phOutputBuffer.size())) {
        MLOGE(LogGroupType, "phXXXputBuffer.size is zero!");
        return;
    }

    StreamInfos inParms = {
        .streamType = StreamType::INPUT,
    };
    StreamInfos outParms = {
        .streamType = StreamType::OUTPUT,
    };

    MLOGD(LogGroupType,
          "get feature[%d: MFNR/AINR/AIHDR/AISHUTTER1/AISHUTTER2/R2R/R2Y/Y2Y/Y2J],"
          "InBuffer.size(%ld),"
          "OutBuffer.size(%ld)",
          rParams.capFeatureType + 1, phInputBuffer.size(), phOutputBuffer.size());
    switch (rParams.capFeatureType) {
    case CapFeatureType::FEATURE_MFNR: {
        CaptureRequest request;
        request.frameNumber = mPluginReqNo;
        std::vector<SubRequest> &subReqs = request.subRequests;
        subReqs.resize(multiFrameCount - 1);
        for (auto i = 0; i < phInputBuffer.size(); ++i) {
            inParms.pPostProcParams = static_cast<PostProcParams *>(phInputBuffer[i].reserved);
            if (i == 0) {
                composeMetadata(phInputBuffer[i], request, inParms);
                composeStreamBuffer(phInputBuffer[i], request.inputBuffers, inParms);
                composeStream(phInputBuffer[i], request.inputBuffers, inParms);
            } else {
                composeMetadata(phInputBuffer[i], subReqs[i - 1], inParms);
                composeStreamBuffer(phInputBuffer[i], subReqs[i - 1].inputBuffers, inParms);
                composeStream(phInputBuffer[i], subReqs[i - 1].inputBuffers, inParms);
            }
        }

        for (auto i = 0; i < phOutputBuffer.size(); ++i) {
            outParms.pPostProcParams = static_cast<PostProcParams *>(phOutputBuffer[i].reserved);
            composeStreamBuffer(phOutputBuffer[i], request.outputBuffers, outParms);
            composeStream(phOutputBuffer[i], request.outputBuffers, outParms);
            std::unique_lock<std::mutex> lk(mx);
            mMapReqNum.emplace(request.frameNumber, i);
        }

        rParams.requestsIn.push_back(request);
        break;
    }
    case CapFeatureType::FEATURE_AISHUTTER1:
    case CapFeatureType::FEATURE_AISHUTTER2:
    case CapFeatureType::FEATURE_AINR: {
        CaptureRequest request;
        request.frameNumber = mPluginReqNo;
        std::vector<SubRequest> &subReqs = request.subRequests;
        subReqs.resize(multiFrameCount - 1);

        // prot1 ->Raw frame
        for (auto i = 0; i < phInputBuffer.size(); ++i) {
            inParms.pPostProcParams = static_cast<PostProcParams *>(phInputBuffer[i].reserved);
            if (i == 0) {
                composeMetadata(phInputBuffer[i], request, inParms);
                composeStreamBuffer(phInputBuffer[i], request.inputBuffers, inParms);
                composeStream(phInputBuffer[i], request.inputBuffers, inParms);
            } else {
                composeMetadata(phInputBuffer[i], subReqs[i - 1], inParms);
                composeStreamBuffer(phInputBuffer[i], subReqs[i - 1].inputBuffers, inParms);
                composeStream(phInputBuffer[i], subReqs[i - 1].inputBuffers, inParms);
            }
        }

        // prot1 ->R3 frame
        auto &phInputBuffer1 = pluginInfo->inputBuffersMap.at(1);
        for (auto i = 0; i < phInputBuffer1.size(); ++i) {
            inParms.pPostProcParams = static_cast<PostProcParams *>(phInputBuffer1[i].reserved);
            if (i == 0) {
                composeStreamBuffer(phInputBuffer1[i], request.inputBuffers, inParms);
                composeStream(phInputBuffer1[i], request.inputBuffers, inParms);
            } else {
                composeStreamBuffer(phInputBuffer1[i], subReqs[i - 1].inputBuffers, inParms);
                composeStream(phInputBuffer1[i], subReqs[i - 1].inputBuffers, inParms);
            }
        }

        for (auto i = 0; i < phOutputBuffer.size(); ++i) {
            outParms.pPostProcParams = static_cast<PostProcParams *>(phOutputBuffer[i].reserved);
            composeStreamBuffer(phOutputBuffer[i], request.outputBuffers, outParms);
            composeStream(phOutputBuffer[i], request.outputBuffers, outParms);
            std::unique_lock<std::mutex> lk(mx);
            mMapReqNum.emplace(request.frameNumber, i);
        }

        rParams.requestsIn.push_back(request);
        break;
    }
    case CapFeatureType::FEATURE_R2R:
    case CapFeatureType::FEATURE_R2Y:
    case CapFeatureType::FEATURE_Y2Y:
    case CapFeatureType::FEATURE_Y2J: {
        std::stringstream ss;
        ss << "{frame, ReqNo}";
        uint32_t reqNo = mPluginReqNo;
        for (auto i = 0; i < phInputBuffer.size(); ++i) {
            inParms.pPostProcParams = static_cast<PostProcParams *>(phInputBuffer[i].reserved);
            outParms.pPostProcParams = inParms.pPostProcParams;
            CaptureRequest request;
            request.frameNumber = reqNo++;
            composeMetadata(phInputBuffer[i], request, inParms);
            composeStreamBuffer(phInputBuffer[i], request.inputBuffers, inParms);
            composeStream(phInputBuffer[i], request.inputBuffers, inParms);
            composeStreamBuffer(phOutputBuffer[i], request.outputBuffers, outParms);
            composeStream(phOutputBuffer[i], request.outputBuffers, outParms);

            std::unique_lock<std::mutex> lk(mx);
            mMapReqNum.emplace(request.frameNumber, i);

            // tuning buffer
            // if (pluginInfo->phInputBuffer.size() != pluginInfo->phOutputBuffer.size() && i == 0)
            // {
            //     for (auto j = 0; j < pluginInfo->phOutputBuffer.size(); ++j) {
            //         if (pluginInfo->phOutputBuffer[j].format.format ==
            //             MiaPixelFormat::CAM_FORMAT_YV12) {
            //             outParms.pPostProcParams =
            //                 static_cast<PostProcParams
            //                 *>(pluginInfo->phOutputBuffer[j].reserved);
            //             composeStreamBuffer(pluginInfo->phOutputBuffer[j], request.outputBuffers,
            //                                 outParms);
            //             composeStream(pluginInfo->phOutputBuffer[j], request.outputBuffers,
            //                           outParms);
            //             mMapReqNum.emplace(request.frameNumber, j);
            //         }
            //     }
            // }

            rParams.requestsIn.push_back(request);
            ss << "{";
            ss << i << ", ";
            ss << request.frameNumber << "} ";
        }
        MLOGD(LogGroupType, "%s", ss.str().c_str());
        break;
    }
    }
}

int MiPluginAdaptor::flush()
{
    MLOGI(LogGroupType, "+");

    auto ret = mPostProcDevice->flush();
    auto waitUntilEmpty = [this](uint64_t timeoutMs = 1000) {
        std::unique_lock<std::mutex> lk(mx);
        return cond.wait_for(lk, std::chrono::milliseconds(timeoutMs),
                             [this] { return mMapReqNum.empty(); });
    };
    // wait until inflight requests done
    if (waitUntilEmpty() && !ret) {
        MLOGD(LogGroupType, "flush done.");
    } else {
        MLOGE(LogGroupType, "flush fail!");
    }

    MLOGI(LogGroupType, "-");
    return ret;
}

int MiPluginAdaptor::destroy()
{
    MLOGI(LogGroupType, "+");

    int ret = -1;
    if (mPostProcDevice) {
        ret = mPostProcDevice->destroy();
        mPostProcDevice = nullptr;
    } else {
        MLOGE(LogGroupType, "mPostProcDevice is nullptr.");
    }

    MLOGI(LogGroupType, "-");
    return ret;
}

void MiPluginAdaptor::processReqNo(const uint32_t &requestNo)
{
    std::unique_lock<std::mutex> lk(mx);
    for (auto &&it = mMapReqNum.begin(); it != mMapReqNum.end();) {
        if (it->first == requestNo) {
            it = mMapReqNum.erase(it);
        } else {
            ++it;
        }
    }
    lk.unlock();
    cond.notify_one();
}

void MiPluginAdaptor::processReqNo(const uint32_t &requestNo, CaptureResult &res)
{
    std::unique_lock<std::mutex> lk(mx);
    for (auto &&it = mMapReqNum.begin(); it != mMapReqNum.end();) {
        if (it->first == requestNo) {
            auto &phOutputBuffer = mPluginInfo->outputBuffersMap.begin()->second;
            if (mPluginInfo != NULL && phOutputBuffer[it->second].reserved != NULL) {
                auto pOutPostProcParams =
                    static_cast<PostProcParams *>(phOutputBuffer[it->second].reserved);
                pOutPostProcParams->halSettings = res.halResult;

                auto itSettings = mMapReqSettings.find(requestNo);
                if (itSettings != mMapReqSettings.end()) {
                    pOutPostProcParams->settings = itSettings->second + res.result;
                    // update outputbuffers logical camera metadata (camera_metadata_t *pMetadata)
                    DecoupleUtil::convertMetadata(pOutPostProcParams->settings,
                                                  phOutputBuffer[it->second].pMetadata);
                    mMapReqSettings.erase(itSettings);
                }
                MLOGD(LogGroupType,
                      "upload plugin outputBuffer[%d] settings(%d) and halsettings(%d)", it->second,
                      pOutPostProcParams->settings.count(),
                      pOutPostProcParams->halSettings.count());
            }
            it = mMapReqNum.erase(it);
        } else {
            ++it;
        }
    }
    lk.unlock();
    cond.notify_one();
}

void MiPluginAdaptor::handleNotify(const std::vector<NotifyMsg> &msgs)
{
    if (msgs.empty()) {
        MLOGI(LogGroupType, "message is empty!");
        return;
    }

    for (auto &&msg : msgs) {
        MLOGD(LogGroupType, "%s", dumpNotify(msg).c_str());
        if (msg.type == MsgType::SHUTTER) {
            MLOGI(LogGroupType,
                  "shutter notify, frame num:%d,"
                  " timestamp:%ld",
                  msg.msg.shutter.frameNumber, msg.msg.shutter.timestamp);
        } else if (msg.type == MsgType::ERROR) {
            processReqNo(msg.msg.error.frameNumber);
            MLOGE(LogGroupType,
                  "error notify, frame num:%d,"
                  " streamID:%ld,"
                  "errorCode:%d",
                  msg.msg.error.frameNumber, msg.msg.error.errorStreamId, msg.msg.error.errorCode);
            callbackInfos.msg = ResultStatus::ERROR;
        }
    }

    // default: callbackInfos.msg = OK
    if (mMapReqNum.empty()) {
        if (callbackInfos.msg == ResultStatus::ERROR) {
            MLOGE(LogGroupType, "handle requestNo[%d] fail", mPluginReqNo);
        } else {
            MLOGI(LogGroupType, "handle requestNo[%d] success", mPluginReqNo);
        }
        mPluginCb->processResultDone(callbackInfos);
        callbackInfos.msg = ResultStatus::OK;
    }
}

void MiPluginAdaptor::handleResult(const std::vector<CaptureResult> &results)
{
    for (auto &&result : results) {
        CaptureResult result_hack = result;
        if (result_hack.inputBuffers.size()) {
            result_hack.inputBuffers.clear();
        }
        MLOGD(LogGroupType, "%s", dumpResult(result_hack).c_str());
        if (result_hack.bLastPartialResult) {
            for (uint32_t i = 0; i < result_hack.outputBuffers.size(); i++) {
#if 0
                std::string name("postprocinterface_output_");
                name += std::to_string(result_hack.frameNumber);
                dumpBuffer(result_hack.outputBuffers[i].buffer, name);
#endif

                if (result_hack.outputBuffers[i].status == BufferStatus::ERROR) {
                    MLOGE(LogGroupType, "outputBuffers[%d]status error, handle requestNo[%d] fail",
                          i, result_hack.frameNumber);
                    callbackInfos.msg = ResultStatus::ERROR;
                }
            }

            // default: callbackInfos.msg = OK
            // Note: callback only after receiving all requests (for example R2Y: n request)
            processReqNo(result_hack.frameNumber, result_hack);
            if (mMapReqNum.empty()) {
                if (callbackInfos.msg == ResultStatus::ERROR) {
                    MLOGE(LogGroupType, "handle requestNo[%d] fail", mPluginReqNo);
                } else {
                    MLOGI(LogGroupType, "handle requestNo[%d] success", mPluginReqNo);
                }
                mPluginCb->processResultDone(callbackInfos);
                callbackInfos.msg = ResultStatus::OK;
            }
        }
    }
}

} // namespace mipostprocinterface
} // namespace mizone