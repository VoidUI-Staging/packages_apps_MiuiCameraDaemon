#ifndef __PROFESSIONAL_PHOTOGRAPHER_H__
#define __PROFESSIONAL_PHOTOGRAPHER_H__

#include <bitset>

#include "CameraMode.h"
#include "Photographer.h"
#include "Session.h"

namespace mihal {

class ProfessionalPhotographer final : public Photographer
{
public:
    ProfessionalPhotographer() = default;
    ProfessionalPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                             std::vector<std::shared_ptr<Stream>> outStreams,
                             std::shared_ptr<StreamConfig> darkroomConfig);
    ~ProfessionalPhotographer() {}

    void applyDependencySettings(std::unique_ptr<CaptureRequest> &captureRequest);

    std::string getName() const override { return "ProfessionalPhotographer"; }
};

} // namespace mihal

#endif
