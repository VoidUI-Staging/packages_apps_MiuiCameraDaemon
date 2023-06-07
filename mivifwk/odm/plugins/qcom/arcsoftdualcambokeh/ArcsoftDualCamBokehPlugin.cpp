#include "ArcsoftDualCamBokehPlugin.h"

#include <MiaPluginUtils.h>

#include "device/device.h"
#include "mialgo_dynamic_blur_control.h"

using namespace mialgo2;

#undef LOG_TAG
#define LOG_TAG "ARCSOFTBOKEH_CAPTURE_BOKEH"

#undef LOG_GROUP
#define LOG_GROUP GROUP_PLUGIN

#define ROLE_REARBOKEH2X 61
#define ROLE_REARBOKEH1X 63

#define SHOW_AUX_ONLY  1
#define SHOW_MAIN_ONLY 2
#define SHOW_HALF_HALF 3

#define FILTER_SHINE_THRES         10
#define FILTER_SHINE_LEVEL         40
#define FILTER_SHINE_LEVEL_REFOCUS 40
#define FILTER_SHINE_THRES_NIGHT   5
#define FILTER_SHINE_THRES_REFOCUS 5
#define FILTER_SHINE_LEVEL_NIGHT   80
#define FILTER_SHINE_THRES_OUTSIDE 2
#define FILTER_SHINE_LEVEL_OUTSIDE 20
#define FILTER_INTENSITY           70

static const S32 g_bokehDump = property_get_int32("persist.vendor.camera.mialgo.capbokeh.dump", 0);
static const S32 g_nViewCtrl =
    property_get_int32("persist.vendor.camera.mialgo.capbokeh.debugview", 0);
static const S32 g_bDrawROI =
    property_get_int32("persist.vendor.camera.mialgo.capbokeh.debugroi", 0);
static const S32 g_IsSingleCameraDepth =
    property_get_int32("persist.vendor.camera.mialgo.capbokeh.singlecameradepth", -1);
static const int32_t g_BokehBlur = property_get_int32("persist.vendor.camera.bokeh.blur", -1);
static const int32_t g_BokehShineThres =
    property_get_int32("persist.vendor.camera.bokeh.shineThres", -1);
static const int32_t g_BokehShineLevel =
    property_get_int32("persist.vendor.camera.bokeh.shineLevel", -1);
static const S32 g_ArcLogLevel = property_get_int32("persist.vendor.camera.arcsoft.cb.logleve", 2);
typedef struct tag_disparity_fileheader_t
{
    int32_t i32Header;
    int32_t i32HeaderLength;
    int32_t i32PointX;
    int32_t i32PointY;
    int32_t i32BlurLevel;
    int32_t reserved_DepthWidth;
    int32_t reserved_DepthHeight;
    int32_t i32Version;
    int32_t i32VendorID;
    int32_t mappingVersion;
    int32_t ShineThres;
    int32_t ShineLevel;
    int32_t reserved[25];
    int32_t i32DataLength;
} disparity_fileheader_t;

ArcsoftDualCamBokehPlugin::~ArcsoftDualCamBokehPlugin() {}

int ArcsoftDualCamBokehPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    Device->Init();
    m_hCaptureBokeh = NULL;
    m_ProcessCount = 0;
    m_NodeInterface = nodeInterface;
    m_blurLevel = 0;
    m_pNodeMetaData = NULL;
    m_superNightEnabled = 0;
    m_hdrBokehEnabled = 0;
    Device->detector->Create(&m_detectorInit, MARK("m_detectorInit"));
    Device->detector->Create(&m_detectorRun, MARK("m_detectorRun"));
    Device->detector->Create(&m_detectorBokehRun, MARK("m_detectorBokehRun"));
    Device->detector->Create(&m_detectorUninit, MARK("m_detectorUninit"));
    Device->detector->Create(&m_detectorReset, MARK("m_detectorReset"));
    Device->detector->Create(&m_detectorCropResizeImg, MARK("m_detectorCropResizeImg"));
    m_pNodeMetaData = new NodeMetaData();
    if (m_pNodeMetaData != NULL) {
        m_pNodeMetaData->Init(0, 0);
    }
    return iniEngine();
}

S32 ArcsoftDualCamBokehPlugin::iniEngine()
{
    S64 ret = RET_OK;
    ret = ARC_DCIRP_SetLogLevel(g_ArcLogLevel);
    DEV_IF_LOGI((ret == RET_OK), "arcsoft bokeh lib loglevel is %d", g_ArcLogLevel);
    const MPBASE_Version *arcVersion = NULL;
    arcVersion = ARC_DCIRP_GetVersion();
    DEV_IF_LOGI((arcVersion != NULL), "Arcsoft Bokeh Algo Lib Version: %s", arcVersion->Version);
    Device->detector->Begin(m_detectorInit, MARK("ARC_DCIRP_Init"), 5000);
    ret = ARC_DCIRP_Init(&m_hCaptureBokeh, ARC_DCIRP_POST_REFOCUS_MODE);
    Device->detector->End(m_detectorInit);
    DEV_IF_LOGD_RETURN_RET((ret != RET_OK || m_hCaptureBokeh == NULL), ret,
                           "ARC_DCIRP_Init Faild ret = %d m_hCaptureBokeh = %p", ret,
                           m_hCaptureBokeh);
    DEV_LOGI("ARC_DCIRP_Init is %d", ret);
    return ret;
}

void ArcsoftDualCamBokehPlugin::destroy()
{
    // m_flustStatus = FALSE;
    if (m_hCaptureBokeh) {
        Device->detector->Begin(m_detectorUninit, MARK("ARC_DCIRP_Uninit"), 5000);
        ARC_DCIRP_Uninit(&m_hCaptureBokeh);
        Device->detector->End(m_detectorUninit);
        m_hCaptureBokeh = NULL;
    }
    if (m_pNodeMetaData) {
        delete m_pNodeMetaData;
        m_pNodeMetaData = NULL;
    }
    Device->detector->Destroy(&m_detectorRun);
    Device->detector->Destroy(&m_detectorBokehRun);
    Device->detector->Destroy(&m_detectorInit);
    Device->detector->Destroy(&m_detectorUninit);
    Device->detector->Destroy(&m_detectorReset);
    Device->detector->Destroy(&m_detectorCropResizeImg);
    Device->Deinit();
}

ProcessRetStatus ArcsoftDualCamBokehPlugin::processRequest(
    ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus ArcsoftDualCamBokehPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    DEV_IF_LOGE_RETURN_RET(pProcessRequestInfo == NULL, (ProcessRetStatus)RET_ERR, "arg err");
    S32 ret = RET_OK;
    auto &inputBuffers = pProcessRequestInfo->inputBuffersMap.begin()->second;
    m_plogicalMeta = inputBuffers[0].pMetadata;
    m_pPhysicalMeta = inputBuffers[0].pPhyCamMetadata;
    S32 outputPorts = pProcessRequestInfo->outputBuffersMap.size();
    U64 runTime = 0;

    DEV_IMAGE_BUF inputMain;
    DEV_IMAGE_BUF inputAux;
    DEV_IMAGE_BUF inputMain_extraEV = {0};
    DEV_IMAGE_BUF outputMain = {0};
    DEV_IMAGE_BUF outputAux = {0};

    // cpu boost 3000 ms
    std::shared_ptr<PluginPerfLockManager> boostCpu(new PluginPerfLockManager(3000));

    auto dumpImg = [&](camera_metadata_t *pLogMeta, DEV_IMAGE_BUF &img, const char *pName,
                       int Idx) {
        S32 expValue = 0;
        m_pNodeMetaData->GetExpValue(pLogMeta, &expValue);
        DEV_LOGI("get image %s EV=%d %dx%d,stride=%dx%d,fmt=%d", pName, expValue, img.width,
                 img.height, img.stride[0], img.sliceHeight[0], img.format);

        if (g_bokehDump) {
            char fileName[128];
            snprintf(fileName, sizeof(fileName), "bokeh_capture_%s_%d_EV%d", pName, Idx, expValue);
            Device->image->DumpImage(&img, fileName, (U32)pProcessRequestInfo->frameNum, ".NV12");
        }
    };

    for (auto &imageParams : pProcessRequestInfo->inputBuffersMap) {
        std::vector<ImageParams> &inputs = imageParams.second;
        uint32_t portId = imageParams.first;
        for (auto &input : inputs) {
            if (portId == 0) {
                ret = Device->image->MatchToImage((void *)&input, &inputMain);
                DEV_IF_LOGE_RETURN_RET((ret != RET_OK), (ProcessRetStatus)ret, "Match buffer err");
                dumpImg(input.pMetadata, inputMain, "inputMain", 0);
                break;
            }
        }
    }

    for (auto &imageParams : pProcessRequestInfo->inputBuffersMap) {
        std::vector<ImageParams> &inputs = imageParams.second;
        uint32_t portId = imageParams.first;
        for (auto &input : inputs) {
            if (portId == 1) {
                ret = Device->image->MatchToImage((void *)&input, &inputAux);
                DEV_IF_LOGE_RETURN_RET((ret != RET_OK), (ProcessRetStatus)ret, "Match buffer err");
                dumpImg(input.pMetadata, inputAux, "inputDepth", 0);
                break;
            }
        }
    }
    for (auto &imageParams : pProcessRequestInfo->outputBuffersMap) {
        std::vector<ImageParams> &outputs = imageParams.second;
        for (auto &output : outputs) {
            if (output.portId == 1) {
                ret = Device->image->MatchToImage((void *)&output, &outputAux);
                DEV_IF_LOGE_RETURN_RET((ret != RET_OK), (ProcessRetStatus)ret, "Match buffer err");
                DEV_LOGI(
                    "output ports %d Aux width: %d, height: %d, stride: %d, scanline: %d, format: "
                    "%d",
                    outputPorts, outputAux.width, outputAux.height, outputAux.stride[0],
                    outputAux.sliceHeight[0], outputAux.format);
                break;
            }
        }
    }
    m_pNodeMetaData->GetHdrEnabled(m_plogicalMeta, &m_hdrBokehEnabled);
    m_pNodeMetaData->GetSuperNightEnabled(m_plogicalMeta, &m_superNightEnabled);
    DEV_LOGI("hdrBokehEnabled %d superNightEnabled %d ", m_hdrBokehEnabled, m_superNightEnabled);

    auto &outBokehBuffers = pProcessRequestInfo->outputBuffersMap.at(0);
    ret = Device->image->MatchToImage((void *)&outBokehBuffers[0], &outputMain);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), (ProcessRetStatus)ret, "Match buffer err");

    DEV_LOGI("output ports %d main width: %d, height: %d, stride: %d, scanline: %d, format: %d",
             outputPorts, outputMain.width, outputMain.height, outputMain.stride[0],
             outputMain.sliceHeight[0], outputMain.format);

    switch (g_nViewCtrl) {
    case SHOW_MAIN_ONLY:
        Device->image->Copy(&outputMain, &inputMain);
        break;
    default:
        Device->detector->Begin(m_detectorRun, MARK("ProcessBokeh"), 10000);
        ret = ProcessBokeh(&inputMain, &inputAux, &outputMain, &outputAux);
        runTime = Device->detector->End(m_detectorRun);
        DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "LibProcess err %d", ret);
        break;
    }

    if (g_bDrawROI) {
        DEV_IMAGE_RECT focusROI = {0};
        DEV_IMAGE_POINT focusTP = {0};
        AlgoMultiCams::MiFaceRect maxfaceRect = {0};
        m_pNodeMetaData->GetfocusROI(m_pPhysicalMeta, &inputMain, &focusROI);
        m_pNodeMetaData->GetfocusTP(m_pPhysicalMeta, &inputMain, &focusTP);
        m_pNodeMetaData->GetMaxFaceRect(m_pPhysicalMeta, &inputMain, &maxfaceRect);
        DEV_IMAGE_RECT faceRect = {.left = (U32)maxfaceRect.x,
                                   .top = (U32)maxfaceRect.y,
                                   .width = (U32)maxfaceRect.width,
                                   .height = (U32)maxfaceRect.height};
        Device->image->DrawPoint(&outputMain, focusTP, 10, 0);
        Device->image->DrawRect(&outputMain, focusROI, 5, 5);
        Device->image->DrawRect(&outputMain, faceRect, 2, 50);
    }

    if (g_bokehDump) {
        Device->image->DumpImage(&outputMain, "bokeh_capture_outputMain",
                                 (U32)pProcessRequestInfo->frameNum, ".NV12");
        Device->image->DumpImage(&outputAux, "bokeh_capture_outputAux",
                                 (U32)pProcessRequestInfo->frameNum, ".NV12");
    }
exit:
    if (ret != RET_OK) {
        DEV_LOGE("ProcessBuffer MEMCOPY =%d", ret);
        Device->image->Copy(&outputMain, &inputMain);
        Device->image->Copy(&outputAux, &inputMain);
    }
    if (m_NodeInterface.pSetResultMetadata != NULL) { // For exif
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf),
                 "arcsoftDualBokeh:{processCount:%d blurLevel:%d "
                 "costTime:%d ret:%d}",
                 m_ProcessCount, m_blurLevel, runTime, ret);
        std::string results(buf);
        m_NodeInterface.pSetResultMetadata(m_NodeInterface.owner, pProcessRequestInfo->frameNum,
                                           pProcessRequestInfo->timeStamp, results);
    }
    m_ProcessCount++;
    return (ProcessRetStatus)RET_OK;
}

S32 ArcsoftDualCamBokehPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

S32 ArcsoftDualCamBokehPlugin::ProcessBokeh(DEV_IMAGE_BUF *inputMain, DEV_IMAGE_BUF *inputDepth,
                                            DEV_IMAGE_BUF *outputMain, DEV_IMAGE_BUF *outputAux)
{
    DEV_IF_LOGE_RETURN_RET((outputMain == NULL || inputMain == NULL || inputDepth == NULL), RET_ERR,
                           "ARG ERR");
    DEV_IF_LOGE_RETURN_RET((m_hCaptureBokeh == NULL), RET_ERR, "ARG ERR");

    S32 ret = RET_OK;
    Device->detector->Begin(m_detectorReset, MARK("ARC_DCIRP_Reset"), 3000);
    ret = ARC_DCIRP_Reset(m_hCaptureBokeh);
    Device->detector->End(m_detectorReset);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "ARC_DCIRP_Reset failed");
    DEV_LOGI("ARC_DCIRP_Reset is %d", ret);
    ARC_DCIRP_REFOCUS_PARAM pRFParam;
    DEV_IMAGE_POINT focusTP = {0};
    ret = m_pNodeMetaData->GetfocusTP(m_pPhysicalMeta, inputMain, &focusTP);
    pRFParam.ptFocus.x = focusTP.x;
    pRFParam.ptFocus.y = focusTP.y;
    FP32 fNumber = 0;
    ret = m_pNodeMetaData->GetFNumberApplied(m_plogicalMeta, &fNumber);
    m_blurLevel = findBlurLevelByFNumber(fNumber, false);
    pRFParam.i32BlurIntensity = (g_BokehBlur != -1) ? g_BokehBlur : m_blurLevel;
    if (m_superNightEnabled == 0) {
        pRFParam.i32ShineLevel = FILTER_SHINE_LEVEL;
        pRFParam.i32lShineThres = FILTER_SHINE_THRES;
    } else {
        pRFParam.i32ShineLevel = FILTER_SHINE_LEVEL_NIGHT;
        pRFParam.i32lShineThres = FILTER_SHINE_THRES_NIGHT;
    }

    S32 sensitivity = 0;
    ret = m_pNodeMetaData->GetSensitivity(m_plogicalMeta, &sensitivity);
    ARC_DCIRP_METADATA pMetaData;
    pMetaData.i32CurrentISO = sensitivity;
    pMetaData.bNoiseOn = 0;
    pMetaData.i32NoiseLevel = ARC_DCIR_NOISE_LEVEL_1;

    ASVLOFFSCREEN AlgoBlurImg_Aux = {0};
    ASVLOFFSCREEN AlgoBlurImg_Main = {0};
    ASVLOFFSCREEN AlgoBlurImg_OutputMain = {0};
    ASVLOFFSCREEN AlgoBlurImg_OutputAux = {0};
    ret = PrepareYUV_OFFSCREEN(inputMain, &AlgoBlurImg_Main);
    ret |= PrepareYUV_OFFSCREEN(inputDepth, &AlgoBlurImg_Aux);
    ret |= PrepareYUV_OFFSCREEN(outputMain, &AlgoBlurImg_OutputMain);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "PrepareYUV_OFFSCREEN fail!!! ");
    disparity_fileheader_t *header = NULL;
    header = (disparity_fileheader_t *)(AlgoBlurImg_Aux.ppu8Plane[0]); // 地址偏移预留头空间
    S32 i32DisparityDataSize = 0;
    i32DisparityDataSize = header->i32DataLength;
    uint8_t *pDisparityData = NULL;
    pDisparityData = AlgoBlurImg_Aux.ppu8Plane[0] + sizeof(disparity_fileheader_t);
    Device->detector->Begin(m_detectorBokehRun, MARK("ARC_DCIRP_Process"), 7000);
    ret = ARC_DCIRP_Process(m_hCaptureBokeh, pDisparityData, i32DisparityDataSize,
                            &AlgoBlurImg_Main, &pRFParam, &pMetaData, &AlgoBlurImg_OutputMain);
    Device->detector->End(m_detectorBokehRun);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "ARC_DCIRP_Process failed");
    DEV_LOGI("ARC_DCIRP_Process is %d", ret);
    if (ret == RET_OK && m_hdrBokehEnabled == 1) {
        MRECT cropsize;
        m_pNodeMetaData->GetArcsoftCropsize(m_plogicalMeta, &cropsize);
        DEV_LOGI("arcsoft hdr cropsize is [%d %d %d %d]", cropsize.top, cropsize.left,
                 cropsize.bottom, cropsize.right);
        if (outputAux->plane[0] != NULL) {
            ret = PrepareYUV_OFFSCREEN(outputAux, &AlgoBlurImg_OutputAux);
            DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "PrepareYUV_OFFSCREEN fail!!! ");
            Device->detector->Begin(m_detectorCropResizeImg, MARK("ARC_DCIRP_CropResizeImg"), 2000);
            ret = ARC_DCIRP_CropResizeImg(m_hCaptureBokeh, &AlgoBlurImg_Main, &cropsize,
                                          &AlgoBlurImg_OutputAux);
            Device->detector->End(m_detectorCropResizeImg);
        } else {
            DEV_LOGW("sdk camera bokeh only has one outputbuffer!!!");
        }
        Device->image->Copy(inputMain, outputMain); //主图inputMain作为临时buffer存放blur效果图
        Device->detector->Begin(m_detectorCropResizeImg, MARK("ARC_DCIRP_CropResizeImg"), 3000);
        ret |= ARC_DCIRP_CropResizeImg(m_hCaptureBokeh, &AlgoBlurImg_Main, &cropsize,
                                       &AlgoBlurImg_OutputMain);
        Device->detector->End(m_detectorCropResizeImg);
        DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "ARC_DCIRP_CropResizeImg failed");
        DEV_LOGI("ARC_DCIRP_CropResizeImg is %d", ret);
    } else {
        // sdk camera bokeh只有普通bokeh，没有sebokeh和hdrbokeh，只output一张bokeh效果图
        DEV_IF_LOGW_RETURN_RET((outputAux->plane[0] == NULL), RET_OK,
                               "sdk camera bokeh only has one outputbuffer!!!");
        Device->image->Copy(outputAux, inputMain);
    }
    return ret;
}

S32 ArcsoftDualCamBokehPlugin::PrepareYUV_OFFSCREEN(DEV_IMAGE_BUF *image, ASVLOFFSCREEN *stImage)
{
    stImage->i32Width = image->width;
    stImage->i32Height = image->height;

    if (image->format != CAM_FORMAT_YUV_420_NV12 && image->format != CAM_FORMAT_YUV_420_NV21 &&
        image->format != CAM_FORMAT_Y16) {
        DEV_LOGE(" format[%d] is not supported!", image->format);
        return RET_ERR;
    }
    if (image->format == CAM_FORMAT_YUV_420_NV12) {
        stImage->u32PixelArrayFormat = ASVL_PAF_NV12;
    } else if (image->format == CAM_FORMAT_YUV_420_NV21) {
        stImage->u32PixelArrayFormat = ASVL_PAF_NV21;
    } else if (image->format == CAM_FORMAT_Y16) {
        stImage->u32PixelArrayFormat = ASVL_PAF_GRAY;
    }
    stImage->pi32Pitch[0] = image->stride[0];
    stImage->pi32Pitch[1] = image->stride[1];
    stImage->pi32Pitch[2] = image->stride[2];
    stImage->ppu8Plane[0] = (MUInt8 *)image->plane[0];
    stImage->ppu8Plane[1] = (MUInt8 *)image->plane[1];
    stImage->ppu8Plane[2] = (MUInt8 *)image->plane[2];
    // stImage->fd[0] = (MInt32)image->fd[0];
    // stImage->fd[1] = (MInt32)image->fd[1];
    // stImage->fd[2] = (MInt32)image->fd[2];
    return RET_OK;
}