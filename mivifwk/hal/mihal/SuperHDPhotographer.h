#ifndef __SUPERHD_PHOTOGRAPHER_H__
#define __SUPERHD_PHOTOGRAPHER_H__

#include <bitset>

#include "CameraMode.h"
#include "Photographer.h"
#include "Session.h"

namespace mihal {

class SuperHDPhotographer final : public Photographer
{
public:
    SuperHDPhotographer() = default;
    SuperHDPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                        std::vector<std::shared_ptr<Stream>> outStreams, SuperHdData *superHdData,
                        Metadata extraMetadata, std::shared_ptr<StreamConfig> darkroomConfig);
    ~SuperHDPhotographer() {}

    void applyDependencySettings(std::unique_ptr<CaptureRequest> &captureRequest);
    std::string getName() const override { return "SuperHDPhotographer"; }

protected:
    void parseSuperHdData(SuperHdData *superHdData);

private:
    bool canDoMfnr(LocalRequest *request);

    uint32_t mAnchorFrame;
    uint32_t mNumOfVendorRequest;
    SuperHdData mSuperHdData;
};

} // namespace mihal

#endif
