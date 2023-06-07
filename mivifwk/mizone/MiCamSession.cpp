/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#define __XIAOMI_CAMERA__

#include "MiCamSession.h"

#include <MiDebugUtils.h>
#include <dlfcn.h>

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "DecoupleUtil.h"
#include "MiCamMode.h"
#include "MiCamOfflineSession.h"
#include "MiCamTrace.h"
#include "MiVendorCamera.h"

using mcam::core::HAL_OK;
using mcam::core::HAL_UNKNOWN_ERROR;
using NSCam::IMetadata;

using mizone::MiCamDevCallback;

using namespace midebug;

#define MZ_LOGV(group, fmt, arg...) MLOGV(group, "[%d]" fmt, mCameraId, ##arg)

#define MZ_LOGD(group, fmt, arg...) MLOGD(group, "[%d]" fmt, mCameraId, ##arg)

#define MZ_LOGI(group, fmt, arg...) MLOGI(group, "[%d]" fmt, mCameraId, ##arg)

#define MZ_LOGW(group, fmt, arg...) MLOGW(group, "[%d]" fmt, mCameraId, ##arg)

#define MZ_LOGE(group, fmt, arg...) MLOGE(group, "[%d]" fmt, mCameraId, ##arg)

#define MZ_LOGF(group, fmt, arg...) MLOGF(group, "[%d]" fmt, mCameraId, ##arg)

namespace mizone {

MiCamSession::~MiCamSession()
{
    MZ_LOGI(LogGroupCore, "dtor");
    if (!mStaticInfo.isEmpty()) {
        mStaticInfo.clear();
    }

    MiDebugInterface::getInstance()->stopOfflineLogger();
}

MiCamSession::MiCamSession(const CreationInfo &info)
    : mCameraDeviceCallback(nullptr),
      mCameraId(info.intanceId),
      mSupportCoreHBM(info.supportCoreHBM),
      mSupportAndroidHBM(info.supportAndroidHBM),
      mPhysicalMetadataMap(info.physicalMetadataMap)
{
    MZ_LOGI(LogGroupCore, "ctor");
    mStaticInfo = info.staticMetadata;

    MiDebugInterface::getInstance()->startOfflineLogger();
}

auto MiCamSession::create(const CreationInfo &info) -> MiCamSession *
{
    auto *pInstance = new MiCamSession(info);

    if (pInstance == nullptr) {
        delete pInstance;
        return nullptr;
    }

    return pInstance;
}

int MiCamSession::open(const std::shared_ptr<mcam::IMtkcamDeviceCallback> &callback)
{
    MZ_LOGD(LogGroupCore, "+");
    auto *pCustomCallback = MiCamDevCallback::create(mCameraId);
    if (pCustomCallback != nullptr) {
        // create MiCamDevCallback
        auto customCallback = std::shared_ptr<mizone::MiCamDevCallback>(pCustomCallback);
        customCallback->setCustomSessionCallback(callback); // set callback in MiCamDevCallback

        mCameraDeviceCallback = customCallback;
    } else {
        MZ_LOGE(LogGroupCore, "create MiCamDevCallback failed");
        return HAL_UNKNOWN_ERROR;
    }
    MZ_LOGI(LogGroupCore, "mCameraDeviceCallback(%p) callback(%p)", mCameraDeviceCallback.get(),
            callback.get());
    MZ_LOGD(LogGroupCore, "-");
    return HAL_OK;
}

int MiCamSession::constructDefaultRequestSettings(const mcam::RequestTemplate type,
                                                  IMetadata &requestTemplate)
{
    return mSession->constructDefaultRequestSettings(type, requestTemplate);
}

// V3.6 is identical to V3.5
int MiCamSession::configureStreams(const mcam::StreamConfiguration &requestedConfiguration,
                                   mcam::HalStreamConfiguration &halConfiguration)
{
    MZ_LOGD(LogGroupCore, "+");
    int status;

    if (!mSession) {
        MZ_LOGE(LogGroupCore, "mSession is nullptr!");
        return HAL_UNKNOWN_ERROR;
    }

    MiMetadata staticInfo;
    DecoupleUtil::convertMetadata(mStaticInfo, staticInfo);

    std::map<int32_t, MiMetadata> physicalMetadataMap;
    for (auto pair : mPhysicalMetadataMap) {
        MiMetadata physicalMetadata;
        DecoupleUtil::convertMetadata(pair.second, physicalMetadata);
        physicalMetadataMap.insert(std::make_pair(pair.first, physicalMetadata));
    }

    auto vendorSession = std::shared_ptr<MiVendorCamera>(MiVendorCamera::create(mSession));

    StreamConfiguration miRequestedConfiguration;
    DecoupleUtil::convertStreamConfiguration(requestedConfiguration, miRequestedConfiguration);

    // NOTE: We should create difference type of MiCamMode according to config params
    //       during configureStreams.
    MiCamMode::CreateInfo info{
        .cameraId = mCameraId,
        .staticInfo = staticInfo,
        .deviceCallback = mCameraDeviceCallback,
        .deviceSession = vendorSession,
        .supportCoreHBM = mSupportCoreHBM,
        .supportAndroidHBM = mSupportAndroidHBM,
        .physicalMetadataMap = physicalMetadataMap,
        .config = miRequestedConfiguration,
    };
    mCamMode = MiCamMode::createMode(info);
    if (mCamMode == nullptr) {
        MZ_LOGE(LogGroupCore, "mCamMode is nullptr!");
        return HAL_UNKNOWN_ERROR;
    }

    HalStreamConfiguration miHalConfiguration;
    if (mCamMode->getModeType() == MiCamMode::AlgoMode) {
        mUsingMode = true;
        status = mCamMode->configureStreams(miRequestedConfiguration, miHalConfiguration);
    } else {
        mUsingMode = false;
        status = mCusSession->configureStreams(requestedConfiguration, halConfiguration);
    }

    DecoupleUtil::convertHalStreamConfiguration(miHalConfiguration, halConfiguration);

    MZ_LOGD(LogGroupCore, "-");
    return status;
}

int MiCamSession::processCaptureRequest(const std::vector<mcam::CaptureRequest> &requests,
                                        uint32_t &numRequestProcessed)
{
    MZ_LOGD(LogGroupCore, "+");
    int status = 0;
    MICAM_TRACE_SYNC_BEGIN_F(MialgoTraceCapture, "mizone req|%d", requests[0].frameNumber);
    numRequestProcessed = 0;

    if (mUsingMode) {
        std::vector<CaptureRequest> miRequests;
        DecoupleUtil::convertCaptureRequests(requests, miRequests);
        status = mCamMode->processCaptureRequest(miRequests, numRequestProcessed);
    } else {
        status = mCusSession->processCaptureRequest(requests, numRequestProcessed);
    }
    MICAM_TRACE_SYNC_END(MialgoTraceCapture);
    MZ_LOGD(LogGroupCore, "-");
    return status;
}

// V3.5
void MiCamSession::signalStreamFlush(const std::vector<int32_t> &streamIds,
                                     uint32_t streamConfigCounter)
{
    MZ_LOGD(LogGroupCore, "+");

    if (mSession == nullptr) {
        MZ_LOGE(LogGroupCore, "Bad MiCamSession");
    }
    if (mUsingMode) {
        mCamMode->signalStreamFlush(streamIds, streamConfigCounter);
    } else {
        mCusSession->signalStreamFlush(streamIds, streamConfigCounter);
    }

    MZ_LOGD(LogGroupCore, "+");
}

int MiCamSession::isReconfigurationRequired(const NSCam::IMetadata &oldSessionParams,
                                            const NSCam::IMetadata &newSessionParams,
                                            bool &reconfigurationNeeded) const
{
    MZ_LOGD(LogGroupCore, "+");
    auto ret = mSession->isReconfigurationRequired(oldSessionParams, newSessionParams,
                                                   reconfigurationNeeded);
    MZ_LOGD(LogGroupCore, "-");
    return ret;
}

// V3.6
int MiCamSession::switchToOffline(const std::vector<int64_t> &streamsToKeep,
                                  mcam::CameraOfflineSessionInfo &offlineSessionInfo,
                                  std::shared_ptr<mcam::IMtkcamOfflineSession> &offlineSession)
{
    MZ_LOGI(LogGroupCore, "+");
    int status = Status::OK;

    if (mUsingMode) {
        CameraOfflineSessionInfo miOfflineSessionInfo;
        status = mCamMode->switchToOffline(streamsToKeep, miOfflineSessionInfo);
        DecoupleUtil::convertCameraOfflineSessionInfo(miOfflineSessionInfo, offlineSessionInfo);
    } else {
        status = mCusSession->switchToOffline(streamsToKeep, offlineSessionInfo, offlineSession);
    }

    if (status != Status::OK) {
        offlineSessionInfo.offlineRequests.clear();
        offlineSessionInfo.offlineStreams.clear();
        offlineSession = nullptr;
        return status;
    }

    // there is no requests needed to switch to offline session,
    // just return offlineSession as nullptr with status OK.
    if (offlineSessionInfo.offlineRequests.empty()) {
        offlineSession = nullptr;
        return status;
    } else if (mUsingMode) {
        offlineSession = std::shared_ptr<IMtkcamOfflineSession>(
            MiCamOfflineSession::create(mCameraId, std::move(mCamMode)));
    }

    MZ_LOGI(LogGroupCore, "-");
    return status;
}

int MiCamSession::flush()
{
    MZ_LOGD(LogGroupCore, "+");

    int status = Status::OK;

    if (mUsingMode) {
        if (mCamMode != nullptr) {
            status = mCamMode->flush();
        } else {
            // NOTE: mCamMode == nullptr here not means an error encountered, it is normal
            //       in following cases:
            //       1. before MiCamSession::configureStreams called;
            //       2. after MiCamSession::switchToOffline,
            //          but before MiCamSession::configureStreams called
            //       so in these cases, just forward this call to Core Session (mSession)
            MZ_LOGI(LogGroupCore, "mCamMode is nullptr");
            status = mSession->flush();
        }
    } else {
        status = mCusSession->flush();
    }

    MZ_LOGD(LogGroupCore, "-");
    return status;
}

int MiCamSession::close()
{
    MZ_LOGD(LogGroupCore, "+");

    int status = Status::OK;
    if (mUsingMode) {
        if (mCamMode != nullptr) {
            status = mCamMode->close();
        } else {
            // NOTE: mCamMode == nullptr here not means an error encountered, it is normal
            //       in following cases:
            //       1. before MiCamSession::configureStreams called;
            //       2. after MiCamSession::switchToOffline,
            //          but before MiCamSession::configureStreams called
            //       so in these cases, just forward this call to Core Session (mSession)
            MZ_LOGI(LogGroupCore, "mCamMode is nullptr");
            status = mSession->close();
        }
    } else {
        status = mCusSession->close();
    }

    if (status != Status::OK) {
        MZ_LOGE(LogGroupCore, "MiCamSession close failed!");
    }

    MZ_LOGD(LogGroupCore, "-");
    return status;
}

void MiCamSession::notify(const std::vector<mcam::NotifyMsg> &msgs)
{
    MZ_LOGD(LogGroupCore, "+");
    if (!msgs.empty()) {
        if (mUsingMode) {
            std::vector<NotifyMsg> miMsgs;
            DecoupleUtil::convertNotifys(msgs, miMsgs);
            mCamMode->notify(miMsgs);
        } else {
            MZ_LOGD(LogGroupCore, "line:%d", __LINE__);
            mCusDeviceCallbacks->notify(msgs);
        }

    } else {
        MZ_LOGE(LogGroupCore, "msgs size is zero, bypass the megs");
    }
    MZ_LOGD(LogGroupCore, "-");
}

void MiCamSession::processCaptureResult(const std::vector<mcam::CaptureResult> &results)
{
    MZ_LOGD(LogGroupCore, "+");

    if (mUsingMode) {
        std::vector<CaptureResult> miResults;
        DecoupleUtil::convertCaptureResults(results, miResults);
        mCamMode->processCaptureResult(miResults);
    } else {
        mCusDeviceCallbacks->processCaptureResult(results);
    }

    MZ_LOGD(LogGroupCore, "-");
}

void MiCamSession::requestStreamBuffers(const std::vector<mcam::BufferRequest> &bufReqs,
                                        requestStreamBuffers_cb cb)
{
    if (mUsingMode) {
        auto miCallback = [&cb](BufferRequestStatus status,
                                const std::vector<StreamBufferRet> &bufferRets) {
            auto mtkStatus = static_cast<mcam::BufferRequestStatus>(status);
            std::vector<mcam::StreamBufferRet> mtkBufferRets;
            // TODO: move convert to DecoupleUtil?
            mtkBufferRets.resize(bufferRets.size());
            for (size_t i = 0; i < bufferRets.size(); ++i) {
                auto &&bufferRet = bufferRets[i];
                auto &mtkBufferRet = mtkBufferRets[i];
                mtkBufferRet.streamId = bufferRet.streamId;
                mtkBufferRet.val.error =
                    static_cast<mcam::StreamBufferRequestError>(bufferRet.val.error);
                DecoupleUtil::convertStreamBuffers(bufferRet.val.buffers, mtkBufferRet.val.buffers);
            }
            cb(mtkStatus, mtkBufferRets);
        };
        std::vector<BufferRequest> miBufReqs;
        // TODO: move convert to DecoupleUtil?
        miBufReqs.resize(bufReqs.size());
        for (size_t i = 0; i < bufReqs.size(); ++i) {
            auto &&bufReq = bufReqs[i];
            auto &miBufReq = miBufReqs[i];
            miBufReq.streamId = bufReq.streamId;
            miBufReq.numBuffersRequested = bufReq.numBuffersRequested;
        }
        mCamMode->requestStreamBuffers(miBufReqs, miCallback);
    } else {
        mCusDeviceCallbacks->requestStreamBuffers(bufReqs, cb);
    }
}

void MiCamSession::returnStreamBuffers(const std::vector<mcam::StreamBuffer> &buffers)
{
    if (mUsingMode) {
        std::vector<StreamBuffer> miBuffers;
        DecoupleUtil::convertStreamBuffers(buffers, miBuffers);
        mCamMode->returnStreamBuffers(miBuffers);
    } else {
        mCusDeviceCallbacks->returnStreamBuffers(buffers);
    }
}

void MiCamSession::setCameraDeviceSession(
    const std::shared_ptr<mcam::IMtkcamDeviceSession> &session)
{
    if (session == nullptr) {
        MZ_LOGE(LogGroupCore, "fail to setCameraDeviceSession due to session is nullptr");
    }
    mSession = session;
}

void MiCamSession::setCusCameraDeviceSession(
    const std::shared_ptr<mcam::IMtkcamDeviceSession> &session)
{
    mCusSession = session;
}

void MiCamSession::setCusCameraDeviceCallback(
    const std::shared_ptr<mcam::IMtkcamDeviceCallback> &callback)
{
    mCusDeviceCallbacks = callback;
}

#if 1
// TODO: copy from MTK, should be removed later
// WARNING: It should be removed after custom zone moved down.
int MiCamSession::getConfigurationResults(const uint32_t streamConfigCounter,
                                          mcam::ExtConfigurationResults &ConfigurationResults)
{
    return mSession->getConfigurationResults(streamConfigCounter, ConfigurationResults);
}
#endif

}; // namespace mizone
