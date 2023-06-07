#ifndef __MI_NODEHDR__
#define __MI_NODEHDR__

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "MiaPluginUtils.h"
#include "miaihdr.h"
// clang-format off
#include "chioemvendortag.h"
#include "hdrinputchecker.h"
// clang-format on

namespace mialgo2 {

#define MI_EXAMPLE_OUTPUT_BUF_CNT 10
#define MAX_HDR_INPUT_NUM         5

typedef struct
{
    float rGain; ///< Red gains
    float gGain; ///< Green gains
    float bGain; ///< Blue gains
} AWBGainParams;

static const uint8_t AWBNumCCMRows = 3;       ///< Number of rows for Color Correction Matrix
static const uint8_t AWBNumCCMCols = 3;       ///< Number of columns for Color Correction Matrix
static const uint8_t MaxCCMs = 3;             ///< Max number of CCMs supported
static const uint32_t AWBAlgoDecisionMax = 3; ///< Number of AWB decision output
static const uint8_t AWBAlgoSAOutputConfSize = 10; ///< Number of AWB SA output confidence size
///< @brief Defines the format of the color correction matrix
typedef struct
{
    bool isCCMOverrideEnabled;               ///< Flag indicates if CCM override is enabled.
    float CCM[AWBNumCCMRows][AWBNumCCMCols]; ///< The color correction matrix
    float CCMOffset[AWBNumCCMRows];          ///< The offsets for color correction matrix
} AWBCCMParams;

/// @brief Defines the format of the decision data for AWB algorithm.
typedef struct
{
    float rg;
    float bg;
} AWBDecisionPoint;

typedef struct
{
    AWBDecisionPoint point[AWBAlgoDecisionMax]; ///< AWB decision point in R/G B/G coordinate
    float correlatedColorTemperature[AWBAlgoDecisionMax]; ///< Correlated Color Temperature: degrees
                                                          ///< Kelvin (K)
    uint32_t decisionCount;                               ///< Number of AWB decision
} AWBAlgoDecisionInformation;

typedef struct
{
    float SAConf;         ///< Scene Analyzer module output confidence
    char *pSADescription; ///< Description of Scene Analyzer
} AWBSAConfInfo;

typedef struct
{
    // Internal Member Variables
    AWBGainParams AWBGains;                              ///< R/G/B gains
    uint32_t colorTemperature;                           ///< Color temperature
    AWBCCMParams AWBCCM[MaxCCMs];                        ///< Color Correction Matrix Value
    uint32_t numValidCCMs;                               ///< Number of Valid CCMs
    AWBAlgoDecisionInformation AWBDecision;              ///< AWB decesion information
    AWBSAConfInfo SAOutputConf[AWBAlgoSAOutputConfSize]; ///< SA output confidence
} AWBFrameControl;

class MiAiHdrPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.hdr";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface) override;

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo) override;

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo) override;

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo) override;

    virtual void destroy() override;

    virtual bool isEnabled(MiaParams settings);

    // AEC static definitions
    static const uint8_t ExposureIndexShort = 0; ///< Exposure setting index for short exposure
    static const uint8_t ExposureIndexLong = 1;  ///< Exposure setting index for long exposure
    static const uint8_t ExposureIndexSafe =
        2; ///< Exposure setting index for safe exposure (between short and long)

    MiAiHdrPlugin() = default;
    ~MiAiHdrPlugin();

private:
    void getAEInfo(camera_metadata_t *metadata, ARC_LLHDR_AEINFO *aeInfo, int *camMode);
    int processBuffer(struct mialgo2::MiImageBuffer *input, int inputNum,
                      struct mialgo2::MiImageBuffer *output, camera_metadata_t *metaData,
                      camera_metadata_t *phyMetadata, std::string *exif_info,
                      HDRMetaData &hdrMetaData);
    void setEv(int inputnum, std::vector<ImageParams> &inputBuffer);
    ChiRect GetCropRegion(camera_metadata_t *metaData, int inputWidth, int inputHeight);

    bool isSdkAlgoUp(camera_metadata_t *metadata);

    void generateExifInfo(int inputNum, ARC_LLHDR_AEINFO &aeInfo, std::string *exif_info,
                          camera_metadata_t *metaData, HDRMetaData *hdrMetaData);

    void getFaceInfo(camera_metadata_t *logicalMetaData, camera_metadata_t *physicalMetaData,
                     int width, int height, HDRMetaData &hdrMetaData);

    void MapFaceROIToImageBuffer(std::vector<ChiRect> &rFaceROIIn,
                                 std::vector<ChiRect> &rFaceROIOut, ChiRect cropInfo,
                                 ChiRect bufferSize);
    void DebugFaceRoi(struct MiImageBuffer *outputImageFrame, HDRMetaData &hdrMetaData);
    MiAIHDR *mHdr;
    bool mFrontCamera;
    uint32_t mFrameworkCameraId;
    int mEV[MAX_EV_NUM];
    MiaNodeInterface mNodeInterface;
    HDRINPUTCHECKER m_pHDRInputChecker;
    SnapshotReqInfo m_hdrSnapshotInfo;
};

PLUMA_INHERIT_PROVIDER(MiAiHdrPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiAiHdrPluginProvider());
    return true;
}

} // namespace mialgo2
#endif //__MI_NODEHDR__
