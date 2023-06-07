#include "MiReLightPlugin.h"

#include <MiaPluginUtils.h>

#include "MiaPostProcType.h"

using namespace mialgo2;

static const int IsDumpData = property_get_int32("persist.vendor.camera.relight.dump", 0);

static long long __attribute__((unused)) gettime()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((long long)time.tv_sec * 1000 + time.tv_usec / 1000);
}

MiReLightPlugin::~MiReLightPlugin() {}

int MiReLightPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    m_lightingMode = 0;
    m_hRelightingHandle = NULL;
    m_NodeInterface = nodeInterface;
    m_width = pCreateInfo->inputFormat.width;
    m_height = pCreateInfo->inputFormat.height;
    m_operationMode = pCreateInfo->operationMode;
    if (CAM_COMBMODE_FRONT == pCreateInfo->frameworkCameraId ||
        CAM_COMBMODE_FRONT_BOKEH == pCreateInfo->frameworkCameraId ||
        CAM_COMBMODE_FRONT_AUX == pCreateInfo->frameworkCameraId) {
        m_RearCamera = false;
    } else {
        m_RearCamera = true;
    }
    MLOGD(Mia2LogGroupPlugin,
          "MiReLightPlugin operationMode 0x%x RearCamera %d ,width_height: %d_%d", m_operationMode,
          m_RearCamera, m_width, m_height);
    return 0;
}

bool MiReLightPlugin::isEnabled(MiaParams settings)
{
    bool relightenable = false;
    if ((StreamConfigModeBokeh == m_operationMode) && m_RearCamera) {
        relightenable = true;
    }

    return relightenable;
}

ProcessRetStatus MiReLightPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;

    camera_metadata_t *pMetaData = inputBuffers[0].pMetadata;
    int inputPorts = processRequestInfo->outputBuffersMap.size();
    int outputPorts = processRequestInfo->outputBuffersMap.size();
    MLOGI(Mia2LogGroupPlugin, "get relight inputPorts:%d outputPorts:%d", inputPorts, outputPorts);

    void *pData = NULL;
    int orientation = 0;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_JPEG_ORIENTATION, &pData);
    if (NULL != pData) {
        orientation = *static_cast<int *>(pData);
    }

    pData = NULL;
    int light_mode = 0;
    const char *BokehLight = "xiaomi.portrait.lighting";
    VendorMetadataParser::getVTagValue(pMetaData, BokehLight, &pData);
    if (NULL != pData) {
        light_mode = *static_cast<int *>(pData);
    }
    switch (light_mode) {
    case 9:
        m_lightingMode = MI_PL_MODE_NEON_SHADOW;
        break; // 霓影
    case 10:
        m_lightingMode = MI_PL_MODE_PHANTOM;
        break; // 魅影
    case 11:
        m_lightingMode = MI_PL_MODE_NOSTALGIA;
        break; // 怀旧
    case 12:
        m_lightingMode = MI_PL_MODE_RAINBOW;
        break; // 彩虹
    case 13:
        m_lightingMode = MI_PL_MODE_WANING;
        break; // 阑珊
    case 14:
        m_lightingMode = MI_PL_MODE_DAZZLE;
        break; // 炫影
    case 15:
        m_lightingMode = MI_PL_MODE_GORGEOUS;
        break; // 斑斓
    case 16:
        m_lightingMode = MI_PL_MODE_PURPLES;
        break; // 嫣红
    case 17:
        m_lightingMode = MI_PL_MODE_DREAM;
        break; // 梦境
    default:
        m_lightingMode = 0;
        break;
    }
    MLOGD(Mia2LogGroupPlugin, "[MI_CTB] get m_lightingMode: %d", m_lightingMode);

    m_hRelightingHandle = NULL;
    if (m_lightingMode > 0) {
        double t0, t1;
        t0 = gettime();
        int ret =
            AILAB_PLI_Init(&m_hRelightingHandle, MI_PL_INIT_MODE_PERFORMANCE, m_width, m_height);
        t1 = gettime();
        if (ret != MOK) {
            MLOGE(Mia2LogGroupPlugin, "midebug: AILAB_PLI_Init =failed= %d\n", ret);
            AILAB_PLI_Uninit(&m_hRelightingHandle);
            m_hRelightingHandle = NULL;
        }
        MLOGI(Mia2LogGroupPlugin, "right after init, got m_hRelightingHandle:%p, spend time=%.2f ",
              m_hRelightingHandle, t1 - t0);
    }

    struct MiImageBuffer inputFrame, outputFrame, bokehimage, bokehoutimage;
    convertImageParams(inputBuffers[0], inputFrame);
    convertImageParams(outputBuffers[0], outputFrame);

    if (inputPorts == 2) {
        auto &inputBokehBuffers = processRequestInfo->inputBuffersMap.at(1);
        convertImageParams(inputBokehBuffers[0], bokehimage);
    }

    if (outputPorts == 2) {
        auto &outputBokehBuffers = processRequestInfo->outputBuffersMap.at(1);
        convertImageParams(outputBokehBuffers[0], bokehoutimage);
    }
    MLOGD(Mia2LogGroupPlugin,
          "MiAI inputFrame %s:%d >>> input width: %d, height: %d, stride: %d, scanline: %d, "
          "output stride: %d scanline: %d, format:%d",
          __func__, __LINE__, inputFrame.width, inputFrame.height, inputFrame.stride,
          inputFrame.scanline, outputFrame.stride, outputFrame.scanline, inputFrame.format);
    MLOGD(Mia2LogGroupPlugin,
          "MiAI bokehimage %s:%d >>> input width: %d, height: %d, stride: %d, scanline: %d, "
          "output stride: %d scanline: %d, format:%d",
          __func__, __LINE__, bokehimage.width, bokehimage.height, bokehimage.stride,
          bokehimage.scanline, bokehimage.stride, bokehimage.scanline, bokehimage.format);

    if (IsDumpData) {
        char inputbuf[128];
        char inputbuf1[128];
        snprintf(inputbuf, sizeof(inputbuf), "relight_input_%dx%d", inputFrame.width,
                 inputFrame.height);
        PluginUtils::dumpToFile(inputbuf, &inputFrame);

        snprintf(inputbuf1, sizeof(inputbuf1), "relightbokeh_input_%dx%d", bokehimage.width,
                 bokehimage.height);
        PluginUtils::dumpToFile(inputbuf1, &bokehimage);
    }

    if (m_lightingMode > 0) {
        int nRet = PROCSUCCESS;
        double t0, t1, t2, t3;
        MIIMAGEBUFFER pSrcImg;
        MIIMAGEBUFFER pDstImg;
        MIIMAGEBUFFER pBokehImg;
        memset(&pSrcImg, 0, sizeof(MIIMAGEBUFFER));
        memset(&pDstImg, 0, sizeof(MIIMAGEBUFFER));
        memset(&pBokehImg, 0, sizeof(MIIMAGEBUFFER));

        PrepareRelightImage(&pSrcImg, &inputFrame);   // [in] The input image data
        PrepareRelightImage(&pDstImg, &outputFrame);  // [in] The output image data
        PrepareRelightImage(&pBokehImg, &bokehimage); // [in] Then depth image data

        MIPLLIGHTREGION mLightRegion = {{0}};

        if (m_hRelightingHandle) {
            t0 = gettime();
            nRet = AILAB_PLI_PreProcess(m_hRelightingHandle, &pSrcImg, orientation, &mLightRegion);
            t1 = gettime();
            if (nRet != MOK) {
                MLOGE(Mia2LogGroupPlugin, "midebug: AILAB_PLI_PreProcess failed= %d\n", nRet);
                AILAB_PLI_Uninit(&m_hRelightingHandle);
                m_hRelightingHandle = NULL;
            } else {
                MLOGD(Mia2LogGroupPlugin, "midebug:AILAB_PLI_PreProcess spend time =%.2f \n",
                      t1 - t0);
            }
        }

        if (m_hRelightingHandle) {
            MIPLDEPTHINFO lDepthInfo;
            MIPLDEPTHINFO *plDepthInfo = &lDepthInfo;
            MIPLLIGHTPARAM LightParam = {0};
            LightParam.i32LightMode = m_lightingMode;
            lDepthInfo.pBlurImage = &pBokehImg;
            t2 = gettime();
            nRet = AILAB_PLI_Process(m_hRelightingHandle, plDepthInfo, &pSrcImg, &LightParam,
                                     &pDstImg);
            t3 = gettime();

            if (nRet != MOK) {
                MLOGE(Mia2LogGroupPlugin, "midebug: AILAB_PLI_Process failed %d\n", nRet);
                PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
                AILAB_PLI_Uninit(&m_hRelightingHandle);
                m_hRelightingHandle = NULL;
            }
            AILAB_PLI_Uninit(&m_hRelightingHandle);
            m_hRelightingHandle = NULL;
            MLOGI(Mia2LogGroupPlugin, "[MI_CTB]:AILAB_PLI_Process= %d: mode=%d costime=%.2f\n",
                  nRet, m_lightingMode, t3 - t2);
        } else {
            PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        }
    } else {
        PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
    }

    PluginUtils::miCopyBuffer(&bokehoutimage, &bokehimage);

    // dump output image end
    if (IsDumpData) {
        char outputbuf[128];
        char outputbuf1[128];
        snprintf(outputbuf, sizeof(outputbuf), "relight_output_%dx%d", outputFrame.width,
                 outputFrame.height);
        PluginUtils::dumpToFile(outputbuf, &outputFrame);

        snprintf(outputbuf1, sizeof(outputbuf1), "relight_bokehoutput_%dx%d", bokehoutimage.width,
                 bokehoutimage.height);
        PluginUtils::dumpToFile(outputbuf1, &bokehoutimage);
    }

    if (m_NodeInterface.pSetResultMetadata != NULL) {
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf), "mirelight:{lightmode:%d orientation:%d}", m_lightingMode,
                 orientation);
        std::string results(buf);
        m_NodeInterface.pSetResultMetadata(m_NodeInterface.owner, pProcessRequestInfo->frameNum,
                                           pProcessRequestInfo->timeStamp, results);
    }

    if (m_hRelightingHandle) {
        AILAB_PLI_Uninit(&m_hRelightingHandle);
        m_hRelightingHandle = NULL;
    }

    return resultInfo;
}

ProcessRetStatus MiReLightPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    return resultInfo;
}

int MiReLightPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

void MiReLightPlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);

    if (m_hRelightingHandle) {
        AILAB_PLI_Uninit(&m_hRelightingHandle);
        m_hRelightingHandle = NULL;
    }
}

void MiReLightPlugin::PrepareRelightImage(MIIMAGEBUFFER *dstimage, struct MiImageBuffer *srtImage)
{
    MLOGI(Mia2LogGroupPlugin,
          "input bokeh width: %d, height: %d, scanline: %d, format: %d,ppu8Plane:%p %p",
          srtImage->width, srtImage->height, srtImage->scanline, srtImage->format,
          srtImage->plane[0], srtImage->plane[1]);

    dstimage->i32Width = srtImage->width;
    dstimage->i32Height = srtImage->height;
    dstimage->i32Scanline = srtImage->scanline;

    if (srtImage->format == CAM_FORMAT_YUV_420_NV12) {
        dstimage->u32PixelArrayFormat = FORMAT_YUV_420_NV12;
    } else if (srtImage->format == CAM_FORMAT_YUV_420_NV21) {
        dstimage->u32PixelArrayFormat = FORMAT_YUV_420_NV21;
    }

    dstimage->pi32Pitch[0] = srtImage->stride;
    dstimage->pi32Pitch[1] = srtImage->stride;

    dstimage->ppu8Plane[0] = srtImage->plane[0];
    dstimage->ppu8Plane[1] = srtImage->plane[1];

    MLOGI(Mia2LogGroupPlugin,
          "image for relight width: %d, height: %d, scanline: %d, format: %d,ppu8Plane:%p %p",
          dstimage->i32Width, dstimage->i32Height, dstimage->i32Scanline,
          dstimage->u32PixelArrayFormat, dstimage->ppu8Plane[0], dstimage->ppu8Plane[1]);
}
