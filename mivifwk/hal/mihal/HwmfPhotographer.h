#ifndef __HWMF_PHOTOGRAPHER_H__
#define __HWMF_PHOTOGRAPHER_H__

#include "Photographer.h"

namespace mihal {

class HwmfPhotographer final : public Photographer
{
public:
    HwmfPhotographer() = delete;
    HwmfPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                     std::shared_ptr<Stream> extraStream, uint32_t frames,
                     const Metadata extraMeta = Metadata{},
                     std::shared_ptr<StreamConfig> darkroomConfig = nullptr);
    ~HwmfPhotographer() = default;

    std::string getName() const override { return "HwmfPhotographer"; }

protected:
    bool skipQuickview() override;
};

} // namespace mihal

#endif
