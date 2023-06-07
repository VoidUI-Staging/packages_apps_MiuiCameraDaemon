#include "include/SkinBeautifierPlugin.h"

static const int32_t sIsAlgoEnabled =
    property_get_int32("persist.vendor.camera.ts.beauty.enabled", 1);

using namespace mialgo2;

static inline std::string property_get_string(const char *key, const char *defaultValue)
{
    char value[PROPERTY_VALUE_MAX];
    property_get(key, value, defaultValue);
    return std::string(value);
}

static const std::string sMiuiRegion = property_get_string("ro.miui.region", "0");

static long long __attribute__((unused)) gettime()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((long long)time.tv_sec * 1000 + time.tv_usec / 1000);
}

SkinBeautifierPlugin::~SkinBeautifierPlugin()
{
    if (1 == sIsAlgoEnabled) {
        if (mSkinBeautifier != NULL) {
            delete mSkinBeautifier;
            mSkinBeautifier = NULL;
        }
    }
}

int SkinBeautifierPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mIsFrontCamera = false;
    mSDKYUVMode = 0;
    mFrameworkCameraId = createInfo->frameworkCameraId;
    mInstanceName = createInfo->nodeInstance;
    mNodeInterface = nodeInterface;
    mSkinBeautifier = NULL;
    mProcessCount = 0;
    mOrientation = 0;
    mRegion = 0;
    mAppModule = 0;
    mBeautyEnabled = false;
    mInitFinish = 0;

    mWidth = createInfo->inputFormat.width;
    mHeight = createInfo->inputFormat.height;
    memset(&mFaceAnalyze, 0, sizeof(OutputMetadataFaceAnalyze));
    memset(&mFrameInfo, 0, sizeof(FrameInfo));

    if (1 == sIsAlgoEnabled) {
        if (mSkinBeautifier == NULL) {
            mSkinBeautifier = new SkinBeautifier();
        }
    }

    int beautyFeatureCounts = mBeautyFeatureParams.featureCounts;
    for (int i = 0; i < beautyFeatureCounts; i++) {
        mBeautyFeatureParams.beautyFeatureInfo[i].featureValue = 0;
    }

    // get region value
    // GLOBAL 0
    // CHINA 1
    // INDIAN 2
    const char *prop = sMiuiRegion.c_str();
    if (strcmp(prop, "GLOBAL") == 0) {
        mRegion = 0;
    } else if (strcmp(prop, "CN") == 0) {
        mRegion = 1;
    } else if (strcmp(prop, "IN") == 0) {
        mRegion = 2;
    } else {
        MLOGI_BF("Get Region Failed! Set Value as Global %d", mRegion);
    }
    mRegion = 1;
    MLOGD_BF("Get Region Success, the Value is %d", mRegion);

    switch (createInfo->operationMode) {
    case StreamConfigModeSAT:
        mAppModule = APP_MODULE_DEFAULT_CAPTURE;
        break;
    case StreamConfigModeMiuiZslFront:
        mAppModule = APP_MODULE_DEFAULT_FRONT_CAPTURE;
        break;
    case StreamConfigModeBokeh:
        mAppModule = APP_MODULE_PORTRAIT;
        break;
    case StreamConfigModeMiuiSuperNight:
        mAppModule = APP_MODULE_SUPER_NIGHT;
        break;
    default:
        break;
    }

    if (CAM_COMBMODE_FRONT == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_BOKEH == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_AUX == mFrameworkCameraId) {
        mIsFrontCamera = true;
    }

    MLOGD_BF("TrueSight capture operationMode is %d, appModule is %d, isfrontCamera is %d",
             createInfo->operationMode, mAppModule, mIsFrontCamera);

    // front init in initialize, default beauty open
    if (1 == sIsAlgoEnabled) {
        if (NULL != mSkinBeautifier && 1 == mIsFrontCamera) {
            int initresult = mSkinBeautifier->Init(mIsFrontCamera);
            if (1 == initresult) {
                MLOGE_BF("TrueSight capture Algorithm frontmode init fail");
                return 1;
            } else {
                mInitFinish = 1;
                MLOGD_BF("TrueSight capture Algorithm frontmode init success");
            }
        }
    }

    return 0;
}

bool SkinBeautifierPlugin::isEnabled(MiaParams settings)
{
    if (0 == sIsAlgoEnabled) {
        return false;
    }

    getBeautyParams(settings.metadata);
    getFrameInfo(settings.metadata);
    // rear uninit if close beauty switch
    if (!mIsFrontCamera && (false == mBeautyEnabled) && (NULL != mSkinBeautifier) &&
        (1 == mInitFinish)) {
        mSkinBeautifier->UnInit();
        mInitFinish = 0;
        MLOGD_BF("mSkinBeautifier unInit, mInitFinish = %d", mInitFinish);
    }
    // rear nofacenum, beauty disable
    if (!mIsFrontCamera && (APP_MODULE_DEFAULT_FRONT_CAPTURE != mAppModule) &&
        (0 == mFaceFeatureParams.miFaceCountNum) && (true == mBeautyEnabled)) {
        mBeautyEnabled = false;
    }
    MLOGD_BF("mBeautyEnabled:%d, mIsFrontCamera:%d, mFaceFeatureParams.miFaceCountNum:%d",
             mBeautyEnabled, mIsFrontCamera, mFaceFeatureParams.miFaceCountNum);
    return (mBeautyEnabled);
}

ProcessRetStatus SkinBeautifierPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;

    if (inputBuffers.size() > 0 && outputBuffers.size() > 0) {
        camera_metadata_t *metaData = inputBuffers[0].pMetadata;

        ImageParams inputImage = inputBuffers[0];
        ImageParams outputImage = outputBuffers[0];
        double startTime = gettime();
        resultInfo = processBuffer(&inputImage, &outputImage, metaData);

        // add Exif info
        updateExifData(processRequestInfo, resultInfo, gettime() - startTime);

        mProcessCount++;
    } else {
        resultInfo = PROCBADSTATE;
        MLOGE_BF("Error Invalid Param: Size of Input Buffer = %d, Size of Output Buffer = %d",
                 inputBuffers.size(), outputBuffers.size());
    }

    return resultInfo;
}

ProcessRetStatus SkinBeautifierPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo;
    resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus SkinBeautifierPlugin::processBuffer(struct ImageParams *input,
                                                     struct ImageParams *output,
                                                     camera_metadata_t *metaData)
{
    int32_t IsDumpData = property_get_int32("persist.vendor.camera.SkinBeautifier.dump", 0);

    if (0 == sIsAlgoEnabled) {
        struct MiImageBuffer inputFrame, outputFrame;
        imageParamsToMiBuffer(&inputFrame, input);
        imageParamsToMiBuffer(&outputFrame, output);

        PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        MLOGD_BF("mSkinBeautifier = %p, mBeautyEnabled = %d, Called MiCopyBuffer()",
                 mSkinBeautifier, mBeautyEnabled);
    }

    if (input == NULL || output == NULL || metaData == NULL || mSkinBeautifier == NULL) {
        MLOGE_BF(
            "Error Invalid Param: input = %p, output = %p, metaData = %p, mSkinBeautifier = %p",
            input, output, metaData, mSkinBeautifier);
        return PROCFAILED;
    }

    struct MiImageBuffer inputdumpFrame, outputdumpFrame;
    inputdumpFrame.format = input->format.format;
    inputdumpFrame.width = input->format.width;
    inputdumpFrame.height = input->format.height;
    inputdumpFrame.plane[0] = input->pAddr[0];
    inputdumpFrame.plane[1] = input->pAddr[1];
    inputdumpFrame.stride = input->planeStride;
    inputdumpFrame.scanline = input->sliceheight;
    if (inputdumpFrame.plane[0] == NULL || inputdumpFrame.plane[1] == NULL) {
        MLOGE(Mia2LogGroupPlugin, "Wrong inputdumpFrame");
    }

    outputdumpFrame.format = output->format.format;
    outputdumpFrame.width = output->format.width;
    outputdumpFrame.height = output->format.height;
    outputdumpFrame.plane[0] = output->pAddr[0];
    outputdumpFrame.plane[1] = output->pAddr[1];
    outputdumpFrame.stride = output->planeStride;
    outputdumpFrame.scanline = output->sliceheight;
    if (outputdumpFrame.plane[0] == NULL || outputdumpFrame.plane[1] == NULL) {
        MLOGE(Mia2LogGroupPlugin, "Wrong outputdumpFrame");
    }
    MLOGI(Mia2LogGroupPlugin,
          "inputdumpFrame width: %d, height: %d, stride: %d, scanline: %d, outputdumpFrame stride: "
          "%d scanline: %d",
          inputdumpFrame.width, inputdumpFrame.height, inputdumpFrame.stride,
          inputdumpFrame.scanline, outputdumpFrame.stride, outputdumpFrame.scanline);

    if (mIsFrontCamera && mBeautyEnabled == false) {
        getBeautyParams(metaData);
        MLOGD_BF("ProcessBuffer getbeautyparams again, mBeautyEnabled = %d", mBeautyEnabled);
    }

    if (mSkinBeautifier != NULL && (mBeautyEnabled == true)) {
        struct MiImageBuffer inputFrame, outputFrame;
        imageParamsToMiBuffer(&inputFrame, input);
        imageParamsToMiBuffer(&outputFrame, output);

        // rear init in process first frame, default beauty close
        if (0 == mIsFrontCamera && 1 != mInitFinish) {
            int initresult = mSkinBeautifier->Init(mIsFrontCamera);
            if (1 == initresult) {
                MLOGE_BF("TrueSight capture Algorithm rearmode init fail");
                return PROCFAILED;
            } else {
                mInitFinish = 1;
                MLOGD_BF("TrueSight capture Algorithm rearmode init success");
            }
            mBeautyFeatureParams.beautyFeatureMode.lastFeatureValue = -1;
        }

        if (1 == mInitFinish) {
            mSkinBeautifier->SetFeatureParams(mBeautyFeatureParams, mAppModule);
            mSkinBeautifier->SetFrameInfo(mFrameInfo);

            mSkinBeautifier->IsFullFeature(true);

            if (IsDumpData) {
                char inputbuf[128];
                snprintf(inputbuf, sizeof(inputbuf), "SkinBeautifier_input_%dx%d",
                         inputdumpFrame.width, inputdumpFrame.height);
                PluginUtils::dumpToFile(inputbuf, &inputdumpFrame);
            }

            mSkinBeautifier->SetImageOrientation(mOrientation);

            mSkinBeautifier->Process(&inputFrame, &outputFrame, mFaceFeatureParams, mSDKYUVMode);

            MLOGD_BF("mSkinBeautifier = %p, mBeautyEnabled = %d, Called SkinBeautifier::Process()",
                     mSkinBeautifier, mBeautyEnabled);

            if (IsDumpData) {
                char outputbuf[128];
                snprintf(outputbuf, sizeof(outputbuf), "SkinBeautifier_output_%dx%d",
                         outputdumpFrame.width, outputdumpFrame.height);
                PluginUtils::dumpToFile(outputbuf, &outputdumpFrame);
            }
        }
    } else {
        struct MiImageBuffer inputFrame, outputFrame;
        imageParamsToMiBuffer(&inputFrame, input);
        imageParamsToMiBuffer(&outputFrame, output);

        PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        MLOGD_BF("mSkinBeautifier = %p, mBeautyEnabled = %d, Called MiCopyBuffer()",
                 mSkinBeautifier, mBeautyEnabled);
    }

    return PROCSUCCESS;
}

void SkinBeautifierPlugin::updateExifData(ProcessRequestInfo *processRequestInfo,
                                          ProcessRetStatus resultInfo, double processTime)
{
    // exif data format is skinbeautifier::{processTime:...}
    std::string results(mInstanceName);
    results += ":{";

    char data[1024] = {0};
    snprintf(data, sizeof(data),
             "procRet:%d procTime:%d skinSmoothRatio:%d whitenRatio:%d stereoPerceptionRatio:%d"
             "eyeBrowDyeRatio:%d slimFaceRatio:%d headNarrowRatio:%d hairPuffyRatio:%d"
             "enlargeEyeRatio:%d noseRatio:%d noseTipRatio:%d templeRatio:%d cheekBoneRatio:%d"
             "jawRatio:%d chinRatio:%d lipsRatio:%d hairlineRatio:%d",
             resultInfo, (int)processTime, mBeautyFeatureParams.beautyFeatureInfo[0].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[1].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[2].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[3].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[4].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[5].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[6].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[7].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[8].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[9].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[10].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[11].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[12].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[13].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[14].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[15].featureValue);

    std::string exifData(data);
    results += exifData;
    results += "}";

    if (NULL != mNodeInterface.pSetResultMetadata) {
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, processRequestInfo->frameNum,
                                          processRequestInfo->timeStamp, results);
    }
}

void SkinBeautifierPlugin::getBeautyParams(camera_metadata_t *metadata)
{
    mBeautyEnabled = false;
    if (mSkinBeautifier) {
        mSkinBeautifier->SetBeautyDimensionEnable(false);
        if (mIsFrontCamera) {
            mSkinBeautifier->SetCameraPosition(true);
        }
    }

    void *data = NULL;
    const char *faceAnalyzeResultTag = "xiaomi.faceAnalyzeResult.result";
    VendorMetadataParser::getVTagValue(metadata, faceAnalyzeResultTag, &data);
    if (NULL != data) {
        memcpy(&mFaceAnalyze, data, sizeof(OutputMetadataFaceAnalyze));
        MLOGD_BF("GetMetaData faceNum %d", mFaceAnalyze.faceNum);
    } else {
        MLOGD_BF("Get MetaData xiaomi.faceAnalyzeResult.result Failed!");
    }

    // add thirdparty sdk yuvtag
    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, "com.xiaomi.sessionparams.thirdPartyYUVSnapshot",
                                       &data);
    if (NULL != data) {
        mSDKYUVMode = *static_cast<int *>(data);
        MLOGD_BF("GetMetaData sdk thirdPartyYUVMode = %d", mSDKYUVMode);
    } else {
        mSDKYUVMode = 0;
        MLOGD_BF("GetMetaData [com.xiaomi.sessionparams.thirdPartyYUVSnapshot] Failed!");
    }

    data = NULL;
    const char *deviceTag = "xiaomi.device.orientation";
    VendorMetadataParser::getVTagValue(metadata, deviceTag, &data);
    if (NULL != data) {
        mOrientation = *static_cast<int *>(data);
        MLOGD_BF("GetMetaData [xiaomi.device.orientation] orientation = %d", mOrientation);
    } else {
        VendorMetadataParser::getTagValue(metadata, ANDROID_JPEG_ORIENTATION, &data);
        mOrientation = *static_cast<int *>(data);
        mOrientation = mOrientation % 360;
        MLOGD_BF("GetMetaData [ANDROID_JPEG_ORIENTATION] orientation = %d", mOrientation);
    }

    // 经典、原生 for test
    // int beautyMode = 0;
    // beautyMode = property_get_int32("persist.vendor.beauty.type", 0);
    // mBeautyFeatureParams.beautyFeatureMode.featureValue = beautyMode;
    // 经典、原生 for test

    // 经典/原生
    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, "xiaomi.beauty.beautyMode", &data);
    if (NULL != data) {
        mBeautyFeatureParams.beautyFeatureMode.featureValue = *static_cast<int *>(data);
        MLOGD_BF("GetMetaData [xiaomi.beauty.beautyMode] beauty type = %d",
                 mBeautyFeatureParams.beautyFeatureMode.featureValue);
    } else {
        mBeautyFeatureParams.beautyFeatureMode.featureValue = 0;
        MLOGW_BF("GetMetaData [xiaomi.beauty.beautyMode] Failed!");
    }

    MLOGD_BF("GetMetaData [xiaomi.beauty.beautyMode] beauty type = %d",
             mBeautyFeatureParams.beautyFeatureMode.featureValue);

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

            data = NULL;
            VendorMetadataParser::getVTagValue(metadata, tagStr, &data);
            if (data != NULL) {
                mBeautyFeatureParams.beautyFeatureInfo[i].featureValue = *static_cast<int *>(data);
                MLOGD_BF("Get Metadata Value TagName: %s, Value: %d", tagStr,
                         mBeautyFeatureParams.beautyFeatureInfo[i].featureValue);

                if (mBeautyFeatureParams.beautyFeatureInfo[i].featureValue !=
                    0) // feature vlude support -100 -- + 100, 0 means no effect
                {
                    if (mSkinBeautifier) {
                        mSkinBeautifier->SetBeautyDimensionEnable(true);
                    }
                    mBeautyEnabled = true;
                }
            } else {
                MLOGI_BF("Get Metadata Failed! TagName: %s", tagStr);
            }
        }
    }

    // 去痣和男性妆容
    int beautyRelatedSwitchesCounts = mBeautyFeatureParams.beautyRelatedSwitchesCounts;
    for (int i = 0; i < beautyRelatedSwitchesCounts; i++) {
        if (mBeautyFeatureParams.beautyRelatedSwitchesInfo[i].featureMask == true) {
            memset(tagStr, 0, sizeof(tagStr));
            memcpy(tagStr, componentName, sizeof(componentName));
            memcpy(tagStr + sizeof(componentName) - 1,
                   mBeautyFeatureParams.beautyRelatedSwitchesInfo[i].tagName,
                   strlen(mBeautyFeatureParams.beautyRelatedSwitchesInfo[i].tagName) + 1);

            data = NULL;
            VendorMetadataParser::getVTagValue(metadata, tagStr, &data);
            if (data != NULL) {
                mBeautyFeatureParams.beautyRelatedSwitchesInfo[i].featureValue =
                    *static_cast<int *>(data);
                MLOGD_BF("Get Metadata Value TagName: %s, Value: %d", tagStr,
                         mBeautyFeatureParams.beautyRelatedSwitchesInfo[i].featureValue);
            }
        }
    }
    // Man makeup switch swap
    if (1 == mBeautyFeatureParams.beautyRelatedSwitchesInfo[0].featureValue) {
        mBeautyFeatureParams.beautyRelatedSwitchesInfo[0].featureValue = 0;
    } else if (0 == mBeautyFeatureParams.beautyRelatedSwitchesInfo[0].featureValue) {
        mBeautyFeatureParams.beautyRelatedSwitchesInfo[0].featureValue = 1;
    }

    if (true == mBeautyEnabled) {
        data = NULL;
        MiFDBeautyParam *pMiFDBeautyParams = NULL;
        mFaceFeatureParams = {0};
        const char *fdBeautyResultTag = "xiaomi.fd.mifdbeautyparam";
        VendorMetadataParser::getVTagValue(metadata, fdBeautyResultTag, &data);

        if (NULL != data) {
            pMiFDBeautyParams = static_cast<MiFDBeautyParam *>(data);
            memcpy(&mFaceFeatureParams, pMiFDBeautyParams, sizeof(MiFDBeautyParam));
        } else {
            MLOGI_BF("Get Metadata Failed! TagName: xiaomi.fd.mifdbeautyparam");
        }
        if (true == mIsFrontCamera) {
            // Translate face roi by zoom ratio
            for (int i = 0; i < mFaceFeatureParams.miFaceCountNum; i++) {
                faceroi faceRect = {0};
                faceRect.left = mFaceFeatureParams.face_roi[i].left;
                faceRect.top = mFaceFeatureParams.face_roi[i].top;
                faceRect.width = mFaceFeatureParams.face_roi[i].width;
                faceRect.height = mFaceFeatureParams.face_roi[i].height;

                MLOGD_BF("Get Metadata before translate faceroi l %f, t %f, w %f, h %f",
                         faceRect.left, faceRect.top, faceRect.width, faceRect.height);

                float verticalZoomRatio =
                    (float)mWidth / (float)mFaceFeatureParams.refdimension.width;
                float horizontalZoomRatio =
                    (float)mHeight / (float)mFaceFeatureParams.refdimension.height;

                MLOGD_BF("Get Metadata before translate faceroi width %d height %d",
                         mFaceFeatureParams.refdimension.width,
                         mFaceFeatureParams.refdimension.height);

                faceRect.left *= verticalZoomRatio;
                faceRect.top *= horizontalZoomRatio;
                faceRect.width *= verticalZoomRatio;
                faceRect.height *= horizontalZoomRatio;

                MLOGD_BF("Get Metadata after translate faceroi l %f, t %f, w %f, h %f",
                         faceRect.left, faceRect.top, faceRect.width, faceRect.height);

                mFaceFeatureParams.face_roi[i].left = faceRect.left;
                mFaceFeatureParams.face_roi[i].top = faceRect.top;
                mFaceFeatureParams.face_roi[i].width = faceRect.width;
                mFaceFeatureParams.face_roi[i].height = faceRect.height;
            }
        }
    }
    MLOGD_BF("Beauty Enabled = %d", mBeautyEnabled);
}

void SkinBeautifierPlugin::getFrameInfo(camera_metadata_t *metadata)
{
    // get iso value
    int sensitivity = 100;
    float ispGain = 1.0;
    void *pData = NULL;

    VendorMetadataParser::getTagValue(metadata, ANDROID_SENSOR_SENSITIVITY, &pData);
    if (NULL != pData) {
        sensitivity = *static_cast<int32_t *>(pData);
    } else {
        MLOGD_BF("GetMetaData [ANDROID_SENSOR_SENSITIVITY] failed!");
    }

    pData = NULL;
    VendorMetadataParser::getVTagValue(metadata, "com.qti.sensorbps.gain", &pData);
    if (NULL != pData) {
        ispGain = *static_cast<float *>(pData);
    } else {
        MLOGD_BF("GetMetaData [com.qti.sensorbps.gain] failed!");
    }

    mFrameInfo.iso = (int)(ispGain * 100) * sensitivity / 100;

    // get AE info
    pData = NULL;
    AECFrameControl *pAECFrameControl = NULL;
    VendorMetadataParser::getVTagValue(metadata, "org.quic.camera2.statsconfigs.AECFrameControl",
                                       &pData);
    if (NULL != pData) {
        pAECFrameControl = static_cast<AECFrameControl *>(pData);

        mFrameInfo.nLuxIndex = pAECFrameControl->luxIndex;
        mFrameInfo.fExposure =
            (float)pAECFrameControl->exposureInfo[ExposureIndexSafe].exposureTime;
        mFrameInfo.fAdrc = pAECFrameControl->exposureInfo[ExposureIndexSafe].sensitivity /
                           pAECFrameControl->exposureInfo[ExposureIndexShort].sensitivity;
    } else {
        MLOGE_BF("GetMetaData [org.quic.camera2.statsconfigs.AECFrameControl] Failed!");
    }

    // get AWB info
    pData = NULL;
    AWBFrameControl *pAWBFrameControl = NULL;
    VendorMetadataParser::getVTagValue(metadata, "org.quic.camera2.statsconfigs.AWBFrameControl",
                                       &pData);

    if (NULL != pData) {
        pAWBFrameControl = static_cast<AWBFrameControl *>(pData);

        mFrameInfo.nColorTemperature = pAWBFrameControl->colorTemperature;
    } else {
        MLOGE_BF("GetMetaData [org.quic.camera2.statsconfigs.AWBFrameControl] Failed!");
    }

    // get flash on or not

    if (true == mIsFrontCamera) { // front
        pData = NULL;
        int miFlashMode = 0;
        VendorMetadataParser::getVTagValue(metadata, "xiaomi.flash.mode", &pData);
        if (NULL != pData) {
            miFlashMode = *(static_cast<int32_t *>(pData));
        } else {
            MLOGE_BF("GetMetaData [xiaomi.flash.mode] Failed!");
        }
        mFrameInfo.nFlashTag =
            miFlashMode; // 0: off; 2: flash on; 104: screenlight on; 106: screenlight & flash on
    } else {             // rear
        pData = NULL;
        int androidFlashMode = 0;
        VendorMetadataParser::getTagValue(metadata, ANDROID_FLASH_MODE, &pData);
        if (NULL != pData) {
            androidFlashMode = *(static_cast<int32_t *>(pData));
        } else {
            MLOGE_BF("GetMetaData [ANDROID_FLASH_MODE] Failed!");
        }

        pData = NULL;
        int miFlashMode = 0;
        VendorMetadataParser::getVTagValue(metadata, "xiaomi.flash.mode", &pData);
        if (NULL != pData) {
            miFlashMode = *(static_cast<int32_t *>(pData));
        } else {
            MLOGE_BF("GetMetaData [xiaomi.flash.mode] Failed!");
        }

        if (0 == miFlashMode) {
            mFrameInfo.nFlashTag = 0; // off
        } else if ((3 == miFlashMode || 1 == miFlashMode) && 2 == androidFlashMode) {
            mFrameInfo.nFlashTag = 1; // auto/on flash on
        } else if (2 == miFlashMode && 2 == androidFlashMode) {
            mFrameInfo.nFlashTag = 2; // torch on
        } else {
            mFrameInfo.nFlashTag = 0; // off
        }
    }

    MLOGD_BF(
        "Frame info: [iso:%d] [luxIndex:%d] [flashTag:%d] [expTime:%f] [adrc:%f] [colorTemp:%d]",
        mFrameInfo.iso, mFrameInfo.nLuxIndex, mFrameInfo.nFlashTag, mFrameInfo.fExposure,
        mFrameInfo.fAdrc, mFrameInfo.nColorTemperature);
}

int SkinBeautifierPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

void SkinBeautifierPlugin::destroy()
{
    if (1 == sIsAlgoEnabled) {
        if (mSkinBeautifier != NULL) {
            delete mSkinBeautifier;
            mSkinBeautifier = NULL;
        }
    }
}

/* void SkinBeautifierPlugin::imageParamsToOffScreen(ASVLOFFSCREEN *offScreen,
                                                  struct ImageParams *imageParams)
{
    offScreen->u32PixelArrayFormat = ASVL_PAF_NV12;
    offScreen->i32Width = imageParams->format.width;
    offScreen->i32Height = imageParams->format.height;
    for (int i = 0; i < imageParams->numberOfPlanes && i < 3; i++) {
        offScreen->pi32Pitch[i] = imageParams->planeStride;
        offScreen->ppu8Plane[i] = imageParams->pAddr[i];
    }
} */
void SkinBeautifierPlugin::imageParamsToMiBuffer(struct MiImageBuffer *miBuffer,
                                                 struct ImageParams *imageParams)
{
    miBuffer->format = imageParams->format.format;
    miBuffer->width = imageParams->format.width;
    miBuffer->height = imageParams->format.height;
    miBuffer->stride = imageParams->planeStride;
    miBuffer->scanline = imageParams->sliceheight;
    miBuffer->numberOfPlanes = imageParams->numberOfPlanes;
    for (int i = 0; i < imageParams->numberOfPlanes; i++) {
        miBuffer->plane[i] = imageParams->pAddr[i];
    }
}
