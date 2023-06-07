// #include "miautil.h"
// #include "miapluginwraper.h"
#include "ldcCaptureUp.h"

#include <fstream>

#include "MiaPluginUtils.h"
#include "device.h"
#include "nodeMetaDataUp.h"
namespace mialgo2 {

#undef LOG_TAG
#define LOG_TAG                                 "LDC-CAPTURE-UP"
#define LDC_TUNING_DATA_AH_FILE_NAME            "/vendor/etc/camera/ldc/LDC_FPC_TUNING.bin"
#define LDC_TUNING_DATA_AI_FILE_NAME            "/vendor/etc/camera/ldc/LDC_FPC_TUNING_AI.bin"
#define LDC_PACKDATA_VENDORID_0x01_FILE_NAME    "/vendor/etc/camera/ldc/LDC_PACKDATA.dat"
#define LDC_PACKDATA_VENDORID_0x03_FILE_NAME    "/vendor/etc/camera/ldc/LDC_PACKDATA.dat"
#define LDC_PACKDATA_VENDORID_0x07_FILE_NAME    "/vendor/etc/camera/ldc/LDC_PACKDATA.dat"
#define LDC_CERTIFATION_VENDORID_0x01_FILE_NAME "/vendor/etc/camera/ldc/ceres_solver-LICENSE"
#define LDC_AI_SEGMETATION_MODEL_FILE_NAME      "/vendor/etc/camera/ldc/AI_Seg.bin"
#define LDC_AC_WORK_PATH                        "/data/vendor/camera_dump/"
#define LDC_DEBUG_PATH                          "/data/vendor/camera_dump/"
#define LDC_DEBUG_FILE_NAME                     "/data/vendor/camera/alAILDC.bin"
#define LDC_TUNING_VALUE                        110.0f
static U32 ldcLibInit_f = FALSE;

static std::mutex gMutex;
// static S64 m_mutexId = 0;
// static S64 m_detectorRunId = 0;
// static S64 m_detectorInitId = 0;
// static S64 m_detectorMaskId = 0;
// static U32 m_format = 0xFF;
// static U32 m_Width = 0;
// static U32 m_Height = 0;
// static U32 m_StrideW = 0;
// static U32 m_StrideH = 0;
// static U32 m_sCurMode = 0;

static U32 m_inFormat = 0xFF;
static U32 m_inWidth = 0;
static U32 m_inHeight = 0;
static U32 m_inStrideW = 0;
static U32 m_inStrideH = 0;

static U32 m_outFormat = 0xFF;
static U32 m_outWidth = 0;
static U32 m_outHeight = 0;
static U32 m_outStrideW = 0;
static U32 m_outStrideH = 0;

LdcCapture::LdcCapture()
{ // @suppress("Class members should be properly initialized")

    m_initFirst = property_get_bool("persist.ldc.init.first", 0);

    DEV_LOGI("LdcCapture() ldcLibInit_f = %x m_inited = %x", ldcLibInit_f, m_inited);

    if (m_debug == 1) {
        // if (m_mutexId == 0) {
        //    m_mutexId = Device->mutex->Create();
        //}
        if (m_detectorRunId == 0) {
            m_detectorRunId = Device->detector->Create();
        }
        if (m_detectorInitId == 0) {
            m_detectorInitId = Device->detector->Create();
        }
        if (m_detectorMaskId == 0) {
            m_detectorMaskId = Device->detector->Create();
        }
    }
}

LdcCapture::~LdcCapture()
{
    // Device->mutex->Lock(m_mutexId);
    std::unique_lock<std::mutex> lock(gMutex);
    DEV_LOGI("~LdcCapture() ldcLibInit_f = %x m_inited = %x", ldcLibInit_f, m_inited);

    if ((ldcLibInit_f == TRUE) && (m_inited == TRUE)) {
        if ((m_feature == LDC_CAPTURE_MODE_ALTEK_AI) || (m_feature == LDC_CAPTURE_MODE_ALTEK_HI)) {
            alAILDC_Buffer_Deinit(0);
            DEV_LOGI("~LdcCapture() alAILDC_Buffer_Deinit");
        }

        if ((m_feature == LDC_CAPTURE_MODE_ALTEK_AH) || (m_feature == LDC_CAPTURE_MODE_ALTEK_HI)) {
            alhLDC_Deinit();
            DEV_LOGI("~LdcCapture() alhLDC_Deinit");
        }

        m_inFormat = 0;
        m_inWidth = 0;
        m_inHeight = 0;
        m_inStrideW = 0;
        m_inStrideH = 0;

        m_outFormat = 0;
        m_outWidth = 0;
        m_outHeight = 0;
        m_outStrideW = 0;
        m_outStrideH = 0;

        ldcLibInit_f = FALSE;
        m_inited = FALSE;
        m_feature = 0;
        m_debug = 0;
    }
    // Device->mutex->Unlock(m_mutexId);
    if (m_debug == 1) {
        Device->detector->Destroy(&m_detectorRunId);
        Device->detector->Destroy(&m_detectorInitId);
        Device->detector->Destroy(&m_detectorMaskId);
        // Device->mutex->Destroy(&m_mutexId);
    }
}

S64 LdcCapture::Init(CreateInfo *pCreateInfo, U32 debug, U32 mode)
{
    DEV_LOGI("Init() enter");
    S64 ret = 0;
    m_feature = mode;
    m_debug = debug;
    DEV_IMAGE_FORMAT devFormat = DEV_IMAGE_FORMAT_YUV420NV21;

    m_CameraId = pCreateInfo->frameworkCameraId;

    DEV_LOGI("Init m_CameraId =  %d", m_CameraId);
    if ((m_CameraId != CAM_COMBMODE_REAR_ULTRA) && (m_CameraId != CAM_COMBMODE_REAR_SAT_WU)) {
        return ret;
    }
    DEV_LOGI("Init m_initFirst =  %d", m_initFirst);
    if (m_initFirst == false) {
        return ret;
    }

    U32 inFormat = pCreateInfo->inputFormat.format;
    U32 inWidth = pCreateInfo->inputFormat.width;
    U32 inHeight = pCreateInfo->inputFormat.height;
    U32 inStrideW = pCreateInfo->inputFormat.width;
    U32 inStrideH = pCreateInfo->inputFormat.height;

    U32 outFormat = pCreateInfo->outputFormat.format;
    U32 outWidth = pCreateInfo->outputFormat.width;
    U32 outHeight = pCreateInfo->outputFormat.height;
    U32 outStrideW = pCreateInfo->outputFormat.width;
    U32 outStrideH = pCreateInfo->outputFormat.height;

    if (inFormat == CAM_FORMAT_YUV_420_NV12) {
        devFormat = DEV_IMAGE_FORMAT_YUV420NV21;
    }

    inFormat = devFormat;
    outFormat = devFormat;

    std::unique_lock<std::mutex> lock(gMutex);

    if ((m_inWidth != inWidth) || (m_inHeight != inHeight) || (m_inStrideW != inStrideW) ||
        (m_inStrideH != inStrideH) || (m_inFormat != inFormat)) {
        ret = InitLib(inWidth, inHeight, inStrideW, inStrideH, outWidth, outHeight, outStrideW,
                      outStrideH, inFormat);
        if (ret) {
            DEV_LOGI("Init() InitLib ERR");
        } else {
            DEV_LOGI("Init() InitLib OK");
        }
    }

    DEV_LOGI(
        "LDC Init() mode=%d inW=%d inH=%d (SW=%d SH=%d) outW=%d outH=%d (SW=%d SH=%d) format=%d "
        "ret = %d",
        m_feature, m_inWidth, m_inHeight, m_inStrideW, m_inStrideH, m_outWidth, m_outHeight,
        m_outStrideW, m_outStrideH, m_inFormat, ret);
    DEV_LOGI("Init() exit");
    return RET_OK;
}

S64 LdcCapture::GetFileData(const char *fileName, void **ppData0, U64 *pSize0)
{
    S64 ret = RET_OK;
    void *dataBuf = NULL;
    S64 dataSize = 0;

    DEV_IF_LOGE_RETURN_RET((NULL == pSize0), RET_ERR_ARG, "pSize0 is null");
    dataSize = Device->file->GetSize(fileName);
    DEV_IF_LOGE_RETURN_RET((0 == dataSize), RET_ERR, "get file err");
    dataBuf = malloc(dataSize);
    DEV_IF_LOGE_RETURN_RET((NULL == dataBuf), RET_ERR, "malloc err");
    ret = Device->file->ReadOnce(fileName, dataBuf, dataSize, 0);
    DEV_IF_LOGE_RETURN_RET((ret != dataSize), RET_ERR, "read file err");
    DEV_IF_LOGI((ret == dataSize), "LDC data FILE[%s]", fileName);
    *pSize0 = dataSize;
    *ppData0 = dataBuf;
    return RET_OK;
}

S64 LdcCapture::FreeFileData(void **ppData0)
{
    if (ppData0 != NULL) {
        if (*ppData0) {
            free(*ppData0);
            *ppData0 = NULL;
        }
    }
    return RET_OK;
}

S64 LdcCapture::GetPackData(LDC_CAPTURE_MODE mode, void **ppData0, U64 *pSize0, void **ppData1,
                            U64 *pSize1)
{
    S64 ret = RET_OK;
    S64 packDataSize = 0;
    void *packDataBuf = NULL;
    void *tuningBuf = NULL;
    S64 tuningSize = 0;
    char packDataFileName[128] = LDC_PACKDATA_VENDORID_0x01_FILE_NAME;
    char tuningDataFileName[128] = LDC_TUNING_DATA_AH_FILE_NAME;
    DEV_IF_LOGE_RETURN_RET((NULL == pSize0), RET_ERR_ARG, "pSize1 is null");
    DEV_IF_LOGE_RETURN_RET((NULL == pSize1), RET_ERR_ARG, "pSize1 is null");
    if (m_feature == LDC_CAPTURE_MODE_ALTEK_AI) {
        sprintf(tuningDataFileName, "%s", LDC_TUNING_DATA_AI_FILE_NAME);
    }
    if (m_feature == LDC_CAPTURE_MODE_ALTEK_AH) {
        sprintf(tuningDataFileName, "%s", LDC_TUNING_DATA_AH_FILE_NAME);
    }
    tuningSize = Device->file->GetSize(tuningDataFileName);
    DEV_IF_LOGE_RETURN_RET((0 == tuningSize), RET_ERR, "get file err");
    tuningBuf = malloc(tuningSize);
    DEV_IF_LOGE_RETURN_RET((NULL == tuningBuf), RET_ERR, "malloc err");
    ret = Device->file->ReadOnce(tuningDataFileName, tuningBuf, tuningSize, 0);
    DEV_IF_LOGE_RETURN_RET((ret != tuningSize), RET_ERR, "read file err");
    if (m_vendorId == 0x03) {
        sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x03_FILE_NAME);
    } else if (m_vendorId == 0x07) {
        sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x07_FILE_NAME);
    } else {
        sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x01_FILE_NAME);
    }
    packDataSize = Device->file->GetSize(packDataFileName);
    DEV_IF_LOGE_RETURN_RET((0 == packDataSize), RET_ERR, "get file err");
    packDataBuf = malloc(packDataSize);
    DEV_IF_LOGE_RETURN_RET((NULL == packDataBuf), RET_ERR, "malloc err");
    ret = Device->file->ReadOnce(packDataFileName, packDataBuf, packDataSize, 0);
    DEV_IF_LOGE_RETURN_RET((ret != packDataSize), RET_ERR, "read file err %" PRIi64, ret);
    DEV_IF_LOGI((ret == packDataSize), "LDC PACK FILE[%s]", packDataFileName);
    *pSize0 = packDataSize;
    *ppData0 = packDataBuf;
    *pSize1 = tuningSize;
    *ppData1 = tuningBuf;
    return RET_OK;
}

S64 LdcCapture::FreePackData(void **ppData0, void **ppData1)
{
    if (ppData0 != NULL) {
        if (*ppData0) {
            free(*ppData0);
            *ppData0 = NULL;
        }
    }
    if (ppData1 != NULL) {
        if (*ppData1) {
            free(*ppData1);
            *ppData1 = NULL;
        }
    }
    return RET_OK;
}

// Should hold mutex m_mutexId before calling this func
S64 LdcCapture::InitLib(U32 inWidth, U32 inHeight, U32 inStrideW, U32 inStrideH, U32 outWidth,
                        U32 outHeight, U32 outStrideW, U32 outStrideH, U32 format)
{
    S64 ret = RET_ERR;
    char version[512] = {0};
    DEV_LOGI("InitLib() enter");

    if ((m_debug & LDC_CAPTURE_DEBUG_INFO) != 0) {
        Device->detector->Begin(m_detectorInitId);
    }

    if (ldcLibInit_f == TRUE) {
        // if (m_sCurMode == LDC_CAPTURE_MODE_ALTEK_DEF) {
        if ((m_feature == LDC_CAPTURE_MODE_ALTEK_AI) || (m_feature == LDC_CAPTURE_MODE_ALTEK_HI)) {
            DEV_LOGI("InitLib() alAILDC_Buffer_Deinit()");
            ret = alAILDC_Buffer_Deinit(0);
        }

        if ((m_feature == LDC_CAPTURE_MODE_ALTEK_AH) || (m_feature == LDC_CAPTURE_MODE_ALTEK_HI)) {
            DEV_LOGI("InitLib() alhLDC_Deinit");
            ret = alhLDC_Deinit();
        }

        // m_format = 0xFF;
        DEV_IF_LOGE((ret != RET_OK), " alhLDC_Deinit err");

        m_inFormat = 0;
        m_inWidth = 0;
        m_inHeight = 0;
        m_inStrideW = 0;
        m_inStrideH = 0;

        m_outFormat = 0;
        m_outWidth = 0;
        m_outHeight = 0;
        m_outStrideW = 0;
        m_outStrideH = 0;

        ldcLibInit_f = FALSE;
        m_inited = FALSE;
    }

    if ((m_feature == LDC_CAPTURE_MODE_ALTEK_AI) || (m_feature == LDC_CAPTURE_MODE_ALTEK_HI)) {
        void *packDataBuf = NULL;
        U64 packDataSize = 0;
        void *tuningBuf = NULL;
        U64 tuningSize = 0;
        void *segDataBuf = NULL;
        U64 segDataSize = 0;
        char acDebugPath[1024] = LDC_DEBUG_PATH;
        char acDebugFile[1024] = LDC_DEBUG_FILE_NAME;
        char acWorkPath[1024] = LDC_AC_WORK_PATH;

        // alAILDC_Set_Flag(alAILDC_LEVEL_0);
        // alAILDC_Set_Debug_Path (acDebugPath, Device->file->GetSize(acDebugFile));
        // Device->file->GetSize(LDC_CERTIFATION_VENDORID_0x01_FILE_NAME));
        // ret |= alAILDC_Set_Working_Path(acWorkPath, Device->file->GetSize(acWorkPath));

        // set version
        ret = alAILDC_VersionInfo_Get(version, sizeof(version));
        DEV_IF_LOGE_RETURN_RET((ret != 0), RET_ERR_ARG, "alAILDC_VersionInfo_Get() fail");
        DEV_LOGI("InitLib() alAILDC_VersionInfo_Get() is %s", version);
        // set work path
        ret = alAILDC_Set_Working_Path(acWorkPath, 100);
        DEV_IF_LOGE_RETURN_RET((ret != 0), RET_ERR_ARG, "alAILDC_Set_Working_Path() fail");
        // get pack data
        ret = GetFileData(LDC_PACKDATA_VENDORID_0x01_FILE_NAME, &packDataBuf, &packDataSize);
        if (ret) {
            FreeFileData(&packDataBuf);
        }
        DEV_IF_LOGE_RETURN_RET((ret != 0), RET_ERR_ARG, "GetFileData(LDC_PACKDATA) fail");
        // get tuning data
        ret = GetFileData(LDC_TUNING_DATA_AI_FILE_NAME, &tuningBuf, &tuningSize);
        if (ret) {
            FreeFileData(&packDataBuf);
            FreeFileData(&tuningBuf);
        }
        DEV_IF_LOGE_RETURN_RET((ret != 0), RET_ERR_ARG, "GetFileData(LDC_TUNING_DATA) fail");
        // get segmetation
        ret = GetFileData(LDC_AI_SEGMETATION_MODEL_FILE_NAME, &segDataBuf, &segDataSize);
        if (ret) {
            FreeFileData(&packDataBuf);
            FreeFileData(&tuningBuf);
            FreeFileData(&segDataBuf);
        }
        DEV_IF_LOGE_RETURN_RET((ret != 0), RET_ERR_ARG, "GetFileData(LDC_AI_SEGMETATION) fail");
        // set buffer init
        ret = alAILDC_Buffer_Init_v2(
            0, inWidth, inHeight, outWidth, outHeight, inStrideW, inStrideH, outStrideW, outStrideH,
            (alAILDC_IMG_FORMAT)transformFormat((DEV_IMAGE_FORMAT)format),
            (unsigned char *)packDataBuf, packDataSize, (unsigned char *)tuningBuf, tuningSize,
            (unsigned char *)segDataBuf, segDataSize);
        FreeFileData(&packDataBuf);
        FreeFileData(&tuningBuf);
        FreeFileData(&segDataBuf);
        DEV_IF_LOGE_RETURN_RET((ret != 0), RET_ERR_ARG, "alAILDC_Buffer_Init_v2() fail");
    }

    if ((m_feature == LDC_CAPTURE_MODE_ALTEK_AH) || (m_feature == LDC_CAPTURE_MODE_ALTEK_HI)) {
        void *packDataBuf = NULL;
        U64 packDataSize = 0;
        void *tuningBuf = NULL;
        U64 tuningSize = 0;

        // set version
        ret = alhLDC_VersionInfo_Get(version, sizeof(version));
        DEV_IF_LOGE_RETURN_RET((ret != 0), RET_ERR_ARG, "alAILDC_VersionInfo_Get() fail");
        DEV_LOGI("InitLib() alhLDC_VersionInfo_Get() is %s", version);

        // get pack data
        ret = GetFileData(LDC_PACKDATA_VENDORID_0x01_FILE_NAME, &packDataBuf, &packDataSize);
        if (ret) {
            FreeFileData(&packDataBuf);
        }
        DEV_IF_LOGE_RETURN_RET((ret != 0), RET_ERR_ARG, "GetFileData(LDC_PACKDATA) fail");
        // get tuning data
        ret = GetFileData(LDC_TUNING_DATA_AH_FILE_NAME, &tuningBuf, &tuningSize);
        if (ret) {
            FreeFileData(&packDataBuf);
            FreeFileData(&tuningBuf);
        }
        DEV_IF_LOGE_RETURN_RET((ret != 0), RET_ERR_ARG, "GetFileData(LDC_TUNING_DATA) fail");

        // set buffer init
        ret |= alhLDC_Set_KernelMethod(ALHLDC_METHOD_MEM_OPT);
        ret |= alhLDC_Init(inWidth, inHeight, outWidth, outHeight, inStrideW, inStrideH, outStrideW,
                           outStrideH, (unsigned char *)packDataBuf, packDataSize,
                           (unsigned char *)tuningBuf, tuningSize);

        FreeFileData(&packDataBuf);
        FreeFileData(&tuningBuf);
        DEV_IF_LOGE_RETURN_RET((ret != 0), RET_ERR_ARG, "alAILDC_Buffer_Init_v2() fail");
        if ((m_debug & LDC_CAPTURE_DEBUG_INFO) != 0) {
            alhLDC_Set_LogLevel(ALHLDC_LEVEL_2);
        }
    }

    ldcLibInit_f = TRUE;
    m_inited = TRUE;

    // m_format = format;
    if ((m_debug & LDC_CAPTURE_DEBUG_INFO) != 0) {
        Device->detector->End(m_detectorInitId, "LDC-CAPTURE_INIT");
    }

    // set global value
    m_inFormat = format;
    m_inWidth = inWidth;
    m_inHeight = inHeight;
    m_inStrideW = inStrideW;
    m_inStrideH = inStrideH;

    m_outFormat = format;
    m_outWidth = outWidth;
    m_outHeight = outHeight;
    m_outStrideW = outStrideW;
    m_outStrideH = outStrideH;
    // m_sCurMode = m_feature;
    DEV_LOGI("LDC Capture INIT0 [%" PRIi64
             "] (Ver:%s) m_feature=%d inW=%d inH=%d (SW=%d SH=%d) outW=%d outH=%d (SW=%d SH=%d) "
             "format=%d",
             ret, version, m_feature, inWidth, inHeight, inStrideW, inStrideH, outWidth, outHeight,
             outStrideW, outStrideH, m_outFormat);
    DEV_LOGI("InitLib() exit");
    return ret;
}

S64 LdcCapture::Process(DEV_IMAGE_BUF *input, DEV_IMAGE_BUF *output,
                        NODE_METADATA_FACE_INFO *pFaceInfo, MIRECT *cropRegion, uint32_t streamMode,
                        bool isFastShot)
{
    DEV_LOGI("Process() enter");
    // asset err
    DEV_IF_LOGE_RETURN_RET((NULL == input), RET_ERR_ARG, "input is null");
    DEV_IF_LOGE_RETURN_RET((NULL == output), RET_ERR_ARG, "output is null");
    DEV_IF_LOGE_RETURN_RET((NULL == pFaceInfo), RET_ERR_ARG, "cropRegion is null");
    // Device->mutex->Lock(m_mutexId);    //lock to prevent re-entry, especially when deinit
    // triggered while another LDC is processing.
    S64 ret = 0;

    std::unique_lock<std::mutex> lock(gMutex);
    double processStartTime = GetCurrentSecondValue();
    double processEndTime = processStartTime;
    DEV_LOGI("Process() ldcLibInit_f = %x m_inited = %x", ldcLibInit_f, m_inited);

    DEV_LOGI(
        "LDC-Info(capture#1): StreamMode:%d inW=%d inH=%d (SW=%d SH=%d) outW=%d outH=%d (SW=%d "
        "SH=%d) format=%d",
        streamMode, input->width, input->height, input->stride[0], input->sliceHeight[0],
        output->width, output->height, output->stride[0], output->sliceHeight[0], output->format);

    // set InitLib
    if ((m_inWidth != input->width) || (m_inHeight != input->height) ||
        (m_inStrideW != input->stride[0]) || (m_inStrideH != input->height) ||
        (m_inFormat != input->format)) {
        ret = InitLib(input->width, input->height, input->stride[0], input->sliceHeight[0],
                      output->width, output->height, output->stride[0], output->sliceHeight[0],
                      input->format);
        DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "LdcCapture Init ERR");
    }
    DEV_IF_LOGE_RETURN_RET((ldcLibInit_f != TRUE), RET_ERR, "InitLib err");

    DEV_LOGI("process() m_feature = %d pFaceInfo->num = %d isFastShot = %d", m_feature,
             pFaceInfo->num, isFastShot);

    if (((m_feature == LDC_CAPTURE_MODE_ALTEK_AI) ||
         ((m_feature == LDC_CAPTURE_MODE_ALTEK_HI) &&
          ((isFastShot == false) && (pFaceInfo->num > 0))))) {
        DEV_LOGI("process() AI LDC enter");
        // set WOI
        alAILDC_RECT pa_ptInZoomWOI;
        pa_ptInZoomWOI.x = cropRegion->left;
        pa_ptInZoomWOI.y = cropRegion->top;
        pa_ptInZoomWOI.width = cropRegion->width;
        pa_ptInZoomWOI.height = cropRegion->height;

        DEV_LOGI("pa_ptInZoomWOI: [%d %d %d %d]", pa_ptInZoomWOI.x, pa_ptInZoomWOI.y,
                 pa_ptInZoomWOI.width, pa_ptInZoomWOI.height);

        // set face
        alAILDC_FACE_INFO alhInFaceInfo;
        memset(&alhInFaceInfo, 0, sizeof(alhInFaceInfo));
        alhInFaceInfo.count = pFaceInfo->num;
        alhInFaceInfo.src_img_height = pFaceInfo->imgHeight;
        alhInFaceInfo.src_img_width = pFaceInfo->imgWidth;
        for (int i = 0; i < alhInFaceInfo.count; i++) {
            alhInFaceInfo.face[i].score = pFaceInfo->score[i];
            alhInFaceInfo.face[i].roi.height = pFaceInfo->face[i].height;
            alhInFaceInfo.face[i].roi.width = pFaceInfo->face[i].width;
            alhInFaceInfo.face[i].roi.x = pFaceInfo->face[i].left;
            alhInFaceInfo.face[i].roi.y = pFaceInfo->face[i].top;
        }
        // set face angle
        alAILDC_SET_SCREEN_ORIENTATION screenOrientation = alAILDC_SCREEN_ANGLE_0;
        if (pFaceInfo->num > 0) {
            if (pFaceInfo->orient[0] == 0) {
                screenOrientation = alAILDC_SCREEN_ANGLE_0;
            } else if (pFaceInfo->orient[0] == 90) {
                screenOrientation = alAILDC_SCREEN_ANGLE_90;
            } else if (pFaceInfo->orient[0] == 180) {
                screenOrientation = alAILDC_SCREEN_ANGLE_180;
            } else if (pFaceInfo->orient[0] == 270) {
                screenOrientation = alAILDC_SCREEN_ANGLE_270;
            }
        }

        // face dump
        if ((m_debug & LDC_CAPTURE_DEBUG_DUMP_IMAGE_CAPTURE) != 0) {
            char faceInfoBuf[1024 * 8] = {0};
            sprintf(faceInfoBuf + strlen(faceInfoBuf), "FACE NUM [%d]\n", alhInFaceInfo.count);
            sprintf(faceInfoBuf + strlen(faceInfoBuf),
                    "-----------------------------------------\n");
            for (int i = 0; i < alhInFaceInfo.count; i++) {
                sprintf(faceInfoBuf + strlen(faceInfoBuf),
                        "FACE [%d/%d] [x=%d,y=%d,w=%d,h=%d] S[%d]\n", i + 1, alhInFaceInfo.count,
                        alhInFaceInfo.face[i].roi.x, alhInFaceInfo.face[i].roi.y,
                        alhInFaceInfo.face[i].roi.width, alhInFaceInfo.face[i].roi.height,
                        alhInFaceInfo.face[i].score);
            }
            Device->image->DumpData(faceInfoBuf, strlen(faceInfoBuf), LOG_TAG, 0,
                                    "FaceInfo.in.txt");
        }

        // process
        // if (m_feature == LDC_CAPTURE_MODE_ALTEK_DEF) {

        if ((m_debug & LDC_CAPTURE_DEBUG_INFO) != 0) {
            Device->detector->Begin(m_detectorRunId);
        }
        // [API] alAILDC_Set_ScreenOrientation
        alAILDC_Set_ScreenOrientation(0, screenOrientation);

        // [API] alAILDC_Set_TuningParameters
        // float tuning_value = LDC_TUNING_VALUE;
        // alAILDC_Set_TuningParameters (0, alAILDC_PARA_EDIT_TYPE_CORRECTION_STRENGTH,
        // &tuning_value);
        ret = alAILDC_Buffer_Run(
            0, (unsigned char *)input[0].plane[0], (unsigned char *)input[0].plane[1],
            (unsigned char *)output[0].plane[0], (unsigned char *)output[0].plane[1], 0,
            NULL /*rotateMat*/, (alAILDC_RECT *)&pa_ptInZoomWOI, &alhInFaceInfo, NULL);

        if ((m_debug & LDC_CAPTURE_DEBUG_INFO) != 0) {
            Device->detector->End(m_detectorRunId, "LDC-CAPTURE_RUN");
        }
    }

    if ((m_feature == LDC_CAPTURE_MODE_ALTEK_AH) ||
        ((m_feature == LDC_CAPTURE_MODE_ALTEK_HI) &&
         ((isFastShot == true) || ((isFastShot == false) && (pFaceInfo->num <= 0))))) {
        DEV_LOGI("process() AH LDC enter");
        // set WOI
        alhLDC_RECT pa_ptInZoomWOI;
        pa_ptInZoomWOI.x = cropRegion->left;
        pa_ptInZoomWOI.y = cropRegion->top;
        pa_ptInZoomWOI.width = cropRegion->width;
        pa_ptInZoomWOI.height = cropRegion->height;
        DEV_LOGI("pa_ptInZoomWOI: [%d %d %d %d]", pa_ptInZoomWOI.x, pa_ptInZoomWOI.y,
                 pa_ptInZoomWOI.width, pa_ptInZoomWOI.height);

        // set face
        alhLDC_FACE_INFO alhInFaceInfo;
        memset(&alhInFaceInfo, 0, sizeof(alhInFaceInfo));
        alhInFaceInfo.count = pFaceInfo->num;
        alhInFaceInfo.src_img_height = pFaceInfo->imgHeight;
        alhInFaceInfo.src_img_width = pFaceInfo->imgWidth;
        for (int i = 0; i < alhInFaceInfo.count; i++) {
            alhInFaceInfo.face[i].score = pFaceInfo->score[i];
            alhInFaceInfo.face[i].roi.height = pFaceInfo->face[i].height;
            alhInFaceInfo.face[i].roi.width = pFaceInfo->face[i].width;
            alhInFaceInfo.face[i].roi.x = pFaceInfo->face[i].left;
            alhInFaceInfo.face[i].roi.y = pFaceInfo->face[i].top;
        }
        // set face angle
        alAILDC_SET_SCREEN_ORIENTATION screenOrientation = alAILDC_SCREEN_ANGLE_0;

        // face dump
        if ((m_debug & LDC_CAPTURE_DEBUG_DUMP_IMAGE_CAPTURE) != 0) {
            char faceInfoBuf[1024 * 8] = {0};
            sprintf(faceInfoBuf + strlen(faceInfoBuf), "FACE NUM [%d]\n", alhInFaceInfo.count);
            sprintf(faceInfoBuf + strlen(faceInfoBuf),
                    "-----------------------------------------\n");
            for (int i = 0; i < alhInFaceInfo.count; i++) {
                sprintf(faceInfoBuf + strlen(faceInfoBuf),
                        "FACE [%d/%d] [x=%d,y=%d,w=%d,h=%d] S[%d]\n", i + 1, alhInFaceInfo.count,
                        alhInFaceInfo.face[i].roi.x, alhInFaceInfo.face[i].roi.y,
                        alhInFaceInfo.face[i].roi.width, alhInFaceInfo.face[i].roi.height,
                        alhInFaceInfo.face[i].score);
            }
            Device->image->DumpData(faceInfoBuf, strlen(faceInfoBuf), LOG_TAG, 0,
                                    "FaceInfo.in.txt");
        }

        // process

        if ((m_debug & LDC_CAPTURE_DEBUG_INFO) != 0) {
            Device->detector->Begin(m_detectorRunId);
        }

        ret = alhLDC_Run((unsigned char *)input[0].plane[0], (unsigned char *)input[0].plane[1],
                         (unsigned char *)output[0].plane[0], (unsigned char *)output[0].plane[1],
                         0, NULL /*rotateMat*/, (alhLDC_RECT *)&pa_ptInZoomWOI, &alhInFaceInfo);

        if ((m_debug & LDC_CAPTURE_DEBUG_INFO) != 0) {
            Device->detector->End(m_detectorRunId, "LDC-CAPTURE_RUN");
        }
    }

    processEndTime = GetCurrentSecondValue();
exit:
    DEV_IF_LOGE((ret != RET_OK), "LDC Capture RUN ERR");
    // Device->mutex->Unlock(m_mutexId);
    DEV_LOGI("Process() ldcLibInit_f = %x m_inited = %x", ldcLibInit_f, m_inited);
    DEV_LOGI("Process() cost time: %f", processEndTime - processStartTime);
    return ret;
}

alAILDC_IMG_FORMAT LdcCapture::transformFormat(DEV_IMAGE_FORMAT format)
{
    alAILDC_IMG_FORMAT fmt;
    DEV_LOGI("Process() enter");
    switch (format) {
    case DEV_IMAGE_FORMAT_YUV420NV21:
        fmt = alAILDC_IMAGE_FORMAT_NV21;
        break;
    case DEV_IMAGE_FORMAT_YUV420NV12:
        fmt = alAILDC_IMAGE_FORMAT_NV12;
        break;
    case DEV_IMAGE_FORMAT_YUV420P010:
        fmt = alAILDC_IMAGE_FORMAT_P010_MSB;
        // fmt = alAILDC_IMAGE_FORMAT_P010_LSB;
        break;
    default:
        fmt = alAILDC_IMAGE_FORMAT_NV21;
        DEV_LOGI("image format(%d) is not supported", format);
    }
    DEV_LOGI("Process() exit");
    return fmt;
}

double LdcCapture::GetCurrentSecondValue()
{
    timeval currentTime;
    gettimeofday(&currentTime, NULL);
    double secondValue =
        ((double)currentTime.tv_sec * 1000.0) + ((double)currentTime.tv_usec / 1000.0);
    return secondValue;
}
} // namespace mialgo2
