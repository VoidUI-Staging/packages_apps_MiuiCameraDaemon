#include "ProfessionalPhotographer.h"

#include <ui/GraphicBufferMapper.h>

#include "Camera3Plus.h"
#include "GraBuffer.h"
#include "Stream.h"
#include "VendorCamera.h"

namespace mihal {

ProfessionalPhotographer::ProfessionalPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest,
                                                   Session *session,
                                                   std::vector<std::shared_ptr<Stream>> outStreams,
                                                   std::shared_ptr<StreamConfig> darkroomConfig)
    : Photographer{fwkRequest, session, darkroomConfig}
{
    MLOGI(LogGroupHAL, "frame: %d, outputbuffer: %d, streamSize %zu", mFwkRequest->getFrameNumber(),
          mFwkRequest->getOutBufferNum(), outStreams.size());

    uint32_t frame = getAndIncreaseFrameSeqToVendor(1);

    std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
    outBuffers.push_back(getInternalQuickviewBuffer(frame));
    for (int i = 0; i < mFwkRequest->getOutBufferNum(); i++) {
        auto buffer = mFwkRequest->getStreamBuffer(i);
        if (buffer->getStream()->isConfiguredToHal()) {
            // vendor may skip preview for manualMode, so no preview
            outBuffers.push_back(buffer);
        }
    }
    for (auto &stream : outStreams) {
        std::shared_ptr<StreamBuffer> extraBuffer = stream->requestBuffer();
        outBuffers.push_back(extraBuffer);
    }

    // both preview and snapshot stream
    auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame, outBuffers,
                                                  *fwkRequest->getMeta());
    auto record =
        std::make_unique<RequestRecord>(dynamic_cast<Photographer *>(this), std::move(request));
    mVendorRequests.insert({frame, std::move(record)});

    MLOGI(LogGroupCore, "VendorRequest : %p, frameNum %d", mVendorRequests[frame]->request.get(),
          frame);

    // populate request settings
    // applyDependencySettings(mVendorRequests[frame]->request);
}

void ProfessionalPhotographer::applyDependencySettings(
    std::unique_ptr<CaptureRequest> &captureRequest)
{
    status_t ret;
    LocalRequest *request = reinterpret_cast<LocalRequest *>(captureRequest.get());

    // disable zsl tag
    uint32_t tagId = ANDROID_CONTROL_ENABLE_ZSL;
    const uint8_t zslValue = ANDROID_CONTROL_ENABLE_ZSL_FALSE;
    ret = request->updateMeta(tagId, (const uint8_t *)&zslValue, 1);
    if (ret != 0) {
        MLOGE(LogGroupCore, "update tag failed! tagId: %u, reqIndex: %d, ret: %d", tagId,
              captureRequest->getFrameNumber(), ret);
    }
}

} // namespace mihal
