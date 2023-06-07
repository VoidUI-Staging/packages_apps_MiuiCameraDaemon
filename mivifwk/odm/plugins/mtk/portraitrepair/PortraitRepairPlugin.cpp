#include "PortraitRepairPlugin.h"

#include <condition_variable>
#include <mutex>

#include "MiaPluginUtils.h"
#include "MiaPostProcType.h"

#undef LOG_TAG
#define LOG_TAG "PortraitRepair"

#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)

#define Model_PATH        \
    "/vendor/etc/camera/" \
    "facesr_hd_composite_ref_4.2.9_mtkg610_apu_block_apu5.0_qualityv35r2_res736_800.model"
#define Rear_Classic_JSON_PATH  "/vendor/etc/camera/portraitrepaire_rear_class.json"
#define Rear_Vivid_JSON_PATH    "/vendor/etc/camera/portraitrepaire_rear_live.json"
#define Front_Classic_JSON_PATH "/vendor/etc/camera/portraitrepaire_front_class.json"
#define Front_Vivid_JSON_PATH   "/vendor/etc/camera/portraitrepaire_front_live.json"
#define IMG_DUMP_PATH           "/data/vendor/camera/"

static std::mutex g_algomutex;
static std::mutex g_destroymutex;
static std::condition_variable g_condProcessWait;
static std::condition_variable g_condDestroyWait;

enum {
    ALGO_IDLE = 0,
    ALGO_INIT_START,
    ALGO_INIT_FAIL,
    ALGO_INIT_DONE,
    ALGO_PROCESS,
    ALGO_CREATE_THREAD_FAIL,
    ALGO_PROCESS_DONE,
    ALGO_PROCESS_TIMEOUT,
};
static int g_algostatus = ALGO_IDLE;
static stm_handle_t g_algohandle = nullptr;
static int g_algoRefcount = 0;

namespace mialgo2 {

typedef struct
{
    int orient;
    int iso;
    int faceNum;
    stm_rect_t *faceInfo;
    FACESR_EXIF_INFO faceexifinfo;
    yuv_image_t stm_input;
    stm_result_t PortraitRepairResult;
} process_thread_params;

PortraitRepair::~PortraitRepair() {}

void *AlgoInitThread(void *data)
{
    PortraitRepair *algoplugin = reinterpret_cast<PortraitRepair *>(data);
    stm_model_t models = {Model_PATH, MODEL_TYPE_MULT};
    g_algomutex.lock();
    g_algohandle = nullptr;
    g_algomutex.unlock();
    double startTime = PluginUtils::nowMSec();
    algoplugin->mPortraitRepairResult = stm_portrait_repair_create(&models, 1, &g_algohandle);
    g_algomutex.lock();
    if (algoplugin->mPortraitRepairResult == STM_OK) {
        g_algostatus = ALGO_INIT_DONE;
    } else {
        g_algostatus = ALGO_INIT_FAIL;
        g_algoRefcount--;
        MLOGE(Mia2LogGroupPlugin, "PortraitRepair init result %d  init fail",
              algoplugin->mPortraitRepairResult);
    }
    g_algomutex.unlock();

    double endTime = PluginUtils::nowMSec();
    MLOGI(Mia2LogGroupPlugin,
          "PortraitRepair g_algostatus %d  init time %.2f ms , count %d , handle %p", g_algostatus,
          endTime - startTime, g_algoRefcount, g_algohandle);

    return NULL;
}

void *AlgoProcessThread(void *data)
{
    process_thread_params *processparam = (process_thread_params *)data;
    double startTime = PluginUtils::nowMSec();
    processparam->PortraitRepairResult = stm_portrait_repair_process_with_yuv_ext(
        g_algohandle, &(processparam->stm_input), processparam->faceInfo, processparam->faceNum,
        processparam->iso, processparam->orient, &(processparam->faceexifinfo));
    double endTime = PluginUtils::nowMSec();
    MLOGI(Mia2LogGroupPlugin,
          "PortraitRepairResult  %d process time %.2f ms, handle %p g_algostatus %d",
          processparam->PortraitRepairResult, endTime - startTime, g_algohandle, g_algostatus);

    if ((processparam->faceexifinfo.faces > 0) || (processparam->PortraitRepairResult == 0)) {
        g_condProcessWait.notify_one();
    }
    return NULL;
}

void *AlgoDestroyThread(void *data)
{
    std::unique_lock<std::mutex> lockWait(g_destroymutex);
    std::cv_status cvsts = g_condDestroyWait.wait_for(lockWait, std::chrono::seconds(1));
    if (cvsts == std::cv_status::timeout) {
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair begain to destroy count %d, g_algostatus %d",
              g_algoRefcount, g_algostatus);
        g_algomutex.lock();
        if (g_algostatus != ALGO_PROCESS) {
            double startTime = PluginUtils::nowMSec();
            stm_portrait_repair_destroy(g_algohandle);
            g_algohandle = nullptr;
            g_algostatus = ALGO_IDLE;
            double endTime = PluginUtils::nowMSec();
            MLOGI(Mia2LogGroupPlugin, "stm_portrait_repair_destroy, count %d destroy time %.2f ms",
                  g_algoRefcount, endTime - startTime);
        } else {
            g_algoRefcount++;
            MLOGI(Mia2LogGroupPlugin,
                  "PortraitRepair algo is process not destroy count %d, g_algostatus %d",
                  g_algoRefcount, g_algostatus);
        }
        g_algomutex.unlock();
    } else {
        g_algomutex.lock();
        g_algoRefcount++;
        g_algomutex.unlock();
        MLOGI(Mia2LogGroupPlugin,
              "PortraitRepair zoom or switch mode not destroy count %d, g_algostatus %d",
              g_algoRefcount, g_algostatus);
    }

    return NULL;
}

int PortraitRepair::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mFrameworkCameraId = createInfo->frameworkCameraId;
    MLOGD(Mia2LogGroupPlugin, "%s:%d call Initialize... %p, mFrameworkCameraId: %d", __func__,
          __LINE__, this, mFrameworkCameraId);
    mNodeInterface = nodeInterface;
    mPortraitRepairResult = STM_OK;
    mEnableStatus = false;
    mOperationMode = createInfo->operationMode;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.portraitrepair.enable", prop, "1");
    mEnableStatus = (int32_t)atoi(prop);
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.portraitrepair.algo.enable", prop, "0");
    mQualityDebug = (int32_t)atoi(prop);
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.portraitrepair.drawface", prop, "0");
    mIsDrawFaceRect = (int32_t)atoi(prop);

    const char *version = stm_portrait_repair_version();
    MLOGI(Mia2LogGroupPlugin,
          "PortraitRepair g_algostatus %d mEnableStatus %d mOperationMode 0x%x PortraitRepair "
          "version %s",
          g_algostatus, mEnableStatus, mOperationMode, version);

    return MIA_RETURN_OK;
}

void PortraitRepair::destroy()
{
    pthread_join(m_thread, NULL);
    MLOGI(Mia2LogGroupPlugin, "this %p PortraitRepair g_algostatus %d handle %p count %d", this,
          g_algostatus, g_algohandle, g_algoRefcount);
    g_algomutex.lock();
    if ((g_algostatus > ALGO_INIT_FAIL) && (g_algoRefcount > 0)) {
        g_algoRefcount--;
        g_algomutex.unlock();
        pthread_t m_destroy_thread = 0;
        if (pthread_create(&m_destroy_thread, NULL, AlgoDestroyThread, this) != 0) {
            MLOGW(Mia2LogGroupPlugin, "pthread_create failed ");
            g_algomutex.lock();
            g_algoRefcount++;
            g_algomutex.unlock();
        }
    } else {
        g_algomutex.unlock();
        MLOGI(Mia2LogGroupPlugin, "this %p PortraitRepair ", this);
    }
}
bool PortraitRepair::isEnabled(MiaParams settings)
{
    bool isEnable = false;
    void *data = NULL;
    mPortraitEnable = false;
    VendorMetadataParser::getTagValue(settings.metadata, XIAOMI_PORTRAITREPAIR_ENABLED, &data);
    if (NULL != data) {
        mPortraitEnable = *static_cast<bool *>(data);
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair getMetaData mPortraitEnable  %d",
              mPortraitEnable);
    } else {
        MLOGW(Mia2LogGroupPlugin, "PortraitRepair can not found tag xiaomi.portraitrepair.enabled");
    }

    data = NULL;
    bool hasFace = false;
    VendorMetadataParser::getTagValue(settings.metadata, ANDROID_STATISTICS_FACE_RECTANGLES, &data);
    if (NULL != data) {
        hasFace = true;
    } else {
        MLOGW(Mia2LogGroupPlugin, "PortraitRepair tag android.statistics.faceRectangles is null");
    }
    /*
    data = NULL;
    const char *faceRect = "xiaomi.snapshot.faceRect";
    VendorMetadataParser::getVTagValue(settings.metadata, faceRect, &data);
    if (NULL != data) {
        hasFace = true;
    } else {
        MLOGW(Mia2LogGroupPlugin, "PortraitRepair can not found face");
    }*/

    if (mEnableStatus && mPortraitEnable && hasFace) {
        isEnable = true;
        g_condDestroyWait.notify_one();
        // init
        preInitProcess();
    }

    g_algomutex.lock();
    if (ALGO_PROCESS_DONE == g_algostatus && mPortraitEnable == false && (g_algoRefcount > 0) &&
        (mOperationMode == StreamConfigModeBokeh || mOperationMode == StreamConfigModeFrontBokeh)) {
        g_algoRefcount--;
        g_algomutex.unlock();
        stm_portrait_repair_destroy(g_algohandle);
        g_algomutex.lock();
        g_algohandle = nullptr;
        g_algostatus = ALGO_IDLE;
        g_algomutex.unlock();
        MLOGI(Mia2LogGroupPlugin, "this %p PortraitRepair stm_portrait_repair_destroy count %d",
              this, g_algoRefcount);
    } else {
        g_algomutex.unlock();
    }

    MLOGI(Mia2LogGroupPlugin,
          "PortraitRepair mPortraitEnable %d enable: %d, g_algostatus %d handle %p",
          mPortraitEnable, isEnable, g_algostatus, g_algohandle);
    return isEnable;
}

int PortraitRepair::preInitProcess()
{
    MLOGI(Mia2LogGroupPlugin, "PortraitRepair preProcess ");
    g_algomutex.lock();
    if (mEnableStatus && (g_algoRefcount == 0) &&
        (g_algostatus == ALGO_IDLE || g_algostatus == ALGO_INIT_FAIL || g_algohandle == nullptr)) {
        g_algostatus = ALGO_INIT_START;
        g_algoRefcount++;
        g_algomutex.unlock();
        // cpu boost 3000 ms
        std::shared_ptr<PluginPerfLockManager> boostCpu(new PluginPerfLockManager(3000));
        if (pthread_create(&m_thread, NULL, AlgoInitThread, this) != 0) {
            MLOGW(Mia2LogGroupPlugin, "pthread_create failed ");
            g_algomutex.lock();
            g_algoRefcount--;
            g_algomutex.unlock();
            return MIAS_OK;
        }
    } else {
        g_algomutex.unlock();
    }
    return MIAS_OK;
}

ProcessRetStatus PortraitRepair::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.portraitrepair.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);
    auto &inputBuffers = pProcessRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = pProcessRequestInfo->outputBuffersMap.begin()->second;
    camera_metadata_t *metadata = inputBuffers[0].pMetadata;
    camera_metadata_t *outmetadata = outputBuffers[0].pMetadata;
    camera_metadata_t *physicalMeta = inputBuffers[0].pPhyCamMetadata;
    MiImageBuffer inputFrame, outputFrame;
    convertImageParams(inputBuffers[0], inputFrame);
    convertImageParams(outputBuffers[0], outputFrame);
    // cpu boost 3000 ms
    std::shared_ptr<PluginPerfLockManager> boostCpu(new PluginPerfLockManager(3000));

    mWidth = inputFrame.width;
    mHeight = inputFrame.height;
    stm_rect_t faceInfo[10] = {};
    ChiRectEXT afRect;
    memset(&afRect, 0, sizeof(ChiRectEXT));
    ChiRect cropRegion;
    memset(&cropRegion, 0, sizeof(ChiRect));
    int faceNum = 0;
    int processFaceNum = 0;
    getFaceInfo(faceInfo, &cropRegion, metadata, physicalMeta, &faceNum, &afRect);
    bool isProcessface = isProcessFace(faceInfo, faceNum, &processFaceNum, afRect);

    void *data = NULL;
    int rotate = 0;
    VendorMetadataParser::getTagValue(metadata, ANDROID_JPEG_ORIENTATION, &data);
    if (NULL != data) {
        int orientation = *static_cast<int32_t *>(data);
        rotate = 360 - (orientation) % 360;
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair jpeg orientation %d rotate %d", orientation,
              rotate);
    }

    int sensitivity = -1;
    data = NULL;
    VendorMetadataParser::getTagValue(metadata, ANDROID_SENSOR_SENSITIVITY, &data);
    if (NULL != data) {
        sensitivity = *static_cast<int32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair get sensitivity %d", sensitivity);
    } else {
        sensitivity = 100;
    }
    float ispGain = 1.0;
    data = NULL;
    VendorMetadataParser::getVTagValue((camera_metadata_t *)metadata, "com.xiaomi.sensorbps.gain",
                                       &data);
    if (NULL != data) {
        ispGain = *static_cast<float *>(data);
        MLOGD(Mia2LogGroupPlugin, "PortraitRepair get ispGain %f", ispGain);
    } else {
        MLOGW(Mia2LogGroupPlugin, "PortraitRepair get ispGain failed, set it to 1.0");
    }
    int iso = (int)(ispGain * 100) * sensitivity / 100;
    int32_t perFrameEv = 0;
    data = NULL;
    VendorMetadataParser::getTagValue(metadata, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &data);
    if (NULL != data) {
        perFrameEv = *static_cast<int32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair get Ev %d", perFrameEv);
    }

    uint8_t stylizationType = 0; // 0: CIVI Vivid;  1: CIVI Classic;
    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, "com.xiaomi.sessionparams.stylizationType", &data);
    if (NULL != data) {
        stylizationType = *static_cast<uint8_t *>(data);
        MLOGD(Mia2LogGroupPlugin, "PortraitRepair get stylizationType %d", stylizationType);
    }

    data = NULL;
    int32_t sceneDetected = 0;
    VendorMetadataParser::getVTagValue(metadata, "xiaomi.ai.asd.sceneDetected", &data);
    if (NULL != data) {
        sceneDetected = *static_cast<int32_t *>(data);
        MLOGD(Mia2LogGroupPlugin, "PortraitRepair get sceneDetected mode: %d ", sceneDetected);
    } else {
        MLOGW(Mia2LogGroupPlugin, "PortraitRepair can not found tag xiaomi.ai.asd.sceneDetected");
    }

    float luxIndex = 0.0f;
    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, "com.xiaomi.statsconfigs.AecInfo", &data);
    if (NULL != data) {
        luxIndex = static_cast<InputMetadataAecInfo *>(data)->luxIndex;
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair get luxindex %f", luxIndex);
    } else {
        MLOGW(Mia2LogGroupPlugin, "PortraitRepair get luxIndex failed");
    }

    float zoom = 0.0f;
    data = NULL;
    VendorMetadataParser::getTagValue(metadata, ANDROID_CONTROL_ZOOM_RATIO, &data);
    if (NULL != data) {
        zoom = *static_cast<float *>(data);
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair get zoom =%f", zoom);
    } else {
        MLOGW(Mia2LogGroupPlugin, "PortraitRepair get android zoom ratio failed");
    }

    if (mIsDrawFaceRect) {
        drawFaceRect(&inputFrame, faceInfo, faceNum, afRect);
    }

    if (iIsDumpData) {
        char inputbuf[128];
        snprintf(
            inputbuf, sizeof(inputbuf),
            "PortraitRepair_input_%dx%d_iso_%d_s_%d_g_%.3f_ev_%d_rotate_%d_zoom_%.3f_luxIndex_%.3f",
            inputFrame.width, inputFrame.height, iso, sensitivity, ispGain, perFrameEv, rotate,
            zoom, luxIndex);
        DumpYuvBuffer(inputbuf, &inputFrame);
    }

    yuv_image_t stm_input, stm_output;
    if (inputFrame.format == CAM_FORMAT_YUV_420_NV12) {
        stm_input.format = YUV_IMAGE_FMT_NV12;
    } else if (inputFrame.format == CAM_FORMAT_YUV_420_NV21) {
        stm_input.format = YUV_IMAGE_FMT_NV21;
    } else {
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair not support the format %d ", inputFrame.format);
    }
    stm_input.width = inputFrame.stride;
    stm_input.height = inputFrame.scanline;
    stm_input.strides[0] = inputFrame.stride;
    stm_input.strides[1] = inputFrame.stride;
    stm_input.pY = inputFrame.plane[0];
    stm_input.pUV = inputFrame.plane[1];

    if (outputFrame.format == CAM_FORMAT_YUV_420_NV12) {
        stm_output.format = YUV_IMAGE_FMT_NV12;
    } else if (outputFrame.format == CAM_FORMAT_YUV_420_NV21) {
        stm_output.format = YUV_IMAGE_FMT_NV21;
    } else {
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair not support the format %d ", outputFrame.format);
    }
    stm_output.width = outputFrame.stride;
    stm_output.height = outputFrame.scanline;
    stm_output.strides[0] = outputFrame.stride;
    stm_output.strides[1] = outputFrame.stride;
    stm_output.pY = outputFrame.plane[0];
    stm_output.pUV = outputFrame.plane[1];

    MLOGD(Mia2LogGroupPlugin,
          "PortraitRepair %s:%d >>> input width: %d, height: %d, data: %p, stride %d", __func__,
          __LINE__, stm_input.width, stm_input.height, stm_input.pY, inputFrame.stride);

    bool isProcessEnable = false;
    if ((pProcessRequestInfo->runningTaskNum < pProcessRequestInfo->maxConcurrentNum - 2) &&
        isProcessface && (processFaceNum > 0) && (sceneDetected != 1) && (zoom < 10)) {
        isProcessEnable = true;
    }

    if (((1.0 <= zoom) && (zoom <= 1.3) && (luxIndex <= 200) && (iso <= 100) &&
         (mOperationMode == StreamConfigModeSAT)) ||
        ((iso < 80) && (mOperationMode == StreamConfigModeMiuiZslFront))) {
        isProcessEnable = false;
    }

    MLOGI(Mia2LogGroupPlugin,
          "PortraitRepair runningTaskNum %d, g_algostatus %d, isProcessEnable %d",
          pProcessRequestInfo->runningTaskNum, g_algostatus, isProcessEnable);

    process_thread_params processParam;
    memset(&processParam, 0, sizeof(processParam));
    pthread_t m_process_thread = 0;

    double startTime = PluginUtils::nowMSec();
    g_algomutex.lock();
    if (isProcessEnable && (g_algostatus > ALGO_INIT_FAIL) && (g_algostatus != ALGO_PROCESS)) {
        g_algostatus = ALGO_PROCESS;
        g_algomutex.unlock();
        mPortraitEnable = true;
        processParam.orient = rotate;
        processParam.iso = iso;
        processParam.faceNum = processFaceNum;
        processParam.faceInfo = faceInfo;
        processParam.stm_input = stm_input;
        loadConfigParams(stylizationType);
        if (pthread_create(&m_process_thread, NULL, AlgoProcessThread, &processParam) != 0) {
            g_algomutex.lock();
            g_algostatus = ALGO_CREATE_THREAD_FAIL;
            g_algomutex.unlock();
            MLOGW(Mia2LogGroupPlugin, "pthread_create failed ");
        } else {
            g_algomutex.lock();
            if (g_algostatus != ALGO_PROCESS_DONE) {
                g_algomutex.unlock();
                std::unique_lock<std::mutex> lockWait(g_algomutex);
                std::cv_status cvsts =
                    g_condProcessWait.wait_for(lockWait, std::chrono::seconds(5));
                if (cvsts == std::cv_status::timeout) {
                    MLOGE(Mia2LogGroupPlugin, "PortraitRepair: process time more than 5s");
                    double t1 = PluginUtils::nowMSec();
                    stm_portrait_repair_cancel(g_algohandle);
                    pthread_join(m_process_thread, NULL);
                    g_algostatus = ALGO_PROCESS_TIMEOUT;
                    double t2 = PluginUtils::nowMSec();
                    mPortraitEnable = false;
                    MLOGI(Mia2LogGroupPlugin, "stm_portrait_repair_cancel cost time %.2f ms",
                          t2 - t1);
                } else {
                    g_algostatus = ALGO_PROCESS_DONE;
                    double t3 = PluginUtils::nowMSec();
                    MLOGI(Mia2LogGroupPlugin,
                          "PortraitRepairResult: %d, g_algostatus %d process time %.2f ms",
                          processParam.PortraitRepairResult, g_algostatus, t3 - startTime);
                }
            } else {
                g_algomutex.unlock();
                pthread_join(m_process_thread, NULL);
                mPortraitEnable = false;
            }
        }

        if ((processParam.PortraitRepairResult == STM_OK) ||
            (g_algostatus != ALGO_CREATE_THREAD_FAIL)) {
            CopyYuvBuffer(&stm_output, &(processParam.stm_input));
        } else {
            PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
            mPortraitEnable = false;
            MLOGW(Mia2LogGroupPlugin, "process fail PortraitRepairResult %d  do memcpy",
                  processParam.PortraitRepairResult);
        }
    } else {
        g_algomutex.unlock();
        PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        mPortraitEnable = false;
        MLOGD(Mia2LogGroupPlugin, "PortraitRepairResult %d memcpy",
              processParam.PortraitRepairResult);
    }

    double end = PluginUtils::nowMSec();
    MLOGD(Mia2LogGroupPlugin, "PortraitRepairResult %d  mPortraitEnable %d process time  %.2f ms",
          processParam.PortraitRepairResult, mPortraitEnable, end - startTime);
    if (mNodeInterface.pSetResultMetadata != NULL) {
        std::stringstream exifmsg;
        exifmsg << "PortraitRepair:" << mPortraitEnable
                << " {result:" << processParam.PortraitRepairResult
                << " faces:" << processParam.faceexifinfo.faces;
        exifmsg << " afRext:" << afRect.left << "_" << afRect.top << "_" << afRect.right << "_"
                << afRect.bottom << " isoverlap:" << isProcessface;
        exifmsg << " iso:" << iso << " rotate:" << rotate << " luxIndex:" << luxIndex
                << " scene:" << sceneDetected << " time:" << end - startTime;
        for (int i = 0; i < processParam.faceexifinfo.faces; i++) {
            exifmsg << " {blur:" << processParam.faceexifinfo.blur[i]
                    << " HDcrop:" << processParam.faceexifinfo.isHDCrop[i];
            exifmsg << " rects:" << processParam.faceexifinfo.rects[i][0] << ","
                    << processParam.faceexifinfo.rects[i][1];
            exifmsg << "," << processParam.faceexifinfo.rects[i][2] << ","
                    << processParam.faceexifinfo.rects[i][3] << "}";
        }
        exifmsg << "}";
        std::string results = exifmsg.str();
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, pProcessRequestInfo->frameNum,
                                          pProcessRequestInfo->timeStamp, results);
    }
    int set_tag_result = VendorMetadataParser::updateTagValue(
        metadata, XIAOMI_PORTRAITREPAIR_ENABLED, &mPortraitEnable, 1, &outmetadata);
    MLOGD(Mia2LogGroupPlugin, "mPortraitEnable %d set_tag_result %d", mPortraitEnable,
          set_tag_result);

    if (iIsDumpData) {
        char outputbuf[128];
        snprintf(outputbuf, sizeof(outputbuf),
                 "PortraitRepair_output_%dx%d_iso_%d_s_%d_g_%.3f_ev_%d_rotate_%d_zoom_%.3f_"
                 "luxIndex_%.3f",
                 outputFrame.width, outputFrame.height, iso, sensitivity, ispGain, perFrameEv,
                 rotate, zoom, luxIndex);
        DumpYuvBuffer(outputbuf, &outputFrame);
    }
    return resultInfo;
};
ProcessRetStatus PortraitRepair::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.portraitrepair.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);
    auto &inputBuffers = pProcessRequestInfo->inputBuffersMap.begin()->second;
    camera_metadata_t *metadata = inputBuffers[0].pMetadata;
    camera_metadata_t *physicalMeta = inputBuffers[0].pPhyCamMetadata;
    MiImageBuffer inputFrame;
    convertImageParams(inputBuffers[0], inputFrame);
    // cpu boost 3000 ms
    std::shared_ptr<PluginPerfLockManager> boostCpu(new PluginPerfLockManager(3000));

    yuv_image_t stm_input;
    if (inputFrame.format == CAM_FORMAT_YUV_420_NV12) {
        stm_input.format = YUV_IMAGE_FMT_NV12;
    } else if (inputFrame.format == CAM_FORMAT_YUV_420_NV21) {
        stm_input.format = YUV_IMAGE_FMT_NV21;
    } else {
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair not support the format %d ", inputFrame.format);
    }
    stm_input.width = inputFrame.width;
    stm_input.height = inputFrame.height;
    stm_input.strides[0] = inputFrame.stride;
    stm_input.strides[1] = inputFrame.stride;
    stm_input.pY = inputFrame.plane[0];
    stm_input.pUV = inputFrame.plane[1];
    MLOGD(Mia2LogGroupPlugin,
          "PortraitRepair %s:%d >>> input width: %d, height: %d, data: %p, stride %d", __func__,
          __LINE__, stm_input.width, stm_input.height, stm_input.pY, inputFrame.stride);

    mWidth = inputFrame.width;
    mHeight = inputFrame.height;
    stm_rect_t faceInfo[10] = {};
    ChiRectEXT afRect;
    memset(&afRect, 0, sizeof(ChiRectEXT));
    ChiRect cropRegion;
    memset(&cropRegion, 0, sizeof(ChiRect));

    int faceNum = 0;
    int processFaceNum = 0;
    getFaceInfo(faceInfo, &cropRegion, metadata, physicalMeta, &faceNum, &afRect);
    bool isProcessface = isProcessFace(faceInfo, faceNum, &processFaceNum, afRect);

    void *data = NULL;
    int rotate = 0;
    VendorMetadataParser::getTagValue(metadata, ANDROID_JPEG_ORIENTATION, &data);
    if (NULL != data) {
        int orientation = *static_cast<int32_t *>(data);
        rotate = 360 - (orientation) % 360;
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair jpeg orientation %d rotate %d", orientation,
              rotate);
    }
    int sensitivity = -1;
    data = NULL;
    VendorMetadataParser::getTagValue(metadata, ANDROID_SENSOR_SENSITIVITY, &data);
    if (NULL != data) {
        sensitivity = *static_cast<int32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair get sensitivity %d", sensitivity);
    } else {
        sensitivity = 100;
    }
    float ispGain = 1.0;
    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, "com.xiaomi.sensorbps.gain", &data);
    if (NULL != data) {
        ispGain = *static_cast<float *>(data);
        MLOGD(Mia2LogGroupPlugin, "PortraitRepair get ispGain %f", ispGain);
    }
    int iso = (int)(ispGain * 100) * sensitivity / 100;
    int32_t perFrameEv = 0;
    data = NULL;
    VendorMetadataParser::getTagValue(metadata, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &data);
    if (NULL != data) {
        perFrameEv = *static_cast<int32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair get Ev %d", perFrameEv);
    }

    uint8_t stylizationType = 0; // 0: CIVI Vivid;  1: CIVI Classic;
    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, "com.xiaomi.sessionparams.stylizationType", &data);
    if (NULL != data) {
        stylizationType = *static_cast<uint8_t *>(data);
        MLOGD(Mia2LogGroupPlugin, "PortraitRepair get stylizationType %d", stylizationType);
    }

    data = NULL;
    int32_t sceneDetected = 0;
    VendorMetadataParser::getVTagValue(metadata, "xiaomi.ai.asd.sceneDetected", &data);
    if (NULL != data) {
        sceneDetected = *static_cast<int32_t *>(data);
        MLOGD(Mia2LogGroupPlugin, "PortraitRepair get sceneDetected mode: %d ", sceneDetected);
    } else {
        MLOGW(Mia2LogGroupPlugin, "PortraitRepair can not found tag xiaomi.ai.asd.sceneDetected");
    }

    float luxIndex = 0.0f;
    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, "com.xiaomi.statsconfigs.AecInfo", &data);
    if (NULL != data) {
        luxIndex = static_cast<InputMetadataAecInfo *>(data)->luxIndex;
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair get luxindex %f", luxIndex);
    } else {
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair getMetaData AECFrameControl failed");
    }

    float zoom = 0.0f;
    data = NULL;
    VendorMetadataParser::getTagValue(metadata, ANDROID_CONTROL_ZOOM_RATIO, &data);
    if (NULL != data) {
        zoom = *static_cast<float *>(data);
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair get zoom =%f", zoom);
    } else {
        MLOGW(Mia2LogGroupPlugin, "PortraitRepair get android zoom ratio failed");
    }
    if (mIsDrawFaceRect) {
        drawFaceRect(&inputFrame, faceInfo, faceNum, afRect);
    }

    if (iIsDumpData) {
        char inputbuf[128];
        snprintf(
            inputbuf, sizeof(inputbuf),
            "PortraitRepair_input_%dx%d_iso_%d_s_%d_g_%.3f_ev_%d_rotate_%d_zoom_%.3f_luxIndex_%.3f",
            inputFrame.width, inputFrame.height, iso, sensitivity, ispGain, perFrameEv, rotate,
            zoom, luxIndex);
        DumpYuvBuffer(inputbuf, &inputFrame);
    }

    bool isProcessEnable = false;
    if ((pProcessRequestInfo->runningTaskNum < pProcessRequestInfo->maxConcurrentNum - 2) &&
        isProcessface && (processFaceNum > 0) && (sceneDetected != 1) && (zoom < 10)) {
        isProcessEnable = true;
    }

    if (((1.0 <= zoom) && (zoom <= 1.3) && (luxIndex <= 200) && (iso <= 100) &&
         (mOperationMode == StreamConfigModeSAT)) ||
        ((iso < 80) && (mOperationMode == StreamConfigModeMiuiZslFront))) {
        isProcessEnable = false;
    }

    MLOGI(Mia2LogGroupPlugin,
          "PortraitRepair runningTaskNum %d, g_algostatus %d, isProcessEnable %d",
          pProcessRequestInfo->runningTaskNum, g_algostatus, isProcessEnable);

    process_thread_params processParam;
    memset(&processParam, 0, sizeof(processParam));
    pthread_t m_process_thread = 0;

    double startTime = PluginUtils::nowMSec();
    g_algomutex.lock();
    if (isProcessEnable && (g_algostatus > ALGO_INIT_FAIL) && (g_algostatus != ALGO_PROCESS)) {
        g_algostatus = ALGO_PROCESS;
        g_algomutex.unlock();
        mPortraitEnable = true;
        processParam.orient = rotate;
        processParam.iso = iso;
        processParam.faceNum = processFaceNum;
        processParam.faceInfo = faceInfo;
        processParam.stm_input = stm_input;
        loadConfigParams(stylizationType);
        if (pthread_create(&m_process_thread, NULL, AlgoProcessThread, &processParam) != 0) {
            MLOGW(Mia2LogGroupPlugin, "pthread_create failed ");
            g_algomutex.lock();
            g_algostatus = ALGO_CREATE_THREAD_FAIL;
            g_algomutex.unlock();
        } else {
            g_algomutex.lock();
            if (g_algostatus != ALGO_PROCESS_DONE) {
                g_algomutex.unlock();
                std::unique_lock<std::mutex> lockWait(g_algomutex);
                std::cv_status cvsts =
                    g_condProcessWait.wait_for(lockWait, std::chrono::seconds(5));
                if (cvsts == std::cv_status::timeout) {
                    MLOGE(Mia2LogGroupPlugin, "PortraitRepair: process time more than 5s");
                    double t1 = PluginUtils::nowMSec();
                    stm_portrait_repair_cancel(g_algohandle);
                    pthread_join(m_process_thread, NULL);
                    g_algostatus = ALGO_PROCESS_TIMEOUT;
                    double t2 = PluginUtils::nowMSec();
                    mPortraitEnable = false;
                    MLOGI(Mia2LogGroupPlugin, "stm_portrait_repair_cancel cost time %.2f ms",
                          t2 - t1);
                } else {
                    g_algostatus = ALGO_PROCESS_DONE;
                    double t3 = PluginUtils::nowMSec();
                    MLOGI(Mia2LogGroupPlugin,
                          "PortraitRepairesult: %d, g_algostatus %d process time "
                          "%.2f ms",
                          processParam.PortraitRepairResult, g_algostatus, t3 - startTime);
                }

            } else {
                g_algomutex.unlock();
                pthread_join(m_process_thread, NULL);
                mPortraitEnable = false;
            }
        }
    } else {
        g_algomutex.unlock();
        mPortraitEnable = false;
    }

    if ((processParam.PortraitRepairResult != STM_OK) ||
        (g_algostatus == ALGO_CREATE_THREAD_FAIL)) {
        mPortraitEnable = false;
        MLOGW(Mia2LogGroupPlugin, "process fail PortraitRepairResult %d",
              processParam.PortraitRepairResult);
    }

    double end = PluginUtils::nowMSec();
    MLOGD(Mia2LogGroupPlugin, "PortraitRepairResult %d  process time  %.2f ms",
          processParam.PortraitRepairResult, end - startTime);
    if (mNodeInterface.pSetResultMetadata != NULL) {
        std::stringstream exifmsg;
        exifmsg << "PortraitRepair:" << mPortraitEnable
                << " {result:" << processParam.PortraitRepairResult
                << " faces:" << processParam.faceexifinfo.faces;
        exifmsg << " afRext:" << afRect.left << "_" << afRect.top << "_" << afRect.right << "_"
                << afRect.bottom << " isoverlap:" << isProcessface;
        exifmsg << " iso:" << iso << " rotate:" << rotate << " luxIndex:" << luxIndex
                << " scene:" << sceneDetected << " time:" << end - startTime;
        for (int i = 0; i < processParam.faceexifinfo.faces; i++) {
            exifmsg << " {blur:" << processParam.faceexifinfo.blur[i]
                    << " HDcrop:" << processParam.faceexifinfo.isHDCrop[i];
            exifmsg << " rects:" << processParam.faceexifinfo.rects[i][0] << ","
                    << processParam.faceexifinfo.rects[i][1];
            exifmsg << "," << processParam.faceexifinfo.rects[i][2] << ","
                    << processParam.faceexifinfo.rects[i][3] << "}";
        }
        exifmsg << "}";
        std::string results = exifmsg.str();
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, pProcessRequestInfo->frameNum,
                                          pProcessRequestInfo->timeStamp, results);
    }

    int set_tag_result = VendorMetadataParser::updateTagValue(
        metadata, XIAOMI_PORTRAITREPAIR_ENABLED, &mPortraitEnable, 1, NULL);
    MLOGD(Mia2LogGroupPlugin, "mPortraitEnable %d set_tag_result %d", mPortraitEnable,
          set_tag_result);

    if (iIsDumpData) {
        char outputbuf[128];
        snprintf(outputbuf, sizeof(outputbuf),
                 "PortraitRepair_output_%dx%d_iso_%d_s_%d_g_%.3f_ev_%d_rotate_%d_zoom_%.3f_"
                 "luxIndex_%.3f",
                 inputFrame.width, inputFrame.height, iso, sensitivity, ispGain, perFrameEv, rotate,
                 zoom, luxIndex);
        DumpYuvBuffer(outputbuf, &inputFrame);
    }

    return resultInfo;
}
int PortraitRepair::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

bool PortraitRepair::isFaceFocusFrameOverlap(_stm_rect *faceInfo, ChiRectEXT afRect, int faceNum)
{
    bool isoverlap = false;
    for (int i = 0; i < faceNum; i++) {
        isoverlap = isFaceFocusFrameOverlap(faceInfo[i], afRect);
        if (isoverlap == true) {
            return isoverlap;
        }
    }
    return isoverlap;
}

bool PortraitRepair::isFaceFocusFrameOverlap(stm_rect_t faceInfo, ChiRectEXT afRect)
{
    bool isoverlap = false;
    // small face not process: https://xiaomi.f.mioffice.cn/docs/dock4dgMKK2Pt0Ny9sy9xOVnlyb
    if (((faceInfo.right - faceInfo.left) * 20 < mWidth) ||
        ((faceInfo.bottom - faceInfo.top) * 20 < mHeight)) {
        return isoverlap;
    }
    if ((afRect.right > faceInfo.left) && (faceInfo.right > afRect.left) &&
        (afRect.bottom > faceInfo.top) && (faceInfo.bottom > afRect.top)) {
        int overlapWidth = (min(faceInfo.right, afRect.right)) - (max(faceInfo.left, afRect.left));
        int overlapHeight = (min(faceInfo.bottom, afRect.bottom)) - (max(faceInfo.top, afRect.top));
        isoverlap = (overlapWidth * overlapHeight) > 10000 ? true : false;
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair overlapWidth_Height: %d_%d isoverlap:%d",
              overlapWidth, overlapHeight, isoverlap);
    }
    return isoverlap;
}

// trigger logic: https://xiaomi.f.mioffice.cn/mindnotes/bmnk4JmMedIlAMZyQgxNJZKmdGc#mindmap
bool PortraitRepair::isProcessFace(_stm_rect *faceInfo, int faceNum, int *processFaceNum,
                                   ChiRectEXT afRect)
{
    bool isProcessFace = false;
    if ((afRect.left == 0) && (afRect.top == 0) && (afRect.right == 0) && (afRect.bottom == 0)) {
        *processFaceNum = faceNum;
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair getMetaData afRect is 0_0_0_0, processFaceNum %d",
              *processFaceNum);
        return true;
    } else {
        double faceAeraRatio = 0.0f;
        int faceAera[faceNum];

        for (int i = 0; i < faceNum; i++) {
            faceAera[i] =
                (faceInfo[i].right - faceInfo[i].left) * (faceInfo[i].bottom - faceInfo[i].top);
        }

        if (faceNum == 1) {
            isProcessFace = isFaceFocusFrameOverlap(faceInfo[0], afRect);
        } else if (faceNum == 2) {
            if (faceAera[1] > faceAera[0]) {
                faceAeraRatio = (double)faceAera[0] / (double)faceAera[1];
            } else {
                faceAeraRatio = (double)faceAera[1] / (double)faceAera[0];
            }
            if (faceAeraRatio >= 0.4) {
                isProcessFace = isFaceFocusFrameOverlap(faceInfo, afRect, faceNum);
            } else {
                if (isFaceFocusFrameOverlap(faceInfo[0], afRect) &&
                    isFaceFocusFrameOverlap(faceInfo[1], afRect)) {
                    isProcessFace = true;
                } else if (isFaceFocusFrameOverlap(faceInfo[0], afRect)) {
                    faceNum = 1;
                    isProcessFace = true;
                } else if (isFaceFocusFrameOverlap(faceInfo[1], afRect)) {
                    faceNum = 1;
                    faceInfo[0] = faceInfo[1];
                    isProcessFace = true;
                }
            }
        } else if (faceNum >= 3) {
            stm_rect_t faceRect;
            int transform;
            for (int i = 0; i < faceNum - 1; i++) {
                for (int j = faceNum - 1; j > i; j--) {
                    if (faceAera[j] > faceAera[j - 1]) {
                        faceRect = faceInfo[j];
                        faceInfo[j] = faceInfo[j - 1];
                        faceInfo[j - 1] = faceRect;
                        transform = faceAera[j];
                        faceAera[j] = faceAera[j - 1];
                        faceAera[j - 1] = transform;
                    }
                }
            }
            faceAeraRatio = (double)faceAera[2] / (double)faceAera[0];
            if (faceAeraRatio >= 0.4) {
                isProcessFace = isFaceFocusFrameOverlap(faceInfo, afRect, faceNum);
            } else if ((double)faceAera[1] / (double)faceAera[0] >= 0.4) {
                faceNum = 2;
                isProcessFace = isFaceFocusFrameOverlap(faceInfo, afRect, faceNum);
            } else {
                if (isFaceFocusFrameOverlap(faceInfo[0], afRect) &&
                    isFaceFocusFrameOverlap(faceInfo[1], afRect)) {
                    if ((double)faceAera[2] / (double)faceAera[1] >= 0.4) {
                        isProcessFace = true;
                    } else if (isFaceFocusFrameOverlap(faceInfo[2], afRect)) {
                        isProcessFace = true;
                    }
                } else if (isFaceFocusFrameOverlap(faceInfo[0], afRect)) {
                    faceNum = 1;
                    isProcessFace = true;
                    if (isFaceFocusFrameOverlap(faceInfo[2], afRect)) {
                        isProcessFace = true;
                        faceNum = 2;
                        faceInfo[1] = faceInfo[2];
                    } else {
                        isProcessFace = true;
                        faceNum = 1;
                    }
                } else if (isFaceFocusFrameOverlap(faceInfo[1], afRect)) {
                    faceNum = 1;
                    faceInfo[0] = faceInfo[1];
                    isProcessFace = true;
                    if ((double)faceAera[2] / (double)faceAera[1] >= 0.4) {
                        isProcessFace = true;
                        faceNum = 2;
                        faceInfo[0] = faceInfo[1];
                        faceInfo[1] = faceInfo[2];
                    } else if (isFaceFocusFrameOverlap(faceInfo[2], afRect)) {
                        isProcessFace = true;
                        faceNum = 2;
                        faceInfo[0] = faceInfo[1];
                        faceInfo[1] = faceInfo[2];
                    } else {
                        isProcessFace = true;
                        faceNum = 1;
                        faceInfo[0] = faceInfo[1];
                    }
                }
            }
        }
    }

    *processFaceNum = faceNum;
    MLOGI(Mia2LogGroupPlugin, "PortraitRepair get afRect:%d_%d_%d_%d, isoverlap %d, faceNum %d_%d",
          afRect.left, afRect.top, afRect.right, afRect.bottom, isProcessFace, faceNum,
          *processFaceNum);
    return isProcessFace;
}

int PortraitRepair::CopyYuvBuffer(yuv_image_t *hOutput, yuv_image_t *hInput)
{
    int ret = 0;
    uint32_t outPlaneStride = hOutput->width;
    uint32_t outHeight = hOutput->height;
    uint32_t outSliceheight = hOutput->height;
    uint32_t inPlaneStride = hInput->width;
    uint32_t inHeight = hInput->height;
    uint32_t inSliceheight = hInput->height;
    uint32_t copyPlaneStride = outPlaneStride > inPlaneStride ? inPlaneStride : outPlaneStride;
    uint32_t copyHeight = outHeight > inHeight ? inHeight : outHeight;
    MLOGI(Mia2LogGroupPlugin, "CopyImage input %d, %d, %d, output %d, %d, %d, format 0x%x 0x%x",
          inPlaneStride, inHeight, inSliceheight, outPlaneStride, outHeight, outSliceheight,
          hInput->format, hOutput->format);
    if ((hOutput->format == YUV_IMAGE_FMT_NV21) || (hOutput->format == YUV_IMAGE_FMT_NV12)) {
        for (uint32_t m = 0; m < copyHeight; m++) {
            memcpy((hOutput->pY + outPlaneStride * m), (hInput->pY + inPlaneStride * m),
                   copyPlaneStride);
        }
        for (uint32_t m = 0; m < (copyHeight >> 1); m++) {
            memcpy((hOutput->pUV + outPlaneStride * m), (hInput->pUV + inPlaneStride * m),
                   copyPlaneStride);
        }
    } else {
        MLOGW(Mia2LogGroupPlugin, "%s:%d unknow format: %d", __func__, __LINE__, hOutput->format);
        ret = -1;
    }
    return ret;
}

int PortraitRepair::CopyBuffer(stm_image_t *hOutput, stm_image_t *hInput)
{
    int ret = 0;
    uint32_t outPlaneStride = hOutput->width;
    uint32_t outHeight = hOutput->height;
    uint32_t outSliceheight = hOutput->height;
    uint32_t inPlaneStride = hInput->width;
    uint32_t inHeight = hInput->height;
    uint32_t inSliceheight = hInput->height;
    uint32_t copyPlaneStride = outPlaneStride > inPlaneStride ? inPlaneStride : outPlaneStride;
    uint32_t copyHeight = outHeight > inHeight ? inHeight : outHeight;
    MLOGI(Mia2LogGroupPlugin, "CopyImage input %d, %d, %d, output %d, %d, %d, format 0x%x 0x%x",
          inPlaneStride, inHeight, inSliceheight, outPlaneStride, outHeight, outSliceheight,
          hInput->format, hOutput->format);
    if ((hOutput->format == STM_IMAGE_FMT_NV21) || (hOutput->format == STM_IMAGE_FMT_NV12)) {
        for (uint32_t m = 0; m < copyHeight; m++) {
            memcpy((hOutput->data + outPlaneStride * m), (hInput->data + inPlaneStride * m),
                   copyPlaneStride);
        }
        for (uint32_t m = 0; m < (copyHeight >> 1); m++) {
            memcpy((hOutput->data + hOutput->width * hOutput->height + outPlaneStride * m),
                   (hInput->data + hInput->width * hInput->height + inPlaneStride * m),
                   copyPlaneStride);
        }
    } else {
        MLOGW(Mia2LogGroupPlugin, "%s:%d unknow format: %d", __func__, __LINE__, hOutput->format);
        ret = -1;
    }
    return ret;
}

void PortraitRepair::DumpYuvBuffer(const char *message, struct MiImageBuffer *buffer)
{
    char fileName[256] = {0};
    int res = 0;
    res = mkdir(IMG_DUMP_PATH, 0755);
    MLOGD(Mia2LogGroupPlugin, "mkdir return is %d,errno=%d ,strerror: %s\n", res, errno,
          strerror(errno));

    char timeBuf[256];
    time_t currentTime;
    struct tm *timeInfo;

    time(&currentTime);
    timeInfo = localtime(&currentTime);
    strftime(timeBuf, sizeof(timeBuf), "IMG_%Y%m%d%H%M%S", timeInfo);
    sprintf(fileName, "%s%s_%s.yuv", IMG_DUMP_PATH, timeBuf, message);
    PluginUtils::miDumpBuffer(buffer, fileName);
}

bool PortraitRepair::IsFrontCamera()
{
    // if front camera is true
    bool msFrontCamera = false;
    if (CAM_COMBMODE_FRONT == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_BOKEH == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_AUX == mFrameworkCameraId) {
        msFrontCamera = true;
    }
    // rear camera
    else {
        msFrontCamera = false;
    }
    MLOGI(Mia2LogGroupCore, "PortraitRepair debug camera front is %d", msFrontCamera);
    return msFrontCamera;
}

void PortraitRepair::loadConfigParams(uint8_t stylizationType)
{
    int maxFace = 0;
    maxFace =
        mOperationMode == StreamConfigModeBokeh || mOperationMode == StreamConfigModeFrontBokeh ? 1
                                                                                                : 3;
    bool isFrontCamera = IsFrontCamera();
    if ((stylizationType == 1) && isFrontCamera) {
        stm_portrait_repair_config_json(g_algohandle, Front_Classic_JSON_PATH);
    } else if ((stylizationType == 0) && isFrontCamera) {
        stm_portrait_repair_config_json(g_algohandle, Front_Vivid_JSON_PATH);
    } else if ((stylizationType == 1) && !isFrontCamera) {
        stm_portrait_repair_config_json(g_algohandle, Rear_Classic_JSON_PATH);
    } else {
        stm_portrait_repair_config_json(g_algohandle, Rear_Vivid_JSON_PATH);
    }
    stm_portrait_repair_config(g_algohandle, STM_CONFIG_QUALITY_DEBUG, &mQualityDebug);
    stm_portrait_repair_config(g_algohandle, STM_CONFIG_MAX_FACES, &maxFace);
    MLOGD(Mia2LogGroupPlugin, "mQualityDebug %d, maxFace %d", mQualityDebug, maxFace);
}

void PortraitRepair::drawFaceRect(MiImageBuffer *image, _stm_rect *faceInfo, int facenum,
                                  ChiRectEXT afRect)
{
    ASVLOFFSCREEN stOutImage;
    prepareImage(image, stOutImage);
    MLOGI(Mia2LogGroupPlugin, "PortraitRepair get facenum:%d", facenum);
    for (int i = 0; i < facenum; i++) {
        MRECT RectDraw = {0};
        RectDraw.left = faceInfo[i].left;
        RectDraw.top = faceInfo[i].top;
        RectDraw.bottom = faceInfo[i].bottom;
        RectDraw.right = faceInfo[i].right;
        DrawRect(&stOutImage, RectDraw, 5, 0);
    }

    MRECT afRectDraw = {0};
    afRectDraw.left = afRect.left;
    afRectDraw.top = afRect.top;
    afRectDraw.bottom = afRect.bottom;
    afRectDraw.right = afRect.right;
    DrawRect(&stOutImage, afRectDraw, 20, 0);
    MLOGD(Mia2LogGroupPlugin, "drawFaceRect");
}

void PortraitRepair::prepareImage(MiImageBuffer *image, ASVLOFFSCREEN &stImage)
{
    stImage.i32Width = image->width;
    stImage.i32Height = image->height;

    if (image->format != CAM_FORMAT_YUV_420_NV12 && image->format != CAM_FORMAT_YUV_420_NV21 &&
        image->format != CAM_FORMAT_Y16) {
        MLOGE(Mia2LogGroupPlugin, "[ARC_CTB] format[%d] is not supported!", image->format);
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

    for (uint32_t i = 0; i < 2; i++) {
        stImage.ppu8Plane[i] = image->plane[i];
        stImage.pi32Pitch[i] = image->stride;
    }
    MLOGD(Mia2LogGroupPlugin, "X");
}

void PortraitRepair::getFaceInfo(_stm_rect *faceInfo, CHIRECT *cropRegion,
                                 camera_metadata_t *metaData, camera_metadata_t *physicalMeta,
                                 int *faceNum, ChiRectEXT *afRect)
{
    void *data = NULL;
    float zoomRatio = 1.0;

    const char *cropInfo = "xiaomi.snapshot.cropRegion";
    VendorMetadataParser::getVTagValue(metaData, cropInfo, &data);

    if (NULL != data) {
        *cropRegion = *reinterpret_cast<CHIRECT *>(data);
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair get CropRegion begin [%d, %d, %d, %d]",
              cropRegion->left, cropRegion->top, cropRegion->width, cropRegion->height);
    } else {
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair get crop failed, try ANDROID_SCALER_CROP_REGION");
        VendorMetadataParser::getTagValue(metaData, ANDROID_SCALER_CROP_REGION, &data);
        if (NULL != data) {
            *cropRegion = *reinterpret_cast<CHIRECT *>(data);
            MLOGI(Mia2LogGroupPlugin, "PortraitRepair get CropRegion begin [%d, %d, %d, %d]",
                  cropRegion->left, cropRegion->top, cropRegion->width, cropRegion->height);
        } else {
            MLOGI(Mia2LogGroupPlugin, "PortraitRepair get CropRegion failed");
            cropRegion->left = 0;
            cropRegion->top = 0;
            cropRegion->width = mWidth;
            cropRegion->height = mHeight;
        }
    }

    if (cropRegion->width == 0 || cropRegion->height == 0) {
        cropRegion->left = 0;
        cropRegion->top = 0;
        cropRegion->width = mWidth;
        cropRegion->height = mHeight;
    }

    if (cropRegion->width != 0)
        zoomRatio = (float)mWidth / cropRegion->width;

    float scaleWidth = 0.0;
    float scaleHeight = 0.0;
    if (mWidth != 0 && mHeight != 0) {
        scaleWidth = (float)cropRegion->width / mWidth;
        scaleHeight = (float)cropRegion->height / mHeight;
    } else {
        MLOGW(Mia2LogGroupPlugin, "get mWidth failed");
    }
    const float AspectRatioTolerance = 0.01f;

    if (scaleHeight > scaleWidth + AspectRatioTolerance) {
        // 16:9 case run to here, it means height is cropped.
        int32_t deltacropHeight = (int32_t)((float)mHeight * scaleWidth);
        cropRegion->top = cropRegion->top + (cropRegion->height - deltacropHeight) / 2;
    } else if (scaleWidth > scaleHeight + AspectRatioTolerance) {
        // 1:1 width is cropped.
        int32_t deltacropWidth = (int32_t)((float)mWidth * scaleHeight);
        cropRegion->left = cropRegion->left + (cropRegion->width - deltacropWidth) / 2;
        if (cropRegion->height != 0) {
            zoomRatio = (float)mHeight / cropRegion->height;
        } else {
            MLOGW(Mia2LogGroupPlugin, "get cropRegion or mWidth failed");
        }
    }
    data = NULL;
    ChiRectEXT AFRect;
    VendorMetadataParser::getTagValue(metaData, ANDROID_CONTROL_AF_REGIONS, &data);

    if (NULL != data) {
        AFRect = *static_cast<ChiRectEXT *>(data);
        int32_t xMin = (AFRect.left - static_cast<int32_t>(cropRegion->left)) * zoomRatio;
        int32_t xMax = (AFRect.right - static_cast<int32_t>(cropRegion->left)) * zoomRatio;
        int32_t yMin = (AFRect.top - static_cast<int32_t>(cropRegion->top)) * zoomRatio;
        int32_t yMax = (AFRect.bottom - static_cast<int32_t>(cropRegion->top)) * zoomRatio;

        xMin = max(min(xMin, (int32_t)mWidth - 2), 0);
        xMax = max(min(xMax, (int32_t)mWidth - 2), 0);
        yMin = max(min(yMin, (int32_t)mHeight - 2), 0);
        yMax = max(min(yMax, (int32_t)mHeight - 2), 0);

        afRect->left = xMin;
        afRect->top = yMin;
        afRect->right = xMax;
        afRect->bottom = yMax;
        MLOGI(Mia2LogGroupPlugin, "PortraitRepair AFRect[%d,%d,%d,%d] afRect [%d,%d,%d,%d]",
              AFRect.left, AFRect.bottom, AFRect.right, AFRect.top, afRect->left, afRect->top,
              afRect->right, afRect->bottom);
    } else {
        MLOGW(Mia2LogGroupPlugin, "PortraitRepair get af region failed.");
    }
    data = NULL;
    size_t tcount;
    FDMetadataResults curFaceInfo = {0};
    VendorMetadataParser::getTagValueCount(metaData, ANDROID_STATISTICS_FACE_RECTANGLES, &data,
                                           &tcount);

    if (NULL != data) {
        *faceNum = ((int32_t)tcount) / 4;
        MLOGD(Mia2LogGroupPlugin, "get faceNumber = %d", *faceNum);
        memcpy(curFaceInfo.faceRect, data, *faceNum * sizeof(MIRECT));
        // face rect get from ANDROID_STATISTICS_FACE_RECTANGLES is [left, top, right, bottom]
        // FDMetadataResultsfaceRect[FDMaxFaces]             --->   [left, top, width, height]
        for (int index = 0; index < (min(10, *faceNum)); index++) {
            int32_t xMin =
                (curFaceInfo.faceRect[index].left - static_cast<int32_t>(cropRegion->left)) *
                zoomRatio;
            int32_t xMax =
                (curFaceInfo.faceRect[index].width - static_cast<int32_t>(cropRegion->left)) *
                zoomRatio;
            int32_t yMin =
                (curFaceInfo.faceRect[index].top - static_cast<int32_t>(cropRegion->top)) *
                zoomRatio;
            int32_t yMax =
                (curFaceInfo.faceRect[index].height - static_cast<int32_t>(cropRegion->top)) *
                zoomRatio;

            xMin = max(min(xMin, (int32_t)mWidth - 2), 0);
            xMax = max(min(xMax, (int32_t)mWidth - 2), 0);
            yMin = max(min(yMin, (int32_t)mHeight - 2), 0);
            yMax = max(min(yMax, (int32_t)mHeight - 2), 0);

            faceInfo[index].left = xMin;
            faceInfo[index].top = yMin;
            faceInfo[index].right = xMax;
            faceInfo[index].bottom = yMax;

            MLOGI(Mia2LogGroupPlugin,
                  "PortraitRepair index:%d faceRect[%d,%d,%d,%d] cvt [%d,%d,%d,%d]", index,
                  curFaceInfo.faceRect[index].left, curFaceInfo.faceRect[index].top,
                  curFaceInfo.faceRect[index].width, curFaceInfo.faceRect[index].height,
                  faceInfo[index].left, faceInfo[index].top, faceInfo[index].right,
                  faceInfo[index].bottom);
        }
    } else {
        MLOGW(Mia2LogGroupPlugin, "PortraitRepairt get face failed or not detect face.");
    }
}
} // namespace mialgo2
