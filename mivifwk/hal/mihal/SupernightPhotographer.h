#ifndef __SUPERNIGHT_PHOTOGRAPHER_H__
#define __SUPERNIGHT_PHOTOGRAPHER_H__

#include <bitset>

#include "CameraMode.h"
#include "Photographer.h"
#include "Session.h"

namespace mihal {

class SupernightPhotographer final : public Photographer
{
public:
    SupernightPhotographer() = default;
    SupernightPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                           std::shared_ptr<Stream> outStream,
                           std::shared_ptr<StreamConfig> darkroomConfig,
                           SuperNightData *superNightData, Metadata extraMetadata);
    ~SupernightPhotographer() {}

    std::string getName() const override { return "SupernightPhotographer"; }

private:
    void applyDependencySettings(CaptureRequest *captureRequest, int32_t ev);
    void initSuperNightInfo();
    bool isSupernightEnable() override { return true; }

    int32_t mEv[METADATA_VALUE_NUMBER_MAX];
    uint32_t mNumOfVendorRequest;
    SuperNightData mSuperNightData;
};

class RawSupernightPhotographer final : public Photographer
{
public:
    RawSupernightPhotographer() = default;
    RawSupernightPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                              std::shared_ptr<Stream> outStream,
                              std::shared_ptr<StreamConfig> darkroomConfig,
                              SuperNightData *superNightData, Metadata extraMetadata);
    ~RawSupernightPhotographer() {}

    std::string getName() const override { return "RawSupernightPhotographer"; }

private:
    void applyDependencySettings(CaptureRequest *captureRequest, int32_t ev);
    void initSuperNightInfo();
    void updateSupernightMode();
    bool isSupernightEnable() override { return true; }

    int32_t mEv[METADATA_VALUE_NUMBER_MAX];
    uint32_t mNumOfVendorRequest;
    SuperNightData mSuperNightData;
};

} // namespace mihal

#endif
