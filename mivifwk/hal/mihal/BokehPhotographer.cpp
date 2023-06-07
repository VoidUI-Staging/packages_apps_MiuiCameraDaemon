#include "BokehPhotographer.h"

namespace mihal {

static const int32_t defaultRearEvArray[METADATA_VALUE_NUMBER_MAX] = {-27, -12, 0, 0, 0, 0, 0, 0,
                                                                      0,   0,   0, 0, 0, 0, 0, 0};
static const int32_t defaultFrontEvArray[METADATA_VALUE_NUMBER_MAX] = {-18, -12, -6, 0, 0, 0, 0, 0,
                                                                       0,   0,   0,  0, 0, 0, 0, 0};

BokehPhotographer::BokehPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest,
                                     Session *session,
                                     std::vector<std::shared_ptr<Stream>> extraStreams,
                                     std::shared_ptr<StreamConfig> darkroomConfig,
                                     const Metadata extraMeta)
    : Photographer{fwkRequest, session, extraMeta, darkroomConfig}
{
    uint32_t frame = getAndIncreaseFrameSeqToVendor(1);

    // TODO: to be compatible with fwkRequest which has preview streams
    std::vector<std::shared_ptr<StreamBuffer>> outBuffers;

    for (int i = 0; i < mFwkRequest->getOutBufferNum(); i++) {
        auto buffer = mFwkRequest->getStreamBuffer(i);
        if (buffer->getStream()->isConfiguredToHal()) {
            outBuffers.push_back(buffer);
        }
    }

    for (auto stream : extraStreams) {
        std::shared_ptr<StreamBuffer> extraBuffer = stream->requestBuffer();
        outBuffers.push_back(extraBuffer);
    }

    // FIXME: here we could be able to avoid a meta copy
    auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame, outBuffers,
                                                  *fwkRequest->getMeta());

    uint8_t mfnrEnable = 0;
    if (isSupportMfnrBokeh()) {
        mfnrEnable = 1;
    }
    request->updateMeta(MI_MFNR_ENABLE, &mfnrEnable, 1);

    uint8_t hdrEnable = 0;
    request->updateMeta(MI_BOKEH_HDR_ENABLE, &hdrEnable, 1);

    auto record = std::make_unique<RequestRecord>(this, std::move(request));
    mVendorRequests.insert({frame, std::move(record)});
}

MDBokehPhotographer::MDBokehPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest,
                                         Session *session,
                                         std::vector<std::shared_ptr<Stream>> extraStreams,
                                         std::vector<int32_t> &expValues,
                                         std::shared_ptr<StreamConfig> darkroomConfig,
                                         const Metadata extraMeta)
    : Photographer{fwkRequest, session, extraMeta, darkroomConfig}
{
    forceDonotAttachPreview();

    uint32_t frames = expValues.size();
    uint32_t frame = getAndIncreaseFrameSeqToVendor(frames);

    // NOTE: if enableBokehOptimize is opened, then in hdr, MD, se bokeh, only anchor frame will
    // take slave stream buffer to qcom
    // WARNING: for hdr and MD, frame 0 is forced to be anchor frame
    int enableBokehOptimize;
    MiviSettings::getVal("enableBokehOptimize", enableBokehOptimize, 0);

    for (int i = 0; i < frames; i++) {
        // TODO: to be compatible with fwkRequest which has preview streams
        std::vector<std::shared_ptr<StreamBuffer>> outBuffers;

        // FIX:NUWA-23328:if enableBokehOptimize is true, then MdBokeh none-anchor frame need to
        // take preview buffer; otherwise, all requests don't need to take preview buffer
        if (enableBokehOptimize && i > 0) {
            outBuffers.push_back(getInternalQuickviewBuffer(frame + i));
        }

        std::shared_ptr<StreamBuffer> extraBuffer = extraStreams[0]->requestBuffer();
        outBuffers.push_back(extraBuffer);

        // NOTE: if enableBokehOptimize is true, then only the anchor frame need to take slave
        // buffers; otherwise, all requests need to take slave buffers
        if (!enableBokehOptimize || (enableBokehOptimize && i == 0)) {
            for (size_t j = 1; j < extraStreams.size(); j++) {
                std::shared_ptr<StreamBuffer> extraBuffer = extraStreams[j]->requestBuffer();
                outBuffers.push_back(extraBuffer);
            }
        }

        // FIXME: here we could be able to avoid a meta copy
        auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame + i,
                                                      outBuffers, *fwkRequest->getMeta());

        uint8_t hdrEnable = 0;
        request->updateMeta(MI_BOKEH_HDR_ENABLE, &hdrEnable, 1);

        int32_t evValue = expValues[i];
        // WORKAROUND: avoid setting ev0 to hal params when users set ev in madrid mode
        if (evValue != 0) {
            request->updateMeta(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &evValue, 1);
        }

        uint8_t zslEnable = ANDROID_CONTROL_ENABLE_ZSL_TRUE;
        if (evValue != 0) {
            zslEnable = ANDROID_CONTROL_ENABLE_ZSL_FALSE;
        }
        request->updateMeta(ANDROID_CONTROL_ENABLE_ZSL, &zslEnable, 1);

        uint8_t aeLock = ANDROID_CONTROL_AE_LOCK_ON;
        request->updateMeta(ANDROID_CONTROL_AE_LOCK, &aeLock, 1);

        uint8_t mfnrEnable = 0;
        if (isSupportMfnrBokeh() && evValue == 0) {
            mfnrEnable = 1;
        }
        request->updateMeta(MI_MFNR_ENABLE, &mfnrEnable, 1);

        int32_t numFrames = frames;
        request->updateMeta(MI_MULTIFRAME_INPUTNUM, &numFrames, 1);

        MLOGD(LogGroupHAL, "frame: %d evValue[%d]=%d zslEnable=%d mfnrEnable=%d", frame + i, i,
              evValue, zslEnable, mfnrEnable);

        auto record = std::make_unique<RequestRecord>(this, std::move(request));
        mVendorRequests.insert({frame + i, std::move(record)});
    }
}

HdrBokehPhotographer::HdrBokehPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest,
                                           Session *session,
                                           std::vector<std::shared_ptr<Stream>> extraStreams,
                                           std::vector<int32_t> &expValues,
                                           std::shared_ptr<StreamConfig> darkroomConfig)
    : Photographer{fwkRequest, session, darkroomConfig}
{
    forceDonotAttachPreview();

    uint32_t frames = expValues.size();
    uint32_t frame = getAndIncreaseFrameSeqToVendor(frames);

    // NOTE: if enableBokehOptimize is opened, then in hdr, MD, se bokeh, only anchor frame will
    // take slave stream buffer to qcom
    // WARNING: for hdr and MD, frame 0 is forced to be anchor frame
    int enableBokehOptimize;
    MiviSettings::getVal("enableBokehOptimize", enableBokehOptimize, 0);

    for (int i = 0; i < frames; i++) {
        std::vector<std::shared_ptr<StreamBuffer>> outBuffers;

        // FIX:NUWA-23328:if enableBokehOptimize is true, then HdrBokeh none-anchor frame need to
        // take preview buffer; otherwise, all requests don't need to take preview buffer
        if (enableBokehOptimize && i > 0) {
            outBuffers.push_back(getInternalQuickviewBuffer(frame + i));
        }

        std::shared_ptr<StreamBuffer> extraBuffer = extraStreams[0]->requestBuffer();
        outBuffers.push_back(extraBuffer);

        // NOTE: if enableBokehOptimize is true, then only the anchor frame need to take slave
        // buffers; otherwise, all requests need to take slave buffers
        if (!enableBokehOptimize || (enableBokehOptimize && i == 0)) {
            for (size_t j = 1; j < extraStreams.size(); j++) {
                std::shared_ptr<StreamBuffer> extraBuffer = extraStreams[j]->requestBuffer();
                outBuffers.push_back(extraBuffer);
            }
        }

        auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame + i,
                                                      outBuffers, *fwkRequest->getMeta());

        int32_t totalFrames = frames;
        request->updateMeta(MI_BURST_TOTAL_NUM, &totalFrames, 1);

        int32_t index = i + 1;
        request->updateMeta(MI_BURST_CURRENT_INDEX, &index, 1);

        uint8_t hdrEnable = true;
        request->updateMeta(MI_BOKEH_HDR_ENABLE, &hdrEnable, 1);

        int32_t evValue = expValues[i];
        request->updateMeta(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &evValue, 1);

        uint8_t zslEnable = ANDROID_CONTROL_ENABLE_ZSL_TRUE;
        if (evValue != 0) {
            zslEnable = ANDROID_CONTROL_ENABLE_ZSL_FALSE;
        }
        request->updateMeta(ANDROID_CONTROL_ENABLE_ZSL, &zslEnable, 1);

        int32_t numFrames = frames;
        request->updateMeta(MI_MULTIFRAME_INPUTNUM, &numFrames, 1);

        uint8_t mfnrEnable = 0;
        if (isSupportMfnrBokeh() && evValue == 0) {
            mfnrEnable = 1;
        }
        request->updateMeta(MI_MFNR_ENABLE, &mfnrEnable, 1);

        MLOGD(LogGroupHAL, "frame: %d evValue[%d]=%d zslEnable=%d mfnrEnable=%d", frame + i, i,
              evValue, zslEnable, mfnrEnable);

        if (!extraStreams.empty()) {
            ImageRect cropRegion = {.left = 0,
                                    .top = 0,
                                    .width = static_cast<int32_t>(extraStreams[0]->getWidth()),
                                    .height = static_cast<int32_t>(extraStreams[0]->getHeight())};
            request->updateMeta(ANDROID_SCALER_CROP_REGION, (const int32_t *)&cropRegion, 4);
        }

        auto record = std::make_unique<RequestRecord>(this, std::move(request));
        mVendorRequests.insert({frame + i, std::move(record)});
    }
}

// https://xiaomi.f.mioffice.cn/docs/dock4Zp1gn3CJsnHMDqdxN6J7Oc
SEBokehPhotographer::SEBokehPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest,
                                         Session *session,
                                         std::vector<std::shared_ptr<Stream>> outStreams,
                                         std::shared_ptr<StreamConfig> darkroomConfig,
                                         SuperNightData *superNightData)
    : Photographer{fwkRequest, session, darkroomConfig}, mSuperNightData{*superNightData}
{
    forceDonotAttachPreview();

    mNumOfVendorRequest = MAX_REAR_REQUEST_NUM;
    memcpy(mEv, defaultRearEvArray, MAX_REAR_REQUEST_NUM * sizeof(int32_t));

    initSuperNightInfo();

    uint32_t frame = getAndIncreaseFrameSeqToVendor(mNumOfVendorRequest);

    // NOTE: if enableBokehOptimize is opened, then in hdr, MD, se bokeh, only anchor frame will
    // take slave stream buffer to qcom
    // WARNING: for se bokeh, frame 2 is forced to be anchor frame
    int enableBokehOptimize;
    MiviSettings::getVal("enableBokehOptimize", enableBokehOptimize, 0);

    for (int i = 0; i < mNumOfVendorRequest; i++) {
        std::vector<std::shared_ptr<StreamBuffer>> outBuffers;

        // FIX:NUWA-23328:if enableBokehOptimize is true, then SeBokeh all frame need to take
        // preview buffer; otherwise, all requests don't need to take preview buffer
        if (enableBokehOptimize && (i != 2)) {
            outBuffers.push_back(getInternalQuickviewBuffer(frame + i));
        }

        std::shared_ptr<StreamBuffer> extraBuffer = outStreams[0]->requestBuffer();
        outBuffers.push_back(extraBuffer);

        // NOTE: if enableBokehOptimize is true, then only the anchor frame need to take slave
        // buffers; otherwise, all requests need to take slave buffers
        if (!enableBokehOptimize || (enableBokehOptimize && i == 2)) {
            for (size_t j = 1; j < outStreams.size(); j++) {
                std::shared_ptr<StreamBuffer> extraBuffer = outStreams[j]->requestBuffer();
                outBuffers.push_back(extraBuffer);
            }
        }

        auto request = std::make_unique<LocalRequest>(fwkRequest->getCamera(), frame + i,
                                                      outBuffers, *fwkRequest->getMeta());

        int32_t totalFrames = mNumOfVendorRequest;
        request->updateMeta(MI_BURST_TOTAL_NUM, &totalFrames, 1);

        int32_t index = i + 1;
        request->updateMeta(MI_BURST_CURRENT_INDEX, &index, 1);

        if (!outStreams.empty()) {
            ImageRect cropRegion = {.left = 0,
                                    .top = 0,
                                    .width = static_cast<int32_t>(outStreams[0]->getWidth()),
                                    .height = static_cast<int32_t>(outStreams[0]->getHeight())};
            request->updateMeta(ANDROID_SCALER_CROP_REGION, (const int32_t *)&cropRegion, 4);
        }

        // populate request settings
        applyDependencySettings(request.get(), mEv[i]);

        auto record = std::make_unique<RequestRecord>(this, std::move(request));
        mVendorRequests.insert({frame + i, std::move(record)});
    }
}

void SEBokehPhotographer::applyDependencySettings(CaptureRequest *captureRequest, int32_t ev)
{
    status_t ret;
    Metadata *metadata = const_cast<Metadata *>(captureRequest->getMeta());
    LocalRequest *request = reinterpret_cast<LocalRequest *>(captureRequest);

    uint8_t hdrEnable = 0;
    request->updateMeta(MI_BOKEH_HDR_ENABLE, &hdrEnable, 1);

    uint8_t bokehSupernightEnable = 1;
    request->updateMeta(MI_BOKEH_SUPERNIGHT_ENABLE, &bokehSupernightEnable, 1);

    // enable mfnr
    uint8_t mfnrEnable = 0;
    request->updateMeta(MI_MFNR_ENABLE, &mfnrEnable, 1);

    // it set by ExtensionModule::SetRawCbInfo
    IdealRawInfo idealRawInfo = {.isIdealRaw = 0, .idealRawType = 0};
    request->updateMeta(QCOM_RAW_CB_INFO_IDEALRAW, (const uint8_t *)&idealRawInfo,
                        sizeof(idealRawInfo));

    // ev set
    request->updateMeta(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, (const int32_t *)&ev, 1);

    request->updateMeta(MI_MULTIFRAME_INPUTNUM, (const int32_t *)&mNumOfVendorRequest, 1);

    const uint8_t mode = mSuperNightData.miviSupernightMode;
    request->updateMeta(MI_MIVI_SUPERNIGHT_MODE, (const uint8_t *)&mode, 1);
}

void SEBokehPhotographer::initSuperNightInfo()
{
    mNumOfVendorRequest = mSuperNightData.superNightChecher.sum;
    memcpy(mEv, mSuperNightData.superNightChecher.ev, mNumOfVendorRequest * sizeof(int32_t));
    updateSupernightMode();

    for (int i = 0; i < mNumOfVendorRequest; i++) {
        MLOGD(LogGroupCore, "cameraId %d, ev[%d]=%d", mSuperNightData.cameraId, i, mEv[i]);
    }
}

void SEBokehPhotographer::updateSupernightMode()
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
        }
    }
}

} // namespace mihal
