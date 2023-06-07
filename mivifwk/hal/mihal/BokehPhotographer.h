#ifndef __BOKEH_PHOTOGRAPHER__
#define __BOKEH_PHOTOGRAPHER__

#include "Photographer.h"

namespace mihal {

class BokehPhotographer final : public Photographer
{
public:
    BokehPhotographer() = delete;
    BokehPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                      std::vector<std::shared_ptr<Stream>> extraStreams,
                      std::shared_ptr<StreamConfig> darkroomConfig,
                      const Metadata extraMeta = Metadata{});
    std::string getName() const override { return "BokehPhotographer"; }
};

class MDBokehPhotographer final : public Photographer
{
public:
    MDBokehPhotographer() = delete;
    MDBokehPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                        std::vector<std::shared_ptr<Stream>> extraStreams,
                        std::vector<int32_t> &expValues,
                        std::shared_ptr<StreamConfig> darkroomConfig,
                        const Metadata extraMeta = Metadata{});
    std::string getName() const override { return "MDBokehPhotographer"; }
};

class HdrBokehPhotographer final : public Photographer
{
public:
    HdrBokehPhotographer() = delete;
    HdrBokehPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                         std::vector<std::shared_ptr<Stream>> extraStreams,
                         std::vector<int32_t> &expValues,
                         std::shared_ptr<StreamConfig> darkroomConfig = nullptr);
    std::string getName() const override { return "HdrBokehPhotographer"; }
};

class SEBokehPhotographer final : public Photographer
{
public:
    SEBokehPhotographer() = delete;
    SEBokehPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                        std::vector<std::shared_ptr<Stream>> outStreams,
                        std::shared_ptr<StreamConfig> darkroomConfig,
                        SuperNightData *superNightData);
    std::string getName() const override { return "SEBokehPhotographer"; }

private:
    void applyDependencySettings(CaptureRequest *captureRequest, int32_t ev);
    bool isSupernightEnable() override { return true; }

    void initSuperNightInfo();
    void updateSupernightMode();

    int32_t mEv[METADATA_VALUE_NUMBER_MAX];
    uint32_t mNumOfVendorRequest;
    SuperNightData mSuperNightData;
};

} // namespace mihal

#endif
