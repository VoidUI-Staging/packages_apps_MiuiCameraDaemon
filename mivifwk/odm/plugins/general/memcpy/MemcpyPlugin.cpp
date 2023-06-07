#include "MemcpyPlugin.h"

#include "MiaPluginUtils.h"

using namespace mialgo2;

static const int32_t sIsDumpData =
    property_get_int32("persist.vendor.camera.algoengine.memcpy.dump", 0);

MemcpyPlugin::~MemcpyPlugin()
{
    MLOGI(Mia2LogGroupPlugin, "%p", this);
}

int MemcpyPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    MLOGI(Mia2LogGroupPlugin, "%p", this);
    mNodeInterface = nodeInterface;
    return 0;
}

ProcessRetStatus MemcpyPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    int32_t isDumpData = sIsDumpData;

    MiImageBuffer inputFrame = {0}, outputFrame = {0};
    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;

    convertImageParams(inputBuffers[0], inputFrame);
    convertImageParams(outputBuffers[0], outputFrame);

    MLOGI(Mia2LogGroupPlugin,
          "input %dx%d,sw=%d,sh=%d,numOfPlanes=%d,fmt=0x%x, "
          "output %dx%d, sw=%d,sh=%d,fmt=0x%x",
          inputFrame.width, inputFrame.height, inputFrame.stride, inputFrame.scanline,
          inputFrame.numberOfPlanes, inputFrame.format, outputFrame.width, outputFrame.height,
          outputFrame.stride, outputFrame.scanline, outputFrame.format);

    if (isDumpData) {
        char inputBuf[128];
        snprintf(inputBuf, sizeof(inputBuf), "memcpy_input_%dx%d", inputFrame.width,
                 inputFrame.height);
        PluginUtils::dumpToFile(inputBuf, &inputFrame);
    }

    int ret = PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
    if (ret == -1) {
        resultInfo = PROCFAILED;
    }
    MLOGI(Mia2LogGroupPlugin, "miCopyBuffer ret: %d", ret);

    if (isDumpData) {
        char outputBuf[128];
        snprintf(outputBuf, sizeof(outputBuf), "memcpy_output_%dx%d", outputFrame.width,
                 outputFrame.height);
        PluginUtils::dumpToFile(outputBuf, &outputFrame);
    }

    // test pSetResultMetadata callback
    // MLOGI(Mia2LogGroupPlugin, "mNodeInterface owner: %p, pSetResultMetadata: %p",
    // mNodeInterface.owner, mNodeInterface.pSetResultMetadata);
    // mNodeInterface.pSetResultMetadata(mNodeInterface.owner, processRequestInfo->frameNum,
    // processRequestInfo->phInputBuffer[0].pMetadata);

    // test get static camera metadata
#if 0
    int cameraId = 6;
    camera_metadata_t* staticMetadata = StaticMetadataWraper::GetInstance()->getStaticMetadata(cameraId);
    void* data = NULL;
    VendorMetadataParser::getVTagValue(staticMetadata, "com.xiaomi.cameraid.role.cameraId", &data);
    if (NULL != data)
    {
        int roleID = *static_cast<int *>(data);
        MLOGI(Mia2LogGroupPlugin, "get cameraid: %d, roleID: %d ", cameraId, roleID);
    }
#endif

    MLOGI(Mia2LogGroupPlugin, "test");
    return resultInfo;
}

ProcessRetStatus MemcpyPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int MemcpyPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

void MemcpyPlugin::destroy()
{
    MLOGI(Mia2LogGroupPlugin, "%p", this);
}
