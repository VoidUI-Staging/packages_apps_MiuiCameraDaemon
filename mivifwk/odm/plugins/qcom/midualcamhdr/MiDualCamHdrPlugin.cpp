#include "MiDualCamHdrPlugin.h"

#include <MiaPluginUtils.h>
#include "chioemvendortag.h"
using namespace mialgo2;

#undef LOG_TAG
#define LOG_TAG "BOKEH_CAPTURE_HDR"

#undef LOG_GROUP
#define LOG_GROUP GROUP_PLUGIN

#define MI_MAX_EV_NUM 12

#define SHOW_AUX_ONLY  1
#define SHOW_MAIN_ONLY 2
#define SHOW_HALF_HALF 3

static S64 m_mutexHdr_Id;

static const S32 g_bokehDump = property_get_int32("persist.vendor.camera.mialgo.caphdr.dump", 0);
static const S32 g_nViewCtrl =
    property_get_int32("persist.vendor.camera.mialgo.caphdr.debugview", 0);
static const S32 g_bDrawROI = property_get_int32("persist.vendor.camera.mialgo.caphdr.debugroi", 0);

typedef struct
{
    int tonelen;
    int brightness;
    int saturation;
    int contrast;
    int ev[MI_MAX_EV_NUM];
} MiHDRParam;

static MiHDRParam MiHDRParams[] = {
    //(tonelen, bright, saturation, contrast, r_ev0, r_ev-, r_ev+)
    // REAR camera
    {20, 0, 5, 50, {0, -10, 6}},
    // FRONT camera
    {1, 0, 0, 0, {0, -12, 0}},
};

enum {
    HDR_PARAM_INDEX_REAR,
    HDR_PARAM_INDEX_FRONT,
};

MiDualCamHdrPlugin::~MiDualCamHdrPlugin() {}

int MiDualCamHdrPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    Device->Init();
    m_ProcessCount = 0;
    m_NodeInterface = nodeInterface;
    m_pNodeMetaData = NULL;
    Device->mutex->Create(&m_mutexHdr_Id, MARK("m_mutexHdr_Id"));
    Device->detector->Create(&m_detectorRun, MARK("m_detectorRun"));
    Device->detector->Create(&m_detectorHdrRun, MARK("m_detectorHdrRun"));
    m_pNodeMetaData = new NodeMetaData();
    if (m_pNodeMetaData != NULL) {
        m_pNodeMetaData->Init(0, 0);
    }
    return RET_OK;
}

void MiDualCamHdrPlugin::destroy()
{
    m_flustStatus = FALSE;
    if (m_pNodeMetaData) {
        delete m_pNodeMetaData;
        m_pNodeMetaData = NULL;
    }
    Device->detector->Destroy(&m_detectorRun);
    Device->detector->Destroy(&m_detectorHdrRun);
    //    Device->mutex->Destroy(&m_mutexHdr_Id);
    Device->Deinit();
}

bool MiDualCamHdrPlugin::isEnabled(MiaParams settings)
{
    S32 hdrEnabled = 0;
    S32 ret = m_pNodeMetaData->GetHdrEnabled(settings.metadata, &hdrEnabled);
    if (ret != RET_OK) {
        hdrEnabled = FALSE;
    }
    DEV_LOGI("enable %d", hdrEnabled);
    return hdrEnabled;
}

ProcessRetStatus MiDualCamHdrPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus MiDualCamHdrPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    DEV_IF_LOGE_RETURN_RET(pProcessRequestInfo == NULL, (ProcessRetStatus)RET_ERR, "arg err");
    S32 ret = RET_OK;
    auto &inputBuffers = pProcessRequestInfo->inputBuffersMap.begin()->second;
    m_plogicalMeta = inputBuffers[0].pMetadata;
    m_pPhysicalMeta = inputBuffers[0].pPhyCamMetadata;
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

    S32 hdrEnabled = 0;
    m_pNodeMetaData->GetHdrEnabled(m_plogicalMeta, &hdrEnabled);
    DEV_IF_LOGE((FALSE == hdrEnabled), "hdrEnabled is false!");

    auto &outBuffers = pProcessRequestInfo->outputBuffersMap.at(0);
    ret = Device->image->MatchToImage((void *)&outBuffers[0], &outputMain);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), (ProcessRetStatus)ret, "Match buffer err");

    DEV_LOGI("output main width: %d, height: %d, stride: %d, scanline: %d, format: %d",
             outputMain.width, outputMain.height, outputMain.stride[0], outputMain.sliceHeight[0],
             outputMain.format);

    switch (g_nViewCtrl) {
    case SHOW_MAIN_ONLY:
        Device->image->Copy(&outputMain, &inputMainVector[0]);
        break;
    default:
        Device->detector->Begin(m_detectorRun, MARK("BokehPlugin"), 5500);
        Device->mutex->Lock(m_mutexHdr_Id);
        ret = ProcessHdr(inputMainVector, &outputMain, logicalCamMetaVector);
        Device->mutex->Unlock(m_mutexHdr_Id);
        runTime = Device->detector->End(m_detectorRun);
        DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Bokeh ProcessHdr failed=%d", ret);
        break;
    }

    if (g_bDrawROI) {
        DEV_IMAGE_RECT focusROI = {0};
        DEV_IMAGE_POINT focusTP = {0};
        m_pNodeMetaData->GetfocusROI(m_pPhysicalMeta, &inputMainVector[0], &focusROI);
        m_pNodeMetaData->GetfocusTP(m_pPhysicalMeta, &inputMainVector[0], &focusTP);
        MIAI_HDR_FACEINFO face_info = {0};
        m_pNodeMetaData->GetFaceRect(m_pPhysicalMeta, &inputMainVector[0], &face_info);
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
                 "miDualHdr:{processCount:%d hdrEnabled:%d "
                 "costTime:%d ret:%d}",
                 m_ProcessCount, hdrEnabled, runTime, ret);
        std::string results(buf);
        m_NodeInterface.pSetResultMetadata(m_NodeInterface.owner, pProcessRequestInfo->frameNum,
                                           pProcessRequestInfo->timeStamp, results);
    }
    m_ProcessCount++;
    return (ProcessRetStatus)RET_OK;
}

S32 MiDualCamHdrPlugin::PrepareToHDR_YUV_OFFSCREEN(DEV_IMAGE_BUF *image, HDR_YUV_OFFSCREEN *stImage)
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
    stImage->fd[0] = (MInt32)image->fd[0];
    stImage->fd[1] = (MInt32)image->fd[1];
    stImage->fd[2] = (MInt32)image->fd[2];
    return RET_OK;
}

S32 MiDualCamHdrPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

MRESULT MiDualCamHdrPlugin::ProcessHdrCb(MLong lProgress, MLong lStatus, MVoid *pParam)
{
    lStatus = 0;
    lProgress = 0;
    MiDualCamHdrPlugin *pMiHdr = (MiDualCamHdrPlugin *)pParam;
    if (pMiHdr != NULL && pMiHdr->m_flustStatus) {
        DEV_LOGE("MERR_USER_CANCEL");
        return MERR_USER_CANCEL;
    }
    return MERR_NONE;
}

S32 MiDualCamHdrPlugin::ProcessHdr(std::vector<DEV_IMAGE_BUF> inputMainVector,
                                   DEV_IMAGE_BUF *outputImage,
                                   const std::vector<camera_metadata_t *> &logicalCamMetaVector)
{
    S64 ret = RET_OK;
    int input_num = 3; // hard code, MDbokeh has 4 frames.
    ARC_HDR_PARAM Param = {0};
    HDR_YUV_INPUTINFO InputImages = {0};
    HDR_EVLIST_AEC_INFO EVAEInfo = {0};
    HDR_YUV_OFFSCREEN DstImg = {0};
    U32 rotateAngle = 0;
    U8 stylizationType = 0; // 0: CIVI Vivid;  1: CIVI Classic;
    ret = m_pNodeMetaData->GetRotateAngle(logicalCamMetaVector[0], &rotateAngle);
    ret = m_pNodeMetaData->GetStylizationType(logicalCamMetaVector[0], &stylizationType);
    InputMetadataBokeh inputMetaBokeh = {0};
    ret = m_pNodeMetaData->GetInputMetadataBokeh(m_plogicalMeta, &inputMetaBokeh);
    MIAI_HDR_FACEINFO face_info = {0};
    m_pNodeMetaData->GetFaceRect(m_pPhysicalMeta, &inputMainVector[0], &face_info);
    HDRMetaData hdrMetaData;
    memset(&hdrMetaData, 0, sizeof(HDRMetaData));
    hdrMetaData.config.camera_id = REAR_CAMERA1;
    hdrMetaData.config.hdr_type = MIAI_HDR_BOKEH;
    hdrMetaData.config.orientation = rotateAngle / 90;
    hdrMetaData.config.hdrSnapShotStyle = stylizationType;
    hdrMetaData.config.face_info = face_info;
    ARC_HDR_AEINFO mAeInfo = {0};
    m_pNodeMetaData->GetAEInfo(m_plogicalMeta, &mAeInfo);
    EVAEInfo.pAEInfo = &mAeInfo;
    InputImages.i32ImgNum = input_num;
    DEV_IF_LOGE_RETURN_RET((inputMainVector.size() < input_num), RET_ERR_ARG, "input_num ERR =%d",
                           input_num);
    for (int i = 0; i < input_num; i++) {
        ret = PrepareToHDR_YUV_OFFSCREEN(&inputMainVector[i], &InputImages.InputImages[i]);
        DEV_IF_LOGE_RETURN_RET((ret != RET_OK), ret, "PrepareToHDR_YUV_OFFSCREEN=%d", ret);
        S32 expValue = 0;
        ret = m_pNodeMetaData->GetExpValue(logicalCamMetaVector[i], &expValue);
        EVAEInfo.ev_list[i] = expValue;
        U64 exposureTime = 0;
        ret = m_pNodeMetaData->GetExposureTime(logicalCamMetaVector[i], &exposureTime);
        EVAEInfo.expossure_time[i] = exposureTime;
        FP32 linearGain = 0;
        ret = m_pNodeMetaData->GetLinearGain(logicalCamMetaVector[i], &linearGain);
        EVAEInfo.total_gain[i] = linearGain;
        FP32 sensitivity = 0;
        ret = m_pNodeMetaData->GetAECsensitivity(logicalCamMetaVector[i], &sensitivity);
        EVAEInfo.sensitivity[i] = sensitivity;
    }
    ret = PrepareToHDR_YUV_OFFSCREEN(outputImage, &DstImg);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), ret, "PrepareToHDR_YUV_OFFSCREEN=%d", ret);
    int index = HDR_PARAM_INDEX_REAR;
    MHandle phExpEffect = NULL;
    Device->detector->Begin(m_detectorHdrRun, MARK("XMI_HDRBOKEH_Init"), 1000);
    ret = XMI_HDRBOKEH_Init(&phExpEffect, HDR_PARAM_INDEX_REAR, ARC_TYPE_QUALITY_FIRST);
    Device->detector->End(m_detectorHdrRun);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "XMI_HDRBOKEH_Init=%d", ret);
    if (NULL == phExpEffect) {
        ret = RET_ERR;
    }
    DEV_IF_LOGE_GOTO(phExpEffect == NULL, exit, "phExpEffect error =%p", phExpEffect);
    ret = XMI_HDRBOKEH_GetDefaultParam(&Param);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "XMI_HDRBOKEH_GetDefaultParam=%d", ret);
    Param.i32ToneLength = MiHDRParams[index].tonelen;
    Param.i32Brightness = MiHDRParams[index].brightness;
    Param.i32Saturation = MiHDRParams[index].saturation;
    Param.i32CurveContrast = MiHDRParams[index].contrast;
    Device->detector->Begin(m_detectorHdrRun, MARK("XMI_HDRBOKEH_Process"), 3800);
    ret = XMI_HDRBOKEH_Process(phExpEffect, &InputImages, &EVAEInfo, &Param, &DstImg, &hdrMetaData,
                               ProcessHdrCb, (void *)this, NULL);
    Device->detector->End(m_detectorHdrRun);
    //-999 The frame difference is large and the process is abandoned,
    // XMI_HDRBOKEH_Process copy EV0 to output
    if (ret == -999) {
        DEV_LOGW(
            "XMI_HDRBOKEH_Process -999 the frame difference is large and the process is abandoned");
        ret = RET_OK;
    }
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "XMI_HDRBOKEH_Process=%d", ret);

exit:
    if (ret != RET_OK) {
        Device->image->Copy(outputImage, &inputMainVector[0]);
    }
    if (phExpEffect != NULL) {
        S64 ret1 = XMI_HDRBOKEH_Uninit(&phExpEffect);
        phExpEffect = NULL;
        DEV_IF_LOGE_RETURN_RET((ret != RET_OK || ret1 != RET_OK), ret, "XMI_HDRBOKEH_Uninit=%d",
                               ret1);
    }
    return ret;
}
