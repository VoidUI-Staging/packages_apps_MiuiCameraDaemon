#ifndef __FUSION_PHOTOGRAPHER_H__
#define __FUSION_PHOTOGRAPHER_H__

#include "Photographer.h"

namespace mihal {

class FusionPhotographer final : public Photographer
{
public:
    FusionPhotographer() = delete;
    FusionPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                       std::vector<std::shared_ptr<Stream>> extraStreams, const Metadata extraMeta,
                       std::shared_ptr<StreamConfig> darkroomConfig = nullptr);
    ~FusionPhotographer() = default;
    std::string getName() const override { return "FusionPhotographer"; }
};

class SrFusionPhotographer final : public Photographer
{
public:
    SrFusionPhotographer() = delete;
    SrFusionPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                         std::vector<std::shared_ptr<Stream>> extraStreams,
                         const Metadata extraMeta,
                         std::shared_ptr<StreamConfig> darkroomConfig = nullptr);
    ~SrFusionPhotographer() = default;
    std::string getName() const override { return "SrFusionPhotographer"; }

private:
    void applyDependencySettings(std::unique_ptr<LocalRequest> &request, int batchIndex);

    // NOTE: sequence num is how many requests actually sent to Qcom
    int32_t mTotalSequenceNum;
};

} // namespace mihal

#endif