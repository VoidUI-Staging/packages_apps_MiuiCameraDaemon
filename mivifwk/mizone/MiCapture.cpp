/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiCapture.h"

#include "AlgoEngine.h"
#include "MiAlgoCamMode.h"
#include "MiBufferManager.h"
#include "MiCamTrace.h"
#include "MiMetadata.h"
#include "utils.h"
// TODO: include custom_metadata_tag.h for
//       MTK_MFNR_FEATURE_BSS_GOLDEN_INDEX / XIAOMI_QUICKVIEW_MFNR_ANCHOR_TIME_STAMP
//       need redefine in decouple utils, and remove this include
#include <MiDebugUtils.h>
#include <sys/prctl.h>

#include <string>

#include "DecoupleUtil.h"
#include "Timer.h"
#include "mtkcam-halif/utils/metadata_tag/1.x/mtk_metadata_tag.h"
#include "mtkcam-interfaces/utils/metadata/hal/mtk_platform_metadata_tag.h"
#include "xiaomi/CameraPlatformInfoXiaomi.h"
#include "xiaomi/MiCommonTypes.h"
using namespace midebug;
using namespace XiaoMi;
namespace mizone {

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Definitions of Derived Classes
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// NOTE: Derived classes (MiPreviewCapture, MiSnapshotCapture...) are defined in .cpp
//       rather than .h file to avoid expose them to other module.
//       MiCamDecision should always create MiCapture through its static create method
//       (MiCapture::create), and MiCamMode should always treat them as the base class (MiCapture).
class MiPreviewCapture : public MiCapture
{
public:
    MiPreviewCapture(const CreateInfo &info);
    ~MiPreviewCapture() override;
    MiPreviewCapture(const MiPreviewCapture &) = delete;
    MiPreviewCapture &operator=(const MiPreviewCapture &) = delete;

    int processVendorRequest() override;
    void processVendorResult(const CaptureResult &result) override;
    void processVendorNotify(const NotifyMsg &msg) override;

protected:
    void processMetaResult(const CaptureResult &result) override;
    void processBufferResult(const CaptureResult &result) override;
    void processShutterMsg(const ShutterMsg &shutterMsg) override;
    void processErrorMsg(const ErrorMsg &errorMsg) override;
    void onCompleted() override;
    void singleFrameOnCompleted(int32_t frameNumber) override;
    void updateResult(std::shared_ptr<MiRequest> miReq) override;
};

class MiSnapshotCapture : public MiCapture
{
public:
    MiSnapshotCapture(const CreateInfo &info);
    ~MiSnapshotCapture() override;
    MiSnapshotCapture(const MiSnapshotCapture &) = delete;
    MiSnapshotCapture &operator=(const MiSnapshotCapture &) = delete;

    int processVendorRequest() override;
    void processVendorResult(const CaptureResult &result) override;
    void processVendorNotify(const NotifyMsg &msg) override;

protected:
    void processMetaResult(const CaptureResult &result) override;
    void processBufferResult(const CaptureResult &result) override;
    void processShutterMsg(const ShutterMsg &shutterMsg) override;
    void processErrorMsg(const ErrorMsg &errorMsg) override;
    void onCompleted() override;
    void singleFrameOnCompleted(int32_t frameNumber) override;
    void updateResult(std::shared_ptr<MiRequest> miReq) override;

    void updateMiOutputMeta(std::shared_ptr<MiRequest> req);
    void updateBokehMeta(std::shared_ptr<MiRequest> req);
    CameraRoleType getCameraRole(const MINT32 masterID);
    bool mQuickViewHasNotified;
    int32_t mQuickShotDelayTime;
    bool mQuickShotEnabled;
    std::shared_ptr<Timer> mQuickShotTimer;
    std::mutex mDelayMutex;
    std::condition_variable mDelayCond;
    bool mIsNeedWait;
    bool mIsZslEnable;
    uint8_t mIsAppZslEnable;
};
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Definition of MiCapture (the base class)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::unique_ptr<MiCapture> MiCapture::create(const CreateInfo &info)
{
    if (info.frameCount == 0 || (info.type == CaptureType::PreviewType && info.frameCount != 1)) {
        MLOGE(LogGroupCore, "create MiCapture (type = %d) with wrong frameCount (%d)!", info.type,
              info.frameCount);
        return nullptr;
    }

    for (auto it = info.streams.begin(); it != info.streams.end(); ++it) {
        if (*it == nullptr) {
            MLOGE(LogGroupCore, "create MiCapture (type = %d) with null stream.", info.type);
            return nullptr;
        }
    }

    std::unique_ptr<MiCapture> capture(nullptr);

    if (info.type == CaptureType::PreviewType) {
        capture = std::make_unique<MiPreviewCapture>(info);
    } else if (info.type == CaptureType::SnapshotType) {
        capture = std::make_unique<MiSnapshotCapture>(info);
    }

    return capture;
}

MiCapture::~MiCapture()
{
    MLOGD(LogGroupCore, "dtor: fwkFrameNum = %d", mFwkRequest.frameNumber);
}

MiCapture::MiCapture(const CreateInfo &info)
    : mType(info.type),
      mCurrentCamMode(info.mode),
      mCameraInfo(info.cameraInfo),
      mFwkRequest(info.fwkRequest),
      mQuickViewEnable(info.quickViewEnable),
      mMapStreamIdToFwk(info.mapStreamIdToFwk),
      mFrameCount(info.frameCount),
      mIsError(false),
      mCaptureTypeName(info.captureTypeName),
      mOperationMode(info.operationMode),
      mSingleFrameEnable(info.singleFrameEnable),
      mInAlgoEngine(false)
{
    MLOGD(LogGroupCore, "ctor +: fwkFrameNum = %d", info.fwkRequest.frameNumber);

    for (const auto &stream : info.streams) {
        if (stream == nullptr) {
            MLOGE(LogGroupCore, "stream in create info is nullptr");
            continue;
        }
        MLOGD(LogGroupCore, "stream id: %d", stream->streamId);
        mStreams.emplace(stream->streamId, stream);
    }

    MLOGD(LogGroupCore, "mStreams.size = %lu", mStreams.size());

    // for error handle
    mMapStreamIdToFwk.emplace(NO_STREAM, NO_STREAM);

    // request fwk buffers from bufferManager
    std::map<int32_t, std::vector<StreamBuffer>> fwkBuffersMap;
    auto ok = requestFwkBuffers(fwkBuffersMap);
    if (!ok) {
        MLOGE(LogGroupCore, "reques fwk buffers failed: ok = %d, fwkBuffersMap.size = %lu", ok,
              fwkBuffersMap.size());
        return;
    }

    auto vndFrameNum = mCurrentCamMode->fetchAddVndFrameNum(mFrameCount);
    mFirstVndFrameNum = vndFrameNum;
    for (size_t i = 0; i < mFrameCount; ++i) {
        std::map<int32_t, std::shared_ptr<MiStream>> streams;
        for (auto &&[streamId, stream] : mStreams) {
            streams.emplace(stream->streamId, stream);
        }
        // request vnd buffers from bufferManager
        std::map<int32_t, std::vector<StreamBuffer>> vndBuffersMap;
        ok = requestVndBuffers(vndBuffersMap, streams);
        if (!ok) {
            MLOGE(LogGroupCore, "reques vnd buffers failed: ok = %d, vndBuffersMap.size = %lu", ok,
                  vndBuffersMap.size());
        }
        auto miRequest = std::make_shared<MiRequest>();
        miRequest->frameNum = vndFrameNum + i;

        auto &vndReq = miRequest->vndRequest;
        vndReq.frameNumber = vndFrameNum + i;

        // optional settings (per frame)
        vndReq.settings = info.settings;
        if (info.settingsPerframe.size() == mFrameCount) {
            vndReq.settings += info.settingsPerframe[i];
        }
        // physical camera settings
        for (auto &&[physicalCameraId, settings] : info.physicalSettings) {
            PhysicalCameraSetting physicalSettings;
            physicalSettings.physicalCameraId = physicalCameraId;
            physicalSettings.settings = settings;
            vndReq.physicalCameraSettings.emplace_back(physicalSettings);
        }

        // prepare vndReq's inputBuffers / outputBuffers
        for (auto &&[streamId, buffers] : vndBuffersMap) {
            vndReq.outputBuffers.emplace_back(buffers[0]);
            miRequest->streams.push_back(mStreams[streamId]);
        }

        if (i == 0) {
            for (auto &&[streamId, buffers] : fwkBuffersMap) {
                vndReq.outputBuffers.emplace_back(buffers[i]);
                miRequest->streams.push_back(mStreams[streamId]);
            }
        }

        mMiRequests[miRequest->frameNum] = miRequest;
    }

    MLOGD(LogGroupCore, "ctor -");
}

bool MiCapture::requestFwkBuffers(std::map<int32_t, std::vector<StreamBuffer>> &bufferMap)
{
    std::map<int32_t, std::shared_ptr<MiStream>> fwkStreams;
    // find stream owned to fwk
    for (auto &&[vndStreamId, stream] : mStreams) {
        if (stream->owner == StreamOwner::StreamOwnerFwk) {
            fwkStreams.emplace(vndStreamId, stream);
        }
    }

    if (fwkStreams.empty()) {
        MLOGD(LogGroupCore, "no need fwk stream buffer");
        return true;
    }

    for (auto &&elem : fwkStreams) {
        const auto &streamId = elem.first;
        const auto &outputBuffers = mFwkRequest.outputBuffers;

        auto it = std::find_if(
            outputBuffers.begin(), outputBuffers.end(),
            [streamId](const StreamBuffer &buffer) { return buffer.streamId == streamId; });
        if (it == outputBuffers.end()) {
            MLOGE(LogGroupCore, "fwkRequest(frameNum = %d) not contain buffer(streamId = %d)",
                  mFwkRequest.frameNumber, streamId);
            continue;
        }

        auto buffer = *it;

        if (!mCameraInfo.supportCoreHBM && (buffer.bufferId == 0 || buffer.buffer == nullptr)) {
            // when Core Session not support HBM, we should request buffers for it here
            auto ok = mCurrentCamMode->getFwkBuffer(mFwkRequest, streamId, buffer);
            if (!ok) {
                MLOGE(LogGroupCore, "fwkRequest(%d) getFwkBuffer for stream(%d) failed!",
                      mFwkRequest.frameNumber, streamId);
            }
        }
        std::vector<StreamBuffer> buffers{buffer};
        bufferMap.emplace(streamId, buffers);
    }

    return !bufferMap.empty();
}

bool MiCapture::requestVndBuffers(std::map<int32_t, std::vector<StreamBuffer>> &bufferMap,
                                  std::map<int32_t, std::shared_ptr<MiStream>> &streams)
{
    if (mCameraInfo.supportAndroidHBM && mCameraInfo.supportCoreHBM) {
        // Core Session support HBM, there is no need to request buffer here
        for (auto &&[streamId, stream] : streams) {
            if (stream->owner == StreamOwner::StreamOwnerVnd) {
                StreamBuffer buffer;
                buffer.streamId = streamId;
                buffer.bufferId = 0;
                buffer.buffer = nullptr;

                std::vector<StreamBuffer> buffers{buffer};
                bufferMap.emplace(streamId, buffers);
            }
        }
        return true;
    }

    // else, request buffers from bufferManager
    auto bufferManager = mCurrentCamMode->getBufferManager();
    if (bufferManager == nullptr) {
        MLOGE(LogGroupCore, "request buffer failed: bufferManager is nullptr");
        return false;
    }

    std::vector<MiBufferManager::MiBufferRequestParam> requestParam; // in param

    for (auto &&[streamId, stream] : streams) {
        // Streams owned to vnd
        if (stream->owner == StreamOwner::StreamOwnerVnd) {
            MLOGD(LogGroupCore, "requesting vnd buffer (streamId = %d) from MiBufferManager...",
                  streamId);

            MiBufferManager::MiBufferRequestParam param{
                .streamId = streamId,
                .bufferNum = 1,
            };
            requestParam.emplace_back(param);
        }
    }

    return bufferManager->requestBuffers(requestParam, bufferMap);
}

int MiCapture::processVendorRequest()
{
    MLOGD(LogGroupCore, "+");
    MICAM_TRACE_SYNC_BEGIN(MialgoTraceCapture, "processVendorRequest");
    int status = Status::OK;

    MLOGD(LogGroupCore, "mMiRequests.size: %lu", mMiRequests.size());
    // NOTE: because result could return mizone during processCaptureRequest call,
    //       copy mMiRequests here to avoid access mMiRequests with result thread at the same time.
    //       and results will never return before call processCaptureRequest, hence the copy
    //       operation is safe here.
    auto miRequests = mMiRequests;
    for (auto &&[frameNum, req] : miRequests) {
        MLOGD(LogGroupCore, "capture req(vndFrameNum: %d, fwkFrameNum: %d): %s", frameNum,
              mFwkRequest.frameNumber, utils::toString(req->vndRequest).c_str());
        status = mCurrentCamMode->forwardRequestToVnd(req->vndRequest);
        if (status != Status::OK) {
            return status;
        }
    }
    MICAM_TRACE_SYNC_END(MialgoTraceCapture);
    MLOGD(LogGroupCore, "-");
    return status;
}

void MiCapture::processVendorNotify(const NotifyMsg &msg)
{
    MLOGD(LogGroupCore, "+");
    auto frameNum =
        msg.type == MsgType::SHUTTER ? msg.msg.shutter.frameNumber : msg.msg.error.frameNumber;
    MLOGD(LogGroupCore, "capture msg(vndFrameNum: %d, fwkFrameNum: %d): %s", frameNum,
          mFwkRequest.frameNumber, utils::toString(msg).c_str());
    if (msg.type == MsgType::SHUTTER) {
        processShutterMsg(msg.msg.shutter);
    } else if (msg.type == MsgType::ERROR) {
        MLOGE(LogGroupCore, "recive error msg: frameNum = %d", msg.msg.error.frameNumber);
        processErrorMsg(msg.msg.error);
    }

    // update record in mMiRequests
    updateRecords(msg);

    // notify completed
    if (isCompleted()) {
        MLOGD(LogGroupCore, "capture (%d) is completed", mFwkRequest.frameNumber);
        onCompleted();
    }
    MLOGD(LogGroupCore, "-");
}

void MiCapture::processVendorResult(const CaptureResult &result)
{
    MLOGD(LogGroupCore, "+");

    MLOGD(LogGroupCore, "capture res(vndFrameNum: %d, fwkFrameNum: %d): %s", result.frameNumber,
          mFwkRequest.frameNumber, utils::toString(result).c_str());

    if (!result.result.isEmpty() || !result.halResult.isEmpty() ||
        !result.physicalCameraMetadata.empty()) {
        processMetaResult(result);
    }

    if (!result.inputBuffers.empty() || !result.outputBuffers.empty()) {
        processBufferResult(result);
    }

    // update record in mMiRequests
    updateRecords(result);

    if (mSingleFrameEnable && isCompleted(result.frameNumber)) {
        singleFrameOnCompleted(result.frameNumber);
    }

    // notify completed
    if (isCompleted()) {
        MLOGD(LogGroupCore, "capture (%d) is completed", mFwkRequest.frameNumber);
        onCompleted();
    }
    MLOGD(LogGroupCore, "-");
}

std::map<uint32_t, std::shared_ptr<MiRequest>> MiCapture::getMiRequests()
{
    return mMiRequests;
}

void MiCapture::updateRecords(const CaptureResult &result)
{
    auto it = mMiRequests.find(result.frameNumber);
    if (it == mMiRequests.end()) {
        MLOGE(LogGroupCore, "recieved result (frameNum = %d) not in mMiRequests",
              result.frameNumber);
        return;
    }
    auto miRequest = it->second;

    // update meta result in mMiRequests
    miRequest->result += result.result;
    miRequest->halResult += result.halResult;
    for (auto &&phyMeta : result.physicalCameraMetadata) {
        miRequest->physicalCameraMetadata[phyMeta.physicalCameraId].physicalCameraId =
            phyMeta.physicalCameraId;
        miRequest->physicalCameraMetadata[phyMeta.physicalCameraId].metadata += phyMeta.metadata;
        miRequest->physicalCameraMetadata[phyMeta.physicalCameraId].halMetadata +=
            phyMeta.halMetadata;
    }
    if (!miRequest->isFullMetadata) {
        miRequest->isFullMetadata = result.bLastPartialResult;
    }

    // update buffers
    // NOTE: when HBM enable, buffers in miRequest->vndRequest may be null,
    //       so need to update buffers recieved from result to miRequest->vndRequest (AlgoEngine
    //       will used them directly when reprocess)
    auto updateBuffers = [](const StreamBuffer &resBuffer, std::vector<StreamBuffer> &reqBuffers) {
        auto it = std::find_if(reqBuffers.begin(), reqBuffers.end(),
                               [resBuffer](const StreamBuffer &buffer) {
                                   return resBuffer.streamId == buffer.streamId;
                               });
        if (it != reqBuffers.end()) {
            *it = resBuffer;
        }
    };
    for (auto &&resultBuffer : result.outputBuffers) {
        updateBuffers(resultBuffer, miRequest->vndRequest.outputBuffers);
    }
    for (auto &&resultBuffer : result.inputBuffers) {
        updateBuffers(resultBuffer, miRequest->vndRequest.inputBuffers);
    }

    // update buffer count in mMiRequests
    miRequest->receivedBuffersCount += result.inputBuffers.size();
    miRequest->receivedBuffersCount += result.outputBuffers.size();
}

void MiCapture::updateRecords(const NotifyMsg &msg)
{
    if (msg.type == MsgType::UNKNOWN) {
        MLOGE(LogGroupCore, "unkown error type, should not recieved!");
        return;
    }
    if (msg.type == MsgType::ERROR) {
        if (msg.msg.error.errorCode == ErrorCode::ERROR_DEVICE ||
            msg.msg.error.errorCode == ErrorCode::ERROR_BUFFER ||
            msg.msg.error.errorCode == ErrorCode::ERROR_REQUEST) {
            // NOTE: with default MTK HAL Layer callback rule (MTK_HALCORE_BYPASS_ALL),
            //       MTK HAL Layer should only return ERROR_RESULT
            MLOGE(LogGroupCore, "this error (code = %d) should not be return from Core Session!",
                  msg.msg.error.errorCode);
            return;
        }
    }

    // find coresponding MiRequest
    auto frameNum =
        msg.type == MsgType::SHUTTER ? msg.msg.shutter.frameNumber : msg.msg.error.frameNumber;
    auto it = mMiRequests.find(frameNum);
    if (it == mMiRequests.end()) {
        MLOGE(LogGroupCore, "recieved msg (frameNum = %d) not in mMiRequests", frameNum);
        return;
    }
    auto miRequest = it->second;

    if (msg.type == MsgType::SHUTTER) {
        miRequest->shutter = msg.msg.shutter.timestamp;
    } else if (msg.type == MsgType::ERROR) {
        miRequest->isError = true;
        mIsError = true;
    }
}

bool MiCapture::isCompleted()
{
    // all mireqs are completed
    return std::all_of(mMiRequests.begin(), mMiRequests.end(), [](auto &&p) {
        const std::shared_ptr<MiRequest> &miReq = p.second;
        return (miReq->isFullMetadata || miReq->isError) &&
               miReq->receivedBuffersCount >= miReq->streams.size();
    });
}

bool MiCapture::isCompleted(uint32_t frameNumber)
{
    auto it = mMiRequests.find(frameNumber);
    if (it == mMiRequests.end()) {
        MLOGE(LogGroupCore, "recieved result (frameNum = %d) not in mMiRequests", frameNumber);
        return false;
    }
    auto miReq = it->second;

    return (miReq->isFullMetadata || miReq->isError) &&
           miReq->receivedBuffersCount >= miReq->streams.size();
}

void MiCapture::pickBuffersOwnedToFwk(const std::vector<StreamBuffer> &vndBuffers /* input */,
                                      std::vector<StreamBuffer> &fwkBuffers /* output */)
{
    fwkBuffers.clear();
    for (const auto &buffer : vndBuffers) {
        auto streamId = buffer.streamId;
        if (mStreams[streamId]->owner == StreamOwner::StreamOwnerFwk) {
            auto fwkBuffer = buffer;
            fwkBuffer.streamId = mMapStreamIdToFwk[streamId]; // map streamId to framework streamId
            fwkBuffers.emplace_back(fwkBuffer);
        }
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// MiPreviewCapture
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MiPreviewCapture::MiPreviewCapture(const CreateInfo &info) : MiCapture(info)
{
    MLOGD(LogGroupCore, "ctor");
}

MiPreviewCapture::~MiPreviewCapture()
{
    MLOGD(LogGroupCore, "dtor");
}

int MiPreviewCapture::processVendorRequest()
{
    auto status = MiCapture::processVendorRequest();
    mCurrentCamMode->updateSettings(mFwkRequest.settings);
    return status;
}

void MiPreviewCapture::processVendorResult(const CaptureResult &result)
{
    MiCapture::processVendorResult(result);
}

void MiPreviewCapture::processMetaResult(const CaptureResult &result)
{
    MLOGD(LogGroupCore, "+");
    if (mIsError) {
        // NOTE: MTK Android Adaptor Layer will ignored meta result after error occured,
        //       no need to forward result to it any more.
        MLOGE(LogGroupCore, "capture (fwkFrameNum: %d) is error!", mFwkRequest.frameNumber);
        return;
    }

    // 1. update result status to MiDecision, if is last partial meta
    if (result.bLastPartialResult) {
        auto miRequest = mMiRequests[result.frameNumber];
        mCurrentCamMode->updateStatus(miRequest->result + result.result);
    }

    // 2. foward to fwk
    CaptureResult fwkResult;
    fwkResult.frameNumber = mFwkRequest.frameNumber; // NOTE: change to fwkFrameNum
    fwkResult.halResult = result.halResult;
    fwkResult.result = result.result;
    fwkResult.bLastPartialResult = result.bLastPartialResult;
    if (!fwkResult.result.isEmpty() || !fwkResult.halResult.isEmpty()) {
        mCurrentCamMode->forwardResultToFwk(fwkResult);
    }
    MLOGD(LogGroupCore, "-");
}

void MiPreviewCapture::processBufferResult(const CaptureResult &result)
{
    MLOGD(LogGroupCore, "+");
    // NOTE: 1. should change frameNumber / streamId here
    //       2. should remove buffer owned to vendor, e.g., quick view cached buffer
    CaptureResult fwkResult;
    fwkResult.frameNumber = mFwkRequest.frameNumber; // NOTE: change to fwkFrameNum
    pickBuffersOwnedToFwk(result.inputBuffers, fwkResult.inputBuffers);
    pickBuffersOwnedToFwk(result.outputBuffers, fwkResult.outputBuffers);

    if (!fwkResult.outputBuffers.empty() || !fwkResult.inputBuffers.empty()) {
        mCurrentCamMode->forwardResultToFwk(fwkResult);
    }

    if (mQuickViewEnable) {
        // update quick view buffer
        for (const auto &buffer : result.outputBuffers) {
            auto streamId = buffer.streamId;
            if (mStreams[streamId]->usageType == StreamUsageType::QuickViewCachedStream) {
                auto miRequest = mMiRequests[result.frameNumber];
                MLOGD(LogGroupCore, "updating quick view buffer, shutter = %lu, buffer = %p",
                      miRequest->shutter, buffer.buffer.get());
                if (miRequest->shutter == 0) {
                    MLOGE(LogGroupCore, "invalid quick view shutter: %lu", miRequest->shutter);
                }
                mCurrentCamMode->updateQuickViewBuffer(mFwkRequest.frameNumber, miRequest->shutter,
                                                       buffer);
                break;
            }
        }
    }
    MLOGD(LogGroupCore, "-");
}

void MiPreviewCapture::processVendorNotify(const NotifyMsg &msg)
{
    MiCapture::processVendorNotify(msg);
}

void MiPreviewCapture::processShutterMsg(const ShutterMsg &shutterMsg)
{
    NotifyMsg fwkMsg{};
    fwkMsg.type = MsgType::SHUTTER;
    fwkMsg.msg.shutter.frameNumber = mFwkRequest.frameNumber;
    fwkMsg.msg.shutter.timestamp = shutterMsg.timestamp;

    mCurrentCamMode->forwardNotifyToFwk(fwkMsg);
}

void MiPreviewCapture::processErrorMsg(const ErrorMsg &errorMsg)
{
    NotifyMsg fwkMsg{};
    fwkMsg.type = MsgType::ERROR;
    fwkMsg.msg.error.frameNumber = mFwkRequest.frameNumber; // need change to fwkFrameNum
    fwkMsg.msg.error.errorCode = errorMsg.errorCode;
    fwkMsg.msg.error.errorStreamId =
        mMapStreamIdToFwk[errorMsg.errorStreamId]; // map streamId to framework streamId

    mCurrentCamMode->forwardNotifyToFwk(fwkMsg);
}

// frame by frame transfer is used for snapshot capture.This function is set empty.
void MiPreviewCapture::singleFrameOnCompleted(int32_t frameNumber) {}
void MiPreviewCapture::updateResult(std::shared_ptr<MiRequest> miReq){};

void MiPreviewCapture::onCompleted()
{
    if (mIsError) {
        MLOGD(LogGroupCore, "error capture (fwkFrameNum: %d), release resources!",
              mFwkRequest.frameNumber);

        // release buffer owned to hal
        for (auto &&[vndFrameNum, miReq] : mMiRequests) {
            std::vector<StreamBuffer> buffers;
            for (auto &&buffer : miReq->vndRequest.outputBuffers) {
                if (mStreams[buffer.streamId]->owner == StreamOwner::StreamOwnerVnd) {
                    buffers.push_back(buffer);
                }
            }
            mCurrentCamMode->getBufferManager()->releaseBuffers(buffers);
        }
        mMiRequests.clear();
    }
    mCurrentCamMode->onPreviewCaptureCompleted(mFwkRequest.frameNumber, mFirstVndFrameNum);
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// MiSnapshotCapture
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MiSnapshotCapture::MiSnapshotCapture(const CreateInfo &info)
    : MiCapture(info),
      mQuickViewHasNotified(false),
      mQuickShotDelayTime(info.quickShotDelayTime),
      mQuickShotEnabled(info.mQuickShotEnabled),
      mIsNeedWait(true),
      mIsZslEnable(info.isZslEnable),
      mIsAppZslEnable(info.isAppZslEnable)
{
    MLOGD(LogGroupCore, "ctor");
    if (mQuickShotDelayTime > 0) {
        MLOGI(LogGroupCore, "QuickShot delay time is %d", mQuickShotDelayTime);
        mQuickShotTimer = Timer::createTimer();
        mQuickShotTimer->runAfter(mQuickShotDelayTime, [this]() {
            mCurrentCamMode->updatePendingSnapRecords(mFwkRequest.frameNumber,
                                                      std::to_string(mFwkRequest.frameNumber),
                                                      SnapMessageType::HAL_BUFFER_RESULT);
            mIsNeedWait = false;
            mDelayCond.notify_all();
            MLOGD(LogGroupCore, "QuickShot HAL_BUFFER_RESULT is notify");
        });
    }

    // NOTE: Buffers for snapshot, should add
    //    MTK_HALCORE_IMAGE_PROCESSING_SETTINGS = MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_HIGH_QUALITY
    //    in bufferSettings, otherwise, this request will not enter P2C
    int snapshotBufferCount = 0;
    for (auto &&[vndFrameNum, miReq] : mMiRequests) {
        for (auto &&buffer : miReq->vndRequest.outputBuffers) {
            if (mStreams[buffer.streamId]->usageType == StreamUsageType::SnapshotStream) {
                MiMetadata::setEntry<MINT32>(&buffer.bufferSettings,
                                             MTK_HALCORE_IMAGE_PROCESSING_SETTINGS,
                                             MTK_HALCORE_IMAGE_PROCESSING_SETTINGS_HIGH_QUALITY);
                snapshotBufferCount++;
            }
        }
    }

    // set mutiframe input number for postProcessSession
    if (mCaptureTypeName.find("MI_ALGO_MIHAL_MFNR_HDR") != std::string::npos) {
        int32_t mfnrNum = 0;
        int32_t hdrNum = 0;
        for (auto &&[vndFrameNum, miReq] : mMiRequests) {
            uint8_t mfnrEnable = 0;
            MiMetadata::getEntry<uint8_t>(&miReq->vndRequest.settings, XIAOMI_MFNR_ENABLED,
                                          mfnrEnable);
            if (mfnrEnable) {
                mfnrNum++;
            } else {
                hdrNum++;
            }
        }
        MLOGD(LogGroupHAL, "MFNR + HDR situation, mfnrNum:%u, hdrNum:%u, totalNum:%lu", mfnrNum,
              hdrNum, mMiRequests.size());

        int inBufferIndex = 1;
        for (auto &&[vndFrameNum, miReq] : mMiRequests) {
            if (inBufferIndex++ > mfnrNum) {
                MiMetadata::setEntry(&miReq->result, XIAOMI_MULTIFEAME_INPUTNUM, hdrNum);
            } else {
                MiMetadata::setEntry(&miReq->result, XIAOMI_MULTIFEAME_INPUTNUM, mfnrNum);
            }
        }
    } else if (mCaptureTypeName.find("MI_ALGO_SR_HDR") != std::string::npos) {
        int32_t srNum = 0;
        int32_t hdrNum = 0;
        for (auto &&[vndFrameNum, miReq] : mMiRequests) {
            MINT32 ev_value = 0;
            MiMetadata::getEntry<MINT32>(&miReq->vndRequest.settings,
                                         ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, ev_value);
            if (ev_value == 0) {
                srNum++;
            } else {
                hdrNum++;
            }
        }
        MLOGD(LogGroupHAL, "SR + HDR situation, srNum:%u, hdrNum:%u, totalNum:%lu", srNum, hdrNum,
              mMiRequests.size());

        int inBufferIndex = 1;
        for (auto &&[vndFrameNum, miReq] : mMiRequests) {
            if (inBufferIndex++ > srNum) {
                MiMetadata::setEntry(&miReq->result, XIAOMI_MULTIFEAME_INPUTNUM, hdrNum);
            } else {
                MiMetadata::setEntry(&miReq->result, XIAOMI_MULTIFEAME_INPUTNUM, srNum);
            }
        }
    } else if (mCaptureTypeName.find("MI_ALGO_BOKEH_HDR") != std::string::npos) {
        int32_t mfnrNum = 4;
        int inBufferIndex = 1;
        for (auto &&[vndFrameNum, miReq] : mMiRequests) {
            if (inBufferIndex++ > mfnrNum) {
                MiMetadata::setEntry(&miReq->result, XIAOMI_MULTIFEAME_INPUTNUM, 1);
            } else {
                MiMetadata::setEntry(&miReq->result, XIAOMI_MULTIFEAME_INPUTNUM, mfnrNum);
            }
        }
    } else {
        for (auto &&[vndFrameNum, miReq] : mMiRequests) {
            MiMetadata::setEntry(&miReq->vndRequest.settings, XIAOMI_MULTIFEAME_INPUTNUM,
                                 static_cast<int32_t>(mMiRequests.size()));
        }
        MLOGD(LogGroupCore, "set multiFrameNum :%d", mMiRequests.size());
    }
    // set snapshot image name for postProcessSession
    for (auto &&[vndFrameNum, miReq] : mMiRequests) {
        std::string pipelineName(mCaptureTypeName);
        pipelineName += "_" + std::to_string(info.fwkRequest.frameNumber);
        MiMetadata::updateEntry<char, MUINT8>(miReq->vndRequest.settings, XIAOMI_SNAPSHOT_IMAGENAME,
                                              pipelineName.c_str(), pipelineName.size() + 1);

        MiMetadata::setEntry<MINT32>(&miReq->vndRequest.settings, XIAOMI_BURST_MIALGOTOTALREQNUM,
                                     snapshotBufferCount);
        MLOGD(LogGroupCore, "vndFrameNum: %d, snapshotBufferCount:%d", vndFrameNum,
              snapshotBufferCount);
    }
}

MiSnapshotCapture::~MiSnapshotCapture()
{
    MLOGD(LogGroupCore, "dtor");
}

int MiSnapshotCapture::processVendorRequest()
{
    return MiCapture::processVendorRequest();
}

void MiSnapshotCapture::processVendorResult(const CaptureResult &result)
{
    MiCapture::processVendorResult(result);
}

void MiSnapshotCapture::processMetaResult(const CaptureResult &result)
{
    MLOGD(LogGroupCore, "+");

    if (mIsError) {
        // NOTE: MTK Android Adaptor Layer will ignored meta result after error occured,
        //       no need to forward result to it any more.
        MLOGE(LogGroupCore, "capture (fwkFrameNum: %d) is error!", mFwkRequest.frameNumber);
        return;
    }

    // notify quick view
    if (mQuickViewEnable && !mQuickViewHasNotified && !result.result.isEmpty()) {
        // TODO: XIAOMI_QUICKVIEW_MFNR_ANCHOR_TIME_STAMP need redefine in decouple utils
        auto entry = result.result.entryFor(XIAOMI_QUICKVIEW_MFNR_ANCHOR_TIME_STAMP);
        if (!entry.isEmpty()) {
            MLOGD(LogGroupCore, "notify quick view with anchor frame shutter");
            auto anchorFrameTimestamp = *reinterpret_cast<const int64_t *>(entry.data());
            // mCurrentCamMode->notifyQuickView(mFwkRequest.frameNumber, anchorFrameTimestamp);
            uint32_t anchorFwkNumber = mCurrentCamMode->getAnchorFrameNumber(anchorFrameTimestamp);
            mCurrentCamMode->addAnchorFrame(mFwkRequest.frameNumber, anchorFwkNumber);
            MLOGD(LogGroupCore,
                  "notify quick view with fwkNumber:%d anchor fwkNumber:%d shutter:%lu",
                  mFwkRequest.frameNumber, anchorFwkNumber, anchorFrameTimestamp);
            mQuickViewHasNotified = true;
        }
    }

    // only forward result of first frame to fwk
    if (result.frameNumber != mFirstVndFrameNum) {
        return;
    }

    CaptureResult fwkResult;
    fwkResult.frameNumber = mFwkRequest.frameNumber; // NOTE: change to fwkFrameNum
    fwkResult.halResult = result.halResult;
    fwkResult.result = result.result;
    if (mIsAppZslEnable) {
        fwkResult.bLastPartialResult =
            false; // should false, we may need return meta after reprocess
    } else {
        fwkResult.bLastPartialResult = result.bLastPartialResult;
    }
    if (!fwkResult.result.isEmpty() || !fwkResult.halResult.isEmpty()) {
        mCurrentCamMode->forwardResultToFwk(fwkResult);
    }
    MLOGD(LogGroupCore, "-");
}

void MiSnapshotCapture::processBufferResult(const CaptureResult &result)
{
    MLOGD(LogGroupCore, "+");
    // only forward result of first frame to fwk
    if (result.frameNumber != mFirstVndFrameNum) {
        return;
    }

    // NOTE: 1. should change frameNumber / streamId here
    //       2. should remove buffer owned to vendor, e.g., quick view cached buffer
    CaptureResult fwkResult;
    fwkResult.frameNumber = mFwkRequest.frameNumber; // change to fwkFrameNum
    pickBuffersOwnedToFwk(result.inputBuffers, fwkResult.inputBuffers);
    pickBuffersOwnedToFwk(result.outputBuffers, fwkResult.outputBuffers);

    if (!fwkResult.outputBuffers.empty() || !fwkResult.inputBuffers.empty()) {
        mCurrentCamMode->forwardResultToFwk(fwkResult);
    }
    MLOGD(LogGroupCore, "-");
}

void MiSnapshotCapture::processVendorNotify(const NotifyMsg &msg)
{
    MiCapture::processVendorNotify(msg);
}

void MiSnapshotCapture::processShutterMsg(const ShutterMsg &shutterMsg)
{
    // NOTE: for snapshot, return shutter after data collected (for shot2shot control),
    //       so do nothing here.

    if (shutterMsg.frameNumber == mFirstVndFrameNum && mQuickShotDelayTime == 0 &&
        mQuickShotEnabled) {
        mCurrentCamMode->updatePendingSnapRecords(mFwkRequest.frameNumber,
                                                  std::to_string(mFwkRequest.frameNumber),
                                                  SnapMessageType::HAL_BUFFER_RESULT);
    }
    if (shutterMsg.frameNumber == mFirstVndFrameNum + mMiRequests.size() - 1) {
        mCurrentCamMode->updatePendingSnapRecords(mFwkRequest.frameNumber,
                                                  std::to_string(mFwkRequest.frameNumber),
                                                  SnapMessageType::HAL_SHUTTER_NOTIFY);
        MLOGD(LogGroupCore, "HAL_SHUTTER_NOTIFY is notify");
    }

#if 1
    // only forward result of first frame to fwk
    if (shutterMsg.frameNumber != mFirstVndFrameNum) {
        return;
    }

    NotifyMsg fwkMsg{};
    fwkMsg.type = MsgType::SHUTTER;
    fwkMsg.msg.shutter.frameNumber = mFwkRequest.frameNumber;
    fwkMsg.msg.shutter.timestamp = shutterMsg.timestamp;

    mCurrentCamMode->forwardNotifyToFwk(fwkMsg);
#endif
}

void MiSnapshotCapture::processErrorMsg(const ErrorMsg &errorMsg)
{
    // NOTE: for error msg, should forward every msg for every frame, rather than only first frame
    // if (errorMsg.frameNumber != mFirstVndFrameNum) {
    //    return;
    // }

    NotifyMsg fwkMsg{};
    fwkMsg.type = MsgType::ERROR;
    fwkMsg.msg.error.frameNumber = mFwkRequest.frameNumber; // change to fwkFrameNum
    fwkMsg.msg.error.errorCode = errorMsg.errorCode;
    fwkMsg.msg.error.errorStreamId =
        mMapStreamIdToFwk[errorMsg.errorStreamId]; // map streamId to framework streamId

    mCurrentCamMode->forwardNotifyToFwk(fwkMsg);
}

void MiSnapshotCapture::singleFrameOnCompleted(int32_t frameNumber)
{
    if (!mIsError) {
        if (mRequestsCompleted.find(frameNumber) == mRequestsCompleted.end()) {
            //下面这个find一定会成功的，若不然，就不会调用到singleFrameOnCompleted函数，在isCompleted(result.frameNumber)中已经查找过一次了。
            auto req = mMiRequests.find(frameNumber)->second;
            updateResult(req);

            // send frame to algoEngine one by one
            // mCurrentCamMode->reprocess(mFwkRequest.frameNumber, std::move(req),
            //                                   mCaptureTypeName, isFirstFrame);
            auto res = mCurrentCamMode->goToAlgoEngine(frameNumber);
            if (res == -1) {
                MLOGE(LogGroupCore,
                      "Snapshot(fwk frameNum: %d,vnd frameNum: %d) go to Algoengine failed",
                      mFwkRequest.frameNumber, frameNumber);
            }
            mRequestsCompleted.insert(frameNumber);
        }
    }
}

void MiSnapshotCapture::updateResult(std::shared_ptr<MiRequest> miReq)
{
    // set shutter for postProcessSession
    MiMetadata::setEntry(&miReq->result, ANDROID_SENSOR_TIMESTAMP,
                         static_cast<int64_t>(miReq->shutter));
}

void MiSnapshotCapture::onCompleted()
{
    if (mInAlgoEngine) {
        // this capture is already sent to AlgoEngine
        MLOGE(LogGroupCore, "capture is already sent to AlgoEngine!");
        return;
    }

    // save vndFrameNums, before mMiRequest moved
    std::vector<uint32_t> vndFrameNums;
    vndFrameNums.reserve(mMiRequests.size());
    for (auto &&[frameNum, req] : mMiRequests) {
        vndFrameNums.emplace_back(frameNum);
        if (!mSingleFrameEnable) {
            updateResult(req);
        }
    }

    if (mIsError) {
        MLOGD(LogGroupCore,
              "error capture (fwkFrameNum: %d), stop send to AlgoEngine, and release resources!",
              mFwkRequest.frameNumber);

        // release buffer owned to hal
        for (auto &&[vndFrameNum, miReq] : mMiRequests) {
            std::vector<StreamBuffer> buffers;
            for (auto &&buffer : miReq->vndRequest.outputBuffers) {
                if (mStreams[buffer.streamId]->owner == StreamOwner::StreamOwnerVnd) {
                    buffers.push_back(buffer);
                }
            }
            mCurrentCamMode->getBufferManager()->releaseBuffers(buffers);
        }
        mMiRequests.clear();

        if (mSingleFrameEnable) {
            MLOGI(LogGroupCore, "collect buffer occured error,begin quickFinish");
            std::string pipelineName =
                mCaptureTypeName + "_" + std::to_string(mFwkRequest.frameNumber);
            mCurrentCamMode->quickFinish(pipelineName);
        } else {
            // return buffers owned to fwk
            CaptureResult fwkResult;
            fwkResult.frameNumber = mFwkRequest.frameNumber;
            fwkResult.bLastPartialResult = true;
            for (auto buffer : mFwkRequest.outputBuffers) {
                buffer.status = BufferStatus::ERROR;
                fwkResult.outputBuffers.push_back(buffer);
            }
            mCurrentCamMode->forwardResultToFwk(fwkResult);
            mCurrentCamMode->notifyCaptureFail(mFwkRequest.frameNumber,
                                               std::to_string(mFwkRequest.frameNumber));
        }
    } else {
        // return shutter
        if (mQuickShotDelayTime == 0) {
            mCurrentCamMode->updatePendingSnapRecords(mFwkRequest.frameNumber,
                                                      std::to_string(mFwkRequest.frameNumber),
                                                      SnapMessageType::HAL_BUFFER_RESULT);
        }

        // {
        //     if(mMiRequests.size()!=0){
        //         auto miReq=mMiRequests.begin()->second;
        //         //for(auto &&[vndFrameNum, miReq] : mMiRequests)
        //             updateMiOutputMeta(miReq);
        //     }
        // }
        // send to AlgoEngine
        MLOGI(LogGroupCore, "Snapshot(fwk frameNum: %d) data collected, send to AlgoEngine.",
              mFwkRequest.frameNumber);
        if (!mSingleFrameEnable) {
            // mCurrentCamMode->reprocess(mFwkRequest.frameNumber, std::move(mMiRequests),
            //                           mCaptureTypeName
            auto res = mCurrentCamMode->goToAlgoEngine(mFirstVndFrameNum);
            mInAlgoEngine = true;
            if (res == -1) {
                MLOGE(LogGroupCore, "Snapshot(fwk frameNum: %d) go to Algoengine failed",
                      mFwkRequest.frameNumber);
            }
        }

        // mMiRequests.clear();
    }
    if (mQuickShotDelayTime > 0 && mIsNeedWait) {
        std::unique_lock<std::mutex> lock(mDelayMutex);
        mDelayCond.wait(lock, [this]() { return !mIsNeedWait; });
    }
    if (mIsZslEnable && !mQuickViewHasNotified) {
        std::unique_lock<std::mutex> lock(mDelayMutex);
        mDelayCond.wait(lock, [this]() { return !mQuickViewHasNotified; });
    }
    mCurrentCamMode->onSnapshotCaptureCompleted(mFwkRequest.frameNumber, vndFrameNums);
    if (mCaptureTypeName.find("MI_ALGO_VENDOR_MFNR") ||
        mCaptureTypeName.find("MI_ALGO_VENDOR_MFNR_HDR")) {
        mCurrentCamMode->subPendingVendorMFNR();
    }
}

void MiSnapshotCapture::updateMiOutputMeta(std::shared_ptr<MiRequest> req)
{
    if (mOperationMode == StreamConfigModeAlgoupDualBokeh) {
        updateBokehMeta(req);
    }

    {
        int32_t ret = 0;
        {
            uint8_t masterID;
            if (MiMetadata::tryGetMetadata(&mCameraInfo.staticInfo,
                                           XIAOMI_CAMERA_BOKEHINFO_MASTERCAMERAID, masterID))
                MLOGD(LogGroupCore, "MTK_MULTI_CAM_MASTER_ID masterID: %d", masterID);

            MultiCameraIdRole role;
            memset(&role, 0, sizeof(MultiCameraIdRole));
            role.currentCameraId = masterID;
            role.currentCameraRole = getCameraRole(masterID);
            ret = MiMetadata::updateMemory(req->result, XIAOMI_CAMERA_ALGOUP_MULTICAMROLE, role);
            MLOGD(LogGroupCore, "output  XIAOMI_CAMERA_ALGOUP_MULTICAMROLE write:%d, ret:%d",
                  role.currentCameraRole, ret);

            if (MiMetadata::trySetMetadata(&req->result, XIAOMI_QCFA_SUPPORTED,
                                           (MUINT8)(masterID == 0 ? 1 : 0)))
                MLOGD(LogGroupCore, "output  XIAOMI_QCFA_SUPPORTED write:%d",
                      (MUINT8)(masterID == 0 ? 1 : 0));
        }

        auto FaceRects = req->result.entryFor(MTK_STATISTICS_FACE_RECTANGLES);
        OutputMetadataFaceAnalyze metaFace;
        memset(&metaFace, 0, sizeof(OutputMetadataFaceAnalyze));
        if (!FaceRects.isEmpty()) {
            metaFace.faceNum = FaceRects.count();
            ret = MiMetadata::updateMemory(req->result, XIAOMI_FACE_ANALYZE_RESULT, metaFace);
            MLOGD(LogGroupCore, "output  XIAOMI_FACE_ANALYZE_RESULT write:%d, ret:%d",
                  metaFace.faceNum, ret);
        } else {
            MLOGD(LogGroupCore,
                  "output  get  MTK_STATISTICS_FACE_RECTANGLES error, using numFaces 0");
            metaFace.faceNum = 0;
            ret = MiMetadata::updateMemory(req->result, XIAOMI_FACE_ANALYZE_RESULT, metaFace);
        }

        auto entry = mCameraInfo.staticInfo.entryFor(MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION);
        if (!entry.isEmpty()) {
            MRect activeArray = entry.itemAt(0, Type2Type<MRect>());
            MiDimension dimen;
            dimen.width = activeArray.s.w;
            dimen.height = activeArray.s.h;
            ret = MiMetadata::updateMemory(req->result, XIAOMI_SENSOR_SIZE, dimen);
            MLOGD(LogGroupCore, "output  XIAOMI_SENSOR_SIZE write:%dx%d, ret:%d", dimen.width,
                  dimen.height, ret);
        } else {
            MLOGE(LogGroupCore, "output  get  MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION error");
        }
    }
}

#define DEF_CAMERA_NUM_MAX     10
#define DEF_CAMERA_ROLE_OFFSET 10 // for skip static int default value 0
static MINT32 S_CameraRole[DEF_CAMERA_NUM_MAX] = {0};

CameraRoleType MiSnapshotCapture::getCameraRole(const MINT32 masterID)
{
    if (DEF_CAMERA_NUM_MAX - 1 < masterID) {
        MLOGE(LogGroupCore, "getCameraRole masterID:%d error", masterID);
        return CameraRoleTypeDefault;
    }

    CameraRoleType nRet = CameraRoleTypeDefault;
    if (0 == S_CameraRole[masterID]) {
        auto staticMeta = DecoupleUtil::getStaticMeta(masterID);
        if (!staticMeta.isEmpty()) {
            auto entry = staticMeta.entryFor(XIAOMI_CAMERA_ID_ALIAS);
            if (!entry.isEmpty()) {
                S_CameraRole[masterID] =
                    DEF_CAMERA_ROLE_OFFSET + entry.itemAt(0, Type2Type<MINT32>());
                MLOGD(LogGroupCore, "getCameraRole %d IXIAOMI_CAMERA_ID_ALIAS get %d", masterID,
                      S_CameraRole[masterID] - DEF_CAMERA_ROLE_OFFSET);
            } else {
                MLOGE(LogGroupCore, "getCameraRole %d IXIAOMI_CAMERA_ID_ALIAS isEmpty!", masterID);
            }
        }
    }

    switch (S_CameraRole[masterID] - DEF_CAMERA_ROLE_OFFSET) {
    case 0:
        nRet = CameraRoleTypeWide;
        break;
    case 24:
        nRet = CameraRoleTypeMacro;
        break;
    case 21:
        nRet = CameraRoleTypeUltraWide;
        break;
    case 20:
        nRet = CameraRoleTypeTele;
        break;
    default:
        nRet = CameraRoleTypeDefault;
        break;
    }

    MLOGD(LogGroupCore, "getCameraRole masterID:%d, alias:%d, ret:%d", masterID,
          S_CameraRole[masterID] - DEF_CAMERA_ROLE_OFFSET, nRet);
    return nRet;
}

inline void checkAFRoi(const MiMetadata::MiEntry entry, const MBOOL isMain, CameraMetadata *outMeta)
{
    const MINT32 afROINum = entry.itemAt(1, Type2Type<MINT32>());
    if ((afROINum == 1) || (afROINum == 2)) {
        const MINT32 afValStart = (afROINum == 1) ? 2 : 7;
        //
        const MINT32 afTopLeftX = entry.itemAt(afValStart + 0, Type2Type<MINT32>());
        const MINT32 afTopLeftY = entry.itemAt(afValStart + 1, Type2Type<MINT32>());
        const MINT32 afBottomRightX = entry.itemAt(afValStart + 2, Type2Type<MINT32>());
        const MINT32 afBottomRightY = entry.itemAt(afValStart + 3, Type2Type<MINT32>());
        const MINT32 afType = entry.itemAt(afValStart + 4, Type2Type<MINT32>());
        outMeta->afFocusROI.xMin = afTopLeftX;
        outMeta->afFocusROI.yMin = afTopLeftY;
        outMeta->afFocusROI.xMax = afBottomRightX;
        outMeta->afFocusROI.yMax = afBottomRightY;
        outMeta->afType = afType;
        MLOGD(LogGroupCore, "%s af roi, info:[t:%d, n:%d, vs:%d, tlx:%d, tly:%d, brx:%d, bry:%d]",
              (isMain ? "Main" : "Sub"), afType, afROINum, afValStart, afTopLeftX, afTopLeftY,
              afBottomRightX, afBottomRightY);
    } else {
        MLOGE(LogGroupCore, "invalid afROINum:%d, and set as default", afROINum);
    }
}
inline void checkFOVRegion(const MiMetadata::MiEntry entry, const MBOOL isMain,
                           CameraMetadata *outMeta)
{
    MRect rFovCrop;
    if (isMain) {
        rFovCrop = entry.itemAt(0, Type2Type<MRect>());
        // rFovCrop.p.x = entry.itemAt(0, Type2Type<MINT32>());
        // rFovCrop.p.y = entry.itemAt(1, Type2Type<MINT32>());
        // rFovCrop.s.w = entry.itemAt(2, Type2Type<MINT32>());
        // rFovCrop.s.h = entry.itemAt(3, Type2Type<MINT32>());
    } else {
        rFovCrop = entry.itemAt(1, Type2Type<MRect>());
        // rFovCrop.p.x = entry.itemAt(4, Type2Type<MINT32>());
        // rFovCrop.p.y = entry.itemAt(5, Type2Type<MINT32>());
        // rFovCrop.s.w = entry.itemAt(6, Type2Type<MINT32>());
        // rFovCrop.s.h = entry.itemAt(7, Type2Type<MINT32>());
    }

    outMeta->fovRectIFE.left = rFovCrop.p.x;
    outMeta->fovRectIFE.top = rFovCrop.p.y;
    outMeta->fovRectIFE.width = rFovCrop.s.w;
    outMeta->fovRectIFE.height = rFovCrop.s.h;

    MLOGD(LogGroupCore, "%s get fov crop region:(%d, %d)x(%d, %d)", (isMain ? "Main" : "Sub"),
          rFovCrop.p.x, rFovCrop.p.y, rFovCrop.s.w, rFovCrop.s.h);
}

void MiSnapshotCapture::updateBokehMeta(std::shared_ptr<MiRequest> req)
{
    // MTK data
    bool isQuadCFASensor;
    MUINT8 mSensorId;
    MiMetadata::MiEntry FaceRects;
    MiMetadata::MiEntry afRoiMan;
    MiMetadata::MiEntry fovCropRegionMan;
    MiMetadata::MiEntry afRoiSub;
    MiMetadata::MiEntry fovCropRegionSub;

    isQuadCFASensor = false;
    MLOGD(LogGroupCore, "is 4cell %d", isQuadCFASensor);

    InputMetadataBokeh bokehMeta;
    memset(&bokehMeta, 0, sizeof(InputMetadataBokeh));
    bokehMeta.cameraMetadata[0].cameraRole = CameraRoleTypeTele;
    bokehMeta.cameraMetadata[1].cameraRole = CameraRoleTypeWide;
    bokehMeta.cameraMetadata[0].isQuadCFASensor = isQuadCFASensor;

    if (MiMetadata::tryGetMetadata<MUINT8>(&mCameraInfo.staticInfo,
                                           XIAOMI_CAMERA_BOKEHINFO_MASTERCAMERAID, mSensorId))
        MLOGD(LogGroupCore, "master sensor ID %d", mSensorId);
    else
        MLOGE(LogGroupCore, "cannot get master ID");

    FaceRects = req->result.entryFor(MTK_STATISTICS_FACE_RECTANGLES);
    if (!FaceRects.isEmpty()) {
        const size_t faceDataCount = FaceRects.count();
        bokehMeta.cameraMetadata[0].fdMetadata.numFaces = faceDataCount;
        for (size_t index = 0; index < faceDataCount; ++index) {
            const MRect faceRect = FaceRects.itemAt(index, Type2Type<MRect>());
            MLOGD(LogGroupCore,
                  "face data, index:%zu/%zu, rectangle(leftTop-rightBottom):(%d, %d)-(%d, %d)",
                  index, faceDataCount, faceRect.p.x, faceRect.p.y, faceRect.s.w, faceRect.s.h);

            bokehMeta.cameraMetadata[0].fdMetadata.faceRect[index].left = faceRect.p.x;
            bokehMeta.cameraMetadata[0].fdMetadata.faceRect[index].top = faceRect.p.y;
            bokehMeta.cameraMetadata[0].fdMetadata.faceRect[index].width = faceRect.s.w;
            bokehMeta.cameraMetadata[0].fdMetadata.faceRect[index].height = faceRect.s.h;
        }
    } else {
        MLOGI(LogGroupCore, "MI cannot get face rects, using numFaces 0");
        bokehMeta.cameraMetadata[0].fdMetadata.numFaces = 0;
    }

    // replace MTK_MULTI_CAM_AF_ROI
    afRoiMan = req->result.entryFor(MTK_3A_FEATURE_AF_ROI);
    // replace MTK_MULTI_CAM_FOV_CROP_REGION, using mtk platform metadata tag:0xc0010000,.
    fovCropRegionMan = req->halResult.entryFor(MTK_MULTI_CAM_FOV_CROP_REGION);
    if (!afRoiMan.isEmpty())
        checkAFRoi(afRoiMan, MTRUE, &bokehMeta.cameraMetadata[0]);
    else
        MLOGW(LogGroupCore, "cannot get main af region");
    if (!fovCropRegionMan.isEmpty()) {
        checkFOVRegion(fovCropRegionMan, MTRUE, &bokehMeta.cameraMetadata[0]);
        checkFOVRegion(fovCropRegionMan, MFALSE, &bokehMeta.cameraMetadata[1]);
    } else
        MLOGW(LogGroupCore, "cannot get main fov crop region");

    // get active region for bokehMeta
    MiMetadata::MiEntry entry =
        mCameraInfo.staticInfo.entryFor(MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION);
    if (!entry.isEmpty()) {
        MRect activeArray = entry.itemAt(0, Type2Type<MRect>());
        bokehMeta.cameraMetadata[0].activeArraySize.left = activeArray.p.x;
        bokehMeta.cameraMetadata[0].activeArraySize.top = activeArray.p.y;
        bokehMeta.cameraMetadata[0].activeArraySize.width = activeArray.s.w;
        bokehMeta.cameraMetadata[0].activeArraySize.height = activeArray.s.h;
        MLOGD(LogGroupCore, "Get info active array: MPoint(%d, %d), MSize(%d, %d)", activeArray.p.x,
              activeArray.p.y, activeArray.s.w, activeArray.s.h);
    } else {
        MLOGE(LogGroupCore, "Get  MTK_SENSOR_INFO_ACTIVE_ARRAY_REGION error");
    }

    MiMetadata::updateMemory(req->result, XIAOMI_CAMERA_ALGOUP_INPUTMETABOKEH, bokehMeta);

    // For JIIGAN bokeh
    std::string bokehModuleInfo = CameraPlatformInfoXiaomi::CreateInstance()->GetBokehModuleInfo();
    if (!bokehModuleInfo.empty()) {
        int err = 0;
        MiMetadata::MiEntry entry(XIAOMI_CAMERA_BOKEHINFO_MODULEINFO);
        for (int i = 0; i < bokehModuleInfo.length() + 1; i++) {
            entry.push_back(bokehModuleInfo[i], Type2Type<MUINT8>());
        }
        err = req->result.update(entry.tag(), entry);
        if (err != 0) {
            MLOGE(LogGroupCore, "MI IMetadata::update,tag XIAOMI_CAMERA_BOKEH_MODULEINFO:%d err:%d",
                  entry.tag(), err);
        }
        MLOGD(LogGroupCore, "string moduleInfo is [%s]", bokehModuleInfo.c_str());
    } else {
        MLOGD(LogGroupCore, "string moduleInfo  is NULL");
    }

    MINT32 isp_gain = 0;
    if (MiMetadata::tryGetMetadata(&req->result, MTK_3A_FEATURE_AE_ISP_GAIN_VALUE, isp_gain)) {
        XiaoMi::InputMetadataAecInfo aecInfo;
        memset(&aecInfo, 0, sizeof(XiaoMi::InputMetadataAecInfo));
        aecInfo.linearGain = isp_gain / 1024.0f;
        MiMetadata::updateMemory(req->result, XIAOMI_CAMERA_ALGOUP_SENSOR_AEC_INFO, aecInfo);
        MLOGD(LogGroupCore, "ouput MTK_3A_FEATURE_AE_ISP_GAIN_VALUE isp_gain:%d, linearGain: %.6f",
              isp_gain, aecInfo.linearGain);
    } else {
        MLOGE(LogGroupCore, "output MTK_3A_FEATURE_AE_ISP_GAIN_VALUE error");
    }

    // MTK have release the related patch
    //
    // MUINT8 slaveId;
    // if (MiMetadata::tryGetMetadata<MUINT8>(&mCameraInfo.staticInfo,
    //                                       XIAOMI_CAMERA_BOKEHINFO_SLAVECAMERAID, slaveId)) {
    //    MRect slaveCrop(MPoint(0, 0), MSize(2400, 1800));
    //    MiMetadata::getEntry(&req->physicalCameraMetadata[slaveId].halMetadata,
    //                         MTK_P1NODE_SCALAR_CROP_REGION, slaveCrop);
    //    MLOGD(LogGroupCore, "bokeh get slave crop (%d,%d)x(%d,%d)", slaveCrop.p.x, slaveCrop.p.y,
    //          slaveCrop.s.w, slaveCrop.s.h);
    //    MiMetadata::setEntry<MRect>(&req->physicalCameraMetadata[slaveId].halMetadata,
    //                                MTK_DUALZOOM_CAP_CROP, slaveCrop);
    //} else {
    //    MLOGE(LogGroupCore,"set slave capture crop failed");
    //}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

} // namespace mizone
