#include "FrontBokehPlugin.h"

#include <MiaPluginUtils.h>

#include "mialgo_dynamic_blur_control.h"

using namespace mialgo2;

#undef LOG_TAG
#define LOG_TAG "FRONT_BOKEH_CAPTURE"

#undef LOG_GROUP
#define LOG_GROUP GROUP_PLUGIN

#define ALGO_INIT_RETRY_TIME 3

#define JSON_PATH "odm/etc/camera/dualcam_bokeh_params_pro_front.json"

static const S32 g_bokehDump =
    property_get_int32("persist.vendor.camera.algoengine.frontbokeh.dump", 0);
static const S32 g_bokehDebugROI =
    property_get_int32("persist.vendor.camera.algoengine.frontbokeh.debugroi", 0);
static const S32 g_bokehDebugAlgo =
    property_get_int32("persist.vendor.camera.algoengine.frontbokeh.debugalgo", 0);

typedef enum {
    Default,
    BokehOnly,     // 1 output   bokeh             for thridpart camera
    BokehDepth,    //
    BokehDepthRaw, // 3 output   raw,bokeh,depth   for miui camera
} FrontBokehOutputMode;

typedef struct tag_disparity_fileheader_t
{
    int32_t i32Header;
    int32_t i32HeaderLength;
    int32_t i32PointX;
    int32_t i32PointY;
    int32_t i32BlurLevel;
    int32_t reserved[32];
    int32_t i32DataLength;
} disparity_fileheader_t;

FrontBokehPlugin::~FrontBokehPlugin() {}

int FrontBokehPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    m_NodeInterface = nodeInterface;
    m_pNodeMetaData = NULL;
    m_hDepthEngine = NULL;
    m_hBlurEngine = NULL;
    m_ProcessedFrame = 0;
    m_depthInitRetry = 0;
    m_BlurInitRetry = 0;

    Device->Init();

    m_pNodeMetaData = new NodeMetaData();
    if (m_pNodeMetaData != NULL) {
        m_pNodeMetaData->Init(0, 0);
    }

    return RET_OK;
}

void FrontBokehPlugin::destroy()
{
    if (m_hBlurEngine) {
        MialgoCaptureBokehDestory(&m_hBlurEngine);
        m_hBlurEngine = NULL;
    }

    if (m_hDepthEngine) {
        ModelRelease(&m_hDepthEngine);
        m_hDepthEngine = NULL;
    }

    if (m_pNodeMetaData) {
        delete m_pNodeMetaData;
        m_pNodeMetaData = NULL;
    }

    Device->Deinit();
}

ProcessRetStatus FrontBokehPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus FrontBokehPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    int ret = 0;

    m_ProcessedFrame = processRequestInfo->frameNum;
    FrontBokehOutputMode outputmode = Default;
    outputmode = FrontBokehOutputMode(processRequestInfo->outputBuffersMap.size());

    DEV_IMAGE_BUF inputFrame = {0};
    DEV_IMAGE_BUF outputBokehFrame = {0};
    DEV_IMAGE_BUF outputRawFrame = {0};
    DEV_IMAGE_BUF outputDepthFrame = {0};
    MialgoImg_AIO potraitMaskImg;

    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    ret = Device->image->MatchToImage((void *)&inputBuffers[0], &inputFrame);
    auto &outEffectBuffers = processRequestInfo->outputBuffersMap.at(0);
    ret |= Device->image->MatchToImage((void *)&outEffectBuffers[0], &outputBokehFrame);
    if (BokehDepthRaw == outputmode) {
        auto &outSrcBuffers = processRequestInfo->outputBuffersMap.at(1);
        auto &outDepthBuffers = processRequestInfo->outputBuffersMap.at(2);
        ret |= Device->image->MatchToImage((void *)&outSrcBuffers[0], &outputRawFrame);
        ret |= Device->image->MatchToImage((void *)&outDepthBuffers[0], &outputDepthFrame);
    }
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), PROCFAILED, "Match buffer err");

    camera_metadata_t *pMetadata = inputBuffers[0].pMetadata;

    if (g_bokehDump) {
        Device->image->DumpImage(&inputFrame, "front_bokeh_capture_input",
                                 (U32)processRequestInfo->frameNum, ".NV12");
    }

    ret = LibDepthInit(pMetadata, &inputFrame);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "LibDepthInit err %d", ret);

    ret = LibDepthProcess(pMetadata, &inputFrame, &potraitMaskImg);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "LibDepthProcess err %d", ret);

    ret = LibBlurInit(pMetadata, &inputFrame);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "LibBlurInit err %d", ret);

    ret = LibBlurProcess(pMetadata, &inputFrame, &potraitMaskImg, &outputBokehFrame,
                         (BokehDepthRaw == outputmode) ? &outputDepthFrame : NULL);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "LibBlurProcess err %d", ret);

exit:
    if (ret != RET_OK) {
        DEV_LOGE("algo fail to do memcopy");

        if (RET_OK != Device->image->Copy(&outputBokehFrame, &inputFrame)) {
            DEV_LOGE("memcopy fail");
        }
    }

    if (BokehDepthRaw == outputmode) {
        Device->image->Copy(&outputRawFrame, &inputFrame);
    }

    if (g_bokehDebugROI) {
        AlgoMultiCams::MiFaceRect faceRect;
        AlgoMultiCams::MiPoint2i faceCentor;
        m_pNodeMetaData->GetMaxFaceRect(pMetadata, outputBokehFrame.width, outputBokehFrame.height,
                                        &faceRect, &faceCentor);
        DEV_IMAGE_RECT faceROI = {(U32)faceRect.x, (U32)faceRect.y, (U32)faceRect.width,
                                  (U32)faceRect.height};
        DEV_IMAGE_POINT faceTP = {faceCentor.x, faceCentor.y};
        Device->image->DrawPoint(&outputBokehFrame, faceTP, 10, 0);
        if (faceROI.width > 0 && faceROI.height > 0) {
            Device->image->DrawRect(&outputBokehFrame, faceROI, 5, 5);
        }
    }

    if (g_bokehDump) {
        Device->image->DumpImage(&outputBokehFrame, "front_bokeh_capture_output_effect",
                                 (U32)processRequestInfo->frameNum, ".NV12");
        if (BokehDepthRaw == outputmode) {
            Device->image->DumpImage(&outputRawFrame, "front_bokeh_capture_output_raw",
                                     (U32)processRequestInfo->frameNum, ".NV12");
            Device->image->DumpImage(&outputDepthFrame, "front_bokeh_capture_output_depth",
                                     (U32)processRequestInfo->frameNum, ".Y16");
        }
    }

    // runTime
    if (m_NodeInterface.pSetResultMetadata != NULL) {
        FLOAT fNumber = 0;
        m_pNodeMetaData->GetFNumberApplied(pMetadata, &fNumber);

        char buf[1024] = {0};
        snprintf(buf, sizeof(buf), "FrontBokeh:{processedFrame:%d fNumber:%.1f}", m_ProcessedFrame,
                 fNumber);
        std::string results(buf);
        m_NodeInterface.pSetResultMetadata(m_NodeInterface.owner, processRequestInfo->frameNum,
                                           processRequestInfo->timeStamp, results);
    }

    return resultInfo;
};

int FrontBokehPlugin::LibDepthInit(camera_metadata_t *pMetadata, DEV_IMAGE_BUF *pInputFrame)
{
    DEV_IF_LOGW_RETURN_RET(m_hDepthEngine != NULL, RET_OK, "m_hDepthEngine is not NULL: %p!!!",
                           m_hDepthEngine);
    DEV_IF_LOGW_RETURN_RET(pInputFrame == NULL, RET_ERR_ARG, "inputFrame is NULL!!!");
    DEV_IF_LOGW_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "metadata is not NULL!!!");
    DEV_IF_LOGW_RETURN_RET(m_depthInitRetry++ >= ALGO_INIT_RETRY_TIME, RET_ERR, "Retry Fail");

    int ret = 0;
    ModelInitParams launchparams;
    memset(&launchparams, 0, sizeof(ModelInitParams));
    launchparams.detRuntime = AI_VISION_RUNTIME_DSP_AIO;
    launchparams.detPriority = AI_VISION_PRIORITY_HIGH_AIO;
    launchparams.detPerformance = AI_VISION_PERFORMANCE_HIGH_AIO;
    launchparams.segRuntime = AI_VISION_RUNTIME_DSP_AIO;
    launchparams.segPriority = AI_VISION_PRIORITY_HIGH_AIO;
    launchparams.segPerformance = AI_VISION_PERFORMANCE_HIGH_AIO;
    launchparams.det_model_name = "human_detect";
    launchparams.seg_model_name = "front_portrait_blur";
    launchparams.asset_path =
        "/vendor/etc/camera"; // asset_path会搜索模型和相关配置文件, 一般指定/vendor/etc/camera
    launchparams.use_detector = false; // 前置use_detector 设置false
    launchparams.srcImageWidth = pInputFrame->width;
    launchparams.srcImageHeight = pInputFrame->height;
    launchparams.modelType = 1; // 如果输入是NV12或者NV21，modelType设置1; rgb，设置0

    if (g_bokehDebugAlgo) {
        launchparams.detRuntime = MialgoAiVisionRuntime_AIO(g_bokehDebugAlgo);
        launchparams.segRuntime = MialgoAiVisionRuntime_AIO(g_bokehDebugAlgo);
    }

    DEV_LOGI(
        "ModelLaunch: srcImageWidth: %d, srcImageHeight: %d, use_detector: %d, modelType: %d, "
        "det:[R-%d Pr-%d Pe-%d] set:[R-%d Pr-%d Pe-%d]",
        launchparams.srcImageWidth, launchparams.srcImageHeight, launchparams.use_detector,
        launchparams.modelType, launchparams.detRuntime, launchparams.detPriority,
        launchparams.detPerformance, launchparams.segRuntime, launchparams.segPriority,
        launchparams.segPerformance);

    ret = ModelLaunch(&m_hDepthEngine, &launchparams);

    DEV_IF_LOGE_RETURN_RET(ret != 0, RET_ERR, "ModelLaunch Fail: %d !!!", ret);
    DEV_LOGI("ModelLaunch success: m_hBlurEngine = %p", m_hDepthEngine);

    return RET_OK;
}

int FrontBokehPlugin::LibDepthProcess(camera_metadata_t *pMetadata, DEV_IMAGE_BUF *pInputImage,
                                      MialgoImg_AIO *pPotraitMaskImg)
{
    DEV_IF_LOGE_RETURN_RET(m_hDepthEngine == NULL, RET_ERR_ARG, "m_hDepthEngine is NULL !!!");
    DEV_IF_LOGE_RETURN_RET(pInputImage == NULL, RET_ERR_ARG, "inputImage is NUL !!!");
    DEV_IF_LOGE_RETURN_RET(pPotraitMaskImg == NULL, RET_ERR_ARG, "potraitMaskData is NUL !!!");
    DEV_IF_LOGW_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "metadata is not NULL!!!");

    int ret = 0;
    S32 rotateAngle = 0;
    ret = m_pNodeMetaData->GetRotateAngle(pMetadata, &rotateAngle);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, RET_ERR, "get meta fail !!!");

    ModelProcParams procparams;
    memset(&procparams, 0, sizeof(ModelProcParams));
    procparams.rotate_angle = rotateAngle;
    procparams.srcImage.imgWidth = pInputImage->width;
    procparams.srcImage.imgHeight = pInputImage->height;
    procparams.srcImage.pYData = pInputImage->plane[0];
    procparams.srcImage.pUVData = pInputImage->plane[1];
    procparams.srcImage.stride = pInputImage->stride[0];
    procparams.srcImage.img_format = MIALGO_IMG_NV12_AIO;

    DEV_LOGI(
        "ModelRun: rotate_angle %d imgWidth %d imgHeight %d pYData %p pYData %p stride %d "
        "img_format %d",
        procparams.rotate_angle, procparams.srcImage.imgWidth, procparams.srcImage.imgHeight,
        procparams.srcImage.pYData, procparams.srcImage.pUVData, procparams.srcImage.stride,
        procparams.srcImage.img_format);

    ret = ModelRun(&m_hDepthEngine, &procparams);

    DEV_LOGI("potraitMask - img_format %d imgWidth %d imgHeight %d stride %d size %d data %p fd %d",
             procparams.potraitMask.img_format, procparams.potraitMask.imgWidth,
             procparams.potraitMask.imgHeight, procparams.potraitMask.stride,
             procparams.potraitMask.mem_info.size, procparams.potraitMask.data,
             procparams.potraitMask.mem_info.fd);

    DEV_IF_LOGE_RETURN_RET(ret != 0, RET_ERR, "ModelRun Fail: %d !!!", ret);
    DEV_IF_LOGE_RETURN_RET(procparams.potraitMask.data == NULL, RET_ERR,
                           "ModelRun Mask is NUL !!!");

    memcpy(pPotraitMaskImg, &procparams.potraitMask, sizeof(procparams.potraitMask));

    DEV_LOGI("ModelRun success: m_hDepthEngine = %p", m_hDepthEngine);

    return RET_OK;
}

int FrontBokehPlugin::LibBlurInit(camera_metadata_t *pMetadata, DEV_IMAGE_BUF *pInputImage)
{
    DEV_IF_LOGW_RETURN_RET(m_hBlurEngine != NULL, RET_OK, "m_hBlurEngine is not NULL: %p!!!",
                           m_hBlurEngine);
    DEV_IF_LOGE_RETURN_RET(pInputImage == NULL, RET_ERR_ARG, "inputImage is NULL: %p!!!");
    DEV_IF_LOGW_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "metadata is not NULL!!!");
    DEV_IF_LOGW_RETURN_RET(m_BlurInitRetry++ >= ALGO_INIT_RETRY_TIME, RET_ERR, "Retry Fail");

    int ret = 0;

    AlgoBokehLaunchParams launchParams;
    launchParams.jsonFilename = JSON_PATH;
    launchParams.MainImgW = pInputImage->width;
    launchParams.MainImgH = pInputImage->height;
    launchParams.isPerformanceMode = false;
    launchParams.isHotMode = false;
    launchParams.FovInfo = AlgoMultiCams::CAPTURE_FOV_1X;
    launchParams.frameInfo = AlgoMultiCams::CAPTURE_FRAME_4_3;
    int imagescale = 10 * pInputImage->width / (pInputImage->height == 0 ? 1 : pInputImage->height);
    if (imagescale == 10 * 4 / 3) {
        launchParams.frameInfo = AlgoMultiCams::CAPTURE_FRAME_4_3;
    } else if (imagescale == 10 * 16 / 9) {
        launchParams.frameInfo = AlgoMultiCams::CAPTURE_FRAME_16_9;
    } else {
        launchParams.frameInfo = AlgoMultiCams::CAPTURE_FRAME_FULL_FRAME;
    }

    DEV_LOGI(
        "jsonFilename %s MainImgW %d MainImgH %d isPerformanceMode %d isHotMode %d frameInfo %d "
        "FovInfo %d",
        launchParams.jsonFilename, launchParams.MainImgW, launchParams.MainImgH,
        launchParams.isPerformanceMode, launchParams.isHotMode, launchParams.frameInfo,
        launchParams.FovInfo);

    ret = MialgoCaptureBokehLaunch(&m_hBlurEngine, launchParams);

    DEV_IF_LOGE_RETURN_RET(ret != 0, RET_ERR, "MialgoCaptureBokehLaunch Fail: %d !!!", ret);

    DEV_LOGI("MialgoCaptureBokehLaunch success: m_hBlurEngine = %p", m_hBlurEngine);

    return RET_OK;
}

int FrontBokehPlugin::LibBlurProcess(camera_metadata_t *pMetadata, DEV_IMAGE_BUF *pInputImage,
                                     MialgoImg_AIO *pPotraitMaskImg, DEV_IMAGE_BUF *pOutputImage,
                                     DEV_IMAGE_BUF *pOutputDepthImage)
{
    DEV_IF_LOGE_RETURN_RET(m_hBlurEngine == NULL, RET_ERR_ARG, "m_hBlurEngine is NULL !!!");
    DEV_IF_LOGE_RETURN_RET(pPotraitMaskImg == NULL, RET_ERR_ARG, "degMask is NULL: %p!!!");
    DEV_IF_LOGW_RETURN_RET(pMetadata == NULL, RET_ERR_ARG, "metadata is not NULL!!!");
    DEV_IF_LOGW_RETURN_RET((pInputImage == NULL || pOutputImage == NULL), RET_ERR_ARG,
                           "metadata is not NULL!!!");

    int ret = 0;

    AlgoMultiCams::MiImage inputImg = {AlgoMultiCams::MI_IMAGE_FMT_INVALID};
    AlgoMultiCams::MiImage outImg = {AlgoMultiCams::MI_IMAGE_FMT_INVALID};
    AlgoMultiCams::MiImage depthImg = {AlgoMultiCams::MI_IMAGE_FMT_INVALID};

    // input and output
    ret = PrepareToMiImage(pInputImage, &inputImg);
    ret |= PrepareToMiImage(pOutputImage, &outImg);
    DEV_IF_LOGE_RETURN_RET(ret != 0, RET_ERR, "PrepareImage Fail: %d !!!");

    // depth Img
    depthImg.fmt = AlgoMultiCams::MI_IMAGE_FMT_GRAY;
    depthImg.width = pPotraitMaskImg->imgWidth;
    depthImg.height = pPotraitMaskImg->imgHeight;
    depthImg.plane_count = 1;
    depthImg.pitch[0] = pPotraitMaskImg->stride;
    depthImg.plane_size[0] = pPotraitMaskImg->mem_info.size;
    depthImg.plane[0] = pPotraitMaskImg->data;
    depthImg.fd[0] = pPotraitMaskImg->mem_info.fd;

    DEV_LOGI("potraitMask - img_format %d imgWidth %d imgHeight %d stride %d size %d data %p fd %d",
             pPotraitMaskImg->img_format, pPotraitMaskImg->imgWidth, pPotraitMaskImg->imgHeight,
             pPotraitMaskImg->stride, pPotraitMaskImg->mem_info.size, pPotraitMaskImg->data,
             pPotraitMaskImg->mem_info.fd);

    // Init param
    AlgoBokehInitParams initParams;
    FLOAT fNumber = 0;
    ret = m_pNodeMetaData->GetFNumberApplied(pMetadata, &fNumber);
    initParams.userInterface.aperture = findBlurLevelByFNumber(fNumber, TRUE);
    ret |= m_pNodeMetaData->GetSensitivity(pMetadata, &initParams.userInterface.ISO);
    ret |= m_pNodeMetaData->GetMaxFaceRect(pMetadata, pInputImage->width, pInputImage->height,
                                           &initParams.userInterface.fRect,
                                           &initParams.userInterface.tp);
    initParams.isSingleCameraDepth = TRUE;
    DEV_IF_LOGE_RETURN_RET(ret != 0, RET_ERR, "get meta Fail: %d !!!");
    DEV_LOGI("fNumber %.1f, aperture %d, ISO %d", fNumber, initParams.userInterface.aperture,
             initParams.userInterface.ISO);

    MialgoCaptureBokehInit(&m_hBlurEngine, initParams);

    int compressedDataSize = 0;
    ret = MIALGO_CaptureGetDepthDataSize_Mono(&m_hBlurEngine, &compressedDataSize);
    if (ret != 0) {
        MialgoCaptureBokehDeinit(&m_hBlurEngine);
        DEV_LOGE("MIALGO_CaptureGetDepthDataSize_Mono Fail: %d !!!", ret);
        return RET_ERR;
    }

    unsigned char *compressedData = (unsigned char *)malloc(compressedDataSize);
    if (compressedData == NULL) {
        MialgoCaptureBokehDeinit(&m_hBlurEngine);
        DEV_LOGE("malloc compressedData fail !!!");
        return RET_ERR;
    }

    // Proc param
    AlgoBokehEffectParams bokehParams;
    memset(&bokehParams, 0, sizeof(bokehParams));
    bokehParams.imgMain = &inputImg;
    bokehParams.imgDepth = &depthImg;
    bokehParams.validDepthSize = pPotraitMaskImg->mem_info.size;
    bokehParams.imgResult = &outImg;
    bokehParams.result_data_buf = compressedData;
    bokehParams.result_data_size = compressedDataSize;

    ret = MialgoCaptureBokehProc(&m_hBlurEngine, bokehParams);

    MialgoCaptureBokehDeinit(&m_hBlurEngine);

    if (NULL != pOutputDepthImage) {
        disparity_fileheader_t header;
        memset(&header, 0, sizeof(disparity_fileheader_t));
        header.i32Header = 0x80;
        header.i32HeaderLength = sizeof(disparity_fileheader_t);
        header.i32PointX = initParams.userInterface.tp.x;
        header.i32PointY = initParams.userInterface.tp.y;
        header.i32BlurLevel = initParams.userInterface.aperture;
        header.i32DataLength = compressedDataSize;

        memcpy(pOutputDepthImage->plane[0], (void *)&header, sizeof(disparity_fileheader_t));
        memcpy(pOutputDepthImage->plane[0] + sizeof(disparity_fileheader_t), compressedData,
               compressedDataSize);
    }

    free(compressedData);

    DEV_IF_LOGE_RETURN_RET(ret != 0, RET_ERR, "MialgoCaptureBokehProc Fail: %d !!!", ret);

    DEV_LOGI("MialgoCaptureBokehProc success: m_hBlurEngine = %p", m_hBlurEngine);

    return RET_OK;
}

S32 FrontBokehPlugin::PrepareToMiImage(DEV_IMAGE_BUF *pImage, AlgoMultiCams::MiImage *pStImage)
{
    pStImage->width = pImage->width;
    pStImage->height = pImage->height;

    if (pImage->format != CAM_FORMAT_YUV_420_NV12 && pImage->format != CAM_FORMAT_YUV_420_NV21 &&
        pImage->format != CAM_FORMAT_Y16) {
        DEV_LOGE(" format[%d] is not supported!", pImage->format);
        return RET_ERR;
    }
    if (pImage->format == CAM_FORMAT_YUV_420_NV12) {
        pStImage->fmt = AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_NV12;
    } else if (pImage->format == CAM_FORMAT_YUV_420_NV21) {
        pStImage->fmt = AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_NV21;
    } else if (pImage->format == CAM_FORMAT_Y16) {
        pStImage->fmt = AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_GRAY;
    }
    pStImage->pitch[0] = pImage->stride[0];
    pStImage->pitch[1] = pImage->stride[1];
    pStImage->pitch[2] = pImage->stride[2];
    pStImage->slice_height[0] = pImage->sliceHeight[0];
    pStImage->slice_height[1] = pImage->sliceHeight[1];
    pStImage->slice_height[2] = pImage->sliceHeight[2];
    pStImage->plane[0] = (unsigned char *)pImage->plane[0];
    pStImage->plane[1] = (unsigned char *)pImage->plane[1];
    pStImage->plane[2] = (unsigned char *)pImage->plane[2];
    pStImage->fd[0] = pImage->fd[0];
    pStImage->fd[1] = pImage->fd[1];
    pStImage->fd[2] = pImage->fd[2];
    pStImage->plane_count = pImage->planeCount;
    return RET_OK;
}

S32 FrontBokehPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

bool FrontBokehPlugin::isEnabled(MiaParams settings)
{
    void *data = NULL;
    int32_t FrontBokehEnabled = 0;
    const char *frontbokeh = "xiaomi.bokeh.enabled";
    VendorMetadataParser::getVTagValue(settings.metadata, frontbokeh, &data);
    if (NULL != data) {
        FrontBokehEnabled = *static_cast<int32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "FrontBokehPlugin getMetaData FrontBokehEnabled %d",
              FrontBokehEnabled);
    } else {
        MLOGE(Mia2LogGroupPlugin, "can not found tag \"xiaomi.bokeh.enabled\"");
    }
    return FrontBokehEnabled ? true : false;
}
