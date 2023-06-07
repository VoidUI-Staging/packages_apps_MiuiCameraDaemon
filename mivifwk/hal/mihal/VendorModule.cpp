#include "VendorModule.h"

#include "LogUtil.h"
#include "VendorMappingCamera.h"
#include "VendorMetadataParser.h"
#include "VirtualCamera.h"

static const int32_t sMihalVtCamera = property_get_int32("persist.vendor.camera.mihal.vtcamera", 0);

namespace mihal {

VendorModule::VendorModule(camera_module *module, int startId)
    : CameraModule{startId}, mModuleImpl{module}
{
    // assert(module != nullptr, "null hardware module!!");
    assert(module != nullptr);
    init();
}

VendorModule::~VendorModule()
{
    for (auto &pair : mPhysicalMetas) {
        pair.second.release();
    }
}

int VendorModule::init()
{
    int ret = 0;

    // TODO: check the module version
    if (mModuleImpl->init)
        ret = mModuleImpl->init();

    int num = mModuleImpl->get_number_of_cameras();
    // TODO: filter out the cameras that we do not want to export to framework in need
    // int num = mModuleImpl->get_number_of_cameras() - 3;
    // ...

    // FIXME: use a more graceful manner to set the gloable vendor tag ops
    // and also consider together with decoupling of MI meta tags
    vendor_tag_ops ops;
    getVendorTagOps(&ops);
    Metadata::setEarlyVendorTagOps(&ops);

    for (int i = 0; i < num; i++) {
        camera_info info;
        int err = mModuleImpl->get_camera_info(i, &info);
        if (err) {
            MLOGE(LogGroupHAL, "failed to get camera info for camera %d", i);
            // TODO: error handle
            continue;
        }

        camera_metadata_t *staticMeta =
            const_cast<camera_metadata_t *>(info.static_camera_characteristics);
        void *pData = VendorMetadataParser::getTag(staticMeta, MI_MIVI_FWK_VERSION);
        if (pData) {
            float miviVersion = 2.0;
            VendorMetadataParser::setVTagValue(staticMeta, MI_MIVI_FWK_VERSION, &miviVersion, 1);
        }

        StaticMetadataWraper::getInstance()->setStaticMetadata(
            const_cast<camera_metadata_t *>(info.static_camera_characteristics), i);

        int32_t roleId = CameraDevice::RoleIdRearWide;
        pData = VendorMetadataParser::getTag(staticMeta, MI_CAMERAID_ROLE_CAMERAID);
        if (pData) {
            roleId = *static_cast<int32_t *>(pData);
        }
        std::unique_ptr<CameraDevice> camera;

        int32_t mihalVtCam = sMihalVtCamera;
        if (!mihalVtCam) {
            camera = std::make_unique<VendorMappingCamera>(i, info, this);
        } else {
            if (roleId == CameraDevice::RoleIdRearVt || roleId == CameraDevice::RoleIdFrontVt) {
                camera = std::make_unique<VirtualCamera>(i, info);
            } else {
                camera = std::make_unique<VendorMappingCamera>(i, info, this);
            }
        }

        MLOGI(LogGroupHAL, "camera[%d] RoleId: %d", i, roleId);
        mCameras.push_back(std::move(camera));
    }

    return 0;
}

int VendorModule::getPhysicalCameraInfo(int physicalId, const Metadata **info)
{
    if (!mModuleImpl->get_physical_camera_info) {
        MLOGE(LogGroupHAL, "not supported for %d", physicalId);
        return -EINVAL;
    }

    int ret;
    if (!mPhysicalMetas.count(physicalId)) {
        camera_metadata *meta;
        ret = mModuleImpl->get_physical_camera_info(physicalId, &meta);
        if (ret) {
            MLOGE(LogGroupHAL, "failed to get physical camera info for %d: %d", physicalId, ret);
            return ret;
        }

        // we just steal the static metadata for query, so we must release the raw meta
        // in deconstructor
        mPhysicalMetas.emplace(std::make_pair(physicalId, meta));
    }

    *info = &mPhysicalMetas[physicalId];

    return 0;
}

int VendorModule::open(const char *id, camera3_device **device)
{
    return mModuleImpl->common.methods->open(&mModuleImpl->common, id,
                                             reinterpret_cast<hw_device_t **>(device));
}

int VendorModule::setCallbacks(const camera_module_callbacks *callbacks)
{
    if (!mModuleImpl->set_callbacks)
        return -EINVAL;

    return mModuleImpl->set_callbacks(callbacks);
}

void VendorModule::getVendorTagOps(vendor_tag_ops *ops)
{
    if (mModuleImpl->get_vendor_tag_ops)
        mModuleImpl->get_vendor_tag_ops(ops);
}

bool VendorModule::isSetTorchModeSupported() const
{
    if (mModuleImpl->set_torch_mode)
        return true;
    else
        return false;
}

int VendorModule::setTorchMode(const char *cameraId, bool enable)
{
    if (!mModuleImpl->set_torch_mode)
        return -EINVAL;

    return mModuleImpl->set_torch_mode(cameraId, enable);
}

int VendorModule::turnOnTorchWithStrengthLevel(const char *cameraId, int32_t torchStrength)
{
    if (!mModuleImpl->turn_on_torch_with_strength_level)
        return -EINVAL;

    return mModuleImpl->turn_on_torch_with_strength_level(cameraId, torchStrength);
}

int VendorModule::getTorchStrengthLevel(const char *cameraId, int32_t *strengthLevel)
{
    if (!mModuleImpl->get_torch_strength_level)
        return -EINVAL;

    return mModuleImpl->get_torch_strength_level(cameraId, strengthLevel);
}

int VendorModule::isStreamCombinationSupported(int cameraId,
                                               const camera_stream_combination *streams) const
{
    if (!mModuleImpl->is_stream_combination_supported)
        return -EINVAL;

    return mModuleImpl->is_stream_combination_supported(cameraId, streams);
}

void VendorModule::notifyDeviceStateChange(uint64_t deviceState)
{
    if (mModuleImpl->notify_device_state_change)
        mModuleImpl->notify_device_state_change(deviceState);
}

int VendorModule::getConcurrentStreamingCameraIds(uint32_t *pConcCamArrayLength,
                                                  concurrent_camera_combination **ppConcCamArray)
{
    if (!mModuleImpl->get_concurrent_streaming_camera_ids)
        return -EINVAL;

    return mModuleImpl->get_concurrent_streaming_camera_ids(pConcCamArrayLength, ppConcCamArray);
}

int VendorModule::isConcurrentStreamCombinationSupported(
    uint32_t numCombinations, const cameraid_stream_combination_t *pStreamComb)
{
    if (!mModuleImpl->is_concurrent_stream_combination_supported)
        return -EINVAL;

    return mModuleImpl->is_concurrent_stream_combination_supported(numCombinations, pStreamComb);
}

void VendorModule::dumpPerfInfo(int fd)
{
    if (mModuleImpl->reserved[0]) {
        typedef void (*PERFFUN)(int);
        PERFFUN dump_perf_info = (PERFFUN)mModuleImpl->reserved[0];
        dump_perf_info(fd);
    } else {
        MLOGE(LogGroupHAL, "no implement to dump perf info in vendor module!!!");
    }
}
} // namespace mihal
