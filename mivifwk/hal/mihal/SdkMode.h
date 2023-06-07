
#ifndef __SDK_CAMERA_MODE_H__
#define __SDK_CAMERA_MODE_H__
#include "CameraMode.h"
#include "MiviSettings.h"
#include "SdkUtils.h"
#include "Session.h"

namespace mihal {

class AlgoSession;

class SdkMode : public CameraMode
{
public:
    SdkMode(AlgoSession *session, CameraDevice *vendorCamera, const StreamConfig *configFromFwk,
            std::shared_ptr<EcologyAdapter> ecologyAdapter);

    int mMasterFormat;
    std::shared_ptr<Photographer> createPhotographerOverride(
        std::shared_ptr<CaptureRequest> fwkRequest) override;
    std::shared_ptr<LocalConfig> buildConfigToDarkroom(int type = 0) override;
    virtual int32_t getIsHdsrZslSupported() override { return false; }

private:
    void buildSessionParameterOverride() override;
    bool sdkCanDoHdr(std::shared_ptr<CaptureRequest> request);
    int determinPhotographer(std::shared_ptr<CaptureRequest> request);
    void getVendorStreamInfoMap(InternalStreamInfoMap &map) override
    {
        MLOGI(LogGroupHAL, "camera mode getVendorStreamInfoMap");
        mEcologyAdapter->getVendorStreamInfoMap(map);
    }

    std::shared_ptr<EcologyAdapter> mEcologyAdapter;
};

}; // namespace mihal
#endif
