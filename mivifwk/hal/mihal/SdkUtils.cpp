#include "SdkUtils.h"

#include <VendorMetadataParser.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <map>
#include <streambuf>

#include "BGService.h"
#include "Metadata.h"
#include "Photographer.h"
#include "rapidjson/Document.h"
#include "rapidjson/StringBuffer.h"
#include "rapidjson/Writer.h"
#include "rapidjson/error/En.h"

using namespace rapidjson;

namespace {
// please check vendor/qcom/proprietary/chi-cdk/core/chiframework/oem/chxcameraextension.cpp
// {PATH_MIVI_INFO} and {PATH_MIVI_INFO_DEFAULT}
const char *ecoCloudInfoPathDefault = "/odm/etc/camera/xiaomi/EcoCloudInfo.json";

static const uint32_t DCENABLEDINDEX = 1;
static const uint32_t LDCENABLEDINDEX = 1 << 1;
static const uint32_t LDCPREVIEWENABLEDINDEX = 1 << 2;
static const uint32_t GrallocUsageHwVideoEncoder = 0x10000;
static const uint32_t HAL_PIXEL_FORMAT_YCbCr_420_P010 = 0x36;
static const uint32_t GrallocUsage10Bit = 0x40000000;    ///< 10 bit usage flag
static const uint32_t GrallocUsagePrivate0 = 0x10000000; ///< UBWC usage flag

static const char *MISdkTagGroup[] = {
    SDK_HDR_MODE,                 // MISdkTag::HdrMode
    SDK_BEAUTY_SKINSMOOTH,        // MISdkTag::SkinSmoothRatio
    SDK_BEAUTY_SLIMFACE,          // MISdkTag::SlimFaceRatio
    SDK_BOKEH_ENABLE,             // MISdkTag::BokehEnable
    SDK_MFNR_ENABLE,              // MISdkTag::MfnrEnable
    SDK_NIGHT_MODE_ENABLE,        // MISdkTag::NightModeEnable
    SDK_DEPURLE_ENABLE,           // MISdkTag::DepurpleEnable
    SDK_ENABLE,                   // MISdkTag::SdkEnabled
    SDK_CAMERAX_ENABLE,           // MISdkTag::CameraXEnabled
    SDK_VIDEO_BOKEH_ENABLE_FRONT, // MISdkTag::VideoBokehFrontEnabled
    SDK_VIDEO_BOKEH_PARAM_FRONT,  // MISdkTag::VideoBokehFrontParam
    SDK_VIDEO_BOKEH_ENABLE_REAR,  // MISdkTag::VideoBokehRearEnabled
    SDK_VIDEO_BOKEH_PARAM_REAR,   // MISdkTag::VideoBokehRearParam
    SDK_DEVICE_ORIENTATION,       // MISdkTag::DeviceOrientation
    SDK_BOKEH_FNUMBER_APPLIED,    // MISdkTag::BokehFNumberApplied
    SDK_VIDEOHDR10_MODE,          // MISdkTag::VideoHdr10Mode
    SDK_VIDEOCONTROL,             // MISdkTag::VideoControl
};

std::map<MISdkTag, MISdkTagInfo> MISdkTagInfoGroup = {
    {MISdkTag::HdrMode, {SDK_HDR_MODE, MISdkTagType::Byte, 1, MISdkTagScenes::Both}},
    {MISdkTag::SkinSmoothRatio,
     {SDK_BEAUTY_SKINSMOOTH, MISdkTagType::Int, 1, MISdkTagScenes::Both}},
    {MISdkTag::SlimFaceRatio, {SDK_BEAUTY_SLIMFACE, MISdkTagType::Int, 1, MISdkTagScenes::Both}},
    {MISdkTag::BokehEnable, {SDK_BOKEH_ENABLE, MISdkTagType::Byte, 1, MISdkTagScenes::Both}},
    {MISdkTag::MfnrEnable, {SDK_MFNR_ENABLE, MISdkTagType::Byte, 1, MISdkTagScenes::Snapshot}},
    {MISdkTag::NightModeEnable,
     {SDK_NIGHT_MODE_ENABLE, MISdkTagType::Byte, 1, MISdkTagScenes::Snapshot}},
    {MISdkTag::DepurpleEnable,
     {SDK_DEPURLE_ENABLE, MISdkTagType::Byte, 1, MISdkTagScenes::Snapshot}},
    {MISdkTag::SdkEnabled, {SDK_ENABLE, MISdkTagType::Byte, 1, MISdkTagScenes::Other}},
    {MISdkTag::CameraXEnabled, {SDK_CAMERAX_ENABLE, MISdkTagType::Byte, 1, MISdkTagScenes::Other}},
    {MISdkTag::VideoBokehFrontEnabled,
     {SDK_VIDEO_BOKEH_PARAM_FRONT, MISdkTagType::Int, 1, MISdkTagScenes::Preview}},
    {MISdkTag::VideoBokehFrontParam,
     {SDK_VIDEO_BOKEH_PARAM_FRONT, MISdkTagType::Float, 1, MISdkTagScenes::Preview}},
    {MISdkTag::VideoBokehRearEnabled,
     {SDK_VIDEO_BOKEH_ENABLE_REAR, MISdkTagType::Int, 1, MISdkTagScenes::Preview}},
    {MISdkTag::VideoBokehRearParam,
     {SDK_VIDEO_BOKEH_PARAM_REAR, MISdkTagType::Int, 1, MISdkTagScenes::Preview}},
    {MISdkTag::DeviceOrientation,
     {SDK_DEVICE_ORIENTATION, MISdkTagType::Int, 1, MISdkTagScenes::Preview}},
    {MISdkTag::BokehFNumberApplied,
     {SDK_BOKEH_FNUMBER_APPLIED, MISdkTagType::String, 256, MISdkTagScenes::Both}},
    {MISdkTag::VideoHdr10Mode,
     {SDK_VIDEOHDR10_MODE, MISdkTagType::Byte, 1, MISdkTagScenes::Preview}},
    {MISdkTag::VideoControl, {SDK_VIDEOCONTROL, MISdkTagType::Int, 1, MISdkTagScenes::Preview}},
};

static const char *VendorTagGroup[] = {
    MI_MFNR_FRAMENUM,               // VendorTag::MfntFrameNums
    MI_HDR_ENABLE,                  // VendorTag::HdrEnabled
    MI_BEAUTY_SKINSMOOTH,           // VendorTag::SkinSmoothRatio
    MI_BEAUTY_SLIMFACE,             // VendorTag::SlimFaceRatio
    MI_BOKEH_ENABLE,                // VendorTag::BokehEnabled
    MI_MFNR_ENABLE,                 // VendorTag::MfnrEnabled
    MI_SUPERNIGHT_ENABLED,          // VendorTag::SuperNightEnabled
    MI_DEPURPLE_ENABLE,             // VendorTag::DepurpleEnabled
    MI_DISTORTION_LEVEL,            // VendorTag::DistortionLevel
    MI_DISTORTION_ULTRAWIDE_LEVEL,  // VendorTag::UltraWideDistortionLevel
    MI_DISTORTION_ULTRAWIDE_ENABLE, // VendorTag::UltraWideDistortionEnable
    MI_HDR_CHECKER_RESULT,          // VendorTag::HdrChecker
    MI_HDR_DETECTED,                // VendorTag::HdrDetected
    MI_HDR_CHECKER_ENABLE,          // VendorTag::HdrCheckerEnabled
    MI_HDR_CHECKER_STATUS,          // VendorTag::HdrCheckerStatus
    MI_HDR_MODE,                    // VendorTag::HdrMode
    MI_SUPERNIGHT_CHECKER,          // VendorTag::SuperNightChecker
    MI_SUPPORTED_ALGO_HDR,          // VendorTag::SupportedAlgoEngineHdr
    MI_ASD_ENABLE,                  // VendorTag::AsdEnable
    QCOM_MFHDR_ENABLE,              // VendorTag::VideoHdrEnable
    MI_COLOR_RETENTION_FRONT_VALUE, // VendorTag::VideoBokehFrontEnabled
    MI_VIDEO_BOKEH_PARAM_FRONT,     // VendorTag::VideoBokehFrontParam
    MI_COLOR_RETENTION_VALUE,       // VendorTag::VideoBokehRearEnabled
    MI_VIDEO_BOKEH_PARAM_BACK,      // VendorTag::VideoBokehRearParam
    MI_DEVICE_ORIENTATION,          // VendorTag::DeviceOrientation
    MI_BOKEH_FNUMBER_APPLIED,       // VendorTag::BokehFNumberApplied
    MI_VIDEO_CONTROL,               // VendorTag::VideoControl
    SDK_SESSION_PARAMS_BOKEHROLE,   // VendorTag::BokehRole
    SDK_SESSION_PARAMS_YUVSNAPSHOT, // VendorTag::ThirdPartyYUVSnapshot
    QCOM_VIDEOHDR10_MODE,           // VendorTag::VideoHdr10Mode
    QCOM_VIDEOHDR10_SESSIONPARAM,   // VendorTag::HDRVideoModeSession
};

std::map<const char *, const char *> TransformTable = {
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::SkinSmoothRatio)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::SkinSmoothRatio)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::SlimFaceRatio)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::SlimFaceRatio)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::BokehEnable)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::BokehEnabled)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::MfnrEnable)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::MfnrEnabled)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::NightModeEnable)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::SuperNightEnabled)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::DepurpleEnable)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::DepurpleEnabled)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::VideoBokehFrontEnabled)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::VideoBokehFrontEnabled)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::VideoBokehFrontParam)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::VideoBokehFrontParam)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::VideoBokehRearEnabled)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::VideoBokehRearEnabled)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::VideoBokehRearParam)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::VideoBokehRearParam)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::DeviceOrientation)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::DeviceOrientation)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::BokehFNumberApplied)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::BokehFNumberApplied)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::VideoHdr10Mode)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::VideoHdr10Mode)]},
    {MISdkTagGroup[static_cast<uint32_t>(MISdkTag::VideoControl)],
     VendorTagGroup[static_cast<uint32_t>(VendorTag::VideoControl)]},
};

std::map<uint32_t, std::string> PhysicalAlgorithm = {
    {DCENABLEDINDEX, VendorTagGroup[static_cast<uint32_t>(VendorTag::DistortionLevel)]},
    {LDCENABLEDINDEX, VendorTagGroup[static_cast<uint32_t>(VendorTag::UltraWideDistortionLevel)]},
    {LDCPREVIEWENABLEDINDEX,
     VendorTagGroup[static_cast<uint32_t>(VendorTag::UltraWideDistortionEnable)]},
};

std::string readFile(const char *fileName)
{
    std::ifstream configFile(fileName);
    if (!configFile.is_open()) {
        MLOGE(LogGroupHAL, "open failed! file: %s", fileName);
    }
    std::string result((std::istreambuf_iterator<char>(configFile)),
                       std::istreambuf_iterator<char>());
    configFile.close();
    return result;
}

} // namespace
namespace mihal {

std::map<std::string, FeatureMask_t> CloudControlerBase::mName2FeatureMask = {
    {"HDR", CameraMode::FeatureMask::FeatureHDR | CameraMode::FeatureMask::FeatureRawHDR},
    {"MFNR", CameraMode::FeatureMask::FeatureMfnr},
    {"BEAUTY", CameraMode::FeatureMask::FeatureBeauty},
    {"DEPURPLE", CameraMode::FeatureMask::FeatureDepurple},
    {"BOKEH_FRONT", CameraMode::FeatureMask::FeatureBokeh},
    {"BOKEH_REAR", CameraMode::FeatureMask::FeatureBokeh},
    {"SUPER_NIGHT", CameraMode::FeatureMask::FeatureSN},
    {"AntiShake", CameraMode::VideoFeatureMask::VideoFeatureAntiShake},
    {"VIDEO_SUPER_NIGHT", CameraMode::VideoFeatureMask::VideoFeatureSN},
    {"VIDEO_HDR", CameraMode::VideoFeatureMask::VideoFeatureHDR},
};

std::string CloudControlerBase::getCloudControlInfo()
{
    return readFile(vendor::xiaomi::hardware::bgservice::implementation::ecoCloudInfoPath);
}

bool CloudControlerBase::parseCloudControlInfo(const char *appName, uint32_t roleId,
                                               const char *modeName, bool isCameraX)
{
    Document doc;
    std::string &&info = getCloudControlInfo();
    const char *cloudControlInfo = info.c_str();

    if (strlen(cloudControlInfo) < 10) {
        cloudControlInfo = readFile(ecoCloudInfoPathDefault).c_str();
    }

    if (doc.Parse(cloudControlInfo).HasParseError()) {
        MLOGE(LogGroupHAL, "[Ecology] parse error: (%d:%zu)%s\n", doc.GetParseError(),
              doc.GetErrorOffset(), GetParseError_En(doc.GetParseError()));
        return RESULT_FAILD;
    }
    if (!doc.IsObject()) {
        MLOGE(LogGroupHAL, "[Ecology] should be an object!");
        return RESULT_FAILD;
    }
    if (!doc.HasMember("SDK")) {
        MLOGE(LogGroupHAL, "[Ecology] 'SDK' no found!");
        return RESULT_FAILD;
    }
    mCloudInfo.sdkName = doc["SDK"].GetString();
    if (!doc.HasMember("Version")) {
        MLOGE(LogGroupHAL, "[Ecology] 'Version' no found!");
        return RESULT_FAILD;
    }
    mCloudInfo.sdkVersion = doc["Version"].GetString();
    MLOGI(LogGroupHAL, "[Ecology] get SdkName: %s, SdkVersion: %s", mCloudInfo.sdkName.c_str(),
          mCloudInfo.sdkVersion.c_str());
    noMatch();

    auto findMapFromJson = [](const rapidjson::Value &object, const char *menber,
                              const char *val) -> int {
        for (SizeType i = 0; i < object.Size(); ++i) {
            if (!object[i].HasMember(menber)) {
                return -1;
            }
            if (strcmp(object[i][menber].GetString(), val) == 0) {
                return i;
            }
        }
        return -1;
    };

    if ((!isCameraX && doc.HasMember("mivi_app_whiteList")) ||
        (isCameraX && doc.HasMember("camera_extension") &&
         doc["camera_extension"].HasMember("mivi_app_blackList"))) {
        const rapidjson::Value &whiteListObject =
            isCameraX ? doc["camera_extension"]["mivi_app_blackList"] : doc["mivi_app_whiteList"];

        if (!(whiteListObject.HasMember("PackageName") &&
              whiteListObject["PackageName"].IsArray() &&
              !whiteListObject["PackageName"].IsNull())) {
            MLOGI(LogGroupHAL, "[Ecology] cloud info has no PackageName attribute");
            return RESULT_SECCESS;
        }
        const rapidjson::Value &appItemObject = whiteListObject["PackageName"];
        int targetIndex = findMapFromJson(appItemObject, "PackageName", appName);
        if (targetIndex == -1) {
            MLOGI(LogGroupHAL, "[Ecology] can not find PackageName: %s", appName);
            return RESULT_SECCESS;
        }
        MLOGI(LogGroupHAL, "[Ecology] find PackageName: %s", appName);
        onlyMatchApp();

        const char *modeKey = isCameraX ? "ModeList" : "except_ModeList";
        if (!(appItemObject[targetIndex].HasMember(modeKey) &&
              appItemObject[targetIndex][modeKey].IsArray() &&
              !appItemObject[targetIndex][modeKey].IsNull())) {
            MLOGI(LogGroupHAL, "[Ecology] cloud info has no %s attribute", modeKey);
            return RESULT_SECCESS;
        }
        const rapidjson::Value &modeListObject = appItemObject[targetIndex][modeKey];
        targetIndex = findMapFromJson(modeListObject, "Mode", modeName);
        if (targetIndex == -1) {
            MLOGI(LogGroupHAL, "[Ecology] can not find Mode: %s", modeName);
            return RESULT_SECCESS;
        }
        MLOGI(LogGroupHAL, "[Ecology] find Mode: %s", modeName);

        if (!(modeListObject[targetIndex].HasMember("ModeAbility") &&
              modeListObject[targetIndex]["ModeAbility"].IsArray() &&
              !modeListObject[targetIndex]["ModeAbility"].IsNull())) {
            MLOGI(LogGroupHAL, "[Ecology] cloud info has no ModeAbility attribute");
            return RESULT_SECCESS;
        }
        const rapidjson::Value &modeObject = modeListObject[targetIndex]["ModeAbility"];
        const char *roleName = nullptr;
        switch (roleId) {
        case CameraDevice::RoleIdRear3PartSat:
            roleName = "Rear3PartSat";
            break;
        case CameraDevice::RoleIdFront:
            roleName = "Front";
            break;
        case CameraDevice::RoleIdRearSat:
            roleName = "PhotoSat";
            break;
        case CameraDevice::RoleIdRearUltra:
            roleName = "UltraWide";
            break;
        default:
            MLOGE(LogGroupHAL, "[Ecology] roleId %d error!", roleId);
            return RESULT_FAILD;
        }
        targetIndex = findMapFromJson(modeObject, "RoleName", roleName);
        if (targetIndex == -1) {
            MLOGI(LogGroupHAL, "[Ecology] can not find RoleName: %s", roleName);
            return RESULT_SECCESS;
        }
        MLOGI(LogGroupHAL, "[Ecology] find RoleName: %s", roleName);

        if (!(modeObject[targetIndex].HasMember("FunctionList") &&
              modeObject[targetIndex]["FunctionList"].IsArray() &&
              !modeObject[targetIndex]["FunctionList"].IsNull())) {
            MLOGI(LogGroupHAL, "[Ecology] cloud info has no FunctionList attribute");
            return RESULT_SECCESS;
        }
        const rapidjson::Value &funcListObject = modeObject[targetIndex]["FunctionList"];
        for (int i = 0; i < funcListObject.Size(); i++) {
            const char *funcName = funcListObject[i].GetString();
            if (mName2FeatureMask.find(funcName) == mName2FeatureMask.end()) {
                MLOGI(LogGroupHAL, "[Ecology] no %s function", funcName);
                continue;
            }

            matchFunction(mName2FeatureMask[funcName]);
            MLOGI(LogGroupHAL, "[Ecology] find function %s", funcName);
        }
    }

    return RESULT_SECCESS;
}

EcologyAdapter::EcologyAdapter(int32_t roleId, const StreamConfig *streamConfig)
    : mNeedReprocess(true), mStreamsBitSet(0)
{
    int result;
    parseSessionConfiguration(streamConfig);

    mStreamInfoMap = MiviSettings::getVendorStreamInfoMapFromJson(
        "odm/etc/camera/xiaomi/ecovendorstreamconfig.json", mSessionConfiguration.operationMode,
        roleId);

    if (mStreamInfoMap.featureMask == 0) {
        mNeedReprocess = false;
        return;
    }

    std::unique_ptr<CloudControlerBase> cloudControler = nullptr;
    if (mSessionConfiguration.isCameraXConnection) {
        cloudControler = std::make_unique<ExtCloudControler>();
    } else {
        cloudControler = std::make_unique<SdkCloudControler>();
    }

    int ret = cloudControler->parseCloudControlInfo(mSessionConfiguration.applicationName.c_str(),
                                                    roleId, mStreamInfoMap.signature.c_str(),
                                                    mSessionConfiguration.isCameraXConnection);
    mCloudControlInfo = *cloudControler->getCloudInfo();
    if (ret == RESULT_FAILD) {
        mNeedReprocess = false;
        return;
    }

    mStreamInfoMap.featureMask = mStreamInfoMap.featureMask & mCloudControlInfo.enableFeatureMask;

    MLOGI(LogGroupHAL, "[Ecology] final Feature Mask: 0x%x, enable Feature Mask: 0x%x",
          mStreamInfoMap.featureMask, mCloudControlInfo.enableFeatureMask);
    if (mStreamInfoMap.featureMask == 0) {
        mNeedReprocess = false;
        return;
    }

    getMatchingUsecase(streamConfig, mSessionConfiguration.operationMode,
                       mStreamInfoMap.isVideoMode);
}

// parse the session_parameters from streamConfig.
// get operationMode,isCameraXConnection and applicationName into mSessionConfiguration
void EcologyAdapter::parseSessionConfiguration(const StreamConfig *streamConfig)
{
    auto meta = streamConfig->getMeta();
    auto entry = meta->find(SDK_SESSION_PARAMS_OPERATION);
    if (entry.count > 0) {
        mSessionConfiguration.operationMode = entry.data.i32[0];
        MLOGD(LogGroupHAL, "operation mode is %d", mSessionConfiguration.operationMode);
    } else {
        MLOGD(LogGroupHAL, "Can't get operation mode");
    }
    entry = meta->find(SDK_SESSION_PARAMS_ISCAMERAX);
    if (entry.count > 0) {
        mSessionConfiguration.isCameraXConnection = entry.data.i32[0];
        MLOGD(LogGroupHAL, "cameraxConnection state is %d",
              mSessionConfiguration.isCameraXConnection);
    } else {
        MLOGD(LogGroupHAL, "Can't get cameraX connection status");
        mSessionConfiguration.isCameraXConnection = 0;
    }
    entry = meta->find(SDK_SESSION_PARAMS_CLIENT);
    if (entry.count > 0) {
        std::string temp(reinterpret_cast<const char *>(entry.data.u8));
        mSessionConfiguration.applicationName = temp;
        MLOGD(LogGroupHAL, "cameraxConnection state is %s",
              mSessionConfiguration.applicationName.c_str());
    } else {
        mSessionConfiguration.applicationName = "com.xiaomi.camera.mivi";
        MLOGD(LogGroupHAL, "Can't get application name");
    }
}

// Select SDK usecaseId according to format of stream and operationMode in session_parameters.
// UsecaseId is used to confirm to be replaced opMode
void EcologyAdapter::getMatchingUsecase(const StreamConfig *streamConfig, uint32_t sessionOperation,
                                        bool isVideoMode)
{
    bool isMatch = false;
    uint32_t streamNum = streamConfig->getStreamNums();
    uint32_t bitSet = 0;
    for (int i = 0; i < streamNum; i++) {
        int format = streamConfig->toRaw()->streams[i]->format;
        if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == format) {
            if (!(streamConfig->toRaw()->streams[i]->usage & GrallocUsageHwVideoEncoder)) {
                bitSet |= BitMaskPreviewFormat;
            } else {
                bitSet |= BitMaskPreviewHWFormat;
            }
        } else if (HAL_PIXEL_FORMAT_YCbCr_420_888 == format) {
            bitSet |= BitMaskYCbCrFormat;
        } else if (HAL_PIXEL_FORMAT_BLOB == format) {
            bitSet |= BitMaskBlobFormat;
        } else if (HAL_PIXEL_FORMAT_YCbCr_420_P010 == format) {
            bitSet |= BitMaskYCbCr10bitFormat;
        }
    }

    mStreamsBitSet = bitSet;

    if (isVideoMode) {
        if (matchVideoStream(bitSet)) {
            mUsecaseId = ThirdPartyUsecaseId::Video;
        } else {
            MLOGI(LogGroupHAL, "[Ecology] can not match video stream!");
            mUsecaseId = ThirdPartyUsecaseId::Default;
        }
        return;
    }

    switch (streamNum) {
    case 1:
        isMatch = true;
        mUsecaseId = ThirdPartyUsecaseId::Default;
        break;
    case 2:
        isMatch = true;
        if ((bitSet & BitMaskPreviewFormat) && (bitSet & BitMaskYCbCrFormat)) {
            mUsecaseId = ThirdPartyUsecaseId::YuvSnapshot;
        } else if ((bitSet & BitMaskPreviewFormat) && (bitSet & BitMaskBlobFormat)) {
            mUsecaseId = ThirdPartyUsecaseId::JpegSnapshot;
        } else {
            mUsecaseId = ThirdPartyUsecaseId::Default;
        }
        break;
    case 3:
        isMatch = true;
        if ((bitSet & BitMaskPreviewFormat) && (bitSet & BitMaskYCbCrFormat) &&
            (bitSet & BitMaskBlobFormat)) {
            mUsecaseId = ThirdPartyUsecaseId::JpegSnapshot;
        } else if ((bitSet & BitMaskPreviewFormat) && (bitSet & BitMaskYCbCrFormat)) {
            mUsecaseId = ThirdPartyUsecaseId::YuvSnapshot;
        } else {
            mUsecaseId = ThirdPartyUsecaseId::Default;
        }
        break;
    default:
        break;
    }
    if (!isMatch) {
        mUsecaseId = ThirdPartyUsecaseId::Default;
    }
}

// This function confirms whether reConfig is needed and then replace opMode if it is.
// streamConfig must be Remote
void EcologyAdapter::buildConfiguration(int32_t roleId, StreamConfig *streamConfig)
{
    if (!mNeedReprocess) {
        return;
    }

    ThirdPartyUsecaseId usecaseId = mUsecaseId;
    mSessionConfiguration.cameraRole = roleId;
    uint32_t &operation_mode = streamConfig->toRaw()->operation_mode;

    switch (usecaseId) {
    case ThirdPartyUsecaseId::YuvSnapshot:
    case ThirdPartyUsecaseId::JpegSnapshot:
    case ThirdPartyUsecaseId::Video:
        operation_mode = mStreamInfoMap.vendorOpMode;
        break;
    default:
        mNeedReprocess = false;
        break;
    }

    if (isFeatureEnable(CameraMode::FeatureBokeh)) {
        if (mStreamInfoMap.signature == "Bokeh" || mStreamInfoMap.signature == "Bokeh2x") {
            int32_t role = CameraDevice::RoleIdRearBokeh1x;

            if (mStreamInfoMap.signature == "Bokeh2x") {
                role = CameraDevice::RoleIdRearBokeh2x;
            }
            auto meta = streamConfig->getModifyMeta();
            meta->update(MI_CAMERA_BOKEH_ROLE, &role, 1);

            uint8_t bokehMode = 2;
            meta->update(SDK_BOKEH_MODE, &bokehMode, 1);
        }
    }
}

bool EcologyAdapter::matchVideoStream(uint32_t streamsBitset)
{
    auto &allowStreamsMask = mStreamInfoMap.allowStreamsMask;
    for (int i = 0; i < allowStreamsMask.size(); i++) {
        if (streamsBitset == allowStreamsMask[i]) {
            return true;
        }
    }

    MLOGI(LogGroupHAL, "[Ecology] can not match current stream config : %d", streamsBitset);
    return false;
}

bool EcologyAdapter::isFeatureEnable(FeatureMask_t mask)
{
    return mStreamInfoMap.featureMask & mask;
}

void EcologyAdapter::convertTag(LocalRequest *captureRequest, bool isSnapshot,
                                std::initializer_list<MISdkTag> sdkTags)
{
    for (auto item : sdkTags) {
        MISdkTagInfo sdkTagInfo = MISdkTagInfoGroup[item];
        convertTagImpl(captureRequest, sdkTagInfo, isSnapshot);
    }
}

void EcologyAdapter::convertTagImpl(LocalRequest *captureRequest, MISdkTagInfo &sdkTagInfo,
                                    bool isSnapshot)
{
    const char *sdkTagName = sdkTagInfo.tagName;
    uint32_t tagLength = sdkTagInfo.length;
    bool needConvert = false;
    switch (sdkTagInfo.scenes) {
    case MISdkTagScenes::Preview:
        if (!isSnapshot) {
            needConvert = true;
        }
        break;
    case MISdkTagScenes::Snapshot:
        if (isSnapshot) {
            needConvert = true;
        }
        break;
    case MISdkTagScenes::Both:
        needConvert = true;
        break;
    case MISdkTagScenes::Other:
    default:
        break;
    }

    camera_metadata_ro_entry entry{.count = 0};
    if (needConvert) {
        entry = captureRequest->findInMeta(sdkTagName);
    }
    int size = 0;
    switch (sdkTagInfo.tagType) {
    case MISdkTagType::Byte: {
        uint8_t *tagValue = new uint8_t[entry.count];
        memset(tagValue, 0, entry.count);
        if (entry.count)
            memcpy(tagValue, entry.data.u8, entry.count);
        captureRequest->updateMeta(TransformTable[sdkTagName], tagValue, entry.count);
        if (entry.count == 1) {
            MLOGD(LogGroupHAL, "[suojp] convertTagImpl %s -> %s : %d", sdkTagName,
                  TransformTable[sdkTagName], (const uint8_t)tagValue[0]);
        }
        delete[] tagValue;
        break;
    }
    case MISdkTagType::Int: {
        int32_t *tagValue = new int32_t[entry.count];
        memset(tagValue, 0, entry.count * sizeof(int));
        if (entry.count)
            memcpy(tagValue, entry.data.i32, entry.count * sizeof(int));
        captureRequest->updateMeta(TransformTable[sdkTagName], tagValue, entry.count);
        delete[] tagValue;
        break;
    }
    case MISdkTagType::Float: {
        float *tagValue = new float[entry.count];
        memset(tagValue, 0, entry.count * sizeof(float));
        if (entry.count)
            memcpy(tagValue, entry.data.f, entry.count * sizeof(float));
        captureRequest->updateMeta(TransformTable[sdkTagName], tagValue, entry.count);
        delete[] tagValue;
        break;
    }
    case MISdkTagType::String: {
        uint8_t *tagValue = new uint8_t[entry.count];
        memset(tagValue, 0, entry.count);
        if (entry.count)
            memcpy(tagValue, entry.data.u8, entry.count);
        captureRequest->updateMeta(TransformTable[sdkTagName], tagValue, entry.count);
        delete[] tagValue;
        break;
    }
    }
}

void EcologyAdapter::setAntiShakeEnabled(LocalRequest *captureRequest)
{
    uint8_t tagValue = 1;
    captureRequest->updateMeta(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &tagValue, 1);
    MLOGD(LogGroupHAL, "set %d: tagValue: %d", ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, tagValue);
}

void EcologyAdapter::setAsdEnabled(LocalRequest *captureRequest)
{
    uint8_t asdEnabled = 1;
    captureRequest->updateMeta(VendorTagGroup[static_cast<uint32_t>(VendorTag::AsdEnable)],
                               &asdEnabled, 1);
}

void EcologyAdapter::setHdrCheckerEnabled(LocalRequest *captureRequest)
{
    uint8_t hdrCheckerEnabled = 1;
    int32_t hdrCheckerStatus = 2;
    // hdrMode  ->  0  : disable hdr
    //          ->  1  : force hdr
    //          ->  2  : auto hdr
    uint8_t hdrMode = 2;

    captureRequest->updateMeta(VendorTagGroup[static_cast<uint32_t>(VendorTag::HdrCheckerEnabled)],
                               &hdrCheckerEnabled, 1);
    captureRequest->updateMeta(VendorTagGroup[static_cast<uint32_t>(VendorTag::HdrCheckerStatus)],
                               &hdrCheckerStatus, 1);
    captureRequest->updateMeta(VendorTagGroup[static_cast<uint32_t>(VendorTag::HdrMode)], &hdrMode,
                               1);
}

void EcologyAdapter::setMFNRFrameNumbers(LocalRequest *captureRequest)
{
    const char *mfnrTagName = VendorTagGroup[static_cast<uint32_t>(VendorTag::MfnrEnabled)];
    auto entry = captureRequest->findInMeta(mfnrTagName);
    if (!entry.count || 0 == entry.data.i32[0]) {
        return;
    } else {
        int32_t numFrames = 5;
        captureRequest->updateMeta(VendorTagGroup[static_cast<uint32_t>(VendorTag::MfntFrameNums)],
                                   &numFrames, 1);
    }
}

void EcologyAdapter::setBeautyPrams(LocalRequest *captureRequest)
{
    uint8_t makeupGender = 1;
    uint8_t removeNevus = 1;
    captureRequest->updateMeta(MI_BEAUTY_MAKEUPGENDER, &makeupGender, 1);
    captureRequest->updateMeta(MI_BEAUTY_REMOVENEVUS, &removeNevus, 1);
}

void EcologyAdapter::resetSuperNightPreviewSettings(LocalRequest *captureRequest, bool isSnapshot)
{
    if (mSessionConfiguration.cameraRole == CameraDevice::CameraRoleId::RoleIdFront) {
        return;
    }

    if (!isSnapshot) {
        uint8_t snMode = 0;
        captureRequest->updateMeta(MI_MIVI_SUPERNIGHT_MODE, &snMode, 1);
    }
}

void EcologyAdapter::setThirdYUVSnapshot(LocalRequest *captureRequest, bool isSnapshot)
{
    if (!isSnapshot || mUsecaseId != ThirdPartyUsecaseId::YuvSnapshot) {
        return;
    }
    uint8_t isThirdPartySnapshot = 1;
    captureRequest->updateMeta(
        VendorTagGroup[static_cast<uint32_t>(VendorTag::ThirdPartyYUVSnapshot)],
        &isThirdPartySnapshot, 1);
}

void EcologyAdapter::setPhyAlgoEnabled(LocalRequest *captureRequest, uint32_t cameraRole)
{
    MiSDKPhyAlgoSupport miSDKPhyAlgoSupport = {};
    getPhyMialgoSupport(miSDKPhyAlgoSupport, cameraRole);
    int32_t index = 1;

    while (miSDKPhyAlgoSupport.value != 0) {
        if (miSDKPhyAlgoSupport.value & 0x1) {
            uint8_t tagEnable = 1;
            captureRequest->updateMeta(PhysicalAlgorithm[index].c_str(), &tagEnable, 1);
        }
        index = index << 1;
        miSDKPhyAlgoSupport.value = miSDKPhyAlgoSupport.value >> 1;
    }
}

void EcologyAdapter::setBokehEnabled(LocalRequest *captureRequest)
{
    uint8_t bokehEnabled = 1;
    captureRequest->updateMeta(VendorTagGroup[static_cast<uint32_t>(VendorTag::BokehEnabled)],
                               &bokehEnabled, 1);
}

void EcologyAdapter::getPhyMialgoSupport(MiSDKPhyAlgoSupport &miSDKPhyAlgoSupport,
                                         uint32_t cameraRole)
{
    switch (cameraRole) {
    case CameraDevice::RoleIdRearWide:
        miSDKPhyAlgoSupport.value = 0x0;
        break;
    case CameraDevice::RoleIdRearUltra:
        miSDKPhyAlgoSupport.value = 0x6;
        break;
    default:
        miSDKPhyAlgoSupport.value = 0x0;
        break;
    }
}

void EcologyAdapter::buildRequest(std::shared_ptr<LocalRequest> captureRequest, bool isSnapshot,
                                  uint32_t cameraRole)
{
    std::vector<MISdkTag> needConvertTags;
    LocalRequest *lq = captureRequest.get();

    convertTag(lq, isSnapshot, {MISdkTag::DeviceOrientation});

    if (mUsecaseId == ThirdPartyUsecaseId::YuvSnapshot) {
        setThirdYUVSnapshot(lq, isSnapshot);
    }

    if (isFeatureEnable(CameraMode::FeatureMask::FeatureHDR)) {
        setHdrCheckerEnabled(lq);
        setAsdEnabled(lq);
    }

    if (isFeatureEnable(CameraMode::FeatureMask::FeatureVendorMfnr)) {
        convertTag(lq, isSnapshot, {MISdkTag::MfnrEnable});
        setMFNRFrameNumbers(lq);
    }

    if (isFeatureEnable(CameraMode::FeatureMask::FeatureBeauty)) {
        convertTag(lq, isSnapshot, {MISdkTag::SkinSmoothRatio, MISdkTag::SlimFaceRatio});
        setBeautyPrams(lq);
    }

    if (isFeatureEnable(CameraMode::FeatureMask::FeatureBokeh)) {
        convertTag(lq, isSnapshot, {MISdkTag::BokehFNumberApplied});
        if (mSessionConfiguration.cameraRole == VendorCamera::RoleIdFront) {
            convertTag(lq, isSnapshot, {MISdkTag::BokehEnable});
        } else {
            setBokehEnabled(lq);
            setAsdEnabled(lq);
        }

        if (mStreamInfoMap.signature == "Bokeh2x") {
            float zoom = 3.0f;
            lq->updateMeta(ANDROID_CONTROL_ZOOM_RATIO, &zoom, 1);
        }
    }

    if (isFeatureEnable(CameraMode::FeatureMask::FeatureSN)) {
        convertTag(lq, isSnapshot, {MISdkTag::NightModeEnable});
        resetSuperNightPreviewSettings(lq, isSnapshot);
        setAsdEnabled(lq);
    }

    if (isFeatureEnable(CameraMode::FeatureMask::FeatureDepurple)) {
        convertTag(lq, isSnapshot, {MISdkTag::DepurpleEnable});
    }

    if (!isFeatureEnable(CameraMode::FeatureMask::FeatureFallBack) &&
        cameraRole == CameraDevice::CameraRoleId::RoleIdRearSat) {
        uint8_t i = 1;
        lq->updateMeta(MI_FALLBACK_DISABLE, &i, 1);
    }
}

void EcologyAdapter::buildVideoRequest(LocalRequest *captureRequest)
{
    if (mStreamInfoMap.isVideoMode == false) {
        return;
    }

    std::vector<MISdkTag> needConvertTags;

    if (isFeatureEnable(CameraMode::VideoFeatureMask::VideoFeatureAntiShake)) {
        setAntiShakeEnabled(captureRequest);
    }

    if (isFeatureEnable(CameraMode::VideoFeatureMask::VideoFeatureBokeh)) {
        if (mSessionConfiguration.cameraRole == CameraDevice::RoleIdFront) {
            convertTag(captureRequest, 0,
                       {MISdkTag::VideoBokehFrontEnabled, MISdkTag::VideoBokehFrontParam});
        } else {
            convertTag(captureRequest, 0,
                       {MISdkTag::VideoBokehRearEnabled, MISdkTag::VideoBokehRearParam});
        }
    }

    if (isFeatureEnable(CameraMode::VideoFeatureMask::VideoFeatureHDR10)) {
        convertTag(captureRequest, 0, {MISdkTag::VideoHdr10Mode});
    }

    if (isFeatureEnable(CameraMode::VideoFeatureMask::VideoFeatureAntiShakeV3)) {
        setAntiShakeEnabled(captureRequest);
        needConvertTags.push_back(MISdkTag::VideoControl);
    }

    if (isFeatureEnable(CameraMode::VideoFeatureMask::VideoFeatureBeauty)) {
        convertTag(captureRequest, 0, {MISdkTag::SkinSmoothRatio, MISdkTag::SlimFaceRatio});
    }
}

void EcologyAdapter::buildVideoConfig(LocalConfig *config)
{
    if (mStreamInfoMap.isVideoMode == false) {
        return;
    }

    if (isFeatureEnable(CameraMode::VideoFeatureMask::VideoFeatureSN)) {
        MLOGI(LogGroupHAL, "sdk is VideoNight");
        Metadata *meta = config->getModifyMeta();
        MASSERT(meta);
        uint8_t videoStabilizitionMode = 1;
        meta->update(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &videoStabilizitionMode, 1);

        if (mStreamInfoMap.fpsRange.size() > 0) {
            int32_t minFPS = mStreamInfoMap.fpsRange[0];
            int32_t maxFPS = mStreamInfoMap.fpsRange[1];
            int32_t fpsRange[] = {minFPS, maxFPS};
            meta->update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE, fpsRange, 2);
            MLOGV(LogGroupHAL, "[Ecology] set fpsrange minFPS is %d , maxFPS is %d", minFPS,
                  maxFPS);
        }
    } else if (isFeatureEnable(CameraMode::VideoFeatureMask::VideoFeatureHDR)) {
        MLOGI(LogGroupHAL, "sdk is VideoHDR");
        Metadata *meta = config->getModifyMeta();
        MASSERT(meta);
        int32_t videoHdrEnable = 1;
        meta->update(VendorTagGroup[static_cast<uint32_t>(VendorTag::VideoHdrEnable)],
                     &videoHdrEnable, 1);
    } else if (isFeatureEnable(CameraMode::VideoFeatureMask::VideoFeatureAntiShake)) {
        MLOGI(LogGroupHAL, "sdk is VideoEIS");
        if (config->getStreamNums() == 2) {
            Stream *stream0 = config->getStreams()[0].get();
            Stream *stream1 = config->getStreams()[1].get();
            if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == stream0->getFormat() &&
                HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == stream1->getFormat()) {
                stream1->setUsage(stream1->getUsage() | GrallocUsageHwVideoEncoder);
            }
        }
    } else if (isFeatureEnable(CameraMode::VideoFeatureMask::VideoFeatureHDR10)) {
        MLOGI(LogGroupHAL, "sdk is VideoHdr10");
        uint32_t PreBlobPreHWflag = 0;
        bool PreBlobYuv10flag = false;
        for (auto stream : config->getStreams()) {
            if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == stream->getFormat()) {
                PreBlobPreHWflag++;
            } else if (HAL_PIXEL_FORMAT_YCbCr_420_P010 == stream->getFormat()) {
                stream->setUsage(0x40010300);
                stream->setDataspace(HAL_DATASPACE_BT709);
                PreBlobYuv10flag = true;
            }
            if (CAMERA3_STREAM_OUTPUT == stream->getType() &&
                HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == stream->getFormat() &&
                (stream->getUsage() & GrallocUsageHwVideoEncoder) && 2 == PreBlobPreHWflag) {
                stream->setUsage(stream->getUsage() | GrallocUsage10Bit | GrallocUsagePrivate0);
            }
        }

        if (PreBlobYuv10flag) {
            Metadata *meta = config->getModifyMeta();
            MASSERT(meta);
            int32_t HDRModeHDR10 = 2;
            meta->update(VendorTagGroup[static_cast<uint32_t>(VendorTag::HDRVideoModeSession)],
                         &HDRModeHDR10, 1);
        }
    }

    if ((mStreamsBitSet & BitMaskYCbCrFormat) != 0) {
        for (auto stream : config->getStreams()) {
            if (HAL_PIXEL_FORMAT_YCbCr_420_888 == stream->getFormat()) {
                stream->setUsage(stream->getUsage() | GrallocUsageHwVideoEncoder);
                break;
            }
        }
    }
}

} // namespace mihal
