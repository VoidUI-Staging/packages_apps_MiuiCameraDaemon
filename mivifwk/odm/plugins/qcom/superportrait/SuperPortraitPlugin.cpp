#include "SuperPortraitPlugin.h"

#include "MiaPluginUtils.h"

using namespace mialgo2;

SuperPortraitPlugin::~SuperPortraitPlugin()
{
    MLOGD(Mia2LogGroupPlugin, "%s:%d %p", __func__, __LINE__, this);
}

int SuperPortraitPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    MLOGD(Mia2LogGroupPlugin, "%s:%d %p", __func__, __LINE__, this);
    mProcessCount = 0;
    mSuperportraitLevel = 0;
    mNodeInstance = NULL;

    mSuperportraitMode = PORTRAIT_MODE_CAPTURE;
    mOrientation = 0;

    if (!mNodeInstance) {
        mNodeInstance = new SuperPortrait();
    }
    return 0;
}

ProcessRetStatus SuperPortraitPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.algoengine.superportrait.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);

    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;
    camera_metadata_t *pMetaData = inputBuffers[0].pMetadata;

    mialgo2::MiImageBuffer inputFrame, outputFrame;
    convertImageParams(inputBuffers[0], inputFrame);
    convertImageParams(outputBuffers[0], outputFrame);

    MLOGI(Mia2LogGroupPlugin,
          "%s:%d input width: %d, height: %d, stride: %d, scanline: %d, output stride: %d "
          "scanline: %d",
          __func__, __LINE__, inputFrame.width, inputFrame.height, inputFrame.stride,
          inputFrame.scanline, outputFrame.stride, outputFrame.scanline);

    if (iIsDumpData) {
        char inputbuf[128];
        snprintf(inputbuf, sizeof(inputbuf), "superportrait_input_%dx%d", inputFrame.width,
                 inputFrame.height);
        PluginUtils::dumpToFile(inputbuf, &inputFrame);
    }

    int ret = 0;
    if (mNodeInstance) {
        ret = mNodeInstance->process(&inputFrame, &outputFrame, mOrientation, mSuperportraitMode);
    } else {
        ret = PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
    }
    if (ret != 0) {
        resultInfo = PROCFAILED;
    }
    MLOGI(Mia2LogGroupPlugin, "%s:%d ProcessBuffer ret: %d", __func__, __LINE__, ret);

    if (iIsDumpData) {
        char outputbuf[128];
        snprintf(outputbuf, sizeof(outputbuf), "superportrait_output_%dx%d", outputFrame.width,
                 outputFrame.height);
        PluginUtils::dumpToFile(outputbuf, &outputFrame);
    }

    mProcessCount++;

    return resultInfo;
}

ProcessRetStatus SuperPortraitPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int SuperPortraitPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

void SuperPortraitPlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, "%s:%d %p", __func__, __LINE__, this);
    if (mNodeInstance) {
        delete mNodeInstance;
        mNodeInstance = NULL;
    }
}

void SuperPortraitPlugin::updateSuperPortraitParams(camera_metadata_t *metaData)
{
    if (metaData == NULL) {
        MLOGI(Mia2LogGroupPlugin, "%s error metaData is null", __func__);
        return;
    }

    void *data = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_JPEG_ORIENTATION, &data);
    if (NULL != data) {
        mOrientation = *static_cast<uint32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "get mOrientation = %d", mOrientation);
    }

    data = NULL;
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.superportrait.enabled", &data);
    if (NULL != data) {
        mSuperportraitLevel = *static_cast<uint32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "arcdebug superportrait: mSuperportraitLevel= %d\n",
              mSuperportraitLevel);
    }
}
