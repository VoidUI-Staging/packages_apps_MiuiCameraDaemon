/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiCamDevCallback.h"

#include <MiDebugUtils.h>
#include <mtkcam-interfaces/utils/metadata/client/mtk_metadata_tag.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "DecoupleUtil.h"

using namespace midebug;

namespace mizone {

MiCamDevCallback::~MiCamDevCallback()
{
    MLOGI(LogGroupCore, "dtor");
}

MiCamDevCallback::MiCamDevCallback(int32_t cameraId) : mCameraId(cameraId)
{
    MLOGI(LogGroupCore, "ctor");
}

MiCamDevCallback *MiCamDevCallback::create(int32_t cameraId)
{
    auto *pInstance = new MiCamDevCallback(cameraId);

    if (pInstance == nullptr) {
        delete pInstance;
        return nullptr;
    }

    return pInstance;
}

void MiCamDevCallback::setCustomSessionCallback(
    const std::shared_ptr<mcam::IMtkcamDeviceCallback> callback)
{
    if (callback != nullptr) {
        mCustomCameraDeviceCallback = callback;
    } else {
        MLOGE(LogGroupCore, "callback is null!");
    }
}

void MiCamDevCallback::processCaptureResult(const std::vector<CaptureResult> &results)
{
    if (auto cb = mCustomCameraDeviceCallback.lock()) {
        std::vector<mcam::CaptureResult> mtkResults;
        DecoupleUtil::convertCaptureResults(results, mtkResults);

        cb->processCaptureResult(mtkResults);
    } else {
        MLOGW(LogGroupCore, "mCustomCameraDeviceCallback does not exist, use_count=%ld",
              mCustomCameraDeviceCallback.use_count());
    }
}

void MiCamDevCallback::notify(const std::vector<NotifyMsg> &msgs)
{
    if (auto cb = mCustomCameraDeviceCallback.lock()) {
        std::vector<mcam::NotifyMsg> mtkMsgs;
        DecoupleUtil::convertNotifys(msgs, mtkMsgs);
        cb->notify(mtkMsgs);
    } else {
        MLOGW(LogGroupCore, "mCustomCameraDeviceCallback does not exist, use_count=%ld",
              mCustomCameraDeviceCallback.use_count());
    }
}

void MiCamDevCallback::requestStreamBuffers(const std::vector<BufferRequest> &bufferRequests,
                                            requestStreamBuffers_cb callback)
{
    if (auto cb = mCustomCameraDeviceCallback.lock()) {
        auto mtkCb = [&callback](mcam::BufferRequestStatus status,
                                 const std::vector<mcam::StreamBufferRet> &bufferRets) {
            auto miStatus = static_cast<BufferRequestStatus>(status);
            std::vector<StreamBufferRet> miBufferRets;
            // TODO: move convert to DecoupleUtil?
            miBufferRets.resize(bufferRets.size());
            for (size_t i = 0; i < bufferRets.size(); ++i) {
                auto &&bufferRet = bufferRets[i];
                auto &mtkBufferRet = miBufferRets[i];
                mtkBufferRet.streamId = bufferRet.streamId;
                mtkBufferRet.val.error = static_cast<StreamBufferRequestError>(bufferRet.val.error);
                DecoupleUtil::convertStreamBuffers(bufferRet.val.buffers, mtkBufferRet.val.buffers);
            }
            callback(miStatus, miBufferRets);
        };

        std::vector<mcam::BufferRequest> mtkBufferRequests;
        // TODO: move convert to DecoupleUtil?
        mtkBufferRequests.resize(bufferRequests.size());
        for (size_t i = 0; i < bufferRequests.size(); ++i) {
            auto &&bufReq = bufferRequests[i];
            auto &&mtkBufReq = mtkBufferRequests[i];
            mtkBufReq.streamId = bufReq.streamId;
            mtkBufReq.numBuffersRequested = bufReq.numBuffersRequested;
        }
        cb->requestStreamBuffers(mtkBufferRequests, mtkCb);
    } else {
        MLOGW(LogGroupCore, "mCustomCameraDeviceCallback does not exist, use_count=%ld",
              mCustomCameraDeviceCallback.use_count());
    }
}

void MiCamDevCallback::returnStreamBuffers(const std::vector<StreamBuffer> &buffers)
{
    if (auto cb = mCustomCameraDeviceCallback.lock()) {
        std::vector<mcam::StreamBuffer> mtkBuffers;
        DecoupleUtil::convertStreamBuffers(buffers, mtkBuffers);
        cb->returnStreamBuffers(mtkBuffers);
    } else {
        MLOGW(LogGroupCore, "mCustomCameraDeviceCallback does not exist, use_count=%ld",
              mCustomCameraDeviceCallback.use_count());
    }
}

}; // namespace mizone
