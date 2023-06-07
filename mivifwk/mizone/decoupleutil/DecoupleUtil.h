#ifndef DECOUPLEUTIL_H_
#define DECOUPLEUTIL_H_

#include <MiDebugUtils.h>
#include <MiaPluginWraper.h>
#include <MiaPostProcType.h>
#include <mtkcam-android/utils/metaconv/IMetadataConverter.h>
#include <mtkcam-android/utils/metastore/IMetadataProvider.h>
#include <mtkcam-halif/device/1.x/types.h>

#include <cstdint>
#include <vector>

#include "MiImageBuffer.h"
#include "MiZoneTypes.h"
#include "mtkcam-halif/utils/metadata/1.x/IMetadata.h"

namespace mizone {

class DecoupleUtil
{
public:
    // test

    DecoupleUtil();

    static auto convertMetadata(const IMetadata &src, MiMetadata &dst) -> int;
    static auto convertMetadata(const MiMetadata &src, IMetadata &dst) -> int;
    static auto convertMetadata(const camera_metadata_t *src, MiMetadata &dst) -> int;
    static auto convertMetadata(const MiMetadata &src, camera_metadata_t *&dst) -> int;
    static auto convertMetadata(const IMetadata &src, camera_metadata_t *&dst) -> int;
    static auto convertMetadata(const camera_metadata_t *src, IMetadata &dst) -> int;

    // configureStream相关
    static auto convertStreamConfiguration(const mcam::StreamConfiguration &mtkConfiguration,
                                           StreamConfiguration &configuration) -> int;

    static auto convertStreamConfiguration(const StreamConfiguration &configuration,
                                           mcam::StreamConfiguration &mtkConfiguration) -> int;

    static auto convertHalStreamConfiguration(
        const mcam::HalStreamConfiguration &mtkHalConfiguration,
        HalStreamConfiguration &HalConfiguration) -> int;

    static auto convertHalStreamConfiguration(const HalStreamConfiguration &HalConfiguration,
                                              mcam::HalStreamConfiguration &mtkHalConfiguration)
        -> int;

    static auto convertStream(const mcam::Stream &mtkStream, Stream &stream) -> int;

    static auto convertStream(const Stream &stream, mcam::Stream &mtkStream) -> int;

    static auto convertHalStream(const mcam::HalStream &mtkStream, HalStream &stream) -> int;

    static auto convertHalStream(const HalStream &stream, mcam::HalStream &mtkStream) -> int;

    static int convertBufferPlanes(const NSCam::BufPlanes &MtkBufPlanes, BufPlanes &bufPlanes);

    static int convertBufferPlanes(const BufPlanes &bufPlanes, NSCam::BufPlanes &MtkBufPlanes);

    // processCaptureRequest相关
    static auto convertCaptureRequests(const std::vector<mcam::CaptureRequest> &mtkRequests,
                                       std::vector<CaptureRequest> &requests) -> int;

    static auto convertCaptureRequests(const std::vector<CaptureRequest> &requests,
                                       std::vector<mcam::CaptureRequest> &mtkRequests) -> int;

    static auto convertPhysicalCameraSettings(const std::vector<mcam::PhysicalCameraSetting> &src,
                                              std::vector<PhysicalCameraSetting> &dst) -> int;

    static auto convertPhysicalCameraSettings(const std::vector<PhysicalCameraSetting> &src,
                                              std::vector<mcam::PhysicalCameraSetting> &dst) -> int;

    static auto convertOneRequest(const mcam::CaptureRequest &mtkRequest, CaptureRequest &request)
        -> int;
    static auto convertOneRequest(const CaptureRequest &request, mcam::CaptureRequest &mtkRequest)
        -> int;
    // auto convertStreamRotation(const uint32_t &srcTransform,
    //                            StreamRotation &dstStreamRotation) -> int;

    static auto convertStreamBuffers(const std::vector<mcam::StreamBuffer> &src,
                                     std::vector<StreamBuffer> &dst) -> int;

    static auto convertStreamBuffers(const std::vector<StreamBuffer> &src,
                                     std::vector<mcam::StreamBuffer> &dst) -> int;

    static auto convertOneStreamBuffer(const mcam::StreamBuffer &src, StreamBuffer &dst) -> int;

    static auto convertOneStreamBuffer(const StreamBuffer &src, mcam::StreamBuffer &dst) -> int;

    static auto convetSubRequests(const std::vector<mcam::SubRequest> &src,
                                  std::vector<SubRequest> &dst) -> int;
    static auto convetSubRequests(const std::vector<SubRequest> &src,
                                  std::vector<mcam::SubRequest> &dst) -> int;

    static auto convetOneSubRequest(const mcam::SubRequest &src, SubRequest &dst) -> int;
    static auto convetOneSubRequest(const SubRequest &src, mcam::SubRequest &dst) -> int;
    // processCaptureResult相关
    static auto convertCaptureResults(const std::vector<mcam::CaptureResult> &src,
                                      std::vector<CaptureResult> &dst) -> int;

    static auto convertCaptureResults(const std::vector<CaptureResult> &src,
                                      std::vector<mcam::CaptureResult> &dst) -> int;

    static auto convertCaptureResult(const mcam::CaptureResult &src, CaptureResult &dst) -> int;
    static auto convertCaptureResult(const CaptureResult &src, mcam::CaptureResult &dst) -> int;

    static auto convertNotifys(const std::vector<mcam::NotifyMsg> &src, std::vector<NotifyMsg> &dst)
        -> int;
    static auto convertNotifys(const std::vector<NotifyMsg> &src, std::vector<mcam::NotifyMsg> &dst)
        -> int;

    static auto convertOneNotify(const mcam::NotifyMsg &src) -> NotifyMsg;
    static auto convertOneNotify(const NotifyMsg &src) -> mcam::NotifyMsg;
    // OfflineSession

    static int convertCameraOfflineSessionInfo(const CameraOfflineSessionInfo &src,
                                               mcam::CameraOfflineSessionInfo &dst);

    // mialogoengine相关

    static auto convertRequest2Image(CaptureRequest &request, ImageParams *input,
                                     ImageParams *output) -> int;

    static auto convert2ProcCaptureRequest(ProcessRequestInfo &src, CaptureRequest &dst) -> int;
    static auto convert2MiaImage(const StreamBuffer &buffer, MiaFrame &image,
                                 const Stream *pSrcStream = nullptr) -> int;

    static auto convertFormat(const MiaPixelFormat &src, EImageFormat &dst) -> int;
    static auto convertFormat(const EImageFormat &src, MiaPixelFormat &dst) -> int;
    static uint32_t getAlgoFormat(const uint32_t format);
    static uint32_t getMtkFormat(const uint32_t format);
    static auto createIonBufferHeapByFd(const ImageParams &src, int usage, MiImageBuffer &dst)
        -> int;
    static auto createGraphicBufferHeap(buffer_handle_t handle)
        -> std::shared_ptr<mcam::IImageBufferHeap>;

    static auto createIonBufferHeapByFd(const ImageParams &src, int usage,
                                        std::shared_ptr<mcam::IImageBufferHeap> &dst) -> int;
    static auto buildMtkInStream(const ProcessRequestInfo &src, mcam::Stream &dst) -> int;
    static auto buildMtkOutStream(const ProcessRequestInfo &src, mcam::Stream &dst) -> int;
    static auto buildIonStreamBuffer(const ImageParams &src, mcam::StreamBuffer &dst) -> int;
    static auto buildGraphicStreamBuffer(const ImageParams &src, mcam::StreamBuffer &dst) -> int;
    static auto buildStreamBuffer(const ImageParams &src, mcam::StreamBuffer &dst) -> int;

    // utils
    static auto dumpMetadata(const IMetadata &src) -> int;
    static auto dumpMetadata(const MiMetadata &src) -> int;

    // Use initialized metadata when camera launch
    static MiMetadata const getStaticMeta(int32_t deviceId);

public:
    //::android::sp<NSCam::IMetadataConverter> sMetadataConverter;
    IMetadata halSetting;
};

}; // namespace mizone

#endif
