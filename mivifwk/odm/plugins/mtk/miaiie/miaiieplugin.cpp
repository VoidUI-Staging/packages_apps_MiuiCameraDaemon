#include "miaiieplugin.h"

#undef LOG_TAG
#define LOG_TAG "MiAiiePlugin"

using namespace mialgo2;

typedef enum enum_flex_yuv_exact_format {
    MTK_CONFIGURE_FLEX_YUV_YV12,
    MTK_CONFIGURE_FLEX_YUV_NV12,
    MTK_CONFIGURE_FLEX_YUV_NV21,
} enum_flex_yuv_exact_format_t;

static long getCurrentTimeStamp()
{
    long ms = 0;
    struct timeval t;
    gettimeofday(&t, NULL);
    ms = t.tv_sec * 1000 + t.tv_usec / 1000;
    return ms;
}

MiAiiePlugin::~MiAiiePlugin()
{
    MLOGD(Mia2LogGroupPlugin, "~MiAiiePlugin()");
}

int MiAiiePlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mProcessCount = 0;
    mInitAIIECapture = false;
    mNodeInstanceCapture = new AIIECapture();
    return 0;
}

bool MiAiiePlugin::isEnabled(MiaParams settings)
{
    void *pData = NULL;
    bool aiieEnable = false;
    const char *aiieCaptureEnableTag = "xiaomi.ai.asd.enabled";
    VendorMetadataParser::getVTagValue(settings.metadata, aiieCaptureEnableTag, &pData);
    if (NULL != pData) {
        aiieEnable = *static_cast<uint8_t *>(pData);
        MLOGD(Mia2LogGroupPlugin, "aiieCaptureEnable = %d", aiieEnable);
    }
    MLOGD(Mia2LogGroupPlugin, "enable aiie %d", aiieEnable);
    return aiieEnable;
}

ProcessRetStatus MiAiiePlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    char prop[PROPERTY_VALUE_MAX];
    int ret = 0;
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.algoengine.miaiie.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.algoengine.miaiie.capture.enable", prop, "1");
    int32_t iSetAiieCaptureEnable = (int32_t)atoi(prop);

    auto &phInputBuffer = processRequestInfo->inputBuffersMap.at(0);
    auto &phOutputBuffer = processRequestInfo->outputBuffersMap.at(0);

    MiImageBuffer inputFrame, outputFrame;
    inputFrame.format = phInputBuffer[0].format.format;
    inputFrame.width = phInputBuffer[0].format.width;
    inputFrame.height = phInputBuffer[0].format.height;
    inputFrame.plane[0] = phInputBuffer[0].pAddr[0];
    inputFrame.plane[1] = phInputBuffer[0].pAddr[1];
    // inputFrame.fd[0] = phInputBuffer[0].fd[0];
    // inputFrame.fd[1] = phInputBuffer[0].fd[0];
    inputFrame.fd[0] = inputFrame.fd[1] = 2; // > 0
    inputFrame.stride = phInputBuffer[0].planeStride;
    inputFrame.scanline = phInputBuffer[0].sliceheight;
    if (inputFrame.plane[0] == NULL || inputFrame.plane[1] == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s:%d wrong input", __func__, __LINE__);
    }
    if (iIsDumpData) {
        char inputbuf[128];
        snprintf(inputbuf, sizeof(inputbuf), "miaiie_input_%dx%d", inputFrame.width,
                 inputFrame.scanline);
        PluginUtils::dumpToFile(inputbuf, &inputFrame);
    }

    outputFrame.format = phOutputBuffer[0].format.format;
    outputFrame.width = phOutputBuffer[0].format.width;
    outputFrame.height = phOutputBuffer[0].format.height;
    outputFrame.plane[0] = phOutputBuffer[0].pAddr[0];
    outputFrame.plane[1] = phOutputBuffer[0].pAddr[1];
    // outputFrame.fd[0] = phOutputBuffer[0].fd[0];
    // outputFrame.fd[1] = phOutputBuffer[0].fd[1];
    outputFrame.fd[0] = outputFrame.fd[1] = 2; // > 0
    outputFrame.stride = phOutputBuffer[0].planeStride;
    outputFrame.scanline = phOutputBuffer[0].sliceheight;
    if (outputFrame.plane[0] == NULL || outputFrame.plane[1] == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s:%d wrong output", __func__, __LINE__);
    }

    camera_metadata_t *metaData = phInputBuffer[0].pMetadata;
    void *data = NULL;
    bool AIIECaptureEnable = false;

    VendorMetadataParser::getVTagValue(metaData, "xiaomi.ai.asd.enabled", &data);
    if (NULL != data) {
        AIIECaptureEnable = *static_cast<uint8_t *>(data);
        MLOGD(Mia2LogGroupPlugin, "%s:%d get aiasd enabled: %d", __func__, __LINE__,
              AIIECaptureEnable);
    }

    MLOGD(Mia2LogGroupPlugin,
          "%s:%d input width: %d, height: %d, stride: %d, scanline: %d, output stride: %d "
          "scanline: %d",
          __func__, __LINE__, inputFrame.width, inputFrame.height, inputFrame.stride,
          inputFrame.scanline, outputFrame.stride, outputFrame.scanline);

    if (AIIECaptureEnable && iSetAiieCaptureEnable) {
        MLOGD(Mia2LogGroupPlugin, "run ProcessBuffer");
        ret = processBuffer(&inputFrame, &outputFrame, metaData);
    } else {
        ret = PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        MLOGD(Mia2LogGroupPlugin, "%s:%d miCopyBuffer ret: %d", __func__, __LINE__, ret);
    }
    if (ret != 0) {
        resultInfo = PROCFAILED;
    }

    if (iIsDumpData) {
        char outputBuf[128];
        snprintf(outputBuf, sizeof(outputBuf), "miaiie_output_%dx%d", outputFrame.width,
                 outputFrame.height);
        PluginUtils::dumpToFile(outputBuf, &outputFrame);
    }

    mProcessCount++;
    return resultInfo;
}

int MiAiiePlugin::processBuffer(MiImageBuffer *input, MiImageBuffer *output,
                                camera_metadata_t *metaData)
{
    std::unique_lock<std::mutex> lock(mProcessMutex);
    int result = 0;
    if (metaData == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s:%d error metaData is null", __func__, __LINE__);
        return -1;
    }
    if (mNodeInstanceCapture) {
        if (!mInitAIIECapture) {
            MLOGI(Mia2LogGroupPlugin,
                  "%s:%d MiaiiePlugin AI Image Enhancement Capture algo Version = %s", __func__,
                  __LINE__, mNodeInstanceCapture->getVersion());

            result = mNodeInstanceCapture->init(input);
            if (result != 0) {
                MLOGE(Mia2LogGroupPlugin, "%s:%d MiaiiePlugin ERROR init false \n", __func__,
                      __LINE__);
                PluginUtils::miCopyBuffer(output, input);
                return result;
            }
            mInitAIIECapture = true;
        }

        // input->format is get from com.mediatek.configure.setting.exactflexyuvfmt
        // input format is the actual bufer format,AIIE algo need the actual bufer
        enum_flex_yuv_exact_format_t bufferFormat = MTK_CONFIGURE_FLEX_YUV_YV12;
        if (NULL != metaData) {
            void *pFormatData = NULL;
            // get exactflexyuvfmt for YUV format
            VendorMetadataParser::getVTagValue(
                metaData, "com.mediatek.configure.setting.exactflexyuvfmt", &pFormatData);
            if (NULL != pFormatData) {
                bufferFormat = *static_cast<enum_flex_yuv_exact_format_t *>(pFormatData);
            } else {
                MLOGD(Mia2LogGroupPlugin, "get MTK_CONFIGURE_FLEX_YUV_EXACT_FORMAT failed");
            }
        }
        if (bufferFormat == MTK_CONFIGURE_FLEX_YUV_NV12)
            output->format = input->format = CAM_FORMAT_YUV_420_NV12;
        else
            output->format = input->format = CAM_FORMAT_YUV_420_NV21;

        MLOGD(Mia2LogGroupPlugin, "input.format 0x%x output.format= 0x%x\n", input->format,
              output->format);

        long t1 = getCurrentTimeStamp();
        result = mNodeInstanceCapture->process(input, output);
        MLOGI(Mia2LogGroupPlugin, "AIIE process return %d, cost time: %ldms", result,
              getCurrentTimeStamp() - t1);

        if (result != 0) {
            MLOGE(Mia2LogGroupPlugin, "%s:%d MiaiiePlugin ERROR process false \n", __func__,
                  __LINE__);
            PluginUtils::miCopyBuffer(output, input);
            return result;
        }
    } else {
        MLOGI(Mia2LogGroupPlugin, "%s:%d Miaiie Now miCopyBuffer\n", __func__, __LINE__);
        PluginUtils::miCopyBuffer(output, input);
    }

    return result;
}

ProcessRetStatus MiAiiePlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int MiAiiePlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

void MiAiiePlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, "Destory");
    if (mNodeInstanceCapture) {
        delete mNodeInstanceCapture;
        mNodeInstanceCapture = NULL;
    }
}
