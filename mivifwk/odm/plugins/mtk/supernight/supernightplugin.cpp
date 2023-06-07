#include "supernightplugin.h"

#include "MiaPluginUtils.h"
#include "VendorMetadataParser.h"
#include "system/camera_metadata.h"

#undef LOG_TAG
#define LOG_TAG "SuperNightPlugin"

using namespace mialgo2;

SuperNightPlugin::~SuperNightPlugin()
{
    MLOGE(Mia2LogGroupPlugin, "%s:%d %p", __func__, __LINE__, this);
    if (m_hPortraitSupernight) {
        delete m_hPortraitSupernight;
        m_hPortraitSupernight = NULL;
    }
}

int SuperNightPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    m_hPortraitSupernight = NULL;
    if (!m_hPortraitSupernight) {
        m_hPortraitSupernight = new MiAiSuperLowlight();
        m_hPortraitSupernight->init();
    }
    MLOGI(Mia2LogGroupPlugin, "%s:%d %p", __func__, __LINE__, this);
    mProcessCount = 0;
    return 0;
}

ProcessRetStatus SuperNightPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo;
    memset(&resultInfo, 0, sizeof(resultInfo));
    resultInfo = PROCSUCCESS;
    char prop[PROPERTY_VALUE_MAX];
    memset(bracketEv, 0, FRAME_MAX_INPUT_NUMBER * sizeof(bracketEv[0]));
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.algoengine.supernight.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);

    struct MiImageBuffer inputFrame[FRAME_MAX_INPUT_NUMBER], outputFrame;

    auto &phInputBuffer = pProcessRequestInfo->inputBuffersMap.at(0);
    auto &phOutputBuffer = pProcessRequestInfo->outputBuffersMap.at(0);
    int inputNum = phInputBuffer.size();
    if (inputNum > FRAME_MAX_INPUT_NUMBER) {
        inputNum = FRAME_MAX_INPUT_NUMBER;
    }

    for (int i = 0; i < inputNum; i++) {
        inputFrame[i].format = phInputBuffer[i].format.format;
        inputFrame[i].width = phInputBuffer[i].format.width;
        inputFrame[i].height = phInputBuffer[i].format.height;
        inputFrame[i].plane[0] = phInputBuffer[i].pAddr[0];
        inputFrame[i].plane[1] = phInputBuffer[i].pAddr[1];
        inputFrame[i].stride = phInputBuffer[0].planeStride;
        inputFrame[i].scanline = phInputBuffer[0].sliceheight;
        inputFrame[i].numberOfPlanes = phInputBuffer[0].numberOfPlanes;

        void *pData = NULL;
        auto pMetaData = phInputBuffer[i].pMetadata;
        VendorMetadataParser::getTagValue(pMetaData, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                          &pData);
        if (NULL != pData) {
            int32_t perFrameEv = *static_cast<int32_t *>(pData);
            bracketEv[i] = perFrameEv;
            MLOGD(Mia2LogGroupPlugin, "Get bracketEv[%d] = %d", i, perFrameEv);
        } else {
            bracketEv[i] = 404;
            MLOGE(Mia2LogGroupPlugin, "Failed to get frame %d -> #%d's ev!",
                  pProcessRequestInfo->frameNum, i);
        }

        if (iIsDumpData) {
            MIAI_LLHDR_AEINFO aeInfo = {0};
            GetAEInfo(pMetaData, &aeInfo);

            char inputbuf[128];
            snprintf(inputbuf, sizeof(inputbuf),
                     "miaisn_input[%d]_%dx%d_ev[%d]_luxindex[%.2f]_iso[%d]", i, inputFrame[i].width,
                     inputFrame[i].height, bracketEv[i], aeInfo.fLuxIndex, aeInfo.iso);
            PluginUtils::dumpToFile(inputbuf, &inputFrame[i]);
        }
    }

    outputFrame.format = phOutputBuffer[0].format.format;
    outputFrame.width = phOutputBuffer[0].format.width;
    outputFrame.height = phOutputBuffer[0].format.height;
    outputFrame.plane[0] = phOutputBuffer[0].pAddr[0];
    outputFrame.plane[1] = phOutputBuffer[0].pAddr[1];
    outputFrame.stride = phOutputBuffer[0].planeStride;
    outputFrame.scanline = phOutputBuffer[0].sliceheight;
    outputFrame.numberOfPlanes = phOutputBuffer[0].numberOfPlanes;

    // process here
    auto pMetaData = phInputBuffer[inputNum - 1].pMetadata;
    int ret = ProcessBuffer(inputFrame, inputNum, &outputFrame, pMetaData);

    if (ret != PROCSUCCESS) {
        resultInfo = PROCFAILED;
    }

    MLOGI(Mia2LogGroupPlugin, "%s:%d ProcessBuffer ret: %d", __func__, __LINE__, ret);

    if (iIsDumpData) {
        char outputbuf[128];
        snprintf(outputbuf, sizeof(outputbuf), "miaisn_output%dx%d", outputFrame.width,
                 outputFrame.height);
        PluginUtils::dumpToFile(outputbuf, &outputFrame);
    }

    mProcessCount++;

    return resultInfo;
}

ProcessRetStatus SuperNightPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo;
    memset(&resultInfo, 0, sizeof(resultInfo));
    resultInfo = PROCSUCCESS;

    return resultInfo;
}

int SuperNightPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

void SuperNightPlugin::destroy()
{
    MLOGE(Mia2LogGroupPlugin, "%s:%d %p", __func__, __LINE__, this);
}

bool SuperNightPlugin::isEnabled(MiaParams settings)
{
    void *pData = NULL;
    bool mfnrEnable = false;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.mfnr.enabled", &pData);
    // VendorMetadataParser::getTagValue(settings.metadata, XIAOMI_MFNR_ENABLED, &pData);
    if (NULL != pData) {
        mfnrEnable = *static_cast<uint8_t *>(pData);
    }
    MLOGD(Mia2LogGroupPlugin, "Get mfnrEnable: %d", mfnrEnable);
    return mfnrEnable ? false : true;
}

ProcessRetStatus SuperNightPlugin::ProcessBuffer(struct MiImageBuffer *input, int inputNum,
                                                 struct MiImageBuffer *output,
                                                 camera_metadata_t *pMetaData)
{
    if (pMetaData == NULL) {
        MLOGE(Mia2LogGroupPlugin, "error metaData is null %s", __func__);
        return PROCFAILED;
    }
    MLOGI(Mia2LogGroupPlugin, "ProcessBuffer input_num: %d, format:%d", inputNum, output->format);
    MIAI_LLHDR_AEINFO aeInfo = {0};
    GetAEInfo(pMetaData, &aeInfo);
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
    VendorMetadataParser::getVTagValue(pMetaData, "com.xiaomi.statsconfigs.AecInfo", &pData);
    if (NULL != pData) {
        m3AInfo[0] = static_cast<InputMetadataAecInfo *>(pData)->linearGain;
        m3AInfo[1] = static_cast<InputMetadataAecInfo *>(pData)->exposureTime;
    } else {
        m3AInfo[0] = 1.0f;
        m3AInfo[1] = 1.0f;
    }
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

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.algoengine.supernight.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);
    if (iIsDumpData) {
        char inputbuf[128] = "miaisn_input_aeinfo";
        PluginUtils::dumpStrToFile(
            inputbuf, "Fno value %.2f\nExposure Time %.2f\nISO %d\nLux Index %.2f\nISPGain %.2f\n",
            aeInfo.apture, aeInfo.exposure, aeInfo.iso, aeInfo.fLuxIndex, aeInfo.fISPGain);
    }

    if (m_hPortraitSupernight) {
        m_hPortraitSupernight->process(input, output, bracketEv, inputNum, camMode, m3AInfo[0],
                                       &aeInfo, flashon_mode, orientation);
    } else {
        PluginUtils::miCopyBuffer(output, input);
    }

    return PROCSUCCESS;
}

void SuperNightPlugin::GetAEInfo(camera_metadata_t *metaData, LPMIAI_LLHDR_AEINFO pAeInfo)
{
    void *pData = NULL;
    const char *BpsGain = "com.xiaomi.sensorbps.gain";
    VendorMetadataParser::getVTagValue(metaData, BpsGain, &pData);
    if (NULL != pData) {
        pAeInfo->fSensorGain = *static_cast<float *>(pData);
    } else {
        pAeInfo->fSensorGain = 1.0f;
    }

    pData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "com.xiaomi.statsconfigs.AecInfo", &pData);
    if (NULL != pData) {
        pAeInfo->fLuxIndex = static_cast<InputMetadataAecInfo *>(pData)->luxIndex;
        pAeInfo->fISPGain = static_cast<InputMetadataAecInfo *>(pData)->linearGain;
        pAeInfo->fADRCGain = static_cast<InputMetadataAecInfo *>(pData)->adrcgain;
        pAeInfo->exposure = static_cast<InputMetadataAecInfo *>(pData)->exposureTime;
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

    pData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_LENS_APERTURE, &pData);
    if (NULL != pData) {
        pAeInfo->apture = *static_cast<float *>(pData);
    } else {
        pAeInfo->apture = 88;
    }

    MLOGI(Mia2LogGroupPlugin, "[MIAI_SNPlugin]: fLuxIndex: %.2f fISPGain=%.2f fADRCGain=%.2f ",
          pAeInfo->fLuxIndex, pAeInfo->fISPGain, pAeInfo->fADRCGain);
    MLOGI(Mia2LogGroupPlugin, "[MIAI_SNPlugin]: apture=%.2f iso=%d exposure=%.2f", pAeInfo->apture,
          pAeInfo->iso, pAeInfo->exposure);
}
