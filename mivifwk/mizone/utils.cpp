/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "utils.h"

#include <MiDebugUtils.h>

#include "MiImageBuffer.h"

using namespace midebug;

namespace mizone::utils {

std::string toString(const Stream &stream)
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

std::string toString(const HalStream &stream)
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

std::string toString(const StreamConfiguration &conf)
{
    std::stringstream ss;

    ss << "{ ";
    ss << "operationMode: 0x" << std::hex << static_cast<int>(conf.operationMode) << ", ";
    ss << "sessionParams.count: " << std::dec << conf.sessionParams.count() << ", ";
    ss << "streamConfigCounter: " << conf.streamConfigCounter << ", ";
    ss << "streams(" << conf.streams.size() << "): ";
    ss << "[ ";
    for (auto &&stream : conf.streams) {
        ss << toString(stream) << ", ";
    }
    ss << "], ";
    ss << "}";

    return ss.str();
}

std::string toString(const HalStreamConfiguration &conf)
{
    std::stringstream ss;
    ss << "{ ";

    ss << "sessionResults.count: " << conf.sessionResults.count() << ", ";
    ss << "streams(" << conf.streams.size() << "): ";
    ss << "[ ";
    for (auto &&stream : conf.streams) {
        ss << toString(stream) << ", ";
    }
    ss << "], ";

    ss << "}";
    return ss.str();
}

std::string toString(const std::shared_ptr<MiImageBuffer> &buffer)
{
    if (buffer == nullptr) {
        return "{}";
    }

    std::stringstream ss;
    ss << "{ ";

    ss << "format: " << buffer->getImgFormat() << ", ";
    ss << "width: " << buffer->getImgWidth() << ", ";
    ss << "height: " << buffer->getImgHight() << ", ";
    ss << "stride: " << buffer->getBufStridesInBytes() << ", ";
    ss << "bufferSize: " << buffer->getBufSize() << ", ";

    ss << "}";
    return ss.str();
}

std::string toString(const StreamBuffer &buffer)
{
    std::stringstream ss;
    ss << "{ ";

    ss << "streamId: " << buffer.streamId << ", ";
    ss << "bufferId: " << buffer.bufferId << ", ";
    ss << "status: " << static_cast<int>(buffer.status) << ", ";
    ss << "acquireFenceFd: " << buffer.acquireFenceFd << ", ";
    ss << "releaseFenceFd: " << buffer.releaseFenceFd << ", ";
    ss << "bufferSettings.count: " << buffer.bufferSettings.count() << ", ";
    ss << "buffer(0x" << buffer.buffer.get() << "): " << toString(buffer.buffer) << ", ";
    ss << "}";

    return ss.str();
}

std::string toString(const NotifyMsg &msg)
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

std::string toString(const CaptureResult &result)
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
            ss << toString(buffer) << ", ";
        }
        ss << "], ";
    }

    ss << "outputBuffers(" << result.outputBuffers.size() << "): [";
    for (const auto &buffer : result.outputBuffers) {
        ss << toString(buffer) << ", ";
    }
    ss << "], ";

    ss << "}";

    return ss.str();
}

std::string toString(const SubRequest &subReq)
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
            ss << toString(buffer) << ", ";
        }
        ss << "], ";
    }
    ss << "}";

    return ss.str();
}

std::string toString(const CaptureRequest &request)
{
    std::stringstream ss;
    ss << "{ ";
    ss << "frameNumber: " << request.frameNumber << ", ";
    ss << "settings.count: " << request.settings.count() << ", ";
    ss << "halSettings.count: " << request.halSettings.count() << ", ";
    ss << "physicalCameraSettings.size: " << request.physicalCameraSettings.size() << ", ";

    if (!request.inputBuffers.empty()) {
        ss << "inputBuffers(" << request.inputBuffers.size() << "): [";
        for (const auto &buffer : request.inputBuffers) {
            ss << toString(buffer) << ", ";
        }
        ss << "], ";
    }

    if (!request.outputBuffers.empty()) {
        ss << "outputBuffers(" << request.outputBuffers.size() << "): [";
        for (const auto &buffer : request.outputBuffers) {
            ss << toString(buffer) << ", ";
        }
        ss << "], ";
    }

    if (!request.subRequests.empty()) {
        ss << "subRequests(" << request.subRequests.size() << "): [";
        for (auto &&subReq : request.subRequests) {
            ss << toString(subReq) << ", ";
        }
        ss << "], ";
    }
    ss << "}";

    return ss.str();
}

bool copyBuffer(std::shared_ptr<MiImageBuffer> src, std::shared_ptr<MiImageBuffer> dst,
                char const *callName)
{
    if (src == nullptr || dst == nullptr) {
        MLOGE(LogGroupCore, "src (%p) or dst (%p) is nullptr!", src.get(), dst.get());
        return false;
    }

    if (src->isEmpty()) {
        MLOGE(LogGroupCore, "src is empty!");
        return false;
    }
    if (dst->isEmpty()) {
        MLOGE(LogGroupCore, "dst is empty!");
        return false;
    }

    if (src->getImgFormat() != dst->getImgFormat() &&
        dst->getImgFormat() != EImageFormat::eImgFmt_BLOB) {
        MLOGE(LogGroupCore, "format not matched: src(format = %d), dst(format = %d)!",
              src->getImgFormat(), dst->getImgFormat());
        return false;
    }

    if (dst->getBufSize() < src->getBufSize()) {
        MLOGE(LogGroupCore, "dst bufferSize (%zu) is less than src bufferSize(%zu)!",
              dst->getBufSize(), src->getBufSize());
        return false;
    }

    bool ok = false;

    if (dst->lockBuf(callName, eBUFFER_USAGE_SW_WRITE_OFTEN | eBUFFER_USAGE_SW_READ_OFTEN)) {
        char *dstVa = (char *)(dst->getBufVA(0));
        if (dstVa != nullptr) {
            if (src->lockBuf(callName, eBUFFER_USAGE_SW_READ_OFTEN)) {
                char *srcVa = (char *)(src->getBufVA(0));
                if (srcVa != nullptr) {
                    memcpy(dstVa, srcVa, src->getBufSize());
                } else {
                    MLOGE(LogGroupCore, "get src buffer virtual address failed!");
                    ok = false;
                }
                if (!(src->unlockBuf(callName))) {
                    MLOGE(LogGroupCore, "failed on src buffer unlockBuf");
                    ok = false;
                }
            } else {
                MLOGE(LogGroupCore, "failed on src buffer lockBuf");
            }
        } else {
            MLOGE(LogGroupCore, "get dst buffer virtual address failed!");
            ok = false;
        }
        if (!(dst->unlockBuf(callName))) {
            MLOGE(LogGroupCore, "failed on dst buffer unlockBuf");
            ok = false;
        }
    } else {
        MLOGE(LogGroupCore, "failed on dst buffer lockBuf");
    }

    return ok;
}

bool dumpBuffer(const std::shared_ptr<MiImageBuffer> &buffer, const std::string &name)
{
    auto format = buffer->getImgFormat();
    std::string ext;
    if (format == eImgFmt_RAW16 || format == eImgFmt_RAW10 || format == eImgFmt_BLOB ||
        (eImgFmt_RAW_START <= format && format < eImgFmt_BLOB_START)) {
        ext = ".raw";
    } else {
        ext = ".yuv";
    }

    std::string path("/data/vendor/camera/");
    path += name + "_" + "format_" + std::to_string(format) + "_" +
            std::to_string(buffer->getImgWidth()) + "x" + std::to_string(buffer->getImgHight()) +
            ext;

    std::shared_ptr<mcam::IImageBuffer> rawBuffer;
    if (format == eImgFmt_BLOB) {
        auto bufferSize = buffer->getBufferHeap()->getBufSizeInBytes(0);
        rawBuffer = buffer->getBufferHeap()->createImageBuffer_FromBlobHeap(0, bufferSize);
    } else {
        rawBuffer = buffer->getBufferHeap()->createImageBuffer();
    }
    MLOGD(LogGroupCore, "dump buffer to %s", path.c_str());
    rawBuffer->lockBuf("dumpBuffer");
    bool ok = rawBuffer->saveToFile(path.c_str());
    rawBuffer->unlockBuf("dumpBuffer");

    return ok;
}

std::string toString(const CameraOfflineSessionInfo &offlineSessionInfo)
{
    std::stringstream ss;
    ss << "{ ";
    ss << "offlineStreams: ";
    ss << "[ ";
    for (auto &&stream : offlineSessionInfo.offlineStreams) {
        ss << "{ "
           << "id: " << stream.id << ", numOutstandingBuffers: " << stream.numOutstandingBuffers
           << ", ";

        ss << "circulatingBufferIds: " << toString(stream.circulatingBufferIds);

        ss << "}, ";
    }
    ss << "], ";

    ss << "offlineRequests: [ ";
    for (auto &&req : offlineSessionInfo.offlineRequests) {
        ss << "{ ";
        ss << "frameNumber: " << req.frameNumber << ", ";
        ss << "pendingStreams: " << toString(req.pendingStreams);
        ss << "}, ";
    }
    ss << "], ";

    ss << "}, ";

    return ss.str();
}

} // namespace mizone::utils
