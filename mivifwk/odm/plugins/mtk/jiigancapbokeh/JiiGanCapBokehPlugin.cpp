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

    // prot0 ->Tele frame
    auto &phInputBuffer = processRequestInfo->inputBuffersMap.at(0);
    // prot0 ->Bokeh frame
    auto &phOutputBuffer = processRequestInfo->outputBuffersMap.at(0);
    MLOGD(Mia2LogGroupPlugin, "info: input.size = %lu, output.size = %lu", phInputBuffer.size(),
          phOutputBuffer.size());

    camera_metadata_t *pMeta = phInputBuffer[0].pMetadata;

    struct MiImageBuffer inputTeleFrame, inputWideFrame, outputBokehFrame, outputTeleFrame,
        outputDepth;
    inputTeleFrame.format = phInputBuffer[0].format.format;
    inputTeleFrame.width = phInputBuffer[0].format.width;
    inputTeleFrame.height = phInputBuffer[0].format.height;
    inputTeleFrame.plane[0] = phInputBuffer[0].pAddr[0];
    inputTeleFrame.plane[1] = phInputBuffer[0].pAddr[1];
    inputTeleFrame.stride = phInputBuffer[0].planeStride;
    inputTeleFrame.scanline = phInputBuffer[0].sliceheight;
    inputTeleFrame.numberOfPlanes = 2;

    // prot1 ->Wide frame
    auto &phInputBuffer1 = processRequestInfo->inputBuffersMap.at(1);
    inputWideFrame.format = phInputBuffer1[0].format.format;
    inputWideFrame.width = phInputBuffer1[0].format.width;
    inputWideFrame.height = phInputBuffer1[0].format.height;
    inputWideFrame.plane[0] = phInputBuffer1[0].pAddr[0];
    inputWideFrame.plane[1] = phInputBuffer1[0].pAddr[1];
    inputWideFrame.stride = phInputBuffer1[0].planeStride;
    inputWideFrame.scanline = phInputBuffer1[0].sliceheight;
    inputWideFrame.numberOfPlanes = 2;

    outputBokehFrame.format = phOutputBuffer[0].format.format;
    outputBokehFrame.width = phOutputBuffer[0].format.width;
    outputBokehFrame.height = phOutputBuffer[0].format.height;
    outputBokehFrame.plane[0] = phOutputBuffer[0].pAddr[0];
    outputBokehFrame.plane[1] = phOutputBuffer[0].pAddr[1];
    outputBokehFrame.stride = phOutputBuffer[0].planeStride;
    outputBokehFrame.scanline = phOutputBuffer[0].sliceheight;
    outputBokehFrame.numberOfPlanes = 2;

    // prot1 ->Tele frame
    auto &phOutputBuffer1 = processRequestInfo->outputBuffersMap.at(1);
    outputTeleFrame.format = phOutputBuffer1[0].format.format;
    outputTeleFrame.width = phOutputBuffer1[0].format.width;
    outputTeleFrame.height = phOutputBuffer1[0].format.height;
    outputTeleFrame.plane[0] = phOutputBuffer1[0].pAddr[0];
    outputTeleFrame.plane[1] = phOutputBuffer1[0].pAddr[1];
    outputTeleFrame.stride = phOutputBuffer1[0].planeStride;
    outputTeleFrame.scanline = phOutputBuffer1[0].sliceheight;
    outputTeleFrame.numberOfPlanes = 2;

    // prot2 ->Depth frame
    auto &phOutputBuffer2 = processRequestInfo->outputBuffersMap.at(2);
    outputDepth.format = phOutputBuffer2[0].format.format; // CAM_FORMAT_Y16
    outputDepth.width = phOutputBuffer2[0].format.width;
    outputDepth.height = phOutputBuffer2[0].format.height;
    outputDepth.plane[0] = phOutputBuffer2[0].pAddr[0];
    outputDepth.stride = phOutputBuffer2[0].planeStride;
    outputDepth.scanline = phOutputBuffer2[0].sliceheight;
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

    std::string results;
    int ret = mBokehInstance->processBuffer(&inputTeleFrame, &inputWideFrame, &outputBokehFrame,
                                            &outputTeleFrame, &outputDepth, pMeta, results);
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
