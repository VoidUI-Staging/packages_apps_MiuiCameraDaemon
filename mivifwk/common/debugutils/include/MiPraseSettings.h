#ifndef __MIPRASESETTINGS__
#define __MIPRASESETTINGS__

#include <android/log.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>

#include <cstdio>
#include <map>
#include <string>

#include "rapidjson/Document.h"
#undef LOG_TAG
#define LOG_TAG "MiAlgoEngine"

namespace midebug {

typedef enum SocType { QCOM, MTK } SocType;

typedef enum {
    offlinedebugmask,
    OpenEngineImageDump,
    OpenLIFO,
    CurrentTempFilePath,
    NeedUpdateFovCropZoomRatio,
    MDBokehOnlyOneSink,
    OpenPrunerAbnormalDump,
    Soc
} SettingMask;

class MiPraseSettings
{
public:
    static MiPraseSettings *getInstance();
    int getIntFromSettingInfo(SettingMask, int defaultValue, const char *propName = "");
    bool getBoolFromSettingInfo(SettingMask, bool defaultValue, const char *propName = "");
    std::string getStrFromSettingInfo(SettingMask, const std::string &defaultValue);

private:
    MiPraseSettings();
    MiPraseSettings(const MiPraseSettings &);
    MiPraseSettings &operator=(const MiPraseSettings &);

private:
    rapidjson::Document mDoc;
};

} // namespace midebug

#endif
