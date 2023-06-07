#include "VendorMappingCamera.h"

using namespace android;

#define EXTENSION_DEBUG
#ifdef EXTENSION_DEBUG
#define MYLOGDD(...) // MLOGD(__VA_ARGS__)
#define MYLOGD(...)  MLOGD(__VA_ARGS__)
#define MYLOGI(...)  MLOGI(__VA_ARGS__)
#else
#define MYLOGDD(...)
#define MYLOGD(...)
#define MYLOGI(...) MLOGI(__VA_ARGS__)
#endif

namespace mihal {

#define DEF_ANY             (-1)
#define DEF_IS_EXTENSION    (1)
#define DEF_NOT_EXTENSION   (0)
#define DEF_CAMERA_ID_REAR  (0)
#define DEF_CAMERA_ID_FRONT (1)
#define DEF_OPTMODE_0       (0)
#define DEF_APP_ANY         (NULL)
#define DEF_FUNC_ANY        (NULL)

//#define EXTENSION_MFNR_ENABLE

typedef struct
{
    int32_t idCamera;
    int32_t RoleIdCamera;
    int32_t isCameraX;
    int32_t paramOptMode;
    int32_t metaOptMode;
    const char *appPackageName;
} s_ExtensionInfo;

typedef struct extension_condition
{
    const char *conditionTag;
    s_ExtensionInfo info;
    bool operator<(const extension_condition &a) const
    {
        return strcmp(a.conditionTag, conditionTag) > 0;
    }
} s_ExtensionCondition;

typedef struct
{
    s_ExtensionInfo info;
    VendorMappingCamera::onChangeMeta funcChangeMeta;
} s_ExtensionResult;

void VendorMappingCamera::onChangeMetaSN(Metadata *metaModify, bool isConfig)
{
    uint8_t night_mode = 1;
    status_t result = metaModify->update(SDK_NIGHT_MODE_ENABLE, &night_mode, 1);
    MYLOGD(LogGroupHAL, "set onChangeMetaSN SDK_NIGHT_MODE_ENABLE %d", result);

    if (isConfig) {
        uint8_t face_detact_mode = ANDROID_STATISTICS_FACE_DETECT_MODE_SIMPLE;
        result = metaModify->update(ANDROID_STATISTICS_FACE_DETECT_MODE, &face_detact_mode, 1);
        MYLOGD(LogGroupHAL, "set onChangeMetaSN ANDROID_STATISTICS_FACE_DETECT_MODE %d", result);
    }
#ifdef EXTENSION_MFNR_ENABLE
    result = metaModify->update(SDK_MFNR_ENABLE, &night_mode, 1);
    MYLOGD(LogGroupHAL, "set onChangeMetaSN SDK_MFNR_ENABLE %d", result);
#endif
}

void VendorMappingCamera::onChangeMetaBokeh(Metadata *metaModify, bool isConfig)
{
    status_t result = 0;
    if (isConfig) {
        int32_t bokehRoleId = mihal::VendorCamera::RoleIdRearBokeh1x;
        result = metaModify->update(MI_CAMERA_BOKEH_ROLE, &bokehRoleId, 1);
        MYLOGD(LogGroupHAL, "set onChangeMetaBokeh MI_CAMERA_BOKEH_ROLE %d", result);
    }

    uint8_t bokeh_mode = 1;
    result = metaModify->update(SDK_BOKEH_ENABLE, &bokeh_mode, 1);
    MYLOGD(LogGroupHAL, "set onChangeMetaBokeh SDK_BOKEH_ENABLE %d", result);
#ifdef EXTENSION_MFNR_ENABLE
    result = metaModify->update(SDK_MFNR_ENABLE, &bokeh_mode, 1);
    MYLOGD(LogGroupHAL, "set onChangeMetaBokeh SDK_MFNR_ENABLE %d", result);
#endif
    const char *bokehFnumber = "4";
    result = metaModify->update(SDK_BOKEH_FNUMBER_APPLIED, bokehFnumber);
    MYLOGD(LogGroupHAL, "set onChangeMetaBokeh SDK_BOKEH_FNUMBER_APPLIED %d", result);
}
void VendorMappingCamera::onChangeMetaBokehFront(Metadata *metaModify, bool isConfig)
{
    uint8_t bokeh_mode = 1;
    status_t result = metaModify->update(SDK_BOKEH_ENABLE, &bokeh_mode, 1);
    MYLOGD(LogGroupHAL, "set onChangeMetaBokehFront SDK_BOKEH_ENABLE %d", result);
    const char *bokehFnumber = "4";
    result = metaModify->update(SDK_BOKEH_FNUMBER_APPLIED, bokehFnumber);
    MYLOGD(LogGroupHAL, "set onChangeMetaBokehFront SDK_BOKEH_FNUMBER_APPLIED %d", result);
}
void VendorMappingCamera::onChangeMetaHdr(Metadata *metaModify, bool isConfig)
{
    uint8_t hdr_mode = 1;
    status_t result = metaModify->update(SDK_HDR_MODE, &hdr_mode, 1);
    MYLOGD(LogGroupHAL, "set onChangeMetaHdr SDK_HDR_MODE %d", result);

#ifdef EXTENSION_MFNR_ENABLE
    result = metaModify->update(SDK_MFNR_ENABLE, &hdr_mode, 1);
    MYLOGD(LogGroupHAL, "set onChangeMetaHdr SDK_MFNR_ENABLE %d", result);
#endif
    hdr_mode = 0;
    result = metaModify->update(ANDROID_CONTROL_ENABLE_ZSL, &hdr_mode, 1);
    MYLOGD(LogGroupHAL, "set onChangeMetaHdr ANDROID_CONTROL_ENABLE_ZSL %d", result);
}
void VendorMappingCamera::onChangeMetaBeauty(Metadata *metaModify, bool isConfig)
{
    status_t result = 0;
#ifdef EXTENSION_MFNR_ENABLE
    uint8_t mfnr_enable = 1;
    result = metaModify->update(SDK_MFNR_ENABLE, &mfnr_enable, 1);
    MYLOGD(LogGroupHAL, "set onChangeMetaBeauty SDK_MFNR_ENABLE %d", result);
#endif
    int32_t beauty_level = 50;
    result = metaModify->update(SDK_BEAUTY_SKINSMOOTH, &beauty_level, 1);
    MYLOGD(LogGroupHAL, "set onChangeMetaBeauty SDK_BEAUTY_SKINSMOOTH %d", result);
    result = metaModify->update(SDK_BEAUTY_SLIMFACE, &beauty_level, 1);
    MYLOGD(LogGroupHAL, "set onChangeMetaBeauty SDK_BEAUTY_SLIMFACE %d", result);
}

static const std::map<s_ExtensionCondition, s_ExtensionResult> s_Condition{
    // {{0, DEF_ANY, 1, 0, StreamConfigModeThirdPartySuperNight, NULL},
    // {DEF_ANY, RoleIdRearWide, 1,  StreamConfigModeMiuiSuperNight, StreamConfigModeMiuiSuperNight,
    // "com.android.camera"}},

    // auto
    // {{DEF_CAMERA_ID_REAR, DEF_ANY, DEF_IS_EXTENSION, DEF_OPTMODE_0,
    //   mihal::StreamConfigModeThirdPartyAutoExtension, DEF_APP_ANY, DEF_FUNC_ANY},
    //  {DEF_ANY, mihal::VendorCamera::RoleIdRearSat, DEF_NOT_EXTENSION, mihal::StreamConfigModeSAT,
    //   DEF_ANY, "com.android.camera", &VendorMappingCamera::onChangeMetaAuto}},

    // night
    {{"1. rear night",
      {DEF_CAMERA_ID_REAR, DEF_ANY, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartySuperNight, DEF_APP_ANY}},
     {{DEF_ANY, DEF_ANY, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartySuperNight, DEF_APP_ANY},
      &VendorMappingCamera::onChangeMetaSN}},
    {{"2. front night",
      {DEF_CAMERA_ID_FRONT, DEF_ANY, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartySuperNight, DEF_APP_ANY}},
     {{DEF_ANY, mihal::VendorCamera::RoleIdFront, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartySuperNight, DEF_APP_ANY},
      &VendorMappingCamera::onChangeMetaSN}},

    // bokeh
    {{"3. rear bokeh",
      {DEF_CAMERA_ID_REAR, DEF_ANY, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartyBokeh, DEF_APP_ANY}}, // 0xff17
     {{DEF_ANY, mihal::VendorCamera::RoleIdRearSat, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartyBokeh, DEF_APP_ANY},
      &VendorMappingCamera::onChangeMetaBokeh}},

    {{"4. front bokeh",
      {DEF_CAMERA_ID_FRONT, DEF_ANY, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartyBokeh, DEF_APP_ANY}},
     {{DEF_ANY, mihal::VendorCamera::RoleIdFront, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartyBokeh, DEF_APP_ANY},
      &VendorMappingCamera::onChangeMetaBokehFront}},

    // hdr
    {{"5. rear hdr",
      {DEF_CAMERA_ID_REAR, mihal::VendorCamera::RoleIdRear3PartSat, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartyHDR, DEF_APP_ANY}},
     {{DEF_ANY, mihal::VendorCamera::RoleIdRearSat, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartyNormal, DEF_APP_ANY},
      &VendorMappingCamera::onChangeMetaHdr}},

    {{"6. front hdr",
      {DEF_CAMERA_ID_FRONT, DEF_ANY, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartyHDR, DEF_APP_ANY}},
     {{DEF_ANY, DEF_ANY, DEF_IS_EXTENSION, DEF_OPTMODE_0, mihal::StreamConfigModeThirdPartyNormal,
       DEF_APP_ANY},
      &VendorMappingCamera::onChangeMetaHdr}},

    // beauty
    {{"7. rear beauty",
      {DEF_CAMERA_ID_REAR, mihal::VendorCamera::RoleIdRear3PartSat, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartyBeauty, DEF_APP_ANY}},
     {{DEF_ANY, mihal::VendorCamera::RoleIdRearSat, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartyNormal, DEF_APP_ANY},
      &VendorMappingCamera::onChangeMetaBeauty}},

    {{"8. front beauty",
      {DEF_CAMERA_ID_FRONT, DEF_ANY, DEF_IS_EXTENSION, DEF_OPTMODE_0,
       mihal::StreamConfigModeThirdPartyBeauty, DEF_APP_ANY}},
     {{DEF_ANY, DEF_ANY, DEF_IS_EXTENSION, DEF_OPTMODE_0, mihal::StreamConfigModeThirdPartyNormal,
       DEF_APP_ANY},
      &VendorMappingCamera::onChangeMetaBeauty}},
};

VendorMappingCamera::VendorMappingCamera(int cameraId, const camera_info &info,
                                         CameraModule *module)
    : VendorCamera{cameraId, info, module}
{
    MYLOGI(LogGroupHAL, "cameraId:%d VendorMappingCamera:%p", cameraId, this);
    mMapping = NULL;
    mIsMapped = false;
    mIsClosed = false;
    mCurMetaCb = NULL;
}
int VendorMappingCamera::open()
{
    MYLOGI(LogGroupHAL, "open id:%s, mMapping:%p, mIsMapped:%d mIsClosed:%d", mId.c_str(), mMapping,
           mIsMapped, mIsClosed);
    if (mIsMapped) {
        MLOGE(LogGroupHAL, "open id:%s mIsMapped is true, error!", mId.c_str());
    }

    mIsMapped = false;
    mIsClosed = false;
    mCurMetaCb = NULL;
    return VendorCamera::open();
}
int VendorMappingCamera::close()
{
    MYLOGI(LogGroupHAL, "close id:%s, mMapping:%p, mIsMapped:%d, mIsClosed:%d", mId.c_str(),
           mMapping, mIsMapped, mIsClosed);
    int ret = 0;
    if (NULL != mMapping) {
        mMapping->flush();
        ret = mMapping->close();
    }

    if (!mIsClosed) {
        ret |= VendorCamera::close();
    }
    // int ret = (NULL == mMapping) ? VendorCamera::close() : mMapping->close();
    mMapping = NULL;
    mIsMapped = false;
    return ret;
}

void VendorMappingCamera::switchCameraDevice(VendorMappingCamera *mappingChanged)
{
    MYLOGD(LogGroupHAL, "switchCameraDevice id:%s, mMapping:%p, mIsMapped:%d mIsClosed:%d",
           mId.c_str(), mMapping, mIsMapped, mIsClosed);

    if (mMapping == NULL) {
        if (NULL == mappingChanged) {
            MYLOGD(LogGroupHAL, "switchCameraDevice ignore id:%s", mId.c_str());
            return;
        }

        MYLOGI(LogGroupHAL, "switchCameraDevice create mappingChanged: %s -> %s", mId.c_str(),
               mappingChanged->mId.c_str());
        VendorCamera::flush();
        VendorCamera::close();
        mIsClosed = true;
    } else {
        if (NULL == mappingChanged) {
            MYLOGI(LogGroupHAL, "switchCameraDevice remove mappingChanged: %s -> %s",
                   mMapping->mId.c_str(), mId.c_str());
            mMapping->flush();
            mMapping->close();
            VendorCamera::open();
            // if (mMapping->mRawCallbacks2Fwk) {
            //     VendorCamera::initialize(mRawCallbacks2Fwk);
            // }

            // VendorCamera::initialize(mMapping->mCallbacks2Client);
            mIsClosed = false;
            mMapping = NULL;
            return;
        }

        if (0 == mMapping->mId.compare(mappingChanged->mId)) {
            MYLOGI(LogGroupHAL, "switchCameraDevice ignore mappingChanged: %s",
                   mappingChanged->mId.c_str());
            return;
        }

        mMapping->flush();
        mMapping->close();
        MYLOGI(LogGroupHAL, "switchCameraDevice change mappingChanged: %s -> %s",
               mMapping->mId.c_str(), mappingChanged->mId.c_str());
        mMapping = NULL;
    }

    // MYLOGI(LogGroupHAL, "switchCameraDevice sleep %s begin", mId.c_str());
    // usleep(20);
    // MYLOGI(LogGroupHAL, "switchCameraDevice sleep %s end", mId.c_str());
    mMapping = mappingChanged;
    mMapping->open();
    if (mRawCallbacks2Fwk) {
        mMapping->initialize(mRawCallbacks2Fwk);
    }

    mMapping->initialize(mCallbacks2Client);
    mMapping->setMapped(true);
}

int VendorMappingCamera::configureStreams(std::shared_ptr<StreamConfig> config)
{
    MYLOGI(LogGroupHAL, "configureStreams id:%s", mId.c_str());
    if (!isExtensionMode()) {
        tryExtensionMode(config);
    }

    condition_dump((NULL == mMapping) ? "mapped" : "mapping", config.get());

    // condition_dump(config.get());
    // MYLOGI(LogGroupHAL, "configureStreams id:%s BBBB operation mode is 0x%x", mId.c_str(),
    //        config->getOpMode());

    // MYLOGDD(LogGroupHAL, "configureStreams id:%s CCCC", mId.c_str());
    // int ret = (NULL == mMapping) ? VendorCamera::configureStreams(config)
    //                              : mMapping->configureStreams(config);
    // MYLOGI(LogGroupHAL, "configureStreams id:%s DDDD end", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::configureStreams(config)
                              : mMapping->configureStreams(config);
}
// int VendorMappingCamera::configureStreams(StreamConfig *config)
// {
//     MYLOGD(LogGroupHAL, "configureStreams id:%s", mId.c_str());
//     return (NULL ==
//     mMapping)?VendorCamera::configureStreams(config):mMapping->configureStreams(config);
// }

const camera_metadata *VendorMappingCamera::defaultSettings(int d) const
{
    MYLOGDD(LogGroupHAL, "defaultSettings id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::defaultSettings(d) : mMapping->defaultSettings(d);
}
int VendorMappingCamera::processCaptureRequest(std::shared_ptr<CaptureRequest> request)
{
    MYLOGDD(LogGroupHAL, "processCaptureRequest id:%s", mId.c_str());
    if (!mIsMapped) {
        processRequest(request);
    }
    return (NULL == mMapping) ? VendorCamera::processCaptureRequest(request)
                              : mMapping->processCaptureRequest(request);
}
// int VendorMappingCamera::processCaptureRequest(CaptureRequest *request)
// {
//     MYLOGD(LogGroupHAL, "processCaptureRequest id:%s", mId.c_str());
//     return (NULL ==
//     mMapping)?VendorCamera::processCaptureRequest(request):mMapping->processCaptureRequest(request);
// }

void VendorMappingCamera::dump(int fd)
{
    MYLOGDD(LogGroupHAL, "dump id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::dump(fd) : mMapping->dump(fd);
}
int VendorMappingCamera::flush()
{
    MYLOGD(LogGroupHAL, "flush id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::flush() : mMapping->flush();
}
void VendorMappingCamera::signalStreamFlush(uint32_t numStream,
                                            const camera3_stream_t *const *streams)
{
    MYLOGD(LogGroupHAL, "signalStreamFlush id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::signalStreamFlush(numStream, streams)
                              : mMapping->signalStreamFlush(numStream, streams);
}

// void VendorMappingCamera::processCaptureResult(const camera3_capture_result *result)
// {
//     MYLOGD(LogGroupHAL, "processCaptureResult id:%s", mId.c_str());
//     return (NULL ==
//     mMapping)?VendorCamera::processCaptureResult(result):mMapping->processCaptureResult(result);
// }
void VendorMappingCamera::processCaptureResult(const CaptureResult *result)
{
    MYLOGDD(LogGroupHAL, "processCaptureResult id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::processCaptureResult(result)
                              : mMapping->processCaptureResult(result);
}
// void VendorMappingCamera::processCaptureResult(std::shared_ptr<const CaptureResult> result)
// {
//     MYLOGD(LogGroupHAL, "processCaptureResult id:%s", mId.c_str());
//     return (NULL ==
//     mMapping)?VendorCamera::processCaptureResult(result):mMapping->processCaptureResult(result);
// }

void VendorMappingCamera::notify(const camera3_notify_msg *msg)
{
    MYLOGDD(LogGroupHAL, "notify id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::notify(msg) : mMapping->notify(msg);
}
camera3_buffer_request_status VendorMappingCamera::requestStreamBuffers(
    uint32_t num_buffer_reqs, const camera3_buffer_request *buffer_reqs,
    /*out*/ uint32_t *num_returned_buf_reqs,
    /*out*/ camera3_stream_buffer_ret *returned_buf_reqs)
{
    // ALOGE("requestStreamBuffers");
    MYLOGDD(LogGroupHAL, "requestStreamBuffers id:%s", mId.c_str());
    // ALOGE("requestStreamBuffers A mMapping:%p", mMapping);
    // camera3_buffer_request_status ret = (NULL ==
    // mMapping)?VendorCamera::requestStreamBuffers(num_buffer_reqs,buffer_reqs,num_returned_buf_reqs,returned_buf_reqs):
    //         mMapping->requestStreamBuffers(num_buffer_reqs,buffer_reqs,num_returned_buf_reqs,returned_buf_reqs);
    // ALOGE("requestStreamBuffers B");
    // return ret;
    return (NULL == mMapping)
               ? VendorCamera::requestStreamBuffers(num_buffer_reqs, buffer_reqs,
                                                    num_returned_buf_reqs, returned_buf_reqs)
               : mMapping->requestStreamBuffers(num_buffer_reqs, buffer_reqs, num_returned_buf_reqs,
                                                returned_buf_reqs);
}
std::shared_ptr<StreamBuffer> VendorMappingCamera::requestStreamBuffers(uint32_t frame,
                                                                        uint32_t streamIdx)
{
    MYLOGDD(LogGroupHAL, "requestStreamBuffers  id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::requestStreamBuffers(frame, streamIdx)
                              : mMapping->requestStreamBuffers(frame, streamIdx);
}
void VendorMappingCamera::returnStreamBuffers(uint32_t num_buffers,
                                              const camera3_stream_buffer *const *buffers)
{
    MYLOGDD(LogGroupHAL, "returnStreamBuffers  id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::returnStreamBuffers(num_buffers, buffers)
                              : mMapping->returnStreamBuffers(num_buffers, buffers);
}

int VendorMappingCamera::initialize2Vendor(const camera3_callback_ops *callbacks)
{
    MYLOGDD(LogGroupHAL, "initialize2Vendor  id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::initialize2Vendor(callbacks)
                              : mMapping->initialize2Vendor(callbacks);
}
int VendorMappingCamera::configureStreams2Vendor(StreamConfig *config)
{
    MYLOGDD(LogGroupHAL, "configureStreams2Vendor  id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::configureStreams2Vendor(config)
                              : mMapping->configureStreams2Vendor(config);
}
int VendorMappingCamera::submitRequest2Vendor(CaptureRequest *request)
{
    MYLOGDD(LogGroupHAL, "submitRequest2Vendor  id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::submitRequest2Vendor(request)
                              : mMapping->submitRequest2Vendor(request);
}
int VendorMappingCamera::flush2Vendor()
{
    MYLOGDD(LogGroupHAL, "flush2Vendor  id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::flush2Vendor() : mMapping->flush2Vendor();
}
void VendorMappingCamera::signalStreamFlush2Vendor(uint32_t numStream,
                                                   const camera3_stream_t *const *streams)
{
    MYLOGDD(LogGroupHAL, "signalStreamFlush2Vendor  id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::signalStreamFlush2Vendor(numStream, streams)
                              : mMapping->signalStreamFlush2Vendor(numStream, streams);
}
int VendorMappingCamera::getCameraInfo(camera_info *info) const
{
    MYLOGDD(LogGroupHAL, "getCameraInfo  id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::getCameraInfo(info) : mMapping->getCameraInfo(info);
}
MiHal3BufferMgr *VendorMappingCamera::getFwkStreamHal3BufMgr()
{
    MYLOGDD(LogGroupHAL, "getFwkStreamHal3BufMgr  id:%s", mId.c_str());
    return (NULL == mMapping) ? VendorCamera::getFwkStreamHal3BufMgr()
                              : mMapping->getFwkStreamHal3BufMgr();
}
#if 0
void VendorMappingCamera::meta_dump(const char *tag, Metadata *meta)
{
    MYLOGI(LogGroupHAL, "%s meta_dump begin", tag);
    const char *summary = meta->toStringSummary().c_str();
    char *tmp = new char[strlen(summary) + 1];
    strcpy(tmp, summary);
    char *token = strtok(tmp, "\n");
    while (token != NULL) {
        MYLOGI(LogGroupHAL, "%s", token);
        token = strtok(NULL, "\n");
    }
    delete[] tmp;
    MYLOGI(LogGroupHAL, "%s meta_dump end", tag);
    // MYLOGI(LogGroupHAL, "%s condition_dump: %s", tag, meta->toStringSummary().c_str());
}
#endif
void VendorMappingCamera::condition_dump(const char *tag, StreamConfig *config)
{
    MYLOGD(LogGroupHAL, "%s condition_dump id: %d, roleid: %d, para_opt_mode:0x%x", tag,
           atoi(mId.c_str()), getRoleId(), config->getOpMode());
    auto meta = config->getMeta();
    auto entry = meta->find(SDK_SESSION_PARAMS_OPERATION);
    if (entry.count > 0) {
        MYLOGD(LogGroupHAL, "%s condition_dump metaOptMode is 0x%x", tag, entry.data.i32[0]);
    } else {
        MYLOGD(LogGroupHAL, "%s condition_dump Can't get metaOptMode", tag);
    }
    entry = meta->find(SDK_SESSION_PARAMS_ISCAMERAX);
    if (entry.count > 0) {
        MYLOGD(LogGroupHAL, "%s condition_dump isCameraX state is %d", tag, entry.data.i32[0]);
    } else {
        MYLOGD(LogGroupHAL, "%s condition_dump Can't get isCameraX", tag);
    }
    entry = meta->find(SDK_SESSION_PARAMS_CLIENT);
    if (entry.count > 0) {
        std::string temp(reinterpret_cast<const char *>(entry.data.u8));
        MYLOGD(LogGroupHAL, "%s condition_dump appPackageName is %s", tag, temp.c_str());
    } else {
        MYLOGD(LogGroupHAL, "%s condition_dump Can't get appPackageName", tag);
    }

    entry = meta->find(MI_CAMERA_BOKEH_ROLE);
    if (entry.count > 0) {
        MYLOGD(LogGroupHAL, "%s condition_dump bokehRole state is %d", tag, entry.data.i32[0]);
    } else {
        MYLOGD(LogGroupHAL, "%s condition_dump Can't get bokehRole", tag);
    }

    entry = meta->find(SDK_BOKEH_ENABLE);
    if (entry.count > 0) {
        MYLOGD(LogGroupHAL, "%s condition_dump SDK_BOKEH_ENABLE is %d", tag, entry.data.u8[0]);
    } else {
        MYLOGD(LogGroupHAL, "%s condition_dump Can't get SDK_BOKEH_ENABLE", tag);
    }

    entry = meta->find(SDK_NIGHT_MODE_ENABLE);
    if (entry.count > 0) {
        MYLOGD(LogGroupHAL, "%s condition_dump SDK_NIGHT_MODE_ENABLE is %d", tag, entry.data.u8[0]);
    } else {
        MYLOGD(LogGroupHAL, "%s condition_dump Can't get SDK_NIGHT_MODE_ENABLE", tag);
    }

    entry = meta->find(SDK_BOKEH_FNUMBER_APPLIED);
    if (entry.count > 0) {
        std::string temp(reinterpret_cast<const char *>(entry.data.u8));
        MYLOGD(LogGroupHAL, "%s condition_dump SDK_BOKEH_FNUMBER_APPLIED is %s", tag, temp.c_str());
    } else {
        MYLOGD(LogGroupHAL, "%s condition_dump Can't get SDK_BOKEH_FNUMBER_APPLIED", tag);
    }
}

bool VendorMappingCamera::isExtensionMode()
{
    MYLOGD(LogGroupHAL, "isExtensionMode mIsMapped:%d, mMapping:%p", mIsMapped, mMapping);
    return mIsMapped;
}

void VendorMappingCamera::tryExtensionMode(std::shared_ptr<StreamConfig> config)
{
    MYLOGD(LogGroupHAL, "tryExtensionMode begin mIsMapped:%d", mIsMapped);

    condition_dump("origin", config.get());
    mCurMetaCb = NULL;
    auto meta = config->getMeta();
    const s_ExtensionInfo *condition = NULL;
    const s_ExtensionResult *result = NULL;
    const char *conditionTagName = NULL;
    int curDeviceId = atoi(mId.c_str());
    int curDeviceRoleId = getRoleId();
    int curDeviceOptModeParam = config->getOpMode();
    int curDeviceIsCameraX = DEF_ANY;
    int curDeviceOptModeMeta = DEF_ANY;
    std::string curDeviceAppName;
    VendorMappingCamera *mappingChanged = NULL;

    for (auto &c : s_Condition) {
        conditionTagName = c.first.conditionTag;
        condition = &(c.first.info);
        if (condition->isCameraX != DEF_ANY) {
            if (curDeviceIsCameraX < 0) {
                auto entry = meta->find(SDK_SESSION_PARAMS_ISCAMERAX);
                if (entry.count < 1) {
                    MYLOGD(LogGroupHAL, "condition %s isCameraX continue none[%d]",
                           conditionTagName, condition->isCameraX);
                    curDeviceIsCameraX = 0;
                    continue;
                }

                curDeviceIsCameraX = entry.data.i32[0];
            }

            if (curDeviceIsCameraX != condition->isCameraX) {
                MYLOGD(LogGroupHAL, "condition %s isCameraX continue %d[%d]", conditionTagName,
                       curDeviceIsCameraX, condition->isCameraX);
                continue;
            }
        }

        if (condition->idCamera != DEF_ANY) {
            if (curDeviceId != condition->idCamera) {
                MYLOGD(LogGroupHAL, "condition %s idCamera continue %d[%d]", conditionTagName,
                       curDeviceId, condition->idCamera);
                continue;
            }
        }

        if (condition->RoleIdCamera != DEF_ANY) {
            if (curDeviceRoleId != condition->RoleIdCamera) {
                MYLOGD(LogGroupHAL, "condition %s RoleIdCamera continue %d[%d]", conditionTagName,
                       curDeviceRoleId, condition->RoleIdCamera);
                continue;
            }
        }

        if (condition->paramOptMode != DEF_ANY) {
            if (curDeviceOptModeParam != condition->paramOptMode) {
                MYLOGD(LogGroupHAL, "condition %s paramOptMode continue %d[%d]", conditionTagName,
                       curDeviceOptModeParam, condition->paramOptMode);
                continue;
            }
        }

        if (condition->metaOptMode != DEF_ANY) {
            if (curDeviceOptModeMeta < 0) {
                auto entry = meta->find(SDK_SESSION_PARAMS_OPERATION);
                if (entry.count < 1) {
                    MYLOGD(LogGroupHAL, "condition %s metaOptMode continue none[%d]",
                           conditionTagName, condition->metaOptMode);
                    curDeviceOptModeMeta = 0;
                    continue;
                }
                curDeviceOptModeMeta = entry.data.i32[0];
            }

            if (curDeviceOptModeMeta != condition->metaOptMode) {
                MYLOGD(LogGroupHAL, "condition %s metaOptMode continue %d[%d]", conditionTagName,
                       curDeviceOptModeMeta, condition->metaOptMode);
                continue;
            }
        }

        if (condition->appPackageName != DEF_APP_ANY) {
            if (curDeviceAppName.length() < 1) {
                auto entry = meta->find(SDK_SESSION_PARAMS_CLIENT);
                if (entry.count < 1) {
                    MYLOGD(LogGroupHAL, "condition %s metaOptMode continue none[%s]",
                           conditionTagName, condition->appPackageName);
                    curDeviceAppName = "com.xiaomi.camera.mivi";
                    continue;
                }
                curDeviceAppName = std::string(reinterpret_cast<const char *>(entry.data.u8));
            }

            if (0 == curDeviceAppName.compare(condition->appPackageName)) {
                MYLOGD(LogGroupHAL, "condition %s metaOptMode continue %s[%s]", conditionTagName,
                       curDeviceAppName.c_str(), condition->appPackageName);
                continue;
            }
        }

        MYLOGI(LogGroupHAL, "condition %s ok, [%d,%d,0x%x,0x%x]", conditionTagName,
               condition->idCamera, condition->RoleIdCamera, condition->paramOptMode,
               condition->metaOptMode);

        result = &(c.second);
        break;
    }

    if (NULL != result) {
        auto resultInfo = &(result->info);
        if (resultInfo->paramOptMode != DEF_ANY &&
            resultInfo->paramOptMode != condition->paramOptMode) {
            config->toRaw()->operation_mode = resultInfo->paramOptMode;
            MYLOGD(LogGroupHAL, "resultInfo set operation_mode 0x%x", resultInfo->paramOptMode);
        }

        if (resultInfo->appPackageName != DEF_APP_ANY &&
            (0 != curDeviceAppName.compare(resultInfo->appPackageName))) {
            auto c = reinterpret_cast<RemoteConfig *>(config.get());
            Metadata *metaModify = c->getModifyMeta();
            metaModify->update(SDK_SESSION_PARAMS_CLIENT,
                               (const uint8_t *)resultInfo->appPackageName,
                               strlen(resultInfo->appPackageName) + 1);
            MYLOGD(LogGroupHAL, "resultInfo set appPackageName %s", resultInfo->appPackageName);
        }

        MYLOGDD(LogGroupHAL, "resultInfo set metaOptMode begin: 0x%x, 0x%x", condition->metaOptMode,
                resultInfo->metaOptMode);

        if (resultInfo->metaOptMode != DEF_ANY &&
            resultInfo->metaOptMode != condition->metaOptMode) {
            auto c = reinterpret_cast<RemoteConfig *>(config.get());
            Metadata *metaModify = c->getModifyMeta();
            metaModify->update(SDK_SESSION_PARAMS_OPERATION, &resultInfo->metaOptMode, 1);
            MYLOGD(LogGroupHAL, "resultInfo set metaOptMode 0x%x", resultInfo->metaOptMode);
        }

        if (result->funcChangeMeta != DEF_FUNC_ANY) {
            auto c = reinterpret_cast<RemoteConfig *>(config.get());
            ((*this).*(result->funcChangeMeta))(c->getModifyMeta(), true); // ((*this).*mFunc)();
            // config->toRaw()->session_parameters
            mCurMetaCb = result->funcChangeMeta;
            MYLOGD(LogGroupHAL, "result set funcChangeMeta");
        }

        if (resultInfo->idCamera != DEF_ANY && resultInfo->idCamera != condition->idCamera) {
            mappingChanged = reinterpret_cast<VendorMappingCamera *>(
                mModule->getCameraDevice(resultInfo->idCamera));
            MYLOGI(LogGroupHAL, "resultInfo set mappingChanged idCamera %d!!!!",
                   resultInfo->idCamera);
        }

        if (NULL == mappingChanged && resultInfo->RoleIdCamera > DEF_ANY) {
            CameraDevice *device = NULL;
            for (int id = 0; id < mModule->getNumberOfCameras(); id++) {
                device = mModule->getCameraDevice(id);
                if (device->getRoleId() != resultInfo->RoleIdCamera) {
                    continue;
                }

                if (curDeviceId == id) {
                    continue;
                }
                MYLOGI(LogGroupHAL, "resultInfo set mappingChanged RoleIdCamera %d[%d]!!!!",
                       resultInfo->RoleIdCamera, id);
                mappingChanged = reinterpret_cast<VendorMappingCamera *>(device);
            }
        }
    }

    switchCameraDevice(mappingChanged);
    MYLOGI(LogGroupHAL, "tryExtensionMode end");
}

void VendorMappingCamera::processRequest(std::shared_ptr<CaptureRequest> captureRequest)
{
    MYLOGDD(LogGroupHAL, "processRequest %p", mCurMetaCb);
    if (NULL == mCurMetaCb) {
        return;
    }
    RemoteRequest *lq = reinterpret_cast<RemoteRequest *>(captureRequest.get());
    Metadata *meta = lq->getModifyMeta();
    if (NULL == meta || meta->isEmpty()) {
        // MYLOGD(LogGroupHAL, "processRequest meta is empty");
        return;
    }
    // meta_dump("processRequest", meta);
    ((*this).*mCurMetaCb)(meta, false); // ((*this).*mFunc)();
    lq->submitMetaModified();
}
}; // namespace mihal