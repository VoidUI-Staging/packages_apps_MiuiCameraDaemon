#include "MiAiPortraitSuperNightPlugin.h"

#include <algorithm>

#undef LOG_TAG
#define LOG_TAG "MiAiPortraitSuperNightPlugin"

using namespace mialgo2;

// Eg: adb shell setprop persist.vendor.algoengine.supernight.dump   1 >>> enable dump
// Eg: adb shell setprop persist.vendor.algoengine.supernight.bypass 1 >>> bypass plugin

const static uint32_t mNeedDump =
    property_get_int32("persist.vendor.algoengine.supernight.dump", 0);
const static uint32_t mNeedBypass =
    property_get_int32("persist.vendor.algoengine.supernight.bypass", 0);

const static std::string macePath = "/sdcard/Android/data/com.android.camera/files";

MiAiPortraitSuperNightPlugin::~MiAiPortraitSuperNightPlugin()
{
    MLOGD(Mia2LogGroupPlugin, "%s:%d %p", __func__, __LINE__, this);
}

int MiAiPortraitSuperNightPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mPortraitSupernight = NULL;
    if (!mPortraitSupernight) {
        mPortraitSupernight = new MiAiSuperLowlight();
        mPortraitSupernight->init();
    }
    MLOGD(Mia2LogGroupPlugin, "%s:%d %p", __func__, __LINE__, this);
    mProcessCount = 0;
    mNodeInterface = nodeInterface;
    MLOGI(Mia2LogGroupPlugin, "front supernight prop {dump: 0x%x, bypass: 0x%x}", mNeedDump,
          mNeedBypass);
    return 0;
}

bool MiAiPortraitSuperNightPlugin::isEnabled(MiaParams settings)
{
    void *pData = nullptr;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.supernight.enabled", &pData);
    isFrontnightEnabled = pData ? *static_cast<int *>(pData) : 0;
    MLOGI(Mia2LogGroupPlugin, "xiaomi.supernight.enabled %d", isFrontnightEnabled);
    if (isFrontnightEnabled) {
        return true;
    } else {
        return false;
    }
}

int32_t MiAiPortraitSuperNightPlugin::bracketEv[FRAME_MAX_INPUT_NUMBER] = {0, 0, 0, 0, 0, 0, 0, 0};
ProcessRetStatus MiAiPortraitSuperNightPlugin::processRequest(
    ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    char prop[PROPERTY_VALUE_MAX];
    memset(bracketEv, 0, FRAME_MAX_INPUT_NUMBER * sizeof(bracketEv[0]));

    struct MiImageBuffer inputFrame[FRAME_MAX_INPUT_NUMBER], outputFrame;
    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;
    int inputNum = inputBuffers.size();
    if (inputNum > FRAME_MAX_INPUT_NUMBER) {
        inputNum = FRAME_MAX_INPUT_NUMBER;
    }

    for (int i = 0; i < inputNum; i++) {
        convertImageParams(inputBuffers[i], inputFrame[i]);

        void *data = NULL;
        camera_metadata_t *metaData = inputBuffers[i].pMetadata;
        VendorMetadataParser::getTagValue(metaData, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                          &data);
        if (NULL != data) {
            int32_t perFrameEv = *static_cast<int32_t *>(data);
            bracketEv[i] = perFrameEv;
            MLOGD(Mia2LogGroupPlugin, "Get bracketEv[%d] = %d", i, perFrameEv);
        } else {
            bracketEv[i] = 404;
            MLOGD(Mia2LogGroupPlugin, "Failed to get frame %d -> #%d's ev!",
                  processRequestInfo->frameNum, i);
        }

        if (mNeedDump) {
            char inputbuf[128];
            snprintf(inputbuf, sizeof(inputbuf), "miaisn_input[%d]_%dx%d_ev[%d]", i,
                     inputFrame[i].width, inputFrame[i].height, bracketEv[i]);
            PluginUtils::dumpToFile(inputbuf, &inputFrame[i]);
        }
    }

    convertImageParams(outputBuffers[0], outputFrame);

    mOutFrameIdx = processRequestInfo->frameNum;
    mOutFrameTimeStamp = processRequestInfo->timeStamp;
    // process here
    camera_metadata_t *metaData = inputBuffers[inputNum - 1].pMetadata;
    int ret = processBuffer(inputFrame, inputNum, &outputFrame, metaData, mNeedBypass);

    if (ret != PROCSUCCESS) {
        resultInfo = PROCFAILED;
    }

    MLOGI(Mia2LogGroupPlugin, "%s:%d miCopyBuffer ret: %d", __func__, __LINE__, ret);

    if (mNeedDump) {
        char outputbuf[128];
        snprintf(outputbuf, sizeof(outputbuf), "miaisn_output%dx%d", outputFrame.width,
                 outputFrame.height);
        PluginUtils::dumpToFile(outputbuf, &outputFrame);
    }

    mProcessCount++;

    return resultInfo;
}

ProcessRetStatus MiAiPortraitSuperNightPlugin::processRequest(
    ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int MiAiPortraitSuperNightPlugin::flushRequest(FlushRequestInfo *pflushRequestInfo)
{
    return 0;
}

void MiAiPortraitSuperNightPlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, "%s:%d %p", __func__, __LINE__, this);
    if (mPortraitSupernight) {
        delete mPortraitSupernight;
        mPortraitSupernight = NULL;
    }
}

ProcessRetStatus MiAiPortraitSuperNightPlugin::processBuffer(struct MiImageBuffer *input,
                                                             int inputNum,
                                                             struct MiImageBuffer *output,
                                                             camera_metadata_t *pMetaData,
                                                             int bypass)
{
    if (pMetaData == NULL) {
        MLOGE(Mia2LogGroupPlugin, "error metaData is null %s", __func__);
        return PROCFAILED;
    }
    MLOGI(Mia2LogGroupPlugin, "processBuffer input_num: %d, format:%d", inputNum, output->format);
    MIAI_LLHDR_AEINFO aeInfo = {0};
    getAEInfo(pMetaData, &aeInfo);
    int camMode = HARDCODE_CAMERAID_FRONT;
    void *pData = NULL;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_JPEG_ORIENTATION, &pData);
    int orientation = 0;
    if (NULL != pData) {
        orientation = *static_cast<int32_t *>(pData);
        orientation = (orientation) % 360;
        MLOGI(Mia2LogGroupPlugin, "getMetaData jpeg orientation  %d", orientation);
    }
    float m3AInfo[2] = {1.0f};
    pData = NULL;
    VendorMetadataParser::getVTagValue(pMetaData, "org.quic.camera2.statsconfigs.AECFrameControl",
                                       &pData);
    if (NULL != pData) {
        m3AInfo[0] =
            static_cast<AECFrameControl *>(pData)->exposureInfo[ExposureIndexSafe].linearGain;
        m3AInfo[1] =
            static_cast<AECFrameControl *>(pData)->exposureInfo[ExposureIndexSafe].exposureTime;
    } else {
        m3AInfo[0] = 1.0f;
        m3AInfo[1] = 1.0f;
    }
    MLOGI(Mia2LogGroupPlugin, "MIAI_SN, getMeta AECFrameControl linearGain %f exposureTime %f",
          m3AInfo[0], m3AInfo[1]);

    int flashon_mode = 0;
    pData = NULL;
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.snapshot.front.ScreenLighting.enabled",
                                       &pData);
    if (pData != NULL) {
        flashon_mode = *static_cast<int32_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "MIAI_SN, getMeta flashon_mode result:%d", flashon_mode);
    } else {
        MLOGI(Mia2LogGroupPlugin, "MIAI_SN, getMeta flashon_mode error %d", flashon_mode);
    }

    if (mNeedDump) {
        char inputbuf[128] = "miaisn_input_aeinfo";
        PluginUtils::dumpStrToFile(
            inputbuf, "Fno value %.2f\nExposure Time %.2f\nISO %d\nLux Index %.2f\nISPGain %.2f\n",
            aeInfo.apture, aeInfo.exposure, aeInfo.iso, aeInfo.fLuxIndex, aeInfo.fISPGain);
    }

    AdditionalParam additionalparam = {0};
    additionalparam.macePath = const_cast<char *>(macePath.c_str());
    if (mPortraitSupernight) {
        MLOGI(Mia2LogGroupPlugin, "process");
        mPortraitSupernight->process(input, output, bracketEv, inputNum, camMode, m3AInfo[0],
                                     &aeInfo, flashon_mode, orientation, bypass, additionalparam);
    } else {
        MLOGI(Mia2LogGroupPlugin, "bypass");
        PluginUtils::miCopyBuffer(output, input);
    }

    std::string frontnight_exif_info = "portraitSN:{";
    if (bypass) {
        frontnight_exif_info += "on:0,";
    } else {
        frontnight_exif_info += "on:1,";
    }
    if (mPortraitSupernight) {
        frontnight_exif_info += "version:" + mPortraitSupernight->getversion() + ",";
    }
    frontnight_exif_info += "fnoValue:" + toStr(aeInfo.apture, 3) + ",";
    frontnight_exif_info += "expTime:" + toStr(aeInfo.exposure, 1) + ",";
    frontnight_exif_info += "iso:" + toStr(aeInfo.iso) + ",";
    frontnight_exif_info += "luxIdx:" + toStr(aeInfo.fLuxIndex, 3) + ",";
    frontnight_exif_info += "ispGain:" + toStr(aeInfo.fISPGain, 3) + "}";
    MLOGI(Mia2LogGroupPlugin, "update front night exif info: %s", frontnight_exif_info.c_str());
    if (mNodeInterface.pSetResultMetadata != NULL) {
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, mOutFrameIdx, mOutFrameTimeStamp,
                                          frontnight_exif_info);
    }
    return PROCSUCCESS;
}

void MiAiPortraitSuperNightPlugin::getAEInfo(camera_metadata_t *metaData,
                                             LPMIAI_LLHDR_AEINFO pAeInfo)
{
    void *pData = NULL;
    const char *BpsGain = "com.qti.sensorbps.gain";
    VendorMetadataParser::getVTagValue(metaData, BpsGain, &pData);
    if (NULL != pData) {
        pAeInfo->fSensorGain = *static_cast<float *>(pData);
    } else {
        pAeInfo->fSensorGain = 1.0f;
    }

    pData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "org.quic.camera2.statsconfigs.AECFrameControl",
                                       &pData);
    if (NULL != pData) {
        pAeInfo->fLuxIndex = static_cast<AECFrameControl *>(pData)->luxIndex;
        pAeInfo->fISPGain =
            static_cast<AECFrameControl *>(pData)->exposureInfo[ExposureIndexSafe].linearGain;
        pAeInfo->fADRCGain =
            static_cast<AECFrameControl *>(pData)->exposureInfo[ExposureIndexSafe].sensitivity /
            static_cast<AECFrameControl *>(pData)->exposureInfo[ExposureIndexShort].sensitivity;
        pAeInfo->exposure =
            static_cast<AECFrameControl *>(pData)->exposureInfo[ExposureIndexSafe].exposureTime;
    } else {
        pAeInfo->fLuxIndex = 50.0f;
        pAeInfo->fISPGain = 1.0f;
        pAeInfo->fADRCGain = 1.0f;
        pAeInfo->exposure = 1.0f;
    }

    pData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_SENSOR_SENSITIVITY, &pData);
    if (NULL != pData) {
        pAeInfo->iso = *static_cast<int32_t *>(pData);
    } else {
        pAeInfo->iso = 100;
    }
    MLOGI(Mia2LogGroupPlugin, "MIAI_SNPlugin get iso %d", pAeInfo->iso);

    pData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_LENS_APERTURE, &pData);
    if (NULL != pData) {
        pAeInfo->apture = *static_cast<float *>(pData);
    } else {
        pAeInfo->apture = 88;
    }

    MLOGD(Mia2LogGroupPlugin, "[MIAI_SNPlugin]: fLuxIndex: %.2f fISPGain=%.2f fADRCGain=%.2f ",
          pAeInfo->fLuxIndex, pAeInfo->fISPGain, pAeInfo->fADRCGain);
    MLOGD(Mia2LogGroupPlugin, "[MIAI_SNPlugin]: apture=%.2f aaa iso=%d exposure=%.2f",
          pAeInfo->apture, pAeInfo->iso, pAeInfo->exposure);
}
