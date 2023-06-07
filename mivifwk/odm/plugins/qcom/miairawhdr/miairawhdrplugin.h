#ifndef __MI_NODEHDR__
#define __MI_NODEHDR__

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "MiaPluginUtils.h"
#include "miairawhdr.h"
// clang-format off
#include "chioemvendortag.h"
#include "hdrinputchecker.h"
// clang-format on

namespace mialgo2 {

#define MI_EXAMPLE_OUTPUT_BUF_CNT 10
#define MAX_HDR_INPUT_NUM         5

struct LSCDimensionCap
{
    int32_t width;  ///< Width
    int32_t height; ///< Height
};

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

class MiAiRAWHdrPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.miairawhdr";
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

    MiAiRAWHdrPlugin() = default;
    ~MiAiRAWHdrPlugin();

private:
    void getAEInfo(camera_metadata_t *metadata, ARC_LLHDR_AEINFO *aeInfo, int *camMode);
    int processBuffer(struct MiImageBuffer *input, int inputNum, struct MiImageBuffer *output,
                      camera_metadata_t *metaData, std::string *exif_info,
                      MIAI_HDR_RAWINFO &rawInfo, std::vector<ImageParams> &inputBufferHandle);
    void setEv(int inputnum, std::vector<ImageParams> &inputBuffer);
    ChiRect GetCropRegion(camera_metadata_t *metaData, int inputWidth, int inputHeight);

    void isSdkAlgoUp(camera_metadata_t *metadata);

    void generateExifInfo(MIAI_HDR_RAWINFO &rawInfo, std::string *exif_info,
                          HDRMetaData *hdrMetaData, camera_metadata_t *metaData,
                          std::vector<ImageParams> &inputBufferHandle);

    void getRawPattern(camera_metadata_t *metadata, MIAI_HDR_RAWINFO &rawInfo);

    void getRawInfo(MIAI_HDR_RAWINFO &rawInfo, int &inputNum, MiImageBuffer &inputFrame,
                    ProcessRequestInfo *processRequestInfo);

    void updateMetaData(camera_metadata_t *metaData, camera_metadata_t *phyMetadat,
                        MIAI_HDR_RAWINFO &rawInfo, struct MiImageBuffer *input);

    void getInputZoomRect(camera_metadata_t *metaData, struct MiImageBuffer *input);

    void getFaceInfo(camera_metadata_t *logicalMetaData, int width, int height,
                     MIAI_HDR_RAWINFO &rawInfo);
    void getBlackLevel(UINT32 inputNum, MIAI_HDR_RAWINFO &rawInfo, camera_metadata_t *pPhyMetadata);
    MiAIRAWHDR *mRawHdr;
    bool mFrontCamera;
    bool mSdkMode;
    uint32_t mFrameworkCameraId;
    int mEV[MAX_EV_NUM];
    MiaNodeInterface mNodeInterface;
    HDRINPUTCHECKER m_pHDRInputChecker;
    SnapshotReqInfo m_hdrSnapshotInfo;
    HDRMetaData hdrMetaData;
    MRECT rectRawHDRCropRegion;

#if defined(ZEUS_CAM) || defined(CUPID_CAM) || defined(MAYFLY_CAM) || defined(DITING_CAM)
    BOOL mLiteEnable;
    UINT32 mResumeNormalAlgo;
#endif
};

PLUMA_INHERIT_PROVIDER(MiAiRAWHdrPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiAiRAWHdrPluginProvider());
    return true;
}

} // namespace mialgo2
#endif //__MI_NODEHDR__
