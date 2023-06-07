#include "miaideblurplugin.h"
#include "MiaPluginUtils.h"

#undef LOG_TAG
#define LOG_TAG "MiA_N_MIAIDeblur"

// #define DEBLUR_SD_OPENCL_BINARY "/vendor/etc/camera/deblur_sd_opencl_binary.bin"
// #define DEBLUR_SD_OPENCL_PARAMS "/vendor/etc/camera/deblur_sd_opencl_params.bin"

// static const char *initParams[] = {DEBLUR_SD_OPENCL_BINARY, DEBLUR_SD_OPENCL_PARAMS};

static const int32_t sMiaideblurDump =
    property_get_int32("persist.vendor.camera.miaideblur.dump", 0);
static const int32_t sDeblurEnable = //-1: 默认逻辑  0: 不使能  1：强制使能
    property_get_int32("persist.vendor.camera.miaideblur.enable", -1);
static const int32_t sTxtDetectProp =
    property_get_int32("persist.vendor.camera.miaideblur.txt_detect", 1);
static const int32_t sResNormalProcNum =
    property_get_int32("persist.vendor.camera.miaideblur.resprocnum", 2);

using namespace mialgo2;

MiAIDeblurPlugin::~MiAIDeblurPlugin()
{
    MLOGD(Mia2LogGroupPlugin, "%s:%d call destructor of MiAIDeblurPlugin : %p", __func__, __LINE__,
          this);
}

int MiAIDeblurPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mNodeInterface = nodeInterface;
    mFrameworkCameraId = createInfo->frameworkCameraId;
    mUseAsdMeta = AIDEBLUR_CONFIG_USEASD;
    MLOGI(Mia2LogGroupPlugin, "Call initialize %p, mFrameworkCameraId=%d, miaideblur enable=%d",
          this, mFrameworkCameraId, sDeblurEnable);
    // If use ASD scene info, algo does not need to initialize scene detect.
    if (sDeblurEnable != 0) {
        bool ret = false;
        if (mUseAsdMeta) {
            ret = mia_deblur::deblur_init(nullptr, mia_deblur::SCENE_DOC, mia_deblur::NOT_INIT);
        } else {
            ret = mia_deblur::deblur_init(nullptr, mia_deblur::SCENE_DOC,
                                          mia_deblur::INIT_WARMUP_MEMORY_SAVER);
        }
        MLOGI(Mia2LogGroupPlugin, "deblur_init Done.ret=%d,useASD=%d", ret, mUseAsdMeta);
    }
    return 0;
}

bool MiAIDeblurPlugin::isEnabled(MiaParams settings)
{
    // persist.vendor.camera.miaideblur.enable
    if (sDeblurEnable != -1) {
        MLOGD(Mia2LogGroupPlugin, "miaideblur.enable:%d (0:bypass 1:force enable)", sDeblurEnable);
        return sDeblurEnable;
    }

    bool bypassRequest = NeedBypassRequest(settings.metadata);
    return !bypassRequest;
}

bool MiAIDeblurPlugin::NeedBypassRequest(camera_metadata_t *metaData)
{
    bool needBypass = false;
    void *pData = NULL;

    do {
        // Zoom Ratio
        float zoomRatio = 1.0f;
        VendorMetadataParser::getTagValue(metaData, ANDROID_CONTROL_ZOOM_RATIO, &pData);
        if (NULL != pData) {
            zoomRatio = *static_cast<float *>(pData);
        } else {
            MLOGW(Mia2LogGroupPlugin, "can not found tag \"ANDROID_CONTROL_ZOOM_RATIO\"");
        }
        if (abs(zoomRatio - 1) > 0.1) {
            MLOGD(Mia2LogGroupPlugin, "deblurBypass: ZoomRatio not 1x[%f]", zoomRatio);
            needBypass = true;
            break;
        }

        // ASD document scene check
        if (mUseAsdMeta) {
            pData = NULL;
            int32_t asdScene = 0;
            VendorMetadataParser::getVTagValue(metaData, "xiaomi.ai.asd.sceneDetected", &pData);
            if (NULL != pData) {
                asdScene = *static_cast<int32_t *>(pData);
                MLOGW(Mia2LogGroupPlugin, "asdScene = %d", asdScene);
            } else {
                MLOGW(Mia2LogGroupPlugin, "can not found tag \"xiaomi.ai.asd.sceneDetected\"");
            }
            // Document scene == 1
            if (asdScene != 1) {
                MLOGD(Mia2LogGroupPlugin, "deblurBypass: current ASD scene is %d", asdScene);
                needBypass = true;
                break;
            }
        }

        // glass model 6: Moire effect
        pData = NULL;
        CameraSceneDetectResult asdglassScene = {};
        VendorMetadataParser::getVTagValue(metaData, "xiaomi.ai.asd.sceneDetectedAFResult", &pData);
        if (NULL != pData) {
            asdglassScene = *static_cast<CameraSceneDetectResult *>(pData);
        } else {
            MLOGW(Mia2LogGroupPlugin, "can not fqound tag \"xiaomi.ai.asd.sceneDetectedAFResult\"");
        }
        if (asdglassScene.resultFlag == 6) {
            MLOGI(Mia2LogGroupPlugin, "deblurBypass: current ASD glass model scene is %d",
                  asdglassScene.resultFlag);
            needBypass = true;
            break;
        }

        // aiportraitdeblur "人像超清"
        pData = NULL;
        uint8_t miaiportraitdeblurEnabled = 0;
        VendorMetadataParser::getVTagValue(metaData, "xiaomi.aiportraitdeblur.enabled", &pData);
        if (NULL != pData) {
            miaiportraitdeblurEnabled = *static_cast<uint8_t *>(pData);
        } else {
            MLOGW(Mia2LogGroupPlugin, "can not found tag \"xiaomi.aiportraitdeblur.enable\"");
        }
        if (miaiportraitdeblurEnabled) {
            MLOGD(Mia2LogGroupPlugin, "deblurBypass: miaiportraitdeblurEnabled.");
            needBypass = true;
            break;
        }

        // Distance.
        pData = NULL;
        float focusDistance = 0.0;
        float focusDistCm = 0.0;
        VendorMetadataParser::getTagValue(metaData, ANDROID_LENS_FOCUS_DISTANCE, &pData);
        if (NULL != pData) {
            focusDistance = *static_cast<float *>(pData);
            if (focusDistance > 0.0f) {
                focusDistCm = 100.0f / focusDistance;
            }
            // MLOGI(Mia2LogGroupPlugin, "focusDistCm = %f", focusDistCm);
        } else {
            MLOGW(Mia2LogGroupPlugin, "\"ANDROID_LENS_FOCUS_DISTANCE\" not found!");
        }
        if (focusDistCm > 50.0f) {
            MLOGD(Mia2LogGroupPlugin, "deblurBypass: focusDistCm = %.2f > 50.00", focusDistCm);
            needBypass = true;
            break;
        }

        // HDR
        // Others hdr tag (xiaomi.hdr.sr.enabled).
        bool rawHdrEnable = false;
        bool yuvHdrEnable = false;
        pData = NULL;
        VendorMetadataParser::getVTagValue(metaData, "xiaomi.hdr.raw.enabled", &pData);
        if (NULL != pData) {
            rawHdrEnable = *static_cast<uint8_t *>(pData);
            // MLOGI(Mia2LogGroupPlugin, "iaomi.hdr.raw.enabled = %u", rawHdrEnable);
        } else {
            MLOGW(Mia2LogGroupPlugin, "\"xiaomi.hdr.raw.enabled\" not found!");
        }
        pData = NULL;
        VendorMetadataParser::getVTagValue(metaData, "xiaomi.hdr.enabled", &pData);
        if (NULL != pData) {
            yuvHdrEnable = *static_cast<uint8_t *>(pData);
            // MLOGI(Mia2LogGroupPlugin, "xiaomi.hdr.enabled = %u", yuvHdrEnable);
        } else {
            MLOGW(Mia2LogGroupPlugin, "\"xiaomi.hdr.enabled\" not found!");
        }
        if (rawHdrEnable || yuvHdrEnable) {
            MLOGD(Mia2LogGroupPlugin, "deblurBypass: HDR Enable");
            needBypass = true;
            break;
        }

        // LLS
        int32_t llsEnabled = 0;
        pData = NULL;
        VendorMetadataParser::getVTagValue(metaData, "com.qti.stats_control.is_lls_needed", &pData);
        if (NULL != pData) {
            llsEnabled = *static_cast<int32_t *>(pData);
        } else {
            MLOGW(Mia2LogGroupPlugin, "\"com.qti.stats_control.is_lls_needed\" not found!");
        }
        if (llsEnabled) {
            MLOGD(Mia2LogGroupPlugin, "deblurBypass: LLS Enable");
            needBypass = true;
            break;
        }

        // supernight
        uint8_t superNightMode = 0;
        pData = NULL;
        VendorMetadataParser::getVTagValue(metaData, "xiaomi.mivi.supernight.mode", &pData);
        if (NULL != pData) {
            superNightMode = *static_cast<uint8_t *>(pData);
        } else {
            MLOGW(Mia2LogGroupPlugin, "\"xiaomi.mivi.supernight.mode\" not found!");
        }
        if (superNightMode) {
            MLOGD(Mia2LogGroupPlugin, "deblurBypass: supernight Enable");
            needBypass = true;
            break;
        }

        // skin beautifier
        pData = NULL;
        int32_t beautyFeatureValue = 0;
        // only need check one tag for "rear camera beautifier".
        VendorMetadataParser::getVTagValue(metaData, "xiaomi.beauty.skinSmoothRatio", &pData);
        if (NULL != pData) {
            beautyFeatureValue = *static_cast<int32_t *>(pData);
        } else {
            MLOGW(Mia2LogGroupPlugin, "\"xiaomi.beauty.skinSmoothRatio\" not found!");
        }
        if (beautyFeatureValue != 0) {
            MLOGD(Mia2LogGroupPlugin, "deblurBypass: beauty Enable");
            needBypass = true;
            break;
        }

    } while (0);

    return needBypass;
}

ProcessRetStatus MiAIDeblurPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    MLOGD(Mia2LogGroupPlugin, "ProcessRequest Start #%u.", pProcessRequestInfo->frameNum);
    ProcessRetStatus resultInfo = PROCSUCCESS;

    auto &inputBuffers = pProcessRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = pProcessRequestInfo->outputBuffersMap.begin()->second;
    camera_metadata_t *logiMeta = inputBuffers[0].pMetadata;
    camera_metadata_t *phyMeta = inputBuffers[0].pPhyCamMetadata;
    MiImageBuffer inputBuffer = {0};
    MiImageBuffer outputBuffer = {0};
    convertImageParams(inputBuffers[0], inputBuffer);
    convertImageParams(outputBuffers[0], outputBuffer);
    MLOGD(Mia2LogGroupPlugin, "Input:%d*%d(%d*%d) format=%d; Output:%d*%d(%d*%d) format=%d",
          inputBuffer.width, inputBuffer.height, inputBuffer.stride, inputBuffer.scanline,
          inputBuffer.format, outputBuffer.width, outputBuffer.height, outputBuffer.stride,
          outputBuffer.scanline, outputBuffer.format);

    if (sMiaideblurDump) {
        char inputDumpName[128];
        snprintf(inputDumpName, sizeof(inputDumpName), "miaideblur_input_%u_%dx%d",
                 pProcessRequestInfo->frameNum, inputBuffer.width, inputBuffer.height);
        PluginUtils::dumpToFile(inputDumpName, &inputBuffer);
    }

    MIAIImageBuffer algoInput;
    algoInput.Width = inputBuffer.width;
    algoInput.Height = inputBuffer.height;
    algoInput.PixelArrayFormat = inputBuffer.format;
    algoInput.Plane[0] = inputBuffer.plane[0];
    algoInput.Plane[1] = inputBuffer.plane[1];
    algoInput.Pitch[0] = inputBuffer.stride;
    algoInput.Pitch[1] = inputBuffer.stride;
    algoInput.Scanline[0] = inputBuffer.scanline;
    MIAIImageBuffer algoOutput;
    algoOutput.Width = outputBuffer.width;
    algoOutput.Height = outputBuffer.height;
    algoOutput.PixelArrayFormat = outputBuffer.format;
    algoOutput.Plane[0] = outputBuffer.plane[0];
    algoOutput.Plane[1] = outputBuffer.plane[1];
    algoOutput.Pitch[0] = outputBuffer.stride;
    algoOutput.Pitch[1] = outputBuffer.stride;
    algoOutput.Scanline[0] = outputBuffer.scanline;

    mDeblurParams.config.txt_detect = sTxtDetectProp;
    mDeblurParams.config.sceneType = mia_deblur::SCENE_DOC; // only doc scene
    mDeblurParams.config.orientation = GetOriention(logiMeta);

    mia_deblur::FaceInfo faceInfo = {0, NULL};
    getFaceInfo(phyMeta, &faceInfo, inputBuffer.width, inputBuffer.height);
    mia_deblur::setFaceInfoBeforeProcess(&faceInfo);

    // to make sure fastshot S2S time, switch to lite algo when concurrent running task >=3
    // resume to normal algo when concurrent<=2. (for now maxConcurrentNum = 5).
    if (pProcessRequestInfo->runningTaskNum >= pProcessRequestInfo->maxConcurrentNum - 2)
        mLiteEnable = 1;
    else if (pProcessRequestInfo->runningTaskNum <= sResNormalProcNum)
        mLiteEnable = 0;
    MLOGI(Mia2LogGroupPlugin, "Lite Check:liteDeblur=%d, runningTaskNum=%d, maxConcurrentNum=%d",
          mLiteEnable, pProcessRequestInfo->runningTaskNum, pProcessRequestInfo->maxConcurrentNum);
    mDeblurParams.config.disable_deblur = mLiteEnable ? 1 : 0;

    // If use asd to get document scene info, set up input para.
    if (mUseAsdMeta) {
        mDeblurParams.config.txt_detect = 0;
        // #define SCENE_FLAG_DOC 10002
        mDeblurParams.config.sceneflag = 10002;
    }

    MLOGD(Mia2LogGroupPlugin, "deblur_process start +++");
    double startTime = PluginUtils::nowMSec();
    int processStatus =
        mia_deblur::deblur_process((void *)&algoInput, (void *)&algoOutput, &mDeblurParams);
    MLOGD(Mia2LogGroupPlugin,
          "deblur_process done, cost %.2fms status= %d(-1:Not work, 0:failed, 1:success)",
          PluginUtils::nowMSec() - startTime, processStatus);

    if (sMiaideblurDump) {
        char outputDumpName[128];
        snprintf(outputDumpName, sizeof(outputDumpName), "miaideblur_output_%u_%dx%d_scenetype_%d",
                 pProcessRequestInfo->frameNum, outputBuffer.width, outputBuffer.height,
                 mDeblurParams.config.sceneType);
        PluginUtils::dumpToFile(outputDumpName, &outputBuffer);
    }

    if (mNodeInterface.pSetResultMetadata != NULL) {
        std::ostringstream exifStream;
        exifStream << "MiaiDeblur:{status:" << processStatus;
        if (processStatus == mia_deblur::WORK_SUCCESS)
            exifStream << " scene:" << mDeblurParams.config.sceneType;
        exifStream << " liteEnable:" << mLiteEnable << "}";
        std::string exif = exifStream.str();
        MLOGD(Mia2LogGroupPlugin, "update exif info:%s", exif.c_str());
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, pProcessRequestInfo->frameNum,
                                          pProcessRequestInfo->timeStamp, exif);
    }

    MLOGD(Mia2LogGroupPlugin, "ProcessRequest End #%u.", pProcessRequestInfo->frameNum);
    return resultInfo;
}

char MiAIDeblurPlugin::GetOriention(camera_metadata_t *metaData)
{
    void *pData = NULL;
    int32_t oriention = 0;
    VendorMetadataParser::getTagValue(metaData, ANDROID_JPEG_ORIENTATION, &pData);
    if (NULL != pData) {
        oriention = *static_cast<int32_t *>(pData);
    } else {
        MLOGW(Mia2LogGroupPlugin, "GetOriention failed!");
    }
    char orientionForAlgo = static_cast<char>((oriention % 360) / 90);
    MLOGW(Mia2LogGroupPlugin, "GetOriention %d -> %d", oriention, orientionForAlgo);

    return orientionForAlgo;
}

void MiAIDeblurPlugin::getFaceInfo(camera_metadata_t *metaData, mia_deblur::FaceInfo *faceInfo,
                                   int w, int h)
{
    MIRECT cropRegion;
    memset(&cropRegion, 0, sizeof(MIRECT));

    void *data = NULL;
    const char *cropInfo = "xiaomi.snapshot.cropRegion";
    VendorMetadataParser::getVTagValue(metaData, cropInfo, &data);
    if (NULL != data) {
        cropRegion = *reinterpret_cast<MIRECT *>(data);
        MLOGI(Mia2LogGroupPlugin, "get sat CropRegion begin [%d, %d, %d, %d]", cropRegion.left,
              cropRegion.top, cropRegion.width, cropRegion.height);
    } else if (NULL == data) {
        MLOGI(Mia2LogGroupPlugin, "get SAT crop failed, try ANDROID_SCALER_CROP_REGION");
        VendorMetadataParser::getTagValue(metaData, ANDROID_SCALER_CROP_REGION, &data);
        if (NULL != data) {
            cropRegion = *reinterpret_cast<MIRECT *>(data);
            MLOGI(Mia2LogGroupPlugin, "get CropRegion2 begin [%d, %d, %d, %d]", cropRegion.left,
                  cropRegion.top, cropRegion.width, cropRegion.height);
        } else {
            MLOGW(Mia2LogGroupPlugin, "get CropRegion failed");
            cropRegion.left = 0;
            cropRegion.top = 0;
            cropRegion.width = 0;
            cropRegion.height = 0;
        }
    }

    faceInfo->faceNum = 0;
    float zoomRatio = 1.0;
    data = NULL;

    VendorMetadataParser::getTagValue(metaData, ANDROID_STATISTICS_FACE_RECTANGLES, &data);
    if (NULL != data) {
        camera_metadata_entry_t entry = {0};
        if (0 == find_camera_metadata_entry(metaData, ANDROID_STATISTICS_FACE_RECTANGLES, &entry)) {
            uint32_t numElemRect = sizeof(MIRECT) / sizeof(uint32_t);
            faceInfo->faceNum = entry.count / numElemRect;
        }
        void *tmpData = reinterpret_cast<void *>(data);
        uint32_t dataIndex = 0;
        for (int index = 0; index < faceInfo->faceNum; index++) {
            int32_t xMin = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t yMin = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t xMax = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t yMax = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            MLOGI(Mia2LogGroupPlugin, "get ANDROID_STATISTICS_FACE_RECTANGLES [%d,%d,%d,%d]", xMin,
                  yMin, xMax, yMax);

            xMin = (xMin - static_cast<int32_t>(cropRegion.left)) * zoomRatio;
            xMax = (xMax - static_cast<int32_t>(cropRegion.left)) * zoomRatio;
            yMin = (yMin - static_cast<int32_t>(cropRegion.top)) * zoomRatio;
            yMax = (yMax - static_cast<int32_t>(cropRegion.top)) * zoomRatio;

            MLOGI(Mia2LogGroupPlugin, "face [xMin,yMin,xMax,yMax][%d,%d,%d,%d]", xMin, yMin, xMax,
                  yMax);

            // convert face rectangle to be based on current input dimension
            faceInfo->faceRectList[index].startX = xMin;
            faceInfo->faceRectList[index].startY = yMin;
            faceInfo->faceRectList[index].width = xMax - xMin + 1;
            faceInfo->faceRectList[index].height = yMax - yMin + 1;
        }
    } else {
        faceInfo->faceNum = 0;
        MLOGI(Mia2LogGroupPlugin, "faces are not found");
    }
}

ProcessRetStatus MiAIDeblurPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int MiAIDeblurPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

void MiAIDeblurPlugin::destroy()
{
    if (mFrameworkCameraId == CAM_COMBMODE_REAR_ULTRA) {
        MLOGD(Mia2LogGroupPlugin, "%s:%d %p onDestory, do nothing in 0.6x, mFrameworkCameraId:%d",
              __func__, __LINE__, this, mFrameworkCameraId);
    }
    if (sDeblurEnable != 0) {
        mia_deblur::deblur_release();
    }
    MLOGD(Mia2LogGroupPlugin, "%s:%d %p onDestory", __func__, __LINE__, this);
}
