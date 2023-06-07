
#include "CameraDevice.h"

#include "CameraManager.h"
#include "LogUtil.h"
#include "Session.h"

namespace mihal {

namespace {

// TODO: convert the raw hal3 structures to MiHAL data-abstracted classed for easy coding

int initialize_(const camera3_device *device, const camera3_callback_ops *callback_ops)
{
    CameraDevice *camera = reinterpret_cast<CameraDevice *>(device->priv);
    if (!camera)
        return -EINVAL;

    return camera->initialize(callback_ops);
}

int configureStreams_(const camera3_device *device, camera3_stream_configuration *stream_list)
{
    CameraDevice *camera = reinterpret_cast<CameraDevice *>(device->priv);
    if (!camera)
        return -EINVAL;

    auto config = std::make_shared<RemoteConfig>(camera, stream_list);

    int ret = camera->configureStreams(config);

    return ret;
}

const camera_metadata *defaultSettings_(const camera3_device *device, int type)
{
    CameraDevice *camera = reinterpret_cast<CameraDevice *>(device->priv);
    if (!camera)
        return nullptr;

    return camera->defaultSettings(type);
}

int processCaptureRequest_(const camera3_device *device, camera3_capture_request *request)
{
    CameraDevice *camera = reinterpret_cast<CameraDevice *>(device->priv);
    if (!camera)
        return -EINVAL;

    // NOTE: create *RemoteRequest* at the very beginning, then recognize request type in Session.
    // TODO: check why LocalRequest will cause frame of input_buffer same as output_buffer.
    auto req = std::make_shared<RemoteRequest>(camera, request);

    return camera->processCaptureRequest(std::move(req));
}

void dump_(const camera3_device *device, int fd)
{
    CameraDevice *camera = reinterpret_cast<CameraDevice *>(device->priv);
    if (!camera)
        return;

    camera->dump(fd);
}

int flush_(const camera3_device *device)
{
    CameraDevice *camera = reinterpret_cast<CameraDevice *>(device->priv);
    if (!camera)
        return -EINVAL;

    return camera->flush();
}

void signalStreamFlush_(const struct camera3_device *device, uint32_t numStreams,
                        const camera3_stream_t *const *streams)
{
    CameraDevice *camera = reinterpret_cast<CameraDevice *>(device->priv);
    if (!camera)
        return;

    camera->signalStreamFlush(numStreams, streams);
}

int close_(hw_device_t *hw_device_t)
{
    camera3_device *device = reinterpret_cast<camera3_device *>(hw_device_t);
    CameraDevice *camera = reinterpret_cast<CameraDevice *>(device->priv);
    if (!camera)
        return -EINVAL;

    MLOGI(LogGroupHAL, "close camera %s", camera->getId().c_str());

    return camera->close();
}

void processCaptureResult_(const camera3_callback_ops *ops, const camera3_capture_result *result)
{
    CameraDevice *camera = static_cast<CameraDevice *>(const_cast<camera3_callback_ops *>(ops));
    camera->processCaptureResult(result);
}

void notify_(const camera3_callback_ops *ops, const camera3_notify_msg *msg)
{
    CameraDevice *camera = static_cast<CameraDevice *>(const_cast<camera3_callback_ops *>(ops));
    camera->notify(msg);
}

camera3_buffer_request_status requestStreamBuffers_(
    const camera3_callback_ops *ops, uint32_t num_buffer_reqs,
    const camera3_buffer_request *buffer_reqs,
    /*out*/ uint32_t *num_returned_buf_reqs,
    /*out*/ camera3_stream_buffer_ret *returned_buf_reqs)
{
    CameraDevice *camera = static_cast<CameraDevice *>(const_cast<camera3_callback_ops *>(ops));
    camera3_buffer_request_status status =
        camera->requestStreamBuffers(num_buffer_reqs, buffer_reqs,
                                     /*out*/ num_returned_buf_reqs,
                                     /*out*/ returned_buf_reqs);

    return status;
}

void returnStreamBuffers_(const camera3_callback_ops *ops, uint32_t num_buffers,
                          const camera3_stream_buffer *const *buffers)
{
    CameraDevice *camera = static_cast<CameraDevice *>(const_cast<camera3_callback_ops *>(ops));
    camera->returnStreamBuffers(num_buffers, buffers);
}

camera3_device_ops deviceOps = {
    .initialize = initialize_,
    .configure_streams = configureStreams_,
    .construct_default_request_settings = defaultSettings_,
    .process_capture_request = processCaptureRequest_,
    .dump = dump_,
    .flush = flush_,
    .signal_stream_flush = signalStreamFlush_,
};

} // namespace

CameraDevice::CameraDevice(std::string cameraId)
    : camera3_callback_ops{processCaptureResult_, notify_, requestStreamBuffers_,
                           returnStreamBuffers_},
      mId{cameraId},
      mRoleId{static_cast<int32_t>(RoleIdTypeNone)},
      mHal3Device{},
      mRawCallbacks2Fwk{nullptr},
      mCallbacks2Client{},
      mNumPartialResults{-1},
      mBufferManagerVersion{UINT32_MAX}
{
    mHal3Device.common.version = CAMERA_DEVICE_API_VERSION_CURRENT;
    mHal3Device.common.tag = HARDWARE_DEVICE_TAG;
    mHal3Device.common.close = close_;
    mHal3Device.ops = &deviceOps;
    mHal3Device.priv = this;
}

std::vector<CameraDevice *> CameraDevice::getPhyCameras() const
{
    if (mPhyCameras.size() || !isMultiCamera())
        return mPhyCameras;

    auto entry = findInStaticInfo(ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS);
    if (entry.count) {
        size_t count = entry.count;
        char *idChars = (char *)(entry.data.u8);
        while (count > 0) {
            std::string id{idChars};
            CameraDevice *camera = CameraManager::getInstance()->getCameraDevice(atoi(idChars));
            if (camera)
                mPhyCameras.push_back(camera);

            size_t len = id.size();
            count -= (len + 1);
            idChars += (len + 1);
        }
    }

    return mPhyCameras;
}

int32_t CameraDevice::getLogicalId() const
{
    // get the correct cameraId to do JPEG
    int32_t cameraId = atoi(mId.c_str());
    if (!isMultiCamera()) {
        return cameraId;
    }

    CameraDevice *maxSizePhyCamera = nullptr;
    int maxSize = 0;
    for (auto camera : mPhyCameras) {
        camera_metadata_ro_entry entry = camera->findInStaticInfo(ANDROID_JPEG_MAX_SIZE);
        if (entry.count) {
            int currentSize = entry.data.i32[0];
            if (maxSize < currentSize) {
                maxSize = currentSize;
                maxSizePhyCamera = camera;
            }
        }
    }

    return maxSizePhyCamera ? atoi(maxSizePhyCamera->getId().c_str()) : cameraId;
}

int32_t CameraDevice::getRoleId()
{
    if (mRoleId != RoleIdTypeNone) {
        return mRoleId;
    }

    camera_metadata_ro_entry entry = findInStaticInfo(MI_CAMERAID_ROLE_CAMERAID);
    if (entry.count) {
        mRoleId = entry.data.i32[0];
    } else {
        mRoleId = RoleIdRearWide;
    }

    return mRoleId;
}

int32_t CameraDevice::getPartialResultCount()
{
    if (mNumPartialResults > 0)
        return mNumPartialResults;

    camera_metadata_ro_entry entry = findInStaticInfo(ANDROID_REQUEST_PARTIAL_RESULT_COUNT);
    if (entry.count) {
        mNumPartialResults = entry.data.i32[0];
    }

    return mNumPartialResults;
}

CameraDevice *CameraDevice::getMostTelePhyCamera() const
{
    CameraDevice *tele = nullptr;

    float focalLength = 0.0f;
    auto phyCameras = getPhyCameras();
    for (CameraDevice *camera : phyCameras) {
        auto entry = camera->findInStaticInfo(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS);
        if (entry.count) {
            if (focalLength < entry.data.f[0]) {
                focalLength = entry.data.f[0];
                tele = camera;
            }
        }
    }

    return tele;
}

bool CameraDevice::isFrontCamera() const
{
    camera_metadata_ro_entry entry = findInStaticInfo(ANDROID_LENS_FACING);
    if (entry.count) {
        return entry.data.u8[0] == ANDROID_LENS_FACING_FRONT;
    } else {
        return false;
    }
}

bool CameraDevice::isMultiCamera() const
{
    // FIXME: NOTE: how to tell whether it is a multi camera?
    // now we have 2 methods: now option 2 seems better
    // 1. use the role id: it's simple but tricky, becasue it depends on how we define the RoleId
    // 2. use the tag android.logicalMultiCamera.physicalIds, it's of standard android
    camera_metadata_ro_entry entry = findInStaticInfo(ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS);
    return entry.count > 0 ? true : false;
}

bool CameraDevice::isSupportAnchorFrame() const
{
    camera_metadata_ro_entry entry = findInStaticInfo(MI_SUPPORT_ANCHOR_FRAME);
    if (entry.count) {
        return entry.data.u8[0];
    } else {
        return false;
    }
}

int CameraDevice::getAnchorFrameMask() const
{
    camera_metadata_ro_entry entry = findInStaticInfo(MI_ANCHOR_FRAME_MASK);
    if (entry.count) {
        return entry.data.i32[0];
    } else {
        return 0;
    }
}

int CameraDevice::getQuickShotSupportMask() const
{
    camera_metadata_ro_entry entry = findInStaticInfo(MI_QUICKSHOT_SUPPORT_MASK);
    if (entry.count) {
        return entry.data.i32[0];
    } else {
        return 0;
    }
}

uint64_t CameraDevice::getQuickShotDelayTimeMask() const
{
    camera_metadata_ro_entry entry = findInStaticInfo(MI_QUICKSHOT_DELAYTIME);
    if (entry.count) {
        return entry.data.i64[0];
    } else {
        return 0;
    }
}

bool CameraDevice::supportBufferManagerAPI()
{
    if (mBufferManagerVersion != UINT32_MAX)
        return mBufferManagerVersion ==
               ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION_HIDL_DEVICE_3_5;

    camera_metadata_ro_entry entry =
        findInStaticInfo(ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION);
    if (entry.count) {
        mBufferManagerVersion = entry.data.u8[0];
    } else {
        mBufferManagerVersion = 1;
    }

    return mBufferManagerVersion ==
           ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION_HIDL_DEVICE_3_5;
}

std::vector<CameraDevice *> CameraDevice::getCurrentFusionCameras(FusionCombination fc)
{
    std::vector<CameraDevice *> fusionCameras;
    if (getRoleId() != RoleIdRearSat) {
        return std::vector<CameraDevice *>();
    }

    std::vector<CameraDevice *> teleAndTele5x = {};
    std::vector<CameraDevice *> wideAndTele5x = {};
    std::vector<CameraDevice *> wideAndTele = {};
    std::vector<CameraDevice *> wideAndUw{};
    auto cameras = getPhyCameras();
    for (auto item : cameras) {
        switch (item->getRoleId()) {
        case RoleIdRearTele5x:
        case RoleIdRearTele10x:
            teleAndTele5x.push_back(item);
            wideAndTele5x.push_back(item);
            break;
        case RoleIdRearTele:
        case RoleIdRearTele2x:
        case RoleIdRearTele3x:
        case RoleIdRearTele4x:
            teleAndTele5x.push_back(item);
            wideAndTele.push_back(item);
            break;
        case RoleIdRearWide:
            wideAndTele5x.push_back(item);
            wideAndTele.push_back(item);
            wideAndUw.push_back(item);
            break;
        case RoleIdRearUltra:
            wideAndUw.push_back(item);
            break;
        default:
            break;
        }
    }
    if (teleAndTele5x.size() == 2 && fc == TeleAndTele5x) {
        fusionCameras = teleAndTele5x;
    } else if (wideAndTele5x.size() == 2 && fc == WideAndTele5x) {
        fusionCameras = wideAndTele5x;
    } else if (wideAndTele.size() == 2 && fc == WideAndTele) {
        fusionCameras = wideAndTele;
    } else if (wideAndUw.size() == 2 && fc == UltraWideAndWide) {
        fusionCameras = wideAndUw;
    }
    return fusionCameras;
}

} // namespace mihal
