#ifndef __HDR_PHOTOGRAPHER_H__
#define __HDR_PHOTOGRAPHER_H__

#include "Photographer.h"

namespace mihal {

enum HDRType {
    // NOTE: HDR is implemented by mialgoengine. Mihal should send mutiple request to vendor
    IMPL_BY_MIALGO = 1,
    // NOTE: HDR is implemented by vendor. Only ONE request should be sent to vendor in this case
    IMPL_BY_VENDOR,
    // NOTE: HDR is implemented by both mialgoengine and vendor, only for DXO test.
    IMPL_BY_VENDOR_WITH_FUSION,
    // NOTE: HDR is implemented by mialgoengine. Mihal should send mutiple raw request to vendor
    IMPL_BY_MIALGO_RAW,
};

enum HDRsupportedCameraMode { NORMAL_MODE, SDK_MODE };

class HdrPhotographer final : public Photographer
{
public:
    HdrPhotographer() = delete;
    HdrPhotographer(std::shared_ptr<CaptureRequest> fwkRequest, Session *session,
                    std::shared_ptr<Stream> extraStream, HdrRelatedData &hdrData,
                    const Metadata extraMeta = Metadata{},
                    std::shared_ptr<StreamConfig> darkroomConfig = nullptr,
                    int cameraMode = NORMAL_MODE);
    ~HdrPhotographer() = default;

    std::string getName() const override { return "HdrPhotographer"; }

    std::string toString() const override;

protected:
    bool skipQuickview() override;

private:
    int numOfReqSent2Vendor();
    void postUpdateReqMeta(std::unique_ptr<LocalRequest> &requestPtr, int batchIndex,
                           HdrRelatedData &hdrData);

    // FIXME: do we really need to store the Expose Values?
    std::vector<int32_t> mExpValues;
    std::vector<int32_t> mRawExpValues;
    int mHDRTye;
    int mCameraMode;
    bool mShouldApplyAeLock;
    int mRoleId;
};

} // namespace mihal

#endif
