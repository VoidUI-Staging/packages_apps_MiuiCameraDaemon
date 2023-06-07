#ifndef __STREAM_H__
#define __STREAM_H__

#include <hardware/camera3.h>

#include <bitset>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include "BufferManager.h"
#include "CamGeneralDefinitions.h"
#include "Timer.h"

namespace mihal {

class CameraDevice;
class StreamBuffer;

// Refer to: hardware/interfaces/camera/device/aidl/android/hardware/camera/device/HalStream.aidl
struct Camera3HalStream
{
    // Stream ID - a nonnegative integer identifier for a stream.
    int32_t id;

    // An override pixel format for the buffers in this stream
    HALPixelFormat overrideFormat;

    // The gralloc producer usage flags for this stream, as needed by the HAL.
    uint64_t producerUsage;

    // The gralloc consumer usage flags for this stream, as needed by the HAL.
    uint64_t consumerUsage;

    // The maximum number of buffers the HAL device may need to have dequeued at the same time
    int maxBuffers;

    // A bitfield override dataSpace for the buffers in this stream.
    uint64_t overrideDataSpace;

    // The physical camera id the current Hal stream belongs to
    std::string physicalCameraId;

    // Whether this stream can be switch to offline mode
    bool supportOffline;
};

class Stream : public std::enable_shared_from_this<Stream>
{
public:
    using BufferListener = std::function<void(bool increased)>;
    enum StreamProp {
        PRIMARY_PREVIEW = 0,
        AUX_PREVIEW,
        SNAPSHOT,
        REALTIME,
        REMOSAIC,
        QUICKVIEW,
        TRIVIAL,
        HEIC_YUV,
        HEIC_THUMBNAIL
    };

    virtual ~Stream();

    bool operator==(const Stream &other) const { return toRaw() == other.toRaw(); }

    virtual int getType() const = 0;
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual uint32_t getUsage() const = 0;
    virtual uint64_t getStreamUsecase() const = 0;
    virtual void setStreamUsecase(uint64_t stream_use_case) const = 0;
    virtual void setUsage(uint32_t usage) const = 0;
    virtual int getFormat() const = 0;
    virtual void setFormat(int format) const = 0;
    virtual uint32_t getMaxBuffers() const = 0;
    virtual void setMaxBuffers(uint32_t max) = 0;
    virtual void *getCookie() const = 0;
    virtual void setCookie(void *cookie) = 0;
    virtual android_dataspace_t getDataspace() const = 0;
    virtual void setDataspace(android_dataspace_t dataSpace) = 0;
    virtual const char *getPhyId() const = 0;
    virtual camera3_stream *toRaw() const = 0;

    CameraDevice *getCamera() const { return mCamera; }
    static std::shared_ptr<Stream> toRichStream(const camera3_stream *stream);
    static int setRetired(const camera3_stream *stream);

    // NOTE: StreamBuffer is created by Stream, but released by itself
    // StreamBuffer could release the buffer by either call StreamBuffer.releaseBuffer or by dctor
    std::shared_ptr<StreamBuffer> requestBuffer();
    // NOTE: 'this->releaseBuffer' is only called by LocalStreamBuffer to release GraBuffer
    void releaseBuffer(std::unique_ptr<GraBuffer> buffer);
    // NOTE: for details, please  refer to:
    // https://xiaomi.f.mioffice.cn/docs/dock4sX28Wx0yYJSKu1UVCDukzc
    void releaseVirtualBuffer();

    // NOTE: these 2 interfaces are dedicated for **internal stream** HAL3 BufferManager
    // implementation
    int requestBuffers(uint32_t num, std::vector<StreamBuffer *> &buffers);
    std::unique_ptr<StreamBuffer> getRequestedStreamBuffer(const StreamBuffer &buffer);

    // NOTE: return all the buffers requested by MiHAL
    int returnStreamBuffers();

    // NOTE: return a requested buffer owned by MiHAL
    int returnStreamBuffer(const camera3_stream_buffer &buffer);

    void setBufferCapacity(uint32_t capacity);
    uint32_t getBufferCapacity() const;
    uint32_t getAvailableBuffers() const;
    // NOTE: for details, please refer to design doc:
    // https://xiaomi.f.mioffice.cn/docs/dock4sX28Wx0yYJSKu1UVCDukzc
    uint32_t getVirtualAvailableBuffers() const;
    uint32_t getPendingBuffersInVendor() const;

    uint32_t getRequestedBuffersSzie();
    int registerListener(BufferListener listener);
    int unregisterListener();

    void setStreamId(uint32_t id) { mId = id; }
    uint32_t getStreamId() const { return mId; };
    void setStreamProp(std::bitset<16> prop) { mProp = prop; };
    void setStreamProp(StreamProp position1) { mProp.set(position1); };

    template <typename... Pos>
    void setStreamProp(StreamProp position1, Pos... positions)
    {
        mProp.set(position1);
        setStreamProp(positions...);
    };

    bool isSnapshot() const { return mProp.test(SNAPSHOT); }

    bool isPreview() const { return mProp.test(PRIMARY_PREVIEW); }

    bool isQuickview() const { return mProp.test(QUICKVIEW); }

    bool isAuxPreview() const { return mProp.test(AUX_PREVIEW); }

    bool isRemosaic() const { return mProp.test(REMOSAIC); }

    bool isRealtime() const { return mProp.test(REALTIME); }

    bool isTrivial() const { return mProp.test(TRIVIAL); }

    bool isHeicYuv() const { return mProp.test(HEIC_YUV); }

    bool isHeicThumbnail() const { return mProp.test(HEIC_THUMBNAIL); }

    bool isInternal() const { return mIsInternal; }
    std::string toString(int indent = 0, bool full = false) const;
    int setRetired();

    bool hasPeerStream() { return mPeerStream != nullptr; }
    Stream *getPeerStream() { return mPeerStream; }
    void setPeerStream(Stream *stream) { mPeerStream = stream; }

    void setConfiguredToHal() { mIsConfiguredToHal = true; }
    bool isConfiguredToHal() { return mIsConfiguredToHal; }
    void resetConsumeCnt() { mConsumeCnt.store(0); }
    uint32_t getConsumeCnt() { return mConsumeCnt.load(); }

    Camera3HalStream *getHalStream()
    {
        camera3_stream *raw = toRaw();
        if (raw->reserved[0]) {
            return reinterpret_cast<Camera3HalStream *>(raw->reserved[0]);
        } else {
            return &mHalStream;
        }
    }

    void setHalStream(Camera3HalStream *halStream)
    {
        camera3_stream *raw = toRaw();
        raw->reserved[0] = reinterpret_cast<void *>(halStream);
    }

protected:
    Stream(CameraDevice *camera);
    Stream(CameraDevice *camera, uint32_t id);
    Stream(CameraDevice *camera, std::bitset<16> prop);
    Stream(CameraDevice *camera, uint32_t id, bool isInternal, std::bitset<16> prop);
    std::vector<std::unique_ptr<GraBuffer>> requestGraBuffers(uint32_t num);

    bool mIsInternal;
    bool mRetired;
    mutable std::recursive_mutex mMutex;
    std::condition_variable_any mCond;
    std::condition_variable_any mPutBufferCond;

private:
    std::unique_ptr<GraBuffer> requestGraBuffer();
    int returnInternalStreamBuffers();
    int returnInternalStreamBuffer(const camera3_stream_buffer &buffer);
    int returnFwkStreamBuffers();
    int returnFwkStreamBuffer(const camera3_stream_buffer &buffer);

    CameraDevice *mCamera;
    uint32_t mId;
    uint32_t mPendingBuffersInVendor;
    std::bitset<16> mProp;
    std::unique_ptr<BufferManager> mBufferManager;
    std::shared_mutex mBufferListenerMutex;
    BufferListener mBufferListener;
    std::shared_ptr<Timer> mTimer;
    std::vector<camera3_stream_buffer> mPutBuffers;
    std::map<buffer_handle_t, std::unique_ptr<StreamBuffer>> mRequestedBuffers;

    // NOTE: this variable is used to count how many buffers is reserved for snapshot. for details,
    // please refer to design doc: https://xiaomi.f.mioffice.cn/docs/dock4sX28Wx0yYJSKu1UVCDukzc
    std::atomic<int> mVirtualBusyBufferCnt = 0;
    std::atomic<int> mConsumeCnt = 0;

    // NOTE: refer to doc: https://xiaomi.f.mioffice.cn/docs/dock4qQImIBogDbm7qEcpxLllld for what
    // mPeerStream means: fwk preview stream's peer stream is internal preview stream and vice versa
    // fwk QR Scan stream's peer stream is internal QR Scan stream and vice versa
    Stream *mPeerStream = nullptr;

    // NOTE: used to denote whether this stream is configured to Qcom
    bool mIsConfiguredToHal = false;

    Camera3HalStream mHalStream; //  Placeholder for aidl hal stream
};

class RemoteStream : public Stream
{
public:
    static std::shared_ptr<Stream> create(CameraDevice *camera, camera3_stream *stream,
                                          std::bitset<16> prop = std::bitset<16>{});
    RemoteStream() = delete;
    ~RemoteStream();

    int getType() const override { return mStream->stream_type; }
    uint32_t getWidth() const override { return mStream->width; }
    uint32_t getHeight() const override { return mStream->height; }
    uint32_t getUsage() const override { return mStream->usage; }
    uint64_t getStreamUsecase() const override { return mStream->stream_use_case; }
    void setStreamUsecase(uint64_t stream_use_case) const override
    {
        mStream->stream_use_case = stream_use_case;
    }
    void setUsage(uint32_t usage) const override { mStream->usage = usage; }
    int getFormat() const override { return mStream->format; }
    void setFormat(int format) const override { mStream->format = format; }
    uint32_t getMaxBuffers() const override { return mStream->max_buffers; }
    void setMaxBuffers(uint32_t max) override { mStream->max_buffers = max; }
    void *getCookie() const override { return mStream->priv; }
    void setCookie(void *cookie) override { mStream->priv = cookie; }
    android_dataspace_t getDataspace() const override { return mStream->data_space; }
    void setDataspace(android_dataspace_t dataSpace) override { mStream->data_space = dataSpace; }
    const char *getPhyId() const override { return mStream->physical_camera_id; }
    camera3_stream *toRaw() const override { return mStream; }

private:
    RemoteStream(CameraDevice *camera, camera3_stream *stream, std::bitset<16> prop);
    camera3_stream *mStream;
};

class LocalStream : public Stream
{
public:
    static std::shared_ptr<Stream> create(CameraDevice *camera, uint32_t width, uint32_t height,
                                          int format, uint32_t usage, uint64_t stream_use_case,
                                          android_dataspace_t space, bool internal = true,
                                          const char *phyId = "", int type = CAMERA3_STREAM_OUTPUT);
    LocalStream() = delete;
    ~LocalStream();

    int getType() const override { return mStream->stream_type; }
    uint32_t getWidth() const override { return mStream->width; }
    uint32_t getHeight() const override { return mStream->height; }
    uint32_t getUsage() const override { return mStream->usage; }
    uint64_t getStreamUsecase() const override { return mStream->stream_use_case; }
    void setStreamUsecase(uint64_t stream_use_case) const override
    {
        mStream->stream_use_case = stream_use_case;
    }
    void setUsage(uint32_t usage) const override { mStream->usage = usage; }
    int getFormat() const override { return mStream->format; }
    void setFormat(int format) const override { mStream->format = format; }
    uint32_t getMaxBuffers() const override { return mStream->max_buffers; }
    void setMaxBuffers(uint32_t max) override { mStream->max_buffers = max; }
    void *getCookie() const override { return mStream->priv; }
    void setCookie(void *cookie) override { mStream->priv = cookie; }
    android_dataspace_t getDataspace() const override { return mStream->data_space; }
    void setDataspace(android_dataspace_t dataSpace) override { mStream->data_space = dataSpace; }
    const char *getPhyId() const override { return mStream->physical_camera_id; }
    camera3_stream *toRaw() const override { return mStream.get(); }

private:
    LocalStream(CameraDevice *camera, uint32_t width, uint32_t height, int format, uint32_t usage,
                uint64_t stream_use_case, android_dataspace_t space, bool internal,
                const char *phyId, int type);
    struct RawStreamWrapper : public camera3_stream
    {
        // RawStreamWrapper(const Stream &stream);
        // RawStreamWrapper(const camera3_stream &stream);
        RawStreamWrapper(uint32_t width, uint32_t height, int format, uint32_t usage,
                         uint64_t stream_use_case, android_dataspace_t space, const char *phyId,
                         int type);
        ~RawStreamWrapper();
    };

    std::unique_ptr<RawStreamWrapper> mStream;
};

} // namespace mihal

#endif
