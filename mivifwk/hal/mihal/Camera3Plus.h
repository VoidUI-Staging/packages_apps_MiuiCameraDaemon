#ifndef __CAMERA3_PLUS_H__
#define __CAMERA3_PLUS_H__

#include <hardware/camera3.h>

#include <condition_variable>
#include <queue>
#include <string>
#include <thread>

#include "GraBuffer.h"
#include "Metadata.h"
#include "Stream.h"

namespace mihal {

class CameraDevice;
class GraBuffer;
class Session;

using PhysicalMetas = std::vector<
    std::pair<std::string /* physcam_id */, std::shared_ptr<Metadata> /* physcam_meta */>>;

class StreamBuffer
{
public:
    StreamBuffer() = delete;
    StreamBuffer &operator=(const camera3_stream_buffer &other);
    StreamBuffer &operator=(const StreamBuffer &other);
    virtual ~StreamBuffer() = default;

    int moveGraBufferFrom(StreamBuffer &&other);
    const camera3_stream_buffer *toRaw() const { return &mRaw; }
    const buffer_handle_t *toRawHandle() const { return toRaw()->buffer; }
    virtual Stream *getStream() const = 0;
    GraBuffer *getBuffer() const { return mBuffer.get(); }
    int setLatedBuffer(const StreamBuffer &buffer);
    int setLatedBuffer(const camera3_stream_buffer &buffer);
    virtual int releaseBuffer() { return -ENOEXEC; }

    int getStatus() const { return mRaw.status; }
    void setStatus(int status) { mRaw.status = status; }
    int getAcquireFence() const { return mRaw.acquire_fence; };
    void setAcquireFence(int fence) { mRaw.acquire_fence = fence; }
    int getReleaseFence() const { return mRaw.release_fence; }
    void setReleaseFence(int fence) { mRaw.release_fence = fence; }

    // NOTE: try to copy the content of other buffer into this buffer
    // @param:
    // other is the source buffer
    // options is some parameter to control the copy process
    // @brief:
    // if 'other' buffer handle is null then this buffer's status will be set as error
    // if 'other' buffer has handle then its content will be copied to this buffer
    // @side effect:
    // if 'other' buffer's stream is peer stream of this buffer, then 'other' buffer will be set as
    // consumed
    void tryToCopyFrom(StreamBuffer &other, const Metadata *options = nullptr);
    void setErrorBuffer();

    bool isSnapshot() const { return getStream()->isSnapshot(); }
    bool isPreview() const { return getStream()->isPreview(); }
    bool isQuickview() const { return getStream()->isQuickview(); }
    // NOTE: isOverrided is only called by fwk buffer, for such buffer, we shouldn't sent them to
    // Qcom since its stream is overrided by mihal and doesn't get configured to Qcom
    bool isOverrided() const { return getStream()->hasPeerStream(); }

    bool isConsumedByPeerStreamBuffer() const { return mIsConsumedByPeerStreamBuffer; }
    void setConsumedByPeerStreamBuffer() { mIsConsumedByPeerStreamBuffer = true; }

    // NOTE: if a buffer is sent to Qcom, then it will be marked and Mihal won't touch these buffer
    void markSentToHal() { mIsSentToHal = true; }
    bool isSentToHal() { return mIsSentToHal; }

    // NOTE: request buffer in place
    static int requestBufferForHijacked(StreamBuffer *streamBuffer);

    int getMialgoPort() { return mMialgoPort; }
    void setMialgoPort(int port) { mMialgoPort = port; }

protected:
    // for RemoteStreamBuffer
    StreamBuffer(const camera3_stream_buffer *sbuffer);
    // for LocalStreamBuffer
    StreamBuffer(std::unique_ptr<GraBuffer> buffer);

    // NOTE: prepare buffer handle if HAL3 buffer manager is on
    // @brief:
    // for the buffers whose stream has peer tream, we get buffer from its stream's mPutBuffers list
    // for the buffers whose stream doesn't have peer stream, just request buffer in place
    // @ret:
    // 0: prepare OK
    // -1: prepare fail
    int prepareBuffer();

    static void copyFromYuvToPreview(GraBuffer *srcBuffer, GraBuffer *dstBuffer,
                                     const Metadata *options);
    static void copyFromPreviewToEarlyYuv(GraBuffer *srcBuffer, GraBuffer *dstBuffer,
                                          const Metadata *options);
    static int convertToJpeg(GraBuffer *inputBuffer, GraBuffer *outputBuffer,
                             const Metadata *options);
    void copyFromIdenticalFormat(const StreamBuffer &other, const Metadata *options);
    void copyFromImpl(const StreamBuffer &other, const Metadata *options);

    camera3_stream_buffer mRaw;
    std::unique_ptr<GraBuffer> mBuffer;

    bool mIsSentToHal = false;
    // NOTE: if a vnd buffer is not consumed by its peer stream's buffer, then when this buffer get
    // released or returned to internal quickview list, then we should also return its peer stream's
    // buffer. For details, you can refer to:
    // https://xiaomi.f.mioffice.cn/docs/dock4qQImIBogDbm7qEcpxLllld
    bool mIsConsumedByPeerStreamBuffer = false;

    // NOTE: which mialgo port this buffer will be sent to
    int mMialgoPort = -1;

private:
    int waitAndCloseAcquireFence();
};

class RemoteStreamBuffer : public StreamBuffer
{
public:
    RemoteStreamBuffer(CameraDevice *camera, const camera3_stream_buffer *sbuffer);
    Stream *getStream() const override { return mStream.get(); }

private:
    std::shared_ptr<Stream> mStream;
};

class LocalStreamBuffer : public StreamBuffer
{
public:
    LocalStreamBuffer(std::shared_ptr<Stream> stream, std::unique_ptr<GraBuffer> graBuffer);
    ~LocalStreamBuffer() { releaseBuffer(); }

    Stream *getStream() const override { return mStream.get(); }

    int releaseBuffer() override
    {
        if (mBuffer)
            mStream->releaseBuffer(std::move(mBuffer));

        mRaw.stream = getStream()->toRaw();
        mRaw.buffer = getBuffer() ? getBuffer()->getHandlePtr() : GraBuffer::getEmptyHandlePtr();
        mRaw.status = CAMERA3_BUFFER_STATUS_OK;
        mRaw.acquire_fence = -1;
        mRaw.release_fence = -1;

        return 0;
    }

private:
    std::shared_ptr<Stream> mStream;
};

class CaptureRequest
{
public:
    CaptureRequest(CameraDevice *camera) : mCamera{camera} {}
    virtual ~CaptureRequest() = default;

    virtual uint32_t getFrameNumber() const = 0;
    virtual void setFrameNumber(uint32_t frame){};
    // FIXME: the interface to get num/StreamBuffers is not simple enought
    // let's return the the vector instead to simplify the interface
    virtual uint32_t getInputBufferNum() const = 0;
    virtual std::shared_ptr<StreamBuffer> getInputStreamBuffer(uint32_t index) const = 0;
    virtual int deleteInputStreamBuffer(std::shared_ptr<StreamBuffer> streamBuffer) = 0;
    virtual uint32_t getOutBufferNum() const = 0;
    virtual std::shared_ptr<StreamBuffer> getStreamBuffer(uint32_t index) const = 0;
    virtual int deleteOutputStreamBuffer(std::shared_ptr<StreamBuffer> streamBuffer) = 0;
    virtual std::shared_ptr<StreamBuffer> getBufferByStream(const StreamBuffer *) const
    {
        return nullptr;
    }
    virtual const Metadata *getMeta() const = 0;
    virtual uint32_t getPhyMetaNum() const = 0;
    virtual const char *getPhyCamera(uint32_t id) const = 0;
    virtual const Metadata *getPhyMeta(uint32_t id) const = 0;
    virtual void safeguardMeta() = 0;
    virtual const PhysicalMetas getPhyMetas() const = 0;
    virtual const camera3_capture_request *toRaw() const = 0;
    virtual std::string toString() const;

    CameraDevice *getCamera() const { return mCamera; }
    bool isSnapshot() const;
    bool fromMockCamera() const;

    template <class... Args>
    camera_metadata_ro_entry findInMeta(Args &&... args) const
    {
        const Metadata *meta = getMeta();
        return meta->find(std::forward<Args>(args)...);
    }

    Metadata &getBufferSpecificMeta(StreamBuffer *buffer) { return mDifMetaPerBuf[buffer]; }

protected:
    CameraDevice *mCamera;

    // NOTE: mihal send stream buffer to mialgo in an one-by-one fashion and for every buffer,
    // different metadata may be needed. This map stores the metadatas which should be set
    // differently per buffer, for other metas shared by all buffer, we use metas get from getMeta()
    std::map<StreamBuffer *, Metadata> mDifMetaPerBuf;
};

class RemoteRequest : public CaptureRequest
{
public:
    RemoteRequest(CameraDevice *camera, camera3_capture_request *request);
    ~RemoteRequest() = default;

    uint32_t getFrameNumber() const override { return mRawRequest->frame_number; }
    uint32_t getInputBufferNum() const override { return mRawRequest->input_buffer ? 1 : 0; }
    int deleteInputStreamBuffer(std::shared_ptr<StreamBuffer> streamBuffer) override;
    std::shared_ptr<StreamBuffer> getInputStreamBuffer(uint32_t index) const override
    {
        if (index > 1)
            return nullptr;

        return mInputStreamBuffer;
    }

    uint32_t getOutBufferNum() const override { return mRawRequest->num_output_buffers; }
    std::shared_ptr<StreamBuffer> getStreamBuffer(uint32_t index) const override;
    int deleteOutputStreamBuffer(std::shared_ptr<StreamBuffer> streamBuffer) override;

    uint32_t getPhyMetaNum() const override { return mRawRequest->num_physcam_settings; }
    const char *getPhyCamera(uint32_t id) const { return mRawRequest->physcam_id[id]; }
    const PhysicalMetas getPhyMetas() const override;
    // FIXME: implement later
    const Metadata *getPhyMeta(uint32_t id) const { return nullptr; }

    const camera3_capture_request *toRaw() const override { return mRawRequest; }
    const Metadata *getMeta() const override { return &mMeta; };
    void safeguardMeta() override
    {
        // the remote request do not support this feature
    }
    Metadata *getModifyMeta() { return &mMeta; };
    void submitMetaModified() { mRawRequest->settings = mMeta.peek(); };

private:
    Metadata mMeta;
    camera3_capture_request *mRawRequest;
    PhysicalMetas mPhyMetas;
    std::shared_ptr<StreamBuffer> mInputStreamBuffer;
    std::vector<std::shared_ptr<RemoteStreamBuffer>> mStreamBuffers;
};

class LocalRequest : public CaptureRequest
{
public:
    // METAs are cloned to *this
    LocalRequest(const CaptureRequest &other);

    // NOTE: METAs are created with CopyOnWrite policy
    LocalRequest(CameraDevice *camera, const camera3_capture_request *request);

    LocalRequest(CameraDevice *camera, uint32_t frame,
                 std::vector<std::shared_ptr<StreamBuffer>> outputStreamBuffers,
                 const Metadata &meta, PhysicalMetas phyMetas = PhysicalMetas{});
    LocalRequest(CameraDevice *camera, uint32_t frame,
                 std::vector<std::shared_ptr<StreamBuffer>> inputStreamBuffers,
                 std::vector<std::shared_ptr<StreamBuffer>> outputStreamBuffers,
                 const Metadata &meta, PhysicalMetas phyMetas = PhysicalMetas{});
    ~LocalRequest() = default;

    uint32_t getFrameNumber() const override { return mFrameNumber; }
    void setFrameNumber(uint32_t frame) override { mFrameNumber = frame; }
    uint32_t getInputBufferNum() const override { return mInputStreamBuffers.size(); }
    std::shared_ptr<StreamBuffer> getInputStreamBuffer(uint32_t index) const override
    {
        if (index > mInputStreamBuffers.size())
            return nullptr;

        return mInputStreamBuffers[index];
    }
    int deleteInputStreamBuffer(std::shared_ptr<StreamBuffer> streamBuffer) override;

    uint32_t getOutBufferNum() const override { return mStreamBuffers.size(); }
    std::shared_ptr<StreamBuffer> getStreamBuffer(uint32_t index) const override;
    int addOutputStreamBuffer(std::shared_ptr<StreamBuffer> streamBuffer);
    int deleteOutputStreamBuffer(std::shared_ptr<StreamBuffer> streamBuffer) override;
    std::shared_ptr<StreamBuffer> getBufferByStream(const StreamBuffer *) const override;

    uint32_t getPhyMetaNum() const override { return mPhyMetas.size(); }
    const char *getPhyCamera(uint32_t id) const { return mPhyMetas[id].first.c_str(); }
    const Metadata *getPhyMeta(uint32_t id) const { return mPhyMetas[id].second.get(); }
    void safeguardMeta() override;
    const PhysicalMetas getPhyMetas() const override { return mPhyMetas; }

    const camera3_capture_request *toRaw() const override;
    const Metadata *getMeta() const override { return &mMeta; };

    std::string toString() const override;

    int append(const Metadata &other);

    template <class... Args>
    int updateMeta(Args &&... args)
    {
        if (mMeta.isEmpty()) {
            // NOTE: If mMeta is empty, it indicates that the meta setting from FWK is null.
            // For Example: In Burstshot case, except for the first frame, the meta settings
            // of the remaining frames sent by FWK are all empty. In the past, once we call
            // this method, the raw meta settings will be copied to new values. It won't be
            // null. But this may lead to null pointer crash on vendor hal which depends on
            // some meta but current setting does not has. So we just ensure that the meta of
            // current request from FWK will be passed to vendor hal unchanged.
            return 0;
        }

        int ret = mMeta.update(std::forward<Args>(args)...);
        if (!ret && mRawRequest)
            mRawRequest->settings = mMeta.peek();

        return ret;
    }

private:
    struct RawRequestWraper : public camera3_capture_request
    {
        RawRequestWraper(const LocalRequest &req);
        RawRequestWraper(const camera3_capture_request &req);
        ~RawRequestWraper();
    };

    uint32_t mFrameNumber;
    std::vector<std::shared_ptr<StreamBuffer>> mInputStreamBuffers;
    std::vector<std::shared_ptr<StreamBuffer>> mStreamBuffers;
    Metadata mMeta;
    PhysicalMetas mPhyMetas;
    mutable std::unique_ptr<RawRequestWraper> mRawRequest;
};

class CaptureResult
{
public:
    CaptureResult(CameraDevice *device) : camera{device} {}
    CaptureResult(const CaptureResult &other) = default;
    virtual ~CaptureResult() = default;

    CameraDevice *getCamera() const { return camera; };
    virtual uint32_t getFrameNumber() const = 0;
    virtual void setFrameNumber(uint32_t frame) = 0;
    virtual uint32_t getPartial() const = 0;
    virtual std::shared_ptr<StreamBuffer> getInputStreamBuffer() const = 0;
    virtual uint32_t getBufferNumber() const = 0;
    // TODO: here streamIdx should be consistent in syntax, which means
    //       we need to check the stream?
    virtual std::shared_ptr<StreamBuffer> getStreamBuffer(uint32_t index) const = 0;
    virtual const Metadata *getMeta() const = 0;
    virtual const PhysicalMetas getPhyMetas() const = 0;
    virtual const camera3_capture_result *toRaw() const = 0;
    virtual bool isWritable() const { return false; }
    virtual bool isErrorResult() const = 0;
    virtual bool isSnapshotError() const = 0;
    virtual std::string toString() const;

    template <class... Args>
    camera_metadata_ro_entry findInMeta(Args &&... args) const
    {
        const Metadata *meta = getMeta();
        if (!meta) {
            return camera_metadata_ro_entry{.count = 0};
        }

        return meta->find(std::forward<Args>(args)...);
    }

protected:
    CameraDevice *camera;
};

class RemoteResult : public CaptureResult
{
public:
    RemoteResult(CameraDevice *camera, const camera3_capture_result *result);
    ~RemoteResult();

    uint32_t getFrameNumber() const override { return mRawResult->frame_number; }
    void setFrameNumber(uint32_t frame) override
    {
        const_cast<camera3_capture_result *>(mRawResult)->frame_number = frame;
    }

    uint32_t getPartial() const override { return mRawResult->partial_result; }
    std::shared_ptr<StreamBuffer> getInputStreamBuffer() const override { return nullptr; }
    uint32_t getBufferNumber() const override { return mRawResult->num_output_buffers; }
    std::shared_ptr<StreamBuffer> getStreamBuffer(uint32_t index) const override
    {
        return mStreamBuffers[index];
    }
    const Metadata *getMeta() const override { return mMeta.get(); }
    const PhysicalMetas getPhyMetas() const override;
    const camera3_capture_result *toRaw() const override { return mRawResult; }
    bool isErrorResult() const override;
    bool isSnapshotError() const override;

private:
    std::unique_ptr<Metadata> mMeta;
    const camera3_capture_result *mRawResult;
    std::vector<std::shared_ptr<RemoteStreamBuffer>> mStreamBuffers;
};

class LocalResult : public CaptureResult
{
public:
    LocalResult(CameraDevice *device, uint32_t frameNumber, uint32_t partial,
                std::unique_ptr<Metadata> meta);

    LocalResult(CameraDevice *device, uint32_t frameNumber, uint32_t partial,
                std::vector<std::shared_ptr<StreamBuffer>> outBuffers,
                std::unique_ptr<Metadata> meta = std::make_unique<Metadata>());

    LocalResult(CameraDevice *device, uint32_t frameNumber, uint32_t partial,
                std::shared_ptr<StreamBuffer> inBuffer,
                std::vector<std::shared_ptr<StreamBuffer>> outBuffers = {},
                std::unique_ptr<Metadata> meta = std::make_unique<Metadata>());

    LocalResult(const RemoteResult &result);
    ~LocalResult() = default;

    uint32_t getFrameNumber() const override { return mFrameNumber; }
    void setFrameNumber(uint32_t frame) override
    {
        mFrameNumber = frame;

        if (mRawResult)
            mRawResult->frame_number = frame;
    }

    uint32_t getPartial() const override { return mPartial; }
    std::shared_ptr<StreamBuffer> getInputStreamBuffer() const override { return mInputBuffer; }
    uint32_t getBufferNumber() const override { return mOutBuffers.size(); }
    std::shared_ptr<StreamBuffer> getStreamBuffer(uint32_t index) const override
    {
        return mOutBuffers[index];
    }
    const Metadata *getMeta() const override { return mMeta.get(); }
    const PhysicalMetas getPhyMetas() const override { return mPhyMetas; }
    const camera3_capture_result *toRaw() const override;
    bool isErrorResult() const override;
    bool isSnapshotError() const override;

    bool isWritable() const override { return true; }

    template <class... Args>
    int updateMeta(Args &&... args)
    {
        int ret = mMeta->update(std::forward<Args>(args)...);

        if (mRawResult)
            mRawResult->result = mMeta->peek();

        return ret;
    }

private:
    struct RawResultWraper : public camera3_capture_result
    {
        RawResultWraper(const LocalResult &result);
        RawResultWraper(const camera3_capture_result &result);
        ~RawResultWraper();
    };

    uint32_t mFrameNumber;
    uint32_t mPartial;
    std::shared_ptr<StreamBuffer> mInputBuffer;
    std::vector<std::shared_ptr<StreamBuffer>> mOutBuffers;
    std::unique_ptr<Metadata> mMeta;
    PhysicalMetas mPhyMetas;
    mutable std::unique_ptr<RawResultWraper> mRawResult;
};

class StreamConfig
{
public:
    StreamConfig(CameraDevice *camera) : mCamera{camera} {}
    virtual ~StreamConfig() = default;

    CameraDevice *getCamera() const { return mCamera; }
    virtual uint32_t getOpMode() const = 0;
    virtual uint32_t getStreamNums() const = 0;
    virtual std::shared_ptr<Stream> getPhyStream(
        std::string phyId, std::function<bool(std::shared_ptr<Stream>)> wanted = nullptr) const = 0;
    virtual int forEachStream(std::function<int(const camera3_stream *)>) const = 0;
    virtual int forEachStream(std::function<int(std::shared_ptr<Stream>)>) const = 0;
    virtual camera3_stream_configuration *toRaw() const = 0;
    virtual const Metadata *getMeta() const = 0;
    virtual std::string toString() const;
    virtual Metadata *getModifyMeta() = 0;

private:
    CameraDevice *mCamera;
};

class RemoteConfig : public StreamConfig
{
public:
    RemoteConfig(CameraDevice *camera, camera3_stream_configuration *config);
    ~RemoteConfig() = default;

    uint32_t getOpMode() const override { return mConfig->operation_mode; }
    uint32_t getStreamNums() const override { return mConfig->num_streams; }
    std::shared_ptr<Stream> getPhyStream(
        std::string phyId,
        std::function<bool(std::shared_ptr<Stream>)> wanted = nullptr) const override
    {
        // NOT supported for a RemoteConfig
        return nullptr;
    }

    int forEachStream(std::function<int(const camera3_stream *)>) const override;
    int forEachStream(std::function<int(std::shared_ptr<Stream>)>) const override;
    camera3_stream_configuration *toRaw() const override { return mConfig; };
    const Metadata *getMeta() const override { return &mMeta; }
    Metadata *getModifyMeta() { return &mMeta; }

private:
    camera3_stream_configuration *mConfig;
    Metadata mMeta;
};

class LocalConfig : public StreamConfig
{
public:
    LocalConfig(std::vector<std::shared_ptr<Stream>> streams, uint32_t opMode,
                Metadata &sessionPara);
    // NOTe: consturct from RemoteConfig
    LocalConfig(const StreamConfig &other);
    ~LocalConfig();

    uint32_t getOpMode() const override { return mOpMode; }
    uint32_t getStreamNums() const override { return mStreams.size(); }
    std::shared_ptr<Stream> getPhyStream(
        std::string phyId,
        std::function<bool(std::shared_ptr<Stream>)> wanted = nullptr) const override;
    int forEachStream(std::function<int(const camera3_stream *)>) const override;
    int forEachStream(std::function<int(std::shared_ptr<Stream>)>) const override;
    camera3_stream_configuration *toRaw() const override;
    const Metadata *getMeta() const override { return &mSessionPara; }
    const std::vector<std::shared_ptr<Stream>> getStreams() const { return mStreams; }
    bool isEqual(const std::shared_ptr<LocalConfig> config) const;
    Metadata *getModifyMeta() { return &mSessionPara; }

private:
    std::vector<std::shared_ptr<Stream>> mStreams;
    uint32_t mOpMode;
    Metadata mSessionPara;

    // TODO: how to handle with the raw config?
    // TODO: refactor: support to modify the original streams
    mutable std::unique_ptr<camera3_stream_configuration> mRawConfig;
};

class NotifyMessage
{
public:
    NotifyMessage(CameraDevice *camera, const camera3_notify_msg *msg);
    NotifyMessage(const NotifyMessage &other, uint32_t frame);
    NotifyMessage(CameraDevice *camera, uint32_t frame, camera3_stream *stream);
    NotifyMessage(CameraDevice *camera, uint32_t frame, int64_t ts);

    // NOTE: error should be either CAMERA3_MSG_ERROR_REQUEST or CAMERA3_MSG_ERROR_RESULT
    NotifyMessage(CameraDevice *camera, uint32_t frame, int error);

    ~NotifyMessage();
    CameraDevice *getCamera() const { return mCamera; }
    uint32_t getFrameNumber() const;
    uint64_t getTimeStamp() const;
    camera3_stream *getErrorStream() const { return mRawMsg->message.error.error_stream; }
    int getErrorCode() const { return mRawMsg->message.error.error_code; }
    void notify() const;
    bool isErrorMsg() const;
    std::string toString() const;
    const camera3_notify_msg *toRaw() const { return mRawMsg; };

private:
    CameraDevice *mCamera;
    const camera3_notify_msg *mRawMsg;
    bool mOwnerOfRaw;
};

class BufferRequests
{
public:
    BufferRequests() = delete;
    BufferRequests(CameraDevice *camera, uint32_t num_buffer_reqs,
                   const camera3_buffer_request *buffer_reqs);

    std::string toString() const;

private:
    using Request = std::pair<std::shared_ptr<Stream>, uint32_t /* num */>;

    CameraDevice *mCamera;
    std::vector<Request> mReqs;
};

class PreProcessConfig
{
public:
    PreProcessConfig(CameraDevice *camera, std::vector<camera3_stream> streams,
                     const Metadata *meta);

    virtual ~PreProcessConfig() = default;

    CameraDevice *getCamera() const { return mCamera; }
    std::vector<camera3_stream> getStreams() const { return mStreams; }
    const Metadata *getMeta() const { return mMeta.get(); }

private:
    CameraDevice *mCamera;
    std::vector<camera3_stream> mStreams;
    std::unique_ptr<Metadata> mMeta;
};

} // namespace mihal

#endif
