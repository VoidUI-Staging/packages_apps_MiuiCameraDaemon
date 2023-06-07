#include <MiaPluginUtils.h>

#include "device/device.h"
#include "mialgo_dynamic_blur_control.h"
#include "nodeMetaData.h"
#include "MiMDBokeh.h"
using namespace mialgo2;

#undef LOG_TAG
#define LOG_TAG "BOKEH_CAPTURE_BOKEH"

#undef LOG_GROUP
#define LOG_GROUP GROUP_PLUGIN

#define CAL_DATA_PATH_MD_BWT "/odm/etc/camera/mdbokeh_bwt.bin"
#define CAL_DATA_PATH_MD_NGT "/odm/etc/camera/mdbokeh_ngt.bin"
#define CAL_DATA_PATH_MD_SFT "/odm/etc/camera/mdbokeh_sft.bin"
#define CAL_DATA_PATH_MD_PRT "/odm/etc/camera/mdbokeh_prt.bin"

#define JSON_PATH_MD         "/odm/etc/camera/dualcam_mdbokeh_params_pro.json"

#define ROLE_REARBOKEH2X 61
#define ROLE_REARBOKEH1X 63

#define MDDATA_LEN 491920

#define MAX_LINKED_SESSIONS 2

#define FRAME_MAX_INPUT_NUM 3
#define MI_MAX_EV_NUM       12
#define MAX_FACE_NUM_BOKEH  10

#define ROLE_REARBOKEH2X 61
#define ROLE_REARBOKEH1X 63

#define SHOW_AUX_ONLY  1
#define SHOW_MAIN_ONLY 2
#define SHOW_HALF_HALF 3

static const S32 g_DebugMode = property_get_int32("persist.vendor.camera.mialgo.cb.debugmode", 0);
static const S32 g_IsSingleCameraMDBokeh =
    property_get_int32("persist.vendor.camera.mialgo.cb.isSingleCameraDepth", -1);

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

enum Region {
    BEAUTYSHOT_REGION_DEFAULT = -1,
    BEAUTYSHOT_REGION_GLOBAL = 0,
    BEAUTYSHOT_REGION_CHINA = 1,
    BEAUTYSHOT_REGION_INDIAN = 2,
    BEAUTYSHOT_REGION_BRAZIL = 3
};
enum {
    HDR_PARAM_INDEX_REAR,
    HDR_PARAM_INDEX_FRONT,
};

MiMDBokeh::~MiMDBokeh() {}

void MiMDBokeh::MDdestroy()
{
    m_flustStatus = FALSE;
    if (m_hCaptureBokeh) {
        MialgoCaptureMdbokehDestory(&m_hCaptureBokeh);
        memset((void *)&m_launchParams, 0, sizeof(m_launchParams));
        m_hCaptureBokeh = NULL;
    }
    FreePackData(&mCaliMdBWT);
    FreePackData(&mCaliMdNGT);
    FreePackData(&mCaliMdSFT);
    FreePackData(&mCaliMdPRT);
    if (m_pNodeMetaData) {
        delete m_pNodeMetaData;
        m_pNodeMetaData = NULL;
    }
    Device->detector->Destroy(&CaptureInitEachPic);
}
int MiMDBokeh::MDinit()
{
    m_hCaptureBokeh = NULL;
    memset((void *)&m_launchParams, 0, sizeof(m_launchParams));
    mCaliMdBWT = NULL;
    mCaliMdNGT = NULL;
    mCaliMdSFT = NULL;
    m_pNodeMetaData = NULL;
    m_MDBlurLevel = 0;
    m_CaliMdBWTLength = 0;
    m_CaliMdNGTLength = 0;
    m_CaliMdSFTLength = 0;
    m_LiteMode = 0;
    Device->detector->Create(&CaptureInitEachPic, MARK("CaptureInitEachPic Run"));
    m_pNodeMetaData = new NodeMetaData();
    if (m_pNodeMetaData != NULL) {
        m_pNodeMetaData->Init(0, 0);
    }
    GetPackData(CAL_DATA_PATH_MD_BWT, &mCaliMdBWT, &m_CaliMdBWTLength);
    GetPackData(CAL_DATA_PATH_MD_NGT, &mCaliMdNGT, &m_CaliMdNGTLength);
    GetPackData(CAL_DATA_PATH_MD_SFT, &mCaliMdSFT, &m_CaliMdSFTLength);
    GetPackData(CAL_DATA_PATH_MD_PRT, &mCaliMdPRT, &m_CaliMdPRTLength);
    return RET_OK;
}

S64 MiMDBokeh::GetPackData(char *fileName, void **ppData0, U64 *pSize0)
{
    S64 ret = RET_OK;
    S64 packDataSize = 0;
    void *packDataBuf = NULL;
    char packDataFileName[128] = CAL_DATA_PATH_MD_BWT;
    Dev_snprintf(packDataFileName, sizeof(packDataFileName), "%s", fileName);
    DEV_IF_LOGE_RETURN_RET((NULL == pSize0), RET_ERR_ARG, "pSize1 is null");
    packDataSize = Device->file->GetSize(packDataFileName);
    DEV_IF_LOGE_RETURN_RET((0 == packDataSize), RET_ERR, "get file err");
    packDataBuf = Device->memoryPool->Malloc(packDataSize, MARK("MDBokeh CalibrationData"));
    DEV_IF_LOGE_RETURN_RET((NULL == packDataBuf), RET_ERR, "malloc err");
    ret = Device->file->ReadOnce(packDataFileName, packDataBuf, packDataSize, 0);
    DEV_IF_LOGE_RETURN_RET((ret != packDataSize), RET_ERR, "read file err %" PRIi64, ret);
    DEV_IF_LOGI((ret == packDataSize), "INFO CROSSTALK FILE[%s][%" PRIi64 "]", packDataFileName,
                packDataSize);
    *pSize0 = packDataSize;
    *ppData0 = packDataBuf;
    return RET_OK;
}

S64 MiMDBokeh::FreePackData(void **ppData0)
{
    if (ppData0 != NULL) {
        if (*ppData0) {
            Device->memoryPool->Free(*ppData0);
            *ppData0 = NULL;
        }
    }
    return RET_OK;
}

S32 MiMDBokeh::PrepareToMiImage(DEV_IMAGE_BUF *image, AlgoMultiCams::MiImage *stImage)
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

S32 MiMDBokeh::GetImageScale(U32 width, U32 height, AlgoMultiCams::ALGO_CAPTURE_FRAME &Scale)
{
    int scale_4_3 = 10 * 4 / 3;
    int scale_16_9 = 10 * 16 / 9;
    int imagescale = 10 * width / height;
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

S32 MiMDBokeh::InitMDBokeh(DEV_IMAGE_BUF *inputMain, camera_metadata_t *pMeta, S32 m_MDMode,
                           S32 m_LiteMode)
{
    S32 ret = RET_OK;
    S32 expValue = -1;

    DEV_IF_LOGE_RETURN_RET((inputMain == NULL), RET_ERR, "error invalid param");

    AlgoMultiCams::ALGO_CAPTURE_FRAME Scale = AlgoMultiCams::ALGO_CAPTURE_FRAME::CAPTURE_FRAME_4_3;
    GetImageScale(inputMain->width, inputMain->height, Scale);
    InputMetadataBokeh inputMetaBokeh = {0};
    ret = m_pNodeMetaData->GetInputMetadataBokeh(pMeta, &inputMetaBokeh);
    DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "GetInputMetadataBokeh err");

    MdbokehLaunchParams launchParams;
    launchParams.MainImgW = inputMain->width;
    launchParams.MainImgH = inputMain->height;
    launchParams.frameInfo = Scale;
    if (m_MDMode == CAPTURE_MDBOKEH_SPIN)
        launchParams.fovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_2X;
    else if (m_MDMode == CAPTURE_MDBOKEH_SOFT)
        launchParams.fovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_3_75X;
    else if (m_MDMode == CAPTURE_MDBOKEH_BLACK)
        launchParams.fovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_1_4X;
    else if (m_MDMode == CAPTURE_MDBOKEH_PORTRAIT)
        launchParams.fovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_3_26X;
    else {
        launchParams.fovInfo = AlgoMultiCams::ALGO_CAPTURE_FOV::CAPTURE_FOV_2X;
        DEV_LOGE("set launchParams.fovInfo error. m_MDMode = %d", m_MDMode);
    }
    launchParams.isQuickSnap = false; // not use now
    launchParams.isUserTouch = false; // not use now

    if (m_LiteMode == 1) {
        launchParams.isHotMode = TRUE;
        launchParams.isPerformanceMode = TRUE;
    } else {
        launchParams.isHotMode = FALSE;
        launchParams.isPerformanceMode = FALSE;
    }

    launchParams.jsonFilename = (char *)JSON_PATH_MD;
    launchParams.mdLensMode = (ALGO_CAPTURE_MDBOKEH_LENS_MODE)(m_MDMode);

    launchParams.mdLensBuf = (unsigned char *)mCaliMdBWT;
    launchParams.mdLensBufSize = m_CaliMdBWTLength;

    if (CAPTURE_MDBOKEH_SPIN == launchParams.mdLensMode) {
        launchParams.mdLensBuf = (unsigned char *)mCaliMdNGT;
        launchParams.mdLensBufSize = m_CaliMdNGTLength;
    }

    if (CAPTURE_MDBOKEH_BLACK == launchParams.mdLensMode) {
        launchParams.mdLensBuf = (unsigned char *)mCaliMdBWT;
        launchParams.mdLensBufSize = m_CaliMdBWTLength;
    }
    if (CAPTURE_MDBOKEH_SOFT == launchParams.mdLensMode) {
        launchParams.mdLensBuf = (unsigned char *)mCaliMdSFT;
        launchParams.mdLensBufSize = m_CaliMdSFTLength;
    }
    if (CAPTURE_MDBOKEH_PORTRAIT == launchParams.mdLensMode) {
        launchParams.mdLensBuf = (unsigned char *)mCaliMdPRT;
        launchParams.mdLensBufSize = m_CaliMdPRTLength;
    }

    if ((memcmp(&m_launchParams, &launchParams, sizeof(launchParams)) != 0)) {
        memcpy(&m_launchParams, &launchParams, sizeof(launchParams));
        DEV_LOGI(
            "INIT INFO M(%dx%d)fovInfo=%d Scale=%d "
            "isHotMode=%d isPerformanceMode=%d mdLensMode=%d ",
            m_launchParams.MainImgW, m_launchParams.MainImgH, m_launchParams.fovInfo,
            m_launchParams.frameInfo, m_launchParams.isHotMode, m_launchParams.isPerformanceMode,
            launchParams.mdLensMode);

        if (m_hCaptureBokeh != NULL) {
            MialgoCaptureMdbokehDestory(&m_hCaptureBokeh);
            m_hCaptureBokeh = NULL;
        }
        Device->detector->Begin(CaptureInitEachPic, MARK("MialgoCaptureMdbokehLaunch"), 1000);
        ret = MialgoCaptureMdbokehLaunch(&m_hCaptureBokeh, m_launchParams);
        Device->detector->End(CaptureInitEachPic);
        DEV_IF_LOGE_RETURN_RET(ret != RET_OK, ret, "MIALGO_CaptureInitWhenLaunch err=%d", ret);
        DEV_IF_LOGE_RETURN_RET(m_hCaptureBokeh == NULL, RET_ERR, "m_hCaptureBokeh err=%p",
                               m_hCaptureBokeh);
    }
    return ret;
}

S32 MiMDBokeh::ProcessMDBokeh(DEV_IMAGE_BUF *outputMain, DEV_IMAGE_BUF *inputMain,
                              DEV_IMAGE_BUF *inputMain_extraEV, DEV_IMAGE_BUF *inputDepth,
                              camera_metadata_t *pMeta, camera_metadata_t *phyMeta)
{
    DEV_IF_LOGE_RETURN_RET((outputMain == NULL || inputMain == NULL || inputDepth == NULL ||
                            inputMain_extraEV == NULL),
                           RET_ERR, "ARG ERR");
    DEV_IF_LOGE_RETURN_RET((m_hCaptureBokeh == NULL), RET_ERR, "ARG ERR");
    S32 ret = RET_OK;
    bool hasFaceNoTouch = false;
    U32 afState = ANDROID_CONTROL_AF_STATE_INACTIVE;
    AlgoMultiCams::MiImage AlgoDepthImg_inputMain = {
        AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_INVALID};
    AlgoMultiCams::MiImage AlgoDepthImg_inputMain_extraEV = {
        AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_INVALID};
    AlgoMultiCams::MiImage AlgoDepthImg_inputDepth = {
        AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_INVALID};
    AlgoMultiCams::MiImage AlgoDepthImg_outputMain = {
        AlgoMultiCams::MiImageFmt::MI_IMAGE_FMT_INVALID};
    disparity_fileheader_t *header = NULL;
    unsigned char *depthData = NULL;
    MdbokehInitParams bokehInitParams = {{0}};
    AlgoMultiCams::MiFaceRect faceRect = {0};
    AlgoMultiCams::MiPoint2i faceCentor = {0};
    DEV_IMAGE_POINT focusTP = {0};
    AlgoMultiCams::MiPoint2i AlgoDepthfocusTP = {0};
    S32 superNightEnabled = 0;
    S32 hdrEnabled = 0;
    S32 sensitivity = 0;
    ret = m_pNodeMetaData->GetSuperNightEnabled(pMeta, &superNightEnabled);
    ret = m_pNodeMetaData->GetHdrEnabled(pMeta, &hdrEnabled);
    ret = m_pNodeMetaData->GetSensitivity(pMeta, &sensitivity);
    InputMetadataBokeh inputMetaBokeh = {0};

    ret |= PrepareToMiImage(inputMain, &AlgoDepthImg_inputMain);
    ret |= PrepareToMiImage(inputDepth, &AlgoDepthImg_inputDepth);
    ret |= PrepareToMiImage(outputMain, &AlgoDepthImg_outputMain);
    ret |= PrepareToMiImage(inputMain_extraEV, &AlgoDepthImg_inputMain_extraEV);
    DEV_IF_LOGE_GOTO(ret != RET_OK, exit, "MallocToAlgoBokehImg=%d", ret);

    ret = m_pNodeMetaData->GetInputMetadataBokeh(pMeta, &inputMetaBokeh);
    m_pNodeMetaData->GetMaxFaceRect(phyMeta, outputMain, &faceRect, &faceCentor);
    ret = m_pNodeMetaData->GetfocusTP(phyMeta, outputMain, &focusTP);
    AlgoDepthfocusTP.x = focusTP.x;
    AlgoDepthfocusTP.y = focusTP.y;
    m_pNodeMetaData->GetAFState(pMeta, &afState);
    if ((ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED != afState &&
         ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN != afState) &&
        (faceRect.width != 0 && faceRect.height != 0)) {
        hasFaceNoTouch = true;
    }
    bokehInitParams.userInterface.fRect = faceRect;
    bokehInitParams.userInterface.tp = hasFaceNoTouch ? faceCentor : AlgoDepthfocusTP;
    bokehInitParams.userInterface.ISO = sensitivity;
    bokehInitParams.userInterface.aperture = m_MDBlurLevel;
    if (superNightEnabled != 0) {
        bokehInitParams.isSingleCameraDepth = TRUE;
        if (g_IsSingleCameraMDBokeh >= 0) {
            bokehInitParams.isSingleCameraDepth = g_IsSingleCameraMDBokeh ? TRUE : FALSE;
        }
    } else {
        bokehInitParams.isSingleCameraDepth = FALSE;
    }

    DEV_LOGI("INIT INFO BlurLevel %d ISO=%d", bokehInitParams.userInterface.aperture,
             bokehInitParams.userInterface.ISO);
    Device->detector->Begin(CaptureInitEachPic, MARK("MialgoCaptureBokehInit"), 1000);
    MialgoCaptureMdbokehInit(&m_hCaptureBokeh, bokehInitParams);
    Device->detector->End(CaptureInitEachPic);
    DEV_IF_LOGE_GOTO((m_hCaptureBokeh == NULL), exit, "MialgoCaptureBokehInit ERR %p",
                     m_hCaptureBokeh);
    header = (disparity_fileheader_t *)AlgoDepthImg_inputDepth.plane[0];
    MdbokehEffectParams bokehParams;
    bokehParams.imgMain = &AlgoDepthImg_inputMain;
    bokehParams.imgDepth = &AlgoDepthImg_inputDepth;
    bokehParams.imgMainEV = NULL;
    bokehParams.imgMainEV = &AlgoDepthImg_inputMain_extraEV;
    bokehParams.validDepthSize = header->i32DataLength;
    bokehParams.hdrTriggered = hdrEnabled;
    bokehParams.seTriggered = superNightEnabled;
    memset(header, 0, sizeof(disparity_fileheader_t));
    bokehParams.imgResult = &AlgoDepthImg_outputMain;
    bokehParams.imgDepth->plane[0] =
        (unsigned char *)(bokehParams.imgDepth->plane[0] +
                          sizeof(disparity_fileheader_t)); // 地址偏移预留头空间
    Device->detector->Begin(CaptureInitEachPic, MARK("MialgoCaptureMdbokehProc"), 4000);
    ret = MialgoCaptureMdbokehProc(&m_hCaptureBokeh, bokehParams);
    Device->detector->End(CaptureInitEachPic);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Bokeh MIALGO_CaptureBokehEffectProc failed=%d", ret);

exit:
    if (m_hCaptureBokeh != NULL) {
        MialgoCaptureMdbokehDeinit(&m_hCaptureBokeh);
        if (ret != RET_OK) {
            if (m_hCaptureBokeh != NULL) {
                MialgoCaptureMdbokehDestory(&m_hCaptureBokeh);
                memset((void *)&m_launchParams, 0, sizeof(m_launchParams));
                m_hCaptureBokeh = NULL;
            }
        }
    }
    return ret;
}
