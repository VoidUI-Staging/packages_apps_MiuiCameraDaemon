#include "MockModule.h"

#include "LogUtil.h"
#include "MockCamera.h"

namespace mihal {

namespace {

const int NUM_OF_MOCK_CAM = 2;

}

MockModule::MockModule(int startId, const MergedVendorCameraStaticInfo &mergedInfo)
    : CameraModule{startId}, mStaticMeta{10, 100}, mDefaultSettings{10, 100}
{
    mCallbacks = {
        .camera_device_status_change = nullptr,
        .torch_mode_status_change = nullptr,
    };
    initCameraInfo(mergedInfo);
    initDefaultSettings();

    // TODO : create the instance when open?
    for (int i = 0; i < NUM_OF_MOCK_CAM; i++) {
        mCameras.push_back(
            std::make_unique<MockCamera>(i + mStartId, &mInfo, &mDefaultSettings, this));
    }
}

void MockModule::initCameraInfo(const MergedVendorCameraStaticInfo &mergedInfo)
{
    uint8_t facing = ANDROID_LENS_FACING_EXTERNAL;
    mStaticMeta.update(ANDROID_LENS_FACING, &facing, 1);

    int32_t orientation = 0;
    mStaticMeta.update(ANDROID_SENSOR_ORIENTATION, &orientation, 1);

    int32_t maxOut[] = {0, 1, 1};
    mStaticMeta.update(ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS, maxOut, 3);

    int32_t maxInput = 1;
    mStaticMeta.update(ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS, &maxInput, 1);

    uint8_t pipelineDepth = 8;
    mStaticMeta.update(ANDROID_REQUEST_PIPELINE_MAX_DEPTH, &pipelineDepth, 1);

    uint8_t hardwareLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_3;
    mStaticMeta.update(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL, &hardwareLevel, 1);

    int32_t partialCount = 1;
    mStaticMeta.update(ANDROID_REQUEST_PARTIAL_RESULT_COUNT, &partialCount, 1);

    std::vector<uint8_t> cap{
        // ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE,
        ANDROID_REQUEST_AVAILABLE_CAPABILITIES_PRIVATE_REPROCESSING,
        ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE,
        ANDROID_REQUEST_AVAILABLE_CAPABILITIES_YUV_REPROCESSING,
        ANDROID_REQUEST_AVAILABLE_CAPABILITIES_SYSTEM_CAMERA,
    };
    mStaticMeta.update(ANDROID_REQUEST_AVAILABLE_CAPABILITIES, cap);

    std::vector<int32_t> sessionKeys;
    uint32_t clientNameTag;
    int err = Metadata::getTagFromName(MI_SESSION_PARA_CLIENT_NAME, &clientNameTag);
    if (!err)
        sessionKeys.push_back(clientNameTag);

    if (sessionKeys.size())
        mStaticMeta.update(ANDROID_REQUEST_AVAILABLE_SESSION_KEYS, sessionKeys);

    int32_t inputOutputMap[] = {HAL_PIXEL_FORMAT_YCBCR_420_888, 2, HAL_PIXEL_FORMAT_BLOB,
                                HAL_PIXEL_FORMAT_YCBCR_420_888};
    mStaticMeta.update(ANDROID_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP, inputOutputMap, 4);

    // TODO: refactor this ugly code
    std::vector<int32_t> streamConfig;
    for (const auto &stream : mergedInfo.streamSet) {
        streamConfig.push_back(std::get<0>(stream));
        streamConfig.push_back(std::get<1>(stream));
        streamConfig.push_back(std::get<2>(stream));
        streamConfig.push_back(std::get<3>(stream));
    }
    mStaticMeta.update(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, streamConfig);

    std::vector<int32_t> xiaomiStreamConfig;
    for (const auto &stream : mergedInfo.xiaomiStreamSet) {
        xiaomiStreamConfig.push_back(std::get<0>(stream));
        xiaomiStreamConfig.push_back(std::get<1>(stream));
        xiaomiStreamConfig.push_back(std::get<2>(stream));
        xiaomiStreamConfig.push_back(std::get<3>(stream));

        xiaomiStreamConfig.push_back(HAL_PIXEL_FORMAT_YCBCR_420_888);
        xiaomiStreamConfig.push_back(std::get<1>(stream));
        xiaomiStreamConfig.push_back(std::get<2>(stream));
        xiaomiStreamConfig.push_back(std::get<3>(stream));

        // NOTE: mihal will send some darkroom stream infos to app when it invite for mock buffer,
        // then app will use these stream infos to configure streams, but camera service will filter
        // out the streams which are not in ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, so we
        // need to make sure that every darkroom stream info (which are generated in
        // buildConfigToDarkroom) we send to app is contained in that metadata, for details, refer
        // to: frameworks/av/services/camera/libcameraservice/api2/CameraDeviceClient.cpp:
        // roundBufferDimensionNearest NOTE: these are bokeh darkroom streams which are generated in
        // BokehMode::buildConfigToDarkroom
        uint32_t depthScale = property_get_int32("persist.vendor.mibokeh.depth.scale", 0);
        uint32_t compressImage =
            property_get_int32("persist.vendor.camera.capturebokeh.compress.image", 0);
        if (depthScale && compressImage == 1) {
            xiaomiStreamConfig.push_back(HAL_PIXEL_FORMAT_Y16);
            xiaomiStreamConfig.push_back(std::get<1>(stream) * depthScale);
            xiaomiStreamConfig.push_back(std::get<2>(stream) * depthScale);
            xiaomiStreamConfig.push_back(std::get<3>(stream));
        }
        xiaomiStreamConfig.push_back(HAL_PIXEL_FORMAT_Y16);
        xiaomiStreamConfig.push_back(std::get<1>(stream) / 2);
        xiaomiStreamConfig.push_back(std::get<2>(stream) / 2);
        xiaomiStreamConfig.push_back(std::get<3>(stream));

        xiaomiStreamConfig.push_back(HAL_PIXEL_FORMAT_Y16);
        xiaomiStreamConfig.push_back(std::get<1>(stream));
        xiaomiStreamConfig.push_back(std::get<2>(stream));
        xiaomiStreamConfig.push_back(std::get<3>(stream));
    }
    mStaticMeta.update(MI_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, xiaomiStreamConfig);

    std::vector<int32_t> thumbnailSizes;
    for (const auto &thumbnail : mergedInfo.thumbnailSet) {
        thumbnailSizes.push_back(thumbnail.first);
        thumbnailSizes.push_back(thumbnail.second);
    }
    mStaticMeta.update(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES, thumbnailSizes);

    mStaticMeta.update(ANDROID_JPEG_MAX_SIZE, &mergedInfo.maxJpegSize, 1);

    mStaticMeta.update(QCOM_JPEG_DEBUG_DATA, &mergedInfo.jpegDebugSize, 1);

    int32_t activeSize[] = {0, 0, 12000, 9000};
    mStaticMeta.update(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, activeSize, 4);

    uint8_t timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    mStaticMeta.update(ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE, &timestampSource, 1);

    int32_t pixSize[] = {12000, 9000};
    mStaticMeta.update(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE, pixSize, 2);

    int32_t preCorrections[] = {0, 0, 12000, 9000};
    mStaticMeta.update(ANDROID_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE, preCorrections, 4);

    float zoomRatioRange[] = {0.1, 100.0};
    mStaticMeta.update(ANDROID_CONTROL_ZOOM_RATIO_RANGE, zoomRatioRange, 2);

    int32_t testPatterns[] = {ANDROID_SENSOR_TEST_PATTERN_MODE_BLACK,
                              ANDROID_SENSOR_TEST_PATTERN_MODE_OFF};
    mStaticMeta.update(ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES, testPatterns, 2);

    // TODO: set the android.scaler.availableMinFrameDurations
    // mStaticMeta.update(ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS, );

    // TODO: set the android.scaler.availableStallDurations
    // mStaticMeta.update(ANDROID_SCALER_AVAILABLE_STALL_DURATIONS, );

    // CameraDevice::RoleIdMock for universal mock camera
    int32_t roleId = CameraDevice::RoleIdMock;
    mStaticMeta.update(MI_CAMERAID_ROLE_CAMERAID, &roleId, 1);

    mStaticMeta.sort();

    mInfo.facing = facing;
    mInfo.orientation = orientation;
    mInfo.device_version = CAMERA_DEVICE_API_VERSION_CURRENT;
    mInfo.static_camera_characteristics = mStaticMeta.peek();
    mInfo.resource_cost = 0;
    mInfo.conflicting_devices = nullptr;
    mInfo.conflicting_devices_length = 0;

    MLLOGV(LogGroupHAL, mStaticMeta.toString(), "Mock Camera static info:");
}

void MockModule::initDefaultSettings()
{
    // JUST for test. is it possible to give a empty default setting?
    int32_t requestId = 1;
    mDefaultSettings.update(ANDROID_REQUEST_ID, &requestId, 1);
}

int MockModule::open(const char *id, camera3_device **device)
{
    return 0;
}

int MockModule::setCallbacks(const camera_module_callbacks *callbacks)
{
    mCallbacks = *callbacks;
    return 0;
}

void MockModule::getVendorTagOps(vendor_tag_ops *ops) {}

void MockModule::notifyDeviceStateChange(uint64_t deviceState){};

#if 0 // TODO: implement torch later
bool MockModule::isSetTorchModeSupported() const
{
    return true;
}

int MockModule::setTorchMode(const char *cameraId, bool enable)
{
    return 0;
}

int MockModule::turnOnTorchWithStrengthLevel(const char* camera_id, int32_t torchStrength)
{
    return 0;
}

int MockModule::getTorchStrengthLevel(const char* camera_id, int32_t* strengthLevel)
{
    return 0;
}
#endif

} // namespace mihal
