/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiAlgoCamMode.h"

#include "AlgoEngine.h"
#include "AppSnapshotController.h"
#include "DecoupleUtil.h"
#include "MiBufferManager.h"
#include "MiCamDecision.h"
#include "MiCamTrace.h"
#include "MiVendorCamera.h"
#include "MiZoneTypes.h"
#include "MialgoTaskMonitor.h"
#include "utils.h"
// TODO: include for XIAOMI_CAMERA_ID_ALIAS, should redefine in decouple utils
#include <MiDebugUtils.h>

#include <thread>

#include "BGService.h"
#include "MiCamJsonUtils.h"
#include "MiCamMonitor.h"
#include "mtkcam-halif/utils/metadata_tag/1.x/custom/custom_metadata_tag.h"
#include "mtkcam-halif/utils/metadata_tag/1.x/mtk_metadata_tag.h"
#include "mtkcam-interfaces/pipeline/utils/packutils/PackUtils_v2.h"
#define MAX_QUICKVIEWBUFFER_SIZE 20
#define MEMORY_BUF_GB            (1024 * 1024 * 512)

using namespace vendor::xiaomi::hardware::bgservice::implementation;
using namespace midebug;

#define MZ_LOGV(group, fmt, arg...) MLOGV(group, "[%d]" fmt, mCameraInfo.cameraId, ##arg)

#define MZ_LOGD(group, fmt, arg...) MLOGD(group, "[%d]" fmt, mCameraInfo.cameraId, ##arg)

#define MZ_LOGI(group, fmt, arg...) MLOGI(group, "[%d]" fmt, mCameraInfo.cameraId, ##arg)

#define MZ_LOGW(group, fmt, arg...) MLOGW(group, "[%d]" fmt, mCameraInfo.cameraId, ##arg)

#define MZ_LOGE(group, fmt, arg...) MLOGE(group, "[%d]" fmt, mCameraInfo.cameraId, ##arg)

#define MZ_LOGF(group, fmt, arg...)                            \
    do {                                                       \
        MLOGE(group, "[%d]" fmt, mCameraInfo.cameraId, ##arg); \
        MASSERT(false);                                        \
    } while (0)

namespace mizone {

MiAlgoCamMode::MiAlgoCamMode(const CreateInfo &info)
    : mStaticInfo(info.staticInfo),
      mDeviceCallback(info.deviceCallback),
      mDeviceSession(info.deviceSession),
      mPhysicalMetadataMap(info.physicalMetadataMap),
      mExitReprocessedResultThread(false),
      mPauseReprocessedCallback(false),
      mVndFrameNum(0),
      mMiZoneBufferCanHandleNextShot(true),
      mIsRetired(false),
      mShot2ShotTimediff(0),
      mPreSnapshotTime(0),
      mPendingMiHALSnapCount(0),
      mPendingVendorHALSnapCount(0),
      mPendingVendorMFNRSnapCount(0)
{
    mCameraInfo.cameraId = info.cameraId;
    MZ_LOGI(LogGroupCore, "ctor +");
    // Each stream 512M memory default
    MiCamJsonUtils::getVal("CaptureBufferThreshold", mThreshold, MEMORY_BUF_GB);
    MiCamJsonUtils::getVal("MialgoTaskConsumption.MialgoCapacity", mMialgoSessionCapacity, 6);

    initCameraInfo(info);

    startResultThread();
    startReprocessedResultThread();
    startQuickviewResultThread();
    mPerfLock = PerfManager::getInstance()->CreatePerfLock();
    if (mPerfLock == NULL) {
        MZ_LOGE(LogGroupCore, "PerfLock create fail");
    }

    // enbale psiMonitorLoop
    PsiMemMonitor::getInstance()->psiMonitorTrigger();

    MZ_LOGI(LogGroupCore, "ctor -");
}

void MiAlgoCamMode::initCameraInfo(const CreateInfo &info)
{
    mCameraInfo.cameraId = info.cameraId;
    MZ_LOGI(LogGroupCore, "cameraId = %d", mCameraInfo.cameraId);

    mCameraInfo.supportCoreHBM = info.supportCoreHBM;
    mCameraInfo.supportAndroidHBM = info.supportAndroidHBM;
    MZ_LOGD(LogGroupCore, "supportCoreHBM = %d, supportAndroidHBM = %d", mCameraInfo.supportCoreHBM,
            mCameraInfo.supportAndroidHBM);

    auto entry = mStaticInfo.entryFor(MTK_REQUEST_AVAILABLE_CAPABILITIES);
    if (!entry.isEmpty()) {
        for (uint32_t i = 0; i < entry.count(); ++i) {
            if (entry.itemAt(i, Type2Type<uint8_t>()) ==
                MTK_REQUEST_AVAILABLE_CAPABILITIES_OFFLINE_PROCESSING) {
                mCameraInfo.supportOffline = true;
                break;
            }
        }
    }
    MZ_LOGD(LogGroupCore, "supportOffline = %d", mCameraInfo.supportOffline);

    // find active array from staticInfo
    // ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE
    auto activeArraySizeEntry = mStaticInfo.entryFor(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE);
    if (activeArraySizeEntry.count() > 0) {
        MZ_LOGD(LogGroupCore, "activeArraySizeEntry.count: %d", activeArraySizeEntry.count());
        const auto *array = static_cast<const int32_t *>(activeArraySizeEntry.data()); // x, y, w, h
        mCameraInfo.activeArraySize.width = array[2];
        mCameraInfo.activeArraySize.height = array[3];
        MZ_LOGD(LogGroupCore, "activeArraySize: %d, %d", array[2], array[3]);
    } else {
        MZ_LOGE(LogGroupCore, "entry ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE is empty!");
    }

    // find roleId
    // TODO: XIAOMI_CAMERA_ID_ALIAS should redefine in decouple utils
    mCameraInfo.roleId = mCameraInfo.cameraId;
    auto roleIdEntry = mStaticInfo.entryFor(XIAOMI_CAMERA_ID_ALIAS);
    if (roleIdEntry.count() > 0) {
        MZ_LOGD(LogGroupCore, "roleIdEntry count: %d", roleIdEntry.count());
        mCameraInfo.roleId = *reinterpret_cast<const int32_t *>(roleIdEntry.data());
        MZ_LOGD(LogGroupCore, "cameraId: %d, roleId: %d", mCameraInfo.cameraId, mCameraInfo.roleId);
    } else {
        MZ_LOGE(LogGroupCore, "entry XIAOMI_CAMERA_ID_ALIAS isEmpty!");
    }

    // find physicalIds
    // ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS
    std::string sensorListString;
    auto physicalIdsEntry = mStaticInfo.entryFor(ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS);

    if (physicalIdsEntry.isEmpty()) {
        MZ_LOGE(LogGroupCore, "entry ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS is empty!");
    }
    MZ_LOGD(LogGroupCore, "physical ids entry count: %d", physicalIdsEntry.count());
    if (physicalIdsEntry.count() == 0) {
        mCameraInfo.physicalCameraIds.push_back(mCameraInfo.cameraId);
    } else {
        sensorListString = std::string(reinterpret_cast<const char *>(physicalIdsEntry.data()),
                                       physicalIdsEntry.count());
    }
    std::istringstream iss(sensorListString);
    for (std::string token; std::getline(iss, token, '\0');) {
        mCameraInfo.physicalCameraIds.push_back(std::stoi(token));
        MZ_LOGD(LogGroupCore, "parse physical id: %d", std::stoi(token));
    }
    MZ_LOGD(LogGroupCore, "physical ids: %s",
            utils::toString(mCameraInfo.physicalCameraIds).c_str());

    mCameraInfo.staticInfo = mStaticInfo;
    mCameraInfo.physicalMetadataMap = mPhysicalMetadataMap;
}

MiAlgoCamMode::~MiAlgoCamMode()
{
    MZ_LOGI(LogGroupCore, "dtor +");

    if (!mInflightVndRequests.empty()) {
        MZ_LOGE(LogGroupCore, "mInflightVndRequests.size: %lu", mInflightVndRequests.size());
    }

    exitAllThreads();

    releaseAllQuickViewBuffers();

    if (mPerfLock) {
        mPerfLock->ReleasePerfLock();
        mPerfLock->Destory();
    }

    MZ_LOGI(LogGroupCore, "dtor -");
}

void MiAlgoCamMode::releaseAllQuickViewBuffers()
{
    std::vector<StreamBuffer> buffers;
    buffers.reserve(mQuickViewBuffers.size());
    for (auto &&buf : mQuickViewBuffers) {
        buffers.emplace_back(buf.second);
    }
    if (mBufferManager) {
        mBufferManager->releaseBuffers(buffers);
    }
    mQuickViewBuffers.clear();
    mQuickViewFwkNumToShutter.clear();
    mAnchorFrames.clear();
    mQuickViewShutterToFwkNum.clear();
}

bool MiAlgoCamMode::isPreviewRequest(const CaptureRequest &req)
{
    for (auto &&buffer : req.outputBuffers) {
        if (mFwkConfig->streams[buffer.streamId]->usageType == StreamUsageType::PreviewStream) {
            return true;
        }
    }
    return false;
}

bool MiAlgoCamMode::isSnapshotRequest(const CaptureRequest &req)
{
    for (auto &&buffer : req.outputBuffers) {
        if (mFwkConfig->streams[buffer.streamId]->usageType == StreamUsageType::SnapshotStream) {
            return true;
        }
    }
    return false;
}

bool MiAlgoCamMode::isPreviewStream(const Stream &stream)
{
    auto usage = static_cast<uint64_t>(stream.usage);

    return (stream.streamType == StreamType::OUTPUT &&
            stream.format == EImageFormat::eImgFmt_NV21 &&
            ((usage & eBUFFER_USAGE_HW_TEXTURE) != 0)) ||
           ((usage & eBUFFER_USAGE_HW_COMPOSER) != 0);
}

bool MiAlgoCamMode::isSnapshotStream(const Stream &stream)
{
    auto usage = static_cast<uint64_t>(stream.usage);

    // NOTE: format Y16 is for bokeh depth buffer
    return stream.streamType == StreamType::OUTPUT &&
           (stream.format == EImageFormat::eImgFmt_BLOB ||
            stream.format == EImageFormat::eImgFmt_NV21 ||
            stream.format == EImageFormat::eImgFmt_Y16) &&
           ((usage ^ (eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_SW_WRITE_OFTEN |
                      eBUFFER_USAGE_HW_CAMERA_WRITE)) == 0);
}

bool MiAlgoCamMode::isSnapshotRawStream(const Stream &stream)
{
    auto usage = static_cast<uint64_t>(stream.usage);

    return stream.streamType == StreamType::OUTPUT &&
           (stream.format == EImageFormat::eImgFmt_RAW16 ||
            stream.format == EImageFormat::eImgFmt_RAW10) &&
           ((usage ^ (eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_SW_WRITE_OFTEN |
                      eBUFFER_USAGE_HW_CAMERA_WRITE)) == 0);
}

bool MiAlgoCamMode::isTuningDataStream(const Stream &stream)
{
    auto usage = static_cast<uint64_t>(stream.usage);

    return stream.streamType == StreamType::OUTPUT &&
           (stream.format == EImageFormat::eImgFmt_YV12) &&
           ((usage ^ (eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_SW_WRITE_OFTEN |
                      eBUFFER_USAGE_HW_CAMERA_WRITE)) == 0);
}

bool MiAlgoCamMode::isStreamSupportOffline(int32_t streamId)
{
    if (!mCameraInfo.supportOffline) {
        return false;
    }

    // NOTE: could determined by more stream info (format, usage, dataSpace...) if necessary,
    //       not limited to StreamUsageType
    // only snapshot / tuning data streams support offline
    auto usageType = mFwkConfig->streams[streamId]->usageType;
    return usageType == StreamUsageType::SnapshotStream ||
           usageType == StreamUsageType::TuningDataStream;
}

void MiAlgoCamMode::prepareFwkConfig(const StreamConfiguration &requestedConfiguration)
{
    // create MiConfig
    mFwkConfig = std::make_shared<MiConfig>();
    mFwkConfig->rawConfig = requestedConfiguration;
    std::vector<int32_t> candiateQuickViewIds;
    int32_t previewWidth, previewHeight;
    for (auto &&stream : requestedConfiguration.streams) {
        auto miStream = std::make_shared<MiStream>();
        miStream->rawStream = stream;
        miStream->streamId = stream.id;
        miStream->owner = StreamOwner::StreamOwnerFwk;
        if (isPreviewStream(stream)) {
            miStream->usageType = StreamUsageType::PreviewStream;
            previewWidth = stream.width;
            previewHeight = stream.height;
            MZ_LOGD(LogGroupCore, "stream (%d) is preview", stream.id);
        } else if (isSnapshotRawStream(stream)) {
            miStream->usageType = StreamUsageType::SnapshotStream;
            MZ_LOGD(LogGroupCore, "stream (%d) is snapshot raw", stream.id);
        } else if (isSnapshotStream(stream)) {
            miStream->usageType = StreamUsageType::SnapshotStream;
            if (stream.width == previewWidth && stream.height == previewHeight &&
                stream.format == eImgFmt_NV21) {
                candiateQuickViewIds.push_back(stream.id);
            }
            MZ_LOGD(LogGroupCore, "stream (%d) is snapshot", stream.id);
        } else if (isTuningDataStream(stream)) {
            miStream->usageType = StreamUsageType::TuningDataStream;
            MZ_LOGD(LogGroupCore, "stream (%d) is tuning data", stream.id);
        } else {
            miStream->usageType = StreamUsageType::UnkownStream;
            MZ_LOGD(LogGroupCore, "stream (%d) is unkown", stream.id);
        }
        mFwkConfig->streams[miStream->streamId] = miStream;
    }
    // find quick view stream
    // NOTE: the format and usage of quick view stream are identical to snapshot stream,
    //       so we could only find quick view stream by their size.
    // TODO: may have better way to find quick view stream
    if (candiateQuickViewIds.size() > 0) {
        int32_t quickViewId = candiateQuickViewIds.front();
        int32_t qrCodeId = candiateQuickViewIds.back();
        mFwkConfig->streams[quickViewId]->usageType = StreamUsageType::QuickViewStream;
        if (candiateQuickViewIds.size() > 1) {
            mFwkConfig->streams[qrCodeId]->usageType = StreamUsageType::QrStream;
        }
        MZ_LOGD(LogGroupCore, "stream (%d) is quickview, stream (%d) is qrCode", quickViewId,
                qrCodeId);
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// External Interface (for MiCamSession)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int MiAlgoCamMode::configureStreams(const StreamConfiguration &requestedConfiguration,
                                    HalStreamConfiguration &halConfiguration)
{
    MZ_LOGD(LogGroupCore, "+");

    // Init HDR/SE status
    AppSnapshotController::getInstance()->UpdateHdrStatus(false, 0);
    AppSnapshotController::getInstance()->UpdateSEStatus(false, 0);

    prepareFwkConfig(requestedConfiguration);

    MZ_LOGD(LogGroupCore, "fwk config: %s", utils::toString(mFwkConfig->rawConfig).c_str());
    for (auto &&stream : mFwkConfig->rawConfig.streams) {
        MZ_LOGD(LogGroupCore, "fwk config stream(%d): %s", stream.id,
                utils::toString(stream).c_str());
    }

    // create MiCamDecision and build vendor config
    MiCamDecision::CreateInfo decCreateInfo;
    decCreateInfo.camMode = this;
    decCreateInfo.configFromFwk = mFwkConfig;
    decCreateInfo.cameraInfo = mCameraInfo;
    mDecision = MiCamDecision::createDecision(decCreateInfo);

    mVndConfig = mDecision->buildMiConfig();
    if (mVndConfig == nullptr) {
        MZ_LOGE(LogGroupCore, "buildMiConfig from MiDecision failed: mVndConfig is nullptr!");
        return Status::INTERNAL_ERROR;
    }

    MZ_LOGD(LogGroupCore, "vnd config: %s", utils::toString(mVndConfig->rawConfig).c_str());
    for (auto &&stream : mVndConfig->rawConfig.streams) {
        MZ_LOGD(LogGroupCore, "vnd config stream(%d): %s", stream.id,
                utils::toString(stream).c_str());
    }

    MiBufferManager::BufferMgrCreateInfo info{
        .fwkConfig = mFwkConfig->rawConfig,
        .vendorConfig = mVndConfig->rawConfig,
        .supportCoreHBM = mCameraInfo.supportCoreHBM,
        .supportAndroidHBM = mCameraInfo.supportAndroidHBM,
        .camMode = this,
        .callback = [this](MiBufferManager::MemoryInfoMap memoryInfo) -> void {
            onMemoryChanged(std::move(memoryInfo));
        },
        .devCallback = mDeviceCallback,
    };
    mBufferManager = mBufferManager->create(info);
    if (mBufferManager == nullptr) {
        MZ_LOGE(LogGroupCore, "mBufferManager is nullptr");
    }

    AlgoEngine::CreateInfo engineCreateInfo{
        .mode = this,
        .cameraInfo = mCameraInfo,
    };
    mAlgoEngine = AlgoEngine::create(engineCreateInfo);
    if (mAlgoEngine == nullptr) {
        MZ_LOGE(LogGroupCore, "mAlgoEngine is nullptr");
    }

    // forward config to Core Session
    // TODO: could we generate halConfiguration here rather than config fwkconfig to Core Session?
    // combine the fwk config and vnd config for generate correct halConfiguration
#if 1

    // configureStreams to core session
    HalStreamConfiguration vndHalConfiguration;
    auto status = mDeviceSession->configureStreams(mVndConfig->rawConfig, vndHalConfiguration);
    MZ_LOGD(LogGroupCore, "vnd hal config: %s", utils::toString(vndHalConfiguration).c_str());
    for (auto &&stream : vndHalConfiguration.streams) {
        MZ_LOGD(LogGroupCore, "vnd hal config stream(%d): %s", stream.id,
                utils::toString(stream).c_str());
    }

    // fill halConfiguration returned to fwk
    halConfiguration.sessionResults = vndHalConfiguration.sessionResults;
    halConfiguration.streams.clear();
    for (auto &&elem : mFwkConfig->streams) {
        auto &&streamId = elem.first;
        auto &&stream = elem.second;

        auto it =
            std::find_if(vndHalConfiguration.streams.begin(), vndHalConfiguration.streams.end(),
                         [streamId](const auto &halStream) { return streamId == halStream.id; });
        if (it != vndHalConfiguration.streams.end()) {
            // for those contained in vnd config, push them directly
            halConfiguration.streams.emplace_back(*it);
        } else {
            // for those not contained in vnd config, fill them manually
            HalStream halStream;
            halStream.id = streamId;
            halStream.producerUsage = stream->rawStream.usage;
            halStream.consumerUsage = 0;
            halStream.maxBuffers = 12;
            halStream.overrideDataSpace = stream->rawStream.dataSpace;
            halStream.overrideFormat = stream->rawStream.format;
            halStream.physicalCameraId = stream->rawStream.physicalCameraId;

            halConfiguration.streams.emplace_back(halStream);
        }
    }

    // only snapshot stream support offline
    for (auto &&halStream : halConfiguration.streams) {
        halStream.supportOffline = isStreamSupportOffline(halStream.id);
    }

    MZ_LOGD(LogGroupCore, "fwk hal config: %s", utils::toString(halConfiguration).c_str());
    for (auto &&stream : halConfiguration.streams) {
        MZ_LOGD(LogGroupCore, "fwk hal config stream(%d): %s", stream.id,
                utils::toString(stream).c_str());
    }

#else

    // 1st configureStreams only for generate correct halConfiguration
    // TODO: need handle halConfiguration in here
    auto status = mDeviceSession->configureStreams(mFwkConfig->rawConfig, halConfiguration);
    for (auto &&halStream : halConfiguration.streams) {
        halStream.supportOffline = isSupportOffline(halStream.id);
    }
    MZ_LOGD(LogGroupCore, "fwk hal config: %s", utils::toString(halConfiguration).c_str());
    for (auto &&stream : halConfiguration.streams) {
        MZ_LOGD(LogGroupCore, "fwk hal config stream(%d): %s", stream.id,
                utils::toString(stream).c_str());
    }

    // 2nd configureStreams is real vnd config
    HalStreamConfiguration vndHalConfiguration;
    for (auto it = mVndConfig->rawConfig.streams.begin();
         it != mVndConfig->rawConfig.streams.end();) {
        if (it->format == eImgFmt_RAW16) {
            it = mVndConfig->rawConfig.streams.erase(it);
        } else {
            ++it;
        }
    }
    status = mDeviceSession->configureStreams(mVndConfig->rawConfig, vndHalConfiguration);
    MZ_LOGD(LogGroupCore, "vnd hal config: %s", utils::toString(vndHalConfiguration).c_str());
    for (auto &&stream : vndHalConfiguration.streams) {
        MZ_LOGD(LogGroupCore, "vnd hal config stream(%d): %s", stream.id,
                utils::toString(stream).c_str());
    }

#endif

    MZ_LOGD(LogGroupCore, "- status = %d", status);
    return status;
}

int MiAlgoCamMode::processCaptureRequest(const std::vector<CaptureRequest> &requests,
                                         uint32_t &numRequestProcessed)
{
    MZ_LOGD(LogGroupCore, "+");
    numRequestProcessed = requests.size();
    for (auto req : requests) {
        // NOTE: sort output buffers by stream id, because some algorithms (like bokeh)
        //       depends on this order.
        {
            std::sort(req.outputBuffers.begin(), req.outputBuffers.end(),
                      [](auto &&a, auto &&b) { return a.streamId < b.streamId; });
        }

        {
            // TODO: may not need in future
            //       app will send snapshot request with a preview buffer,
            //       remove it from req and mark as error buffer return early
            bool containSnapshot = false;
            bool containPreviewBuffer = false;
            std::vector<StreamBuffer> snapshotBuffers;
            std::vector<StreamBuffer> previewBuffers;
            for (auto &&buffer : req.outputBuffers) {
                if (mFwkConfig->streams[buffer.streamId]->usageType ==
                    StreamUsageType::SnapshotStream) {
                    mStreamToFwkNumber[buffer.streamId].insert({req.frameNumber, 0});
                    containSnapshot = true;
                }
                if (mFwkConfig->streams[buffer.streamId]->usageType ==
                    StreamUsageType::TuningDataStream) {
                    mStreamToFwkNumber[buffer.streamId].insert({req.frameNumber, 0});
                }
                if (mFwkConfig->streams[buffer.streamId]->usageType ==
                    StreamUsageType::PreviewStream) {
                    containPreviewBuffer = true;
                    previewBuffers.push_back(buffer);
                } else {
                    snapshotBuffers.push_back(buffer);
                }
            }

            // if (containSnapshot && containPreviewBuffer) {
            //     MZ_LOGI(LogGroupCore, "contain snapshot and preview buffers in a request: %s",
            //           utils::toString(req).c_str());
            //     req.outputBuffers = snapshotBuffers;

            //     CaptureResult errorResult;
            //     errorResult.frameNumber = req.frameNumber;
            //     for (auto buffer : previewBuffers) {
            //         buffer.status = BufferStatus::ERROR;
            //         errorResult.outputBuffers.emplace_back(buffer);
            //     }
            //     MZ_LOGI(LogGroupCore, "return error buffer: %s",
            //           utils::toString(errorResult).c_str());
            //     mDeviceCallback->processCaptureResult({errorResult});
            // }
        }

        // AppSnapshotController
        bool isSnapShotRequest = isSnapshotRequest(req);
        std::map<int32_t, int32_t> buffersBeforeSnapshot;
        if (isSnapShotRequest) {
            std::unique_lock<std::mutex> locker{mRecordMutex};
            std::bitset<SnapMessageType::HAL_MESSAGE_COUNT> recorder;
            mPendingSnapshotRecord.insert({req.frameNumber, recorder});
            locker.unlock();
            calShot2shotTimeDiff();
            mPendingMiHALSnapCount.fetch_add(1);
            mPendingVendorHALSnapCount.fetch_add(1);
            for (const auto &stream : mVndConfig->streams) {
                if (stream.second->usageType == StreamUsageType::SnapshotStream) {
                    uint32_t streamId = stream.second->streamId;
                    if (mBuffersConsumed.find(streamId) == mBuffersConsumed.end()) {
                        buffersBeforeSnapshot[streamId] =
                            mBufferManager->queryMemoryUsedByStream(streamId);
                    }
                }
            }

            MZ_LOGI(LogGroupCore, "mizone capture request begint to boost");
            mPerfLock->AcquirePerfLock(5000);

            // PsiMemMonitor 记录拍照数加一
            PsiMemMonitor::getInstance()->snapshotAdd();
        }

        if (!isSnapShotRequest) {
            mLatestPreviewRequest = req;
        }

        // 0. build MiCapture through MiCamDecision
        MICAM_TRACE_SYNC_BEGIN_F(MialgoTraceCapture, "buildMiCapture|req:%d", req.frameNumber);
        auto capture = mDecision->buildMiCapture(req);
        if (capture == nullptr) {
            MZ_LOGE(LogGroupCore, "buildMiCapture failed: capture is nullptr!");
            returnErrorRequest(req);
            continue;
        }
        if (isSnapShotRequest && (capture->getmCaptureTypeName().find("MI_ALGO_VENDOR_MFNR") ||
                                  capture->getmCaptureTypeName().find("MI_ALGO_VENDOR_MFNR_HDR"))) {
            addPendingVendorMFNR();
        }
        MICAM_TRACE_SYNC_END(MialgoTraceCapture);

        // 1. create FwkRequestRecord and insert to mInflightFwkRequests
        auto fwkRequestRecord = std::make_shared<FwkRequestRecord>(req);
        {
            std::lock_guard<std::mutex> lock(mInflightFwkRequestsMutex);
            mInflightFwkRequests.emplace(req.frameNumber, std::move(fwkRequestRecord));
        }

        MZ_LOGD(LogGroupCore, "fwk req: %s", utils::toString(req).c_str());

        // 2. insert to mInflightVndRequests
        auto miRequests = capture->getMiRequests();
        {
            std::lock_guard<std::mutex> lock(mInflightVndRequestsMutex);
            for (auto &&[frameNum, req] : miRequests) {
                mInflightVndRequests[frameNum] = capture;
            }
        }

        // 3. forward request to MiCapture
        auto status = capture->processVendorRequest();

        if (status != 0) {
            // TODO: how to handle process error from Core Session?
            MZ_LOGE(LogGroupCore, "Core Session process error, status: %d", status);
        }

        // AppSnapshotController
        if (isSnapShotRequest) {
            std::map<int32_t, int32_t> buffersAfterSnapshot;
            std::map<uint32_t, int32_t> streamIds;
            for (auto &&[frameNum, req] : miRequests) {
                for (auto &&buffer : req->vndRequest.outputBuffers) {
                    streamIds[buffer.streamId]++;
                }
            }
            for (auto &&[streamId, nums] : streamIds) {
                if (mVndConfig->streams[streamId]->usageType == StreamUsageType::SnapshotStream) {
                    buffersAfterSnapshot[streamId] =
                        mBufferManager->queryMemoryUsedByStream(streamId);
                    if (mBuffersConsumed.find(streamId) == mBuffersConsumed.end()) {
                        if ((buffersAfterSnapshot[streamId] - buffersBeforeSnapshot[streamId]) >
                            0) {
                            mBuffersConsumed[streamId] =
                                buffersAfterSnapshot[streamId] - buffersBeforeSnapshot[streamId];
                            mOneBuffersConsumed[streamId] =
                                mBuffersConsumed[streamId] / miRequests.size();
                        }
                    } else {
                        mBuffersConsumed[streamId] = mOneBuffersConsumed[streamId] * nums;
                    }
                }
            }
            onMemoryChanged(buffersAfterSnapshot);
        }
    }
    MZ_LOGD(LogGroupCore, "-");
    return Status::OK;
}

void MiAlgoCamMode::processCaptureResult(const std::vector<CaptureResult> &results)
{
    MZ_LOGD(LogGroupCore, "+");

    for (auto &&res : results) {
        MZ_LOGD(LogGroupCore, "vnd res: %s", utils::toString(res).c_str());

        MiResult miResult;
        miResult.frameNum = res.frameNumber;
        miResult.result = std::make_shared<CaptureResult>(res);
        // find MiCapture by vnd frame num from mInflightVndRequests
        {
            std::lock_guard<std::mutex> lock(mInflightVndRequestsMutex);
            auto it = mInflightVndRequests.find(miResult.frameNum);
            if (it != mInflightVndRequests.end()) {
                miResult.capture = it->second;
            } else {
                MZ_LOGE(LogGroupCore, "result (%d) could not find in mInflightVndRequests!",
                        miResult.frameNum);
                for (auto &&buffer : res.outputBuffers) {
                    if (mVndConfig->streams[buffer.streamId]->owner ==
                        StreamOwner::StreamOwnerVnd) {
                        std::vector<StreamBuffer> streamBuffers{buffer};
                        mBufferManager->releaseBuffers(streamBuffers);
                    }
                }
                continue;
            }
        }

        // enque to mPendingVndResults
        {
            std::lock_guard<std::mutex> lock(mResultsMutex);
            MZ_LOGD(LogGroupCore, "before enque, mPendingVndResults: %lu",
                    mPendingVndResults.size());
            mPendingVndResults.push_back(miResult);
            MZ_LOGD(LogGroupCore, "after enque, mPendingVndResults: %lu",
                    mPendingVndResults.size());
        }
        mResultCond.notify_one();
    }

    MZ_LOGD(LogGroupCore, "-");
}

void MiAlgoCamMode::notify(const std::vector<NotifyMsg> &msgs)
{
    MZ_LOGD(LogGroupCore, "+");

    for (auto &&msg : msgs) {
        MZ_LOGD(LogGroupCore, "vnd msg: %s", utils::toString(msg).c_str());

        MiResult miResult;
        miResult.frameNum = -1;

        if (msg.type == MsgType::SHUTTER) {
            miResult.frameNum = msg.msg.shutter.frameNumber;
        } else if (msg.type == MsgType::ERROR) {
            miResult.frameNum = msg.msg.error.frameNumber;
        }

        miResult.msg = std::make_shared<NotifyMsg>(msg);
        // find MiCapture by vnd frame num from mInflightVndRequests
        {
            std::lock_guard<std::mutex> lock(mInflightVndRequestsMutex);
            auto it = mInflightVndRequests.find(miResult.frameNum);
            if (it != mInflightVndRequests.end()) {
                miResult.capture = it->second;
            } else {
                MZ_LOGE(LogGroupCore, "notify (%d) could not find in mInflightVndRequests!",
                        miResult.frameNum);
                continue;
            }
        }

        // enque to mPendingVndResults
        {
            std::lock_guard<std::mutex> lock(mResultsMutex);
            MZ_LOGD(LogGroupCore, "before enque, mPendingVndResults: %lu",
                    mPendingVndResults.size());
            mPendingVndResults.push_back(miResult);
            MZ_LOGD(LogGroupCore, "after enque, mPendingVndResults: %lu",
                    mPendingVndResults.size());
        }
        mResultCond.notify_one();
    }

    MZ_LOGD(LogGroupCore, "-");
}

int MiAlgoCamMode::flush()
{
    MZ_LOGD(LogGroupCore, "+");

    // flush core session
    auto status = mDeviceSession->flush();
    MZ_LOGD(LogGroupCore, "core session flushed!");

    // exit result thread to ensure all result has return to fwk
    exitResultThread();
    exitAlgoEngineThread();

    mAlgoEngine->flush(true);

    // NOTE: HAL should keep the capacity of processing request after flush, so need restart
    //        result thread here.
    // TODO: or need an implenmentation to notify request / result thread just to flush rather than
    //       exit directly.

    startAlgoEngineThread();
    startResultThread();

    MZ_LOGD(LogGroupCore, "-");
    return status;
}

void MiAlgoCamMode::returnErrorRequest(const CaptureRequest &request)
{
    MZ_LOGD(LogGroupCore, "+: req (%d)", request.frameNumber);
    NotifyMsg msg{};
    msg.type = MsgType::ERROR;
    msg.msg.error.frameNumber = request.frameNumber;
    msg.msg.error.errorCode = ErrorCode::ERROR_REQUEST;
    msg.msg.error.errorStreamId = NO_STREAM;
    MZ_LOGD(LogGroupCore, "error fwk msg: %s", utils::toString(msg).c_str());
    mDeviceCallback->notify({msg});

    CaptureResult res;
    res.frameNumber = request.frameNumber;
    for (auto buffer : request.inputBuffers) {
        buffer.status = BufferStatus::ERROR;
        res.inputBuffers.emplace_back(buffer);
    }
    for (auto buffer : request.outputBuffers) {
        buffer.status = BufferStatus::ERROR;
        res.outputBuffers.emplace_back(buffer);
    }
    MZ_LOGD(LogGroupCore, "error fwk res: %s", utils::toString(res).c_str());
    mDeviceCallback->processCaptureResult({res});
}

int MiAlgoCamMode::close()
{
    MZ_LOGD(LogGroupCore, "+");

    int status = Status::OK;

    // NOTE: after switchToOffline, MiCamMode should not visit Core Session anymore.
    //       and in this case, it is MiCamSession's responsbility to close Core Session, rather than
    //       MiCamMode.
    if (mDeviceSession != nullptr) {
        status = mDeviceSession->close();
    }

    mAlgoEngine->flush();

    exitAllThreads();

    // For app control
    setRetired();

    MZ_LOGD(LogGroupCore, "-");
    return status;
}

// for HAL Buffer Manage (HBM)
// NOTE: HBM is implemented by MiBufferManager, MiCamMode only forwards HBM calls to it.
void MiAlgoCamMode::signalStreamFlush(const std::vector<int32_t> &streamIds,
                                      uint32_t streamConfigCounter)
{
    MZ_LOGD(LogGroupCore, "+");
    // NOTE: mDeviceSession and mBufferManager are both possible to hold additional buffers,
    //       nedd to forward signalStreamFlush to both to signal them to return buffers.
    mDeviceSession->signalStreamFlush(streamIds, streamConfigCounter);
    mBufferManager->signalStreamFlush(streamIds, streamConfigCounter);
    MZ_LOGD(LogGroupCore, "-");
}

void MiAlgoCamMode::requestStreamBuffers(const std::vector<BufferRequest> &bufReqs,
                                         requestStreamBuffers_cb cb)
{
    MZ_LOGD(LogGroupCore, "+");

    mBufferManager->requestStreamBuffers(bufReqs, cb);

    MZ_LOGD(LogGroupCore, "-");
}

void MiAlgoCamMode::returnStreamBuffers(const std::vector<StreamBuffer> &buffers)
{
    MZ_LOGD(LogGroupCore, "+");

    mBufferManager->returnStreamBuffers(buffers);

    MZ_LOGD(LogGroupCore, "-");
}

// for Offline Session
int MiAlgoCamMode::switchToOffline(const std::vector<int64_t> &streamsToKeep,
                                   CameraOfflineSessionInfo &offlineSessionInfo)
{
    MZ_LOGD(LogGroupCore, "+");

    offlineSessionInfo.offlineStreams.clear();
    offlineSessionInfo.offlineRequests.clear();

    MZ_LOGD(LogGroupCore, "streamsToKeep: %s", utils::toString(streamsToKeep).c_str());

    for (auto &&streamId : streamsToKeep) {
        if (!isStreamSupportOffline(static_cast<int32_t>(streamId))) {
            MZ_LOGE(LogGroupCore, "stream(id = %ld) is not support offline", streamId);
            return Status::ILLEGAL_ARGUMENT;
        }
    }

    // 1. wait snapshot data collected
    auto isWaitDataCollectedOk = waitSnapshotDataCollected();
    if (!isWaitDataCollectedOk) {
        MZ_LOGE(LogGroupCore, "waitSnapshotDataCollected timeout!");
    }

    // 2. flush Core Session to ensure any preview request in core session return
    mDeviceSession->flush();

    exitQuickviewResultThread();

    // check pending quickview here
    bool IsQuickViewReturn = true;
    {
        std::lock_guard<std::mutex> lock(mInflightFwkRequestsMutex);
        for (auto &&[fwkNumber, reqRecord] : mInflightFwkRequests) {
            for (auto &&[streamId, isPending] : reqRecord->pendingStreams) {
                if (isPending &&
                    mFwkConfig->streams[streamId]->usageType == StreamUsageType::QuickViewStream) {
                    IsQuickViewReturn = false;
                    addAnchorFrame(fwkNumber, fwkNumber - 1);
                    MZ_LOGI(LogGroupCore, "quickview stream isn't return");
                }
            }
        }
    }
    if (!IsQuickViewReturn && mAnchorFrames.size() > 0) {
        for (auto it = mAnchorFrames.begin(); it != mAnchorFrames.end();) {
            auto pendingFrameNum = it->first;
            if (mQuickViewBuffers.size() > 0) {
                auto pendingShutter = mQuickViewFwkNumToShutter[it->first];
                if (notifyQuickView(pendingFrameNum, pendingShutter)) {
                    MZ_LOGI(LogGroupCore,
                            "notifyQuickView pending succeed: pendingFrameNum = %d, pendingShutter "
                            "= %lu",
                            pendingFrameNum, pendingShutter);
                } else {
                    returnQuickViewError(pendingFrameNum);
                    MZ_LOGI(LogGroupCore, "return Error: pendingFrameNum = %d", pendingFrameNum);
                }
            } else {
                returnQuickViewError(pendingFrameNum);
                MZ_LOGI(LogGroupCore, "return Error: pendingFrameNum = %d", pendingFrameNum);
            }
            mAnchorFrames.erase(it++);
        }
    }

    // 3. exit result thread
    // result thread should process remaining result and then exit
    exitResultThread();
    exitAlgoEngineThread();

    // 4. signal AlgoEngine to request all buffers needed
    mAlgoEngine->switchToOffline();

    // TODO: before pause callback, may need to notify bufferManager to returnStreamBuffers,
    //       if bufferManager hold some buffers requested through requestStreamBuffers for cache.

    // 5. pause reprocessed result callback
    {
        std::lock_guard<std::mutex> lock(mPauseReprocessedCallbackMutex);
        mPauseReprocessedCallback = true;
    }
    MZ_LOGD(LogGroupCore, "reprocessed callback is paused!");
    // and pause HBM callback
    mBufferManager->pauseCallback();
    MZ_LOGD(LogGroupCore, "HBM callback is paused!");

    // 6. collect offlineSessionInfo
    // NOTE: all requests and their pending buffers in mInflightFwkRequests reach here
    //       are all need to switchToOffline
    collectOfflineSessionInfo(offlineSessionInfo);

    MZ_LOGD(LogGroupCore, "offlineSessionInfo: %s", utils::toString(offlineSessionInfo).c_str());
    MZ_LOGD(LogGroupCore, "mPendingVndResults: %lu", mPendingVndResults.size());
    MZ_LOGD(LogGroupCore, "mPendingReprocessedResults: %lu", mPendingReprocessedResults.size());

    // some resource could be early released here
    releaseAllQuickViewBuffers();

    // For app control
    setRetired();

    if (offlineSessionInfo.offlineRequests.empty()) {
        // NOTE: no requests needed to switch to offline session, MiCamMode should still keep the
        //       capacity of processing request, hence here:
        //       1. mDeviceSession could not be set to nullptr
        //       2.  result thread should be restarted
        startAlgoEngineThread();
        startResultThread();

    } else {
        // NOTE: after switchToOffline, MiCamMode should not visit Core Session anymore,
        //       set it as nullptr to avoid visitation.
        mDeviceSession = nullptr;
    }

    MZ_LOGD(LogGroupCore, "-");
    return Status::OK;
}

bool MiAlgoCamMode::waitSnapshotDataCollected()
{
    MZ_LOGD(LogGroupCore, "+");

    {
        std::lock_guard<std::mutex> lock(mInflightVndRequestsMutex);

        std::stringstream ss;
        ss << "[ ";
        for (auto &&[frameNum, capture] : mInflightVndRequests) {
            ss << "{ vndFrameNum(" << frameNum << "): ";
            ss << (capture->isSnapshot() ? "snapshot" : "preview");
            ss << "}, ";
        }
        ss << "]";
        MZ_LOGD(LogGroupCore, "mInflightVndRequests: %s", ss.str().c_str());

        // none snapshot in mInflightVndRequests means data collected
        // bool dataCollected = std::none_of(mInflightVndRequests.begin(),
        // mInflightVndRequests.end(),
        //        [](const std::pair<uint32_t, std::shared_ptr<MiCapture>> &p) -> bool {
        //    return p.second->isSnapshot();
        //});
        bool dataCollected = getPendingSnapCount() == 0;
        if (dataCollected) {
            MZ_LOGD(LogGroupCore, "- snapshot data already collected, no need to wait!");
            return true;
        }
    }

    // start a thread to continue send fake preview request to Core Session during
    // waitSnapshotDataCollected, to avoid ZslProcessor sellect failed due to no new frames
    std::atomic_bool exitFakeReqThread = false;
    std::thread fakeReqThread([&exitFakeReqThread, funcName = __FUNCTION__, this]() {
        const char *logPrefix = "fakeReqThread:";
        MZ_LOGD(LogGroupCore, "[%s]%s +", funcName, logPrefix);

        // find quick view cache stream, we use it as output buffer of fake request
        auto it = std::find_if(
            mVndConfig->streams.begin(), mVndConfig->streams.end(),
            [](auto &&p) { return p.second->usageType == StreamUsageType::QuickViewCachedStream; });
        if (it == mVndConfig->streams.end()) {
            MZ_LOGE(LogGroupCore, "[%s]%s not found QuickViewCachedStream!", funcName, logPrefix);
            return;
        }
        auto streamId = it->second->streamId;

        // NOTE: zsl selection has a timeout of 1000 ms
        //       set maxFakeRequestTimes to 45 (about 1500 ms under 30 fps) to cover the timeout
        const uint32_t maxFakeRequestTimes = 45;
        for (uint32_t i = 0; !exitFakeReqThread && i < maxFakeRequestTimes; i++) {
            auto fakePreviewReq = mLatestPreviewRequest;
            fakePreviewReq.frameNumber = mVndFrameNum.fetch_add(1);
            fakePreviewReq.outputBuffers.clear();

            MiBufferManager::MiBufferRequestParam param{
                .streamId = streamId,
                .bufferNum = 1,
            };
            std::map<int32_t, std::vector<StreamBuffer>> bufferMap;
            auto ok = mBufferManager->requestBuffers({param}, bufferMap);
            if (!ok || bufferMap.empty()) {
                MZ_LOGE(LogGroupCore, "[%s]%s could not request buffers form MiBufferManager!",
                        funcName, logPrefix);
                break;
            }
            fakePreviewReq.outputBuffers.push_back(bufferMap[streamId].front());

            MZ_LOGD(LogGroupCore, "[%s]%s fake req: %s", funcName, logPrefix,
                    utils::toString(fakePreviewReq).c_str());
            uint32_t numRequestProcessed = 0;
            auto status =
                mDeviceSession->processCaptureRequest({fakePreviewReq}, numRequestProcessed);
            if (status != Status::OK || numRequestProcessed == 0) {
                MZ_LOGE(LogGroupCore, "[%s]%s send fake preview request to Core Session failed!",
                        funcName, logPrefix);
                break;
            }
        }
        MZ_LOGD(LogGroupCore, "[%s]%s -", funcName, logPrefix);
    });

    bool ok = true;
    {
        std::unique_lock<std::mutex> lock(mInflightVndRequestsMutex);

        // wait for snapshot capture data collected
        auto t0 = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(kDataCollectWaitTimeoutMs);
        ok = mSnapshotDoneCond.wait_for(lock, timeout, [this]() -> bool {
            // wait until all MiCapture in mInflightVndRequests are not snapshot
            // return std::none_of(mInflightVndRequests.begin(), mInflightVndRequests.end(),
            //        [](const std::pair<uint32_t, std::shared_ptr<MiCapture>> &p) -> bool {
            //    return p.second->isSnapshot();
            //});
            return getPendingSnapCount() == 0;
        });
        auto t1 = std::chrono::steady_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
        MZ_LOGD(LogGroupCore, "wait time: %lld ms", dt.count());

        if (ok) {
            MZ_LOGI(LogGroupCore, "snapshot data collected!");
        } else {
            MZ_LOGE(LogGroupCore, "wait snapshot data collected timeout (%d ms)!",
                    kDataCollectWaitTimeoutMs);
        }
    }

    exitFakeReqThread = true;
    if (fakeReqThread.joinable()) {
        fakeReqThread.join();
    }

    MZ_LOGD(LogGroupCore, "-");
    return ok;
}

void MiAlgoCamMode::collectOfflineSessionInfo(CameraOfflineSessionInfo &offlineSessionInfo)
{
    auto toString = [](const std::map<uint32_t, std::shared_ptr<FwkRequestRecord>> &inflights) {
        std::stringstream ss;
        ss << "inflights.size: " << inflights.size() << ", ";
        ss << "[ ";
        for (auto &&[frameNum, record] : inflights) {
            ss << "{ ";
            ss << "fwkFrameNum: " << record->fwkFrameNum << ", ";
            ss << "pendingStreams: [";
            for (auto &&[streamId, isPending] : record->pendingStreams) {
                ss << "streamId(" << streamId << "): " << std::boolalpha << isPending << ", ";
            }
            ss << "], ";
            ss << "outstandingBuffers: [";
            for (auto &&[streamId, bufferCount] : record->outstandingBuffers) {
                ss << "streamId(" << streamId << "): " << bufferCount << ", ";
            }
            ss << "], ";
            ss << "}, ";
        }
        ss << "], ";

        return ss.str();
    };

    std::map<int32_t, OfflineStream> offlineStreams;
    {
        std::lock_guard<std::mutex> lock(mInflightFwkRequestsMutex);
        MZ_LOGD(LogGroupCore, "  %s", toString(mInflightFwkRequests).c_str());
        std::map<uint32_t, std::vector<uint32_t>> tempMp;
        for (auto &it : mStreamToFwkNumber) {
            uint32_t streamId = it.first;
            for (auto iter = it.second.begin(); iter != it.second.end(); iter++) {
                tempMp[iter->first].push_back(streamId);
            }
        }
        for (auto &it : tempMp) {
            OfflineRequest offlineRequest;
            offlineRequest.frameNumber = it.first;
            for (auto &iter : it.second) {
                uint32_t streamId = iter;
                offlineRequest.pendingStreams.push_back(streamId);
                auto &stream = offlineStreams[streamId];
                stream.id = streamId;
                stream.numOutstandingBuffers += 1;
            }
            if (!offlineRequest.pendingStreams.empty()) {
                offlineSessionInfo.offlineRequests.push_back(offlineRequest);
            } else {
                // NOTE: all requests in mInflightFwkRequests reach here should have at least one
                // 	  pending stream (offline stream)
                MZ_LOGE(LogGroupCore, "offlineRequest[%d] has no pending streams",
                        offlineRequest.frameNumber);
            }
        }
    }

    for (auto &&[streamId, stream] : offlineStreams) {
        offlineSessionInfo.offlineStreams.push_back(stream);
        MZ_LOGE(LogGroupCore, "streamid:[%d] has %d pending streams", streamId,
                stream.numOutstandingBuffers);
    }
}

void MiAlgoCamMode::setCallback(std::shared_ptr<MiCamDevCallback> deviceCallback)
{
    MZ_LOGD(LogGroupCore, "+");
    if (!deviceCallback) {
        MZ_LOGE(LogGroupCore, "setCallback failed, due to deviceCallback is nullptr!");
        return;
    }
    mDeviceCallback = std::move(deviceCallback);

    mBufferManager->setCallback(mDeviceCallback);

    // notify reprocess result thread to continue callback
    {
        std::lock_guard<std::mutex> lock(mPauseReprocessedCallbackMutex);
        mPauseReprocessedCallback = false;
    }
    mPauseReprocessedCallbackCond.notify_one();
    MZ_LOGD(LogGroupCore, "-");
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//// Internal Operations (for submodule, i.e., MiCapture, AlgoEngine...)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
bool MiAlgoCamMode::isFwkRequestCompleted(const std::shared_ptr<FwkRequestRecord> &fwkRequestRecord)
{
    if (!fwkRequestRecord->isFullMetadata) {
        return false;
    }
    auto &&pendingStreams = fwkRequestRecord->pendingStreams;
    if (std::any_of(pendingStreams.begin(), pendingStreams.end(),
                    [](const std::pair<int32_t, bool> &p) { return p.second; })) {
        // has any pending streams
        return false;
    }
    return true;
}

void MiAlgoCamMode::onFwkRequestCompleted(uint32_t fwkFrameNum)
{
    if (isSnapshotRequest(mInflightFwkRequests[fwkFrameNum]->fwkRequest) && mPerfLock) {
        MZ_LOGI(LogGroupCore, "mizone capture stop boost");
        mPendingVendorHALSnapCount.fetch_sub(1);
        mPerfLock->ReleasePerfLock();
    }
    MZ_LOGD(LogGroupCore, "removing %d from mInflightFwkRequests", fwkFrameNum);
    mInflightFwkRequests.erase(fwkFrameNum);
}

std::shared_ptr<FwkRequestRecord> MiAlgoCamMode::getFwkRequestRecordLocked(uint32_t fwkFrameNum)
{
    std::shared_ptr<FwkRequestRecord> fwkRequestRecord = nullptr;

    auto it = mInflightFwkRequests.find(fwkFrameNum);
    if (it != mInflightFwkRequests.end()) {
        fwkRequestRecord = it->second;
    } else {
        MZ_LOGE(LogGroupCore, "not found record (fwkFrameNum = %d) in mInflightFwkRequests",
                fwkFrameNum);
    }

    return fwkRequestRecord;
}

void MiAlgoCamMode::forwardResultToFwk(const CaptureResult &result)
{
    MZ_LOGD(LogGroupCore, "+");
    MZ_LOGD(LogGroupCore, "fwk res: %s", utils::toString(result).c_str());

    // update record info (metadata/pendingStreams) in mInflightFwkRequests
    std::lock_guard<std::mutex> lock(mInflightFwkRequestsMutex);
    auto fwkRequestRecord = getFwkRequestRecordLocked(result.frameNumber);
    if (fwkRequestRecord != nullptr) {
        fwkRequestRecord->result += result.result;
        fwkRequestRecord->halResult += result.halResult;
        for (auto &&buffer : result.outputBuffers) {
            fwkRequestRecord->pendingStreams[buffer.streamId] = false;
            fwkRequestRecord->outstandingBuffers[buffer.streamId]--;
            if (mStreamToFwkNumber.find(buffer.streamId) != mStreamToFwkNumber.end() &&
                mStreamToFwkNumber[buffer.streamId].size() > 0) {
                uint32_t tempFwkNumber = mStreamToFwkNumber[buffer.streamId].begin()->first;
                mStreamToFwkNumber[buffer.streamId][result.frameNumber] = 1;
                if (tempFwkNumber == result.frameNumber) {
                    for (auto it = mStreamToFwkNumber[buffer.streamId].begin();
                         it != mStreamToFwkNumber[buffer.streamId].end();) {
                        if (it->second) {
                            it = mStreamToFwkNumber[buffer.streamId].erase(it);
                        } else {
                            break;
                        }
                    }
                }
            }
        }
        for (auto &&buffer : result.inputBuffers) {
            fwkRequestRecord->pendingStreams[buffer.streamId] = false;
            fwkRequestRecord->outstandingBuffers[buffer.streamId]--;
        }
        if (result.bLastPartialResult) {
            fwkRequestRecord->isFullMetadata = true;
        }

        if (isFwkRequestCompleted(fwkRequestRecord)) {
            onFwkRequestCompleted(result.frameNumber);
        }
    } else {
        MZ_LOGE(LogGroupCore, "fwkRequestRecord is nullptr");
    }

    mDeviceCallback->processCaptureResult({result});
    MZ_LOGD(LogGroupCore, "-");
}

void MiAlgoCamMode::forwardNotifyToFwk(const NotifyMsg &msg)
{
    MZ_LOGD(LogGroupCore, "+");
    MZ_LOGD(LogGroupCore, "fwk msg: %s", utils::toString(msg).c_str());

    auto frameNum =
        msg.type == MsgType::SHUTTER ? msg.msg.shutter.frameNumber : msg.msg.error.frameNumber;

    // update record info (metadata/pendingStreams) in mInflightFwkRequests
    std::lock_guard<std::mutex> lock(mInflightFwkRequestsMutex);
    auto fwkRequestRecord = getFwkRequestRecordLocked(frameNum);
    if (fwkRequestRecord != nullptr) {
        if (msg.type == MsgType::SHUTTER) {
            fwkRequestRecord->shutter = msg.msg.shutter.timestamp;
        } else if (msg.type == MsgType::ERROR) {
            // TODO: need error handle?
            // mark as fullmeta to ensure this record will be removed correctly later
            fwkRequestRecord->isFullMetadata = true;
        }

        if (isFwkRequestCompleted(fwkRequestRecord)) {
            onFwkRequestCompleted(frameNum);
        }
    } else {
        MZ_LOGE(LogGroupCore, "fwkRequestRecord is nullptr");
    }

    mDeviceCallback->notify({msg});
    MZ_LOGD(LogGroupCore, "-");
}

int MiAlgoCamMode::forwardRequestToVnd(const CaptureRequest &request)
{
    uint32_t numRequestProcessed = 0;
    auto status = mDeviceSession->processCaptureRequest({request}, numRequestProcessed);
    if (status != 0 || numRequestProcessed < 1) {
        MZ_LOGE(LogGroupCore, "Core Session process error, status: %d, numRequestProcessed: %d",
                status, numRequestProcessed);
    }
    return status;
}

// For MiCamDecision
void MiAlgoCamMode::updateSettings(const MiMetadata &settings)
{
    mDecision->updateSettings(settings);
}

void MiAlgoCamMode::updateStatus(const MiMetadata &status)
{
    mDecision->updateStatus(status);
}

int MiAlgoCamMode::queryFeatureSetting(const MiMetadata &info, uint32_t &frameCount,
                                       std::vector<MiMetadata> &settings)
{
    int ret = 0;
    ret = mDeviceSession->queryFeatureSetting(info, frameCount, settings);
    return ret;
}

// For quick view
void MiAlgoCamMode::addAnchorFrame(uint32_t fwkFrameNum, uint32_t anchorFwkFrameNum)
{
    MZ_LOGI(LogGroupCore, "RequestNumber: %d add anchorFrame fwkNumber: %d to mAnchorFrames",
            fwkFrameNum, anchorFwkFrameNum);
    std::lock_guard<std::mutex> lock(mQuickviewResultsMutex);
    mAnchorFrames.emplace(std::make_pair(fwkFrameNum, anchorFwkFrameNum));
    mQuickviewResultCond.notify_all();
}

uint32_t MiAlgoCamMode::getAnchorFrameNumber(uint64_t shutter)
{
    std::lock_guard<std::mutex> lock(mQuickviewResultsMutex);
    if (mQuickViewShutterToFwkNum.find(shutter) != mQuickViewShutterToFwkNum.end()) {
        return mQuickViewShutterToFwkNum[shutter];
    } else {
        uint64_t preShutter;
        uint64_t firstShutter = mQuickViewShutterToFwkNum.begin()->first;
        uint64_t lastShutter = mQuickViewShutterToFwkNum.rbegin()->first;
        if (shutter > firstShutter && shutter < lastShutter) {
            for (auto iter : mQuickViewShutterToFwkNum) {
                uint64_t tempShutter = iter.first;
                if (tempShutter < shutter) {
                    preShutter = tempShutter;
                }
            }
            return mQuickViewShutterToFwkNum[preShutter];
        } else if (shutter < firstShutter) {
            return mQuickViewShutterToFwkNum[firstShutter];
        } else {
            return mQuickViewShutterToFwkNum[lastShutter];
        }
    }
    return 0;
}

void MiAlgoCamMode::updateQuickViewBuffer(uint32_t fwkFrameNum, uint64_t shutter,
                                          const StreamBuffer &buffer)
{
    MZ_LOGD(
        LogGroupCore,
        "updating quick view buffer, fwkFrameNum = %d, shutter = %lu, mQuickViewBuffers.size = %lu",
        fwkFrameNum, shutter, mQuickViewBuffers.size());
    if (buffer.status != BufferStatus::ERROR) {
        std::lock_guard<std::mutex> lock(mQuickviewResultsMutex);
        if (mQuickViewBuffers.size() >= MAX_QUICKVIEWBUFFER_SIZE) {
            auto it = mQuickViewBuffers.begin();
            std::vector<StreamBuffer> buffers{it->second};
            mBufferManager->releaseBuffers(buffers);
            mQuickViewBuffers.erase(it);
            auto iter = mQuickViewFwkNumToShutter.begin();
            mQuickViewFwkNumToShutter.erase(iter);
            auto itTemp = mQuickViewShutterToFwkNum.begin();
            mQuickViewShutterToFwkNum.erase(itTemp);
        }
        mQuickViewBuffers.emplace(std::make_pair(shutter, buffer));
        mQuickViewFwkNumToShutter.emplace(std::make_pair(fwkFrameNum, shutter));
        mQuickViewShutterToFwkNum.emplace(std::make_pair(shutter, fwkFrameNum));
        mQuickviewResultCond.notify_all();
    }

    std::shared_ptr<FwkRequestRecord> inflightRequest(nullptr);
    {
        std::lock_guard<std::mutex> lock(mInflightFwkRequestsMutex);
        inflightRequest = getFwkRequestRecordLocked(fwkFrameNum);
        if (inflightRequest == nullptr) {
            MZ_LOGE(LogGroupCore, "could not find record (frameNum = %d) in mInflightFwkRequests!",
                    fwkFrameNum);
            return;
        }
    }

    auto &fwkRequest = inflightRequest->fwkRequest;
    for (auto &qrBuffer : fwkRequest.outputBuffers) {
        auto streamId = qrBuffer.streamId;
        if (mFwkConfig->streams[streamId]->usageType == StreamUsageType::QrStream) {
            StreamBuffer requestedBuffer;
            auto ok = getFwkBuffer(fwkRequest, qrBuffer.streamId, requestedBuffer);
            if (!ok) {
                MZ_LOGE(LogGroupCore, "getFwkBuffer for stream %d failed", buffer.streamId);
                return;
            }

            if (buffer.status == BufferStatus::ERROR) {
                requestedBuffer.status = BufferStatus::ERROR;
            } else {
                utils::copyBuffer(buffer.buffer, requestedBuffer.buffer, "QrStream");
            }
            CaptureResult fwkQrResult;
            fwkQrResult.frameNumber = fwkFrameNum;
            fwkQrResult.outputBuffers.push_back(requestedBuffer);
            forwardResultToFwk(fwkQrResult);
            break;
        }
    }
    if (buffer.status == BufferStatus::ERROR) {
        std::vector<StreamBuffer> buffers{buffer};
        mBufferManager->releaseBuffers(buffers);
    }
}

bool MiAlgoCamMode::notifyQuickView(uint32_t fwkFrameNum, uint64_t shutter)
{
    MZ_LOGD(LogGroupCore, "frameNum = %d, shutter = %lu", fwkFrameNum, shutter);
    std::shared_ptr<FwkRequestRecord> inflightRequest(nullptr);
    {
        std::lock_guard<std::mutex> lock(mInflightFwkRequestsMutex);
        inflightRequest = getFwkRequestRecordLocked(fwkFrameNum);
        if (inflightRequest == nullptr) {
            MZ_LOGE(LogGroupCore, "could not find record (frameNum = %d) in mInflightFwkRequests!",
                    fwkFrameNum);
            return false;
        }
    }

    if (mQuickViewBuffers.empty()) {
        MZ_LOGE(LogGroupCore, "mQuickViewBuffers is empty!");
        return false;
    }

    auto &fwkRequest = inflightRequest->fwkRequest;

    // find the buffer whose timestamps not less than shutter,
    // because sometimes we could not find one matched exactly
    auto it = mQuickViewBuffers.find(shutter);
    if (it == mQuickViewBuffers.end()) {
        MZ_LOGI(LogGroupCore,
                "frameNum=%d: shutter [%lu] not found in mQuickViewBuffers, waiting new QuickView "
                "buffer arriving...",
                fwkFrameNum, shutter);
        uint64_t preShutter;
        uint64_t firstShutter = mQuickViewBuffers.begin()->first;
        uint64_t lastShutter = mQuickViewBuffers.rbegin()->first;
        if (shutter > firstShutter && shutter < lastShutter) {
            for (auto iter : mQuickViewBuffers) {
                uint64_t tempShutter = iter.first;
                if (tempShutter < shutter) {
                    preShutter = tempShutter;
                }
            }
            it = mQuickViewBuffers.find(preShutter);
        } else if (shutter < firstShutter) {
            it = mQuickViewBuffers.find(firstShutter);
        } else {
            it = mQuickViewBuffers.find(lastShutter);
        }
    }

    if (it->first != shutter) {
        MZ_LOGW(LogGroupCore, "no shutter match [%lu] exactly, use [%lu] instead", shutter,
                it->first);
    }

    auto quickViewBuffer = it->second;
    for (auto &buffer : fwkRequest.outputBuffers) {
        if (mFwkConfig->streams[buffer.streamId]->usageType == StreamUsageType::QuickViewStream) {
            StreamBuffer requestedBuffer;
            auto ok = getFwkBuffer(fwkRequest, buffer.streamId, requestedBuffer);
            if (!ok) {
                MZ_LOGE(LogGroupCore, "getFwkBuffer for stream %d failed, will retry later!",
                        buffer.streamId);
                uint32_t anchorFrame = fwkFrameNum - 1;
                for (auto iter : mQuickViewFwkNumToShutter) {
                    if (iter.second == shutter) {
                        anchorFrame = iter.first;
                        break;
                    }
                }
                mAnchorFrames[fwkFrameNum] = anchorFrame;
                return false;
            }
            // utils::dumpBuffer(quickViewBuffer.buffer, std::to_string(fwkFrameNum)+"prev");
            utils::copyBuffer(quickViewBuffer.buffer, requestedBuffer.buffer, "QvStream");
            nv21ToNv12(requestedBuffer.buffer);
            // utils::dumpBuffer(requestedBuffer.buffer, std::to_string(fwkFrameNum));
            CaptureResult fwkResult;
            fwkResult.frameNumber = fwkFrameNum;
            fwkResult.outputBuffers.push_back(requestedBuffer);
            forwardResultToFwk(fwkResult);
            break;
        }
    }

    return true;
}

void MiAlgoCamMode::returnQuickViewError(uint32_t fwkFrameNum)
{
    std::shared_ptr<FwkRequestRecord> inflightRequest(nullptr);
    {
        std::lock_guard<std::mutex> lock(mInflightFwkRequestsMutex);
        inflightRequest = getFwkRequestRecordLocked(fwkFrameNum);
        if (inflightRequest == nullptr) {
            MZ_LOGE(LogGroupCore, "could not find record (frameNum = %d) in mInflightFwkRequests!",
                    fwkFrameNum);
            return;
        }
    }
    auto &fwkRequest = inflightRequest->fwkRequest;
    for (auto &buffer : fwkRequest.outputBuffers) {
        if (mFwkConfig->streams[buffer.streamId]->usageType == StreamUsageType::QuickViewStream) {
            buffer.status = BufferStatus::ERROR;
            CaptureResult fwkResult;
            fwkResult.frameNumber = fwkFrameNum;
            fwkResult.outputBuffers.push_back(buffer);
            forwardResultToFwk(fwkResult);
            break;
        }
    }
}

bool MiAlgoCamMode::nv21ToNv12(std::shared_ptr<MiImageBuffer> in)
{
    if (in == nullptr) {
        MLOGE(LogGroupCore, "src (%p) is nullptr!", in.get());
        return false;
    }

    if (in->isEmpty()) {
        MLOGE(LogGroupCore, "in is empty!");
        return false;
    }
    bool ok = false;
    auto align64 = [](int num) { return ((num + 63) >> 6) << 6; };
    if (in->lockBuf("nv21ToNv12", eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_SW_READ_OFTEN)) {
        int temp;
        int width = align64(in->getImgWidth());
        int height = in->getImgHight();
        unsigned char *yimg = (unsigned char *)(in->getBufVA(0));
        unsigned char *vimg = yimg + width * height;
        unsigned char *uimg = vimg + 1;
        if (yimg != nullptr) {
            for (int i = 0; i < height / 2; i++) {
                for (int j = 0; j < width; j = j + 2) {
                    if (vimg != nullptr && uimg != nullptr) {
                        temp = *vimg;
                        *vimg = *uimg;
                        *uimg = temp;
                        uimg += 2;
                        vimg += 2;
                    }
                }
            }
            ok = true;
        }
        if (!(in->unlockBuf("nv21ToNv12"))) {
            MLOGE(LogGroupCore, "failed on in buffer unlockBuf");
            ok = false;
        }
    }
    return ok;
}

void MiAlgoCamMode::startQuickviewResultThread()
{
    if (mQuickviewResultThread.joinable()) {
        MZ_LOGE(LogGroupCore, "quickview result thread is runing, could not be restarted!");
        return;
    }
    mExitQuickviewResultThread = false;
    mQuickviewResultThread = std::thread([this]() { quickviewResultThreadLoop(); });
}

void MiAlgoCamMode::exitQuickviewResultThread()
{
    MZ_LOGI(LogGroupCore, "+");

    {
        std::lock_guard<std::mutex> lck(mQuickviewResultsMutex);
        mExitQuickviewResultThread = true;
    }
    mQuickviewResultCond.notify_all();

    if (mQuickviewResultThread.joinable()) {
        mQuickviewResultThread.join();
    }

    MZ_LOGI(LogGroupCore, "-");
}

void MiAlgoCamMode::quickviewResultThreadLoop()
{
    MZ_LOGI(LogGroupCore, "+");
    prctl(PR_SET_NAME, "mizone_quickviewResultLoop");

    while (true) {
        std::unique_lock<std::mutex> lock(mQuickviewResultsMutex);
        uint32_t fwkNumber, finalAnchorFrame;
        mQuickviewResultCond.wait(lock, [this, &fwkNumber, &finalAnchorFrame]() {
            if (!mAnchorFrames.empty()) {
                uint32_t anchorFrame = mAnchorFrames.begin()->second;
                fwkNumber = mAnchorFrames.begin()->first;
                if (mQuickViewFwkNumToShutter.find(anchorFrame) !=
                    mQuickViewFwkNumToShutter.end()) {
                    finalAnchorFrame = anchorFrame;
                    return true;
                } else {
                    uint32_t firstFrame = mQuickViewFwkNumToShutter.begin()->first;
                    uint32_t lastFrame = mQuickViewFwkNumToShutter.rbegin()->first;
                    if (anchorFrame > firstFrame && anchorFrame < lastFrame) {
                        uint32_t preFrame = 0;
                        for (auto it : mQuickViewFwkNumToShutter) {
                            uint32_t tempFrame = it.first;
                            if (tempFrame < anchorFrame) {
                                preFrame = tempFrame;
                            }
                        }
                        finalAnchorFrame = preFrame;
                        return true;
                    } else if (firstFrame > anchorFrame) {
                        finalAnchorFrame = firstFrame;
                        return true;
                    }
                }
            }
            return mExitQuickviewResultThread.load();
        });

        if (mExitQuickviewResultThread) {
            return;
        }

        MZ_LOGD(LogGroupCore, "before deque, PendingQvResults: %d", mAnchorFrames.size());

        if (notifyQuickView(fwkNumber, mQuickViewFwkNumToShutter[finalAnchorFrame])) {
            MZ_LOGD(LogGroupCore, "notifyQuickView succeed: FwkFrameNum = %d, Shutter = %lu",
                    fwkNumber, mQuickViewFwkNumToShutter[finalAnchorFrame]);
            auto it = mAnchorFrames.find(fwkNumber);
            mAnchorFrames.erase(it);
        }
    }

    MZ_LOGI(LogGroupCore, "-");
}

int MiAlgoCamMode::updatePendingSnapRecords(uint32_t fwkFrameNum, std::string imageName, int type)
{
    if (type != SnapMessageType::HAL_BUFFER_RESULT && type != SnapMessageType::HAL_SHUTTER_NOTIFY) {
        MLOGE(LogGroupHAL, "[Photographer]: unsupported type %d", type);
        return -1;
    }
    std::unique_lock<std::mutex> locker{mRecordMutex};
    auto it = mPendingSnapshotRecord.find(fwkFrameNum);
    if (it == mPendingSnapshotRecord.end()) {
        MLOGE(LogGroupHAL, "fwkFrame:%u not found in snapshot record", fwkFrameNum);
        return -1;
    }

    auto &recorder = it->second;
    recorder.set(type);
    if (recorder.all()) {
        mPendingSnapshotRecord.erase(it);
        MLOGI(LogGroupHAL, "fwkFrame:%u imageName:%s complete, now pending snapshot req num:%zu",
              fwkFrameNum, imageName.c_str(), mPendingSnapshotRecord.size());
        locker.unlock();

        BGServiceWrap::getInstance()->onCaptureCompleted(imageName, fwkFrameNum);
    }
    return 0;
}

void MiAlgoCamMode::notifyCaptureFail(uint32_t fwkFrameNum, std::string imageName)
{
    BGServiceWrap::getInstance()->onCaptureFailed(imageName, fwkFrameNum);
}

// For AlgoEngine
// buffer 收齐后统一进行reprocss和逐帧送入共用一个reprocess函数。
//参数frameFirstArrived默认为true，只有逐帧送入的时候，后面几帧设为false
// https://xiaomi.f.mioffice.cn/docs/dock4o9T0BwvWy2EBgTow1DxhHd#WzwIRk
int MiAlgoCamMode::reprocess(uint32_t fwkFrameNum,
                             std::map<uint32_t, std::shared_ptr<MiRequest>> requests,
                             std::string captureTypeName, bool frameFirstArrived)
{
    MZ_LOGI(LogGroupCore, "+");

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.mizone.reprocess.dump", prop, "0");
    bool needDump = (int32_t)atoi(prop) == 1;
    // dump snapshot buffer before reprocess
    if (needDump) {
        for (auto &&[vndFrameNum, miReq] : requests) {
            for (auto &&buffer : miReq->vndRequest.outputBuffers) {
                if (mVndConfig->streams[buffer.streamId]->usageType !=
                    StreamUsageType::SnapshotStream) {
                    continue;
                }
                std::string name("before_reprocess_");
                name += std::to_string(fwkFrameNum) + "_" + std::to_string(vndFrameNum) + "_" +
                        std::to_string(buffer.bufferId) + "_" + std::to_string(miReq->shutter) +
                        "_";
                utils::dumpBuffer(buffer.buffer, name);
            }
        }
    }

    CaptureRequest fwkRequest;
    if (frameFirstArrived) {
        onAlgoTaskNumChanged(true, captureTypeName);
        mFwkNumToCaptureTypeName[fwkFrameNum] = captureTypeName;
        MZ_LOGD(LogGroupHAL, "[AppSnapshotController] captureTypeName = %s",
                captureTypeName.c_str());
        std::shared_ptr<FwkRequestRecord> fwkReqRecord(nullptr);
        {
            std::lock_guard<std::mutex> lock(mInflightFwkRequestsMutex);
            fwkReqRecord = getFwkRequestRecordLocked(fwkFrameNum);
            if (fwkReqRecord == nullptr) {
                MZ_LOGE(LogGroupCore, "not found record (fwkFrameNum = %d)", fwkFrameNum);
                return 0;
            }
        }

        fwkRequest = fwkReqRecord->fwkRequest;
    }

    {
        std::lock_guard<std::mutex> lock(mPostProcDataLock);
        // new PostProcParams
        for (auto &&[vndFrameNum, miReq] : requests) {
            for (auto &&buffer : miReq->vndRequest.outputBuffers) {
                if (mVndConfig->streams[buffer.streamId]->usageType !=
                    StreamUsageType::SnapshotStream) {
                    continue;
                }
                auto tempData = std::make_shared<MiPostProcData>();
                tempData->pPostProcParams = new PostProcParams;
                DecoupleUtil::convertMetadata(miReq->result, tempData->pPostProcParams->settings);
                DecoupleUtil::convertMetadata(miReq->halResult,
                                              tempData->pPostProcParams->halSettings);
                tempData->physicalCameraId =
                    mVndConfig->streams[buffer.streamId]->rawStream.physicalCameraId;
                tempData->pPostProcParams->vndRequestNo = vndFrameNum;
                miReq->vPostProcData.emplace_back(tempData);
            }
            mPostProcData[fwkFrameNum].emplace(vndFrameNum, miReq->vPostProcData);
        }
        // debug
        MZ_LOGD(LogGroupCore, "mPostProcData size:%d", mPostProcData.size());
        for (auto &&[fwkFrameNum, tempmap] : mPostProcData) {
            std::stringstream ss;
            ss << "{"
               << "fwkNum:" << fwkFrameNum;
            for (auto &&[vndFrameNum, vPata] : tempmap) {
                ss << "["
                   << "vndFrameNum:" << vndFrameNum << " vPata size:" << vPata.size();
                for (auto &&data : vPata) {
                    ss << "("
                       << "phyId:" << data->physicalCameraId;
                    ss << " pParams:" << data->pPostProcParams << ")";
                }
                ss << "]";
            }
            ss << "}";
            MZ_LOGD(LogGroupCore, "%s", ss.str().c_str());
        }
    }

    mAlgoEngine->process(fwkRequest, requests);

    MZ_LOGI(LogGroupCore, "-");
    return 0;
}

void MiAlgoCamMode::quickFinish(std::string pipelineName)
{
    MZ_LOGI(LogGroupCore, "+");
    mAlgoEngine->quickFinish(pipelineName);
    MZ_LOGI(LogGroupCore, "-");
}

void MiAlgoCamMode::onReprocessResultComing(const CaptureResult &result)
{
    MZ_LOGI(LogGroupCore, "+");

    std::shared_ptr<FwkRequestRecord> fwkRequestRecord(nullptr);
    {
        std::lock_guard<std::mutex> lock(mInflightFwkRequestsMutex);
        fwkRequestRecord = getFwkRequestRecordLocked(result.frameNumber);
        if (fwkRequestRecord == nullptr) {
            MZ_LOGE(LogGroupCore, "fwkRequestRecord is nullptr");
            return;
        }
    }

    // append partial result
    auto fwkResult = result;
    if (result.bLastPartialResult) {
        fwkResult.result = fwkRequestRecord->result + result.result;
    }

    {
        // packing tuning buffer
        bool containTuningBuffer = false;
        StreamBuffer tuningBuffer;
        for (auto &&buffer : fwkRequestRecord->fwkRequest.outputBuffers) {
            if (mFwkConfig->streams[buffer.streamId]->usageType ==
                StreamUsageType::TuningDataStream) {
                containTuningBuffer = true;
                tuningBuffer = buffer;
            }
        }

        if (result.bLastPartialResult && containTuningBuffer) {
            MZ_LOGD(LogGroupCore, "ready to packing tuningBuffer");
            CaptureResult fwkTuningResult;
            bool ok =
                getFwkBuffer(fwkRequestRecord->fwkRequest, tuningBuffer.streamId, tuningBuffer);
            if (!ok) {
                MZ_LOGE(LogGroupCore, "get tuning buffer failed!");
            }

            std::map<uint32_t /*fwkFrameNum*/,
                     std::map<int32_t, std::vector<std::shared_ptr<MiPostProcData>>>>::iterator
                itFwkReq;
            PostProcParams *pTuningData = NULL;
            {
                std::lock_guard<std::mutex> lock(mPostProcDataLock);
                itFwkReq = mPostProcData.find(result.frameNumber);
                if (itFwkReq != mPostProcData.end()) {
                    auto itdex = itFwkReq->second.begin();
                    pTuningData = itdex->second[0]->pPostProcParams;
                }
                MZ_LOGD(LogGroupCore, "[fwkNum:%d] get pTuningData(%p) to packing tuningBuffer",
                        result.frameNumber, pTuningData);
            }

            ok = packTuningBuffer(fwkRequestRecord, pTuningData, tuningBuffer.buffer);
            if (!ok) {
                MZ_LOGE(LogGroupCore, "packing tuning buffer failed!");
            }
            fwkTuningResult.frameNumber = result.frameNumber;
            fwkTuningResult.outputBuffers.emplace_back(tuningBuffer);
            {
                std::lock_guard<std::mutex> lock(mReprocessedResultsMutex);
                MZ_LOGD(LogGroupCore, "before enque, mPendingReprocessedResults: %lu",
                        mPendingReprocessedResults.size());
                mPendingReprocessedResults.push_back(fwkTuningResult);
                MZ_LOGD(LogGroupCore, "after enque, mPendingReprocessedResults: %lu",
                        mPendingReprocessedResults.size());
            }

            {
                std::lock_guard<std::mutex> lock(mPostProcDataLock);
                // release PostProcParams
                for (auto &&[index, vpData] : itFwkReq->second) {
                    for (auto &&pData : vpData) {
                        delete pData->pPostProcParams;
                    }
                }
                MZ_LOGD(LogGroupCore, "release fwkNum(%d) PostProcData.", itFwkReq->first);
                mPostProcData.erase(itFwkReq);
                std::stringstream ss;
                ss << "mPostProcData size:" << mPostProcData.size();
                for (auto &&[fwkFrameNum, tempmap] : mPostProcData) {
                    ss << "{"
                       << " fwkNum:" << fwkFrameNum << "}";
                }
                MZ_LOGD(LogGroupCore, "%s", ss.str().c_str());
            }
        }
    }

#if 0
    // TODO: add a system prop or a settings option to control buffer dump
    // dump snapshot buffer from AlgoEngine
    for (auto &&buffer : result.outputBuffers) {
        std::string name("after_reprocess_");
        name += std::to_string(result.frameNumber) + "_" + std::to_string(buffer.bufferId) + "_";
        utils::dumpBuffer(buffer.buffer, name);
    }
#endif

    {
        std::lock_guard<std::mutex> lock(mReprocessedResultsMutex);
        MZ_LOGD(LogGroupCore, "before enque, mPendingReprocessedResults: %lu",
                mPendingReprocessedResults.size());
        mPendingReprocessedResults.push_back(fwkResult);
        MZ_LOGD(LogGroupCore, "after enque, mPendingReprocessedResults: %lu",
                mPendingReprocessedResults.size());
    }
    mReprocessedResultCond.notify_one();

    MZ_LOGI(LogGroupCore, "-");
}

bool MiAlgoCamMode::packTuningBuffer(
    const std::shared_ptr<FwkRequestRecord> &fwkRequestRecord, const PostProcParams *pTuningData,
    const std::shared_ptr<MiImageBuffer> &tuningBuffer /* output */)
{
    MZ_LOGI(LogGroupCore, "+");
    if (fwkRequestRecord == nullptr || pTuningData == NULL) {
        return false;
    }

    auto packUtils = NSCam::v3::PackUtilV2::IIspTuningDataPackUtil::getInstance();
    if (packUtils == nullptr) {
        MZ_LOGE(LogGroupCore, "IIspTuningDataPackUtil::createInstance() fail!!");
        return false;
    }

    int32_t totalUsedBytes;
    NSCam::v3::PackUtilV2::IIspTuningDataPackUtil::PackInputParam inputParam;
    inputParam.sensorId = mCameraInfo.cameraId;
    inputParam.frameNo = fwkRequestRecord->fwkRequest.frameNumber;
    inputParam.requestNo = fwkRequestRecord->fwkRequest.frameNumber;
    DecoupleUtil::convertMetadata(fwkRequestRecord->fwkRequest.settings, inputParam.appControlMeta);
    inputParam.appResultMeta = pTuningData->settings;
    inputParam.halResultMeta = pTuningData->halSettings;
    inputParam.haltimestamp = static_cast<int64_t>(fwkRequestRecord->shutter);
    inputParam.timestamp = static_cast<int64_t>(fwkRequestRecord->shutter);
    tuningBuffer->lockBuf(__FUNCTION__);
    auto bufferHeap = tuningBuffer->getBufferHeap();
    int status = 0;
    //                <sensorId, vector<info>>
    std::unordered_map<int32_t, std::vector<std::shared_ptr<IImageStreamInfo>>> streamInfo;
    streamInfo.emplace(mCameraInfo.cameraId, std::vector<std::shared_ptr<IImageStreamInfo>>());
    status = packUtils->getHwBufferHeapFromIspTuningHeap(bufferHeap, streamInfo, nullptr, nullptr);
    status = packUtils->packIspTuningDataToHeap(bufferHeap, totalUsedBytes, inputParam);
    tuningBuffer->unlockBuf(__FUNCTION__);
    MZ_LOGD(LogGroupCore, "packing tuning buffer: status = %d, totalUsedBytes = %d", status,
            totalUsedBytes);

    MZ_LOGI(LogGroupCore, "-");
    return true;
}

// for buffers used in mizone, e.g., Snapshot YUV buffer, QuickView buffer, TuningData buffer...
// please always get them from this interface.
// for HBM disable case, it will find buffer in fwkRequest and return directly
// for HBM enable case, it will request from bufferManager (forward to fwk)
bool MiAlgoCamMode::getFwkBuffer(const CaptureRequest &fwkRequest, int32_t fwkStreamId,
                                 StreamBuffer &bufferOutput)
{
    MZ_LOGD(LogGroupCore, "+");
    MZ_LOGD(LogGroupCore, "frameNum = %d, fwkStreamId = %d", fwkRequest.frameNumber, fwkStreamId);

    auto it = std::find_if(
        fwkRequest.outputBuffers.begin(), fwkRequest.outputBuffers.end(),
        [fwkStreamId](const StreamBuffer &buffer) { return buffer.streamId == fwkStreamId; });
    if (it == fwkRequest.outputBuffers.end()) {
        MZ_LOGE(LogGroupCore, "fwkRequest(frameNum = %d) not contain buffer(streamId = %d)",
                fwkRequest.frameNumber, fwkStreamId);
        return false;
    }

    const auto &buffer = *it;

    // HBM disable
    if (buffer.bufferId != 0 && buffer.buffer != nullptr) {
        // request contain buffer, no need to request, just return
        bufferOutput = buffer;
        MZ_LOGD(LogGroupCore, "frameNum = %d: %s", fwkRequest.frameNumber,
                utils::toString(bufferOutput).c_str());
        return true;
    }

    // HBM enable, request from bufferManager
    std::vector<MiBufferManager::MiBufferRequestParam> requestParam(1);
    auto &param = requestParam.front();
    param.streamId = buffer.streamId;
    param.bufferNum = 1;
    std::map<int32_t, std::vector<StreamBuffer>> streamBufferMap;
    auto ok = mBufferManager->requestBuffers(requestParam, streamBufferMap);
    if (!ok || streamBufferMap.empty()) {
        MZ_LOGE(LogGroupHAL, "request buffer (stream %d) from bufferManager failed!",
                buffer.streamId);
        return false;
    }
    bufferOutput = streamBufferMap[buffer.streamId].front();
    MZ_LOGD(LogGroupCore, "frameNum = %d: %s", fwkRequest.frameNumber,
            utils::toString(bufferOutput).c_str());

    std::lock_guard<std::mutex> lock(mInflightFwkRequestsMutex);
    auto fwkRequestRecord = getFwkRequestRecordLocked(fwkRequest.frameNumber);
    if (fwkRequestRecord != nullptr) {
        fwkRequestRecord->outstandingBuffers[fwkStreamId]++;
    }
    return true;
}

void MiAlgoCamMode::onReprocessCompleted(uint32_t fwkFrameNum)
{
    MZ_LOGI(LogGroupCore, "+");
    if (mFwkNumToCaptureTypeName.find(fwkFrameNum) != mFwkNumToCaptureTypeName.end()) {
        onAlgoTaskNumChanged(false, mFwkNumToCaptureTypeName[fwkFrameNum]);
    } else {
        MZ_LOGE(LogGroupHAL, "[AppSnapshotController]: get captureTypeName fail");
    }

    MZ_LOGI(LogGroupCore, "-");
}

// For AppSnapshotController
int32_t MiAlgoCamMode::getPendingSnapCount()
{
    return mPendingMiHALSnapCount.load();
}

int32_t MiAlgoCamMode::getPendingVendorHALSnapCount()
{
    return mPendingVendorHALSnapCount.load();
}

void MiAlgoCamMode::calShot2shotTimeDiff()
{
    auto time = std::chrono::steady_clock::now().time_since_epoch();
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(time);
    int64_t nowTime = ms.count();
    if (!mPreSnapshotTime) {
        mPreSnapshotTime = nowTime;
        mShot2ShotTimediff = 0;
    } else {
        mShot2ShotTimediff = nowTime - mPreSnapshotTime;
        mPreSnapshotTime = nowTime;
    }
}

int64_t MiAlgoCamMode::getShot2shotTimeDiff()
{
    return mShot2ShotTimediff;
}

void MiAlgoCamMode::splitString(std::string &captureTypeName, std::ostringstream &typeName,
                                std::string &decisionModeStr)
{
    int index = captureTypeName.find_last_of(" ");
    typeName << "MialgoTaskConsumption.";
    if (index > 0 && index < captureTypeName.size() - 1) {
        typeName << captureTypeName.substr(index + 1, captureTypeName.size() - index - 1);
    }
    decisionModeStr = captureTypeName.substr(0, index);
}

void MiAlgoCamMode::onAlgoTaskNumChanged(bool isSendToMialgo, std::string captureTypeName)
{
    int consumption;
    std::string decisionModeStr;
    std::ostringstream typeName;
    splitString(captureTypeName, typeName, decisionModeStr);
    MiCamJsonUtils::getVal(typeName.str().c_str(), consumption, 1);
    if (isSendToMialgo) {
        MZ_LOGD(LogGroupHAL, "[AppSnapshotController]: send a task to mialgo, consumption: %d",
                consumption);
        MialgoTaskMonitor::getInstance()->onMialgoTaskNumChanged(decisionModeStr, consumption);
    } else {
        MZ_LOGD(LogGroupHAL,
                "[AppSnapshotController]: my task finished by mialgo, consumption "
                "released: %d",
                consumption);
        MialgoTaskMonitor::getInstance()->onMialgoTaskNumChanged(decisionModeStr, -consumption);
    }
}

void MiAlgoCamMode::onMemoryChanged(std::map<int32_t, int32_t> memoryInfo)
{
    // NOTE: if CameraMode is retired then its status is no longer useful
    if (mIsRetired.load()) {
        return;
    }
    std::unique_lock<std::mutex> lg(mCheckStatusLock);
    bool newStatus = canCamModeHandleNextSnapshot(memoryInfo);
    bool oldStatus = mMiZoneBufferCanHandleNextShot.exchange(newStatus);

    // NOTE: we only notify snapshot controller if we have different status
    if (oldStatus != newStatus) {
        if (newStatus) {
            MZ_LOGD(LogGroupHAL,
                    "[AppSnapshotController] CameraMode: %p have enough free buffer again, notify "
                    "controller",
                    this);
        } else {
            MZ_LOGD(
                LogGroupHAL,
                "[AppSnapshotController] CameraMode: %p may not has enough free buffer for next "
                "snapshot, notify controller",
                this);
        }
        AppSnapshotController::getInstance()->onRecordUpdate(newStatus, this);
    }
    lg.unlock();
}

bool MiAlgoCamMode::canCamModeHandleNextSnapshot(std::map<int32_t, int32_t> memoryInfo)
{
    bool avail = true;
    int64_t freeBufferSize = mThreshold;
    int64_t bufferUsePrediction = 0;
    for (const auto &stream : mVndConfig->streams) {
        if (stream.second->usageType == StreamUsageType::SnapshotStream) {
            uint32_t streamId = stream.second->streamId;
            if (memoryInfo.find(streamId) != memoryInfo.end()) {
                freeBufferSize = freeBufferSize - memoryInfo[streamId];
            }
            bufferUsePrediction = bufferUsePrediction + bufferUsePredicForNextSnapshot(streamId);
        }
    }
    if (freeBufferSize < bufferUsePrediction) {
        MZ_LOGD(LogGroupHAL,
                "[AppSnapshotController] may not have enough free buffers for "
                "next snapshot! bufferAvailable:prediction = %d:%d",
                freeBufferSize, bufferUsePrediction);
        avail = false;
    }
    return avail;
}

int32_t MiAlgoCamMode::bufferUsePredicForNextSnapshot(uint32_t streamId)
{
    // NOTE: default behavior: return buffer consumption of last snapshot as prediction
    int32_t bufferConsumption = 0;
    if (mBuffersConsumed.find(streamId) != mBuffersConsumed.end()) {
        bufferConsumption = mBuffersConsumed.at(streamId).load();
    }

    if (bufferConsumption <= 0) {
        bufferConsumption = 0;
    }

    if (mOneBuffersConsumed.find(streamId) != mOneBuffersConsumed.end()) {
        bool isEnable = false;
        int frameNum = 6;
        AppSnapshotController::getInstance()->GetHdrStatus(isEnable, frameNum);
        if (isEnable) {
            bufferConsumption = frameNum * mOneBuffersConsumed[streamId];
        }

        AppSnapshotController::getInstance()->GetSEStatus(isEnable, frameNum);
        if (isEnable) {
            bufferConsumption = frameNum * mOneBuffersConsumed[streamId];
        }
    }
    MZ_LOGD(LogGroupHAL, "[AppSnapshotController] stream: %d prediction = %d", streamId,
            bufferConsumption);
    return bufferConsumption;
}

void MiAlgoCamMode::setRetired()
{
    mIsRetired.store(true);
    AppSnapshotController::getInstance()->onRecordDelete(this);

    // for PsiMemMonitor
    PsiMemMonitor::getInstance()->snapshotCountReclaim();
    PsiMemMonitor::getInstance()->psiMonitorClose();
}

void MiAlgoCamMode::onPreviewCaptureCompleted(uint32_t fwkFrameNum, uint32_t vndFrameNum)
{
    std::lock_guard<std::mutex> lock(mInflightVndRequestsMutex);
    auto it = mInflightVndRequests.find(vndFrameNum);
    if (it != mInflightVndRequests.end()) {
        MZ_LOGD(LogGroupCore, "removing %d from mInflightVndRequests", vndFrameNum);
        mInflightVndRequests.erase(it);
    }
}

void MiAlgoCamMode::onSnapshotCaptureCompleted(uint32_t fwkFrameNum,
                                               const std::vector<uint32_t> &vndFrameNums)
{
    mPendingMiHALSnapCount.fetch_sub(1);
    std::lock_guard<std::mutex> lock(mInflightVndRequestsMutex);
    for (auto num : vndFrameNums) {
        MZ_LOGD(LogGroupCore, "removing %d from mInflightVndRequests", num);
        mInflightVndRequests.erase(num);
    }
    mSnapshotDoneCond.notify_one();
}

uint32_t MiAlgoCamMode::fetchAddVndFrameNum(uint32_t increment)
{
    return mVndFrameNum.fetch_add(increment);
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void MiAlgoCamMode::resultThreadLoop()
{
    MZ_LOGI(LogGroupCore, "+");
    prctl(PR_SET_NAME, "mizone_resultLoop");

    while (true) {
        // deque from mPendingVndResults
        std::unique_lock<std::mutex> lock(mResultsMutex);
        mResultCond.wait(lock,
                         [this]() { return !mPendingVndResults.empty() || mExitResultThread; });
        MZ_LOGD(LogGroupCore, "before deque, mPendingVndResults: %lu", mPendingVndResults.size());

        if (mExitResultThread && mPendingVndResults.empty()) {
            break;
        }

        auto res = mPendingVndResults.front();
        mPendingVndResults.pop_front();
        MZ_LOGD(LogGroupCore, "after deque, mPendingVndResults: %lu", mPendingVndResults.size());
        lock.unlock();

        if (res.result != nullptr) {
            // After the first frame preview returns, initialize mialgoengine.
            if (!mFirstPreviewArrived && !res.capture->isSnapshot() &&
                !res.result->outputBuffers.empty()) {
                mFirstPreviewArrived = true;
                MLOGI(LogGroupHAL, "[Performance] mizone received first frame");
                startAlgoEngineThread();
            }
            res.capture->processVendorResult(*res.result);
        }

        if (res.msg != nullptr) {
            res.capture->processVendorNotify(*res.msg);
        }
    }

    MZ_LOGI(LogGroupCore, "-");
}

int MiAlgoCamMode::goToAlgoEngine(uint32_t vndFrameNum)
{
    std::shared_ptr<MiCapture> capture;
    // find MiCapture by vnd frame num from mInflightVndRequests
    {
        std::lock_guard<std::mutex> lock(mInflightVndRequestsMutex);
        auto it = mInflightVndRequests.find(vndFrameNum);
        if (it != mInflightVndRequests.end()) {
            capture = it->second;
        } else {
            MZ_LOGE(LogGroupCore, "MiCapture (%d) could not find in mInflightVndRequests!",
                    vndFrameNum);
            return -1;
        }
    }
    {
        std::lock_guard<std::mutex> lock(mMiCaptureMutex);
        MZ_LOGD(LogGroupCore, "before enque, mPendingMiCaptures: %lu", mPendingMiCaptures.size());
        mPendingMiCaptures.emplace_back(vndFrameNum, capture);
        MZ_LOGD(LogGroupCore, "after enque, mPendingMiCaptures: %lu", mPendingMiCaptures.size());
    }
    mMiCaptureCond.notify_one();
    return 0;
}

void MiAlgoCamMode::algoEngineRequestLoop()
{
    MZ_LOGI(LogGroupCore, "+");
    if (!mAlgoEngine->isConfigCompleted()) {
        bool ok = mAlgoEngine->configure(mFwkConfig, mVndConfig);
        if (!ok) {
            MZ_LOGF(LogGroupCore, "AlgoEngine configure failed!");
        }
    }

    while (true) {
        // deque from mPendingVndResults
        std::unique_lock<std::mutex> lock(mMiCaptureMutex);
        mMiCaptureCond.wait(
            lock, [this]() { return !mPendingMiCaptures.empty() || mExitAlgoEngineThread; });
        MZ_LOGD(LogGroupCore, "before deque, mPendingMiCaptures: %lu", mPendingMiCaptures.size());

        if (mExitAlgoEngineThread && mPendingMiCaptures.empty()) {
            break;
        }

        auto [vndFrameNum, capture] = mPendingMiCaptures.front();
        mPendingMiCaptures.pop_front();
        MZ_LOGD(LogGroupCore, "after deque, mPendingMiCaptures: %lu", mPendingMiCaptures.size());
        lock.unlock();

        if (capture->isSingleFrameEnabled()) {
            bool isFirstFrame = vndFrameNum == capture->getFirstVndFrameNum() ? true : false;
            std::map<uint32_t, std::shared_ptr<MiRequest>> req;
            req[vndFrameNum] = capture->getMiRequests()[vndFrameNum];
            reprocess(capture->getFwkFrameNumber(), std::move(req), capture->getmCaptureTypeName(),
                      isFirstFrame);

        } else {
            reprocess(capture->getFwkFrameNumber(), std::move(capture->getMiRequests()),
                      capture->getmCaptureTypeName());
        }
    }

    MZ_LOGI(LogGroupCore, "-");
}

void MiAlgoCamMode::reprocessedResultThreadLoop()
{
    MZ_LOGI(LogGroupCore, "+");
    while (true) {
        // deque from mPendingReprocessedResults
        std::unique_lock<std::mutex> lock(mReprocessedResultsMutex);
        mReprocessedResultCond.wait(lock, [this]() {
            return !mPendingReprocessedResults.empty() || mExitReprocessedResultThread;
        });
        MZ_LOGD(LogGroupCore, "before deque, mPendingReprocessedResults: %lu",
                mPendingReprocessedResults.size());

        if (mExitReprocessedResultThread && mPendingReprocessedResults.empty()) {
            break;
        }

        auto res = mPendingReprocessedResults.front();
        mPendingReprocessedResults.pop_front();
        MZ_LOGD(LogGroupCore, "after deque, mPendingReprocessedResults: %lu",
                mPendingReprocessedResults.size());
        lock.unlock();

        // if callback is paused by switchToOffline, wait until new callbak set by setCallback
        auto waitForCallback = [this](std::unique_lock<std::mutex> &lock) {
            mPauseReprocessedCallbackCond.wait(
                lock, [this]() -> bool { return !mPauseReprocessedCallback; });
        };

        std::unique_lock<std::mutex> pauseLock(mPauseReprocessedCallbackMutex);
        waitForCallback(pauseLock);
        forwardResultToFwk(res);
    }
    MZ_LOGI(LogGroupCore, "-");
}

void MiAlgoCamMode::startResultThread()
{
    if (mResultThread.joinable()) {
        MZ_LOGE(LogGroupCore, "result thread is runing, could not be restarted!");
        return;
    }
    mExitResultThread = false;
    mResultThread = std::thread([this]() { resultThreadLoop(); });
}

void MiAlgoCamMode::startReprocessedResultThread()
{
    if (mReprocessedResultThread.joinable()) {
        MZ_LOGE(LogGroupCore, "reprocessed result thread is runing, could not be restarted!");
        return;
    }
    mExitReprocessedResultThread = false;
    mReprocessedResultThread = std::thread([this]() { reprocessedResultThreadLoop(); });
}

void MiAlgoCamMode::startAlgoEngineThread()
{
    if (mAlgoEngineThread.joinable()) {
        MZ_LOGE(LogGroupCore, "Algoengine reqeust thread is runing, could not be restarted!");
        return;
    }
    MZ_LOGI(LogGroupHAL, "start algoengine thread");
    mExitAlgoEngineThread = false;
    mAlgoEngineThread = std::thread{[this]() { algoEngineRequestLoop(); }};
}

void MiAlgoCamMode::exitResultThread()
{
    MZ_LOGI(LogGroupCore, "+");
    {
        std::lock_guard<std::mutex> lck(mResultsMutex);
        mExitResultThread = true;
    }
    mResultCond.notify_all();

    if (mResultThread.joinable()) {
        mResultThread.join();
    }

    MZ_LOGI(LogGroupCore, "-");
}

void MiAlgoCamMode::exitAlgoEngineThread()
{
    MZ_LOGI(LogGroupCore, "+");
    {
        std::lock_guard<std::mutex> lck(mMiCaptureMutex);
        mExitAlgoEngineThread = true;
    }
    mMiCaptureCond.notify_all();

    if (mAlgoEngineThread.joinable()) {
        mAlgoEngineThread.join();
    }

    MZ_LOGI(LogGroupCore, "-");
}

void MiAlgoCamMode::exitReprocessedResultThread()
{
    MZ_LOGI(LogGroupCore, "+");

    {
        std::lock_guard<std::mutex> lck(mReprocessedResultsMutex);
        mExitReprocessedResultThread = true;
    }
    mReprocessedResultCond.notify_all();

    if (mReprocessedResultThread.joinable()) {
        mReprocessedResultThread.join();
    }

    MZ_LOGI(LogGroupCore, "-");
}

void MiAlgoCamMode::exitAllThreads()
{
    exitResultThread();
    exitAlgoEngineThread();
    exitReprocessedResultThread();
    exitQuickviewResultThread();
}

} // namespace mizone
