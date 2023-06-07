#undef LOG_TAG
#define LOG_TAG "PluginTest"

#include "MiBuf.h"
//#include <MetaCreate.h>

using namespace mialgo2;
///@brief test the basic function of the plugin interface
/// example : memcpyplugin
const char fileNameIn[128] = {"/data/misc/media/input_image0_6016x4512.yuv"};
const char fileNameOut[128] = {"/data/misc/media/output_image_pt_6016x4512.yuv"};

void test(PluginWraper *pluginWraper)
{
    CreateInfo creatInfo;
    creatInfo.frameworkCameraId = 0;
    creatInfo.operationMode = StreamConfigModeTestMemcpy; // No use.
    creatInfo.outputFormat.format = creatInfo.inputFormat.format = gTestFormat;
    creatInfo.outputFormat.width = creatInfo.inputFormat.width = gTestWidth;
    creatInfo.outputFormat.height = creatInfo.inputFormat.height = gTestHeight;

    MiaNodeInterface nodeInterface; // callback most case no use, wait for modify

    int ret = pluginWraper->initialize(&creatInfo, nodeInterface);
    if (ret == 0) {
        MLOGI(Mia2LogGroupCore, "Plugin initialize succeed");
    } else {
        MLOGE(Mia2LogGroupCore, "Plugin initialize failed");
        return;
    }

    ImageParams *input = NULL;
    ImageParams *output = NULL;
    input = initImageParams(fileNameIn, true);
    output = initImageParams(fileNameOut, false);
    if (input == NULL || output == NULL) {
        MLOGE(Mia2LogGroupCore, "Input or output is null");
        return;
    } else {
        MLOGI(Mia2LogGroupCore, "Input and output image load success");
    }

    ProcessRetStatus resultInfo = PROCSUCCESS;
    ProcessRequestInfo processRequestInfo;
    std::vector<ImageParams> inputBuffers;
    std::vector<ImageParams> outputBuffers;
    inputBuffers.push_back(*input);
    outputBuffers.push_back(*output);

    processRequestInfo.frameNum = 1;
    processRequestInfo.inputBuffersMap.insert({0, inputBuffers});
    processRequestInfo.outputBuffersMap.insert({0, outputBuffers});
    resultInfo = pluginWraper->processRequest(&processRequestInfo);
    if (resultInfo == PROCSUCCESS) {
        MLOGI(Mia2LogGroupCore, "Plugin ProcessRequestV1 succeed");
    } else {
        MLOGE(Mia2LogGroupCore, "Plugin ProcessRequestV1 failed");
    }

    ret = writeNV12(outputBuffers[0], fileNameOut);
    if (ret) {
        MLOGI(Mia2LogGroupCore, "Write output file success");
    } else {
        MLOGE(Mia2LogGroupCore, "Write output file failed");
    }

    ProcessRequestInfoV2 processRequestInfoV2;
    processRequestInfoV2.frameNum = 1;
    processRequestInfoV2.inputBuffersMap.insert({0, inputBuffers});
    resultInfo = pluginWraper->processRequest(&processRequestInfoV2);
    if (resultInfo == PROCSUCCESS) {
        MLOGI(Mia2LogGroupCore, "Plugin ProcessRequestV2 succeed");
    } else {
        MLOGE(Mia2LogGroupCore, "Plugin ProcessRequestV2 failed");
    }

    FlushRequestInfo flushRequestInfo;
    flushRequestInfo.frameNum = processRequestInfo.frameNum;
    ret = pluginWraper->flushRequest(&flushRequestInfo);
    if (ret == 0) {
        MLOGI(Mia2LogGroupCore, "Plugin flush succeed");
    } else {
        MLOGE(Mia2LogGroupCore, "Plugin flush failed");
    }

    pluginWraper->destroy();
    MLOGI(Mia2LogGroupCore, "Plugin destroy succeed");

    releaseImageParams(input);
    releaseImageParams(output);

    MLOGI(Mia2LogGroupCore, "Plugin test end");
}

int main()
{
    std::string pluginName = "com.xiaomi.plugin.memcpy";
    Pluma plugins;
    std::vector<PluginWraperProvider *> pluginWraperProviderVec;
    PluginWraper *pluginWraper = NULL;
    // register provider
    plugins.acceptProviderType<PluginWraperProvider>();

    // Load libraries
    plugins.loadFromFolder("plugins");
    // Get device providers into a vector
    plugins.getProviders(pluginWraperProviderVec);

    for (auto i : pluginWraperProviderVec) {
        if (!i->getName().compare(pluginName)) {
            MLOGI(Mia2LogGroupCore, "find pluginName: %s", pluginName.c_str());
            pluginWraper = i->create();
        }
    }

    if (pluginWraper == NULL) {
        MLOGE(Mia2LogGroupCore, "getPluginWraperByPluginName failed");
    } else {
        test(pluginWraper);
    }
    return 0;
}
