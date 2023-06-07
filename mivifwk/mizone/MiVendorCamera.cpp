/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiVendorCamera.h"

#include <mtkcam-halif/utils/metadata_tag/1.x/mtk_metadata_tag.h>

#include <utility>

#include "DecoupleUtil.h"
#include "MiDebugUtils.h"

using namespace midebug;

namespace mizone {

MiVendorCamera::MiVendorCamera(std::shared_ptr<mcam::IMtkcamDeviceSession> session)
    : mSession(std::move(session))
{
    MLOGD(LogGroupCore, "ctor +");
    IProviderCreationParams param = {
        .providerName = "PostProcEngine",
    };
    mPostProcProvider = getPostProcProviderInstance(&param);
    std::shared_ptr<mcam::IMtkcamDevice> dev;
    mPostProcProvider->getDeviceInterface(MTK_POSTPROCDEV_DEVICE_TYPE_CAPTURE, dev);
    auto status = dev->open(nullptr, mPostProcSession);
    if (status != Status::OK || mPostProcSession == nullptr) {
        MLOGE(LogGroupCore, "open PostProcSession fail: mPostProcSession = %p, status = %d",
              mPostProcSession.get(), status);
    }
    MLOGD(LogGroupCore, "ctor -");
}

MiVendorCamera::~MiVendorCamera()
{
    MLOGD(LogGroupCore, "dtor");
}

std::unique_ptr<MiVendorCamera> MiVendorCamera::create(
    const std::shared_ptr<mcam::IMtkcamDeviceSession> &session)
{
    return std::unique_ptr<MiVendorCamera>(new MiVendorCamera(session));
}

int MiVendorCamera::constructDefaultRequestSettings(const RequestTemplate type,
                                                    MiMetadata &requestTemplate)
{
    MLOGD(LogGroupCore, "+");
    auto mtkType = static_cast<mcam::RequestTemplate>(type);
    IMetadata mtkRequestTemplate;
    auto status = mSession->constructDefaultRequestSettings(mtkType, mtkRequestTemplate);

    DecoupleUtil::convertMetadata(mtkRequestTemplate, requestTemplate);
    MLOGD(LogGroupCore, "-");
    return status;
}

int MiVendorCamera::configureStreams(const StreamConfiguration &requestedConfiguration,
                                     HalStreamConfiguration &halConfiguration)
{
    MLOGD(LogGroupCore, "+");
    mcam::StreamConfiguration mtkRequestedConfiguration;
    mcam::HalStreamConfiguration mtkHalConfiguration;

    DecoupleUtil::convertStreamConfiguration(requestedConfiguration, mtkRequestedConfiguration);
    auto status = mSession->configureStreams(mtkRequestedConfiguration, mtkHalConfiguration);
    DecoupleUtil::convertHalStreamConfiguration(mtkHalConfiguration, halConfiguration);

    MLOGD(LogGroupCore, "-");
    return status;
}

int MiVendorCamera::processCaptureRequest(const std::vector<CaptureRequest> &requests,
                                          uint32_t &numRequestProcessed)
{
    MLOGD(LogGroupCore, "+");
    std::vector<mcam::CaptureRequest> mtkRequests;

    DecoupleUtil::convertCaptureRequests(requests, mtkRequests);
    auto status = mSession->processCaptureRequest(mtkRequests, numRequestProcessed);
    MLOGD(LogGroupCore, "-");
    return status;
}

int MiVendorCamera::queryFeatureSetting(const MiMetadata &info, uint32_t &frameCount,
                                        std::vector<MiMetadata> &settings)
{
    int ret = 0;
    IMetadata mtkInfo;
    DecoupleUtil::convertMetadata(info, mtkInfo);
    std::vector<IMetadata> mtkSettings;
    ret = mPostProcSession->queryFeatureSetting(mtkInfo, frameCount, mtkSettings);
    settings.clear();
    for (auto mtkSet : mtkSettings) {
        MiMetadata eachSetting;
        DecoupleUtil::convertMetadata(mtkSet, eachSetting);
        settings.push_back(std::move(eachSetting));
    }
    return ret;
}

int MiVendorCamera::flush()
{
    return mSession->flush();
}

int MiVendorCamera::close()
{
    return mSession->close();
}

void MiVendorCamera::signalStreamFlush(const std::vector<int32_t> &streamIds,
                                       uint32_t streamConfigCounter)
{
    mSession->signalStreamFlush(streamIds, streamConfigCounter);
}

int MiVendorCamera::isReconfigurationRequired(const MiMetadata &oldSessionParams,
                                              const MiMetadata &newSessionParams,
                                              bool &reconfigurationNeeded) const
{
    return 0;
}

} // namespace mizone