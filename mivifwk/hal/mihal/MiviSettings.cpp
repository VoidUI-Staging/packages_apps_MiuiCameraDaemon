#include "MiviSettings.h"

#include <cutils/properties.h>

#include <fstream>

#include "rapidjson/Document.h"
#include "rapidjson/StringBuffer.h"
#include "rapidjson/Writer.h"
#include "rapidjson/error/En.h"

using namespace rapidjson;

namespace mihal {

namespace {
template <typename T>
void printSettingsImp(const T &jsonVal, std::string indent = "    ")
{
    switch (jsonVal.GetType()) {
    case kNullType:
        MLOGD(LogGroupHAL, "%sNULL", indent.c_str());
        break;

    case kFalseType:
        MLOGD(LogGroupHAL, "%sFALSE", indent.c_str());
        break;

    case kTrueType:
        MLOGD(LogGroupHAL, "%sTRUE", indent.c_str());
        break;

    case kObjectType:
        MLOGD(LogGroupHAL, "%s{", indent.c_str());
        for (Value::ConstMemberIterator itr = jsonVal.MemberBegin(); itr != jsonVal.MemberEnd();
             ++itr) {
            MLOGD(LogGroupHAL, "    %s%s:", indent.c_str(), itr->name.GetString());
            printSettingsImp(itr->value, indent + "    ");
        }
        MLOGD(LogGroupHAL, "%s}", indent.c_str());
        break;

    case kArrayType:
        MLOGD(LogGroupHAL, "%s[", indent.c_str());
        for (Value::ConstValueIterator itr = jsonVal.Begin(); itr != jsonVal.End(); ++itr) {
            printSettingsImp(*itr, indent + "    ");
        }
        MLOGD(LogGroupHAL, "%s]", indent.c_str());
        break;

    case kStringType:
        MLOGD(LogGroupHAL, "%s%s", indent.c_str(), jsonVal.GetString());
        break;

    case kNumberType:
        if (jsonVal.IsInt64()) {
            MLOGD(LogGroupHAL, "%s%ld", indent.c_str(), jsonVal.GetInt64());
        } else {
            MLOGD(LogGroupHAL, "%s%f", indent.c_str(), jsonVal.GetDouble());
        }
        break;
    }
}
} // namespace

void MiviSettings::printSettings()
{
    printSettingsImp(getInstance()->FinalConfig);
}

MiviSettings *MiviSettings::getInstance()
{
    static MiviSettings info;

    return &info;
}

MiviSettings::MiviSettings() : mInited(false)
{
    mInited = initialize();
    if (!mInited) {
        MLOGE(LogGroupHAL, "[MiviSettings] Initialize FAIL!");
    } else {
        MLOGI(LogGroupHAL, "[MiviSettings] Initialize SUCCESS!, print result");
        printSettingsImp(FinalConfig);
    }
}

bool MiviSettings::initialize()
{
    // file path of file which contains config related to vendor platform
    char basePath[PROPERTY_VALUE_MAX];
    // file path of file which contains config related to Xiaomi phone
    char overridePath[PROPERTY_VALUE_MAX];
    property_get("persist.camera.base.settings.filepath", basePath,
                 "odm/etc/camera/xiaomi/settings.json");
    property_get("persist.camera.override.settings.filepath", overridePath,
                 "odm/etc/camera/xiaomi/overridesettings.json");

    std::string BaseJsonFile = readFile(basePath);
    if (BaseJsonFile.empty()) {
        MLOGE(LogGroupHAL, "[MiviSettings] BaseJsonFile Empty !!!");
        return false;
    }

    std::string OverrideJsonFile = readFile(overridePath);
    if (OverrideJsonFile.empty()) {
        MLOGE(LogGroupHAL, "[MiviSettings] OverrideJsonFile Empty !!!");
        return false;
    }

    if (!parseJsonFile(BaseJsonFile, FinalConfig)) {
        MLOGE(LogGroupHAL, "[MiviSettings] parse BaseJsonFile error !!!");
        return false;
    }

    if (!parseJsonFile(OverrideJsonFile, OverrideConfig)) {
        MLOGE(LogGroupHAL, "[MiviSettings] parse OverrideJsonFile error !!!");
        return false;
    }

    // NOTE: Override entries of FinalConfig which exsit in OverrideConfig
    MergeTwoConfig(FinalConfig, OverrideConfig);
    return true;
}

std::string MiviSettings::readFile(const char *fileName)
{
    std::ifstream configFile(fileName);
    if (!configFile.is_open()) {
        MLOGE(LogGroupHAL, "[MiviSettings] open failed! file: %s", fileName);
        return std::string();
    }
    std::string result((std::istreambuf_iterator<char>(configFile)),
                       std::istreambuf_iterator<char>());
    return result;
}

bool MiviSettings::parseJsonFile(std::string &info, Document &doc)
{
    const char *jsonStr = info.c_str();
    if (jsonStr == nullptr) {
        return false;
    }

    // NOTE: kParseCommentsFlag means support comments in Json file
    if (doc.Parse<kParseCommentsFlag>(jsonStr).HasParseError()) {
        MLOGE(LogGroupHAL, "[MiviSettings] parse error: (%d:%zu)%s\n", doc.GetParseError(),
              doc.GetErrorOffset(), GetParseError_En(doc.GetParseError()));
        return false;
    }

    if (!doc.IsObject()) {
        MLOGE(LogGroupHAL, "[MiviSettings] should be an object!");
        return false;
    }

    return true;
}

InternalStreamInfoMap MiviSettings::getVendorStreamInfoMapFromJson(const char *fileName,
                                                                   uint32_t opMode, int lensFacing)
{
    InternalStreamInfoMap map;
    Document doc;

    std::ifstream configFile(fileName);
    if (!configFile.is_open()) {
        MLOGE(LogGroupHAL, "[StreamPaser] open failed! file: %s", fileName);
        return map;
    }
    std::string jsonFile((std::istreambuf_iterator<char>(configFile)),
                         std::istreambuf_iterator<char>());
    if (jsonFile.empty()) {
        MLOGE(LogGroupHAL, "[StreamPaser] mivivendorstreamconfig.json Empty !!!");
        return map;
    }

    // NOTE: kParseCommentsFlag means support comments in Json file
    if (doc.Parse<kParseCommentsFlag>(jsonFile.c_str()).HasParseError()) {
        MLOGE(LogGroupHAL, "[StreamPaser] parse error: (%d:%zu)%s\n", doc.GetParseError(),
              doc.GetErrorOffset(), GetParseError_En(doc.GetParseError()));
        return map;
    }

    if (!doc.IsObject()) {
        MLOGE(LogGroupHAL, "[StreamPaser] doc should be an object!");
        return map;
    }

    if (doc.HasMember("ModeList") && doc["ModeList"].IsObject() && !doc["ModeList"].IsNull()) {
        if (doc["ModeList"]["Mode"].IsNull()) {
            MLOGE(LogGroupHAL, "[StreamPaser] Mode is null!");
            return map;
        }

        const rapidjson::Value &object = doc["ModeList"]["Mode"];
        MLOGI(LogGroupHAL, "Get ModeList size: %d", object.Size());

        for (SizeType i = 0; i < object.Size(); ++i) {
            uint32_t sessionOpModeFromJson = 0xFFFF;
            uint32_t vendorOpModeFromJson = 0xFFFF;
            CameraUser userFromJson = CameraUser::MIUI;
            int lensFacingFromJson = 0;

            if (object[i].HasMember("SessionOpMode") && object[i]["SessionOpMode"].IsUint()) {
                sessionOpModeFromJson = object[i]["SessionOpMode"].GetUint();
                map.sessionOpMode = sessionOpModeFromJson;
                MLOGI(LogGroupHAL, "[StreamPaser] Get SessionOpMode: %d", sessionOpModeFromJson);
            }

            if (object[i].HasMember("VendorOpMode") && object[i]["VendorOpMode"].IsUint()) {
                vendorOpModeFromJson = object[i]["VendorOpMode"].GetUint();
                map.vendorOpMode = vendorOpModeFromJson;
                MLOGI(LogGroupHAL, "[StreamPaser] Get VendorOpMode: %d", vendorOpModeFromJson);
            }

            if (object[i].HasMember("LensFacing")) {
                if (object[i]["LensFacing"].IsInt()) {
                    lensFacingFromJson = object[i]["LensFacing"].GetInt();
                    MLOGI(LogGroupHAL, "[StreamPaser] Get LensFacing: %d", lensFacingFromJson);
                } else if (object[i]["LensFacing"].IsArray()) {
                    const rapidjson::Value &subObject = object[i]["LensFacing"];
                    MLOGI(LogGroupHAL, "[StreamPaser] Get LensFacing num: %d", subObject.Size());
                    for (SizeType i = 0; i < subObject.Size(); i++) {
                        if (lensFacing == subObject[i]) {
                            lensFacingFromJson = subObject[i].GetInt();
                            MLOGI(LogGroupHAL, "[StreamPaser] Get LensFacing: %d",
                                  lensFacingFromJson);
                        }
                    }
                }
            }

            // for Ecology Function,"SessionOpMode" is the key,not VendorOpMode.
            // so if sessionOpModeFromJson is not the default,use it to compare whit opMode
            if (((sessionOpModeFromJson == 0xFFFF && vendorOpModeFromJson == opMode) ||
                 sessionOpModeFromJson == opMode) &&
                (lensFacingFromJson == lensFacing)) {
                if (object[i].HasMember("MialgoOpMode") && object[i]["MialgoOpMode"].IsUint()) {
                    map.mialgoOpMode = object[i]["MialgoOpMode"].GetUint();
                    MLOGI(LogGroupHAL, "[StreamPaser] Get MialgoOpMode: %d", map.mialgoOpMode);
                } else {
                    map.mialgoOpMode = 0xFFFF;
                }

                if (object[i].HasMember("Signature") && object[i]["Signature"].IsString()) {
                    map.signature = object[i]["Signature"].GetString();
                    MLOGI(LogGroupHAL, "[StreamPaser] Get signature: %s", map.signature.c_str());
                } else {
                    map.signature = "Error";
                    MLOGE(LogGroupHAL, "[StreamPaser] signature is null, check it!");
                }

                if (object[i].HasMember("BufferLimitCnt") && object[i]["BufferLimitCnt"].IsUint()) {
                    map.bufferLimitCnt = object[i]["BufferLimitCnt"].GetUint();
                    MLOGI(LogGroupHAL, "[StreamPaser] Get BufferLimitCnt: %d", map.bufferLimitCnt);
                } else {
                    map.bufferLimitCnt = 1;
                    MLOGW(LogGroupHAL, "[StreamPaser] BufferLimitCnt is null, use default value");
                }

                if (object[i].HasMember("FeatureMask") && object[i]["FeatureMask"].IsUint()) {
                    map.featureMask = object[i]["FeatureMask"].GetUint();
                    MLOGI(LogGroupHAL, "[StreamPaser] Get FeatureMask: 0x%x", map.featureMask);
                } else {
                    map.featureMask = 0x2;
                    MLOGW(LogGroupHAL, "[StreamPaser] FeatureMask is null, use default value");
                }

                if (object[i].HasMember("VendorSnapshotBufferQueueSize") &&
                    object[i]["VendorSnapshotBufferQueueSize"].IsUint()) {
                    map.snapshotBufferQueueSize =
                        object[i]["VendorSnapshotBufferQueueSize"].GetUint();
                } else {
                    map.snapshotBufferQueueSize = 0;
                }
                MLOGI(LogGroupHAL, "[StreamPaser] Get VendorSnapshotBufferQueueSize: %d",
                      map.snapshotBufferQueueSize);

                if (object[i].HasMember("FlushWaitTimeMs") &&
                    object[i]["FlushWaitTimeMs"].IsInt64()) {
                    map.flushWaitTime = object[i]["FlushWaitTimeMs"].GetInt64();
                } else {
                    map.flushWaitTime = 4000;
                }
                MLOGI(LogGroupHAL, "[StreamPaser] Get FlushWaitTimeMs: %d", map.flushWaitTime);

                if (object[i].HasMember("IsVendorLowCaps") &&
                    object[i]["IsVendorLowCaps"].IsBool()) {
                    map.isVendorLowCaps = object[i]["IsVendorLowCaps"].GetBool();
                } else {
                    map.isVendorLowCaps = false;
                }
                MLOGI(LogGroupHAL, "[StreamPaser] Get isVendorLowCaps: %d", map.isVendorLowCaps);

                // Ecology need process video
                if (object[i].HasMember("VideoMode") && object[i]["VideoMode"].IsBool()) {
                    map.isVideoMode = object[i]["VideoMode"].GetBool();
                } else {
                    map.isVideoMode = false;
                }
                MLOGI(LogGroupHAL, "[StreamPaser] Get isVideoMode: %d", map.isVideoMode);

                if (map.isVideoMode) {
                    if (object[i].HasMember("AllowStreamsMask") &&
                        object[i]["AllowStreamsMask"].IsArray()) {
                        const rapidjson::Value &subObject = object[i]["AllowStreamsMask"];
                        MLOGI(LogGroupHAL, "[StreamPaser] Get AllowStreamsMask num: %d",
                              subObject.Size());
                        for (SizeType i = 0; i < subObject.Size(); i++) {
                            map.allowStreamsMask.push_back(subObject[i].GetUint());
                        }
                    } else {
                        MLOGE(LogGroupHAL, "[StreamPaser] AllowStreamsMask is null in Video mode!");
                        return map;
                    }

                    if (object[i].HasMember("FPSRange") && object[i]["FPSRange"].IsArray()) {
                        const rapidjson::Value &subObject = object[i]["FPSRange"];
                        MLOGI(LogGroupHAL, "[StreamPaser]  Get FPSRange num: %d", subObject.Size());
                        for (SizeType i = 0; i < subObject.Size(); i++) {
                            map.fpsRange.push_back(subObject[i].GetUint());
                        }
                    } else {
                        MLOGV(LogGroupHAL, "[StreamPaser]  FPSRange  is null in Video mode!");
                        return map;
                    }
                }

                if (object[i].HasMember("StreamConfig") && object[i].IsObject() &&
                    !object[i].IsNull()) {
                    const rapidjson::Value &subObject = object[i]["StreamConfig"];
                    MLOGI(LogGroupHAL, "[StreamPaser] Get StreamConfig size: %d", subObject.Size());

                    for (SizeType j = 0; j < subObject.Size(); ++j) {
                        InternalStreamInfo info;

                        if (subObject[j].HasMember("roleId") && subObject[j]["roleId"].IsUint()) {
                            info.roleId = subObject[j]["roleId"].GetUint();
                        } else {
                            info.roleId = 0xFF;
                        }

                        if (subObject[j].HasMember("isPhysicalStream") &&
                            subObject[j]["isPhysicalStream"].IsBool()) {
                            info.isPhysicalStream = subObject[j]["isPhysicalStream"].GetBool();
                        } else {
                            info.isPhysicalStream = false;
                        }

                        if (subObject[j].HasMember("ratio") && subObject[j]["ratio"].IsDouble()) {
                            info.ratio = subObject[j]["ratio"].GetDouble();
                        } else {
                            info.ratio = 0.0f;
                        }

                        if (subObject[j].HasMember("format") && subObject[j]["format"].IsInt()) {
                            info.format = subObject[j]["format"].GetInt();
                        }

                        if (subObject[j].HasMember("usage") && subObject[j]["usage"].IsUint()) {
                            info.usage = subObject[j]["usage"].GetUint();
                        }

                        if (subObject[j].HasMember("stream_use_case") &&
                            subObject[j]["stream_use_case"].IsUint64()) {
                            info.stream_use_case = subObject[j]["stream_use_case"].GetUint64();
                        } else {
                            info.stream_use_case = 0;
                        }

                        if (subObject[j].HasMember("dataSpace") &&
                            subObject[j]["dataSpace"].IsUint()) {
                            info.space = subObject[j]["dataSpace"].GetUint();
                        }

                        if (subObject[j].HasMember("bokehSig") &&
                            subObject[j]["bokehSig"].IsUint()) {
                            info.bokehSig =
                                static_cast<BokehCamSig>(subObject[j]["bokehSig"].GetUint());
                        }

                        if (subObject[j].HasMember("isFakeSat") &&
                            subObject[j]["isFakeSat"].IsBool()) {
                            info.isFakeSat = subObject[j]["isFakeSat"].GetBool();
                        }

                        if (subObject[j].HasMember("isRemosaic") &&
                            subObject[j]["isRemosaic"].IsBool()) {
                            info.isRemosaic = subObject[j]["isRemosaic"].GetBool();
                        }

                        MLOGI(
                            LogGroupHAL,
                            "[StreamPaser] pase info[%d]: roleId:%d, isPhysicalStream:%d, ratio:%f,"
                            " format:%d, usage:%d, streamUsage: %" PRId64
                            ", space:%d, bokehSig:%d, isFakeSat:%d,"
                            " isRemosaic:%d",
                            j, info.roleId, info.isPhysicalStream, info.ratio, info.format,
                            info.usage, info.stream_use_case, info.space, info.bokehSig,
                            info.isFakeSat, info.isRemosaic);
                        map.streamInfos.push_back(info);
                    }
                } else {
                    // ecology video does not need StreamConfig
                    if (!map.isVideoMode) {
                        MLOGE(LogGroupHAL, "[StreamPaser] StreamConfig is null!");
                        return map;
                    }
                }

                if (object[i].HasMember("SupportFovCropCameras") && object[i].IsObject() &&
                    !object[i].IsNull()) {
                    const rapidjson::Value &subObject = object[i]["SupportFovCropCameras"];
                    MLOGI(LogGroupHAL, "[StreamPaser] SupportFovCrop Cameras number:%d",
                          subObject.Size());

                    for (SizeType j = 0; j < subObject.Size(); ++j) {
                        if (subObject[j].IsUint()) {
                            map.supportFovCropCameras.insert(subObject[j].GetUint());
                            MLOGI(LogGroupHAL, "[StreamPaser] Support FovCrop CameraRoleId:%d",
                                  subObject[j].GetUint());
                        }
                    }
                }

                break;
            } else {
                MLOGI(LogGroupHAL, "[StreamPaser] StreamConfig don't match, continue");
                continue;
            }
        }
    } else {
        MLOGE(LogGroupHAL, "[StreamPaser] ModesList is null!");
    }

    if (!map.streamInfos.size() && !map.isVideoMode) {
        MLOGE(LogGroupHAL, "[StreamPaser] parse vendorStreamConfig failed!!!");
    }

    return map;
}

CameraRoleConfigInfoMap MiviSettings::getCameraRoleConfigInfoFromJson()
{
    CameraRoleConfigInfoMap cameraRoleConfigInfoMap;
    Document doc;

    char *fileName = "odm/etc/camera/xiaomi/mivistreamformat.json";
    std::ifstream configFile(fileName);
    if (!configFile.is_open()) {
        MLOGE(LogGroupHAL, "[StreamPaser] open failed! file: %s", fileName);
        return cameraRoleConfigInfoMap;
    }
    std::string jsonFile((std::istreambuf_iterator<char>(configFile)),
                         std::istreambuf_iterator<char>());
    if (jsonFile.empty()) {
        MLOGE(LogGroupHAL, "[StreamPaser] mivivendorstreamconfig.json Empty !!!");
        return cameraRoleConfigInfoMap;
    }

    // NOTE: kParseCommentsFlag means support comments in Json file
    if (doc.Parse<kParseCommentsFlag>(jsonFile.c_str()).HasParseError()) {
        MLOGE(LogGroupHAL, "[StreamPaser] parse error: (%d:%zu)%s\n", doc.GetParseError(),
              doc.GetErrorOffset(), GetParseError_En(doc.GetParseError()));
        return cameraRoleConfigInfoMap;
    }

    if (!doc.IsObject()) {
        MLOGE(LogGroupHAL, "[StreamPaser] doc should be an object!");
        return cameraRoleConfigInfoMap;
    }

    if (doc.HasMember("RoleIdList") && doc["RoleIdList"].IsObject() &&
        !doc["RoleIdList"].IsNull()) {
        if (doc["RoleIdList"]["CameraRole"].IsNull()) {
            MLOGE(LogGroupHAL, "[StreamPaser] Mode is null!");
            return cameraRoleConfigInfoMap;
        }

        const rapidjson::Value &object = doc["RoleIdList"]["CameraRole"];
        MLOGI(LogGroupHAL, "Get RoleIdList size: %d", object.Size());

        for (SizeType i = 0; i < object.Size(); ++i) {
            uint32_t roleId = 0;

            if (object[i].HasMember("RoleId") && object[i]["RoleId"].IsUint()) {
                roleId = object[i]["RoleId"].GetUint();
                MLOGI(LogGroupHAL, "[StreamPaser] Get roleId: %d", roleId);
            } else {
                continue;
            }

            CameraRoleConfigInfo cameraConfigInfo{};
            cameraConfigInfo.supportFakeSat = false;
            cameraConfigInfo.fovCropZoomRatio = 1.0;

            if (object[i].HasMember("PhgStreamFormat") && object[i].IsObject() &&
                !object[i].IsNull()) {
                const rapidjson::Value &subObject = object[i]["PhgStreamFormat"];
                MLOGI(LogGroupHAL, "[StreamPaser] Get StreamFormat size: %d", subObject.Size());

                for (SizeType j = 0; j < subObject.Size(); ++j) {
                    uint32_t phgType = 0;
                    std::vector<int> formatVec;

                    if (subObject[j].HasMember("phgType") && subObject[j]["phgType"].IsUint()) {
                        phgType = subObject[j]["phgType"].GetUint();
                    }
                    if (subObject[j].HasMember("format") && subObject[j]["format"].IsArray()) {
                        for (auto &f : subObject[j]["format"].GetArray()) {
                            if (f.IsInt()) {
                                int format = f.GetInt();
                                MLOGI(LogGroupHAL, "[StreamPaser] phgType:%d format:%d", phgType,
                                      format);
                                formatVec.push_back(format);
                            } else {
                                MLOGE(LogGroupHAL, "[StreamPaser] format error");
                            }
                        }
                    }

                    cameraConfigInfo.phgStreamFormatMap.insert({phgType, formatVec});
                }
            }

            if (object[i].HasMember("FakeSatZoomStart") &&
                object[i]["FakeSatZoomStart"].IsFloat()) {
                float zoomStart = object[i]["FakeSatZoomStart"].GetFloat();
                cameraConfigInfo.fakeSatInfo.fakeSatThreshold.zoomStart = zoomStart;
                MLOGI(LogGroupHAL, "[StreamPaser] Get FakeSatZoomStart: %f", zoomStart);
            }

            if (object[i].HasMember("FakeSatZoomStop") && object[i]["FakeSatZoomStop"].IsFloat()) {
                float zoomStop = object[i]["FakeSatZoomStop"].GetFloat();
                cameraConfigInfo.fakeSatInfo.fakeSatThreshold.zoomStop = zoomStop;
                MLOGI(LogGroupHAL, "[StreamPaser] Get FakeSatZoomStop: %f", zoomStop);
            }

            if (object[i].HasMember("FakeSatLuxIndex") && object[i]["FakeSatLuxIndex"].IsFloat()) {
                float luxIndex = object[i]["FakeSatLuxIndex"].GetFloat();
                cameraConfigInfo.fakeSatInfo.fakeSatThreshold.luxIndex = luxIndex;
                MLOGI(LogGroupHAL, "[StreamPaser] Get FakeSatLuxIndex: %f", luxIndex);
            }

            if (object[i].HasMember("FakeSatShortGain") &&
                object[i]["FakeSatShortGain"].IsFloat()) {
                float shortGain = object[i]["FakeSatShortGain"].GetFloat();
                cameraConfigInfo.fakeSatInfo.fakeSatThreshold.shortGain = shortGain;
                MLOGI(LogGroupHAL, "[StreamPaser] Get FakeSatShortGain: %f", shortGain);
            }

            if (object[i].HasMember("FakeSatAdrcGain") && object[i]["FakeSatAdrcGain"].IsFloat()) {
                float adrcGain = object[i]["FakeSatAdrcGain"].GetFloat();
                cameraConfigInfo.fakeSatInfo.fakeSatThreshold.adrcGain = adrcGain;
                MLOGI(LogGroupHAL, "[StreamPaser] Get FakeSatAdrcGain: %f", adrcGain);
            }

            if (object[i].HasMember("FakeSatImageSizes") && object[i].IsObject() &&
                !object[i].IsNull()) {
                const rapidjson::Value &subObject = object[i]["FakeSatImageSizes"];
                MLOGI(LogGroupHAL, "[StreamPaser] Get FakeSatImageSizes size: %d",
                      subObject.Size());

                cameraConfigInfo.supportFakeSat = true;
                for (SizeType j = 0; j < subObject.Size(); ++j) {
                    std::map<uint32_t, FakeSatSize> fakeSatSizeMap{};
                    float frameRatio = 0;

                    if (subObject[j].HasMember("FrameRatio") &&
                        subObject[j]["FrameRatio"].IsFloat()) {
                        frameRatio = subObject[j]["FrameRatio"].GetFloat();
                    }

                    if (subObject[j].HasMember("ImageSizes") && subObject[j].IsObject() &&
                        !subObject[j].IsNull()) {
                        const rapidjson::Value &sub2Object = subObject[j]["ImageSizes"];
                        MLOGI(LogGroupHAL, "[StreamPaser] Get ImageSizes size: %d",
                              sub2Object.Size());

                        for (SizeType k = 0; k < sub2Object.Size(); ++k) {
                            FakeSatSize fakeSatSize{};
                            uint32_t type = 0;
                            if (sub2Object[k].HasMember("type") && sub2Object[k]["type"].IsUint()) {
                                type = sub2Object[k]["type"].GetUint();
                            }
                            if (sub2Object[k].HasMember("width") &&
                                sub2Object[k]["width"].IsUint()) {
                                fakeSatSize.width = sub2Object[k]["width"].GetUint();
                            }
                            if (sub2Object[k].HasMember("height") &&
                                sub2Object[k]["height"].IsUint()) {
                                fakeSatSize.height = sub2Object[k]["height"].GetUint();
                            }

                            fakeSatSizeMap.insert({type, fakeSatSize});
                        }
                    }
                    cameraConfigInfo.fakeSatInfo.ratioSizeMap.insert(
                        {frameRatio, std::move(fakeSatSizeMap)});
                }
            }

            if (object[i].HasMember("FovCropZoomRatio") &&
                object[i]["FovCropZoomRatio"].IsFloat()) {
                cameraConfigInfo.fovCropZoomRatio = object[i]["FovCropZoomRatio"].GetFloat();
            }
            MLOGI(LogGroupHAL, "[StreamPaser] Get FovCropZoomRatio: %0.2fX",
                  cameraConfigInfo.fovCropZoomRatio);

            if (object[i].HasMember("XCFASnapshotThreshold") && object[i].IsObject() &&
                !object[i].IsNull()) {
                const rapidjson::Value &subObject = object[i]["XCFASnapshotThreshold"];
                MLOGI(LogGroupHAL, "[StreamPaser] Get XCFASnapshotThreshold size: %d",
                      subObject.Size());

                for (SizeType j = 0; j < subObject.Size(); ++j) {
                    if (subObject[j].HasMember("xcfaShortGain") &&
                        subObject[j]["xcfaShortGain"].IsFloat()) {
                        float shortGain = subObject[j]["xcfaShortGain"].GetFloat();
                        cameraConfigInfo.xcfaSnapshotThreshold.shortGain = shortGain;
                        MLOGI(LogGroupHAL,
                              "[StreamPaser] Get xcfaSnapshotThreshold shortGain: %0.1f",
                              shortGain);
                    }
                    if (subObject[j].HasMember("xcfaLuxIndex") &&
                        subObject[j]["xcfaLuxIndex"].IsFloat()) {
                        float luxIndex = subObject[j]["xcfaLuxIndex"].GetFloat();
                        cameraConfigInfo.xcfaSnapshotThreshold.luxIndex = luxIndex;
                        MLOGI(LogGroupHAL,
                              "[StreamPaser] Get xcfaSnapshotThreshold luxIndex: %0.1f", luxIndex);
                    }
                    if (subObject[j].HasMember("xcfaAdrcGain") &&
                        subObject[j]["xcfaAdrcGain"].IsFloat()) {
                        float adrcGain = subObject[j]["xcfaAdrcGain"].GetFloat();
                        cameraConfigInfo.xcfaSnapshotThreshold.adrcGain = adrcGain;
                        MLOGI(LogGroupHAL,
                              "[StreamPaser] Get xcfaSnapshotThreshold adrcGain: %0.1f", adrcGain);
                    }
                    if (subObject[j].HasMember("xcfaDarkRatio") &&
                        subObject[j]["xcfaDarkRatio"].IsFloat()) {
                        float darkRatio = subObject[j]["xcfaDarkRatio"].GetFloat();
                        cameraConfigInfo.xcfaSnapshotThreshold.darkRatio = darkRatio;
                        MLOGI(LogGroupHAL,
                              "[StreamPaser] Get xcfaSnapshotThreshold darkRatio: %0.2f",
                              darkRatio);
                    }
                    if (subObject[j].HasMember("flashModeSingle") &&
                        subObject[j]["flashModeSingle"].IsInt()) {
                        int8_t flashModeSingle = subObject[j]["flashModeSingle"].GetInt();
                        cameraConfigInfo.xcfaSnapshotThreshold.flashModeSingle = flashModeSingle;
                        MLOGI(LogGroupHAL,
                              "[StreamPaser] Get xcfaSnapshotThreshold flashModeSingle: %d",
                              flashModeSingle);
                    }
                    if (subObject[j].HasMember("flashModeTorch") &&
                        subObject[j]["flashModeTorch"].IsInt()) {
                        int8_t flashModeTorch = subObject[j]["flashModeTorch"].GetInt();
                        cameraConfigInfo.xcfaSnapshotThreshold.flashModeTorch = flashModeTorch;
                        MLOGI(LogGroupHAL,
                              "[StreamPaser] Get xcfaSnapshotThreshold flashModeTorch: %d",
                              flashModeTorch);
                    }
                }
            }

            cameraRoleConfigInfoMap.insert({roleId, cameraConfigInfo});
        }
    } else {
        MLOGE(LogGroupHAL, "[StreamPaser] RoleIdList is null!");
    }

    return cameraRoleConfigInfoMap;
}

} // namespace mihal
