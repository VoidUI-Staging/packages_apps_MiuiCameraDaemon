#include "MiBokehPlugin.h"

/*for xiaomi portrait light*/
#include "mi_portrait_3d_lighting_image.h"
#include "mi_portrait_lighting_common.h"

namespace mialgo2 {

typedef enum {
    Default,
    BokehOnly,     // 1 output   bokeh             for thridpart camera
    BokehDepth,    //
    BokehDepthRaw, // 3 output   raw,bokeh,depth   for miui camera
} MibokehOutputMode;

Mibokehplugin::~Mibokehplugin() {}

int Mibokehplugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    double startTime = PluginUtils::nowMSec();
    mCurrentFlip = 0;
    mNodeInstance = NULL;
    mSceneFlag = 0;
    mDepthMap = NULL;
    mOrientation = 0;
    mOrient = 0;
    mRelightingHandle = NULL;
    mNodeInterface = nodeInterface;

    if (NULL == mNodeInstance) {
        mNodeInstance = new MiBokehMiui();
        mNodeInstance->init(false);
    }

    MLOGI(Mia2LogGroupPlugin, "MiBokeh");

    return MIA_RETURN_OK;
}

void Mibokehplugin::destroy()
{
    /*begin add for front 3D light*/
    if (mDepthMap != NULL) {
        MLOGI(Mia2LogGroupPlugin, "free mDepthMap %p", mDepthMap);
        free(mDepthMap);
        mDepthMap = NULL;
        mDepthSize = 0;
    }

    if (mRelightingHandle) {
        AILAB_PLI_Uninit(&mRelightingHandle);
        mRelightingHandle = NULL;
    }
    /*end add for front 3D light*/

    if (NULL != mNodeInstance) {
        delete mNodeInstance;
        mNodeInstance = NULL;
    }
}

bool Mibokehplugin::isEnabled(MiaParams settings)
{
    void *data = NULL;
    int32_t mibokehEnable = 0;
    const char *mibokeh = "xiaomi.bokeh.enabled";
    VendorMetadataParser::getVTagValue(settings.metadata, mibokeh, &data);
    if (NULL != data) {
        mibokehEnable = *static_cast<int32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "Mibokehplugin getMetaData MibokehEnabled  %d", mibokehEnable);
    } else {
        MLOGW(Mia2LogGroupPlugin, "can not found tag \"xiaomi.bokeh.enabled\"");
    }
    return mibokehEnable ? true : false;
}

ProcessRetStatus Mibokehplugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.algoengine.mibokeh.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);
    mProcessedFrame = processRequestInfo->frameNum;
    MibokehOutputMode outputmode = Default;
    outputmode = MibokehOutputMode(processRequestInfo->outputBuffersMap.size());
    MLOGI(Mia2LogGroupPlugin, "output buffer number = %d", outputmode);

    double start = PluginUtils::nowMSec();
    switch (outputmode) {
    case BokehOnly:
        processBokehOnly(iIsDumpData, processRequestInfo);
        break;
    case BokehDepth:
        break;
    case BokehDepthRaw:
        processBokehDepthRaw(iIsDumpData, processRequestInfo);
        break;
    default:
        break;
    }
    double end = PluginUtils::nowMSec();

    if (mNodeInterface.pSetResultMetadata != NULL) {
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf),
                 "miBokeh:{processedFrame:%d blurLevel:%.3f sceneFlag:%d costTime:%.3f}",
                 mProcessedFrame, mNumberApplied, mSceneFlag, end - start);
        std::string results(buf);
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, processRequestInfo->frameNum,
                                          processRequestInfo->timeStamp, results);
    }

    return resultInfo;
};

void Mibokehplugin::processBokehDepthRaw(int32_t iIsDumpData,
                                         ProcessRequestInfo *processRequestInfo)
{
    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    camera_metadata_t *metadata = inputBuffers[0].pMetadata;

    MiImageBuffer inputFrame = {0};
    MiImageBuffer outputBokehFrame = {0};
    MiImageBuffer outputRawFrame = {0};
    MiImageBuffer outputDepthFrame = {0};
    convertImageParams(inputBuffers[0], inputFrame);

    convertImageParams(processRequestInfo->outputBuffersMap.at(0)[0],
                       outputBokehFrame); // prot0 ->bokeh frame
    convertImageParams(processRequestInfo->outputBuffersMap.at(1)[0],
                       outputRawFrame); // prot1 ->raw frame
    convertImageParams(processRequestInfo->outputBuffersMap.at(2)[0],
                       outputDepthFrame); // prot2 ->depth frame

    MLOGI(Mia2LogGroupPlugin,
          "%s:%d input width: %d, height: %d, stride: %d, scanline: %d, format: %d", __func__,
          __LINE__, inputFrame.width, inputFrame.height, inputFrame.stride, inputFrame.scanline,
          inputFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "%s:%d output bokeh width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          __func__, __LINE__, outputBokehFrame.width, outputBokehFrame.height,
          outputBokehFrame.stride, outputBokehFrame.scanline, outputBokehFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "%s:%d output raw width: %d, height: %d, stride: %d, scanline: %d, format: %d", __func__,
          __LINE__, outputRawFrame.width, outputRawFrame.height, outputRawFrame.stride,
          outputRawFrame.scanline, outputRawFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "%s:%d output depth width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          __func__, __LINE__, outputDepthFrame.width, outputDepthFrame.height,
          outputDepthFrame.stride, outputDepthFrame.scanline, outputDepthFrame.format);

    if (iIsDumpData) {
        char inbuf[128];
        snprintf(inbuf, sizeof(inbuf), "mibokeh_input_%dx%d", inputFrame.width, inputFrame.height);
        PluginUtils::dumpToFile(inbuf, &inputFrame);
    }

    PluginUtils::miCopyBuffer(&outputRawFrame, &inputFrame);
    int ret = processBuffer(&inputFrame, &outputBokehFrame, &outputDepthFrame, metadata);
    if (ret) {
        MLOGE(Mia2LogGroupPlugin, "[CAM_LOG_SYSTEM] MiBokeh processBuffer fail %d", ret);
    }

    if (iIsDumpData) {
        char outputbokehbuf[128];
        snprintf(outputbokehbuf, sizeof(outputbokehbuf), "mibokeh_output_bokeh_%dx%d",
                 outputBokehFrame.width, outputBokehFrame.height);
        PluginUtils::dumpToFile(outputbokehbuf, &outputBokehFrame);
        char outputRawbuf[128];
        snprintf(outputRawbuf, sizeof(outputRawbuf), "mibokeh_output_Raw_%dx%d",
                 outputRawFrame.width, outputRawFrame.height);
        PluginUtils::dumpToFile(outputRawbuf, &outputRawFrame);
        char outputdepthbuf[128];
        snprintf(outputdepthbuf, sizeof(outputdepthbuf), "mibokeh_output_depth_%dx%d",
                 outputDepthFrame.width, outputDepthFrame.height);
        PluginUtils::dumpToFile(outputdepthbuf, &outputDepthFrame);
    }
}

void Mibokehplugin::processBokehOnly(int32_t iIsDumpData, ProcessRequestInfo *processRequestInfo)
{
    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;
    camera_metadata_t *metadata = inputBuffers[0].pMetadata;

    MiImageBuffer inputFrame = {0};
    MiImageBuffer outputBokehFrame = {0};
    MiImageBuffer outputDepthFrame = {0};

    convertImageParams(inputBuffers[0], inputFrame);
    convertImageParams(outputBuffers[0], outputBokehFrame); // prot0 ->bokeh frame

    outputDepthFrame.format = CAM_FORMAT_Y16; // CAM_FORMAT_Y16
    outputDepthFrame.width = outputBuffers[0].format.width / 2;
    outputDepthFrame.height = outputBuffers[0].format.height / 2;
    outputDepthFrame.stride = outputBuffers[0].planeStride / 2;
    outputDepthFrame.scanline = outputBuffers[0].sliceheight / 2;
    outputDepthFrame.plane[0] =
        (uint8_t *)malloc(outputDepthFrame.stride * outputDepthFrame.scanline);
    outputDepthFrame.numberOfPlanes = 1;

    MLOGI(Mia2LogGroupPlugin,
          "%s:%d input width: %d, height: %d, stride: %d, scanline: %d, format: %d", __func__,
          __LINE__, inputFrame.width, inputFrame.height, inputFrame.stride, inputFrame.scanline,
          inputFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "%s:%d output bokeh width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          __func__, __LINE__, outputBokehFrame.width, outputBokehFrame.height,
          outputBokehFrame.stride, outputBokehFrame.scanline, outputBokehFrame.format);

    if (iIsDumpData) {
        char inbuf[128];
        snprintf(inbuf, sizeof(inbuf), "mibokeh_input_%dx%d", inputFrame.width, inputFrame.height);
        PluginUtils::dumpToFile(inbuf, &inputFrame);
    }

    int ret = processBuffer(&inputFrame, &outputBokehFrame, &outputDepthFrame, metadata);
    if (ret) {
        MLOGE(Mia2LogGroupPlugin, "[CAM_LOG_SYSTEM] MiBokeh processBuffer fail %d", ret);
    }
    free(outputDepthFrame.plane[0]);

    if (iIsDumpData) {
        char outputbokehbuf[128];
        snprintf(outputbokehbuf, sizeof(outputbokehbuf), "mibokeh_output_bokeh_%dx%d",
                 outputBokehFrame.width, outputBokehFrame.height);
        PluginUtils::dumpToFile(outputbokehbuf, &outputBokehFrame);
    }
}

ProcessRetStatus Mibokehplugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int Mibokehplugin::processBuffer(MiImageBuffer *input, MiImageBuffer *outputBokeh,
                                 MiImageBuffer *outputDepth, camera_metadata_t *metadata)
{
    int result = MIA_RETURN_OK;
    if (metadata == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s error metadata is null", __func__);
        return MIA_RETURN_INVALID_PARAM;
    }

    // get flip
    void *data = NULL;
    const char *flip = "xiaomi.flip.enabled";
    VendorMetadataParser::getVTagValue(metadata, flip, &data);
    if (NULL != data) {
        // mCurrentFlip = *static_cast<uint32_t *>(data); //Fix me
        MLOGI(Mia2LogGroupPlugin, "Mibokehplugin getMetaData mCurrentFlip  %d", mCurrentFlip);
    }

    // get BokehFNumber
    data = NULL;
    char fNumber[8] = {0};
    const char *BokehFNumber = "xiaomi.bokeh.fNumberApplied";
    VendorMetadataParser::getVTagValue(metadata, BokehFNumber, &data);
    if (NULL != data) {
        memcpy(fNumber, static_cast<char *>(data), sizeof(fNumber));
        mNumberApplied = atof(fNumber);
        MLOGI(Mia2LogGroupPlugin, "Mibokehplugin getMetaData mNumberApplied  %f", mNumberApplied);
    }

    // get ai sceneDetected
    data = NULL;
    const char *aiAsdDetected = "xiaomi.ai.asd.sceneDetected";
    VendorMetadataParser::getVTagValue(metadata, aiAsdDetected, &data);
    if (NULL != data) {
        mSceneFlag = *static_cast<int32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "Mibokehplugin getMetaData mSceneFlag  %d", mSceneFlag);
    }

    // get ORIENTATION
    data = NULL;
    VendorMetadataParser::getTagValue(metadata, ANDROID_JPEG_ORIENTATION, &data);
    if (NULL != data) {
        int orientation = *static_cast<int32_t *>(data);
        mOrientation = (orientation) % 360;
        MLOGI(Mia2LogGroupPlugin, "MiaNodeMiBokehn getMetaData jpeg orientation  %d", mOrientation);
    }

    int32_t light_mode = 0;
    data = NULL;
    const char *BokehLight = "xiaomi.portrait.lighting";
    VendorMetadataParser::getVTagValue(metadata, BokehLight, &data);
    if (NULL != data) {
        light_mode = *static_cast<int32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "MiaNodeMiBokehn getMetaData light_mode  %d", light_mode);
    }

    switch (light_mode) {
    case 9:
        mLightingMode = MI_PL_MODE_NEON_SHADOW;
        break; // 霓影
    case 10:
        mLightingMode = MI_PL_MODE_PHANTOM;
        break; // 魅影
    case 11:
        mLightingMode = MI_PL_MODE_NOSTALGIA;
        break; // 怀旧
    case 12:
        mLightingMode = MI_PL_MODE_RAINBOW;
        break; // 彩虹
    case 13:
        mLightingMode = MI_PL_MODE_WANING;
        break; // 阑珊
    case 14:
        mLightingMode = MI_PL_MODE_DAZZLE;
        break; // 炫影
    case 15:
        mLightingMode = MI_PL_MODE_GORGEOUS;
        break; // 斑斓
    case 16:
        mLightingMode = MI_PL_MODE_PURPLES;
        break; // 嫣红
    case 17:
        mLightingMode = MI_PL_MODE_DREAM;
        break; // 梦境
    default:
        mLightingMode = 0;
        break;
    }

    MLOGI(Mia2LogGroupPlugin,
          "Get mOrient %d mOrientation %d light_mode %d mLightingMode %d mNodeInstance: %p",
          mOrient, mOrientation, light_mode, mLightingMode, mNodeInstance);

    checkPersist();

    // get timestamp current frame
    int64_t timestamp = PluginUtils::nowMSec();

    MUInt8 *depthOutput = outputDepth->plane[0];
    if (depthOutput != NULL) {
        memset(depthOutput, 0x0F, outputDepth->stride * outputDepth->scanline);
    }
    if (depthOutput != NULL) {
        if (NULL != mNodeInstance) {
            mDepthSize = mNodeInstance->process(input, outputBokeh, depthOutput, 0, mOrientation,
                                                timestamp, 0, mNumberApplied, mSceneFlag);
        }
    }
    /*begin for relight*/
    if (mLightingMode > 0) {
        miProcessRelightingEffect(input, outputBokeh);
    }

    return result;
}

void Mibokehplugin::dumpRawData(uint8_t *data, int32_t size, uint32_t index, const char *namePrefix)
{
    MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] %s:%d: (IN)", __func__, __LINE__);

    // long long current_time = PluginUtils::nowMSec();
    char filename[256];
    memset(filename, 0, sizeof(char) * 256);
    snprintf(filename, sizeof(filename), "%s%s_%d_%d.pdata", PluginUtils::getDumpPath(), namePrefix,
             size, index);

    MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] try to open file : %s", filename);

    int file_fd = open(filename, O_RDWR | O_CREAT, 0777);
    if (file_fd != -1) {
        MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] : dumpYUVData open success");
        uint32_t bytes_write;
        bytes_write = write(file_fd, data, size);
        MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB]  data bytes_read: %d", size);
        close(file_fd);
    } else {
        MLOGE(Mia2LogGroupPlugin, "[MIBOKEH_CTB] fileopen  error: %d", errno);
    }

    MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] %s:%d: (OUT)", __func__, __LINE__);
}

int Mibokehplugin::miPrepareRelightImage(MiImageBuffer *src, MIIMAGEBUFFER *dst)
{
    if ((src == NULL) || (dst == NULL)) {
        MLOGE(Mia2LogGroupPlugin, "invalid param: src=%p, dst=%p!", src, dst);
        return MIA_RETURN_INVALID_PARAM;
    }

    if (src->format != CAM_FORMAT_YUV_420_NV12 && src->format != CAM_FORMAT_YUV_420_NV21) {
        MLOGE(Mia2LogGroupPlugin, "format[%d] is not supported!", src->format);
        return MIA_RETURN_INVALID_PARAM;
    }

    if (src->format == CAM_FORMAT_YUV_420_NV12) {
        dst->u32PixelArrayFormat = FORMAT_YUV_420_NV12;
        MLOGD(Mia2LogGroupPlugin, "FORMAT_YUV_420_NV12");
    } else if (src->format == CAM_FORMAT_YUV_420_NV21) {
        dst->u32PixelArrayFormat = FORMAT_YUV_420_NV21;
        MLOGD(Mia2LogGroupPlugin, "FORMAT_YUV_420_NV21");
    }

    dst->i32Width = src->width;
    dst->i32Height = src->height;
    dst->i32Scanline = src->height; // why not scanline
    dst->pi32Pitch[0] = src->stride;
    dst->pi32Pitch[1] = src->stride;

    dst->ppu8Plane[0] = (MUInt8 *)malloc(dst->i32Height * dst->pi32Pitch[0] * 3 / 2);
    if (dst->ppu8Plane[0]) {
        dst->ppu8Plane[1] = dst->ppu8Plane[0] + dst->i32Height * dst->pi32Pitch[0];
    } else {
        MLOGE(Mia2LogGroupPlugin, "out of memory");
        return MIA_RETURN_NO_MEM;
    }

    memcpy(dst->ppu8Plane[0], src->plane[0], dst->i32Height * dst->pi32Pitch[0]);
    memcpy(dst->ppu8Plane[1], src->plane[1], dst->i32Height / 2 * dst->pi32Pitch[0]);

    return MIA_RETURN_OK;
}

void Mibokehplugin::miReleaseRelightImage(MIIMAGEBUFFER *img)
{
    if (img != NULL) {
        if (img->ppu8Plane[0] != NULL) {
            free(img->ppu8Plane[0]);
            img->ppu8Plane[0] = NULL;
        }
    }
}

int Mibokehplugin::miCopyRelightToMiImage(MIIMAGEBUFFER *src, MiImageBuffer *dst)
{
    int32_t i;
    uint8_t *srcTemp = NULL;
    uint8_t *dstTemp = NULL;

    if (src == NULL || dst == NULL) {
        MLOGE(Mia2LogGroupPlugin, "invalid param: src=%p, dst=%p!", src, dst);
        return MIA_RETURN_INVALID_PARAM;
    }

    dstTemp = dst->plane[0];
    srcTemp = (uint8_t *)src->ppu8Plane[0];
    for (i = 0; i < dst->height; i++) {
        memcpy(dstTemp, srcTemp, dst->width);
        dstTemp += dst->stride;
        srcTemp += src->pi32Pitch[0];
    }

    dstTemp = dst->plane[1];
    for (i = 0; i < dst->height / 2; i++) {
        memcpy(dstTemp, srcTemp, dst->width);
        dstTemp += dst->stride;
        srcTemp += src->pi32Pitch[1];
    }

    return MIA_RETURN_OK;
}

int Mibokehplugin::miProcessRelightingEffect(MiImageBuffer *input, MiImageBuffer *output)
{
    int res = MOK;
    double t0, t1, t2, t3;

    t2 = PluginUtils::nowMSec();

    // use for AILAB
    MIIMAGEBUFFER srcImg = {0};
    MIIMAGEBUFFER dstImg = {0};
    MIIMAGEBUFFER bokehImg = {0};
    miPrepareRelightImage(input, &srcImg);
    miPrepareRelightImage(output, &dstImg);
    miPrepareRelightImage(output, &bokehImg);

    if (mDumpYUV) {
        char fileName[256];
        memset(fileName, 0, sizeof(char) * 256);
        snprintf(fileName, sizeof(fileName), "IN_Relight_SrcImg_Frame_%d", mProcessedFrame);
        PluginUtils::dumpToFile(fileName, input);

        memset(fileName, 0, sizeof(char) * 256);
        snprintf(fileName, sizeof(fileName), "IN_Relight_BokehImg_Frame_%d", mProcessedFrame);
        PluginUtils::dumpToFile(fileName, output);
    }

    // init
    if (mRelightingHandle == NULL) {
        MInt32 width = srcImg.i32Width;
        MInt32 height = srcImg.i32Height;

        t0 = PluginUtils::nowMSec();
        res = AILAB_PLI_Init(&mRelightingHandle, MI_PL_INIT_MODE_PERFORMANCE, width, height);
        if (MOK != res) {
            MLOGE(Mia2LogGroupPlugin, "[Relight] AILAB_PLI_Init failed = %d", res);
        }
        t1 = PluginUtils::nowMSec();

        MLOGD(Mia2LogGroupPlugin,
              "[Relight] AILAB_PLI_Init = %d: width=%d, height=%d, mRelightingHandle=%p, cost %.2f "
              "ms",
              res, width, height, mRelightingHandle, t1 - t0);
    } else {
        MLOGE(Mia2LogGroupPlugin, "[Relight] AILAB_PLI_Init mRelightingHandle %p",
              mRelightingHandle);
    }

    // preprocess
    if (res == MOK && mRelightingHandle) {
        MIPLLIGHTREGION mLightRegion{};

        t0 = PluginUtils::nowMSec();
        res = AILAB_PLI_PreProcess(mRelightingHandle, &srcImg, mOrientation, &mLightRegion);
        if (MOK != res) {
            MLOGE(Mia2LogGroupPlugin, "[Relight] AILAB_PLI_PreProcess failed = %d\n", res);
            AILAB_PLI_Uninit(&mRelightingHandle);
            mRelightingHandle = NULL;
        }
        t1 = PluginUtils::nowMSec();

        MLOGD(Mia2LogGroupPlugin,
              "[Relight] AILAB_PLI_PreProcess = %d: orientation=%d, cost %.2f ms", res,
              mOrientation, t1 - t0);
    } else {
        MLOGE(Mia2LogGroupPlugin, "[Relight] relight process failed. mRelightingHandle is %p!",
              mRelightingHandle);
        res = MFAILED;
    }

    // process
    if (res == MOK && mRelightingHandle) {
        // LightParam
        MIPLLIGHTPARAM LightParam = {0};
        LightParam.i32LightMode = mLightingMode;

        MIPLDEPTHINFO lDepthInfo = {0};
        lDepthInfo.i32MaskWidth = 0;
        lDepthInfo.i32MaskHeight = 0;
        lDepthInfo.pMaskImage = NULL;
        lDepthInfo.pBlurImage = &bokehImg;

        t0 = PluginUtils::nowMSec();
        res = AILAB_PLI_Process(mRelightingHandle, &lDepthInfo, &srcImg, &LightParam, &dstImg);
        if (MOK != res) {
            MLOGE(Mia2LogGroupPlugin, "[Relight] AILAB_PLI_Process =failed= %d\n", res);
        }
        t1 = PluginUtils::nowMSec();

        AILAB_PLI_Uninit(&mRelightingHandle);
        mRelightingHandle = NULL;

        MLOGI(Mia2LogGroupPlugin,
              "[Relight] relight process end. AILAB_PLI_Process = %d: lightingMode=%d, costime "
              "%.2f ms",
              res, mLightingMode, t1 - t0);
    } else {
        MLOGE(Mia2LogGroupPlugin, "[Relight] relight process failed. mRelightingHandle is %p!",
              mRelightingHandle);
        res = MFAILED;
    }

    if (res == MOK) {
        miCopyRelightToMiImage(&dstImg, output);
    } else {
        miCopyRelightToMiImage(&bokehImg, output);
    }

    if (mDumpYUV) {
        char fileName[256];
        memset(fileName, 0, sizeof(char) * 256);
        snprintf(fileName, sizeof(fileName), "OUT_Relight_SrcImg_Frame_%d", mProcessedFrame);
        PluginUtils::dumpToFile(fileName, output);
    }

    miReleaseRelightImage(&srcImg);
    miReleaseRelightImage(&dstImg);
    miReleaseRelightImage(&bokehImg);

    t3 = PluginUtils::nowMSec();
    MLOGI(Mia2LogGroupPlugin, "[Relight] Mi Relighting Effect res = %d, total costime=%.2f\n", res,
          t3 - t2);

    return res;
}

void Mibokehplugin::checkPersist()
{
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.xiaomi.relight.dumpimg", prop, "0");
    mDumpYUV = (bool)atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.xiaomi.relight.dumppriv", prop, "0");
    mDumpPriv = (bool)atoi(prop);

    MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] %s:%d: mDumpPriv %d (OUT)", __func__, __LINE__,
          mDumpPriv);
}

int Mibokehplugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

} // namespace mialgo2
