#include "SupernightPhotographer.h"

#include <ui/GraphicBufferMapper.h>

#include "Camera3Plus.h"
#include "GraBuffer.h"
#include "Stream.h"
#include "VendorCamera.h"

namespace mihal {

SupernightPhotographer::SupernightPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest,
                                               Session *session, std::shared_ptr<Stream> outStream,
                                               std::shared_ptr<StreamConfig> darkroomConfig,
                                               SuperNightData *superNightData,
                                               Metadata extraMetadata)
    : Photographer{fwkRequest, session, extraMetadata, darkroomConfig},
      mNumOfVendorRequest{1},
      mSuperNightData{*superNightData}
{
    MLOGI(LogGroupHAL, "%s frame: %d, cameraId %d, stream %s", getName().c_str(),
          mFwkRequest->getFrameNumber(), mSuperNightData.cameraId, outStream->toString().c_str());

    initSuperNightInfo();

    uint32_t frame = getAndIncreaseFrameSeqToVendor(mNumOfVendorRequest);
    for (int i = 0; i < mNumOfVendorRequest; i++) {
        std::vector<std::shared_ptr<StreamBuffer>> outBuffers;

        // According to xiaomi.capabilities.MIVISuperNightSupportMask, for m2 value is 0x070707,
        // all mode supernight capture skip preview buffer.
        if (!mSuperNightData.skipPreviewBuffer) {
            outBuffers.push_back(getInternalQuickviewBuffer(frame + i));
        }

        std::shared_ptr<StreamBuffer> extraBuffer = outStream->requestBuffer();
        outBuffers.push_back(extraBuffer);

        auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame + i,
                                                      outBuffers, *fwkRequest->getMeta());

        int32_t totalFrames = mNumOfVendorRequest;
        request->updateMeta(MI_BURST_TOTAL_NUM, &totalFrames, 1);

        int32_t index = i + 1;
        request->updateMeta(MI_BURST_CURRENT_INDEX, &index, 1);

        // populate request settings
        applyDependencySettings(request.get(), mEv[i]);

        auto record = std::make_unique<RequestRecord>(this, std::move(request));
        mVendorRequests.insert({frame + i, std::move(record)});
    }
}

void SupernightPhotographer::initSuperNightInfo()
{
    mNumOfVendorRequest = mSuperNightData.superNightChecher.sum;
    memcpy(mEv, mSuperNightData.superNightChecher.ev, mNumOfVendorRequest * sizeof(int32_t));
    mSuperNightData.miviSupernightMode = MIVISuperNightNone;

    for (int i = 0; i < mNumOfVendorRequest; i++) {
        MLOGD(LogGroupCore, "cameraId=%d ev[%d]=%d", mSuperNightData.cameraId, i, mEv[i]);
    }
}

void SupernightPhotographer::applyDependencySettings(CaptureRequest *captureRequest, int32_t ev)
{
    Metadata *metadata = const_cast<Metadata *>(captureRequest->getMeta());
    LocalRequest *request = reinterpret_cast<LocalRequest *>(captureRequest);

    const uint8_t mode = mSuperNightData.miviSupernightMode;
    request->updateMeta(MI_MIVI_SUPERNIGHT_MODE, (const uint8_t *)&mode, 1);

    // for sll/ell, zslEnable = 0, to avoid wrong ev value
    uint8_t zslEnable = 0;
    request->updateMeta(ANDROID_CONTROL_ENABLE_ZSL, &zslEnable, 1);

    bool supportMfnrForSn = false;
    MiviSettings::getVal("FeatureList.FeatureSN.supportMfnrForSn", supportMfnrForSn, false);

    // enable sn
    uint8_t superNightEnable = 1;
    if (!ev && supportMfnrForSn) {
        // when ev0 need to do mfnr on front sn, sn algo should be disabled.
        superNightEnable = 0;
    }
    request->updateMeta(MI_SUPERNIGHT_ENABLED, &superNightEnable, 1);

    // disable mfnr
    uint8_t mfnrEnable = 0;
    uint8_t superNightMfnrEnable = 0;
    if (!ev && supportMfnrForSn) {
        // when ev0 need to do mfnr on front sn, mfnr should be enabled.
        mfnrEnable = 1;
        superNightMfnrEnable = 1;
    }
    request->updateMeta(MI_MFNR_ENABLE, &mfnrEnable, 1);
    request->updateMeta(MI_SUPERNIGHT_MFNR_ENABLED, &superNightMfnrEnable, 1);

    // ev set
    int32_t data = ev;
    request->updateMeta(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, (const int32_t *)&data, 1);

    // frame num set
    request->updateMeta(MI_MULTIFRAME_INPUTNUM, (const int32_t *)&mNumOfVendorRequest, 1);

    // ae bracket mode set
    const uint8_t aeBracketMode = 1;
    request->updateMeta(QCOM_AE_BRACKET_MODE, (const uint8_t *)&aeBracketMode, 1);

    // ae lock: rear lock at QcomHal but front lock at app, so need unified management
    const uint8_t aeLock = ANDROID_CONTROL_AE_LOCK_ON;
    request->updateMeta(ANDROID_CONTROL_AE_LOCK, (const uint8_t *)&aeLock, 1);
}

// NOTE: RawSupernightPhotographer is just used for raw sn
RawSupernightPhotographer::RawSupernightPhotographer(
    std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
    std::shared_ptr<Stream> outStream, std::shared_ptr<StreamConfig> darkroomConfig,
    SuperNightData *superNightData, Metadata extraMetadata)
    : Photographer{fwkRequest, session, extraMetadata, darkroomConfig},
      mNumOfVendorRequest{1},
      mSuperNightData{*superNightData}
{
    MLOGI(LogGroupHAL, "%s frame: %d, cameraId %d, stream %s", getName().c_str(),
          mFwkRequest->getFrameNumber(), mSuperNightData.cameraId, outStream->toString().c_str());

    initSuperNightInfo();

    uint32_t frame = getAndIncreaseFrameSeqToVendor(mNumOfVendorRequest);
    for (int i = 0; i < mNumOfVendorRequest; i++) {
        std::vector<std::shared_ptr<StreamBuffer>> outBuffers;

        // According to xiaomi.capabilities.MIVISuperNightSupportMask, for m2 value is 0x070707,
        // all mode supernight capture skip preview buffer.
        if (!mSuperNightData.skipPreviewBuffer) {
            outBuffers.push_back(getInternalQuickviewBuffer(frame + i));
        }

        std::shared_ptr<StreamBuffer> extraBuffer = outStream->requestBuffer();
        outBuffers.push_back(extraBuffer);

        auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame + i,
                                                      outBuffers, *fwkRequest->getMeta());

        int32_t totalFrames = mNumOfVendorRequest;
        request->updateMeta(MI_BURST_TOTAL_NUM, &totalFrames, 1);

        int32_t index = i + 1;
        request->updateMeta(MI_BURST_CURRENT_INDEX, &index, 1);

        if (outStream->getFormat() == HAL_PIXEL_FORMAT_RAW10) {
            uint8_t rawNonZslEnable = 1;
            request->updateMeta(MI_MIVI_RAW_NONZSL_ENABLE, &rawNonZslEnable, 1);
        }

        // populate request settings
        applyDependencySettings(request.get(), mEv[i]);

        auto record = std::make_unique<RequestRecord>(this, std::move(request));
        mVendorRequests.insert({frame + i, std::move(record)});
    }
}

void RawSupernightPhotographer::initSuperNightInfo()
{
    mNumOfVendorRequest = mSuperNightData.superNightChecher.sum;
    memcpy(mEv, mSuperNightData.superNightChecher.ev, mNumOfVendorRequest * sizeof(int32_t));
    updateSupernightMode();

    for (int i = 0; i < mNumOfVendorRequest; i++) {
        MLOGD(LogGroupCore, "cameraId=%d ev[%d]=%d", mSuperNightData.cameraId, i, mEv[i]);
    }
}

void RawSupernightPhotographer::updateSupernightMode()
{
    int sceneNum = sizeof(mSuperNightData.aiMisdScenes.aiMisdScene) / sizeof(AiMisdScene);

    for (int i = 0; i < sceneNum; i++) {
        AiMisdScene *aiMisdScene = mSuperNightData.aiMisdScenes.aiMisdScene + i;
        if (aiMisdScene->type == NightSceneSuperNight) {
            switch (aiMisdScene->value >> 8) {
            case NormalSE:
                mSuperNightData.miviSupernightMode = MIVISuperNightHand;
                break;
            case NormalELLC:
                mSuperNightData.miviSupernightMode = MIVISuperLowLightHand;
                break;
            default:
                MLOGE(LogGroupCore, "Unsupport value %d", aiMisdScene->value);
                break;
            }
        } else if (aiMisdScene->type == SuperNightSceneSNSC) {
            switch (aiMisdScene->value >> 8) {
            case NormalSNSC:
                mSuperNightData.miviSupernightMode = MIVINightMotionHand; // snsc
                mIsMiviAnchor = true;
                MLOGD(LogGroupCore, "[EarlyPic] mIsMiviAnchor %d", mIsMiviAnchor);
                break;
            case NormalSNSCNonZSL:
                mSuperNightData.miviSupernightMode = MIVINightMotionHandNonZSL; // snsc
                mIsMiviAnchor = true;
                MLOGD(LogGroupCore, "[EarlyPic] mIsMiviAnchor %d", mIsMiviAnchor);
                break;
            case NormalSNSCStg:
                mSuperNightData.miviSupernightMode = MIVINightMotionHandStg; // snsc
                break;
            default:
                MLOGE(LogGroupCore, "Unsupport value %d", aiMisdScene->value);
                break;
            }
        }
        MLOGD(LogGroupCore, "[SN]: miviSupernightMode %d", mSuperNightData.miviSupernightMode);
    }
}

void RawSupernightPhotographer::applyDependencySettings(CaptureRequest *captureRequest, int32_t ev)
{
    Metadata *metadata = const_cast<Metadata *>(captureRequest->getMeta());
    LocalRequest *request = reinterpret_cast<LocalRequest *>(captureRequest);

    IdealRawInfo idealRawInfo = {.isIdealRaw = 1, .idealRawType = 2};
    request->updateMeta(QCOM_RAW_CB_INFO_IDEALRAW, (const uint8_t *)&idealRawInfo,
                        sizeof(idealRawInfo));

    const uint8_t mode = mSuperNightData.miviSupernightMode;
    if (mode < MIVINightMotionHand) { // for sn
        request->updateMeta(MI_MIVI_SUPERNIGHT_MODE, (const uint8_t *)&mode, 1);
        // for sll/ell, zslEnable = 0, to avoid wrong ev value
        uint8_t zslEnable = 0;
        request->updateMeta(ANDROID_CONTROL_ENABLE_ZSL, &zslEnable, 1);
    } else if (mode >= MIVINightMotionHand && mode <= MIVINightMotionHandStg) {
        // for snsc, detial: https://xiaomi.f.mioffice.cn/docs/dock4npFQtcsQg8Il0r3RSTlA2e
        int32_t nightMotionMode = mode - MIVINightMotionHand + 1;
        request->updateMeta(MI_MIVI_NIGHT_MOTION_MODE, &nightMotionMode, 1);
        mFakeQuickShotEnabled = true;
    }

    // raw sn do not need to enable this tag
    uint8_t superNightEnable = 0;
    request->updateMeta(MI_SUPERNIGHT_ENABLED, &superNightEnable, 1);

    // disable mfnr
    uint8_t mfnrEnable = 0;
    uint8_t superNightMfnrEnable = 0;
    request->updateMeta(MI_MFNR_ENABLE, &mfnrEnable, 1);
    request->updateMeta(MI_SUPERNIGHT_MFNR_ENABLED, &superNightMfnrEnable, 1);

    // ev set
    int32_t data = ev;
    request->updateMeta(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, (const int32_t *)&data, 1);

    // frame num set
    request->updateMeta(MI_MULTIFRAME_INPUTNUM, (const int32_t *)&mNumOfVendorRequest, 1);

    // ae bracket mode set
    const uint8_t aeBracketMode = 1;
    request->updateMeta(QCOM_AE_BRACKET_MODE, (const uint8_t *)&aeBracketMode, 1);

    // ae lock: rear lock at QcomHal but front lock at app, so need unified management
    const uint8_t aeLock = ANDROID_CONTROL_AE_LOCK_ON;
    request->updateMeta(ANDROID_CONTROL_AE_LOCK, (const uint8_t *)&aeLock, 1);
}

} // namespace mihal
