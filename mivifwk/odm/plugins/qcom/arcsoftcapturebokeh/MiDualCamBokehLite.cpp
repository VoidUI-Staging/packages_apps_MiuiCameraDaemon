#include "MiDualCamBokehLite.h"

#include <MiaPluginUtils.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "asvloffscreen.h"

using namespace mialgo2;

#define CAL_DATA_PATH          "/data/vendor/camera/"
#define CAL_DATA_LITE_PATH_W_T "/vendor/etc/camera/com.xiaomi.dcallite.wt.data"
#define CAL_DATA_LITE_PATH_W_U "/vendor/etc/camera/com.xiaomi.dcallite.wu.data"
#define CAL_DATA_PATH_FRONT    "/data/vendor/camera/com.xiaomi.dcal.ffaux.data"
#define JSON_PATH              "/vendor/etc/camera/dualcam_bokeh_params.json"

#define CALIBRATIONDATA_LEN 4096

#define ROLE_REARBOKEH2X 61
#define ROLE_REARBOKEH1X 63

#define SHOW_WIDE_ONLY 1
#define SHOW_TELE_ONLY 2
#define SHOW_HALF_HALF 3

#define ALGO_CAPTURE_BOKEH_ARRANGEMENT 45

#define MATHL_MAX(a, b) ((a) > (b) ? (a) : (b))
#define MATH_MIN(a, b)  ((a) < (b) ? (a) : (b))
#define SAFE_FREE(ptr) \
    if (ptr) {         \
        free(ptr);     \
        ptr = NULL;    \
    }

static const int32_t m_bDumpYUV = property_get_int32("persist.vendor.camera.mialgo.cb.dumpimg", 0);
static const int32_t m_nViewCtrl =
    property_get_int32("persist.vendor.camera.mialgo.cb.debugview", 0);
static const int32_t m_bDrawROI = property_get_int32("persist.vendor.camera.mialgo.cb.drawroi", 0);
static const int32_t m_DebugMode =
    property_get_int32("persist.vendor.camera.mialgo.cb.debugmode", 0);

typedef struct tag_disparity_fileheader_t
{
    int32_t i32Header;
    int32_t i32HeaderLength;
    int32_t i32PointX;
    int32_t i32PointY;
    int32_t i32BlurLevel;
    int32_t reserved_DepthWidth;
    int32_t reserved_DepthHeight;
    int32_t i32Version;  // Agreement with app
    int32_t i32VendorID; // 0:xiaomi self   1:arcsoft
    int32_t mappingVersion;
    int32_t ShineThres;
    int32_t ShineLevel;
    int32_t reserved[25];
    int32_t i32DataLength;
} disparity_fileheader_t;

typedef struct
{
    int orient;
    int mode;
    MIIMAGEBUFFER pSrcImg;
    MIIMAGEBUFFER pDstImg;
    MIIMAGEBUFFER pBokehImg;
    MHandle *m_hRelightingHandle;
} _tlocal_thread_params;

typedef struct
{
    // int m_mapped_orient;
    int i32Width;  // [in] The image width
    int i32Height; // [in] The image height
    MHandle *m_phRelightingHandle;
} relight_thread_params;

static bool m_useBokeh = false;

struct bokeh_facebeauty_params
{
    LPASVLOFFSCREEN pImage;
    BeautyFeatureParams beautyFeatureParams;
    // int facebeautyLevel;
    float gender;
    int orient;
    int rearCamera;
};

// used for performance
static long long __attribute__((unused)) gettime()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((long long)time.tv_sec * 1000 + time.tv_usec / 1000);
}

int get_device_region_lite()
{
    char devicePtr[PROPERTY_VALUE_MAX];
    property_get("ro.boot.hwc", devicePtr, "0");
    if (strcasecmp(devicePtr, "India") == 0)
        return BEAUTYSHOT_REGION_INDIAN;
    else if (strcasecmp(devicePtr, "CN") == 0)
        return BEAUTYSHOT_REGION_CHINA;

    return BEAUTYSHOT_REGION_GLOBAL;
}

static void *bokeh_facebeauty_thread_proc(void *params)
{
    bokeh_facebeauty_params *beautyParams = (bokeh_facebeauty_params *)params;
    LPASVLOFFSCREEN pImage = beautyParams->pImage;
    BeautyShot_Image_Algorithm *m_BeautyShot_Image_Algorithm = NULL;
    int orient = beautyParams->orient;
    // int facebeautyLevel = beautyParams->facebeautyLevel;
    int region_code = get_device_region_lite();
    int mfeatureCounts = beautyParams->beautyFeatureParams.featureCounts;

    MLOGD(Mia2LogGroupPlugin, "%s:%d [ARC_CTB_BTY] arcdebug bokeh orient=%d region=%d \n", __func__,
          __LINE__, orient, region_code);
    double t0, t1;
    t0 = gettime();

    if (Create_BeautyShot_Image_Algorithm(BeautyShot_Image_Algorithm::CLASSID,
                                          &m_BeautyShot_Image_Algorithm) != MOK ||
        m_BeautyShot_Image_Algorithm == NULL) {
        MLOGE(Mia2LogGroupPlugin,
              "%s:%d [ARC_CTB_BTY] arcdebug bokeh: Create_BeautyShot_Image_Algorithm, fail",
              __func__, __LINE__);
        return NULL;
    }

    MRESULT ret =
        m_BeautyShot_Image_Algorithm->Init(ABS_INTERFACE_VERSION, XIAOMI_FB_FEATURE_LEVEL);
    if (ret != MOK) {
        m_BeautyShot_Image_Algorithm->Release();
        m_BeautyShot_Image_Algorithm = NULL;
        return NULL;
    }

    for (unsigned int i = 0; i < mfeatureCounts; i++) {
        m_BeautyShot_Image_Algorithm->SetFeatureLevel(
            beautyParams->beautyFeatureParams.beautyFeatureInfo[i].featureKey,
            beautyParams->beautyFeatureParams.beautyFeatureInfo[i].featureValue);
        MLOGD(Mia2LogGroupPlugin, "[ARC_CTB_BTY]arcsoft debug beauty capture: %s = %d\n",
              beautyParams->beautyFeatureParams.beautyFeatureInfo[i].tagName,
              beautyParams->beautyFeatureParams.beautyFeatureInfo[i].featureValue);
    }

    ABS_TFaces faceInfoIn, faceInfoOut;
    ret = m_BeautyShot_Image_Algorithm->Process(pImage, pImage, &faceInfoIn, &faceInfoOut, orient,
                                                region_code);

    m_BeautyShot_Image_Algorithm->UnInit();
    m_BeautyShot_Image_Algorithm->Release();
    m_BeautyShot_Image_Algorithm = NULL;
    t1 = gettime();
    MLOGD(
        Mia2LogGroupPlugin,
        "%s:%d [ARC_CTB_BTY] arcdebug bokeh: beauty capture process YUV ret=%ld w:%d, h:%d, total "
        "time: %.2f",
        __func__, __LINE__, ret, pImage->i32Width, pImage->i32Height, t1 - t0);
    return NULL;
}

static void *bokeh_relighting_thread_proc_init(void *params)
{
    MLOGD(Mia2LogGroupPlugin, "[MI_CTB] relighting init thread start");
    int nRet = 0;
    double t0, t1;
    t0 = t1 = 0.0f;
    relight_thread_params *prtp = (relight_thread_params *)params;
    int width = prtp->i32Width;
    int height = prtp->i32Height;
    MHandle m_hRelightingHandle = NULL;
    m_useBokeh = false;

    if (m_hRelightingHandle) {
        MLOGE(Mia2LogGroupPlugin, "[MI_CTB] ERR! m_hRelightingHandle %p", m_hRelightingHandle);
        return NULL;
    }
    t0 = gettime();

    nRet = AILAB_PLI_Init(&m_hRelightingHandle, MI_PL_INIT_MODE_PERFORMANCE, width, height);
    if (nRet != MOK) {
        MLOGE(Mia2LogGroupPlugin, "[MI_CTB] midebug: AILAB_PLI_Init =failed= %d\n", nRet);
        m_useBokeh = true;
        return NULL;
    }

    t1 = gettime();
    *(prtp->m_phRelightingHandle) = m_hRelightingHandle;
    MLOGD(Mia2LogGroupPlugin,
          "[MI_CTB] midebug:AILAB_PLI_Init = %d: cost =%.2f, m_hRelightingHandle:%p\n", nRet,
          t1 - t0, m_hRelightingHandle);
    return NULL;
}

static void *bokeh_relighting_thread_proc_preprocess(void *params)
{
    MLOGD(Mia2LogGroupPlugin, "[MI_CTB] midebug: relighting process thread start");
    _tlocal_thread_params *pParam = (_tlocal_thread_params *)params;
    int nRet = 0;
    double t0, t1;
    t0 = t1 = 0.0f;
    MIPLLIGHTREGION mLightRegion = {0};
    int orientation = pParam->orient;
    MHandle m_hRelightingHandle = *(pParam->m_hRelightingHandle);
    MLOGI(Mia2LogGroupPlugin, "[MI_CTB] midebug: m_hRelightingHandle:%p", m_hRelightingHandle);

    m_useBokeh = false;
    t0 = gettime();

    MLOGD(Mia2LogGroupPlugin, "[MI_CTB] AILAB_PLI_PreProcess src i32Width %d i32Height %d ",
          pParam->pSrcImg.i32Width, pParam->pSrcImg.i32Height);
    MLOGD(Mia2LogGroupPlugin, "[MI_CTB] AILAB_PLI_PreProcess dst i32Width %d i32Height %d ",
          pParam->pDstImg.i32Width, pParam->pDstImg.i32Height);

    nRet = AILAB_PLI_PreProcess(m_hRelightingHandle, &pParam->pSrcImg, orientation, &mLightRegion);
    if (nRet != MOK) {
        MLOGE(Mia2LogGroupPlugin, "[MI_CTB] midebug: AILAB_PLI_PreProcess failed= %d\n", nRet);
        m_useBokeh = true;
        AILAB_PLI_Uninit(&m_hRelightingHandle);
        m_hRelightingHandle = NULL;
        *(pParam->m_hRelightingHandle) = NULL;
        return NULL;
    }

    t1 = gettime();

    MLOGD(Mia2LogGroupPlugin, "[MI_CTB] midebug:AILAB_PLI_PreProcess= %d: preprocess=%.2f \n", nRet,
          t1 - t0);
    return NULL;
}

MiDualCamBokeh::~MiDualCamBokeh() {}

int MiDualCamBokeh::Init(uint32_t FrameworkCameraId)
{
    m_hCaptureBokeh = NULL;
    m_pDisparityData = NULL;
    m_pCalibrationWU = NULL;
    m_pCalibrationWT = NULL;
    m_pCalibrationFront = NULL;
    m_pLogicalMetaData = NULL;
    m_hRelightingHandle = NULL;
    m_bDumpYUV = false;
    m_Initialized = false;
    m_hdrEnabled = false;
    m_isInitFaild = -1;
    m_bDrawROI = 0;
    m_nViewCtrl = 0;
    m_lightingMode = 0;
    m_BeautyEnabled = 0;
    m_CameraId = FrameworkCameraId;
    m_sceneMode = 0;
    m_sceneEnable = 0;
    m_blurLevel = 0;

    memset(&m_bokehframParams, 0, sizeof(bokeh_params_t));
    memset(&m_fovRectIFEForTele, 0, sizeof(MIRECT));
    memset(&m_fovRectIFEForWide, 0, sizeof(MIRECT));
    memset(&m_activeArraySizeWide, 0, sizeof(MIRECT));
    memset(&m_activeArraySizeTele, 0, sizeof(MIRECT));
    memset(&m_rtFocusROIOnInputData, 0, sizeof(MIRECT));

    if (CAM_COMBMODE_FRONT == m_CameraId || CAM_COMBMODE_FRONT_BOKEH == m_CameraId ||
        CAM_COMBMODE_FRONT_AUX == m_CameraId) {
        m_bRearCamera = false;
    } else {
        m_bRearCamera = true;
    }

    if (m_bRearCamera) {
        m_pCalibrationWU = malloc(CALIBRATIONDATA_LEN);
        m_pCalibrationWT = malloc(CALIBRATIONDATA_LEN);
        ReadCalibrationDataFromFile(CAL_DATA_LITE_PATH_W_U, m_pCalibrationWU, CALIBRATIONDATA_LEN);
        ReadCalibrationDataFromFile(CAL_DATA_LITE_PATH_W_T, m_pCalibrationWT, CALIBRATIONDATA_LEN);
    } else {
        m_pCalibrationFront = malloc(CALIBRATIONDATA_LEN);
        ReadCalibrationDataFromFile(CAL_DATA_PATH_FRONT, m_pCalibrationFront, CALIBRATIONDATA_LEN);
    }

    MLOGI(Mia2LogGroupPlugin, "[MI_CTB] Initialize m_bRearCamera:%d, cameraId: 0x%x", m_bRearCamera,
          m_CameraId);

    int beautyFeatureCounts = mBeautyFeatureParams.featureCounts;
    for (int i = 0; i < beautyFeatureCounts; i++) {
        mBeautyFeatureParams.beautyFeatureInfo[i].featureValue = 0;
    }

    return PROCSUCCESS;
}

int MiDualCamBokeh::deInit()
{
    MLOGD(Mia2LogGroupPlugin, "[MI_CTB] deInit E");

    if (m_hCaptureBokeh) {
        MIALGO_CaptureDestoryWhenClose(&m_hCaptureBokeh);
        m_hCaptureBokeh = NULL;
    }

    SAFE_FREE(m_pCalibrationWU);
    SAFE_FREE(m_pCalibrationWT);
    SAFE_FREE(m_pCalibrationFront);

    if (m_pDisparityData) {
        free(m_pDisparityData);
        m_pDisparityData = NULL;
        m_i32DisparityDataSize = 0;
        m_i32DisparityDataActiveSize = 0;
    }

    if (m_hRelightingHandle) {
        AILAB_PLI_Uninit(&m_hRelightingHandle);
        m_hRelightingHandle = NULL;
    }

    MLOGD(Mia2LogGroupPlugin, "[MI_CTB] deInit X");
    return PROCSUCCESS;
}

int MiDualCamBokeh::ReadCalibrationDataFromFile(const char *szFile, void *pCaliBuff,
                                                uint32_t dataSize)
{
    if (dataSize == 0 || NULL == pCaliBuff) {
        MLOGE(Mia2LogGroupPlugin, "[MI_CTB] (OUT) failed ! Invalid param");
        return PROCFAILED;
    }

    uint32_t bytes_read = 0;
    FILE *califile = NULL;
    califile = fopen(szFile, "r");
    if (NULL != califile) {
        bytes_read = fread(pCaliBuff, 1, dataSize, califile);
        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] read calidata bytes_read = %d, dataSize:%d, file: %s",
              bytes_read, dataSize, szFile);
        fclose(califile);
    } else {
        MLOGE(Mia2LogGroupPlugin, "[MI_CTB]open califile(%s) failed %s", szFile, strerror(errno));
        return PROCFAILED;
    }
    if (bytes_read != dataSize) {
        MLOGE(Mia2LogGroupPlugin, "[MI_CTB](OUT) failed ! Read incorrect size %d vs %d", bytes_read,
              dataSize);
        return PROCFAILED;
    }

    MLOGD(Mia2LogGroupPlugin, "[MI_CTB]ReadCalibrationDataFromFile success !");
    return PROCSUCCESS;
}

int MiDualCamBokeh::ProcessLiteBuffer(std::vector<struct MiImageBuffer *> teleInputvector,
                                      std::vector<struct MiImageBuffer *> wideInputvector,
                                      struct MiImageBuffer *bokehOutput,
                                      struct MiImageBuffer *teleOutput,
                                      struct MiImageBuffer *depthOut, bokeh_lite_params &liteParam)
{
    int result = PROCSUCCESS;
    double startTime = PluginUtils::nowMSec();
    bool bDumpVisible = PluginUtils::isDumpVisible();

    struct MiImageBuffer *teleInputev0 = teleInputvector[0];
    struct MiImageBuffer *wideInputev0 = wideInputvector[0];

    unsigned int MainImgW = teleInputev0->width;
    unsigned int MainImgH = teleInputev0->height;
    unsigned int AuxImgW = wideInputev0->width;
    unsigned int AuxImgH = wideInputev0->height;

    MLOGD(Mia2LogGroupPlugin,
          "[MI_CTB]  MainImgW = %d MainImgH = %d AuxImgW = %d AuxImgH = %d m_DebugMode = %d",
          MainImgW, MainImgH, AuxImgW, AuxImgH, m_DebugMode);

    camera_metadata_t *m_plogicalMeta = m_pLogicalMetaData;

    result = SetFrameParams(teleInputev0, wideInputev0, m_plogicalMeta);
    if (PROCSUCCESS != result) {
        MLOGE(Mia2LogGroupPlugin, "[MI_CTB]SetFrameParams failed:%d", result);
        return result;
    }

    // INIT
    if (!m_Initialized) {
        if (m_Initialized && m_hCaptureBokeh) {
            MIALGO_CaptureDestoryWhenClose(&m_hCaptureBokeh);
            m_hCaptureBokeh = NULL;
        }

        ALGO_CAPTURE_BOKEH_FRAME Scale = CAPTURE_BOKEH_4_3;
        GetImageScale(MainImgW, MainImgH, Scale);

        AlgoBokehLaunchParams launchParams;
        launchParams.calib_buf_size = CALIBRATIONDATA_LEN;
        launchParams.jsonFilename = (char *)JSON_PATH;
        launchParams.MainImgW = MainImgW;
        launchParams.MainImgH = MainImgH;
        launchParams.AuxImgW = AuxImgW;
        launchParams.AuxImgH = AuxImgH;
        launchParams.frame = Scale;
        launchParams.zoomRatio = m_zoomRatio;
        launchParams.arrangement = ALGO_CAPTURE_BOKEH_ARRANGEMENT;
        launchParams.bokehMode = CAPTURE_BOKEH_PERFORMANCE_MODE;
        launchParams.thermallevel = (unsigned int)3; // normal 0 ；lite 3
        if (m_zoomRatio) {
            launchParams.calib_buf = (unsigned char *)m_pCalibrationWU;
        } else {
            launchParams.calib_buf = (unsigned char *)m_pCalibrationWT;
        }
        if (m_DebugMode == 0) {
            launchParams.debugMode = CAPTURE_BOKEH_DEBUG_OFF;
        } else {
            launchParams.debugMode = CAPTURE_BOKEH_DEBUG_ON;
        }

        m_isInitFaild = MIALGO_CaptureInitWhenLaunch(&m_hCaptureBokeh, launchParams);

        m_Initialized = true;
        if (m_isInitFaild) {
            PluginUtils::miCopyBuffer(bokehOutput, teleInputev0);
            MLOGE(Mia2LogGroupPlugin, "[MI_CTB] Bokeh MIALGO_CaptureInitWhenLaunch failed %d",
                  m_isInitFaild);
            return PROCFAILED;
        }
    }

    switch (m_nViewCtrl) {
    case SHOW_WIDE_ONLY:
        PluginUtils::miCopyBuffer(bokehOutput, wideInputev0);
        break;
    case SHOW_TELE_ONLY:
        PluginUtils::miCopyBuffer(bokehOutput, teleInputev0);
        break;
    case SHOW_HALF_HALF:
        PluginUtils::miCopyBuffer(bokehOutput, teleInputev0);
        break;
    default:
        result = DoProcess(teleInputvector, wideInputvector, bokehOutput, teleOutput, depthOut);
        break;
    }

    if (m_bDrawROI) {
        DrawPoint(bokehOutput, m_bokehframParams.tp, 10, 0);
        DrawRectROI(bokehOutput, m_rtFocusROIOnInputData, 5, 0);
    }

    if (m_bDumpYUV) {
        char outputbokehbuf[128];
        snprintf(outputbokehbuf, sizeof(outputbokehbuf), "midualcambokeh_output_bokeh_%dx%d",
                 bokehOutput->width, bokehOutput->height);
        PluginUtils::dumpToFile(outputbokehbuf, bokehOutput);

        char outputtelebuf[128];
        snprintf(outputtelebuf, sizeof(outputtelebuf), "midualcambokeh_output_tele_%dx%d",
                 teleOutput->width, teleOutput->height);
        PluginUtils::dumpToFile(outputtelebuf, teleOutput);
        char outputdepthbuf[128];
        snprintf(outputdepthbuf, sizeof(outputdepthbuf), "midualcambokeh_output_depth_%dx%d",
                 depthOut->width, depthOut->height);
        PluginUtils::dumpToFile(outputdepthbuf, depthOut);
    }

    liteParam.BlurLevel = m_blurLevel;
    liteParam.BeautyEnabled = m_BeautyEnabled;
    liteParam.LightingMode = m_lightingMode;
    liteParam.HdrEnabled = m_hdrEnabled;

    MLOGD(Mia2LogGroupPlugin, "[MI_CTB]  totalcostTime = %.2f", PluginUtils::nowMSec() - startTime);
    return PROCSUCCESS;
}

void MiDualCamBokeh::PrepareScreenImage(struct MiImageBuffer *image, ASVLOFFSCREEN &stImage)
{
    MLOGD(Mia2LogGroupPlugin,
          "[MI_CTB] PrepareImage width: %d, height: %d, planeStride: %d, sliceHeight:%d, plans: %d",
          image->width, image->height, image->stride, image->scanline, image->numberOfPlanes);
    stImage.i32Width = image->width;
    stImage.i32Height = image->height;

    if (image->format != CAM_FORMAT_YUV_420_NV12 && image->format != CAM_FORMAT_YUV_420_NV21 &&
        image->format != CAM_FORMAT_Y16) {
        MLOGE(Mia2LogGroupPlugin, "[MI_CTB] format[%d] is not supported!", image->format);
        return;
    }

    if (image->format == CAM_FORMAT_YUV_420_NV12) {
        stImage.u32PixelArrayFormat = ASVL_PAF_NV12;
        MLOGD(Mia2LogGroupPlugin, "stImage(ASVL_PAF_NV12)");
    } else if (image->format == CAM_FORMAT_YUV_420_NV21) {
        stImage.u32PixelArrayFormat = ASVL_PAF_NV21;
        MLOGD(Mia2LogGroupPlugin, "stImage(ASVL_PAF_NV21)");
    } else if (image->format == CAM_FORMAT_Y16) {
        stImage.u32PixelArrayFormat = ASVL_PAF_GRAY;
        MLOGD(Mia2LogGroupPlugin, "stImage(ASVL_PAF_GRAY)");
    }

    stImage.pi32Pitch[0] = image->stride;
    stImage.pi32Pitch[1] = image->stride;

    stImage.ppu8Plane[0] =
        (unsigned char *)malloc(stImage.i32Height * stImage.pi32Pitch[0] * 3 / 2);
    if (stImage.ppu8Plane[0]) {
        stImage.ppu8Plane[1] = stImage.ppu8Plane[0] + stImage.i32Height * stImage.pi32Pitch[0];
    }

    memcpy(stImage.ppu8Plane[0], image->plane[0], stImage.i32Height * stImage.pi32Pitch[0]);
    memcpy(stImage.ppu8Plane[1], image->plane[1], stImage.i32Height / 2 * stImage.pi32Pitch[0]);

    MLOGD(Mia2LogGroupPlugin, "X");
}

void MiDualCamBokeh::PrepareImage(struct MiImageBuffer *image, AlgoBokehImg &stImage)
{
    MLOGD(Mia2LogGroupPlugin,
          "[MI_CTB]PrepareImage width: %d, height: %d, planeStride: %d, sliceHeight:%d, plans: %d  "
          "%lu  %lu",
          image->width, image->height, image->stride, image->scanline, image->numberOfPlanes,
          image->plane[0], image->plane[1]);

    stImage.width = image->width;
    stImage.height = image->height;

    if (image->format != CAM_FORMAT_YUV_420_NV12 && image->format != CAM_FORMAT_YUV_420_NV21 &&
        image->format != CAM_FORMAT_Y16) {
        MLOGE(Mia2LogGroupPlugin, "[MI_CTB] format[%d] is not supported!", image->format);
        return;
    }
    if (image->format == CAM_FORMAT_YUV_420_NV12) {
        stImage.format = BOKEH_FORMAT_NV12;
        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] stImage(MIALGO_IMG_NV12)");
    } else if (image->format == CAM_FORMAT_YUV_420_NV21) {
        stImage.format = BOKEH_FORMAT_NV21;
        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] stImage(MIALGO_IMG_NV21)");
    } else if (image->format == CAM_FORMAT_Y16) {
        stImage.format = BOKEH_FORMAT_OTHERS;
        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] stImage(BOKEH_FORMAT_OTHERS)");
    }

    stImage.stride = image->width;
    stImage.slice = image->height;

    stImage.pData = (unsigned char *)malloc(stImage.height * stImage.stride * 3 / 2);
    CopyToAlgoImage(image, &stImage);

    MLOGD(Mia2LogGroupPlugin, "[MI_CTB]PrepareImage %p", stImage.pData);

    MLOGD(Mia2LogGroupPlugin, "X");
}

int MiDualCamBokeh::SetFrameParams(struct MiImageBuffer *teleInput, struct MiImageBuffer *wideInput,
                                   camera_metadata_t *metaData)
{
    MLOGD(Mia2LogGroupPlugin, "E");

    WeightedRegion focusROI = {0, 0, 0, 0, 0};

    m_inputDataWidth = teleInput->width;
    m_inputDataHeight = teleInput->height;
    void *pMetaData = NULL;

    // get BokehFNumber
    pMetaData = NULL;
    char fNumber[8] = {0};
    const char *BokehFNumber = "xiaomi.bokeh.fNumberApplied";
    VendorMetadataParser::getVTagValue(metaData, BokehFNumber, &pMetaData);
    if (NULL != pMetaData) {
        memcpy(fNumber, static_cast<char *>(pMetaData), sizeof(fNumber));
        float fNumberApplied = atof(fNumber);
        MLOGI(Mia2LogGroupPlugin, "[MI_CTB] %s:%d get m_fNumberApplied: %f", __func__, __LINE__,
              fNumberApplied);
        m_blurLevel = findBlurLevelForLite(fNumberApplied, false);
        MLOGI(Mia2LogGroupPlugin, "[MI_CTB] %s:%d capture blur level = %d", __func__, __LINE__,
              m_blurLevel);
        m_bokehframParams.Interface.aperture = m_blurLevel;
    }

    int orient = 0;
    pMetaData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_JPEG_ORIENTATION, &pMetaData);
    if (NULL != pMetaData) {
        orient = *static_cast<int *>(pMetaData);
    }

    m_faceOrientation = orient;
    m_bokehframParams.faceRect.angle = orient;
    m_bokehframParams.ptFocus.angle = orient;

    MLOGD(Mia2LogGroupPlugin, "[MI_CTB]SetFrameParams m_faceOrientation:%d", m_faceOrientation);

    InputMetadataBokeh *pInputCTB = NULL;
    const char *BokehMetadata = "com.qti.chi.multicamerainputmetadata.InputMetadataBokeh";
    VendorMetadataParser::getVTagValue(metaData, BokehMetadata, &pMetaData);
    if (NULL != pMetaData) {
        pInputCTB = static_cast<InputMetadataBokeh *>(pMetaData);
    } else {
        MLOGE(Mia2LogGroupPlugin, "[MI_CTB] %s:%d ERR! get BokehMetadata failed!", __func__,
              __LINE__);
        return PROCFAILED;
    }

    if (ROLE_REARBOKEH2X == pInputCTB->bokehRole) {
        m_zoomRatio = CAPTURE_BOKEH_2X;
    } else {
        m_zoomRatio = CAPTURE_BOKEH_1X;
    }

    for (uint8_t i = 0; i < MAX_LINKED_SESSIONS; i++) {
        MLOGD(Mia2LogGroupPlugin,
              "[MI_CTB]SetFrameParams cameraMetadata[%d] masterCameraId %d cameraId %d", i,
              pInputCTB->cameraMetadata[i].masterCameraId, pInputCTB->cameraMetadata[i].cameraId);

        if (pInputCTB->cameraMetadata[i].masterCameraId == pInputCTB->cameraMetadata[i].cameraId) {
            m_bokehframParams.Interface.AfCode = pInputCTB->cameraMetadata[i].focusDistCm;
            MLOGD(Mia2LogGroupPlugin, "[MI_CTB]SetFrameParams AfCode %d",
                  m_bokehframParams.Interface.AfCode);

            m_fovRectIFEForTele = pInputCTB->cameraMetadata[i].fovRectIFE;
            m_activeArraySizeTele = pInputCTB->cameraMetadata[i].activeArraySize;

            focusROI = pInputCTB->cameraMetadata[i].afFocusROI;
            MIRECT rtFocusROI = {(uint32_t)focusROI.xMin, (uint32_t)focusROI.yMin,
                                 (uint32_t)(focusROI.xMax - focusROI.xMin + 1),
                                 (uint32_t)(focusROI.yMax - focusROI.yMin + 1)};
            memset(&m_rtFocusROIOnInputData, 0, sizeof(MIRECT));
            MapRectToInputData(rtFocusROI, m_activeArraySizeTele, m_inputDataWidth,
                               m_inputDataHeight, m_rtFocusROIOnInputData);
            if (!m_bRearCamera) {
                MLOGD(Mia2LogGroupPlugin, "[MI_CTB] : capture facenum = %d",
                      pInputCTB->cameraMetadata[i].fdMetadata.numFaces);
                if (pInputCTB->cameraMetadata[i].fdMetadata.numFaces > 0) {
                    unsigned int mLargeFaceRegion = 0;
                    MIRECT rtOnInputData;
                    memset(&rtOnInputData, 0, sizeof(MIRECT));
                    MapRectToInputData(pInputCTB->cameraMetadata[i].fdMetadata.faceRect[0],
                                       m_activeArraySizeTele, m_inputDataWidth, m_inputDataHeight,
                                       rtOnInputData);
                    m_bokehframParams.ptFocus.x = rtOnInputData.left;
                    m_bokehframParams.ptFocus.y = rtOnInputData.top;
                    m_bokehframParams.ptFocus.width = rtOnInputData.width;
                    m_bokehframParams.ptFocus.height = rtOnInputData.height;
                    m_bokehframParams.tp.x = (rtOnInputData.left + rtOnInputData.width / 2) / 2;
                    m_bokehframParams.tp.y = (rtOnInputData.top + rtOnInputData.height / 2) / 2;
                    mLargeFaceRegion = rtOnInputData.width * rtOnInputData.height;
                    MLOGD(Mia2LogGroupPlugin,
                          "[MI_CTB] : capture facerect[0](l,t,w,h)[%d,%d,%d,%d]",
                          rtOnInputData.left, rtOnInputData.top, rtOnInputData.width,
                          rtOnInputData.height);

                    for (int nFaceIndex = 1;
                         nFaceIndex < pInputCTB->cameraMetadata[i].fdMetadata.numFaces;
                         ++nFaceIndex) {
                        memset(&rtOnInputData, 0, sizeof(MIRECT));
                        MapRectToInputData(
                            pInputCTB->cameraMetadata[i].fdMetadata.faceRect[nFaceIndex],
                            m_activeArraySizeTele, m_inputDataWidth, m_inputDataHeight,
                            rtOnInputData);
                        if (mLargeFaceRegion < rtOnInputData.width * rtOnInputData.height) {
                            m_bokehframParams.ptFocus.x = rtOnInputData.left;
                            m_bokehframParams.ptFocus.y = rtOnInputData.top;
                            m_bokehframParams.ptFocus.width = rtOnInputData.width;
                            m_bokehframParams.ptFocus.height = rtOnInputData.height;
                            m_bokehframParams.tp.x =
                                (rtOnInputData.left + rtOnInputData.width / 2) / 2;
                            m_bokehframParams.tp.y =
                                (rtOnInputData.top + rtOnInputData.height / 2) / 2;
                        }
                        MLOGD(Mia2LogGroupPlugin,
                              "[MI_CTB] : capture facerect[%d](l,t,w,h)[%d,%d,%d,%d]", nFaceIndex,
                              rtOnInputData.left, rtOnInputData.top, rtOnInputData.width,
                              rtOnInputData.height);
                    }
                } else {
                    m_bokehframParams.ptFocus.x = m_rtFocusROIOnInputData.left;
                    m_bokehframParams.ptFocus.y = m_rtFocusROIOnInputData.top;
                    m_bokehframParams.ptFocus.width = m_rtFocusROIOnInputData.width;
                    m_bokehframParams.ptFocus.height = m_rtFocusROIOnInputData.height;
                    m_bokehframParams.tp.x =
                        m_rtFocusROIOnInputData.left + m_rtFocusROIOnInputData.width / 2;
                    m_bokehframParams.tp.y =
                        m_rtFocusROIOnInputData.top + m_rtFocusROIOnInputData.height / 2;
                    if (m_bokehframParams.tp.x == 0 || m_bokehframParams.tp.y == 0) {
                        m_bokehframParams.tp.x = m_inputDataWidth / 2;
                        m_bokehframParams.tp.y = m_inputDataHeight / 2;
                    }
                }
            } else {
                m_bokehframParams.ptFocus.x = m_rtFocusROIOnInputData.left;
                m_bokehframParams.ptFocus.y = m_rtFocusROIOnInputData.top;
                m_bokehframParams.ptFocus.width = m_rtFocusROIOnInputData.width;
                m_bokehframParams.ptFocus.height = m_rtFocusROIOnInputData.height;
                m_bokehframParams.tp.x =
                    m_rtFocusROIOnInputData.left + m_rtFocusROIOnInputData.width / 2;
                m_bokehframParams.tp.y =
                    m_rtFocusROIOnInputData.top + m_rtFocusROIOnInputData.height / 2;

                if (m_bokehframParams.tp.x == 0 && m_bokehframParams.tp.y == 0) {
                    m_bokehframParams.tp.x = m_bokehframParams.ptFocus.width / 2;
                    m_bokehframParams.tp.y = m_bokehframParams.ptFocus.height / 2;
                }
            }

            // for Face info
            ConvertToFaceInfo(pInputCTB->cameraMetadata[i].fdMetadata, m_bokehframParams.faceRect);

            MLOGD(Mia2LogGroupPlugin, "[MI_CTB]SetFrameParams focus rect(l,t,w,h):[%d,%d,%d,%d] ",
                  focusROI.xMin, focusROI.yMin, focusROI.xMax, focusROI.yMax);

            MLOGD(Mia2LogGroupPlugin, "[MI_CTB] : tele facenum = %d",
                  pInputCTB->cameraMetadata[i].fdMetadata.numFaces);
            for (int nFaceIndex = 0; nFaceIndex < pInputCTB->cameraMetadata[i].fdMetadata.numFaces;
                 ++nFaceIndex) {
                MLOGD(Mia2LogGroupPlugin, "[MI_CTB] : facerect[%d](l,t,w,h)[%d,%d,%d,%d]",
                      nFaceIndex, pInputCTB->cameraMetadata[i].fdMetadata.faceRect[nFaceIndex].left,
                      pInputCTB->cameraMetadata[i].fdMetadata.faceRect[nFaceIndex].top,
                      pInputCTB->cameraMetadata[i].fdMetadata.faceRect[nFaceIndex].width,
                      pInputCTB->cameraMetadata[i].fdMetadata.faceRect[nFaceIndex].height);
            }

        } else {
            m_fovRectIFEForWide = pInputCTB->cameraMetadata[i].fovRectIFE;
            m_activeArraySizeWide = pInputCTB->cameraMetadata[i].activeArraySize;
        }
    }

    pMetaData = NULL;
    static char componentName[] = "xiaomi.beauty.";
    char tagStr[256] = {0};
    int beautyFeatureCounts = mBeautyFeatureParams.featureCounts;
    for (int i = 0; i < beautyFeatureCounts; i++) {
        if (mBeautyFeatureParams.beautyFeatureInfo[i].featureMask == true) {
            memset(tagStr, 0, sizeof(tagStr));
            memcpy(tagStr, componentName, sizeof(componentName));
            memcpy(tagStr + sizeof(componentName) - 1,
                   mBeautyFeatureParams.beautyFeatureInfo[i].tagName,
                   strlen(mBeautyFeatureParams.beautyFeatureInfo[i].tagName) + 1);

            pMetaData = NULL;
            VendorMetadataParser::getVTagValue(metaData, tagStr, &pMetaData);
            if (pMetaData != NULL) {
                mBeautyFeatureParams.beautyFeatureInfo[i].featureValue =
                    *static_cast<int *>(pMetaData);
                MLOGD(Mia2LogGroupPlugin, " [MI_CTB] Get Metadata Value TagName: %s, Value: %d",
                      tagStr, mBeautyFeatureParams.beautyFeatureInfo[i].featureValue);
                if (mBeautyFeatureParams.beautyFeatureInfo[i].featureValue !=
                    0) // feature vlude support -100 -- + 100, 0 means no effect
                {
                    m_BeautyEnabled = true;
                }
            } else {
                MLOGE(Mia2LogGroupPlugin, "[MI_CTB]Get Metadata Failed! TagName: %s", tagStr);
            }
            // m_BeautyEnabled = 0; // for test
        }
    }

    int light_mode = 0;
    const char *BokehLight = "xiaomi.portrait.lighting";
    VendorMetadataParser::getVTagValue(metaData, BokehLight, &pMetaData);
    if (NULL != pMetaData) {
        light_mode = *static_cast<int *>(pMetaData);
    }
    switch (light_mode) {
    case 9:
        m_lightingMode = MI_PL_MODE_NEON_SHADOW;
        break; // 霓影
    case 10:
        m_lightingMode = MI_PL_MODE_PHANTOM;
        break; // 魅影
    case 11:
        m_lightingMode = MI_PL_MODE_NOSTALGIA;
        break; // 怀旧
    case 12:
        m_lightingMode = MI_PL_MODE_RAINBOW;
        break; // 彩虹
    case 13:
        m_lightingMode = MI_PL_MODE_WANING;
        break; // 阑珊
    case 14:
        m_lightingMode = MI_PL_MODE_DAZZLE;
        break; // 炫影
    case 15:
        m_lightingMode = MI_PL_MODE_GORGEOUS;
        break; // 斑斓
    case 16:
        m_lightingMode = MI_PL_MODE_PURPLES;
        break; // 嫣红
    case 17:
        m_lightingMode = MI_PL_MODE_DREAM;
        break; // 梦境
    default:
        m_lightingMode = 0;
        break;
    }
    // m_lightingMode = 0; // for test
    MLOGD(Mia2LogGroupPlugin, "[MI_CTB] get m_lightingMode: %d", m_lightingMode);

    /*pMetaData = NULL;
    const char* FaceAnalyzeResult = "xiaomi.faceAnalyzeResult.result";
    VendorMetadataParser::getVTagValue(metaData, FaceAnalyzeResult, &pMetaData);
    if (NULL != pMetaData)
    {
        memcpy(&m_faceAnalyze, pMetaData, sizeof(OutputMetadataFaceAnalyze));
        MLOGD(Mia2LogGroupPlugin, "getMetaData faceNum %d", m_faceAnalyze.faceNum);
    }*/

    pMetaData = NULL;
    const char *AiAsdEnabled = "xiaomi.ai.asd.enabled";
    VendorMetadataParser::getVTagValue(metaData, AiAsdEnabled, &pMetaData);
    if (NULL != pMetaData) {
        m_sceneEnable = *static_cast<int *>(pMetaData);
        m_sceneEnable = 0; // for test
        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] getMetaData m_sceneEnable: %d", m_sceneEnable);
    }

    if (m_sceneEnable) {
        pMetaData = NULL;
        const char *AiAsdDetected = "xiaomi.ai.asd.sceneDetected";
        VendorMetadataParser::getVTagValue(metaData, AiAsdDetected, &pMetaData);
        if (NULL != pMetaData) {
            m_sceneMode = *static_cast<int *>(pMetaData);
            MLOGD(Mia2LogGroupPlugin, "[MI_CTB] getMetaData m_sceneMode: %d", m_sceneMode);
        }
    } else {
        m_sceneMode = 0;
        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] getMetaData m_sceneMode: %d", m_sceneMode);
    }

    pMetaData = NULL;
    LaserDistance *LaserDist = NULL;
    const char *Laser = "xiaomi.ai.misd.laserDist";
    VendorMetadataParser::getVTagValue(metaData, Laser, &pMetaData);
    if (NULL != pMetaData) {
        LaserDist = static_cast<LaserDistance *>(pMetaData);
        m_bokehframParams.Interface.TOF = (int)LaserDist->distance;
        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] getMetaData laserDist: %d",
              m_bokehframParams.Interface.TOF);
    }

    pMetaData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_SENSOR_SENSITIVITY, &pMetaData);
    if (NULL != pMetaData) {
        int iso = *static_cast<int *>(pMetaData);
        m_bokehframParams.Interface.ISO = iso;
        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] getMetaData iso: %d", iso);
    }

    pMetaData = NULL;
    const char *AecFrameControl = "org.quic.camera2.statsconfigs.AECFrameControl";
    VendorMetadataParser::getVTagValue(metaData, AecFrameControl, &pMetaData);
    if (NULL != pMetaData) {
        float lux_index = static_cast<AECFrameControl *>(pMetaData)->luxIndex;
        m_bokehframParams.luxIndex = lux_index;
        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] getMetaData lux_index: %f", lux_index);
    }

    m_bokehframParams.Interface.motionFlag = 0;

    m_hdrEnabled = false; // for test

    return PROCSUCCESS;
}

void MiDualCamBokeh::MapRectToInputData(MIRECT &rtSrcRect, MIRECT &rtIFERect, int &nDataWidth,
                                        int &nDataHeight, MIRECT &rtDstRectOnInputData)
{
    MLOGD(Mia2LogGroupPlugin,
          "[MI_CTB] rtSrcRect w:%d, h:%d, rtIFERect w:%d, h:%d, nDataWidth:%d, nDataHeight:%d",
          rtSrcRect.width, rtSrcRect.height, rtIFERect.width, rtIFERect.height, nDataWidth,
          nDataHeight);

    if (rtSrcRect.width <= 0 || rtSrcRect.height <= 0 || rtIFERect.width <= 0 ||
        rtIFERect.height <= 0 || nDataWidth <= 0 || nDataHeight <= 0)
        return;

    MIRECT rtSrcOnIFE = rtSrcRect;

    if (rtSrcOnIFE.left > rtIFERect.left) {
        rtSrcOnIFE.left -= rtIFERect.left;
    } else {
        rtSrcOnIFE.left = 0;
    }
    if (rtSrcOnIFE.top > rtIFERect.top) {
        rtSrcOnIFE.top -= rtIFERect.top;
    } else {
        rtSrcOnIFE.top = 0;
    }

    float fLRRatio = nDataWidth / (float)rtIFERect.width;
    // float fTBRatio = nDataHeight / (float)rtIFERect.height;
    int iTBOffset = (((int)rtIFERect.height) - (int)rtIFERect.width * nDataHeight / nDataWidth) / 2;
    if (iTBOffset < 0)
        iTBOffset = 0;

    if (rtSrcOnIFE.top > (float)iTBOffset) {
        rtDstRectOnInputData.top = round((rtSrcOnIFE.top - iTBOffset) * fLRRatio);
    } else {
        rtDstRectOnInputData.top = 0;
    }

    rtDstRectOnInputData.left = round(rtSrcOnIFE.left * fLRRatio);
    rtDstRectOnInputData.width = floor(rtSrcOnIFE.width * fLRRatio);
    rtDstRectOnInputData.height = floor(rtSrcOnIFE.height * fLRRatio);
    rtDstRectOnInputData.width &= 0xFFFFFFFE;
    rtDstRectOnInputData.height &= 0xFFFFFFFE;

    MLOGD(Mia2LogGroupPlugin,
          "[MI_CTB] MapRectToInputData rtSrcRect(l,t,w,h)= [%d,%d,%d,%d], rtIFERect(l,t,w,h)= "
          "[%d,%d,%d,%d], rtDstRectOnInputData[%d,%d,%d,%d]",
          rtSrcRect.left, rtSrcRect.top, rtSrcRect.width, rtSrcRect.height, rtIFERect.left,
          rtIFERect.top, rtIFERect.width, rtIFERect.height, rtDstRectOnInputData.left,
          rtDstRectOnInputData.top, rtDstRectOnInputData.width, rtDstRectOnInputData.height);
}

void MiDualCamBokeh::ConvertToFaceInfo(FDMetadataResults &fdMeta, AlgoBokehRect &faceinfo)
{
    MIRECT rtOnInputData;
    unsigned int mLargeFaceRegion = 0;

    // reset face rect for no face
    memset(&faceinfo, 0, sizeof(AlgoBokehRect));

    for (int i = 0; i < fdMeta.numFaces && i < MAX_FACE_NUM_BOKEH; ++i) {
        memset(&rtOnInputData, 0, sizeof(MIRECT));
        MapRectToInputData(fdMeta.faceRect[i], m_activeArraySizeTele, m_inputDataWidth,
                           m_inputDataHeight, rtOnInputData);

        if (mLargeFaceRegion < rtOnInputData.width * rtOnInputData.height) {
            faceinfo.x = rtOnInputData.left;
            faceinfo.y = rtOnInputData.top;
            faceinfo.width = rtOnInputData.width;
            faceinfo.height = rtOnInputData.height;
            faceinfo.angle = m_faceOrientation;
        }
        MLOGD(Mia2LogGroupPlugin,
              "[MI_CTB] : ConvertToArcFaceInfo face[%d] = (l,t,w,h)[%d,%d,%d,%d],faceDegree = %d",
              i, fdMeta.faceRect[i].left, fdMeta.faceRect[i].top, fdMeta.faceRect[i].width,
              fdMeta.faceRect[i].height, m_faceOrientation);
    }
}

int MiDualCamBokeh::DoProcess(std::vector<struct MiImageBuffer *> teleInputvector,
                              std::vector<struct MiImageBuffer *> wideInputvector,
                              struct MiImageBuffer *bokehOutput, struct MiImageBuffer *teleOutput,
                              struct MiImageBuffer *depthOut)
{
    MLOGD(Mia2LogGroupPlugin, "[MI_CTB] DoDepthBlur E");
    int ret = 0;
    double t0, t1, tDepthS, tDepthE, tBlurS, tBlurE, tProcessS;
    tProcessS = gettime();

    AlgoBokehImg YUV_Result;
    MLOGD(Mia2LogGroupPlugin, "[MI_CTB] PrepareImage(bokehOutput, YUV_Result);");
    PrepareImage(bokehOutput, YUV_Result);

    PluginUtils::miCopyBuffer(bokehOutput, teleInputvector[0]);
    PluginUtils::miCopyBuffer(teleOutput, teleInputvector[0]);

    _tlocal_thread_params threadParam;
    relight_thread_params rtp;
    m_hRelightingHandle = NULL;
    pthread_t tidRelighting;
    void *tidRelightingExitStatus = NULL;
    bool relighting_joinable = false;
    memset(&threadParam, 0, sizeof(threadParam));

    do {
        MLOGD(Mia2LogGroupPlugin,
              "[MI_CTB] Process Param facerect(l,t,w,h,angle)[%d,%d,%d,%d,%d] FocusPoint[%d,%d] "
              "AFcode %d, blur %d isoValue %d laserDist %d motionFlag=%d",
              m_bokehframParams.faceRect.x, m_bokehframParams.faceRect.y,
              m_bokehframParams.faceRect.width, m_bokehframParams.faceRect.height,
              m_bokehframParams.faceRect.angle, m_bokehframParams.tp.x, m_bokehframParams.tp.y,
              m_bokehframParams.Interface.AfCode, m_bokehframParams.Interface.aperture,
              m_bokehframParams.Interface.ISO, m_bokehframParams.Interface.TOF,
              m_bokehframParams.Interface.motionFlag);

        // PrepareImage
        AlgoBokehImg YUV_Aux;
        AlgoBokehImg YUV_Main;
        PrepareImage(teleInputvector[0], YUV_Main);
        PrepareImage(wideInputvector[0], YUV_Aux);

        AlgoBokeInitParams initParams;
        initParams.imgL = &YUV_Main;
        initParams.imgR = &YUV_Aux;
        initParams.imgL_EV_1 = NULL;
        initParams.fRect = m_bokehframParams.faceRect;
        initParams.tp = m_bokehframParams.tp;
        initParams.interface = m_bokehframParams.Interface;
        initParams.dstPath = NULL;
        initParams.cShortName = NULL;

        if (m_hdrEnabled && teleInputvector.size() >= 2 && m_HdrMode) {
            AlgoBokehImg YUV_MainEV_1;
            PrepareImage(teleInputvector[1], YUV_MainEV_1);
            initParams.imgL_EV_1 = &YUV_MainEV_1;
            MIALGO_CaptureInitEachPic(&m_hCaptureBokeh, initParams);
            ReleaseImage(&YUV_MainEV_1);
        } else {
            MIALGO_CaptureInitEachPic(&m_hCaptureBokeh, initParams);
        }

        // ReleaseImage
        ReleaseImage(&YUV_Main);
        ReleaseImage(&YUV_Aux);

        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] Process Param sceneTag:%d sceneTagConfidence:%d ",
              m_bokehframParams.Interface.sceneTag, m_bokehframParams.Interface.sceneTagConfidence);

        // do depth
        int lSize = 0;
        ret = MIALGO_CaptureGetDepthDataSize(&m_hCaptureBokeh, &lSize);
        if (ret) {
            MLOGE(Mia2LogGroupPlugin,
                  "[MI_CTB] Bokeh MIALGO_CaptureGetDepthDataSize failed res = %d", ret);
            ret = PROCFAILED;
            break;
        }
        if ((lSize > 0 && NULL == m_pDisparityData) ||
            (lSize > 0 && lSize > m_i32DisparityDataSize)) {
            if (NULL != m_pDisparityData)
                free(m_pDisparityData);

            MLOGD(Mia2LogGroupPlugin, "[MI_CTB] : malloc %lu buffer", lSize * sizeof(uint8_t));
            m_pDisparityData = (uint8_t *)malloc(lSize * sizeof(uint8_t));
            m_i32DisparityDataSize = lSize;
        }
        m_i32DisparityDataActiveSize = lSize;
        if (NULL == m_pDisparityData) {
            MLOGE(Mia2LogGroupPlugin, "[MI_CTB] :m_pDisparityData == NULL");
            break;
        }

        // PrepareImage ai mask
        AlgoBokehImg AI_Mask;
        AI_Mask.width = MIALGO_MULTICAM_MASK_W;
        AI_Mask.height = MIALGO_MULTICAM_MASK_H;
        AI_Mask.stride = AI_Mask.width;
        AI_Mask.format = BOKEH_FORMAT_GRAY;
        AI_Mask.pData = (unsigned char *)malloc(AI_Mask.height * AI_Mask.stride);

        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] AI_Mask width height stride [%d,%d,%d] ", AI_Mask.width,
              AI_Mask.height, AI_Mask.stride);

        AlgoBokehDepthParams depthParams;
        depthParams.result_data_buf = m_pDisparityData;
        depthParams.result_data_size = m_i32DisparityDataSize;
        depthParams.imgMask = &AI_Mask;

        tDepthS = gettime();
        ret = MIALGO_CaptureGetDepthProc(&m_hCaptureBokeh, depthParams);
        tDepthE = gettime();
        ReleaseImage(&AI_Mask);
        if (ret) {
            MLOGE(Mia2LogGroupPlugin, "[MI_CTB] Bokeh ARC_DCIR_GetDisparityData failed res = %d",
                  ret);
            ret = PROCFAILED;
            break;
        }

        // copy disparity data to output
        if (depthOut != nullptr) {
            MInt32 plane_size =
                depthOut->height * depthOut->stride - sizeof(disparity_fileheader_t);
            MLOGD(Mia2LogGroupPlugin,
                  "[MI_CTB] disparity_fileheader_t size %lu ,DisparityDataActiveSize = %d "
                  "plane_size %d",
                  sizeof(disparity_fileheader_t), m_i32DisparityDataActiveSize, plane_size);
            MLOGD(Mia2LogGroupPlugin, "[MI_CTB] plane_size size %lu ", plane_size);

            if (depthOut->plane[0] != NULL && m_i32DisparityDataActiveSize < plane_size) {
                disparity_fileheader_t header;
                memset(&header, 0, sizeof(disparity_fileheader_t));

                header.i32Header = 0x80;
                header.i32HeaderLength = sizeof(disparity_fileheader_t);
                header.i32PointX = (int32_t)m_bokehframParams.tp.x;
                header.i32PointY = (int32_t)m_bokehframParams.tp.y;
                header.i32BlurLevel = (int32_t)m_bokehframParams.Interface.aperture;
                header.i32Version = (int32_t)3;
                header.i32VendorID = (int32_t)0;
                header.i32DataLength = (int32_t)m_i32DisparityDataActiveSize;
                memcpy(depthOut->plane[0], &header, sizeof(disparity_fileheader_t));
                memcpy(depthOut->plane[0] + sizeof(disparity_fileheader_t), m_pDisparityData,
                       m_i32DisparityDataActiveSize);
            } else {
                MLOGD(Mia2LogGroupPlugin,
                      "[MI_CTB] : DoProcess DisparityDataActiveSize = %d plane_size %d",
                      m_i32DisparityDataActiveSize, plane_size);
            }
        }

        if (m_lightingMode > 0) {
            rtp.i32Width = teleInputvector[0]->width;
            rtp.i32Height = teleInputvector[0]->height;
            rtp.m_phRelightingHandle = &m_hRelightingHandle;
            MLOGI(Mia2LogGroupPlugin,
                  "[MI_CTB] before init, give :width %d height %d, m_phRelightingHandle:%p, "
                  "m_bRearCamera:%d",
                  rtp.i32Width, rtp.i32Height, *rtp.m_phRelightingHandle, m_bRearCamera);
            pthread_create(&tidRelighting, NULL, bokeh_relighting_thread_proc_init, &rtp);
            MLOGI(Mia2LogGroupPlugin,
                  "[MI_CTB] right after init, got m_hRelightingHandle:%p, m_bRearCamera:%d",
                  m_hRelightingHandle, m_bRearCamera);
            relighting_joinable = true;
        }

        ASVLOFFSCREEN tmpTeleImage;
        ASVLOFFSCREEN tmpArlDstImage;
        PrepareScreenImage(teleOutput, tmpTeleImage);
        PrepareScreenImage(teleOutput, tmpArlDstImage);

        // do beauty
        pthread_t tidBeauty;
        void *tidBeautyExitStatus = NULL;
        struct bokeh_facebeauty_params params;
        if (m_BeautyEnabled > 0) {
            params.pImage = &tmpTeleImage;
            params.orient = m_faceOrientation;
            params.rearCamera = m_bRearCamera;
            memcpy(&params.beautyFeatureParams.beautyFeatureInfo[0],
                   &mBeautyFeatureParams.beautyFeatureInfo[0],
                   sizeof(mBeautyFeatureParams.beautyFeatureInfo));
            bokeh_facebeauty_thread_proc(&params);
        }

        if (m_lightingMode > 0) {
            MIIMAGEBUFFER pSrcImg;
            MIIMAGEBUFFER pDstImg;
            MIIMAGEBUFFER pBokehImg;
            memset(&pSrcImg, 0, sizeof(MIIMAGEBUFFER));
            memset(&pDstImg, 0, sizeof(MIIMAGEBUFFER));
            memset(&pBokehImg, 0, sizeof(MIIMAGEBUFFER));

            PrepareRelightImage(pSrcImg, tmpTeleImage);   // [in] The input image data
            PrepareRelightImage(pDstImg, tmpArlDstImage); // [in] The output image data

            threadParam.pSrcImg = pSrcImg;
            threadParam.pDstImg = pDstImg;
            threadParam.orient = m_faceOrientation;
            threadParam.mode = m_lightingMode;
            threadParam.pBokehImg = pBokehImg;
            pthread_join(tidRelighting, &tidRelightingExitStatus);
            MLOGI(Mia2LogGroupPlugin,
                  "[MI_CTB] after init done , got m_hRelightingHandle:%p, m_bRearCamera:%d",
                  m_hRelightingHandle, m_bRearCamera);

            if (m_hRelightingHandle) {
                threadParam.m_hRelightingHandle = &m_hRelightingHandle;
                pthread_create(&tidRelighting, NULL, bokeh_relighting_thread_proc_preprocess,
                               &threadParam);
            }
        }

        if (m_sceneMode != 0 || teleInputvector.size() > 1 || m_BeautyEnabled) {
            CopyScreenToMiImage(&tmpTeleImage, teleOutput);
        }
        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] m_sceneMode %d, m_hdrEnabled %d, m_BeautyEnabled %d",
              m_sceneMode, m_hdrEnabled, m_BeautyEnabled);

        AlgoBokehImg YUV_Inputbokeh;
        MLOGD(Mia2LogGroupPlugin, "[MI_CTB] PrepareImage(teleOutput, YUV_Inputbokeh);");
        PrepareImage(teleOutput, YUV_Inputbokeh);

        AlgoBokehEffectParams bokehParams;
        bokehParams.imgOri = &YUV_Inputbokeh;
        bokehParams.depth_info_buf = m_pDisparityData;
        bokehParams.depth_info_size = lSize;
        bokehParams.imgResult = &YUV_Result;
        tBlurS = gettime();
        ret = MIALGO_CaptureBokehEffectProc(&m_hCaptureBokeh, bokehParams);
        tBlurE = gettime();
        if (ret != PROCSUCCESS) {
            MLOGE(Mia2LogGroupPlugin, "[MI_CTB] Bokeh MIALGO_CaptureBokehProc failed");
            PluginUtils::miCopyBuffer(bokehOutput, teleOutput);
        } else {
            CopyToMiImage(&YUV_Result, bokehOutput);
        }
        ReleaseImage(&YUV_Inputbokeh);

        if (m_lightingMode > 0)
            pthread_join(tidRelighting, &tidRelightingExitStatus);
        relighting_joinable = false;

        if (m_lightingMode > 0) {
            ASVLOFFSCREEN tmpDestImage;
            PrepareScreenImage(bokehOutput, tmpDestImage);
            if (ret == PROCSUCCESS) {
                processRelighting(&tmpTeleImage, &tmpArlDstImage, &tmpDestImage);
                if (!m_useBokeh) {
                    CopyScreenToMiImage(&tmpArlDstImage, bokehOutput);
                }
            }
            ReleaseScreenImage(&tmpDestImage);
        }

        if (m_hRelightingHandle) {
            AILAB_PLI_Uninit(&m_hRelightingHandle);
            m_hRelightingHandle = NULL;
        }

        ReleaseScreenImage(&tmpTeleImage);
        ReleaseScreenImage(&tmpArlDstImage);
    } while (false);

    if (m_hCaptureBokeh != NULL) {
        MIALGO_CaptureDestoryEachPic(&m_hCaptureBokeh);
    }
    ReleaseImage(&YUV_Result);

    MLOGD(Mia2LogGroupPlugin,
          "[MI_CTB] DoDepthBlur X, "
          "MIALGO_CaptureGetDepthProc costime=%.2f  "
          "MIALGO_CaptureBokehEffectProc costime=%.2f     total=%.2f",
          tDepthE - tDepthS, tBlurE - tBlurS, PluginUtils::nowMSec() - tProcessS);

    return PROCSUCCESS;
}

void MiDualCamBokeh::DrawPoint(struct MiImageBuffer *pOffscreen, AlgoBokehPoint xPoint,
                               MInt32 nPointWidth, MInt32 iType)
{
    if (MNull == pOffscreen) {
        return;
    }
    MInt32 nPoints = 10;
    nPoints = (nPoints < nPointWidth) ? nPointWidth : nPoints;

    int nStartX = xPoint.x - (nPoints / 2);
    int nStartY = xPoint.y - (nPoints / 2);
    nStartX = nStartX < 0 ? 0 : nStartX;
    nStartY = nStartY < 0 ? 0 : nStartY;
    if (nStartX > pOffscreen->width) {
        nStartX = pOffscreen->width - nPoints;
    }
    if (nStartY > pOffscreen->height) {
        nStartY = pOffscreen->height - nPoints;
    }

    if (pOffscreen->format == CAM_FORMAT_YUV_420_NV12 ||
        pOffscreen->format == CAM_FORMAT_YUV_420_NV21) {
        MUInt8 *pByteYTop = pOffscreen->plane[0] + nStartY * pOffscreen->stride + nStartX;
        MUInt8 *pByteVTop = pOffscreen->plane[1] + (nStartY / 2) * (pOffscreen->stride) + nStartX;
        for (int i = 0; i < nPoints; i++) {
            if (0 == iType) {
                memset(pByteYTop, 255, nPoints);
                memset(pByteVTop, 0, nPoints);
            } else {
                memset(pByteYTop, 81, nPoints);
                memset(pByteVTop, 238, nPoints);
            }
            pByteYTop += (pOffscreen->stride);
            if (i % 2 == 1)
                pByteVTop += (pOffscreen->stride);
        }
    }
}

void MiDualCamBokeh::DrawRectROI(struct MiImageBuffer *pOffscreen, MIRECT rt, MInt32 nLineWidth,
                                 MInt32 iType)
{
    int l = rt.left;
    int t = rt.top;
    int r = rt.left + rt.width;
    int b = rt.top + rt.height;
    MRECT rect = {l, t, r, b};
    DrawRect(pOffscreen, rect, nLineWidth, iType);
}

void MiDualCamBokeh::DrawRect(struct MiImageBuffer *pOffscreen, MRECT RectDraw, MInt32 nLineWidth,
                              MInt32 iType)
{
    if (MNull == pOffscreen) {
        return;
    }
    nLineWidth = (nLineWidth + 1) / 2 * 2;
    MInt32 nPoints = nLineWidth <= 0 ? 1 : nLineWidth;

    MRECT RectImage = {0, 0, (MInt32)pOffscreen->width, (MInt32)pOffscreen->height};
    MRECT rectIn = intersection(RectDraw, RectImage);

    MInt32 nLeft = rectIn.left;
    MInt32 nTop = rectIn.top;
    MInt32 nRight = rectIn.right;
    MInt32 nBottom = rectIn.bottom;
    // MUInt8* pByteY = pOffscreen->ppu8Plane[0];
    int nRectW = nRight - nLeft;
    int nRecrH = nBottom - nTop;
    if (nTop + nPoints + nRecrH > pOffscreen->height) {
        nTop = pOffscreen->height - nPoints - nRecrH;
    }
    if (nTop < nPoints / 2) {
        nTop = nPoints / 2;
    }
    if (nBottom - nPoints < 0) {
        nBottom = nPoints;
    }
    if (nBottom + nPoints / 2 > pOffscreen->height) {
        nBottom = pOffscreen->height - nPoints / 2;
    }
    if (nLeft + nPoints + nRectW > pOffscreen->width) {
        nLeft = pOffscreen->width - nPoints - nRectW;
    }
    if (nLeft < nPoints / 2) {
        nLeft = nPoints / 2;
    }

    if (nRight - nPoints < 0) {
        nRight = nPoints;
    }
    if (nRight + nPoints / 2 > pOffscreen->width) {
        nRight = pOffscreen->width - nPoints / 2;
    }

    nRectW = nRight - nLeft;
    nRecrH = nBottom - nTop;

    if (pOffscreen->format == CAM_FORMAT_YUV_420_NV12 ||
        pOffscreen->format == CAM_FORMAT_YUV_420_NV21) {
        // draw the top line
        MUInt8 *pByteYTop =
            pOffscreen->plane[0] + (nTop - nPoints / 2) * pOffscreen->stride + nLeft - nPoints / 2;
        MUInt8 *pByteVTop = pOffscreen->plane[1] + ((nTop - nPoints / 2) / 2) * pOffscreen->stride +
                            nLeft - nPoints / 2;
        MUInt8 *pByteYBottom = pOffscreen->plane[0] +
                               (nTop + nRecrH - nPoints / 2) * pOffscreen->stride + nLeft -
                               nPoints / 2;
        MUInt8 *pByteVBottom = pOffscreen->plane[1] +
                               ((nTop + nRecrH - nPoints / 2) / 2) * pOffscreen->stride + nLeft -
                               nPoints / 2;
        for (int i = 0; i < nPoints; i++) {
            pByteVTop = pOffscreen->plane[1] + ((nTop + i - nPoints / 2) / 2) * pOffscreen->stride +
                        nLeft - nPoints / 2;
            pByteVBottom = pOffscreen->plane[1] +
                           ((nTop + i + nRecrH - nPoints / 2) / 2) * pOffscreen->stride + nLeft -
                           nPoints / 2;
            memset(pByteYTop, 255, nRectW + nPoints);
            memset(pByteYBottom, 255, nRectW + nPoints);
            pByteYTop += pOffscreen->stride;
            pByteYBottom += pOffscreen->stride;
            memset(pByteVTop, 60 * iType, nRectW + nPoints);
            memset(pByteVBottom, 60 * iType, nRectW + nPoints);
        }
        for (int i = 0; i < nRecrH; i++) {
            MUInt8 *pByteYLeft =
                pOffscreen->plane[0] + (nTop + i) * pOffscreen->stride + nLeft - nPoints / 2;
            MUInt8 *pByteYRight = pOffscreen->plane[0] + (nTop + i) * pOffscreen->stride + nLeft +
                                  nRectW - nPoints / 2;
            MUInt8 *pByteVLeft = pOffscreen->plane[1] + (((nTop + i) / 2) * pOffscreen->stride) +
                                 nLeft - nPoints / 2;
            MUInt8 *pByteVRight = pOffscreen->plane[1] + (((nTop + i) / 2) * pOffscreen->stride) +
                                  nLeft + nRectW - nPoints / 2;
            memset(pByteYLeft, 255, nPoints);
            memset(pByteYRight, 255, nPoints);
            memset(pByteVLeft, 60 * iType, nPoints);
            memset(pByteVRight, 60 * iType, nPoints);
        }
    }
}

MRECT MiDualCamBokeh::intersection(const MRECT &a, const MRECT &d)
{
    int l = MATHL_MAX(a.left, d.left);
    int t = MATHL_MAX(a.top, d.top);
    int r = MATH_MIN(a.right, d.right);
    int b = MATH_MIN(a.bottom, d.bottom);
    if (l >= r || t >= b) {
        l = 0;
        t = 0;
        r = 0;
        b = 0;
    }
    MRECT c = {l, t, r, b};
    return c;
}

void MiDualCamBokeh::CopyToAlgoImage(struct MiImageBuffer *pSrcImg, AlgoBokehImg *pDstImg)
{
    if (pSrcImg == NULL || pDstImg == NULL)
        return;

    int32_t i;
    uint8_t *l_pSrc = NULL;
    uint8_t *l_pDst = NULL;

    l_pSrc = pSrcImg->plane[0];
    l_pDst = (uint8_t *)pDstImg->pData;
    for (i = 0; i < pDstImg->height; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->width);
        l_pDst += pDstImg->stride;
        l_pSrc += pSrcImg->stride;
    }

    l_pSrc = pSrcImg->plane[1];
    for (i = 0; i < pDstImg->height / 2; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->width);
        l_pDst += pDstImg->stride;
        l_pSrc += pSrcImg->stride;
    }
}

void MiDualCamBokeh::CopyToMiImage(AlgoBokehImg *pSrcImg, struct MiImageBuffer *pDstImg)
{
    if (pSrcImg == NULL || pDstImg == NULL)
        return;

    int32_t i;
    uint8_t *l_pSrc = NULL;
    uint8_t *l_pDst = NULL;

    l_pDst = pDstImg->plane[0];
    l_pSrc = (uint8_t *)pSrcImg->pData;
    for (i = 0; i < pDstImg->height; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->width);
        l_pDst += pDstImg->stride;
        l_pSrc += pSrcImg->stride;
    }

    l_pDst = pDstImg->plane[1];
    for (i = 0; i < pDstImg->height / 2; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->width);
        l_pDst += pDstImg->stride;
        l_pSrc += pSrcImg->stride;
    }
}

void MiDualCamBokeh::CopyScreenToMiImage(ASVLOFFSCREEN *pSrcImg, struct MiImageBuffer *pDstImg)
{
    if (pSrcImg == NULL || pDstImg == NULL)
        return;

    int32_t i;
    uint8_t *l_pSrc = NULL;
    uint8_t *l_pDst = NULL;

    l_pDst = pDstImg->plane[0];
    l_pSrc = (uint8_t *)pSrcImg->ppu8Plane[0];
    for (i = 0; i < pDstImg->height; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->width);
        l_pDst += pDstImg->stride;
        l_pSrc += pSrcImg->pi32Pitch[0];
    }

    l_pDst = pDstImg->plane[1];
    for (i = 0; i < pDstImg->height / 2; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->width);
        l_pDst += pDstImg->stride;
        l_pSrc += pSrcImg->pi32Pitch[1];
    }
}

void MiDualCamBokeh::ReleaseImage(AlgoBokehImg *pImage)
{
    if (pImage->pData)
        free(pImage->pData);
    pImage->pData = NULL;
}

void MiDualCamBokeh::ReleaseScreenImage(ASVLOFFSCREEN *pImage)
{
    if (pImage->ppu8Plane[0])
        free(pImage->ppu8Plane[0]);
    pImage->ppu8Plane[0] = NULL;
}

int MiDualCamBokeh::processRelighting(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output,
                                      ASVLOFFSCREEN *bokehImg)
{
    int nRet = PROCSUCCESS;
    double t0, t1, t2;
    t0 = gettime();
    MIPLLIGHTPARAM LightParam = {0};
    LightParam.i32LightMode = m_lightingMode;

    MIIMAGEBUFFER SrcImg;
    MIIMAGEBUFFER DstImg;
    MIIMAGEBUFFER SrcDokehImg;

    PrepareRelightImage(SrcImg, *input);         // [in] The input image data
    PrepareRelightImage(DstImg, *output);        // [in] The output image data
    PrepareRelightImage(SrcDokehImg, *bokehImg); // [in] The bokeh image data

    m_useBokeh = false;
    MLOGI(Mia2LogGroupPlugin, "[MI_CTB] m_hRelightingHandle: %p", m_hRelightingHandle);
    if (m_hRelightingHandle) {
        MIPLDEPTHINFO lDepthInfo;
        MIPLDEPTHINFO *plDepthInfo = &lDepthInfo;
        lDepthInfo.pBlurImage = &SrcDokehImg;
        t1 = gettime();
        nRet = AILAB_PLI_Process(m_hRelightingHandle, plDepthInfo, &SrcImg, &LightParam, &DstImg);
        t2 = gettime();
        if (nRet != MOK) {
            MLOGE(Mia2LogGroupPlugin, "[MI_CTB] midebug: AILAB_PLI_Process failed %d\n", nRet);
            AILAB_PLI_Uninit(&m_hRelightingHandle);
            m_hRelightingHandle = NULL;
            m_useBokeh = true;
            return nRet;
        }
        AILAB_PLI_Uninit(&m_hRelightingHandle);
        m_hRelightingHandle = NULL;
        MLOGI(Mia2LogGroupPlugin, "[MI_CTB] done, set m_hRelightingHandle to: %p",
              m_hRelightingHandle);

        MLOGI(Mia2LogGroupPlugin, "[MI_CTB]:AILAB_PLI_Process= %d: mode=%d costime=%.2f\n", nRet,
              m_lightingMode, t2 - t0);
    } else {
        m_useBokeh = true;
    }
    return nRet;
}

void MiDualCamBokeh::PrepareRelightImage(MIIMAGEBUFFER &dstimage, ASVLOFFSCREEN &srtImage)
{
    dstimage.i32Width = srtImage.i32Width;
    dstimage.i32Height = srtImage.i32Height;
    dstimage.i32Scanline = srtImage.i32Height;

    if (srtImage.u32PixelArrayFormat == ASVL_PAF_NV12) {
        dstimage.u32PixelArrayFormat = FORMAT_YUV_420_NV12;
    } else if (srtImage.u32PixelArrayFormat == ASVL_PAF_NV21) {
        dstimage.u32PixelArrayFormat = FORMAT_YUV_420_NV21;
    }

    dstimage.ppu8Plane[0] = srtImage.ppu8Plane[0];
    dstimage.ppu8Plane[1] = srtImage.ppu8Plane[1];
    dstimage.pi32Pitch[0] = srtImage.pi32Pitch[0];
    dstimage.pi32Pitch[1] = srtImage.pi32Pitch[1];
}

void MiDualCamBokeh::GetImageScale(unsigned int width, unsigned int height,
                                   ALGO_CAPTURE_BOKEH_FRAME &Scale)
{
    int scale_4_3 = 10 * 4 / 3;
    int scale_16_9 = 10 * 16 / 9;
    int imagescale = 10 * width / height;

    if (scale_4_3 == imagescale) {
        Scale = CAPTURE_BOKEH_4_3;
    } else if (scale_16_9 == imagescale) {
        Scale = CAPTURE_BOKEH_16_9;
    } else {
        Scale = CAPTURE_BOKEH_FULL_FRAME;
    }

    MLOGI(Mia2LogGroupPlugin, "[MI_CTB] GetImageScale scale %d", Scale);
    return;
}
