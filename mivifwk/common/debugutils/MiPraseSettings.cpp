#include "MiPraseSettings.h"

#include "MiDebugOfflineLogger.h"
#include "MiaPlatformSetting.h"
#include "MiaSettings.h"
#include "rapidjson/StringBuffer.h"
#include "rapidjson/Writer.h"
#include "rapidjson/error/En.h"

#undef LOG_TAG
#define LOG_TAG "MiPraseSettings"

namespace midebug {

MiPraseSettings *MiPraseSettings::getInstance()
{
    static MiPraseSettings sMiPraseSettings;
    return &sMiPraseSettings;
}

MiPraseSettings::MiPraseSettings()
{
    FILE *fp = fopen(SettingFilePath, "rb");
    if (!fp) {
        MLOGE(Mia2LogGroupCore, "open failed! file: %s", SettingFilePath);
        return;
    }

    fseek(fp, 0L, SEEK_END);
    long int length = ftell(fp);
    if (length < 0) {
        MLOGE(Mia2LogGroupCore, "ftell failed! file: %s", SettingFilePath);
        return;
    }

    char *buf = (char *)malloc(length + 1);
    memset(buf, 0, sizeof(char) * (length + 1));

    fseek(fp, 0L, SEEK_SET);
    size_t size = fread(buf, 1, length, fp); // length must be >= 0
    buf[length] = '\0';
    fclose(fp);

    if (mDoc.Parse(buf).HasParseError()) {
        MLOGE(Mia2LogGroupCore, "parse error: (%d:%lu)%s\n", mDoc.GetParseError(),
              mDoc.GetErrorOffset(), GetParseError_En(mDoc.GetParseError()));
        return;
    }

    if (!mDoc.IsObject()) {
        MLOGE(Mia2LogGroupCore, "should be an object!");
        return;
    }

    free(buf);
}

int MiPraseSettings::getIntFromSettingInfo(SettingMask mask, int defaultValue, const char *propName)
{
    if (strlen(propName) > 0) {
        char prop[128] = {0};
        property_get(propName, prop, NULL);
        int propNum = atoi(prop);
        if (propNum > 0) {
            return propNum;
        }
    }

    switch (mask) {
    case offlinedebugmask:
        if (mDoc.HasMember("offlinedebugmask")) {
            return mDoc["offlinedebugmask"].GetInt();
        } else {
            MLOGI(Mia2LogGroupCore, "offlinedebugmask is not find in mivisettings.json");
        }
        break;
    case OpenLIFO:
        if (mDoc.HasMember("OpenLIFO")) {
            return mDoc["OpenLIFO"].GetInt();
        } else {
            MLOGI(Mia2LogGroupCore, "OpenLIFO is not find in mivisettings.json");
        }
        break;
    case Soc:
        if (mDoc.HasMember("Soc")) {
            std::string socStr = mDoc["Soc"].GetString();
            if (socStr.find("MTK") != std::string::npos) {
                return SocType::MTK;
            } else {
                return SocType::QCOM;
            }
        } else {
            MLOGI(Mia2LogGroupCore, "Soc is not find in mivisettings.json");
        }
        break;
    default:
        break;
    }

    return defaultValue;
}

bool MiPraseSettings::getBoolFromSettingInfo(SettingMask mask, bool defaultValue,
                                             const char *propName)
{
    if (strlen(propName) > 0) {
        char prop[128] = {0};
        property_get(propName, prop, NULL);
        int propNum = atoi(prop);
        if (propNum > 0) {
            return propNum;
        }
    }

    switch (mask) {
    case OpenEngineImageDump:
        if (mDoc.HasMember("OpenEngineImageDump")) {
            return mDoc["OpenEngineImageDump"].GetBool();
        } else {
            MLOGI(Mia2LogGroupCore, "OpenEngineImageDump is not find in mivisettings.json");
        }
        break;
    case NeedUpdateFovCropZoomRatio:
        if (mDoc.HasMember("NeedUpdateFovCropZoomRatio")) {
            return mDoc["NeedUpdateFovCropZoomRatio"].GetBool();
        } else {
            MLOGI(Mia2LogGroupCore, "NeedUpdateFovCropZoomRatio is not find in mivisettings.json");
        }
        break;
    case MDBokehOnlyOneSink:
        if (mDoc.HasMember("MDBokehOnlyOneSink")) {
            return mDoc["MDBokehOnlyOneSink"].GetBool();
        } else {
            MLOGI(Mia2LogGroupCore, "MDBokehOnlyOneSink is not find in mivisettings.json");
        }
        break;
    case OpenPrunerAbnormalDump:
        if (mDoc.HasMember("OpenMiaPipelinePrunerDump")) {
            return mDoc["OpenMiaPipelinePrunerDump"].GetBool();
        } else {
            MLOGI(Mia2LogGroupCore, "OpenMiaPipelinePrunerDump is not find in mivisettings.json");
        }
        break;
    default:
        break;
    }

    return defaultValue;
}

std::string MiPraseSettings::getStrFromSettingInfo(SettingMask mask,
                                                   const std::string &defaultValue)
{
    switch (mask) {
    case CurrentTempFilePath:
        if (mDoc.HasMember("CurrentTempFilePath")) {
            return mDoc["CurrentTempFilePath"].GetString();
        } else {
            MLOGI(Mia2LogGroupCore, "CurrentTempFilePath is not find in mivisettings.json");
        }
        break;
    default:
        break;
    }

    return defaultValue;
}

} // namespace midebug
