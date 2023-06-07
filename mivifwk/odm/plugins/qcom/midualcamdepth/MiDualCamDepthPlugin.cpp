#include "MiDualCamDepthPlugin.h"

#include <MiaPluginUtils.h>
#include "dualcam_mdbokeh_capture_proc.h"

using namespace mialgo2;

#undef LOG_TAG
#define LOG_TAG "BOKEH_CAPTURE_DEPTH"

#undef LOG_GROUP
#define LOG_GROUP GROUP_PLUGIN

#define CAL_DATA_PATH_W_T   "/data/vendor/camera/com.xiaomi.dcal.wt.data"
#define CAL_DATA_PATH_W_U   "/data/vendor/camera/com.xiaomi.dcal.wu.data"
#define CAL_GOLDEN_PATH_W_T "/odm/etc/camera/com.xiaomi.dcal.wt.golden"
#define CAL_GOLDEN_PATH_W_U "/odm/etc/camera/com.xiaomi.dcal.wu.golden"

#define CAL_DATA_PATH_FRONT "/data/vendor/camera/com.xiaomi.dcal.ffaux.data"
#define MD_DEPTH_JSON_PATH  "/odm/etc/camera/dualcam_mdbokeh_params_depth.json"
#define DEPTH_JSON_PATH_PRO "/odm/etc/camera/dualcam_depth_params_pro.json"
#define DEPTH_JSON_PATH     "/odm/etc/camera/dualcam_depth_params.json"

#define ROLE_REARBOKEH2X 61
#define ROLE_REARBOKEH1X 63

#define CALIBRATIONDATA_LEN 4096

static const S32 g_DebugBypss =
    property_get_int32("persist.vendor.camera.mialgo.capdepth.debugbypass", 0);
static const S32 g_bokehDump = property_get_int32("persist.vendor.camera.mialgo.capdepth.dump", 0);
static const S32 g_bDrawROI =
    property_get_int32("persist.vendor.camera.mialgo.capdepth.debugroi", 0);
static const S32 g_BokehLite =
    property_get_int32("persist.vendor.camera.mialgo.capdepth.debuglitemode", -1);
static const S32 g_MdMode =
    property_get_int32("persist.vendor.camera.mialgo.capdepth.debugmdmode", 0);
static const S32 g_IsSingleCameraDepth =
    property_get_int32("persist.vendor.camera.mialgo.capdepth.singlecameradepth", -1);

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

MiDualCamDepthPlugin::~MiDualCamDepthPlugin() {}

int MiDualCamDepthPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    Device->Init();
    m_hCaptureDepth = NULL;
    mCalibrationFront = NULL;
    mCalibrationWU = NULL;
    mCalibrationWT = NULL;
    m_superNightEnabled = 0;
    m_ProcessCount = 0;
    m_NodeInterface = nodeInterface;
    m_pNodeMetaData = NULL;
    m_blurLevel = 0;
    m_LiteMode = 0;
    m_MDMode = 0;
    memset((void *)&m_depthlaunchParams, 0, sizeof(m_depthlaunchParams));
    m_bokeh2xFallback = 0;
    Device->detector->Create(&m_detectorRun, MARK("m_detectorRun"));
    Device->detector->Create(&m_detectorDepthRun, MARK("m_detectorDepthRun"));
    m_pNodeMetaData = new NodeMetaData();
    if (m_pNodeMetaData != NULL) {
        m_pNodeMetaData->Init(0, 0);
    }
    if (RET_OK != GetPackData(CAL_DATA_PATH_W_U, &mCalibrationWU, &m_CalibrationWULength)) {
        DEV_LOGW("read dcal data path: %s fail", CAL_DATA_PATH_W_U);
        if (RET_OK != GetPackData(CAL_GOLDEN_PATH_W_U, &mCalibrationWU, &m_CalibrationWULength)) {
            DEV_LOGE("fallback to dcal golden path: %s fail", CAL_GOLDEN_PATH_W_U);
        }
    }
    if (RET_OK != GetPackData(CAL_DATA_PATH_W_T, &mCalibrationWT, &m_CalibrationWTLength)) {
        DEV_LOGW("Read dcal data path: %s fail", CAL_DATA_PATH_W_T);
        if (RET_OK != GetPackData(CAL_GOLDEN_PATH_W_T, &mCalibrationWT, &m_CalibrationWTLength)) {
            DEV_LOGE("fallback to dcal golden path: %s fail", CAL_GOLDEN_PATH_W_T);
        }
    }
    return RET_OK;
}

void MiDualCamDepthPlugin::destroy()
{
    m_flustStatus = FALSE;

    if (m_hCaptureDepth) {
        MialgoCaptureDepthDestory(&m_hCaptureDepth);
        m_hCaptureDepth = NULL;
    }

    FreePackData(&mCalibrationFront);
    FreePackData(&mCalibrationWU);
    FreePackData(&mCalibrationWT);
    if (m_pNodeMetaData) {
        delete m_pNodeMetaData;
        m_pNodeMetaData = NULL;
    }
    memset((void *)&m_depthlaunchParams, 0, sizeof(m_depthlaunchParams));
    Device->detector->Destroy(&m_detectorRun);
    Device->detector->Destroy(&m_detectorDepthRun);
    Device->Deinit();
}

S64 MiDualCamDepthPlugin::GetPackData(char *fileName, void **ppData0, U64 *pSize0)
{
    S64 ret = RET_OK;
    S64 packDataSize = 0;
    void *packDataBuf = NULL;
    char packDataFileName[128] = CAL_DATA_PATH_W_U;
    Dev_snprintf(packDataFileName, sizeof(packDataFileName), "%s", fileName);
    DEV_IF_LOGE_RETURN_RET((NULL == pSize0), RET_ERR_ARG, "pSize1 is null");
    packDataSize = Device->file->GetSize(packDataFileName);
    DEV_IF_LOGE_RETURN_RET((0 == packDataSize), RET_ERR, "get file err");
    packDataBuf = Device->memoryPool->Malloc(packDataSize, MARK("Bokeh CalibrationData"));
    DEV_IF_LOGE_RETURN_RET((NULL == packDataBuf), RET_ERR, "malloc err");
    ret = Device->file->ReadOnce(packDataFileName, packDataBuf, packDataSize, 0);
    if (ret != packDataSize) {
        Device->memoryPool->Free(packDataBuf);
    }
    DEV_IF_LOGE_RETURN_RET((ret != packDataSize), RET_ERR, "read file err %" PRIi64, ret);
    DEV_IF_LOGI((ret == packDataSize), "INFO CROSSTALK FILE[%s][%" PRIi64 "]", packDataFileName,
                packDataSize);
    *pSize0 = packDataSize;
    *ppData0 = packDataBuf;
    return RET_OK;
}

S64 MiDualCamDepthPlugin::FreePackData(void **ppData0)
{
    if (ppData0 != NULL) {
        if (*ppData0) {
            Device->memoryPool->Free(*ppData0);
            *ppData0 = NULL;
        }
    }
    return RET_OK;
}

ProcessRetStatus MiDualCamDepthPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus MiDualCamDepthPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    DEV_IF_LOGE_RETURN_RET(pProcessRequestInfo == NULL, (ProcessRetStatus)RET_ERR, "arg err");
    S32 ret = RET_OK;
    auto &inputBuffers = pProcessRequestInfo->inputBuffersMap.begin()->second;
    m_plogicalMeta = inputBuffers[0].pMetadata;
    m_pPhysicalMeta = inputBuffers[0].pPhyCamMetadata;
    U64 runTime = 0;
    S32 thermalLevel = 0; // The Phone temperature 0: Low < 35; 1: 35 ~ 37;2: 41 ~43;
                          // 3: 43 ~ 45; 4: 46 ~ 48; 5:49~50
    DEV_IMAGE_BUF inputMain = {0};
    DEV_IMAGE_BUF inputAux = {0};
    DEV_IMAGE_BUF outputDepth = {0};
    S32 depthDataSize = 0;

    // cpu boost 3000 ms
    std::shared_ptr<PluginPerfLockManager> boostCpu(
        new PluginPerfLockManager(3000, PerfLock_BOKEH_DEPTH));

    ret = m_pNodeMetaData->GetSuperNightEnabled(m_plogicalMeta, &m_superNightEnabled);
    if (ret != RET_OK) {
        m_superNightEnabled = 0;
    }
    ret = m_pNodeMetaData->GetMDMode(m_plogicalMeta, &m_MDMode);
    if (ret != RET_OK) {
        m_MDMode = 0;
    }
    ret = m_pNodeMetaData->GetThermalLevel(m_plogicalMeta, &thermalLevel);
    if (g_BokehLite < 0) {
        if (thermalLevel > 1) {
            m_LiteMode = 1;
        } else {
            m_LiteMode = 0;
        }
    } else {
        m_LiteMode = g_BokehLite ? TRUE : FALSE; // for debug
    }

    ret = m_pNodeMetaData->GetFallbackStatus(m_plogicalMeta, &m_bokeh2xFallback);
    if (ret != RET_OK) {
        m_bokeh2xFallback = 0;
    }

    auto dumpImg = [&](camera_metadata_t *pLogMeta, DEV_IMAGE_BUF &img, const char *pName,
                       int Idx) {
        S32 expValue = 0;
        m_pNodeMetaData->GetExpValue(pLogMeta, &expValue);
        DEV_LOGI("get image %s EV=%d %dx%d,stride=%dx%d,fmt=%d", pName, expValue, img.width,
                 img.height, img.stride[0], img.sliceHeight[0], img.format);

        if (g_bokehDump) {
            char fileName[128];
            snprintf(fileName, sizeof(fileName), "bokeh_capture_depth_%s_%d_EV%d", pName, Idx,
                     expValue);
            Device->image->DumpImage(&img, fileName, (U32)pProcessRequestInfo->frameNum, ".NV12");
        }
    };

    for (auto &imageParams : pProcessRequestInfo->inputBuffersMap) {
        std::vector<ImageParams> &inputs = imageParams.second;
        U32 portId = imageParams.first;
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
        U32 portId = imageParams.first;
        for (auto &input : inputs) {
            if (portId == 1) {
                ret = Device->image->MatchToImage((void *)&input, &inputAux);
                DEV_IF_LOGE_RETURN_RET((ret != RET_OK), (ProcessRetStatus)ret, "Match buffer err");
                dumpImg(input.pMetadata, inputAux, "inputAux", 0);
                break;
            }
        }
    }
    DEV_LOGI("superNightEnabled %d m_bokeh2xFallback %d", m_superNightEnabled, m_bokeh2xFallback);
    auto &outBuffers = pProcessRequestInfo->outputBuffersMap.at(0);
    ret = Device->image->MatchToImage((void *)&outBuffers[0], &outputDepth);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), (ProcessRetStatus)ret, "Match buffer err");

    DEV_LOGI("output main width: %d, height: %d, stride: %d, scanline: %d, format: %d",
             outputDepth.width, outputDepth.height, outputDepth.stride[0],
             outputDepth.sliceHeight[0], outputDepth.format);

    if (0 == g_DebugBypss) {
        Device->detector->Begin(m_detectorRun, MARK("BokehPlugin"), 5500);
        ret = InitDepth(&inputMain, &inputAux);
        DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "InitDepth err %d", ret);

        ret = ProcessDepth(&inputMain, &inputAux, &outputDepth, &depthDataSize);
        DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "ProcessDepth failed=%d", ret);
        runTime = Device->detector->End(m_detectorRun);
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
        Device->image->DrawPoint(&outputDepth, focusTP, 10, 0);
        Device->image->DrawRect(&outputDepth, focusROI, 5, 5);
        Device->image->DrawRect(&outputDepth, faceRect, 2, 50);
    }

    if (g_bokehDump) {
        Device->image->DumpImage(&outputDepth, "bokeh_capture_depth_outputMain",
                                 (U32)pProcessRequestInfo->frameNum, ".NV12");
    }
exit:
    // just log and don't copy input to output because output of depth is not image
    DEV_IF_LOGE((RET_OK != ret), "process fail: %d", ret);

    if (m_NodeInterface.pSetResultMetadata != NULL) { // For exif
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf),
                 "miDualDepth:{processCount:%d blurLevel:%d "
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

S32 MiDualCamDepthPlugin::InitDepth(DEV_IMAGE_BUF *inputMain, DEV_IMAGE_BUF *inputAux)
{
    DEV_IF_LOGE_RETURN_RET(inputMain == NULL, RET_ERR, "inputMain err");
    DEV_IF_LOGE_RETURN_RET(inputAux == NULL, RET_ERR, "inputAux err");
    S32 ret = RET_OK;
    S32 expValue = -1;
    S32 thermalLevel = 0; // The Phone temperature 0: Low < 35; 1: 35 ~ 37;2: 41 ~43;
                          // 3: 43 ~ 45; 4: 46 ~ 48; 5:49~50

    AlgoMultiCams::ALGO_CAPTURE_FRAME Scale = AlgoMultiCams::ALGO_CAPTURE_FRAME::CAPTURE_FRAME_4_3;
    GetImageScale(inputMain->width, inputMain->height, Scale);
    InputMetadataBokeh inputMetaBokeh = {0};
    ret = m_pNodeMetaData->GetInputMetadataBokeh(m_plogicalMeta, &inputMetaBokeh);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetInputMetadataBokeh err");

    BokehSensorConfig bokehSensorConfig = {0};
    ret = m_pNodeMetaData->GetBokehSensorConfig(m_plogicalMeta, &bokehSensorConfig);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetBokehSensorConfig err");

    AlgoDepthLaunchParams depthlaunchParams;
    depthlaunchParams.MainImgW = inputMain->width;
    depthlaunchParams.MainImgH = inputMain->height;
    depthlaunchParams.AuxImgW = inputAux->width;
    depthlaunchParams.AuxImgH = inputAux->height;
    depthlaunchParams.frameInfo = Scale;

    if (bokehSensorConfig.sensorCombination == BOKEH_COMBINATION_T_W) {
        depthlaunchParams.sensorCombination =
            AlgoMultiCams::ALGO_CAPTURE_SENSOR_COMBINATION::ALGO_T_W;
        depthlaunchParams.calibBuffer = (unsigned char *)mCalibrationWT;
        depthlaunchParams.calibBufferSize = CALIBRATIONDATA_LEN;
    } else {
        depthlaunchParams.sensorCombination =
            AlgoMultiCams::ALGO_CAPTURE_SENSOR_COMBINATION::ALGO_W_UW;
        depthlaunchParams.calibBuffer = (unsigned char *)mCalibrationWU;
        depthlaunchParams.calibBufferSize = CALIBRATIONDATA_LEN;
    }
    if (bokehSensorConfig.SensorCapHintMaster == BOKEH_SENSORCAP_BINING) {
        depthlaunchParams.mainSensorMode =
            AlgoMultiCams::ALGO_CAPTURE_SENSOR_MODE::ALGO_BINING_MODE;
    } else {
        depthlaunchParams.mainSensorMode =
            AlgoMultiCams::ALGO_CAPTURE_SENSOR_MODE::ALGO_REMOSAIC_MODE;
    }
    if (bokehSensorConfig.SensorCapHintSlave == BOKEH_SENSORCAP_BINING) {
        depthlaunchParams.auxSensorMode = AlgoMultiCams::ALGO_CAPTURE_SENSOR_MODE::ALGO_BINING_MODE;
    } else {
        depthlaunchParams.auxSensorMode =
            AlgoMultiCams::ALGO_CAPTURE_SENSOR_MODE::ALGO_REMOSAIC_MODE;
    }

    if (ROLE_REARBOKEH2X == inputMetaBokeh.bokehRole || m_bokeh2xFallback) {
        depthlaunchParams.FovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_3X;
    } else {
        depthlaunchParams.FovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_1X;
    }

    depthlaunchParams.isMirroredInput = false;

    if (m_MDMode) {
        if (m_MDMode == CAPTURE_MDBOKEH_SPIN) {
            depthlaunchParams.FovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_2X;
        } else if (m_MDMode == CAPTURE_MDBOKEH_SOFT) {
            depthlaunchParams.FovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_3_75X;
        } else if (m_MDMode == CAPTURE_MDBOKEH_BLACK) {
            depthlaunchParams.FovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_1_4X;
        } else if (m_MDMode == CAPTURE_MDBOKEH_PORTRAIT) {
            depthlaunchParams.FovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_3_26X;
        } else {
            depthlaunchParams.FovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_2X;
            DEV_LOGE("set depthlaunchParams.FovInfo error. m_MDMode = %d", m_MDMode);
        }
    }

    if (m_LiteMode == 1) {
        depthlaunchParams.isHotMode = TRUE;
        depthlaunchParams.isPerformanceMode = TRUE;
    } else {
        depthlaunchParams.isHotMode = FALSE;
        depthlaunchParams.isPerformanceMode = FALSE;
    }
    if (m_MDMode) {
        depthlaunchParams.jsonFilename = (char *)MD_DEPTH_JSON_PATH;
    } else {
        depthlaunchParams.jsonFilename = (char *)DEPTH_JSON_PATH_PRO;
    }

    if (memcmp(&m_depthlaunchParams, &depthlaunchParams, sizeof(depthlaunchParams)) != 0) {
        memcpy(&m_depthlaunchParams, &depthlaunchParams, sizeof(depthlaunchParams));
        DEV_LOGI(
            "INIT INFO M(%dx%d) A(%dx%d) sensorCombination=%d mainSensorMode=%d auxSensorMode=%d "
            "calibBuffer=%p mCalibrationWT=%p mCalibrationWU=%p zoomRatio=%d Scale=%d "
            "isHotMode=%d isMirroredInput=%d",
            m_depthlaunchParams.MainImgW, m_depthlaunchParams.MainImgH, m_depthlaunchParams.AuxImgW,
            m_depthlaunchParams.AuxImgH, m_depthlaunchParams.sensorCombination,
            m_depthlaunchParams.mainSensorMode, m_depthlaunchParams.auxSensorMode,
            m_depthlaunchParams.calibBuffer, mCalibrationWT, mCalibrationWU,
            m_depthlaunchParams.FovInfo, m_depthlaunchParams.frameInfo,
            m_depthlaunchParams.isHotMode, depthlaunchParams.isMirroredInput);
        if (m_hCaptureDepth != NULL) {
            MialgoCaptureDepthDestory(&m_hCaptureDepth);
            m_hCaptureDepth = NULL;
        }
        Device->detector->Begin(m_detectorDepthRun, MARK("MialgoCaptureDepthLaunch"), 3500);
        ret = MialgoCaptureDepthLaunch(&m_hCaptureDepth, m_depthlaunchParams);
        Device->detector->End(m_detectorDepthRun);
        DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "MialgoCaptureDepthLaunch err=%d", ret);
        DEV_IF_LOGE_RETURN_RET(m_hCaptureDepth == NULL, RET_ERR, "m_hCaptureDepth err=%p",
                               m_hCaptureDepth);
    }
    return ret;
}

S32 MiDualCamDepthPlugin::PrepareToMiImage(DEV_IMAGE_BUF *image, AlgoMultiCams::MiImage *stImage)
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
    stImage->fd[0] = (int)image->fd[0];
    stImage->fd[1] = (int)image->fd[1];
    stImage->fd[2] = (int)image->fd[2];
    stImage->plane_count = (int)image->planeCount;
    return RET_OK;
}

S32 MiDualCamDepthPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

S32 MiDualCamDepthPlugin::GetImageScale(U32 width, U32 height,
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

S32 MiDualCamDepthPlugin::ProcessDepth(DEV_IMAGE_BUF *inputMain, DEV_IMAGE_BUF *inputAaux,
                                       DEV_IMAGE_BUF *outputDepth, S32 *depthDataSize)
{
    DEV_IF_LOGE_RETURN_RET(depthDataSize == NULL, RET_ERR, "ARG ERR");
    DEV_IF_LOGE_RETURN_RET(inputAaux == NULL, RET_ERR, "ARG ERR");
    DEV_IF_LOGE_RETURN_RET((outputDepth == NULL || depthDataSize == 0), RET_ERR, "ARG ERR");
    S32 ret = RET_OK;
    U32 afState = ANDROID_CONTROL_AF_STATE_INACTIVE;
    AlgoMultiCams::MiImage AlgoDepthImg_Aux = {AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_INVALID};
    AlgoMultiCams::MiImage AlgoDepthImg_Main = {AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_INVALID};
    AlgoMultiCams::MiImage AlgoDepthImg_Depth = {AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_INVALID};
    U32 JPEGOrientation = 0;
    disparity_fileheader_t *header = NULL;
    unsigned char *depthData = NULL;
    AlgoDepthInitParams depthInitParams = {0};
    AlgoDepthParams depthParams = {0};
    AlgoMultiCams::MiFaceRect faceRect = {0};
    DEV_IMAGE_POINT focusTP = {0};
    AlgoMultiCams::MiPoint2i AlgoDepthfocusTP = {0};
    InputMetadataBokeh inputMetaBokeh = {0};
    LaserDistance laserDist = {0};
    ret = m_pNodeMetaData->GetLaserDist(m_plogicalMeta, &laserDist);
    S32 sensitivity = 0;
    ret = m_pNodeMetaData->GetSensitivity(m_plogicalMeta, &sensitivity);
    FP32 fNumber = 0;
    ret = m_pNodeMetaData->GetFNumberApplied(m_plogicalMeta, &fNumber);
    m_blurLevel = findBlurLevelByFNumber(fNumber, false);
    S32 focusDistCm = 0;
    ret = m_pNodeMetaData->GetfocusDistCm(m_plogicalMeta, &focusDistCm);
    m_pNodeMetaData->GetRotateAngle(m_plogicalMeta, &JPEGOrientation);

    ret |= PrepareToMiImage(inputMain, &AlgoDepthImg_Main);
    ret |= PrepareToMiImage(inputAaux, &AlgoDepthImg_Aux);
    ret |= PrepareToMiImage(outputDepth, &AlgoDepthImg_Depth);
    DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "PrepareToMiImage=%d", ret);

    depthInitParams.imgMian = &AlgoDepthImg_Main;
    depthInitParams.imgAux = &AlgoDepthImg_Aux;
    ret = m_pNodeMetaData->GetInputMetadataBokeh(m_plogicalMeta, &inputMetaBokeh);
    m_pNodeMetaData->GetMaxFaceRect(m_pPhysicalMeta, inputMain, &faceRect);
    ret = m_pNodeMetaData->GetfocusTP(m_pPhysicalMeta, inputMain, &focusTP);
    AlgoDepthfocusTP.x = focusTP.x;
    AlgoDepthfocusTP.y = focusTP.y;
    m_pNodeMetaData->GetAFState(m_plogicalMeta, &afState);
    if ((ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED != afState &&
         ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN != afState) &&
        (faceRect.width != 0 && faceRect.height != 0)) {
        AlgoDepthfocusTP.x = faceRect.x + faceRect.width / 2;
        AlgoDepthfocusTP.y = faceRect.y + faceRect.height / 2;
        if (AlgoDepthfocusTP.x <= 0 || AlgoDepthfocusTP.y <= 0 ||
            AlgoDepthfocusTP.x > AlgoDepthImg_Main.width ||
            AlgoDepthfocusTP.y > AlgoDepthImg_Main.height) {
            AlgoDepthfocusTP.x = AlgoDepthImg_Main.width / 2;
            AlgoDepthfocusTP.y = AlgoDepthImg_Main.height / 2;
        }
        DEV_LOGI("focusTP(%d, %d), faceTP (%d, %d), afState %d", focusTP.x, focusTP.y,
                 AlgoDepthfocusTP.x, AlgoDepthfocusTP.y, afState);
    }
    depthInitParams.userInterface.fRect = faceRect;
    depthInitParams.userInterface.tp = AlgoDepthfocusTP;
    depthInitParams.userInterface.TOF = laserDist.distance;
    depthInitParams.userInterface.ISO = sensitivity;
    depthInitParams.userInterface.aperture = m_blurLevel;
    depthInitParams.userInterface.AfCode = focusDistCm;
    depthInitParams.userInterface.phoneOrientation = JPEGOrientation;
    memset(&depthInitParams.mianCropRegion, 0, sizeof(depthInitParams.mianCropRegion));
    memset(&depthInitParams.auxCropRegion, 0, sizeof(depthInitParams.auxCropRegion));

    DEV_LOGI("INIT INFO BlurLevel %d rotateAngle %d TOF=%d ISO=%d Tp=(%d,%d)", m_blurLevel,
             JPEGOrientation, depthInitParams.userInterface.TOF, depthInitParams.userInterface.ISO,
             depthInitParams.userInterface.tp.x, depthInitParams.userInterface.tp.y);

    if (m_superNightEnabled != 0) {
        depthInitParams.isSingleCameraDepth = TRUE;
        if (g_IsSingleCameraDepth >= 0) {
            depthInitParams.isSingleCameraDepth = g_IsSingleCameraDepth ? TRUE : FALSE;
        }
    } else {
        depthInitParams.isSingleCameraDepth = FALSE;
    }
    Device->detector->Begin(m_detectorDepthRun, MARK("MialgoCaptureDepthInit"), 2000);
    MialgoCaptureDepthInit(&m_hCaptureDepth, depthInitParams);
    Device->detector->End(m_detectorDepthRun);
    DEV_IF_LOGE_GOTO((m_hCaptureDepth == NULL), exit, "MialgoCaptureDepthInit ERR %p",
                     m_hCaptureDepth);
    // do depth
    depthParams.depthOutput = &AlgoDepthImg_Depth;
    depthParams.depthOutput->plane[0] =
        (depthParams.depthOutput->plane[0] + sizeof(disparity_fileheader_t)); // 地址偏移预留头空间
    depthParams.validDepthSize = depthDataSize;
    Device->detector->Begin(m_detectorDepthRun, MARK("MialgoCaptureDepthProc"), 4500);
    ret = MialgoCaptureDepthProc(&m_hCaptureDepth, depthParams);
    Device->detector->End(m_detectorDepthRun);
    DEV_IF_LOGE_GOTO(((ret != RET_OK) || ((*depthDataSize) == 0)), exit,
                     "Depth MialgoCaptureDepthProc failed=%d", ret);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Depth MialgoCaptureDepthProc failed=%d", ret);
    header = (disparity_fileheader_t *)outputDepth->plane[0];
    memset(header, 0, sizeof(disparity_fileheader_t));
    header->i32Header = 0x80;
    header->i32HeaderLength = sizeof(disparity_fileheader_t);
    header->i32PointX = (int32_t)focusTP.x;
    header->i32PointY = (int32_t)focusTP.y;
    header->i32BlurLevel = (int32_t)m_blurLevel;
    header->i32DataLength = (int32_t)(*depthDataSize);
exit:
    if (m_hCaptureDepth != NULL) {
        MialgoCaptureDepthDeinit(&m_hCaptureDepth);
        if (ret != RET_OK) {
            if (m_hCaptureDepth != NULL) {
                MialgoCaptureDepthDestory(&m_hCaptureDepth);
                memset((void *)&m_depthlaunchParams, 0, sizeof(m_depthlaunchParams));
                m_hCaptureDepth = NULL;
            }
        }
    }
    return ret;
}
