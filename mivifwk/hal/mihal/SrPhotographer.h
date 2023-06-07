#ifndef __SR_PHOTOGRAPHER_H__
#define __SR_PHOTOGRAPHER_H__

#include <set>

#include "Photographer.h"

namespace mihal {

class SrPhotographer final : public Photographer
{
public:
    SrPhotographer() = delete;
    SrPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                   std::shared_ptr<Stream> extraStream, uint32_t frames, const Metadata extraMeta,
                   bool remosaic = false, std::shared_ptr<StreamConfig> darkroomConfig = nullptr);
    ~SrPhotographer() = default;

    std::string getName() const override { return "SrPhotographer"; }

private:
    void postUpdateReqMeta(std::unique_ptr<LocalRequest> &request, int batchIndex,
                           int32_t multiFrameNum, bool isRawStream);
};

class SrHdrPhotographer final : public Photographer
{
public:
    SrHdrPhotographer() = delete;
    SrHdrPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                      std::shared_ptr<Stream> extraStream, HdrRelatedData &hdrData,
                      const Metadata extraMeta,
                      std::shared_ptr<StreamConfig> darkroomConfig = nullptr);
    ~SrHdrPhotographer() = default;
    std::string getName() const override { return "SrHdrPhotographer"; }

private:
    void decideMialgoPortImpl() override
    {
        uint32_t baseFrameNum = mVendorRequests.begin()->first;

        for (auto &pair : mVendorRequests) {
            auto index = pair.first - baseFrameNum;
            CaptureRequest *request = pair.second->request.get();

            for (int i = 0; i < request->getOutBufferNum(); ++i) {
                auto buffer = request->getStreamBuffer(i);

                if (buffer->getStream()->isInternal() && !buffer->isQuickview()) {
                    if (mExpandedExpValues[index] == mExpandedExpValues[0]) {
                        // NOTE: for ev0 buffers, they need to be sent to port0
                        buffer->setMialgoPort(0);
                    } else {
                        // NOTE: for ev+ ev- buffers, they need to be sent to port1
                        buffer->setMialgoPort(1);
                    }
                }
            }
        }
    }

    void initDataMember(std::vector<int32_t> &checkerExpValues, std::vector<int32_t> &reqNumPerEv);
    void postUpdateReqMeta(std::unique_ptr<LocalRequest> &request, int batchIndex,
                           std::vector<int32_t> &reqNumPerEv, bool isRawStream);

    std::vector<int32_t> mExpandedExpValues;
    // NOTE: sequence num is how many requests actually sent to Qcom
    int32_t mTotalSequenceNum;
    Metadata mExtraMeta;
};

} // namespace mihal

#endif
