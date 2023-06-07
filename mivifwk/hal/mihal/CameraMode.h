#ifndef __CAMERA_MODE_H__
#define __CAMERA_MODE_H__

#include <cutils/properties.h>

#include <atomic>
#include <cmath>
#include <condition_variable>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// clang-format off
#include <MiaPostProcType.h>

#include "BGService.h"
#include "CamGeneralDefinitions.h"
#include "Camera3Plus.h"
#include "CameraManager.h"
#include "MiviSettings.h"
// clang-format on

namespace mihal {

using namespace mialgo2;

class AlgoSession;
class AsyncAlgoSession;
class Photographer;

static const uint32_t MIVI_SUPER_NIGHT_SHIFT_SKIP_PREVIEW_BUFFER = 16;

static std::vector<uint32_t> sPreprocessForcedMetaTag = {ANDROID_STATISTICS_FACE_RECTANGLES};

struct HdrStatus
{
    HdrStatus() : hdrDetected{false}, expVaules{5, 0}, rawExpVaules{5, 0} {}
    std::string toString() const;

    bool hdrDetected;
    std::vector<int32_t> expVaules;
    std::vector<int32_t> rawExpVaules;
};

// NOTE: hdr data read from checker
struct HdrRelatedData
{
    HdrStatus hdrStatus = {};
    int32_t hdrSceneType = 0;
    int32_t hdrAdrc = 0;
    bool isZslHdrEnabled = false;
    bool hdrMotionDetected = false;
    int32_t hdrHighThermalLevel = 0;
    // NOTE: used to denote the number of requests for each ev val in SR+HDR
    std::vector<int32_t> reqNumPerEv = {};
    // NOTE: used to decide which frame should be set mfnr enabled tag in MFNR+HDR
    std::vector<int32_t> reqMfnrSetting = {};
    SnapshotReqInfo snapshotReqInfo;

    HdrRelatedData()
    {
        snapshotReqInfo.size = 0;
        snapshotReqInfo.data = nullptr;
    }

    virtual ~HdrRelatedData()
    {
        if (snapshotReqInfo.data) {
            free(snapshotReqInfo.data);
            snapshotReqInfo.data = nullptr;
        }
    }

    HdrRelatedData &operator=(const HdrRelatedData &other)
    {
        hdrStatus = other.hdrStatus;
        hdrSceneType = other.hdrSceneType;
        hdrAdrc = other.hdrAdrc;
        isZslHdrEnabled = other.isZslHdrEnabled;
        hdrMotionDetected = other.hdrMotionDetected;
        hdrHighThermalLevel = other.hdrHighThermalLevel;
        reqNumPerEv = other.reqNumPerEv;
        reqMfnrSetting = other.reqMfnrSetting;

        if (!snapshotReqInfo.size) {
            snapshotReqInfo.size = other.snapshotReqInfo.size;
            snapshotReqInfo.data = (uint8_t *)malloc(snapshotReqInfo.size);
            memset(snapshotReqInfo.data, 0, snapshotReqInfo.size);
        }
        memcpy(snapshotReqInfo.data, other.snapshotReqInfo.data, snapshotReqInfo.size);

        return *this;
    }
};

struct SuperNightData
{
    uint32_t cameraId; // logicalCameraId
    bool superNightEnabled;
    bool skipPreviewBuffer;
    uint32_t miviSupernightMode;
    AiMisdScenes aiMisdScenes;
    SuperNightChecker superNightChecher;
    int32_t miviSupernightSupportMask; // 0:none, 1:sat  2:supportnight 4:bokeh
    std::mutex mutex;

    SuperNightData() = default;

    SuperNightData(const SuperNightData &other) { operator=(other); }

    SuperNightData &operator=(const SuperNightData &other)
    {
        cameraId = other.cameraId;
        superNightEnabled = other.superNightEnabled;
        skipPreviewBuffer = other.skipPreviewBuffer;
        miviSupernightMode = other.miviSupernightMode;
        aiMisdScenes = other.aiMisdScenes;
        superNightChecher = other.superNightChecher;
        miviSupernightSupportMask = other.miviSupernightSupportMask;

        return *this;
    }
};

struct SuperHdData
{
    uint32_t cameraId;
    bool isQcfaSensor;
    ImageSize superResolution;
    ImageSize binningResolution;
    uint8_t remosaicNeeded;
    bool isRemosaic;
};

struct ASDInfo
{
    std::string exifInfo;
    std::mutex mutex;

    ASDInfo() = default;
    ASDInfo(const ASDInfo &other) { operator=(other); }
    ASDInfo &operator=(const ASDInfo &other)
    {
        exifInfo = other.exifInfo;
        return *this;
    }
};

struct FlashData
{
    uint8_t flashMode;
    uint8_t flashState;
};

union TagValue {
    uint8_t byteValue[8];
    uint8_t byteTpye;
    int32_t intType;
    float fType;
    int64_t longType;
    double dType;
};

enum class BokehVendor {
    Default = 0,
    ArcSoft = 1,
};

class CameraMode
{
public:
    enum ModeType {
        MODE_UNSPECIFIED = 0,
        MODE_CAPTURE = 1, // SAT
        MODE_PORTRAIT = 2,
        MODE_PROFESSIONAL = 3,
        MODE_SUPER_NIGHT = 4,
        MODE_SUPER_HD = 5,
        MODE_RECORD_VIDEO = 6,
    };

    enum SNModeMask {
        NONE_MASK = 0x0,
        CAPTURE_MASK = 0x1,
        SUPER_NIGHT_MASK = 0x2,
        PORTRAIT_MASK = 0x4,
    };

    enum HdrMode {
        HDR_OFF = 0,
        HDR_ON,
        HDR_AUTO,
    };

    enum BokehStreamType {
        BokehMasterYuv = 0,
        BokehMasterRaw,
        BokehSlaveYuv,
        BokehSlaveRaw,
    };

    enum BokehMadridLensMode {
        CAPTURE_MADRID_OFF = 0,
        CAPTURE_MADRID_NIGHT,
        CAPTURE_MADRID_SOFT,
        CAPTURE_MADRID_BLACK,
    };

    /**
     * Bit Mask Of Capture Feature Support
     * ----------------------------------------------------------------
     * |    0~3    |     -     |   Burst   | vendorMfnr |  mihalMfnr  |
     * |    4~7    |  RawHdr   |   HdrSr   |     Sr     |     Hdr     |
     * |    8~11   | FallBack  |     -     |  FakeSat   |     Sn      |
     * |   12~15   |   Bokeh   |   Beauty  |  Depurple  |    Fusion   |
     * ----------------------------------------------------------------
     **/
    enum FeatureMask {
        FeatureMihalMfnr = 0x1,
        FeatureVendorMfnr = 0x2,
        FeatureMfnr = 0x3,
        FeatureBurst = 0x4,
        FeatureFallBack = 0x8,
        FeatureHDR = 0x10,
        FeatureSR = 0x20,
        FeatureSRHDR = 0x40,
        FeatureRawHDR = 0x80,
        FeatureSN = 0x100,
        FeatureFakeSat = 0x200,
        FeatureFusion = 0x1000,
        FeatureDepurple = 0x2000,
        FeatureBeauty = 0x4000,
        FeatureBokeh = 0x8000,
    };

    enum VideoFeatureMask {
        VideoFeatureHDR = 0x1,
        VideoFeatureSN = 0x2,
        VideoFeatureBokeh = 0x4,
        VideoFeatureHDR10 = 0x8,
        VideoFeatureAntiShake = 0x10,
        VideoFeatureBeauty = 0x20,
        VideoFeatureAntiShakeV3 = 0x40,
    };

    CameraMode(AlgoSession *session, CameraDevice *vendorCamera, const StreamConfig *configFromFwk);
    virtual ~CameraMode();

    // NOTE: according to the following info to create the config to vendor:
    // 1. the operation_mode and/or the session_parameter in the original config
    // 2. the static_info(physical cameras consist of if it's a multicamera) of the opened vendor
    //    camera
    // 3. some special configurations from the extra *.json files if existed
    // 4. anything else if needed
    LocalConfig *buildConfigToVendor();
    virtual std::shared_ptr<LocalConfig> buildConfigToDarkroom(int type = 0);
    uint32_t getDarkroomOutputNumber()
    {
        if (isGalleryBokehReFocusNeeded()) {
            return 3;
        } else {
            return 1;
        }
    }
    void buildConfigToMock(std::shared_ptr<CaptureRequest> fwkRequest);
    void updateFwkOverridedStream();
    void applyDefaultSettingsForSnapshot(std::shared_ptr<CaptureRequest> fwkRequest);
    std::shared_ptr<Photographer> createPhotographer(std::shared_ptr<CaptureRequest> fwkRequest);
    virtual void updateSettings(CaptureRequest *request);
    void updateFlashSettings(CaptureRequest *request);
    void updateFlashStatus(CaptureResult *result);
    virtual void updateStatus(CaptureResult *result);
    void returnInternalQuickviewBuffer(std::shared_ptr<StreamBuffer> streamBuffer,
                                       uint32_t frame /*vndframe*/, int64_t timestamp);
    void returnInternalFrameTimestamp(uint32_t frame /*vndframe*/, int64_t timestamp);
    std::shared_ptr<StreamBuffer> getInternalQuickviewBuffer(uint32_t frame);
    void notifyAnchorFrame(uint32_t anchorFrame, uint32_t snapshotFrame);
    void notifyLastFrame(uint32_t snapshotFrame);
    uint32_t getFirstWaitedPreview() const;
    int getCameraModeType() const { return mModeType; }
    bool isQuickShot(std::shared_ptr<CaptureRequest> fwkRequest);
    bool isMarcoMode(std::shared_ptr<CaptureRequest> fwkRequest) const;
    bool mLlsNeed;
    std::shared_ptr<ResultData> inviteForMockRequest(int32_t frame);

    void setRetired();

    bool needForwardEarlypic(uint32_t snapshotFrame, bool isSupportQuickview);

    int getRawStreamFormat(camera3_stream *stream)
    {
        int format = INVALIDFORMAT;
        auto it = mRawStreamFormat.find(stream);
        if (it != mRawStreamFormat.end()) {
            format = static_cast<int>(it->second);
        }

        return format;
    }

    // NOTE: in some camera mode, Qcom's preview pipeline can only output one port, in such cases,
    // we should only send quickview buffer to Qcom, for details, refer to doc:
    // https://xiaomi.f.mioffice.cn/docs/dock4qQImIBogDbm7qEcpxLllld
    bool isHalLowCapability() { return mIsVendorLowCaps; };

    float getUIZoomRatio() { return mUIZoomRatio.load(); }

    float getFovCropZoomRatio() { return mFovCropZoomRatio; }

    bool isSupportFovCrop() { return mIsSupportFovCrop; }

    void onMihalBufferChanged();

    void waitLoop();

    void setBufferLimitCnt(uint32_t cnt)
    {
        MLOGI(LogGroupHAL, "[AppSnapshotController] set mBufferLimitCnt = %d", cnt);
        mBufferLimitCnt = cnt;
    }

    void resetBufferLimitCnt()
    {
        MLOGI(LogGroupHAL, "[AppSnapshotController] reset mBufferLimitCnt = %d",
              mBufferDefaultLimitCnt);
        mBufferLimitCnt = mBufferDefaultLimitCnt;
    }

    uint32_t findAnchorVndframeByTs(int64_t ts);

    std::chrono::milliseconds mWaitFlushMillisecs;

    virtual int32_t getIsHdsrZslSupported() { return mIsHdsrZslSupported; }

    void setupPreprocessForcedMeta(Metadata &meta)
    {
        std::lock_guard<std::mutex> lg(mPreprocessForcedMetaMutex);
        meta.update(mPreprocessForcedMeta);
    }

protected:
    std::vector<std::shared_ptr<Stream>> getCurrentCaptureStreams(
        std::shared_ptr<CaptureRequest> fwkRequest, int type);
    int createInternalQuickviewStream();
    void loop();
    int processAnchorFrameResult(uint32_t anchorFrame, uint32_t snapshotFrame,
                                 std::shared_ptr<StreamBuffer> inputStreamBuffer);
    void convertAspectRatioToActualResolution(InternalStreamInfo &para, uint32_t featureMask = 0,
                                              const char *optimalSizeTag = nullptr);
    std::shared_ptr<Stream> createInternalStream(InternalStreamInfo &para);

    Metadata updateOutputSize(const Metadata *metadata, ImageSize imageSize);
    void addToMockInvitations(std::shared_ptr<CaptureRequest> fwkRequest,
                              const std::vector<std::shared_ptr<Stream>> &streams);

    bool isSupportQuickShot(int type, std::shared_ptr<CaptureRequest> fwkRequest) const;
    uint64_t getQuickShotDelayTime(int type) const;

    CameraUser getCameraUser();

    uint32_t getSDKMialgoOpMode()
    {
        uint32_t mialgoOpMode = 0xffff;

        auto meta = mConfigFromFwk->getMeta();
        auto entry = meta->find(SDK_SESSION_PARAMS_OPERATION);
        if (entry.count > 0) {
            mialgoOpMode = entry.data.i32[0];
            MLOGV(LogGroupHAL, "mialgo opMode: %d", mialgoOpMode);
        } else {
            MLOGE(LogGroupHAL, "Can't get mialgo operation mode for sdk!!!");
        }

        return mialgoOpMode;
    }

    void resetPreProcessRelatedMetaLocked()
    {
        for (auto tag : sPreprocessForcedMetaTag) {
            auto entry = mPreprocessForcedMeta.find(tag);
            if (!entry.count) {
                continue;
            }

            switch (entry.type) {
            case TYPE_BYTE: {
                std::vector<uint8_t> data(entry.count, 0);
                mPreprocessForcedMeta.update(tag, data.data(), entry.count);
                break;
            }
            case TYPE_INT32: {
                std::vector<int32_t> data(entry.count, 0);
                mPreprocessForcedMeta.update(tag, data.data(), entry.count);
                break;
            }
            case TYPE_FLOAT: {
                std::vector<float> data(entry.count, 0);
                mPreprocessForcedMeta.update(tag, data.data(), entry.count);
                break;
            }
            case TYPE_INT64: {
                std::vector<int64_t> data(entry.count, 0);
                mPreprocessForcedMeta.update(tag, data.data(), entry.count);
                break;
            }
            case TYPE_DOUBLE: {
                std::vector<double> data(entry.count, 0);
                mPreprocessForcedMeta.update(tag, data.data(), entry.count);
                break;
            }
            default:
                break;
            }
        }
    }

    void updatePreProcessRelatedMeta(CaptureResult *result)
    {
        std::lock_guard<std::mutex> lg(mPreprocessForcedMetaMutex);

        resetPreProcessRelatedMetaLocked();

        for (auto tag : sPreprocessForcedMetaTag) {
            auto entry = result->findInMeta(tag);
            if (entry.count) {
                mPreprocessForcedMeta.update(entry);
            }
        }
    }

    void update3AStatus(CaptureResult *result)
    {
        auto entry = result->findInMeta(MI_MIVI_SHORTGAIN);
        if (entry.count) {
            mShortGain = entry.data.f[0];
        }
        auto luxEntry = result->findInMeta(MI_MIVI_LUXINDEX);
        if (luxEntry.count) {
            mLuxIndex.store(luxEntry.data.f[0]);
        }
        auto sceneAppliedEntry = result->findInMeta(MI_AI_ASD_SCENE_APPLIED);
        if (sceneAppliedEntry.count) {
            mAiAsdSceneApplied.store(sceneAppliedEntry.data.i32[0]);
        }
        auto adrcGainEntry = result->findInMeta(MI_MIVI_ADRCGAIN);
        if (adrcGainEntry.count) {
            mAdrcGain.store(adrcGainEntry.data.f[0]);
        }
        auto darkRatioEntry = result->findInMeta(MI_MIVI_DARKRATIO);
        if (darkRatioEntry.count) {
            mDarkRatio.store(darkRatioEntry.data.f[0]);
        }
        auto ExpoTimeEntry = result->findInMeta(ANDROID_SENSOR_EXPOSURE_TIME);
        if (ExpoTimeEntry.count) {
            mExposureTime.store(ExpoTimeEntry.data.i64[0]);
        }
        auto flashModeEntry = result->findInMeta(MI_FLASH_MODE);
        if (flashModeEntry.count) {
            mFlashMode.store(flashModeEntry.data.u8[0]);
        }
    }

    void updateThermalStatus(CaptureResult *result)
    {
        auto entry = result->findInMeta(MI_THERMAL_LEVEL);
        if (entry.count) {
            mThermalLevel.store(entry.data.i32[0]);
        }
    }

    int getMultiFrameNums(int type);

    /*
     * NOTE: BurstCapture related functions
     */
    bool isFeatureBurstSupported()
    {
        return mVendorInternalStreamInfoMap.featureMask & FeatureMask::FeatureBurst;
    }

    void updateBurstSettings(CaptureRequest *request);
    bool canDoBurstCapture() { return mIsBurstCapture; }

    /*
     * NOTE: Mfnr/QuickShot related functions
     */
    bool isFeatureMfnrSupported()
    {
        return mVendorInternalStreamInfoMap.featureMask & FeatureMask::FeatureMfnr;
    }

    bool isFeatureMihalMfnrSupported()
    {
        return mVendorInternalStreamInfoMap.featureMask & FeatureMask::FeatureMihalMfnr;
    }

    bool isFeatureVendorMfnrSupported()
    {
        return mVendorInternalStreamInfoMap.featureMask & FeatureMask::FeatureVendorMfnr;
    }

    void initMfnrCapabilities()
    {
        auto mfnrRatioEntry = mVendorCamera->findInStaticInfo(MI_QUICKSHOT_MFNR_RATIOTHRESHOLD);
        if (mfnrRatioEntry.count > 0) {
            MLOGD(LogGroupHAL, "current mode camera hav tag zoomRatioThresholdQuickshotByMfnr");
            for (int i = 0; i < mfnrRatioEntry.count / 4; i += 4) {
                int32_t cameraRoleID = mfnrRatioEntry.data.i32[i];
                float mfnrStartZoomRatio = mfnrRatioEntry.data.f[i + 2];
                float mfnrEndZoomRatio = mfnrRatioEntry.data.f[i + 3];
                mMfnrRoleIdToThresholdMap[cameraRoleID].first = mfnrStartZoomRatio;
                mMfnrRoleIdToThresholdMap[cameraRoleID].second = mfnrEndZoomRatio;
                MLOGD(LogGroupHAL,
                      "cameraRoleID:%d,corresponding sr zoom threshold: %0.1f to %0.1f",
                      cameraRoleID, mfnrStartZoomRatio, mfnrEndZoomRatio);
            }
        } else {
            MLOGD(LogGroupHAL, "current mode do not have tag zoomRatioThresholdQuickshotByMfnr");
        }
    }

    int getMihalMfnrSum()
    {
        if (mShortGain < 4.0f && mShortGain > 0.0f) {
            return 5;
        } else {
            return 8;
        }
    }

    void updateQuickSnapshot(int &type, int &frameNums, std::shared_ptr<CaptureRequest> fwkRequest);

    /*
     * NOTE: Supernight related functions
     */
    bool isFeatureSuperNightSupported()
    {
        return mVendorInternalStreamInfoMap.featureMask & FeatureMask::FeatureSN;
    }

    void initSupernightCapabilities()
    {
        auto entry = mVendorCamera->findInStaticInfo(MI_MIVI_SUPERNIGHT_SUPPORTMASK);
        if (!entry.count) {
            MLOGI(LogGroupHAL, "get miviSupernightSupportMask failed!");
        } else {
            mSuperNightData.miviSupernightSupportMask =
                *static_cast<const int32_t *>(entry.data.i32);
            mSuperNightData.skipPreviewBuffer = isSuperNightSkipPreview();
        }

        mSuperNightData.cameraId = static_cast<uint32_t>(mVendorCamera->getLogicalId());
        MLOGI(LogGroupHAL, "get cameraid %d mask %d skippreview %d", mSuperNightData.cameraId,
              mSuperNightData.miviSupernightSupportMask, mSuperNightData.skipPreviewBuffer);
    }

    bool canDoSuperNight(std::shared_ptr<CaptureRequest> fwkReq);
    bool canCheckerValDoSuperNight(const SuperNightData &checkerNightVal);
    bool isMiviSuperNightSupport() { return mSuperNightData.miviSupernightSupportMask & 0x7; }
    int getSuperNightCheckerSum() { return mApplySuperNightData.superNightChecher.sum; }
    void updateSuperNightStatus(CaptureResult *result);
    bool isSuperNightSkipPreview()
    {
        int32_t shiftMask =
            mSuperNightData.miviSupernightSupportMask >> MIVI_SUPER_NIGHT_SHIFT_SKIP_PREVIEW_BUFFER;

        switch (mModeType) {
        case MODE_CAPTURE:
            return shiftMask & CAPTURE_MASK;
        case MODE_SUPER_NIGHT:
            return shiftMask & SUPER_NIGHT_MASK;
        case MODE_PORTRAIT:
            return shiftMask & PORTRAIT_MASK;
        default:
            return false;
        }
    }
    void updateSuperNightSettings(CaptureRequest *request);

    /*
     * NOTE: Sr related functions
     */
    bool isFeatureSrSupported()
    {
        return mVendorInternalStreamInfoMap.featureMask & FeatureMask::FeatureSR;
    }

    void initSrCapabilities()
    {
        // NOTE: MI_SR_RATIO_THRESHOLD is used to check whether current zoom ratio is in the HdrSr
        // zoom interval. This meta is in the form of [role0, zoomRatio0, role1, zoomRatio1, role2,
        // zoomRatio2, role3, zoomRatio3] for details, you can refer to:
        // packages/apps/MiuiCamera/src/com/android/camera2/CameraCapabilities.java and doc:
        // https://xiaomi.f.mioffice.cn/docs/dock4zC0q7smPaMMzQSKD1bFAeb
        auto srRatioEntry = mVendorCamera->findInStaticInfo(MI_SR_RATIO_THRESHOLD);
        if (srRatioEntry.count > 0) {
            MLOGD(LogGroupHAL, "[HDR][SR]: sat mode camera have tag zoomRatioThresholdToStartSr");
            for (int i = 0; i < srRatioEntry.count / 4; i += 2) {
                int32_t cameraRoleID = srRatioEntry.data.i32[i];
                float zoomThreshold = srRatioEntry.data.f[i + 1];
                mSrRoleIdToThresholdMap[cameraRoleID] = zoomThreshold;
                MLOGD(LogGroupHAL,
                      "[HDR][SR]: cameraRoleID: %d, corresponding sr zoom threshold: %0.1f",
                      cameraRoleID, zoomThreshold);
            }
        } else {
            MLOGD(LogGroupHAL,
                  "[HDR][SR]: sat mode camera do not have tag zoomRatioThresholdToStartSr");
        }

        auto entry = mVendorCamera->findInStaticInfo(MI_SRHDR_ZSL_ENALBE);
        if (entry.count) {
            mIsHdsrZslSupported = entry.data.i32[0];
            MLOGI(LogGroupHAL, "mIsHdsrZslSupported %d", mIsHdsrZslSupported);
        } else {
            MLOGW(LogGroupHAL, "get xiaomi.superResolution.isHdsrZSLSupported failed!");
        }
    }

    int getSrCheckerSum()
    {
        int sum = 8;
        MiviSettings::getVal("FeatureList.FeatureSR.NumOfSnapshotRequiredBySR", sum, 8);
        return sum;
    }

    void updateZoomRatioSettings(CaptureRequest *request)
    {
        auto entry = request->findInMeta(ANDROID_CONTROL_ZOOM_RATIO);
        if (entry.count) {
            mUIZoomRatio.store(entry.data.f[0]);
            MLOGV(LogGroupRT, "zoom ratio get updated, now UI is %0.1fX", mUIZoomRatio.load());
        }
    }

    bool canDoSr();

    bool canDoFakeSat(int roleId);
    /*
     * NOTE: Hdr related functions
     */
    bool isFeatureHdrSupported()
    {
        return mVendorInternalStreamInfoMap.featureMask & FeatureMask::FeatureHDR;
    }

    bool isFeatureRawHdrSupported()
    {
        return mVendorInternalStreamInfoMap.featureMask & FeatureMask::FeatureRawHDR;
    }

    int getHdrCheckerSum() { return mApplyHdrData.hdrStatus.expVaules.size(); }
    void updateHdrSettings(CaptureRequest *request);
    void updateHdrStatus(CaptureResult *result);
    bool canDoHdr();

    /*
     * NOTE: HdrSr related functions
     */
    bool isFeatureHdrSrSupported()
    {
        return mVendorInternalStreamInfoMap.featureMask & FeatureMask::FeatureSRHDR;
    }

    int getHdrSrCheckerSum()
    {
        int sum = 10;
        MiviSettings::getVal("FeatureList.FeatureSRHDR.NumOfSnapshotRequiredBySRHDR", sum, 10);
        return sum;
    }

    bool canDoHdrSr();

    void updateHdrSrStatus(CaptureResult *result)
    {
        // FIXME: is this tag obsolete??
        auto entry = result->findInMeta(MI_HDR_SR_HDR_DETECTED);
        if (entry.count > 0) {
            bool isHdrSrDetected = entry.data.u8[0];
            mHdrSrDetected.store(isHdrSrDetected);
            MLOGI(LogGroupRT, "[HDR][SR]: mHdrSrDetected %d", isHdrSrDetected);
        }
    }

    bool isFeatureFakeSatSupported()
    {
        if (!isSat()) {
            return false;
        } else {
            return mVendorInternalStreamInfoMap.featureMask & FeatureMask::FeatureFakeSat;
        }
    }

    /*
     * NOTE: Fusion related functions
     */
    bool isFeatureFusionSupported()
    {
        return (mVendorInternalStreamInfoMap.featureMask & FeatureMask::FeatureFusion) &&
               mIsFusionSupported;
    }

    void initFusionCapabilities()
    {
        auto entry = mVendorCamera->findInStaticInfo(MI_CAPTURE_FUSION_SUPPORT_CPFUSION);
        if (entry.count) {
            mIsFusionSupported = entry.data.u8[0];
            MLOGI(LogGroupHAL, "mIsFusionSupported %d", mIsFusionSupported);
        } else {
            MLOGW(LogGroupHAL, "get xiaomi.capturefusion.supportCPFusion failed!");
        }
    }

    void updateFusionStatus(CaptureResult *result)
    {
        auto entry = result->findInMeta(MI_CAPTURE_FUSION_IS_PIPELINE_READY);
        if (entry.count > 0) {
            bool ready = entry.data.u8[0];
            mIsFusionReady.store(ready);
            MLOGV(LogGroupRT, "mIsFusionReady %d", ready);
        }
    }

    std::vector<std::shared_ptr<Stream>> getFusionStreams(FusionCombination fc) const
    {
        std::vector<std::shared_ptr<Stream>> fusionStreams;
        std::vector<CameraDevice *> fusionCameras = mVendorCamera->getCurrentFusionCameras(fc);
        for (auto item : fusionCameras) {
            fusionStreams.push_back(mConfigToVendor->getPhyStream(item->getId().c_str()));
        }
        return fusionStreams;
    }

    bool canDoMfnrFusion()
    {
        float currentZoomRatio = mUIZoomRatio.load();
        float mfnrFusionZoomLowerBound, mfnrFusionZoomUpperBound;
        MiviSettings::getVal("FeatureList.FeatureFusion.MfnrFusionZoomRegion.lowerBound",
                             mfnrFusionZoomLowerBound, 0.6f);
        MiviSettings::getVal("FeatureList.FeatureFusion.MfnrFusionZoomRegion.upperBound",
                             mfnrFusionZoomUpperBound, 1.0f);

        if (currentZoomRatio >= mfnrFusionZoomLowerBound &&
            currentZoomRatio <= mfnrFusionZoomUpperBound && mIsFusionReady.load()) {
            return true;
        }

        return false;
    }

    bool canDoSrFusion()
    {
        float currentZoomRatio = mUIZoomRatio.load();
        float srFusionZoomLowerBound, srFusionZoomUpperBound;
        MiviSettings::getVal("FeatureList.FeatureFusion.SrFusionZoomRegion.lowerBound",
                             srFusionZoomLowerBound, 3.0f);
        MiviSettings::getVal("FeatureList.FeatureFusion.SrFusionZoomRegion.upperBound",
                             srFusionZoomUpperBound, 5.0f);

        if (currentZoomRatio >= srFusionZoomLowerBound &&
            currentZoomRatio <= srFusionZoomUpperBound && mIsFusionReady.load()) {
            return true;
        }

        return false;
    }

    /*
     * NOTE: MultiCamera related functions
     */
    void updateMasterId(CaptureResult *result)
    {
        auto entry = result->findInMeta(ANDROID_LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID);
        if (entry.count) {
            std::string masterId = reinterpret_cast<const char *>(entry.data.u8);
            mMasterId.store(atoi(masterId.c_str()));
        }
    }

    void updateMotionStatus(CaptureResult *result)
    {
        auto entry = result->findInMeta(MI_AI_MISD_MOTIONCAPTURETYPE);
        if (entry.count) {
            mMotion = entry.data.u8[0];
        }
    }

    bool isSat() const
    {
        return (isMultiCameraCase() &&
                (mConfigFromFwk->getOpMode() == mihal::StreamConfigModeAlgoDualSAT));
    }

    bool isExtension() const
    {
        return (isMultiCameraCase() &&
                mVendorCamera->getRoleId() == CameraDevice::RoleIdRear3PartSat);
    }

    bool isDualBokeh() const
    {
        return (isMultiCameraCase() &&
                (mConfigFromFwk->getOpMode() == mihal::StreamConfigModeAlgoDual));
    }

    bool isSingleBokeh() const
    {
        return (mConfigFromFwk->getOpMode() == mihal::StreamConfigModeAlgoSingleRTB);
    }

    bool isGalleryBokehReFocusNeeded() const
    {
        return ((isDualBokeh() || isSingleBokeh()) && !getBokehMDMode());
    }

    std::shared_ptr<Stream> getCurrentPhyStream(std::string phyId, int format)
    {
        return mConfigToVendor->getPhyStream(phyId, [format](std::shared_ptr<Stream> stream) {
            if (stream->getFormat() == format)
                return true;
            else
                return false;
        });
    }

    std::shared_ptr<Stream> getFakeSatStream(std::string phyId, int format)
    {
        return mConfigToVendor->getPhyStream(phyId, [format](std::shared_ptr<Stream> stream) {
            if (stream->getFormat() == format && stream->isRemosaic())
                return true;
            else
                return false;
        });
    }

    // TODO: we hardcode format here but this is not the ideal way.
    std::shared_ptr<Stream> getBokehMasterStream(int format)
    {
        return getCurrentPhyStream(std::to_string(getBokehMasterId()), format);
    }

    std::shared_ptr<Stream> getBokehSlaveStream(int format)
    {
        return getCurrentPhyStream(std::to_string(getBokehSlaveId()), format);
    }

    int32_t getRoleIdByCurrentCameraDevice() const
    {
        std::string masterId = std::to_string(mMasterId.load());
        CameraDevice *currentCameraDevice = mVendorCamera;
        if (isSat()) {
            for (auto camera : mVendorCamera->getPhyCameras()) {
                if (masterId == camera->getId()) {
                    currentCameraDevice = camera;
                    break;
                }
            }
        }

        int32_t currentCameraRoleId = currentCameraDevice->getRoleId();
        if (isDualBokeh()) {
            // NOTE: Dual bokeh role id need to be getted from special meta tag.
            currentCameraRoleId = getBokehRoleId();
        }

        return currentCameraRoleId;
    }

    bool isMultiCameraCase() const
    {
        auto entry = mVendorCamera->findInStaticInfo(ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS);
        return entry.count > 0 ? true : false;
    }

    uint32_t getCameraPhysicalIdByTag(const char *cameraIdTag) const
    {
        uint32_t cameraId = 0xFFFF;
        if (cameraIdTag != nullptr) {
            auto entry = mVendorCamera->findInStaticInfo(cameraIdTag);
            if (entry.count > 0) {
                cameraId = entry.data.i32[0];
            } else {
                MLOGE(LogGroupHAL, "get phy camera id failed!");
            }
        }
        return cameraId;
    }

    void initMadridBokehCapabilities()
    {
        auto entry = mVendorCamera->findInStaticInfo(MI_BOKEH_MADRID_MODE_SUPPORTED);
        if (entry.count) {
            mBokehMadridSupported = entry.data.i32[0];
            MLOGI(LogGroupHAL, "Bokeh Madrid supported: %d", mBokehMadridSupported);
        }

        entry = mVendorCamera->findInStaticInfo(MI_BOKEH_MADRID_EV_LIST);
        if (entry.count) {
            for (int i = 0; i < entry.count; i++) {
                mBokehMadridEvData.push_back(entry.data.i32[i]);
                MLOGI(LogGroupHAL, "Bokeh Madrid Ev[%d] %d", i, entry.data.i32[i]);
            }
        }
    }

    bool isMadridBokehModeSupported() const { return mBokehMadridSupported; };

    bool isMadridBokehModeEnabled(const Metadata *metadata) const
    {
        int32_t mode = 0;

        auto entry = metadata->find(MI_BOKEH_MADRID_MODE);
        if (entry.count) {
            mode = entry.data.u8[0];
            MLOGI(LogGroupHAL, "Bokeh Madrid mode: %d", mode);
        }

        return (mode != BokehMadridLensMode::CAPTURE_MADRID_OFF);
    }

    int getBokehMDMode() const { return mBokehMDMode; }
    int getBokehRoleId() const { return mBokehRoleId; }
    uint32_t getBokehMasterId() const { return mBokehMasterId; }
    uint32_t getBokehSlaveId() const { return mBokehSlaveId; }
    uint32_t getMultiCameraMasterId() const
    {
        if (isSat()) {
            return mMasterId.load();
        } else if (isDualBokeh()) {
            return getBokehMasterId();
        } else {
            return 0xFF;
        }
    }

    uint8_t getBokehVendor() const
    {
        auto entry = mVendorCamera->findInStaticInfo(MI_CAMERA_SUPPORTED_FEATURES_BOKEH_VENDORID);
        if (entry.count <= 0) {
            return 0;
        } else {
            uint8_t vendorId = entry.data.u8[0];
            return vendorId;
        }
    }

    std::tuple<char * /* cameraIdTag */, char * /* optimalSizeTag */> getDualBokehInfo(
        BokehCamSig sig)
    {
        char *cameraIdTag = nullptr;
        char *optimalSizeTag = nullptr;
        uint32_t opMode = mConfigFromFwk->getOpMode();
        int role = getBokehRoleId();

        if (opMode == StreamConfigModeAlgoDual) {
            switch (sig) {
            case BokehCamSig::masterYuv:
                if (role == CameraDevice::RoleIdRearBokeh1x) {
                    cameraIdTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_MASTER1X_CAMERAID);
                    optimalSizeTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_MASTER_1X_OPTIMALSIZE);
                } else {
                    cameraIdTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_MASTER_CAMERAID);
                    optimalSizeTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_MASTER_OPTIMALSIZE);
                }
                break;
            case BokehCamSig::slaveYuv:
                if (role == CameraDevice::RoleIdRearBokeh1x) {
                    cameraIdTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_SLAVE1X_CAMERAID);
                    optimalSizeTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_SLAVE_1X_OPTIMALSIZE);
                } else {
                    cameraIdTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_SLAVE_CAMERAID);
                    optimalSizeTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_SLAVE_OPTIMALSIZE);
                }
                break;
            case BokehCamSig::masterRaw:
                if (role == CameraDevice::RoleIdRearBokeh1x) {
                    cameraIdTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_MASTER1X_CAMERAID);
                    optimalSizeTag =
                        const_cast<char *>(MI_CAMERA_BOKEHINFO_MASTER_1X_RAW_OPTIMALSIZE);
                } else {
                    cameraIdTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_MASTER_CAMERAID);
                    optimalSizeTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_MASTER_RAW_OPTIMALSIZE);
                }
                break;
            case BokehCamSig::slaveRaw:
                if (role == CameraDevice::RoleIdRearBokeh1x) {
                    cameraIdTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_SLAVE1X_CAMERAID);
                    optimalSizeTag =
                        const_cast<char *>(MI_CAMERA_BOKEHINFO_SLAVE_1X_RAW_OPTIMALSIZE);
                } else {
                    cameraIdTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_SLAVE_CAMERAID);
                    optimalSizeTag = const_cast<char *>(MI_CAMERA_BOKEHINFO_SLAVE_RAW_OPTIMALSIZE);
                }
                break;
            }
        }

        return std::make_tuple(cameraIdTag, optimalSizeTag);
    }

    /*
     * NOTE: Professional related functions
     */
    bool isProfessionalMode() const
    {
        bool isManual = false;
        uint32_t opMode = mConfigFromFwk->getOpMode();
        if (opMode == StreamConfigModeAlgoManual) {
            isManual = true;
        }

        return isManual;
    }

    bool isManualMode() const { return (isProfessionalMode() || isManualSuperHDMode()); }

    /*
     * NOTE: ManualSuperHD related functions
     */
    bool isManualSuperHDMode() const
    {
        bool isManualSuperHD = false;
        uint32_t opMode = mConfigFromFwk->getOpMode();
        if (opMode == StreamConfigModeAlgoManualSuperHD) {
            isManualSuperHD = true;
        }

        return isManualSuperHD;
    }

    /*
     * NOTE: SuperHD related functions
     */
    bool isSuperHDMode() const
    {
        bool isSuperHD = false;
        uint32_t opMode = mConfigFromFwk->getOpMode();
        if ((opMode == StreamConfigModeAlgoQCFAMFNR) ||
            (opMode == StreamConfigModeAlgoManualSuperHD)) {
            isSuperHD = true;
        }

        return isSuperHD;
    }

    bool isRemosaic() const { return isRemosaicNeeded() && isQcfaSensor(); }

    /*
     * NOTE: isSuperHDRemosaic() can only be used once when creating HD photographer
     *       to prevent different results caused by thread concurrency
     */
    bool isSuperHDRemosaic() { return isSuperHDMode() && isRemosaic(); }

    bool isQcfaSensor() const
    {
        bool isQcfaSensor = false;
        camera_metadata_ro_entry entryQcfaSensor =
            mVendorCamera->findInStaticInfo(QCOM_IS_QCFA_SENSOR);
        if (!entryQcfaSensor.count) {
            MLOGW(LogGroupHAL, "get quadra_cfa_is_qcfa_sensor failed!");
        } else {
            isQcfaSensor = *(entryQcfaSensor.data.u8);
        }
        MLOGI(LogGroupHAL, "cameraRole %d is qcfa sensor: %d", mVendorCamera->getRoleId(),
              isQcfaSensor);

        return isQcfaSensor;
    }

    ImageSize getSuperResolution();

    // FIXME: The current implementation is dangerous. The way to get binning size is nonstandard
    ImageSize getBinningSizeOfSuperResolution()
    {
        ImageSize superResolution = getSuperResolution();
        ImageSize binningSize = {superResolution.width / 2, superResolution.height / 2};

        MLOGI(LogGroupHAL, "binning size of super resolution w*d: [%d * %d]", binningSize.width,
              binningSize.height);
        return binningSize;
    }

    bool isRemosaicNeeded() const;

    bool isUpScaleCase() const
    {
        if (!isMultiCameraCase() && mVendorCamera->isFrontCamera()) {
            return true;
        }
        return false;
    }

    void initUIRelatedMetaList();

    void updateUIRelatedSettins(CaptureRequest *request);

    void updataASDExifInfo(CaptureResult *result);

    AlgoSession *mSession;
    CameraDevice *mVendorCamera;
    std::unique_ptr<LocalConfig> mConfigFromFwk;
    std::unique_ptr<LocalConfig> mConfigToVendor;
    std::shared_ptr<LocalConfig> mConfigToDarkroom;

    InternalStreamInfoMap mVendorInternalStreamInfoMap;

    // NOTE: configure stream related variables
    float mFrameRatio = 1.0f;
    Metadata mSessionParameter;
    std::shared_ptr<Stream> mInternalQuickviewStream;
    std::map<uint32_t /* frame */, std::shared_ptr<StreamBuffer>> mInternalQuickviewBuffers;
    std::map<uint32_t /* frame */, int64_t /* ts */> mInternalFrameTimestamp;

    // TODO : It will be changed to anchor timestamp later
    uint32_t mInternalQuickviewBufferSize;
    std::map<uint32_t /*Snapshot frame */, uint32_t /*Anchor frame */> mVndAnchorFrames;
    std::mutex mMutex;
    std::condition_variable mCond;

    std::mutex mConfigDarkroomMutex;
    bool mExitThread;
    std::thread mThread;
    int mModeType;
    uint32_t mBufferCapacity;
    std::mutex mMapMutex;
    std::map<uint32_t /* frame */, std::shared_ptr<ResultData>> mMockInvitations;
    bool mMotion = false;

    uint32_t mFwkStreamMaxBufferQueueSize;

    float mShortGain;
    std::atomic<float> mLuxIndex;
    std::atomic<float> mAdrcGain;
    std::atomic<float> mDarkRatio;
    std::atomic<uint64_t> mExposureTime;
    std::atomic<uint8_t> mFlashMode;
    std::atomic<int32_t> mAiAsdSceneApplied;
    std::atomic<int32_t> mThermalLevel;

    bool mIsBurstCapture;
    Metadata mBurstMeta;

    // NOTE: mivi2. 0 adds the QuickShot logic of mfnr. See the document for details
    // https://xiaomi.f.mioffice.cn/docs/dock4HW9S5J2mxB9JYpLE2y0SBf#NtMMmB
    /**
     * 快拍的场景下，各个roleId对应的camera使用MFNR替换SR的zoom ratio区间。
     * 数据结构：
     *     [roleId_x, zoom_count, zoom_0_lower, zoom_0_upper, zoom_1_lower, zoom_1_upper, roleId_y,
     *...] 对应的数据类型： [int, int, float, float, float, float] 　其中, zoom_count是包含的zoom
     *value的个数. 例如, 如果有１组[zoom_lower zoom_upper], 则其值为2.
     */
    std::unordered_map<int32_t /*camera role ID*/,
                       std::pair<float /*mfnrStartZoomRatio*/, float /*mfnrEndZoomRatio*/>>
        mMfnrRoleIdToThresholdMap;

    // NOTE: supernight related parameters
    SuperNightData mSuperNightData;
    SuperNightData mApplySuperNightData;
    // When the timed burst shooting and live photo mode are turned on, or AutoSN is off, the SN/SE
    // should not be triggerred.
    bool mPureSnapshot;

    // NOTE: hdr related parameters
    std::mutex mHdrStatusLock;
    FlashData mFlashData;
    std::atomic<uint8_t> mHdrMode;
    HdrRelatedData mHdrData;
    HdrRelatedData mApplyHdrData;

    // NOTE: sr related parameters
    std::atomic<float> mUIZoomRatio;
    std::atomic<bool> mIsSrNeeded;
    // NOTE: for details of this map, please refer to
    // packages/apps/MiuiCamera/src/com/android/camera2/CameraCapabilities.java
    std::unordered_map<int32_t /*camera role ID*/, float /*sr threshold for this camera*/>
        mSrRoleIdToThresholdMap;

    // NOTE: hdr+sr related parameters
    std::atomic<bool> mHdrSrDetected;
    int32_t mIsHdsrZslSupported;

    // NOTE: fusion related parameters
    bool mIsFusionSupported;
    std::atomic<bool> mIsFusionReady;

    // NOTE: this id denotes which ohysical camera is now being using
    std::atomic<int> mMasterId;

    // NOTE: dual bokeh related parameters
    uint32_t mBokehMasterId;
    uint32_t mBokehSlaveId;
    std::vector<int32_t> mBokehMadridEvData = {};
    bool mBokehMadridSupported;

    // NOTE: to record buffer consumption when do snapshot
    std::map<uint32_t /*stream id*/, std::atomic<int>> mBuffersConsumed;
    // NOTE: to save raw format of mihal stream config
    std::map<camera3_stream * /* raw stream */, int /* format */> mRawStreamFormat;

    int mMaxQuickShotCount;
    int mBokehMDMode;
    int mBokehRoleId;

    struct SuperHdData mSuperHdData;

    uint32_t mBufferLimitCnt;
    uint32_t mBufferDefaultLimitCnt;

    bool mIsVendorLowCaps;
    // QcZoomRatio = mUIZoomRatio * mFovCropZoomRatio
    float mFovCropZoomRatio;
    bool mIsSupportFovCrop;

private:
    int getPhotographerType(std::shared_ptr<CaptureRequest> fwkRequest);
    // NOTE: to check whether CameraMode's streams have enough buffers to handle next  snapshot
    bool canCamModeHandleNextSnapshot();
    bool IsRetired() { return mIsRetired.load(); }

    std::shared_ptr<StreamBuffer> tryGetAnchorFrameBuffer(uint32_t snapshotFrame,
                                                          uint32_t anchorFrame);

    void initConfigParameter()
    {
        if (isDualBokeh()) {
            auto entry = mSessionParameter.find(MI_CAMERA_BOKEH_MDMODE);
            if (entry.count > 0) {
                mBokehMDMode = entry.data.u8[0];
                MLOGD(LogGroupHAL, "bokehMDMode %d", mBokehMDMode);
            } else {
                MLOGW(LogGroupHAL, "get bokehMDMode failed!");
            }

            entry = mSessionParameter.find(MI_CAMERA_BOKEH_ROLE);
            if (entry.count > 0) {
                mBokehRoleId = entry.data.i32[0];
                MLOGD(LogGroupHAL, "bokehRoleId %d", mBokehRoleId);
            } else {
                MLOGE(LogGroupHAL, "get bokehRole failed!");
            }
        }
    }

    void initCapabilities();

    void updateSupportFovCrop(std::vector<std::shared_ptr<Stream>> &streams);
    // NOTE: these three functions are used to generate stream configuration sent to vendor
    virtual void buildInternalSnapshotStreamOverride(std::vector<std::shared_ptr<Stream>> &streams);
    void buildFakeSatSnapshotStream(std::vector<std::shared_ptr<Stream>> &streams);
    virtual void buildSessionParameterOverride();
    virtual void buildOpmodeOverride(uint32_t &opMode){};

    virtual std::shared_ptr<Photographer> createPhotographerOverride(
        std::shared_ptr<CaptureRequest> fwkRequest);
    virtual int bufferUsePredicForNextSnapshot(uint32_t streamId);
    InternalStreamInfoMap buildFinalInternalStreamInfoMap();

    virtual void getVendorStreamInfoMap(InternalStreamInfoMap &map)
    {
        map = MiviSettings::getVendorStreamInfoMapFromJson(
            "odm/etc/camera/xiaomi/mivivendorstreamconfig.json", mConfigFromFwk->getOpMode(),
            mVendorCamera->isFrontCamera() ? 1 : 0);
    }

    std::vector<int> getOverrideStreamFormat(int roleId, int phgType);
    bool supportFakeSat(int roleId);
    FakeSatSize getFakeSatImageSize(uint32_t roleId, uint32_t type);

    bool isMiui3rdCapture(std::shared_ptr<CaptureRequest> request)
    {
        auto entry = request->findInMeta(MI_MIVI_MIUI3RD);
        if (entry.count) {
            bool isMiui3rdCapture = entry.data.u8[0];
            MLOGD(LogGroupHAL,
                  "isMiui3rdCapture: %d, 1 means not need high quality noise reduction",
                  isMiui3rdCapture);
            return isMiui3rdCapture;
        }
        return false;
    }

    // NOTE: variables to cache the last status of CameraMode.
    std::atomic<bool> mCanHandleNextShot;
    std::mutex mCheckStatusLock;

    // NOTE: when a CameraMode is retired, its status is no longer useful for judging whether mihal
    // can handle next snapshot
    std::atomic<bool> mIsRetired;

    CameraRoleConfigInfoMap mCameraRoleConfigInfoMap;
    std::map<uint32_t, FakeSatThreshold> mFakeSatThreshold;

    std::vector<std::string> mUIRelatedMetaList; // UI Related
    ASDInfo mASDInfo;

    std::atomic<int> mLastPreviewFrame;

    // NOTE: in preprocess, some mialgo plugin may need preview meta
    std::mutex mPreprocessForcedMetaMutex;
    Metadata mPreprocessForcedMeta{};
};

} // namespace mihal

#endif
