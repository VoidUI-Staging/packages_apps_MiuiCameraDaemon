/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaImage.h"

#include <string.h>

#include "MiaSettings.h"

static const int32_t sIsDumpData =
    property_get_int32("persist.vendor.camera.algoengine.metadata.dump", 0);

namespace mialgo2 {

MiaImage::~MiaImage()
{
    if (mType != MI_IMG_INTERNAL) {
        return;
    }

    buffer_handle_t buffer = mBuffer.phHandle;
    uint32_t bufferSize = mBuffer.bufferSize;
    void *ptr = (void *)mBuffer.pAddr[0];
    if ((buffer != NULL) && (bufferSize > 0) && (ptr != NULL) &&
        (mState != MI_IMG_STATE_RELEASED)) {
        MiaBufferManager::putCPUAddress(buffer, bufferSize, ptr);
    }
    if (mBufferManager.get() && mInternalBufHandle) { // for internalBuffer
        mBufferManager->releaseImageBuffer(mInternalBufHandle);
        mBufferManager.clear();
        mBufferManager = NULL;
    }

    mState = MI_IMG_STATE_UNINITIALED;
    MLOGI(Mia2LogGroupCore,
          "release callback %p, type: %s, mFrameNum: %llu, timestamp: %lld, portId: %u", mFrameCb,
          getImageTypeName(mType), mFrameNum, mTimestamp, mPortId);
}

void MiaImage::releaseExternalFrame()
{
    if (mType == MI_IMG_INTERNAL || mState == MI_IMG_STATE_RELEASED) {
        return;
    }

    buffer_handle_t buffer = mBuffer.phHandle;
    uint32_t bufferSize = mBuffer.bufferSize;
    void *ptr = (void *)mBuffer.pAddr[0];
    if ((buffer != NULL) && (bufferSize > 0) && (ptr != NULL)) {
        /* this function is best executed before "mFrameCb->release",
           otherwise unknown situation may appear. */
        MiaBufferManager::putCPUAddress(buffer, bufferSize, ptr);
    }

    if (mFrameCb) {
        // releaseExternalFrame may be called multiple times,
        // but do only the first time releaseExternalFrame is called
        buffer_handle_t bufferHandle = mBuffer.phHandle;
        if (mType == MI_IMG_SOURCE) {
            mFrameCb->release(mFrameNum, mTimestamp, MiaFrameType::MI_FRAME_TYPE_INPUT, mPortId,
                              bufferHandle);
        } else if (mType == MI_IMG_SINK || mType == MI_IMG_VIRTUAL_SINK) {
            mFrameCb->release(mFrameNum, mTimestamp, MiaFrameType::MI_FRAME_TYPE_OUTPUT, mPortId,
                              bufferHandle);
        }
    }
    mState = MI_IMG_STATE_RELEASED;

    MLOGI(Mia2LogGroupCore,
          "release callback %p, type: %s, mFrameNum: %llu, timestamp: %lld, portId: %u", mFrameCb,
          getImageTypeName(mType), mFrameNum, mTimestamp, mPortId);
}

MiaImage::MiaImage(MiaFrame *miaFrame, MiaImageType type)
    : mBufferManager{nullptr},
      mMetaWraper{nullptr},
      mInternalBufHandle{nullptr},
      mBuffer{miaFrame->pBuffer},
      mFrameCb{miaFrame->callback},
      mFrameNum{miaFrame->frameNumber},
      mTimestamp{miaFrame->timeStampUs},
      mInputNumber{1},
      mTotalReqNum{0},
      mPortId{miaFrame->streamId},
      mFormat{miaFrame->format},
      mType{type},
      mState{MI_IMG_STATE_INITIALED}
{
    mUseCount = 1;

    uint32_t logicEntryCapacity = InnerMetadataEntryCapacity;
    uint32_t logicDataCapacity = InnerMetadataDataCapacity;
    uint32_t phyEntryCapacity = InnerMetadataEntryCapacity;
    uint32_t phyDataCapacity = InnerMetadataDataCapacity;

    if (mType == MI_IMG_SOURCE) {
        camera_metadata_t *logicMetadata = miaFrame->metaWraper->getLogicalMetadata();
        if (logicMetadata != NULL) {
            logicEntryCapacity = get_camera_metadata_entry_capacity(logicMetadata);
            logicDataCapacity = get_camera_metadata_data_capacity(logicMetadata);
        }
        camera_metadata_t *phyMetadata = miaFrame->metaWraper->getPhysicalMetadata();
        if (phyMetadata != NULL) {
            phyEntryCapacity = get_camera_metadata_entry_capacity(phyMetadata);
            phyDataCapacity = get_camera_metadata_data_capacity(phyMetadata);
        }
    }

    mMetaWraper = std::make_shared<DynamicMetadataWraper>(logicEntryCapacity, logicDataCapacity,
                                                          phyEntryCapacity, phyDataCapacity);
    mMetaWraper->updateMetadata(miaFrame->metaWraper);
    miaFrame->metaWraper.reset();

    LenOffsetInfo offsetInfo{};
    PaddingInfo padInfo{};
    if (mFormat.format == CAM_FORMAT_IMPLEMENTATION_DEFINED) {
        padInfo.width_padding = MI_PAD_TO_512;
        padInfo.height_padding = MI_PAD_TO_512;
        padInfo.plane_padding = MI_PAD_TO_512;
    } else if (mFormat.format == CAM_FORMAT_P010) {
        padInfo.width_padding = MI_PAD_TO_256;
        padInfo.height_padding = MI_PAD_TO_32;
        padInfo.plane_padding = MI_PAD_TO_1K;
    } else {
        padInfo.width_padding = MI_PAD_TO_64;
        padInfo.height_padding = MI_PAD_TO_64;
        padInfo.plane_padding = MI_PAD_TO_64;
        if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::MTK) {
            padInfo.height_padding = MI_PAD_NONE;
            padInfo.plane_padding = MI_PAD_NONE;
        }
    }

    mBuffer.bufferType = GRALLOC_TYPE;
    if (mBuffer.pAddr[0] == NULL) {
        buffer_handle_t buffer = mBuffer.phHandle;
        if (buffer == NULL) {
            mState = MI_IMG_STATE_UNINITIALED;
            // it's miuicamera output image, buffer will be filled when node acquire output
            // buffer
            MLOGI(Mia2LogGroupCore,
                  "buffer handle is null, it will be filled when node acquire output buffer");
            return;
        }
        mBuffer.bufferSize = MiaBufferManager::getBufferSize(buffer);
        void *address = MiaBufferManager::getCPUAddress(buffer, mBuffer.bufferSize);
        if (address == NULL) {
            mState = MI_IMG_STATE_UNINITIALED;
            MLOGE(Mia2LogGroupCore, "Failed: invalid address bufferSize: %u", mBuffer.bufferSize);
            return;
        }

        MiaUtil::calcOffset(mFormat, &padInfo, &offsetInfo);
        mBuffer.fd[0] = mBuffer.fd[1] = reinterpret_cast<const native_handle_t *>(buffer)->data[0];
        if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::MTK) {
            mBuffer.fd[0] = mBuffer.fd[1] =
                reinterpret_cast<const native_handle_t *>(buffer)->data[1];
        }

        uint8_t *frame = (uint8_t *)address;
        mBuffer.pAddr[0] = frame;
        mBuffer.pAddr[1] = frame + offsetInfo.mp[0].len;
        if (mFormat.format == CAM_FORMAT_Y16 || mFormat.format == CAM_FORMAT_RAW10 ||
            mFormat.format == CAM_FORMAT_RAW16) {
            mBuffer.fd[1] = -1;
            mBuffer.pAddr[1] = NULL;
        }

        MLOGD(Mia2LogGroupCore,
              "Image pAddr[0]=%p pAddr[1]=%p, fd:%d bufferSize: %d, frame_len: %d",
              mBuffer.pAddr[0], mBuffer.pAddr[1], mBuffer.fd[0], mBuffer.bufferSize,
              offsetInfo.frame_len);
    } else {
        // MiuiCamera
        MLOGI(Mia2LogGroupCore, "MiuiCamera case");
        MiaUtil::calcOffset(mFormat, &padInfo, &offsetInfo);
        mBuffer.pAddr[1] = mBuffer.pAddr[0] + offsetInfo.mp[0].len;
    }
    if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::MTK) {
        if (mFormat.format != CAM_FORMAT_P010) {
            mFormat.planeStride = PAD_TO_SIZE(mFormat.width, padInfo.width_padding);
            mFormat.sliceheight = PAD_TO_SIZE(mFormat.height, padInfo.height_padding);
        }
    } else {
        mFormat.planeStride = PAD_TO_SIZE(mFormat.width, padInfo.width_padding);
        mFormat.sliceheight = PAD_TO_SIZE(mFormat.height, padInfo.height_padding);
    }

    if (mFormat.format == CAM_FORMAT_Y16) {
        mBuffer.planes = 1;
    } else {
        mBuffer.planes = 2;
    }

    if (mFormat.format == CAM_FORMAT_RAW10 || mFormat.format == CAM_FORMAT_RAW16) {
        mBuffer.planes = 1;
        mFormat.planeStride = offsetInfo.mp[0].stride;
        mFormat.sliceheight = offsetInfo.mp[0].scanline;
    }

    camera_metadata_t *metadata = mMetaWraper->getLogicalMetadata();
    size_t settingsSize = get_camera_metadata_size(metadata);
    camera_metadata_t *phyCamMetadata = mMetaWraper->getPhysicalMetadata();
    size_t phySettingsSize = get_camera_metadata_size(phyCamMetadata);
    MLOGD(Mia2LogGroupCore,
          "get frameNumber: %d, metadata: %p, size: %d, phyMetadata: %p, size: %d", mFrameNum,
          metadata, settingsSize, phyCamMetadata, phySettingsSize);

    if (metadata != NULL) {
        void *pData = NULL;
        VendorMetadataParser::getTagValue(metadata, ANDROID_SENSOR_TIMESTAMP, &pData);
        if (NULL != pData) {
            int64_t timestamp = *static_cast<int64_t *>(pData);
            mTimestamp = timestamp;
        }

        pData = NULL;
        size_t count = 0;
        VendorMetadataParser::getVTagValueCount(metadata, "xiaomi.snapshot.imageName", &pData,
                                                &count);
        if (NULL != pData) {
            uint8_t *imageName = static_cast<uint8_t *>(pData);
            mImageName = std::string(imageName, imageName + count);
            MLOGI(Mia2LogGroupCore, "getMetaData imageName = %s", mImageName.c_str());
        }

        pData = NULL;
        VendorMetadataParser::getVTagValue(metadata, "xiaomi.multiframe.inputNum", &pData);
        if (NULL != pData) {
            uint32_t inputNum = *static_cast<unsigned int *>(pData);
            MLOGI(Mia2LogGroupCore, "get multiframe inputNum = %d", inputNum);
            mInputNumber = inputNum;
        }

        pData = NULL;
        VendorMetadataParser::getVTagValue(metadata, "xiaomi.burst.mialgoTotalReqNum", &pData);
        if (NULL != pData) {
            uint32_t totalReqNum = *static_cast<unsigned int *>(pData);
            MLOGI(Mia2LogGroupCore, "get totalReqNum = %d", totalReqNum);
            mTotalReqNum = totalReqNum;
        }
    }

    int32_t iIsDumpMetaData = sIsDumpData;
    if (iIsDumpMetaData && (metadata != NULL)) {
        char inputbuf[128];
        snprintf(inputbuf, sizeof(inputbuf), "%smialgo_dumpmeta_%d.txt", DUMP_DATA_FILE_PATH,
                 miaFrame->frameNumber);
        VendorMetadataParser::dumpMetadata(reinterpret_cast<const camera_metadata_t *>(metadata),
                                           inputbuf);
    }

    MLOGI(Mia2LogGroupCore,
          "Construct %s image cameraId:%d %dx%d format:%d streamId:%d timestamp: %" PRIu64,
          getImageTypeName(mType), mFormat.cameraId, mFormat.width, mFormat.height, mFormat.format,
          miaFrame->streamId, mTimestamp);
}

MiaImage::MiaImage(sp<MiaBufferManager> paramBufferManager, MiaImageFormat format)
    : mBufferManager{paramBufferManager},
      mMetaWraper{std::make_shared<DynamicMetadataWraper>()},
      mInternalBufHandle{nullptr},
      mBuffer{},
      mFrameCb{nullptr},
      mFrameNum{0},
      mTimestamp{0},
      mInputNumber{1},
      mPortId{0},
      mFormat{format},
      mType{MI_IMG_INTERNAL},
      mState{MI_IMG_STATE_INITIALED},
      mUseCount{0}
{
    LenOffsetInfo offsetInfo{};
    PaddingInfo padInfo{};
    if (format.format == CAM_FORMAT_IMPLEMENTATION_DEFINED) {
        padInfo.width_padding = MI_PAD_TO_512;
        padInfo.height_padding = MI_PAD_TO_512;
        padInfo.plane_padding = MI_PAD_TO_512;
    } else if (format.format == CAM_FORMAT_P010) {
        padInfo.width_padding = MI_PAD_TO_256;
        padInfo.height_padding = MI_PAD_TO_32;
        padInfo.plane_padding = MI_PAD_TO_1K;
    } else {
        padInfo.width_padding = MI_PAD_TO_64;
        padInfo.height_padding = MI_PAD_TO_64;
        padInfo.plane_padding = MI_PAD_TO_64;
        if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::MTK) {
            padInfo.height_padding = MI_PAD_NONE;
            padInfo.plane_padding = MI_PAD_NONE;
        }
    }

    if (paramBufferManager == NULL) {
        MLOGE(Mia2LogGroupCore, "paramBufferManager is NULL");
        return;
    }

    mInternalBufHandle = mBufferManager->getBuffer();

    MLOGD(Mia2LogGroupCore, "Mialgo Internal image, buffer_type: %d", mInternalBufHandle->type);

    void *address = NULL;
    if (mInternalBufHandle->type == GRALLOC_TYPE) {
        mBuffer.phHandle = mInternalBufHandle->grallocHandle.nativeHandle;
        mBuffer.bufferSize =
            MiaBufferManager::getBufferSize(mInternalBufHandle->grallocHandle.nativeHandle);
        address = MiaBufferManager::getCPUAddress(mInternalBufHandle->grallocHandle.nativeHandle,
                                                  mBuffer.bufferSize);
        mBuffer.fd[0] = mBuffer.fd[1] = reinterpret_cast<const native_handle_t *>(
                                            mInternalBufHandle->grallocHandle.nativeHandle)
                                            ->data[0];
        if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::MTK) {
            mBuffer.fd[0] = mBuffer.fd[1] = reinterpret_cast<const native_handle_t *>(
                                                mInternalBufHandle->grallocHandle.nativeHandle)
                                                ->data[1];
        }

    } else {
        mBuffer.phHandle = NULL;
        mBuffer.bufferSize = mInternalBufHandle->ionHandle.buffer_len;
        address = mInternalBufHandle->ionHandle.buffer;
        mBuffer.fd[0] = mBuffer.fd[1] = mInternalBufHandle->ionHandle.data_fd;
    }
    mBuffer.bufferType = mInternalBufHandle->type;
    if (address == NULL) {
        MLOGE(Mia2LogGroupCore, "Failed: invalid address");
        return;
    }

    CDKResult rc = MiaUtil::calcOffset(format, &padInfo, &offsetInfo);
    if (MIAS_OK != rc) {
        MLOGE(Mia2LogGroupCore, "Fail to new input  image: Fail to calculate offset\n");
        if (mInternalBufHandle->type == GRALLOC_TYPE) {
            MiaBufferManager::putCPUAddress(mInternalBufHandle->grallocHandle.nativeHandle,
                                            mBuffer.bufferSize, address);
            address = NULL;
        }
        return;
    }

    // mBuffer.planes = 2; // TODO
    mBuffer.planes = offsetInfo.num_planes; // TODO
    uint8_t *frame = (uint8_t *)address;
    if (mBuffer.planes == 2) {
        mBuffer.pAddr[0] = frame;
        mBuffer.pAddr[1] = frame + offsetInfo.mp[0].len;
    } else if (mBuffer.planes == 1) {
        mBuffer.pAddr[0] = frame;
    }

    if (mFormat.format == CAM_FORMAT_RAW10 || mFormat.format == CAM_FORMAT_RAW16) {
        mFormat.planeStride = offsetInfo.mp[0].stride;
        mFormat.sliceheight = offsetInfo.mp[0].scanline;
    } else if (mFormat.format == CAM_FORMAT_P010) {
        mFormat.planeStride = PAD_TO_SIZE(mFormat.width * 2, padInfo.width_padding);
        mFormat.sliceheight = PAD_TO_SIZE(mFormat.height, padInfo.height_padding);
    } else {
        mFormat.planeStride = PAD_TO_SIZE(mFormat.width, padInfo.width_padding);
        mFormat.sliceheight = PAD_TO_SIZE(mFormat.height, padInfo.height_padding);
    }

    MLOGI(Mia2LogGroupCore, "Construct %s image %dx%d format:%d, fd: %d", getImageTypeName(mType),
          format.width, format.height, format.format, mBuffer.fd[0]);
}

MiaImage::MiaImage(MiaImageFormat format, int64_t timestamp, uint32_t portId, MiaFrameCb *callback,
                   MiaImageType sinkType)
    : mBufferManager{nullptr},
      mMetaWraper{nullptr},
      mInternalBufHandle{nullptr},
      mBuffer{},
      mFrameCb{callback},
      mFrameNum{0},
      mTimestamp{timestamp},
      mInputNumber{1},
      mPortId{portId},
      mFormat{format},
      mType{sinkType},
      mState{MI_IMG_STATE_INITIALED}
{
    if (callback == NULL) {
        MLOGE(Mia2LogGroupCore, "Failed: MiaFrameCb is null");
        return;
    }

    mUseCount = 1;

    LenOffsetInfo offsetInfo{};
    PaddingInfo padInfo{};
    if (format.format == CAM_FORMAT_IMPLEMENTATION_DEFINED) {
        padInfo.width_padding = MI_PAD_TO_512;
        padInfo.height_padding = MI_PAD_TO_512;
        padInfo.plane_padding = MI_PAD_TO_512;
    } else if (format.format == CAM_FORMAT_P010) {
        padInfo.width_padding = MI_PAD_TO_256;
        padInfo.height_padding = MI_PAD_TO_32;
        padInfo.plane_padding = MI_PAD_TO_1K;
    } else {
        padInfo.width_padding = MI_PAD_TO_64;
        padInfo.height_padding = MI_PAD_TO_64;
        padInfo.plane_padding = MI_PAD_TO_64;
    }

    buffer_handle_t buffer = NULL;
    int ret = callback->getBuffer(timestamp, (OutSurfaceType)mPortId, &buffer);
    if (ret != 0 || buffer == NULL) {
        mState = MI_IMG_STATE_UNINITIALED;
        MLOGE(Mia2LogGroupCore, "Failed: buffer handle is null");
        return;
    }

    mBuffer.bufferType = GRALLOC_TYPE;
    mBuffer.phHandle = buffer;
    mBuffer.bufferSize = MiaBufferManager::getBufferSize(buffer);
    MLOGI(Mia2LogGroupCore, "buffer: %p, bufferSize: %d", buffer, mBuffer.bufferSize);

    void *address = MiaBufferManager::getCPUAddress(buffer, mBuffer.bufferSize);
    if (address == NULL) {
        MLOGE(Mia2LogGroupCore, "Failed: invalid address bufferSize: %u", mBuffer.bufferSize);
        return;
    }
    MiaUtil::calcOffset(format, &padInfo, &offsetInfo);
    mBuffer.fd[0] = mBuffer.fd[1] = reinterpret_cast<const native_handle_t *>(buffer)->data[0];
    if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::MTK) {
        mBuffer.fd[0] = mBuffer.fd[1] = reinterpret_cast<const native_handle_t *>(buffer)->data[1];
    }

    uint8_t *frame = (uint8_t *)address;
    mBuffer.pAddr[0] = frame;
    mBuffer.pAddr[1] = frame + offsetInfo.mp[0].len;
    if (mFormat.format == CAM_FORMAT_Y16 || mFormat.format == CAM_FORMAT_RAW10 ||
        mFormat.format == CAM_FORMAT_RAW16) {
        mBuffer.fd[1] = -1;
        mBuffer.pAddr[1] = NULL;
    }

    MLOGI(Mia2LogGroupCore,
          "pAddr[0]=%p pAddr[1]=%p, fd: %d, bufferSize: %d, frame_len: %d, padding: %d",
          mBuffer.pAddr[0], mBuffer.pAddr[1], mBuffer.fd[0], mBuffer.bufferSize,
          offsetInfo.frame_len, padInfo.width_padding);

    mFormat.planeStride = PAD_TO_SIZE(mFormat.width, padInfo.width_padding);
    mFormat.sliceheight = PAD_TO_SIZE(mFormat.height, padInfo.height_padding);
    if (mFormat.format == CAM_FORMAT_Y16) {
        mBuffer.planes = 1;
    }
    if (mFormat.format == CAM_FORMAT_RAW10 || mFormat.format == CAM_FORMAT_RAW16) {
        mBuffer.planes = 1;
        mFormat.planeStride = offsetInfo.mp[0].stride;
        mFormat.sliceheight = offsetInfo.mp[0].scanline;
    } else {
        mBuffer.planes = 2;
    }

    if (mType == MI_IMG_VIRTUAL_SINK) {
        mMetaWraper = std::make_shared<DynamicMetadataWraper>();
    }

    mFrameCb = callback;
    MLOGI(Mia2LogGroupCore,
          "Construct %s image[%d] %dx%d format:0x%x, timestamp: %" PRIu64 ", portId: %d",
          getImageTypeName(mType), mPortId, mFormat.width, mFormat.height, mFormat.format,
          mTimestamp, portId);
}

void MiaImage::copyParams(const sp<MiaImage> &image, int64_t timeStamp, int mergeNum)
{
    mTimestamp = timeStamp;
    mFrameNum = image->mFrameNum;
    mImageName = image->getImageName();
    mBuffer.reserved = image->mBuffer.reserved;
    setMergeInputNumber(mergeNum); // this image will as childNode inputImage

    if (mType == MI_IMG_INTERNAL || mType == MI_IMG_VIRTUAL_SINK) {
        auto srcMeta = image->getMetadataWraper();
        mMetaWraper->updateMetadata(srcMeta);
    }
}

const char *MiaImage::getImageTypeName(int type)
{
    if (type == MI_IMG_SOURCE) {
        return "Source";
    } else if (type == MI_IMG_INTERNAL) {
        return "Internal";
    } else if (type == MI_IMG_SINK) {
        return "Sink";
    } else if (type == MI_IMG_VIRTUAL_SINK) {
        return "VirtualSink";
    } else {
        return "Nnknow";
    }
}

} // namespace mialgo2
