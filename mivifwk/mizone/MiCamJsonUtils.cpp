/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiCamJsonUtils.h"

#include <fstream>

using namespace std;
using namespace rapidjson;
using namespace midebug;

namespace mizone {

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

void MiCamJsonUtils::printSettings()
{
    printSettingsImp(getInstance()->FinalConfig);
}

MiCamJsonUtils *MiCamJsonUtils::getInstance()
{
    static MiCamJsonUtils info;

    return &info;
}

MiCamJsonUtils::MiCamJsonUtils() : mInited(false)
{
    mInited = initialize();
    if (!mInited) {
        MLOGE(LogGroupHAL, "[MiCamJsonUtils] Initialize FAIL!");
    } else {
        MLOGI(LogGroupHAL, "[MiCamJsonUtils] Initialize SUCCESS!, print result");
        printSettingsImp(FinalConfig);
    }
}

bool MiCamJsonUtils::initialize()
{
    // file path of file which contains config related to vendor platform
    char basePath[PROPERTY_VALUE_MAX];
    // file path of file which contains config related to Xiaomi phone
    char overridePath[PROPERTY_VALUE_MAX];
    property_get("persist.camera.base.settings.filepath", basePath,
                 "vendor/etc/camera/xiaomi/settings.json");
    property_get("persist.camera.override.settings.filepath", overridePath,
                 "vendor/etc/camera/xiaomi/overridesettings.json");

    std::string BaseJsonFile = readFile(basePath);
    if (BaseJsonFile.empty()) {
        MLOGE(LogGroupHAL, "[MiCamJsonUtils] BaseJsonFile Empty !!!");
        return false;
    }

    std::string OverrideJsonFile = readFile(overridePath);
    if (OverrideJsonFile.empty()) {
        MLOGE(LogGroupHAL, "[MiCamJsonUtils] OverrideJsonFile Empty !!!");
        return false;
    }

    if (!parseJsonFile(BaseJsonFile, FinalConfig)) {
        MLOGE(LogGroupHAL, "[MiCamJsonUtils] parse BaseJsonFile error !!!");
        return false;
    }

    if (!parseJsonFile(OverrideJsonFile, OverrideConfig)) {
        MLOGE(LogGroupHAL, "[MiCamJsonUtils] parse OverrideJsonFile error !!!");
        return false;
    }

    // NOTE: Override entries of FinalConfig which exsit in OverrideConfig
    MergeTwoConfig(FinalConfig, OverrideConfig);
    return true;
}

std::string MiCamJsonUtils::readFile(const char *fileName)
{
    std::ifstream configFile(fileName);
    if (!configFile.is_open()) {
        MLOGE(LogGroupHAL, "[MiCamJsonUtils] open failed! file: %s", fileName);
        return std::string();
    }
    std::string result((std::istreambuf_iterator<char>(configFile)),
                       std::istreambuf_iterator<char>());
    return result;
}

bool MiCamJsonUtils::parseJsonFile(std::string &info, Document &doc)
{
    const char *jsonStr = info.c_str();
    if (jsonStr == nullptr) {
        return false;
    }

    // NOTE: kParseCommentsFlag means support comments in Json file
    if (doc.Parse<kParseCommentsFlag>(jsonStr).HasParseError()) {
        MLOGE(LogGroupHAL, "[MiCamJsonUtils] parse error: (%d:%zu)%s\n", doc.GetParseError(),
              doc.GetErrorOffset(), GetParseError_En(doc.GetParseError()));
        return false;
    }

    if (!doc.IsObject()) {
        MLOGE(LogGroupHAL, "[MiCamJsonUtils] should be an object!");
        return false;
    }

    return true;
}

} // namespace mizone
