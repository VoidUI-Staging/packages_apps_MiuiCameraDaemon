#include "miaideblurplugin.h"

#define LOG_TAG                 "MiA_N_MIAIDeblur"
#define DEVICE_ENABLE           1
#define DEBLUR_SD_OPENCL_BINARY "/vendor/etc/camera/deblur_sd_opencl_binary.bin"
#define DEBLUR_SD_OPENCL_PARAMS "/vendor/etc/camera/deblur_sd_opencl_params.bin"

static const char *initParams[] = {DEBLUR_SD_OPENCL_BINARY, DEBLUR_SD_OPENCL_PARAMS};

static const int32_t sPropSceneFlag =
    property_get_int32("persist.vendor.camera.miaideblur.scene_flag", 0);
static const int32_t sFrameRatio =
    property_get_int32("persist.vendor.camera.miaideblur.frameratio", -1);
static const int32_t sMiaideblurDump =
    property_get_int32("persist.vendor.camera.miaideblur.dump", 0);
static const int32_t sDisableDeblur =
    property_get_int32("persist.vendor.camera.miaideblur.disable_deblur", 0);
static const int32_t sTxtDetectProp =
    property_get_int32("persist.vendor.camera.miaideblur.txt_detect", -1);
static const int32_t sPropTof = property_get_int32("persist.vendor.camera.miaideblur.tof", 0);
static const int32_t sBsTest = property_get_int32("persist.camera.arc.bs.test", 0);
static const int32_t DeblurEnable =
    property_get_int32("persist.vendor.camera.miaideblur.enable", 1);

using namespace mialgo2;

MiAIDeblurPlugin::~MiAIDeblurPlugin()
{
    MLOGD(Mia2LogGroupPlugin, "%s:%d call destructor of MiAIDeblurPlugin   : %p", __func__,
          __LINE__, this);
}

int MiAIDeblurPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    mFrameworkCameraId = pCreateInfo->frameworkCameraId;
    MLOGD(Mia2LogGroupPlugin, "%s:%d call Initialize... %p, mFrameworkCameraId: %d", __func__,
          __LINE__, this, mFrameworkCameraId);

    initFlags = new int[SCENE_COUNT];
    for (int i = 0; i < SCENE_COUNT; i++) {
        initFlags[i] = 0;
    }
    int propSceneFlag = sPropSceneFlag;
    if (propSceneFlag > 0) {
        MLOGD(Mia2LogGroupPlugin, "got scene_flag from prop:%d, metadata scene_flag:%d",
              propSceneFlag, mSceneFlag);
        mSceneFlag = propSceneFlag;
    }
    MLOGD(Mia2LogGroupPlugin, "mSceneFlag:%d", mSceneFlag);
    std::unique_lock<std::mutex> lock(mMutex);
    mDeblurParams.config.sceneType = mia_deblur::SCENE_DOC;

    MLOGD(Mia2LogGroupPlugin, "sceneType:%d", mDeblurParams.config.sceneType);

    mPostMiDeblurInitFinish = false;
    mPostMiDeblurInitThread = new std::thread([this] {
        MLOGD(Mia2LogGroupPlugin, "%s:%d call deblur_init......", __func__, __LINE__);
        // mia_deblur::deblur_init(initParams, mia_deblur::SCENE_DOC);
        initDeblur(mDeblurParams.config.sceneType);
        mPostMiDeblurInitFinish = true;
        MLOGD(Mia2LogGroupPlugin, "%s:%d deblur_init finished......", __func__, __LINE__);
    });
    MLOGD(Mia2LogGroupPlugin, "%s:%d ends of Initialize......", __func__, __LINE__);
    return 0;
}

bool MiAIDeblurPlugin::isEnabled(MiaParams settings)
{
    int device_enable = DEVICE_ENABLE;
    bool byPassRequest = isByPassRequest(settings.metadata);
    MLOGD(Mia2LogGroupPlugin, "%s:  DEVICE_ENABLE:%d, byPassRequest:%d", __func__, device_enable,
          byPassRequest);
    return device_enable && !byPassRequest;
}

bool MiAIDeblurPlugin::isByPassRequest(camera_metadata_t *pMetadata)
{
    if (!DeblurEnable) {
        MLOGD(Mia2LogGroupPlugin, "set deblurenable = %d, bypass", DeblurEnable);
        return true;
    }

    if (mFrameworkCameraId == CAM_COMBMODE_REAR_ULTRA ||
        mFrameworkCameraId == CAM_COMBMODE_REAR_SAT_WU
        /*|| mFrameworkCameraId == CAM_COMBMODE_REAR_ULTRA_TELE*/) {
        MLOGD(Mia2LogGroupPlugin, "%s:%d deblurBypass : 0.6x/macro/0.6x fusion [cameraId: %d]",
              __func__, __LINE__, mFrameworkCameraId);
        return true;
    }
    getMetaData(pMetadata);
    //！= 1X bypass
    if (abs(mZoom - 1) > 0.1) {
        MLOGD(Mia2LogGroupPlugin, "%s:%d deblurBypass : not 1x", __func__, __LINE__);
        return true;
    }
    bool deblurEnable = isDeblurEnable(pMetadata);
    if (!deblurEnable) {
        MLOGD(Mia2LogGroupPlugin, "%s:%d deblurBypass : exclusive scene condition take effect",
              __func__, __LINE__);
        return true;
    }

    return false;
}

bool MiAIDeblurPlugin::isDeblurEnable(camera_metadata_t *pMetadata)
{
    if (pMetadata == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s error metaData is null", __func__);
        return false;
    }
    // check hdr scene
    bool hdrEnable = false;
    void *pData = NULL;
    VendorMetadataParser::getVTagValue(pMetadata, "xiaomi.hdr.enabled", &pData);
    if (NULL != pData) {
        hdrEnable = *static_cast<uint8_t *>(pData);
    }
    if (hdrEnable) {
        MLOGI(Mia2LogGroupPlugin, "MIAI_DEBLUR, hdrEnable=%d\n", hdrEnable);
        return false;
    }
    // check skinbeauty scene
    bool m_beautyEnabled = false;
    BeautyFeatureParams mBeautyFeatureParams;
    int beautyFeatureCounts = mBeautyFeatureParams.featureCounts;
    for (int i = 0; i < beautyFeatureCounts; i++) {
        if (mBeautyFeatureParams.beautyFeatureInfo[i].featureMask == true) {
            pData = NULL;
            VendorMetadataParser::getVTagValue(
                pMetadata, mBeautyFeatureParams.beautyFeatureInfo[i].TagName, &pData);
            if (NULL != pData) {
                mBeautyFeatureParams.beautyFeatureInfo[i].featureValue =
                    *static_cast<int32_t *>(pData);
            }
            if (m_beautyEnabled == false &&
                mBeautyFeatureParams.beautyFeatureInfo[i].featureValue != 0) {
                m_beautyEnabled = true;
            }
        }
    }
    if (m_beautyEnabled) {
        MLOGI(Mia2LogGroupPlugin, "MIAI_DEBLUR, skinBeautyEnable=%d\n", m_beautyEnabled);
        return false;
    }
    // check for supernight scene
    bool snEnable = false;
    pData = NULL;
    VendorMetadataParser::getVTagValue(pMetadata, "xiaomi.supernight.raw.enabled", &pData);
    if (NULL != pData) {
        snEnable = *static_cast<uint8_t *>(pData);
    }
    if (snEnable) {
        MLOGI(Mia2LogGroupPlugin, "MIAI_DEBLUR, snEnable=%d\n", snEnable);
        return false;
    }
    return true;
}

void MiAIDeblurPlugin::getMetaData(camera_metadata_t *pMetadata)
{
    MLOGD(Mia2LogGroupPlugin, "%s:%d  enter GetMetaData ......", __func__, __LINE__);
    void *pData = NULL;
    mDeblurParams.config.orientation = static_cast<char>(0);
    pData = NULL;
    // get jpeg orientation
    VendorMetadataParser::getTagValue(pMetadata, ANDROID_JPEG_ORIENTATION, &pData);
    if (NULL != pData) {
        int32_t oriention = 0;
        oriention = *static_cast<int32_t *>(pData);
        mDeblurParams.config.orientation = static_cast<char>((oriention % 360) / 90);
        MLOGD(Mia2LogGroupPlugin, "%s:%d    got oriention:%d, mOrientation: %d", __func__, __LINE__,
              oriention, mDeblurParams.config.orientation);
    } else {
        MLOGW(Mia2LogGroupPlugin, "%s:%d  get orientation metaData failed........", __func__,
              __LINE__);
    }

    // get zoomratio
    mZoom = 1.0f;
    pData = NULL;
    VendorMetadataParser::getTagValue(pMetadata, ANDROID_CONTROL_ZOOM_RATIO, &pData);
    if (NULL != pData) {
        mZoom = *static_cast<float *>(pData);
        MLOGD(Mia2LogGroupPlugin, "get zoom =%f", mZoom);
    } else {
        MLOGI(Mia2LogGroupPlugin, "get android zoom ratio failed");
    }
}

ProcessRetStatus MiAIDeblurPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    MLOGD(Mia2LogGroupPlugin, "%s:%d call ProcessRequest : %p", __func__, __LINE__, this);
    // ProcessResultInfo resultInfo;
    // resultInfo.result = PROCSUCCESS;
    ProcessRetStatus resultStatus;
    resultStatus = PROCSUCCESS;
    int32_t iIsDumpData = sMiaideblurDump;

    auto &phInputBuffer = pProcessRequestInfo->inputBuffersMap.at(0);
    auto &phOutputBuffer = pProcessRequestInfo->outputBuffersMap.at(0);
    int inputNum = phInputBuffer.size();
    MiImageBuffer inputFrame, outputFrame;
    inputFrame.format = phInputBuffer[0].format.format;
    inputFrame.width = phInputBuffer[0].format.width;
    inputFrame.height = phInputBuffer[0].format.height;
    inputFrame.plane[0] = phInputBuffer[0].pAddr[0];
    inputFrame.plane[1] = phInputBuffer[0].pAddr[1];
    inputFrame.stride = phInputBuffer[0].planeStride;
    inputFrame.scanline = phInputBuffer[0].sliceheight;
    inputFrame.numberOfPlanes = phInputBuffer[0].numberOfPlanes;
    camera_metadata_t *metaData = phInputBuffer[0].pMetadata;

    outputFrame.format = phOutputBuffer[0].format.format;
    outputFrame.width = phOutputBuffer[0].format.width;
    outputFrame.height = phOutputBuffer[0].format.height;
    outputFrame.plane[0] = phOutputBuffer[0].pAddr[0];
    outputFrame.plane[1] = phOutputBuffer[0].pAddr[1];
    outputFrame.stride = phOutputBuffer[0].planeStride;
    outputFrame.scanline = phOutputBuffer[0].sliceheight;
    outputFrame.numberOfPlanes = phOutputBuffer[0].numberOfPlanes;

    MLOGD(Mia2LogGroupPlugin,
          "%s:%d >>> input width: %d, height: %d, stride: %d, scanline: %d, numberOfPlanes: %d, "
          "output stride: %d scanline: %d, format:%d, inputNum:%d",
          __func__, __LINE__, inputFrame.width, inputFrame.height, inputFrame.stride,
          inputFrame.scanline, inputFrame.numberOfPlanes, outputFrame.stride, outputFrame.scanline,
          inputFrame.format, inputNum);

    if (iIsDumpData) {
        char inputbuf[128];
        snprintf(inputbuf, sizeof(inputbuf), "miaideblur_input_%dx%d", inputFrame.width,
                 inputFrame.height);
        PluginUtils::dumpToFile(inputbuf, &inputFrame);
    }
    // do process
    int ret = PROCSUCCESS;
    MIAIImageBuffer inputBuf;
    inputBuf.Width = inputFrame.width;
    inputBuf.Height = inputFrame.height;
    inputBuf.PixelArrayFormat = inputFrame.format;
    inputBuf.Plane[0] = inputFrame.plane[0];
    inputBuf.Plane[1] = inputFrame.plane[1];
    inputBuf.Pitch[0] = inputFrame.stride;
    inputBuf.Pitch[1] = inputFrame.stride;
    inputBuf.Scanline[0] = inputFrame.scanline;

    MIAIImageBuffer outputBuf;
    outputBuf.Width = outputFrame.width;
    outputBuf.Height = outputFrame.height;
    outputBuf.PixelArrayFormat = outputFrame.format;
    outputBuf.Plane[0] = outputFrame.plane[0];
    outputBuf.Plane[1] = outputFrame.plane[1];
    outputBuf.Pitch[0] = outputFrame.stride;
    outputBuf.Pitch[1] = outputFrame.stride;
    outputBuf.Scanline[0] = outputFrame.scanline;

    mDeblurParams.config.disable_deblur = sDisableDeblur;

    int txtDetect = 1;
    int32_t txtDetectProp = sTxtDetectProp;
    if (txtDetectProp != -1) {
        txtDetect = txtDetectProp;
    }

    mDeblurParams.config.txt_detect = txtDetect;

    if (mPostMiDeblurInitThread && mPostMiDeblurInitThread->joinable()) {
        MLOGD(Mia2LogGroupPlugin, "%s:%d join init before process......", __func__, __LINE__);
        mPostMiDeblurInitThread->join();
    }
    MLOGD(Mia2LogGroupPlugin, "%s:%d before call deblur_process......mPostMiDeblurInitFinish: %d",
          __func__, __LINE__, mPostMiDeblurInitFinish);

    void *pData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_LENS_FOCUS_DISTANCE, &pData);
    float focusDistance;
    if (NULL != pData) {
        focusDistance = *static_cast<float *>(pData);
        MLOGD(Mia2LogGroupPlugin, "deblur get focusdistance %f", focusDistance);
    }
    // get faceinfo
    mia_deblur::FaceInfo faceInfo = {0, NULL};
    getFaceInfo(metaData, &faceInfo, inputFrame.width, inputFrame.height);
    if (faceInfo.faceNum > 0) {
        mia_deblur::setFaceInfoBeforeProcess(&faceInfo);
    }
    int processStatus = mia_deblur::WORK_FAILED;
#ifdef AGATE_CAMERA
    if (focusDistance > 2) {
        processStatus =
            mia_deblur::deblur_process((void *)&inputBuf, (void *)&outputBuf, &mDeblurParams);
    }
#else
    processStatus =
        mia_deblur::deblur_process((void *)&inputBuf, (void *)&outputBuf, &mDeblurParams);
#endif
    if (processStatus == mia_deblur::WORK_SUCCESS) { // run deblur
        MLOGD(Mia2LogGroupPlugin, "deblur processed...processStatus:%d", processStatus);
    } else {
        // ret = PluginUtils::MiCopyBuffer(&outputFrame, &inputFrame);
        ret = PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        MLOGD(Mia2LogGroupPlugin, "deblur not process...processStatus:%d", processStatus);
        MLOGI(Mia2LogGroupPlugin, "%s:%d miCopyBuffer ret: %d", __func__, __LINE__, ret);
    }
    if (ret == -1) {
        // resultInfo.result = PROCFAILED;
        resultStatus = PROCFAILED;
    }
    if (iIsDumpData) {
        char outputbuf[128];
        snprintf(outputbuf, sizeof(outputbuf),
                 "miaideblur_output_%dx%d_scenetype_%d_focusdistance_%f", outputFrame.width,
                 outputFrame.height, mDeblurParams.config.sceneType, focusDistance);
        PluginUtils::dumpToFile(outputbuf, &outputFrame);
    }

    return resultStatus;
}

void MiAIDeblurPlugin::getFaceInfo(camera_metadata_t *metaData, mia_deblur::FaceInfo *faceInfo,
                                   int w, int h)
{
    faceInfo->faceNum = 0;
    float zoomRatio = 1.0;
    void *data = NULL;
    size_t tcount = 0;
    VendorMetadataParser::getTagValueCount(metaData, ANDROID_STATISTICS_FACE_RECTANGLES, &data,
                                           &tcount);
    if (NULL == data) {
        faceInfo->faceNum = 0;
        return;
    }
    uint32_t numElemRect = sizeof(MIRECT) / sizeof(uint32_t);
    faceInfo->faceNum = (int32_t)(tcount / numElemRect);
    if (faceInfo->faceNum > FDMaxFaces) {
        faceInfo->faceNum = FDMaxFaces;
    }
    MLOGI(Mia2LogGroupPlugin, "[deblur]: GetFaceInfo i32FaceNum:%d", faceInfo->faceNum);

    MIRECT *rect = new MIRECT[faceInfo->faceNum];
    memcpy(rect, data, sizeof(MIRECT) * faceInfo->faceNum);
    for (MInt32 index = 0; index < faceInfo->faceNum; index++) {
        faceInfo->faceRectList[index].startX = rect->left;
        faceInfo->faceRectList[index].startY = rect->top;
        faceInfo->faceRectList[index].width = rect->width - rect->left + 1;
        faceInfo->faceRectList[index].height = rect->height - rect->top + 1;
        MLOGI(Mia2LogGroupPlugin, "deblur facerect [left,top,width,height][%d,%d,%d,%d]",
              faceInfo->faceRectList[index].startX, faceInfo->faceRectList[index].startY,
              faceInfo->faceRectList[index].width, faceInfo->faceRectList[index].height);
    }

    delete[] rect;
    rect = NULL;
}

void MiAIDeblurPlugin::initDeblur(mia_deblur::SceneType sceneType)
{
    int sceneTypeIndex = static_cast<int>(sceneType);
    for (int i = 0; i < SCENE_COUNT; i++) {
        MLOGD(Mia2LogGroupPlugin, "%s:%d call deblur_init for sceneTypeIndex:%d initFlag[%d]:%d",
              __func__, __LINE__, sceneTypeIndex, i, initFlags[i]);
    }
    int initFlag = initFlags[sceneTypeIndex - 1];
    if (initFlag == 0) {
        MLOGD(Mia2LogGroupPlugin,
              "%s:%d before deblur_init for sceneTypeIndex:%d, openclBinaryPath:%s, "
              "openclParamsPath:%s",
              __func__, __LINE__, sceneType, initParams[0], initParams[1]);

        // mia_deblur::SDInitMode sdInitMode = mia_deblur::INIT_WARMUP;
        // enum mia_deblur::SDInitMode  sdinitmode = mia_deblur::INIT_WARMUP;
        bool initResult =
            mia_deblur::deblur_init(initParams, sceneType, mia_deblur::SDInitMode::INIT_WARMUP);
        initFlags[sceneTypeIndex - 1] = 1;
        MLOGD(Mia2LogGroupPlugin, "%s:%d deblur_init finished for sceneTypeIndex:%d, initResult:%d",
              __func__, __LINE__, sceneTypeIndex, initResult);
    } else {
        MLOGD(Mia2LogGroupPlugin, "%s:%d already init deblur for sceneTypeIndex:%d", __func__,
              __LINE__, sceneTypeIndex);
    }
}

ProcessRetStatus MiAIDeblurPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    // ProcessResultInfo resultInfo;
    // resultInfo.result = PROCSUCCESS;
    ProcessRetStatus resultStatus;
    resultStatus = PROCSUCCESS;
    return resultStatus;
}

int MiAIDeblurPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

void MiAIDeblurPlugin::destroy()
{
    bool initFlag = false;
    initFlag = true;
    mPostMiDeblurInitFinish = false;
    if (mPostMiDeblurInitThread && mPostMiDeblurInitThread->joinable()) {
        mPostMiDeblurInitThread->join();
    }
    if (mPostMiDeblurInitThread) {
        delete mPostMiDeblurInitThread;
        mPostMiDeblurInitThread = NULL;
    }
    for (int i = 0; i < SCENE_COUNT; i++) {
        if (initFlags[i] == 1) {
            initFlag = true;
            break;
        }
    }

    if (initFlag) {
        mia_deblur::deblur_release();
    }
    delete[] initFlags;
    MLOGD(Mia2LogGroupPlugin, "%s:%d %p onDestory", __func__, __LINE__, this);
}