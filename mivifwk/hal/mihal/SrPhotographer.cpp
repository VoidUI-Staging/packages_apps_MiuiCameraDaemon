#include "SrPhotographer.h"
#include "CameraMode.h"
#include "MiviSettings.h"
namespace mihal {

SrPhotographer::SrPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                               std::shared_ptr<Stream> extraStream, uint32_t frames,
                               const Metadata extraMeta, bool remosaic,
                               std::shared_ptr<StreamConfig> darkroomConfig)
    : Photographer{fwkRequest, session, extraMeta, darkroomConfig}
{
    uint32_t frame = getAndIncreaseFrameSeqToVendor(frames);
    MLOGI(LogGroupHAL, "frame: %d, remosaic: %d", frame, remosaic);

    for (int i = 0; i < frames; i++) {
        std::vector<std::shared_ptr<StreamBuffer>> outPutBuffers;
        if (!skipQuickview()) {
            outPutBuffers.push_back(getInternalQuickviewBuffer(frame + i));
        }

        outPutBuffers.push_back(extraStream->requestBuffer());

        auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame + i,
                                                      outPutBuffers, *fwkRequest->getMeta());
        int32_t index = i + 1;
        request->updateMeta(MI_BURST_CURRENT_INDEX, &index, 1);

        if (remosaic) {
            uint8_t enableRemosaic = 1;
            request->updateMeta(MI_REMOSAIC_ENABLED, &enableRemosaic, 1);
        }

        postUpdateReqMeta(request, i, frames, extraStream->getFormat() == HAL_PIXEL_FORMAT_RAW10);

        auto record = std::make_unique<RequestRecord>(this, std::move(request));
        mVendorRequests.insert({frame + i, std::move(record)});
    }
}

void SrPhotographer::postUpdateReqMeta(std::unique_ptr<LocalRequest> &request, int batchIndex,
                                       int32_t multiFrameNum, bool isRawStream)
{
    uint8_t srEnable = true;
    request->updateMeta(MI_SUPER_RESOLUTION_ENALBE, &srEnable, 1);
    request->updateMeta(MI_MULTIFRAME_INPUTNUM, &multiFrameNum, 1);
    request->updateMeta(MI_BURST_TOTAL_NUM, &multiFrameNum, 1);
    uint8_t mfnrEnabled = 0;
    request->updateMeta(MI_MFNR_ENABLE, &mfnrEnabled, 1);
    uint8_t hdrSrEnable = 0;
    request->updateMeta(MI_HDR_SR_ENABLE, &hdrSrEnable, 1);
    uint8_t hdrEnable = 0;
    request->updateMeta(MI_HDR_ENABLE, &hdrEnable, 1);
    uint8_t zslEnable = 1;
    request->updateMeta(ANDROID_CONTROL_ENABLE_ZSL, &zslEnable, 1);

    if (isRawStream) {
        uint8_t rawZslEnable = 1;
        request->updateMeta(MI_MIVI_RAW_ZSL_ENABLE, &rawZslEnable, 1);
        mIsMiviAnchor = true;
    }
}

// NOTE: the Photographer_HDRSR logic below is just a reiteration of the one in miui camera app in
// MIVI1.0: link: packages/apps/MiuiCamera/src/com/android/camera2/MiCamera2ShotParallelBurst.java
// @applySuperResolutionParameter the code there is totally a mess, if you are a brave guy, you can
// refer to the link above for detail or refer to GaoChuan for blame
SrHdrPhotographer::SrHdrPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest,
                                     Session *session, std::shared_ptr<Stream> extraStream,
                                     HdrRelatedData &hdrData, const Metadata extraMeta,
                                     std::shared_ptr<StreamConfig> darkroomConfig)
    : Photographer{fwkRequest, session, extraMeta, darkroomConfig}
{
    forceDonotAttachPreview();

    initDataMember(hdrData.hdrStatus.expVaules, hdrData.reqNumPerEv);

    uint32_t frameNumber = mExpandedExpValues.size();
    uint32_t frame = getAndIncreaseFrameSeqToVendor(frameNumber);
    MLOGD(LogGroupHAL, "[HDR][SR] SrHdrPhotographer frame number %d", frameNumber);

    for (int i = 0; i < frameNumber; i++) {
        std::vector<std::shared_ptr<StreamBuffer>> outPutBuffers;
        outPutBuffers.push_back(extraStream->requestBuffer());

        auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame + i,
                                                      outPutBuffers, *fwkRequest->getMeta());

        MLOGD(LogGroupHAL, "[HDR][SR] update SrHdr crop region");
        ImageRect cropRegion = {.left = 0,
                                .top = 0,
                                .width = static_cast<int32_t>(extraStream->getWidth()),
                                .height = static_cast<int32_t>(extraStream->getHeight())};
        request->updateMeta(ANDROID_SCALER_CROP_REGION, (const int32_t *)&cropRegion, 4);

        postUpdateReqMeta(request, i, hdrData.reqNumPerEv,
                          extraStream->getFormat() == HAL_PIXEL_FORMAT_RAW10);

        auto record = std::make_unique<RequestRecord>(this, std::move(request));
        mVendorRequests.insert({frame + i, std::move(record)});
    }
}

void SrHdrPhotographer::initDataMember(std::vector<int32_t> &checkerExpValues,
                                       std::vector<int32_t> &reqNumPerEv)
{
    // NOTE: if do not support tag srhdrRequestNumber
    if (reqNumPerEv.empty()) {
        MLOGD(LogGroupHAL, "[HDR][SR]: no expand rule");

        MiviSettings::getVal("FeatureList.FeatureSRHDR.NumOfSnapshotRequiredBySRHDR",
                             mTotalSequenceNum, 12);
        mExpandedExpValues = checkerExpValues;
        if (mExpandedExpValues.size() < mTotalSequenceNum) {
            MiviSettings::getVal("FeatureList.FeatureSRHDR.DefaultExpValuesOfSRHDR",
                                 mExpandedExpValues,
                                 std::vector<int32_t>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -6, -12});
        }
    } else {
        // NOTE: if support tag srhdrRequestNumber, we should expand ev vals according to
        // mReqNumPerEv
        MLOGD(LogGroupHAL, "[HDR][SR]: use expand rule to generate requests");

        for (int i = 0; i < reqNumPerEv.size(); ++i) {
            std::vector<int32_t> temp(reqNumPerEv[i], checkerExpValues[i]);
            mExpandedExpValues.insert(mExpandedExpValues.end(), temp.begin(), temp.end());
        }

        mTotalSequenceNum = mExpandedExpValues.size();
    }

    MASSERT(!mExpandedExpValues.empty());

    std::ostringstream str;
    for (auto ev : mExpandedExpValues) {
        str << ev << ", ";
    }
    MLOGI(LogGroupHAL, "[HDR][SR]: expandedEv[] = %s", str.str().c_str());
    MLOGI(LogGroupHAL, "[HDR][SR]: totalSequenceNum = %d", mTotalSequenceNum);
}

void SrHdrPhotographer::postUpdateReqMeta(std::unique_ptr<LocalRequest> &request, int batchIndex,
                                          std::vector<int32_t> &reqNumPerEv, bool isRawStream)
{
    request->updateMeta(MI_BURST_TOTAL_NUM, &mTotalSequenceNum, 1);
    uint8_t hdrSrEnable = 1;
    request->updateMeta(MI_HDR_SR_ENABLE, &hdrSrEnable, 1);
    uint8_t mfnrEnable = 0;
    request->updateMeta(MI_MFNR_ENABLE, &mfnrEnable, 1);
    uint8_t hdrEnable = 0;
    request->updateMeta(MI_HDR_ENABLE, &hdrEnable, 1);

    // FIXME: is this necessary? just because it is in the original procedure of App version
    int32_t index = batchIndex + 1;
    request->updateMeta(MI_BURST_CURRENT_INDEX, &index, 1);

    int32_t evValue = mExpandedExpValues[batchIndex];
    request->updateMeta(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &evValue, 1);

    uint8_t aeLock = ANDROID_CONTROL_AE_LOCK_ON;
    request->updateMeta(ANDROID_CONTROL_AE_LOCK, &aeLock, 1);

    uint8_t bracketMode = 1;
    request->updateMeta(QCOM_AE_BRACKET_MODE, &bracketMode, 1);

    if (reqNumPerEv.empty()) {
        // NOTE: by default, ev0 has 10 frames
        reqNumPerEv.push_back(10);
    }

    // NOTE: normally, expValues[0] will be zero, which is normal exposure value, for frames whose
    // evVal=expValues[0], they need to do Sr, but for other frames such as ev- or ev+, they
    // shouldn't do Sr, otherwise the FOV of each frame will not be aligned, for details, you can
    // refer to: packages/apps/MiuiCamera/src/com/android/camera2/MiCamera2ShotParallelBurst.java or
    // doc: https://xiaomi.f.mioffice.cn/docs/dock4zC0q7smPaMMzQSKD1bFAeb#
    if (mExpandedExpValues[batchIndex] == mExpandedExpValues[0]) {
        // NOTE: reprocess is done by mialgoengine, mialgo need MI_MULTIFRAME_INPUTNUM to count how
        // many buffers will be sent to each mialgo port
        int32_t multiFrameInputNum = reqNumPerEv[0];
        request->updateMeta(MI_MULTIFRAME_INPUTNUM, &multiFrameInputNum, 1);
        uint8_t srEnable = 1;
        request->updateMeta(MI_SUPER_RESOLUTION_ENALBE, &srEnable, 1);
        // NOTE: by now, Sr+Hdr do not enable ZSL, but may be needed in the future, you can refer to
        // this doc: https://xiaomi.f.mioffice.cn/docs/dock4dasB2gnYn2cQYDSxUgcfVg
        uint8_t zslEnable = 0;
        CameraMode *mode = mSession->getMode();
        zslEnable = mode->getIsHdsrZslSupported();
        request->updateMeta(ANDROID_CONTROL_ENABLE_ZSL, &zslEnable, 1);
        request->updateMeta(MI_SRHDR_EV_ARRAY, mExpandedExpValues.data(), mTotalSequenceNum);
        if (isRawStream) {
            uint8_t rawZslEnable = 1;
            request->updateMeta(MI_MIVI_RAW_ZSL_ENABLE, &rawZslEnable, 1);
            mIsMiviAnchor = true;
        }
    } else {
        int32_t multiFrameInputNum = mExpandedExpValues.size() - reqNumPerEv[0];
        request->updateMeta(MI_MULTIFRAME_INPUTNUM, &multiFrameInputNum, 1);
        uint8_t srEnable = 0;
        request->updateMeta(MI_SUPER_RESOLUTION_ENALBE, &srEnable, 1);
        uint8_t zslEnable = 0;
        CameraMode *mode = mSession->getMode();
        zslEnable = mode->getIsHdsrZslSupported();
        request->updateMeta(ANDROID_CONTROL_ENABLE_ZSL, &zslEnable, 1);
        request->updateMeta(MI_SRHDR_EV_ARRAY, mExpandedExpValues.data(), mTotalSequenceNum);
    }
}

} // namespace mihal
