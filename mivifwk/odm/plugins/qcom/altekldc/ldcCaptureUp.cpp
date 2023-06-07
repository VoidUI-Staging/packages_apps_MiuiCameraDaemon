#include "ldcCaptureUp.h"
#include "MiaPluginUtils.h"
#include "device/device.h"
#include "nodeMetaDataUp.h"

namespace mialgo2 {

#undef LOG_TAG
#define LOG_TAG "LDC-CAPTURE-UP"

// clang-format off
#define LDC_TUNING_DATA_FILE_NAME            "/vendor/etc/camera/LDC_FPC_TUNING_DATA.bin"
#define LDC_TUNING_DATA_AI_FILE_NAME         "/vendor/etc/camera/LDC_FPC_TUNING_DATA_AI.bin"
#define LDC_PACKDATA_VENDORID_0x01_FILE_NAME "/vendor/etc/camera/LDC_PACKDATA_VENDORID_0x01.bin"
#define LDC_PACKDATA_VENDORID_0x03_FILE_NAME "/vendor/etc/camera/LDC_PACKDATA_VENDORID_0x03.bin"
#define LDC_PACKDATA_VENDORID_0x07_FILE_NAME "/vendor/etc/camera/LDC_PACKDATA_VENDORID_0x07.bin"
#define LDC_TUNING_DATA_AI_FILE_NAME_CN         "/vendor/etc/camera/LDC_FPC_TUNING_DATA_AI_CN.bin"
#define LDC_TUNING_DATA_AI_K1_FILE_NAME_CN      "/vendor/etc/camera/LDC_FPC_TUNING_DATA_AI_K1_CN.bin"
#define LDC_PACKDATA_VENDORID_0x01_FILE_NAME_CN "/vendor/etc/camera/LDC_PACKDATA_VENDORID_0x01_CN.bin"
#define LDC_PACKDATA_VENDORID_0x03_FILE_NAME_CN "/vendor/etc/camera/LDC_PACKDATA_VENDORID_0x03_CN.bin"
#define LDC_PACKDATA_VENDORID_0x07_FILE_NAME_CN "/vendor/etc/camera/LDC_PACKDATA_VENDORID_0x07_CN.bin"
#define LDC_AI_SEGMETATION_MODEL_FILE_NAME      "/vendor/etc/camera/LDC_ALTEK_AI_SEGMETATION.bin"
#define LDC_AC_WORK_PATH                        "/data/vendor/camera/"


static S64 g_mutexId = 0;
// Global variables mark the properties of the singleton algo lib to be initialized.
static U32 g_format = 0xFF;
static U32 g_width = 0;
static U32 g_height = 0;
static U32 g_stride = 0;
static U32 g_sliceHeight = 0;
static U32 g_curMode = 0;
static S32 g_isLowCamLevel = property_get_int32("ro.boot.camera.config", -1);

LdcCapture::LdcCapture(U32 mode, U32 debug)
{
    Device->mutex->Create(&g_mutexId, MARK("g_mutexId"));
    if ((m_debug & LDC_DEBUG_INFO) != 0) {
       Device->detector->Create(&m_detectorRunId, MARK("m_detectorRunId"));
       Device->detector->Create(&m_detectorInitId, MARK("m_detectorInitId"));
    }
    m_captureMode = mode;
    m_debug = debug;
    m_libInitFlag = FALSE;
    m_lastReqHasFace = FALSE;
    m_hMiCaptureLDC = NULL;
    m_hMiMaskHandle = NULL;
    m_miMaskInitFlag = FALSE;
}

LdcCapture::~LdcCapture()
{
    S64 ret = RET_OK;
    DEV_LOGI("~LdcCapture() enter m_libInitFlag = %d", m_libInitFlag);
    Device->mutex->Lock(g_mutexId);
    if (m_libInitFlag == TRUE) {
        if (g_curMode == LDC_CAPTURE_MODE_ALTEK_DEF) {
            alhLDC_Deinit();
            DEV_LOGI("~LdcCapture() alhLDC_Deinit()");
        }
        if (g_curMode == LDC_CAPTURE_MODE_ALTEK_AI) {
            alAILDC_Buffer_Deinit(1);
            DEV_LOGI("~LdcCapture() alAILDC_Buffer_Deinit()");
        }

        g_width = 0;
        g_height = 0;
        g_stride = 0;
        g_sliceHeight = 0;
        g_format = 0xFF;
        m_libInitFlag = FALSE;
    }
    if (g_curMode == LDC_CAPTURE_MODE_ALTEK_AI) {
#if defined(ENABLE_FACE_MASK_MIALGOSEG)
      if((LDC_FACE_MASK_MIALGOSEG == FACE_MASK_TYPE && TRUE == m_miMaskInitFlag)) {
         ret = MIALGO_LDCSegDestory(&m_hMiMaskHandle);
         DEV_IF_LOGI((0 == ret), "MIMASK: SegDestory Done.");
         DEV_IF_LOGW((0 != ret), "MIMASK: SegDestory failed.");
      }
#endif
    }
    Device->mutex->Unlock(g_mutexId);

    //m_captureMode = LDC_CAPTURE_MODE_MIPHONE;
    m_debug = 0;
    if ((m_debug & LDC_DEBUG_INFO) != 0) {
       Device->detector->Destroy(&m_detectorRunId);
       Device->detector->Destroy(&m_detectorInitId);
    }
    DEV_LOGI("~LdcCapture() exit m_libInitFlag = %d", m_libInitFlag);
    // Device->mutex->Destroy(g_mutexId);
}


S64 LdcCapture::GetPackData(U32 vendorId, void **ppData0, U64 *pSize0, void **ppData1, U64 *pSize1)
{
    DEV_IF_LOGE_RETURN_RET((NULL == pSize0), RET_ERR_ARG, "pSize1 is null");
    DEV_IF_LOGE_RETURN_RET((NULL == pSize1), RET_ERR_ARG, "pSize1 is null");
    S64 ret = RET_OK;
    S64 packDataSize = 0;
    void *packDataBuf = NULL;
    void *tuningBuf = NULL;
    S64 tuningSize = 0;
    char packDataFileName[128] = LDC_PACKDATA_VENDORID_0x01_FILE_NAME;
    char tuningDataFileName[128] = LDC_TUNING_DATA_FILE_NAME;
#if defined(STAR_CAM)
    S32 cameraConfig = property_get_int32("ro.boot.camera.config", 1);
    CHAR regionProp[PROPERTY_VALUE_MAX] = {0};
    //property_get("ro.miui.build.region", regionProp, "0");
    if (m_captureMode == LDC_CAPTURE_MODE_ALTEK_AI) {
        if (strcmp(regionProp, "cn") == 0 || strcmp(regionProp, "in") == 0) {
          sprintf(tuningDataFileName, "%s", LDC_TUNING_DATA_AI_FILE_NAME_CN);
        } else {
          sprintf(tuningDataFileName, "%s", LDC_TUNING_DATA_AI_FILE_NAME);
        }
    }
#else
    if (m_captureMode == LDC_CAPTURE_MODE_ALTEK_AI) {
        sprintf(tuningDataFileName, "%s", LDC_TUNING_DATA_AI_FILE_NAME);
    }
#endif
    tuningSize = Device->file->GetSize(tuningDataFileName);
    DEV_IF_LOGE_RETURN_RET((0 == tuningSize), RET_ERR, "get file err");
    tuningBuf = malloc(tuningSize);
    DEV_IF_LOGE_RETURN_RET((NULL == tuningBuf), RET_ERR, "malloc err");
    ret = Device->file->ReadOnce(tuningDataFileName, tuningBuf, tuningSize, 0);
    DEV_IF_LOGE_RETURN_RET((ret != tuningSize), RET_ERR, "read file err");
    DEV_IF_LOGI((ret == tuningSize), "LDC PACK FILE[%s]", tuningDataFileName);
#if defined(STAR_CAM)
    if (vendorId == 0x03) {
        if (strcmp(regionProp, "cn") == 0 || strcmp(regionProp, "in") == 0) {
            sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x03_FILE_NAME_CN);
        } else {
            sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x03_FILE_NAME);
        }
    } else if (vendorId == 0x07) {
        if (strcmp(regionProp, "cn") == 0 || strcmp(regionProp, "in") == 0) {
            sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x07_FILE_NAME_CN);
        } else {
            sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x07_FILE_NAME);
        }
    } else {
        if (strcmp(regionProp, "cn") == 0 || strcmp(regionProp, "in") == 0) {
            sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x01_FILE_NAME_CN);
        } else {
            sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x01_FILE_NAME);
        }
    }
#else
    if (vendorId == 0x03) {
        sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x03_FILE_NAME);
    } else if (vendorId == 0x07) {
        sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x07_FILE_NAME);
    } else {
        sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x01_FILE_NAME);
    }
#endif
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

S64 LdcCapture::GetFileData(const char *fileName, void **ppData0, U64 *pSize0) {
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


S64 LdcCapture::FreeFileData(void **ppData0) {
    if (ppData0 != NULL) {
        if (*ppData0) {
            free(*ppData0);
            *ppData0 = NULL;
        }
    }
    return RET_OK;
}

// Should hold mutex g_mutexId before calling this func
S64 LdcCapture::InitLib(U32 inWidth, U32 inHeight, U32 inStrideW, U32 inStrideH, U32 outWidth,
                        U32 outHeight, U32 outStrideW, U32 outStrideH, U32 format, U32 vendorId)
{
    S64 ret = 0;
    char version[512] = {0};
    DEV_LOGI("InitLib() enter m_libInitFlag = %d", m_libInitFlag);
    if ((m_debug & LDC_DEBUG_INFO) != 0) {
        Device->detector->Begin(m_detectorInitId);
    }
    if (m_libInitFlag == TRUE) {
        if (g_curMode == LDC_CAPTURE_MODE_ALTEK_DEF) {
            ret = alhLDC_Deinit();
            DEV_IF_LOGE((ret != RET_OK), "alhLDC_Deinit failed!");
            DEV_LOGI("InitLib() alhLDC_Deinit()");
        }
        if (g_curMode == LDC_CAPTURE_MODE_ALTEK_AI) {
            ret = alAILDC_Buffer_Deinit(1);
            DEV_IF_LOGE((ret != RET_OK), "alAILDC_Buffer_Deinit failed!");
            DEV_LOGI("InitLib() alAILDC_Buffer_Deinit()");
        }
        g_format = 0xFF;
        m_libInitFlag = FALSE;
    }


    if (m_captureMode == LDC_CAPTURE_MODE_ALTEK_DEF) {
        void *packDataBuf = NULL;
        U64 packDataSize = 0;
        void *tuningBuf = NULL;
        U64 tuningSize = 0;
        ret |= GetPackData(vendorId, &packDataBuf, &packDataSize, &tuningBuf, &tuningSize);
        ret |= alhLDC_VersionInfo_Get(version, sizeof(version));
        ret |= alhLDC_Set_KernelMethod(ALHLDC_METHOD_MEM_OPT);
        ret |= alhLDC_Init(inWidth, inHeight, outWidth, outHeight, inStrideW, inStrideH, outStrideW,
                           outStrideH, (unsigned char *)packDataBuf, packDataSize,
                           (unsigned char *)tuningBuf, tuningSize);
        FreePackData(&packDataBuf, &tuningBuf);
    } else if (m_captureMode == LDC_CAPTURE_MODE_ALTEK_AI){

#if defined(ENABLE_FACE_MASK_AILAB) || defined(ENABLE_FACE_MASK_MIALGOSEG)
       if((FACE_MASK_TYPE == LDC_FACE_MASK_AILAB) ||
          (FACE_MASK_TYPE == LDC_FACE_MASK_MIALGOSEG)){
         void *packDataBuf = NULL;
         U64 packDataSize = 0;
         void *tuningBuf = NULL;
         U64 tuningSize = 0;
         ret |= GetPackData(vendorId, &packDataBuf, &packDataSize, &tuningBuf, &tuningSize);
         alAILDC_IMG_FORMAT a_nImgFormat = GetMiLDCFormat(format);
         ret = alAILDC_VersionInfo_Get(version, sizeof(version));
         ret |= alAILDC_Buffer_Init(1, inWidth, inHeight, outWidth, outHeight, inStrideW, inStrideH,
                                   outStrideW, outStrideH, (alAILDC_IMG_FORMAT)a_nImgFormat,
                                   (unsigned char *)packDataBuf, packDataSize,
                                   (unsigned char *)tuningBuf, tuningSize);
         FreePackData(&packDataBuf, &tuningBuf);
       }
#endif
#if defined(ENABLE_FACE_MASK_ALTEK)
       if(FACE_MASK_TYPE == LDC_FACE_MASK_ALTEK){
         void *packDataBuf = NULL;
         U64 packDataSize = 0;
         void *tuningBuf = NULL;
         U64 tuningSize = 0;
         void *segDataBuf = NULL;
         U64 segDataSize = 0;
         char acWorkPath[1024] = LDC_AC_WORK_PATH;
         alAILDC_IMG_FORMAT a_nImgFormat = GetMiLDCFormat(format);
         ret |= alAILDC_VersionInfo_Get(version, sizeof(version));
         ret |= GetPackData(vendorId, &packDataBuf, &packDataSize, &tuningBuf, &tuningSize);
         ret |= GetFileData(LDC_AI_SEGMETATION_MODEL_FILE_NAME, &segDataBuf,  &segDataSize);
         ret |= alAILDC_Set_Working_Path(acWorkPath, 100);
         ret |= alAILDC_Buffer_Init_v2(1, inWidth, inHeight, outWidth, outHeight, inStrideW, inStrideH,
                                   outStrideW, outStrideH, (alAILDC_IMG_FORMAT)a_nImgFormat,
                                   (unsigned char *)packDataBuf, packDataSize,
                                   (unsigned char *)tuningBuf, tuningSize,
                                   (unsigned char *)segDataBuf, segDataSize);
         FreePackData(&packDataBuf, &tuningBuf);
         FreeFileData(&segDataBuf);

      }
#endif
       //else {
       //  DEV_LOGE("LDC err not suport LDC FACE SEGMETATION MODE  FACE_MASK_TYPE = %d", FACE_MASK_TYPE);
       //  ret = RET_ERR;
       //}

    } else {
      DEV_LOGE("LDC Capture g_curMode err %d", g_curMode);
      ret = RET_ERR;
    }

    if ((m_debug & LDC_DEBUG_INFO) != 0) {
        if (m_captureMode == LDC_CAPTURE_MODE_ALTEK_DEF) {
            alhLDC_Set_LogLevel(ALHLDC_LEVEL_2);
        }
        if (m_captureMode == LDC_CAPTURE_MODE_ALTEK_AI) {
            alAILDC_Set_Flag(alAILDC_LEVEL_2);
        }
    }


    if (ret == RET_OK) {
        m_libInitFlag = TRUE;
        g_format = format;
        g_width = inWidth;
        g_height = inHeight;
        g_stride = inStrideW;
        g_sliceHeight = inStrideH;
        g_curMode = m_captureMode;
    }

    if ((m_debug & LDC_DEBUG_INFO) != 0) {
        Device->detector->End(m_detectorInitId, "LDC-CAPTURE_INIT");
    }
    DEV_LOGI("InitLib() exit m_libInitFlag = %d", m_libInitFlag);

    DEV_LOGI(
        "LDC Capture INIT0 [%" PRIi64
        "] (Ver:%s) mode=%d inW=%d inH=%d (SW=%d SH=%d) outW=%d outH=%d (SW=%d SH=%d) format=%d",
        ret, version, m_captureMode, inWidth, inHeight, inStrideW, inStrideH, outWidth, outHeight,
        outStrideW, outStrideH, g_format);
    return ret;
}

S32 LdcCapture::PreProcess()
{
    S64 ret = RET_OK;
    DEV_LOGI("PreProcess() enter");
    Device->mutex->Lock(g_mutexId);
    if (g_curMode == LDC_CAPTURE_MODE_ALTEK_AI){
      DEV_LOGI("MiBokehLdc: PreProcess +++.");
     // 1.Burstshot this function is not called;
     // 2.If the last request faceNum > 0, use this function early init;

#if defined(ENABLE_FACE_MASK_AILAB)
     if (FACE_MASK_TYPE == LDC_FACE_MASK_AILAB && TRUE == m_lastReqHasFace) {
        MiBokehLdc::GetInstance()->AddRef();
     }
#endif

    }
    Device->mutex->Unlock(g_mutexId);
    DEV_LOGI("PreProcess() exit");
    return ret;
}

S64 LdcCapture::Process(LdcProcessInputInfo &inputInfo)
{
    // lock to prevent re-entry, especially when deinit triggered while another LDC is processing.
    DEV_LOGI("Process()  enter m_libInitFlag = %d", m_libInitFlag);
    Device->mutex->Lock(g_mutexId);
    S64 ret = RET_OK;
    alhLDC_RECT pa_ptInZoomWOI;
    alhLDC_FACE_INFO alhInFaceInfo;
    DEV_IMAGE_BUF *input = inputInfo.inputBuf;
    DEV_IMAGE_BUF *output = inputInfo.outputBuf;
    DEV_LOGI("process mode=%d inW=%d inH=%d (SW=%d SH=%d) outW=%d outH=%d (SW=%d SH=%d) format=%d vendorId = %d",
        m_captureMode, input->width, input->height, input->stride[0], input->sliceHeight[0],
        output->width, output->height, output->stride[0], output->sliceHeight[0],
        input->format, inputInfo.vendorId);

    if ((g_width != input->width) || (g_height != input->height) ||
        (g_stride != input->stride[0]) || (g_sliceHeight != input->sliceHeight[0]) ||
        (g_format != input->format) || (g_curMode != m_captureMode)) {
        ret = InitLib(input->width, input->height, input->stride[0], input->sliceHeight[0],
                      output->width, output->height, output->stride[0], output->sliceHeight[0],
                      input->format, inputInfo.vendorId);
    }
    if (RET_OK != ret || TRUE != m_libInitFlag) {
        Device->mutex->Unlock(g_mutexId);
        DEV_IF_LOGE_RETURN_RET(TRUE, RET_ERR, "InitLib error");
    }

    // Burstshot does not protect face.
    if (TRUE == inputInfo.isBurstShot) {
        inputInfo.faceInfo.num = 0;
    }

    DEV_LOGI("process ZoomWOI (%d, %d, %d, %d)",inputInfo.zoomWOI.left, inputInfo.zoomWOI.top,
             inputInfo.zoomWOI.width, inputInfo.zoomWOI.height);
    if (inputInfo.faceInfo.num > 0){
      DEV_LOGI("process Face (%d, %d, %d, %d) score = %d orient = %d",
         inputInfo.faceInfo.face[0].left, inputInfo.faceInfo.face[0].top,
         inputInfo.faceInfo.face[0].width, inputInfo.faceInfo.face[0].height,
           inputInfo.faceInfo.score[0], inputInfo.faceInfo.orient[0]);
    }
    //algo alhLDC Altek
    if (g_curMode == LDC_CAPTURE_MODE_ALTEK_DEF) {
       alhLDC_RECT pa_ptInZoomWOI;
       alhLDC_FACE_INFO alhInFaceInfo;

       //set ZoomWOI
       pa_ptInZoomWOI.x = inputInfo.zoomWOI.left;
       pa_ptInZoomWOI.y = inputInfo.zoomWOI.top;
       pa_ptInZoomWOI.width = inputInfo.zoomWOI.width;
       pa_ptInZoomWOI.height = inputInfo.zoomWOI.height;

       //set faces
       alhInFaceInfo.count = inputInfo.faceInfo.num;
       alhInFaceInfo.src_img_height = input->height;
       alhInFaceInfo.src_img_width = input->width;
       for (int i = 0; i < alhInFaceInfo.count; i++) {
         alhInFaceInfo.face[i].score = inputInfo.faceInfo.score[i];
         alhInFaceInfo.face[i].roi.height = inputInfo.faceInfo.face[i].height;
         alhInFaceInfo.face[i].roi.width = inputInfo.faceInfo.face[i].width;
         alhInFaceInfo.face[i].roi.x = inputInfo.faceInfo.face[i].left;
         alhInFaceInfo.face[i].roi.y = inputInfo.faceInfo.face[i].top;
        }

        //alhLDC run
        if ((m_debug & LDC_DEBUG_INFO) != 0) {
          Device->detector->Begin(m_detectorRunId);
        }
        ret = alhLDC_Run((unsigned char *)input[0].plane[0], (unsigned char *)input[0].plane[1],
                         (unsigned char *)output[0].plane[0], (unsigned char *)output[0].plane[1],
                         0, NULL /*rotateMat*/, (alhLDC_RECT *)&pa_ptInZoomWOI, &alhInFaceInfo);
        if ((m_debug & LDC_DEBUG_INFO) != 0) {
           Device->detector->End(m_detectorRunId, "LDC-CAPTURE_RUN");
        }
    } else if (m_captureMode == LDC_CAPTURE_MODE_ALTEK_AI){
        alAILDC_RECT pa_ptInZoomWOI;
        alAILDC_FACE_INFO alAiInFaceInfo;
        alAILDC_SET_SCREEN_ORIENTATION screenOrientation = alAILDC_SCREEN_ANGLE_0;
        //set ZoomWOI
        pa_ptInZoomWOI.x = inputInfo.zoomWOI.left;
        pa_ptInZoomWOI.y = inputInfo.zoomWOI.top;
        pa_ptInZoomWOI.width = inputInfo.zoomWOI.width;
        pa_ptInZoomWOI.height = inputInfo.zoomWOI.height;

        //set faces
        alAiInFaceInfo.count = inputInfo.faceInfo.num;
        if(inputInfo.faceInfo.num > 0){
          alAiInFaceInfo.src_img_height = input->height;
          alAiInFaceInfo.src_img_width = input->width;
          for (int i = 0; i < alAiInFaceInfo.count; i++) {
            alAiInFaceInfo.face[i].score = inputInfo.faceInfo.score[i];
            alAiInFaceInfo.face[i].roi.height = inputInfo.faceInfo.face[i].height;
            alAiInFaceInfo.face[i].roi.width = inputInfo.faceInfo.face[i].width;
            alAiInFaceInfo.face[i].roi.x = inputInfo.faceInfo.face[i].left;
            alAiInFaceInfo.face[i].roi.y = inputInfo.faceInfo.face[i].top;
          }

            //set face angle
          if (inputInfo.faceInfo.orient[0] == 0) {
            screenOrientation = alAILDC_SCREEN_ANGLE_0;
          } else if (inputInfo.faceInfo.orient[0] == 90) {
            screenOrientation = alAILDC_SCREEN_ANGLE_90;
          } else if (inputInfo.faceInfo.orient[0] == 180) {
            screenOrientation = alAILDC_SCREEN_ANGLE_180;
          } else if (inputInfo.faceInfo.orient[0] == 270) {
            screenOrientation = alAILDC_SCREEN_ANGLE_270;
          }
        }

        //algo mask AILAB aiLDC aiAltek
#if defined(ENABLE_FACE_MASK_AILAB)
        if (FACE_MASK_TYPE == LDC_FACE_MASK_AILAB) {
          DEV_IMAGE_BUF maskDepthoutput = {0};
          if (inputInfo.faceInfo.num > 0) {
            int maskRet = RET_OK;
            //get AILAB face mask
            MiBokehLdc::GetInstance()->AddRefIfZero();
            maskRet = Device->image->Alloc(
                &maskDepthoutput, input->width, input->width, input->height, input->height, 0,
                (DEV_IMAGE_FORMAT)DEV_IMAGE_FORMAT_Y8, MARK("maskDepthoutput"));
            DEV_IF_LOGE((maskRet != RET_OK), "alloc maskDepthoutput ERR");
            if (maskRet == RET_OK) {
                double startTime = PluginUtils::nowMSec();
                maskRet = MiBokehLdc::GetInstance()->GetDepthMask(input, &maskDepthoutput,
                                                                  inputInfo.faceInfo.orient[0], 0);
                if (maskRet != RET_OK) {
                    Device->image->Free(&maskDepthoutput);
                    DEV_LOGE("MiBokehLdc: GetDepthMask Error.");
                } else {
                    DEV_LOGI("MiBokehLdc: GetDepthMask Done, cost %.2f ms.",
                             PluginUtils::nowMSec() - startTime);
                }
            }
            if (((m_debug & LDC_DEBUG_DUMP_IMAGE_CAPTURE) != 0) && maskRet == RET_OK) {
                Device->image->DumpImage(&maskDepthoutput, LOG_TAG, 0, "maskDepthoutput.out.Y8");
            }
            // Get facemask error, do once non-faceprotect process.
            if (maskRet != RET_OK) {
                inputInfo.faceInfo.num = 0;
            }
           //set AILAB face mask
            ret = alAILDC_Set_SubjectMask(1, (unsigned char *)maskDepthoutput.plane[0],
                                              maskDepthoutput.width, maskDepthoutput.height);
            if (ret != RET_OK) {
                Device->image->Free(&maskDepthoutput);
                DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "alAILDC_Set_SubjectMask ERR");
            }
            MiBokehLdc::GetInstance()->ReleaseRef();
            ret = alAILDC_Set_ScreenOrientation(1, screenOrientation);
            if (ret != RET_OK) {
              Device->image->Free(&maskDepthoutput);
              DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "alAILDC_Set_ScreenOrientation ERR");
            }
          }
          //aiAILDC run AILAB
          if ((m_debug & LDC_DEBUG_INFO) != 0) {
             Device->detector->Begin(m_detectorRunId);
          }
          ret = alAILDC_Buffer_Run(
              1, (unsigned char *)input[0].plane[0], (unsigned char *)input[0].plane[1],
              (unsigned char *)output[0].plane[0], (unsigned char *)output[0].plane[1], 0,
              NULL /*rotateMat*/, (alAILDC_RECT *)&pa_ptInZoomWOI,
              (alAILDC_FACE_INFO *)&alAiInFaceInfo, NULL);
          if ((m_debug & LDC_DEBUG_INFO) != 0) {
              Device->detector->End(m_detectorRunId, "LDC-CAPTURE_RUN");
          }
          if (inputInfo.faceInfo.num > 0) {
            Device->image->Free(&maskDepthoutput);
          }
          DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "alAILDC AILAB run run ERR");
        //algo mask MISEG aiLDC Altek
        }
#endif

#if defined(ENABLE_FACE_MASK_MIALGOSEG)
         if (FACE_MASK_TYPE == LDC_FACE_MASK_MIALGOSEG) {
           DEV_IMAGE_BUF maskDepthoutput = {0};
           if (inputInfo.faceInfo.num > 0) {
            DEV_IMAGE_BUF maskDepthoutput = {0};
            int maskRet = RET_OK;

            if (FALSE == m_miMaskInitFlag) {
              int maskInitRet = 0;
              //get miseg face mask
              const char *maskVersion = MIALGO_LDCSegModelVersionGet();
              DEV_IF_LOGW((maskVersion == NULL), "MIMASK: VersionGet failed.");
              DEV_IF_LOGI((maskVersion != NULL), "MIMASK: maskVersion = %s", maskVersion);

              LDCSegModelInitParams initPara = {
                .config_path = "/vendor/etc/camera/mialgo_seg_ldc_parameters.json",
                .srcImageWidth = static_cast<int>(input->width),
                .srcImageHeight = static_cast<int>(input->height),
                .runtime = AI_VISION_RUNTIME_DSP_AIO,
                .priority = AI_VISION_PRIORITY_HIGH_AIO,
                .performance = AI_VISION_PERFORMANCE_HIGH_AIO};
              DEV_LOGI("MIMASK: Init width:%d height:%d runtime:%d priority:%d performance:%d",
                     initPara.srcImageWidth, initPara.srcImageHeight, initPara.runtime,
                     initPara.priority, initPara.performance);
              maskInitRet = MIALGO_LDCSegInitWhenLaunch(&m_hMiMaskHandle, &initPara);
              DEV_IF_LOGI((0 == maskInitRet), "MIMASK: InitWhenLaunch Done");
              DEV_IF_LOGW((0 != maskInitRet), "MIMASK: InitWhenLaunch failed.");
              if (maskInitRet == RET_OK) {
                m_miMaskInitFlag = TRUE;
              }

              if (m_miMaskInitFlag == FALSE) {
                inputInfo.faceInfo.num = 0;
              }
            }
            if ((FACE_MASK_TYPE == LDC_FACE_MASK_MIALGOSEG) || (TRUE == m_miMaskInitFlag)) {
               maskRet = GetMaskFromMiSeg(inputInfo, &maskDepthoutput);
               // Get facemask error, do once non-faceprotect process.
               if (maskRet != RET_OK) {
                  inputInfo.faceInfo.num = 0;
               }
            }

            if (((m_debug & LDC_DEBUG_DUMP_IMAGE_CAPTURE) != 0) && maskRet == RET_OK) {
                Device->image->DumpImage(&maskDepthoutput, LOG_TAG, 0, "maskDepthoutput.out.Y8");
            }
            // Get facemask error, do once non-faceprotect process.
            if (maskRet != RET_OK) {
                inputInfo.faceInfo.num = 0;
            }
           //set AILAB face mask
            ret = alAILDC_Set_SubjectMask(1, (unsigned char *)maskDepthoutput.plane[0],
                                          maskDepthoutput.width, maskDepthoutput.height);
            if (ret != RET_OK) {
                Device->image->Free(&maskDepthoutput);
                DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "alAILDC_Set_SubjectMask ERR");
            }

            MiBokehLdc::GetInstance()->ReleaseRef();

            ret = alAILDC_Set_ScreenOrientation(1, screenOrientation);
            if (ret != RET_OK) {
              Device->image->Free(&maskDepthoutput);
              DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "alAILDC_Set_ScreenOrientation ERR");
            }
          }
          //algo run
          if ((m_debug & LDC_DEBUG_INFO) != 0) {
            Device->detector->Begin(m_detectorRunId);
          }

          ret = alAILDC_Buffer_Run(
            1, (unsigned char *)input[0].plane[0], (unsigned char *)input[0].plane[1],
               (unsigned char *)output[0].plane[0], (unsigned char *)output[0].plane[1], 0,
              NULL /*rotateMat*/, (alAILDC_RECT *)&pa_ptInZoomWOI,
              (alAILDC_FACE_INFO *)&alAiInFaceInfo, NULL);
          if ((m_debug & LDC_DEBUG_INFO) != 0) {
             Device->detector->End(m_detectorRunId, "LDC-CAPTURE_RUN");
          }
          if (inputInfo.faceInfo.num > 0) {
            Device->image->Free(&maskDepthoutput);
          }
          DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "alAILDC miseg run run ERR");
        //algo mask ALTEK aiLDC Altek
        }
#endif
#if defined(ENABLE_FACE_MASK_ALTEK)

       	if (FACE_MASK_TYPE == LDC_FACE_MASK_ALTEK){
           if (inputInfo.faceInfo.num > 0) {
               ret = alAILDC_Set_ScreenOrientation(1, screenOrientation);
               if (ret != RET_OK) {
                 DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "alAILDC_Set_ScreenOrientation ERR");
               }
           }
            /*
            ret = alAILDC_Set_SubjectMask(1, (unsigned char *)maskDepthoutput.plane[0],
                                              maskDepthoutput.width, maskDepthoutput.height);
            if (ret != RET_OK) {
             DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "alAILDC_Set_SubjectMask ERR");
            }
            */
            if ((m_debug & LDC_DEBUG_INFO) != 0) {
              Device->detector->Begin(m_detectorRunId);
            }
            ret = alAILDC_Buffer_Run(1, (unsigned char *)input[0].plane[0], (unsigned char *)input[0].plane[1],
              (unsigned char *)output[0].plane[0], (unsigned char *)output[0].plane[1], 0,NULL /*rotateMat*/,
              (alAILDC_RECT *)&pa_ptInZoomWOI, (alAILDC_FACE_INFO *)&alAiInFaceInfo, NULL);
            if ((m_debug & LDC_DEBUG_INFO) != 0) {
              Device->detector->End(m_detectorRunId, "LDC-CAPTURE_RUN");
            }
            if (ret != RET_OK) {
              DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "alAILDC altek mask run ERR");
            }
        }
#endif
    }
exit:
    DEV_LOGI("LDC-Info(capture#3):EXIT m_libInitFlag = %d.", m_libInitFlag);
    DEV_IF_LOGE((ret != RET_OK), "LDC Capture RUN ERR");
    Device->mutex->Unlock(g_mutexId);
    return ret;
}

alAILDC_IMG_FORMAT LdcCapture::GetMiLDCFormat(int srcFormat)
{
    alAILDC_IMG_FORMAT miFormat = alAILDC_IMAGE_FORMAT_NV21;
    switch (srcFormat) {
    case DEV_IMAGE_FORMAT_YUV420NV12:
        miFormat = alAILDC_IMAGE_FORMAT_NV12;
        break;
    case DEV_IMAGE_FORMAT_YUV420NV21:
        miFormat = alAILDC_IMAGE_FORMAT_NV21;
        break;
    case DEV_IMAGE_FORMAT_YUV420P010:
        miFormat = alAILDC_IMAGE_FORMAT_P010_MSB;
        //alAILDC_IMAGE_FORMAT_P010_LSB;
        break;
    default:
        miFormat = alAILDC_IMAGE_FORMAT_NV21;
    }
    return miFormat;
}
#if defined(ENABLE_FACE_MASK_MIALGOSEG)
S32 LdcCapture::GetMaskFromMiSeg(LdcProcessInputInfo &inputInfo, DEV_IMAGE_BUF *maskDepthoutput)
{
    int ret = RET_OK;
    DEV_IMAGE_BUF *input = inputInfo.inputBuf;

    LDCSegModelProcParams procPara = {
        .rotation_angle = static_cast<int>(inputInfo.faceInfo.orient[0]),
        .srcImage.imgWidth = static_cast<int>(input->width),
        .srcImage.imgHeight = static_cast<int>(input->height),
        .srcImage.pYData = input->plane[0],
        .srcImage.pUVData = input->plane[1],
        .srcImage.stride = static_cast<int>(input->stride[0]),
        .srcImage.img_format = MIALGO_IMG_NV12_AIO};
    DEV_LOGI("MIMASK: procIn angle:%d width:%d height:%d stride:%d", procPara.rotation_angle,
             procPara.srcImage.imgWidth, procPara.srcImage.imgHeight, procPara.srcImage.stride);
    double startTime = PluginUtils::nowMSec();
    ret = MIALGO_LDCSegProc(&m_hMiMaskHandle, &procPara);
    DEV_IF_LOGE_RETURN_RET((0 != ret), ret, "MIMASK: LDCSegProc failed.");
    DEV_IF_LOGI((0 == ret), "MIMASK: LDCSegProc Done, cost %.2fms.",
                PluginUtils::nowMSec() - startTime);
    DEV_LOGI("MIMASK: procOut ptr = %p, (%d,%d) stride:%d", procPara.potraitMask.pYData,
             procPara.potraitMask.imgWidth, procPara.potraitMask.imgHeight,
             procPara.potraitMask.stride);
    ret = Device->image->Alloc(maskDepthoutput, procPara.potraitMask.imgWidth,
                               procPara.potraitMask.imgWidth, procPara.potraitMask.imgHeight,
                               procPara.potraitMask.imgHeight, 0,
                               (DEV_IMAGE_FORMAT)DEV_IMAGE_FORMAT_Y8, MARK("maskDepthoutput"));
    DEV_IF_LOGE_RETURN_RET((0 != ret), ret, "MIMASK: Alloc failed.");
    DEV_IMAGE_BUF inputMask;
    inputMask.width = procPara.potraitMask.imgWidth;
    inputMask.height = procPara.potraitMask.imgHeight;
    inputMask.stride[0] = procPara.potraitMask.imgWidth;
    inputMask.format = DEV_IMAGE_FORMAT_Y8;
    inputMask.plane[0] = (char *)procPara.potraitMask.pYData;
    ret = Device->image->Copy(maskDepthoutput, &inputMask);
    if ((m_debug & LDC_DEBUG_DUMP_IMAGE_CAPTURE) != 0) {
        Device->image->DumpImage(maskDepthoutput, "LDC-PLUGIN", inputInfo.frameNum,
                                 "maskDepthoutput.Y8");
    }
    return ret;
}
#endif
} // namespace mialgo2
