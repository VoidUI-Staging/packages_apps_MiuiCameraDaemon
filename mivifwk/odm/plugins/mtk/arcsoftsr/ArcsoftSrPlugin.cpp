#include "ArcsoftSrPlugin.h"

#include <MiaPluginUtils.h>
#include <VendorMetadataParser.h>
#include <stdio.h>
#include <string.h>
#include <utils/Log.h>
#define FRAME_MAX_INPUT_NUMBER 10

using namespace mialgo2;

ArcsoftSrPlugin::~ArcsoftSrPlugin()
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);
}

int ArcsoftSrPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    MLOGD(Mia2LogGroupPlugin, "%s:%d %p", __func__, __LINE__, this);

    mProcessCount = 0;
    m_width = createInfo->inputFormat.width;
    m_height = createInfo->inputFormat.height;
    mARCSOFTSR = new mialgo2::ArcSoftSR(m_width, m_height);

    return 0;
}

bool ArcsoftSrPlugin::isEnabled(MiaParams settings)
{
    void *pData = NULL;
    int srEnable = false;
    const char *SrEnable = "xiaomi.superResolution.enabled";
    VendorMetadataParser::getVTagValue(settings.metadata, SrEnable, &pData);
    if (NULL != pData) {
        srEnable = *static_cast<int *>(pData);
    }
    MLOGD(Mia2LogGroupPlugin, "Get SrEnable: %d", srEnable);

    pData = NULL;
    int hdrSrEnable = false;
    const char *HdrSrEnable = "xiaomi.hdr.sr.enabled";
    VendorMetadataParser::getVTagValue(settings.metadata, HdrSrEnable, &pData);
    if (NULL != pData) {
        hdrSrEnable = *static_cast<int *>(pData);
    }
    MLOGD(Mia2LogGroupPlugin, "Get hdrSrEnable: %d", hdrSrEnable);

    if (srEnable || hdrSrEnable) {
        return true;
    } else {
        return false;
    }
}

ProcessRetStatus ArcsoftSrPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    auto &phInputBuffer = processRequestInfo->inputBuffersMap.at(0);
    auto &phOutputBuffer = processRequestInfo->outputBuffersMap.at(0);

    int inputNum = phInputBuffer.size();
    if (inputNum > FRAME_MAX_INPUT_NUMBER) {
        inputNum = FRAME_MAX_INPUT_NUMBER;
    }

    camera_metadata_t *pMetaData[FRAME_MAX_INPUT_NUMBER] = {NULL};
    camera_metadata_t *pPhysMeta[FRAME_MAX_INPUT_NUMBER] = {NULL};

    MiImageBuffer inputFrame[FRAME_MAX_INPUT_NUMBER];
    MiImageBuffer outputFrame;
    for (int i = 0; i < inputNum; i++) {
        inputFrame[i].format = phInputBuffer[i].format.format;
        inputFrame[i].width = phInputBuffer[i].format.width;
        inputFrame[i].height = phInputBuffer[i].format.height;
        inputFrame[i].plane[0] = phInputBuffer[i].pAddr[0];
        inputFrame[i].plane[1] = phInputBuffer[i].pAddr[1];
        inputFrame[i].stride = phInputBuffer[0].planeStride;
        inputFrame[i].scanline = phInputBuffer[0].sliceheight;
        pMetaData[i] = phInputBuffer[i].pMetadata;
        pPhysMeta[i] = phInputBuffer[i].pPhyCamMetadata;
        if (NULL == pMetaData[i]) {
            return resultInfo;
        }
    }

    outputFrame.format = phOutputBuffer[0].format.format;
    outputFrame.width = phOutputBuffer[0].format.width;
    outputFrame.height = phOutputBuffer[0].format.height;
    outputFrame.plane[0] = phOutputBuffer[0].pAddr[0];
    outputFrame.plane[1] = phOutputBuffer[0].pAddr[1];
    outputFrame.stride = phOutputBuffer[0].planeStride;
    outputFrame.scanline = phOutputBuffer[0].sliceheight;

    MLOGI(Mia2LogGroupPlugin,
          "%s:%d input width: %d, height: %d, stride: %d, scanline: %d, output stride: %d "
          "scanline: %d",
          __func__, __LINE__, inputFrame[0].width, inputFrame[0].height, inputFrame[0].stride,
          inputFrame[0].scanline, outputFrame.stride, outputFrame.scanline);
    int ret = mARCSOFTSR->ProcessBuffer(inputFrame, inputNum, &outputFrame, pMetaData, pPhysMeta);
    if (ret != PROCSUCCESS) {
        resultInfo = PROCFAILED;
    }

    MLOGD(Mia2LogGroupPlugin, "ProcessBuffer ret: %d", ret);

    mProcessCount++;

    return resultInfo;
}

ProcessRetStatus ArcsoftSrPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int ArcsoftSrPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

void ArcsoftSrPlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);
    if (mARCSOFTSR != NULL) {
        delete mARCSOFTSR;
        mARCSOFTSR = NULL;
    }
}
