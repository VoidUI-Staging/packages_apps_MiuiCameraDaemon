#include "MiaPipelinePruner.h"

namespace mialgo2 {

void MiaCustomizationMTK::disablePortSelect(
    const std::string &pipelineName, camera_metadata_t *metadata,
    std::map<std::string, std::set<disablePort>> &disablePorts)
{
    uint8_t ainrEnable = false;
    bool mfnrEnable = false;
    void *data = NULL;
    // tag: MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_FEATURE
    VendorMetadataParser::getTagValue(metadata, 0x800B000E, &data);
    if (NULL != data) {
        int32_t feature = *static_cast<int32_t *>(data);
        if (feature == 2 || feature == 5 || feature == 6) {
            ainrEnable = true;
        } else if (feature == 1) {
            mfnrEnable = true;
        }
    }

    data = NULL;
    bool hdrEnable = false;
    // const char *hdrEnablePtr = "xiaomi.hdr.enabled";
    VendorMetadataParser::getVTagValue(metadata, "xiaomi.hdr.enabled", &data);
    // VendorMetadataParser::getTagValue(metadata, XIAOMI_HDR_FEATURE_ENABLED, &data);
    if (NULL != data) {
        hdrEnable = *static_cast<uint8_t *>(data);
    }

    data = NULL;
    bool hdrSrEnable = false;
    VendorMetadataParser::getVTagValue(metadata, "xiaomi.hdr.sr.enabled", &data);
    // VendorMetadataParser::getTagValue(metadata, XIAOMI_HDR_SR_FEATURE_ENABLED, &data);
    if (NULL != data) {
        hdrSrEnable = *static_cast<uint8_t *>(data);
    }

    data = NULL;
    bool rawSnEnable = false;
    VendorMetadataParser::getVTagValue(metadata, "xiaomi.supernight.raw.enabled", &data);
    // VendorMetadataParser::getTagValue(metadata, XIAOMI_SuperNight_FEATURE_ENABLED, &data);
    if (NULL != data) {
        rawSnEnable = *static_cast<int *>(data);
    }
    MLOGD(Mia2LogGroupPlugin, "Get ainr: %d, mfnr: %d, hdr: %d, rawSn: %d", ainrEnable, mfnrEnable,
          hdrEnable, rawSnEnable);
    if (pipelineName == "DualBokehSnapshot") {
        if (!hdrEnable) {
            disablePorts["CapbokehInstance"].insert({2, INPUT});
            disablePorts["CapbokehInstance"].insert({3, INPUT});
        }
    } else if (pipelineName == "FrontBokehSnapshot" /*|| pipelineName == "DualBokehSnapshot"*/) {
        if (!ainrEnable) {
            disablePorts["MfnrInstance"].insert({1, INPUT});
        }
    } else if (pipelineName == "ManualSnapshot" || pipelineName == "FrontSuperNightSnapshot" ||
               pipelineName == "SuperHdSnapshot") {
        if (!ainrEnable) {
            disablePorts["MfnrInstance"].insert({1, INPUT});
        }
    } else if (pipelineName == "SatSnapshot" || pipelineName == "FrontSingleSnapshot") {
        if (!ainrEnable) {
            disablePorts["MfnrInstance"].insert({1, INPUT});
        }
        if (!hdrEnable && !hdrSrEnable) {
            disablePorts["HdrInstance"].insert({2, INPUT});
        }
    } else {
        MLOGD(Mia2LogGroupPlugin, "no need to disable pipeline's port!");
    }
}

bool MiaCustomizationMTK::nodeBypassRuleSelect(
    const std::string &pipelineName, camera_metadata_t *metadata, const std::string &nodeInstance,
    std::map<std::string, std::vector<throughPath>> &nodesBypassRule,
    std::map<std::string, std::set<disablePort>> &disablePorts)
{
    bool haveMatch = false;

    return haveMatch;
}

void MiaCustomizationMTK::resetNodeMask(camera_metadata_t *metadata, const std::string &instance,
                                        PipelineInfo &pipeline)
{
}

void MiaCustomizationMTK::reMapSourcePortId(uint32_t &sourcePort, camera_metadata_t *metadata,
                                            camera_metadata_t *phyMetadata,
                                            const std::string &pipelineName,
                                            sp<MiaImage> inputImage)
{
}

void MiaCustomizationMTK::pipelineSelect(const camera_metadata_t *metadata /*no use now*/,
                                         const PipelineInfo &pipeInfo, std::string &pipelineName,
                                         std::vector<LinkItem> &links)
{
}

} // namespace mialgo2
