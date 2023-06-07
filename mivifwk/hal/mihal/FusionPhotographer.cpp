#include "FusionPhotographer.h"

#include "MiviSettings.h"

namespace mihal {

FusionPhotographer::FusionPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest,
                                       Session *session,
                                       std::vector<std::shared_ptr<Stream>> extraStreams,
                                       const Metadata extraMeta,
                                       std::shared_ptr<StreamConfig> darkroomConfig)
    : Photographer{fwkRequest, session, extraMeta, darkroomConfig}
{
    uint32_t frame = getAndIncreaseFrameSeqToVendor(1);
    std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
    for (auto stream : extraStreams) {
        std::shared_ptr<StreamBuffer> extraBuffer = stream->requestBuffer();
        outBuffers.push_back(extraBuffer);
    }

    // FIXME: here we could be able to avoid a meta copy
    auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame, outBuffers,
                                                  *fwkRequest->getMeta());

    uint8_t mfnrEnable = 1;
    request->updateMeta(MI_MFNR_ENABLE, &mfnrEnable, 1);

    uint8_t srFusion = 1;
    request->updateMeta(MI_CAPTURE_FUSION_IS_ON, &srFusion, 1);

    auto record = std::make_unique<RequestRecord>(this, std::move(request));
    mVendorRequests.insert({frame, std::move(record)});
}

// NOTE: https://wiki.n.miui.com/pages/viewpage.action?pageId=519708358
SrFusionPhotographer::SrFusionPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest,
                                           Session *session,
                                           std::vector<std::shared_ptr<Stream>> extraStreams,
                                           const Metadata extraMeta,
                                           std::shared_ptr<StreamConfig> darkroomConfig)
    : Photographer{fwkRequest, session, extraMeta, darkroomConfig}
{
    MiviSettings::getVal("FeatureList.FeatureSR.NumOfSnapshotRequiredBySR", mTotalSequenceNum, 8);

    uint32_t frame = getAndIncreaseFrameSeqToVendor(mTotalSequenceNum);

    for (int i = 0; i < mTotalSequenceNum; i++) {
        std::vector<std::shared_ptr<StreamBuffer>> outPutBuffers;
        for (auto stream : extraStreams) {
            std::shared_ptr<StreamBuffer> buffer = stream->requestBuffer();
            outPutBuffers.push_back(buffer);
        }

        auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame + i,
                                                      outPutBuffers, *fwkRequest->getMeta());

        // populate request settings
        applyDependencySettings(request, i);

        auto record = std::make_unique<RequestRecord>(this, std::move(request));
        mVendorRequests.insert({frame + i, std::move(record)});
    }
}

void SrFusionPhotographer::applyDependencySettings(std::unique_ptr<LocalRequest> &request,
                                                   int batchIndex)
{
    request->updateMeta(MI_MULTIFRAME_INPUTNUM, &mTotalSequenceNum, 1);

    request->updateMeta(MI_BURST_TOTAL_NUM, &mTotalSequenceNum, 1);

    int32_t index = batchIndex + 1;
    request->updateMeta(MI_BURST_CURRENT_INDEX, &index, 1);

    uint8_t mfnrEnable = 0;
    request->updateMeta(MI_MFNR_ENABLE, &mfnrEnable, 1);

    uint8_t srEnable = 1;
    request->updateMeta(MI_SUPER_RESOLUTION_ENALBE, &srEnable, 1);

    uint8_t srFusion = 1;
    request->updateMeta(MI_CAPTURE_FUSION_IS_ON, &srFusion, 1);
}

} // namespace mihal
