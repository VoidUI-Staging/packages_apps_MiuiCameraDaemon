#include "ArcsoftDualCamHdrPlugin.h"

#include <MiaPluginUtils.h>
#include "chioemvendortag.h"
using namespace mialgo2;

#undef LOG_TAG
#define LOG_TAG "ARCSOFTBOKEH_CAPTURE_HDR"

#undef LOG_GROUP
#define LOG_GROUP GROUP_PLUGIN

#define MI_MAX_EV_NUM 12

#define SHOW_AUX_ONLY  1
#define SHOW_MAIN_ONLY 2
#define SHOW_HALF_HALF 3

#define SAFE_FREE(ptr) \
    if (ptr) {         \
        free(ptr);     \
        ptr = NULL;    \
    }

static S64 m_mutexHdr_Id;

static const S32 g_bokehDump = property_get_int32("persist.vendor.camera.mialgo.caphdr.dump", 0);
static const S32 g_nViewCtrl =
    property_get_int32("persist.vendor.camera.mialgo.caphdr.debugview", 0);
static const S32 g_bDrawROI = property_get_int32("persist.vendor.camera.mialgo.caphdr.debugroi", 0);
static const int32_t g_arcBhdrLoglevel =
    property_get_int32("persist.vendor.camera.arcsoft.cb.loglevel", 0);
static const S32 g_i32ToneLength = property_get_int32("persist.camera.hdrbokeh.i32ToneLength", -1);
static const S32 g_i32Brightness = property_get_int32("persist.camera.hdrbokeh.i32Brightness", -1);
static const S32 g_i32Saturation = property_get_int32("persist.camera.hdrbokeh.i32Saturation", -1);
static const S32 g_i32Contrast = property_get_int32("persist.camera.hdrbokeh.i32Contrast", -1);
#define ARC_BHDR_VendorTag_bypass "persist.vendor.camera.arcsoft.bhdr.bypass"
#define ARC_BHDR_VendorTag_dump   "persist.vendor.camera.arcsoft.bhdr.dump"

ArcsoftDualCamHdrPlugin::~ArcsoftDualCamHdrPlugin() {}

int ArcsoftDualCamHdrPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    Device->Init();
    m_ProcessCount = 0;
    m_NodeInterface = nodeInterface;
    m_pNodeMetaData = NULL;
    Device->mutex->Create(&m_mutexHdr_Id, MARK("m_mutexHdr_Id"));
    Device->detector->Create(&m_detectorInit, MARK("m_detectorInit"));
    Device->detector->Create(&m_detectorRun, MARK("m_detectorRun"));
    Device->detector->Create(&m_detectorHdrRun, MARK("m_detectorHdrRun"));
    Device->detector->Create(&m_detectorPreProc, MARK("m_detectorPreProc"));
    Device->detector->Create(&m_detectorGetCropSize, MARK("m_detectorGetCropSize"));
    Device->detector->Create(&m_detectorUninit, MARK("m_detectorUninit"));
    m_pNodeMetaData = new NodeMetaData();
    if (m_pNodeMetaData != NULL) {
        m_pNodeMetaData->Init(0, 0);
    }
    iniEngine();
    return RET_OK;
}

void ArcsoftDualCamHdrPlugin::iniEngine()
{
    ARC_BHDR_SetLogLevel(g_arcBhdrLoglevel);
    DEV_LOGI("arcsoft bokeh hdr sdk log level is %d", g_arcBhdrLoglevel);
    const MPBASE_Version *version = ARC_BHDR_GetVersion();
    DEV_LOGI("Arcsoft Bokeh HDR Algo Lib Version: %s ", version->Version);
}

void ArcsoftDualCamHdrPlugin::destroy()
{
    m_flustStatus = FALSE;
    if (m_pNodeMetaData) {
        delete m_pNodeMetaData;
        m_pNodeMetaData = NULL;
    }
    Device->detector->Destroy(&m_detectorRun);
    Device->detector->Destroy(&m_detectorHdrRun);
    Device->detector->Destroy(&m_detectorInit);
    Device->detector->Destroy(&m_detectorPreProc);
    Device->detector->Destroy(&m_detectorGetCropSize);
    Device->detector->Destroy(&m_detectorUninit);
    //    Device->mutex->Destroy(&m_mutexHdr_Id);
    Device->Deinit();
}

bool ArcsoftDualCamHdrPlugin::isEnabled(MiaParams settings)
{
    S32 hdrEnabled = 0;
    S32 ret = m_pNodeMetaData->GetHdrEnabled(settings.metadata, &hdrEnabled);
    if (ret != RET_OK) {
        hdrEnabled = FALSE;
    }
    return hdrEnabled;
}

ProcessRetStatus ArcsoftDualCamHdrPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus ArcsoftDualCamHdrPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    DEV_IF_LOGE_RETURN_RET(pProcessRequestInfo == NULL, (ProcessRetStatus)RET_ERR, "arg err");
    S32 ret = RET_OK;
    auto &inputBuffers = pProcessRequestInfo->inputBuffersMap.begin()->second;
    m_plogicalMeta = inputBuffers[0].pMetadata;
    m_pPhysicalMeta = inputBuffers[0].pPhyCamMetadata;
    m_outputMetadata = pProcessRequestInfo->outputBuffersMap.begin()->second[0].pMetadata;
    U64 runTime = 0;
    std::vector<camera_metadata_t *> logicalCamMetaVector;
    std::vector<camera_metadata_t *> mainPhyCamMetaMetaVector;

    std::vector<DEV_IMAGE_BUF> inputMainVector;
    DEV_IMAGE_BUF outputMain = {0};

    auto dumpImg = [&](camera_metadata_t *pLogMeta, DEV_IMAGE_BUF &img, const char *pName,
                       int Idx) {
        S32 expValue = 0;
        m_pNodeMetaData->GetExpValue(pLogMeta, &expValue);
        DEV_LOGI("get image %s EV=%d %dx%d,stride=%dx%d,fmt=%d", pName, expValue, img.width,
                 img.height, img.stride[0], img.sliceHeight[0], img.format);
        if (g_bokehDump) {
            char fileName[128];
            snprintf(fileName, sizeof(fileName), "bokeh_capture_hdr_%s_%d_EV%d", pName, Idx,
                     expValue);
            Device->image->DumpImage(&img, fileName, (U32)pProcessRequestInfo->frameNum, ".NV12");
        }
    };

    for (auto &imageParams : pProcessRequestInfo->inputBuffersMap) {
        std::vector<ImageParams> &inputs = imageParams.second;
        U32 portId = imageParams.first;
        for (auto &input : inputs) {
            if (portId == 0) {
                DEV_IMAGE_BUF inputMain = {0};
                ret = Device->image->MatchToImage((void *)&input, &inputMain);
                DEV_IF_LOGE_RETURN_RET((ret != RET_OK), (ProcessRetStatus)ret, "Match buffer err");
                inputMainVector.push_back(inputMain);
                logicalCamMetaVector.push_back(input.pMetadata);
                mainPhyCamMetaMetaVector.push_back(input.pPhyCamMetadata);
                dumpImg(input.pMetadata, inputMain, "inputMain", inputMainVector.size());
            }
        }
    }
    DEV_IF_LOGE_RETURN_RET((inputMainVector.size() <= 0), (ProcessRetStatus)RET_ERR,
                           "invalid param");

    auto &outBuffers = pProcessRequestInfo->outputBuffersMap.at(0);
    ret = Device->image->MatchToImage((void *)&outBuffers[0], &outputMain);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), (ProcessRetStatus)ret, "Match buffer err");

    DEV_LOGI("output main width: %d, height: %d, stride: %d, scanline: %d, format: %d",
             outputMain.width, outputMain.height, outputMain.stride[0], outputMain.sliceHeight[0],
             outputMain.format);

    S32 hdrEnabled = 0;
    m_pNodeMetaData->GetHdrEnabled(m_plogicalMeta, &hdrEnabled);
    DEV_LOGI("hdrEnabled %d", hdrEnabled);

    switch (g_nViewCtrl) {
    case SHOW_MAIN_ONLY:
        Device->image->Copy(&outputMain, &inputMainVector[0]);
        break;
    default:
        Device->detector->Begin(m_detectorRun, MARK("ProcessHdr"), 10000);
        if (hdrEnabled == TRUE) {
            Device->mutex->Lock(m_mutexHdr_Id);
            ret = ProcessHdr(inputMainVector, &outputMain, logicalCamMetaVector);
            Device->mutex->Unlock(m_mutexHdr_Id);
            DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Bokeh ProcessHdr failed=%d", ret);
        }
        runTime = Device->detector->End(m_detectorRun);
        break;
    }

    if (g_bDrawROI) {
        DEV_IMAGE_RECT focusROI = {0};
        DEV_IMAGE_POINT focusTP = {0};
        m_pNodeMetaData->GetfocusROI(m_pPhysicalMeta, &inputMainVector[0], &focusROI);
        m_pNodeMetaData->GetfocusTP(m_pPhysicalMeta, &inputMainVector[0], &focusTP);
        U32 rotateAngle = 0;
        m_pNodeMetaData->GetRotateAngle(logicalCamMetaVector[0], &rotateAngle);
        ARC_BHDR_FACEINFO face_info = {0};
        m_pNodeMetaData->GetFaceRect(m_pPhysicalMeta, &inputMainVector[0], rotateAngle, face_info);
        for (S8 i = 0; i < face_info.nFace; i++) {
            DEV_IMAGE_RECT faceRect = {
                .left = (U32)face_info.rcFace[i].left,
                .top = (U32)face_info.rcFace[i].top,
                .width = (U32)(face_info.rcFace[i].right - face_info.rcFace[i].left),
                .height = (U32)(face_info.rcFace[i].bottom - face_info.rcFace[i].top)};
            Device->image->DrawRect(&outputMain, faceRect, 2, 50);
        }
        Device->image->DrawPoint(&outputMain, focusTP, 10, 0);
        Device->image->DrawRect(&outputMain, focusROI, 5, 5);
    }

    if (g_bokehDump) {
        Device->image->DumpImage(&outputMain, "bokeh_capture_hdr_output",
                                 (U32)pProcessRequestInfo->frameNum, ".NV12");
    }
exit:
    if (ret != RET_OK) {
        DEV_LOGE("ProcessBuffer MEMCOPY =%d", ret);
        Device->image->Copy(&outputMain, &inputMainVector[0]);
    }
    if (m_NodeInterface.pSetResultMetadata != NULL) { // For exif
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf),
                 "arcsoftDualHdr:{processCount:%d hdrEnabled:%d "
                 "costTime:%d ret:%d}",
                 m_ProcessCount, hdrEnabled, runTime, ret);
        std::string results(buf);
        m_NodeInterface.pSetResultMetadata(m_NodeInterface.owner, pProcessRequestInfo->frameNum,
                                           pProcessRequestInfo->timeStamp, results);
    }
    m_ProcessCount++;
    return (ProcessRetStatus)RET_OK;
}

S32 ArcsoftDualCamHdrPlugin::PrepareToHDR_YUV_OFFSCREEN(DEV_IMAGE_BUF *image,
                                                        ASVLOFFSCREEN *stImage)
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

S32 ArcsoftDualCamHdrPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

S32 ArcsoftDualCamHdrPlugin::ProcessHdr(
    std::vector<DEV_IMAGE_BUF> inputMainVector, DEV_IMAGE_BUF *outputImage,
    const std::vector<camera_metadata_t *> &logicalCamMetaVector)
{
    S64 ret = RET_OK;
    int input_num = ARC_BHDR_MAX_INPUT_IMAGE_NUM; // hard code, MDbokeh has 4 frames.
    DEV_IF_LOGE_RETURN_RET((inputMainVector.size() < input_num), ret, "input_num ERR =%d",
                           input_num);
    ARC_BHDR_PARAM Param = {0};
    ARC_BHDR_INPUTINFO InputImages = {0};
    HDR_EVLIST_AEC_INFO EVAEInfo = {0};
    ASVLOFFSCREEN DstImg = {0};
    U32 rotateAngle = 0;
    ret = m_pNodeMetaData->GetRotateAngle(logicalCamMetaVector[0], &rotateAngle);
    ARC_BHDR_FACEINFO face_info;
    memset(&face_info, 0, sizeof(ARC_BHDR_FACEINFO));
    face_info.lFaceOrient =
        (MInt32 *)Device->memoryPool->Malloc(sizeof(MInt32) * MAX_FACE_NUM, MARK("lFaceOrient"));
    face_info.rcFace =
        (MRECT *)Device->memoryPool->Malloc(sizeof(MRECT) * MAX_FACE_NUM, MARK("rcFace"));
    m_pNodeMetaData->GetFaceRect(m_pPhysicalMeta, &inputMainVector[0], rotateAngle, face_info);
    MHandle phExpEffect = NULL;
    S32 CameraMode = ARC_BHDR_MODE_WIDE_CAMERA;
    Device->detector->Begin(m_detectorInit, MARK("ARC_BHDR_Init"), 5000);
    ret = ARC_BHDR_Init(&phExpEffect, CameraMode);
    Device->detector->End(m_detectorInit);
    DEV_IF_LOGE_GOTO((ret != RET_OK) || (phExpEffect == NULL), exit, "ARC_BHDR_Init=%d", ret);
    ret = ARC_BHDR_SetSceneType(phExpEffect, 0);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "ARC_BHDR_SetSceneType=%d", ret);
    ret = ARC_BHDR_GetDefaultParam(phExpEffect, &Param);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "ARC_BHDR_GetDefaultParam=%d", ret);

    Param.i32ToneLength = g_i32ToneLength >= 0 ? g_i32ToneLength : 20;
    Param.i32Brightness = g_i32Brightness >= 0 ? g_i32Brightness : 0;
    Param.i32Saturation = g_i32Saturation >= 0 ? g_i32Saturation : 0;
    Param.i32Contrast = g_i32Contrast >= 0 ? g_i32Contrast : 0;
    DEV_LOGI("i32ToneLength:%d i32Brightness:%d i32Saturation:%d i32Contrast:%d",
             Param.i32ToneLength, Param.i32Brightness, Param.i32Saturation, Param.i32Contrast);

    InputImages.i32ImgNum = input_num;
    for (int i = 0; i < input_num; i++) {
        ret = PrepareToHDR_YUV_OFFSCREEN(&inputMainVector[i], &InputImages.InputImages[i]);
        DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "PrepareToHDR_YUV_OFFSCREEN=%d", ret);
        Device->detector->Begin(m_detectorPreProc, MARK("ARC_BHDR_PreProcess"), 3000);
        ret = ARC_BHDR_PreProcess(phExpEffect, &InputImages, i);
        Device->detector->End(m_detectorPreProc);
        DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "ARC_BHDR_PreProcess=%d", ret);
    }
    ret = PrepareToHDR_YUV_OFFSCREEN(outputImage, &DstImg);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "PrepareToHDR_YUV_OFFSCREEN=%d", ret);

    Device->detector->Begin(m_detectorHdrRun, MARK("ARC_BHDR_Process"), 5000);
    ret = ARC_BHDR_Process(phExpEffect, &Param, &face_info, &DstImg);
    Device->detector->End(m_detectorHdrRun);
    DEV_IF_LOGE_GOTO(
        (ret != RET_OK && ret != HDR_ERR_CONTAIN_GHOST && ret != HDR_ERR_GHOST_IN_OVERREGION), exit,
        "ARC_BHDR_Process=%d", ret);
    MRECT cropsize;
    Device->detector->Begin(m_detectorGetCropSize, MARK("ARC_BHDR_GetCropSize"), 2000);
    ret = ARC_BHDR_GetCropSize(phExpEffect, &cropsize);
    Device->detector->End(m_detectorGetCropSize);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "ARC_BHDR_GetCropSize=%d", ret);
    m_pNodeMetaData->SetMetaData(m_outputMetadata, "xiaomi.bokeh.hdrCropsize", 0, &cropsize,
                                 sizeof(MRECT));
    DEV_LOGI("arcsoft hdr cropsize is [%d %d %d %d]", cropsize.top, cropsize.left, cropsize.bottom,
             cropsize.right);
exit:
    if (phExpEffect != NULL) {
        Device->detector->Begin(m_detectorUninit, MARK("ARC_BHDR_Uninit"), 3000);
        ARC_BHDR_Uninit(&phExpEffect);
        Device->detector->End(m_detectorUninit);
        phExpEffect = NULL;
    }
    if (face_info.lFaceOrient != NULL) {
        Device->memoryPool->Free(face_info.lFaceOrient);
        face_info.lFaceOrient = NULL;
    }
    if (face_info.rcFace != NULL) {
        Device->memoryPool->Free(face_info.rcFace);
        face_info.rcFace = NULL;
    }
    return ret;
}
