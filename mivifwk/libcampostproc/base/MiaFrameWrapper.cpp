/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaFrameWrapper.h"

#include "VendorMetadataParser.h"

namespace mialgo {

MiaFrameWrapper::~MiaFrameWrapper()
{
    if (mFrame.result_metadata != NULL) {
        VendorMetadataParser::freeMetadata((camera_metadata_t *)mFrame.result_metadata);
        mFrame.result_metadata = NULL;
    }

    if (mFrame.phycam_metadata != NULL) {
        VendorMetadataParser::freeMetadata((camera_metadata_t *)mFrame.phycam_metadata);
        mFrame.phycam_metadata = NULL;
    }

    if (mFrame.callback != NULL) {
        MLOGD("Release input release %p, frame_number: %d", mFrame.callback, mFrame.frame_number);
        mFrame.callback->release(&mFrame);
        delete mFrame.callback;
        mFrame.callback = NULL;
    }
}

/*
 *   Construct in this way, it will always be input
 */
MiaFrameWrapper::MiaFrameWrapper(MiaFrame *frameHandle) : mFormat(frameHandle->mibuffer.format)
{
    mInputNumber = 1;
    mFrame = *frameHandle;
    mTimestampUs = 0;
    mRequestId = frameHandle->requestId;

    camera_metadata_t *result = NULL;
    camera_metadata_t *phyCamResult = NULL;

    if (frameHandle->result_metadata)
        result = VendorMetadataParser::allocateCopyMetadata(frameHandle->result_metadata);

    if (frameHandle->phycam_metadata) {
        phyCamResult = VendorMetadataParser::allocateCopyMetadata(frameHandle->phycam_metadata);
    }

    if (result != NULL) {
        void *data = NULL;
        VendorMetadataParser::getTagValue(result, ANDROID_SENSOR_TIMESTAMP, &data);
        if (NULL != data) {
            int64_t timestamp = *static_cast<int64_t *>(data);
            mTimestampUs = timestamp;
            MLOGD("getMetaData timestamp %ld", timestamp);
        }
        if (frameHandle->request_metadata != NULL) {
            camera_metadata_t *appRequestMeta =
                VendorMetadataParser::allocateCopyMetadata(frameHandle->request_metadata);
            mergeAppRequestSetting(appRequestMeta, result);
            if (NULL != appRequestMeta) {
                VendorMetadataParser::freeMetadata(appRequestMeta);
                appRequestMeta = NULL;
            }
        }
    }

    mFrame.result_metadata = result;
    mFrame.phycam_metadata = phyCamResult;

    MLOGI("RequestId %d, frameNum %lu, format: 0x%x, W*H(%d*%d) result %p, phyCamResult: %p",
          mRequestId, mFrame.frame_number, mFormat.format, mFormat.width, mFormat.height, result,
          phyCamResult);
}

int32_t MiaFrameWrapper::mergeAppRequestSetting(camera_metadata_t *src, camera_metadata_t *&dest)
{
    CDKResult rc = MIAS_OK;
    if (src == NULL || dest == NULL) {
        MLOGI("mergeAppRequestSetting src: %p, dest: %p", src, dest);
        return MIAS_INVALID_PARAM;
    }

    void *data = NULL;
    bool hdrEnable = false;
    VendorMetadataParser::getVTagValue(src, "xiaomi.hdr.enabled", &data);
    if (NULL != data) {
        hdrEnable = *static_cast<uint8_t *>(data);
        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.hdr.enabled", &hdrEnable, 1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.hdr.enabled", &hdrEnable, 1,
                                                  metadata);
        }
    }
    data = NULL;
    bool rawHdrEnable = false;
    VendorMetadataParser::getVTagValue(src, "xiaomi.hdr.raw.enabled", &data);
    if (NULL != data) {
        rawHdrEnable = *static_cast<uint8_t *>(data);
    }
    if (VendorMetadataParser::setVTagValue(dest, "xiaomi.hdr.raw.enabled", &rawHdrEnable, 1)) {
        camera_metadata_t **metadata = &dest;
        // need resize
        VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.hdr.raw.enabled", &rawHdrEnable, 1,
                                              metadata);
        MLOGI("update rawHdr enable value: %d", rawHdrEnable);
    }

    data = NULL;
    bool mfnrEnable = false;
    VendorMetadataParser::getVTagValue(src, "xiaomi.mfnr.enabled", &data);
    if (NULL != data) {
        mfnrEnable = *static_cast<uint8_t *>(data);
        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.mfnr.enabled", &mfnrEnable, 1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.mfnr.enabled", &mfnrEnable, 1,
                                                  metadata);
        }
    }

    data = NULL;
    bool depurpleEnable = false;
    VendorMetadataParser::getVTagValue(src, "xiaomi.depurple.enabled", &data);
    if (NULL != data) {
        depurpleEnable = *static_cast<uint8_t *>(data);
        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.depurple.enabled", &depurpleEnable,
                                               1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.depurple.enabled",
                                                  &depurpleEnable, 1, metadata);
        }
    }

    data = NULL;
    unsigned int flipEnable = 1;
    VendorMetadataParser::getVTagValue(src, "xiaomi.flip.enabled", &data);
    if (NULL != data) {
        flipEnable = *static_cast<unsigned int *>(data);
        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.flip.enabled", &flipEnable, 1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.flip.enabled", &flipEnable, 1,
                                                  metadata);
        }
    }

    data = NULL;
    bool hdrBokehEnable = false;
    VendorMetadataParser::getVTagValue(src, "xiaomi.bokeh.hdrEnabled", &data);
    if (NULL != data) {
        hdrBokehEnable = *static_cast<uint8_t *>(data);
        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.bokeh.hdrEnabled", &hdrBokehEnable,
                                               1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.bokeh.hdrEnabled",
                                                  &hdrBokehEnable, 1, metadata);
        }
    }

    data = NULL;
    int lightMode = 0;
    VendorMetadataParser::getVTagValue(src, "xiaomi.portrait.lighting", &data);
    if (NULL != data) {
        lightMode = *static_cast<unsigned int *>(data);
        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.portrait.lighting", &lightMode, 1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.portrait.lighting", &lightMode,
                                                  1, metadata);
        }
    }

    data = NULL;
    int inputNum = 1;
    VendorMetadataParser::getVTagValue(src, "xiaomi.multiframe.inputNum", &data);
    if (NULL != data) {
        inputNum = *static_cast<unsigned int *>(data);
        mInputNumber = inputNum;
        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.multiframe.inputNum", &inputNum, 1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.multiframe.inputNum",
                                                  &inputNum, 1, metadata);
        }
    }

    data = NULL;
    int srEnable = 0;
    VendorMetadataParser::getVTagValue(src, "xiaomi.superResolution.enabled", &data);
    if (NULL != data) {
        srEnable = *static_cast<int *>(data);
        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.superResolution.enabled", &srEnable,
                                               1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.superResolution.enabled",
                                                  &srEnable, 1, metadata);
        }
    }

    data = NULL;
    bool captureFusionEnable = false;
    VendorMetadataParser::getVTagValue(src, "xiaomi.capturefusion.isFusionOn", &data);
    if (NULL != data) {
        captureFusionEnable = *static_cast<uint8_t *>(data);
        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.capturefusion.isFusionOn",
                                               &captureFusionEnable, 1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.capturefusion.isFusionOn",
                                                  &captureFusionEnable, 1, metadata);
        }
    }

    data = NULL;
    bool hdrSrEnable = false;
    VendorMetadataParser::getVTagValue(src, "xiaomi.hdr.sr.enabled", &data);
    if (NULL != data) {
        hdrSrEnable = *static_cast<uint8_t *>(data);
        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.hdr.sr.enabled", &hdrSrEnable, 1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.hdr.sr.enabled", &hdrSrEnable,
                                                  1, metadata);
        }
    }

    data = NULL;
    VendorMetadataParser::getTagValue(src, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &data);
    if (NULL != data) {
        int32_t frameEv = *static_cast<int32_t *>(data);
        MLOGI("get frameEv: %d", frameEv);
        if (VendorMetadataParser::setTagValue(dest, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                              &frameEv, 1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateTagValue(
                *metadata, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &frameEv, 1, metadata);
        }
    }

    data = NULL;
    int burstEnable = 0;
    VendorMetadataParser::getVTagValue(src, "xiaomi.burst.captureHint", &data);
    if (NULL != data) {
        burstEnable = *static_cast<int *>(data);
    }
    if (VendorMetadataParser::setVTagValue(dest, "xiaomi.burst.captureHint", &burstEnable, 1)) {
        camera_metadata_t **metadata = &dest;
        // need resize
        VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.burst.captureHint", &burstEnable,
                                              1, metadata);
    }

    void *pData = NULL;
    size_t count = 0;
    VendorMetadataParser::getVTagValueExt(src, "xiaomi.snapshot.imageName", &pData, count);
    if (NULL != pData) {
        uint8_t *imageName = static_cast<uint8_t *>(pData);
        std::string name(imageName, imageName + count);
        MLOGI("Get image name: %s, size: %d", name.c_str(), count);
        mFileName = name;

        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.snapshot.imageName", imageName,
                                               count)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.snapshot.imageName", imageName,
                                                  count, metadata);
        }
    }

    data = NULL;
    uint32_t superNightMode = 0;
    VendorMetadataParser::getVTagValue(src, "xiaomi.mivi.supernight.mode", &data);
    if (NULL != data) {
        superNightMode = *static_cast<uint32_t *>(data);
        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.mivi.supernight.mode", &superNightMode,
                                               1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.mivi.supernight.mode",
                                                  &superNightMode, 1, metadata);
        }
    }

    data = NULL;
    uint32_t nightMotionMode = 0;
    VendorMetadataParser::getVTagValue(src, "xiaomi.nightmotioncapture.mode", &data);
    if (NULL != data) {
        nightMotionMode = *static_cast<uint32_t *>(data);
        if (VendorMetadataParser::setVTagValue(dest, "xiaomi.nightmotioncapture.mode",
                                               &nightMotionMode, 1)) {
            camera_metadata_t **metadata = &dest;
            // need resize
            VendorMetadataParser::updateVTagValue(*metadata, "xiaomi.nightmotioncapture.mode",
                                                  &nightMotionMode, 1, metadata);
        }
    }

    MLOGI(
        "Get hdrEnable:%d, mfnrEnable:%d, depurpleEnable:%d, flipEnable:%d, "
        "hdrBokehEnable:%d,lightMode:%d, inputNum:%d, srEnable:%d, captureFusionEnable:%d, "
        "hdrSrEnable:%d, burstEnable:%d,superNightMode:%d, rawHdrEnable:%d",
        hdrEnable, mfnrEnable, depurpleEnable, flipEnable, hdrBokehEnable, lightMode, inputNum,
        srEnable, captureFusionEnable, hdrSrEnable, burstEnable, superNightMode, rawHdrEnable);
    return rc;
}

} // namespace mialgo
