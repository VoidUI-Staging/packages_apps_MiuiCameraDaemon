#include "HwmfPhotographer.h"

namespace mihal {

HwmfPhotographer::HwmfPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest,
                                   Session *session, std::shared_ptr<Stream> extraStream,
                                   uint32_t frames, const Metadata extraMeta,
                                   std::shared_ptr<StreamConfig> darkroomConfig)
    : Photographer{fwkRequest, session, extraMeta, darkroomConfig}
{
    mIsMiviAnchor = true;
    MLOGD(LogGroupCore, "[EarlyPic] mIsMiviAnchor %d", mIsMiviAnchor);

    uint32_t frame = getAndIncreaseFrameSeqToVendor(frames);
    MLOGI(LogGroupHAL, "frame: %d, outNum:%d, stream: %s", frame, fwkRequest->getOutBufferNum(),
          extraStream->toString(0).c_str());

    for (int i = 0; i < frames; i++) {
        std::vector<std::shared_ptr<StreamBuffer>> outPutBuffers;

        if (!skipQuickview())
            outPutBuffers.push_back(getInternalQuickviewBuffer(frame + i));

        std::shared_ptr<StreamBuffer> extraBuffer = extraStream->requestBuffer();
        outPutBuffers.push_back(extraBuffer);

        auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame + i,
                                                      outPutBuffers, *fwkRequest->getMeta());
        // FIXME: is this necessary? just because it is in the original procedure of App version
        int32_t index = i + 1;
        request->updateMeta(MI_BURST_CURRENT_INDEX, &index, 1);
        int32_t f = frames;
        request->updateMeta(MI_BURST_TOTAL_NUM, &f, 1);

        IdealRawInfo idealRawInfo = {.isIdealRaw = 0, .idealRawType = 0};
        request->updateMeta(QCOM_RAW_CB_INFO_IDEALRAW, (const uint8_t *)&idealRawInfo,
                            sizeof(idealRawInfo));

        auto record = std::make_unique<RequestRecord>(this, std::move(request));
        mVendorRequests.insert({frame + i, std::move(record)});
    }
}

bool HwmfPhotographer::skipQuickview()
{
    return true;
}

} // namespace mihal
