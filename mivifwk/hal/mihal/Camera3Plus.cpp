#include "Camera3Plus.h"

#include <sync/sync.h>

#include <sstream>

#include "CameraDevice.h"
#include "ExifUtils.h"
#include "JpegCompressor.h"
#include "LogUtil.h"
#include "MiGrallocMapper.h"
#include "MiHal3BufferMgr.h"
#include "Session.h"

namespace mihal {

using MetadataType = android::hardware::graphics::mapper::V4_0::IMapper::MetadataType;

StreamBuffer::StreamBuffer(const camera3_stream_buffer *sbuffer)
    : mRaw{*sbuffer},
      mBuffer{std::make_unique<GraBuffer>(sbuffer->stream->width, sbuffer->stream->height,
                                          sbuffer->stream->format, sbuffer->stream->usage,
                                          *sbuffer->buffer)}
{
}

StreamBuffer::StreamBuffer(std::unique_ptr<GraBuffer> buffer) : mRaw{}, mBuffer{std::move(buffer)}
{
}

StreamBuffer &StreamBuffer::operator=(const camera3_stream_buffer &other)
{
    mRaw = other;
    mBuffer = std::make_unique<GraBuffer>(other.stream->width, other.stream->height,
                                          other.stream->format, other.stream->usage, *other.buffer);

    return *this;
}

StreamBuffer &StreamBuffer::operator=(const StreamBuffer &other)
{
    auto stream = other.getStream();
    auto buffer = other.getBuffer();
    mBuffer =
        std::make_unique<GraBuffer>(stream->getWidth(), stream->getHeight(), stream->getFormat(),
                                    stream->getUsage(), buffer->getHandle());
    mRaw = other.mRaw;

    return *this;
}

int StreamBuffer::moveGraBufferFrom(StreamBuffer &&other)
{
    mBuffer = std::move(other.mBuffer);
    mRaw = other.mRaw;
    other.mRaw.buffer = GraBuffer::getEmptyHandlePtr();

    return 0;
}

int StreamBuffer::setLatedBuffer(const StreamBuffer &buffer)
{
    if (mBuffer)
        return -EEXIST;

    Stream *stream = getStream();
    if (!stream->getCamera()->supportBufferManagerAPI())
        return -EFAULT;

    auto cachedBuffer = stream->getRequestedStreamBuffer(buffer);
    if (!cachedBuffer) {
        MLOGE(LogGroupHAL, "failed to get cached buffer for stream:%p", stream->toRaw());
        operator=(buffer);
        return -ENOENT;
    }

    moveGraBufferFrom(std::move(*cachedBuffer));

    return 0;
}

int StreamBuffer::setLatedBuffer(const camera3_stream_buffer &buffer)
{
    Stream *stream = getStream();
    if (!stream->getCamera()->supportBufferManagerAPI())
        return -EFAULT;

    mBuffer =
        std::make_unique<GraBuffer>(buffer.stream->width, buffer.stream->height,
                                    buffer.stream->format, buffer.stream->usage, *buffer.buffer);
    mRaw = buffer;

    return 0;
}

int StreamBuffer::waitAndCloseAcquireFence()
{
    int fd = mRaw.acquire_fence;
    int ret = 0;
    if (fd != -1) {
        ret = sync_wait(fd, 500);
        if (ret) {
            MLOGE(LogGroupHAL, "wait acquire fence %d failed:ret=%d, errno=%d(%s), stream:%s", fd,
                  ret, -errno, strerror(errno), getStream()->toString(4).c_str());
        }
        close(fd);
    }

    return ret;
}

int StreamBuffer::prepareBuffer()
{
    int ret = 0;

    // NOTE: if buffer handle is already there, then just return OK
    if (mBuffer && mBuffer->getHandle()) {
        return ret;
    }

    auto camera = getStream()->getCamera();
    auto buffers = camera->getFwkStreamHal3BufMgr()->getBuffer(getStream()->toRaw(), 1);
    if (buffers.empty()) {
        ret = -1;
        MLOGW(LogGroupHAL, "request buffer from stream: %p FAIL", getStream()->toRaw());
    } else {
        setLatedBuffer(buffers[0]);
    }

    return ret;
}

#define PAD_TO_SIZE(size, padding) (((size) + padding - 1) / padding * padding)
void StreamBuffer::copyFromYuvToPreview(GraBuffer *srcBuffer, GraBuffer *dstBuffer,
                                        const Metadata *options)
{
    int dstWidth = dstBuffer->getWidth();
    int dstStride = dstBuffer->getStride();
    int dstHeight = dstBuffer->getHeight();
    if (dstStride < dstWidth) {
        // NOTE: preview format is 512 aligned
        dstStride = PAD_TO_SIZE(dstWidth, 512);
    }

    dstBuffer->processData([dstWidth, dstStride, dstHeight, srcBuffer](uint8_t *data, size_t size) {
        int srcWidth = srcBuffer->getWidth();
        int srcStride = srcBuffer->getStride();
        int srcHeight = srcBuffer->getHeight();
        if (srcStride < srcWidth) {
            // NOTE: yuv format is 64 aligned
            srcStride = PAD_TO_SIZE(srcWidth, 64);
        }

        srcBuffer->processData([data, dstWidth, dstStride, dstHeight, srcStride, srcHeight,
                                srcWidth](uint8_t *srcData, size_t srcSize) {
            // XXX: way to align is different between Src and Dst,
            //      can not do memcpy directly.
            int srcScanline = PAD_TO_SIZE(srcHeight, 64);
            int dstScanline = PAD_TO_SIZE(dstHeight, 64);

            uint8_t *srcY = srcData;
            uint8_t *srcUV = srcData + srcStride * srcScanline;

            uint8_t *dstY = data;
            uint8_t *dstUV = data + dstStride * dstScanline;

            for (int i = 0; i < dstHeight; i++) {
                memcpy(dstY, srcY, dstWidth);
                dstY += dstStride;
                srcY += srcStride;
            }

            // NOTE: the dump result shows that Qcom QR Scan buffer is NV21 not NV12, so we need to
            // swap U and V
            for (int i = 0; i < dstHeight / 2; i++) {
                memcpy(dstUV, srcUV, dstWidth);
                dstUV += dstStride;
                srcUV += srcStride;
            }

            static int enableDumpQcomYuv =
                property_get_int32("persist.vendor.mihal.enableDumpEarlyYuv", 0);
            if (enableDumpQcomYuv) {
                char inputBuf[128];
                snprintf(inputBuf, sizeof(inputBuf), "/data/vendor/camera/YQ_Qcom_QrScan_%d_%d.yuv",
                         srcWidth, srcHeight);

                int scanline = PAD_TO_SIZE(srcHeight, 64);
                uint8_t *y = srcData;
                uint8_t *uv = srcData + srcStride * scanline;

                FILE *fp = fopen(inputBuf, "wb+");
                for (int i = 0; i < srcHeight; i++) {
                    fwrite(y, srcWidth, 1, fp);
                    y += srcStride;
                }
                for (int i = 0; i < srcHeight / 2; i++) {
                    fwrite(uv, srcWidth, 1, fp);
                    uv += srcStride;
                }
                fclose(fp);
            }

            return 0;
        });

        return 0;
    });
}

void StreamBuffer::copyFromPreviewToEarlyYuv(GraBuffer *srcBuffer, GraBuffer *dstBuffer,
                                             const Metadata *options)
{
    int dstWidth = dstBuffer->getWidth();
    int dstHeight = dstBuffer->getHeight();
    int dstYstride = dstBuffer->getYstrideBytes();
    int dstUVstride = dstBuffer->getUVstrideBytes();
    uint64_t dstYoffset = dstBuffer->getYplaneOffset();
    uint64_t dstUVoffset = dstBuffer->getUVplaneOffset();
    MLOGI(LogGroupHAL,
          "dstWidth:%u, dstHeight:%u, dstStride:%u, dstYstride:%u, dstUVstride:%u, dstYoffset:%u, "
          "dstUVoffset:%u",
          dstWidth, dstHeight, dstBuffer->getStride(), dstYstride, dstUVstride, dstYoffset,
          dstUVoffset);

    dstBuffer->processData([=](uint8_t *data, size_t size) {
        int srcWidth = srcBuffer->getWidth();
        int srcHeight = srcBuffer->getHeight();
        int srcYstride = srcBuffer->getYstrideBytes();
        int srcUVstride = srcBuffer->getUVstrideBytes();
        uint64_t srcYoffset = srcBuffer->getYplaneOffset();
        uint64_t srcUVoffset = srcBuffer->getUVplaneOffset();
        MLOGI(LogGroupHAL,
              "srcWidth:%u, srcHeight:%u, srcStride:%u, srcYstride:%u, srcUVstride:%u, "
              "srcYoffset:%u, srcUVoffset:%u",
              srcWidth, srcHeight, srcBuffer->getStride(), srcYstride, srcUVstride, srcYoffset,
              srcUVoffset);

        srcBuffer->processData([=](uint8_t *srcData, size_t srcSize) {
            uint8_t *srcY = srcData + srcYoffset;
            uint8_t *srcUV = srcData + srcUVoffset;

            uint8_t *dstY = data + dstYoffset;
            uint8_t *dstUV = data + dstUVoffset;

            for (int i = 0; i < dstHeight; i++) {
                memcpy(dstY, srcY, dstWidth);
                dstY += dstYstride;
                srcY += srcYstride;
            }

            for (int i = 0; i < dstHeight / 2; i++) {
                memcpy(dstUV, srcUV, dstWidth);
                dstUV += dstUVstride;
                srcUV += srcUVstride;
            }
            return 0;
        });

        static int enableDumpEarlyYuv =
            property_get_int32("persist.vendor.mihal.enableDumpEarlyYuv", 0);
        if (enableDumpEarlyYuv) {
            char inputBuf[128];
            snprintf(inputBuf, sizeof(inputBuf), "/data/vendor/camera/EarlyYuv_output_%d_%d.yuv",
                     dstWidth, dstHeight);

            uint8_t *y = data + dstYoffset;
            uint8_t *uv = data + dstUVoffset;

            FILE *fp = fopen(inputBuf, "wb+");
            for (int i = 0; i < dstHeight; i++) {
                fwrite(y, dstWidth, 1, fp);
                y += dstYstride;
            }
            for (int i = 0; i < dstHeight / 2; i++) {
                fwrite(uv, dstWidth, 1, fp);
                uv += dstUVstride;
            }
            fclose(fp);
        }

        return 0;
    });
}

int StreamBuffer::convertToJpeg(GraBuffer *inputBuffer, GraBuffer *outputBuffer,
                                const Metadata *options)
{
    if (!options) {
        MLOGE(LogGroupHAL, "can not get fwk meta");
        return -1;
    }

    MLOGI(LogGroupHAL, "input %dx%d format:%d, output %dx%d format:%d", inputBuffer->getWidth(),
          inputBuffer->getHeight(), inputBuffer->getFormat(), outputBuffer->getWidth(),
          outputBuffer->getHeight(), outputBuffer->getFormat());

    int jpegQuality = 80;
    camera_metadata_ro_entry entry = options->find(ANDROID_JPEG_QUALITY);
    if (entry.count) {
        jpegQuality = entry.data.u8[0];
    }

    ExifUtils *exifUtils = ExifUtils::create();
    auto exifRes = exifUtils->initialize(outputBuffer->getWidth(), outputBuffer->getHeight());
    if (!exifRes) {
        MLOGE(LogGroupHAL, "Failed to initialize ExifUtils object");
        return -1;
    }

    entry = options->find(ANDROID_JPEG_ORIENTATION);
    if (entry.count) {
        if (!exifUtils->setOrientation(entry.data.i32[0])) {
            MLOGE(LogGroupHAL, "setting orientation failed.");
            return -1;
        }
    }

    if (!exifUtils->generateApp1()) {
        MLOGE(LogGroupHAL, "Unable to generate App1 buffer");
        return -1;
    }

    outputBuffer->processData([&inputBuffer, &jpegQuality, &exifUtils](uint8_t *data, size_t size) {
        JpegCompressor compressor;
        JpegCompressorParams inputFrame{};
        inputFrame.width = inputBuffer->getWidth();
        inputFrame.height = inputBuffer->getHeight();
        inputFrame.format = inputBuffer->getFormat();
        inputFrame.stride = inputBuffer->getStride();
        if (inputFrame.format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
            inputFrame.scanline = PAD_TO_SIZE(inputFrame.height, 512);
        } else if (inputFrame.format == HAL_PIXEL_FORMAT_YCBCR_420_888) {
            inputFrame.scanline = PAD_TO_SIZE(inputFrame.height, 64);
        } else {
            inputFrame.scanline = inputFrame.height;
        }

        // NOTE: Out size must be set, otherwise out buffer will be reallocated
        compressor.setCompressedImage(&data, size);
        inputBuffer->processData([&compressor, &inputFrame, &jpegQuality, &exifUtils](
                                     uint8_t *inputData, size_t inputSize) {
            inputFrame.pAddr = inputData;
            inputFrame.size = inputSize;

#if 0
            char inputBuf[128];
            snprintf(inputBuf, sizeof(inputBuf), "/data/vendor/camera/EarlyYuv_%d_%d.yuv", inputFrame.width, inputFrame.height);
            uint8_t *y = inputFrame.pAddr;
            uint8_t *uv = inputFrame.pAddr + inputFrame.stride * inputFrame.scanline;
            FILE *fp = fopen(inputBuf, "wb+");
            for (int i = 0; i < inputFrame.height; i++) {
                fwrite(y, inputFrame.width, 1, fp);
                y += inputFrame.stride;
            }
            for (int i = 0; i < inputFrame.height/2; i++) {
                fwrite(uv, inputFrame.width, 1, fp);
                uv += inputFrame.stride;
            }
            fclose(fp);
#endif

            if (!compressor.compressImage(inputFrame, jpegQuality, exifUtils->getApp1Buffer(),
                                          exifUtils->getApp1Length())) {
                MLOGE(LogGroupHAL, "JPEG image compression failed");
                return -1;
            }

            return 0;
        });

        // set actual jpeg size
        camera3_jpeg_blob *blob =
            reinterpret_cast<camera3_jpeg_blob *>(data + size - sizeof(*blob));
        blob->jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;
        blob->jpeg_size = compressor.getCompressedImageSize();
#if 0
            unsigned long len = compressor.getCompressedImageSize();
            MLOGI(LogGroupHAL, "Early JPEG file size %u", len);

            char outputBuf[128];
            snprintf(outputBuf, sizeof(outputBuf), "/data/vendor/camera/EarlyJpeg.jpg");
            FILE *fp1 = fopen(outputBuf, "wb+");
            fwrite(data, len, 1, fp1);
            fclose(fp1);
#endif
        return 0;
    });

    if (exifUtils) {
        delete exifUtils;
        exifUtils = nullptr;
    }

    return 0;
}

void StreamBuffer::copyFromIdenticalFormat(const StreamBuffer &other, const Metadata *options)
{
    MASSERT(other.mRaw.status == CAMERA3_BUFFER_STATUS_OK);

    waitAndCloseAcquireFence();

    GraBuffer *src = other.getBuffer();
    GraBuffer *dst = getBuffer();
    MASSERT(src);
    MASSERT(dst);
    dst->processData([src](uint8_t *dstData, size_t dstSize) {
        src->processData([dstData, dstSize](uint8_t *srcData, size_t srcSize) {
            int copySize = dstSize < srcSize ? dstSize : srcSize;
            memcpy(dstData, srcData, copySize);
            return 0;
        });

        return 0;
    });
}

void StreamBuffer::copyFromImpl(const StreamBuffer &other, const Metadata *options)
{
    // NOTE: if 'other' buffer is error content, we copy the status and fence info
    if (other.mRaw.status != CAMERA3_BUFFER_STATUS_OK) {
        MLOGD(LogGroupRT,
              "[Buffer]: src buffer from stream:%p 's content is Error, hence set stream:%p 's "
              "buffer as error",
              other.getStream()->toRaw(), getStream()->toRaw());
        mRaw.status = other.mRaw.status;
        mRaw.acquire_fence = other.mRaw.acquire_fence;
        mRaw.release_fence = other.mRaw.release_fence;
        return;
    }

    GraBuffer *src = other.getBuffer();
    GraBuffer *dst = getBuffer();

    // NOTE: dispatch copy behavior on buffer format
    auto dstFormat = getStream()->getFormat();
    auto srcFormat = other.getStream()->getFormat();
    MLOGD(LogGroupRT,
          "[Buffer]: copy src buffer: stream:%p format:%d TO dst buffer: stream:%p format: %d",
          other.getStream()->toRaw(), srcFormat, getStream()->toRaw(), dstFormat);

    if (dstFormat == srcFormat) {
        // NOTE: in preview scenario, buffer metadata is also need to be copied because app need
        // this to decode colour info, otherwise the preview will be slightly dark
        if (srcFormat == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
            // NOTE: 10001 stands for QTI_COLOR_METADATA
            MetadataType metaDataType = {"QTI", static_cast<int64_t>(10001)};
            MiGrallocMapper::getInstance()->copyBufferMeta(src, dst, metaDataType);
        }

        copyFromIdenticalFormat(other, options);
        return;
    }

    if (dstFormat == HAL_PIXEL_FORMAT_BLOB) {
        // XXX: i don't know if we should waitAndCloseAcquireFence
        StreamBuffer::convertToJpeg(src, dst, options);
        return;
    }

    if (srcFormat == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED &&
        dstFormat == HAL_PIXEL_FORMAT_YCBCR_420_888) {
        // XXX: i don't know if we should waitAndCloseAcquireFence
        StreamBuffer::copyFromPreviewToEarlyYuv(src, dst, options);
        return;
    }

    if (srcFormat == HAL_PIXEL_FORMAT_YCBCR_420_888 &&
        dstFormat == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
        // XXX: i don't know if we should waitAndCloseAcquireFence
        StreamBuffer::copyFromYuvToPreview(src, dst, options);
        return;
    }
}

void StreamBuffer::tryToCopyFrom(StreamBuffer &other, const Metadata *options)
{
    // NOTE: if 'other' buffer's handle is null, we just set error
    if (!other.getBuffer() || !other.getBuffer()->getHandle()) {
        MLOGD(
            LogGroupRT,
            "[Buffer]: src buffer handle from stream:%p is null, set errot to stream:%p 's buffer",
            other.getStream()->toRaw(), getStream()->toRaw());
        setErrorBuffer();
        return;
    }

    // NOTE: if 'other' buffer is ERROR status, then we just set error
    if (other.mRaw.status != CAMERA3_BUFFER_STATUS_OK) {
        MLOGD(LogGroupRT,
              "[Buffer]: src buffer from stream:%p 's content is Error, hence set stream:%p 's "
              "buffer as error",
              other.getStream()->toRaw(), getStream()->toRaw());
        setErrorBuffer();
        return;
    }

    // NOTE: next, prepare buffer if HAL3 buffer manager is on
    if (!mBuffer || !mBuffer->getHandle()) {
        MASSERT(getStream()->getCamera()->supportBufferManagerAPI());

        int ret = prepareBuffer();
        // NOTE: if we prepare fail, we just set it as error
        if (ret) {
            MLOGW(LogGroupRT, "[Buffer]: prepare stream:%p 's buffer FAIL!!", getStream()->toRaw());
            setErrorBuffer();
            return;
        }
    }

    // NOTE: next, copy 'other' buffer's content into this buffer
    MASSERT(mBuffer && mBuffer->getHandle());
    copyFromImpl(other, options);
}

void StreamBuffer::setErrorBuffer()
{
    waitAndCloseAcquireFence();
    mRaw.status = CAMERA3_BUFFER_STATUS_ERROR;
    mRaw.acquire_fence = -1;
    mRaw.release_fence = -1;
}

RemoteStreamBuffer::RemoteStreamBuffer(CameraDevice *camera, const camera3_stream_buffer *sbuffer)
    : StreamBuffer{sbuffer}, mStream{RemoteStream::create(camera, sbuffer->stream)}
{
}

LocalStreamBuffer::LocalStreamBuffer(std::shared_ptr<Stream> stream,
                                     std::unique_ptr<GraBuffer> buffer)
    : StreamBuffer{std::move(buffer)}, mStream{stream}
{
    mRaw.stream = getStream()->toRaw();
    mRaw.buffer = getBuffer() ? getBuffer()->getHandlePtr() : GraBuffer::getEmptyHandlePtr();
    mRaw.status = CAMERA3_BUFFER_STATUS_OK;
    mRaw.acquire_fence = -1;
    mRaw.release_fence = -1;
}

std::string CaptureRequest::toString() const
{
    std::ostringstream str;
    const camera3_capture_request *request = toRaw();

    str << " \tcamera=" << mCamera->getId() << " frame=" << getFrameNumber()
        << " meta=" << request->settings << " numPhyMeta=" << getPhyMetaNum();

    if (request->input_buffer)
        str << " inputBuffer=" << request->input_buffer;

    str << " numOut=" << getOutBufferNum();
    for (int i = 0; i < getOutBufferNum(); i++) {
        str << "\n\t\tstream=" << request->output_buffers[i].stream << " buffer:["
            << request->output_buffers[i].buffer << "]=" << *request->output_buffers[i].buffer
            << " status=" << request->output_buffers[i].status
            << " a_fence=" << request->output_buffers[i].acquire_fence
            << " r_fence=" << request->output_buffers[i].release_fence;
    }

    return str.str();
}

bool CaptureRequest::fromMockCamera() const
{
    return mCamera->isMockCamera();
}

bool CaptureRequest::isSnapshot() const
{
    bool snapshot = false;

    for (int i = 0; i < getOutBufferNum(); i++) {
        StreamBuffer *sbuffer = getStreamBuffer(i).get();
        if (sbuffer->isSnapshot()) {
            snapshot = true;
            break;
        }
    }

    return snapshot;
}

RemoteRequest::RemoteRequest(CameraDevice *camera, camera3_capture_request *request)
    : CaptureRequest{camera},
      mMeta{request->settings},
      mRawRequest{request},
      mInputStreamBuffer{nullptr}
{
    if (mRawRequest->input_buffer) {
        mInputStreamBuffer =
            std::make_shared<RemoteStreamBuffer>(camera, mRawRequest->input_buffer);
    }

    for (int i = 0; i < getOutBufferNum(); i++) {
        mStreamBuffers.push_back(
            std::make_shared<RemoteStreamBuffer>(camera, &request->output_buffers[i]));
    }
}

std::shared_ptr<StreamBuffer> RemoteRequest::getStreamBuffer(uint32_t index) const
{
    if (index >= mStreamBuffers.size())
        return nullptr;

    return mStreamBuffers[index];
}

int RemoteRequest::deleteInputStreamBuffer(std::shared_ptr<StreamBuffer> streamBuffer)
{
    MASSERT(false);
    return -1;
}

int RemoteRequest::deleteOutputStreamBuffer(std::shared_ptr<StreamBuffer> streamBuffer)
{
    MASSERT(false);
    return -1;
}

const PhysicalMetas RemoteRequest::getPhyMetas() const
{
    PhysicalMetas metas;

    for (int i = 0; i < mRawRequest->num_physcam_settings; i++) {
        metas.emplace_back(mRawRequest->physcam_id[i],
                           std::make_shared<Metadata>(mRawRequest->physcam_settings[i]));
    }

    return metas;
}

LocalRequest::RawRequestWraper::RawRequestWraper(const LocalRequest &req)
    : camera3_capture_request{}
{
    frame_number = req.getFrameNumber();
    settings = req.getMeta()->peek();
    num_output_buffers = req.getOutBufferNum();

    if (num_output_buffers) {
        camera3_stream_buffer *buffers = new camera3_stream_buffer[num_output_buffers];
        if (!buffers)
            return;

        for (int i = 0; i < num_output_buffers; i++) {
            buffers[i] = *(req.getStreamBuffer(i)->toRaw());
        }

        output_buffers = buffers;
    }

    if (req.mInputStreamBuffers.size()) {
        input_buffer = const_cast<camera3_stream_buffer *>(req.mInputStreamBuffers[0]->toRaw());
    }

    num_physcam_settings = req.getPhyMetaNum();
    if (num_physcam_settings) {
        physcam_id = new const char *[num_physcam_settings];
        physcam_settings = new const camera_metadata *[num_physcam_settings];

        for (int i = 0; i < num_physcam_settings; i++) {
            physcam_id[i] = req.getPhyCamera(i);
            physcam_settings[i] = req.getPhyMeta(i)->peek();
        }
    }
}

LocalRequest::RawRequestWraper::~RawRequestWraper()
{
    delete[] output_buffers;
    delete[] physcam_id;
    delete[] physcam_settings;
}

LocalRequest::LocalRequest(const CaptureRequest &other)
    : CaptureRequest{other.getCamera()},
      mFrameNumber{other.getFrameNumber()},
      mMeta{*other.getMeta()}
{
    for (int i = 0; i < other.getOutBufferNum(); i++) {
        mStreamBuffers.push_back(other.getStreamBuffer(i));
    }

    for (int i = 0; i < other.getPhyMetaNum(); i++) {
        mPhyMetas.push_back(
            {other.getPhyCamera(i), std::make_shared<Metadata>(*other.getPhyMeta(i))});
    }
}

// XXX: now meta is CopyOnWrite. Do we need to Create meta/phyMeta with CopyNow or CopyOnWrite?
LocalRequest::LocalRequest(CameraDevice *camera, const camera3_capture_request *request)
    : CaptureRequest{camera},
      mFrameNumber{request->frame_number},
      mMeta{request->settings, Metadata::CopyNow}
{
    for (int i = 0; i < request->num_output_buffers; i++) {
        mStreamBuffers.push_back(
            std::make_shared<RemoteStreamBuffer>(camera, &request->output_buffers[i]));
    }

    if (request->input_buffer) {
        mInputStreamBuffers.push_back(
            std::make_shared<RemoteStreamBuffer>(camera, request->input_buffer));
    }

    for (int i = 0; i < request->num_physcam_settings; i++) {
        mPhyMetas.push_back(
            {request->physcam_id[i],
             std::make_shared<Metadata>(request->physcam_settings[i], Metadata::CopyNow)});
    }
}

LocalRequest::LocalRequest(CameraDevice *camera, uint32_t frame,
                           std::vector<std::shared_ptr<StreamBuffer>> outputStreamBuffers,
                           const Metadata &meta, PhysicalMetas phyMetas)
    : CaptureRequest{camera},
      mFrameNumber{frame},
      mStreamBuffers{outputStreamBuffers},
      mMeta{meta},
      mPhyMetas{phyMetas}
{
}

LocalRequest::LocalRequest(CameraDevice *camera, uint32_t frame,
                           std::vector<std::shared_ptr<StreamBuffer>> inputStreamBuffers,
                           std::vector<std::shared_ptr<StreamBuffer>> outputStreamBuffers,
                           const Metadata &meta, PhysicalMetas phyMetas)
    : CaptureRequest{camera},
      mFrameNumber{frame},
      mInputStreamBuffers{inputStreamBuffers},
      mStreamBuffers{outputStreamBuffers},
      mMeta{meta},
      mPhyMetas{phyMetas}
{
}

std::shared_ptr<StreamBuffer> LocalRequest::getStreamBuffer(uint32_t index) const
{
    if (index >= mStreamBuffers.size())
        return nullptr;

    return mStreamBuffers[index];
}

int LocalRequest::deleteInputStreamBuffer(std::shared_ptr<StreamBuffer> streamBuffer)
{
    for (int i = 0; i < mInputStreamBuffers.size(); i++) {
        if (mInputStreamBuffers[i] == streamBuffer) {
            mInputStreamBuffers[i] = nullptr;
            return 0;
        }
    }

    return -ENOENT;
}

int LocalRequest::addOutputStreamBuffer(std::shared_ptr<StreamBuffer> streamBuffer)
{
    // FIXME: who is reponsible to set the stream ID
    mStreamBuffers.push_back(streamBuffer);
    return 0;
}

int LocalRequest::deleteOutputStreamBuffer(std::shared_ptr<StreamBuffer> streamBuffer)
{
    for (int i = 0; i < mStreamBuffers.size(); i++) {
        if (mStreamBuffers[i] == streamBuffer) {
            mStreamBuffers[i] = nullptr;
            return 0;
        }
    }

    return -ENOENT;
}

std::shared_ptr<StreamBuffer> LocalRequest::getBufferByStream(const StreamBuffer *fromResult) const
{
    if (!fromResult || !fromResult->toRaw() || !fromResult->toRaw()->buffer) {
        MLOGE(LogGroupHAL, "could not get by null raw buffer");
        return nullptr;
    }

    Stream *wanted = fromResult->getStream();
    std::shared_ptr<StreamBuffer> requestedBuffer;
    for (auto buffer : mStreamBuffers) {
        if (*buffer->getStream() == *wanted) {
            requestedBuffer = buffer;
            break;
        }
    }

    if (!requestedBuffer) {
        for (auto buffer : mInputStreamBuffers) {
            if (*buffer->getStream() == *wanted) {
                requestedBuffer = buffer;
                break;
            }
        }
    }

    if (requestedBuffer) {
        // NOTE: if BufferManager API enabled, we need to correct the original StreamBuffer
        if (wanted->isInternal() && fromResult->getBuffer()->getHandle()) {
            if (!requestedBuffer->getBuffer()) {
                requestedBuffer->setLatedBuffer(*fromResult);
            }
        } else if (!wanted->isInternal()) {
            if (!(*requestedBuffer->toRaw()->buffer)) {
                *requestedBuffer = *fromResult;
            }
        }

        requestedBuffer->setStatus(fromResult->getStatus());
    }

    return requestedBuffer;
}

void LocalRequest::safeguardMeta()
{
    mMeta.safeguard();

    for (auto &p : mPhyMetas) {
        p.second->safeguard();
    }
}

const camera3_capture_request *LocalRequest::toRaw() const
{
    if (!mRawRequest)
        mRawRequest = std::make_unique<RawRequestWraper>(*this);

    return mRawRequest.get();
}

std::string LocalRequest::toString() const
{
    std::ostringstream str;

    str << " \t\tcamera=" << mCamera->getId() << " frame=" << getFrameNumber()
        << " meta=" << getMeta()->peek() << " numPhyMeta=" << getPhyMetaNum()
        << " inBufferNum=" << mInputStreamBuffers.size()
        << " outBufferNum=" << mStreamBuffers.size();

    for (const auto &inBuffer : mInputStreamBuffers) {
        str << "\n\t\tinputBuffer=" << inBuffer->toRaw()
            << " stream=" << inBuffer->getStream()->toRaw();
    }

    for (const auto &outBuffer : mStreamBuffers) {
        str << "\n\t\toutBuffer=" << outBuffer->toRaw()
            << " stream=" << outBuffer->getStream()->toRaw() << " buffer:["
            << outBuffer->toRaw()->buffer << "]=" << *outBuffer->toRaw()->buffer
            << " status=" << outBuffer->getStatus() << " a_fence=" << outBuffer->getAcquireFence()
            << " r_fence=" << outBuffer->getReleaseFence();
    }

    return str.str();
}

int LocalRequest::append(const Metadata &other)
{
    return mMeta.append(other);
}

std::string CaptureResult::toString() const
{
    std::ostringstream str;

    const camera3_capture_result *raw = toRaw();
    if (!raw)
        return str.str();

    str << " frame=" << raw->frame_number << " meta=" << raw->result
        << " numIn=" << (raw->input_buffer ? 1 : 0) << " numOut=" << raw->num_output_buffers
        << " partial=" << raw->partial_result << " numPhyMeta=" << raw->num_physcam_metadata;

    if (raw->input_buffer)
        str << "\n\t\tinput stream=" << raw->input_buffer->stream << " buffer:["
            << raw->input_buffer->buffer << "]=" << *raw->input_buffer->buffer
            << " status=" << raw->input_buffer->status
            << " acquire_fence=" << raw->input_buffer->acquire_fence
            << " release_fence=" << raw->input_buffer->release_fence;

    for (int i = 0; i < raw->num_output_buffers; i++)
        str << "\n\t\toutput stream=" << raw->output_buffers[i].stream << " buffer:["
            << raw->output_buffers[i].buffer << " ]=" << *raw->output_buffers[i].buffer
            << " status=" << raw->output_buffers[i].status
            << " acquire_fence=" << raw->output_buffers[i].acquire_fence
            << " release_fence=" << raw->output_buffers[i].release_fence;

    return str.str();
}

RemoteResult::RemoteResult(CameraDevice *camera, const camera3_capture_result *result)
    : CaptureResult{camera}, mMeta{std::make_unique<Metadata>(result->result)}, mRawResult{result}
{
    // FIXME: we should optimize here, since each request would new objects of RemoteStreamBuffer
    for (int i = 0; i < getBufferNumber(); i++) {
        mStreamBuffers.push_back(
            std::make_shared<RemoteStreamBuffer>(camera, &result->output_buffers[i]));
    }
}

const PhysicalMetas RemoteResult::getPhyMetas() const
{
    PhysicalMetas metas;

    for (int i = 0; i < mRawResult->num_physcam_metadata; i++) {
        metas.emplace_back(mRawResult->physcam_ids[i],
                           std::make_shared<Metadata>(mRawResult->physcam_metadata[i]));
    }

    return metas;
}

RemoteResult::~RemoteResult()
{
    mMeta->release();
}

bool RemoteResult::isErrorResult() const
{
    for (auto &streamBuffer : mStreamBuffers) {
        if (streamBuffer->toRaw()->status != CAMERA3_BUFFER_STATUS_OK)
            return true;
    }

    return false;
}

bool RemoteResult::isSnapshotError() const
{
    for (auto &streamBuffer : mStreamBuffers) {
        if ((streamBuffer->toRaw()->status != CAMERA3_BUFFER_STATUS_OK) &&
            (streamBuffer->isSnapshot()))
            return true;
    }

    return false;
}

LocalResult::LocalResult(CameraDevice *device, uint32_t frameNumber, uint32_t partial,
                         std::unique_ptr<Metadata> meta)
    : CaptureResult{device}, mFrameNumber{frameNumber}, mPartial{partial}, mMeta{std::move(meta)}
{
}

LocalResult::LocalResult(CameraDevice *device, uint32_t frameNumber, uint32_t partial,
                         std::vector<std::shared_ptr<StreamBuffer>> outBuffers,
                         std::unique_ptr<Metadata> meta)
    : CaptureResult{device},
      mFrameNumber{frameNumber},
      mPartial{partial},
      mOutBuffers{outBuffers},
      mMeta{std::move(meta)}
{
}

LocalResult::LocalResult(CameraDevice *device, uint32_t frameNumber, uint32_t partial,
                         std::shared_ptr<StreamBuffer> inBuffer,
                         std::vector<std::shared_ptr<StreamBuffer>> outBuffers,
                         std::unique_ptr<Metadata> meta)
    : CaptureResult{device},
      mFrameNumber{frameNumber},
      mPartial{partial},
      mInputBuffer{inBuffer},
      mOutBuffers{outBuffers},
      mMeta{std::move(meta)}
{
}

LocalResult::LocalResult(const RemoteResult &result)
    : CaptureResult{result},
      mFrameNumber{result.getFrameNumber()},
      mPartial{result.getPartial()},
      mMeta{std::make_unique<Metadata>(*(result.getMeta()))}
{
}

bool LocalResult::isErrorResult() const
{
    for (auto &streamBuffer : mOutBuffers) {
        if (streamBuffer->toRaw()->status != CAMERA3_BUFFER_STATUS_OK)
            return true;
    }

    return false;
}

bool LocalResult::isSnapshotError() const
{
    for (auto &streamBuffer : mOutBuffers) {
        if ((streamBuffer->toRaw()->status != CAMERA3_BUFFER_STATUS_OK) &&
            (streamBuffer->isSnapshot()))
            return true;
    }

    return false;
}

LocalResult::RawResultWraper::RawResultWraper(const LocalResult &rw) : camera3_capture_result{}
{
    frame_number = rw.mFrameNumber;
    result = rw.getMeta()->peek();
    partial_result = rw.getPartial();
    input_buffer = rw.mInputBuffer ? rw.mInputBuffer->toRaw() : nullptr;
    num_output_buffers = rw.mOutBuffers.size();
    num_physcam_metadata = rw.mPhyMetas.size();

    if (num_output_buffers) {
        camera3_stream_buffer *buffers = new camera3_stream_buffer[num_output_buffers];
        if (!buffers)
            return;

        for (int i = 0; i < num_output_buffers; i++) {
            buffers[i] = *(rw.mOutBuffers[i]->toRaw());
        }
        output_buffers = buffers;
    } else {
        output_buffers = nullptr;
    }

    if (!num_physcam_metadata)
        return;

    physcam_ids = new const char *[num_physcam_metadata];
    physcam_metadata = new const camera_metadata *[num_physcam_metadata];
    for (int i = 0; i < num_physcam_metadata; i++) {
        physcam_ids[i] = rw.mPhyMetas[i].first.c_str();
        physcam_metadata[i] = rw.mPhyMetas[i].second->peek();
    }
}

LocalResult::RawResultWraper::~RawResultWraper()
{
    delete[] output_buffers;
    delete[] physcam_ids;
    delete[] physcam_metadata;
}

const camera3_capture_result *LocalResult::toRaw() const
{
    if (!mRawResult)
        mRawResult = std::make_unique<RawResultWraper>(*this);

    return mRawResult.get();
}

std::string StreamConfig::toString() const
{
    std::ostringstream str;

    str << " numStreams=" << getStreamNums() << " op_mode=" << std::hex << std::showbase
        << getOpMode() << std::dec;

    uint32_t idx = 0;
    forEachStream([&str, &idx](const camera3_stream *raw) {
        auto stream = Stream::toRichStream(raw);
        if (!stream) {
            str << "\n\tstream[" << idx << "]: "
                << "no matched rich stream,raw=" << raw;
            idx++;
            return 0;
        }

        str << "\n\tstream[" << idx << "]: " << stream->toString(8, true);
        idx++;
        return 0;
    });

    return str.str();
}

RemoteConfig::RemoteConfig(CameraDevice *camera, camera3_stream_configuration *config)
    : StreamConfig{camera}, mConfig{config}, mMeta{config->session_parameters}
{
    for (int i = 0; i < config->num_streams; i++) {
        // set the stream index as the cookie
        config->streams[i]->priv = reinterpret_cast<void *>(i);
    }
}

int RemoteConfig::forEachStream(std::function<int(const camera3_stream *)> func) const
{
    for (int i = 0; i < mConfig->num_streams; i++) {
        if (func(mConfig->streams[i]) == -EINTR)
            break;
    }

    return 0;
}

int RemoteConfig::forEachStream(std::function<int(std::shared_ptr<Stream>)> func) const
{
    for (int i = 0; i < mConfig->num_streams; i++) {
        auto stream = Stream::toRichStream(mConfig->streams[i]);
        if (!stream)
            continue;

        if (func(stream) == -EINTR)
            break;
    }

    return 0;
}

LocalConfig::LocalConfig(std::vector<std::shared_ptr<Stream>> streams, uint32_t opMode,
                         Metadata &sessionPara)
    : StreamConfig{streams[0]->getCamera()}, mStreams{streams}, mOpMode{opMode}
{
    for (int i = 0; i < mStreams.size(); i++) {
        auto stream = mStreams[i];
        stream->setCookie(nullptr);
    }

    if (!sessionPara.isEmpty())
        mSessionPara.acquire(sessionPara);
}

// NOTE: other is a RemoteConfig, here we parse the attribute of each stream
LocalConfig::LocalConfig(const StreamConfig &other)
    : StreamConfig{other.getCamera()}, mOpMode{other.getOpMode()}, mSessionPara{*other.getMeta()}
{
    camera3_stream_configuration *raw = other.toRaw();

    for (int i = 0; i < raw->num_streams; i++) {
        std::bitset<16> prop;
        camera3_stream *rawStream = raw->streams[i];
        if (rawStream->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
            if (i == 0)
                prop.set(Stream::PRIMARY_PREVIEW);
            else {
                // NOTE: trivial means this stream's buffer is not important and we can set them to
                // error when we need to
                prop.set(Stream::AUX_PREVIEW);
                prop.set(Stream::TRIVIAL);
            }
            if (rawStream->data_space == (int32_t)HAL_DATASPACE_HEIF) {
                prop.set(Stream::HEIC_YUV);
            }
        } else if (rawStream->format == HAL_PIXEL_FORMAT_BLOB ||
                   rawStream->format == HAL_PIXEL_FORMAT_YCBCR_420_888) {
            if (i == 1)
                prop.set(Stream::SNAPSHOT);
            else {
                // NOTE: by now, realtime means QR Scan yuv buffer. It's used for gesture detection
                // or QR Scan
                prop.set(Stream::REALTIME);
                // QR Scan stream_use_case , mihal support on Android-T or better
                rawStream->stream_use_case = 65538;
            }
            if (rawStream->format == HAL_PIXEL_FORMAT_BLOB &&
                rawStream->data_space == (int32_t)HAL_DATASPACE_JPEG_APP_SEGMENTS) {
                prop.set(Stream::HEIC_THUMBNAIL);
            }
        }

        auto stream = RemoteStream::create(getCamera(), raw->streams[i], prop);
        stream->setStreamId(i);
        mStreams.push_back(stream);
    }
}

std::shared_ptr<Stream> LocalConfig::getPhyStream(
    std::string phyId, std::function<bool(std::shared_ptr<Stream>)> wanted) const
{
    // NOTE: if there are more physical streams with the same the phyID, return the first one
    // if 'wanted' is null
    for (auto &stream : mStreams) {
        camera3_stream *rawStream = stream->toRaw();
        if (rawStream->physical_camera_id && phyId == rawStream->physical_camera_id) {
            if (!wanted) {
                return stream;
            } else if (wanted(stream)) {
                return stream;
            }
        }
    }

    return nullptr;
}

int LocalConfig::forEachStream(std::function<int(const camera3_stream *)> func) const
{
    for (auto stream : mStreams) {
        if (func(stream->toRaw()) == -EINTR)
            break;
    }

    return 0;
}

int LocalConfig::forEachStream(std::function<int(std::shared_ptr<Stream>)> func) const
{
    for (auto stream : mStreams) {
        if (func(stream) == -EINTR)
            break;
    }

    return 0;
}

camera3_stream_configuration *LocalConfig::toRaw() const
{
    if (!mRawConfig) {
        mRawConfig = std::make_unique<camera3_stream_configuration>();
        if (!mRawConfig)
            return nullptr;

        mRawConfig->operation_mode = mOpMode;
        mRawConfig->num_streams = mStreams.size();
        mRawConfig->streams = new camera3_stream *[mStreams.size()];
        for (int i = 0; i < mStreams.size(); i++) {
            mRawConfig->streams[i] = mStreams[i]->toRaw();
        }
    }

    mRawConfig->session_parameters = mSessionPara.peek();
    return mRawConfig.get();
}

LocalConfig::~LocalConfig()
{
    if (mRawConfig) {
        delete[] mRawConfig->streams;
    }
}

bool LocalConfig::isEqual(const std::shared_ptr<LocalConfig> config) const
{
    if (getStreamNums() != config->getStreamNums()) {
        return false;
    }

    for (int i = 0; i < getStreamNums(); i++) {
        auto lStream = mStreams[i];
        auto rStream = config->getStreams().at(i);
        if ((lStream->getWidth() != rStream->getWidth()) ||
            (lStream->getHeight() != rStream->getHeight()) ||
            (lStream->getFormat() != rStream->getFormat())) {
            return false;
        }
    }

    return true;
}

NotifyMessage::NotifyMessage(CameraDevice *camera, const camera3_notify_msg *msg)
    : mCamera{camera}, mRawMsg{msg}, mOwnerOfRaw{false}
{
}

NotifyMessage::NotifyMessage(const NotifyMessage &other, uint32_t frame) : mCamera{other.mCamera}
{
    camera3_notify_msg *msg = new camera3_notify_msg{*other.mRawMsg};
    MASSERT(msg);

    mRawMsg = msg;
    mOwnerOfRaw = true;

    if (other.getFrameNumber() == frame)
        return;

    if (other.isErrorMsg())
        msg->message.error.frame_number = frame;
    else
        msg->message.shutter.frame_number = frame;
}

NotifyMessage::NotifyMessage(CameraDevice *camera, uint32_t frame, camera3_stream *stream)
    : mCamera{camera}
{
    camera3_notify_msg *msg = new camera3_notify_msg{};
    MASSERT(msg);

    msg->type = CAMERA3_MSG_ERROR;
    msg->message.error.frame_number = frame;
    msg->message.error.error_stream = stream;
    msg->message.error.error_code = CAMERA3_MSG_ERROR_BUFFER;

    mRawMsg = msg;
    mOwnerOfRaw = true;
}

NotifyMessage::NotifyMessage(CameraDevice *camera, uint32_t frame, int64_t ts) : mCamera{camera}
{
    camera3_notify_msg *msg = new camera3_notify_msg{};
    MASSERT(msg);

    msg->type = CAMERA3_MSG_SHUTTER;
    msg->message.shutter.frame_number = frame;
    msg->message.shutter.timestamp = ts;
    msg->message.shutter.readoutTimestamp = ts;

    mRawMsg = msg;
    mOwnerOfRaw = true;
}

NotifyMessage::NotifyMessage(CameraDevice *camera, uint32_t frame, int error) : mCamera{camera}
{
    MASSERT(error == CAMERA3_MSG_ERROR_REQUEST || error == CAMERA3_MSG_ERROR_RESULT);

    camera3_notify_msg *msg = new camera3_notify_msg{};
    MASSERT(msg);

    msg->type = CAMERA3_MSG_ERROR;
    msg->message.error.frame_number = frame;
    msg->message.error.error_stream = nullptr;
    msg->message.error.error_code = error;

    mRawMsg = msg;
    mOwnerOfRaw = true;
}

NotifyMessage::~NotifyMessage()
{
    if (mOwnerOfRaw && mRawMsg)
        delete mRawMsg;
}

void NotifyMessage::notify() const
{
    mCamera->notify(mRawMsg);
}

uint32_t NotifyMessage::getFrameNumber() const
{
    uint32_t frame;
    if (mRawMsg->type == CAMERA3_MSG_ERROR)
        frame = mRawMsg->message.error.frame_number;
    else
        frame = mRawMsg->message.shutter.frame_number;

    return frame;
}

uint64_t NotifyMessage::getTimeStamp() const
{
    uint64_t timestamp = 0;
    if (mRawMsg->type == CAMERA3_MSG_SHUTTER)
        timestamp = mRawMsg->message.shutter.timestamp;

    return timestamp;
}

bool NotifyMessage::isErrorMsg() const
{
    return mRawMsg->type == CAMERA3_MSG_ERROR;
}

std::string NotifyMessage::toString() const
{
    std::ostringstream str;

    if (mRawMsg->type == CAMERA3_MSG_ERROR) {
        str << " type=ERROR"
            << " frame=" << mRawMsg->message.error.frame_number
            << " stream=" << mRawMsg->message.error.error_stream
            << " error=" << mRawMsg->message.error.error_code;
    } else {
        str << " type=SHUTTER"
            << " frame=" << mRawMsg->message.shutter.frame_number
            << " ts=" << mRawMsg->message.shutter.timestamp;
    }

    return str.str();
}

PreProcessConfig::PreProcessConfig(CameraDevice *camera, std::vector<camera3_stream> streams,
                                   const Metadata *meta)
    : mCamera{camera}, mStreams{streams}, mMeta{std::make_unique<Metadata>(*meta)}
{
}

} // namespace mihal
