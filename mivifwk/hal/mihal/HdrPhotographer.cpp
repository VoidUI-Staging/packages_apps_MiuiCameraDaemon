#include "HdrPhotographer.h"

#include <sstream>

#include "MiviSettings.h"

namespace mihal {

HdrPhotographer::HdrPhotographer(std::shared_ptr<CaptureRequest> fwkRequest, Session *session,
                                 std::shared_ptr<Stream> extraStream, HdrRelatedData &hdrData,
                                 const Metadata extraMeta,
                                 std::shared_ptr<StreamConfig> darkroomConfig, int cameraMode)
    : Photographer{fwkRequest, session, extraMeta, darkroomConfig},
      mExpValues{hdrData.hdrStatus.expVaules},
      mRawExpValues{hdrData.hdrStatus.rawExpVaules},
      mHDRTye{IMPL_BY_MIALGO},
      mCameraMode{cameraMode},
      mRoleId{CameraDevice::RoleIdTypeNone}
{
    forceDonotAttachPreview();

    MiviSettings::getVal("FeatureList.FeatureHDR.shouldApplyAElock", mShouldApplyAeLock, false);

    if (extraStream->getFormat() != HAL_PIXEL_FORMAT_YCBCR_420_888) {
        mHDRTye = IMPL_BY_MIALGO_RAW;
    }
    int numOfReq = numOfReqSent2Vendor();

    uint32_t frame = getAndIncreaseFrameSeqToVendor(numOfReq);

    int cameraId = atoi(extraStream->getPhyId());
    auto camera = CameraManager::getInstance()->getCameraDevice(cameraId);
    if (NULL != camera) {
        mRoleId = camera->getRoleId();
    }
    MLOGD(LogGroupHAL, "[HDR]: numOfReq is: %d, hdrType is: %d, roleId is %d", numOfReq, mHDRTye,
          mRoleId);

    for (int i = 0; i < numOfReq; ++i) {
        std::vector<std::shared_ptr<StreamBuffer>> outBuffers;

        // NOTE: We send quickview buffer to vendor, and vendor will return error buffer
        if (!skipQuickview())
            outBuffers.push_back(getInternalQuickviewBuffer(frame + i));

        outBuffers.push_back(extraStream->requestBuffer());

        auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame + i,
                                                      outBuffers, *fwkRequest->getMeta());

        // NOTE: post update some meta
        postUpdateReqMeta(request, i, hdrData);

        auto record = std::make_unique<RequestRecord>(this, std::move(request));
        mVendorRequests.insert({frame + i, std::move(record)});
    }
}

bool HdrPhotographer::skipQuickview()
{
    if (mHDRTye == IMPL_BY_MIALGO_RAW || (mHDRTye == IMPL_BY_MIALGO) ||
        Session::SyncAlgoSessionType == getSessionType()) {
        return true;
    } else {
        return false;
    }
}

int HdrPhotographer::numOfReqSent2Vendor()
{
    switch (mHDRTye) {
    case IMPL_BY_MIALGO:
        return mExpValues.size();
    case IMPL_BY_MIALGO_RAW:
        return mRawExpValues.size();

    // NOTE: HDR implemented in vendor, for example, stagger HDR, in such cases, we only need to
    // send one request to vendor
    case IMPL_BY_VENDOR:
    case IMPL_BY_VENDOR_WITH_FUSION:
    default:
        return 1;
    }
}

// NOTE: the logic below is just a reiteration of MIVI1.0 app logic, if you are brave enough, you
// can refer to: packages/apps/MiuiCamera/src/com/android/camera2/MiCamera2ShotParallelBurst.java
// @applyHdrParameter the code there is totally a mess
void HdrPhotographer::postUpdateReqMeta(std::unique_ptr<LocalRequest> &request, int batchIndex,
                                        HdrRelatedData &hdrData)
{
    switch (mHDRTye) {
    case IMPL_BY_MIALGO:
    case IMPL_BY_MIALGO_RAW:
        switch (mCameraMode) {
        // TODO: for single mode(aka: front camera, phycamID = 1) HDR, each request in the batch
        // need to set different meta
        case NORMAL_MODE: {
            int32_t index = batchIndex + 1;
            request->updateMeta(MI_BURST_CURRENT_INDEX, &index, 1);

            int32_t totalSequenceNum = mExpValues.size();
            if (mHDRTye == IMPL_BY_MIALGO_RAW) {
                totalSequenceNum = mRawExpValues.size();
            }

            request->updateMeta(MI_BURST_TOTAL_NUM, &totalSequenceNum, 1);
            request->updateMeta(MI_MULTIFRAME_INPUTNUM, &totalSequenceNum, 1);

            uint8_t bracketMode = 1;
            request->updateMeta(QCOM_AE_BRACKET_MODE, &bracketMode, 1);

            if (mShouldApplyAeLock) {
                MLOGD(LogGroupHAL, "[HDR]: ev[%d] should aply AE lock", batchIndex);
                uint8_t aeLock = ANDROID_CONTROL_AE_LOCK_ON;
                request->updateMeta(ANDROID_CONTROL_AE_LOCK, &aeLock, 1);
            }
            int32_t ev = 0;
            uint8_t hdrEnable = false;
            uint8_t rawHdrEnable = false;
            if (mHDRTye == IMPL_BY_MIALGO) {
                hdrEnable = true;
                rawHdrEnable = false;
                ev = mExpValues[batchIndex];
            } else if (mHDRTye == IMPL_BY_MIALGO_RAW) {
                hdrEnable = false;
                rawHdrEnable = true;
                ev = mRawExpValues[batchIndex];
            }
            request->updateMeta(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &ev, 1);
            MLOGD(LogGroupHAL, "[HDR]: ev[%d] value is: %d, ", batchIndex, ev);

            request->updateMeta(MI_HDR_ENABLE, &hdrEnable, 1);
            request->updateMeta(MI_RAWHDR_ENABLE, &rawHdrEnable, 1);
            request->updateMeta(MI_HDR_CHECKER_SCENE_TYPE, &hdrData.hdrSceneType, 1);
            MLOGD(LogGroupHAL, "[HDR]: ev[%d] hdr scene type is: %d, ", batchIndex,
                  hdrData.hdrSceneType);

            request->updateMeta(MI_HDR_CHECKER_ADRC, &hdrData.hdrAdrc, 1);
            MLOGD(LogGroupHAL, "[HDR]: ev[%d] hdr adrc is: %d, ", batchIndex, hdrData.hdrAdrc);

            uint8_t hdrSrEnable = 0;
            request->updateMeta(MI_HDR_SR_ENABLE, &hdrSrEnable, 1);

            uint8_t *snapshotInfo = hdrData.snapshotReqInfo.data;
            request->updateMeta(MI_AI_ASD_SNAPSHOT_REQ_INFO, snapshotInfo,
                                hdrData.snapshotReqInfo.size);

            uint8_t isZSLHDR = hdrData.isZslHdrEnabled;
            request->updateMeta(MI_HDR_ZSL, &isZSLHDR, 1);

            uint8_t mfnrEnabled = 0;
            uint8_t zslEnable = 1;

            if (mRoleId == CameraDevice::RoleIdRearWide ||
                mRoleId == CameraDevice::RoleIdRearUltra) {
                zslEnable = 0;
            }
            if (Session::AsyncAlgoSessionType == getSessionType() &&
                mHDRTye != IMPL_BY_MIALGO_RAW) {
                // NOTE: if tag "xiaomi.hdr.hdrFrameReq" is not defined, then set zsl and mfnr for
                // ev=0 frame as default. if tag "xiaomi.hdr.hdrFrameReq" is defined, then set zsl
                // and mfnr for the frame whose corresponding value in "xiaomi.hdr.hdrFrameReq" is 1
                // if ((hdrData.reqMfnrSetting.empty() && ev == 0) ||
                //     (!hdrData.reqMfnrSetting.empty() && hdrData.reqMfnrSetting[batchIndex])) {
                //     mfnrEnabled = 1;
                //     mIsMFNRHDR = true;
                // }
                if (batchIndex == 0) {
                    mfnrEnabled = 1;
                    zslEnable = 1;
                }
            } else if (Session::SyncAlgoSessionType == getSessionType()) {
                zslEnable = 0;
            }
            request->updateMeta(MI_MFNR_ENABLE, &mfnrEnabled, 1);
            request->updateMeta(ANDROID_CONTROL_ENABLE_ZSL, &zslEnable, 1);
            MLOGD(LogGroupHAL, "[HDR]: ev[%d] mfnr: %d, zsl: %d", batchIndex, mfnrEnabled,
                  zslEnable);

            // TODO: maybe need to handle some MTK platform corner case

            break;
        }
        case SDK_MODE: {
            uint8_t hdrEnable = true;
            request->updateMeta(MI_HDR_ENABLE, &hdrEnable, 1);

            int32_t index = batchIndex + 1;
            request->updateMeta(MI_BURST_CURRENT_INDEX, &index, 1);

            int32_t totalSequenceNum = mExpValues.size();
            request->updateMeta(MI_BURST_TOTAL_NUM, &totalSequenceNum, 1);
            request->updateMeta(MI_MULTIFRAME_INPUTNUM, &totalSequenceNum, 1);

            int32_t ev = mExpValues[batchIndex];
            request->updateMeta(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &ev, 1);
            MLOGD(LogGroupHAL, "[HDR]: ev[%d] value is: %d, ", batchIndex, ev);

            request->updateMeta(MI_HDR_CHECKER_SCENE_TYPE, &hdrData.hdrSceneType, 1);
            MLOGD(LogGroupHAL, "[HDR]: ev[%d] hdr scene type is: %d, ", batchIndex,
                  hdrData.hdrSceneType);

            request->updateMeta(MI_HDR_CHECKER_ADRC, &hdrData.hdrAdrc, 1);
            MLOGD(LogGroupHAL, "[HDR]: ev[%d] hdr adrc is: %d, ", batchIndex, hdrData.hdrAdrc);

            uint8_t hdrSrEnable = 0;
            request->updateMeta(MI_HDR_SR_ENABLE, &hdrSrEnable, 1);

            uint8_t *snapshotInfo = hdrData.snapshotReqInfo.data;
            request->updateMeta(MI_AI_ASD_SNAPSHOT_REQ_INFO, snapshotInfo,
                                hdrData.snapshotReqInfo.size);

            break;
        }
        }
        break;
    case IMPL_BY_VENDOR:
    case IMPL_BY_VENDOR_WITH_FUSION:
        uint8_t hdrEnable = true;
        request->updateMeta(MI_HDR_ENABLE, &hdrEnable, 1);

        int32_t multiFrameNum = 1;
        request->updateMeta(MI_MULTIFRAME_INPUTNUM, &multiFrameNum, 1);

        // NOTE: we need to set this tag because Qcom vendor need MFNR to provide high quality
        // ev0 and ev+ pictures. For details, please refer to HDR training video in xiaomi kpan.
        uint8_t mfnrEnabled = 1;
        mIsMFNRHDR = true;
        request->updateMeta(MI_MFNR_ENABLE, &mfnrEnabled, 1);
        break;
    }
}

std::string HdrPhotographer::toString() const
{
    std::ostringstream str{Photographer::toString(), std::ios_base::ate};

    str << "\n\t\tEVs:";
    for (int32_t ev : mExpValues) {
        str << " " << ev;
    }

    return str.str();
}

} // namespace mihal
