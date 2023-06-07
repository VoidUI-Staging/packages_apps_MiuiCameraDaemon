#include "ArcsoftDualCamDepthPlugin.h"

#include <MiaPluginUtils.h>

#include "dualcam_mdbokeh_capture_proc.h"

using namespace mialgo2;

#undef LOG_TAG
#define LOG_TAG "ARCSOFTBOKEH_CAPTURE_DEPTH"

#undef LOG_GROUP
#define LOG_GROUP GROUP_PLUGIN

#define CAL_DATA_PATH_W_T "/data/vendor/camera/com.xiaomi.dcal.wt.data"
#define CAL_DATA_PATH_W_U "/data/vendor/camera/com.xiaomi.dcal.wu.data"

#define ROLE_REARBOKEH2X 61
#define ROLE_REARBOKEH1X 63

#define CALIBRATIONDATA_LEN 2048
#define ARCSOFT_MAXFOV      78.5

#define SAFE_FREE(ptr) \
    if (ptr) {         \
        free(ptr);     \
        ptr = NULL;    \
    }

#define FILTER_SHINE_THRES         2
#define FILTER_SHINE_LEVEL         70
#define FILTER_SHINE_LEVEL_REFOCUS 40
#define FILTER_SHINE_THRES_NIGHT   5
#define FILTER_SHINE_THRES_REFOCUS 5
#define FILTER_SHINE_LEVEL_NIGHT   30
#define FILTER_SHINE_THRES_OUTSIDE 2
#define FILTER_SHINE_LEVEL_OUTSIDE 20
#define FILTER_INTENSITY           70

#define ARCSOFT_BLURTABLE_1X 5 // other version: 1 and 4
#define ARCSOFT_BLURTABLE_2X 2

static const S32 g_bokehDump = property_get_int32("persist.vendor.camera.mialgo.capdepth.dump", 0);
static const S32 g_bDrawROI =
    property_get_int32("persist.vendor.camera.mialgo.capdepth.debugroi", 0);
static const S32 g_BokehLite =
    property_get_int32("persist.vendor.camera.mialgo.capdepth.debuglitemode", -1);
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

ArcsoftDualCamDepthPlugin::~ArcsoftDualCamDepthPlugin() {}

int ArcsoftDualCamDepthPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    Device->Init();
    m_hCaptureDepth = NULL;
    mCalibrationWU = NULL;
    mCalibrationWT = NULL;
    m_superNightEnabled = 0;
    m_ProcessCount = 0;
    m_NodeInterface = nodeInterface;
    m_pNodeMetaData = NULL;
    m_blurLevel = 0;
    m_LiteMode = 0;
    memset(&m_algoDepthSetParams, 0, sizeof(AlgoDepthSetParam));
    Device->detector->Create(&m_detectorRun, MARK("m_detectorRun"));
    Device->detector->Create(&m_detectorInit, MARK("m_detectorInit"));
    Device->detector->Create(&m_detectorReset, MARK("m_detectorReset"));
    Device->detector->Create(&m_detectorUninit, MARK("m_detectorUninit"));
    Device->detector->Create(&m_detectorCalcDisparityData, MARK("m_detectorCalcDisparityData"));
    Device->detector->Create(&m_detectorGetDisparityDataSize,
                             MARK("m_detectorGetDisparityDataSize"));
    Device->detector->Create(&m_detectorGetDisparityData, MARK("m_detectorGetDisparityData"));
    m_pNodeMetaData = new NodeMetaData();
    if (m_pNodeMetaData != NULL) {
        m_pNodeMetaData->Init(0, 0);
    }
    GetPackData(CAL_DATA_PATH_W_U, &mCalibrationWU, &m_CalibrationWULength);
    return InitDepth();
}

void ArcsoftDualCamDepthPlugin::destroy()
{
    m_flustStatus = FALSE;

    if (m_hCaptureDepth != NULL) {
        Device->detector->Begin(m_detectorUninit, MARK("ARC_DCIR_Uninit"), 5000);
        ARC_DCIR_Uninit(&m_hCaptureDepth);
        Device->detector->End(m_detectorUninit);
        m_hCaptureDepth = NULL;
        memset(&m_algoDepthSetParams, 0, sizeof(AlgoDepthSetParam));
    }

    FreePackData(&mCalibrationWU);
    // FreePackData(&mCalibrationWT);
    if (m_pNodeMetaData) {
        delete m_pNodeMetaData;
        m_pNodeMetaData = NULL;
    }
    Device->detector->Destroy(&m_detectorRun);
    Device->detector->Destroy(&m_detectorInit);
    Device->detector->Destroy(&m_detectorReset);
    Device->detector->Destroy(&m_detectorUninit);
    Device->detector->Destroy(&m_detectorCalcDisparityData);
    Device->detector->Destroy(&m_detectorGetDisparityDataSize);
    Device->detector->Destroy(&m_detectorGetDisparityData);
    Device->Deinit();
}

S64 ArcsoftDualCamDepthPlugin::GetPackData(char *fileName, void **ppData0, U64 *pSize0)
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
    DEV_IF_LOGE_RETURN_RET((ret != packDataSize), RET_ERR, "read file err %" PRIi64, ret);
    DEV_IF_LOGI((ret == packDataSize), "INFO CROSSTALK FILE[%s][%" PRIi64 "]", packDataFileName,
                packDataSize);
    *pSize0 = packDataSize;
    *ppData0 = packDataBuf;
    return RET_OK;
}

S64 ArcsoftDualCamDepthPlugin::FreePackData(void **ppData0)
{
    if (ppData0 != NULL) {
        if (*ppData0) {
            Device->memoryPool->Free(*ppData0);
            *ppData0 = NULL;
        }
    }
    return RET_OK;
}

ProcessRetStatus ArcsoftDualCamDepthPlugin::processRequest(
    ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus ArcsoftDualCamDepthPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
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

    ret = m_pNodeMetaData->GetSuperNightEnabled(m_plogicalMeta, &m_superNightEnabled);
    if (ret != RET_OK) {
        m_superNightEnabled = 0;
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

    auto &outBuffers = pProcessRequestInfo->outputBuffersMap.at(0);
    ret = Device->image->MatchToImage((void *)&outBuffers[0], &outputDepth);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), (ProcessRetStatus)ret, "Match buffer err");

    DEV_LOGI("output main width: %d, height: %d, stride: %d, scanline: %d, format: %d",
             outputDepth.width, outputDepth.height, outputDepth.stride[0],
             outputDepth.sliceHeight[0], outputDepth.format);

    Device->detector->Begin(m_detectorRun, MARK("BokehPlugin"), 10000);
    ret = InitDepth();
    DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "InitDepth failed=%d", ret);
    ret = ProcessDepth(&inputMain, &inputAux, &outputDepth);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "ProcessDepth failed=%d", ret);
    runTime = Device->detector->End(m_detectorRun);

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
    if (ret != RET_OK) {
        DEV_LOGE("ProcessBuffer MEMCOPY =%d", ret);
        Device->image->Copy(&outputDepth, &inputMain);
    }
    if (m_NodeInterface.pSetResultMetadata != NULL) { // For exif
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf),
                 "arcsoftDualDepth:{processCount:%d blurLevel:%d "
                 "seBokeh:%d liteMode:%d costTime:%d ret:%d}",
                 m_ProcessCount, m_blurLevel, m_superNightEnabled, m_LiteMode, runTime, ret);
        std::string results(buf);
        m_NodeInterface.pSetResultMetadata(m_NodeInterface.owner, pProcessRequestInfo->frameNum,
                                           pProcessRequestInfo->timeStamp, results);
    }
    m_ProcessCount++;
    return (ProcessRetStatus)RET_OK;
}

S32 ArcsoftDualCamDepthPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

S32 ArcsoftDualCamDepthPlugin::InitDepth(void)
{
    S32 ret = RET_OK;
    DEV_IF_LOGI_RETURN_RET((m_hCaptureDepth != NULL), RET_OK, "depthHandle is ready");
    ret = ARC_DCIR_SetLogLevel(g_ArcLogLevel);
    DEV_IF_LOGI((ret == RET_OK), "arcsoft lib loglevel is %d", g_ArcLogLevel);
    const MPBASE_Version *arcVersion = NULL;
    arcVersion = ARC_DCIR_GetVersion();
    DEV_IF_LOGI((arcVersion != NULL), "Arcsoft depth Algo Lib Version: %s", arcVersion->Version);
    Device->detector->Begin(m_detectorInit, MARK("ARC_DCIR_Init"), 5000);
    ret = ARC_DCIR_Init(&m_hCaptureDepth, ARC_DCIR_NORMAL_MODE);
    Device->detector->End(m_detectorInit);
    DEV_IF_LOGE_GOTO((ret != RET_OK || m_hCaptureDepth == NULL), exit,
                     "ARC_DCIR_Init Faild ret = %d m_hCaptureDepth %p", ret, m_hCaptureDepth);
exit:
    if (ret != RET_OK) {
        if (m_hCaptureDepth != NULL) {
            Device->detector->Begin(m_detectorUninit, MARK("ARC_DCIR_Uninit"), 5000);
            ARC_DCIR_Uninit(&m_hCaptureDepth);
            Device->detector->End(m_detectorUninit);
            m_hCaptureDepth = NULL;
            memset(&m_algoDepthSetParams, 0, sizeof(AlgoDepthSetParam));
        }
    }
    return ret;
}

S32 ArcsoftDualCamDepthPlugin::ProcessDepth(DEV_IMAGE_BUF *inputMain, DEV_IMAGE_BUF *inputAaux,
                                            DEV_IMAGE_BUF *outputDepth)
{
    DEV_IF_LOGE_RETURN_RET(inputMain == NULL, RET_ERR, "ARG ERR");
    DEV_IF_LOGE_RETURN_RET(inputAaux == NULL, RET_ERR, "ARG ERR");
    DEV_IF_LOGE_RETURN_RET(m_hCaptureDepth == NULL, RET_ERR, "ARG ERR");
    S32 ret = RET_OK;
    uint8_t *pDisparityData = NULL;
    S32 disparitySize = 0;
    ARC_DCIR_METADATA metaData = {0};
    U32 orient = 0;
    disparity_fileheader_t *header = NULL;
    ASVLOFFSCREEN AlgoDepthImg_Aux = {0};
    ASVLOFFSCREEN AlgoDepthImg_Main = {0};
    ASVLOFFSCREEN AlgoDepthImg_Depth = {0};
    S32 mappingVersion = ARCSOFT_BLURTABLE_2X;
    ARC_DCIR_PARAM faceInfo = {0};
    DEV_IMAGE_POINT focusTP = {0};
    FP32 fNumber = 0;
    ARC_REFOCUSCAMERAIMAGE_PARAM imageInfo = {0};
    CHIRECT tempChiRect;
    AlgoDepthSetParam algoDepthSetParams = {0};
    S32 captureMode = 0;
    S32 superNightLevel = 0;

    ret |= PrepareYUV_OFFSCREEN(inputMain, &AlgoDepthImg_Main);
    ret |= PrepareYUV_OFFSCREEN(inputAaux, &AlgoDepthImg_Aux);
    ret |= PrepareYUV_OFFSCREEN(outputDepth, &AlgoDepthImg_Depth);
    DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "PrepareYUV_OFFSCREEN=%d", ret);

    ret = m_pNodeMetaData->GetBokehSensorConfig(m_plogicalMeta,
                                                &algoDepthSetParams.bokehSensorConfig);
    DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "GetBokehSensorConfig err =%d", ret);
    if (algoDepthSetParams.bokehSensorConfig.sensorCombination == BOKEH_COMBINATION_T_W) {
        algoDepthSetParams.pCaliData.pCalibData = (unsigned char *)mCalibrationWT;
        algoDepthSetParams.pCaliData.i32CalibDataSize = CALIBRATIONDATA_LEN;
    } else {
        algoDepthSetParams.pCaliData.pCalibData = (unsigned char *)mCalibrationWU;
        algoDepthSetParams.pCaliData.i32CalibDataSize = CALIBRATIONDATA_LEN;
    }
    if (memcmp(&m_algoDepthSetParams, &algoDepthSetParams, sizeof(m_algoDepthSetParams)) != 0) {
        memcpy(&m_algoDepthSetParams, &algoDepthSetParams, sizeof(m_algoDepthSetParams));
        ret = ARC_DCIR_SetCaliData(m_hCaptureDepth, &m_algoDepthSetParams.pCaliData);
        DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "ARC_DCIR_SetCaliData failed is %d", ret);
    }

    Device->detector->Begin(m_detectorReset, MARK("ARC_DCIR_Reset"), 2000);
    ret = ARC_DCIR_Reset(m_hCaptureDepth);
    Device->detector->End(m_detectorReset);
    DEV_IF_LOGE_GOTO((ret != RET_OK || m_hCaptureDepth == NULL), exit,
                     "ARC_DCIR_Reset failed is %d m_hCaptureDepth %p", ret, m_hCaptureDepth);

    if (m_superNightEnabled == 1) {
        captureMode = ARC_DCIR_NIGHT_MODE;
        m_pNodeMetaData->GetExtremeDarkSeResult(m_plogicalMeta, &superNightLevel);
        if (0 == superNightLevel) {
            superNightLevel = 1;
        } else {
            superNightLevel = 2;
        }
    } else {
        captureMode = ARC_DCIR_NORMAL_MODE;
    }
    ret = ARC_DCIR_SetCaptureMode(m_hCaptureDepth, captureMode);
    DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "ARC_DCIR_SetCaptureMode failed is %d", ret);
    if (captureMode == ARC_DCIR_NIGHT_MODE) {
        ret = ARC_DCIR_SetNightLevel(m_hCaptureDepth, superNightLevel);
        DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "ARC_DCIR_SetNightLevel failed is %d", ret);
    }
    DEV_LOGI("superNightEnabled %d supernightLevel %d", m_superNightEnabled, superNightLevel);
    ret = m_pNodeMetaData->GetCropRegion(m_pPhysicalMeta, &tempChiRect);
    DEV_IF_LOGI(ret != RET_OK, "GetCropRegion is %d", ret);
    imageInfo.i32MainWidth_CropNoScale = tempChiRect.width;
    imageInfo.i32MainHeight_CropNoScale = tempChiRect.width * inputMain->height / inputMain->width;
    imageInfo.i32AuxWidth_CropNoScale = inputAaux->width;
    imageInfo.i32AuxHeight_CropNoScale = inputAaux->height;
    imageInfo.i32ImageDegree = 0;
    DEV_LOGI("cropregion:mainCropsize[%d %d], auxCropsize[%d %d]",
             imageInfo.i32MainWidth_CropNoScale, imageInfo.i32MainHeight_CropNoScale,
             imageInfo.i32AuxWidth_CropNoScale, imageInfo.i32AuxHeight_CropNoScale);
    ret = ARC_DCIR_SetCameraImageInfo(m_hCaptureDepth, &imageInfo);
    DEV_IF_LOGE(ret != RET_OK, "ARC_DCIR_SetCameraImageInfo failed is %d", ret);

    ret = m_pNodeMetaData->GetRotateAngle(m_plogicalMeta, &orient);
    DEV_IF_LOGE(ret != RET_OK, "GetRotateAngle err is %d", ret);
    DEV_LOGI("GetRotateAngle is %d", ret);
    switch (orient) {
    case 0:
        metaData.i32CameraRoll = ARC_DCIR_REFOCUS_CAMERA_ROLL_0;
        break;
    case 90:
        metaData.i32CameraRoll = ARC_DCIR_REFOCUS_CAMERA_ROLL_90;
        break;
    case 180:
        metaData.i32CameraRoll = ARC_DCIR_REFOCUS_CAMERA_ROLL_180;
        break;
    case 270:
        metaData.i32CameraRoll = ARC_DCIR_REFOCUS_CAMERA_ROLL_270;
        break;
    }
    metaData.i32CalMode = m_LiteMode;

    faceInfo.faceParam.pi32FaceAngles =
        (MInt32 *)Device->memoryPool->Malloc(sizeof(MInt32) * MAX_FACE_NUM, MARK("pi32FaceAngles"));
    faceInfo.faceParam.prtFaces =
        (PMRECT)Device->memoryPool->Malloc(sizeof(MRECT) * MAX_FACE_NUM, MARK("prtFaces"));
    DEV_IF_LOGE_GOTO(
        ((faceInfo.faceParam.pi32FaceAngles == NULL) || (faceInfo.faceParam.prtFaces == NULL)),
        exit, "malloc faceInfo memory failed!");
    faceInfo.fMaxFOV = ARCSOFT_MAXFOV;
    ret = m_pNodeMetaData->GetArcsoftFaceInfo(m_pPhysicalMeta, inputMain, faceInfo.faceParam);
    DEV_IF_LOGW(ret != RET_OK, "GetArcsoftFaceInfo err is %d", ret);
    for (int i = 0; i < faceInfo.faceParam.i32FacesNum; ++i) {
        DEV_LOGI("ArcsoftFaceInfo [%d/%d] [left top right bottom] is [%d %d %d %d]", i + 1,
                 faceInfo.faceParam.i32FacesNum, faceInfo.faceParam.prtFaces[i].left,
                 faceInfo.faceParam.prtFaces[i].top, faceInfo.faceParam.prtFaces[i].right,
                 faceInfo.faceParam.prtFaces[i].top);
    }
    Device->detector->Begin(m_detectorCalcDisparityData, MARK("ARC_DCIR_CalcDisparityData"), 7000);
    ret = ARC_DCIR_CalcDisparityData(m_hCaptureDepth, &AlgoDepthImg_Main, &AlgoDepthImg_Aux,
                                     &faceInfo, &metaData);
    Device->detector->End(m_detectorCalcDisparityData);
    DEV_IF_LOGW_GOTO((ret == 5), exit, "ARC_DCIR_CalcDisparityData failed=%d", ret);
    DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "ARC_DCIR_CalcDisparityData failed=%d", ret);
    DEV_LOGI("ARC_DCIR_CalcDisparityData is %d", ret);
    Device->detector->Begin(m_detectorGetDisparityDataSize, MARK("ARC_DCIR_GetDisparityDataSize"),
                            2000);
    ret = ARC_DCIR_GetDisparityDataSize(m_hCaptureDepth, &disparitySize);
    Device->detector->End(m_detectorGetDisparityDataSize);
    DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "ARC_DCIR_GetDisparityDataSize failed=%d", ret);
    DEV_LOGI("DisparityDataSize is %d", disparitySize);

    header = (disparity_fileheader_t *)AlgoDepthImg_Depth.ppu8Plane[0];
    memset(header, 0, sizeof(disparity_fileheader_t));
    header->i32Header = 0x80;
    header->i32HeaderLength = sizeof(disparity_fileheader_t);

    ret = m_pNodeMetaData->GetfocusTP(m_pPhysicalMeta, inputMain, &focusTP);
    DEV_IF_LOGE(ret != RET_OK, "GetfocusTP err is %d", ret);
    header->i32PointX = (int32_t)focusTP.x;
    header->i32PointY = (int32_t)focusTP.y;

    ret = m_pNodeMetaData->GetFNumberApplied(m_plogicalMeta, &fNumber);
    DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "GetFNumberApplied err is %d", ret);
    DEV_LOGI("GetFNumberApplied fNumber is %f", fNumber);
    m_blurLevel = findBlurLevelByFNumber(fNumber, false);

    header->i32BlurLevel = (int32_t)m_blurLevel;
    header->i32DataLength = (int32_t)disparitySize;
    header->i32Version = (int32_t)3;
    header->i32VendorID = (int32_t)1;
    if (m_algoDepthSetParams.bokehSensorConfig.sensorCombination == BOKEH_COMBINATION_T_W) {
        mappingVersion = ARCSOFT_BLURTABLE_2X;
    } else {
        mappingVersion = ARCSOFT_BLURTABLE_1X;
    }
    header->mappingVersion = mappingVersion;
    header->ShineThres = FILTER_SHINE_THRES_REFOCUS;
    header->ShineLevel = FILTER_SHINE_LEVEL_REFOCUS;
    pDisparityData = AlgoDepthImg_Depth.ppu8Plane[0] + sizeof(disparity_fileheader_t);
    Device->detector->Begin(m_detectorGetDisparityData, MARK("ARC_DCIR_GetDisparityData"), 2000);
    ret = ARC_DCIR_GetDisparityData(m_hCaptureDepth, pDisparityData);
    Device->detector->End(m_detectorGetDisparityData);
    DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "ARC_DCIR_GetDisparityData failed=%d", ret);
exit:
    if (ret != RET_OK) {
        if (m_hCaptureDepth != NULL) {
            Device->detector->Begin(m_detectorUninit, MARK("ARC_DCIR_Uninit"), 5000);
            ARC_DCIR_Uninit(&m_hCaptureDepth);
            Device->detector->End(m_detectorUninit);
            m_hCaptureDepth = NULL;
            memset(&m_algoDepthSetParams, 0, sizeof(AlgoDepthSetParam));
        }
    }
    if (faceInfo.faceParam.pi32FaceAngles) {
        Device->memoryPool->Free(faceInfo.faceParam.pi32FaceAngles);
        faceInfo.faceParam.pi32FaceAngles = NULL;
    }
    if (faceInfo.faceParam.prtFaces) {
        Device->memoryPool->Free(faceInfo.faceParam.prtFaces);
        faceInfo.faceParam.prtFaces = NULL;
    }
    return ret;
}

S32 ArcsoftDualCamDepthPlugin::PrepareYUV_OFFSCREEN(DEV_IMAGE_BUF *image, ASVLOFFSCREEN *stImage)
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
    return RET_OK;
}
