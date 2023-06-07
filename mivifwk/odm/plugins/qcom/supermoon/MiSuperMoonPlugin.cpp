#include "MiSuperMoonPlugin.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "supermoon.h"

#define MOON_DATA_PATH "/data/vendor/camera/misupermoon"

#define MODELS_DATA_PATH  "/odm/etc/camera/MobNetv2TF_0.35_iter200000_zoom2.5x_h1500w2000.dlc"
#define ADVANCEMOONMODEON (35)

using namespace mialgo2;

MiSuperMoonPlugin::~MiSuperMoonPlugin() {}

int MiSuperMoonPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    supermoon_init = false;
    m_NodeInterface = nodeInterface;
    MLOGD(Mia2LogGroupPlugin, "MiSuperMoon");
    return 0;
}

bool MiSuperMoonPlugin::isEnabled(MiaParams settings)
{
    void *pData = NULL;
    bool supermoonEnable = false;
    bool AiMoonEffect = false;
    int mode = 0;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.supermoon.isEnable", prop, "0");
    int32_t isMoonEnableDebug = (int32_t)atoi(prop);

    pData = NULL;
    const char *TagSupermoonEnable = "xiaomi.ai.asd.AiMoonEffectEnabled";
    VendorMetadataParser::getVTagValue(settings.metadata, TagSupermoonEnable, &pData);
    if (NULL != pData) {
        AiMoonEffect = *static_cast<uint8_t *>(pData);
        MLOGD(Mia2LogGroupPlugin, "AiMoonEffect: %d ", AiMoonEffect);
    } else {
        MLOGW(Mia2LogGroupPlugin, "AiMoonEffect is null ");
    }

    pData = NULL;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.ai.asd.sceneApplied", &pData);
    if (NULL != pData) {
        mode = *static_cast<int *>(pData);
        MLOGD(Mia2LogGroupPlugin, "mode: %d ", mode);
    }
    if (mode == ADVANCEMOONMODEON && (AiMoonEffect == true || isMoonEnableDebug == 1)) {
        supermoonEnable = true;
    }
    MLOGD(Mia2LogGroupPlugin, "supermoonEnable: %d", supermoonEnable);

    return supermoonEnable;
}

ProcessRetStatus MiSuperMoonPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    auto &inputBuffers = pProcessRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = pProcessRequestInfo->outputBuffersMap.begin()->second;
    camera_metadata_t *pMetaData = inputBuffers[0].pMetadata;

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.capture.supermoon.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);

    struct mialgo2::MiImageBuffer inputFrame, outputFrame;
    convertImageParams(inputBuffers[0], inputFrame);
    convertImageParams(outputBuffers[0], outputFrame);

    struct mialgo::MiImageBuffer input, output;
    input.PixelArrayFormat = inputFrame.format;
    input.Width = inputFrame.width;
    input.Height = inputFrame.height;
    input.Pitch[0] = inputFrame.stride;
    input.Plane[0] = inputFrame.plane[0];
    input.Plane[1] = inputFrame.plane[1];
    input.Scanline[0] = inputFrame.scanline;

    output.PixelArrayFormat = outputFrame.format;
    output.Width = outputFrame.width;
    output.Height = outputFrame.height;
    output.Pitch[0] = outputFrame.stride;
    output.Plane[0] = outputFrame.plane[0];
    output.Plane[1] = outputFrame.plane[1];
    output.Scanline[0] = outputFrame.scanline;

    MLOGD(Mia2LogGroupPlugin,
          "%s:%d input width: %d, height: %d, stride: %d, scanline: %d, output stride: %d "
          "scanline: %d",
          __func__, __LINE__, inputFrame.width, inputFrame.height, inputFrame.stride,
          inputFrame.scanline, outputFrame.stride, outputFrame.scanline);

    float zoom = supermoongetZoom(pMetaData);

    // dump input image start
    if (iIsDumpData) {
        char inputbuf[128];
        snprintf(inputbuf, sizeof(inputbuf), "SuperMoon_input_%dx%d_zoom_%f", inputFrame.width,
                 inputFrame.height, zoom);
        PluginUtils::dumpToFile(inputbuf, &inputFrame);
    }
    // dump input image end

    // supermoon_init
    int initresult = 0;
    double t0, t1, t2, t3;
    t0 = t1 = t2 = t3 = 0.0;
    if (!supermoon_init) {
        int res = 0;
        res = mkdir(MOON_DATA_PATH, 0755);
        MLOGD(Mia2LogGroupPlugin, "mkdir return is %d,err=%d ,strerror: %s\n", res, errno,
              strerror(errno));
        t0 = PluginUtils::nowMSec();
        initresult = mialgo::moon_init(MOON_DATA_PATH, MODELS_DATA_PATH);
        t1 = PluginUtils::nowMSec();
        if (initresult == 0) {
            supermoon_init = true;
            MLOGD(Mia2LogGroupPlugin, "supermoon pre init succeed:%d, moon init time %.2f",
                  initresult, t1 - t0);
        } else {
            MLOGW(Mia2LogGroupPlugin, "supermoon pre init time is more than 5s");
        }
    }

    // moon_sr
    int processresult = 0;
    if (supermoon_init) {
        t2 = PluginUtils::nowMSec();
        processresult = mialgo::moon_sr(&input, &output, zoom);
        t3 = PluginUtils::nowMSec();
        MLOGI(Mia2LogGroupPlugin, "moon_sr return %d moon process time %.2f", processresult,
              t3 - t2);

        if (processresult == -2) {
            PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
            MLOGE(Mia2LogGroupPlugin, "[CAM_LOG_SYSTEM] moon_sr process fail %d, zoom %d",
                  processresult, zoom);
        }
    } else {
        PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
    }
    if (m_NodeInterface.pSetResultMetadata != NULL) {
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf),
                 "MiSuperMoon:{on:%d  initres:%d init_time %.2f ret:%d process_time %.2f zoom:%f}",
                 supermoon_init, initresult, t1 - t0, processresult, t3 - t2, zoom);
        std::string results(buf);
        m_NodeInterface.pSetResultMetadata(m_NodeInterface.owner, pProcessRequestInfo->frameNum,
                                           pProcessRequestInfo->timeStamp, results);
    }

    // dump output image start
    if (iIsDumpData) {
        char outputbuf[128];
        snprintf(outputbuf, sizeof(outputbuf), "SuperMoon_output_%dx%d_zoom_%f", outputFrame.width,
                 outputFrame.height, zoom);
        PluginUtils::dumpToFile(outputbuf, &outputFrame);
    }
    // dump output image end

    return resultInfo;
}

ProcessRetStatus MiSuperMoonPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    return resultInfo;
}

int MiSuperMoonPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

void MiSuperMoonPlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);
    if (supermoon_init) {
        mialgo::moon_release();
        supermoon_init = false;
        MLOGD(Mia2LogGroupPlugin, "moon_release \n");
    }
}

float MiSuperMoonPlugin::supermoongetZoom(camera_metadata_t *pMetaData)
{
    void *pData = NULL;
    float zoom = 0.0;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_CONTROL_ZOOM_RATIO, &pData);
    if (NULL != pData) {
        zoom = *static_cast<float *>(pData);
        MLOGD(Mia2LogGroupPlugin, "get zoom =%f", zoom);
    } else {
        MLOGW(Mia2LogGroupPlugin, "get android zoom ratio failed");
    }

    return zoom;
}
