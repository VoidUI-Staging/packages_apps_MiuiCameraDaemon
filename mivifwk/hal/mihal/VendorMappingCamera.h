#ifndef __VENDOR_MAPPING_CAMERA_H__
#define __VENDOR_MAPPING_CAMERA_H__

#include <map>
#include <memory>

#include "VendorCamera.h"

namespace mihal {

class VendorMappingCamera : public VendorCamera
{
public:
    VendorMappingCamera(int cameraId, const camera_info &info, CameraModule *module);
    ~VendorMappingCamera() = default;

    int getCameraInfo(camera_info *info) const override;
    int open() override;
    int close() override;

    int configureStreams(std::shared_ptr<StreamConfig> config) override;
    // int configureStreams(StreamConfig *config) override;

    const camera_metadata *defaultSettings(int) const override;
    int processCaptureRequest(std::shared_ptr<CaptureRequest> request) override;
    // int processCaptureRequest(CaptureRequest *) override;

    void dump(int fd) override;
    int flush() override;
    void signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams) override;

    // void processCaptureResult(const camera3_capture_result *result) override;
    void processCaptureResult(const CaptureResult *result) override;
    // void processCaptureResult(std::shared_ptr<const CaptureResult> result) override;

    void notify(const camera3_notify_msg *msg) override;
    camera3_buffer_request_status requestStreamBuffers(
        uint32_t num_buffer_reqs, const camera3_buffer_request *buffer_reqs,
        /*out*/ uint32_t *num_returned_buf_reqs,
        /*out*/ camera3_stream_buffer_ret *returned_buf_reqs) override;
    std::shared_ptr<StreamBuffer> requestStreamBuffers(uint32_t frame, uint32_t streamIdx) override;
    void returnStreamBuffers(uint32_t num_buffers,
                             const camera3_stream_buffer *const *buffers) override;

    int initialize2Vendor(const camera3_callback_ops *callbacks) override;
    int configureStreams2Vendor(StreamConfig *config) override;
    int submitRequest2Vendor(CaptureRequest *request) override;
    int flush2Vendor() override;
    void signalStreamFlush2Vendor(uint32_t numStream,
                                  const camera3_stream_t *const *streams) override;
    MiHal3BufferMgr *getFwkStreamHal3BufMgr() override;
    void setMapped(bool isMapped) { mIsMapped = isMapped; }

    typedef void (VendorMappingCamera::*onChangeMeta)(Metadata *metaModify, bool isConfig);

    void onChangeMetaSN(Metadata *metaModify, bool isConfig);
    void onChangeMetaBokeh(Metadata *metaModify, bool isConfig);
    void onChangeMetaBokehFront(Metadata *metaModify, bool isConfig);
    void onChangeMetaHdr(Metadata *metaModify, bool isConfig);
    void onChangeMetaBeauty(Metadata *metaModify, bool isConfig);

private:
    void tryExtensionMode(std::shared_ptr<StreamConfig> config);
    bool isExtensionMode();
    void condition_dump(const char *tag, StreamConfig *config);
    // void meta_dump(const char *tag, Metadata *meta);
    void switchCameraDevice(VendorMappingCamera *mapping);
    void processRequest(std::shared_ptr<CaptureRequest> request);

    VendorMappingCamera *mMapping;
    bool mIsMapped;
    bool mIsClosed;
    onChangeMeta mCurMetaCb;
};
}; // namespace mihal

#endif