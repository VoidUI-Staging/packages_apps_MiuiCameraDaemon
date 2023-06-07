#ifndef __MI_SETTINGS__
#define __MI_SETTINGS__
#include <string>

#include "MiPraseSettings.h"

namespace midebug {

class MiaInternalProp
{
public:
    static MiaInternalProp *getInstance();
    int getInt(SettingMask mask);
    bool getBool(SettingMask mask);
    const std::string &getString(SettingMask mask, std::string &currentFilePath);

private:
    MiaInternalProp();
    MiaInternalProp(const MiaInternalProp &);
    MiaInternalProp &operator=(const MiaInternalProp &);
    ~MiaInternalProp(){};

private:
    bool mOpenEngineImageDump;
    int mOpenLIFO;
    bool mNeedUpdateFovCropZoomRatio;
    bool mMDBokehOnlyOneSink;
    bool mOpenPrunerAbnormalDump;
    std::string mCurrentTempFilePath;
    int mSocType;
};

} // namespace midebug

#endif
