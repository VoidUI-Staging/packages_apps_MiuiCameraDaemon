#include "ArcSoftCaptureBokehPlugin.h"

#include <MiaPluginUtils.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "arcsoft_debuglog.h"
#include "arcsoft_dualcam_image_refocus.h"
#include "arcsoft_dynamic_blur_control.h"
#include "arcsoft_utils.h"
#include "asvloffscreen.h"

using namespace mialgo2;

#define CODE_ENABLE 0

#define CAL_DATA_PATH       "/data/vendor/camera/"
#define CAL_DATA_PATH_W_T   "/data/vendor/camera/com.xiaomi.dcal.wt.data"
#define CAL_DATA_PATH_W_U   "/data/vendor/camera/com.xiaomi.dcal.wu.data"
#define CAL_DATA_PATH_FRONT "/data/vendor/camera/com.xiaomi.dcal.ffaux.data"

#define CALIBRATIONDATA_LEN 2048
#ifdef ANDROMEDA_CAM
#define CALIBRATIONDATA_OFFSET_W_T 5780
#else
#define CALIBRATIONDATA_OFFSET_W_T 7942
#endif
#define CALIBRATIONDATA_OFFSET_W_U   5732
#define CALIBRATIONDATA_OFFSET_FRONT 2911

#define ROLE_REARBOKEH2X 61
#define ROLE_REARBOKEH1X 63

#define MAX_LINKED_NUMBER 3

#define SHOW_WIDE_ONLY 1
#define SHOW_TELE_ONLY 2
#define SHOW_HALF_HALF 3

#define MI_BOKEH_LITE 2

#define SAFE_FREE(ptr) \
    if (ptr) {         \
        free(ptr);     \
        ptr = NULL;    \
    }

static inline std::string property_get_string(const char *key, const char *defaultValue)
{
    char value[PROPERTY_VALUE_MAX];
    property_get(key, value, defaultValue);
    return std::string(value);
}

static const int32_t sArccapBokehDump =
    property_get_int32("persist.vendor.camera.algoengine.arccapbokeh.dump", 0);
static const int32_t sArchsoftCbDumpimg =
    property_get_int32("persist.vendor.camera.arcsoft.cb.dumpimg", 0);
static const int32_t sArchsoftCbDebugView =
    property_get_int32("persist.vendor.camera.arcsoft.cb.debugview", 0);
static const int32_t sArchsoftCbDrawroi =
    property_get_int32("persist.vendor.camera.arcsoft.cb.drawroi", 0);
static const int32_t sArchsoftCbLogLevel =
    property_get_int32("persist.vendor.camera.arcsoft.cb.loglevel", 0);
static const int32_t sBokehBlur = property_get_int32("persist.vendor.camera.bokeh.blur", -1);
static const int32_t sBokehShineThres =
    property_get_int32("persist.vendor.camera.bokeh.shineThres", -1);
static const int32_t sBokehShineLevel =
    property_get_int32("persist.vendor.camera.bokeh.shineLevel", -1);
static const std::string sDeviceVersion = property_get_string("ro.boot.hwc", "0");
static const int32_t sBokehLite = property_get_int32("persist.vendor.camera.bokeh.LiteMode", 0);
static long long dumpTimeIndex;

// used for performance
static long long __attribute__((unused)) gettime()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((long long)time.tv_sec * 1000 + time.tv_usec / 1000);
}

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

typedef struct
{
    LPASVLOFFSCREEN pInputImage;
    LPASVLOFFSCREEN pOutputImage;
    BeautyFeatureParams beautyFeatureParams;
    float gender;
    int orient;
    int rearCamera;
} bokeh_facebeauty_params;

int get_device_region()
{
    const char *devicePtr = sDeviceVersion.c_str();
    if (strcasecmp(devicePtr, "India") == 0)
        return BEAUTYSHOT_REGION_INDIAN;
    else if (strcasecmp(devicePtr, "CN") == 0)
        return BEAUTYSHOT_REGION_CHINA;

    return BEAUTYSHOT_REGION_GLOBAL;
}

static void *bokeh_facebeauty_thread_proc(void *params)
{
    bokeh_facebeauty_params *beautyParams = (bokeh_facebeauty_params *)params;
    LPASVLOFFSCREEN pInputImage = beautyParams->pInputImage;
    LPASVLOFFSCREEN pOutputImage = beautyParams->pOutputImage;
    BeautyShot_Image_Algorithm *m_BeautyShot_Image_Algorithm = NULL;
    int orient = beautyParams->orient;
    int region_code = get_device_region();
    int mfeatureCounts = beautyParams->beautyFeatureParams.featureCounts;

    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB_BTY] orient=%d region=%d featureCounts=%d", orient,
          region_code, mfeatureCounts);
    double t0, t1;
    t0 = gettime();

    if (Create_BeautyShot_Image_Algorithm(BeautyShot_Image_Algorithm::CLASSID,
                                          &m_BeautyShot_Image_Algorithm) != MOK ||
        m_BeautyShot_Image_Algorithm == NULL) {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB_BTY] Create_BeautyShot_Image_Algorithm, fail");
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
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB_BTY] capture %s = %d\n",
              beautyParams->beautyFeatureParams.beautyFeatureInfo[i].tagName,
              beautyParams->beautyFeatureParams.beautyFeatureInfo[i].featureValue);
    }

    ABS_TFaces faceInfoIn, faceInfoOut;
    ret = m_BeautyShot_Image_Algorithm->Process(pInputImage, pOutputImage, &faceInfoIn,
                                                &faceInfoOut, orient, region_code);

    if (NULL != m_BeautyShot_Image_Algorithm) {
        m_BeautyShot_Image_Algorithm->UnInit();
        m_BeautyShot_Image_Algorithm->Release();
        m_BeautyShot_Image_Algorithm = NULL;
    }
    t1 = gettime();
    MLOGD(Mia2LogGroupPlugin,
          " [ARC_CTB_BTY] capture process YUV ret=%ld w:%d, h:%d, total "
          "time: %.2f",
          ret, pInputImage->i32Width, pInputImage->i32Height, t1 - t0);
    return NULL;
}

/*
   ------------------------------------------------------
   |   MIAI_ASD   |   ARCSOFT FILTER
   ------------------------------------------------------
   neg           0         ARC_DCR_FILTER_NONE
   flower        2         ARC_DCR_FILTER_FLOWER
   sky           5         ARC_DCR_FILTER_BLUESKY
   sunset        6         ARC_DCR_FILTER_SUNRISESET
   plant         9         ARC_DCR_FILTER_GREEN
   nightscene    10        ARC_DCR_FILTER_NIGHT
   snow          11        ARC_DCR_FILTER_SNOW
   waterside     12        ARC_DCR_FILTER_WATER
   autumn        13        ARC_DCR_FILTER_AUTUMN
   redleaf       17        ARC_DCR_FILTER_MAPLELEAF
   cloud         21        ARC_DCR_FILTER_CLOUDY
   beach         26        ARC_DCR_FILTER_BEACH
   driving       27        ARC_DCR_FILTER_SEABED
 */
MInt32 readASDinfo(int32_t asdInfo)
{
    MInt32 i32FilterMode = ARC_DCR_FILTER_NONE;
    switch (asdInfo) {
    case 2:
        i32FilterMode = ARC_DCR_FILTER_FLOWER;
        break;
    case 5:
        i32FilterMode = ARC_DCR_FILTER_BLUESKY;
        break;
    case 6:
        i32FilterMode = ARC_DCR_FILTER_SUNRISESET;
        break;
    case 9:
        i32FilterMode = ARC_DCR_FILTER_GREEN;
        break;
    case 10:
        i32FilterMode = ARC_DCR_FILTER_NIGHT;
        break;
    case 11:
        i32FilterMode = ARC_DCR_FILTER_SNOW;
        break;
    case 12:
        i32FilterMode = ARC_DCR_FILTER_WATER;
        break;
    case 13:
        i32FilterMode = ARC_DCR_FILTER_AUTUMN;
        break;
    case 17:
        i32FilterMode = ARC_DCR_FILTER_MAPLELEAF;
        break;
    case 21:
        i32FilterMode = ARC_DCR_FILTER_CLOUDY;
        break;
    case 32:
        i32FilterMode = ARC_DCR_FILTER_BEACH;
        break;
    case 33:
        i32FilterMode = ARC_DCR_FILTER_SEABED;
        break;
    case 0:
    default:
        i32FilterMode = ARC_DCR_FILTER_NONE;
        break;
    }

    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] filtermode: %d", i32FilterMode);
    return i32FilterMode;
}

ArcCapBokehPlugin::~ArcCapBokehPlugin() {}

int ArcCapBokehPlugin::deInit()
{
    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] E");

    if (mCaptureBokeh) {
        ARC_DCIR_Uninit(&mCaptureBokeh);
        mCaptureBokeh = NULL;
    }

    SAFE_FREE(mBokehParam.faceParam.pi32FaceAngles);
    SAFE_FREE(mBokehParam.faceParam.prtFaces);
    SAFE_FREE(mCalibrationFront);
    SAFE_FREE(mCalibrationWU);
    SAFE_FREE(mCalibrationWT);

    mInt32DisparityDataActiveSize = 0;
    mIsSetCaliFinish = false;

    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] X");
    return PROCSUCCESS;
}

int ArcCapBokehPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mCaptureBokeh = NULL;
    mCalibrationFront = NULL;
    mCalibrationWU = NULL;
    mCalibrationWT = NULL;
    mDumpYUV = false;
    mDumpPriv = false;
    mDrawROI = 0;
    mViewCtrl = 0;
    mLightingMode = 0;
    mOrient = 0;
    mBeautyEnabled = 0;
    mFrameworkCameraId = createInfo->frameworkCameraId;
    mProcessCount = 0;
    mMaxParallelNum = 0;
    mCurrentParallelNum = 0;
    mNodeInterface = nodeInterface;
    mIsSetCaliFinish = false;
    mMaxParallelNum = 0;
    mCurrentParallelNum = 0;
    mCalcMode = ARC_DCIR_BOKEH_NORMAL;
    mThermallevel = 0;
    mMainPhysicalMeta = NULL;
    mAuxPhysicalMeta = NULL;

    if (CAM_COMBMODE_FRONT == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_BOKEH == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_AUX == mFrameworkCameraId) {
        mRearCamera = false;
    } else {
        mRearCamera = true;
    }

    if (0xFF12 == createInfo->operationMode || 0xFF15 == createInfo->operationMode) {
        mSDKMode = true;
    } else {
        mSDKMode = false;
    }

    memset(&mBokehParam, 0, sizeof(ARC_DCIR_PARAM));
    memset(&mFrameParams, 0, sizeof(frame_params_t));
    memset(&mRtFocusROIOnInputData, 0, sizeof(CHIRECT));

    MLOGI(Mia2LogGroupPlugin, " [ARC_CTB] init mRearCamera:%d, cameraId: 0x%x, mSDKMode:%d",
          mRearCamera, mFrameworkCameraId, mSDKMode);

    return iniEngine();
}

int ArcCapBokehPlugin::iniEngine()
{
    int ret = PROCSUCCESS;
    MRESULT res = 0;

    MPBASE_Version *version = NULL;
    mBlurLevel = 0;
    if (mBokehParam.faceParam.pi32FaceAngles == NULL && mBokehParam.faceParam.prtFaces == NULL) {
        mBokehParam.faceParam.prtFaces = (PMRECT)malloc(sizeof(MRECT) * MAX_FACE_NUM);
        mBokehParam.faceParam.pi32FaceAngles = (MInt32 *)malloc(sizeof(MInt32) * MAX_FACE_NUM);
        memset(mBokehParam.faceParam.prtFaces, 0, sizeof(MRECT) * MAX_FACE_NUM);
        memset(mBokehParam.faceParam.pi32FaceAngles, 0, sizeof(MInt32) * MAX_FACE_NUM);

        mBokehParam.faceParam.i32FacesNum = 0;
    }

    MLOGI(Mia2LogGroupPlugin, " [ARC_CTB] arcoft sdk log level %d", sArchsoftCbLogLevel);
    ARC_DCIR_SetLogLevel(sArchsoftCbLogLevel);

    version = (MPBASE_Version *)ARC_DCIR_GetVersion();
    MLOGI(Mia2LogGroupPlugin, " [ARC_CTB] Arcsoft Bokeh Algo Lib Version: %s cameraId %d",
          version->Version, mFrameworkCameraId);

    res = ARC_DCIR_Init(&mCaptureBokeh, ARC_DCIR_NORMAL_MODE);
    if (res) {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCVR_Init Faild res = %ld", res);
        return PROCFAILED;
    } else {
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCVR_Init Successfull");
    }

    if (mRearCamera) {
        mCalibrationWU = malloc(CALIBRATIONDATA_LEN);
        mCalibrationWT = malloc(CALIBRATIONDATA_LEN);
        readCalibrationDataFromFile(CAL_DATA_PATH_W_U, mCalibrationWU, CALIBRATIONDATA_LEN);
        readCalibrationDataFromFile(CAL_DATA_PATH_W_T, mCalibrationWT, CALIBRATIONDATA_LEN);
    } else {
        mCalibrationFront = malloc(CALIBRATIONDATA_LEN);
        readCalibrationDataFromFile(CAL_DATA_PATH_FRONT, mCalibrationFront, CALIBRATIONDATA_LEN);
    }

    // mBeautyFeatureParams initialized by BeautyFeatureParams in SkinBeautifierDef.h define
    // so not memset mBeautyFeatureParams to 0 in initialize function
    int beautyFeatureCounts = mBeautyFeatureParams.featureCounts;
    for (int i = 0; i < beautyFeatureCounts; i++) {
        mBeautyFeatureParams.beautyFeatureInfo[i].featureValue = 0;
    }

    return ret;
}

int ArcCapBokehPlugin::readCalibrationDataFromFile(const char *szFile, void *caliBuff,
                                                   uint32_t dataSize)
{
    if (dataSize == 0 || NULL == caliBuff) {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] Invalid param");
        return MIA_RETURN_INVALID_PARAM;
    }

    uint32_t bytesRead = 0;
    FILE *caliFile = NULL;
    caliFile = fopen(szFile, "r");
    if (NULL != caliFile) {
        bytesRead = fread(caliBuff, 1, dataSize, caliFile);
        MLOGI(Mia2LogGroupPlugin, " [ARC_CTB] read calidata bytesRead = %d, dataSize:%d", bytesRead,
              dataSize);
        fclose(caliFile);
    } else {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] open caliFile(%s) failed %s", szFile,
              strerror(errno));
        return MIA_RETURN_OPEN_FAILED;
    }
    if (bytesRead != dataSize) {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] read incorrect size %d vs %d", bytesRead, dataSize);
        return MIA_RETURN_UNKNOWN_ERROR;
    }

    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] read %s success !", szFile);
    return MIA_RETURN_OK;
}

ProcessRetStatus ArcCapBokehPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    int32_t iIsDumpData = (int32_t)sArccapBokehDump;

    mMaxParallelNum = pProcessRequestInfo->maxConcurrentNum;
    mCurrentParallelNum = pProcessRequestInfo->runningTaskNum;
    std::vector<ImageParams> input_tele_vector;
    std::vector<ImageParams> input_wide_vector;

    for (auto &imageParams : pProcessRequestInfo->inputBuffersMap) {
        std::vector<ImageParams> &inputs = imageParams.second;
        uint32_t portId = imageParams.first;
        void *pData = NULL;
        MLOGI(Mia2LogGroupPlugin, "input portId: %d size: %d", portId, inputs.size());

        for (auto &input : inputs) {
            camera_metadata_t *pMeta = input.pMetadata;
            int ret = -1;
            ret = VendorMetadataParser::getTagValue(pMeta, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                                    &pData);
            if (PROCSUCCESS == ret && NULL != pData) {
                int expValue = *static_cast<int32_t *>(pData);
                MLOGI(Mia2LogGroupPlugin, "[ARC_CTB] get EV %d", expValue);
            } else {
                MLOGE(Mia2LogGroupPlugin, "[ARC_CTB] get EV fail");
            }

            if (portId == 0) {
                input_tele_vector.push_back(input);
                if (1 == input_tele_vector.size()) {
                    mMainPhysicalMeta = input.pPhyCamMetadata;
                }
            } else if (portId == 1) {
                input_wide_vector.push_back(input);
                if (1 == input_wide_vector.size()) {
                    mAuxPhysicalMeta = input.pPhyCamMetadata;
                }
            }
        }
    }
    MLOGI(Mia2LogGroupPlugin, "[ARC_CTB] input_tele_vector size:  %d, input_wide_vector size : %d",
          input_tele_vector.size(), input_wide_vector.size());

    uint32_t outputPorts = pProcessRequestInfo->outputBuffersMap.size();
    struct MiImageBuffer outputBokehFrame, outputTeleFrame, outputDepth;
    memset(&outputTeleFrame, 0, sizeof(MiImageBuffer));
    memset(&outputDepth, 0, sizeof(MiImageBuffer));

    auto &outBokehBuffers = pProcessRequestInfo->outputBuffersMap.at(0);

    convertImageParams(outBokehBuffers[0], outputBokehFrame);

    mOutputNum = outputPorts;
    if (mOutputNum == 3) {
        auto &outTeleBuffers = pProcessRequestInfo->outputBuffersMap.at(1);
        auto &outDepthBuffers = pProcessRequestInfo->outputBuffersMap.at(2);

        convertImageParams(outTeleBuffers[0], outputTeleFrame);
        convertImageParams(outDepthBuffers[0], outputDepth);
    }

    for (int i = 0; i < input_tele_vector.size(); i++) {
        void *pMetaData = NULL;
        int teleExpValue = -1;
        int ret = -1;
        ret = VendorMetadataParser::getTagValue(
            input_tele_vector[i].pMetadata, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &pMetaData);
        if (PROCSUCCESS == ret && NULL != pMetaData) {
            teleExpValue = *static_cast<int32_t *>(pMetaData);
        } else {
            MLOGW(Mia2LogGroupPlugin, " [ARC_CTB] tele %d get EV fail", i);
        }

        MLOGI(Mia2LogGroupPlugin,
              " [ARC_CTB] input tele [%d/%d] EV %d, width: %d, height: %d, stride: %d, scanline: "
              "%d, format: %d",
              i + 1, input_tele_vector.size(), teleExpValue, input_tele_vector[i].format.width,
              input_tele_vector[i].format.height, input_tele_vector[i].planeStride,
              input_tele_vector[i].sliceheight, input_tele_vector[i].format.format);
    }

    for (int i = 0; i < input_wide_vector.size(); i++) {
        void *pMetaData = NULL;
        int wideExpValue = -1;
        int ret = -1;
        ret = VendorMetadataParser::getTagValue(
            input_wide_vector[i].pMetadata, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &pMetaData);
        if (PROCSUCCESS == ret && NULL != pMetaData) {
            wideExpValue = *static_cast<int32_t *>(pMetaData);
        } else {
            MLOGW(Mia2LogGroupPlugin, " [ARC_CTB] wide %d get EV fail", i);
        }

        MLOGI(Mia2LogGroupPlugin,
              " [ARC_CTB] input wide [%d/%d] EV %d, width: %d, height: %d, stride: %d, scanline: "
              "%d, format: %d",
              i + 1, input_wide_vector.size(), wideExpValue, input_wide_vector[i].format.width,
              input_wide_vector[i].format.height, input_wide_vector[i].planeStride,
              input_wide_vector[i].sliceheight, input_wide_vector[i].format.format);
    }

    MLOGI(Mia2LogGroupPlugin,
          " [ARC_CTB] output bokeh width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          outputBokehFrame.width, outputBokehFrame.height, outputBokehFrame.stride,
          outputBokehFrame.scanline, outputBokehFrame.format);

    if (mOutputNum == 3) {
        MLOGI(Mia2LogGroupPlugin,
              " [ARC_CTB] output tele width: %d, height: %d, stride: %d, scanline: %d, format: %d",
              outputTeleFrame.width, outputTeleFrame.height, outputTeleFrame.stride,
              outputTeleFrame.scanline, outputTeleFrame.format);
        MLOGI(Mia2LogGroupPlugin,
              " [ARC_CTB] output depth width: %d, height: %d, stride: %d, scanline: %d, format: %d",
              outputDepth.width, outputDepth.height, outputDepth.stride, outputDepth.scanline,
              outputDepth.format);
    }

    if (iIsDumpData) {
        for (int i = 0; i < input_tele_vector.size(); i++) {
            DumpInputData(input_tele_vector[i], "arccapbokeh_input_tele", i);
        }
        for (int i = 0; i < input_wide_vector.size(); i++) {
            DumpInputData(input_wide_vector[i], "arccapbokeh_input_wide", i);
        }
    }

    int ret;
    double start = PluginUtils::nowMSec();
    if (mOutputNum == 1) {
        ret = processBuffer(input_tele_vector, input_wide_vector, &outputBokehFrame, nullptr,
                            nullptr);
    } else {
        ret = processBuffer(input_tele_vector, input_wide_vector, &outputBokehFrame,
                            &outputTeleFrame, &outputDepth);
    }
    double end = PluginUtils::nowMSec();

    if (ret != PROCSUCCESS) {
        resultInfo = PROCFAILED;
    }
    MLOGI(Mia2LogGroupPlugin, " [ARC_CTB] processBuffer ret: %d", ret);

    if (iIsDumpData) {
        char outputbokehbuf[128] = {0};
        snprintf(outputbokehbuf, sizeof(outputbokehbuf), "arccapbokeh_output_bokeh_%dx%d",
                 outputBokehFrame.width, outputBokehFrame.height);
        PluginUtils::dumpToFile(outputbokehbuf, &outputBokehFrame);
        if (mOutputNum == 3) {
            char outputtelebuf[128] = {0};
            snprintf(outputtelebuf, sizeof(outputtelebuf), "arccapbokeh_output_tele_%dx%d",
                     outputTeleFrame.width, outputTeleFrame.height);
            PluginUtils::dumpToFile(outputtelebuf, &outputTeleFrame);
            char outputdepthbuf[128] = {0};
            snprintf(outputdepthbuf, sizeof(outputdepthbuf), "arccapbokeh_output_depth_%dx%d",
                     outputDepth.width, outputDepth.height);
            PluginUtils::dumpToFile(outputdepthbuf, &outputDepth);
        }
    }

    if (mNodeInterface.pSetResultMetadata != NULL) {
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf),
                 "arcsoftDualBokeh:{processCount:%d blurLevel:%d beautyEnabled:%d hdrBokeh:%d "
                 "seBokeh:%d calcMode:%d costTime:%.3f}",
                 mProcessCount, mBlurLevel, mBeautyEnabled, mHdrEnabled, mNightLevel, mCalcMode,
                 end - start);
        std::string results(buf);
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, pProcessRequestInfo->frameNum,
                                          pProcessRequestInfo->timeStamp, results);
    }

    mProcessCount++;

    return resultInfo;
}

ProcessRetStatus ArcCapBokehPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int ArcCapBokehPlugin::processBuffer(std::vector<ImageParams> teleInputvector,
                                     std::vector<ImageParams> wideInputvector,
                                     struct MiImageBuffer *bokehOutput,
                                     struct MiImageBuffer *teleOutput,
                                     struct MiImageBuffer *depthOut)
{
    int result = PROCSUCCESS;
    int teleVecSize = teleInputvector.size();
    int wideVecSize = wideInputvector.size();
    if (teleVecSize <= 0 || wideVecSize <= 0 || bokehOutput == NULL) {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] error invalid param %d %d %p", teleVecSize,
              wideVecSize, bokehOutput);
        return PROCFAILED;
    }

    std::vector<struct MiImageBuffer *> teleimagevector, wideimagevector;
    struct MiImageBuffer teleInputFrame[ARC_DCIR_MAX_INPUT_IMAGES];
    struct MiImageBuffer wideInputFrame[ARC_DCIR_MAX_INPUT_IMAGES];
    std::vector<camera_metadata_t *> logicalMetaVector;
    for (int i = 0; i < teleVecSize && i < ARC_DCIR_MAX_INPUT_IMAGES; i++) {
        PrepareMiImageBuffer(&teleInputFrame[i], teleInputvector[i]);
        teleimagevector.push_back(&teleInputFrame[i]);
        logicalMetaVector.push_back(teleInputvector[i].pMetadata);
    }
    for (int i = 0; i < wideVecSize && i < ARC_DCIR_MAX_INPUT_IMAGES; i++) {
        PrepareMiImageBuffer(&wideInputFrame[i], wideInputvector[i]);
        wideimagevector.push_back(&wideInputFrame[i]);
    }
    uint8_t *depthOutput = depthOut ? depthOut->plane[0] : NULL;
    if (depthOutput != NULL) {
        memset(depthOutput, 0x0F, (depthOut->stride) * (depthOut->scanline));
    }

    result = setFrameParams(teleimagevector[0], wideimagevector[0], bokehOutput, teleOutput,
                            depthOut, logicalMetaVector);
    if (PROCSUCCESS != result) {
        PluginUtils::miCopyBuffer(bokehOutput, &teleInputFrame[0]);
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] ERR! setFrameParams failed:%d", result);
        return result;
    }

    checkPersist();

    ASVLOFFSCREEN &stWideImage = mFrameParams.auximage;
    ASVLOFFSCREEN &stTeleImage = mFrameParams.mainimg;
    ASVLOFFSCREEN &stDestImage = mFrameParams.outimage;

    dumpTimeIndex = gettime();

    switch (mViewCtrl) {
    case SHOW_WIDE_ONLY:
        copyImage(&stWideImage, &stDestImage);
        break;
    case SHOW_TELE_ONLY:
        copyImage(&stTeleImage, &stDestImage);
        break;
    case SHOW_HALF_HALF:
        copyImage(&stTeleImage, &stDestImage);
        mergeImage(&stDestImage, &stWideImage, 2);
        break;
    default:
        result = doProcess(teleimagevector);
        break;
    }

    if (mDrawROI) {
        DrawPoint(&stDestImage, mFrameParams.refocus.ptFocus, 10, 0);
        DrawRectROI(&stDestImage, mRtFocusROIOnInputData, 5, 0);
        for (int i = 0; i < mBokehParam.faceParam.i32FacesNum; i++)
            DrawRect(&stDestImage, mBokehParam.faceParam.prtFaces[i], 5, 4);
    }

    return result;
}

int ArcCapBokehPlugin::setFrameParams(struct MiImageBuffer *teleInput,
                                      struct MiImageBuffer *wideInput,
                                      struct MiImageBuffer *bokehOutput,
                                      struct MiImageBuffer *teleOutput,
                                      struct MiImageBuffer *depthOut,
                                      const std::vector<camera_metadata_t *> &logicalMetaVector)
{
    ASVLOFFSCREEN &stWideImage = mFrameParams.auximage;
    ASVLOFFSCREEN &stTeleImage = mFrameParams.mainimg;
    ASVLOFFSCREEN &stDestImage = mFrameParams.outimage;
    ASVLOFFSCREEN &stDepthImage = mFrameParams.depthimage;
    ASVLOFFSCREEN &stDestTeleImage = mFrameParams.outteleimage;

    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] (teleInput   ->plane = %p)", teleInput->plane);
    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] (wideInput   ->plane = %p)", wideInput->plane);
    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] (bokehOutput ->plane = %p)", bokehOutput->plane);
    if (depthOut)
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] (depthOut    ->plane = %p)", depthOut->plane);
    if (teleOutput)
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] (teleOutput  ->plane = %p)", teleOutput->plane);

    WeightedRegion focusROI = {0};
    uint32_t afStatus;
    prepareImage(teleInput, stTeleImage);
    prepareImage(wideInput, stWideImage);
    prepareImage(bokehOutput, stDestImage);
    if (teleOutput)
        prepareImage(teleOutput, stDestTeleImage);
    if (depthOut)
        prepareImage(depthOut, stDepthImage);

    int ret = -1;
    void *localMetadata = NULL;
    const char *hdrenabletag = "xiaomi.bokeh.hdrEnabled";
    ret = VendorMetadataParser::getVTagValue(logicalMetaVector[0], hdrenabletag, &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        mHdrEnabled = *static_cast<bool *>(localMetadata);
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] hdrEnabled: %d", mHdrEnabled);
    } else {
        mHdrEnabled = false;
        MLOGI(Mia2LogGroupPlugin, " [ARC_CTB] get BokehHDRenabled failed");
    }
    if (mHdrEnabled && (logicalMetaVector.size() < 2)) {
        // to cover the case: app request mfnr capture since ev changed
        MLOGW(Mia2LogGroupPlugin, " [ARC_CTB] input fewer than 2, set hdrEnabled to false");
        mHdrEnabled = false;
    }

    camera_metadata_t *pLogicalMetaData = mHdrEnabled ? logicalMetaVector[1] : logicalMetaVector[0];

    if (NULL == pLogicalMetaData) {
        MLOGW(Mia2LogGroupPlugin, " [ARC_CTB] get pLogicalMetaData is NULL!");
        return PROCFAILED;
    }

    ret = -1;
    localMetadata = NULL;
    mCaptureMode = 1;
    mNightLevel = 0;
    uint8_t mSuperNightEnable;
    const char *supernightenable = "xiaomi.bokeh.superNightEnabled";
    ret = VendorMetadataParser::getVTagValue(pLogicalMetaData, supernightenable, &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        mSuperNightEnable = *static_cast<uint8_t *>(localMetadata);
        if (mSuperNightEnable == 1) {
            mCaptureMode = 2;
            localMetadata = NULL;
            const char *nightleveltag = "xiaomi.mivi.supernight.mode";
            int ret =
                VendorMetadataParser::getVTagValue(pLogicalMetaData, nightleveltag, &localMetadata);
            if (PROCSUCCESS == ret && NULL != localMetadata) {
                int nightmode = *static_cast<int *>(localMetadata);
                if (nightmode == 1) {
                    mNightLevel = 1;
                } else if (nightmode == 5) {
                    mNightLevel = 2;
                }
            }
        }
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] CaptureMode: %d, NightLevel: %d", mCaptureMode,
              mNightLevel);
    }

    ret = -1;
    localMetadata = NULL;
    const char *thermallevel = "xiaomi.thermal.thermalLevel";
    ret = VendorMetadataParser::getVTagValue(pLogicalMetaData, thermallevel, &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        mThermallevel = *static_cast<uint32_t *>(localMetadata);
        MLOGD(Mia2LogGroupPlugin, "thermallevel: %d ", mThermallevel);
    }

    if ((mCurrentParallelNum >= (mMaxParallelNum - 1) || mThermallevel > 1) &&
        mCalcMode == ARC_DCIR_BOKEH_NORMAL) {
        mCalcMode = ARC_DCIR_BOKEH_LITE;
    } else if (mCurrentParallelNum <= 2 && mCalcMode == ARC_DCIR_BOKEH_LITE && mThermallevel <= 1) {
        mCalcMode = ARC_DCIR_BOKEH_NORMAL;
    }

    // for debug
    if (mCalcMode == ARC_DCIR_BOKEH_LITE) {
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] CalcMode: ARC_DCIR_BOKEH_LITE");
    } else if (mCalcMode == MI_BOKEH_LITE) {
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] CalcMode: MI_BOKEH_LITE");
    } else {
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] CalcMode: ARC_DCIR_BOKEH_NORMAL");
    }

    ret = -1;
    localMetadata = NULL;
    InputMetadataBokeh *inputCTB = NULL;
    const char *bokehMetadata = "com.qti.chi.multicamerainputmetadata.InputMetadataBokeh";
    ret = VendorMetadataParser::getVTagValue(pLogicalMetaData, bokehMetadata, &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        inputCTB = static_cast<InputMetadataBokeh *>(localMetadata);
    } else {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] get InputMetadataBokeh failed!");
        return PROCFAILED;
    }

    if (false == mIsSetCaliFinish) {
        ARC_DC_CALDATA caliData = {0};
        caliData.i32CalibDataSize = CALIBRATIONDATA_LEN;
        if (ROLE_REARBOKEH2X == inputCTB->bokehRole) {
            caliData.pCalibData = mCalibrationWT;
        } else {
            caliData.pCalibData = mCalibrationWU;
        }

        if (NULL != caliData.pCalibData) {
            MRESULT res = ARC_DCIR_SetCaliData(mCaptureBokeh, &caliData);
            if (res) {
                MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCVR_SetCaliData fail: %ld", res);
            } else {
                MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCIR_SetCaliData Successfull");
            }
        } else {
            MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] CalibData is NULL!");
        }
        mIsSetCaliFinish = true;

        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] bokehRole %d", inputCTB->bokehRole);
    }

    ret = -1;
    int orient = 0;
    localMetadata = NULL;
    ret = VendorMetadataParser::getTagValue(pLogicalMetaData, ANDROID_JPEG_ORIENTATION,
                                            &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        orient = *static_cast<int *>(localMetadata);
    }
    mFaceOrientation = orient;
    mOrient = orient;
    switch (orient) {
    case 0:
        mFrameParams.i32Orientation = ARC_DCIR_REFOCUS_CAMERA_ROLL_0;
        break;
    case 90:
        mFrameParams.i32Orientation = ARC_DCIR_REFOCUS_CAMERA_ROLL_90;
        break;
    case 180:
        mFrameParams.i32Orientation = ARC_DCIR_REFOCUS_CAMERA_ROLL_180;
        break;
    case 270:
        mFrameParams.i32Orientation = ARC_DCIR_REFOCUS_CAMERA_ROLL_270;
        break;
    }
    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] cameraroll:%d", mFrameParams.i32Orientation);

    ret = -1;
    localMetadata = NULL;
    CHIRECT *pMainCropRegin = NULL;
    const char *fovdata = "xiaomi.snapshot.cropRegion";
    ret = VendorMetadataParser::getVTagValue(mMainPhysicalMeta, fovdata, &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        pMainCropRegin = static_cast<CHIRECT *>(localMetadata);
        MLOGI(Mia2LogGroupPlugin, "[ARC_CTB]Main camera CropRegion [%d %d %d %d]",
              pMainCropRegin->left, pMainCropRegin->top, pMainCropRegin->width,
              pMainCropRegin->height);
        mFrameParams.caminfo.i32MainWidth_CropNoScale = pMainCropRegin->width;
        mFrameParams.caminfo.i32MainHeight_CropNoScale =
            pMainCropRegin->width * stTeleImage.i32Height / stTeleImage.i32Width;
    } else {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] get pMainCropRegin failed!");
        mFrameParams.caminfo.i32MainWidth_CropNoScale = stTeleImage.i32Width;
        mFrameParams.caminfo.i32MainHeight_CropNoScale = stTeleImage.i32Height;
    }

    ret = -1;
    localMetadata = NULL;
    CHIRECT *pAuxCropRegin = NULL;
    ret = VendorMetadataParser::getVTagValue(mAuxPhysicalMeta, fovdata, &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        pAuxCropRegin = static_cast<CHIRECT *>(localMetadata);
        pAuxCropRegin->width = stWideImage.i32Width; // hard code
        pAuxCropRegin->height = stWideImage.i32Height;
        MLOGI(Mia2LogGroupPlugin, "[ARC_CTB] Aux camera CropRegion [%d %d %d %d]",
              pAuxCropRegin->left, pAuxCropRegin->top, pAuxCropRegin->width, pAuxCropRegin->height);
        mFrameParams.caminfo.i32AuxWidth_CropNoScale = pAuxCropRegin->width;
        mFrameParams.caminfo.i32AuxHeight_CropNoScale = pAuxCropRegin->height;
    } else {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] get AuxCropRegin failed!");
        mFrameParams.caminfo.i32AuxWidth_CropNoScale = stWideImage.i32Width;
        mFrameParams.caminfo.i32AuxHeight_CropNoScale = stWideImage.i32Height;
    }

    ret = -1;
    localMetadata = NULL;
    int32_t *pAfRegion = NULL;
    uint32_t afData = ANDROID_CONTROL_AF_REGIONS;
    ret = VendorMetadataParser::getTagValue(mMainPhysicalMeta, afData, &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        pAfRegion = static_cast<int32_t *>(localMetadata);
    } else {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] get AfRegion failed!");
    }
    CHIRECT afRect = {uint32_t(pAfRegion[0]), uint32_t(pAfRegion[1]),
                      uint32_t(pAfRegion[2] - pAfRegion[0]), uint32_t(pAfRegion[3] - pAfRegion[1])};
    memset(&mRtFocusROIOnInputData, 0, sizeof(CHIRECT));
    mapRectToInputData(afRect, *pMainCropRegin, stTeleImage.i32Width, stTeleImage.i32Height,
                       mRtFocusROIOnInputData);
    mFrameParams.refocus.ptFocus.x = mRtFocusROIOnInputData.left + mRtFocusROIOnInputData.width / 2;
    mFrameParams.refocus.ptFocus.y = mRtFocusROIOnInputData.top + mRtFocusROIOnInputData.height / 2;

    if (mFrameParams.refocus.ptFocus.x == 0 && mFrameParams.refocus.ptFocus.y == 0) {
        mFrameParams.refocus.ptFocus.x = mFrameParams.mainimg.i32Width / 2; /******hard code*******/
        mFrameParams.refocus.ptFocus.y =
            mFrameParams.mainimg.i32Height / 2; /******hard code*******/
    }
    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB]  ptFocus:[%d,%d]", mFrameParams.refocus.ptFocus.x,
          mFrameParams.refocus.ptFocus.y);

    ret = -1;
    localMetadata = NULL;
    FaceResult *pFaceResult = NULL;
    const char *pFaceData = "xiaomi.snapshot.faceRect";
    ret = VendorMetadataParser::getVTagValue(mMainPhysicalMeta, pFaceData, &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        pFaceResult = static_cast<FaceResult *>(localMetadata);
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] get FaceResult!");
    } else {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] get Face  failed!");
        // return PROCFAILED;
    }
    // for Face info
    convertToArcFaceInfo(pFaceResult, mBokehParam.faceParam, *pMainCropRegin, stTeleImage.i32Width,
                         stTeleImage.i32Height);

    MLOGD(Mia2LogGroupPlugin,
          " [ARC_CTB] "
          "LeftFullWidth:%d, LeftFullHeight:%d, RightFullWidth:%d, RightFullHeight:%d",
          mFrameParams.caminfo.i32MainWidth_CropNoScale,
          mFrameParams.caminfo.i32MainHeight_CropNoScale,
          mFrameParams.caminfo.i32AuxWidth_CropNoScale,
          mFrameParams.caminfo.i32AuxHeight_CropNoScale);

    mBokehParam.fMaxFOV = 78.5;

    // the correspond positions of main and aux camera
    mBokehParam.i32ImgDegree = mRearCamera ? CAPTURE_BOKEH_DEGREE_0 : CAPTURE_BOKEH_DEGREE_270;
    if (ROLE_REARBOKEH2X == inputCTB->bokehRole) {
        mBokehParam.i32ImgDegree = CAPTURE_BOKEH_DEGREE_WT;
        mMappingVersion = ArcSoftBokeh2x;
    } else {
        mBokehParam.i32ImgDegree = CAPTURE_BOKEH_DEGREE_WU;
        mMappingVersion = ArcSoftBokeh1x;
    }

    // get BokehFNumber
    ret = -1;
    localMetadata = NULL;
    char fNumber[8] = {0};
    const char *BokehFNumber = "xiaomi.bokeh.fNumberApplied";
    ret = VendorMetadataParser::getVTagValue(pLogicalMetaData, BokehFNumber, &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        memcpy(fNumber, static_cast<char *>(localMetadata), sizeof(fNumber));
        float fNumberApplied = atof(fNumber);
        mBlurLevel = findBlurLevelByCaptureType(fNumberApplied, false, inputCTB->bokehRole);
        mFrameParams.refocus.i32BlurIntensity = (sBokehBlur != -1) ? sBokehBlur : mBlurLevel;
        MLOGI(Mia2LogGroupPlugin, " [ARC_CTB] fNumberApplied %f, BlurLevel = %d", fNumberApplied,
              mBlurLevel);
    }

    localMetadata = NULL;
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

            localMetadata = NULL;
            ret = -1;
            ret = VendorMetadataParser::getVTagValue(pLogicalMetaData, tagStr, &localMetadata);
            if (PROCSUCCESS == ret && localMetadata != NULL) {
                mBeautyFeatureParams.beautyFeatureInfo[i].featureValue =
                    *static_cast<int *>(localMetadata);
                MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] Get Metadata Value TagName: %s, Value: %d",
                      tagStr, mBeautyFeatureParams.beautyFeatureInfo[i].featureValue);
                if (mBeautyFeatureParams.beautyFeatureInfo[i].featureValue != 0) {
                    // feature vlude support -100 -- + 100, 0 means no effect
                    mBeautyEnabled = true;
                }
            } else {
                MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] Get Metadata Failed! TagName: %s", tagStr);
            }
        }
    }

    ret = -1;
    localMetadata = NULL;
    const char *faceAnalyzeResult = "xiaomi.faceAnalyzeResult.result";
    ret = VendorMetadataParser::getVTagValue(pLogicalMetaData, faceAnalyzeResult, &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        memcpy(&mFaceAnalyze, localMetadata, sizeof(OutputMetadataFaceAnalyze));
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] getMetaData faceNum %d", mFaceAnalyze.faceNum);
    }

    ret = -1;
    localMetadata = NULL;
    const char *tagMultiCameraIds = "com.qti.chi.multicamerainfo.MultiCameraIds";
    ret = VendorMetadataParser::getVTagValue(pLogicalMetaData, tagMultiCameraIds, &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        MultiCameraIds inputMetadata = *static_cast<MultiCameraIds *>(localMetadata);
    }

    ret = -1;
    localMetadata = NULL;
    ret = VendorMetadataParser::getTagValue(pLogicalMetaData, ANDROID_SENSOR_SENSITIVITY,
                                            &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        int iso = *static_cast<int *>(localMetadata);
        mFrameParams.isoValue = iso;
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] getMetaData iso: %d", iso);
    }

    ret = -1;
    localMetadata = NULL;
    const char *aecFrameControl = "org.quic.camera2.statsconfigs.AECFrameControl";
    ret = VendorMetadataParser::getVTagValue(pLogicalMetaData, aecFrameControl, &localMetadata);
    if (PROCSUCCESS == ret && NULL != localMetadata) {
        float lux_index = static_cast<AECFrameControl *>(localMetadata)->luxIndex;
        mFrameParams.luxIndex = lux_index;
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] getMetaData lux_index: %f", lux_index);
    }
    return PROCSUCCESS;
}

void ArcCapBokehPlugin::convertToArcFaceInfo(FaceResult *pFaceResult, ARC_DCR_FACE_PARAM &faceInfo,
                                             CHIRECT &IFERect, int &nDataWidth, int &nDataHeight)
{
    if (faceInfo.prtFaces == NULL || faceInfo.pi32FaceAngles == NULL)
        return;
    if (pFaceResult == NULL) {
        faceInfo.i32FacesNum = 0;
        return;
    }
    faceInfo.i32FacesNum = pFaceResult->faceNumber;
    CHIRECT rtOnInputData;
    for (int i = 0; i < pFaceResult->faceNumber && i < MAX_FACE_NUM; ++i) {
        UINT32 dataIndex = 0;
        int32_t xMin = pFaceResult->chiFaceInfo[i].left;
        int32_t yMin = pFaceResult->chiFaceInfo[i].bottom;
        int32_t xMax = pFaceResult->chiFaceInfo[i].right;
        int32_t yMax = pFaceResult->chiFaceInfo[i].top;
        CHIRECT tmpFaceRect = {uint32_t(xMin), uint32_t(yMin), uint32_t(xMax - xMin + 1),
                               uint32_t(yMax - yMin + 1)};

        memset(&rtOnInputData, 0, sizeof(CHIRECT));
        mapRectToInputData(tmpFaceRect, IFERect, nDataWidth, nDataHeight, rtOnInputData);
        faceInfo.prtFaces[i].left = rtOnInputData.left;
        faceInfo.prtFaces[i].top = rtOnInputData.top;
        faceInfo.prtFaces[i].right = rtOnInputData.left + rtOnInputData.width - 1;
        faceInfo.prtFaces[i].bottom = rtOnInputData.top + rtOnInputData.height - 1;
        faceInfo.pi32FaceAngles[i] = getFaceDegree(mFaceOrientation);
        MLOGD(Mia2LogGroupPlugin,
              " [ARC_CTB] convertToArcFaceInfo capture facenum = %d,face[%d] = "
              "(l,t,w,h)[%d,%d,%d,%d],faceDegree = %d",
              faceInfo.i32FacesNum, i, rtOnInputData.left, rtOnInputData.top, rtOnInputData.width,
              rtOnInputData.height, getFaceDegree(mFaceOrientation));
    }
}

int ArcCapBokehPlugin::getFaceDegree(int faceOrientation)
{
    switch (faceOrientation) {
    case 0:
        return ARC_DCR_FOC_0;
    case 90:
        return ARC_DCR_FOC_90;
    case 180:
        return ARC_DCR_FOC_180;
    case 270:
        return ARC_DCR_FOC_270;
    }

    return ARC_DCR_FOC_0;
}

void ArcCapBokehPlugin::mapRectToInputData(CHIRECT &rtSrcRect, CHIRECT &rtIFERect, int &nDataWidth,
                                           int &nDataHeight, CHIRECT &rtDstRectOnInputData)
{
    MLOGD(Mia2LogGroupPlugin,
          " [ARC_CTB] rtSrcRect w:%d, h:%d, rtIFERect w:%d, h:%d, nDataWidth:%d, nDataHeight:%d",
          rtSrcRect.width, rtSrcRect.height, rtIFERect.width, rtIFERect.height, nDataWidth,
          nDataHeight);
    if (rtSrcRect.width <= 0 || rtSrcRect.height <= 0 || rtIFERect.width <= 0 ||
        rtIFERect.height <= 0 || nDataWidth <= 0 || nDataHeight <= 0)
        return;

    CHIRECT rtSrcOnIFE = rtSrcRect;

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
    float fTBRatio = nDataHeight / (float)rtIFERect.height;
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
          " [ARC_CTB] mapRectToInputData rtSrcRect(l,t,w,h)= [%d,%d,%d,%d], rtIFERect(l,t,w,h)= "
          "[%d,%d,%d,%d], rtDstRectOnInputData[%d,%d,%d,%d]",
          rtSrcRect.left, rtSrcRect.top, rtSrcRect.width, rtSrcRect.height, rtIFERect.left,
          rtIFERect.top, rtIFERect.width, rtIFERect.height, rtDstRectOnInputData.left,
          rtDstRectOnInputData.top, rtDstRectOnInputData.width, rtDstRectOnInputData.height);
}

void ArcCapBokehPlugin::prepareImage(struct MiImageBuffer *image, ASVLOFFSCREEN &stImage)
{
    MLOGD(Mia2LogGroupPlugin,
          " [ARC_CTB] prepareImage width: %d, height: %d, stride: %d, sliceHeight:%d, format:%d, "
          "planes: %d, plane[0]: %p, plane[1]: %p",
          image->width, image->height, image->stride, image->scanline, image->format,
          image->numberOfPlanes, image->plane[0], image->plane[1]);
    stImage.i32Width = image->width;
    stImage.i32Height = image->height;

    if (image->format != CAM_FORMAT_YUV_420_NV12 && image->format != CAM_FORMAT_YUV_420_NV21 &&
        image->format != CAM_FORMAT_Y16) {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] format[%d] is not supported!", image->format);
        return;
    }
    if (image->format == CAM_FORMAT_YUV_420_NV12) {
        stImage.u32PixelArrayFormat = ASVL_PAF_NV12;
    } else if (image->format == CAM_FORMAT_YUV_420_NV21) {
        stImage.u32PixelArrayFormat = ASVL_PAF_NV21;
    } else if (image->format == CAM_FORMAT_Y16) {
        stImage.u32PixelArrayFormat = ASVL_PAF_GRAY;
    }

    for (uint32_t i = 0; i < image->numberOfPlanes; i++) {
        stImage.ppu8Plane[i] = image->plane[i];
        stImage.pi32Pitch[i] = image->stride;
    }
    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] X");
}

int ArcCapBokehPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

void ArcCapBokehPlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);
    deInit();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ArcCapBokehPlugin::CreateHDRImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MRESULT ArcCapBokehPlugin::CreateHDRImage(std::vector<struct MiImageBuffer *> teleInputvector,
                                          LPASVLOFFSCREEN pEnhanceOffscreen, MRECT *cropRect,
                                          uint8_t *pDisparityData)
{
    if ((NULL == pEnhanceOffscreen) || (teleInputvector.size() < 2)) {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB_HDR] invalid input parameters");
        return MERR_INVALID_PARAM;
    }
    MRESULT res = MOK;

    ARC_DCIR_HDR_PARAM hdrParam;
    memset(&hdrParam, 0, sizeof(ARC_DCIR_HDR_PARAM));

    res = ARC_DCIR_GetDefaultParam_HDR(mCaptureBokeh, &hdrParam);
    if (res) {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB_HDR] Get Default Param Fail");
    }

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.hdrbokeh.i32ToneLength", prop, "-1");
    MInt32 i32ToneLength = atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.hdrbokeh.i32Brightness", prop, "-1");
    MInt32 i32Brightness = atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.hdrbokeh.i32Brightness", prop, "-1");
    MInt32 i32Saturation = atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.hdrbokeh.i32Contrast", prop, "-1");
    MInt32 i32Contrast = atoi(prop);

    hdrParam.i32ToneLength = i32ToneLength >= 0 ? i32ToneLength : 20;
    hdrParam.i32Brightness = i32Brightness >= 0 ? i32Brightness : 0;
    hdrParam.i32Saturation = i32Saturation >= 0 ? i32Saturation : 0;
    hdrParam.i32Contrast = i32Contrast >= 0 ? i32Contrast : 0;

    MLOGD(Mia2LogGroupPlugin,
          " [ARC_CTB_HDR] i32ToneLength:%d i32Brightness:%d i32Saturation:%d i32Contrast:%d",
          hdrParam.i32ToneLength, hdrParam.i32Brightness, hdrParam.i32Saturation,
          hdrParam.i32Contrast);

    ARC_DCIR_INPUTINFO mainImages;
    memset(&mainImages, 0, sizeof(ARC_DCIR_INPUTINFO));
    mainImages.lImgNum = teleInputvector.size();
    for (int i = 0; i < mainImages.lImgNum; i++) {
        mainImages.pImages[i] = (LPASVLOFFSCREEN)malloc(sizeof(ASVLOFFSCREEN));
        if (NULL == mainImages.pImages[i]) {
            MLOGE(Mia2LogGroupPlugin, " [ARC_CTB_HDR] malloc mainImages fail at %d", i);
            res = MERR_NO_MEMORY;
            break;
        }
        prepareImage(teleInputvector[i], *mainImages.pImages[i]);
    }

    for (int i = 0; i < mainImages.lImgNum; i++) {
        MLOGD(Mia2LogGroupPlugin,
              " [ARC_CTB_HDR] pImages[%d] format:%u, w:%d, h:%d, p0:%p, p1:%p p2:%p p3:%p pi0:%d "
              "pi1:%d pi2:%d pi3:%d",
              i, mainImages.pImages[i]->u32PixelArrayFormat, mainImages.pImages[i]->i32Width,
              mainImages.pImages[i]->i32Height, mainImages.pImages[i]->ppu8Plane[0],
              mainImages.pImages[i]->ppu8Plane[1], mainImages.pImages[i]->ppu8Plane[2],
              mainImages.pImages[i]->ppu8Plane[3], mainImages.pImages[i]->pi32Pitch[0],
              mainImages.pImages[i]->pi32Pitch[1], mainImages.pImages[i]->pi32Pitch[2],
              mainImages.pImages[i]->pi32Pitch[3]);
    }

    for (int i = 0; i < mBokehParam.faceParam.i32FacesNum; i++) {
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB_HDR] faceParam [%d] (%d %d %d %d)", i,
              mBokehParam.faceParam.prtFaces[i].left, mBokehParam.faceParam.prtFaces[i].top,
              mBokehParam.faceParam.prtFaces[i].right, mBokehParam.faceParam.prtFaces[i].bottom);
    }

    if (sArccapBokehDump && mainImages.lImgNum > 1) {
        dumpYUVData(mainImages.pImages[0], mProcessCount, "arccapbokeh_input_hdr_0");
        dumpYUVData(mainImages.pImages[1], mProcessCount, "arccapbokeh_input_hdr_1");
    }

    double t0 = PluginUtils::nowMSec();
    if (res == MOK) {
        res = ARC_DCIR_HDRProcess(mCaptureBokeh, &mainImages, pDisparityData,
                                  mInt32DisparityDataActiveSize, &hdrParam, &mBokehParam.faceParam,
                                  pEnhanceOffscreen, cropRect);
    }
    double t1 = PluginUtils::nowMSec();

    if (sArccapBokehDump) {
        dumpYUVData(pEnhanceOffscreen, 0, "arcsoftbokeh_output_hdr");
    }

    for (int i = 0; i < mainImages.lImgNum; i++) {
        if (mainImages.pImages[i]) {
            free(mainImages.pImages[i]);
        }
    }

    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB_HDR] ARC_DCIR_HDRProcess res = %ld, cost time %.3f", res,
          t1 - t0);

    return res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ArcCapBokehPlugin::doProcess
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ArcCapBokehPlugin::doProcess(std::vector<struct MiImageBuffer *> teleInputvector)
{
    MLOGI(Mia2LogGroupPlugin, " [ARC_CTB] (IN)");
    int result = MIA_RETURN_OK;
    MRESULT arcResult = MOK;
    double t0, t1, t2, t3, t4;
    t0 = PluginUtils::nowMSec();

    ASVLOFFSCREEN &stInAuxImage = mFrameParams.auximage;
    ASVLOFFSCREEN &stInMainImage = mFrameParams.mainimg;
    ASVLOFFSCREEN &stOutEffectImage = mFrameParams.outimage;
    ASVLOFFSCREEN &stOutDepthImage = mFrameParams.depthimage;
    ASVLOFFSCREEN &stOutSrcImage = mFrameParams.outteleimage;

    ASVLOFFSCREEN tmpHdrImage = {0};
    ASVLOFFSCREEN HdrcropImage = {0};
    uint8_t *pDisparityData = NULL;

    do {
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] stInMainImage %p, %p, %d, %d",
              stInMainImage.ppu8Plane[0], stInMainImage.ppu8Plane[1], stInMainImage.pi32Pitch[0],
              stInMainImage.pi32Pitch[1]);

        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] stInAuxImage %p, %p, %d, %d",
              stInAuxImage.ppu8Plane[0], stInAuxImage.ppu8Plane[1], stInAuxImage.pi32Pitch[0],
              stInAuxImage.pi32Pitch[1]);

        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] stOutEffectImage %p, %p, %d, %d",
              stOutEffectImage.ppu8Plane[0], stOutEffectImage.ppu8Plane[1],
              stOutEffectImage.pi32Pitch[0], stOutEffectImage.pi32Pitch[1]);

        arcResult = ARC_DCIR_Reset(mCaptureBokeh);
        if (arcResult) {
            MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCIR_Reset failed res = %ld", arcResult);
            result = MIA_RETURN_INVALID_PARAM;
            break;
        }

        arcResult = ARC_DCIR_SetCaptureMode(mCaptureBokeh, mCaptureMode);
        if (arcResult) {
            MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCIR_SetCaptureMode failed res = %ld",
                  arcResult);
            result = MIA_RETURN_INVALID_PARAM;
            break;
        }

        arcResult = ARC_DCIR_SetNightLevel(mCaptureBokeh, mNightLevel);
        if (arcResult) {
            MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCIR_SetNightLevel failed res = %ld",
                  arcResult);
            result = MIA_RETURN_INVALID_PARAM;
            break;
        }

        arcResult = ARC_DCIR_SetBokehCalcMode(mCaptureBokeh, mCalcMode);
        if (arcResult) {
            MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCIR_SetBokehCalcMode failed res = %ld",
                  arcResult);
            result = MIA_RETURN_INVALID_PARAM;
            break;
        }

        arcResult = ARC_DCIR_SetCameraImageInfo(mCaptureBokeh, &mFrameParams.caminfo);
        if (arcResult) {
            MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCIR_SetCameraImageInfo failed res = %ld",
                  arcResult);
            result = MIA_RETURN_INVALID_PARAM;
            break;
        }

        ARC_DCIR_SetCameraRoll(mCaptureBokeh, mFrameParams.i32Orientation);

        t1 = PluginUtils::nowMSec();
        arcResult =
            ARC_DCIR_CalcDisparityData(mCaptureBokeh, &stInMainImage, &stInAuxImage, &mBokehParam);
        t2 = PluginUtils::nowMSec();
        if (arcResult) {
            MLOGW(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCIR_CalcDisparityData failed res = %ld",
                  arcResult);
            result = MIA_RETURN_INVALID_PARAM;
            break;
        } else {
            MLOGI(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCIR_CalcDisparityData res = %ld", arcResult);
        }

        MInt32 lSize = 0;
        // get the size of disparity map
        arcResult = ARC_DCIR_GetDisparityDataSize(mCaptureBokeh, &lSize);
        if (arcResult) {
            MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCIR_GetDisparityDataSize failed res = %ld",
                  arcResult);
            result = MIA_RETURN_INVALID_PARAM;
            break;
        }

        mInt32DisparityDataActiveSize = lSize;

        if (stOutDepthImage.ppu8Plane[0] != NULL && mOutputNum == 3) {
            disparity_fileheader_t header;
            memset(&header, 0, sizeof(disparity_fileheader_t));

            header.i32Header = 0x80;
            header.i32HeaderLength = sizeof(disparity_fileheader_t);
            header.i32PointX = mFrameParams.refocus.ptFocus.x;
            header.i32PointY = mFrameParams.refocus.ptFocus.y;
            header.i32BlurLevel = mBlurLevel;
            header.i32Version = (int32_t)3;
            header.i32VendorID = (int32_t)1;
            header.i32DataLength = mInt32DisparityDataActiveSize;
            header.mappingVersion = mMappingVersion;
            header.ShineThres = FILTER_SHINE_THRES_REFOCUS;
            header.ShineLevel = FILTER_SHINE_LEVEL_REFOCUS;
            memcpy(stOutDepthImage.ppu8Plane[0], &header, sizeof(disparity_fileheader_t));

            MInt32 plane_size = stOutDepthImage.i32Height * stOutDepthImage.pi32Pitch[0] -
                                sizeof(disparity_fileheader_t);
            if (mInt32DisparityDataActiveSize > plane_size) {
                MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] DisparityDataActiveSize = %d plane_size %d",
                      mInt32DisparityDataActiveSize, plane_size);
                result = MIA_RETURN_INVALID_PARAM;
                break;
            }
        }

        if (mSDKMode) {
            pDisparityData = (unsigned char *)malloc(lSize);
        } else {
            pDisparityData = stOutDepthImage.ppu8Plane[0] + sizeof(disparity_fileheader_t);
        }

        MLOGD(Mia2LogGroupPlugin,
              " [ARC_CTB] copy disparity data to output stream, Disparity:%p size %d "
              "i32Height %d pi32Pitch %d",
              pDisparityData, mInt32DisparityDataActiveSize, stOutDepthImage.i32Height,
              stOutDepthImage.pi32Pitch[0]);
        // get the data of disparity map
        arcResult = ARC_DCIR_GetDisparityData(mCaptureBokeh, pDisparityData);
        if (arcResult) {
            MLOGE(Mia2LogGroupPlugin,
                  " [ARC_CTB] doProcess ARC_DCIR_GetDisparityData failed res = %ld", arcResult);
            result = MIA_RETURN_INVALID_PARAM;
            break;
        }

        if (mDumpPriv) {
            dumpRawData(pDisparityData, mInt32DisparityDataActiveSize, 5, "OUT_disparity_");
        }

        ARC_DCIR_SetNoiseOnOff(mCaptureBokeh, false);
        ARC_DCIR_SetFullWeakBlur(mCaptureBokeh, 0);
        ARC_DCIR_SetShineParam(mCaptureBokeh, FILTER_SHINE_THRES, FILTER_SHINE_THRES);
    } while (false);

    ASVLOFFSCREEN *pInSrcImage = &stInMainImage;
    ASVLOFFSCREEN *pOutEffectImage = &stOutEffectImage;
    MRECT cropRect = {0}; /// HdrBokeh CropRect
    if (arcResult == MOK && mHdrEnabled) {
        allocImage(&tmpHdrImage, &stInMainImage);
        allocImage(&HdrcropImage, &stInMainImage);
        arcResult = CreateHDRImage(teleInputvector, &tmpHdrImage, &cropRect, pDisparityData);
        if (arcResult == MOK) {
            pInSrcImage = &tmpHdrImage;
            pOutEffectImage = &HdrcropImage;
        }
    }

    if (mBeautyEnabled > 0) {
        bokeh_facebeauty_params beautyParams;
        beautyParams.pInputImage = pInSrcImage;
        beautyParams.pOutputImage = pInSrcImage;
        beautyParams.gender = (mFaceAnalyze.gender)[0];
        beautyParams.orient = mOrient;
        beautyParams.rearCamera = mRearCamera;
        memcpy(&beautyParams.beautyFeatureParams.beautyFeatureInfo[0],
               &mBeautyFeatureParams.beautyFeatureInfo[0],
               sizeof(mBeautyFeatureParams.beautyFeatureInfo));
        bokeh_facebeauty_thread_proc(&beautyParams);
    }

    if (mOutputNum == 3) {
        if (arcResult == MOK && mHdrEnabled) {
            if (ARC_DCIR_CropResizeImg(mCaptureBokeh, pInSrcImage, &cropRect, &stOutSrcImage)) {
                copyImage(&stInMainImage, &stOutSrcImage);
            }
        } else {
            copyImage(pInSrcImage, &stOutSrcImage);
        }
    }

    if (arcResult == MOK) {
        t3 = PluginUtils::nowMSec();
        result = ARC_DCIR_Process(mCaptureBokeh, pDisparityData, mInt32DisparityDataActiveSize,
                                  pInSrcImage, &mFrameParams.refocus, pOutEffectImage);
        t4 = PluginUtils::nowMSec();
        MLOGW(Mia2LogGroupPlugin, " [ARC_CTB] ARC_DCIR_Process result = %d", result);

        if (mHdrEnabled) {
            pInSrcImage = pOutEffectImage; // hdrcrop
            pOutEffectImage = &stOutEffectImage;
            arcResult =
                ARC_DCIR_CropResizeImg(mCaptureBokeh, pInSrcImage, &cropRect, pOutEffectImage);
        }
    }

    if (mSDKMode && pDisparityData != NULL) {
        free(pDisparityData);
    }

    if (arcResult) {
        copyImage(&stInMainImage, pOutEffectImage); // relight need continuous memory for plane
    }
    pOutEffectImage = NULL;
    pInSrcImage = NULL;
    releaseImage(&tmpHdrImage);
    releaseImage(&HdrcropImage);

    MLOGI(Mia2LogGroupPlugin,
          " [ARC_CTB] ARC_DCIR_CalcDisparityData %.2f, ARC_DCIR_Process %.2f, total %.2f", t2 - t1,
          t4 - t3, PluginUtils::nowMSec() - t0);
    return PROCSUCCESS;
}

void ArcCapBokehPlugin::allocImage(ASVLOFFSCREEN *dstImage, ASVLOFFSCREEN *srcImage)
{
    memcpy(dstImage, srcImage, sizeof(ASVLOFFSCREEN));
    dstImage->ppu8Plane[0] =
        (unsigned char *)malloc(dstImage->i32Height * dstImage->pi32Pitch[0] * 3 / 2);
    if (dstImage->ppu8Plane[0]) {
        dstImage->ppu8Plane[1] =
            dstImage->ppu8Plane[0] + dstImage->i32Height * dstImage->pi32Pitch[0];
    } else {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] malloc fail, may be out of memory");
    }
}

void ArcCapBokehPlugin::releaseImage(ASVLOFFSCREEN *image)
{
    if (image->ppu8Plane[0])
        free(image->ppu8Plane[0]);
    image->ppu8Plane[0] = NULL;
}

void ArcCapBokehPlugin::checkPersist()
{
    mDumpYUV = (bool)sArchsoftCbDumpimg;
    mDumpPriv = (bool)sArchsoftCbDumpimg;

    // m_nDebugView:
    //    0:algo
    //    1:only show wide
    //    2:only show tele
    //    3:show half wide and half tele
    mViewCtrl = sArchsoftCbDebugView;
    mDrawROI = sArchsoftCbDrawroi;
}

void ArcCapBokehPlugin::dumpYUVData(ASVLOFFSCREEN *aslIn, uint32_t index, const char *namePrefix)
{
    if (NULL == aslIn) {
        MLOGW(Mia2LogGroupPlugin, " [ARC_CTB] aslIn is NULL");
        return;
    }

    char timeBuf[32];
    time_t currentTime;
    struct tm *timeInfo;
    time(&currentTime);
    timeInfo = localtime(&currentTime);
    strftime(timeBuf, sizeof(timeBuf), "%Y%m%d%H%M%S", timeInfo);

    char fileName[256];
    memset(fileName, 0, sizeof(char) * 256);
    snprintf(fileName, sizeof(fileName), "%sIMG_%s_%s_%d_%dx%d.yuv", PluginUtils::getDumpPath(),
             timeBuf, namePrefix, index, aslIn->pi32Pitch[0], aslIn->i32Height);

    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] try to open file : %s", fileName);

    int fileFd = open(fileName, O_RDWR | O_CREAT, 0777);
    if (fileFd) {
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] dumpYUVData open success, ppu8Plane[0, 1]: (%p, %p)",
              aslIn->ppu8Plane[0], aslIn->ppu8Plane[1]);
        uint32_t bytes_write;
        bytes_write = write(fileFd, aslIn->ppu8Plane[0], aslIn->pi32Pitch[0] * aslIn->i32Height);
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] plane[0] bytes_write: %u", bytes_write);
        bytes_write =
            write(fileFd, aslIn->ppu8Plane[1], aslIn->pi32Pitch[0] * (aslIn->i32Height >> 1));
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] plane[1] bytes_write: %u", bytes_write);
        close(fileFd);
    } else {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] fileopen  error: %d", errno);
    }
}

void ArcCapBokehPlugin::DumpInputData(ImageParams inputimage, const char *namePrefix, int Index)
{
    int expValue = 0;
    void *pMetaData = NULL;
    int ret = -1;
    camera_metadata_t *pMeta = inputimage.pMetadata;
    ret = VendorMetadataParser::getTagValue(pMeta, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                            &pMetaData);
    if (PROCSUCCESS == ret && NULL != pMetaData) {
        expValue = *static_cast<int32_t *>(pMetaData);
    } else {
        expValue = 0;
    }

    struct MiImageBuffer inputFrame;
    inputFrame.format = inputimage.format.format;
    inputFrame.width = inputimage.format.width;
    inputFrame.height = inputimage.format.height;
    inputFrame.plane[0] = inputimage.pAddr[0];
    inputFrame.plane[1] = inputimage.pAddr[1];
    inputFrame.stride = inputimage.planeStride;
    inputFrame.scanline = inputimage.sliceheight;
    inputFrame.numberOfPlanes = 2;

    char inputtelebuf[128];
    snprintf(inputtelebuf, sizeof(inputtelebuf), "%s_%dx%d_EV%d_%d", namePrefix, inputFrame.width,
             inputFrame.height, expValue, Index);
    PluginUtils::dumpToFile(inputtelebuf, &inputFrame);
}

void ArcCapBokehPlugin::PrepareMiImageBuffer(struct MiImageBuffer *dstImage, ImageParams srcImage)
{
    dstImage->format = srcImage.format.format;
    dstImage->width = srcImage.format.width;
    dstImage->height = srcImage.format.height;
    dstImage->plane[0] = srcImage.pAddr[0];
    dstImage->plane[1] = srcImage.pAddr[1];
    dstImage->stride = srcImage.planeStride;
    dstImage->scanline = srcImage.sliceheight;
    dstImage->numberOfPlanes = 2;

    MLOGD(Mia2LogGroupPlugin,
          " [ARC_RTB] PrepareMiImageBuffer width: %d, height: %d, planeStride: %d, sliceHeight:%d, "
          "plans: %d %lu %lu",
          dstImage->width, dstImage->height, dstImage->stride, dstImage->scanline,
          dstImage->numberOfPlanes, dstImage->plane[0], dstImage->plane[1]);
}

void ArcCapBokehPlugin::copyImage(ASVLOFFSCREEN *srcImg, ASVLOFFSCREEN *dstImg)
{
    if (srcImg == NULL || dstImg == NULL)
        return;

    int32_t i;
    uint8_t *tempSrc = NULL;
    uint8_t *tempDst = NULL;

    tempSrc = srcImg->ppu8Plane[0];
    tempDst = dstImg->ppu8Plane[0];
    for (i = 0; i < dstImg->i32Height; i++) {
        memcpy(tempDst, tempSrc, dstImg->i32Width);
        tempDst += dstImg->pi32Pitch[0];
        tempSrc += srcImg->pi32Pitch[0];
    }

    tempSrc = srcImg->ppu8Plane[1];
    tempDst = dstImg->ppu8Plane[1];
    for (i = 0; i < dstImg->i32Height / 2; i++) {
        memcpy(tempDst, tempSrc, dstImg->i32Width);
        tempDst += dstImg->pi32Pitch[1];
        tempSrc += srcImg->pi32Pitch[1];
    }
}

void ArcCapBokehPlugin::mergeImage(ASVLOFFSCREEN *baseImg, ASVLOFFSCREEN *overImg, int downSize)
{
    if (baseImg == NULL || overImg == NULL || downSize <= 0)
        return;

    int32_t i, j, index;
    uint8_t *tempBase = NULL;
    uint8_t *tempOver = NULL;

    tempBase = baseImg->ppu8Plane[0];
    tempOver = overImg->ppu8Plane[0];
    for (i = 0; i < overImg->i32Height;) {
        for (j = 0, index = 0; j < overImg->i32Width; index++, j += downSize) {
            *(tempBase + index) = *(tempOver + j);
        }
        tempBase += baseImg->pi32Pitch[0];
        tempOver += overImg->pi32Pitch[0] * downSize;
        i += downSize;
    }

    tempBase = baseImg->ppu8Plane[1];
    tempOver = overImg->ppu8Plane[1];
    for (i = 0; i < overImg->i32Height / 2;) {
        for (j = 0, index = 0; j < overImg->i32Width / 2; index++, j += downSize) {
            *(tempBase + 2 * index) = *(tempOver + 2 * j);
            *(tempBase + 2 * index + 1) = *(tempOver + 2 * j + 1);
        }

        tempBase += baseImg->pi32Pitch[1];
        tempOver += overImg->pi32Pitch[1] * downSize;
        i += downSize;
    }
}

void ArcCapBokehPlugin::dumpRawData(uint8_t *data, int32_t size, uint32_t index,
                                    const char *namePrefix)
{
    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] (IN)");

    long long currentTime = gettime();
    char fileName[256];
    memset(fileName, 0, sizeof(char) * 256);
    snprintf(fileName, sizeof(fileName), "%s%llu_%s_%d_%d.pdata", PluginUtils::getDumpPath(),
             currentTime, namePrefix, size, index);

    ARC_LOGD(" [ARC_CTB] try to open file : %s", fileName);

    int fileFd = open(fileName, O_RDWR | O_CREAT, 0777);
    if (fileFd) {
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] : dumpYUVData open success");
        uint32_t bytesWrite;
        bytesWrite = write(fileFd, data, size);
        MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] data bytes_read: %d", size);
        close(fileFd);
    } else {
        MLOGE(Mia2LogGroupPlugin, " [ARC_CTB] fileopen  error: %d", errno);
    }

    MLOGD(Mia2LogGroupPlugin, " [ARC_CTB] (OUT)");
}

void ArcCapBokehPlugin::getShineParam(MInt32 i32ISOValue, MInt32 &i32ShineLevel,
                                      MInt32 &i32ShineThres)
{
    if (i32ISOValue <= 50) {
        i32ShineLevel = FILTER_SHINE_LEVEL_OUTSIDE;
        i32ShineThres = FILTER_SHINE_THRES_OUTSIDE;
    } else {
        i32ShineLevel = FILTER_SHINE_LEVEL;
        i32ShineThres = FILTER_SHINE_THRES;
    }
}

void ArcCapBokehPlugin::writeFileLog(char *file, const char *fmt, ...)
{
    FILE *fd = fopen(file, "ab+");
    if (fd) {
        va_list ap;
        char buf[256] = {0};
        va_start(ap, fmt);
        vsnprintf(buf, 256, fmt, ap);
        va_end(ap);
        fwrite(buf, strlen(buf) * sizeof(char), sizeof(char), fd);
        fclose(fd);
    }
}
