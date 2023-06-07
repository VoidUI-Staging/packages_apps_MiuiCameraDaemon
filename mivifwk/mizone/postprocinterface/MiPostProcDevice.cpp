#include "MiPostProcDevice.h"

namespace mizone {
namespace mipostprocinterface {

void MiPostProcDevice::getPostProcProvider()
{
    IProviderCreationParams param = {
        .providerName = "PostProcEngine",
    };
    mProvider = getPostProcProviderInstance(&param);
    if (mProvider == nullptr) {
        MLOGE(LogGroupType, "provider is nullptr, get PostProcProvider fail!");
    }
}

void MiPostProcDevice::getPostProcSession(uint32_t engineTag)
{
    switch (engineTag) {
    case EngineType::ENGINE_CAPTURE:
    default:
        mProvider->getDeviceInterface(MTK_POSTPROCDEV_DEVICE_TYPE_CAPTURE, mDevice);
        break;
    }
    if (mDevice == nullptr) {
        MLOGE(LogGroupType, "cannot get postproc device through provider");
    }
    auto status = mDevice->open(mDeviceCb, mDeviceSession);
    if (status || mDeviceSession == nullptr) {
        MLOGE(LogGroupType, "mDeviceSession is nullptr, open PostProcSession fail!");
    }
}

auto MiPostProcDevice::create(CreationInfos &rInfos) -> std::shared_ptr<MiPostProcDevice>
{
    auto rSession = std::make_shared<MiPostProcDevice>(rInfos);
    if (rSession == nullptr) {
        MLOGE(LogGroupType, "session is nullptr, create MiPostProcDevice fail!");
    }
    return rSession;
}

MiPostProcDevice::MiPostProcDevice(CreationInfos &rInfos)
{
    MLOGD(LogGroupType, "MiPostProcDevice");

    mDeviceCb = std::make_shared<MiPostProcDeviceCb>(this);
    getPostProcProvider();
    getPostProcSession(rInfos.engineType);
    MLOGI(LogGroupType, "PostProcSession: %p", mDeviceSession.get());
    getPostProcEngine(rInfos);
}

int MiPostProcDevice::process(ProcessInfos &pInfos)
{
    mAdaptorCb = pInfos.adaptorCb;
    auto res = mEngineSession->process(pInfos);
    return res;
}

void MiPostProcDevice::getPostProcEngine(CreationInfos &rInfos)
{
    StreamConfiguration strCfg;
    HalStreamConfiguration strHalCfg;

    switch (rInfos.engineType) {
    case EngineType::ENGINE_CAPTURE:
        mEngineSession = MiCaptureEngine::openPostProcEngine();
        break;
    default:
        MLOGW(LogGroupType, "do not support engine type!");
        break;
    }
}

MiPostProcDevice::~MiPostProcDevice()
{
    MLOGD(LogGroupType, "~MiPostProcDevice");
}

int MiPostProcDevice::configure(StreamConfiguration &streamCfg)
{
    MLOGI(LogGroupType, "+");
    HalStreamConfiguration streamHalCfg;
    if (streamCfg.streams.size()) {
        mEngineSession->configure(streamCfg);
    } else {
        MLOGE(LogGroupType, "stream is empty, captureEngine configure failed!");
    }

    MLOGD(LogGroupType, "send stream infos to PostProcSession %p", mDeviceSession.get());
    MLOGD(LogGroupType, "dump StreamConfiguration infos");
    MLOGD(LogGroupType, "%s", dumpStreamConfiguration(streamCfg).c_str());

    auto res = mDeviceSession->configureStreams(streamCfg, streamHalCfg);
    if (res != 0) {
        MLOGE(LogGroupType, "mDeviceSession process configure fail, res = %d", res);
    }
    MLOGI(LogGroupType, "-");
    return res;
}

int MiPostProcDevice::request(std::vector<CaptureRequest> &requests)
{
    MLOGI(LogGroupType, "+");
    if (requests.size() != 0) {
        mEngineSession->request(requests);
    } else {
        MLOGE(LogGroupType, "request is empty, captureEngine request failed!");
    }

    MLOGD(LogGroupType, "send requests to PostProcSession %p", mDeviceSession.get());
    MLOGD(LogGroupType, "dump CaptureRequest infos");
    for (auto &&request : requests) {
        MLOGD(LogGroupType, "%s", dumpRequest(request).c_str());
        MLOGD(LogGroupType, "%s",
              dumpStreamBuffers(request.inputBuffers, "inputBuffers size").c_str());
        MLOGD(LogGroupType, "%s",
              dumpStreamBuffers(request.outputBuffers, "outputBuffers size").c_str());
        if (!request.subRequests.empty()) {
            MLOGD(LogGroupType, "subRequests size: %lu", request.subRequests.size());
            for (auto &&subReq : request.subRequests) {
                MLOGD(LogGroupType, "%s", dumpSubRequest(subReq).c_str());
            }
        }
    }

    uint32_t numRequestProcessed = 0;
    auto res = mDeviceSession->processCaptureRequest(requests, numRequestProcessed);
    if (res != 0 || numRequestProcessed <= 0) {
        MLOGE(LogGroupType, "mDeviceSession process processRequest failed, res = %d", res);
    }

    MLOGI(LogGroupType, "-");
    return res;
}

int MiPostProcDevice::flush()
{
    MLOGD(LogGroupType, "mDeviceSession %p, wait flush....", mDeviceSession.get());
    auto ret = mDeviceSession->flush();
    return ret;
}

int MiPostProcDevice::destroy()
{
    mEngineSession->destroy();
    mEngineSession = nullptr;
    mDeviceCb = nullptr;
    mAdaptorCb = nullptr;

    MLOGI(LogGroupType, "destroy PostProcSession: %p", mDeviceSession.get());
    mDeviceSession = nullptr;
    mDevice = nullptr;
    mProvider = nullptr;
    return 0;
}

void MiPostProcDevice::handleNotify(const std::vector<NotifyMsg> &msgs)
{
    mAdaptorCb->handleNotify(msgs);
}

void MiPostProcDevice::handleResult(const std::vector<CaptureResult> &results)
{
    mAdaptorCb->handleResult(results);
}

/**********************Capture Engine*****************************/
auto MiCaptureEngine::openPostProcEngine() -> std::shared_ptr<IMiPostProcEngine>
{
    auto rSession = std::make_shared<MiCaptureEngine>();
    if (rSession == nullptr) {
        MLOGE(LogGroupType, "session is nullptr, create MiCaptureEngine fail!");
    }
    return rSession;
}

MiCaptureEngine::MiCaptureEngine()
{
    MLOGD(LogGroupType, "MiCaptureEngine");
}

int MiCaptureEngine::process(ProcessInfos &pInfos)
{
    mInfos = pInfos;
    return 0;
}

int MiCaptureEngine::configure(StreamConfiguration &cfg)
{
    IMetadata::IEntry entry(MTK_POSTPROCDEV_CAPTURE_STREAM_USAGE_MAP);

    switch (mInfos.capFeatureType) {
    case CapFeatureType::FEATURE_R2Y:
    case CapFeatureType::FEATURE_Y2Y:
    case CapFeatureType::FEATURE_Y2J:
    case CapFeatureType::FEATURE_R2R:
        for (int i = 0; i < cfg.streams.size(); ++i) {
            if (cfg.streams[i].streamType == StreamType::INPUT) {
                switch (cfg.streams[i].format) {
                case NSCam::eImgFmt_BAYER10:
                case NSCam::eImgFmt_BAYER12_UNPAK:
                case NSCam::eImgFmt_BAYER16_UNPAK:
                case NSCam::eImgFmt_RAW16:
                case NSCam::eImgFmt_NV21:
                case NSCam::eImgFmt_NV12:
                case NSCam::eImgFmt_MTK_YUV_P010:
                    entry.push_back(static_cast<MINT64>(cfg.streams[i].id),
                                    NSCam::Type2Type<MINT64>());
                    entry.push_back(
                        static_cast<MINT64>(MTK_POSTPROCDEV_CAPTURE_STREAM_USAGE_MAIN_INPUT),
                        NSCam::Type2Type<MINT64>());
                    break;
                default:
                    MLOGW(LogGroupType, "do not support format:%d", cfg.streams[i].format);
                    break;
                }
            } else if (cfg.streams[i].streamType == StreamType::OUTPUT) {
                switch (cfg.streams[i].format) {
                case NSCam::eImgFmt_JPEG:
                    int32_t setYuvDirectJpeg = 0x03;
                    IMetadata::setEntry<MINT32>(&cfg.sessionParams,
                                                MTK_POSTPROCDEV_CAPTURE_YUV_DIRECT_JPEG,
                                                setYuvDirectJpeg);
                    MLOGD(LogGroupType, "YUV will be set to JPEG directly");
                    break;
                }
            }
        }
        break;
    case CapFeatureType::FEATURE_AISHUTTER2:
    case CapFeatureType::FEATURE_AISHUTTER1:
    case CapFeatureType::FEATURE_AIHDR:
    case CapFeatureType::FEATURE_AINR:
    case CapFeatureType::FEATURE_MFNR:
        for (int i = 0; i < cfg.streams.size(); ++i) {
            if (cfg.streams[i].streamType == StreamType::INPUT) {
                switch (cfg.streams[i].format) {
                case NSCam::eImgFmt_MTK_YUV_P010:
                    entry.push_back(static_cast<MINT64>(cfg.streams[i].id),
                                    NSCam::Type2Type<MINT64>());
                    entry.push_back(
                        static_cast<MINT64>(MTK_POSTPROCDEV_CAPTURE_STREAM_USAGE_YUV_R3_INPUT),
                        NSCam::Type2Type<MINT64>());
                    break;
                case NSCam::eImgFmt_BAYER10:
                case NSCam::eImgFmt_BAYER16_UNPAK:
                case NSCam::eImgFmt_NV21:
                case NSCam::eImgFmt_NV12:
                    entry.push_back(static_cast<MINT64>(cfg.streams[i].id),
                                    NSCam::Type2Type<MINT64>());
                    entry.push_back(
                        static_cast<MINT64>(MTK_POSTPROCDEV_CAPTURE_STREAM_USAGE_MAIN_INPUT),
                        NSCam::Type2Type<MINT64>());
                    break;
                default:
                    MLOGW(LogGroupType, "do not support format:%d", cfg.streams[i].format);
                    break;
                }
            }
        }
        break;
    default:
        break;
    }
    // reset "ispMetaEnable"
    IMetadata::setEntry<uint8_t>(&cfg.sessionParams, MTK_CONTROL_CAPTURE_ISP_TUNING_DATA_ENABLE, 0);
    cfg.sessionParams.update(MTK_POSTPROCDEV_CAPTURE_STREAM_USAGE_MAP, entry);
    MLOGD(LogGroupType, "add streamConfiguration sessionParams meta");
    return 0;
}

int MiCaptureEngine::request(std::vector<CaptureRequest> &requests)
{
    switch (mInfos.capFeatureType) {
    case CapFeatureType::FEATURE_AISHUTTER2:
    case CapFeatureType::FEATURE_AISHUTTER1:
    case CapFeatureType::FEATURE_AIHDR:
    case CapFeatureType::FEATURE_AINR:
    case CapFeatureType::FEATURE_MFNR:
        for (auto &mainReq : requests) {
            int32_t feature = 0;
            int32_t frameCnt = mInfos.multiFrameNum;
            switch (mInfos.capFeatureType) {
            case CapFeatureType::FEATURE_MFNR:
                feature = MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_MFNR;
                break;
            case CapFeatureType::FEATURE_AINR:
                feature = MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AINR;
                break;
            case CapFeatureType::FEATURE_AIHDR:
                feature = MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AIHDR;
                break;
            case CapFeatureType::FEATURE_AISHUTTER1:
                feature = MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AISHUTTER1;
                break;
            case CapFeatureType::FEATURE_AISHUTTER2:
                feature = MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AISHUTTER2;
                break;
            default:
                MLOGW(LogGroupType, "do not support type:%d", mInfos.capFeatureType);
                break;
            }

            // add metadata
            IMetadata::setEntry<MINT32>(&mainReq.settings, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_COUNT,
                                        frameCnt);
            IMetadata::setEntry<MINT32>(&mainReq.settings, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_INDEX,
                                        0);
            IMetadata::setEntry<MINT32>(&mainReq.settings,
                                        MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_FEATURE, feature);
            IMetadata::setEntry(&mainReq.settings, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 0);

            std::vector<SubRequest> &subRequests = mainReq.subRequests;
            for (int32_t i = 0; i < subRequests.size(); ++i) {
                SubRequest &subReq = subRequests[i];
                subReq.subFrameIndex = i + 1;

                IMetadata::setEntry<MINT32>(&subReq.settings,
                                            MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_COUNT, frameCnt);
                IMetadata::setEntry<MINT32>(&subReq.settings,
                                            MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_INDEX, i + 1);
                IMetadata::setEntry<MINT32>(
                    &subReq.settings, MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_FEATURE, feature);
                IMetadata::setEntry(&subReq.settings, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 0);
            }
        }
        break;
    default:
        for (auto &mainReq : requests) {
            IMetadata::setEntry(&mainReq.settings, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 0);
        }
        break;
    }
    MLOGD(LogGroupType, "add request settings meta");
    return 0;
}

MiCaptureEngine::~MiCaptureEngine()
{
    MLOGD(LogGroupType, "~MiCaptureEngine");
}

void MiCaptureEngine::destroy() {}
/**********************Capture Engine*****************************/

} // namespace mipostprocinterface
} // namespace mizone
