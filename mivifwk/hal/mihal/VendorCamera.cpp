#include "VendorCamera.h"

#include <sys/mman.h>
#include <ui/GraphicBufferMapper.h>

#include "Camera3Plus.h"
#include "CameraManager.h"
#include "MiHal3BufferMgr.h"
#include "OfflineDebugDataUtils.h"
#include "Session.h"

using namespace android;

namespace {

void processVendorResult_(const camera3_callback_ops *ops, const camera3_capture_result *result)
{
    const VendorCamera::CallbackTransfer *callback =
        static_cast<const VendorCamera::CallbackTransfer *>(ops);

    if (!callback->pSessionCallbacks) {
        MLOGE(LogGroupHAL, "Camera: %s can't find session callback",
              callback->pVendorCamera->getId().c_str());
        return;
    }

    callback->pSessionCallbacks->process_capture_result(callback->pSessionCallbacks, result);
}

void processVendorNotify_(const camera3_callback_ops *ops, const camera3_notify_msg *msg)
{
    const VendorCamera::CallbackTransfer *callback =
        static_cast<const VendorCamera::CallbackTransfer *>(ops);

    if (!callback->pSessionCallbacks) {
        MLOGE(LogGroupHAL, "Camera: %s can't find session callback",
              callback->pVendorCamera->getId().c_str());
        return;
    }

    callback->pSessionCallbacks->notify(callback->pSessionCallbacks, msg);
}

camera3_buffer_request_status requestStreamBuffers_(
    const camera3_callback_ops *ops, uint32_t num_buffer_reqs,
    const camera3_buffer_request *buffer_reqs,
    /*out*/ uint32_t *num_returned_buf_reqs,
    /*out*/ camera3_stream_buffer_ret *returned_buf_reqs)
{
    const VendorCamera::CallbackTransfer *callback =
        static_cast<const VendorCamera::CallbackTransfer *>(ops);

    if (!callback->pSessionCallbacks) {
        MLOGE(LogGroupHAL, "Camera: %s can't find session callback",
              callback->pVendorCamera->getId().c_str());
        MASSERT(false);
    }

    return callback->pSessionCallbacks->request_stream_buffers(
        callback->pSessionCallbacks, num_buffer_reqs, buffer_reqs, num_returned_buf_reqs,
        returned_buf_reqs);
}

void returnStreamBuffers_(const camera3_callback_ops *ops, uint32_t num_buffers,
                          const camera3_stream_buffer *const *buffers)
{
    const VendorCamera::CallbackTransfer *callback =
        static_cast<const VendorCamera::CallbackTransfer *>(ops);

    if (!callback->pSessionCallbacks) {
        MLOGE(LogGroupHAL, "Camera: %s can't find session callback",
              callback->pVendorCamera->getId().c_str());
        MASSERT(false);
    }

    callback->pSessionCallbacks->return_stream_buffers(callback->pSessionCallbacks, num_buffers,
                                                       buffers);
}

} // namespace

namespace mihal {

VendorCamera::VendorCamera(int cameraId, const camera_info &info, CameraModule *module)
    : CameraDevice{std::to_string(cameraId)},
      mRawInfo{info},
      mOverrideInfo{std::make_unique<Metadata>()},
      mDeviceImpl{nullptr},
      mModule{module},
      mSession{nullptr},
      mCallbackTransfer{
          {processVendorResult_, processVendorNotify_, requestStreamBuffers_, returnStreamBuffers_},
          nullptr,
          this},
      mFwkHal3BufMgr{std::make_shared<MiHal3BufferMgr>(this)}
{
    // TODO: override
    // clone and overriding the static meta here in need
    *mOverrideInfo = mRawInfo.static_camera_characteristics;

    mHal3Device.common.version = mRawInfo.device_version;

    mFlushed.store(false);
}

int VendorCamera::getCameraInfo(camera_info *info) const
{
    if (!info)
        return -EINVAL;

    *info = mRawInfo;
    info->static_camera_characteristics = mOverrideInfo->peek();

    return 0;
}

int VendorCamera::setTorchMode(bool enabled)
{
    return mModule->setTorchMode(mId.c_str(), enabled);
}

int VendorCamera::turnOnTorchWithStrengthLevel(int32_t torchStrength)
{
    return mModule->turnOnTorchWithStrengthLevel(mId.c_str(), torchStrength);
}

int VendorCamera::getTorchStrengthLevel(int32_t *strengthLevel)
{
    return mModule->getTorchStrengthLevel(mId.c_str(), strengthLevel);
}

int VendorCamera::open()
{
    std::lock_guard<std::mutex> lg(mOpsLock);

    MLOGI(LogGroupHAL, "[MiHalOp]%s[open]: Begin open", getLogPrefix().c_str());

    int err = mModule->open(mId.c_str(), &mDeviceImpl);
    if (err || !mDeviceImpl) {
        MLOGE(LogGroupHAL, "[MiHalOp]%s[open]: open failed, err=%d", getLogPrefix().c_str(), err);
        return err;
    }

    if (!mDeviceImpl->common.close) {
        MLOGE(LogGroupHAL, "[MiHalOp]%s[open]: function close() of impl could not be null",
              getLogPrefix().c_str());
        return -EFAULT;
    }
    MLOGI(LogGroupHAL, "[MiHalOp]%s[open]: End open", getLogPrefix().c_str());

    // NOTE: Offline DebugData start
    if (getRoleId() == RoleIdRearVt || getRoleId() == RoleIdFrontVt) {
        SetVTStatus(true);
    }
    // NOTE: Offline DebugData end

    return 0;
}

int VendorCamera::close()
{
    std::lock_guard<std::mutex> lg(mOpsLock);

    if (!mFlushed.load()) {
        MLOGI(LogGroupHAL, "[MiHalOp]%s[close]: flush before close", getLogPrefix().c_str());
        flush();
    }

    int err;

    MLOGI(LogGroupHAL, "[MiHalOp]%s[close]: Begin close", getLogPrefix().c_str());
    // NOTE: Offline DebugData start
    if (getRoleId() == RoleIdRearVt || getRoleId() == RoleIdFrontVt) {
        SetVTStatus(false);
    }
    // NOTE: Offline DebugData end

    // Note: Some rare calling flow scene as open->initialize->close,
    // but we call vendor's initialize in the creating session stage.
    // in vendor's initialize reorganize camera3 device ops. if not executed,
    // when calling vendor's close will fail.so here add a hysteresis initialize.
    if (!mSession) {
        err = mDeviceImpl->ops->initialize(mDeviceImpl, mRawCallbacks2Fwk);
        if (err)
            MLOGE(LogGroupHAL, "[MiHalOp]%s[close]: failed to initialize, err=%d",
                  getLogPrefix().c_str(), err);
    }

    err = mDeviceImpl->common.close(&mDeviceImpl->common);
    if (err)
        MLOGE(LogGroupHAL, "[MiHalOp]%s[close]: failed to closed, err=%d", getLogPrefix().c_str(),
              err);

    if (mSession) {
        if (mSession->getSessionType() == Session::AsyncAlgoSessionType)
            SessionManager::getInstance()->unregisterSession(
                std::static_pointer_cast<AsyncAlgoSession>(mSession));
        mSession = nullptr;
    }
    mDeviceImpl = nullptr;
    MLOGI(LogGroupHAL, "[MiHalOp]%s[close]: End close", getLogPrefix().c_str());

    return err;
}

bool VendorCamera::isAsyncAlgoStreamConfigMode(const StreamConfig *config)
{
    uint32_t opMode = config->getOpMode();
    const Metadata *meta = config->getMeta();
    auto entry = meta->find(MI_SESSION_PARA_CLIENT_NAME);
    if (entry.count > 0) {
        std::string apkName = reinterpret_cast<const char *>(entry.data.u8);
        if (apkName != "com.android.camera") {
            return false;
        }
    }

    if (((opMode >= StreamConfigModeAlgoDual) && (opMode <= StreamConfigModeAlgoQCFAMFNRFront)) ||
        (opMode == StreamConfigModeSAT) || (opMode == StreamConfigModeBokeh) ||
        (opMode == StreamConfigModeMiuiManual) || (opMode == StreamConfigModeMiuiZslFront) ||
        (opMode == StreamConfigModeMiuiQcfaFront) || (opMode == StreamConfigModeMiuiSuperNight))
        return true;

    return false;
}

bool VendorCamera::isSyncAlgoStreamConfigMode(StreamConfig *config,
                                              std::shared_ptr<EcologyAdapter> &ecoAdapter)
{
    const Metadata *meta = config->getMeta();
    auto entry = meta->find(SDK_SESSION_PARAMS_OPERATION);
    if (entry.count == 0 || (entry.data.i32[0] & 0xff00) != 0xff00) {
        MLOGI(LogGroupHAL, "[Ecology] there is no SESSION_PARAMS_OPERATION,it is not sdk scene");
        return false;
    }

    int32_t roleId = getRoleId();
    // it will parse the cloud info,platform info and session parameters of configuration
    // during create the EcologyAdapter
    ecoAdapter = std::make_shared<EcologyAdapter>(roleId, config);

    // This function will judge whether the sdk is enabled,if it is,replace opMode of config
    ecoAdapter->buildConfiguration(roleId, config);

    if (ecoAdapter->isNeedReprocess()) {
        if (ecoAdapter->isVideo()) {
            MLOGI(LogGroupHAL, "[Ecology] start video mode");
        } else {
            MLOGI(LogGroupHAL, "[Ecology] start snapshot mode");
            return true;
        }
    } else {
        MLOGI(LogGroupHAL, "[Ecology] not meet requirement,do not need reprocess");
        ecoAdapter = nullptr;
    }

    return false;
}

// NOTE:TODO: now temporarily create a simple session with 2 streams: preview + snapshot
int VendorCamera::createSession(std::shared_ptr<StreamConfig> config)
{
    int ret = 0;

    if (isAsyncAlgoStreamConfigMode(config.get())) {
        mSession = AsyncAlgoSession::create(this, config);
    } else {
        std::shared_ptr<EcologyAdapter> ecoAdapter = nullptr;
        if (isSyncAlgoStreamConfigMode(config.get(), ecoAdapter)) {
            MASSERT(ecoAdapter);
            mSession = std::make_shared<SyncAlgoSession>(this, config, ecoAdapter);
        } else {
            mSession = std::make_shared<NonAlgoSession>(this, config, ecoAdapter);
        }
    }

    if (!mSession->inited()) {
        mSession = nullptr;
        ret = -EINVAL;
    }

    return ret;
}

int VendorCamera::prepareReConfig(uint32_t &oldSessionFrameNum)
{
    MLOGI(LogGroupHAL, "Old session is: %s", mSession->getLogPrefix().c_str());

    switch (mSession->getSessionType()) {
    case Session::AsyncAlgoSessionType:
        // NOTE: Reclaim resources related to fwk
        mSession->resetResource();

        // WARNING: we shouldn't delete old session here because Qcom HAL maybe still using some old
        // session's resources (e.g. Qcom will use mihal stream to asynchronously create offline
        // feature), we can only delete old session when we can make sure that vendor HAL will no
        // longer use old session's resource
        // NOTE: actually we can do a flush() here and then Qcom will not touch old session's data
        // any more, but that will cost 20ms, it's too time consuming
        mLastSession = mSession;
        break;

    case Session::SyncAlgoSessionType:
        mSession->flush();
        break;

    case Session::NonAlgoSessionType:
        break;

    default:
        break;
    }

    oldSessionFrameNum = mSession->getSessionFrameNum();
    mSession = nullptr;
    return 0;
}

int VendorCamera::setupCallback2Vendor()
{
    mCallbackTransfer.pSessionCallbacks = mSession->getSessionCallback();
    int ret = initialize2Vendor(&mCallbackTransfer);
    if (ret) {
        MLOGE(LogGroupHAL, "failed to set callback to vendor:%d", ret);
    }
    return ret;
}

int VendorCamera::updateCallback2Vendor()
{
    mCallbackTransfer.pSessionCallbacks = mSession->getSessionCallback();
    return 0;
}

int VendorCamera::updateSessionData(uint32_t oldSessionFrameNum)
{
    MLOGI(LogGroupHAL, "pass old session frame num:%u to new session", oldSessionFrameNum);
    mSession->updateSessionFrameNum(oldSessionFrameNum);
    return 0;
}

int VendorCamera::processConfig()
{
    int ret = configureStreams2Vendor(mSession->getConfig2Vendor());
    if (ret) {
        MLOGE(LogGroupHAL, "config failed:%d", ret);
        return ret;
    }

    mSession->endConfig();
    MLOGI(LogGroupHAL, "config stream OK");

    mFlushed.store(false);

    return 0;
}

int VendorCamera::configureStreams(std::shared_ptr<StreamConfig> config)
{
    std::lock_guard<std::mutex> lg(mOpsLock);

    MLLOGI(LogGroupHAL, config->toString(),
           "[MiHalOp]%s[Config]: fwk configure_streams:", getLogPrefix().c_str());

    bool isReconfig = false;
    // NOTE: for reconfig, we need to update frame num of newly created session
    uint32_t oldSessionFrameNum = 0;

    if (mSession) {
        isReconfig = true;
        MLOGI(LogGroupHAL, "[MiHalOp]%s[Config]: This is reconfig!", getLogPrefix().c_str());
        prepareReConfig(oldSessionFrameNum);
    }

    int ret = createSession(config);
    if (ret) {
        MLOGE(LogGroupHAL, "%s[Config]: failed to create session", getLogPrefix().c_str());
        return ret;
    }
    MLOGI(LogGroupHAL, "%s[Config][KEY]: create session:%s", getLogPrefix().c_str(),
          mSession->getLogPrefix().c_str());

    // NOTE: update callbacks
    if (!isReconfig) {
        // NOTE: first time configure stream we need to send callback to vendor
        MLOGI(LogGroupHAL, "%s[Config]: first config, setup vendor callback",
              getLogPrefix().c_str());
        setupCallback2Vendor();
    } else {
        // NOTE: for reconfig case, we need to update session callback to newly created session
        // XXX: assumption: before reconfig, app must send a flush command, and Qcom mustn't send
        // result during reconfig, otherwise, the Qcom result will send to newly created session.
        MLOGI(LogGroupHAL, "%s[Config]: reconfig, update callback to new session",
              getLogPrefix().c_str());
        updateCallback2Vendor();
    }

    // NOTE: for reconfig, we need to pass some old session data to new one
    if (isReconfig) {
        MLOGI(LogGroupHAL, "%s[Config]: reconfig, update new session data", getLogPrefix().c_str());
        updateSessionData(oldSessionFrameNum);
    }

    MLOGI(LogGroupHAL, "%s[Config]: set config info to vendor", getLogPrefix().c_str());
    auto configResult = processConfig();

    // NOTE: record new stream info into buffer manager
    // NOTE: fwk buffer manager should configure streams after we finish configure vendor because
    // fwk buffer manager need to use camera3_stream's max_buffers info which is set by Qcom and
    // mihal
    if (configResult == 0 && mSession->getSessionType() != Session::NonAlgoSessionType) {
        mFwkHal3BufMgr->configureStreams(config->toRaw());
    }

    // NOTE: After we finish configuring new session to vendor, we can make sure Qcom HAL will no
    // longer use old session's resource, so we can delete old session now
    if (mLastSession) {
        SessionManager::getInstance()->unregisterSession(
            std::static_pointer_cast<AsyncAlgoSession>(mLastSession));

        mLastSession = nullptr;
    }

    if (mSession->getSessionType() == Session::AsyncAlgoSessionType) {
        SessionManager::getInstance()->registerSession(
            std::static_pointer_cast<AsyncAlgoSession>(mSession));
    }

    MLOGI(LogGroupHAL, "[MiHalOp][Config] End config result %d", configResult);

    return configResult;
}

const camera_metadata *VendorCamera::defaultSettings(int type) const
{
    MLOGI(LogGroupHAL, "get default setting for camera %s", getId().c_str());

    // TODO: could default request settings exposed to FW be more simple? then we could
    // construct the real default request setting just in mihal
    return mDeviceImpl->ops->construct_default_request_settings(mDeviceImpl, type);
}

int VendorCamera::processCaptureRequest(std::shared_ptr<CaptureRequest> request)
{
    // NOTE: for NonAlgoSessionType, we bypass everything
    if (mSession->getSessionType() != Session::NonAlgoSessionType) {
        mFwkHal3BufMgr->processCaptureRequest(request->toRaw());
    }

    return mSession->processRequest(std::move(request));
}

void VendorCamera::processCaptureResult(const CaptureResult *result)
{
    MLOGV(LogGroupRT, "%s: result:\n\t%s", getLogPrefix().c_str(), result->toString().c_str());

    // NOTE: we should firstly let HAL3 buf mgr process result, otherwise, the buffer in the capture
    // result will be released by app after process_capture_result and then if hal3 buf mgr
    // processed it, a crash would occur
    // NOTE: for NonAlgoSessionType, we bypass everything
    if (mSession->getSessionType() != Session::NonAlgoSessionType) {
        mFwkHal3BufMgr->processCaptureResult(result->toRaw());
    }

    if (mRawCallbacks2Fwk) {
        mRawCallbacks2Fwk->process_capture_result(mRawCallbacks2Fwk, result->toRaw());
    }
}

void VendorCamera::dump(int fd)
{
    mDeviceImpl->ops->dump(mDeviceImpl, fd);
}

void VendorCamera::flushRemainingInternalStreams()
{
    LocalConfig *configToVednor = static_cast<LocalConfig *>(mSession->getConfig2Vendor());
    std::vector<shared_ptr<Stream>> streams = configToVednor->getStreams();
    std::vector<camera3_stream_t *> remainingStreams;
    for (auto stream : streams) {
        if (stream->getRequestedBuffersSzie()) {
            remainingStreams.push_back(stream->toRaw());
        }
    }
    uint32_t size = remainingStreams.size();
    if (size) {
        signalStreamFlush(size, remainingStreams.data());
        MLOGW(LogGroupHAL, "%s[FlushRemain]: return remaining requested buffers",
              getLogPrefix().c_str());
    }
}

int VendorCamera::flush()
{
    MLOGI(LogGroupHAL, "[MiHalOp]%s[Flush]: Begin flush [Promise]: End flush",
          getLogPrefix().c_str());

    int ret;
    if (mSession) {
        ret = mSession->flush();
        // NOTE: we only care about algo session, for non-algo session, everything is bypassed
        if (mSession->getSessionType() != Session::NonAlgoSessionType) {
            // NOTE: drain all buffers, normally, this function should be invoked in
            // signalStreamFlush, but occasionally, camera service will not call signalStreamFlush
            // and that leaves some buffers remaining in mhal and hence buffer leackage.
            MLOGI(LogGroupHAL,
                  "%s[Flush][Hal3BufferMgr]: drain HAL3 buffer manager [Promise]: End drain",
                  getLogPrefix().c_str());
            mFwkHal3BufMgr->waitAndResetToInitStatus();
            MLOGI(LogGroupHAL, "%s[Flush][Hal3BufferMgr]: [Fulfill] End drain HAL3 buffer manager",
                  getLogPrefix().c_str());
            // NOTE: if requested buffers for internal streams are detained in QCom after
            // vendorCamera flush, it needs to signalStreamFlush.
            flushRemainingInternalStreams();
        }
    } else {
        // NOTE: the calling procedure of "open() / initialize() / flush()" is also permitted
        ret = flush2Vendor();
    }

    mFlushed.store(true);

    MLOGI(LogGroupHAL, "[MiHalOp]%s[Flush]: [Fulfill]: End flush", getLogPrefix().c_str());
    return ret;
}

void VendorCamera::signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams)
{
    std::lock_guard<std::mutex> lg(mOpsLock);
    MLOGI(LogGroupHAL, "[MiHalOp]%s begin", getLogPrefix().c_str());

    if (mSession)
        mSession->signalStreamFlush(numStream, streams);
    else
        // NOTE: the calling procedure of "open() / initialize() / flush()" is also permitted
        signalStreamFlush2Vendor(numStream, streams);

    MLOGI(LogGroupHAL, "[MiHalOp] end");
}

int VendorCamera::initialize2Vendor(const camera3_callback_ops *callbacks)
{
    return mDeviceImpl->ops->initialize(mDeviceImpl, callbacks);
}

int VendorCamera::configureStreams2Vendor(StreamConfig *config)
{
    return mDeviceImpl->ops->configure_streams(mDeviceImpl, config->toRaw());
}

int VendorCamera::submitRequest2Vendor(CaptureRequest *request)
{
    if (!mSession->mFlush) {
        return mDeviceImpl->ops->process_capture_request(
            mDeviceImpl, const_cast<camera3_capture_request *>(request->toRaw()));
    }
    auto algosession = std::static_pointer_cast<AlgoSession>(mSession);
    algosession->removeFromVendorInflights(request->getFrameNumber());
    MLOGW(LogGroupHAL, "[Flush][Statistic]:in flushing , do not send request to vendor!");
    return -ENOENT;
}

int VendorCamera::flush2Vendor()
{
    return mDeviceImpl->ops->flush(mDeviceImpl);
}

void VendorCamera::signalStreamFlush2Vendor(uint32_t numStream,
                                            const camera3_stream_t *const *streams)
{
    if (NULL != mDeviceImpl && NULL != mDeviceImpl->ops) {
        return mDeviceImpl->ops->signal_stream_flush(mDeviceImpl, numStream, streams);
    } else {
        MLOGW(LogGroupHAL,
              "[Flush] warning:signalStreamFlush2Vendor after close,now mDeviceImpl is null "
              "pointer!!!");
        return;
    }
}

} // namespace mihal
