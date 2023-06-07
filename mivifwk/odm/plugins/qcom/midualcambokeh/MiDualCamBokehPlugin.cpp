#include <MiaPluginUtils.h>

#include "device/device.h"
#include "mialgo_dynamic_blur_control.h"
#include "nodeMetaData.h"
#include "MiDualCamBokehPlugin.h"

using namespace mialgo2;

#undef LOG_TAG
#define LOG_TAG "BOKEH_CAPTURE_BOKEH"

#undef LOG_GROUP
#define LOG_GROUP GROUP_PLUGIN

#define BOKEH_JSON_PATH_PRO "/odm/etc/camera/dualcam_bokeh_params_pro.json"
#define BOKEH_JSON_PATH     "/odm/etc/camera/dualcam_bokeh_params.json"

#define ROLE_REARBOKEH2X 61
#define ROLE_REARBOKEH1X 63

#define SHOW_AUX_ONLY  1
#define SHOW_MAIN_ONLY 2
#define SHOW_HALF_HALF 3

static const S32 g_bokehDump = property_get_int32("persist.vendor.camera.mialgo.capbokeh.dump", 0);
static const S32 g_nViewCtrl =
    property_get_int32("persist.vendor.camera.mialgo.capbokeh.debugview", 0);
static const S32 g_bDrawROI =
    property_get_int32("persist.vendor.camera.mialgo.capbokeh.debugroi", 0);
static const S32 g_BokehLite =
    property_get_int32("persist.vendor.camera.mialgo.capbokeh.debuglitemode", -1);
static const S32 g_MdMode =
    property_get_int32("persist.vendor.camera.mialgo.capbokeh.debugmdmode", 0);
static const S32 g_IsSingleCameraDepth =
    property_get_int32("persist.vendor.camera.mialgo.capbokeh.singlecameradepth", -1);

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

MiDualCamBokehPlugin::~MiDualCamBokehPlugin() {}

int MiDualCamBokehPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    Device->Init();
    Device->detector->Create(&m_detectorInit, MARK("m_detectorInit"));
    Device->detector->Begin(m_detectorInit, MARK("MiDualCamBokehPlugin initialize"), 1000);
    m_hCaptureBokeh = NULL;
    m_superNightEnabled = 0;
    m_ProcessCount = 0;
    m_NodeInterface = nodeInterface;
    m_pNodeMetaData = NULL;
    m_blurLevel = 0;
    m_LiteMode = 0;
    m_MDLiteMode = 0;
    m_MDMode = 0;
    memset((void *)&m_bokehlaunchParams, 0, sizeof(m_bokehlaunchParams));
    m_pMiMDBokeh = NULL;
    m_bokeh2xFallback = 0;
    Device->detector->Create(&m_detectorRun, MARK("m_detectorRun"));
    Device->detector->Create(&m_detectorBokehRun, MARK("m_detectorBokehRun"));
    m_pNodeMetaData = new NodeMetaData();
    if (m_pNodeMetaData != NULL) {
        m_pNodeMetaData->Init(0, 0);
    }
    Device->detector->End(m_detectorInit);
    return RET_OK;
}

void MiDualCamBokehPlugin::destroy()
{
    Device->detector->Begin(m_detectorInit, MARK("MiDualCamBokehPlugin destroy"), 1500);
    if (m_MDMode && (m_pMiMDBokeh != NULL)) {
        m_pMiMDBokeh->MDdestroy();
        delete m_pMiMDBokeh;
        m_pMiMDBokeh = NULL;
    }
    m_flustStatus = FALSE;
    if (m_hCaptureBokeh) {
        MialgoCaptureBokehDestory(&m_hCaptureBokeh);
        m_hCaptureBokeh = NULL;
    }

    if (m_pNodeMetaData) {
        delete m_pNodeMetaData;
        m_pNodeMetaData = NULL;
    }
    memset((void *)&m_bokehlaunchParams, 0, sizeof(m_bokehlaunchParams));
    Device->detector->Destroy(&m_detectorRun);
    Device->detector->Destroy(&m_detectorBokehRun);
    Device->detector->End(m_detectorInit);
    Device->detector->Destroy(&m_detectorInit);
    Device->Deinit();
}

ProcessRetStatus MiDualCamBokehPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus MiDualCamBokehPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    DEV_IF_LOGE_RETURN_RET(pProcessRequestInfo == NULL, (ProcessRetStatus)RET_ERR, "arg err");
    S32 ret = RET_OK;
    auto &inputBuffers = pProcessRequestInfo->inputBuffersMap.begin()->second;
    m_plogicalMeta = inputBuffers[0].pMetadata;
    m_pPhysicalMeta = inputBuffers[0].pPhyCamMetadata;
    S32 outputPorts = pProcessRequestInfo->outputBuffersMap.size();

    U64 runTime = 0;
    FP32 fNumber = 0;
    S32 thermalLevel = 0; // The Phone temperature 0: Low < 35; 1: 35 ~ 37;2: 41 ~43;
                          // 3: 43 ~ 45; 4: 46 ~ 48; 5:49~50
    DEV_IMAGE_BUF inputMain;
    DEV_IMAGE_BUF inputAux;
    DEV_IMAGE_BUF inputMain_extraEV = {0};
    DEV_IMAGE_BUF outputMain = {0};
    DEV_IMAGE_BUF outputAux = {0};

    // cpu boost 3000 ms
    std::shared_ptr<PluginPerfLockManager> boostCpu(
        new PluginPerfLockManager(3000, PerfLock_BOKEH_BOKEH));

    m_pNodeMetaData->GetSuperNightEnabled(m_plogicalMeta, &m_superNightEnabled);
    m_pNodeMetaData->GetMDMode(m_plogicalMeta, &m_MDMode);
    m_pNodeMetaData->GetThermalLevel(m_plogicalMeta, &thermalLevel);
    if (g_BokehLite < 0) {
        if (thermalLevel > 1) {
            m_LiteMode = 1;
        } else {
            m_LiteMode = 0;
        }
        //'m_MDLiteMode' just for mdBokeh blur, mdBokeh depth  still uses 'm_LiteMode'
        if (thermalLevel > 4) {
            m_MDLiteMode = 1;
        } else if (thermalLevel <= 4) {
            m_MDLiteMode = 0;
        }
    } else {
        m_LiteMode = g_BokehLite ? TRUE : FALSE; // for debug
        m_MDLiteMode = m_LiteMode;               // for debug
    }

    m_pNodeMetaData->GetFallbackStatus(m_plogicalMeta, &m_bokeh2xFallback);
    m_pNodeMetaData->GetFNumberApplied(m_plogicalMeta, &fNumber);
    m_blurLevel = findBlurLevelByFNumber(fNumber, false);

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

    for (auto &imageParams : pProcessRequestInfo->inputBuffersMap) {
        std::vector<ImageParams> &inputs = imageParams.second;
        uint32_t portId = imageParams.first;
        for (auto &input : inputs) {
            if (portId == 2) {
                ret = Device->image->MatchToImage((void *)&input, &inputMain_extraEV);
                DEV_IF_LOGE_RETURN_RET((ret != RET_OK), (ProcessRetStatus)ret, "Match buffer err");
                dumpImg(input.pMetadata, inputMain_extraEV, "inputMD", 0);
                break;
            }
        }
    }

    DEV_LOGI("superNightEnabled %d m_bokeh2xFallback %d", m_superNightEnabled, m_bokeh2xFallback);

    for (auto &imageParams : pProcessRequestInfo->outputBuffersMap) {
        std::vector<ImageParams> &outputs = imageParams.second;
        for (auto &output : outputs) {
            if (output.portId == 1) {
                ret = Device->image->MatchToImage((void *)&output, &outputAux);
                ret |= Device->image->Copy(&outputAux, &inputMain);
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
        Device->detector->Begin(m_detectorRun, MARK("BokehPlugin"), 5500);

        if (m_MDMode) {
            if (m_pMiMDBokeh == NULL) {
                m_pMiMDBokeh = new MiMDBokeh();
                m_pMiMDBokeh->MDinit();
            }
            ret = m_pMiMDBokeh->InitMDBokeh(&inputMain, m_plogicalMeta, m_MDMode, m_MDLiteMode);
#if 0
            if (inputMain_extraEV.width == 0) {
                // use last main input. Runs to here for non-Supernight mode.
                DEV_IF_LOGE((inputMainVector.size() < 2), "main inputNum=%d err", inputMainVector.size());
                inputMain_extraEV = inputMainVector[inputMainVector.size() - 1];
            }
#endif
        } else {
            ret = InitBokeh(&inputMain);
        }

        if (m_MDMode) {
            m_pMiMDBokeh->m_MDBlurLevel = m_blurLevel;
            ret = m_pMiMDBokeh->ProcessMDBokeh(&outputMain, &inputMain, &inputMain_extraEV,
                                               &inputAux, m_plogicalMeta, m_pPhysicalMeta);
        } else {
            ret = ProcessBokeh(&inputMain, &inputAux, &outputMain);
        }

        DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "LibProcess err %d", ret);
        runTime = Device->detector->End(m_detectorRun);
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
    }
    if (m_NodeInterface.pSetResultMetadata != NULL) { // For exif
        char buf[1024] = {0};
        if (m_MDMode) {
            m_LiteMode = m_MDLiteMode;
        }
        snprintf(buf, sizeof(buf),
                 "miDualBokeh:{processCount:%d blurLevel:%d "
                 "seBokeh:%d liteMode:%d mdMode:%d fallback:%d costTime:%d ret:%d}",
                 m_ProcessCount, m_blurLevel, m_superNightEnabled, m_LiteMode, m_MDMode,
                 m_bokeh2xFallback, runTime, ret);
        std::string results(buf);
        m_NodeInterface.pSetResultMetadata(m_NodeInterface.owner, pProcessRequestInfo->frameNum,
                                           pProcessRequestInfo->timeStamp, results);
    }
    m_ProcessCount++;
    return (ProcessRetStatus)RET_OK;
}

S32 MiDualCamBokehPlugin::InitBokeh(DEV_IMAGE_BUF *inputMain)
{
    S32 ret = RET_OK;
    S32 expValue = -1;
    DEV_IF_LOGE_RETURN_RET((inputMain == NULL), RET_ERR, "error invalid param");
    U32 MainImgW = inputMain->width;
    U32 MainImgH = inputMain->height;
    AlgoMultiCams::ALGO_CAPTURE_FRAME Scale = AlgoMultiCams::ALGO_CAPTURE_FRAME::CAPTURE_FRAME_4_3;
    GetImageScale(MainImgW, MainImgH, Scale);
    InputMetadataBokeh inputMetaBokeh = {0};
    ret = m_pNodeMetaData->GetInputMetadataBokeh(m_plogicalMeta, &inputMetaBokeh);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetInputMetadataBokeh err");

    U32 afMode = 0;
    ret = m_pNodeMetaData->GetAFMode(m_plogicalMeta, &afMode);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetAFMode err");
    U32 motionCaptureType = 0;
    ret = m_pNodeMetaData->GetMotionCaptureType(m_plogicalMeta, &motionCaptureType);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetMotionCaptureType err");

    AlgoBokehLaunchParams bokehlaunchParams;
    bokehlaunchParams.MainImgW = MainImgW;
    bokehlaunchParams.MainImgH = MainImgH;
    bokehlaunchParams.frameInfo = Scale;
    if (ROLE_REARBOKEH2X == inputMetaBokeh.bokehRole || m_bokeh2xFallback) {
        bokehlaunchParams.FovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_3X;
    } else {
        bokehlaunchParams.FovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_1X;
    }
    bokehlaunchParams.jsonFilename = (char *)BOKEH_JSON_PATH_PRO;
    DEV_LOGI(" afMode = %d", afMode);
    if (afMode == 1) {
        bokehlaunchParams.isQuickSnap = true;
    } else {
        bokehlaunchParams.isQuickSnap = false;
    }
    DEV_LOGI(" motionCaptureType = %4X", motionCaptureType);
    if ((motionCaptureType & 0xFF) != 0) {
        bokehlaunchParams.isUserTouch = true;
    } else {
        bokehlaunchParams.isUserTouch = false;
    }

    if (m_LiteMode == 1) {
        bokehlaunchParams.isHotMode = TRUE;
        bokehlaunchParams.isPerformanceMode = TRUE;
    } else {
        bokehlaunchParams.isHotMode = FALSE;
        bokehlaunchParams.isPerformanceMode = FALSE;
    }

    if (memcmp(&m_bokehlaunchParams, &bokehlaunchParams, sizeof(bokehlaunchParams)) != 0) {
        memcpy(&m_bokehlaunchParams, &bokehlaunchParams, sizeof(bokehlaunchParams));
        DEV_LOGI(
            "INIT INFO INIT M(%dx%d) zoomRatio=%d Scale=%d "
            "isHotMode=%d jsonPath=%s",
            m_bokehlaunchParams.MainImgW, m_bokehlaunchParams.MainImgH, m_bokehlaunchParams.FovInfo,
            m_bokehlaunchParams.frameInfo, m_bokehlaunchParams.isHotMode,
            m_bokehlaunchParams.jsonFilename);
        if (m_hCaptureBokeh != NULL) {
            MialgoCaptureBokehDestory(&m_hCaptureBokeh);
            m_hCaptureBokeh = NULL;
        }
        Device->detector->Begin(m_detectorBokehRun, MARK("MialgoCaptureBokehLaunch"), 1000);
        ret = MialgoCaptureBokehLaunch(&m_hCaptureBokeh, m_bokehlaunchParams);
        Device->detector->End(m_detectorBokehRun);
        DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "MialgoCaptureBokehLaunch err=%d", ret);
        DEV_IF_LOGE_RETURN_RET(m_hCaptureBokeh == NULL, RET_ERR, "m_hCaptureBokeh err=%p",
                               m_hCaptureBokeh);
    }
    return ret;
}

S32 MiDualCamBokehPlugin::PrepareToMiImage(DEV_IMAGE_BUF *image, AlgoMultiCams::MiImage *stImage)
{
    stImage->width = image->width;
    stImage->height = image->height;

    if (image->format != CAM_FORMAT_YUV_420_NV12 && image->format != CAM_FORMAT_YUV_420_NV21 &&
        image->format != CAM_FORMAT_Y16) {
        DEV_LOGE(" format[%d] is not supported!", image->format);
        return RET_ERR;
    }
    if (image->format == CAM_FORMAT_YUV_420_NV12) {
        stImage->fmt = AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_NV12;
    } else if (image->format == CAM_FORMAT_YUV_420_NV21) {
        stImage->fmt = AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_NV21;
    } else if (image->format == CAM_FORMAT_Y16) {
        stImage->fmt = AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_GRAY;
    }
    stImage->pitch[0] = image->stride[0];
    stImage->pitch[1] = image->stride[1];
    stImage->pitch[2] = image->stride[2];
    stImage->slice_height[0] = image->sliceHeight[0];
    stImage->slice_height[1] = image->sliceHeight[1];
    stImage->slice_height[2] = image->sliceHeight[2];
    stImage->plane[0] = (unsigned char *)image->plane[0];
    stImage->plane[1] = (unsigned char *)image->plane[1];
    stImage->plane[2] = (unsigned char *)image->plane[2];
    stImage->fd[0] = image->fd[0];
    stImage->fd[1] = image->fd[1];
    stImage->fd[2] = image->fd[2];
    stImage->plane_count = image->planeCount;
    return RET_OK;
}

S32 MiDualCamBokehPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

S32 MiDualCamBokehPlugin::GetImageScale(U32 width, U32 height,
                                        AlgoMultiCams::ALGO_CAPTURE_FRAME &Scale)
{
    int scale_4_3 = 10 * 4 / 3;
    int scale_16_9 = 10 * 16 / 9;
    int imagescale = 10 * width / ((height == 0) ? 1 : height);
    if (scale_4_3 == imagescale) {
        Scale = AlgoMultiCams::ALGO_CAPTURE_FRAME::CAPTURE_FRAME_4_3;
    } else if (scale_16_9 == imagescale) {
        Scale = AlgoMultiCams::ALGO_CAPTURE_FRAME::CAPTURE_FRAME_16_9;
    } else {
        Scale = AlgoMultiCams::ALGO_CAPTURE_FRAME::CAPTURE_FRAME_FULL_FRAME;
    }
    //    DEV_LOGI("GetImageScale scale %d", Scale);
    return RET_OK;
}

S32 MiDualCamBokehPlugin::ProcessBokeh(DEV_IMAGE_BUF *inputMain, DEV_IMAGE_BUF *inputDepth,
                                       DEV_IMAGE_BUF *outputMain)
{
    DEV_IF_LOGE_RETURN_RET((outputMain == NULL || inputMain == NULL || inputDepth == NULL), RET_ERR,
                           "ARG ERR");
    DEV_IF_LOGE_RETURN_RET((m_hCaptureBokeh == NULL), RET_ERR, "ARG ERR");

    S32 ret = RET_OK;
    bool hasFaceNoTouch = false;
    U32 afState = ANDROID_CONTROL_AF_STATE_INACTIVE;
    AlgoMultiCams::MiImage AlgoDepthImg_inputMain = {
        AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_INVALID};
    AlgoMultiCams::MiImage AlgoDepthImg_inputDepth = {
        AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_INVALID};
    AlgoMultiCams::MiImage AlgoDepthImg_outputMain = {
        AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_INVALID};
    disparity_fileheader_t *header = NULL;
    unsigned char *depthData = NULL;
    AlgoBokehInitParams bokehInitParams = {{0}};
    AlgoMultiCams::MiFaceRect faceRect = {0};
    AlgoMultiCams::MiPoint2i faceCentor = {0};
    DEV_IMAGE_POINT focusTP = {0};
    AlgoMultiCams::MiPoint2i AlgoDepthfocusTP = {0};
    S32 sensitivity = 0;
    InputMetadataBokeh inputMetaBokeh = {0};
    ret = m_pNodeMetaData->GetSensitivity(m_plogicalMeta, &sensitivity);

    S32 superNightEnabled = 0;
    S32 hdrEnabled = 0;
    m_pNodeMetaData->GetSuperNightEnabled(m_plogicalMeta, &superNightEnabled);
    m_pNodeMetaData->GetHdrEnabled(m_plogicalMeta, &hdrEnabled);

    ret |= PrepareToMiImage(inputMain, &AlgoDepthImg_inputMain);
    ret |= PrepareToMiImage(inputDepth, &AlgoDepthImg_inputDepth);
    ret |= PrepareToMiImage(outputMain, &AlgoDepthImg_outputMain);
    DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "PrepareToMiImage=%d", ret);
    ret = m_pNodeMetaData->GetInputMetadataBokeh(m_plogicalMeta, &inputMetaBokeh);
    m_pNodeMetaData->GetMaxFaceRect(m_pPhysicalMeta, inputMain, &faceRect, &faceCentor);
    ret = m_pNodeMetaData->GetfocusTP(m_pPhysicalMeta, inputMain, &focusTP);
    AlgoDepthfocusTP.x = focusTP.x;
    AlgoDepthfocusTP.y = focusTP.y;
    m_pNodeMetaData->GetAFState(m_plogicalMeta, &afState);
    if ((ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED != afState &&
         ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN != afState) &&
        (faceRect.width != 0 && faceRect.height != 0)) {
        hasFaceNoTouch = true;
    }
    bokehInitParams.userInterface.fRect = faceRect;
    bokehInitParams.userInterface.tp = hasFaceNoTouch ? faceCentor : AlgoDepthfocusTP;
    bokehInitParams.userInterface.ISO = sensitivity;
    bokehInitParams.userInterface.aperture = m_blurLevel;

    if (m_superNightEnabled != 0) {
        bokehInitParams.isSingleCameraDepth = TRUE;
        if (g_IsSingleCameraDepth >= 0) {
            bokehInitParams.isSingleCameraDepth = g_IsSingleCameraDepth ? TRUE : FALSE;
        }
    } else {
        bokehInitParams.isSingleCameraDepth = FALSE;
    }

    DEV_LOGI("INIT INFO BlurLevel %d ISO=%d", bokehInitParams.userInterface.aperture,
             bokehInitParams.userInterface.ISO);
    Device->detector->Begin(m_detectorBokehRun, MARK("MialgoCaptureBokehInit"), 1000);
    MialgoCaptureBokehInit(&m_hCaptureBokeh, bokehInitParams);
    Device->detector->End(m_detectorBokehRun);
    DEV_IF_LOGE_GOTO((m_hCaptureBokeh == NULL), exit, "MialgoCaptureBokehInit ERR %p",
                     m_hCaptureBokeh);
    AlgoBokehEffectParams bokehParams;
    bokehParams.hdrTriggered = hdrEnabled;
    bokehParams.seTriggered = superNightEnabled;
    bokehParams.imgMain = &AlgoDepthImg_inputMain;
    bokehParams.imgDepth = &AlgoDepthImg_inputDepth;
    header = (disparity_fileheader_t *)(bokehParams.imgDepth->plane[0]); // 地址偏移预留头空间
    bokehParams.validDepthSize = header->i32DataLength;
    bokehParams.imgResult = &AlgoDepthImg_outputMain;
    bokehParams.imgDepth->plane[0] =
        (unsigned char *)(bokehParams.imgDepth->plane[0] +
                          sizeof(disparity_fileheader_t)); // 地址偏移预留头空间
    Device->detector->Begin(m_detectorBokehRun, MARK("MialgoCaptureBokehProc"), 4600);
    ret = MialgoCaptureBokehProc(&m_hCaptureBokeh, bokehParams);
    Device->detector->End(m_detectorBokehRun);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Bokeh MIALGO_CaptureBokehEffectProc failed=%d", ret);

exit:
    if (m_hCaptureBokeh != NULL) {
        MialgoCaptureBokehDeinit(&m_hCaptureBokeh);
        if (ret != RET_OK) {
            if (m_hCaptureBokeh != NULL) {
                MialgoCaptureBokehDestory(&m_hCaptureBokeh);
                memset((void *)&m_bokehlaunchParams, 0, sizeof(m_bokehlaunchParams));
                m_hCaptureBokeh = NULL;
            }
        }
    }
    return ret;
}
