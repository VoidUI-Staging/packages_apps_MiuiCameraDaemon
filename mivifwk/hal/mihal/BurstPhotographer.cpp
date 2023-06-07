#include "BurstPhotographer.h"

#include "ExifUtils.h"
#include "GraBuffer.h"
#include "JpegCompressor.h"

using namespace vendor::xiaomi::hardware::bgservice::implementation;

namespace mihal {

#define PAD_TO_SIZE(size, padding) (((size) + padding - 1) / padding * padding)

BurstPhotographer::BurstPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest,
                                     Session *session, std::shared_ptr<Stream> extraStream,
                                     const Metadata extraMeta, const Metadata requestMetadata,
                                     std::shared_ptr<StreamConfig> darkroomConfig)
    : Photographer{fwkRequest, session, extraStream, extraMeta, darkroomConfig},
      mRequestMetadata{requestMetadata}
{
}

int BurstPhotographer::execute()
{
    for (int i = 0; i < mFwkRequest->getOutBufferNum(); i++) {
        std::shared_ptr<StreamBuffer> streamBuffer = mFwkRequest->getStreamBuffer(i);
        if (streamBuffer->isSnapshot()) {
            streamBuffer->setErrorBuffer();
            LocalResult lr{mFwkRequest->getCamera(), mFwkRequest->getFrameNumber(), 0,
                           std::vector<std::shared_ptr<StreamBuffer>>{streamBuffer}};
            forwardResultToFwk(&lr);
            break;
        }
    }

    return Photographer::execute();
}

std::string BurstPhotographer::getImageName() const
{
    std::string name;
    auto entry = mRequestMetadata.find(MI_SNAPSHOT_IMAGE_NAME);
    if (entry.count) {
        name = reinterpret_cast<const char *>(entry.data.u8);
    }

    return name;
}

}; // namespace mihal
