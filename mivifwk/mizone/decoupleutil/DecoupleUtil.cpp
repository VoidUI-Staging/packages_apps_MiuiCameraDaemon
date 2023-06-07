
#include "DecoupleUtil.h"

#include <mtkcam-android/utils/imgbuf/IGraphicImageBufferHeap.h>

//这个是存在一定依赖的,要制定这个头文件的位置B
#include <VendorMetadataParser.h>
#include <mtkcam-halif/utils/metadata_tag/1.x/mtk_metadata_tag.h>
#include <mtkcam-interfaces/utils/debug/Properties.h>
#include <mtkcam-interfaces/utils/std/Log.h>
#include <mtkcam-interfaces/utils/std/ULog.h>
#include <mtkcam-interfaces/utils/std/ULogDef.h>
#include <string.h>

#include <memory>
#include <string>

#include "MiMetadata.h"
#include "mtkcam-halif/utils/metadata/1.x/IMetadata.h"

using std::vector;

namespace mizone {

static ::android::sp<NSCam::IMetadataConverter> sMetadataConverter =
    NSCam::IMetadataConverter::createInstance(
        NSCam::IDefaultMetadataTagSet::singleton()->getTagSet());

DecoupleUtil::DecoupleUtil() {}

//从mtk的类型转换到mizone的类型
auto DecoupleUtil::convertStreamConfiguration(const mcam::StreamConfiguration &mtkConfiguration,
                                              StreamConfiguration &configuration) -> int
{
    // int res, status = 0;
    //设置configuretion的Streams的size大小
    size_t configurationSize = mtkConfiguration.streams.size();
    configuration.streams.resize(configurationSize);

    configuration.operationMode =
        static_cast<StreamConfigurationMode>(mtkConfiguration.operationMode);

    //通过make_shared metadata先直接赋值
    configuration.sessionParams.mIMetadata =
        std::make_shared<NSCam::IMetadata>(mtkConfiguration.sessionParams);

    configuration.streamConfigCounter = mtkConfiguration.streamConfigCounter;

    for (size_t i = 0; i < configurationSize; i++) {
        auto &rStream = configuration.streams[i];
        auto &rMtkStream = mtkConfiguration.streams[i];
        // convert Stream
        convertStream(rMtkStream, rStream);
    }

    return 0;
}

//从mizone的类型转换到mtk的类型
auto DecoupleUtil::convertStreamConfiguration(const StreamConfiguration &configuration,
                                              mcam::StreamConfiguration &mtkConfiguration) -> int
{
    // int res, status = 0;
    size_t configurationSize = configuration.streams.size();
    mtkConfiguration.streams.resize(configurationSize);

    mtkConfiguration.operationMode =
        static_cast<mcam::StreamConfigurationMode>(configuration.operationMode);

    //当前metadata先直接赋值
    mtkConfiguration.sessionParams = *(configuration.sessionParams.mIMetadata);

    mtkConfiguration.streamConfigCounter = configuration.streamConfigCounter;

    for (size_t i = 0; i < configurationSize; i++) {
        auto &rStream = configuration.streams[i];
        auto &rMtkStream = mtkConfiguration.streams[i];
        // convert Stream
        convertStream(rStream, rMtkStream);
    }
    return 0;
}

auto DecoupleUtil::convertHalStreamConfiguration(
    const mcam::HalStreamConfiguration &mtkHalConfiguration,
    HalStreamConfiguration &HalConfiguration) -> int
{
    HalConfiguration.sessionResults.mIMetadata =
        std::make_shared<NSCam::IMetadata>(mtkHalConfiguration.sessionResults);
    HalConfiguration.streams.resize(mtkHalConfiguration.streams.size());
    for (size_t i = 0; i < mtkHalConfiguration.streams.size(); i++) {
        auto &rHalStream = HalConfiguration.streams[i];
        auto &rMtkHalStream = mtkHalConfiguration.streams[i];
        convertHalStream(rMtkHalStream, rHalStream);
    }
    return 0;
}

auto DecoupleUtil::convertHalStreamConfiguration(const HalStreamConfiguration &HalConfiguration,
                                                 mcam::HalStreamConfiguration &mtkHalConfiguration)
    -> int
{
    mtkHalConfiguration.sessionResults = *(HalConfiguration.sessionResults.mIMetadata);
    mtkHalConfiguration.streams.resize(HalConfiguration.streams.size());
    for (size_t i = 0; i < HalConfiguration.streams.size(); i++) {
        auto &rHalStream = HalConfiguration.streams[i];
        auto &rMtkHalStream = mtkHalConfiguration.streams[i];
        convertHalStream(rHalStream, rMtkHalStream);
    }
    return 0;
}

auto DecoupleUtil::convertCaptureRequests(const std::vector<mcam::CaptureRequest> &mtkRequests,
                                          std::vector<CaptureRequest> &requests) -> int
{
    requests.resize(mtkRequests.size());
    for (size_t i = 0; i < mtkRequests.size(); i++) {
        convertOneRequest(mtkRequests[i], requests[i]);
    }
    return 0;
}

auto DecoupleUtil::convertOneRequest(const mcam::CaptureRequest &mtkRequest,
                                     CaptureRequest &request) -> int
{
    request.frameNumber = mtkRequest.frameNumber;
    request.settings.mIMetadata = std::make_shared<IMetadata>(mtkRequest.settings);
    request.halSettings.mIMetadata = std::make_shared<IMetadata>(mtkRequest.halSettings);
    convertStreamBuffers(mtkRequest.inputBuffers, request.inputBuffers);
    convertStreamBuffers(mtkRequest.outputBuffers, request.outputBuffers);

    convertPhysicalCameraSettings(mtkRequest.physicalCameraSettings,
                                  request.physicalCameraSettings);

    convetSubRequests(mtkRequest.subRequests, request.subRequests);

    return 0;
}

auto DecoupleUtil::convetSubRequests(const std::vector<mcam::SubRequest> &src,
                                     std::vector<SubRequest> &dst) -> int
{
    int subSize = src.size();
    dst.resize(subSize);
    for (size_t i = 0; i < subSize; i++) {
        convetOneSubRequest(src[i], dst[i]);
    }

    return 0;
}

auto DecoupleUtil::convetOneSubRequest(const mcam::SubRequest &src, SubRequest &dst) -> int
{
    dst.subFrameIndex = src.subFrameIndex;
    convertStreamBuffers(src.inputBuffers, dst.inputBuffers);
    dst.settings.mIMetadata = std::make_shared<IMetadata>(src.settings);
    dst.halSettings.mIMetadata = std::make_shared<IMetadata>(src.halSettings);
    convertPhysicalCameraSettings(src.physicalCameraSettings, dst.physicalCameraSettings);
    return 0;
}

auto DecoupleUtil::convertStreamBuffers(const std::vector<mcam::StreamBuffer> &src,
                                        std::vector<StreamBuffer> &dst) -> int
{
    int bufSize = src.size();
    dst.resize(bufSize);
    for (size_t i = 0; i < bufSize; i++) {
        convertOneStreamBuffer(src[i], dst[i]);
    }
    return 0;
}

auto DecoupleUtil::convertOneStreamBuffer(const mcam::StreamBuffer &src, StreamBuffer &dst) -> int
{
    dst.streamId = src.streamId;
    dst.bufferId = src.bufferId;
    if (src.buffer == nullptr) {
        dst.buffer = nullptr;
    } else {
        dst.buffer = std::make_shared<MiImageBuffer>(src.buffer);
    }

    dst.status = static_cast<BufferStatus>(src.status);
    dst.acquireFenceFd = src.acquireFenceFd;
    dst.releaseFenceFd = src.releaseFenceFd;
    dst.bufferSettings.mIMetadata = std::make_shared<IMetadata>(src.bufferSettings);
    return 0;
}
auto DecoupleUtil::convertPhysicalCameraSettings(
    const std::vector<mcam::PhysicalCameraSetting> &src, std::vector<PhysicalCameraSetting> &dst)
    -> int
{
    dst.resize(src.size());
    for (size_t i = 0; i < src.size(); i++) {
        auto &srcSetting = src[i];
        auto &dstSetting = dst[i];
        // physicalCameraId
        dstSetting.physicalCameraId = srcSetting.physicalCameraId;
        dstSetting.settings.mIMetadata = std::make_shared<IMetadata>(srcSetting.settings);
        dstSetting.halSettings.mIMetadata = std::make_shared<IMetadata>(srcSetting.halSettings);
    }
    return 0;
}

//从mizone的类型转换到mtk的类型
auto DecoupleUtil::convertCaptureRequests(const std::vector<CaptureRequest> &requests,
                                          std::vector<mcam::CaptureRequest> &mtkRequests) -> int
{
    mtkRequests.resize(requests.size());
    for (size_t i = 0; i < requests.size(); i++) {
        convertOneRequest(requests[i], mtkRequests[i]);
    }
    return 0;
}
auto DecoupleUtil::convertOneRequest(const CaptureRequest &request,
                                     mcam::CaptureRequest &mtkRequest) -> int
{
    mtkRequest.frameNumber = request.frameNumber;
    mtkRequest.settings = *(request.settings.mIMetadata);
    mtkRequest.halSettings = *(request.halSettings.mIMetadata);
    convertStreamBuffers(request.inputBuffers, mtkRequest.inputBuffers);
    convertStreamBuffers(request.outputBuffers, mtkRequest.outputBuffers);

    convertPhysicalCameraSettings(request.physicalCameraSettings,
                                  mtkRequest.physicalCameraSettings);

    convetSubRequests(request.subRequests, mtkRequest.subRequests);
    return 0;
}

auto DecoupleUtil::convetSubRequests(const std::vector<SubRequest> &src,
                                     std::vector<mcam::SubRequest> &dst) -> int
{
    int subSize = src.size();
    dst.resize(subSize);
    for (size_t i = 0; i < subSize; i++) {
        convetOneSubRequest(src[i], dst[i]);
    }

    return 0;
}

auto DecoupleUtil::convetOneSubRequest(const SubRequest &src, mcam::SubRequest &dst) -> int
{
    dst.subFrameIndex = src.subFrameIndex;
    convertStreamBuffers(src.inputBuffers, dst.inputBuffers);
    dst.settings = *(src.settings.mIMetadata);
    dst.halSettings = *(src.halSettings.mIMetadata);
    convertPhysicalCameraSettings(src.physicalCameraSettings, dst.physicalCameraSettings);
    return 0;
}

auto DecoupleUtil::convertStreamBuffers(const std::vector<StreamBuffer> &src,
                                        std::vector<mcam::StreamBuffer> &dst) -> int
{
    int bufSize = src.size();
    dst.resize(bufSize);
    for (size_t i = 0; i < bufSize; i++) {
        convertOneStreamBuffer(src[i], dst[i]);
    }
    return 0;
}

auto DecoupleUtil::convertOneStreamBuffer(const StreamBuffer &src, mcam::StreamBuffer &dst) -> int
{
    dst.streamId = src.streamId;
    dst.bufferId = src.bufferId;
    if (src.buffer != nullptr) {
        dst.buffer = src.buffer->mImageBufferHeap;
    } else {
        dst.buffer = nullptr;
    }

    dst.status = static_cast<mcam::BufferStatus>(src.status);
    dst.acquireFenceFd = src.acquireFenceFd;
    dst.releaseFenceFd = src.releaseFenceFd;
    dst.bufferSettings = *(src.bufferSettings.mIMetadata);
    return 0;
}

auto DecoupleUtil::convertPhysicalCameraSettings(const std::vector<PhysicalCameraSetting> &src,
                                                 std::vector<mcam::PhysicalCameraSetting> &dst)
    -> int
{
    dst.resize(src.size());
    for (size_t i = 0; i < src.size(); i++) {
        auto &srcSetting = src[i];
        auto &dstSetting = dst[i];
        // physicalCameraId
        dstSetting.physicalCameraId = srcSetting.physicalCameraId;
        dstSetting.settings = *(srcSetting.settings.mIMetadata);
        dstSetting.halSettings = *(srcSetting.halSettings.mIMetadata);
    }
    return 0;
}

/*
auto convertStreamRotation(const uint32_t& srcTransform,StreamRotation&
dstStreamRotation) -> int {
  // Note: StreamRotation is applied counterclockwise,
  //       and Transform is applied clockwise.
  switch (srcTransform) {
    case NSCam::eTransform_None:
      dstStreamRotation = StreamRotation::ROTATION_0;
      break;
    case NSCam::eTransform_ROT_270:
      dstStreamRotation = StreamRotation::ROTATION_90;
      break;
    case NSCam::eTransform_ROT_180:
      dstStreamRotation = StreamRotation::ROTATION_180;
      break;
    case NSCam::eTransform_ROT_90:
      dstStreamRotation = StreamRotation::ROTATION_270;
      break;
    default:
      //MY_LOGE("Unknown stream rotation: %u", srcStreamRotation);
      return -EINVAL;
  }

  return 0;
}
*/

int DecoupleUtil::convertBufferPlanes(const NSCam::BufPlanes &MtkBufPlanes, BufPlanes &bufPlanes)
{
    bufPlanes.count = MtkBufPlanes.count;
    for (int i = 0; i < 3; i++) {
        bufPlanes.planes[i].rowStrideInBytes = MtkBufPlanes.planes[i].rowStrideInBytes;
        bufPlanes.planes[i].sizeInBytes = MtkBufPlanes.planes[i].sizeInBytes;
    }
    return 0;
}

int DecoupleUtil::convertBufferPlanes(const BufPlanes &bufPlanes, NSCam::BufPlanes &MtkBufPlanes)
{
    MtkBufPlanes.count = bufPlanes.count;
    for (int i = 0; i < 3; i++) {
        MtkBufPlanes.planes[i].rowStrideInBytes = bufPlanes.planes[i].rowStrideInBytes;
        MtkBufPlanes.planes[i].sizeInBytes = bufPlanes.planes[i].sizeInBytes;
    }
    return 0;
}

auto DecoupleUtil::convertStream(const mcam::Stream &MtkStream, Stream &stream) -> int
{
    stream.id = MtkStream.id;
    stream.streamType = static_cast<StreamType>(MtkStream.streamType);
    stream.width = MtkStream.width;
    stream.height = MtkStream.height;
    stream.physicalCameraId = MtkStream.physicalCameraId;
    stream.bufferSize = MtkStream.bufferSize;
    stream.usage = MtkStream.usage;

    //下面的变量都是mtk对android进行了一定的扩展，所以用mtk定义的变量了。
    stream.transform = MtkStream.transform;
    stream.dataSpace = static_cast<Dataspace>(MtkStream.dataSpace);
    stream.format = static_cast<EImageFormat>(MtkStream.format);
    convertBufferPlanes(MtkStream.bufPlanes, stream.bufPlanes);
    stream.settings.mIMetadata = std::make_shared<NSCam::IMetadata>(MtkStream.settings);

    return 0;
}

auto DecoupleUtil::convertStream(const Stream &stream, mcam::Stream &MtkStream) -> int
{
    MtkStream.id = stream.id;
    MtkStream.streamType = static_cast<mcam::StreamType>(stream.streamType);
    MtkStream.width = stream.width;
    MtkStream.height = stream.height;
    MtkStream.physicalCameraId = stream.physicalCameraId;
    MtkStream.bufferSize = stream.bufferSize;
    MtkStream.usage = stream.usage;

    //下面的变量都是mtk对android进行了一定的扩展，所以用mtk定义的变量了。
    MtkStream.transform = stream.transform;
    MtkStream.dataSpace = static_cast<mcam::Dataspace>(stream.dataSpace);
    MtkStream.format = static_cast<NSCam::EImageFormat>(stream.format);
    convertBufferPlanes(stream.bufPlanes, MtkStream.bufPlanes);
    MtkStream.settings = *(stream.settings.mIMetadata);

    return 0;
}

// void DecoupleUtil::sampleConfigure(
//        const mcam::StreamConfiguration& mtkConfiguration){
//  StreamConfiguration configuration;
//  int status = convertRequestedConfiguration(mtkConfiguration, configuration);
//  if (status != 0) {
//    // MY_LOGE("failed to parseStreamConfiguration");
//    // return -EINVAL;
//  }
//}

auto DecoupleUtil::convertHalStream(const mcam::HalStream &mtkStream, HalStream &stream) -> int
{
    stream.id = mtkStream.id;
    stream.overrideFormat = static_cast<EImageFormat>(mtkStream.overrideFormat);
    stream.producerUsage = mtkStream.producerUsage;
    stream.consumerUsage = mtkStream.consumerUsage;
    stream.maxBuffers = mtkStream.maxBuffers;
    stream.overrideDataSpace = static_cast<Dataspace>(mtkStream.overrideDataSpace);
    stream.physicalCameraId = mtkStream.physicalCameraId;
    stream.supportOffline = mtkStream.supportOffline;
    stream.results.mIMetadata = std::make_shared<NSCam::IMetadata>(mtkStream.results);
    return 0;
}

auto DecoupleUtil::convertHalStream(const HalStream &stream, mcam::HalStream &mtkStream) -> int
{
    mtkStream.id = stream.id;
    mtkStream.overrideFormat = static_cast<NSCam::EImageFormat>(stream.overrideFormat);
    mtkStream.producerUsage = stream.producerUsage;
    mtkStream.consumerUsage = stream.consumerUsage;
    mtkStream.maxBuffers = stream.maxBuffers;
    mtkStream.overrideDataSpace = static_cast<mcam::Dataspace>(stream.overrideDataSpace);
    mtkStream.physicalCameraId = stream.physicalCameraId;
    mtkStream.supportOffline = stream.supportOffline;
    mtkStream.results = *(stream.results.mIMetadata);
    return 0;
}

auto DecoupleUtil::convertCaptureResults(const std::vector<mcam::CaptureResult> &src,
                                         std::vector<CaptureResult> &dst) -> int
{
    size_t size = src.size();
    dst.resize(size);
    for (size_t i = 0; i < size; i++) {
        convertCaptureResult(src[i], dst[i]);
    }
    return 0;
}

auto DecoupleUtil::convertCaptureResults(const std::vector<CaptureResult> &src,
                                         std::vector<mcam::CaptureResult> &dst) -> int
{
    size_t size = src.size();
    dst.resize(size);
    for (size_t i = 0; i < size; i++) {
        convertCaptureResult(src[i], dst[i]);
    }
    return 0;
}

auto DecoupleUtil::convertCaptureResult(const mcam::CaptureResult &src, CaptureResult &dst) -> int
{
    dst.frameNumber = src.frameNumber;
    dst.bLastPartialResult = src.bLastPartialResult;
    dst.result.mIMetadata = std::make_shared<NSCam::IMetadata>(src.result);
    dst.halResult.mIMetadata = std::make_shared<NSCam::IMetadata>(src.halResult);
    convertStreamBuffers(src.inputBuffers, dst.inputBuffers);
    convertStreamBuffers(src.outputBuffers, dst.outputBuffers);
    // convert PhysicalCameraMetadata
    size_t metaSize = src.physicalCameraMetadata.size();
    dst.physicalCameraMetadata.resize(metaSize);
    for (size_t i = 0; i < metaSize; i++) {
        dst.physicalCameraMetadata[i].physicalCameraId =
            src.physicalCameraMetadata[i].physicalCameraId;
        dst.physicalCameraMetadata[i].metadata.mIMetadata =
            std::make_shared<IMetadata>(src.physicalCameraMetadata[i].metadata);
        dst.physicalCameraMetadata[i].halMetadata.mIMetadata =
            std::make_shared<IMetadata>(src.physicalCameraMetadata[i].halMetadata);
    }
    return 0;
}

auto DecoupleUtil::convertCaptureResult(const CaptureResult &src, mcam::CaptureResult &dst) -> int
{
    dst.frameNumber = src.frameNumber;
    dst.bLastPartialResult = src.bLastPartialResult;
    dst.result = *(src.result.mIMetadata);
    dst.halResult = *(src.halResult.mIMetadata);
    convertStreamBuffers(src.inputBuffers, dst.inputBuffers);
    convertStreamBuffers(src.outputBuffers, dst.outputBuffers);
    // convert PhysicalCameraMetadata
    size_t metaSize = src.physicalCameraMetadata.size();
    dst.physicalCameraMetadata.resize(metaSize);
    for (size_t i = 0; i < metaSize; i++) {
        dst.physicalCameraMetadata[i].physicalCameraId =
            src.physicalCameraMetadata[i].physicalCameraId;
        dst.physicalCameraMetadata[i].metadata =
            *(src.physicalCameraMetadata[i].metadata.mIMetadata);
        dst.physicalCameraMetadata[i].halMetadata =
            *(src.physicalCameraMetadata[i].halMetadata.mIMetadata);
    }
    return 0;
}

// Notify相关

auto DecoupleUtil::convertNotifys(const std::vector<mcam::NotifyMsg> &src,
                                  std::vector<NotifyMsg> &dst) -> int
{
    for (size_t i = 0; i < src.size(); i++) {
        dst.push_back(convertOneNotify(src[i]));
    }
    return 0;
}
auto DecoupleUtil::convertNotifys(const std::vector<NotifyMsg> &src,
                                  std::vector<mcam::NotifyMsg> &dst) -> int
{
    for (size_t i = 0; i < src.size(); i++) {
        dst.push_back(convertOneNotify(src[i]));
    }
    return 0;
}

auto DecoupleUtil::convertOneNotify(const mcam::NotifyMsg &src) -> NotifyMsg
{
    switch (src.type) {
    case mcam::MsgType::ERROR: {
        NotifyMsg dst{.type = MsgType::ERROR, .msg = {.error = ErrorMsg()}};
        dst.msg.error.frameNumber = src.msg.error.frameNumber;
        dst.msg.error.errorCode = static_cast<ErrorCode>(src.msg.error.errorCode);
        dst.msg.error.errorStreamId = src.msg.error.errorStreamId;
        return dst;
    }
    case mcam::MsgType::SHUTTER: {
        NotifyMsg dst{.type = MsgType::SHUTTER, .msg = {.shutter = ShutterMsg()}};
        dst.msg.shutter.frameNumber = src.msg.shutter.frameNumber;
        dst.msg.shutter.timestamp = src.msg.shutter.timestamp;
        return dst;
    }
    default:
        break;
    }
    NotifyMsg dst{.type = MsgType::UNKNOWN, .msg = {.error = ErrorMsg()}};
    return dst;
}
auto DecoupleUtil::convertOneNotify(const NotifyMsg &src) -> mcam::NotifyMsg
{
    switch (src.type) {
    case MsgType::ERROR: {
        mcam::NotifyMsg dst{.type = mcam::MsgType::ERROR, .msg = {.error = mcam::ErrorMsg()}};
        dst.msg.error.frameNumber = src.msg.error.frameNumber;
        dst.msg.error.errorCode = static_cast<mcam::ErrorCode>(src.msg.error.errorCode);
        dst.msg.error.errorStreamId = src.msg.error.errorStreamId;
        return dst;
    }
    case MsgType::SHUTTER: {
        mcam::NotifyMsg dst{.type = mcam::MsgType::SHUTTER, .msg = {.shutter = mcam::ShutterMsg()}};
        dst.msg.shutter.frameNumber = src.msg.shutter.frameNumber;
        dst.msg.shutter.timestamp = src.msg.shutter.timestamp;
        return dst;
    }
    default:
        break;
    }
    mcam::NotifyMsg dst{.type = mcam::MsgType::UNKNOWN, .msg = {.error = mcam::ErrorMsg()}};
    return dst;
}

// mialegoengine相关
auto DecoupleUtil::convertRequest2Image(CaptureRequest &request, ImageParams *input,
                                        ImageParams *output) -> int
{
    if (request.inputBuffers.size() != 0) {
        input = new ImageParams;
        if (input == nullptr) {
            return -1;
        }
        auto &rInputBuffer = request.inputBuffers[0];
        auto bufferHandle = rInputBuffer.buffer->mImageBufferHeap;
        auto mSize = bufferHandle->getImgSize();
        input->format.width = mSize.w;
        input->format.height = mSize.h;
        input->format.format = bufferHandle->getImgFormat();
    }
    if (request.outputBuffers.size() != 0) {
        output = new ImageParams;
        if (output == nullptr) {
            return -1;
        }
    }

    return 0;
}

auto DecoupleUtil::convertMetadata(const camera_metadata_t *src, MiMetadata &dst) -> int
{
    MLOGD(LogGroupHAL, "+");
    sMetadataConverter->convert(src, *(dst.mIMetadata.get()));
    MLOGD(LogGroupHAL, "-");

    return 0;
}

auto DecoupleUtil::convertMetadata(const MiMetadata &src, camera_metadata_t *&dst) -> int
{
    MLOGD(LogGroupHAL, "+");
    size_t size = 0;
    IMetadata &mtkMetadata = *(src.mIMetadata.get());
    sMetadataConverter->convert(mtkMetadata, dst, &size);
    MLOGD(LogGroupHAL, "-");
    return 0;
}

auto DecoupleUtil::convertMetadata(const IMetadata &src, camera_metadata_t *&dst) -> int
{
    MLOGD(LogGroupHAL, "+");
    size_t size = 0;
    sMetadataConverter->convert(src, dst, &size);
    MLOGD(LogGroupHAL, "-");
    return 0;
}
auto DecoupleUtil::convertMetadata(const camera_metadata_t *src, IMetadata &dst) -> int
{
    MLOGD(LogGroupHAL, "+");
    sMetadataConverter->convert(src, dst);
    MLOGD(LogGroupHAL, "-");
    return 0;
}

auto DecoupleUtil::convertFormat(const MiaPixelFormat &src, EImageFormat &dst) -> int
{
    switch (src) {
    case CAM_FORMAT_YUV_420_NV21:
        dst = eImgFmt_NV21;
        break;
    case CAM_FORMAT_YUV_420_NV12:
        dst = eImgFmt_NV12;
        break;
    case CAM_FORMAT_RAW10:
        dst = eImgFmt_BAYER10;
        break;
    case CAM_FORMAT_RAW16:
        dst = eImgFmt_RAW16;
        break;
    case CAM_FORMAT_P010:
        dst = eImgFmt_MTK_YUV_P010;
        break;
    case CAM_FORMAT_JPEG:
        dst = eImgFmt_JPEG;
        break;
    case CAM_FORMAT_YV12:
        dst = eImgFmt_ISP_HIDL_TUNING;
        break;
    default:
        dst = static_cast<EImageFormat>(src);
        MLOGE(LogGroupCore, "convert format failed");
        break;
    }
    return 0;
}

auto DecoupleUtil::convertFormat(const EImageFormat &src, MiaPixelFormat &dst) -> int
{
    switch (src) {
    case eImgFmt_NV21:
        dst = CAM_FORMAT_YUV_420_NV21;
        break;
    case eImgFmt_NV12:
        dst = CAM_FORMAT_YUV_420_NV12;
        break;
    case eImgFmt_BAYER10:
        dst = CAM_FORMAT_RAW10;
        break;
    case eImgFmt_RAW16:
    case eImgFmt_BAYER16_UNPAK:
        dst = CAM_FORMAT_RAW16;
        break;
    case eImgFmt_MTK_YUV_P010:
        dst = CAM_FORMAT_P010;
        break;
    case eImgFmt_JPEG:
        dst = CAM_FORMAT_JPEG;
        break;
    default:
        dst = static_cast<MiaPixelFormat>(src);
        MLOGE(LogGroupCore, "convert format failed");
        break;
    }
    return 0;
}

uint32_t DecoupleUtil::getAlgoFormat(const uint32_t format)
{
    EImageFormat src = static_cast<EImageFormat>(format);
    MiaPixelFormat dst;
    convertFormat(src, dst);
    return static_cast<uint32_t>(dst);
}

uint32_t DecoupleUtil::getMtkFormat(const uint32_t format)
{
    MiaPixelFormat src = static_cast<MiaPixelFormat>(format);
    EImageFormat dst;
    convertFormat(src, dst);
    return static_cast<uint32_t>(dst);
}

auto DecoupleUtil::convert2MiaImage(const StreamBuffer &src, MiaFrame &dst,
                                    const Stream *pSrcStream) -> int
{
    auto &bufferHandle = src.buffer->mImageBufferHeap;
    if (pSrcStream) {
        dst.format.width = pSrcStream->width;
        dst.format.height = pSrcStream->height;
        dst.format.format = getAlgoFormat(pSrcStream->format);
        dst.format.planeStride = pSrcStream->bufPlanes.planes[0].rowStrideInBytes;
        dst.pBuffer.bufferSize = pSrcStream->bufferSize;
    } else {
        auto mSize = bufferHandle->getImgSize();
        dst.format.width = mSize.w;
        dst.format.height = mSize.h;
        dst.format.format = getAlgoFormat(bufferHandle->getImgFormat());
        dst.format.planeStride = src.buffer->getBufStridesInBytes();
    }
    MLOGD(LogGroupHAL, "buffer stride: %u, width: %d, height: %d", dst.format.planeStride,
          dst.format.width, dst.format.height);

    // buffer
    if (bufferHandle.get() != nullptr &&
        IGraphicImageBufferHeap::castFrom(bufferHandle.get()) != nullptr &&
        IGraphicImageBufferHeap::castFrom(bufferHandle.get())->getBufferHandlePtr() != nullptr) {
        dst.pBuffer.phHandle =
            *(IGraphicImageBufferHeap::castFrom(bufferHandle.get())->getBufferHandlePtr());
        dst.pBuffer.planes = bufferHandle->getPlaneCount();
        dst.pBuffer.bufferType = GRALLOC_TYPE;

    } else {
        dst.pBuffer.bufferType = ION_TYPE;
        dst.pBuffer.phHandle = nullptr;
    }
    // MY_LOGD("null srcBuffer");
    bufferHandle->lockBuf("convert2MiaImage", eBUFFER_USAGE_HW_CAMERA_READWRITE |
                                                  eBUFFER_USAGE_SW_READ_OFTEN |
                                                  eBUFFER_USAGE_SW_WRITE_OFTEN);
    dst.pBuffer.fd[0] = bufferHandle->getHeapID();
    //这里给pAddr[0]赋值，只是为了在mialgoengine中跳过MiaImage中的一段代码。
    dst.pBuffer.pAddr[0] = (uint8_t *)bufferHandle->getBufVA(0);
    bufferHandle->unlockBuf("convert2MiaImage");
    dst.pBuffer.pAddr[1] = nullptr;

    return 0;
}

auto DecoupleUtil::convertMetadata(const IMetadata &src, MiMetadata &dst) -> int
{
    MLOGD(LogGroupHAL, "+");
    dst.mIMetadata = std::make_shared<NSCam::IMetadata>(src);
    MLOGD(LogGroupHAL, "-");
    return 0;
}

auto DecoupleUtil::convertMetadata(const MiMetadata &src, IMetadata &dst) -> int
{
    MLOGD(LogGroupHAL, "+");
    dst = *(src.mIMetadata);
    MLOGD(LogGroupHAL, "-");
    return 0;
}

int DecoupleUtil::convertCameraOfflineSessionInfo(const CameraOfflineSessionInfo &src,
                                                  mcam::CameraOfflineSessionInfo &dst)
{
    dst.offlineStreams.clear();
    dst.offlineStreams.resize(src.offlineStreams.size());
    for (size_t i = 0; i < src.offlineStreams.size(); ++i) {
        auto &miStream = src.offlineStreams[i];
        auto &mtkStream = dst.offlineStreams[i];

        mtkStream.id = miStream.id;
        mtkStream.circulatingBufferIds = miStream.circulatingBufferIds;
        mtkStream.numOutstandingBuffers = miStream.numOutstandingBuffers;
    }

    dst.offlineRequests.clear();
    dst.offlineRequests.resize(src.offlineRequests.size());
    for (size_t i = 0; i < src.offlineRequests.size(); ++i) {
        auto &miRequest = src.offlineRequests[i];
        auto &mtkRequest = dst.offlineRequests[i];

        mtkRequest.frameNumber = miRequest.frameNumber;
        mtkRequest.pendingStreams = miRequest.pendingStreams;
    }

    return 0;
}

auto DecoupleUtil::buildStreamBuffer(const ImageParams &src, mcam::StreamBuffer &dst) -> int
{
    // NOTE: Could not check src.bufferType here, because mialgoengine set bufferType = GRALLOC_TYPE
    //       always in MiaImage's ctor when construct a source image buffer.
    //       We check pNativeHandle here as a work-around solution, however, the better way is
    //       to update mialgoengine to support ION buffer as source image buffer.
    if (src.pNativeHandle == nullptr) {
        buildIonStreamBuffer(src, dst);
    } else {
        buildGraphicStreamBuffer(src, dst);
    }
    return 0;
}

auto DecoupleUtil::buildIonStreamBuffer(const ImageParams &src, mcam::StreamBuffer &dst) -> int
{
    createIonBufferHeapByFd(
        src, mcam::eBUFFER_USAGE_SW_MASK | mcam::eBUFFER_USAGE_HW_CAMERA_READWRITE, dst.buffer);
    return 0;
}

auto DecoupleUtil::buildGraphicStreamBuffer(const ImageParams &src, mcam::StreamBuffer &dst) -> int
{
    buffer_handle_t handle = reinterpret_cast<buffer_handle_t>(src.pNativeHandle);
    EImageFormat format;
    convertFormat((MiaPixelFormat)src.format.format, format);
    dst.buffer = NSCam::IGraphicImageBufferHeap::create(
        __FUNCTION__, mcam::eBUFFER_USAGE_SW_MASK | mcam::eBUFFER_USAGE_HW_CAMERA_READWRITE,
        mcam::MSize(src.format.width, src.format.height), format, &handle);
    return 0;
}

auto DecoupleUtil::createIonBufferHeapByFd(const ImageParams &src, int usage, MiImageBuffer &dst)
    -> int
{
    std::shared_ptr<mcam::IImageBufferHeap> heap;
    auto status = createIonBufferHeapByFd(src, usage, heap);
    dst.mImageBufferHeap = heap;
    return status;
}

auto DecoupleUtil::createIonBufferHeapByFd(const ImageParams &src, int usage,
                                           std::shared_ptr<mcam::IImageBufferHeap> &dst) -> int
{
    EImageFormat format;
    convertFormat((MiaPixelFormat)src.format.format, format);

    mcam::IImageBufferAllocator::ImgParam param(format, MSize(src.format.width, src.format.height),
                                                usage);
    if (format == eImgFmt_RAW16) {
        // if stream format is eImgFmt_RAW16, it need to set blob buffer param
        param = mcam::IImageBufferAllocator::ImgParam::getBlobParam(src.bufferSize, usage);
    }

    for (uint32_t i = 0; i < src.numberOfPlanes; ++i) {
        param.strideInByte[i] = src.planeStride;
        // NOTE: ImageParams does not contain planeStride and sliceheight for each plane,
        //       there is no enough info to calcalte bufSizeInByte for each plane, so leave it as
        //       default below.
        // param.bufSizeInByte[i] = src.planeStride * src.sliceheight;
    }
    MLOGD(LogGroupHAL,
          "src format:%d, buffersize:%d, buffer stride: %d, width: %d, height: %d;"
          "dst format:%d, buffer stride: %d, width: %d, height: %d;",
          src.format.format, src.bufferSize, src.planeStride, src.format.width, src.format.height,
          param.format, param.strideInByte[0], param.size.w, param.size.h);

    dst = mcam::IImageBufferHeapFactory::createDummyFromBufferFD(
        std::to_string(src.fd[0]).c_str(), std::to_string(src.fd[0]).c_str(), src.fd[0], 0, param);
    if (dst == nullptr) {
        MLOGE(LogGroupHAL, "create buffer failed,the old fd is %d", src.fd[0]);
    }
    return 0;
}

auto DecoupleUtil::createGraphicBufferHeap(buffer_handle_t handle)
    -> std::shared_ptr<IImageBufferHeap>
{
    //  buffer_handle_t *handle;
    //  if (buffer.get() != nullptr &&
    //      NSCam::IGraphicImageBufferHeap::castFrom(buffer.get()) != nullptr &&
    //      NSCam::IGraphicImageBufferHeap::castFrom(buffer.get())
    //              ->getBufferHandlePtr() != nullptr) {
    //    handle = (NSCam::IGraphicImageBufferHeap::castFrom(buffer.get())
    //                  ->getBufferHandlePtr());
    //  }
    auto bufferHeap = NSCam::IGraphicImageBufferHeap::create(
        __FUNCTION__, mcam::eBUFFER_USAGE_SW_MASK | mcam::eBUFFER_USAGE_HW_CAMERA_READWRITE,
        &handle);
    return bufferHeap;
}

auto DecoupleUtil::dumpMetadata(const IMetadata &src) -> int
{
    sMetadataConverter->dumpAll(src);
    return 0;
}

auto DecoupleUtil::dumpMetadata(const MiMetadata &src) -> int
{
    IMetadata dst;
    convertMetadata(src, dst);
    sMetadataConverter->dumpAll(dst);
    return 0;
}

MiMetadata const DecoupleUtil::getStaticMeta(int32_t deviceId)
{
    auto pMetadataProvider = NSMetadataProviderManager::valueFor(deviceId);
    if (pMetadataProvider != NULL) {
        IMetadata const &staticMetadata = pMetadataProvider->getMtkStaticCharacteristics();
        MiMetadata const meta(staticMetadata);
        return meta;
    }
    return MiMetadata();
}

}; // namespace mizone
