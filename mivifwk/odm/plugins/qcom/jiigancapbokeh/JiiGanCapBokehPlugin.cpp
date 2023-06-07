#include "JiiGanCapBokehPlugin.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

using namespace mialgo2;

#undef LOG_TAG
#define LOG_TAG "MIA_N_JG_RCBK"

int JiiGanCapBokhPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    MLOGI(Mia2LogGroupPlugin, "[JG_CTB] Initialize E");

    mBokehInstance = NULL;
    if (!mBokehInstance) {
        mBokehInstance = new MiaNodeIntSenseCapBokeh();
        mBokehInstance->init(createInfo->frameworkCameraId);
    }

    MLOGI(Mia2LogGroupPlugin, "[JG_CTB] Initialize");

    return 0;
}

ProcessRetStatus JiiGanCapBokhPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    return resultInfo;
}

ProcessRetStatus JiiGanCapBokhPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.algoengine.jgcapbokeh.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);

    camera_metadata_t *pMeta = processRequestInfo->phInputBuffer[0].pMetadata;

    struct MiImageBuffer inputTeleFrame, inputWideFrame, outputBokehFrame, outputTeleFrame,
        outputDepth;
    inputTeleFrame.format = processRequestInfo->phInputBuffer[0].format.format;
    inputTeleFrame.width = processRequestInfo->phInputBuffer[0].format.width;
    inputTeleFrame.height = processRequestInfo->phInputBuffer[0].format.height;
    inputTeleFrame.plane[0] = processRequestInfo->phInputBuffer[0].pAddr[0];
    inputTeleFrame.plane[1] = processRequestInfo->phInputBuffer[0].pAddr[1];
    inputTeleFrame.stride = processRequestInfo->phInputBuffer[0].planeStride;
    inputTeleFrame.scanline = processRequestInfo->phInputBuffer[0].sliceheight;
    inputTeleFrame.numberOfPlanes = 2;

    inputWideFrame.format = processRequestInfo->phInputBuffer[1].format.format;
    inputWideFrame.width = processRequestInfo->phInputBuffer[1].format.width;
    inputWideFrame.height = processRequestInfo->phInputBuffer[1].format.height;
    inputWideFrame.plane[0] = processRequestInfo->phInputBuffer[1].pAddr[0];
    inputWideFrame.plane[1] = processRequestInfo->phInputBuffer[1].pAddr[1];
    inputWideFrame.stride = processRequestInfo->phInputBuffer[1].planeStride;
    inputWideFrame.scanline = processRequestInfo->phInputBuffer[1].sliceheight;
    inputWideFrame.numberOfPlanes = 2;

    outputBokehFrame.format = processRequestInfo->phOutputBuffer[0].format.format;
    outputBokehFrame.width = processRequestInfo->phOutputBuffer[0].format.width;
    outputBokehFrame.height = processRequestInfo->phOutputBuffer[0].format.height;
    outputBokehFrame.plane[0] = processRequestInfo->phOutputBuffer[0].pAddr[0];
    outputBokehFrame.plane[1] = processRequestInfo->phOutputBuffer[0].pAddr[1];
    outputBokehFrame.stride = processRequestInfo->phOutputBuffer[0].planeStride;
    outputBokehFrame.scanline = processRequestInfo->phOutputBuffer[0].sliceheight;
    outputBokehFrame.numberOfPlanes = 2;

    outputTeleFrame.format = processRequestInfo->phOutputBuffer[1].format.format;
    outputTeleFrame.width = processRequestInfo->phOutputBuffer[1].format.width;
    outputTeleFrame.height = processRequestInfo->phOutputBuffer[1].format.height;
    outputTeleFrame.plane[0] = processRequestInfo->phOutputBuffer[1].pAddr[0];
    outputTeleFrame.plane[1] = processRequestInfo->phOutputBuffer[1].pAddr[1];
    outputTeleFrame.stride = processRequestInfo->phOutputBuffer[1].planeStride;
    outputTeleFrame.scanline = processRequestInfo->phOutputBuffer[1].sliceheight;
    outputTeleFrame.numberOfPlanes = 2;

    outputDepth.format = processRequestInfo->phOutputBuffer[2].format.format; // CAM_FORMAT_Y16
    outputDepth.width = processRequestInfo->phOutputBuffer[2].format.width;
    outputDepth.height = processRequestInfo->phOutputBuffer[2].format.height;
    outputDepth.plane[0] = processRequestInfo->phOutputBuffer[2].pAddr[0];
    outputDepth.stride = processRequestInfo->phOutputBuffer[2].planeStride;
    outputDepth.scanline = processRequestInfo->phOutputBuffer[2].sliceheight;
    outputDepth.numberOfPlanes = 1;

    MLOGI(Mia2LogGroupPlugin,
          "[JG_CTB] %s:%d input tele width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          __func__, __LINE__, inputTeleFrame.width, inputTeleFrame.height, inputTeleFrame.stride,
          inputTeleFrame.scanline, inputTeleFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "[JG_CTB] %s:%d input wide width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          __func__, __LINE__, inputWideFrame.width, inputWideFrame.height, inputWideFrame.stride,
          inputWideFrame.scanline, inputWideFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "[JG_CTB] %s:%d output bokeh width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          __func__, __LINE__, outputBokehFrame.width, outputBokehFrame.height,
          outputBokehFrame.stride, outputBokehFrame.scanline, outputBokehFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "[JG_CTB] %s:%d output tele width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          __func__, __LINE__, outputTeleFrame.width, outputTeleFrame.height, outputTeleFrame.stride,
          outputTeleFrame.scanline, outputTeleFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "[JG_CTB] %s:%d output depth width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          __func__, __LINE__, outputDepth.width, outputDepth.height, outputDepth.stride,
          outputDepth.scanline, outputDepth.format);

    if (iIsDumpData) {
        char inputtelebuf[128];
        snprintf(inputtelebuf, sizeof(inputtelebuf), "jgcapbokeh_input_tele_%dx%d",
                 inputTeleFrame.width, inputTeleFrame.height);
        PluginUtils::dumpToFile(inputtelebuf, &inputTeleFrame);
        char inputwidebuf[128];
        snprintf(inputwidebuf, sizeof(inputwidebuf), "jgcapbokeh_input_wide_%dx%d",
                 inputWideFrame.width, inputWideFrame.height);
        PluginUtils::dumpToFile(inputwidebuf, &inputWideFrame);
    }

    int ret = mBokehInstance->processBuffer(&inputTeleFrame, &inputWideFrame, &outputBokehFrame,
                                            &outputTeleFrame, &outputDepth, pMeta);
    if (ret != PROCSUCCESS) {
        resultInfo = PROCFAILED;
    }
    MLOGI(Mia2LogGroupPlugin, "[JG_CTB] %s:%d miCopyBuffer ret: %d", __func__, __LINE__, ret);

    if (iIsDumpData) {
        char outputbokehbuf[128];
        snprintf(outputbokehbuf, sizeof(outputbokehbuf), "jgcapbokeh_output_bokeh_%dx%d",
                 outputBokehFrame.width, outputBokehFrame.height);
        PluginUtils::dumpToFile(outputbokehbuf, &outputBokehFrame);
        char outputtelebuf[128];
        snprintf(outputtelebuf, sizeof(outputtelebuf), "jgcapbokeh_output_tele_%dx%d",
                 outputTeleFrame.width, outputTeleFrame.height);
        PluginUtils::dumpToFile(outputtelebuf, &outputTeleFrame);
        char outputdepthbuf[128];
        snprintf(outputdepthbuf, sizeof(outputdepthbuf), "jgcapbokeh_output_depth_%dx%d",
                 outputDepth.width, outputDepth.height);
        PluginUtils::dumpToFile(outputdepthbuf, &outputDepth);
    }

    return resultInfo;
}

int JiiGanCapBokhPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    MLOGI(Mia2LogGroupPlugin, "[JG_CTB] JiiGanCapBokhPlugin x");
    return 0;
}

void JiiGanCapBokhPlugin::destroy()
{
    if (mBokehInstance) {
        delete mBokehInstance;
        mBokehInstance = NULL;
    }
    MLOGI(Mia2LogGroupPlugin, "[JG_CTB] JiiGanCapBokhPlugin x");
}

JiiGanCapBokhPlugin::~JiiGanCapBokhPlugin()
{
    MLOGI(Mia2LogGroupPlugin, "[JG_CTB] ~JiiGanCapBokhPlugin");
};
