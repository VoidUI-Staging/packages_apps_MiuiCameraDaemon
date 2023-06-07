/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _MIZONE_MICAMDECISION_H_
#define _MIZONE_MICAMDECISION_H_

#include <functional>
#include <queue>

#include "MiBufferManager.h"
#include "MiCapture.h"
#include "MiZoneTypes.h"

namespace mizone {
class MiAlgoCamMode;
class MiBufferManager;

class MiCamDecision
{
public:
    struct CreateInfo
    {
        std::shared_ptr<MiConfig> configFromFwk;
        MiAlgoCamMode *camMode;
        CameraInfo cameraInfo;
    };

protected:
    struct StreamParameter
    {
        uint32_t cameraId;
        float ratio;
        bool isPhysicalStream;
        EImageFormat format;
        bool needYuvR3Stream;
    };

    enum AlgoType {
        MI_ALGO_NONE = 0,
        MI_ALGO_VENDOR_MFNR = 1,
        MI_ALGO_MIHAL_MFNR = 1 << 1,
        MI_ALGO_HDR = 1 << 2,
        MI_ALGO_VENDOR_MFNR_HDR = MI_ALGO_VENDOR_MFNR + MI_ALGO_HDR,
        MI_ALGO_MIHAL_MFNR_HDR = MI_ALGO_MIHAL_MFNR + MI_ALGO_HDR,
        MI_ALGO_SUPERNIGHT = 1 << 3,
        MI_ALGO_BOKEH = 1 << 4,
        MI_ALGO_BOKEH_VENDOR_MFNR = MI_ALGO_BOKEH + MI_ALGO_VENDOR_MFNR,
        MI_ALGO_BOKEH_MIHAL_MFNR = MI_ALGO_BOKEH + MI_ALGO_MIHAL_MFNR,
        MI_ALGO_BOKEH_HDR = MI_ALGO_BOKEH + MI_ALGO_HDR,
        MI_ALGO_BOKEH_SUPERNIGHT = MI_ALGO_BOKEH + MI_ALGO_SUPERNIGHT,
        MI_ALGO_SR = 1 << 5,
        MI_ALGO_SR_HDR = MI_ALGO_SR + MI_ALGO_HDR,
        MI_ALGO_FUSION = 1 << 6,
        MI_ALGO_FUSION_SR = MI_ALGO_FUSION + MI_ALGO_SR,
    };

    enum MotionAlgoType {
        TYPE_DEFAULT, // 默认算法类型
        TYPE_MFNR,    // 平台MFNR降噪算法
        TYPE_AIS1,    // 平台AIS1降噪算法
        TYPE_AIS2,    // 平台AIS2降噪算法,比AIS1有更好的动态范围
        TYPE_AINR,    // 平台AINR降噪算法
    };

    enum MotionCaptureEnableMask {
        MASK_CAPTURE_MODE_ENABLE = 0x1, // 拍照模式运动抓拍是否使能的mask
        MASK_CAPTURE_MODE_DENOISE_ENABLE = 0x2, // 拍照模式运动抓拍专用降噪算法是否使能的mask
        MASK_CAPTURE_MODE_HDR_ENABLE = 0x4, // 拍照模式运动抓拍hdr算法是否使能的mask
        MASK_CAPTURE_MODE_REPLACE_SR_WITH_MFNR = 0x8, // 运动场景下是否用MFNR替换SR
        MASK_PORTRAIT_MODE_ENABLE = 0x100,            // 人像模式运动抓拍是否使能的mask
        MASK_PORTRAIT_MODE_DENOISE_ENABLE = 0x200, // 人像模式运动抓拍专用降噪算法是否使能的mask
        MASK_PORTRAIT_MODE_HDR_ENABLE = 0x400, // 人像模式运动抓拍hdr算法是否使能的mask

        MASK_DEFAULT_BEHAVIOR_SHIFT = 11, // 默认控制行为位在第11个bit上
        MASK_DEFAULT_BEHAVIOR = 0x3 << MASK_DEFAULT_BEHAVIOR_SHIFT, // bit[11:12]控制默认行为的mask
        /***MTK 标志位从bit16开始***/
        MASK_MTK_AISHUTTER_VERSION_ONE_ENABLE = 0x10000, // mtk新的aishutter 1.0实现方式
        MASK_MTK_AISHUTTER_VERSION_TWO_ENABLE = 0x20000, // mtk新的aishutter 2.0实现方式
    };

    /**
     * Bit Mask Of HighQualityQuickShot Support
     * Bit[0]		 - Support MFSR/LLS in SAT mode
     * Bit[1]		 - Support HDR in SAT mode
     * Bit[2]		 - Support SR in SAT mode
     * Bit[3]		 - Support SuperNightSE in SAT mode
     * Bit[4~7]   - reserve
     * Bit[8]		 - Support Bokeh MFNR in Back Camera
     * Bit[9]		 - Support Bokeh HDR in Back Camera
     * Bit[10] 	- Support MFNR in Front Camera
     * Bit[11] 	- Support HDR in Front Camera
     * Bit[12] 	- Support Bokeh in Front Camera
     * Bit[13] 	- Support  Macro Mode
     * Bit[14~15]	 - reserve
     * Bit[16~19]	 - HighQualityQuickShot queue length(max number of HighQualityQuickShot)
     * Bit[20] 	 - Support reuse RDI buffer or not
     * Bit[21] 	 - Support Limit MFNR input frames or not
     * Bit[22~31]	 - reserve
     **/
    enum HighQualityQuickShotType {
        QS_SUPPORT_NONE = 0,
        QS_SUPPORT_SAT_MFNR = 0x1,
        QS_SUPPORT_SAT_HDR = 0x2,
        QS_SUPPORT_SAT_SR = 0x4,
        QS_SUPPORT_SAT_SUPER_NIGHT = 0x8,
        QS_SUPPORT_BACK_BOKEH_MFNR = 0x100,
        QS_SUPPORT_BACK_BOKEH_HDR = 0x200,
        QS_SUPPORT_FRONT_MFNR = 0x400,
        QS_SUPPORT_FRONT_HDR = 0x800,
        QS_SUPPORT_FRONT_BOKEH = 0x1000,
        QS_SUPPORT_MACRO_MODE = 0x2000,
        QS_QUEUE_LENGTH_MASK = 0xF0000,
    };

    /**
     *Bit Mask for HighQualityQuickShotDelayTime configure
     *Bit[0 ~ 3]   - DelayTime XX ms of Bokeh MFNR in Back Camera
     *Bit[4 ~ 7]   - DelayTime XX ms of Bokeh MFNR in Front Camera
     *Bit[8 ~ 11]  - DelayTime XX ms of Back Normal Capture
     *Bit[12 ~ 15] - DelayTime XX ms of HDR in Front Camera
     *Bit[16 ~ 19] - DelayTime XX ms of HDR in Back Camera
     *Bit[20 ~ 23] - DelayTime XX ms of SuperNightSe in Back Camera
     *Bit[24 ~ 27] - DelayTime XX ms of SR in Back Camera
     *Bit[28 ~ 31] - DelayTime XX ms of Front Normal Capture
     *Bit[32 ~ 35] - DelayTime XX ms of MacroMode in Back Camera
     *Bit[36 ~ 39] - DelayTime XX ms of Bokeh HDR in Back Camera
     *Bit[40 ~ 63] - reserv
     */
    enum HighQualityQuickShotDelayTime {
        QS_DELAYTIME_NONE = 0,
        QS_MI_ALGO_BACK_BOKEH_MIHAL_MFNR_DELAY_TIME = 0xFL,
        QS_MI_ALGO_FRONT_BOKEH_MIHAL_MFNR_DELAY_TIME = 0xF0L,
        QS_MI_ALGO_BACK_MIHAL_MFNR_CAPTURE_DELAY_TIME = 0xF00L,
        QS_MI_ALGO_FRONT_HDR_DELAY_TIME = 0xF000L,
        QS_MI_ALGO_BACK_HDR_DELAY_TIME = 0xF0000L,
        QS_MI_ALGO_BACK_SUPER_NIGHT_SE_DELAY_TIME = 0xF00000L,
        QS_MI_ALGO_BACK_SR_CAPTURE_DELAY_TIME = 0xF000000L,
        QS_MI_ALGO_FRONT_NORMAL_CAPTURE_DELAY_TIME = 0xF0000000L,
        QS_MI_ALGO_BACK_MIHAL_AINR_CAPTURE_DELAY_TIME = 0xF00000000L,
        QS_MI_ALGO_FRONT_HDR_HELF_DELAY_TIME = 0xF000000000L,
    };

    enum HdrType {
        NOMAL_HDR = 0,
        VENDOR_MFNR_HDR = MI_ALGO_VENDOR_MFNR,
        MIHAL_MFNR_HDR = MI_ALGO_MIHAL_MFNR,
    };

public:
    static std::unique_ptr<MiCamDecision> createDecision(const CreateInfo &info);
    virtual ~MiCamDecision();

protected:
    MiCamDecision(const CreateInfo &info);

public:
    virtual std::shared_ptr<MiConfig> buildMiConfig();
    virtual std::shared_ptr<MiCapture> buildMiCapture(CaptureRequest &fwkRequest);
    virtual void updateSettings(const MiMetadata &settings);
    virtual void updateStatus(const MiMetadata &status);

    std::map<int32_t, int32_t> getStreamIdToFwkMap() const { return mStreamIdToFwkMap; }

    std::string getMiCamDecisionName();

protected:
    virtual bool buildMiConfigOverride();
    // for mtk query feature,input app metadata of latest preview,return framecount,output metadata
    // setting
    int queryFeatureSetting(const MiMetadata &info, uint32_t &frameCount,
                            std::vector<MiMetadata> &settings);

    void initFeatureSetting();

    std::shared_ptr<MiStream> createBasicMiStream(const Stream &srcStream,
                                                  StreamUsageType usageType, StreamOwner owner,
                                                  int32_t fwkStreamId);
    std::shared_ptr<MiStream> createSnapshotMiStream(StreamParameter &para, int overrideWidth = -1,
                                                     int overrideHeight = -1);
    void composeRawStream(Stream &stream);
    void composeYuvStream(Stream &stream);
    void composeYuvoR3Stream(Stream &stream);
    MiSize getPhysicalResolution(int32_t cameraId);

    void updateAlgoMetaInCapture(MiCapture::CreateInfo &capInfo);
    std::shared_ptr<MiStream> getPhyStream(EImageFormat format, int32_t phyId = -1) const;

    virtual bool buildMiCaptureOverride(MiCapture::CreateInfo &capInfo);
    virtual void determineAlgoType();
    virtual bool updateMetaInCapture(MiCapture::CreateInfo &capInfo, bool isZsl,
                                     int32_t *ev = nullptr);
    virtual void updateMetaInCaptureOverride(MiCapture::CreateInfo &capInfo){};

    virtual void updateSettingsOverride(const MiMetadata &settings) {}
    virtual void updateStatusOverride(const MiMetadata &status) {}
    virtual void updateSettingsInSnapShot(const MiMetadata &settings) {}

    bool composeCapInfoSingleNone(MiCapture::CreateInfo &capInfo,
                                  EImageFormat format = eImgFmt_NV21, bool isZsl = true);
    bool composeCapInfoVendorMfnr(MiCapture::CreateInfo &capInfo);
    bool composeCapInfoMihalMfnr(MiCapture::CreateInfo &capInfo);
    // For app sanpshot control
    void setCaptureTypeName(MiCapture::CreateInfo &capInfo);
    virtual void updateMialgoType();
    void updateQuickSnapshot(MiCapture::CreateInfo &capInfo);
    uint64_t getQuickShotDelayTime();
    bool isSupportQuickShot() const;

    bool isRawFormat(const EImageFormat &format);

    bool getMotionAlgoType();
    bool isCurrentModeSupportAISDenoise();
    bool isCurrentModeSupportHdrAIS();
    bool isMtkAiShutterVersionOne()
    {
        return 0 != (mAiShutterSupport & MASK_MTK_AISHUTTER_VERSION_ONE_ENABLE);
    }
    bool isMtkAiShutterVersionTwo()
    {
        return 0 != (mAiShutterSupport & MASK_MTK_AISHUTTER_VERSION_TWO_ENABLE);
    }
    bool isCaptureAiShutterEnable() { return 0 != (mAiShutterSupport & MASK_CAPTURE_MODE_ENABLE); }
    bool isPortraitAiShutterEnable()
    {
        return 0 != (mAiShutterSupport & MASK_PORTRAIT_MODE_ENABLE);
    }
    bool isCaptureAiShutterHDREnable()
    {
        return (0 != (mAiShutterSupport & MASK_CAPTURE_MODE_HDR_ENABLE)) &&
               (0 != (mAiShutterSupport & MASK_CAPTURE_MODE_ENABLE));
    }
    bool isPortraitAiShutterHDREnable()
    {
        return (0 != (mAiShutterSupport & MASK_PORTRAIT_MODE_HDR_ENABLE)) &&
               (0 != (mAiShutterSupport & MASK_PORTRAIT_MODE_ENABLE));
    }
    bool isCaptureAiShutterDenoiseEnable()
    {
        return (0 != (mAiShutterSupport & MASK_CAPTURE_MODE_DENOISE_ENABLE)) &&
               (0 != (mAiShutterSupport & MASK_CAPTURE_MODE_ENABLE));
    }
    bool isPortraitAiShutterDenoiseEnable()
    {
        return (0 != (mAiShutterSupport & MASK_PORTRAIT_MODE_DENOISE_ENABLE)) &&
               (0 != (mAiShutterSupport & MASK_PORTRAIT_MODE_ENABLE));
    }
    bool isNeedReplaceSrWithMfnr()
    {
        return 0 != (mAiShutterSupport & MASK_CAPTURE_MODE_REPLACE_SR_WITH_MFNR);
    }

    uint32_t mOperationMode;
    uint32_t mAlgoType;
    uint32_t mSupportAlgoMask;
    float mFrameRatio;

    CameraInfo mCamInfo;

    int32_t mSpecshotMode; // 0:mfnr >=1:ainr
    bool mIsAinr;

    uint8_t mIsQuickSnapshot;

    // for AIS
    uint8_t mAiShutterEnable; // aishutter icon in app
    int32_t mAiShutterSupport;
    bool mIsMotionExisted;
    uint32_t mMotionAlgoType;

    uint8_t mFlashMode; // open flash mfnr
    uint8_t mFlashState;
    uint8_t mAeMode;

    int32_t mAdjustEv;

    uint8_t mAiEnable;

    MiAlgoCamMode *mCurrentCamMode;

    std::shared_ptr<MiConfig> mFwkConfig; // create in CamMode
    std::shared_ptr<MiConfig> mVndConfig; // create in Decision

    int32_t mFwkCapStreamId;

    std::map<int32_t, int32_t> mStreamIdToFwkMap;

    int32_t mVndOwnedStreamId = STREAM_ID_VENDOR_START;
    std::shared_ptr<MiStream> mPreviewMiStream;
    std::shared_ptr<MiStream> mQuickViewMiStream;
    std::shared_ptr<MiStream> mQrMiStream;
    std::shared_ptr<MiStream> mQuickViewCachedMiStream;
    std::shared_ptr<MiStream> mYuvR3MiStream;
    std::string mCaptureTypeName;
    int32_t mMaxQuickShotCount;
    uint64_t mQuickShotDelayTimeMask;
    uint32_t mQuickShotSupportedMask;
    bool mIsBurstCapture;
    bool mIsHeifFormat;

    // algo fuction map: Key: AlgoType  Value: composeCapInfoXXX function
    std::map<int, std::function<bool(MiCapture::CreateInfo &)>> mAlgoFuncMap;

    MiMetadata mPreviewCaptureResult;
    bool mContainQuickViewBuffer;
};

} // namespace mizone

#endif //_MIZONE_MICAMDECISION_H_
