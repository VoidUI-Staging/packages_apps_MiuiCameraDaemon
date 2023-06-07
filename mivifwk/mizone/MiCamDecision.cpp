#include "MiCamDecision.h"

#include <MiDebugUtils.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <string>

#include "AppSnapshotController.h"
#include "MiBufferManager.h"
#include "MiCamJsonUtils.h"
#include "MiCamMode.h"
#include "mtkcam-halif/utils/metadata_tag/1.x/mtk_metadata_tag.h"
#include "xiaomi/MiCommonTypes.h"

using namespace midebug;

namespace mizone {

enum HdrMode {
    HDR_OFF = 0,
    HDR_ON,
    HDR_AUTO,
};

struct HdrChecker
{
    char hdr_hint[4] = {'h', 'd', 'r', ' '};
    int scene_flag;
    uint32_t number;
    int32_t ev[8];
    int scene_type;
};

enum HDRTYPE {
    MODE_HDR = 0,
    MODE_LLHDR,
    MODE_SUPER_LOW_LIGHT_HDR,
};

#define METADATA_VALUE_NUMBER_MAX 16

struct SuperNightChecker
{
    int32_t num;
    int32_t ev[METADATA_VALUE_NUMBER_MAX];
    uint32_t sumExposureTime;
};

/**
 * 夜景Trigger（人像/普通拍照一致)
 * 对应tag: xiaomi.ai.misd.NonSemanticScene
 * 传的是一个AiMisdScene[2]数组
 * 当type字段等于3时，其所对应的value是夜景检测的结果。
 * trigger的结果分两部分：
 * 1.第0位到第7位：app是否需要显示超夜图标（旧项目使用，现在已遗弃）
 * 2.从第8位到第31位：是否trigger夜景算法。0：不触发，1：手持虹软SE，2：手持夜枭
 *（以前就只有虹软SE，所以 0：不触发，1：触发。现在有所改动，请注意）
 **
 * 脚架检测
 * 对应tag: xiaomi.ai.misd.StateScene
 * 传的是一个AiMisdScene结构体
 * type应该为4，value值 0：不认为手机在脚架上，1：认为手机在脚架上
 */
struct AiMisdScene
{
    int32_t type;
    int32_t value;
};

struct SuperNightExif
{
    float lux_index;
    float light;
    float dark_ratio;
    float middle_ratio;
    float bright_ratio;
    float lux_threshold;
};

enum SpecshotMode { SPECSHOT_MODE_MFNR = 0, SPECSHOT_MODE_AINR, SPECSHOT_MODE_AISAINR };

enum MIVISuperNightMode {
    MIVISuperNightNone = 0,
    MIVISuperNightHand = 1,
    MIVISuperNightTripod = 2,
    MIVISuperNightHandExt = 3,
    MIVISuperNightTripodExt = 4,
    MIVISuperLowLightHand = 5,
    MIVISuperLowLightTripod = 6,
    MIVISuperLowLightHandExt = 7,
    MIVISuperLowLightTripodExt = 8,
    MIVISuperNightManuallyOff = 10,
};

template <class T>
bool getEntryForMemory(const MiMetadata *metadata, const MiMetadata::Tag_t tag, T &val,
                       std::string tagName)
{
    if (metadata == nullptr) {
        MLOGD(LogGroupHAL, "MiMetadata input is empty!");
        return false;
    }

    auto entry = metadata->entryFor(tag);
    if (entry.isEmpty()) {
        MLOGD(LogGroupHAL, "%s entry is empty!", tagName.c_str());
        return false;
    } else {
        auto temp = entry.itemAt(0, Type2Type<IMetadata::Memory>());
        val = *reinterpret_cast<T *>(temp.editArray());
    }

    return true;
}

/*************************Definition***************************/
class MiCamDecisionSimple : public MiCamDecision
{
protected:
    struct HdrData
    {
        uint8_t hdrEnabled;
        uint8_t hdrDetected;
        HdrChecker hdrChecker;
    };

    struct SuperNightData
    {
        uint8_t superNightEnabled;
        AiMisdScene seTrigger; // trigger for super night se
        AiMisdScene onTripod;
        SuperNightChecker superNightChecker;
        SuperNightExif SNExif; // camera scene algo supernight se exif
    };

public:
    MiCamDecisionSimple(const CreateInfo &info);
    virtual ~MiCamDecisionSimple();

protected:
    // virtual bool buildMiCaptureOverride(MiCapture::CreateInfo &capInfo) override;
    virtual void determineAlgoType() override;

    virtual void updateSettingsOverride(const MiMetadata &settings) override;
    virtual void updateStatusOverride(const MiMetadata &status) override;

    bool canDoHdr();
    bool composeCapInfoHDR(MiCapture::CreateInfo &capInfo);
    bool composeCapInfoVendorMfnrHDR(MiCapture::CreateInfo &capInfo);
    bool composeCapInfoMihalMfnrHDR(MiCapture::CreateInfo &capInfo);
    bool composeCapInfoSN(MiCapture::CreateInfo &capInfo);

    void updateHdrSetting(const MiMetadata &settings);
    void updateSuperNightSetting(const MiMetadata &settings);
    void updateHdrStatus(const MiMetadata &status);
    void updateSuperNightStatus(const MiMetadata &status);

    bool checkBeauty(const MiMetadata *pAppInMeta);

    // 0: not supported 1: vendor mfnrhdr 2: raw mfnrhdr
    int getHdrType();

    uint8_t mHdrMode;
    uint8_t mMiviSuperNightMode;
    HdrData mHdrData;
    SuperNightData mSnData;
};

class MiCamDecisionBokeh final : public MiCamDecisionSimple
{
public:
    struct BokehInfo
    {
        uint8_t masterCamId;
        uint8_t slaveCamId;
        MiSize masterSize;
        MiSize slaveSize;
    };

public:
    MiCamDecisionBokeh(const CreateInfo &info);

private:
    bool getBokehCamId();
    bool getBokehCamOptSize(float ratio);
    bool buildMiConfigOverride() override;
    void determineAlgoType() override;

    bool composeCapInfoDualBokeh(MiCapture::CreateInfo &capInfo);
    bool composeCapInfoMFNRDualBokeh(MiCapture::CreateInfo &capInfo);
    bool composeCapInfoMihalMfnrHDRDualBokeh(MiCapture::CreateInfo &capInfo);
    bool composeCapInfoSEDualBokeh(MiCapture::CreateInfo &capInfo);
    BokehInfo mBokehInfo;
    std::shared_ptr<MiStream> mMasterStream;
    std::shared_ptr<MiStream> mMasterRawStream;
    std::shared_ptr<MiStream> mMasterSeRawStream;
    std::shared_ptr<MiStream> mSlaveStream;
    std::shared_ptr<MiStream> mSlaveRawStream;
    std::shared_ptr<MiStream> mSlaveSeRawStream;
};

class MiCamDecisionSat final : public MiCamDecisionSimple
{
public:
    struct FusionInfo
    {
        bool isFusionNeed;
        // TODO
        uint32_t camGroupId; // ervery bit is a sensor
    };

public:
    MiCamDecisionSat(const CreateInfo &info);
    // std::shared_ptr<MiConfig> buildMiConfig() override { return nullptr; }
    // void updateSettings(const MiMetadata &settings) override {}
    // void updateStatus(const MiMetadata &status) override {}

private:
    // bool buildMiConfigOverride() override {}
    // bool buildMiCaptureOverride(MiCapture::CreateInfo &capInfo) override;
    void updateSettingsOverride(const MiMetadata &settings) override;
    void updateSettingsInSnapShot(const MiMetadata &settings) override;
    // void updateStatusOverride(const MiMetadata &status) override;
    void determineAlgoType() override;
    void updateMetaInCaptureOverride(MiCapture::CreateInfo &capInfo) override;
    bool composeCapInfoHDSR(MiCapture::CreateInfo &capInfo);
    bool composeCapInfoSR(MiCapture::CreateInfo &capInfo);
    void updateZoomRatioSetting(const MiMetadata &settings);
    void updateBurstSettings(const MiMetadata &settings);
    void updateFusionStatus(const MiMetadata &status) {}
    void CorrectRegionBoundary(int32_t &left, int32_t &top, int32_t &width, int32_t &height,
                               const int32_t bound_w, const int32_t bound_h);
    void ConvertZoomRatio(const float zoom_ratio, MRect &rect,
                          const XiaoMi::MiDimension &active_array_dimension);
    void ConvertZoomRatio(const float zoom_ratio, int32_t &left, int32_t &top, int32_t &width,
                          int32_t &height, const XiaoMi::MiDimension &active_array_dimension);
    virtual void updateMialgoType();

    // NOTE: used to judge whether we should do Sr or Sr+Hdr or other muti frame algorithm
    float mZoomRatio;
    bool mIsZoom1xPlus;
    bool mIsSrNeeded;
    bool mIsHdsrNeeded;

    bool mIsBurstCapture;

    FusionInfo mFusionInfo;
};

class MiCamDecisionSuperHD final : public MiCamDecision
{
public:
    struct SuperHdData
    {
        uint8_t isQcfaSensor;
        MiSize superResolution;
        MiSize binningResolution;
        uint8_t remosaicNeeded;
        uint8_t isRemosaic;
    };

public:
    MiCamDecisionSuperHD(const CreateInfo &info);

private:
    bool getSuperHdData();
    bool buildMiConfigOverride() override;
    void determineAlgoType() override;
    void updateMetaInCaptureOverride(MiCapture::CreateInfo &capInfo) override;

    void updateStatusOverride(const MiMetadata &status) override;

    SuperHdData mSuperHdData;
};

class MiCamDecisionProfessional final : public MiCamDecision
{
public:
    MiCamDecisionProfessional(const CreateInfo &info);

private:
    bool buildMiConfigOverride() override;
    void determineAlgoType() override;
    // bool buildMiCaptureOverride(MiCapture::CreateInfo &capInfo) override;

    bool mHasRawStreamOut = false;
};

/**********************************implementation**********************************************/

std::unique_ptr<MiCamDecision> MiCamDecision::createDecision(const CreateInfo &info)
{
    uint32_t operationMode = static_cast<uint32_t>(info.configFromFwk->rawConfig.operationMode);
    uint32_t roleId = info.cameraInfo.roleId;
    // TODO:should delete in future
    // operationMode = StreamConfigModeAlgoupNormal;
    MLOGI(LogGroupHAL, "opmode 0x%x, roleId %d", operationMode, roleId);
    std::unique_ptr<MiCamDecision> decision;
    switch (operationMode) {
    case StreamConfigModeAlgoupNormal:
        if (roleId == RoleIdFront) {
            decision = std::make_unique<MiCamDecisionSimple>(info);
        } else {
            decision = std::make_unique<MiCamDecisionSat>(info);
        }
        break;
    case StreamConfigModeAlgoupDualBokeh:
        decision = std::make_unique<MiCamDecisionBokeh>(info);
        break;
    case StreamConfigModeAlgoupSingleBokeh:
    case StreamConfigModeMiuiSuperNight:
        decision = std::make_unique<MiCamDecisionSimple>(info);
        break;
    case StreamConfigModeAlgoupHD:
    case StreamConfigModeAlgoupManualUltraPixel:
        decision = std::make_unique<MiCamDecisionSuperHD>(info);
        break;
    case StreamConfigModeAlgoupManual:
        decision = std::make_unique<MiCamDecisionProfessional>(info);
        break;
    default:
        decision = std::unique_ptr<MiCamDecision>(new MiCamDecision(info));
        break;
    }
    return decision;
}

MiCamDecision::MiCamDecision(const CreateInfo &info)
    : mOperationMode(static_cast<uint32_t>(info.configFromFwk->rawConfig.operationMode)),
      mAlgoType(0),
      mFrameRatio(0.f),
      mCamInfo(info.cameraInfo),
      mSpecshotMode(0),
      mIsAinr(false),
      mIsQuickSnapshot(0),
      mAiShutterEnable(0),
      mAiShutterSupport(0),
      mIsMotionExisted(false),
      mMotionAlgoType(0),
      mFlashMode(0),
      mFlashState(0),
      mAdjustEv(0),
      mAiEnable(0),
      mCurrentCamMode(info.camMode),
      mFwkConfig(info.configFromFwk),
      mFwkCapStreamId(-1),
      mPreviewMiStream(nullptr),
      mQrMiStream(nullptr),
      mQuickViewMiStream(nullptr),
      mQuickViewCachedMiStream(nullptr),
      mYuvR3MiStream(nullptr),
      mMaxQuickShotCount(1),
      mIsBurstCapture(false)
{
    MLOGI(LogGroupHAL, "ctor +");
    int32_t isSupportMiZone =
        property_get_int32("persist.vendor.camera.mizone.supportalgomask", -1);
    if (isSupportMiZone >= 0) {
        mSupportAlgoMask = isSupportMiZone;
    } else {
        int32_t basicMask = MI_ALGO_MIHAL_MFNR + MI_ALGO_VENDOR_MFNR;
        switch (mOperationMode) {
        case StreamConfigModeAlgoupNormal:
            mSupportAlgoMask = basicMask + (MI_ALGO_HDR + MI_ALGO_SUPERNIGHT);
            if (mCamInfo.roleId != RoleIdFront) {
                // Rear
                mSupportAlgoMask += MI_ALGO_SR;
            }
            break;
        case StreamConfigModeAlgoupDualBokeh:
            mSupportAlgoMask = MI_ALGO_BOKEH_MIHAL_MFNR + MI_ALGO_SUPERNIGHT + MI_ALGO_HDR;
            break;
        case StreamConfigModeAlgoupSingleBokeh:
            mSupportAlgoMask = MI_ALGO_BOKEH_MIHAL_MFNR;
            break;
        case StreamConfigModeMiuiSuperNight:
            mSupportAlgoMask = basicMask + MI_ALGO_SUPERNIGHT;
            break;
        case StreamConfigModeAlgoupHD:
        case StreamConfigModeAlgoupManualUltraPixel:
        case StreamConfigModeAlgoupManual:
        default:
            mSupportAlgoMask = MI_ALGO_MIHAL_MFNR;
            break;
        }
    }
    MLOGI(LogGroupHAL, "mSupportAlgoMask = %d", mSupportAlgoMask);
    mCaptureTypeName = "Default";
    // use [] there instead of emplace, because we need to override algo function sometimes
    mAlgoFuncMap[MI_ALGO_NONE] = [this](MiCapture::CreateInfo &capInfo) {
        // set isZsl = false because burst capture in L2M
        MINT32 cshot = 0;
        bool isZsl = true;
        MiMetadata::getEntry<MINT32>(&capInfo.settings, MTK_CSHOT_FEATURE_CAPTURE, cshot);
        if (cshot)
            isZsl = false;
        return composeCapInfoSingleNone(capInfo, eImgFmt_NV21, isZsl);
    };
    MiCamJsonUtils::getVal("DecisionList.Sat.MaxQuickShotNums", mMaxQuickShotCount, 3);
    // MiCamJsonUtils::getVal("QuickShot.DelayTimeMask", mQuickShotDelayTimeMask, 0x222022022);
    std::string delayMaskStr, supportMask;
    MiCamJsonUtils::getVal("QuickShot.QuickShotDelayTimeMask", delayMaskStr, "0");
    std::string::size_type sz = 0;
    mQuickShotDelayTimeMask = std::stoull(delayMaskStr, &sz, 0);

    MiCamJsonUtils::getVal("QuickShot.QuickShotSupportedMask", supportMask, "0");
    mQuickShotSupportedMask = std::stoull(supportMask, &sz, 0);
    MLOGI(LogGroupHAL,
          "mQuickShotSupportedMask=0x%x mQuickShotDelayTimeMask=0x%x mMaxQuickShotCount=%d",
          mQuickShotSupportedMask, mQuickShotDelayTimeMask, mMaxQuickShotCount);
    mAlgoFuncMap[MI_ALGO_VENDOR_MFNR] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoVendorMfnr(capInfo);
    };
    mAlgoFuncMap[MI_ALGO_MIHAL_MFNR] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoMihalMfnr(capInfo);
    };

    if (MiMetadata::getEntry<int32_t>(&mCamInfo.staticInfo, XIAOMI_AISHUTTER_SUPPORTED,
                                      mAiShutterSupport)) {
        MLOGD(LogGroupHAL, "get XIAOMI_AISHUTTER_SUPPORTED: %d", mAiShutterSupport);
    }
    MLOGI(LogGroupHAL, "ctor -");
}

MiCamDecision::~MiCamDecision()
{
    MLOGI(LogGroupHAL, "dtor");
}

std::shared_ptr<MiConfig> MiCamDecision::buildMiConfig()
{
    mVndConfig = std::make_shared<MiConfig>();
    float frameRatio = 1.0f;
    for (const auto &[fwkId, fwkStream] : mFwkConfig->streams) {
        switch (fwkStream->usageType) {
        case StreamUsageType::PreviewStream:
            mPreviewMiStream = createBasicMiStream(fwkStream->rawStream, fwkStream->usageType,
                                                   StreamOwner::StreamOwnerFwk, fwkId);
            break;
        case StreamUsageType::QrStream:
            mQrMiStream = createBasicMiStream(fwkStream->rawStream, fwkStream->usageType,
                                              StreamOwner::StreamOwnerFwk, fwkId);
            break;
        case StreamUsageType::QuickViewStream:
            // quickview stream (owned to fwk)
            mQuickViewMiStream =
                createBasicMiStream(fwkStream->rawStream, StreamUsageType::QuickViewStream,
                                    StreamOwner::StreamOwnerFwk, fwkId);

            // quickview cached stream (owned to vnd)
            mQuickViewCachedMiStream =
                createBasicMiStream(fwkStream->rawStream, StreamUsageType::QuickViewCachedStream,
                                    StreamOwner::StreamOwnerVnd, fwkId);
            break;
        case StreamUsageType::SnapshotStream:
            // just calculate and record some parameter here,
            // snapshot streams are created in buildMiConfigOverride
            if (fwkStream->rawStream.format != EImageFormat::eImgFmt_RAW16) {
                mFwkCapStreamId = fwkId;
                frameRatio = static_cast<float>(fwkStream->rawStream.width) /
                             static_cast<float>(fwkStream->rawStream.height);
            } else {
                createBasicMiStream(fwkStream->rawStream, StreamUsageType::SnapshotStream,
                                    StreamOwner::StreamOwnerFwk, fwkId);
            }
            break;
        case StreamUsageType::UnkownStream:
            createBasicMiStream(fwkStream->rawStream, fwkStream->usageType,
                                StreamOwner::StreamOwnerFwk, fwkId);
            break;
        default:
            // tuning data stream, do nothing here
            break;
        }
    }
    mFrameRatio = frameRatio;

    // create snapshot stream when there is cap stream in fwk
    MLOGD(LogGroupHAL, "mFwkCapStreamId: %d", mFwkCapStreamId);
    if (mFwkCapStreamId >= 0) {
        if (!buildMiConfigOverride()) {
            MLOGE(LogGroupHAL, "buildMiConfigOverride failed!!!");
            return nullptr;
        }
    }

    // new mVndConfig->rawConfig,reconfig rawConfig.streams,copy other data
    mVndConfig->rawConfig.operationMode = mFwkConfig->rawConfig.operationMode;
    mVndConfig->rawConfig.streamConfigCounter = mFwkConfig->rawConfig.streamConfigCounter;
    mVndConfig->rawConfig.sessionParams = mFwkConfig->rawConfig.sessionParams;

    // close tuning data
    MiMetadata::setEntry<uint8_t>(&mVndConfig->rawConfig.sessionParams,
                                  MTK_CONTROL_CAPTURE_ISP_TUNING_DATA_ENABLE, 0);

    // no need zsl mode in manual / super night mode
    if (mOperationMode == StreamConfigModeAlgoupManual ||
        mOperationMode == StreamConfigModeAlgoupManualUltraPixel ||
        mOperationMode == StreamConfigModeMiuiSuperNight) {
        MiMetadata::setEntry<uint8_t>(&mVndConfig->rawConfig.sessionParams,
                                      MTK_CONTROL_CAPTURE_ZSL_MODE,
                                      MTK_CONTROL_CAPTURE_ZSL_MODE_OFF);
    } else {
        MiMetadata::setEntry<uint8_t>(&mVndConfig->rawConfig.sessionParams,
                                      MTK_CONTROL_CAPTURE_ZSL_MODE,
                                      MTK_CONTROL_CAPTURE_ZSL_MODE_ON);
    }

    for (auto &[id, stream] : mVndConfig->streams) {
        mVndConfig->rawConfig.streams.push_back(stream->rawStream);
    }

    for (auto [vndId, fwkId] : mStreamIdToFwkMap) {
        MLOGD(LogGroupHAL, "streamId: VND:%d FWK:%d", vndId, fwkId);
    }
    return mVndConfig;
}

void MiCamDecision::updateAlgoMetaInCapture(MiCapture::CreateInfo &capInfo)
{
    if (capInfo.operationMode == StreamConfigModeAlgoupNormal ||
        capInfo.operationMode == StreamConfigModeAlgoupDualBokeh ||
        capInfo.operationMode == StreamConfigModeAlgoupSingleBokeh ||
        capInfo.operationMode == StreamConfigModeMiuiSuperNight) {
        // enable portraitrepair algorithm
        MiMetadata::setEntry<MUINT8>(&capInfo.settings, XIAOMI_PORTRAITREPAIR_ENABLED, 1);
    }
}

std::shared_ptr<MiCapture> MiCamDecision::buildMiCapture(CaptureRequest &fwkRequest)
{
    MLOGD(LogGroupHAL, "+: frameNum = %d", fwkRequest.frameNumber);
    MiCapture::CreateInfo capInfo{
        .mode = mCurrentCamMode,
        .cameraInfo = mCamInfo,
        .fwkRequest = fwkRequest,
        .frameCount = 1,
        .settings = fwkRequest.settings,
        .mapStreamIdToFwk = mStreamIdToFwkMap,
        .operationMode = mOperationMode,
    };
    MLOGD(LogGroupHAL, "fwkRequest.physicalCameraSettings.size = %lu",
          fwkRequest.physicalCameraSettings.size());

    bool containPreview = false;
    bool containQrStream = false;
    bool containSnapshot = false;
    mContainQuickViewBuffer = false;
    mIsHeifFormat = false;
    for (auto &&buffer : fwkRequest.outputBuffers) {
        auto usageType = mFwkConfig->streams[buffer.streamId]->usageType;
        if (usageType == StreamUsageType::PreviewStream) {
            MLOGD(LogGroupHAL, "fwk req (frameNum = %d),contain preview", fwkRequest.frameNumber);
            containPreview = true;
        } else if (usageType == StreamUsageType::QrStream) {
            MLOGD(LogGroupHAL, "fwk req (frameNum = %d),contain qr", fwkRequest.frameNumber);
            containQrStream = true;
        } else if (usageType == StreamUsageType::SnapshotStream) {
            MLOGI(LogGroupHAL, "fwk req (frameNum = %d),contain snapshot", fwkRequest.frameNumber);
            containSnapshot = true;
        } else if (usageType == StreamUsageType::QuickViewStream) {
            MLOGD(LogGroupHAL, "fwk req (frameNum = %d),contain quickView", fwkRequest.frameNumber);
            mContainQuickViewBuffer = true;
        }
    }

    if (!containPreview && !containSnapshot) {
        MLOGE(LogGroupHAL, "fwk req (frameNum = %d) contains no preview or snapshot buffers!",
              fwkRequest.frameNumber);
        return nullptr;
    }

    if (containSnapshot) {
        capInfo.type = MiCapture::CaptureType::SnapshotType;
        updateSettingsInSnapShot(fwkRequest.settings);
    } else {
        capInfo.type = MiCapture::CaptureType::PreviewType;
    }

    if (containPreview) {
        if (mPreviewMiStream != nullptr) {
            capInfo.streams.push_back(mPreviewMiStream);
        } else {
            MLOGE(LogGroupHAL, "mPreviewMiStream is nullptr!!!");
            return nullptr;
        }
        if (mQuickViewCachedMiStream != nullptr && !containSnapshot) {
            capInfo.streams.push_back(mQuickViewCachedMiStream);
        }
        // if (containQrStream && mQrMiStream != nullptr) {
        //     capInfo.streams.push_back(mQrMiStream);
        // }
    }
    MiMetadata::getEntry<uint8_t>(&capInfo.settings, MTK_CONTROL_ENABLE_ZSL,
                                  capInfo.isAppZslEnable);
    if (!buildMiCaptureOverride(capInfo)) {
        MLOGE(LogGroupHAL, "buildMiCaptureOverride failed!!!");
        return nullptr;
    }

    if (capInfo.type == MiCapture::CaptureType::SnapshotType) {
        updateAlgoMetaInCapture(capInfo);
        setCaptureTypeName(capInfo);
        std::string pipelineName(capInfo.captureTypeName);
        pipelineName += "_" + std::to_string(fwkRequest.frameNumber);
        MiMetadata::updateEntry<char, MUINT8>(fwkRequest.settings, XIAOMI_SNAPSHOT_IMAGENAME,
                                              pipelineName.c_str(), pipelineName.size() + 1);
    }

    MLOGD(LogGroupHAL, "-");

    return MiCapture::create(capInfo);
}

int MiCamDecision::queryFeatureSetting(const MiMetadata &info, uint32_t &frameCount,
                                       std::vector<MiMetadata> &settings)
{
    int ret = 0;
    ret = mCurrentCamMode->queryFeatureSetting(info, frameCount, settings);
    return ret;
}

bool MiCamDecision::buildMiConfigOverride()
{
    // create yuv snapshot stream for vendor mfnr
    StreamParameter yuvPara = {
        .isPhysicalStream = false,
        .format = eImgFmt_NV21,
    };
    if (nullptr == createSnapshotMiStream(yuvPara)) {
        MLOGE(LogGroupHAL, "createSnapshotMiStream failed!");
        return false;
    }

    // create BAYER10 snapshot stream ,only one rawStream
    StreamParameter rawPara = {
        .isPhysicalStream = false,
        .format = eImgFmt_BAYER10,
    };
    if (mSupportAlgoMask & AlgoType::MI_ALGO_MIHAL_MFNR) {
        rawPara.needYuvR3Stream = true;
    }
    if (nullptr == createSnapshotMiStream(rawPara)) {
        MLOGE(LogGroupHAL, "createSnapshotMiStream failed!");
        return false;
    }

    // create eImgFmt_RAW16 snapshot stream for sn/ell
    StreamParameter raw16Para = {
        .isPhysicalStream = false,
        .format = eImgFmt_RAW16,
    };
    if (nullptr == createSnapshotMiStream(raw16Para)) {
        MLOGE(LogGroupHAL, "createSnapshotMiStream failed!");
        return false;
    }

    return true;
}

std::shared_ptr<MiStream> MiCamDecision::createBasicMiStream(const Stream &srcStream,
                                                             StreamUsageType usageType,
                                                             StreamOwner owner, int32_t fwkStreamId)
{
    auto stream = std::make_shared<MiStream>();
    stream->rawStream = srcStream;
    stream->usageType = usageType;
    stream->owner = owner;
    if (owner == StreamOwner::StreamOwnerFwk) {
        stream->streamId = fwkStreamId;
    } else {
        stream->streamId = mVndOwnedStreamId++;
    }
    stream->rawStream.id = stream->streamId;
    mStreamIdToFwkMap.emplace(stream->streamId, fwkStreamId);
    mVndConfig->streams.emplace(stream->streamId, stream);
    return stream;
}

std::shared_ptr<MiStream> MiCamDecision::createSnapshotMiStream(StreamParameter &para,
                                                                int overrideWidth,
                                                                int overrideHeight)
{
    bool isRaw = isRawFormat(para.format);
    if (para.format != eImgFmt_NV21 && !isRaw) {
        MLOGI(LogGroupHAL, "format %d does not support creating snapshot streams", para.format);
        return nullptr;
    }
    MiSize resolution;
    if (overrideWidth == -1 && overrideHeight == -1) {
        resolution = {mCamInfo.activeArraySize.width, mCamInfo.activeArraySize.height};
        MiSize physicalRes = getPhysicalResolution(para.cameraId);
        if (physicalRes.width > 0 && isRaw) {
            resolution = physicalRes;
        }
    } else {
        resolution = {(uint32_t)overrideWidth, (uint32_t)overrideHeight};
    }
    MLOGI(LogGroupHAL, "width = %d, height = %d", resolution.width, resolution.height);

    // get fwk snapshot Stream ptr only for read
    if (mFwkCapStreamId < 0) {
        MLOGE(LogGroupHAL, "mFwkCapStreamId error!");
        return nullptr;
    }

    // copy from fwk cap stream
    Stream rawStream = mFwkConfig->streams[mFwkCapStreamId]->rawStream;
    rawStream.width = resolution.width;
    rawStream.height = resolution.height;
    rawStream.format = para.format;
    // TODO:should change buffer size in future;

    if (para.isPhysicalStream) {
        rawStream.physicalCameraId = para.cameraId;
    }
    if (isRaw) {
        composeRawStream(rawStream);
        rawStream.streamType = StreamType::OUTPUT;
        // NOTE: if set MTK_HALCORE_STREAM_SOURCE here, Core Session will not maintain a
        //       zsl pool, that means we should maintain one in mizone
        // MiMetadata::setEntry<MINT32>(&stream->rawStream.settings, MTK_HALCORE_STREAM_SOURCE,
        //                              MTK_HALCORE_STREAM_SOURCE_FULL_RAW);
    }
    auto stream = createBasicMiStream(rawStream, StreamUsageType::SnapshotStream,
                                      StreamOwner::StreamOwnerVnd, mFwkCapStreamId);
    if (para.needYuvR3Stream) {
        Stream yuvR3Stream;
        composeYuvoR3Stream(yuvR3Stream);
        mYuvR3MiStream = createBasicMiStream(yuvR3Stream, StreamUsageType::SnapshotStream,
                                             StreamOwner::StreamOwnerVnd, mFwkCapStreamId);
    }
    return stream;
}

void MiCamDecision::composeRawStream(Stream &stream)
{
    stream.transform = NSCam::eTransform_None;
    int bits;
    if (stream.format == eImgFmt_BAYER10 || stream.format == eImgFmt_RAW10) {
        bits = 10;
    } else {
        bits = 16;
    }

    // 64 bytes alignment covers: 2, 4, 8, 16, 32, 64 bytes aligned constrains
    auto align64 = [](int num) { return ((num + 63) >> 6) << 6; };

    // request the buffer size
    if (stream.format == eImgFmt_RAW16) {
        // it need to modify rowStrideInBytes for different algorithms
        stream.bufPlanes.planes[0].rowStrideInBytes = stream.width * bits / 8;
    } else {
        // BAYER10 is 10 bits packed RAW, bits per pixel is 10
        stream.bufPlanes.planes[0].rowStrideInBytes = ((align64(stream.width)) * bits / 8);
    }
    stream.bufPlanes.planes[0].sizeInBytes =
        stream.bufPlanes.planes[0].rowStrideInBytes * stream.height;

    stream.bufPlanes.count = 1; // single plan
    stream.bufferSize = stream.bufPlanes.planes[0].sizeInBytes;
}

void MiCamDecision::composeYuvStream(Stream &stream) {}

void MiCamDecision::composeYuvoR3Stream(Stream &stream)
{
    stream.streamType = StreamType::OUTPUT;
    stream.format = EImageFormat::eImgFmt_MTK_YUV_P010;
    stream.transform = NSCam::eTransform_None;
    // MiMetadata::setEntry<MINT32>(&stream.settings, MTK_HALCORE_STREAM_SOURCE,
    //                             MTK_HALCORE_STREAM_SOURCE_YUV_R3);

    const int w_min = 992, w_max = 1248;
    const int h_min = 736, h_max = 944;
    const int align = 16;
    int w = mCamInfo.activeArraySize.width / 4;
    int h = mCamInfo.activeArraySize.height / 4;

    stream.width = (std::clamp(w, w_min, w_max) / align) * align;
    stream.height = (std::clamp(h, h_min, h_max) / align) * align;
}

MiSize MiCamDecision::getPhysicalResolution(int32_t cameraId)
{
    MiSize result = {0, 0};
    if (mCamInfo.physicalMetadataMap.count(cameraId) > 0) {
        auto activeArraySizeEntry =
            mCamInfo.physicalMetadataMap[cameraId].entryFor(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE);
        if (activeArraySizeEntry.count() > 0) {
            MLOGD(LogGroupCore, "activeArraySizeEntry.count: %d", activeArraySizeEntry.count());
            const auto *array =
                static_cast<const int32_t *>(activeArraySizeEntry.data()); // x, y, w, h
            result.width = array[2];
            result.height = array[3];
            MLOGD(LogGroupCore, "activeArraySize: %d, %d", array[2], array[3]);
        } else {
            MLOGE(LogGroupCore, "entry ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE is empty!");
        }
    }
    return result;
}

std::shared_ptr<MiStream> MiCamDecision::getPhyStream(EImageFormat format, int32_t phyId) const
{
    for (auto &pair : mVndConfig->streams) {
        std::shared_ptr<MiStream> stream = pair.second;
        if (stream->usageType == StreamUsageType::SnapshotStream) {
            if (stream->rawStream.physicalCameraId == phyId && stream->rawStream.format == format)
                return stream;
        }
    }
    MLOGE(LogGroupHAL, "getPhyStream empty!!!");
    return nullptr;
}

void MiCamDecision::updateSettings(const MiMetadata &settings)
{
    // NOTE: read flash light mode (MTK_FLASH_MODE or ANDROID_FLASH_MODE)
    if (MiMetadata::getEntry<uint8_t>(&settings, MTK_FLASH_MODE, mFlashMode)) {
        MLOGD(LogGroupHAL, "user set flash light mode: %d", mFlashMode);
    }

    if (MiMetadata::getEntry<MUINT8>(&settings, MTK_CONTROL_AE_MODE, mAeMode)) {
        MLOGD(LogGroupHAL, "user set AE mode: %d", mAeMode);
    }

    if (MiMetadata::getEntry<uint8_t>(&settings, XIAOMI_AISHUTTER_FEATURE_ENABLED,
                                      mAiShutterEnable)) {
        MLOGD(LogGroupHAL, "user set aishtter enable: %d", mAiShutterEnable);
    }

    if (MiMetadata::getEntry<MINT32>(&settings, MTK_CONTROL_AE_EXPOSURE_COMPENSATION, mAdjustEv)) {
        MLOGD(LogGroupHAL, "user set adjust ev: %d", mAdjustEv);
    }

    updateSettingsOverride(settings);
}

void MiCamDecision::updateStatus(const MiMetadata &status)
{
    mPreviewCaptureResult = status;
    // NOTE: read flash light status (MTK_FLASH_STATE)
    if (MiMetadata::getEntry<uint8_t>(&status, MTK_FLASH_STATE, mFlashState)) {
        MLOGD(LogGroupHAL, "hal return flash light state: %d", mFlashState);
    }

    // NOTE: read specshot mode (XIAOMI_SPECSHOT_MODE_DETECTED) MFNR=0，AINR=1
    // default: 0, set to 1 when WIDE and iso > 1200 (in MiMetaDataResult3A.cpp)
    if (MiMetadata::getEntry<int32_t>(&status, XIAOMI_SPECSHOT_MODE_DETECTED, mSpecshotMode)) {
        MLOGD(LogGroupHAL, "hal return specshot Mode: %d", mSpecshotMode);
    }

    int32_t isAiShutExistMotion = 0;
    if (MiMetadata::getEntry<int32_t>(&status, MTK_3A_FEATURE_AISHUT_CAPTURE,
                                      isAiShutExistMotion)) {
        MLOGD(LogGroupHAL, "hal return isAiShutExistMotion: %d", isAiShutExistMotion);
    } else {
        isAiShutExistMotion = 0;
        MLOGD(LogGroupHAL, "could not get MTK_3A_FEATURE_AISHUT_CAPTURE");
    }

    int32_t value = 0;
    if (MiMetadata::getEntry<int32_t>(&status, XIAOMI_MOTION_CAPTURE_TYPE, value)) {
        value &= 0x0F;
        int type = value & 0xFF;
        if (type > 0)
            mIsMotionExisted = true;
        else
            mIsMotionExisted = false;

        // For MTK devices that do not support default on, use isAishutExistMotion to get the motion
        // status. And use the || to compatible with different situation.
        mIsMotionExisted = mIsMotionExisted || (isAiShutExistMotion && mAiShutterEnable);
        MLOGD(LogGroupHAL, "hal return isMotionExisted: %d", mIsMotionExisted);
    } else {
        mIsMotionExisted = false;
        MLOGD(LogGroupHAL, "could not get XIAOMI_MOTION_CAPTURE_TYPE");
    }

    updateStatusOverride(status);
}

bool MiCamDecision::buildMiCaptureOverride(MiCapture::CreateInfo &capInfo)
{
    MLOGD(LogGroupHAL, "buildMiCaptureOverride()+");
    MLOGD(LogGroupHAL, "capInfo.type = %d", capInfo.type);
    if (capInfo.type == MiCapture::CaptureType::PreviewType) {
        return true;
    }
    updateQuickSnapshot(capInfo);
    determineAlgoType();
    if ((MI_ALGO_NONE != mAlgoType) && getMotionAlgoType()) {
        mAlgoType = (mAlgoType & MI_ALGO_BOKEH) ? MI_ALGO_BOKEH_MIHAL_MFNR : MI_ALGO_MIHAL_MFNR;
    }
    MLOGI(LogGroupHAL, "mAlgoType = %d", mAlgoType);
    if (mAlgoFuncMap.find(mAlgoType) != mAlgoFuncMap.end()) {
        if (!mAlgoFuncMap[mAlgoType](capInfo)) {
            MLOGE(LogGroupHAL, "composeCapInfo failed!");
            return false;
        }
    } else {
        MLOGE(LogGroupHAL, "mAlgoType error!");
    }
    MLOGD(LogGroupHAL, "buildMiCaptureOverride()-");
    return true;
}

void MiCamDecision::updateQuickSnapshot(MiCapture::CreateInfo &capInfo)
{
    mIsQuickSnapshot = 0;
    MiMetadata::getEntry<MUINT8>(&capInfo.settings, XIAOMI_QUICKSNAPSHOT_ISQUICKSNAPSHOT,
                                 mIsQuickSnapshot);
    if (isSupportQuickShot()) {
        MiMetadata::setEntry<MUINT8>(&capInfo.settings, XIAOMI_HIGHQUALITYQUICKSHOT_ISHQQUICKSHOT,
                                     1);
        capInfo.quickShotDelayTime = getQuickShotDelayTime();
        capInfo.mQuickShotEnabled = true;
    }
    MiMetadata::setEntry<MUINT8>(&capInfo.settings, XIAOMI_QUICKSNAPSHOT_ISQUICKSNAPSHOT,
                                 mIsQuickSnapshot);
    MLOGI(LogGroupHAL, "mIsQuickSnapshot: %d", mIsQuickSnapshot);
}

bool MiCamDecision::updateMetaInCapture(MiCapture::CreateInfo &capInfo, bool isZsl, int32_t *ev)
{
    if (isZsl) {
        MiMetadata::setEntry<uint8_t>(&capInfo.settings, MTK_CONTROL_ENABLE_ZSL,
                                      MTK_CONTROL_ENABLE_ZSL_TRUE);
        if (mContainQuickViewBuffer) {
            if (capInfo.frameCount > 1) {
                capInfo.isZslEnable = true;
            } else {
                mCurrentCamMode->addAnchorFrame(capInfo.fwkRequest.frameNumber,
                                                capInfo.fwkRequest.frameNumber - 1);
            }
        }
    } else {
        // non zsl return N-1 QR buffer
        if (mQuickViewCachedMiStream != nullptr && mContainQuickViewBuffer) {
            if (mAlgoType != MI_ALGO_MIHAL_MFNR) {
                mCurrentCamMode->addAnchorFrame(capInfo.fwkRequest.frameNumber,
                                                capInfo.fwkRequest.frameNumber - 1);
            }
        }
        MiMetadata::setEntry<uint8_t>(&capInfo.settings, MTK_CONTROL_ENABLE_ZSL,
                                      MTK_CONTROL_ENABLE_ZSL_FALSE);
    }

    uint8_t mfnrEnable =
        ((mAlgoType & MI_ALGO_VENDOR_MFNR) || ((mAlgoType & MI_ALGO_MIHAL_MFNR) && !mIsAinr)) ? 1
                                                                                              : 0;
    MiMetadata::setEntry<uint8_t>(&capInfo.settings, XIAOMI_MFNR_ENABLED, mfnrEnable);

    uint8_t hdrEnable = (mAlgoType & MI_ALGO_HDR) ? 1 : 0;
    MiMetadata::setEntry<MUINT8>(&capInfo.settings, XIAOMI_HDR_FEATURE_ENABLED, hdrEnable);
    int isFrontRawProcess = 0;
    MiCamJsonUtils::getVal("DecisionList.SuperNight.isFrontRawProcess", isFrontRawProcess, 0);
    uint8_t rawSnEnable = ((mAlgoType & MI_ALGO_SUPERNIGHT) &&
                           ((RoleIdFront != mCamInfo.roleId) || isFrontRawProcess))
                              ? 1
                              : 0;
    MiMetadata::setEntry<MUINT8>(&capInfo.settings, XIAOMI_SuperNight_FEATURE_ENABLED, rawSnEnable);

    uint8_t srEnable = (mAlgoType & MI_ALGO_SR) ? 1 : 0;
    MiMetadata::setEntry<MUINT8>(&capInfo.settings, XIAOMI_SR_ENABLED, srEnable);

    if (capInfo.frameCount >= 1) {
        MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_COUNT,
                                     capInfo.frameCount);
    }

    updateMetaInCaptureOverride(capInfo);

    if (capInfo.frameCount <= 1 || !capInfo.settingsPerframe.empty()) {
        MLOGD(LogGroupHAL, "skip multiframe config");
        return true;
    }
    for (int i = 0; i < capInfo.frameCount; ++i) {
        MiMetadata eachSetting;
        // or MTK_CONTROL_AE_EXPOSURE_COMPENSATION
        if (nullptr != ev) {
            MiMetadata::setEntry<MINT32>(&eachSetting, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                         ev[i]);
        }
        MiMetadata::setEntry<MINT32>(&eachSetting, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_INDEX, i);
        capInfo.settingsPerframe.push_back(std::move(eachSetting));
    }
    return true;
}

bool MiCamDecision::composeCapInfoSingleNone(MiCapture::CreateInfo &capInfo, EImageFormat format,
                                             bool isZsl)
{
    auto stream = getPhyStream(format);
    if (stream == nullptr) {
        MLOGE(LogGroupHAL, "getPhyStream failed");
        return false;
    }
    capInfo.streams.push_back(stream);
    if (!updateMetaInCapture(capInfo, isZsl)) {
        return false;
    }
    return true;
}

bool MiCamDecision::composeCapInfoVendorMfnr(MiCapture::CreateInfo &capInfo)
{
    return composeCapInfoSingleNone(capInfo);
}

bool MiCamDecision::composeCapInfoMihalMfnr(MiCapture::CreateInfo &capInfo)
{
    auto stream = getPhyStream(eImgFmt_BAYER10);
    // For app sanpshot control
    if (stream == nullptr) {
        MLOGE(LogGroupHAL, "getPhyStream failed");
        return false;
    }
    capInfo.streams.push_back(stream);
    MiMetadata info = mPreviewCaptureResult;
    std::vector<MiMetadata> featureSettings;
    int32_t feature = 0;
    uint32_t frame_cnt = 0;
    bool isZsl = false;

    switch (mMotionAlgoType) {
    case TYPE_AIS1:
        mIsAinr = true;
        feature = MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AINR;
        MLOGD(LogGroupHAL, "multiframe feature is aishutter1.0");
        break;
    case TYPE_AIS2:
        mIsAinr = true;
        feature = MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AISHUTTER2;
        MLOGD(LogGroupHAL, "multiframe feature is aishutter2.0");
        break;
    case TYPE_MFNR:
        mIsAinr = false;
        feature = MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_MFNR;
        MLOGD(LogGroupHAL, "multiframe feature is mfnr");
        break;
    default:
        // use AEMode instead of flashMode in hal
        mIsAinr = MTK_CONTROL_AE_MODE_ON_AUTO_FLASH != mAeMode && mSpecshotMode != 0 &&
                  (mSpecshotMode == SPECSHOT_MODE_AINR || mSpecshotMode == SPECSHOT_MODE_AISAINR) &&
                  !mIsQuickSnapshot;
        feature = mIsAinr ? MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AINR
                          : MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_MFNR;
    }
    MLOGD(LogGroupHAL, "motionAlgoType: %d, multiframe feature: %d", mMotionAlgoType, feature);

    if (TYPE_AIS1 == mMotionAlgoType || TYPE_AIS2 == mMotionAlgoType) {
        int32_t aiShutIso = 0;
        int32_t aiShutExposureTime = 0;
        if (MiMetadata::getEntry<int32_t>(&info, MTK_3A_FEATURE_AISHUT_ISO, aiShutIso)) {
            MiMetadata::setEntry<int32_t>(&info, MTK_SENSOR_SENSITIVITY, aiShutIso);
        }
        if (MiMetadata::getEntry<int32_t>(&info, MTK_3A_FEATURE_AISHUT_EXPOSURETIME,
                                          aiShutExposureTime)) {
            MiMetadata::setEntry<MINT64>(&info, MTK_SENSOR_EXPOSURE_TIME,
                                         (int64_t)aiShutExposureTime);
        }
    } else {
        int32_t isoValue = 0;
        int64_t exposureTimeValue = 0;
        if (MiMetadata::getEntry<int32_t>(&capInfo.settings, MTK_SENSOR_SENSITIVITY, isoValue)) {
            MiMetadata::setEntry<int32_t>(&info, MTK_SENSOR_SENSITIVITY, isoValue);
        }
        if (MiMetadata::getEntry<MINT64>(&capInfo.settings, MTK_SENSOR_EXPOSURE_TIME,
                                         exposureTimeValue)) {
            MiMetadata::setEntry<MINT64>(&info, MTK_SENSOR_EXPOSURE_TIME, exposureTimeValue);
        }
    }

    // MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_FEATURE = MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING
    MiMetadata::setEntry<MINT32>(&info, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, feature);

    MiMetadata::setEntry<MUINT8>(&info, XIAOMI_QUICKSNAPSHOT_ISQUICKSNAPSHOT, mIsQuickSnapshot);
    if (!queryFeatureSetting(info, frame_cnt, featureSettings)) {
        capInfo.frameCount = frame_cnt;
        // add metadata (private redefined tag)
        // NOTE: doBss left, note that should also set MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS = 0
        //       when request to PostProc Session
        if (TYPE_AIS1 == mMotionAlgoType) {
            feature = MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_AISHUTTER1;
        }
        MiMetadata::setEntry<MINT32>(&capInfo.settings,
                                     MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_FEATURE, feature);
        MiMetadata::setEntry(&capInfo.settings, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 1);
        if (mIsAinr) {
            if (mYuvR3MiStream == nullptr) {
                MLOGE(LogGroupHAL, "YuvR3MiStream is nullptr!!!");
                return false;
            }
            capInfo.streams.push_back(mYuvR3MiStream);
        }
        MLOGI(LogGroupHAL, "queryFeatureSetting result: %d", frame_cnt);
    } else {
        MLOGE(LogGroupHAL, "queryFeatureSetting failed");
        mAlgoType = MI_ALGO_NONE;
        frame_cnt = 0;
        MiMetadata::setEntry<MINT32>(&capInfo.settings,
                                     MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_FEATURE, 0);
    }

    // zsl enable (no need in manual / front supter night mode)
    if (mOperationMode == StreamConfigModeAlgoupManual ||
        mOperationMode == StreamConfigModeAlgoupManualUltraPixel ||
        mOperationMode == StreamConfigModeMiuiSuperNight) {
        isZsl = false;
    } else {
        isZsl = true;
    }

    for (size_t i = 0; i < frame_cnt; ++i) {
        // append featureSettings
        MiMetadata eachSetting = featureSettings[i];
        MiMetadata::setEntry(&eachSetting, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_INDEX,
                             static_cast<int32_t>(i));
        capInfo.settingsPerframe.push_back(std::move(eachSetting));
    }

    if (!updateMetaInCapture(capInfo, isZsl)) {
        return false;
    }
    return true;
}

void MiCamDecision::determineAlgoType()
{
    // mAlgoType = MI_ALGO_VENDOR_MFNR;
    if (mSupportAlgoMask & MI_ALGO_MIHAL_MFNR) {
        mAlgoType = MI_ALGO_MIHAL_MFNR;
    }
}

bool MiCamDecision::getMotionAlgoType()
{
    mMotionAlgoType = TYPE_DEFAULT;
    bool shouldDoSR = MI_ALGO_SR == mAlgoType;
    bool isHDR = (mAlgoType & MI_ALGO_HDR);
    MLOGD(LogGroupHAL, "shouldDoSR: %d, isMotionExisted: %d, isQuickSnapshot: %d", shouldDoSR,
          mIsMotionExisted, mIsQuickSnapshot);
    bool isMotionCapture = false;
    if (mIsMotionExisted) {
        if (shouldDoSR) {
            // 1.如果当前shouldDoSR为true，那么检查下配置里面是不是设置了替换成MFNR
            // 如果是，则shouldDoSR=false，后续走MFNR。MTK平台会替换为AINR或者MFNR
            bool MFNRReplaceSRWhenMotion = isNeedReplaceSrWithMfnr();
            shouldDoSR = !MFNRReplaceSRWhenMotion;
        }
        // 2.如果当前isMotionCapture是true，则走专用的运动降噪算法。
        //  对于MTK平台，这个返回值也区分了支持运动抓拍默认开启的机型跟不支持的机型，分别是true跟false。
        isMotionCapture = isCurrentModeSupportAISDenoise();
        MLOGD(LogGroupHAL, "isMotionCapture：: %d, shouldDoSR: %d, isQuickSnapshot: %d",
              isMotionCapture, shouldDoSR, mIsQuickSnapshot);
        // 3.对于MTK平台，需要进一步确定具体用的是AIS1还是AIS2算法来做运动抓拍。
        if (!shouldDoSR) {
            // 4.如果isMotionCapture是true，则说明是支持运动抓拍默认开启的机型，支持MFNR/AIS1/AIS2三种算法的动态切换。
            if (isMotionCapture) {
                // 4.1.非快拍下，才触发运动抓拍的专用算法。快拍条件下，算法类型按照快拍的策略。
                // TODO: isSupportedMixQuickShot()
                if (!mIsQuickSnapshot) {
                    // 4.2.如果是运动+HDR条件，则走AIS2算法
                    if (isHDR && isCurrentModeSupportHdrAIS()) {
                        mMotionAlgoType = TYPE_AIS2;
                        MLOGD(LogGroupHAL, "select AIS2 in HDR & motion scenario");
                        return true;
                        // 4.3.运动+非HDR条件，走AIS1拍照
                    } else {
                        mMotionAlgoType = TYPE_AIS1;
                        MLOGD(LogGroupHAL, "select AIS1 in motion scenario");
                        return true;
                    }
                } else if (isHDR) {
                    // 运动快拍HDR维持MFNR+HDR
                    return false;
                } else {
                    // 4.4 打开AIS,有motion快拍的情况需要切换到MFNR
                    mMotionAlgoType = TYPE_MFNR;
                    MLOGD(LogGroupHAL, "use mfnr replace AIS");
                    return true;
                }
                // 5.如果是如果isMotionCapture是false，则说明是不支持运动抓拍默认开启的机型。运动抓拍的算法类型
                // 根据底层上报的AIS版本是1还是2直接走到对应的算法。
            } else {
                // 5.1.当前是支持AIS2的机型，直接走AIS2
                if (isMtkAiShutterVersionTwo()) {
                    mMotionAlgoType = TYPE_AIS2;
                    MLOGD(LogGroupHAL, "select AIS2 in device that supports AIS2");
                    return true;
                    // 5.2.当前是支持AIS1的机型，直接走AIS1
                } else if (isMtkAiShutterVersionOne()) {
                    mMotionAlgoType = TYPE_AIS1;
                    MLOGD(LogGroupHAL, "select AIS1 in device that supports AIS1");
                    return true;
                }
            }
        }
    }

    return false;
}

bool MiCamDecision::isCurrentModeSupportAISDenoise()
{
    return (StreamConfigModeAlgoupNormal == mOperationMode && isCaptureAiShutterDenoiseEnable()) ||
           (StreamConfigModeAlgoupDualBokeh == mOperationMode &&
            isPortraitAiShutterDenoiseEnable());
}

bool MiCamDecision::isCurrentModeSupportHdrAIS()
{
    return (StreamConfigModeAlgoupNormal == mOperationMode && isCaptureAiShutterHDREnable()) ||
           (StreamConfigModeAlgoupDualBokeh == mOperationMode && isPortraitAiShutterHDREnable());
}

// For app sanpshot control
void MiCamDecision::setCaptureTypeName(MiCapture::CreateInfo &capInfo)
{
    capInfo.captureTypeName += mCaptureTypeName;
    if (capInfo.streams.size() > 0) {
        capInfo.captureTypeName += "_";
        capInfo.captureTypeName += std::to_string(capInfo.streams[0]->rawStream.width).c_str();
        capInfo.captureTypeName += "x";
        capInfo.captureTypeName += std::to_string(capInfo.streams[0]->rawStream.height).c_str();
        capInfo.captureTypeName += " ";
    }
    switch (mAlgoType) {
    case MI_ALGO_VENDOR_MFNR:
        capInfo.captureTypeName += "MI_ALGO_VENDOR_MFNR";
        break;
    case MI_ALGO_MIHAL_MFNR:
        if (!(getMiCamDecisionName().compare("SuperHD"))) {
            capInfo.captureTypeName += "MI_ALGO_HD_UPSCALE";
        } else {
            if (mIsAinr) {
                capInfo.captureTypeName += "MI_ALGO_MIHAL_AINR";
            } else {
                capInfo.captureTypeName += "MI_ALGO_MIHAL_MFNR";
            }
        }
        break;
    case MI_ALGO_HDR:
        if (capInfo.frameCount == 8) {
            capInfo.captureTypeName += "MI_ALGO_LLHDR";
        } else {
            capInfo.captureTypeName += "MI_ALGO_HDR";
        }
        break;
    case MI_ALGO_SUPERNIGHT:
        if (capInfo.cameraInfo.roleId == 1) {
            capInfo.captureTypeName += "MI_ALGO_SUPERNIGHT_FRONT";
        } else {
            capInfo.captureTypeName += "MI_ALGO_SUPERNIGHT_BACK";
            capInfo.singleFrameEnable = true;
        }

        break;
    case MI_ALGO_SR:
        capInfo.captureTypeName += "MI_ALGO_SR";
        break;
    case MI_ALGO_BOKEH:
        capInfo.captureTypeName += "MI_ALGO_BOKEH";
        break;
    case MI_ALGO_BOKEH_VENDOR_MFNR:
        capInfo.captureTypeName += "MI_ALGO_BOKEH_VENDOR_MFNR";
        break;
    case MI_ALGO_BOKEH_MIHAL_MFNR:
        capInfo.captureTypeName += "MI_ALGO_BOKEH_MIHAL_MFNR";
        break;
    case MI_ALGO_BOKEH_HDR:
        capInfo.captureTypeName += "MI_ALGO_BOKEH_HDR";
        break;
    case MI_ALGO_BOKEH_SUPERNIGHT:
        capInfo.captureTypeName += "MI_ALGO_BOKEH_SUPERNIGHT";
        break;
    case MI_ALGO_SR_HDR:
        capInfo.captureTypeName += "MI_ALGO_SR_HDR";
        break;
    case MI_ALGO_FUSION:
        capInfo.captureTypeName += "MI_ALGO_FUSION";
        break;
    case MI_ALGO_FUSION_SR:
        capInfo.captureTypeName += "MI_ALGO_FUSION_SR";
        break;
    case MI_ALGO_VENDOR_MFNR_HDR:
        capInfo.captureTypeName += "MI_ALGO_VENDOR_MFNR_HDR";
        break;
    case MI_ALGO_MIHAL_MFNR_HDR:
        capInfo.captureTypeName += "MI_ALGO_MIHAL_MFNR_HDR";
        break;
    default:
        capInfo.captureTypeName += "MI_ALGO_NONE";
        break;
    }
    MLOGD(LogGroupHAL, "[AppSnapshotController] captureTypeName = %s",
          capInfo.captureTypeName.c_str());
}

void MiCamDecision::updateMialgoType()
{
    int32_t penddingCount = 0;
    uint32_t originalType = mAlgoType;
    int32_t penddingVendorHALCount = 0;
    int32_t pendingMFNRCount = 0;

    if (mCurrentCamMode != NULL) {
        penddingCount = mCurrentCamMode->getPendingSnapCount();
        penddingVendorHALCount = mCurrentCamMode->getPendingVendorHALSnapCount();
        pendingMFNRCount = mCurrentCamMode->getPendingVendorMFNR();
    }
    bool lackBuffer = penddingCount > mMaxQuickShotCount;
    bool queueFull = pendingMFNRCount >= mMaxQuickShotCount;

    if (mAlgoType == MI_ALGO_VENDOR_MFNR_HDR) {
        if (lackBuffer || mIsQuickSnapshot) {
            mAlgoType = MI_ALGO_VENDOR_MFNR;
        }
    } else if (mAlgoType == MI_ALGO_MIHAL_MFNR_HDR) {
        if (lackBuffer || mIsQuickSnapshot) {
            mAlgoType = MI_ALGO_MIHAL_MFNR;
        }
    }

    if (mAlgoType == MI_ALGO_VENDOR_MFNR) {
        if (queueFull) {
            mAlgoType = MI_ALGO_NONE;
        }
    }
    if (mAlgoType == MI_ALGO_MIHAL_MFNR) {
        lackBuffer = penddingVendorHALCount > mMaxQuickShotCount;
        if (lackBuffer) {
            mAlgoType = MI_ALGO_NONE;
        }
    }
    MLOGI(LogGroupHAL, "[MiviQuickShot] type %d->%d isQuickSnapshot: %d", originalType, mAlgoType,
          mIsQuickSnapshot);
}

bool MiCamDecision::isRawFormat(const EImageFormat &format)
{
    return format == eImgFmt_BAYER10 || format == eImgFmt_RAW10 || format == eImgFmt_RAW16 ||
           format == eImgFmt_BAYER12_UNPAK;
}

std::string MiCamDecision::getMiCamDecisionName()
{
    return mCaptureTypeName;
}

uint64_t MiCamDecision::getQuickShotDelayTime()
{
    uint64_t delayTime = 0;
    uint32_t baseMs = 100;
    if (mAlgoType == MI_ALGO_BOKEH_MIHAL_MFNR) {
        if (mOperationMode == StreamConfigModeAlgoupDualBokeh) {
            delayTime =
                ((mQuickShotDelayTimeMask & QS_MI_ALGO_BACK_BOKEH_MIHAL_MFNR_DELAY_TIME)) * baseMs;
        } else {
            delayTime =
                ((mQuickShotDelayTimeMask & QS_MI_ALGO_FRONT_BOKEH_MIHAL_MFNR_DELAY_TIME) >> 4) *
                baseMs;
        }
    } else if (mAlgoType == MI_ALGO_MIHAL_MFNR_HDR) {
        if (mCamInfo.roleId == RoleIdFront) {
            if (mIsHeifFormat) {
                delayTime =
                    ((mQuickShotDelayTimeMask & QS_MI_ALGO_FRONT_HDR_HELF_DELAY_TIME) >> 36) *
                    baseMs;
            } else {
                delayTime =
                    ((mQuickShotDelayTimeMask & QS_MI_ALGO_FRONT_HDR_DELAY_TIME) >> 12) * baseMs;
            }
        } else {
            delayTime = ((mQuickShotDelayTimeMask & QS_MI_ALGO_BACK_HDR_DELAY_TIME) >> 16) * baseMs;
        }
    } else if (mAlgoType == MI_ALGO_SR) {
        delayTime =
            ((mQuickShotDelayTimeMask & QS_MI_ALGO_BACK_SR_CAPTURE_DELAY_TIME) >> 24) * baseMs;
    } else if (mAlgoType == MI_ALGO_MIHAL_MFNR) {
        if (mIsAinr) {
            delayTime =
                ((mQuickShotDelayTimeMask & QS_MI_ALGO_BACK_MIHAL_AINR_CAPTURE_DELAY_TIME) >> 32) *
                baseMs;
        } else {
            if (mCamInfo.roleId == RoleIdFront) {
                delayTime =
                    ((mQuickShotDelayTimeMask & QS_MI_ALGO_FRONT_NORMAL_CAPTURE_DELAY_TIME) >> 28) *
                    baseMs;
            } else {
                delayTime =
                    ((mQuickShotDelayTimeMask & QS_MI_ALGO_BACK_MIHAL_MFNR_CAPTURE_DELAY_TIME) >>
                     8) *
                    baseMs;
            }
        }
    } else if (mAlgoType == MI_ALGO_SUPERNIGHT) {
        if (mOperationMode == StreamConfigModeAlgoupNormal && mCamInfo.roleId != RoleIdFront) {
            delayTime =
                ((mQuickShotDelayTimeMask & QS_MI_ALGO_BACK_SUPER_NIGHT_SE_DELAY_TIME) >> 20) *
                baseMs;
        }
    }
    MLOGI(LogGroupHAL, "QuickShot get type %d, delayTime %" PRId64, mAlgoType, delayTime);
    return delayTime;
}

bool MiCamDecision::isSupportQuickShot() const
{
    if (mOperationMode != StreamConfigModeAlgoupNormal &&
        mOperationMode != StreamConfigModeAlgoupDualBokeh &&
        mOperationMode != StreamConfigModeAlgoupSingleBokeh) {
        return false;
    }
    uint32_t mask = mQuickShotSupportedMask;
    bool isSupport = false;
    if (mOperationMode == StreamConfigModeAlgoupDualBokeh) {
        if (mAlgoType == MI_ALGO_BOKEH_HDR)
            isSupport = mask & QS_SUPPORT_BACK_BOKEH_HDR;
        else if (mAlgoType == MI_ALGO_BOKEH_SUPERNIGHT) {
            isSupport = mask & QS_SUPPORT_SAT_SUPER_NIGHT;
        } else {
            isSupport = mask & QS_SUPPORT_BACK_BOKEH_MFNR;
        }
    } else if (mOperationMode == StreamConfigModeAlgoupSingleBokeh) {
        isSupport = mask & QS_SUPPORT_FRONT_BOKEH;
    } else if (mOperationMode == StreamConfigModeAlgoupNormal) {
        if (mAlgoType == MI_ALGO_SR) {
            isSupport = mask & QS_SUPPORT_SAT_SR;
        } else if (mAlgoType == MI_ALGO_VENDOR_MFNR || mAlgoType == MI_ALGO_MIHAL_MFNR) {
            isSupport = mask & QS_SUPPORT_SAT_MFNR;
        } else if (mAlgoType == MI_ALGO_MIHAL_MFNR_HDR) {
            isSupport =
                mask & (mCamInfo.roleId == RoleIdFront ? QS_SUPPORT_FRONT_HDR : QS_SUPPORT_SAT_HDR);
        } else if (mAlgoType == MI_ALGO_SUPERNIGHT) {
            if (mCamInfo.roleId != RoleIdFront) {
                isSupport = mask & QS_SUPPORT_SAT_SUPER_NIGHT;
            }
        } else if (mAlgoType == MI_ALGO_NONE && mIsBurstCapture) {
            isSupport = false;
        } else {
            isSupport = mask & (mCamInfo.roleId == RoleIdFront ? QS_SUPPORT_FRONT_MFNR
                                                               : QS_SUPPORT_SAT_MFNR);
        }
    }
    return isSupport;
}

/******************MiCamDecisionSimple************/
MiCamDecisionSimple::MiCamDecisionSimple(const CreateInfo &info)
    : MiCamDecision(info), mHdrMode(0), mMiviSuperNightMode(0)
{
    MLOGI(LogGroupHAL, "MiCamDecisionSimple ctor +");
    memset(&mHdrData, 0, sizeof(HdrData));
    memset(&mSnData, 0, sizeof(SuperNightData));
    if (mOperationMode == StreamConfigModeMiuiSuperNight) {
        mSnData.superNightEnabled = 1;
        mSupportAlgoMask = MI_ALGO_MIHAL_MFNR + MI_ALGO_SUPERNIGHT;
        mCaptureTypeName = "SuperNight";
    } else if (mOperationMode == StreamConfigModeAlgoupSingleBokeh) {
        mCaptureTypeName = "Bokeh";
    } else {
        mCaptureTypeName = "Simple";
    }
    mAlgoFuncMap[MI_ALGO_HDR] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoHDR(capInfo);
    };
    mAlgoFuncMap[MI_ALGO_VENDOR_MFNR_HDR] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoVendorMfnrHDR(capInfo);
    };
    mAlgoFuncMap[MI_ALGO_MIHAL_MFNR_HDR] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoMihalMfnrHDR(capInfo);
    };
    mAlgoFuncMap[MI_ALGO_SUPERNIGHT] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoSN(capInfo);
    };
    mAlgoFuncMap[MI_ALGO_BOKEH] = [this](MiCapture::CreateInfo &capInfo) {
        MiMetadata::setEntry<uint8_t>(&capInfo.settings, XIAOMI_BOKEH_FEATURE_ENABLED, 1);
        return composeCapInfoSingleNone(capInfo, eImgFmt_BAYER10);
    };

    mAlgoFuncMap[MI_ALGO_BOKEH_VENDOR_MFNR] = [this](MiCapture::CreateInfo &capInfo) {
        MiMetadata::setEntry<uint8_t>(&capInfo.settings, XIAOMI_BOKEH_FEATURE_ENABLED, 1);
        return composeCapInfoSingleNone(capInfo);
    };

    mAlgoFuncMap[MI_ALGO_BOKEH_MIHAL_MFNR] = [this](MiCapture::CreateInfo &capInfo) {
        MiMetadata::setEntry<uint8_t>(&capInfo.settings, XIAOMI_BOKEH_FEATURE_ENABLED, 1);
        return composeCapInfoMihalMfnr(capInfo);
    };
    MLOGI(LogGroupHAL, "MiCamDecisionSimple ctor -");
}

MiCamDecisionSimple::~MiCamDecisionSimple() {}

bool MiCamDecisionSimple::composeCapInfoHDR(MiCapture::CreateInfo &capInfo)
{
    auto stream = getPhyStream(eImgFmt_BAYER10);
    // For app sanpshot control
    if (stream == nullptr) {
        MLOGE(LogGroupHAL, "getPhyStream failed");
        return false;
    }
    capInfo.streams.push_back(stream);
    capInfo.frameCount = mHdrData.hdrChecker.number;
    MLOGI(LogGroupHAL,
          "[HDR]hdrdata number = %d, ev[0] = %d, ev[1] = %d, ev[2] = %d, ev[3] = %d, ev[4] = %d, "
          "ev[5] = %d, ev[6] = %d, ev[7] = %d, scene_type = %d",
          mHdrData.hdrChecker.number, mHdrData.hdrChecker.ev[0], mHdrData.hdrChecker.ev[1],
          mHdrData.hdrChecker.ev[2], mHdrData.hdrChecker.ev[3], mHdrData.hdrChecker.ev[4],
          mHdrData.hdrChecker.ev[5], mHdrData.hdrChecker.ev[6], mHdrData.hdrChecker.ev[7],
          mHdrData.hdrChecker.scene_type);
    // add metadata (private redefined tag)
    if (mHdrData.hdrChecker.scene_flag == HDRTYPE::MODE_LLHDR) {
        MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                     XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_LLHDR);
    } else {
        MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                     XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_HDR);
    }

    MiMetadata::setEntry<int32_t>(&capInfo.settings, XIAOMI_HDR_FEATURE_HDR_SCENE_TYPE,
                                  mHdrData.hdrChecker.scene_type);

    // need set lock AE in MTK
    MiMetadata::setEntry<MUINT8>(&capInfo.settings, MTK_CONTROL_AE_LOCK, MTK_CONTROL_AE_LOCK_ON);

    if (!updateMetaInCapture(capInfo, false, mHdrData.hdrChecker.ev)) {
        return false;
    }
    return true;
}

bool MiCamDecisionSat::composeCapInfoHDSR(MiCapture::CreateInfo &capInfo)
{
    auto stream = getPhyStream(eImgFmt_BAYER10);
    if (stream == nullptr) {
        MLOGE(LogGroupHAL, "getPhyStream failed");
        return false;
    }
    capInfo.streams.push_back(stream);
    mIsAinr = 0; //?

    uint32_t frame_cnt = 0; // get SR frame number from json
    MiCamJsonUtils::getVal("DecisionList.Sat.NumOfSnapshotRequiredBySR", frame_cnt, 8);
    capInfo.frameCount = mHdrData.hdrChecker.number + frame_cnt - 1;

    MiMetadata::setEntry<int32_t>(&capInfo.settings, XIAOMI_HDR_FEATURE_HDR_SCENE_TYPE,
                                  mHdrData.hdrChecker.scene_type);
    MiMetadata::setEntry<MUINT8>(&capInfo.settings, XIAOMI_HDR_SR_FEATURE_ENABLED, 1);
    MiMetadata::setEntry<MUINT8>(&capInfo.settings, XIAOMI_HDR_FEATURE_ENABLED, 0);
    MiMetadata::setEntry<MUINT8>(&capInfo.settings, XIAOMI_SR_ENABLED, 0);

    if (mHdrData.hdrChecker.scene_flag == HDRTYPE::MODE_LLHDR) {
        MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                     XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_LLHDR);
    } else {
        MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                     XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_HDR);
    }

    std::vector<int> preCollectEnable;
    for (int i = 0; i < capInfo.frameCount; ++i) {
        if (i < frame_cnt) {
            preCollectEnable.push_back(0);
        } else {
            preCollectEnable.push_back(1);
        }
    }

    std::string str = "[ ";
    for (auto i : preCollectEnable) {
        str += (std::to_string(i) + ' ');
    }
    str += ']';
    MLOGD(LogGroupHAL, "preCollectEnable: %s, frameCount = %d", str.c_str(), capInfo.frameCount);

    if (preCollectEnable.empty() || preCollectEnable.size() < mHdrData.hdrChecker.number) {
        MLOGE(LogGroupHAL, "preCollectEnable size error");
        return false;
    }

    // enable zsl
    MiMetadata::setEntry<uint8_t>(&capInfo.settings, MTK_CONTROL_ENABLE_ZSL,
                                  MTK_CONTROL_ENABLE_ZSL_TRUE);
    capInfo.isZslEnable = true;

    // add metadata (private redefined tag)
    MRect cropRegion;
    if (MiMetadata::getEntry<MRect>(&capInfo.settings, ANDROID_SCALER_CROP_REGION, cropRegion)) {
        MLOGD(LogGroupHAL, "crop region before conversion: (x:%d, y:%d, w:%d, h:%d)",
              cropRegion.p.x, cropRegion.p.y, cropRegion.s.w, cropRegion.s.h);
    } else {
        cropRegion.p.x = 0;
        cropRegion.p.y = 0;
        cropRegion.s.w = mCamInfo.activeArraySize.width;
        cropRegion.s.h = mCamInfo.activeArraySize.height;
    }
    ConvertZoomRatio(mZoomRatio, cropRegion,
                     {mCamInfo.activeArraySize.width, mCamInfo.activeArraySize.height});

    for (int i = 0; i < capInfo.frameCount; ++i) {
        MiMetadata eachSetting;

        // set precollect enable for EV+/-
        MiMetadata::setEntry<MINT32>(&eachSetting, MTK_CONTROL_CAPTURE_PRECOLLECT_ENABLE,
                                     preCollectEnable[i]);
        // set all input buffer as full size && full FOV
        MiMetadata::setEntry<MFLOAT>(&eachSetting, ANDROID_CONTROL_ZOOM_RATIO, 1.0);
        // set cropregion for SR algo
        MiMetadata::setEntry<MRect>(&eachSetting, XIAOMI_SR_CROP_REOGION_MTK, cropRegion);
        MLOGD(LogGroupHAL, "zoomRatio:%f, crop region after conversion: (x:%d, y:%d, w:%d, h:%d)",
              mZoomRatio, cropRegion.p.x, cropRegion.p.y, cropRegion.s.w, cropRegion.s.h);
        // set total number for hdr+sr
        MiMetadata::setEntry<MINT32>(&eachSetting, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_COUNT,
                                     capInfo.frameCount);
        // set index for each frame
        MiMetadata::setEntry<MINT32>(&eachSetting, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_INDEX, i);

        if (preCollectEnable[i] == 1) {
            MiMetadata::setEntry<uint8_t>(&eachSetting, XIAOMI_MFNR_ENABLED, 0);
            MiMetadata::setEntry<MUINT8>(&eachSetting, MTK_CONTROL_AE_LOCK, MTK_CONTROL_AE_LOCK_ON);
            int index = i - frame_cnt + 1;
            if (index <= 0 || index >= mHdrData.hdrChecker.number) {
                MLOGE(LogGroupHAL, "[HDSR] ev index error! index = %d", index);
                return false;
            }
            if (mHdrData.hdrChecker.scene_flag == HDRTYPE::MODE_LLHDR) {
                MiMetadata::setEntry<MINT32>(
                    &eachSetting, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                    XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_LLHDR);
            } else {
                MiMetadata::setEntry<MINT32>(
                    &eachSetting, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                    XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_HDR);
            }
            MiMetadata::setEntry(&eachSetting, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 0);
            MiMetadata::setEntry<MINT32>(&eachSetting, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                         mHdrData.hdrChecker.ev[index]);
            MLOGD(LogGroupHAL, "[HDSR] ev[%d] = %d", index, mHdrData.hdrChecker.ev[index]);
        } else {
            MiMetadata::setEntry(&eachSetting, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 1);
            MiMetadata::setEntry<MUINT8>(&eachSetting, MTK_CONTROL_AE_LOCK,
                                         MTK_CONTROL_AE_LOCK_OFF);
            MiMetadata::setEntry<MINT32>(&eachSetting, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                         XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_MFSR);
            MiMetadata::setEntry<MINT32>(&eachSetting, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, 0);
            MiMetadata::setEntry<MINT32>(&eachSetting, MTK_CONTROL_CAPTURE_SINGLE_YUV_NR, 0);
        }
        capInfo.settingsPerframe.push_back(std::move(eachSetting));
    }

    return true;
}

bool MiCamDecisionSimple::composeCapInfoVendorMfnrHDR(MiCapture::CreateInfo &capInfo)
{
    auto stream = getPhyStream(eImgFmt_NV21);
    if (stream == nullptr) {
        MLOGE(LogGroupHAL, "getPhyStream failed");
        return false;
    }
    capInfo.streams.push_back(stream);

    mIsAinr = 0;

    capInfo.frameCount = mHdrData.hdrChecker.number;
    MLOGI(LogGroupHAL,
          "[HDR]hdrdata number = %d, ev[0] = %d, ev[1] = %d, ev[2] = %d, ev[3] = %d, ev[4] = %d, "
          "ev[5] = %d, ev[6] = %d, ev[7] = %d, scene_type = %d",
          mHdrData.hdrChecker.number, mHdrData.hdrChecker.ev[0], mHdrData.hdrChecker.ev[1],
          mHdrData.hdrChecker.ev[2], mHdrData.hdrChecker.ev[3], mHdrData.hdrChecker.ev[4],
          mHdrData.hdrChecker.ev[5], mHdrData.hdrChecker.ev[6], mHdrData.hdrChecker.ev[7],
          mHdrData.hdrChecker.scene_type);

    if (mHdrData.hdrChecker.scene_flag == HDRTYPE::MODE_LLHDR) {
        MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                     XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_LLHDR);
    } else {
        MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                     XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_HDR);
    }

    MiMetadata::setEntry<int32_t>(&capInfo.settings, XIAOMI_HDR_FEATURE_HDR_SCENE_TYPE,
                                  mHdrData.hdrChecker.scene_type);

    std::vector<int32_t> preCollectEnable;
    auto entry = mPreviewCaptureResult.entryFor(MTK_CONTROL_CAPTURE_PRECOLLECT_ENABLE);
    for (int i = 0; i < entry.count(); ++i) {
        preCollectEnable.push_back(entry.itemAt(i, Type2Type<MINT32>()));
    }

    std::string str = "[ ";
    for (auto i : preCollectEnable) {
        str += (std::to_string(i) + ' ');
    }
    str += ']';
    MLOGD(LogGroupHAL, "preCollectEnable: %s", str.c_str());

    if (preCollectEnable.empty() || preCollectEnable.size() < mHdrData.hdrChecker.number) {
        MLOGE(LogGroupHAL, "preCollectEnable size error");
        return false;
    }

    if (!updateMetaInCapture(capInfo, true, mHdrData.hdrChecker.ev)) {
        return false;
    }

    for (int i = 0; i < capInfo.frameCount; ++i) {
        MiMetadata::setEntry<MINT32>(&capInfo.settingsPerframe[i],
                                     MTK_CONTROL_CAPTURE_PRECOLLECT_ENABLE, preCollectEnable[i]);
        if (preCollectEnable[i] == 1) {
            MiMetadata::setEntry<uint8_t>(&capInfo.settingsPerframe[i], XIAOMI_MFNR_ENABLED, 0);
            MiMetadata::setEntry<MUINT8>(&capInfo.settings, MTK_CONTROL_AE_LOCK,
                                         MTK_CONTROL_AE_LOCK_ON);
        } else {
            MiMetadata::setEntry<uint8_t>(&capInfo.settingsPerframe[i], XIAOMI_MFNR_ENABLED, 1);
            MiMetadata::setEntry<MUINT8>(&capInfo.settings, MTK_CONTROL_AE_LOCK,
                                         MTK_CONTROL_AE_LOCK_OFF);
        }
    }

    return true;
}

bool MiCamDecisionSimple::composeCapInfoMihalMfnrHDR(MiCapture::CreateInfo &capInfo)
{
    auto stream = getPhyStream(eImgFmt_BAYER10);
    if (stream == nullptr) {
        MLOGE(LogGroupHAL, "getPhyStream failed");
        return false;
    }
    capInfo.streams.push_back(stream);
    mIsAinr = 0;

    // TODO: make a function for initFeatureSetting
    MiMetadata info = mPreviewCaptureResult;
    std::vector<MiMetadata> featureSettings;
    int32_t feature = 0;
    uint32_t frame_cnt = 0;
    feature = MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_MFNR;
    int32_t isoValue = 0;
    int64_t exposureTimeValue = 0;
    if (MiMetadata::getEntry<int32_t>(&capInfo.settings, MTK_SENSOR_SENSITIVITY, isoValue)) {
        MiMetadata::setEntry<int32_t>(&info, MTK_SENSOR_SENSITIVITY, isoValue);
    }
    if (MiMetadata::getEntry<MINT64>(&capInfo.settings, MTK_SENSOR_EXPOSURE_TIME,
                                     exposureTimeValue)) {
        MiMetadata::setEntry<MINT64>(&info, MTK_SENSOR_EXPOSURE_TIME, exposureTimeValue);
    }

    // MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_FEATURE = MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING
    MiMetadata::setEntry<MINT32>(&info, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, feature);
    MiMetadata::setEntry<MUINT8>(&info, XIAOMI_HDR_FEATURE_ENABLED, 1);
    if (!queryFeatureSetting(info, frame_cnt, featureSettings)) {
        MLOGI(LogGroupHAL, "queryFeatureSetting result: %d", frame_cnt);
    } else {
        MiCamJsonUtils::getVal("DecisionList.Simple.MFNRHDRSetting.MFNRNums", frame_cnt, 4);
        MLOGW(LogGroupHAL, "queryFeatureSetting failed! force set mfnr frame cnt = %d", frame_cnt);
    }
    capInfo.frameCount = mHdrData.hdrChecker.number + frame_cnt - 1;

    MiMetadata::setEntry<int32_t>(&capInfo.settings, XIAOMI_HDR_FEATURE_HDR_SCENE_TYPE,
                                  mHdrData.hdrChecker.scene_type);
    MiMetadata::setEntry<MUINT8>(&capInfo.settings, XIAOMI_HDR_FEATURE_ENABLED, 1);

    // if (mHdrData.hdrChecker.scene_flag == HDRTYPE::MODE_LLHDR) {
    //     MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
    //                                  XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_LLHDR);
    // } else {
    //     MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
    //                                  XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_HDR);
    // }

    // TODO: maybe can set in config_request_metadata__.h
    std::vector<int> preCollectEnable;
    for (int i = 0; i < frame_cnt - 1; ++i) {
        preCollectEnable.push_back(0);
    }
    auto entry = mPreviewCaptureResult.entryFor(MTK_CONTROL_CAPTURE_PRECOLLECT_ENABLE);
    for (int i = 0; i < entry.count(); ++i) {
        preCollectEnable.push_back(entry.itemAt(i, Type2Type<MINT32>()));
    }

    std::string str = "[ ";
    for (auto i : preCollectEnable) {
        str += (std::to_string(i) + ' ');
    }
    str += ']';
    MLOGD(LogGroupHAL, "preCollectEnable: %s, frameCount = %d", str.c_str(), capInfo.frameCount);

    if (preCollectEnable.empty() || preCollectEnable.size() < mHdrData.hdrChecker.number) {
        MLOGE(LogGroupHAL, "preCollectEnable size error");
        return false;
    }

    MiMetadata::setEntry<uint8_t>(&capInfo.settings, MTK_CONTROL_ENABLE_ZSL,
                                  MTK_CONTROL_ENABLE_ZSL_TRUE);
    capInfo.isZslEnable = true;

    for (int i = 0; i < capInfo.frameCount; ++i) {
        MiMetadata eachSetting;
        MiMetadata::setEntry<MINT32>(&eachSetting, MTK_CONTROL_CAPTURE_PRECOLLECT_ENABLE,
                                     preCollectEnable[i]);
        if (preCollectEnable[i] == 1) {
            MiMetadata::setEntry<uint8_t>(&eachSetting, XIAOMI_MFNR_ENABLED, 0);
            MiMetadata::setEntry(&eachSetting, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 0);
            MiMetadata::setEntry<MUINT8>(&eachSetting, MTK_CONTROL_AE_LOCK, MTK_CONTROL_AE_LOCK_ON);
            int index = i - frame_cnt + 1;
            if (index <= 0 || index >= mHdrData.hdrChecker.number) {
                MLOGE(LogGroupHAL, "[MFNRHDR] ev index error! index = %d", index);
                return false;
            }
            if (mHdrData.hdrChecker.scene_flag == HDRTYPE::MODE_LLHDR) {
                MiMetadata::setEntry<MINT32>(
                    &eachSetting, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                    XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_LLHDR);
            } else {
                MiMetadata::setEntry<MINT32>(
                    &eachSetting, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                    XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_HDR);
            }
            // MiMetadata::setEntry<MINT32>(&eachSetting,
            //                              MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_COUNT,
            //                              (mHdrData.hdrChecker.number-1));
            // MiMetadata::setEntry<MINT32>(&eachSetting,
            //                              MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_INDEX, (index - 1));
            MiMetadata::setEntry<MINT32>(&eachSetting, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                         mHdrData.hdrChecker.ev[index]);
        } else {
            MiMetadata::setEntry<uint8_t>(&eachSetting, XIAOMI_MFNR_ENABLED, 1);
            MiMetadata::setEntry<MUINT8>(&eachSetting, MTK_CONTROL_AE_LOCK,
                                         MTK_CONTROL_AE_LOCK_OFF);
            MiMetadata::setEntry(&eachSetting, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 1);
            MiMetadata::setEntry<MINT32>(&eachSetting, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                         MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_MFNR);
            MiMetadata::setEntry<MINT32>(&eachSetting, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_COUNT,
                                         frame_cnt);
            MiMetadata::setEntry<MINT32>(&eachSetting, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_INDEX, i);
            MiMetadata::setEntry<MINT32>(&eachSetting, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, 0);
        }
        capInfo.settingsPerframe.push_back(std::move(eachSetting));
    }

    return true;
}

bool MiCamDecisionSimple::composeCapInfoSN(MiCapture::CreateInfo &capInfo)
{
    int isFrontRawProcess = 0;
    MiCamJsonUtils::getVal("DecisionList.SuperNight.isFrontRawProcess", isFrontRawProcess, 0);

    auto stream = ((mCamInfo.roleId != RoleIdFront) || isFrontRawProcess)
                      ? getPhyStream(eImgFmt_RAW16)
                      : getPhyStream(eImgFmt_NV21);
    // For app sanpshot control
    if (stream == nullptr) {
        MLOGE(LogGroupHAL, "getPhyStream failed");
        return false;
    }
    capInfo.streams.push_back(stream);
    capInfo.frameCount = mSnData.superNightChecker.num;
    // add metadata (private redefined tag)
    if (mCamInfo.roleId != RoleIdFront || isFrontRawProcess) {
        int ispTuningHint = XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_SUPERNIGHT;
        auto entry = mPreviewCaptureResult.entryFor(XIAOMI_SUPERNIGHT_AEPLINE_CHECKER);
        if (entry.count() >= 2) {
            MINT32 flag = entry.itemAt(0, Type2Type<MINT32>());
            MINT32 hint_type = entry.itemAt(1, Type2Type<MINT32>());
            if (1 == flag)
                ispTuningHint = hint_type;
        }
        MLOGI(LogGroupHAL, "[SN]ispTuningHint = %d", ispTuningHint);
        MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                     ispTuningHint);
    }
    MLOGI(LogGroupHAL,
          "[SN]SuperNightChecker: number = %d, ev[0] = %d, ev[1] = %d, ev[2] = %d, ev[3] = %d, "
          "ev[4] = %d, ev[5] = %d, ev[6] = %d, ev[7] = %d. sumExposureTime:%d",
          mSnData.superNightChecker.num, mSnData.superNightChecker.ev[0],
          mSnData.superNightChecker.ev[1], mSnData.superNightChecker.ev[2],
          mSnData.superNightChecker.ev[3], mSnData.superNightChecker.ev[4],
          mSnData.superNightChecker.ev[5], mSnData.superNightChecker.ev[6],
          mSnData.superNightChecker.ev[7], mSnData.superNightChecker.sumExposureTime);

    // need set lock AE in MTK
    MiMetadata::setEntry<MUINT8>(&capInfo.settings, MTK_CONTROL_AE_LOCK, MTK_CONTROL_AE_LOCK_ON);

    // BAYER12_UNPACK for sn，BAYER14_UNPACK for ell
    MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_PROCESS_RAW_ENABLE, 1);
    MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_RAW_BPP, 12);

    if (!updateMetaInCapture(capInfo, false, mSnData.superNightChecker.ev)) {
        return false;
    }
    return true;
}

void MiCamDecisionSimple::determineAlgoType()
{
    mHdrData.hdrEnabled = canDoHdr();
    MLOGI(LogGroupHAL, "mSnData.superNightEnabled = %d, mHdrData.hdrEnabled = %d",
          mSnData.superNightEnabled, mHdrData.hdrEnabled);
    if (mSnData.superNightEnabled && (mSupportAlgoMask & MI_ALGO_SUPERNIGHT)) {
        mAlgoType = MI_ALGO_SUPERNIGHT;
    } else if (mHdrData.hdrEnabled && (mSupportAlgoMask & MI_ALGO_HDR)) {
        mAlgoType = MI_ALGO_HDR + getHdrType();
    } else if (mSupportAlgoMask & MI_ALGO_MIHAL_MFNR) {
        mAlgoType = MI_ALGO_MIHAL_MFNR;
    } else if (mSupportAlgoMask & MI_ALGO_VENDOR_MFNR) {
        mAlgoType = MI_ALGO_VENDOR_MFNR;
    }

    if (mOperationMode == StreamConfigModeAlgoupSingleBokeh) {
        if (mSupportAlgoMask & MI_ALGO_MIHAL_MFNR) {
            mAlgoType = MI_ALGO_BOKEH_MIHAL_MFNR;
        } else if (mSupportAlgoMask & MI_ALGO_VENDOR_MFNR) {
            mAlgoType = MI_ALGO_BOKEH_VENDOR_MFNR;
        } else {
            mAlgoType = MI_ALGO_BOKEH;
        }
    }

    if (isSupportQuickShot())
        updateMialgoType(); // for quickshot
}

bool MiCamDecisionSimple::canDoHdr()
{
    if (!mHdrMode) {
        MLOGD(LogGroupHAL, "[HDR]: user set hdr off, will not do hdr!");
        return false;
    }

    if (mAdjustEv != 0) {
        MLOGD(LogGroupHAL, "[HDR]: user set adjust ev, will not do hdr!!");
        return false;
    }

    // NOTE: Hdr checker didn't detect hdr scenes
    if (!mHdrData.hdrDetected) {
        MLOGD(LogGroupHAL, "[HDR]: hdr checker didn't detect hdr scenes, will not do hdr!!");
        return false;
    }

    if (mFlashMode == ANDROID_FLASH_MODE_TORCH) {
        MLOGW(LogGroupHAL, "[HDR]: user set flash light on, will not do hdr!!");
        return false;
    } else if (mFlashMode == ANDROID_FLASH_MODE_SINGLE) {
        if (mFlashState >= ANDROID_FLASH_STATE_FIRED) {
            MLOGW(LogGroupHAL,
                  "[HDR]: user set flash light auto and flash light is fired, will not do hdr!!");
            return false;
        }
    }

    MLOGI(LogGroupHAL, "[HDR]this snapshot will do hdr!");
    return true;
}

void MiCamDecisionSimple::updateSettingsOverride(const MiMetadata &settings)
{
    updateHdrSetting(settings);
    updateSuperNightSetting(settings);
}

void MiCamDecisionSimple::updateStatusOverride(const MiMetadata &status)
{
    updateHdrStatus(status);
    updateSuperNightStatus(status);
}

void MiCamDecisionSimple::updateHdrSetting(const MiMetadata &settings)
{
    // NOTE: read hdr checker status(=mode) (XIAOMI_HDR_FEATURE_HDR_CHECKER_STATUS) off:0 on:1
    // auto:2
    // TODO: maybe tag xiaomi.hdr.hdrMode,but havenot it in MTK now
    if (MiMetadata::getEntry<uint8_t>(&settings, XIAOMI_HDR_FEATURE_HDR_CHECKER_STATUS, mHdrMode)) {
        MLOGD(LogGroupHAL, "HDR Mode: %d", mHdrMode);
        if (!mHdrMode) {
            mHdrData.hdrDetected = 0;
        }
    } else {
        MLOGD(LogGroupHAL, "couldn't find tag: XIAOMI_HDR_FEATURE_HDR_CHECKER_STATUS");
    }
}

void MiCamDecisionSimple::updateSuperNightSetting(const MiMetadata &settings)
{
    if (MiMetadata::getEntry<MUINT8>(&settings, XIAOMI_MIVI_SUPER_NIGHT_MODE,
                                     mMiviSuperNightMode)) {
        MLOGD(LogGroupHAL, "miviSuperNightMode: %d", mMiviSuperNightMode);
    } else {
        mMiviSuperNightMode = MIVISuperNightNone;
        MLOGD(LogGroupHAL, "couldn't find tag: XIAOMI_MIVI_SUPER_NIGHT_MODE");
    }
}

void MiCamDecisionSimple::updateHdrStatus(const MiMetadata &status)
{
    if (mSupportAlgoMask & MI_ALGO_HDR) {
        // NOTE: read hdr checker detected (XIAOMI_HDR_FEATURE_HDR_DETECTED)
        if (MiMetadata::getEntry<uint8_t>(&status, XIAOMI_HDR_FEATURE_HDR_DETECTED,
                                          mHdrData.hdrDetected)) {
            MLOGD(LogGroupHAL, "hdrDetected: %d", mHdrData.hdrDetected);
        }

        // NOTE: read hdr checker result (XIAOMI_HDR_FEATURE_HDRCHECKER)
        if (getEntryForMemory<HdrChecker>(&status, XIAOMI_HDR_FEATURE_HDRCHECKER,
                                          mHdrData.hdrChecker, "HDRCHECKER")) {
            MLOGD(LogGroupHAL,
                  "HDRCHECKER number = %d, ev[0] = %d, ev[1] = %d, ev[2] = %d, ev[3] = %d, ev[4] = "
                  "%d, "
                  "ev[5] = %d, ev[6] = %d, ev[7] = %d, scene_type = %d",
                  mHdrData.hdrChecker.number, mHdrData.hdrChecker.ev[0], mHdrData.hdrChecker.ev[1],
                  mHdrData.hdrChecker.ev[2], mHdrData.hdrChecker.ev[3], mHdrData.hdrChecker.ev[4],
                  mHdrData.hdrChecker.ev[5], mHdrData.hdrChecker.ev[6], mHdrData.hdrChecker.ev[7],
                  mHdrData.hdrChecker.scene_type);
        }
        AppSnapshotController::getInstance()->UpdateHdrStatus(mHdrData.hdrDetected,
                                                              mHdrData.hdrChecker.number);
        // NOTE: read hdr checker scene_type (XIAOMI_HDR_FEATURE_HDR_SCENE_TYPE)
        // there is same thing in hdr checker result,so can skip it
        /*
        if (MiMetadata::getEntry<int32_t>(&status, XIAOMI_HDR_FEATURE_HDR_SCENE_TYPE,
                                          mHdrData.hdrChecker.scene_type)) {
            MLOGI(LogGroupHAL, "hdrDetected: %d", mHdrData.hdrChecker.scene_type);
        }
        */
    }
}

void MiCamDecisionSimple::updateSuperNightStatus(const MiMetadata &status)
{
    if (mSupportAlgoMask & MI_ALGO_SUPERNIGHT) {
        if (getEntryForMemory<SuperNightChecker>(&status, XIAOMI_SUPERNIGHT_CHECKER,
                                                 mSnData.superNightChecker, "SUPERNIGHT_CHECKER")) {
            MLOGD(LogGroupHAL,
                  "SuperNightChecker: number = %d, ev[0] = %d, ev[1] = %d, ev[2] = %d, ev[3] = %d, "
                  "ev[4] = %d, ev[5] = %d, ev[6] = %d, ev[7] = %d. sumExposureTime:%d",
                  mSnData.superNightChecker.num, mSnData.superNightChecker.ev[0],
                  mSnData.superNightChecker.ev[1], mSnData.superNightChecker.ev[2],
                  mSnData.superNightChecker.ev[3], mSnData.superNightChecker.ev[4],
                  mSnData.superNightChecker.ev[5], mSnData.superNightChecker.ev[6],
                  mSnData.superNightChecker.ev[7], mSnData.superNightChecker.sumExposureTime);
        }

        if (getEntryForMemory<AiMisdScene>(&status, XIAOMI_AI_SUPERNIGHTSETRIGGER,
                                           mSnData.seTrigger, "SUPERNIGHTSETRIGGER")) {
            MLOGD(LogGroupHAL, "seTrigger: type = %d, value = %d", mSnData.seTrigger.type,
                  mSnData.seTrigger.value);
            uint8_t isSupportUWSuperNightSE = 0;
            MiCamJsonUtils::getVal("DecisionList.Sat.isSupportUWSuperNightSE",
                                   isSupportUWSuperNightSE, 0);
            uint8_t isSupportSEBokeh = 0;
            MiCamJsonUtils::getVal("DecisionList.Bokeh.isSupportSEBokeh", isSupportSEBokeh, 0);
            bool enableSE = mCamInfo.roleId == RoleIdRearWide ||
                            (isSupportUWSuperNightSE && mCamInfo.roleId == RoleIdRearUltra) ||
                            (isSupportSEBokeh && mCamInfo.roleId == RoleIdRearBokeh);
            if (mSnData.seTrigger.type == 3 && enableSE) {
                mSnData.superNightEnabled = mSnData.seTrigger.value == 0x100 ? 1 : 0;
            }
        }

        uint8_t frontSnMode = 0;
        if (MiMetadata::getEntry<uint8_t>(&status, XIAOMI_SUPERNIGHT_CAPTUREMODE, frontSnMode)) {
            MLOGD(LogGroupHAL, "frontSnMode: %d", frontSnMode);
            if ((mOperationMode == StreamConfigModeMiuiSuperNight) &&
                (mCamInfo.roleId == RoleIdFront)) {
                mSnData.superNightEnabled = frontSnMode;
            }
        }

        // user set sn settings
        if (MIVISuperNightManuallyOff == mMiviSuperNightMode) {
            mSnData.superNightEnabled = 0;
        }

        if (getEntryForMemory<AiMisdScene>(&status, XIAOMI_AI_STATESCENE, mSnData.onTripod,
                                           "STATESCENE")) {
            MLOGD(LogGroupHAL, "onTripod: type = %d, value = %d", mSnData.onTripod.type,
                  mSnData.onTripod.value);
        }

        if (getEntryForMemory<SuperNightExif>(&status, XIAOMI_AI_SUPERNIGHTEXIF, mSnData.SNExif,
                                              "SUPERNIGHTEXIF")) {
            MLOGD(LogGroupHAL, "SNExif: lux_index %f, light %f ratio=[%f,%f,%f] lux_threshold %f",
                  mSnData.SNExif.lux_index, mSnData.SNExif.light, mSnData.SNExif.bright_ratio,
                  mSnData.SNExif.middle_ratio, mSnData.SNExif.dark_ratio,
                  mSnData.SNExif.lux_threshold);
        }
        AppSnapshotController::getInstance()->UpdateSEStatus(mSnData.seTrigger.value,
                                                             mSnData.superNightChecker.num);
        MLOGD(LogGroupHAL, "mSnData.superNightEnabled = %d", mSnData.superNightEnabled);
    }
}

bool MiCamDecisionSimple::checkBeauty(const MiMetadata *pAppInMeta)
{
    int beautyFeatureInfo[7] = {
        XIAOMI_BEAUTY_FEATURE_SKIN_SMOOTH_RATIO, XIAOMI_BEAUTY_FEATURE_ENLARGE_EYE_RATIO,
        XIAOMI_BEAUTY_FEATURE_SLIM_FACE_RATIO,   XIAOMI_BEAUTY_FEATURE_NOSE_RATIO,
        XIAOMI_BEAUTY_FEATURE_CHIN_RATIO,        XIAOMI_BEAUTY_FEATURE_LIPS_RATIO,
        XIAOMI_BEAUTY_FEATURE_HAIR_LINE_RATIO};
    int beautyFeatureCounts = sizeof(beautyFeatureInfo) / sizeof(beautyFeatureInfo[0]);
    MINT32 BeautyFeatureEnable = 0;

    if (pAppInMeta) {
        for (int i = 0; i < beautyFeatureCounts; i++) {
            MiMetadata::getEntry<MINT32>(pAppInMeta, beautyFeatureInfo[i], BeautyFeatureEnable);
            if (BeautyFeatureEnable != 0) {
                break;
            }
        }
    }
    return BeautyFeatureEnable != 0;
}

int MiCamDecisionSimple::getHdrType()
{
    int result = 0;
    int hdrType = 0;
    MiCamJsonUtils::getVal("DecisionList.Simple.HdrType", hdrType, 0);
    auto entry = mPreviewCaptureResult.entryFor(MTK_CONTROL_CAPTURE_PRECOLLECT_ENABLE);
    // TODO: maybe should use && there,
    // after set right MTK_CONTROL_CAPTURE_PRECOLLECT_ENABLE in raw MFNRHDR
    if (entry.isEmpty())
        result = NOMAL_HDR;
    else if (entry.count() == 4 && hdrType == VENDOR_MFNR_HDR)
        result = VENDOR_MFNR_HDR;
    else if ( // entry.count() == 7 &&
        hdrType == MIHAL_MFNR_HDR)
        result = MIHAL_MFNR_HDR;

    return result;
}

/******************MiCamDecisionBokeh************/
MiCamDecisionBokeh::MiCamDecisionBokeh(const CreateInfo &info) : MiCamDecisionSimple(info)
{
    MLOGI(LogGroupHAL, "ctor +");
    memset(&mBokehInfo, 0, sizeof(BokehInfo));
    mCaptureTypeName = "Bokeh";

    mAlgoFuncMap[MI_ALGO_BOKEH] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoDualBokeh(capInfo);
    };
    mAlgoFuncMap[MI_ALGO_BOKEH_VENDOR_MFNR] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoDualBokeh(capInfo);
    };
    mAlgoFuncMap[MI_ALGO_BOKEH_MIHAL_MFNR] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoMFNRDualBokeh(capInfo);
    };
    mAlgoFuncMap[MI_ALGO_BOKEH_SUPERNIGHT] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoSEDualBokeh(capInfo);
    };
    mAlgoFuncMap[MI_ALGO_BOKEH_HDR] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoMihalMfnrHDRDualBokeh(capInfo);
    };
    MLOGI(LogGroupHAL, "ctor -");
}

bool MiCamDecisionBokeh::getBokehCamId()
{
    if (!MiMetadata::getEntry<uint8_t>(&mCamInfo.staticInfo, XIAOMI_CAMERA_BOKEHINFO_MASTERCAMERAID,
                                       mBokehInfo.masterCamId)) {
        MLOGE(LogGroupHAL, "couldn't find tag: XIAOMI_CAMERA_BOKEHINFO_MASTERCAMERAID");
        return false;
    }

    if (!MiMetadata::getEntry<uint8_t>(&mCamInfo.staticInfo, XIAOMI_CAMERA_BOKEHINFO_SLAVECAMERAID,
                                       mBokehInfo.slaveCamId)) {
        MLOGE(LogGroupHAL, "couldn't find tag: XIAOMI_CAMERA_BOKEHINFO_SLAVECAMERAID");
        return false;
    }

    auto moduleInfoEntry = mCamInfo.staticInfo.entryFor(XIAOMI_CAMERA_BOKEHINFO_MODULEINFO);
    MLOGI(LogGroupHAL, "moduleInfoEntry size = %d", moduleInfoEntry.count());

    return true;
}

bool MiCamDecisionBokeh::getBokehCamOptSize(float ratio)
{
    auto sizeEntry = mCamInfo.staticInfo.entryFor(XIAOMI_CAMERA_BOKEHINFO_MASTEROPTSIZE);
    if (sizeEntry.count() > 0) {
        MLOGD(LogGroupCore, "sizeEntry.count: %d", sizeEntry.count());
        const auto *array = static_cast<const int32_t *>(sizeEntry.data()); // w, h

        for (int i = 0; i < sizeEntry.count(); i += 2) {
            MiSize imgSize = {
                .width = static_cast<uint32_t>(array[i]),
                .height = static_cast<uint32_t>(array[i + 1]),
            };
            if (imgSize.isSameRatio(ratio)) {
                mBokehInfo.masterSize = imgSize;
            }
        }
    } else {
        MLOGE(LogGroupHAL, "XIAOMI_CAMERA_BOKEHINFO_MASTEROPTSIZE entry count error");
        return false;
    }

    sizeEntry = mCamInfo.staticInfo.entryFor(XIAOMI_CAMERA_BOKEHINFO_SLAVEOPTSIZE);
    if (sizeEntry.count() > 0) {
        MLOGD(LogGroupCore, "sizeEntry.count: %d", sizeEntry.count());
        const auto *array = static_cast<const int32_t *>(sizeEntry.data()); // w, h

        for (int i = 0; i < sizeEntry.count(); i += 2) {
            MiSize imgSize = {
                .width = static_cast<uint32_t>(array[i]),
                .height = static_cast<uint32_t>(array[i + 1]),
            };
            if (imgSize.isSameRatio(ratio)) {
                mBokehInfo.slaveSize = imgSize;
            }
        }
    } else {
        MLOGE(LogGroupHAL, "XIAOMI_CAMERA_BOKEHINFO_SLAVEOPTSIZE entry count error");
        return false;
    }

    return true;
}

bool MiCamDecisionBokeh::buildMiConfigOverride()
{
    if (getBokehCamId()) {
        MLOGI(LogGroupHAL, "[BOKEH]BokehInfo: masterCamId = %d, slaveCamId = %d",
              mBokehInfo.masterCamId, mBokehInfo.slaveCamId);
    } else {
        MLOGE(LogGroupHAL, "[BOKEH]getBokehCamId failed");
        return false;
    }
    // create yuv and raw snapshot stream for bokeh
    StreamParameter para = {
        .cameraId = mBokehInfo.masterCamId,
        .isPhysicalStream = true,
        .format = eImgFmt_NV21,
    };
    bool ret = true;
    ret = getBokehCamOptSize(mFrameRatio);
    MLOGI(LogGroupHAL,
          "[BOKEH]BokehInfo: masterCamId = %d, masterSize = %d*%d, slaveCamId = %d, slaveSize "
          "= %d*%d",
          mBokehInfo.masterCamId, mBokehInfo.masterSize.width, mBokehInfo.masterSize.height,
          mBokehInfo.slaveCamId, mBokehInfo.slaveSize.width, mBokehInfo.slaveSize.height);
    if (!ret) {
        MLOGE(LogGroupHAL, "[BOKEH]getBokehCamOptSize failed");
        return false;
    }
    mMasterStream =
        createSnapshotMiStream(para, mBokehInfo.masterSize.width, mBokehInfo.masterSize.height);

    para.cameraId = mBokehInfo.slaveCamId;
    mSlaveStream =
        createSnapshotMiStream(para, mBokehInfo.slaveSize.width, mBokehInfo.slaveSize.height);

    // se
    para.format = eImgFmt_RAW16;
    para.cameraId = mBokehInfo.masterCamId;
    ret = getBokehCamOptSize(1.33);
    MLOGI(LogGroupHAL,
          "[BOKEH]BokehInfo SE: masterCamId = %d, masterSize = %d*%d, slaveCamId = %d, slaveSize "
          "= %d*%d",
          mBokehInfo.masterCamId, mBokehInfo.masterSize.width, mBokehInfo.masterSize.height,
          mBokehInfo.slaveCamId, mBokehInfo.slaveSize.width, mBokehInfo.slaveSize.height);
    if (!ret) {
        MLOGE(LogGroupHAL, "[BOKEH] SE getBokehCamOptSize failed");
        return false;
    }
    mMasterSeRawStream =
        createSnapshotMiStream(para, mBokehInfo.masterSize.width, mBokehInfo.masterSize.height);

    para.cameraId = mBokehInfo.slaveCamId;
    mSlaveSeRawStream =
        createSnapshotMiStream(para, mBokehInfo.slaveSize.width, mBokehInfo.slaveSize.height);

    // miHalMfnr
    para.format = eImgFmt_BAYER10;
    para.cameraId = mBokehInfo.masterCamId;
    ret = getBokehCamOptSize(1.33);
    MLOGI(LogGroupHAL,
          "[BOKEH]BokehInfo: masterCamId = %d, masterSize = %d*%d, slaveCamId = %d, slaveSize "
          "= %d*%d",
          mBokehInfo.masterCamId, mBokehInfo.masterSize.width, mBokehInfo.masterSize.height,
          mBokehInfo.slaveCamId, mBokehInfo.slaveSize.width, mBokehInfo.slaveSize.height);
    if (!ret) {
        MLOGE(LogGroupHAL, "[BOKEH]getBokehCamOptSize failed");
        return false;
    }
    mMasterRawStream =
        createSnapshotMiStream(para, mBokehInfo.masterSize.width, mBokehInfo.masterSize.height);

    para.cameraId = mBokehInfo.slaveCamId;
    mSlaveRawStream =
        createSnapshotMiStream(para, mBokehInfo.slaveSize.width, mBokehInfo.slaveSize.height);

    if (mMasterStream == nullptr || mSlaveStream == nullptr || mMasterRawStream == nullptr ||
        mSlaveRawStream == nullptr || mMasterSeRawStream == nullptr ||
        mSlaveSeRawStream == nullptr) {
        MLOGE(LogGroupHAL, "createSnapshotMiStream failed!");
        return false;
    }
    return true;
}

bool MiCamDecisionBokeh::composeCapInfoDualBokeh(MiCapture::CreateInfo &capInfo)
{
    // yuv now
    capInfo.streams.push_back(mMasterStream);
    capInfo.streams.push_back(mSlaveStream);
    MiMetadata::setEntry<uint8_t>(&capInfo.settings, XIAOMI_MFNR_ENABLED,
                                  (mAlgoType & MI_ALGO_VENDOR_MFNR));
    MiMetadata::setEntry<uint8_t>(&capInfo.settings, MTK_CONTROL_ENABLE_ZSL,
                                  MTK_CONTROL_ENABLE_ZSL_TRUE);

    // initialize physical settings
    MiMetadata phySetting;
    capInfo.physicalSettings.emplace(mBokehInfo.masterCamId, phySetting);
    capInfo.physicalSettings.emplace(mBokehInfo.slaveCamId, phySetting);

    return true;
}

bool MiCamDecisionBokeh::composeCapInfoMFNRDualBokeh(MiCapture::CreateInfo &capInfo)
{
    std::vector<MiMetadata> featureSettings;
    uint32_t frame_cnt = 0;
    MiMetadata::setEntry(&capInfo.settings, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 1);

    // force to mfnr
    MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_FEATURE,
                                 MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_MFNR);

    if (!queryFeatureSetting(capInfo.settings, frame_cnt, featureSettings)) {
        capInfo.frameCount = frame_cnt;
        MiMetadata::setEntry<uint8_t>(&capInfo.settings, XIAOMI_MFNR_ENABLED, 1);
        MLOGI(LogGroupHAL, "queryFeatureSetting result: %d", frame_cnt);
    } else {
        MLOGE(LogGroupHAL, "queryFeatureSetting failed");
        return false;
    }
    MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_COUNT,
                                 capInfo.frameCount);
    MiMetadata::setEntry<uint8_t>(&capInfo.settings, MTK_CONTROL_ENABLE_ZSL,
                                  MTK_CONTROL_ENABLE_ZSL_TRUE);
    capInfo.streams.push_back(mMasterRawStream);
    capInfo.streams.push_back(mSlaveRawStream);

    // initialize physical settings
    MiMetadata phySetting;
    MiMetadata::setEntry<MINT32>(&phySetting, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 1);
    capInfo.physicalSettings.emplace(mBokehInfo.masterCamId, phySetting);
    // physettings will overwrite common settings
    MiMetadata::setEntry<MINT32>(&phySetting, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 0);
    capInfo.physicalSettings.emplace(mBokehInfo.slaveCamId, phySetting);

    for (size_t i = 0; i < capInfo.frameCount; ++i) {
        // append featureSettings
        MiMetadata eachSetting = featureSettings[i];
        MiMetadata::setEntry(&eachSetting, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_INDEX,
                             static_cast<int32_t>(i));
        capInfo.settingsPerframe.push_back(std::move(eachSetting));
    }
    return true;
}

bool MiCamDecisionBokeh::composeCapInfoSEDualBokeh(MiCapture::CreateInfo &capInfo)
{
    capInfo.streams.push_back(mMasterSeRawStream);
    capInfo.streams.push_back(mSlaveSeRawStream);
    // bypass doBss left
    MiMetadata::setEntry(&capInfo.settings, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 0);
    capInfo.frameCount = mSnData.superNightChecker.num;
    // add metadata (private redefined tag)

    int ispTuningHint = XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_SUPERNIGHT;
    if (mCamInfo.roleId != RoleIdFront) {
        auto entry = mPreviewCaptureResult.entryFor(XIAOMI_SUPERNIGHT_AEPLINE_CHECKER);
        if (entry.count() >= 2) {
            MINT32 flag = entry.itemAt(0, Type2Type<MINT32>());
            MINT32 hint_type = entry.itemAt(1, Type2Type<MINT32>());
            if (1 == flag)
                ispTuningHint = hint_type;
        }
        MLOGI(LogGroupHAL, "[SN]ispTuningHint = %d", ispTuningHint);
        // MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
        //                              ispTuningHint);
    }
    MLOGI(
        LogGroupHAL,
        "[SEBOKEH]SuperNightChecker: number = %d, ev[0] = %d, ev[1] = %d, ev[2] = %d, ev[3] = %d, "
        "ev[4] = %d, ev[5] = %d, ev[6] = %d, ev[7] = %d. sumExposureTime:%d",
        mSnData.superNightChecker.num, mSnData.superNightChecker.ev[0],
        mSnData.superNightChecker.ev[1], mSnData.superNightChecker.ev[2],
        mSnData.superNightChecker.ev[3], mSnData.superNightChecker.ev[4],
        mSnData.superNightChecker.ev[5], mSnData.superNightChecker.ev[6],
        mSnData.superNightChecker.ev[7], mSnData.superNightChecker.sumExposureTime);

    // need set lock AE in MTK
    MiMetadata::setEntry<MUINT8>(&capInfo.settings, MTK_CONTROL_AE_LOCK, MTK_CONTROL_AE_LOCK_ON);

    MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_MFNR_FEATURE_BSS_GOLDEN_INDEX, 3);

    MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_PROCESS_RAW_ENABLE, 1);
    MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_RAW_BPP, 12);

    if (!updateMetaInCapture(capInfo, false, mSnData.superNightChecker.ev)) {
        return false;
    }

    MiMetadata phySetting;
    MiMetadata::setEntry<MINT32>(&phySetting, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                 ispTuningHint);
    capInfo.physicalSettings.emplace(mBokehInfo.masterCamId, phySetting);
    capInfo.physicalSettings.emplace(mBokehInfo.slaveCamId, phySetting);

    return true;
}

bool MiCamDecisionBokeh::composeCapInfoMihalMfnrHDRDualBokeh(MiCapture::CreateInfo &capInfo)
{
    mIsAinr = 0;
    capInfo.streams.push_back(mMasterRawStream);
    capInfo.streams.push_back(mSlaveRawStream);

    MLOGI(LogGroupHAL,
          "[Bokeh]hdrdata number = %d, ev[0] = %d, ev[1] = %d, ev[2] = %d, ev[3] = %d, ev[4] = %d, "
          "ev[5] = %d, ev[6] = %d, ev[7] = %d, scene_type = %d",
          mHdrData.hdrChecker.number, mHdrData.hdrChecker.ev[0], mHdrData.hdrChecker.ev[1],
          mHdrData.hdrChecker.ev[2], mHdrData.hdrChecker.ev[3], mHdrData.hdrChecker.ev[4],
          mHdrData.hdrChecker.ev[5], mHdrData.hdrChecker.ev[6], mHdrData.hdrChecker.ev[7],
          mHdrData.hdrChecker.scene_type);

    // TODO: make a function for initFeatureSetting
    MiMetadata info = mPreviewCaptureResult;
    std::vector<MiMetadata> featureSettings;
    int32_t feature = 0;
    uint32_t frame_cnt = 0;
    feature = MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_MFNR;
    int32_t isoValue = 0;
    int64_t exposureTimeValue = 0;
    if (MiMetadata::getEntry<int32_t>(&capInfo.settings, MTK_SENSOR_SENSITIVITY, isoValue)) {
        MiMetadata::setEntry<int32_t>(&info, MTK_SENSOR_SENSITIVITY, isoValue);
    }
    if (MiMetadata::getEntry<MINT64>(&capInfo.settings, MTK_SENSOR_EXPOSURE_TIME,
                                     exposureTimeValue)) {
        MiMetadata::setEntry<MINT64>(&info, MTK_SENSOR_EXPOSURE_TIME, exposureTimeValue);
    }

    // MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_FEATURE = MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING
    MiMetadata::setEntry<MINT32>(&info, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING, feature);
    MiMetadata::setEntry<MUINT8>(&info, XIAOMI_HDR_FEATURE_ENABLED, 1);
    if (!queryFeatureSetting(info, frame_cnt, featureSettings)) {
        MLOGI(LogGroupHAL, "queryFeatureSetting result: %d", frame_cnt);
    } else {
        MiCamJsonUtils::getVal("DecisionList.Simple.MFNRHDRSetting.MFNRNums", frame_cnt, 4);
        MLOGW(LogGroupHAL, "queryFeatureSetting failed! force set mfnr frame cnt = %d", frame_cnt);
    }

    capInfo.frameCount = mHdrData.hdrChecker.number + frame_cnt - 1;
    if (mContainQuickViewBuffer) {
        if (capInfo.frameCount > 1) {
            capInfo.isZslEnable = true;
        } else {
            mCurrentCamMode->addAnchorFrame(capInfo.fwkRequest.frameNumber,
                                            capInfo.fwkRequest.frameNumber - 1);
        }
    }

    MiMetadata::setEntry<int32_t>(&capInfo.settings, XIAOMI_HDR_FEATURE_HDR_SCENE_TYPE,
                                  mHdrData.hdrChecker.scene_type);
    MiMetadata::setEntry<MUINT8>(&capInfo.settings, XIAOMI_HDR_FEATURE_ENABLED, 1);

    std::vector<int> preCollectEnable;
    for (int i = 0; i < frame_cnt - 1; ++i) {
        preCollectEnable.push_back(0);
    }
    auto entry = mPreviewCaptureResult.entryFor(MTK_CONTROL_CAPTURE_PRECOLLECT_ENABLE);
    for (int i = 0; i < entry.count(); ++i) {
        preCollectEnable.push_back(entry.itemAt(i, Type2Type<MINT32>()));
    }

    std::string str = "[ ";
    for (auto i : preCollectEnable) {
        str += (std::to_string(i) + ' ');
    }
    str += ']';
    MLOGD(LogGroupHAL, "preCollectEnable: %s, frameCount = %d", str.c_str(), capInfo.frameCount);

    if (preCollectEnable.empty() || preCollectEnable.size() < mHdrData.hdrChecker.number) {
        MLOGE(LogGroupHAL, "preCollectEnable size error");
        return false;
    }

    for (int i = 0; i < capInfo.frameCount; ++i) {
        MiMetadata eachSetting;
        MiMetadata::setEntry<MINT32>(&eachSetting, MTK_CONTROL_CAPTURE_PRECOLLECT_ENABLE,
                                     preCollectEnable[i]);

        if (preCollectEnable[i] == 1) {
            MiMetadata::setEntry<uint8_t>(&eachSetting, XIAOMI_MFNR_ENABLED, 0);
            MiMetadata::setEntry(&eachSetting, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 0);
            MiMetadata::setEntry<MUINT8>(&eachSetting, MTK_CONTROL_AE_LOCK, MTK_CONTROL_AE_LOCK_ON);
            int index = i - frame_cnt + 1;
            if (index <= 0 || index >= mHdrData.hdrChecker.number) {
                MLOGE(LogGroupHAL, "[MFNRHDR] ev index error! index = %d", index);
                return false;
            }

            if (mHdrData.hdrChecker.scene_flag == HDRTYPE::MODE_LLHDR) {
                MiMetadata::setEntry<MINT32>(
                    &eachSetting, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                    XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_HDR);
                // XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_LLHDR);
            } else {
                MiMetadata::setEntry<MINT32>(
                    &eachSetting, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                    XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_HDR);
            }

            // MiMetadata::setEntry<MINT32>(&eachSetting,
            //                              MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_COUNT,
            //                              (mHdrData.hdrChecker.number-1));
            // MiMetadata::setEntry<MINT32>(&eachSetting,
            //                              MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_INDEX, (index - 1));
            MiMetadata::setEntry<MINT32>(&eachSetting, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                         mHdrData.hdrChecker.ev[index]);
        } else {
            MiMetadata::setEntry<uint8_t>(&eachSetting, XIAOMI_MFNR_ENABLED, 1);
            MiMetadata::setEntry<MUINT8>(&eachSetting, MTK_CONTROL_AE_LOCK,
                                         MTK_CONTROL_AE_LOCK_OFF);
            MiMetadata::setEntry(&eachSetting, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 1);
            MiMetadata::setEntry<MINT32>(&eachSetting, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                         MTK_POSTPROCDEV_CAPTURE_HINT_MULTIFRAME_MFNR);
            MiMetadata::setEntry<MINT32>(&eachSetting, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_COUNT,
                                         frame_cnt);
            MiMetadata::setEntry<MINT32>(&eachSetting, MTK_POSTPROCDEV_CAPTURE_MULTIFRAME_INDEX, i);
            MiMetadata::setEntry<MINT32>(&eachSetting, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, 0);
        }
        capInfo.settingsPerframe.push_back(std::move(eachSetting));
    }

    MiMetadata phySetting;
    capInfo.physicalSettings.emplace(mBokehInfo.masterCamId, phySetting);
    // physettings will overwrite common settings
    MiMetadata::setEntry<MINT32>(&phySetting, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 0);
    capInfo.physicalSettings.emplace(mBokehInfo.slaveCamId, phySetting);

    return true;
}

void MiCamDecisionBokeh::determineAlgoType()
{
    uint8_t isSupportHDRBokeh = 0;
    MiCamJsonUtils::getVal("DecisionList.Bokeh.isSupportHDRBokeh", isSupportHDRBokeh, 0);
    mHdrData.hdrEnabled = mHdrData.hdrDetected &&
                          (mOperationMode == StreamConfigModeAlgoupDualBokeh) && isSupportHDRBokeh;
    MLOGI(LogGroupHAL, "mSnData.superNightEnabled = %d, mHdrData.hdrEnabled = %d",
          mSnData.superNightEnabled, mHdrData.hdrEnabled);
    if (mSnData.superNightEnabled && (mSupportAlgoMask & MI_ALGO_SUPERNIGHT)) {
        mAlgoType = MI_ALGO_BOKEH_SUPERNIGHT;
    } else if (mHdrData.hdrEnabled && (mSupportAlgoMask & MI_ALGO_HDR)) {
        mAlgoType = MI_ALGO_BOKEH_HDR;
    } else if (mSupportAlgoMask & MI_ALGO_MIHAL_MFNR) {
        mAlgoType = MI_ALGO_BOKEH_MIHAL_MFNR;
    } else if (mSupportAlgoMask & MI_ALGO_VENDOR_MFNR) {
        mAlgoType = MI_ALGO_BOKEH_VENDOR_MFNR;
    } else {
        mAlgoType = MI_ALGO_BOKEH;
    }
}

/******************MiCamDecisionSat************/
MiCamDecisionSat::MiCamDecisionSat(const CreateInfo &info)
    : MiCamDecisionSimple(info), mZoomRatio(0), mIsZoom1xPlus(false), mIsSrNeeded(false)
{
    MLOGI(LogGroupHAL, "ctor +");
    memset(&mFusionInfo, 0, sizeof(FusionInfo));
    if (mOperationMode == StreamConfigModeMiuiSuperNight) {
        mCaptureTypeName = "SuperNight";
    } else {
        mCaptureTypeName = "Sat";
    }
    mAlgoFuncMap[MI_ALGO_SR] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoSR(capInfo);
    };
    mAlgoFuncMap[MI_ALGO_SR_HDR] = [this](MiCapture::CreateInfo &capInfo) {
        return composeCapInfoHDSR(capInfo);
    };
    MLOGI(LogGroupHAL, "ctor -");
}

bool MiCamDecisionSat::composeCapInfoSR(MiCapture::CreateInfo &capInfo)
{
    auto stream = getPhyStream(eImgFmt_BAYER10);
    // For app sanpshot control
    if (stream == nullptr) {
        MLOGE(LogGroupHAL, "getPhyStream failed");
        return false;
    }
    capInfo.streams.push_back(stream);
    uint32_t frame_cnt = 0;
    MiCamJsonUtils::getVal("DecisionList.Sat.NumOfSnapshotRequiredBySR", frame_cnt, 8);
    capInfo.frameCount = frame_cnt;

    // add metadata (private redefined tag)
    MRect cropRegion;
    if (MiMetadata::getEntry<MRect>(&capInfo.settings, ANDROID_SCALER_CROP_REGION, cropRegion)) {
        MLOGD(LogGroupHAL, "crop region before conversion: (x:%d, y:%d, w:%d, h:%d)",
              cropRegion.p.x, cropRegion.p.y, cropRegion.s.w, cropRegion.s.h);
    } else {
        cropRegion.p.x = 0;
        cropRegion.p.y = 0;
        cropRegion.s.w = mCamInfo.activeArraySize.width;
        cropRegion.s.h = mCamInfo.activeArraySize.height;
    }
    ConvertZoomRatio(mZoomRatio, cropRegion,
                     {mCamInfo.activeArraySize.width, mCamInfo.activeArraySize.height});
    MiMetadata::setEntry<MRect>(&capInfo.settings, XIAOMI_SR_CROP_REOGION_MTK, cropRegion);
    MLOGD(LogGroupHAL, "zoomRatio:%f, crop region after conversion: (x:%d, y:%d, w:%d, h:%d)",
          mZoomRatio, cropRegion.p.x, cropRegion.p.y, cropRegion.s.w, cropRegion.s.h);
    MiMetadata::setEntry<MFLOAT>(&capInfo.settings, ANDROID_CONTROL_ZOOM_RATIO, 1.0);
    MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING,
                                 XIAOMI_CONTROL_CAPTURE_HINT_FOR_ISP_TUNING_CUSTOMER_MFSR);
    MiMetadata::setEntry(&capInfo.settings, MTK_MFNR_FEATURE_DO_ZIP_WITH_BSS, 1);
    MiMetadata::setEntry<MINT32>(&capInfo.settings, MTK_CONTROL_CAPTURE_SINGLE_YUV_NR, 0);
    if (!updateMetaInCapture(capInfo, true)) {
        return false;
    }
    return true;
}

auto MiCamDecisionSat::ConvertZoomRatio(const float zoom_ratio, MRect &rect,
                                        const XiaoMi::MiDimension &active_array_dimension) -> void
{
    int32_t left = rect.p.x;
    int32_t top = rect.p.y;
    int32_t width = rect.s.w;
    int32_t height = rect.s.h;

    ConvertZoomRatio(zoom_ratio, left, top, width, height, active_array_dimension);

    rect.p.x = left;
    rect.p.y = top;
    rect.s.w = width;
    rect.s.h = height;
}

auto MiCamDecisionSat::ConvertZoomRatio(const float zoom_ratio, int32_t &left, int32_t &top,
                                        int32_t &width, int32_t &height,
                                        const XiaoMi::MiDimension &active_array_dimension) -> void
{
    if (zoom_ratio <= 0) {
        MLOGE(LogGroupHAL, "invalid params");
        return;
    }

    left = std::round(left / zoom_ratio +
                      0.5f * active_array_dimension.width * (1.0f - 1.0f / zoom_ratio));
    top = std::round(top / zoom_ratio +
                     0.5f * active_array_dimension.height * (1.0f - 1.0f / zoom_ratio));
    width = std::round(width / zoom_ratio);
    height = std::round(height / zoom_ratio);

    if (zoom_ratio >= 1.0f) {
        CorrectRegionBoundary(left, top, width, height, active_array_dimension.width,
                              active_array_dimension.height);
    }
}

auto MiCamDecisionSat::CorrectRegionBoundary(int32_t &left, int32_t &top, int32_t &width,
                                             int32_t &height, const int32_t bound_w,
                                             const int32_t bound_h) -> void
{
    left = std::max(left, 0);
    left = std::min(left, bound_w - 1);

    top = std::max(top, 0);
    top = std::min(top, bound_h - 1);

    width = std::max(width, 1);
    width = std::min(width, bound_w - left);

    height = std::max(height, 1);
    height = std::min(height, bound_h - top);
}

void MiCamDecisionSat::determineAlgoType()
{
    mHdrData.hdrEnabled = canDoHdr();
    if (mIsSrNeeded && mHdrData.hdrEnabled) {
        mHdrData.hdrEnabled = false;
        mIsSrNeeded = false;
        mIsHdsrNeeded = true;
    } else {
        mIsHdsrNeeded = false;
    }

    MLOGI(LogGroupHAL,
          "mSnData.superNightEnabled = %d, mHdrData.hdrEnabled = %d, mIsHdsrNeeded = %d",
          mSnData.superNightEnabled, mHdrData.hdrEnabled, mIsHdsrNeeded);
    if (mIsBurstCapture) {
        mAlgoType = MI_ALGO_NONE;
    } else if (mSnData.superNightEnabled && (mSupportAlgoMask & MI_ALGO_SUPERNIGHT)) {
        mAlgoType = MI_ALGO_SUPERNIGHT;
    } else if (mHdrData.hdrEnabled && (mSupportAlgoMask & MI_ALGO_HDR)) {
        mAlgoType = MI_ALGO_HDR + getHdrType();
    } else if (mIsSrNeeded && (mSupportAlgoMask & MI_ALGO_SR)) {
        mAlgoType = MI_ALGO_SR;
    } else if (mIsHdsrNeeded && (mSupportAlgoMask & MI_ALGO_SR_HDR)) {
        mAlgoType = MI_ALGO_SR_HDR;
    } else if (mSupportAlgoMask & MI_ALGO_MIHAL_MFNR) {
        mAlgoType = MI_ALGO_MIHAL_MFNR;
    } else if (mSupportAlgoMask & MI_ALGO_VENDOR_MFNR) {
        mAlgoType = MI_ALGO_VENDOR_MFNR;
    }

    if (isSupportQuickShot())
        updateMialgoType(); // for quickshot
}

void MiCamDecisionSat::updateMetaInCaptureOverride(MiCapture::CreateInfo &capInfo)
{
    // copy from MIVI2.0
    // NOTE: for Burst Capture, we need to lock AF.
    // WARNING: for this mechanism to work, we should make sure there's no preview request sneaks in
    // when do burst snapshot
    static bool isAFlockTriggered = false;
    if (mIsBurstCapture && !isAFlockTriggered) {
        isAFlockTriggered = true;
        MLOGD(LogGroupHAL, "[BurstCapture][AF]: trigger AF lock to avoid blink");
        uint8_t triggerMode = 1;
        MiMetadata::setEntry<MUINT8>(&capInfo.settings, ANDROID_CONTROL_AF_TRIGGER, triggerMode);
    } else if (!mIsBurstCapture && isAFlockTriggered) {
        isAFlockTriggered = false;
        MLOGD(LogGroupHAL, "[BurstCapture][AF]: burst shot finished, cancel AF lock");
        uint8_t triggerMode = 2;
        MiMetadata::setEntry<MUINT8>(&capInfo.settings, ANDROID_CONTROL_AF_TRIGGER, triggerMode);
    }
}

void MiCamDecisionSat::updateSettingsOverride(const MiMetadata &settings)
{
    updateZoomRatioSetting(settings);
    updateHdrSetting(settings);
    updateSuperNightSetting(settings);
}

void MiCamDecisionSat::updateSettingsInSnapShot(const MiMetadata &settings)
{
    updateBurstSettings(settings);
}

void MiCamDecisionSat::updateZoomRatioSetting(const MiMetadata &settings)
{
    // NOTE: read zoom ratio (ANDROID_CONTROL_ZOOM_RATIO)
    if (MiMetadata::getEntry<float>(&settings, ANDROID_CONTROL_ZOOM_RATIO, mZoomRatio)) {
        MLOGD(LogGroupHAL, "mZoomRatio: %f", mZoomRatio);
        mIsZoom1xPlus = (mZoomRatio - 1.0f) > std::numeric_limits<float>::epsilon();
        if (mCamInfo.roleId == RoleIdRearWide) {
            mIsSrNeeded = mIsZoom1xPlus;
        }
    }
}

void MiCamDecisionSat::updateBurstSettings(const MiMetadata &settings)
{
    int32_t burstCapHint = 0;
    if (MiMetadata::getEntry<int32_t>(&settings, XIAOMI_BURST_CAPTURE_HINT, burstCapHint)) {
        MLOGI(LogGroupHAL, "[BurstCapture]burstCapHint: %d", burstCapHint);
        mIsBurstCapture = burstCapHint;
    } else {
        mIsBurstCapture = false;
        MLOGE(LogGroupHAL, "couldn't find tag: XIAOMI_BURST_CAPTURE_HINT");
    }
}

void MiCamDecisionSat::updateMialgoType()
{
    int32_t penddingCount = 0;
    uint32_t originalType = mAlgoType;
    int32_t penddingVendorHALCount = 0;
    int32_t pendingMFNRCount = 0;

    if (mCurrentCamMode != NULL) {
        penddingCount = mCurrentCamMode->getPendingSnapCount();
        penddingVendorHALCount = mCurrentCamMode->getPendingVendorHALSnapCount();
        pendingMFNRCount = mCurrentCamMode->getPendingVendorMFNR();
    }
    bool lackBuffer = penddingCount > mMaxQuickShotCount;
    bool queueFull = pendingMFNRCount >= mMaxQuickShotCount;

    if (mAlgoType == MI_ALGO_VENDOR_MFNR_HDR) {
        if (lackBuffer || mIsQuickSnapshot) {
            mAlgoType = MI_ALGO_VENDOR_MFNR;
        }
    } else if (mAlgoType == MI_ALGO_MIHAL_MFNR_HDR) {
        if (lackBuffer || mIsQuickSnapshot) {
            mAlgoType = MI_ALGO_MIHAL_MFNR;
        }
    } else if (mAlgoType == MI_ALGO_SR) {
        if (mZoomRatio > 1 && mZoomRatio <= 3.1) {
            if (lackBuffer || mIsQuickSnapshot) {
                if (mSupportAlgoMask & MI_ALGO_MIHAL_MFNR) {
                    mAlgoType = MI_ALGO_MIHAL_MFNR;
                } else if (mSupportAlgoMask & MI_ALGO_VENDOR_MFNR) {
                    mAlgoType = MI_ALGO_VENDOR_MFNR;
                }
            }
        }
    }

    if (mAlgoType == MI_ALGO_VENDOR_MFNR) {
        if (queueFull) {
            mAlgoType = MI_ALGO_NONE;
        }
    }
    if (mAlgoType == MI_ALGO_MIHAL_MFNR) {
        lackBuffer = penddingVendorHALCount > mMaxQuickShotCount;
        if (lackBuffer) {
            mAlgoType = MI_ALGO_NONE;
        }
    }
    MLOGI(LogGroupHAL, "[MiviQuickShot] type %d->%d isQuickSnapshot: %d", originalType, mAlgoType,
          mIsQuickSnapshot);
}

/******************MiCamDecisionSuperHD************/
MiCamDecisionSuperHD::MiCamDecisionSuperHD(const CreateInfo &info) : MiCamDecision(info)
{
    MLOGI(LogGroupHAL, "ctor +");
    memset(&mSuperHdData, 0, sizeof(SuperHdData));
    mCaptureTypeName = "SuperHD";
    mAlgoFuncMap[MI_ALGO_NONE] = [this](MiCapture::CreateInfo &capInfo) {
        // set isZsl = true because remosaic + mfnr green pic problem
        return composeCapInfoSingleNone(capInfo, eImgFmt_NV21, true);
    };
    MLOGI(LogGroupHAL, "ctor -");
}

bool MiCamDecisionSuperHD::getSuperHdData()
{
    if (!MiMetadata::getEntry<uint8_t>(&mCamInfo.staticInfo, XIAOMI_QCFA_SUPPORTED,
                                       mSuperHdData.isQcfaSensor)) {
        MLOGE(LogGroupHAL, "couldn't find tag: XIAOMI_QCFA_SUPPORTED");
        return false;
    }

    auto entry = mCamInfo.staticInfo.entryFor(XIAOMI_SCALER_AVALIABLE_SR_STREAM_CONFIGURATIONS);
    MLOGI(LogGroupHAL, "XIAOMI_SCALER_AVALIABLE_SR_STREAM_CONFIGURATIONS entry.count() = %d",
          entry.count());
    if (entry.count() >= 4) {
        const auto *array = static_cast<const int32_t *>(entry.data()); // format, w, h, dir
        int32_t format = array[0];
        uint32_t width = array[1];
        uint32_t height = array[2];
        int32_t dir = array[3];
        mSuperHdData.superResolution = {width, height};
        MLOGI(LogGroupHAL, "SR_STREAM_CONFIGURATIONS: format = %d, width = %d, height=%d,dir = %d",
              format, width, height, dir);
    } else {
        MLOGE(LogGroupHAL, "couldn't find tag: XIAOMI_SCALER_AVALIABLE_SR_STREAM_CONFIGURATIONS");
        return false;
    }

    return true;
}

bool MiCamDecisionSuperHD::buildMiConfigOverride()
{
    if (!getSuperHdData()) {
        MLOGE(LogGroupHAL, "[SuperHD]getSuperHdData failed");
        return false;
    }

    int factor = 2;
    MiCamJsonUtils::getVal("DecisionList.SuperHD.SuperResolToBinningResolFactor", factor, 2);
    MLOGI(LogGroupHAL, "SuperResolToBinningResolFactor = %d", factor);
    mSuperHdData.binningResolution = {mSuperHdData.superResolution.width / factor,
                                      mSuperHdData.superResolution.height / factor};
    MLOGI(LogGroupHAL,
          "[SuperHD]SuperHdData: isQcfaSensor = %d, superResolution [w,h] = [%d,%d], "
          "binningResolution [w,h] = [%d,%d]",
          mSuperHdData.isQcfaSensor, mSuperHdData.superResolution.width,
          mSuperHdData.superResolution.height, mSuperHdData.binningResolution.width,
          mSuperHdData.binningResolution.height);

    // create qcfa size yuv stream
    StreamParameter para = {
        .isPhysicalStream = false,
        .format = eImgFmt_NV21,
    };
    auto stream = createSnapshotMiStream(para, mSuperHdData.superResolution.width,
                                         mSuperHdData.superResolution.height);
    if (stream == nullptr) {
        MLOGE(LogGroupHAL, "createSnapshotMiStream failed!");
        return false;
    }

    // create biningsize BAYER10 snapshot stream ,only one rawStream
    StreamParameter para2 = {
        .isPhysicalStream = false,
        .format = eImgFmt_BAYER10,
    };
    if (mSupportAlgoMask & AlgoType::MI_ALGO_MIHAL_MFNR) {
        para2.needYuvR3Stream = true;
    }
    auto rawStream = createSnapshotMiStream(para2, mSuperHdData.binningResolution.width,
                                            mSuperHdData.binningResolution.height);
    if (rawStream == nullptr) {
        MLOGE(LogGroupHAL, "createSnapshotMiStream rawStream failed!");
        return false;
    }

    return true;
}

void MiCamDecisionSuperHD::determineAlgoType()
{
    mSuperHdData.isRemosaic = mSuperHdData.remosaicNeeded && mSuperHdData.isQcfaSensor;
    if (mSuperHdData.isRemosaic) {
        mAlgoType = MI_ALGO_NONE;
    } else if (mSupportAlgoMask & MI_ALGO_MIHAL_MFNR) {
        mAlgoType = MI_ALGO_MIHAL_MFNR;
    } else if (mSupportAlgoMask & MI_ALGO_VENDOR_MFNR) {
        mAlgoType = MI_ALGO_VENDOR_MFNR;
    }
}

void MiCamDecisionSuperHD::updateMetaInCaptureOverride(MiCapture::CreateInfo &capInfo)
{
    if (mSuperHdData.isRemosaic) {
        MiMetadata::setEntry<MUINT8>(&capInfo.settings, XIAOMI_REMOSAIC_ENABLED, 1);
        // enable mfnr default
        // decide also by camera_custom_feature_table.cpp
        // MTK_FEATURE_REMOSAIC_MFNR 48M enable REMOSAIC + MFNR, while 108M disable it
        MiMetadata::setEntry<uint8_t>(&capInfo.settings, XIAOMI_MFNR_ENABLED, 1);

        MiMetadata::setEntry<MUINT8>(&capInfo.settings, MTK_CONTROL_CAPTURE_REMOSAIC_EN, 1);
    }
}

void MiCamDecisionSuperHD::updateStatusOverride(const MiMetadata &status)
{
    // NOTE: read remosaic detected (XIAOMI_REMOSAIC_DETECTED)
    if (MiMetadata::getEntry<uint8_t>(&status, XIAOMI_REMOSAIC_DETECTED,
                                      mSuperHdData.remosaicNeeded)) {
        MLOGI(LogGroupHAL, "check remosaicNeeded : %d", mSuperHdData.remosaicNeeded);
    }
}

/******************MiCamDecisionProfessional************/
MiCamDecisionProfessional::MiCamDecisionProfessional(const CreateInfo &info) : MiCamDecision(info)
{
    MLOGI(LogGroupHAL, "ctor +");
    mSupportAlgoMask = AlgoType::MI_ALGO_MIHAL_MFNR;
    mCaptureTypeName = "Professional";
    // this function is exit, cover it
    mAlgoFuncMap[MI_ALGO_NONE] = [this](MiCapture::CreateInfo &capInfo) {
        auto raw16Stream = getPhyStream(eImgFmt_RAW16);
        auto nv21Stream = getPhyStream(eImgFmt_NV21);
        if (raw16Stream == nullptr || nv21Stream == nullptr) {
            MLOGE(LogGroupHAL, "getPhyStream failed");
            return false;
        }
        capInfo.streams.push_back(raw16Stream);
        capInfo.streams.push_back(nv21Stream);
        mCurrentCamMode->addAnchorFrame(capInfo.fwkRequest.frameNumber,
                                        capInfo.fwkRequest.frameNumber - 1);
        return true;
    };
    MLOGI(LogGroupHAL, "ctor -");
}

bool MiCamDecisionProfessional::buildMiConfigOverride()
{
    MLOGI(LogGroupHAL, "+");
    for (auto &&[id, s] : mFwkConfig->streams) {
        if (s->rawStream.format == eImgFmt_RAW16) {
            mHasRawStreamOut = true;
            break;
        }
    }
    MLOGI(LogGroupHAL, "mHasRawStreamOut(%d)", mHasRawStreamOut);
    if (!mHasRawStreamOut) {
        // create BAYER10 snapshot stream, only one rawStream
        StreamParameter para = {
            .isPhysicalStream = false,
            .format = eImgFmt_BAYER10,
        };
        if (mSupportAlgoMask & AlgoType::MI_ALGO_MIHAL_MFNR) {
            para.needYuvR3Stream = true;
        }
        auto stream = createSnapshotMiStream(para);
        if (stream == nullptr) {
            MLOGE(LogGroupHAL, "createSnapshotMiStream failed!");
            return false;
        }
    }

    StreamParameter para = {
        .isPhysicalStream = false,
        .format = eImgFmt_NV21,
    };
    auto stream = createSnapshotMiStream(para);
    if (stream == nullptr) {
        MLOGE(LogGroupHAL, "createSnapshotMiStream failed!");
        return false;
    }

    MLOGI(LogGroupHAL, "-");
    return true;
}

void MiCamDecisionProfessional::determineAlgoType()
{
    mAlgoType = MI_ALGO_NONE;

    if (!mHasRawStreamOut && (mSupportAlgoMask & MI_ALGO_MIHAL_MFNR)) {
        mAlgoType = MI_ALGO_MIHAL_MFNR;
    }
}

} // namespace mizone
