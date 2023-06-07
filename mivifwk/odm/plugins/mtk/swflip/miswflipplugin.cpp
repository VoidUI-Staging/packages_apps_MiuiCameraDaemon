#include "miswflipplugin.h"

#include <xiaomi/MiMetaData.h>

#undef LOG_TAG
#define LOG_TAG "MiSwFlipPlugin"

using namespace mialgo2;

MiSwFlipPlugin::~MiSwFlipPlugin() {}

int MiSwFlipPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    MLOGD(Mia2LogGroupPlugin, "Initialize");
    mProcessCount = 0;
    mNodeInterface = nodeInterface;
    return 0;
}

bool MiSwFlipPlugin::isEnabled(MiaParams settings)
{
    void *pData = NULL;
    bool bFlipEnable = false;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.flip.enabled", &pData);
    // VendorMetadataParser::getTagValue(settings.metadata, XIAOMI_SENSOR_FLIP_ENABLED, &pData);
    if (NULL != pData) {
        bFlipEnable = *static_cast<uint8_t *>(pData);
    }

    return bFlipEnable;
}

ProcessRetStatus MiSwFlipPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    MLOGD(Mia2LogGroupPlugin, "Now check data %p\n", pProcessRequestInfo);
    ProcessRetStatus result;
    result = PROCSUCCESS;

    auto &phInputBuffer = pProcessRequestInfo->inputBuffersMap.at(0);
    auto &phOutputBuffer = pProcessRequestInfo->outputBuffersMap.at(0);
    void *pMetaData = (void *)phInputBuffer[0].pMetadata;

    MiImageBuffer inputFrame, outputFrame;
    inputFrame.format = phInputBuffer[0].format.format;
    inputFrame.width = phInputBuffer[0].format.width;
    inputFrame.height = phInputBuffer[0].format.height;
    inputFrame.plane[0] = phInputBuffer[0].pAddr[0];
    inputFrame.plane[1] = phInputBuffer[0].pAddr[1];
    inputFrame.stride = phInputBuffer[0].planeStride;
    inputFrame.scanline = phInputBuffer[0].sliceheight;
    if (inputFrame.plane[0] == NULL || inputFrame.plane[1] == NULL) {
        MLOGE(Mia2LogGroupPlugin, "wrong input");
    }
    outputFrame.format = phOutputBuffer[0].format.format;
    outputFrame.width = phOutputBuffer[0].format.width;
    outputFrame.height = phOutputBuffer[0].format.height;
    outputFrame.plane[0] = phOutputBuffer[0].pAddr[0];
    outputFrame.plane[1] = phOutputBuffer[0].pAddr[1];
    outputFrame.stride = phOutputBuffer[0].planeStride;
    outputFrame.scanline = phOutputBuffer[0].sliceheight;
    if (outputFrame.plane[0] == NULL || outputFrame.plane[1] == NULL) {
        MLOGE(Mia2LogGroupPlugin, "wrong output");
    }
    MiaParams settings = {.metadata = (camera_metadata_t *)pMetaData};
    if (!isEnabled(settings)) {
        PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
    } else {
        result = processBuffer(&inputFrame, &outputFrame, pMetaData);
        mProcessCount++;
    }
    return result;
}

ProcessRetStatus MiSwFlipPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    // ProcessResultInfo resultInfo;
    // resultInfo.result = PROCSUCCESS;
    // resultInfo.isBypassed = false;

    return PROCSUCCESS;
}

ProcessRetStatus MiSwFlipPlugin::processBuffer(MiImageBuffer *input, MiImageBuffer *output,
                                               void *metaData)
{
    void *pData = NULL;
    int32_t orientation = 0;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.capture.swflip.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);

    // add ORIENTATION
    // MetadataUtils::getInstance()->getTagValue(metaData, ANDROID_JPEG_ORIENTATION, &pData);
    VendorMetadataParser::getTagValue((camera_metadata_t *)metaData, ANDROID_JPEG_ORIENTATION,
                                      &pData);
    if (NULL != pData) {
        orientation = *static_cast<int32_t *>(pData);
        m_orientation = (orientation) % 360;
    }

    if (iIsDumpData) {
        char inbuf[128];
        snprintf(inbuf, sizeof(inbuf), "swflip_input_%dx%d", input->stride, input->height);
        PluginUtils::dumpToFile(inbuf, input);
    }

    process(input, output);

    if (iIsDumpData) {
        char outbuf[128];
        snprintf(outbuf, sizeof(outbuf), "swflip_output_%dx%d", output->stride, output->height);
        PluginUtils::dumpToFile(outbuf, output);
    }

    return PROCSUCCESS;
}

void MiSwFlipPlugin::process(MiImageBuffer *input, MiImageBuffer *output)
{
    int width = input->width;
    int height = input->height;
    int stride = input->stride;
    uint8_t *input_y = input->plane[0];
    uint8_t *input_uv = input->plane[1];
    uint8_t *output_y = output->plane[0];
    uint8_t *output_uv = output->plane[1];

    if ((m_orientation % 180) != 0) {
        int idx = 0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < stride; j += 2) {
                output_y[idx++] = *(input_y + stride * (height - 1 - i) + j);
                output_y[idx++] = *(input_y + stride * (height - 1 - i) + j + 1);
            }
        }

        // UV
        idx = 0;
        for (int i = 0; i < height / 2; i++) {
            for (int j = 0; j < stride; j += 2) {
                output_uv[idx++] = *(input_uv + stride * (height / 2 - 1 - i) + j);
                output_uv[idx++] = *(input_uv + stride * (height / 2 - 1 - i) + j + 1);
            }
        }
    } else {
        int idx = 0;
        for (int i = 0; i < height * stride; i += stride) {
            for (int j = 0; j < width; j++) {
                output_y[idx++] = *(input_y + i + width - 1 - j);
            }
            idx += (stride - width);
        }

        // UV
        idx = 0;
        for (int i = 0; i < height * stride / 2; i += stride) {
            for (int j = 0; j < width; j += 2) {
                output_uv[idx++] = *(input_uv + i + width - 2 - j);
                output_uv[idx++] = *(input_uv + i + width - 1 - j);
            }
            idx += (stride - width);
        }
    }
}

int MiSwFlipPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

void MiSwFlipPlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, " %p", this);
}
