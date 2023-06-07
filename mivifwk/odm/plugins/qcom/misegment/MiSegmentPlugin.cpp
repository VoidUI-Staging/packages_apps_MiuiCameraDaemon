#include "MiSegmentPlugin.h"

using namespace mialgo2;

static const int32_t sDebugData = property_get_int32("persist.camera.capture.misegment.dump", 0);

MiSegmentPlugin::~MiSegmentPlugin() {}

int MiSegmentPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    double startTime = PluginUtils::nowMSec();
    mIsPreview = 0;
    mNodeInstance = NULL;
    mTimestamp = 0;

    if (!mNodeInstance) {
        mNodeInstance = new MiSegmentMiui();
        mNodeInstance->init(mIsPreview);
    }

    mProcessCount = 0;
    return 0;
}

bool MiSegmentPlugin::isEnabled(MiaParams settings)
{
    void *pData = NULL;
    bool segmentEnable = false;
    const char *SegmentEnabled = "xiaomi.ai.segment.enabled";
    VendorMetadataParser::getVTagValue(settings.metadata, SegmentEnabled, &pData);
    if (NULL != pData) {
        segmentEnable = *static_cast<uint8_t *>(pData);
    }

    return segmentEnable;
}

ProcessRetStatus MiSegmentPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    MLOGD(Mia2LogGroupPlugin, "Now check data %p\n", processRequestInfo);
    ProcessRetStatus resultInfo = PROCSUCCESS;
    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;
    camera_metadata_t *metaData = inputBuffers[0].pMetadata;

    mialgo2::MiImageBuffer inputFrame, outputFrame;
    convertImageParams(inputBuffers[0], inputFrame);
    convertImageParams(outputBuffers[0], outputFrame);

    resultInfo = processBuffer(&inputFrame, &outputFrame, metaData);
    mProcessCount++;
    return resultInfo;
}

ProcessRetStatus MiSegmentPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus MiSegmentPlugin::processBuffer(MiImageBuffer *input, MiImageBuffer *output,
                                                camera_metadata_t *metaData)
{
    void *data = NULL;
    int sceneFlag = 0;
    int32_t rotation = 0;
    int32_t orientation = 0;

    // sceneFlag
    data = NULL;
    const char *AiAsdDetected = "xiaomi.ai.asd.sceneDetected";
    VendorMetadataParser::getVTagValue(metaData, AiAsdDetected, &data);
    if (NULL != data) {
        sceneFlag = *static_cast<int32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "MiaNodeMiSegment get sceneFlag %d", sceneFlag);
    }
    // add ORIENTATION
    data = NULL;
    if (mIsPreview) {
        const char *Device = "xiaomi.device.orientation";
        VendorMetadataParser::getVTagValue(metaData, Device, &data);
        if (NULL != data) {
            orientation = *static_cast<int32_t *>(data);
            mOrientation = (360 - (orientation + 90)) % 360;
        }
    } else {
        VendorMetadataParser::getTagValue(metaData, ANDROID_JPEG_ORIENTATION, &data);
        if (NULL != data) {
            rotation = *static_cast<int32_t *>(data);
            mOrientation = (rotation) % 360;
        }
    }
    // timestamp
    data = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_SENSOR_TIMESTAMP, &data);
    if (NULL != data) {
        mTimestamp = *static_cast<int64_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "MiaNodeMiSegment get mTimestamp %ld", mTimestamp);
    }

    process(input, output, sceneFlag);
    return PROCSUCCESS;
}

void MiSegmentPlugin::process(MiImageBuffer *input, MiImageBuffer *output, int32_t sceneFlag)
{
    int32_t iIsDumpData = sDebugData;

    if (iIsDumpData) {
        char inbuf[128];
        snprintf(inbuf, sizeof(inbuf), "misegment_input_%dx%d", input->stride, input->height);
        PluginUtils::dumpToFile(inbuf, input);
    }
    if (!mNodeInstance) {
        PluginUtils::miCopyBuffer(output, input);
        return;
    }
    if (mNodeInstance)
        mNodeInstance->process(input, output, mIsPreview, mOrientation, sceneFlag, mTimestamp);
    if (iIsDumpData) {
        char outbuf[128];
        snprintf(outbuf, sizeof(outbuf), "misegment_output_%dx%d", output->stride, output->height);
        PluginUtils::dumpToFile(outbuf, output);
    }
}

int MiSegmentPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

void MiSegmentPlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);
    if (mNodeInstance) {
        delete mNodeInstance;
        mNodeInstance = NULL;
    }
}
