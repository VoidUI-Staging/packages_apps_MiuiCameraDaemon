#ifndef MIPOSTPROCDEVICEUTIL_H
#define MIPOSTPROCDEVICEUTIL_H

#include <iostream>
#include <memory>

#include "MiDebugUtils.h"
#include "mtkcam-halif/device/1.x/IMtkcamDevice.h"
#include "mtkcam-halif/device/1.x/IMtkcamDeviceCallback.h"
#include "mtkcam-halif/device/1.x/IMtkcamDeviceSession.h"
#include "mtkcam-halif/device/1.x/types.h"
#include "mtkcam-halif/provider/1.x/IMtkcamProvider.h"
#include "mtkcam-halif/utils/imgbuf/1.x/IImageBuffer.h"
#include "mtkcam-halif/utils/metadata_tag/1.x/mtk_metadata_tag.h"

namespace mizone {
namespace mipostprocinterface {

using namespace midebug;

using mcam::HalStream;
using mcam::HalStreamConfiguration;
using mcam::PhysicalCameraSetting;
using mcam::Stream;
using mcam::StreamConfiguration;
using mcam::StreamConfigurationMode;
using mcam::StreamType;

using mcam::IMtkcamDevice;
using mcam::IMtkcamDeviceCallback;
using mcam::IMtkcamDeviceSession;
using mcam::IMtkcamProvider;

using mcam::CaptureRequest;
using mcam::CaptureResult;
using mcam::SubRequest;

using mcam::BufferRequest;
using mcam::BufferStatus;
using mcam::Dataspace;
using mcam::IImageBufferHeap;
using mcam::StreamBuffer;
using NSCam::EImageFormat;

using mcam::ErrorCode;
using mcam::MsgType;
using mcam::NotifyMsg;

#define LogGroupType LogGroupHAL

enum EngineType : uint32_t {
    ENGINE_CAPTURE = 0,
};

enum CapFeatureType : uint32_t {
    FEATURE_MFNR = 0,
    FEATURE_AINR,
    FEATURE_AIHDR,
    FEATURE_AISHUTTER1,
    FEATURE_AISHUTTER2,
    FEATURE_R2R,
    FEATURE_R2Y,
    FEATURE_Y2Y,
    FEATURE_Y2J,
};

enum class ResultStatus : uint32_t {
    OK = 0,
    ERROR = 1,
};

/**
 * configure mode.
 *
 * ONCE: feature only needs to be configured for the first time, not when it appears again.
 *       [Note: must need add a new R2Y hwplugin]
 *
 * REPEATED: need to configure each time before sending request.
 *
 */
enum class ConfigMode : uint32_t {
    ONCE = 0,
    REPEATED,
};

struct CallbackInfos
{
    ResultStatus msg = ResultStatus::OK;
    std::string settings; // TODO metadata
};

class MiPostProcPluginCb
{
public:
    virtual ~MiPostProcPluginCb() {}
    virtual void processResultDone(CallbackInfos &callbackInfos) = 0;
};

class MiPostProcAdaptorCb
{
public:
    virtual ~MiPostProcAdaptorCb() {}
    virtual void handleNotify(const std::vector<NotifyMsg> &msgs) = 0;
    virtual void handleResult(const std::vector<CaptureResult> &results) = 0;
};

/**
 * Structure used to create PostProc.
 *
 * @param engineType: mtk platform PostProc engine, default(Capture Engine).
 *
 */
struct CreationInfos
{
    EngineType engineType = ENGINE_CAPTURE;
};

/**
 * Structure used to MiPluginAdaptor process function.
 *
 * @param xxxFeatureType: feature used by engine. Currently only using Capture engine.
 *
 * @param configMode: confiure streams mode. Currently using ONCE.
 *
 * @param multiFrameNum: the number of buffers passed in by hwplugin. for example:MFNR/AINR frameCnt
 *
 * @param cb: hwplugin callback handle
 *
 * @param adaptorCb: MiPluginAdaptor callback handle
 *
 */
struct ProcessInfos
{
    CapFeatureType capFeatureType = FEATURE_MFNR;
    ConfigMode configMode = ConfigMode::ONCE;
    int32_t multiFrameNum = 0;
    MiPostProcPluginCb *cb;
    MiPostProcAdaptorCb *adaptorCb;
};

struct AdaptorParams
{
    StreamConfiguration streamCfgIn;
    std::vector<CaptureRequest> requestsIn;
    CapFeatureType capFeatureType;
};

static inline auto dumpStream(const Stream &stream) -> std::string
{
    std::stringstream ss;

    ss << "{ ";
    ss << "id: " << stream.id << ", ";
    ss << "streamType: " << static_cast<int>(stream.streamType) << ", ";
    ss << "width: " << stream.width << ", ";
    ss << "height: " << stream.height << ", ";
    ss << "usage: " << stream.usage << ", ";
    ss << "physicalCameraId: " << stream.physicalCameraId << ", ";
    ss << "bufferSize: " << stream.bufferSize << ", ";
    ss << "transform: " << stream.transform << ", ";
    ss << "format: " << stream.format << ", ";
    ss << "dataSpace: " << static_cast<int>(stream.dataSpace) << ", ";
    ss << "settings.count: " << stream.settings.count() << ", ";
    ss << "bufPlanes.count: " << stream.bufPlanes.count << ", ";
    ss << "}";

    return ss.str();
}

static inline auto dumpHalStream(const HalStream &stream) -> std::string
{
    std::stringstream ss;

    ss << "{ ";
    ss << "id: " << stream.id << ", ";
    ss << "overrideFormat: " << stream.overrideFormat << ", ";
    ss << "producerUsage: " << stream.producerUsage << ", ";
    ss << "consumerUsage: " << stream.consumerUsage << ", ";
    ss << "maxBuffers: " << stream.maxBuffers << ", ";
    ss << "overrideDataSpace: " << static_cast<int>(stream.overrideDataSpace) << ", ";
    ss << "physicalCameraId: " << stream.physicalCameraId << ", ";
    ss << "supportOffline: " << stream.supportOffline << ", ";
    ss << "results.count: " << stream.results.count() << ", ";
    ss << "}";

    return ss.str();
}

static inline auto dumpStreamConfiguration(const StreamConfiguration &conf) -> std::string
{
    std::stringstream ss;

    ss << "{ ";
    ss << "operationMode: 0x" << std::hex << static_cast<int>(conf.operationMode) << ", ";
    ss << "sessionParams.count: " << std::dec << conf.sessionParams.count() << ", ";
    ss << "streamConfigCounter: " << conf.streamConfigCounter << ", ";
    ss << "streams(" << conf.streams.size() << "): ";
    ss << "[ ";
    for (auto &&stream : conf.streams) {
        ss << dumpStream(stream) << ", ";
    }
    ss << "], ";
    ss << "}";

    return ss.str();
}

static inline auto dumpHalStreamConfiguration(const HalStreamConfiguration &conf) -> std::string
{
    std::stringstream ss;
    ss << "{ ";

    ss << "sessionResults.count: " << conf.sessionResults.count() << ", ";
    ss << "streams(" << conf.streams.size() << "): ";
    ss << "[ ";
    for (auto &&stream : conf.streams) {
        ss << dumpHalStream(stream) << ", ";
    }
    ss << "], ";

    ss << "}";
    return ss.str();
}

static inline auto dumpImageBuffer(const std::shared_ptr<IImageBufferHeap> &buffer) -> std::string
{
    std::stringstream ss;
    ss << "{ ";

    ss << "format: " << buffer->getImgFormat() << ", ";
    ss << "width: " << buffer->getImgSize().w << ", ";
    ss << "height: " << buffer->getImgSize().h << ", ";
    for (auto i = 0; i < buffer->getPlaneCount(); ++i) {
        ss << "plane: " << i << ", ";
        ss << "stride: " << buffer->getBufStridesInBytes(i) << ", ";
        ss << "bufferSize: " << buffer->getBufSizeInBytes(i) << ", ";
    }

    ss << "}";
    return ss.str();
}

static inline auto dumpStreamBuffer(const StreamBuffer &buffer) -> std::string
{
    std::stringstream ss;
    ss << "{ ";

    ss << "streamId: " << buffer.streamId << ", ";
    ss << "bufferId: " << buffer.bufferId << ", ";
    ss << "status: " << static_cast<int>(buffer.status) << ", ";
    ss << "acquireFenceFd: " << buffer.acquireFenceFd << ", ";
    ss << "releaseFenceFd: " << buffer.releaseFenceFd << ", ";
    ss << "bufferSettings.count: " << buffer.bufferSettings.count() << ", ";
    ss << "buffer(0x" << buffer.buffer.get() << "): " << dumpImageBuffer(buffer.buffer) << ", ";
    ss << "}";

    return ss.str();
}

static inline auto dumpStreamBuffers(const std::vector<StreamBuffer> &buffers, std::string name)
    -> std::string
{
    std::stringstream ss;
    ss << name << ": " << buffers.size() << ", ";
    for (const auto &buffer : buffers) {
        ss << "{ ";
        ss << "streamId: " << buffer.streamId << ", ";
        ss << "bufferId: " << buffer.bufferId << ", ";
        ss << "status: " << static_cast<int>(buffer.status) << ", ";
        ss << "acquireFenceFd: " << buffer.acquireFenceFd << ", ";
        ss << "releaseFenceFd: " << buffer.releaseFenceFd << ", ";
        ss << "bufferSettings.count: " << buffer.bufferSettings.count() << ", ";
        ss << "buffer(0x" << buffer.buffer.get() << "): " << dumpImageBuffer(buffer.buffer) << ", ";
        ss << "}";
    }
    return ss.str();
}

static inline auto dumpSubRequest(const SubRequest &subReq) -> std::string
{
    std::stringstream ss;
    ss << "{ ";
    ss << "subFrameIndex: " << subReq.subFrameIndex << ", ";
    ss << "settings.count: " << subReq.settings.count() << ", ";
    ss << "halSettings.count: " << subReq.halSettings.count() << ", ";
    ss << "physicalCameraSettings.size: " << subReq.physicalCameraSettings.size() << ", ";
    if (!subReq.inputBuffers.empty()) {
        ss << "inputBuffers(" << subReq.inputBuffers.size() << "): [";
        for (const auto &buffer : subReq.inputBuffers) {
            ss << dumpStreamBuffer(buffer) << ", ";
        }
        ss << "], ";
    }
    ss << "}";

    return ss.str();
}

static inline auto dumpRequest(const CaptureRequest &request) -> std::string
{
    std::stringstream ss;
    ss << "{ ";
    ss << "frameNumber: " << request.frameNumber << ", ";
    ss << "settings.count: " << request.settings.count() << ", ";
    ss << "halSettings.count: " << request.halSettings.count() << ", ";
    ss << "physicalCameraSettings.size: " << request.physicalCameraSettings.size() << ", ";

    return ss.str();
}

static inline auto dumpNotify(const NotifyMsg &msg) -> std::string
{
    std::stringstream ss;
    ss << "{ ";
    if (msg.type == MsgType::ERROR) {
        ss << "type: error, frameNumber: " << msg.msg.error.frameNumber << ", ";
        ss << "errorCode: " << static_cast<int>(msg.msg.error.errorCode) << ", ";
        ss << "errorStreamId: " << msg.msg.error.errorStreamId << ", ";
    } else if (msg.type == MsgType::SHUTTER) {
        ss << "type: shutter, frameNumber: " << msg.msg.shutter.frameNumber << ", ";
        ss << "timestamp: " << msg.msg.shutter.timestamp << ", ";
    } else {
        ss << "type: unkown, ";
    }
    ss << "}";
    return ss.str();
}

static inline auto dumpResult(const CaptureResult &result) -> std::string
{
    std::stringstream ss;
    ss << "{ ";
    ss << "frameNumber: " << result.frameNumber << ", ";
    ss << "result: " << result.result.count() << ", ";
    ss << "halResult: " << result.halResult.count() << ", ";
    ss << "physicalCameraMetadata: " << result.physicalCameraMetadata.size() << ", ";
    ss << "bLastPartialResult: " << result.bLastPartialResult << ", ";

    if (!result.inputBuffers.empty()) {
        ss << "inputBuffers(" << result.inputBuffers.size() << "): [";
        for (const auto &buffer : result.inputBuffers) {
            ss << dumpStreamBuffer(buffer) << ", ";
        }
        ss << "], ";
    }

    ss << "outputBuffers(" << result.outputBuffers.size() << "): [";
    for (const auto &buffer : result.outputBuffers) {
        ss << dumpStreamBuffer(buffer) << ", ";
    }
    ss << "], ";

    ss << "}";

    return ss.str();
}

static inline auto dumpBuffer(const std::shared_ptr<IImageBufferHeap> &buffer,
                              const std::string &name) -> void
{
    std::string typeName;
    auto format = buffer->getImgFormat();
    switch (format) {
    case NSCam::eImgFmt_BAYER10:
    case NSCam::eImgFmt_BAYER16_UNPAK:
    case NSCam::eImgFmt_BAYER12_UNPAK:
    case NSCam::eImgFmt_RAW16:
    case NSCam::eImgFmt_BLOB:
        typeName = ".raw";
        break;
    case NSCam::eImgFmt_YVU_P010:
    case NSCam::eImgFmt_NV21:
    case NSCam::eImgFmt_NV12:
        typeName = ".yuv";
        break;
    }

    auto imgBuf = buffer->createImageBuffer();
    imgBuf->lockBuf("HalIFTest", mcam::eBUFFER_USAGE_SW_READ_OFTEN |
                                     mcam::eBUFFER_USAGE_SW_WRITE_OFTEN |
                                     mcam::eBUFFER_USAGE_HW_CAMERA_READWRITE);
    std::string path("/data/vendor/camera/mizone_image/");
    path += name + "_" + "format_" + std::to_string(format) + "_" +
            std::to_string(buffer->getImgSize().w) + "x" + std::to_string(buffer->getImgSize().h) +
            typeName;
    MLOGD(LogGroupType, "dump buffer to %s", path.c_str());
    auto ok = imgBuf->saveToFile(path.c_str());
    MLOGD(LogGroupType, "dump buffer to %s, ok = %d", path.c_str(), ok);
    imgBuf->unlockBuf("HalIFTest");
}

} // namespace mipostprocinterface
} // namespace mizone

#endif
