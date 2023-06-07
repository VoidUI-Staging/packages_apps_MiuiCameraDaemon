#include "SuperHDPhotographer.h"

#include <ui/GraphicBufferMapper.h>

#include "Camera3Plus.h"
#include "GraBuffer.h"
#include "MiviSettings.h"
#include "Stream.h"
#include "VendorCamera.h"

namespace mihal {

SuperHDPhotographer::SuperHDPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest,
                                         Session *session,
                                         std::vector<std::shared_ptr<Stream>> outStreams,
                                         SuperHdData *superHdData, Metadata extraMetadata,
                                         std::shared_ptr<StreamConfig> darkroomConfig)
    : Photographer{fwkRequest, session, extraMetadata, darkroomConfig}, mNumOfVendorRequest{1}
{
    MLOGI(LogGroupHAL, "fwkFrame:%u, outputbuffer:%d, streamSize %zu",
          mFwkRequest->getFrameNumber(), mFwkRequest->getOutBufferNum(), outStreams.size());

    parseSuperHdData(superHdData);
    MLOGI(LogGroupHAL, "[GreenPic]: isRemosaic: %d", mSuperHdData.isRemosaic);

    // may need support remosaic + SR, so do multiframe process
    uint32_t frame = getAndIncreaseFrameSeqToVendor(mNumOfVendorRequest);

    for (int reqIndex = 0; reqIndex < mNumOfVendorRequest; reqIndex++, frame++) {
        std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
        for (int i = 0; i < mFwkRequest->getOutBufferNum(); i++) {
            auto buffer = mFwkRequest->getStreamBuffer(i);
            if (buffer->getStream()->isConfiguredToHal() && !mSuperHdData.isRemosaic)
                outBuffers.push_back(buffer);
        }
        for (auto &stream : outStreams) {
            std::shared_ptr<StreamBuffer> extraBuffer = stream->requestBuffer();
            outBuffers.push_back(extraBuffer);
        }

        if (reqIndex == 0)
            mAnchorFrame = frame;
        auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame, outBuffers,
                                                      *fwkRequest->getMeta());
        auto record =
            std::make_unique<RequestRecord>(dynamic_cast<Photographer *>(this), std::move(request));
        mVendorRequests.insert({frame, std::move(record)});

        MLOGI(LogGroupCore, "VendorRequest vndFrame:%u", frame);

        // populate request settings
        applyDependencySettings(mVendorRequests[frame]->request);
    }
}

void SuperHDPhotographer::parseSuperHdData(SuperHdData *superHdData)
{
    mSuperHdData = *superHdData;
}

bool SuperHDPhotographer::canDoMfnr(LocalRequest *request)
{
    bool mfnrEnabled = false;
    auto expTimeEntry = request->getMeta()->find(ANDROID_SENSOR_EXPOSURE_TIME);
    if (expTimeEntry.count) {
        uint64_t exposureTime = expTimeEntry.data.i64[0];
        // NOTE: the unit is nano second
        MLOGI(LogGroupHAL, "exposure time is %ld", exposureTime);

        // NOTE: if we have a short exposure time, we can use mfnr to improve picture quality. the
        // unit of exposure time is nano second
        uint64_t mfnrMaxExpTime;
        MiviSettings::getVal("FeatureList.FeatureHD.MfnrMaxExpTime", mfnrMaxExpTime, 250000000);
        MLOGI(LogGroupHAL, "[MiviSettings] max exposure time for mfnr is %ld(exclude)",
              mfnrMaxExpTime);
        if (exposureTime < mfnrMaxExpTime) {
            mfnrEnabled = true;
        }
    }

    MLOGI(LogGroupHAL, "mfnrEnabled is %d", mfnrEnabled);
    return mfnrEnabled;
}

void SuperHDPhotographer::applyDependencySettings(std::unique_ptr<CaptureRequest> &captureRequest)
{
    status_t ret = 0;
    LocalRequest *request = reinterpret_cast<LocalRequest *>(captureRequest.get());
    bool mfnrEnabled = mSuperHdData.isRemosaic ? false : canDoMfnr(request);
    ret = request->updateMeta(MI_REMOSAIC_ENABLED, (const uint8_t *)&mSuperHdData.isRemosaic, 1);
    if (ret != 0) {
        MLOGE(LogGroupCore, "update tag failed! tagName: %s, reqIndex: %d, ret: %d",
              MI_REMOSAIC_ENABLED, captureRequest->getFrameNumber(), ret);
    }
    ret = request->updateMeta(MI_MFNR_ENABLE, (const uint8_t *)&mfnrEnabled, 1);
    if (ret != 0) {
        MLOGE(LogGroupCore, "update tag failed! tagName: %s, reqIndex: %d, ret: %d", MI_MFNR_ENABLE,
              captureRequest->getFrameNumber(), ret);
    }
}

} // namespace mihal
