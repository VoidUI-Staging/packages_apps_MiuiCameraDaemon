#include "MiaSettings.h"

#include "MiDebugUtils.h"

namespace midebug {

MiaInternalProp *MiaInternalProp::getInstance()
{
    static MiaInternalProp sMiaInternalProp;
    return &sMiaInternalProp;
}

MiaInternalProp::MiaInternalProp()
{
    mOpenEngineImageDump = MiPraseSettings::getInstance()->getBoolFromSettingInfo(
        OpenEngineImageDump, false, "persist.vendor.camera.mialgoengine.opendump");
    mOpenLIFO = MiPraseSettings::getInstance()->getIntFromSettingInfo(
        OpenLIFO, 0, "persist.vendor.camera.mialgoengine.openlifo");
    mOpenPrunerAbnormalDump = MiPraseSettings::getInstance()->getBoolFromSettingInfo(
        OpenPrunerAbnormalDump, true, "persist.vendor.camera.mialgoengine.pipelineprunerdump");

    mCurrentTempFilePath = MiPraseSettings::getInstance()->getStrFromSettingInfo(
        CurrentTempFilePath, "/sys/class/thermal/thermal_message/board_sensor_temp");
    mNeedUpdateFovCropZoomRatio =
        MiPraseSettings::getInstance()->getBoolFromSettingInfo(NeedUpdateFovCropZoomRatio, false);
    mMDBokehOnlyOneSink =
        MiPraseSettings::getInstance()->getBoolFromSettingInfo(MDBokehOnlyOneSink, false);
    mSocType = MiPraseSettings::getInstance()->getIntFromSettingInfo(Soc, 0, "");

    MLOGI(Mia2LogGroupCore,
          "mOpenEngineImageDump:[%d] mOpenLIFO:[%d] mNeedUpdateFovCropZoomRatio:[%d] "
          "mOpenPipelinePrunerAbnormalDump:[%d] mMDBokehOnlyOneSink:[%d] mCurrentTempFilePath:[%s]",
          mOpenEngineImageDump, mOpenLIFO, mNeedUpdateFovCropZoomRatio, mOpenPrunerAbnormalDump,
          mMDBokehOnlyOneSink, mCurrentTempFilePath.c_str());
}

int MiaInternalProp::getInt(SettingMask mask)
{
    switch (mask) {
    case Soc:
        return mSocType;
    default:
        break;
    }

    return -1;
}

bool MiaInternalProp::getBool(SettingMask mask)
{
    switch (mask) {
    case OpenEngineImageDump:
        return mOpenEngineImageDump;
    case OpenLIFO:
        return OpenLIFO > 0 ? true : false;
    case NeedUpdateFovCropZoomRatio:
        return mNeedUpdateFovCropZoomRatio;
    case MDBokehOnlyOneSink:
        return mMDBokehOnlyOneSink;
    case OpenPrunerAbnormalDump:
        return mOpenPrunerAbnormalDump > 0 ? true : false;
    default:
        break;
    }

    return false;
}

const std::string &MiaInternalProp::getString(SettingMask mask, std::string &currentFilePath)
{
    switch (mask) {
    case CurrentTempFilePath:
        currentFilePath = mCurrentTempFilePath;
        break;
    default:
        break;
    }

    return currentFilePath;
}

} // namespace midebug
