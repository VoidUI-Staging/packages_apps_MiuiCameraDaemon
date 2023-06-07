#ifndef __SDKUTILS_H__
#define __SDKUTILS_H__
#include <map>
#include <mutex>
#include <set>
#include <string>

#include "Camera3Plus.h"
#include "CameraManager.h"
#include "CameraMode.h"

const bool RESULT_SECCESS = 1;
const bool RESULT_FAILD = 0;

enum class MISdkTag {
    HdrMode = 0,
    SkinSmoothRatio,
    SlimFaceRatio,
    BokehEnable,
    MfnrEnable,
    NightModeEnable,
    DepurpleEnable,
    SdkEnabled,
    CameraXEnabled,
    VideoBokehFrontEnabled,
    VideoBokehFrontParam,
    VideoBokehRearEnabled,
    VideoBokehRearParam,
    DeviceOrientation,
    BokehFNumberApplied,
    VideoHdr10Mode,
    VideoControl,
};

enum class VendorTag {
    MfntFrameNums = 0,
    HdrEnabled,
    SkinSmoothRatio,
    SlimFaceRatio,
    BokehEnabled,
    MfnrEnabled,
    SuperNightEnabled,
    DepurpleEnabled,
    DistortionLevel,
    UltraWideDistortionLevel,
    UltraWideDistortionEnable,
    HdrChecker,
    HdrDetected,
    HdrCheckerEnabled,
    HdrCheckerStatus,
    HdrMode,
    SuperNightChecker,
    SupportedAlgoEngineHdr,
    AsdEnable,
    VideoHdrEnable,
    VideoBokehFrontEnabled,
    VideoBokehFrontParam,
    VideoBokehRearEnabled,
    VideoBokehRearParam,
    DeviceOrientation,
    BokehFNumberApplied,
    VideoControl,
    BokehRole,
    ThirdPartyYUVSnapshot,
    VideoHdr10Mode,
    HDRVideoModeSession,
};

enum class SessionOperation {
    Default = 0x0,        // 0x0 platform default function
    Normal = 0xFF0A,      // mode 1 (HDR Beauty FrontBokeh MFNR SR)
    Hq = 0xFF0B,          // mode 2 (HQ picture quality)
    SuperNight = 0xFF0C,  // mode 3 (supernight)
    AntiShake = 0xFF0D,   // mode 4 (EIS)
    SmartEngine = 0xFF0E, // mode 5 (Smart Engine)
    Fast = 0xFF0F,        // mode 5 (fast mode)
    VideoHDR = 0xFF10,
    VideoSuperNight = 0xFF11,
    Bokeh = 0xFF12,
    VideoNormal = 0xFF13,
    VideoHdr10 = 0xFF14,
    AntiShakeV3 = 0xFF16, // mode12  (EISV3)
};

enum class ThirdPartyOperation {
    Video = 0x8004,
    SuperNight = 0x800A,
    PreviewEIS = 0x8019,

    Bokeh = 0x9000,
    Normal = 0x9002,

    VideoEIS = 0x8019,
    SnapshotEIS = 0x8019,
    VideoEISV2Multi = 0x9206,
    VideoEISV3Multi = 0x8004,
    VideoSuperNight = 0x9203,

    ThreeStreamsEIS = 0x8019,
    VideoNormal = 0x8019,
    VideoHDR = 0x9204,
    VideoHdr10 = 0x801d,

    AutoExtension = 0x9002,
};

enum class ThirdPartyUsecaseId {
    Default = 0,
    JpegSnapshot,
    YuvSnapshot,
    Video,
};

enum class MISdkTagType {
    Byte,
    Int,
    Float,
    String,
};

enum class MISdkTagScenes {
    Preview,
    Snapshot,
    Both,
    Other,
};

enum BitMaskFormat {
    BitMaskPreviewFormat = 1,
    BitMaskYCbCrFormat = 1 << 1,
    BitMaskBlobFormat = 1 << 2,
    BitMaskPreviewHWFormat = 1 << 3,
    BitMaskYCbCr10bitFormat = 1 << 4,
};

struct MISdkTagInfo
{
    const char *tagName;
    MISdkTagType tagType;
    uint32_t length;
    MISdkTagScenes scenes;
};

typedef union {
    struct
    {
        unsigned int dc : 1;
        unsigned int ldc : 1;
    } algoabled;
    unsigned int value;
} MiSDKPhyAlgoSupport;

namespace mihal {

struct CloudControlInfo
{
    std::string sdkName;
    std::string sdkVersion;
    FeatureMask_t enableFeatureMask;
};

class CloudControlerBase
{
public:
    std::string getCloudControlInfo();
    virtual ~CloudControlerBase() {}
    bool parseCloudControlInfo(const char *appName, uint32_t roleId, const char *modeName,
                               bool isCameraX);
    const CloudControlInfo *getCloudInfo() { return &mCloudInfo; }

protected:
    virtual void noMatch() = 0;
    virtual void onlyMatchApp() = 0;
    virtual void matchFunction(FeatureMask_t mask) = 0;

    static std::map<std::string, FeatureMask_t> mName2FeatureMask;
    CloudControlInfo mCloudInfo;
};

class SdkCloudControler : public CloudControlerBase
{
public:
    SdkCloudControler() {}
    virtual ~SdkCloudControler() {}

private:
    virtual void noMatch() override { mCloudInfo.enableFeatureMask = 0; }

    virtual void onlyMatchApp() override { mCloudInfo.enableFeatureMask = ~0; }

    virtual void matchFunction(FeatureMask_t mask) override
    {
        mCloudInfo.enableFeatureMask &= ~mask;
    }
};

class ExtCloudControler : public CloudControlerBase
{
public:
    ExtCloudControler() {}
    virtual ~ExtCloudControler() {}

private:
    virtual void noMatch() override { mCloudInfo.enableFeatureMask = ~0; }

    virtual void onlyMatchApp() override { mCloudInfo.enableFeatureMask = 0; }

    virtual void matchFunction(FeatureMask_t mask) override
    {
        mCloudInfo.enableFeatureMask |= mask;
    }
};

class EcologyAdapter
{
public:
    EcologyAdapter(int32_t roleId, const StreamConfig *streamConfig);
    bool isNeedReprocess() { return mNeedReprocess; }
    bool isVideo() { return mUsecaseId == ThirdPartyUsecaseId::Video ? true : false; }
    uint32_t getPostProcOpMode() { return mStreamInfoMap.sessionOpMode; }
    void buildConfiguration(int32_t roleId, StreamConfig *streamConfig);
    void buildRequest(std::shared_ptr<LocalRequest> captureRequest, bool isSnapshot,
                      uint32_t cameraRole);
    void buildVideoRequest(LocalRequest *captureRequest);
    void buildVideoConfig(LocalConfig *config);

    struct SdkSessionConfiguration
    {
        uint32_t operationMode;
        std::string applicationName;
        bool isCameraXConnection;
        uint32_t cameraRole;
    };

    void getVendorStreamInfoMap(InternalStreamInfoMap &map) { map = mStreamInfoMap; }

private:
    void getMatchingUsecase(const StreamConfig *streamConfig, uint32_t sessionOperation,
                            bool isVideo);
    void parseSessionConfiguration(const StreamConfig *streamConfig);
    bool matchVideoStream(uint32_t streamsBitset);
    void convertTag(LocalRequest *captureRequest, bool isSnapshot,
                    std::initializer_list<MISdkTag> sdkTags);
    void convertTagImpl(LocalRequest *captureRequest, MISdkTagInfo &sdkTagInfo, bool isSnapshot);
    void getPhyMialgoSupport(MiSDKPhyAlgoSupport &miSDKPhyAlgoSupport, uint32_t cameraRole);
    void setPhyAlgoEnabled(LocalRequest *captureRequest, uint32_t cameraRole);
    void setThirdYUVSnapshot(LocalRequest *captureRequest, bool isSnapshot);
    inline bool isFeatureEnable(FeatureMask_t mask);
    inline void setAntiShakeEnabled(LocalRequest *captureRequest);
    inline void setBokehEnabled(LocalRequest *captureRequest);
    inline void setMFNRFrameNumbers(LocalRequest *captureRequest);
    inline void resetSuperNightPreviewSettings(LocalRequest *captureRequest, bool isSnapshot);
    inline void setHdrCheckerEnabled(LocalRequest *captureRequest);
    inline void setAsdEnabled(LocalRequest *captureRequest);
    inline void setBeautyPrams(LocalRequest *captureRequest);

    bool mNeedReprocess;
    SdkSessionConfiguration mSessionConfiguration;
    CloudControlInfo mCloudControlInfo;
    InternalStreamInfoMap mStreamInfoMap;
    ThirdPartyUsecaseId mUsecaseId;
    uint32_t mStreamsBitSet;
};

} // namespace mihal
#endif // __MICAMSDKUTILS_H__
