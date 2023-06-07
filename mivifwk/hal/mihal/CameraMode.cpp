#include "CameraMode.h"

#include <sys/prctl.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>

#include "AppSnapshotController.h"
#include "BokehPhotographer.h"
#include "BurstPhotographer.h"
#include "FusionPhotographer.h"
#include "HdrPhotographer.h"
#include "HwmfPhotographer.h"
#include "LogUtil.h"
#include "Metadata.h"
#include "MihalMonitor.h"
#include "Photographer.h"
#include "ProfessionalPhotographer.h"
#include "Reprocess.h"
#include "Session.h"
#include "SrPhotographer.h"
#include "SuperHDPhotographer.h"
#include "SupernightPhotographer.h"

namespace mihal {

using namespace std::chrono_literals;
using namespace vendor::xiaomi::hardware::bgservice::implementation;

#define MAX_NO_GAUSSIAN_TIME 60 * 1000 * 1000
#define SUPERMOON            35

// NOTE: check if two stream size is close to each other
bool isSizeSimilar(int width1, int height1, int width2, int height2)
{
    static int sDeviation = 0;
    if (sDeviation == 0) {
        MiviSettings::getVal("streamSizeDifferenceThreshold", sDeviation, 50);
    }

    return (std::abs(width1 - width2) <= sDeviation) && (std::abs(height1 - height2) <= sDeviation);
};

bool ImageSize::isSameRatio(float ratio, int matchedDecimalNumbers) const
{
    auto double_round = [](float src, int bits) {
        std::stringstream stream;
        float result;
        stream << std::fixed << std::setprecision(bits) << src;
        stream >> result;
        return result;
    };
    float thisRatio = double_round(float(width) / height, matchedDecimalNumbers);
    ratio = double_round(ratio, matchedDecimalNumbers);
    return std::fabs(thisRatio - ratio) < std::numeric_limits<float>::epsilon();
}

std::string HdrStatus::toString() const
{
    std::ostringstream str;

    str << "hdrDetected=" << hdrDetected << ", EV:";

    for (int32_t ev : expVaules) {
        str << " " << ev;
    }

    return str.str();
}

// The tag listed here must be completely set by mihal
std::map<const char * /* TAG */, int /* Type */> deaultTagStrings = {
    {MI_BOKEH_HDR_ENABLE, TYPE_BYTE},     {MI_SUPER_RESOLUTION_ENALBE, TYPE_BYTE},
    {MI_CAPTURE_FUSION_IS_ON, TYPE_BYTE}, {MI_HDR_SR_ENABLE, TYPE_BYTE},
    {MI_MIVI_RAW_ZSL_ENABLE, TYPE_BYTE},  {MI_BURST_TOTAL_NUM, TYPE_INT32},
    {MI_BURST_CURRENT_INDEX, TYPE_INT32}, {MI_HDR_ENABLE, TYPE_BYTE}};

// The tag listed here must be completely set by mihal for 3rdAPP use MIUI
std::map<const char * /* TAG */, int /* Type */> deaultTagStringsfor3rd = {
    {MI_BOKEH_HDR_ENABLE, TYPE_BYTE},
    {MI_SUPER_RESOLUTION_ENALBE, TYPE_BYTE},
    {MI_SUPERNIGHT_ENABLED, TYPE_BYTE},
    {MI_MIVI_SUPERNIGHT_MODE, TYPE_BYTE},
    {MI_MIVI_NIGHT_MOTION_MODE, TYPE_INT32},
    {MI_CAPTURE_FUSION_IS_ON, TYPE_BYTE},
    {MI_HDR_SR_ENABLE, TYPE_BYTE},
    {MI_MIVI_RAW_ZSL_ENABLE, TYPE_BYTE},
    {MI_BURST_TOTAL_NUM, TYPE_INT32},
    {MI_BURST_CURRENT_INDEX, TYPE_INT32},
    {MI_HDR_ENABLE, TYPE_BYTE}};

CameraMode::CameraMode(AlgoSession *session, CameraDevice *vendorCamera,
                       const StreamConfig *configFromFwk)
    : mSession{session},
      mVendorCamera{vendorCamera},
      mConfigFromFwk{std::make_unique<LocalConfig>(*configFromFwk)},
      mExitThread{false},
      mModeType{MODE_CAPTURE},
      mBufferCapacity{0},
      mIsRetired{false},
      mHdrMode{0},
      mMasterId{0},
      mBokehMDMode{0},
      mBokehMadridSupported{false},
      mBokehRoleId{CameraDevice::RoleIdTypeNone},
      mFlashData{ANDROID_FLASH_MODE_OFF, ANDROID_FLASH_STATE_UNAVAILABLE},
      mIsFusionSupported{false},
      mIsFusionReady{false},
      mHdrSrDetected{false},
      mUIZoomRatio{1.0f},
      mFovCropZoomRatio{1.0f},
      mLlsNeed{false},
      mMaxQuickShotCount{3},
      mInternalQuickviewBufferSize{0},
      mBufferLimitCnt{1},
      mBufferDefaultLimitCnt{1},
      mShortGain{0.0f},
      mLuxIndex{0.0f},
      mAiAsdSceneApplied{0},
      mThermalLevel{0},
      mIsBurstCapture{false},
      mWaitFlushMillisecs{4000},
      mIsHdsrZslSupported{0},
      mBokehMasterId{0},
      mBokehSlaveId{0},
      mIsVendorLowCaps{false},
      mPureSnapshot{false},
      mIsSupportFovCrop{false}
{
    if (Session::AsyncAlgoSessionType == session->getSessionType()) {
        mThread = std::thread{[this]() { loop(); }};

        // TODO: This logical will be changed later. Now we just keep it for transition stage.
        switch (mConfigFromFwk->getOpMode()) {
        case StreamConfigModeAlgoDualSAT:
            mModeType = MODE_CAPTURE;
            break;
        case mihal::StreamConfigModeAlgoDual:
        case mihal::StreamConfigModeAlgoSingleRTB:
            mModeType = MODE_PORTRAIT;
            break;
        case mihal::StreamConfigModeMiuiSuperNight:
            mModeType = MODE_SUPER_NIGHT;
            break;
        case mihal::StreamConfigModeAlgoQCFAMFNR:
        case mihal::StreamConfigModeAlgoManualSuperHD:
            mModeType = MODE_SUPER_HD;
            break;
        case mihal::StreamConfigModeAlgoManual:
            mModeType = MODE_PROFESSIONAL;
            break;
        case mihal::StreamConfigModeQCFALite:
            mModeType = MODE_CAPTURE;
            break;
        default:
            mModeType = MODE_CAPTURE;
            break;
        }
    }
    memset(&mSuperHdData, 0, sizeof(SuperHdData));

    MiviSettings::getVal("FwkStreamMaxBufferQueueSize", mFwkStreamMaxBufferQueueSize, 8);

    if (isSat()) {
        for (auto camera : mVendorCamera->getPhyCameras()) {
            if (camera->getRoleId() == CameraDevice::RoleIdRearWide) {
                mMasterId.store(atoi(camera->getId().c_str()));
                break;
            }
        }
    }

    mCameraRoleConfigInfoMap = MiviSettings::getCameraRoleConfigInfoFromJson();
    uint32_t currentCameraRoleId = mVendorCamera->getRoleId();
    auto it = mCameraRoleConfigInfoMap.find(currentCameraRoleId);
    if (it != mCameraRoleConfigInfoMap.end()) {
        auto configInfo = it->second;
        mFovCropZoomRatio = configInfo.fovCropZoomRatio;
        MLOGI(LogGroupHAL, "Camera %s mFovCropZoomRatio %0.2fX", mVendorCamera->getId().c_str(),
              mFovCropZoomRatio);
    }
    if (isMultiCameraCase()) {
        PsiMemMonitor::getInstance()->psiMonitorTrigger();
    }
}

CameraMode::~CameraMode()
{
    for (const auto &stream : mConfigToVendor->getStreams()) {
        if (stream->isInternal() && !stream->isQuickview()) {
            stream->unregisterListener();
        }
    }

    setRetired();

    PsiMemMonitor::getInstance()->snapshotCountReclaim();

    if (mThread.joinable()) {
        std::unique_lock<std::mutex> locker(mMutex);
        mExitThread = true;
        locker.unlock();

        mCond.notify_one();
        mThread.join();
        MLOGI(LogGroupHAL, "camera %s closed with thread exit", mVendorCamera->getId().c_str());
    }
}

CameraUser CameraMode::getCameraUser()
{
    if (Session::SyncAlgoSessionType == mSession->getSessionType()) {
        return CameraUser::SDK;
    } else {
        return CameraUser::MIUI;
    }
}

void CameraMode::setRetired()
{
    if (mIsRetired) {
        return;
    }

    mIsRetired.store(true);
    PsiMemMonitor::getInstance()->psiMonitorClose();
    AppSnapshotController::getInstance()->onRecordDelete(this);
}

void CameraMode::addToMockInvitations(std::shared_ptr<CaptureRequest> fwkRequest,
                                      const std::vector<std::shared_ptr<Stream>> &streams)
{
    auto frame = fwkRequest->getFrameNumber();
    auto invitation = std::make_shared<ResultData>();

    invitation->cameraId = atoi(mVendorCamera->getId().c_str());
    invitation->frameNumber = frame;
    invitation->sessionId = mSession->getId();
    invitation->timeStampUs = 0;

    invitation->clientId = mSession->mClientId;
    invitation->isSDK = (mSession->getSessionType() == Session::SyncAlgoSessionType);

    for (const auto &stream : streams) {
        ImageFormat format = {stream->getFormat(), (int32_t)stream->getWidth(),
                              (int32_t)stream->getHeight()};

        invitation->imageForamts.push_back(format);
        MLOGI(LogGroupHAL,
              "[MockInvitations][GreenPic]: invited mock buffers size:%uX%u, format: 0x%x",
              format.width, format.height, format.format);
    }

    std::lock_guard<std::mutex> locker{mMapMutex};
    mMockInvitations.insert({frame, invitation});
}

bool CameraMode::isQuickShot(std::shared_ptr<CaptureRequest> fwkRequest)
{
    const Metadata *meta = fwkRequest->getMeta();
    auto entry = meta->find(MI_QUICKSHOT_ISQUICKSNAPSHOT);
    if (entry.count) {
        return entry.data.u8[0];
    }
    return false;
}

bool CameraMode::isMarcoMode(std::shared_ptr<CaptureRequest> fwkRequest) const
{
    const Metadata *meta = fwkRequest->getMeta();
    auto entry = meta->find(MI_MACROMODE_ENABLED);
    if (entry.count) {
        return entry.data.u8[0];
    }
    return false;
}

std::shared_ptr<ResultData> CameraMode::inviteForMockRequest(int32_t frame)
{
    std::lock_guard<std::mutex> locker{mMapMutex};
    auto it = mMockInvitations.find(frame);
    if (it == mMockInvitations.end()) {
        MLOGW(LogGroupHAL, "[Mock][inviteRequest]: fwkFrame:%u not found, maybe already invited",
              frame);
        return nullptr;
    }

    auto result = it->second;

    mMockInvitations.erase(frame);

    return result;
}

void CameraMode::initCapabilities()
{
    initMfnrCapabilities();

    if (isFeatureSrSupported())
        initSrCapabilities();

    if (isFeatureSuperNightSupported())
        initSupernightCapabilities();

    if (isFeatureFusionSupported())
        initFusionCapabilities();

    if (isDualBokeh())
        initMadridBokehCapabilities();
}

void CameraMode::updateSettings(CaptureRequest *request)
{
    updateZoomRatioSettings(request);

    updateFlashSettings(request);

    if (isFeatureBurstSupported())
        updateBurstSettings(request);

    if (isFeatureHdrSupported())
        updateHdrSettings(request);

    if (request->isSnapshot() && isFeatureSuperNightSupported())
        updateSuperNightSettings(request);

    if (request->isSnapshot())
        // Now used for sn & Asd exifInfo, limit condition should same as updataASDExifInfo
        updateUIRelatedSettins(request);
}

void CameraMode::updateStatus(CaptureResult *result)
{
    // NOTE: in preprocess, some mialgo plugin may need preview meta
    updatePreProcessRelatedMeta(result);

    update3AStatus(result);

    updateThermalStatus(result);

    updateFlashStatus(result);

    if (isMultiCameraCase()) {
        updateMasterId(result);
        updateMotionStatus(result);
    }

    if (isFeatureSuperNightSupported())
        updateSuperNightStatus(result);

    if (isFeatureHdrSupported() && mHdrMode.load()) {
        updateHdrStatus(result);
        if (isFeatureHdrSrSupported())
            updateHdrSrStatus(result);
    }

    if (isFeatureFusionSupported())
        updateFusionStatus(result);

    updataASDExifInfo(result);
}

void CameraMode::updateSupportFovCrop(std::vector<std::shared_ptr<Stream>> &streams)
{
    auto &cameraRoleIdSets = mVendorInternalStreamInfoMap.supportFovCropCameras;
    if (cameraRoleIdSets.find(mVendorCamera->getRoleId()) != cameraRoleIdSets.end()) {
        mIsSupportFovCrop = true;

        if (std::fabs(mFovCropZoomRatio - 1.0) < std::numeric_limits<float>::epsilon())
            mIsSupportFovCrop = false;
        else if (mConfigFromFwk->getOpMode() == StreamConfigModeAlgoManual &&
                 mConfigFromFwk->getStreamNums() == 3) {
            for (auto iter : streams) {
                if (iter->toRaw()->format == HAL_PIXEL_FORMAT_RAW16) {
                    // Manual Raw not need FovCrop
                    mIsSupportFovCrop = false;
                    break;
                }
            }
        }
    }
}

// NOTE: Refer to this doc:
// https://xiaomi.f.mioffice.cn/docs/dock4nEZ6c82dQGPVWmEarfE75O
InternalStreamInfoMap CameraMode::buildFinalInternalStreamInfoMap()
{
    std::vector<InternalStreamInfo> finalStreamInfos;
    InternalStreamInfoMap map;
    getVendorStreamInfoMap(map);

    std::vector<CameraDevice *> phyCameras;
    if (isMultiCameraCase()) {
        phyCameras = mVendorCamera->getPhyCameras();
    }

    for (auto &info : map.streamInfos) {
        char *cameraIdTag = nullptr;
        char *optimalSizeTag = nullptr;

        // NOTE: This code block is temporarily used to prune useless streams.
        {
            bool isFeatureMihalMfnrSupported = map.featureMask & FeatureMask::FeatureMihalMfnr;
            if (StreamConfigModeAlgoDualSAT == map.vendorOpMode && !isFeatureMihalMfnrSupported &&
                info.format == HAL_PIXEL_FORMAT_RAW10) {
                continue;
            }

            bool isFeatureFakeSatSupported = map.featureMask & FeatureMask::FeatureFakeSat;
            if (!isFeatureFakeSatSupported && info.isFakeSat) {
                continue;
            }
        }

        if (map.vendorOpMode == StreamConfigModeAlgoDual) {
            // NOTE: we will get camera id from these exclusive tags for dual bokeh
            std::tie(cameraIdTag, optimalSizeTag) = getDualBokehInfo(info.bokehSig);
            info.cameraId = getCameraPhysicalIdByTag(cameraIdTag);

            if (info.bokehSig == BokehCamSig::masterYuv) {
                mBokehMasterId = info.cameraId;
                mMasterId.store(mBokehMasterId);
            } else if (info.bokehSig == BokehCamSig::slaveYuv) {
                mBokehSlaveId = info.cameraId;
            }
        } else if (phyCameras.size() && info.isPhysicalStream && (info.roleId != 0xFF)) {
            for (auto camera : phyCameras) {
                uint32_t roleId = camera->getRoleId();
                if (roleId == info.roleId) {
                    info.cameraId = static_cast<uint32_t>(atoi(camera->getId().c_str()));
                    break;
                }
            }
        } else {
            info.cameraId = static_cast<uint32_t>(atoi(mVendorCamera->getId().c_str()));
        }

        if (std::fabs(info.ratio - 0.0f) < std::numeric_limits<float>::epsilon()) {
            info.ratio = mFrameRatio;
        }

        convertAspectRatioToActualResolution(info, map.featureMask, optimalSizeTag);
        finalStreamInfos.push_back(info);
    }

    if (getCameraUser() == CameraUser::SDK) {
        map.mialgoOpMode = getSDKMialgoOpMode();
    }
    map.streamInfos = finalStreamInfos;

    MLOGI(LogGroupHAL, "size of stream info map : %d", map.streamInfos.size());
    return map;
}

void CameraMode::buildInternalSnapshotStreamOverride(std::vector<std::shared_ptr<Stream>> &streams)
{
    mVendorInternalStreamInfoMap = buildFinalInternalStreamInfoMap();
    for (auto &info : mVendorInternalStreamInfoMap.streamInfos) {
        auto internalStream = createInternalStream(info);
        internalStream->setStreamId(streams.size());
        streams.push_back(internalStream);
    }

    mBufferLimitCnt = mBufferDefaultLimitCnt = mVendorInternalStreamInfoMap.bufferLimitCnt;
    mBufferCapacity = mVendorInternalStreamInfoMap.snapshotBufferQueueSize;
    mWaitFlushMillisecs = std::chrono::milliseconds{mVendorInternalStreamInfoMap.flushWaitTime};
    mIsVendorLowCaps = mVendorInternalStreamInfoMap.isVendorLowCaps;

    updateSupportFovCrop(streams);

    MLOGI(LogGroupHAL,
          "opMode 0x%x %s build stream done, size %d, mBufferLimitCnt %d, "
          "mBufferCapacity %d, mWaitFlushMillisecs %d, mIsVendorLowCaps %d, mIsSupportFovCrop %d",
          mVendorInternalStreamInfoMap.vendorOpMode, mVendorInternalStreamInfoMap.signature.c_str(),
          streams.size(), mBufferLimitCnt, mBufferCapacity, mWaitFlushMillisecs, mIsVendorLowCaps,
          mIsSupportFovCrop);
}

void CameraMode::buildSessionParameterOverride()
{
    bool isMiviRawCbSupported = false;
    MiviSettings::getVal("supportMiviFullSceneRawCB", isMiviRawCbSupported, false);
    if (isSat() && isMiviRawCbSupported) {
        uint8_t isMiviFullSceneRawCb = 1;
        mSessionParameter.update(MI_MIVI_FULL_SCENE_RAW_CB_SUPPORTED, &isMiviFullSceneRawCb, 1);
    }
}

LocalConfig *CameraMode::buildConfigToVendor()
{
    MLLOGI(LogGroupHAL, mConfigFromFwk->toString(),
           "camera %s: dump config from framework:", mVendorCamera->getId().c_str());

    std::string dump = mConfigFromFwk->getMeta()->toString();
    MLLOGV(LogGroupHAL, dump, "camera %s: dump session parameter:", mVendorCamera->getId().c_str());

    MiviSettings::getVal("InternalPreviewBufferQueueSize", mInternalQuickviewBufferSize, 20);

    if (createInternalQuickviewStream() != 0) {
        MLOGE(LogGroupRT, "create internal quickview stream failed!");
        MASSERT(false);
    }

    // NOTE: the streams in stream configuration are composed of three parts:
    // 1. internal quick view stream
    // 2. app streams excluding early pic stream and those streams which has peer stream
    // 3. internal snapshot streams (different CameraModes vary a lot)

    std::vector<std::shared_ptr<Stream>> streams;
    camera3_stream_configuration *raw = mConfigFromFwk->toRaw();
    float frameRatio = 1.0f;

    for (int i = 0; i < raw->num_streams; i++) {
        auto stream = RemoteStream::create(mVendorCamera, raw->streams[i]);
        if (!stream->isSnapshot()) {
            if (stream->hasPeerStream()) {
                // NOTE: fwk peer stream is overrided by its peer stream in mihal
                stream->getPeerStream()->setStreamId(streams.size());
                streams.push_back(stream->getPeerStream()->shared_from_this());
                // NOTE: because these fwk stream is not sent to vendor, it's mihal's duty to set
                // its max buffer num
                raw->streams[i]->max_buffers = mFwkStreamMaxBufferQueueSize;
            } else {
                // NOTE: other non snapshot Fwk streams will be configured to Qcom
                stream->setStreamId(streams.size());
                streams.push_back(stream);
            }
        } else {
            // NOTE: Fwk snapshot stream is just a early pic stream, it will be filled by mihal not
            // Qcom so we don't sent it to vendor NOTE: For the same reason, now it's mihal's not
            // vendor's responsibility to set max_buffers
            raw->streams[i]->max_buffers = mFwkStreamMaxBufferQueueSize;
            frameRatio = static_cast<float>(stream->getWidth()) / stream->getHeight();
        }
    }

    mFrameRatio = frameRatio;
    // NOTE: Metadata assignment will clone the whole metadata
    mSessionParameter = raw->session_parameters;

    initConfigParameter();

    buildInternalSnapshotStreamOverride(streams);

    buildSessionParameterOverride();

    uint32_t opMode = raw->operation_mode;
    buildOpmodeOverride(opMode);

    initCapabilities();

    initUIRelatedMetaList();

    // NOTE: mark all streams which will be sent to Qcom
    for (auto &s : streams) {
        s->setConfiguredToHal();
    }

    // WARNING: mSessionParameter is moved to mConfigToVendor, so you mustn't use it again
    mConfigToVendor = std::make_unique<LocalConfig>(streams, opMode, mSessionParameter);

    if (mConfigToVendor) {
        MLLOGI(LogGroupHAL, mConfigToVendor->toString(),
               " for camera %s, built config to vendor:", mVendorCamera->getId().c_str());

        for (const auto &stream : mConfigToVendor->getStreams()) {
            if (stream->isInternal() && !stream->isQuickview()) {
                stream->setBufferCapacity(mBufferCapacity);
                stream->registerListener([this](bool increased) { onMihalBufferChanged(); });
            }

            // NOTE: Save raw format of mihal stream config here. Once vendor has misbehavior
            // on format, we can query correct raw value from mRawStreamFormat.
            mRawStreamFormat.insert({stream->toRaw(), stream->getFormat()});
        }
    }

    return mConfigToVendor.get();
}

std::shared_ptr<LocalConfig> CameraMode::buildConfigToDarkroom(int type)
{
    std::lock_guard<std::mutex> lock{mConfigDarkroomMutex};

    CameraDevice *cameraDevice = mVendorCamera;
    uint32_t cameraId = static_cast<uint32_t>(atoi(mVendorCamera->getId().c_str()));
    std::string cameraIdStr = mVendorCamera->getId();
    std::vector<std::shared_ptr<Stream>> streams;
    std::ostringstream finalSignatureStr;
    std::string defaultSig = mVendorInternalStreamInfoMap.signature;
    uint32_t mialgoOpMode = mVendorInternalStreamInfoMap.mialgoOpMode;
    ImageSize inputSize = {0, 0};
    ImageSize outputSize = {0, 0};

    // NOTE: At present, only sat has different master id.
    if (isSat()) {
        cameraId = mMasterId.load();
        cameraIdStr = std::to_string(cameraId);

        for (auto camera : mVendorCamera->getPhyCameras()) {
            if (cameraIdStr == camera->getId()) {
                cameraDevice = camera;
                break;
            }
        }
        MASSERT(cameraDevice);
    } else if (isDualBokeh()) {
        // NOTE: We only use master yuv stream to build mialgo streams.
        cameraId = getBokehMasterId();
    }

    // NOTE: At present, we only config yuv stream to mialgoengine for most scenes.
    // Meanwhile, We only config one input stream and one output stream.
    for (const auto &stream : mConfigToVendor->getStreams()) {
        if (!stream->isInternal() || !stream->isSnapshot())
            continue;

        if (stream->getFormat() != HAL_PIXEL_FORMAT_YCBCR_420_888 &&
            stream->getFormat() != HAL_PIXEL_FORMAT_RAW10)
            continue;

        if (isSat() && cameraId != atoi(stream->getPhyId()))
            continue;

        if (isSuperHDMode()) {
            ImageSize superResolution = getSuperResolution();
            ImageSize binningResolution = getBinningSizeOfSuperResolution();
            if (type == Photographer::Photographer_HD_REMOSAIC) {
                if (!isSizeSimilar(stream->getWidth(), stream->getHeight(), superResolution.width,
                                   superResolution.height)) {
                    continue;
                }
            } else {
                if (!isSizeSimilar(stream->getWidth(), stream->getHeight(), binningResolution.width,
                                   binningResolution.height)) {
                    continue;
                }
            }
        }

        inputSize.width = outputSize.width = stream->getWidth();
        inputSize.height = outputSize.height = stream->getHeight();

        if (isDualBokeh() && getBokehMDMode()) {
            finalSignatureStr << "Madrid_";
        }

        if (type == Photographer::Photographer_BURST) {
            finalSignatureStr << "Burst_";
        }
        finalSignatureStr << defaultSig << "_Camera_" << cameraIdStr << "_input(" << inputSize.width
                          << "x" << inputSize.height << ")_output(" << outputSize.width << "x"
                          << outputSize.height << ")";
        MLOGD(LogGroupHAL, "finalSignatureStr: %s", finalSignatureStr.str().c_str());

        if (mConfigToDarkroom) {
            std::string lastSignatureStr;
            const Metadata *meta = mConfigToDarkroom->getMeta();
            auto entry = meta->find(MI_MIVI_POST_SESSION_SIG);
            if (entry.count) {
                lastSignatureStr = reinterpret_cast<const char *>(entry.data.u8);
                MLOGD(LogGroupHAL, "lastSignatureStr: %s", lastSignatureStr.c_str());

                if (lastSignatureStr == finalSignatureStr.str()) {
                    MLOGD(LogGroupHAL,
                          "already has the same size config:%s, no need to config darkroom "
                          "again",
                          finalSignatureStr.str().c_str());
                    return mConfigToDarkroom;
                }
            }
        }

        auto inputStream = LocalStream::create(cameraDevice, inputSize.width, inputSize.height,
                                               stream->getFormat(), stream->getUsage(),
                                               stream->getStreamUsecase(), stream->getDataspace(),
                                               true, stream->getPhyId(), CAMERA3_STREAM_INPUT);
        streams.push_back(inputStream);

        int outputFormat = HAL_PIXEL_FORMAT_YCBCR_420_888;
        if (type == Photographer::Photographer_BURST) {
            outputFormat = HAL_PIXEL_FORMAT_BLOB;
        }
        auto outputStream = LocalStream::create(cameraDevice, outputSize.width, outputSize.height,
                                                outputFormat, stream->getUsage(),
                                                stream->getStreamUsecase(), stream->getDataspace(),
                                                true, stream->getPhyId(), CAMERA3_STREAM_OUTPUT);
        streams.push_back(outputStream);

        if (isGalleryBokehReFocusNeeded()) {
            // NOTE: It means we need one input stream and three output streams to realize the
            // function that we can do refocus for picture in gallery. Now it's needed in bokeh.

            // NOTE: For the original picture.
            auto originalOutputStream = LocalStream::create(
                mVendorCamera, stream->getWidth(), stream->getHeight(), stream->getFormat(),
                stream->getUsage(), stream->getStreamUsecase(), stream->getDataspace(), true,
                stream->getPhyId(), CAMERA3_STREAM_OUTPUT);
            streams.push_back(originalOutputStream);

            // NOTE: For the depth Y16 fromat stream.
            uint32_t depthStreamWidth = stream->getWidth();
            uint32_t depthStreamHeight = stream->getHeight();
            // NOTE: depthScale and compressImage are used by algorithm team to debug
            uint32_t depthScale = property_get_int32("persist.vendor.mibokeh.depth.scale", 0);
            uint32_t compressImage =
                property_get_int32("persist.vendor.camera.capturebokeh.compress.image", 0);
            if (depthScale && compressImage == 1) {
                depthStreamWidth *= depthScale;
                depthStreamHeight *= depthScale;
                MLOGD(LogGroupHAL,
                      "[Bokeh]: debug info: dapthScale:%d, after scale: depth buffer width:%u, "
                      "height:%u",
                      depthScale, depthStreamWidth, depthStreamHeight);
            } else if (getBokehVendor() == static_cast<uint8_t>(BokehVendor::ArcSoft)) {
                // XXX:NOTE: the depth stream, how to decide the size? half or quarter?
                // I think there is a tag to determin it, now hardcode
                depthStreamWidth /= 2;
                depthStreamHeight /= 2;
            } else {
                // NOTE: For the depth Y16 fromat stream.
                // NOTE: ((Width/2)*(Height))*2 > depth buf size actual size 8.2MB
                depthStreamWidth = stream->getWidth() / 2;
                depthStreamHeight = stream->getHeight();
            }

            auto depthOutputStream = LocalStream::create(
                mVendorCamera, depthStreamWidth, depthStreamHeight, HAL_PIXEL_FORMAT_Y16,
                stream->getUsage(), stream->getStreamUsecase(), stream->getDataspace(), true,
                stream->getPhyId(), CAMERA3_STREAM_OUTPUT);
            streams.push_back(depthOutputStream);
        }

        break;
    }

    Metadata meta{};
    meta.update(MI_MIVI_POST_SESSION_SIG, finalSignatureStr.str().c_str());

    if (type == Photographer::Photographer_BURST) {
        mialgoOpMode = mialgo2::StreamConfigModeSATBurst;
    }
    mConfigToDarkroom = std::make_shared<LocalConfig>(streams, mialgoOpMode, meta);

    return mConfigToDarkroom;
}

void CameraMode::buildConfigToMock(std::shared_ptr<CaptureRequest> fwkRequest)
{
    auto darkroomStreams = mConfigToDarkroom->getStreams();
    std::vector<std::shared_ptr<Stream>> inviteStreams;
    for (auto stream : darkroomStreams) {
        if (stream->getType() == CAMERA3_STREAM_OUTPUT) {
            inviteStreams.push_back(stream);
        }
    }
    addToMockInvitations(fwkRequest, inviteStreams);
}

void CameraMode::updateFwkOverridedStream()
{
    // update fwk stream which is overrided by mihal stream
    for (const auto &stream : mConfigFromFwk->getStreams()) {
        if (stream->hasPeerStream()) {
            stream->setUsage(stream->getPeerStream()->getUsage());
        }
    }
}

int CameraMode::getMultiFrameNums(int type)
{
    int frameNums = 1;
    if (type == Photographer::Photographer_MIHAL_MFNR) {
        frameNums = getMihalMfnrSum();
    } else if (type == Photographer::Photographer_HDR) {
        frameNums = getHdrCheckerSum();
    } else if (type == Photographer::Photographer_SR) {
        frameNums = getSrCheckerSum();
    } else if (type == Photographer::Photographer_SN) {
        frameNums = getSuperNightCheckerSum();
    }
    return frameNums;
}

// https://xiaomi.f.mioffice.cn/docs/dock45SZPZq2wkWOCnEayzajJbd#
void CameraMode::updateQuickSnapshot(int &type, int &frameNums,
                                     std::shared_ptr<CaptureRequest> fwkRequest)
{
    int pendingCount = mSession->getPendingFinalSnapCount();
    int pendingMFNRCount = mSession->getPendingVendorMFNR();
    int originalType = type;
    auto currentZoomRatio = mUIZoomRatio.load();
    std::string masterId = std::to_string(mMasterId.load());
    // Note: mihal Use app's signal to decide whether to execute quickshot and cancel the
    // calculation of timediff
    bool isQuickSnapshot = isQuickShot(fwkRequest);
    bool isMoonMode = (mAiAsdSceneApplied.load() == SUPERMOON) ? true : false;
    bool lackBuffer = pendingCount > mMaxQuickShotCount;
    bool queueFull = pendingMFNRCount >= mMaxQuickShotCount;
    // NOTE:if memFull is true , we execute Photographer_SIMPLE by high priority
    bool memFull = PsiMemMonitor::getInstance()->isPsiMemFull();
    bool isLowRamDev = PsiMemMonitor::getInstance()->isLowRamDev();

    if (type == Photographer::Photographer_HDR) {
        if (memFull || (isLowRamDev && lackBuffer && isQuickSnapshot)) {
            type = Photographer::Photographer_SIMPLE;
            frameNums = 1;
        } else if (lackBuffer || isQuickSnapshot) {
            if (isFeatureMihalMfnrSupported()) {
                type = Photographer::Photographer_MIHAL_MFNR;
                frameNums = 5;
            } else {
                type = Photographer::Photographer_VENDOR_MFNR;
                frameNums = 1;
            }
        }
    } else if ((type == Photographer::Photographer_SR ||
                type == Photographer::Photographer_HDRSR) &&
               !isMoonMode) {
        int32_t currentCameraRoleId = getRoleIdByCurrentCameraDevice();
        if (currentZoomRatio >= mMfnrRoleIdToThresholdMap[currentCameraRoleId].first &&
            currentZoomRatio < mMfnrRoleIdToThresholdMap[currentCameraRoleId].second) {
            if (memFull) {
                type = Photographer::Photographer_SIMPLE;
                frameNums = 1;
            } else if (lackBuffer || isQuickSnapshot) {
                if (isFeatureMihalMfnrSupported()) {
                    type = Photographer::Photographer_MIHAL_MFNR;
                    frameNums = 5;
                } else {
                    type = Photographer::Photographer_VENDOR_MFNR;
                    frameNums = 1;
                }
            }
        }
    }

    if (type == Photographer::Photographer_MIHAL_MFNR) {
        if (lackBuffer) {
            type = Photographer::Photographer_SIMPLE;
            frameNums = 1;
        } else if (isQuickSnapshot) {
            frameNums = 5;
        }
    } else if (type == Photographer::Photographer_VENDOR_MFNR) {
        if (queueFull) {
            if (isFeatureMihalMfnrSupported()) {
                type = Photographer::Photographer_MIHAL_MFNR;
                frameNums = 5;
            } else {
                type = Photographer::Photographer_VENDOR_MFNR;
                frameNums = 1;
            }
        }
    }

    MLOGI(LogGroupHAL, "[MiviQuickShot] type %d->%d isQuickSnapshot: %d", originalType, type,
          isQuickSnapshot);
}

int CameraMode::getPhotographerType(std::shared_ptr<CaptureRequest> fwkRequest)
{
    int type = Photographer::Photographer_SIMPLE;

    if (isMiui3rdCapture(fwkRequest))
        return type;

    if (isFeatureBurstSupported() && canDoBurstCapture()) {
        type = Photographer::Photographer_BURST;
    } else if (isFeatureSuperNightSupported() && canDoSuperNight(fwkRequest)) {
        type = isDualBokeh() ? Photographer::Photographer_BOKEH_SE : Photographer::Photographer_SN;
    } else if (isFeatureHdrSupported() && canDoHdr()) {
        if (isFeatureHdrSrSupported() && canDoHdrSr()) {
            type = Photographer::Photographer_HDRSR;
        } else {
            type = isDualBokeh() ? Photographer::Photographer_BOKEH_HDR
                                 : Photographer::Photographer_HDR;
        }
    } else if (isFeatureFusionSupported() && canDoMfnrFusion()) {
        type = Photographer::Photographer_FUSION;
    } else if (isFeatureFusionSupported() && canDoSrFusion()) {
        type = Photographer::Photographer_SR_FUSION;
    } else if (isFeatureSrSupported() && canDoSr()) {
        type = Photographer::Photographer_SR;
    } else {
        if (isDualBokeh()) {
            if (isMadridBokehModeSupported() && isMadridBokehModeEnabled(fwkRequest->getMeta())) {
                type = Photographer::Photographer_BOKEH_MD;
            } else {
                type = Photographer::Photographer_BOKEH;
            }
        } else if (isSingleBokeh()) {
            type = Photographer::Photographer_BOKEH;
        } else if (isSuperHDRemosaic()) {
            mSuperHdData.isRemosaic = true;
            type = Photographer::Photographer_HD_REMOSAIC;
        } else if (isSuperHDMode()) {
            mSuperHdData.isRemosaic = false;
            type = Photographer::Photographer_HD_UPSCALE;
        } else if (isProfessionalMode()) {
            type = Photographer::Photographer_MANUAL;
        } else if (isFeatureMihalMfnrSupported()) {
            type = Photographer::Photographer_MIHAL_MFNR;
        } else if (isFeatureMfnrSupported()) {
            type = Photographer::Photographer_VENDOR_MFNR;
        }
    }

    return type;
}

std::vector<std::shared_ptr<Stream>> CameraMode::getCurrentCaptureStreams(
    std::shared_ptr<CaptureRequest> fwkRequest, int type)
{
    std::string masterId = std::to_string(mMasterId.load());
    std::vector<std::shared_ptr<Stream>> streams;
    int32_t currentCameraRoleId = getRoleIdByCurrentCameraDevice();
    std::vector<int> formatVec = getOverrideStreamFormat(currentCameraRoleId, type);
    int format = HAL_PIXEL_FORMAT_YCBCR_420_888;
    if (!formatVec.empty()) {
        format = formatVec[0];
    }

    if (isMultiCameraCase()) {
        if (isDualBokeh() && (formatVec.size() > 1)) {
            int masterFormat = formatVec[0];
            int slaveFormat = formatVec[1];
            MLOGI(LogGroupHAL, "master:slave format: [%d %d]", masterFormat, slaveFormat);
            streams.push_back(getBokehMasterStream(masterFormat));
            streams.push_back(getBokehSlaveStream(slaveFormat));
        } else if (type == Photographer::Photographer_FUSION) {
            streams = getFusionStreams(UltraWideAndWide);
        } else if (type == Photographer::Photographer_SR_FUSION) {
            streams = getFusionStreams(WideAndTele);
        } else if (isFeatureFakeSatSupported() && canDoFakeSat(currentCameraRoleId) &&
                   (type == Photographer::Photographer_SR)) {
            std::shared_ptr<Stream> masterStream = getFakeSatStream(masterId, format);
            if (!masterStream) {
                MLOGI(LogGroupHAL, "Get null fakesat stream with role %d format %d",
                      currentCameraRoleId, format);
                masterStream = getCurrentPhyStream(masterId, format);
            } else {
                LocalRequest *lr = reinterpret_cast<LocalRequest *>(fwkRequest.get());
                uint8_t fakeSatEnabled = 1;
                lr->updateMeta(MI_FAKESAT_ENABLED, &fakeSatEnabled, 1);
                MLOGI(LogGroupHAL, "[FAKESAT]fakeSat enabled size %dx%d", masterStream->getWidth(),
                      masterStream->getHeight());
            }
            MASSERT(masterStream);
            streams.push_back(masterStream);
        } else {
            std::shared_ptr<Stream> masterStream = getCurrentPhyStream(masterId, format);
            MASSERT(masterStream);
            streams.push_back(masterStream);
        }
    } else {
        for (const auto &stream : mConfigToVendor->getStreams()) {
            bool isMatch = false;
            if (!stream->isInternal() || stream->isQuickview())
                continue;

            if (type == Photographer::Photographer_SN) {
                if (mVendorCamera->isFrontCamera() && stream->getFormat() == format) {
                    isMatch = true;
                } else if (!mVendorCamera->isFrontCamera() && stream->getFormat() == format) {
                    isMatch = true;
                }
            } else if (type == Photographer::Photographer_HD_REMOSAIC) {
                ImageSize superResolution = getSuperResolution();
                if (isSizeSimilar(stream->getWidth(), stream->getHeight(), superResolution.width,
                                  superResolution.height))
                    isMatch = true;
            } else if (type == Photographer::Photographer_HD_UPSCALE) {
                ImageSize binningResolution = getBinningSizeOfSuperResolution();
                if (isSizeSimilar(stream->getWidth(), stream->getHeight(), binningResolution.width,
                                  binningResolution.height)) {
                    isMatch = true;
                }
            } else if (stream->getFormat() == format) {
                isMatch = true;
            }

            if (isMatch) {
                streams.push_back(stream);
                break;
            }
        }
    }

    return streams;
}

std::shared_ptr<Photographer> CameraMode::createPhotographerOverride(
    std::shared_ptr<CaptureRequest> fwkRequest)
{
    Photographer *phg{};
    Metadata extraMeta = {};
    std::vector<std::shared_ptr<Stream>> streams;
    LocalRequest *lr = reinterpret_cast<LocalRequest *>(fwkRequest.get());

    // HDR-DISPLAY ADD: start
    int32_t tmpShotAdrc = 0;
    tmpShotAdrc = mAdrcGain.load() * 100;
    lr->updateMeta(MI_SNAPSHOT_ADRC, &tmpShotAdrc, 1);
    // HDR-DISPLAY ADD: end

    int type = getPhotographerType(fwkRequest);
    MLOGI(LogGroupHAL, "[ProcessRequest][KEY][fwkFrame:%u]: phg type:%d",
          fwkRequest->getFrameNumber(), type);

    // TODO: Now the logic of updateQuickSnapshot just work in sat, but it may be
    // needed in the other modes later.
    int multiFrameNum = getMultiFrameNums(type);
    bool isQuickShotType = false;
    if (isSupportQuickShot(type, fwkRequest)) {
        isQuickShotType = true;
        // NOTE: we should tell qcom with this tag
        LocalRequest *lr = reinterpret_cast<LocalRequest *>(fwkRequest.get());
        uint8_t isSupportHQQuick = 1;
        lr->updateMeta(MI_QUICKSHOT_ISHQQUICKSHOT, &isSupportHQQuick, 1);
        MLOGI(LogGroupHAL, "[MiviQuickShot]HQQuickShot :1 ");
        if (mModeType == MODE_CAPTURE) {
            updateQuickSnapshot(type, multiFrameNum, fwkRequest);
        }
    }
    MLOGI(LogGroupHAL, "[ProcessRequest][KEY][fwkFrame:%u]: final phg type:%d",
          fwkRequest->getFrameNumber(), type);

    // NOTE: build config to mialgo and mockcamera
    auto darkroomConfig = buildConfigToDarkroom(type);
    buildConfigToMock(fwkRequest);

    streams = getCurrentCaptureStreams(fwkRequest, type);
    if (streams.size()) {
        for (auto &stream : streams) {
            uint32_t availbleBuffers = stream->getAvailableBuffers();
            MLOGI(LogGroupHAL,
                  "[ProcessRequest][Statistic][fwkFrame:%u]: stream:%s 's availBuffers:%u",
                  fwkRequest->getFrameNumber(), stream->toString(0).c_str(), availbleBuffers);
        }

        // NOTE: update output size and get the extra meta
        if (isSat()) {
            std::pair<int, int> resolution{};
            resolution = CameraManager::getInstance()->getMaxYUVResolution(
                static_cast<uint32_t>(mMasterId.load()), mFrameRatio, false);
            ImageSize outputImage = {static_cast<uint32_t>(resolution.first),
                                     static_cast<uint32_t>(resolution.second)};
            extraMeta = updateOutputSize(fwkRequest->getMeta(), outputImage);
        } else {
            ImageSize imageSize = {streams[0]->getWidth(), streams[0]->getHeight()};

            // for 1:1/16:9/full sizem, to get raw
            if (darkroomConfig.get()) {
                std::vector<std::shared_ptr<Stream>> darkroomStreams = darkroomConfig->getStreams();
                for (auto stream : darkroomStreams) {
                    if (stream->getType() == CAMERA3_STREAM_OUTPUT) {
                        imageSize = {stream->getWidth(), stream->getHeight()};
                        break;
                    }
                }
            }

            if (isUpScaleCase()) {
                std::pair<int, int> resolution{};
                resolution = CameraManager::getInstance()->getMaxYUVResolution(
                    static_cast<uint32_t>(atoi(mVendorCamera->getId().c_str())),
                    static_cast<float>(imageSize.width) / imageSize.height, true);

                imageSize = {static_cast<uint32_t>(resolution.first),
                             static_cast<uint32_t>(resolution.second)};
            } else if (type == Photographer::Photographer_HD_REMOSAIC ||
                       type == Photographer::Photographer_HD_UPSCALE) {
                imageSize = {getSuperResolution().width, getSuperResolution().height};
            }

            MLOGD(LogGroupHAL, "[ProcessRequest] final pic size: %d x %d", imageSize.width,
                  imageSize.height);
            extraMeta = updateOutputSize(fwkRequest->getMeta(), imageSize);
        }
    } else {
        MLOGE(LogGroupHAL, "[ProcessRequest][KEY] internal streams is null for fwk frame:%u !!!",
              fwkRequest->getFrameNumber());
        MASSERT(false);
    }

    if (isManualMode()) {
        // NOTE: In manual mode, mfnr may takes a long time so we should set the flushWaitTime
        // to a large value like 4s to avoid small picture when flush immediately after capture.
        // But if this capture uses long exposure and it does not run mfnr, it will not take a
        // long time after generating shutter. So we should set the flushWaitTime to a small
        // value like 700ms to avoid long time gaussian blur when reentering camera after long
        // exposure capture.
        const Metadata *requestMetadata = fwkRequest->getMeta();
        uint8_t mfnrEnabledByApp = false;
        auto entry = requestMetadata->find(MI_MFNR_ENABLE);
        if (entry.count) {
            mfnrEnabledByApp = reinterpret_cast<const uint8_t>(entry.data.u8[0]);
        }
        if (mfnrEnabledByApp) {
            mWaitFlushMillisecs =
                std::chrono::milliseconds{mVendorInternalStreamInfoMap.flushWaitTime};
            MLOGD(LogGroupHAL, "[Professional] mfnr is enabled by app, flushWaitTime:%zdms",
                  mVendorInternalStreamInfoMap.flushWaitTime);
        } else {
            int64_t flushWaitTime = 700;
            mWaitFlushMillisecs = std::chrono::milliseconds{flushWaitTime};
            MLOGD(LogGroupHAL, "[Professional] mfnr is disabled, flushWaitTime:%zdms",
                  flushWaitTime);
        }
    }

    // HDR-DISPLAY ADD: start
    lr->updateMeta(MI_SNAPSHOT_SHOTTYPE, &type, 1);
    // HDR-DISPLAY ADD: end

    switch (type) {
    case Photographer::Photographer_BOKEH_SE: {
        phg = new SEBokehPhotographer{fwkRequest, mSession, streams, darkroomConfig,
                                      &mApplySuperNightData};
        break;
    }
    case Photographer::Photographer_BOKEH_HDR: {
        phg = new HdrBokehPhotographer{fwkRequest, mSession, streams,
                                       mApplyHdrData.hdrStatus.expVaules, darkroomConfig};
        break;
    }
    case Photographer::Photographer_BOKEH_MD: {
        phg = new MDBokehPhotographer{fwkRequest, mSession, streams, mBokehMadridEvData,
                                      darkroomConfig};
        break;
    }
    case Photographer::Photographer_BOKEH: {
        phg = new BokehPhotographer{fwkRequest, mSession, streams, darkroomConfig, extraMeta};
        break;
    }
    case Photographer::Photographer_SN: {
        if (HAL_PIXEL_FORMAT_RAW16 == streams[0]->getFormat() ||
            HAL_PIXEL_FORMAT_RAW10 == streams[0]->getFormat()) {
            phg = new RawSupernightPhotographer(fwkRequest, mSession, streams[0], darkroomConfig,
                                                &mApplySuperNightData, extraMeta);
        } else {
            phg = new SupernightPhotographer(fwkRequest, mSession, streams[0], darkroomConfig,
                                             &mApplySuperNightData, extraMeta);
        }
        break;
    }
    case Photographer::Photographer_HDR: {
        phg = new HdrPhotographer{fwkRequest,    mSession,  streams[0],
                                  mApplyHdrData, extraMeta, darkroomConfig};
        break;
    }
    case Photographer::Photographer_SR: {
        phg = new SrPhotographer{
            fwkRequest, mSession, streams[0],    static_cast<uint32_t>(getSrCheckerSum()),
            extraMeta,  false,    darkroomConfig};
        break;
    }
    case Photographer::Photographer_HDRSR: {
        phg = new SrHdrPhotographer{fwkRequest,    mSession,  streams[0],
                                    mApplyHdrData, extraMeta, darkroomConfig};
        break;
    }
    case Photographer::Photographer_FUSION:
    case Photographer::Photographer_SR_FUSION: {
        phg = new FusionPhotographer{fwkRequest, mSession, streams, extraMeta, darkroomConfig};
        break;
    }
    case Photographer::Photographer_HD_UPSCALE:
    case Photographer::Photographer_HD_REMOSAIC: {
        phg = new SuperHDPhotographer{fwkRequest,    mSession,  streams,
                                      &mSuperHdData, extraMeta, darkroomConfig};
        break;
    }
    case Photographer::Photographer_MANUAL: {
        phg = new ProfessionalPhotographer{fwkRequest, mSession, streams, darkroomConfig};
        break;
    }
    case Photographer::Photographer_BURST: {
        const Metadata *requestMetadata =
            mBurstMeta.isEmpty() ? fwkRequest->getMeta() : &mBurstMeta;
        // Use previous request to control AWB
        uint8_t isQuickSnapshot = 1;
        lr->updateMeta(MI_QUICKSHOT_ISQUICKSNAPSHOT, &isQuickSnapshot, 1);
        uint8_t zslEnable = 1;
        lr->updateMeta(ANDROID_CONTROL_ENABLE_ZSL, &zslEnable, 1);
        if (streams[0]->getFormat() == HAL_PIXEL_FORMAT_RAW10) {
            uint8_t rawZslEnable = 1;
            lr->updateMeta(MI_MIVI_RAW_ZSL_ENABLE, &rawZslEnable, 1);
        }

        phg = new BurstPhotographer{fwkRequest, mSession,         streams[0],
                                    extraMeta,  *requestMetadata, darkroomConfig};
        break;
    }
    case Photographer::Photographer_MIHAL_MFNR: {
        uint32_t availbleBuffers = streams[0]->getAvailableBuffers();
        if (availbleBuffers < multiFrameNum) {
            uint8_t mfnrEnabled = 0;
            lr->updateMeta(MI_MFNR_ENABLE, &mfnrEnabled, 1);
            if (streams[0]->getFormat() == HAL_PIXEL_FORMAT_RAW10) {
                uint8_t rawZslEnable = 1;
                lr->updateMeta(MI_MIVI_RAW_ZSL_ENABLE, &rawZslEnable, 1);
            }

            type = Photographer::Photographer_SIMPLE;
            phg = new Photographer{fwkRequest, mSession, streams[0], extraMeta, darkroomConfig};
        } else {
            uint8_t mfnrEnabled = 2;
            lr->updateMeta(MI_MFNR_ENABLE, &mfnrEnabled, 1);
            uint8_t rawZslEnable = 1;
            lr->updateMeta(MI_MIVI_RAW_ZSL_ENABLE, &rawZslEnable, 1);

            phg = new HwmfPhotographer{fwkRequest, mSession,
                                       streams[0], static_cast<uint32_t>(multiFrameNum),
                                       extraMeta,  darkroomConfig};
        }
        break;
    }
    case Photographer::Photographer_VENDOR_MFNR:
    case Photographer::Photographer_SIMPLE: {
        uint8_t mfnrEnabled = 0;
        if (type == Photographer::Photographer_VENDOR_MFNR) {
            mfnrEnabled = 1;
        }
        lr->updateMeta(MI_MFNR_ENABLE, &mfnrEnabled, 1);
        uint8_t hdrEnable = 0;
        lr->updateMeta(MI_HDR_ENABLE, &hdrEnable, 1);
        uint8_t srEnable = 0;
        lr->updateMeta(MI_SUPER_RESOLUTION_ENALBE, &srEnable, 1);
        uint8_t zslEnable = 1;
        lr->updateMeta(ANDROID_CONTROL_ENABLE_ZSL, &zslEnable, 1);
        bool snMfnrEnabled = false;
        lr->updateMeta(MI_SUPERNIGHT_MFNR_ENABLED, (const uint8_t *)&snMfnrEnabled, 1);
        lr->updateMeta(MI_MULTIFRAME_INPUTNUM, &multiFrameNum, 1);
        if (streams[0]->getFormat() == HAL_PIXEL_FORMAT_RAW10) {
            uint8_t rawZslEnable = 1;
            lr->updateMeta(MI_MIVI_RAW_ZSL_ENABLE, &rawZslEnable, 1);
        }

        phg = new Photographer{fwkRequest, mSession, streams[0], extraMeta, darkroomConfig};
        break;
    }
    }

    MASSERT(phg);

    std::shared_ptr<Photographer> sharedPhg{phg, [](Photographer *p) {
                                                MLOGI(LogGroupHAL, "delete Photographer: \n%s",
                                                      p->toString().c_str());
                                                delete p;
                                            }};

    if (isQuickShotType) {
        uint64_t delayTime = 0;
        int pendingCount = mSession->getPendingFinalSnapCount();
        delayTime = getQuickShotDelayTime(type);
        MLOGI(LogGroupHAL,
              "[MiviQuickShot][Performance] phtgph %s oriType %d pendingCnt %d delaytime %" PRId64,
              sharedPhg->getName().c_str(), type, pendingCount, delayTime);

        sharedPhg->enableShotWithDealyTime(delayTime);
    }

    sharedPhg->setPhtgphType(type);
    if (isMultiCameraCase()) {
        sharedPhg->setMasterPhyId(getMultiCameraMasterId());
    }

    return sharedPhg;
}

std::shared_ptr<Photographer> CameraMode::createPhotographer(
    std::shared_ptr<CaptureRequest> fwkRequest)
{
    // NOTE: record num of buffers per stream before snapshot

    for (const auto &stream : mConfigToVendor->getStreams()) {
        if (stream->isInternal() && !stream->isQuickview()) {
            auto streamId = stream->getStreamId();
            stream->resetConsumeCnt();
        }
    }

    std::shared_ptr<Photographer> phtgph = createPhotographerOverride(fwkRequest);
    MLOGI(LogGroupHAL, "%s[ProcessRequest][KEY]: create %s", phtgph->getLogPrefix().c_str(),
          phtgph->toString().c_str());

    // NOTE: after phg is constructed, we can decide what mialgo port these buffers will be sent to
    phtgph->decideMialgoPortBufferSentTo();

    // mialgo preprocess
    if (phtgph->getPhtgphType() != Photographer::Photographer_BURST) {
        phtgph->buildPreRequestToDarkroom();
    }

    if (mVendorCamera->isSupportAnchorFrame() &&
        Session::AsyncAlgoSessionType == mSession->getSessionType()) {
        phtgph->updateAnchorFrameQuickView();
    }

    // NOTE: record num of buffers per stream after snapshot and calculate num of buffers consumed
    // by this snapshot
    std::map<uint32_t, int> buffersAfterSnapshot;
    for (const auto &stream : mConfigToVendor->getStreams()) {
        if (stream->isInternal() && !stream->isQuickview()) {
            auto streamId = stream->getStreamId();
            // XXX: when mihal constructing phg, mialgo may release some buffer, so the
            // mBuffersConsumed may not be the real buffer consumption of this snapshot and the
            // value may even be negative which means mialgo has released a lot of buffer but this
            // snapshot just needs very few buffer
            buffersAfterSnapshot[streamId] = stream->getVirtualAvailableBuffers();
            mBuffersConsumed[streamId] = stream->getConsumeCnt();
            MLOGD(LogGroupHAL,
                  "[AppSnapshotController] stream %d:%p consumed:remains %d:%d free "
                  "buffers,Capacity %d,bufferrLimit %d",
                  streamId, stream->toRaw(), mBuffersConsumed[streamId].load(),
                  buffersAfterSnapshot[streamId], stream->getBufferCapacity(), mBufferLimitCnt);
            if (mBufferLimitCnt > stream->getBufferCapacity()) {
                MLOGE(LogGroupHAL, "[AppSnapshotController] Capacity  %d < bufferrLimit %d",
                      stream->getBufferCapacity(), mBufferLimitCnt);
            }
        }
    }

    return phtgph;
}

void CameraMode::onMihalBufferChanged()
{
    // NOTE: if CameraMode is retired then its status is no longer useful
    if (mIsRetired.load())
        return;

    std::unique_lock<std::mutex> lg(mCheckStatusLock);
    bool status = canCamModeHandleNextSnapshot();

    if (status) {
        MLOGD(LogGroupHAL, "[AppSnapshotController] CameraMode: %p have enough free buffer", this);
    } else {
        MLOGD(LogGroupHAL,
              "[AppSnapshotController] CameraMode: %p may not has enough free buffer for next "
              "snapshot/too many MFNR requests",
              this);
    }
    AppSnapshotController::getInstance()->onRecordUpdate(status, this);
}

bool CameraMode::canCamModeHandleNextSnapshot()
{
    bool avail = true;
    for (const auto &stream : mConfigToVendor->getStreams()) {
        if (stream->isInternal() && !stream->isQuickview()) {
            auto bufferAvailable = stream->getVirtualAvailableBuffers();
            // auto bufferUsePrediction = bufferUsePredicForNextSnapshot(stream->getStreamId());
            if (bufferAvailable < mBufferLimitCnt) {
                MLOGD(LogGroupHAL,
                      "[AppSnapshotController] stream: %p may not have enough free buffers for "
                      "next snapshot! bufferAvailable:Limit = %d:%d",
                      stream->toRaw(), bufferAvailable, mBufferLimitCnt);
                avail = false;
                break;
            }
        }
    }

    if (avail) {
        int32_t num = mSession->getPendingFinalSnapCount();
        if (num >= 10) {
            MLOGD(LogGroupHAL,
                  "[AppSnapshotController] session: %p pending vendor and mialgoengine request "
                  "num: %d",
                  mSession, num);
            avail = false;
        }
    }

    return avail;
}

int CameraMode::bufferUsePredicForNextSnapshot(uint32_t streamId)
{
    // NOTE: default behavior: return buffer consumption of last snapshot as prediction
    int bufferConsumption = mBuffersConsumed.at(streamId).load();
    if (bufferConsumption <= 0)
        bufferConsumption = 0;

    if (canDoHdr()) {
        bufferConsumption = getHdrCheckerSum();
    } else {
        SuperNightData tmp;
        {
            std::lock_guard<std::mutex> lg(mSuperNightData.mutex);
            tmp = mSuperNightData;
        }

        if (canCheckerValDoSuperNight(tmp)) {
            // NOTE: if vnd checker can do super night, chances are the next snapshot will be super
            // night, but still, it depends on app
            bufferConsumption = getSuperNightCheckerSum();
        }
    }

    return bufferConsumption;
}

int CameraMode::createInternalQuickviewStream()
{
    if (mSession->getSessionType() == Session::SyncAlgoSessionType) {
        return 0;
    }
    for (const auto &stream : mConfigFromFwk->getStreams()) {
        // NOTE: for now, the internal quickview stream is a preview stream
        if (stream->isPreview()) {
            mInternalQuickviewStream = LocalStream::create(
                mVendorCamera, stream->getWidth(), stream->getHeight(), stream->getFormat(),
                stream->getUsage(), stream->getStreamUsecase(), stream->getDataspace(), true);
            mInternalQuickviewStream->setStreamProp(Stream::QUICKVIEW);
            MLOGI(LogGroupHAL, "stream getUsage: %d", stream->getUsage());

            mInternalQuickviewStream->setPeerStream(stream.get());
            stream->setPeerStream(mInternalQuickviewStream.get());
            break;
        }
    }

    if (!mInternalQuickviewStream) {
        MLOGE(LogGroupHAL, "can't find preview stream of mConfigToVendor");
        return -ENOENT;
    }

    // mInternalQuickviewStream->setMaxBuffers(mInternalQuickviewBufferSize);
    mInternalQuickviewStream->setBufferCapacity(mInternalQuickviewBufferSize);

    return 0;
}

void CameraMode::returnInternalQuickviewBuffer(std::shared_ptr<StreamBuffer> streamBuffer,
                                               uint32_t frame, int64_t timestamp)
{
    MLOGD(LogGroupRT, "frame: %d, bufferPool size=%zu, stream pool size:%d", frame,
          mInternalQuickviewBuffers.size(), mInternalQuickviewStream->getAvailableBuffers());

    std::lock_guard<std::mutex> locker{mMutex};
    if (streamBuffer->getStatus() != CAMERA3_BUFFER_STATUS_OK) {
        const camera3_stream_buffer *rawStreamBuffer = streamBuffer->toRaw();
        MLOGW(LogGroupHAL, "return error status vndFrame:%u stream %p", frame,
              rawStreamBuffer->stream);
        streamBuffer->releaseBuffer();
        return;
    }
    mInternalQuickviewBuffers.insert({frame, streamBuffer});
    mLastPreviewFrame.store(frame);
    if (timestamp != -1)
        mInternalFrameTimestamp.insert({frame, timestamp});

    if (mInternalQuickviewBuffers.size() > 16) {
        auto it = mInternalQuickviewBuffers.begin();
        it->second->releaseBuffer();
        uint32_t rmFrame = it->first;
        mInternalFrameTimestamp.erase(rmFrame);
        mInternalQuickviewBuffers.erase(it);
    }
    mCond.notify_one();
}

void CameraMode::returnInternalFrameTimestamp(uint32_t frame, int64_t timestamp)
{
    std::lock_guard<std::mutex> locker{mMutex};
    if (timestamp != -1 &&
        mInternalQuickviewBuffers.find(frame) != mInternalQuickviewBuffers.end()) {
        mInternalFrameTimestamp.insert({frame, timestamp});
        MLOGV(LogGroupRT, "internal Preview vndFrame:%u time %" PRId64 "", frame, timestamp);
    }
}

uint32_t CameraMode::findAnchorVndframeByTs(int64_t ts)
{
    std::lock_guard<std::mutex> locker{mMutex};

    int64_t interval_mini = INT64_MAX;
    uint32_t frame = -1;

    for (auto iter : mInternalFrameTimestamp) {
        if (interval_mini > labs(iter.second - ts)) {
            frame = iter.first;
            MLOGV(LogGroupHAL, "[EarlyPic] preview timestamp:%" PRId64 " vndframe:%d ",
                  mInternalFrameTimestamp[frame], frame);
            interval_mini = labs(iter.second - ts);
            if (iter.second == ts)
                break;
        }
    }

    MLOGI(LogGroupHAL,
          "[EarlyPic] nearest timestam:%" PRId64 " vndframe:%d ,MAX_NO_GAUSSIAN_TIME is  60000000",
          mInternalFrameTimestamp[frame], frame);

    return frame;
}

std::shared_ptr<StreamBuffer> CameraMode::getInternalQuickviewBuffer(uint32_t frame)
{
    std::shared_ptr<StreamBuffer> streamBuffer;

    MLOGD(LogGroupRT, "frame: %d, bufferPool size=%zu, stream pool size:%d", frame,
          mInternalQuickviewBuffers.size(), mInternalQuickviewStream->getAvailableBuffers());
    MASSERT(mInternalQuickviewStream);
    if (mInternalQuickviewStream->getAvailableBuffers() > 0) {
        streamBuffer = mInternalQuickviewStream->requestBuffer();
    } else {
        std::lock_guard<std::mutex> locker{mMutex};
        if (mInternalQuickviewBuffers.empty()) {
            MLOGE(LogGroupRT, "there is no free buffer");
            MASSERT(false);
        }
        auto it = mInternalQuickviewBuffers.begin();
        streamBuffer = it->second;
        uint32_t rmFrame = it->first;
        mInternalFrameTimestamp.erase(rmFrame);
        mInternalQuickviewBuffers.erase(it);

        if (mVendorCamera->supportBufferManagerAPI())
            streamBuffer->releaseBuffer();
    }

    streamBuffer->setStatus(CAMERA3_BUFFER_STATUS_OK);
    streamBuffer->setAcquireFence(-1);
    streamBuffer->setReleaseFence(-1);

    MLOGD(LogGroupRT, "frame %d", frame);
    return streamBuffer;
}

void CameraMode::notifyAnchorFrame(uint32_t anchorFrame, uint32_t snapshotFrame)
{
    std::lock_guard<std::mutex> locker{mMutex};
    mVndAnchorFrames[snapshotFrame] = anchorFrame;
    mCond.notify_one();
}

void CameraMode::notifyLastFrame(uint32_t snapshotFrame)
{
    std::lock_guard<std::mutex> locker{mMutex};
    mVndAnchorFrames[snapshotFrame] = mLastPreviewFrame.load();
    mCond.notify_one();
}

std::shared_ptr<StreamBuffer> CameraMode::tryGetAnchorFrameBuffer(uint32_t snapshotFrame,
                                                                  uint32_t anchorFrame)
{
    std::shared_ptr<StreamBuffer> chosenBuffer;
    if (!mInternalQuickviewBuffers.size()) {
        MLOGW(LogGroupHAL, "[EarlyPic] mInternalQuickviewBuffers is empty ");
        return nullptr;
    }
    // check anchorFrame is valid
    auto it = mInternalQuickviewBuffers.find(anchorFrame);
    if (it != mInternalQuickviewBuffers.end()) {
        chosenBuffer = it->second;
        mInternalFrameTimestamp.erase(anchorFrame);
        mInternalQuickviewBuffers.erase(it);
    }

    if (chosenBuffer) {
        MLOGI(LogGroupHAL, "[EarlyPic]: find expected vndFrame:%u in quickview queue", anchorFrame);
        return chosenBuffer;
    } else {
        auto begin = mInternalQuickviewBuffers.begin();
        uint32_t firstFrame = begin->first;
        uint32_t lastFrame = mInternalQuickviewBuffers.rbegin()->first;
        uint32_t checkFrame = anchorFrame;
        bool isAnchorFrameVaild = false;
        bool isSnapshotFrameVaild = false;

        if (anchorFrame > firstFrame && anchorFrame < lastFrame)
            isAnchorFrameVaild = true;

        if (snapshotFrame >= firstFrame && firstFrame > anchorFrame)
            isSnapshotFrameVaild = true;

        MLOGD(LogGroupHAL,
              "[EarlyPic]: QuickviewRange[%d %d] isAnchorFrameVaild:%d, isSnapshotFrameVaild:%d",
              firstFrame, lastFrame, isAnchorFrameVaild, isSnapshotFrameVaild);

        // NOTE: When anchor result is not as expected, there will be three situations:
        // 1. The anchorFrame is within the scope of mInternalQuickviewBuffers.
        //    We will select the frame closest to and less than the anchorFrame in the
        //    mInternalQuickviewBuffers.
        // 2. The anchorFrame is out of the mInternalQuickviewBuffers but snapshotFrame is large
        //    than the firstFrame of the mInternalQuickviewBuffers.
        //    We will select the middle frame of the mInternalQuickviewBuffers, because the middle
        //    frame is closer to the result of anchor in general. But if we capture fast moving
        //    scenes, it will be inaccurate. So it just an optimization option in extreme scenarios.
        // 3. The anchorFrame and the snapshotFrame are all invaild.
        //    We return nullptr to wait the next frame notify.
        uint32_t preFrame = 0;
        if (isAnchorFrameVaild) {
            for (auto iter : mInternalQuickviewBuffers) {
                uint32_t tempFrame = iter.first;
                if (tempFrame <= anchorFrame) {
                    preFrame = tempFrame;
                }
            }

            MLOGI(LogGroupHAL,
                  "[EarlyPic]: didn't find vndFrame:%u, use preframe:%u in quickview queue",
                  anchorFrame, preFrame);

            auto nrIt = mInternalQuickviewBuffers.find(preFrame);
            chosenBuffer = nrIt->second;
            mInternalFrameTimestamp.erase(preFrame);
            mInternalQuickviewBuffers.erase(nrIt);
            return chosenBuffer;
        } else if (isSnapshotFrameVaild) {
            uint32_t index = 0;
            uint32_t middleIndex = mInternalQuickviewBuffers.size() / 2;
            for (auto iter : mInternalQuickviewBuffers) {
                uint32_t tempFrame = iter.first;
                if (index == middleIndex) {
                    preFrame = tempFrame;
                    break;
                }
                index++;
            }

            MLOGI(LogGroupHAL,
                  "[EarlyPic]: didn't find vndFrame:%u, use middleFrame:%u in quickview queue",
                  anchorFrame, preFrame);

            auto nrIt = mInternalQuickviewBuffers.find(preFrame);
            chosenBuffer = nrIt->second;
            mInternalFrameTimestamp.erase(preFrame);
            mInternalQuickviewBuffers.erase(nrIt);
            return chosenBuffer;
        } else {
            return chosenBuffer;
        }
    }
}

void CameraMode::waitLoop()
{
    // FIXME: wait for CameraMode loop thread to reset.
    if (mThread.joinable()) {
        std::unique_lock<std::mutex> locker(mMutex);
        mExitThread = true;
        locker.unlock();

        mCond.notify_one();
        mThread.join();
        MLOGI(LogGroupHAL, "camera %s closed with thread exit by flush",
              mVendorCamera->getId().c_str());
    }
}

void CameraMode::loop()
{
    MLOGI(LogGroupHAL, "[EarlyPic][KEY]: early process thread started");
    prctl(PR_SET_NAME, "MiHalCMLoop");
    while (true) {
        std::unique_lock<std::mutex> locker{mMutex};
        std::shared_ptr<StreamBuffer> inputStreamBuffer;

        mCond.wait(locker, [this, &inputStreamBuffer]() {
            if (!mVndAnchorFrames.empty()) {
                uint32_t snapshotFrame = mVndAnchorFrames.begin()->first;
                uint32_t anchorFrame = mVndAnchorFrames.begin()->second;
                inputStreamBuffer = tryGetAnchorFrameBuffer(snapshotFrame, anchorFrame);
                if (inputStreamBuffer)
                    return true;
            }
            return mExitThread;
        });

        if (mExitThread) {
            MLOGI(LogGroupHAL, "[EarlyPic]: early process thread exit");
            break;
        }
        MLOGD(LogGroupHAL, "[EarlyPic]: mExitThread %d, mVndAnchorFrames.size:%zu", mExitThread,
              mVndAnchorFrames.size());

        uint32_t snapshotFrame = mVndAnchorFrames.begin()->first;
        uint32_t anchorFrame = mVndAnchorFrames.begin()->second;
        locker.unlock();

        // TODO: frame should be replaced with timestamp
        processAnchorFrameResult(anchorFrame, snapshotFrame, inputStreamBuffer);
    }
}

int CameraMode::processAnchorFrameResult(uint32_t anchorFrame, uint32_t snapshotFrame,
                                         std::shared_ptr<StreamBuffer> inputStreamBuffer)
{
    int ret = 0;
    MLOGI(LogGroupHAL, "[EarlyPic]: poped snapshot vndFrame:%u, try to fill its early pic buffer",
          snapshotFrame);
    ret = reinterpret_cast<AsyncAlgoSession *>(mSession)->processEarlySnapshotResult(
        snapshotFrame, inputStreamBuffer);
    if (ret != 0) {
        MLOGE(LogGroupHAL, "[EarlyPic]: processEarlyJpegResult snapshot vndFrame: %u fail",
              snapshotFrame);
    }

    std::unique_lock<std::mutex> locker{mMutex};
    mVndAnchorFrames.erase(snapshotFrame);
    MLOGD(LogGroupHAL, "[EarlyPic]: remove snapshot frame %u, early pic end", snapshotFrame);
    // NOTE: Becaues we take the chosen buffer in tryGetAnchorFrameBuffer,
    // so we need to return it back so that other executor can use it later
    mInternalFrameTimestamp.insert({0, 0});
    mInternalQuickviewBuffers.insert({0, inputStreamBuffer});

    MLOGI(LogGroupHAL,
          "[EarlyPic]: finish this anchor frame event SUCCESS! picked anchor vndFrame: %u, "
          "snapshot vndFrame: %u, ret: %d",
          anchorFrame, snapshotFrame, ret);
    return ret;
}

bool CameraMode::needForwardEarlypic(uint32_t snapshotFrame, bool isSupportQuickview)
{
    if (isSupportQuickview) {
        std::lock_guard<std::mutex> locker{mMutex};
        auto it = mVndAnchorFrames.find(snapshotFrame);
        if (it != mVndAnchorFrames.end()) {
            MLOGI(LogGroupRT,
                  "[EarlyPic]: snapshot vndFrame: %u, anchor vndFrame: %u, wait process. The "
                  "corresponding executor will not be removed temporarily",
                  it->first, it->second);
            return true;
        }
    }

    return false;
}

void CameraMode::convertAspectRatioToActualResolution(InternalStreamInfo &info,
                                                      uint32_t featureMask,
                                                      const char *optimalSizeTag)
{
    if (info.format != HAL_PIXEL_FORMAT_YCBCR_420_888 && info.format != HAL_PIXEL_FORMAT_RAW16 &&
        info.format != HAL_PIXEL_FORMAT_RAW_OPAQUE && info.format != HAL_PIXEL_FORMAT_RAW10) {
        MLOGE(LogGroupHAL, "format %d does not support get compatible parameter!", info.format);
        return;
    }

    ImageSize finalImageSize = {0, 0};
    bool isFeatureFakeSatSupported = featureMask & FeatureMask::FeatureFakeSat;

    if (isFeatureFakeSatSupported && supportFakeSat(info.roleId) && info.isFakeSat) {
        FakeSatSize size = getFakeSatImageSize(info.roleId, FakeSatType::YUVIN);
        if (size.width == 0 || size.height == 0) {
            MLOGE(LogGroupHAL, "[FakeSat] fail to pase size, check it!");
        } else {
            finalImageSize.width = size.width;
            finalImageSize.height = size.height;
            MLOGD(LogGroupHAL, "[FakeSat] role:%d size(%dx%d)", info.roleId, size.width,
                  size.height);
        }
    } else if (info.isRemosaic && isQcfaSensor()) {
        // NOTE: this case is temporarily dedicated to dealing with HD mode
        finalImageSize = getSuperResolution();
    } else if (!optimalSizeTag) {
        // NOTE: common case
        std::pair<int, int> resolution{};
        if (info.format == HAL_PIXEL_FORMAT_YCBCR_420_888) {
            resolution =
                CameraManager::getInstance()->getMaxYUVResolution(info.cameraId, info.ratio);
            if (getCameraUser() == CameraUser::SDK && info.roleId != CameraDevice::RoleIdRearWide &&
                CameraDevice::RoleIdRearSat == mVendorCamera->getRoleId()) {
                for (auto camera : mVendorCamera->getPhyCameras()) {
                    if (CameraDevice::RoleIdRearWide == camera->getRoleId()) {
                        auto cameraId = static_cast<uint32_t>(atoi(camera->getId().c_str()));
                        std::pair<int, int> resolutionRearWide =
                            CameraManager::getInstance()->getMaxYUVResolution(cameraId, info.ratio);
                        if (resolutionRearWide.first * resolutionRearWide.second >
                            resolution.first * resolution.second) {
                            MLOGI(LogGroupHAL,
                                  "SDK sat force set role:%d yuv output size(%dx%d) to (%dx%d)",
                                  info.roleId, resolution.first, resolution.second,
                                  resolutionRearWide.first, resolutionRearWide.second);
                            resolution = resolutionRearWide;
                        }
                        break;
                    }
                }
            }
        } else {
            resolution = CameraManager::getInstance()->getMaxRawResolution(info.cameraId,
                                                                           info.ratio, info.format);
        }
        finalImageSize.width = resolution.first;
        finalImageSize.height = resolution.second;
    } else {
        // NOTE: this case is temporarily dedicated to dealing with dual bokeh stream
        auto resolutionEntry = mVendorCamera->findInStaticInfo(optimalSizeTag);
        if (resolutionEntry.count > 0) {
            for (int i = 0; i < resolutionEntry.count; i = i + 2) {
                ImageSize imageSize = {
                    .width = static_cast<uint32_t>(resolutionEntry.data.i32[i]),
                    .height = static_cast<uint32_t>(resolutionEntry.data.i32[i + 1]),
                };
                if (imageSize.isSameRatio(info.ratio)) {
                    MLOGD(LogGroupHAL,
                          "[BokehMode]: selected cameraId: %d, width: %d, height: %d, format: %d",
                          info.cameraId, imageSize.width, imageSize.height, info.format);
                    finalImageSize = imageSize;
                }
            }
        }
    }

    // NOTE: fill the compatible stream parameters
    info.width = finalImageSize.width;
    info.height = finalImageSize.height;
}

std::shared_ptr<Stream> CameraMode::createInternalStream(InternalStreamInfo &info)
{
    uint32_t cameraId = info.cameraId;
    uint32_t width = info.width;
    uint32_t height = info.height;
    uint32_t usage = info.usage;
    uint64_t stream_use_case = info.stream_use_case;
    android_dataspace_t space = (android_dataspace_t)info.space;
    int format = info.format;

    if (format != HAL_PIXEL_FORMAT_YCBCR_420_888 && format != HAL_PIXEL_FORMAT_RAW16 &&
        format != HAL_PIXEL_FORMAT_RAW_OPAQUE && format != HAL_PIXEL_FORMAT_RAW10) {
        MLOGI(LogGroupHAL, "format %d does not support creating internal streams", format);
        return nullptr;
    }

    MLOGI(LogGroupHAL, "cameraId %d create internal stream: w x h : %d x %d, format: %d", cameraId,
          width, height, format);

    std::shared_ptr<Stream> stream;
    if (info.isPhysicalStream) {
        stream = LocalStream::create(mVendorCamera, width, height, format, usage, stream_use_case,
                                     space, true, std::to_string(cameraId).c_str());
    } else {
        stream = LocalStream::create(mVendorCamera, width, height, format, usage, stream_use_case,
                                     space, true);
    }

    if (info.isFakeSat) {
        stream->setStreamProp(Stream::REMOSAIC);
    }
    stream->setStreamProp(Stream::SNAPSHOT);

    return stream;
}

Metadata CameraMode::updateOutputSize(const Metadata *metadata, ImageSize imageSize)
{
    Metadata meta = {};
    MLOGD(LogGroupHAL, "imageSize = %d * %d", imageSize.width, imageSize.height);
    if (imageSize.width * imageSize.height != 0) {
        meta.update(MI_MIVI_SNAPSHOT_OUTPUTSIZE, (int32_t *)(&imageSize), 2);
    }

    auto entry = metadata->find(MI_HEICSNAPSHOT_ENABLED);

    bool isHeicSnapshot = false;
    if (entry.count) {
        isHeicSnapshot = entry.data.u8[0];
    }

    if (isHeicSnapshot) {
        entry = {};
        int orientatation = 0;
        entry = metadata->find(ANDROID_JPEG_ORIENTATION);
        if (entry.count > 0) {
            orientatation = entry.data.i32[0];
        }
        if (orientatation == 90 || orientatation == 270) {
            MLOGD(LogGroupHAL, "imageSize = %d * %d", imageSize.height, imageSize.width);
            ImageSize image = {imageSize.height, imageSize.width};
            meta.update(MI_MIVI_SNAPSHOT_OUTPUTSIZE, (int32_t *)(&image), 2);
        }
    }

    entry = {};
    entry = metadata->find(MI_PARAMS_CAPTURE_RATION);
    if (entry.count > 0) {
        uint8_t frameRatio = entry.data.u8[0];
        if (frameRatio == 0) {
            uint32_t width = std::min(imageSize.width, imageSize.height);
            ImageSize image = {width, width};
            MLOGD(LogGroupHAL, "imageSize = %d * %d", image.height, image.width);
            meta.update(MI_MIVI_SNAPSHOT_OUTPUTSIZE, (int32_t *)(&image), 2);
        }
    }

    return meta;
}

void CameraMode::applyDefaultSettingsForSnapshot(std::shared_ptr<CaptureRequest> fwkRequest)
{
    LocalRequest *lr = reinterpret_cast<LocalRequest *>(fwkRequest.get());
    TagValue defalut;
    memset(&defalut, 0, sizeof(TagValue));

    std::map<const char * /* TAG */, int /* Type */> *tagStrings = &deaultTagStrings;
    if (isMiui3rdCapture(fwkRequest))
        tagStrings = &deaultTagStringsfor3rd;

    for (auto tag : (*tagStrings)) {
        switch (tag.second) {
        case TYPE_BYTE:
            lr->updateMeta(tag.first, &defalut.byteTpye, 1);
            break;
        case TYPE_INT32:
            lr->updateMeta(tag.first, &defalut.intType, 1);
            break;
        case TYPE_FLOAT:
            lr->updateMeta(tag.first, &defalut.fType, 1);
            break;
        case TYPE_INT64:
            lr->updateMeta(tag.first, &defalut.longType, 1);
            break;
        case TYPE_DOUBLE:
            lr->updateMeta(tag.first, &defalut.dType, 1);
            break;
        default:
            break;
        }
    }
}

void CameraMode::updateBurstSettings(CaptureRequest *request)
{
    auto entry = request->findInMeta(MI_BURST_CAPTURE_HINT);
    if (entry.count) {
        mBurstMeta = *request->getMeta();
        mIsBurstCapture = entry.data.i32[0];
    } else {
        mBurstMeta = {};
        mIsBurstCapture = false;
    }

    // NOTE: NUWA-17651
    if (mIsBurstCapture) {
        setBufferLimitCnt(0);
    } else if (!mIsBurstCapture) {
        resetBufferLimitCnt();
    }
}

void CameraMode::updateSuperNightSettings(CaptureRequest *request)
{
    uint32_t opMode = mConfigFromFwk->getOpMode();
    if (opMode == StreamConfigModeAlgoDualSAT) {
        auto pureSnapEntry = request->findInMeta(MI_SNAPSHOT_ISPURESNAPSHOT);
        mPureSnapshot = false;
        if (pureSnapEntry.count) {
            mPureSnapshot = pureSnapEntry.data.u8[0];
            MLOGI(LogGroupHAL, "[SN]: outo supernight off : %d", mPureSnapshot);
            // When the timed burst shooting and live photo mode are turned on, or AutoSN is off,
            // the SN/SE should not be triggerred.
        } else {
            MLOGV(LogGroupHAL, "[SN]: couldn't find tag: MI_SNAPSHOT_ISPURESNAPSHOT");
        }
    }
}

void CameraMode::updateSuperNightStatus(CaptureResult *result)
{
    if (isMiviSuperNightSupport()) {
        std::lock_guard<std::mutex> lg(mSuperNightData.mutex);
        auto entryChecker = result->findInMeta(MI_SUPERNIGHT_CHECKER);

        if (entryChecker.count) {
            // may need lock
            mSuperNightData.superNightChecher =
                *reinterpret_cast<const SuperNightChecker *>(entryChecker.data.u8);

            MLOGD(LogGroupRT, "sperNightChecher sum %d", mSuperNightData.superNightChecher.sum);
            for (uint32_t i = 0; i < mSuperNightData.superNightChecher.sum; ++i)
                MLOGD(LogGroupRT, "sperNightChecher ev[%d] %d", i,
                      mSuperNightData.superNightChecher.ev[i]);
        }

        auto entryScene = result->findInMeta(MI_AI_MISD_NONSEMANTICSECENE);
        if (entryScene.count) {
            mSuperNightData.aiMisdScenes =
                *reinterpret_cast<const AiMisdScenes *>(entryScene.data.u8); // index 0 is sn type
            MLOGD(LogGroupRT, "sperNightChecher scene 0x%x", mSuperNightData.aiMisdScenes);
        }
    }
}

bool CameraMode::canDoSuperNight(std::shared_ptr<CaptureRequest> fwkReq)
{
    // NOTE: mSuperNightData is volatile(updated during every preview), fix it into
    // mApplySuperNightData for this snapshot
    {
        std::lock_guard<std::mutex> lg(mSuperNightData.mutex);
        mApplySuperNightData = mSuperNightData;
    }

    // NOTE: check if App metadata support night related snapshot
    bool isFrontCam = mVendorCamera->isFrontCamera();

    // NOTE: for front camera, check "xiaomi.supernight.enabled" tag
    if (isFrontCam) {
        MLOGD(LogGroupHAL,
              "[SN][Front] front camera detected, check if app req supports front night");

        auto entry = fwkReq->getMeta()->find(MI_SUPERNIGHT_ENABLED);
        if (entry.count) {
            uint8_t isFrontNightEnabled = entry.data.u8[0];
            MLOGI(LogGroupHAL, "[SN][Front] app set MI_SUPERNIGHT_ENABLED as %d",
                  isFrontNightEnabled);

            if (isFrontNightEnabled == 0) {
                MLOGI(LogGroupHAL, "[SN][Front] app does not support front night for this req!!");
                return false;
            }
        }
    }

    // NOTE: for rear camera, check other supernight tag
    if (!isFrontCam) {
        auto entry = fwkReq->getMeta()->find(MI_MIVI_NIGHT_MOTION_MODE);
        if (entry.count) {
            auto motionMode = entry.data.i32[0];
            MLOGI(LogGroupHAL, "[SN][MotionCap] app set NIGHT_MOTION_MODE as %d", motionMode);

            // NOTE: night motion mode == 0 means do not do night motion
            if (motionMode == 0) {
                // NOTE: still need to check whether we can do normal rear super night
                entry = fwkReq->getMeta()->find(MI_MIVI_SUPERNIGHT_MODE);
                if (entry.count) {
                    auto nightMode = entry.data.u8[0];
                    MLOGI(LogGroupHAL, "[SN] app set SUPERNIGHT_MODE as %d", nightMode);

                    if (nightMode == MIVISuperNightNone || nightMode == MIVINightMotionHandNonZSL) {
                        MLOGI(LogGroupHAL,
                              "[SN] or [MotionCap] app neither support night motion or super night "
                              "for this fwk request");
                        return false;
                    }
                }
            }
        }
    }

    // NOTE: now app allows night capture, check if checker's info support doing supernight
    bool canCheckerDoNight = canCheckerValDoSuperNight(mApplySuperNightData);

    // NOTE: if canCheckerDoNight == false, then it means checker does not allow super night while
    // app allows. Since app's tag will be sent to vendor and mialgoengine, this discrepency will
    // cause abnormal behavior in these parts. Therefore, we need to reset night related tag to
    // ensure logic integrity.
    if (canCheckerDoNight == false) {
        int32_t motionMode = 0;
        uint8_t nightMode = 0;
        uint8_t frontNightEnabled = 0;
        auto meta = const_cast<Metadata *>(fwkReq->getMeta());

        auto entry = meta->find(MI_SUPERNIGHT_ENABLED);
        if (entry.count) {
            meta->update(MI_SUPERNIGHT_ENABLED, &frontNightEnabled, 1);
        }

        entry = meta->find(MI_MIVI_NIGHT_MOTION_MODE);
        if (entry.count) {
            meta->update(MI_MIVI_NIGHT_MOTION_MODE, &motionMode, 1);
        }

        entry = meta->find(MI_MIVI_SUPERNIGHT_MODE);
        if (entry.count) {
            meta->update(MI_MIVI_SUPERNIGHT_MODE, &nightMode, 1);
        }

        MLOGI(LogGroupHAL,
              "[SN] app supports night motion or super night for this fwk request, but checker "
              "does not support, reset app tag value");
    }
    return canCheckerDoNight;
}

bool CameraMode::canCheckerValDoSuperNight(const SuperNightData &checkerNightVal)
{
    if (mFlashData.flashMode == ANDROID_FLASH_MODE_TORCH) {
        MLOGW(LogGroupHAL, "[SN]: user set flash light on, will not do SN!!");
        return false;
    } else if (mFlashData.flashMode == ANDROID_FLASH_MODE_SINGLE) {
        if (mFlashData.flashState >= ANDROID_FLASH_STATE_FIRED) {
            MLOGW(LogGroupHAL,
                  "[SN]: user set flash light auto and flash light is fired, will not do SN!!");
            return false;
        }
    }

    int snTrigger = NoneType; // 1SN,2ELL,3SNSC,4SNSCNonZSL,5SCstg
    int sceneNum = sizeof(checkerNightVal.aiMisdScenes.aiMisdScene) / sizeof(AiMisdScene);
    for (int i = 0; i < sceneNum; i++) {
        const AiMisdScene *aiMisdScene = checkerNightVal.aiMisdScenes.aiMisdScene + i;

        if (aiMisdScene->type == NightSceneSuperNight && !mPureSnapshot) {
            snTrigger = aiMisdScene->value >> 8;
            if (snTrigger > NoneType && snTrigger <= NormalELLC)
                break;
        } else if (mModeType == MODE_CAPTURE && aiMisdScene->type == SuperNightSceneSNSC) {
            snTrigger = aiMisdScene->value >> 8;
            if (snTrigger >= NormalSNSC && snTrigger <= NormalSNSCStg)
                break;
        }
        snTrigger = 0;
    }

    MLOGI(LogGroupHAL, "[SN]: sn trigger: %d sum: %d", snTrigger,
          checkerNightVal.superNightChecher.sum);
    return (checkerNightVal.superNightChecher.sum > 0) && (snTrigger > 0) &&
           (mAiAsdSceneApplied.load() != SUPERMOON);
}

bool CameraMode::canDoSr()
{
    // NOTE: MotionCapture shouldn't execute SR
    // the value of mMotion have ~450ms delay.
    // if HDR scene, do not disable sr, or will output black picture
    if (mMotion && !canDoHdr()) {
        return false;
    }
    // NOTE: if this camera has tag zoomRatioThresholdToStartSr, then check if the current zoom
    // ratio is in Sr zoom interval
    auto currentZoomRatio = mUIZoomRatio.load();
    // NOTE: if mSrRoleIdToThresholdMap doesn't have corresponding element of current CameraDevice
    // or the zoom ratio threshold in mSrRoleIdToThresholdMap is zero, then we shouldn't do HdrSr
    // you can also refer to: packages/apps/MiuiCamera/src/com/android/camera/Util.java and doc:
    // https://xiaomi.f.mioffice.cn/docs/dock4zC0q7smPaMMzQSKD1bFAeb#15vklW
    int32_t currentCameraRoleId = getRoleIdByCurrentCameraDevice();
    if (!mSrRoleIdToThresholdMap.count(currentCameraRoleId) ||
        mSrRoleIdToThresholdMap[currentCameraRoleId] == 0.0f) {
        MLOGW(LogGroupHAL,
              "[SR]: current camera roleId: %u have no Sr zoom ratio threshold, do not do "
              "Sr!!",
              currentCameraRoleId);
        mIsSrNeeded.store(false);
    } else if (currentZoomRatio < mSrRoleIdToThresholdMap[currentCameraRoleId]) {
        MLOGD(LogGroupHAL,
              "[SR]: current zoom ratio is: %0.1f, doesn't exceed threshold: %0.1f of camera "
              "roleId: %u, do not do Sr!!",
              currentZoomRatio, mSrRoleIdToThresholdMap[currentCameraRoleId], currentCameraRoleId);
        mIsSrNeeded.store(false);
    } else {
        MLOGI(LogGroupHAL,
              "[SR]: current zoom ratio is: %0.1f, exceed threshold: %0.1f of camera roleId: "
              "%u, do Sr!!",
              currentZoomRatio, mSrRoleIdToThresholdMap[currentCameraRoleId], currentCameraRoleId);
        mIsSrNeeded.store(true);
    }

    return mIsSrNeeded.load();
}

bool CameraMode::canDoFakeSat(int roleId)
{
    // check if the current zoom ratio is in FakeSat zoomrange interval
    auto currentZoomRatio = mUIZoomRatio.load();
    auto currentLuxIndex = mLuxIndex.load();
    auto currentShortGain = mShortGain;
    auto sceneApplied = mAiAsdSceneApplied.load();
    int32_t curThermalLevel = mThermalLevel.load();
    bool isMoonMode = (sceneApplied == SUPERMOON);
    FakeSatThreshold pFakeSatThreshold{};
    bool result = false;

    if (mFakeSatThreshold.find(roleId) != mFakeSatThreshold.end()) {
        pFakeSatThreshold = mFakeSatThreshold[roleId];
        if (currentZoomRatio >= pFakeSatThreshold.zoomStart &&
            currentZoomRatio <= pFakeSatThreshold.zoomStop) {
            static int32_t FakeSatDebug =
                property_get_int32("persist.vendor.camera.FakeSat.enable", -1);
            if (FakeSatDebug == 1 || (FakeSatDebug && !isMoonMode && curThermalLevel <= 1 &&
                                      currentLuxIndex <= pFakeSatThreshold.luxIndex &&
                                      currentShortGain <= pFakeSatThreshold.shortGain)) {
                result = true;
            }
        }
        MLOGI(LogGroupHAL,
              "[FakeSat]: enable %d moon: %d thermal: %d zoom ratio: [%0.1f: %0.1f-%0.1f], lux: "
              "[%0.1f: %0.1f], shortGain: [%0.1f: %0.1f]",
              result, isMoonMode, curThermalLevel, currentZoomRatio, pFakeSatThreshold.zoomStart,
              pFakeSatThreshold.zoomStop, currentLuxIndex, pFakeSatThreshold.luxIndex,
              currentShortGain, pFakeSatThreshold.shortGain);
    } else {
        MLOGI(LogGroupHAL, "[FakeSat]:get zoom range failed!");
    }

    return result;
}

void CameraMode::updateFlashSettings(CaptureRequest *request)
{
    // NOTE: read flash light mode
    auto flashLightEntry = request->findInMeta(ANDROID_FLASH_MODE);
    if (flashLightEntry.count) {
        mFlashData.flashMode = flashLightEntry.data.u8[0];
        MLOGI(LogGroupHAL, "[HDR][SN]: user set flash light mode: %d", mFlashData.flashMode);
    } else {
        MLOGV(LogGroupHAL, "[HDR][SN]: couldn't find tag: ANDROID_FLASH_MODE");
    }
}

void CameraMode::updateFlashStatus(CaptureResult *result)
{
    // NOTE: read flash status, if flash is on, then we need to mute HDR
    auto flashEntry = result->findInMeta(ANDROID_FLASH_STATE);
    if (flashEntry.count > 0) {
        mFlashData.flashState = flashEntry.data.u8[0];
        MLOGI(LogGroupRT, "[HDR][SN]: flashState: %d", mFlashData.flashState);
    } else {
        MLOGI(LogGroupRT, "[HDR][SN]: didn't find tag: android.flash.state");
    }
}

void CameraMode::updateHdrSettings(CaptureRequest *request)
{
    auto hdrModeEntry = request->findInMeta(MI_HDR_MODE);
    if (!hdrModeEntry.count)
        return;

    uint8_t oldMode = mHdrMode.exchange(hdrModeEntry.data.u8[0]);
    uint8_t newMode = mHdrMode.load();
    if (newMode == oldMode)
        return;

    MLOGI(LogGroupHAL, "[HDR]: set HDR Mode:%d, Old Mode:%d", newMode, oldMode);
    uint8_t asdEnable = 0;
    uint8_t checkEnable = 0;
    if (newMode == 1 || newMode == 2) {
        asdEnable = 1;
        checkEnable = 1;
    }

    LocalRequest *lq = reinterpret_cast<LocalRequest *>(request);
    lq->updateMeta(MI_ASD_ENABLE, &asdEnable, 1);
    lq->updateMeta(MI_HDR_CHECKER_ENABLE, &checkEnable, 1);
}

void CameraMode::updateHdrStatus(CaptureResult *result)
{
    // NOTE: first update hdr checker result, hdr checker will report whether it has detected hdr
    // scenes and report ev values
    auto entry = result->findInMeta(MI_HDR_CHECKER_RESULT);
    if (!entry.count) {
        MLOGW(LogGroupRT, "[HDR]: didn't find hdr checker result");
        return;
    }

    std::vector<int32_t> expValues = {};
    bool hdrDetected = false;

    // NOTE: Hdr num on sat may be 3, but it may be 2 on bokeh+hdr, only determined by checker
    int minNum;
    MiviSettings::getVal("FeatureList.FeatureHDR.MinNumOfSnapshotRequiredByHDR", minNum, 2);

    int32_t hdrNum = entry.data.i32[0];
    MLOGD(LogGroupRT, "[HDR]: hdr ev num: %d", hdrNum);
    if (entry.data.i32[0] < minNum) {
        MLOGW(LogGroupRT,
              "[HDR]: checker only returns %d ev vals, but required at least %d ev vals", hdrNum,
              minNum);
        return;
    }

    for (int i = 0; i < hdrNum; i++)
        expValues.push_back(entry.data.i32[i + 1]);

    // NOTE: the tag srhdrRequestNumber indicates how many requests for each ev value should be sent
    // to Qcom doc: https://xiaomi.f.mioffice.cn/docs/dock4zC0q7smPaMMzQSKD1bFAeb# mivi1.0 app code:
    // packages/apps/MiuiCamera/src/com/android/camera2/vendortag/CaptureResultVendorTags.java
    std::vector<int32_t> reqNumPerEv{};
    auto srRequestNumEntry = result->findInMeta(MI_HDR_SR_REQUEST_NUM);
    if (srRequestNumEntry.count > 0) {
        for (int i = 0; i < hdrNum; ++i) {
            reqNumPerEv.push_back(srRequestNumEntry.data.u8[i]);
            MLOGI(LogGroupRT, "[HDR][SR]: ev[%d] req num: %d", i, reqNumPerEv[i]);
        }
    } else {
        MLOGI(LogGroupRT,
              "[HDR][SR]: ev[%d] checker of this platform do not have tag: "
              "xiaomi.hdr.srhdrRequestNumber");
    }

    // NOTE: this tag is used to decide which ev val's corresponding request should take mfnr tag to
    // Qcom when do HDR you can refer to doc:
    // https://xiaomi.f.mioffice.cn/docs/dock4zC0q7smPaMMzQSKD1bFAeb#
    std::vector<int32_t> reqMfnrSetting{};
    auto hdrReqSettingEntry = result->findInMeta(MI_HDR_REQ_SETTING);
    if (hdrReqSettingEntry.count > 0) {
        for (int i = 0; i < hdrNum; ++i) {
            reqMfnrSetting.push_back(hdrReqSettingEntry.data.i32[i]);
            MLOGI(LogGroupRT, "[HDR]: ev[%d]'s mfnr setting: %d", i, reqMfnrSetting[i]);
        }
    } else {
        MLOGI(LogGroupRT, "[HDR]: do not have tag MI_HDR_REQ_SETTING");
    }

    // NOTE: read hdr checker scene type
    int32_t hdrSceneType = 0;
    auto hdrSceneEntry = result->findInMeta(MI_HDR_CHECKER_SCENE_TYPE);
    if (hdrSceneEntry.count > 0) {
        hdrSceneType = hdrSceneEntry.data.i32[0];
        MLOGI(LogGroupRT, "[HDR]: hdr scene type: %d", hdrSceneType);
    } else {
        MLOGW(LogGroupRT, "[HDR]: didn't find hdr scene type in preview result");
    }

    // NOTE: read hdr checker adrc
    int32_t hdrAdrc = 0;
    auto hdrAdrcEntry = result->findInMeta(MI_HDR_CHECKER_ADRC);
    if (hdrAdrcEntry.count > 0) {
        hdrAdrc = hdrAdrcEntry.data.i32[0];
        MLOGI(LogGroupRT, "[HDR]: hdr adrc: %d", hdrAdrc);
    } else {
        MLOGW(LogGroupRT, "[HDR]: didn't find hdr adrc in preview result");
    }

    // NOTE: check whether it is zsl hdr
    int32_t isZslHdr = 0;
    auto hdrZslEntry = result->findInMeta(MI_HDR_ZSL);
    if (hdrZslEntry.count > 0) {
        isZslHdr = hdrZslEntry.data.i32[0];
        MLOGI(LogGroupRT, "[HDR]: is zsl hdr: %d", isZslHdr);
    } else {
        MLOGW(LogGroupRT, "[HDR]: didn't find zsl hdr tag in preview result");
    }

    // NOTE: checker report whether it has detected hdr scenes
    entry = result->findInMeta(MI_HDR_DETECTED);
    if (entry.count) {
        hdrDetected = entry.data.u8[0];
    }

    // NOTE: read thermal level
    int32_t hdrHighThermalLevel = 0;
    auto thermalEntry = result->findInMeta(MI_HDR_HIGH_THERMAL_DETECTED);
    if (!thermalEntry.count) {
        MLOGW(LogGroupRT, "[HDR]: didn't find hdr high thermal tag");
    } else {
        hdrHighThermalLevel = thermalEntry.data.i32[0];
        MLOGD(LogGroupRT, "[HDR]: is hdr high thermal: %d", thermalEntry.data.i32[0]);
    }

    // NOTE: read motion
    bool hdrMotionDetected = false;
    auto motionEntry = result->findInMeta(MI_HDR_MOTION_DETECTED);
    if (!motionEntry.count) {
        MLOGW(LogGroupRT, "[HDR]: didn't find motion detection tag");
    } else {
        hdrMotionDetected = motionEntry.data.u8[0];
        MLOGD(LogGroupRT, "[HDR]: is hdr motion detected: %d", motionEntry.data.u8[0]);
    }

    // NOTE: update hdr related vaiables
    std::lock_guard<std::mutex> lock(mHdrStatusLock);
    entry = result->findInMeta(MI_AI_ASD_SNAPSHOT_REQ_INFO);
    if (entry.count) {
        if (!mHdrData.snapshotReqInfo.size) {
            mHdrData.snapshotReqInfo.size = entry.count;
            mHdrData.snapshotReqInfo.data = (uint8_t *)malloc(entry.count);
            memset(mHdrData.snapshotReqInfo.data, 0, entry.count);
        } else if (mHdrData.snapshotReqInfo.size != entry.count) {
            MLOGE(LogGroupHAL, "SnapshotReqInfo count needs to be checked");
            MASSERT(false);
        }
        memcpy(mHdrData.snapshotReqInfo.data, entry.data.u8, entry.count);
    }

    if (isFeatureRawHdrSupported()) {
        // raw hdr
        entry = result->findInMeta(MI_HDR_RAW_CHECKER_RESULT);
        if (!entry.count) {
            MLOGW(LogGroupRT, "[HDR]: didn't find raw hdr checker result");
            return;
        }

        int32_t rawHdrNum = entry.data.i32[0];
        std::vector<int32_t> rawExpValues = {};
        MLOGD(LogGroupRT, "[HDR]: raw hdr ev num: %d", rawHdrNum);
        if (entry.data.i32[0] < minNum) {
            MLOGW(LogGroupRT,
                  "[HDR]: raw checker only returns %d ev vals, but required at least %d ev vals",
                  rawHdrNum, minNum);
            return;
        }
        for (int i = 0; i < rawHdrNum; i++)
            rawExpValues.push_back(entry.data.i32[i + 1]);

        mHdrData.hdrStatus.rawExpVaules = rawExpValues;
    }

    mHdrData.hdrStatus.expVaules = expValues;
    mHdrData.hdrStatus.hdrDetected = hdrDetected;
    mHdrData.hdrHighThermalLevel = hdrHighThermalLevel;
    mHdrData.isZslHdrEnabled = isZslHdr;
    mHdrData.hdrMotionDetected = hdrMotionDetected;
    mHdrData.hdrSceneType = hdrSceneType;
    mHdrData.hdrAdrc = hdrAdrc;
    mHdrData.reqNumPerEv = reqNumPerEv;
    mHdrData.reqMfnrSetting = reqMfnrSetting;
    MLOGD(LogGroupRT, "[HDR]: hdrDetected: %d", hdrDetected);
}

bool CameraMode::canDoHdr()
{
    std::lock_guard<std::mutex> lock(mHdrStatusLock);
    // NOTE: user set hdr off
    if (!mHdrMode.load()) {
        MLOGD(LogGroupHAL, "[HDR]: user set hdr off, will not do hdr!!");
        return false;
    }

    // NOTE: Hdr checker didn't detect hdr scenes
    if (!mHdrData.hdrStatus.hdrDetected) {
        MLOGD(LogGroupHAL, "[HDR]: hdr checker didn't detect hdr scenes, will not do hdr!!");
        return false;
    }

    // NOTE: if in moon mode, then don't do hdr
    if (mHdrData.hdrMotionDetected) {
        MLOGD(LogGroupHAL, "[HDR]: hdr motion detected, will not do hdr!!");
        return false;
    }

    // NOTE: if temperature is high, then don't do hdr
    if (mHdrData.hdrHighThermalLevel > 0) {
        MLOGW(LogGroupHAL, "[HDR]: phone is in high temperature, will not do hdr!!");
        return false;
    }

    // if you are brave enough, you can refer to mivi1.0 app code:
    // packages/apps/MiuiCamera/src/com/android/camera/module/Camera2Module.java
    // or refer to GaoChuan
    if (mFlashData.flashMode == ANDROID_FLASH_MODE_TORCH) {
        MLOGW(LogGroupHAL, "[HDR]: user set flash light on, will not do hdr!!");
        return false;
    }

    mApplyHdrData = mHdrData;

    MLOGI(LogGroupHAL, "[HDR]: this snapshot will do hdr!!");
    return true;
}

bool CameraMode::canDoHdrSr()
{
    if (!canDoSr()) {
        return false;
    }

    if (mHdrMode.load() == HDR_OFF) {
        return false;
    }

    // NOTE: if this camera do not support tag zoomRatioThresholdToStartSr
    if (mSrRoleIdToThresholdMap.empty()) {
        return mHdrMode.load() == HDR_AUTO && mIsSrNeeded.load() && mHdrSrDetected.load();
    }

    return true;
}

bool CameraMode::isSupportQuickShot(int type, std::shared_ptr<CaptureRequest> fwkRequest) const
{
    if ((mModeType != MODE_CAPTURE && mModeType != MODE_PORTRAIT &&
         mModeType != MODE_PROFESSIONAL) ||
        mLlsNeed) {
        return false;
    }
    int mask = mVendorCamera->getQuickShotSupportMask();
    bool isFrontCamera = mVendorCamera->isFrontCamera();
    bool isSupport = false;
    bool isMacroLens = isMarcoMode(fwkRequest);
    if (mModeType == MODE_PORTRAIT) {
        if (isFrontCamera) {
            isSupport = mask & CameraDevice::QS_SUPPORT_FRONT_BOKEH;
        } else {
            if (type == Photographer::Photographer_BOKEH_HDR)
                isSupport = mask & CameraDevice::QS_SUPPORT_BACK_BOKEH_HDR;
            else if (type == Photographer::Photographer_BOKEH_SE) {
                isSupport = mask & CameraDevice::QS_SUPPORT_SAT_SUPER_NIGHT;
            } else {
                isSupport = mask & CameraDevice::QS_SUPPORT_BACK_BOKEH_MFNR;
            }
        }
    } else if (mModeType == MODE_PROFESSIONAL) {
        isSupport = mask & CameraDevice::QS_SUPPORT_PRO_MODE;
    } else {
        if (type == Photographer::Photographer_SR) {
            if (getRoleIdByCurrentCameraDevice() == CameraDevice::RoleIdRearWide) {
                isSupport = true;
            } else {
                isSupport = mask & CameraDevice::QS_SUPPORT_SAT_SR;
            }
        } else if (type == Photographer::Photographer_MIHAL_MFNR ||
                   type == Photographer::Photographer_VENDOR_MFNR) {
            if (isMacroLens)
                isSupport = mask & CameraDevice::QS_SUPPORT_MACRO_MODE;
            else
                isSupport = mask & CameraDevice::QS_SUPPORT_SAT_MFNR;
        } else if (type == Photographer::Photographer_HDR) {
            isSupport = mask & (isFrontCamera ? CameraDevice::QS_SUPPORT_FRONT_HDR
                                              : CameraDevice::QS_SUPPORT_SAT_HDR);
        } else if (type == Photographer::Photographer_SN) {
            if (!isFrontCamera) {
                isSupport = mask & CameraDevice::QS_SUPPORT_SAT_SUPER_NIGHT;
            }
        } else if (type == Photographer::Photographer_HDRSR) {
            isSupport = mask & CameraDevice::QS_SUPPORT_SAT_HDRSR;
        } else if (type == Photographer::Photographer_BURST) {
            isSupport = false;
        } else {
            isSupport = mask & (isFrontCamera ? CameraDevice::QS_SUPPORT_FRONT_MFNR
                                              : CameraDevice::QS_SUPPORT_SAT_MFNR);
        }
    }
    return isSupport;
}

uint64_t CameraMode::getQuickShotDelayTime(int type) const
{
    bool isFrontCamera = mVendorCamera->isFrontCamera();
    uint64_t mask = mVendorCamera->getQuickShotDelayTimeMask();
    uint64_t delayTime = 0;
    uint32_t baseMs = 100;
    if (isFrontCamera) {
        if (mModeType == MODE_PORTRAIT) {
            delayTime = ((mask & CameraDevice::QS_FRONT_BOKEH_MFNR_DELAY_TIME) >> 4) * baseMs;
        } else {
            if (type == Photographer::Photographer_HDR) {
                delayTime = ((mask & CameraDevice::QS_FRONT_HDR_DELAY_TIME) >> 12) * baseMs;
            } else {
                delayTime =
                    ((mask & CameraDevice::QS_FRONT_NORMAL_CAPTURE_DELAY_TIME) >> 28) * baseMs;
            }
        }
    } else {
        if (mModeType == MODE_PORTRAIT) {
            if (type == Photographer::Photographer_BOKEH_HDR) {
                delayTime = ((mask & CameraDevice::QS_BACK_BOKEH_HDR_DELAY_TIME) >> 36) * baseMs;
            } else {
                delayTime = (mask & CameraDevice::QS_BACK_BOKEH_MFNR_DELAY_TIME) * baseMs;
            }
        } else if (mModeType == MODE_PROFESSIONAL) {
            delayTime = ((mask & CameraDevice::QS_BACK_PRO_MODE_DELAY_TIME) >> 44) * baseMs;
        } else {
            if (type == Photographer::Photographer_SR) {
                delayTime = ((mask & CameraDevice::QS_BACK_SR_CAPTURE_DELAY_TIME) >> 24) * baseMs;
            } else if (type == Photographer::Photographer_HDRSR) {
                delayTime = ((mask & CameraDevice::QS_BACK_HDRSR_DELAY_TIME) >> 40) * baseMs;
            } else if (type == Photographer::Photographer_HDR) {
                delayTime = ((mask & CameraDevice::QS_BACK_HDR_DELAY_TIME) >> 16) * baseMs;
            } else if (type == Photographer::Photographer_SN) {
                delayTime =
                    ((mask & CameraDevice::QS_BACK_SUPER_NIGHT_SE_DELAY_TIME) >> 20) * baseMs;
            } else {
                delayTime =
                    ((mask & CameraDevice::QS_BACK_NORMAL_CAPTURE_DELAY_TIME) >> 8) * baseMs;
            }
        }
    }

    MLOGV(LogGroupHAL, "[MiviQuickShot] type %d mask 0x%x, delayTime %" PRId64, type, mask,
          delayTime);
    return delayTime;
}

std::vector<int> CameraMode::getOverrideStreamFormat(int roleId, int phgType)
{
    if (mCameraRoleConfigInfoMap.empty()) {
        return std::vector<int>{};
    }

    auto it = mCameraRoleConfigInfoMap.find(roleId);
    if (it != mCameraRoleConfigInfoMap.end()) {
        auto configInfo = it->second;
        auto itr = configInfo.phgStreamFormatMap.find(phgType);
        if (itr != configInfo.phgStreamFormatMap.end()) {
            return itr->second;
        }
    }

    return std::vector<int>{};
}

bool CameraMode::supportFakeSat(int roleId)
{
    if (mCameraRoleConfigInfoMap.empty()) {
        return false;
    }

    auto it = mCameraRoleConfigInfoMap.find(roleId);
    if (it != mCameraRoleConfigInfoMap.end()) {
        auto configInfo = it->second;
        return configInfo.supportFakeSat;
    }

    return false;
}

FakeSatSize CameraMode::getFakeSatImageSize(uint32_t roleId, uint32_t type)
{
    FakeSatSize fakeSatSize{};
    if (mCameraRoleConfigInfoMap.empty()) {
        return fakeSatSize;
    }

    auto it = mCameraRoleConfigInfoMap.find(roleId);
    if (it != mCameraRoleConfigInfoMap.end()) {
        auto configInfo = it->second;
        if (!mFakeSatThreshold.count(roleId)) {
            mFakeSatThreshold.insert({roleId, configInfo.fakeSatInfo.fakeSatThreshold});
        }

        for (auto &ratioSize : configInfo.fakeSatInfo.ratioSizeMap) {
            float ratio = ratioSize.first;
            float tolerance = 0.03f;
            if (fabsf(ratio - mFrameRatio) <= tolerance) {
                auto iter = ratioSize.second.find(type);
                if (iter != ratioSize.second.end()) {
                    return iter->second;
                }
            }
        }
    }

    return fakeSatSize;
}

void CameraMode::initUIRelatedMetaList()
{
    auto entry = mVendorCamera->findInStaticInfo(MI_MIVI_UI_RELATED_METAS);
    if (!entry.count) {
        MLOGW(LogGroupHAL, "get ui_related_metas failed!");
    } else {
        UIRelatedMetas metas = *reinterpret_cast<const UIRelatedMetas *>(entry.data.u8);
        /*metas = {3,
                 {"xiaomi.supernight.checker", "xiaomi.ai.misd.NonSemanticScene",
                  "xiaomi.ai.asd.asdExifInfo"}};*/

        uint32_t count = metas.count;
        for (int i = 0; i < count; ++i) {
            std::string tagName{metas.tagInfo[i]};
            MLOGI(LogGroupHAL, "UIRelatedMeta %s", tagName.c_str());
            mUIRelatedMetaList.push_back(tagName);
        }
    }
}

void CameraMode::updateUIRelatedSettins(CaptureRequest *request)
{
    LocalRequest *lq = reinterpret_cast<LocalRequest *>(request);
    std::string snEV = "snEV:", snTrigger = "snTriggerType:Normal-None snTrigger:0";

    for (auto iter : mUIRelatedMetaList) {
        std::string tagName = iter;
        auto entry = request->findInMeta(tagName.c_str());
        if (entry.count) {
            if (!tagName.compare(MI_SUPERNIGHT_CHECKER)) {
                std::lock_guard<std::mutex> lg(mSuperNightData.mutex);

                mSuperNightData.superNightChecher =
                    *reinterpret_cast<const SuperNightChecker *>(entry.data.u8);
                MLOGI(LogGroupHAL, "[SN] sperNightChecher sum %d",
                      mSuperNightData.superNightChecher.sum);
                for (uint32_t i = 0; i < mSuperNightData.superNightChecher.sum; ++i) {
                    MLOGI(LogGroupHAL, "[SN] sperNightChecher ev[%d] %d", i,
                          mSuperNightData.superNightChecher.ev[i]);
                    snEV += (" " + std::to_string(mSuperNightData.superNightChecher.ev[i]));
                }

                const char *tag = tagName.c_str();
                auto meta = std::make_unique<Metadata>(lq->getMeta()->peek());
                meta->erase(tag);
            } else if (!tagName.compare(MI_AI_MISD_NONSEMANTICSECENE)) {
                std::lock_guard<std::mutex> lg(mSuperNightData.mutex);

                snTrigger = "snTriggerType:";
                mSuperNightData.aiMisdScenes =
                    *reinterpret_cast<const AiMisdScenes *>(entry.data.u8);
                MLOGI(LogGroupHAL, "[SN] sperNightChecher scene 0x%x",
                      mSuperNightData.aiMisdScenes);

                switch (mModeType) {
                case MODE_SUPER_NIGHT:
                    snTrigger += "Night";
                    break;
                case MODE_CAPTURE:
                    snTrigger += "Normal";
                    break;
                case MODE_PORTRAIT:
                    snTrigger += "Bokeh";
                    break;
                default:
                    snTrigger += "Other";
                    break;
                }

                snTrigger += "-";

                int sceneNum =
                    sizeof(mSuperNightData.aiMisdScenes.aiMisdScene) / sizeof(AiMisdScene);
                for (int i = 0; i < sceneNum; i++) {
                    AiMisdScene *aiMisdScene = mSuperNightData.aiMisdScenes.aiMisdScene + i;
                    if (aiMisdScene->type == NightSceneSuperNight ||
                        aiMisdScene->type == SuperNightSceneSNSC) {
                        switch (aiMisdScene->value >> 8) {
                        case NormalSE:
                            snTrigger += (std::string("SN") + std::string(" snTrigger:") +
                                          std::to_string(NormalSE));
                            break;
                        case NormalELLC:
                            snTrigger += (std::string("ELL") + std::string(" snTrigger:") +
                                          std::to_string(NormalELLC));
                            break;
                        case NormalSNSC:
                        case NormalSNSCNonZSL:
                            snTrigger += (std::string("SNSC") + std::string(" snTrigger:") +
                                          std::to_string(aiMisdScene->value >> 8));
                            break;
                        default:
                            snTrigger += (std::string("None") + std::string(" snTrigger:") +
                                          std::to_string(aiMisdScene->value >> 8));
                            break;
                        }
                        break;
                    } else {
                        snTrigger += (std::string("None") + std::string(" snTrigger:") +
                                      std::to_string(aiMisdScene->value >> 8));
                    }
                }

                if (sceneNum <= 0)
                    snTrigger +=
                        (std::string("None") + std::string(" snTrigger:") + std::to_string(0));

                const char *tag = tagName.c_str();
                auto meta = std::make_unique<Metadata>(lq->getMeta()->peek());
                meta->erase(tag);

            } else if (!tagName.compare(MI_AI_ASD_ASD_EXIF_INFO)) {
                std::string exifInfo;
                {
                    std::lock_guard<std::mutex> lg(mASDInfo.mutex);
                    exifInfo = std::move(mASDInfo.exifInfo);
                }
                if (exifInfo.length()) {
                    std::size_t snEVLoc = exifInfo.find("snEV:");
                    std::size_t aiSceneLoc = exifInfo.find(" aiScene:");
                    if (snEVLoc != std::string::npos && aiSceneLoc != std::string::npos &&
                        aiSceneLoc > snEVLoc) {
                        MLOGI(LogGroupHAL, "[HDR][SN]: update asdExifInfo %s", snEV.c_str());
                        exifInfo.replace(exifInfo.find("snEV:"), (aiSceneLoc - snEVLoc), snEV);
                    }

                    std::size_t snTriggerLoc = exifInfo.find("snTriggerType:");
                    std::size_t algoOutSELoc = exifInfo.find(" algoOutSE:");
                    if (snTriggerLoc != std::string::npos && algoOutSELoc != std::string::npos &&
                        algoOutSELoc > snTriggerLoc) {
                        MLOGI(LogGroupHAL, "[HDR][SN]: update asdExifInfo %s", snTrigger.c_str());
                        exifInfo.replace(exifInfo.find("snTriggerType:"),
                                         (algoOutSELoc - snTriggerLoc), snTrigger);
                    }

                    // "preview_asd" from App Async Tag updated sn related info
                    MLOGI(LogGroupRT, "[HDR][SN]: ASDExifInfo %s", exifInfo.c_str());

                    lq->updateMeta(
                        MI_AI_ASD_ASD_EXIF_INFO,
                        reinterpret_cast<uint8_t *>(const_cast<char *>(exifInfo.c_str())),
                        exifInfo.length() + 1);
                } else {
                    MLOGW(LogGroupHAL,
                          "[HDR][SN]: No previous ASDExifInfo, preview_asd from App request");
                }
                // If App Async Tag do not have ASDExifInfoTag, preview_asd use capture result Tag.
            } else {
                MLOGW(LogGroupHAL, "UnKnown tag %s, please check", tagName.c_str());
            }
        } else {
            MLOGW(LogGroupHAL, "App request get %s failed", tagName.c_str());
        }
    }
}

ImageSize CameraMode::getSuperResolution()
{
    // NOTE: cache resolution result to avoid duplicated calculation
    static std::map<CameraDevice *, ImageSize> sCachedResult;

    if (sCachedResult.count(mVendorCamera)) {
        auto size = sCachedResult[mVendorCamera];
        MLOGD(LogGroupHAL, "get cached super resolution w*d: [%d * %d]", size.width, size.height);
        return size;
    }

    ImageSize imageSize;
    MiviSettings::getVal("FeatureList.FeatureHD.SuperResolutionImageRatio.width", imageSize.width,
                         4);
    MiviSettings::getVal("FeatureList.FeatureHD.SuperResolutionImageRatio.height", imageSize.height,
                         3);
    camera_metadata_ro_entry entry =
        mVendorCamera->findInStaticInfo(MI_SCALER_AVAILABLE_SUPERRESOLUTION_STREAM_CONFIGURATIONS);

    if (entry.count <= 0) {
        MLOGE(LogGroupHAL, "didn't find super resolution");
        return imageSize;
    }

    ImageSize largestSizeStrictRatioRestrict = {0, 0};
    ImageSize largestSizeLooseRatioRestrict = {0, 0};

    for (int i = 0; i < entry.count; i += 4) {
        int32_t format = entry.data.i32[i];
        if (format != HAL_PIXEL_FORMAT_YCBCR_420_888)
            continue;

        uint32_t width = static_cast<uint32_t>(entry.data.i32[i + 1]);
        uint32_t height = static_cast<uint32_t>(entry.data.i32[i + 2]);
        int32_t dir = entry.data.i32[i + 3];

        // NOTE: we only focus on output size
        if (dir != 0) {
            continue;
        }

        ImageSize image = {width, height};
        MLOGI(LogGroupHAL, "iterate size: w*d: [%d * %d]", image.width, image.height);

        // NOTE: find largest size with best frame ratio, ratio precision is 0.001
        if (imageSize.isSameRatio(static_cast<float>(width) / height, 3) &&
            largestSizeStrictRatioRestrict.height <= image.height &&
            largestSizeStrictRatioRestrict.width <= image.width) {
            largestSizeStrictRatioRestrict = image;
        }

        // NOTE: find largest size with loose frame ratio restriction, ratio precision is only
        // 0.1
        if (imageSize.isSameRatio(static_cast<float>(width) / height, 1) &&
            largestSizeLooseRatioRestrict.height <= image.height &&
            largestSizeLooseRatioRestrict.width <= image.width) {
            largestSizeLooseRatioRestrict = image;
        }
    }

    // NOTE: if we find high frame ratio precision result and this result does not differ too much
    // from its loose frame ratio counterpart(to filter out 1440X1080 case), then we use it
    if ((largestSizeStrictRatioRestrict.width * largestSizeStrictRatioRestrict.height != 0) &&
        isSizeSimilar(largestSizeStrictRatioRestrict.width, largestSizeStrictRatioRestrict.height,
                      largestSizeLooseRatioRestrict.width, largestSizeLooseRatioRestrict.height)) {
        // NOTE: use high frame ratio precision result first
        imageSize = largestSizeStrictRatioRestrict;
        MLOGI(LogGroupHAL, "use high frame ratio precision super resolution result: w*d: [%d * %d]",
              imageSize.width, imageSize.height);
    } else {
        imageSize = largestSizeLooseRatioRestrict;
        MLOGI(LogGroupHAL, "use low frame ratio precision super resolution result: w*d: [%d * %d]",
              imageSize.width, imageSize.height);
    }

    if (imageSize.width * imageSize.height == 0) {
        MLOGE(LogGroupHAL, "get super resolution error");
    } else {
        MLOGI(LogGroupHAL, "super resolution w*d: [%d * %d]", imageSize.width, imageSize.height);
    }

    sCachedResult[mVendorCamera] = imageSize;
    return imageSize;
}

void CameraMode::updataASDExifInfo(CaptureResult *result)
{
    auto entry = result->findInMeta(MI_AI_ASD_ASD_EXIF_INFO);
    if (!entry.count) {
        MLOGW(LogGroupRT, "[HDR][SN]: preview result get asdExifInfo failed");
        return;
    } else {
        std::lock_guard<std::mutex> lg(mASDInfo.mutex);
        mASDInfo.exifInfo = *reinterpret_cast<const ASDExifInfo *>(entry.data.u8);
        MLOGI(LogGroupRT, "[HDR][SN]: preview result ASDExifInfo %s", mASDInfo.exifInfo.c_str());
    }
}

} // namespace mihal

bool CameraMode::isRemosaicNeeded() const
{
    auto currentLuxIndex = mLuxIndex.load();
    auto currentShortGain = mShortGain;
    auto currentDarkRatio = mDarkRatio.load();
    auto currentAdrcGain = mAdrcGain.load();
    auto currentExposureTime = mExposureTime.load();
    auto currentFlashMode = mFlashMode.load();
    const uint64_t NanoSecondsPerMilliSecond = 1000000ULL;
    uint64_t exposureTimeThreshold = 125 * NanoSecondsPerMilliSecond;
    bool result = false;

    if (mCameraRoleConfigInfoMap.empty()) {
        MLOGW(LogGroupHAL, "[SuperHD]mCameraRoleConfigInfoMap is NULL!");
        return false;
    }

    static int32_t SuperHDDebug = property_get_int32("persist.vendor.camera.REMOSAIC.enable", -1);

    uint32_t roleId = static_cast<uint32_t>(getRoleIdByCurrentCameraDevice());
    auto it = mCameraRoleConfigInfoMap.find(roleId);

    if (SuperHDDebug == 0) {
        result = false;
    } else if (SuperHDDebug == 1) {
        result = true;
    } else if (it != mCameraRoleConfigInfoMap.end()) {
        const XCFASnapshotThreshold &pXCFASnapshotThreshold = it->second.xcfaSnapshotThreshold;

        MLOGI(LogGroupHAL,
              "[SuperHD_Remosaic]: lux: [%0.1f : %0.1f], shortGain: [%0.1f : %0.1f], adrcGain: "
              "[%0.1f : %0.1f], darkRatio: [%0.2f: %0.2f], exposureTime: [%u: %u], FlashMode: [%d]",
              currentLuxIndex, pXCFASnapshotThreshold.luxIndex, currentShortGain,
              pXCFASnapshotThreshold.shortGain, currentAdrcGain, pXCFASnapshotThreshold.adrcGain,
              currentDarkRatio, pXCFASnapshotThreshold.darkRatio, currentExposureTime,
              exposureTimeThreshold, currentFlashMode);

        if ((pXCFASnapshotThreshold.flashModeSingle == -1
                 ? 1
                 : currentFlashMode != pXCFASnapshotThreshold.flashModeSingle) &&
            (pXCFASnapshotThreshold.flashModeTorch == -1
                 ? 1
                 : currentFlashMode != pXCFASnapshotThreshold.flashModeTorch) &&
            (currentExposureTime < exposureTimeThreshold) &&
            (pXCFASnapshotThreshold.luxIndex == -1
                 ? 1
                 : currentLuxIndex <= pXCFASnapshotThreshold.luxIndex) &&
            (pXCFASnapshotThreshold.shortGain == -1
                 ? 1
                 : currentShortGain <= pXCFASnapshotThreshold.shortGain) &&
            (pXCFASnapshotThreshold.adrcGain == -1
                 ? 1
                 : (currentAdrcGain <= pXCFASnapshotThreshold.adrcGain ||
                    (pXCFASnapshotThreshold.darkRatio == -1
                         ? -1
                         : currentDarkRatio <= pXCFASnapshotThreshold.darkRatio)))) {
            result = true;
        }
    } else {
        MLOGI(LogGroupHAL, "[SuperHD_Remosaic]:get Threshold failed!");
    }

    MLOGI(LogGroupHAL, "[SuperHD_Remosaic]: enable remosaic = %d, SuperHDDebug = %d, roleId = %d",
          result, SuperHDDebug, roleId);

    return result;
}
