#include "SkinBeautifierPlugin.h"

#undef LOG_TAG
#define LOG_TAG "SkinBeautifierPlugin"

#if defined(ZIYI_CAM)
#define whitenAjustRatio 4
#endif

#define MY_LOGI(fmt, arg...) ALOGI("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...) ALOGD("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...) ALOGW("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...) ALOGE("[%s] " fmt, __FUNCTION__, ##arg)
#define MY_LOGD_IF(cond, ...)     \
    do {                          \
        if (cond) {               \
            MY_LOGD(__VA_ARGS__); \
        }                         \
    } while (0)
#define MY_LOGI_IF(cond, ...)     \
    do {                          \
        if (cond) {               \
            MY_LOGI(__VA_ARGS__); \
        }                         \
    } while (0)
#define MY_LOGI_IF(cond, ...)     \
    do {                          \
        if (cond) {               \
            MY_LOGE(__VA_ARGS__); \
        }                         \
    } while (0)
#if 0
static inline std::string property_get_string(const char *key, const char *defaultValue)
{
    char value[PROPERTY_VALUE_MAX];
    property_get(key, value, defaultValue);
    return std::string(value);
}
#endif

static long long __attribute__((unused)) gettime()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((long long)time.tv_sec * 1000 + time.tv_usec / 1000);
}

SkinBeautifierPlugin::SkinBeautifierPlugin()
{
    MY_LOGI_IF(m_LogEnable, "enter");
    mHeight = 0;
    mRegion = 0;
    mWidth = 0;
    m_local_operation_mode = StreamConfigModeSAT;
    sWhitenRatioWithFace =
        property_get_int32("persist.vendor.camera.algoengine.beauty.whitenratio", -1);
    sIsAlgoEnabled = property_get_int32("persist.beauty.cap.enabled", 1);
    m_isDumpData = property_get_int32("persist.vendor.camera.SkinBeautifier.dump", 0);

#if 0
     sModuleInfo = 
      property_get_string("persist.vendor.camera.mi.module.info", "module");
#endif
    m_debugFaceDrawGet = property_get_bool("persist.beauty.cap.face.draw.get", 0);
    m_LogEnable = property_get_bool("persist.beauty.cap.log.enable", 1);
    MY_LOGI_IF(m_LogEnable, "finish");
}

SkinBeautifierPlugin::~SkinBeautifierPlugin() {}

int SkinBeautifierPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    if (false == sIsAlgoEnabled) {
        return 0;
    }
    MY_LOGI_IF(m_LogEnable, "enter.");
    int ret = -1;
    mIsFrontCamera = false;
    mBeautyMode = BEAUTY_MODE_CAPTURE;
    mFrameworkCameraId = pCreateInfo->frameworkCameraId;
    m_local_operation_mode = pCreateInfo->operationMode;
    mSkinBeautifier = NULL;
    mProcessCount = 0;
    mOrientation = 0;
    mRegion = 0;
    mAppModule = 0;
    mBeautyEnabled = false;
    mDoFaceWhiten = false;
    mFaceWhitenRatio = 0;
    mWidth = pCreateInfo->inputFormat.width;
    mHeight = pCreateInfo->inputFormat.height;
    memset(&mFaceAnalyze, 0, sizeof(OutputMetadataFaceAnalyze));
    memset(&mFrameInfo, 0, sizeof(CFrameInfo));

    if (mSkinBeautifier == NULL) {
        mSkinBeautifier = new SkinBeautifier();
    }

    int beautyFeatureCounts = mBeautyFeatureParams.featureCounts;
    for (int i = 0; i < beautyFeatureCounts; i++) {
        mBeautyFeatureParams.beautyFeatureInfo[i].featureValue = 0;
    }

    // get region value
    // GLOBAL 0
    // CHINA 1
    // INDIAN 2
    char prop[PROPERTY_VALUE_MAX];
    property_get("ro.miui.region", prop, "0");
    if (strcmp(prop, "GLOBAL") == 0) {
        mRegion = 0;
    } else if (strcmp(prop, "CN") == 0) {
        mRegion = 1;
    } else if (strcmp(prop, "IN") == 0) {
        mRegion = 2;
    } else {
        MY_LOGE("Get Region Failed! Set Value as Global %d", mRegion);
    }

    MY_LOGI("Get Region Success, the Value is %d", mRegion);

    switch (pCreateInfo->operationMode) {
    case StreamConfigModeSAT:
    case StreamConfigModeMiuiZslFront:
        mAppModule = APP_MODULE_DEFAULT_CAPTURE;
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
    // consider
    if (CAM_COMBMODE_FRONT == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_BOKEH == mFrameworkCameraId ||
        CAM_COMBMODE_FRONT_AUX == mFrameworkCameraId) {
        mIsFrontCamera = true;
    }
    // consider
    // mNodeInterface = nodeInterface;

    mSkinBeautifier->SetBeautyDimensionEnable(true);
    MY_LOGI("CameraId = %d CAM_COMBMODE_FRONT = %d", mFrameworkCameraId, CAM_COMBMODE_FRONT);
    mSkinBeautifier->SetCameraPosition(mIsFrontCamera);

    ret = mSkinBeautifier->Initialize(mIsFrontCamera, mRegion);
    if (ret) {
        MY_LOGE("mSkinBeautifier Initialize fail");
    }

    MY_LOGI_IF(m_LogEnable, "finish");

    return 0;
}

bool SkinBeautifierPlugin::isEnabled(MiaParams settings)
{
    camera_metadata_t *pMetadata = settings.metadata;
    bool beautyEnabled = false;
    if (false == sIsAlgoEnabled) {
        return beautyEnabled;
    }
    MY_LOGI_IF(m_LogEnable, "enter");

    memset(&mFaceLuma, 0, sizeof(mFaceLuma));
    beautyEnabled = getBeautyParams(pMetadata);
    getFrameInfo(pMetadata);
// MY_LOGI("faceROISize: %f", mFaceLuma.faceROISize);
// consider
#if 0
    if (!mIsFrontCamera && (0 == mFaceFeatureParams.miFaceCountNum) && mBeautyEnabled &&
        (CAM_COMBMODE_REAR_MACRO != mFrameworkCameraId)) {
    //consider
    //if (!mIsFrontCamera && (0.0f == mFaceLuma.faceROISize) && (true == mBeautyEnabled) &&
    //    (CAM_COMBMODE_REAR_SAT_WT == mFrameworkCameraId)) {
        mBeautyEnabled = false;
    }
#endif

    MY_LOGI("beautyEnabled: %d, mDoFaceWhiten:%d, facecount:%d", beautyEnabled, mDoFaceWhiten,
            mFaceFeatureParams.miFaceCountNum);

    MY_LOGI_IF(m_LogEnable, "finish");
    return (beautyEnabled || mDoFaceWhiten);
}

ProcessRetStatus SkinBeautifierPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    MY_LOGI_IF(m_LogEnable, "enter");

    ProcessRetStatus resultInfo = PROCSUCCESS;
    if (false == sIsAlgoEnabled) {
        ProcessRetStatus resultInfo = PROCFAILED;
        return resultInfo;
    }

    if (processRequestInfo->inputBuffersMap.size() > 0 &&
        processRequestInfo->outputBuffersMap.size() > 0) {
        auto &phInputBuffer = processRequestInfo->inputBuffersMap.at(0);
        auto &phOutputBuffer = processRequestInfo->outputBuffersMap.at(0);
        // bool skinBeautyEnable = false;
        // camera_metadata_t *pMetaData = (camera_metadata_t *)phInputBuffer[0].pMetadata;
        void *metaData = (camera_metadata_t *)phInputBuffer[0].pMetadata;

        double startTime = gettime();
        // consider
#if 0
        if (mIsFrontCamera == true) {
            // If face roi didn't be adjusted in FaceAlign node, then Beauty node need do it
            // Convert face roi only, because Beauty node do skinsmooth in rear camera without
            // FaceAlign node
            //updateFaceInfo(processRequestInfo, &mFaceFeatureParams);
            getFaceInfo(pProcessRequestInfo);
        }
#else
        getFaceInfo(processRequestInfo);

#endif
        ImageParams inputImage = phInputBuffer[0];
        ImageParams outputImage = phOutputBuffer[0];

        resultInfo = ProcessBuffer(&inputImage, &outputImage, metaData);
        mProcessCount++;

        // add Exif info
        // consider
        updateExifData(processRequestInfo, resultInfo, gettime() - startTime);

    } else {
        resultInfo = PROCBADSTATE;
        MY_LOGI_IF(m_LogEnable,
                   "Error Invalid Param:: Size of Input Buffer = %lu, Size of Output Buffer = %lu",
                   processRequestInfo->inputBuffersMap.size(),
                   processRequestInfo->outputBuffersMap.size());
    }
    MY_LOGI_IF(m_LogEnable, "finish");
    return resultInfo;
}

ProcessRetStatus SkinBeautifierPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus SkinBeautifierPlugin::ProcessBuffer(struct ImageParams *input,
                                                     struct ImageParams *output, void *metaData)
{
    MY_LOGI_IF(m_LogEnable, "enter");

    if (input == NULL || output == NULL || metaData == NULL || mSkinBeautifier == NULL) {
        MY_LOGE("Error Invalid Param: input = %p, output = %p, metaData = %p, mSkinBeautifier = %p",
                input, output, metaData, mSkinBeautifier);
        return PROCFAILED;
    }

    if (NULL == mSkinBeautifier) {
        struct MiImageBuffer inputFrame, outputFrame;
        imageParamsToMiBuffer(&inputFrame, input);
        imageParamsToMiBuffer(&outputFrame, output);

        PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        MY_LOGI_IF(m_LogEnable, "mSkinBeautifier = %p, mBeautyEnabled = %d, Called MiCopyBuffer()",
                   mSkinBeautifier, mBeautyEnabled);
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
        MY_LOGI_IF(m_LogEnable, "Wrong inputdumpFrame");
    }

    outputdumpFrame.format = output->format.format;
    outputdumpFrame.width = output->format.width;
    outputdumpFrame.height = output->format.height;
    outputdumpFrame.plane[0] = output->pAddr[0];
    outputdumpFrame.plane[1] = output->pAddr[1];
    outputdumpFrame.stride = output->planeStride;
    outputdumpFrame.scanline = output->sliceheight;
    if (outputdumpFrame.plane[0] == NULL || outputdumpFrame.plane[1] == NULL) {
        MY_LOGE("Wrong outputdumpFrame");
    }
    MY_LOGI_IF(m_LogEnable, "inputdumpFrame in format = %d out format = %d", input->format.format,
               output->format.format);
    MY_LOGI_IF(m_LogEnable,
               "inputdumpFrame width: %d, height: %d, stride: %d, scanline: %d, outputdumpFrame "
               "stride: "
               "%d scanline: %d",
               inputdumpFrame.width, inputdumpFrame.height, inputdumpFrame.stride,
               inputdumpFrame.scanline, outputdumpFrame.stride, outputdumpFrame.scanline);
    MY_LOGI_IF(m_LogEnable, "inputdumpFrame mBeautyEnabled = %d mDoFaceWhiten = %d", mBeautyEnabled,
               mDoFaceWhiten);
    if (mBeautyEnabled == true || mDoFaceWhiten == true) {
        struct MiImageBuffer inputFrame, outputFrame;
        imageParamsToMiBuffer(&inputFrame, input);
        imageParamsToMiBuffer(&outputFrame, output);
        if (mDoFaceWhiten == true) {
            if (sWhitenRatioWithFace != -1) {
                mFaceWhitenRatio = sWhitenRatioWithFace; // debug use
            }
            MY_LOGI_IF(m_LogEnable, "Beauty disable, whitenRatio: %d", mFaceWhitenRatio);
            mBeautyFeatureParams.beautyFeatureInfo[1].featureValue = mFaceWhitenRatio;
        }

        mSkinBeautifier->SetFeatureParams(mBeautyFeatureParams, mAppModule);
        mSkinBeautifier->SetFrameInfo(mFrameInfo);
        // no used
        mSkinBeautifier->IsFullFeature(true);

        if (m_isDumpData) {
            char inputbuf[128];
            snprintf(inputbuf, sizeof(inputbuf), "SkinBeautifier_input_%dx%d", inputdumpFrame.width,
                     inputdumpFrame.height);
            PluginUtils::dumpToFile(inputbuf, &inputdumpFrame);
        }

        mSkinBeautifier->SetImageOrientation(mOrientation);
        if (m_debugFaceDrawGet == true) {
            drawFaceBox((char *)(input->pAddr[0]), mFaceFeatureParams, input->format.width,
                        input->format.height, input->planeStride, true, 0x0);
        }
        mSkinBeautifier->Process(&inputFrame, &outputFrame, mFaceFeatureParams, mBeautyMode);
        if (m_debugFaceDrawGet == true) {
            drawFaceBox((char *)(output->pAddr[0]), mFaceFeatureParams, input->format.width,
                        input->format.height, input->planeStride, false, 0xff);
        }
        MY_LOGI_IF(m_LogEnable, "mSkinBeautifier = %p, mBeautyEnabled = %d", mSkinBeautifier,
                   mBeautyEnabled);

        if (m_isDumpData) {
            char outputbuf[128];
            snprintf(outputbuf, sizeof(outputbuf), "SkinBeautifier_output_%dx%d",
                     outputdumpFrame.width, outputdumpFrame.height);
            PluginUtils::dumpToFile(outputbuf, &outputdumpFrame);
        }

    } else {
        struct MiImageBuffer inputFrame, outputFrame;
        imageParamsToMiBuffer(&inputFrame, input);
        imageParamsToMiBuffer(&outputFrame, output);

        PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        MY_LOGI_IF(m_LogEnable, "mSkinBeautifier = %p, mBeautyEnabled = %d, Called MiCopyBuffer()",
                   mSkinBeautifier, mBeautyEnabled);
    }
    MY_LOGI_IF(m_LogEnable, "finish");
    return PROCSUCCESS;
}

bool SkinBeautifierPlugin::getBeautyParams(camera_metadata_t *pMetaData)
{
    bool beautyEnabled = false;
    MY_LOGI_IF(m_LogEnable, "enter");
    mBeautyEnabled = false;

    if (mSkinBeautifier) {
        mSkinBeautifier->SetBeautyDimensionEnable(false);
    }

    // consider
    void *data = NULL;
    const char *faceAnalyzeResultTag = "xiaomi.faceAnalyzeResult.result";
    VendorMetadataParser::getVTagValue(pMetaData, faceAnalyzeResultTag, &data);
    if (NULL != data) {
        memcpy(&mFaceAnalyze, data, sizeof(OutputMetadataFaceAnalyze));
        MY_LOGI_IF(m_LogEnable, "GetMetaData faceNum %d", mFaceAnalyze.faceNum);
    } else {
        MY_LOGE("Get MetaData xiaomi.faceAnalyzeResult.result Failed!");
    }
    // consider
    data = NULL;
    const char *deviceTag = "xiaomi.device.orientation";
    VendorMetadataParser::getVTagValue(pMetaData, deviceTag, &data);
    if (NULL != data) {
        mOrientation = *static_cast<int *>(data);
        MY_LOGI_IF(m_LogEnable, "GetMetaData [xiaomi.device.orientation] orientation = %d",
                   mOrientation);
    } else {
        VendorMetadataParser::getTagValue(pMetaData, ANDROID_JPEG_ORIENTATION, &data);
        mOrientation = *static_cast<int *>(data);
        mOrientation = mOrientation % 360;
        MY_LOGI_IF(m_LogEnable, "GetMetaData [ANDROID_JPEG_ORIENTATION] orientation = %d",
                   mOrientation);
    }

    // 经典、原生 for test
    // int beautyMode = 0;
    // beautyMode = property_get_int32("persist.vendor.beauty.type", 0);
    // mBeautyFeatureParams.beautyFeatureMode.featureValue = beautyMode;
    // 经典、原生 for test
    // 经典/原生
    data = NULL;
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.beauty.beautyMode", &data);
    if (NULL != data) {
        mBeautyFeatureParams.beautyFeatureMode.featureValue = *static_cast<int *>(data);
        MY_LOGI_IF(m_LogEnable, "GetMetaData [xiaomi.beauty.beautyMode] beauty type = %d",
                   mBeautyFeatureParams.beautyFeatureMode.featureValue);
    } else {
        mBeautyFeatureParams.beautyFeatureMode.featureValue = 0;
        MY_LOGE("GetMetaData [xiaomi.beauty.beautyMode] Failed!");
    }
    // consider
    if (APP_MODULE_PORTRAIT == mAppModule && true == mIsFrontCamera) {
        mBeautyFeatureParams.beautyFeatureMode.featureValue = 1;
    }
    MY_LOGI_IF(m_LogEnable, "beauty type = %d",
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
            VendorMetadataParser::getVTagValue(pMetaData, tagStr, &data);
            if (data != NULL) {
                mBeautyFeatureParams.beautyFeatureInfo[i].featureValue = *static_cast<int *>(data);
                MY_LOGI_IF(m_LogEnable, "TagName: %s, Value: %d", tagStr,
                           mBeautyFeatureParams.beautyFeatureInfo[i].featureValue);

                if (mBeautyFeatureParams.beautyFeatureInfo[i].featureValue !=
                    0) // feature vlude support -100 -- + 100, 0 means no effect
                {
                    // consider
                    mSkinBeautifier->SetBeautyDimensionEnable(true);
                    mBeautyEnabled = true;
                    beautyEnabled = true;
                }
            } else {
                MY_LOGE("Get Metadata Failed! TagName: %s", tagStr);
            }
        }
    }

#if defined(ZIYI_CAM)
    // 获取妆容模式
    data = NULL;
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.beauty.beautyStyle", &data);
    if (NULL != data) {
        mBeautyFeatureParams.beautyMakeupMode.featureValue = *static_cast<int *>(data);
        MY_LOGI_IF(m_LogEnable, "GetMetaData [xiaomi.beauty.beautyStyle] makeup mode = %d",
                   mBeautyFeatureParams.beautyMakeupMode.featureValue);
        if (mBeautyFeatureParams.beautyMakeupMode.featureValue != 0) {
            mBeautyEnabled = true;
            beautyEnabled = true;
        }
    } else {
        mBeautyFeatureParams.beautyMakeupMode.featureValue = 0;
        MY_LOGE("GetMetaData [xiaomi.beauty.beautyStyle] Failed!");
    }

    // 获取妆容参数
    int beautyMakeupCounts = mBeautyFeatureParams.beautyMakeupCounts;
    for (int i = 0; i < beautyMakeupCounts; i++) {
        if (mBeautyFeatureParams.beautyMakeupInfo[i].featureMask == true) {
            memset(tagStr, 0, sizeof(tagStr));
            memcpy(tagStr, componentName, sizeof(componentName));
            memcpy(tagStr + sizeof(componentName) - 1,
                   mBeautyFeatureParams.beautyMakeupInfo[i].tagName,
                   strlen(mBeautyFeatureParams.beautyMakeupInfo[i].tagName) + 1);

            data = NULL;
            VendorMetadataParser::getVTagValue(pMetaData, tagStr, &data);
            if (data != NULL) {
                mBeautyFeatureParams.beautyMakeupInfo[i].featureValue = *static_cast<int *>(data);
                MY_LOGI_IF(m_LogEnable, "TagName: %s, Value: %d", tagStr,
                           mBeautyFeatureParams.beautyMakeupInfo[i].featureValue);

                // if (mBeautyFeatureParams.beautyMakeupInfo[i].featureValue != 0) // feature vlude
                // support -100 -- + 100, 0 means no effect
                //{
                //    mBeautyEnabled = true;
                //}
            } else {
                MY_LOGE("Get Metadata Failed! TagName: %s", tagStr);
            }
        }
    }

    // 获取light
    data = NULL;
    VendorMetadataParser::getVTagValue(pMetaData, "xiaomi.beauty.ambientLightingType", &data);
    if (NULL != data) {
        mBeautyFeatureParams.beautyLightMode.featureValue = *static_cast<int *>(data);
        if (mBeautyFeatureParams.beautyLightMode.featureValue != 0) {
            mBeautyEnabled = true;
            beautyEnabled = true;
        }
        MY_LOGI_IF(m_LogEnable, "GetMetaData [xiaomi.beauty.ambientLightingType] light mode = %d",
                   mBeautyFeatureParams.beautyLightMode.featureValue);
    } else {
        mBeautyFeatureParams.beautyLightMode.featureValue = 0;
        MY_LOGE("GetMetaData [xiaomi.beauty.ambientLightingType] Failed!");
    }
#endif

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
            VendorMetadataParser::getVTagValue(pMetaData, tagStr, &data);
            if (data != NULL) {
                if (strcmp(mBeautyFeatureParams.beautyRelatedSwitchesInfo[i].tagName,
                           "makeupGender") == 0) {
                    mBeautyFeatureParams.beautyRelatedSwitchesInfo[i].featureValue =
                        1 - *static_cast<int *>(data);
                } else {
                    mBeautyFeatureParams.beautyRelatedSwitchesInfo[i].featureValue =
                        *static_cast<int *>(data);
                }
                MY_LOGI_IF(m_LogEnable, "TagName: %s, Value: %d", tagStr,
                           mBeautyFeatureParams.beautyRelatedSwitchesInfo[i].featureValue);
// consider
#if defined(ZIYI_CAM)
#else
                // if (mBeautyFeatureParams.beautyRelatedSwitchesInfo[i].featureValue != 0){ //
                // feature vlude support -100 -- + 100, 0 means no effect
                //    mBeautyEnabled = true;
                //}
            } else {
                MY_LOGE("Get Metadata Failed! TagName: %s", tagStr);
                // mBeautyEnabled = false;
#endif
            }
        }
    }
// consider
#if 0
#if defined(ZIYI_CAM)
    if (mBeautyEnabled) {
#else
    if (true == mIsFrontCamera && true == mBeautyEnabled) {
#endif
        data = NULL;
        MiFDBeautyParam *pMiFDBeautyParams = NULL;
        mFaceFeatureParams = {0};
        const char *fdBeautyResultTag = "xiaomi.fd.mifdbeautyparam";
        MetadataUtils::getInstance()->getVTagValue(pMetaData, fdBeautyResultTag, &data);

        if (NULL != data) {
            pMiFDBeautyParams = static_cast<MiFDBeautyParam *>(data);
            memcpy(&mFaceFeatureParams, pMiFDBeautyParams, sizeof(MiFDBeautyParam));
        } else {
            ALOGI("Get Metadata Failed! TagName: xiaomi.fd.mifdbeautyparam");
	    return;
        }

        // Translate face roi by zoom ratio
        if (mIsFrontCamera) {
            for (int i = 0; i < mFaceFeatureParams.miFaceCountNum; i++) {
                faceroi faceRect = {0};
                faceRect.left = mFaceFeatureParams.face_roi[i].left;
                faceRect.top = mFaceFeatureParams.face_roi[i].top;
                faceRect.width = mFaceFeatureParams.face_roi[i].width;
                faceRect.height = mFaceFeatureParams.face_roi[i].height;

                ALOGD("Get Metadata before translate faceroi l %d, t %d, w %d, h %d",
                         faceRect.left, faceRect.top, faceRect.width, faceRect.height);

                float verticalZoomRatio =
                    (float)mWidth / (float)mFaceFeatureParams.refdimension.width;
                float horizontalZoomRatio =
                    (float)mHeight / (float)mFaceFeatureParams.refdimension.height;

                ALOGD("Get Metadata before translate faceroi width %d height %d",
                         mFaceFeatureParams.refdimension.width,
                         mFaceFeatureParams.refdimension.height);

                faceRect.left *= verticalZoomRatio;
                faceRect.top *= horizontalZoomRatio;
                faceRect.width *= verticalZoomRatio;
                faceRect.height *= horizontalZoomRatio;

                ALOGD("Get Metadata after translate faceroi l %d, t %d, w %d, h %d",
                         faceRect.left, faceRect.top, faceRect.width, faceRect.height);

                mFaceFeatureParams.face_roi[i].left = faceRect.left;
                mFaceFeatureParams.face_roi[i].top = faceRect.top;
                mFaceFeatureParams.face_roi[i].width = faceRect.width;
                mFaceFeatureParams.face_roi[i].height = faceRect.height;
            }
        }
    }
#endif
    MY_LOGI_IF(m_LogEnable, "finish");
    MY_LOGI("Enabled = %d", mBeautyEnabled);
    return beautyEnabled;
}

void SkinBeautifierPlugin::getFrameInfo(camera_metadata_t *pMetadata)
{
    int sensitivity = 100;
    void *pData = NULL;
    MY_LOGI_IF(m_LogEnable, "enter");

    pData = NULL;
    VendorMetadataParser::getTagValue(pMetadata, ANDROID_SENSOR_SENSITIVITY, &pData);
    if (NULL != pData) {
        sensitivity = *static_cast<int32_t *>(pData);
    } else {
        sensitivity = 100;
    }
    MY_LOGI_IF(m_LogEnable, "get iso %d", sensitivity);

    pData = NULL;
    float ispGain = 0.0f;
    VendorMetadataParser::getVTagValue(pMetadata, "com.xiaomi.sensorbps.gain", &pData);
    if (NULL != pData) {
        ispGain = *static_cast<float *>(pData);
        MY_LOGI_IF(m_LogEnable, "get sensor_gain: %f", ispGain);
    }

    mFrameInfo.iso = (int)(ispGain * 100) * sensitivity / 100;

    pData = NULL;
    int fLuxIndex = 0;
    // float fISPGain = 0.0;
    float fADRCGain = 0.0;
    float exposure = 0.0;
    VendorMetadataParser::getVTagValue(pMetadata, "com.xiaomi.statsconfigs.AecInfo", &pData);
    if (NULL != pData) {
        fLuxIndex = static_cast<InputMetadataAecInfo *>(pData)->luxIndex;
        // fISPGain = static_cast<InputMetadataAecInfo *>(pData)->linearGain;
        fADRCGain = static_cast<InputMetadataAecInfo *>(pData)->adrcgain;
        exposure = static_cast<InputMetadataAecInfo *>(pData)->exposureTime;
    } else {
        fLuxIndex = 50.0f;
        // fISPGain = 1.0f;
        fADRCGain = 1.0f;
        exposure = 1.0f;
    }

    mFrameInfo.nLuxIndex = fLuxIndex;
    mFrameInfo.fExposure = exposure;
    mFrameInfo.fAdrc = fADRCGain;

#if 0
       pData = NULL;
        MetadataUtils::getInstance()->getVTagValue(pMetadata, "xiaomi.superResolution.awbgain", &pData);
        if (NULL != pData)
        {
            memcpy(AWbGain, pData, 4 * sizeof(float));
            MY_LOGI_IF(m_LogEnable, "AWBGain [%f %f %f %f]", AWbGain[0],AWbGain[1],
                  AWbGain[2],AWbGain[3]);
        }else {
            MY_LOGE("AWbGain failed");
        }
#endif

    // get flash on or not

    if (true == mIsFrontCamera) { // front

        int miFlashMode = 0;
        pData = NULL;
        VendorMetadataParser::getVTagValue(pMetadata,
                                           "xiaomi.snapshot.front.ScreenLighting.enabled", &pData);
        if (pData != NULL) {
            miFlashMode = *static_cast<int32_t *>(pData);
            MY_LOGI_IF(m_LogEnable, "getMeta miFlashMode result:%d", miFlashMode);
        } else {
            MY_LOGE("getMeta miFlashMode error %d", miFlashMode);
        }

#if defined(ZIYI_CAM)
        // 107 ambientlight flash; 108   front softlight on
        switch (miFlashMode) {
        case 0:
        case 1:
        case 2:
            mFrameInfo.nFlashTag = miFlashMode;
            break;
        case 107:
            mFrameInfo.nFlashTag = 2;
            break;
        case 108:
            mFrameInfo.nFlashTag = 1;
            break;
        default:
            mFrameInfo.nFlashTag = miFlashMode;
            break;
        }
#else

        mFrameInfo.nFlashTag = miFlashMode; // 0: off; 1 auto; 2: flash on; 104: screenlight on;
                                            // 106: screenlight & flash on
#endif

    } else { // rear
        pData = NULL;
        int androidFlashMode = 0;
        int miFlashMode = 0;
        VendorMetadataParser::getTagValue(pMetadata, ANDROID_FLASH_MODE, &pData);
        if (NULL != pData) {
            androidFlashMode = *(static_cast<int32_t *>(pData));
        } else {
            MY_LOGE("GetMetaData [ANDROID_FLASH_MODE] Failed!");
        }

        pData = NULL;
        VendorMetadataParser::getVTagValue(pMetadata, "xiaomi.flash.mode", &pData);
        if (NULL != pData) {
            miFlashMode = *(static_cast<int32_t *>(pData));
        } else {
            MY_LOGE("GetMetaData [xiaomi.flash.mode] Failed!");
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

#if 0
    // get iso value
    int sensitivity = 100;
    float ispGain = 1.0;
    void *pData = NULL;

    MetadataUtils::getInstance()->getTagValue(pMetadata, ANDROID_SENSOR_SENSITIVITY, &pData);
    if (NULL != pData) {
        sensitivity = *static_cast<int32_t *>(pData);
    } else {
        ALOGD("GetMetaData [ANDROID_SENSOR_SENSITIVITY] failed!");
    }

    pData = NULL;
    MetadataUtils::getInstance()->getVTagValue(pMetadata, "com.qti.sensorbps.gain", &pData);
    if (NULL != pData) {
        ispGain = *static_cast<float *>(pData);
    } else {
        ALOGD("GetMetaData [com.qti.sensorbps.gain] failed!");
    }

    mFrameInfo.iso = (int)(ispGain * 100) * sensitivity / 100;

    // get AE info
    pData = NULL;
    AECFrameControl *pAECFrameControl = NULL;
    MetadataUtils::getInstance()->getVTagValue(pMetadata, "org.quic.camera2.statsconfigs.AECFrameControl",
                                       &pData);

    if (NULL != pData) {
        pAECFrameControl = static_cast<AECFrameControl *>(pData);

        mFrameInfo.nLuxIndex = pAECFrameControl->luxIndex;
        mFrameInfo.fExposure =
            (float)pAECFrameControl->exposureInfo[ExposureIndexSafe].exposureTime;
        mFrameInfo.fAdrc = pAECFrameControl->exposureInfo[ExposureIndexSafe].sensitivity /
                           pAECFrameControl->exposureInfo[ExposureIndexShort].sensitivity;
    } else {
        ALOGE("GetMetaData [org.quic.camera2.statsconfigs.AECFrameControl] Failed!");
    }

#if defined(ZIYI_CAM)
    if (!mIsFrontCamera) {
#else
    if (false == mBeautyEnabled && false == mIsFrontCamera) {
#endif
        // get FaceLuma info
        MiFDFaceLumaInfo *miFdFaceLuma = NULL;
        pData = NULL;

        MetadataUtils::getInstance()->getVTagValue(pMetadata, "xiaomi.fd.mifdfaceluma", &pData);
        if (NULL != pData) {
            miFdFaceLuma = static_cast<MiFDFaceLumaInfo *>(pData);
            memcpy(&mFaceLuma, miFdFaceLuma, sizeof(MiFDFaceLumaInfo));

            mDoFaceWhiten = GetFaceWhitenRatioByFaceLumaInfo(mFaceLuma, mFrameInfo.nLuxIndex,
                                                             &mFaceWhitenRatio);

#if defined(ZIYI_CAM)
            if (mDoFaceWhiten && (sModuleInfo.find("imx766_ii") != sModuleInfo.npos)) {
                mFaceWhitenRatio = (mFaceWhitenRatio > whitenAjustRatio)
                                       ? (mFaceWhitenRatio - whitenAjustRatio)
                                       : mFaceWhitenRatio;
            }
            ALOGD(
                "GetMetaData faceROISize: %f, facelumaVSframeluma: %f, cameraId: %d, "
                "AECLocked: %d, mFaceWhitenRatio %d",
                mFaceLuma.faceROISize, mFaceLuma.facelumaVSframeluma, mFaceLuma.cameraId,
                mFaceLuma.AECLocked, mFaceWhitenRatio);
#else
            ALOGD(
                "GetMetaData faceROISize: %f, frameLuma: %f, cameraId: %d, AECLocked: %d, "
                "faceLuma: %f",
                mFaceLuma.faceROISize, mFaceLuma.frameLuma, mFaceLuma.cameraId, mFaceLuma.AECLocked,
                mFaceLuma.faceLuma);
#endif

        } else {
            ALOGD("GetMetaData mifdfaceluma is NULL");
            mFaceWhitenRatio = 0;
            mDoFaceWhiten = false;
        }
    } else {
        mFaceWhitenRatio = 0;
        mDoFaceWhiten = false;
    }

    // get AWB info
    pData = NULL;
    AWBFrameControl *pAWBFrameControl = NULL;
    MetadataUtils::getInstance()->getVTagValue(pMetadata, "org.quic.camera2.statsconfigs.AWBFrameControl",
                                       &pData);

    if (NULL != pData) {
        pAWBFrameControl = static_cast<AWBFrameControl *>(pData);

        mFrameInfo.nColorTemperature = pAWBFrameControl->colorTemperature;
    } else {
        ALOGE("GetMetaData [org.quic.camera2.statsconfigs.AWBFrameControl] Failed!");
    }

    // get flash on or not

    if (true == mIsFrontCamera) { // front
        pData = NULL;
        int miFlashMode = 0;
        MetadataUtils::getInstance()->getVTagValue(pMetadata, "xiaomi.flash.mode", &pData);
        if (NULL != pData) {
            miFlashMode = *(static_cast<int32_t *>(pData));
        } else {
            ALOGE("GetMetaData [xiaomi.flash.mode] Failed!");
        }

#if defined(ZIYI_CAM)
        // 107 ambientlight flash; 108   front softlight on
        switch (miFlashMode) {
        case 0:
        case 1:
        case 2:
            mFrameInfo.nFlashTag = miFlashMode;
            break;
        case 107:
            mFrameInfo.nFlashTag = 2;
            break;
        case 108:
            mFrameInfo.nFlashTag = 1;
            break;
        default:
            mFrameInfo.nFlashTag = miFlashMode;
            break;
        }
#else
        mFrameInfo.nFlashTag = miFlashMode; // 0: off; 1 auto; 2: flash on; 104: screenlight on;
                                            // 106: screenlight & flash on
#endif

    } else { // rear
        pData = NULL;
        int androidFlashMode = 0;
        MetadataUtils::getInstance()->getTagValue(pMetadata, ANDROID_FLASH_MODE, &pData);
        if (NULL != pData) {
            androidFlashMode = *(static_cast<int32_t *>(pData));
        } else {
            ALOGE("GetMetaData [ANDROID_FLASH_MODE] Failed!");
        }

        pData = NULL;
        int miFlashMode = 0;
        MetadataUtils::getInstance()->getVTagValue(pMetadata, "xiaomi.flash.mode", &pData);
        if (NULL != pData) {
            miFlashMode = *(static_cast<int32_t *>(pData));
        } else {
            ALOGE("GetMetaData [xiaomi.flash.mode] Failed!");
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

    ALOGD(
        "Frame info: [iso:%d] [luxIndex:%d] [flashTag:%d] [expTime:%f] [adrc:%f] "
        "[colorTemp:%d]",
        mFrameInfo.iso, mFrameInfo.nLuxIndex, mFrameInfo.nFlashTag, mFrameInfo.fExposure,
        mFrameInfo.fAdrc, mFrameInfo.nColorTemperature);
#endif
}

int SkinBeautifierPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

void SkinBeautifierPlugin::destroy()
{
    if (mSkinBeautifier != NULL) {
        delete mSkinBeautifier;
        mSkinBeautifier = NULL;
    }
}

void SkinBeautifierPlugin::updateExifData(ProcessRequestInfo *processRequestInfo,
                                          ProcessRetStatus resultInfo, double processTime)
{
    char data[1024] = {0};

    snprintf(data, sizeof(data),
             "skinbeautifier:{procRet:%d procTime:%d beautyMode:%d faceWhiten:%d lightMode:%d "
             "makeupMode:%d",
             resultInfo, (int)processTime, mBeautyFeatureParams.beautyFeatureMode.featureValue,
             mFaceWhitenRatio, mBeautyFeatureParams.beautyLightMode.featureValue,
             mBeautyFeatureParams.beautyMakeupMode.featureValue);

    std::string exifData(data);
    // add makeup params and values
    for (int i = 0; i < mBeautyFeatureParams.beautyMakeupCounts; ++i) {
        exifData = exifData + ' ' + mBeautyFeatureParams.beautyMakeupInfo[i].tagName + ':' +
                   std::to_string(mBeautyFeatureParams.beautyMakeupInfo[i].featureValue);
    }

    // add beauty params and values
    for (int i = 0; i < mBeautyFeatureParams.featureCounts; ++i) {
        exifData = exifData + ' ' + mBeautyFeatureParams.beautyFeatureInfo[i].tagName + ':' +
                   std::to_string(mBeautyFeatureParams.beautyFeatureInfo[i].featureValue);
    }
    exifData += "}";
    // syg
    // if (NULL != mNodeInterface.pSetResultMetadata) {
    //    mNodeInterface.pSetResultMetadata(mNodeInterface.owner, processRequestInfo->frameNum,
    //                                      processRequestInfo->timeStamp, exifData);
    //}
}

void SkinBeautifierPlugin::updateFaceInfo(ProcessRequestInfo *processRequestInfo,
                                          MiFDBeautyParam *faceFeatureParams)
{
    if (processRequestInfo->inputBuffersMap.size() <= 0) {
        return;
    }

    auto &phInputBuffer = processRequestInfo->inputBuffersMap.at(0);
    // auto &phOutputBuffer = processRequestInfo->outputBuffersMap.at(0);
    // bool skinBeautyEnable = false;
    // camera_metadata_t *pMetaData = (camera_metadata_t *)phInputBuffer[0].pMetadata;
    camera_metadata_t *metaData = (camera_metadata_t *)phInputBuffer[0].pMetadata;

    ImageParams inputImage = phInputBuffer[0];
    // ImageParams outputImage = phOutputBuffer[0];
    int inputWidth = inputImage.format.width;
    int inputHeight = inputImage.format.height;

    MY_LOGI_IF(m_LogEnable, "inputWidth %d,inputHeight %d", inputWidth, inputHeight);
    void *pData = NULL;
    MIRECT *androidCropChiRect = {0};

    VendorMetadataParser::getTagValue(metaData, ANDROID_SCALER_CROP_REGION, &pData);
    if (NULL != pData) {
        androidCropChiRect = static_cast<MIRECT *>(pData);
        MY_LOGE("Get Metadata ANDROID_SCALER_CROP_REGION l %d, t %d, w %d, h %d",
                androidCropChiRect->left, androidCropChiRect->top, androidCropChiRect->width,
                androidCropChiRect->height);
    }

    // Get crop region
    MIRECT cropRegion = {0};
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.snapshot.cropRegion", &pData);
    MY_LOGI_IF(m_LogEnable, "Get Metadata cropRegion tag %p", pData);
    if (NULL != pData) {
        cropRegion = *(reinterpret_cast<MIRECT *>(pData));
        MY_LOGE("Get Metadata cropRegion l %d, t %d, w %d, h %d facecount %d", cropRegion.left,
                cropRegion.top, cropRegion.width, cropRegion.height,
                mFaceFeatureParams.miFaceCountNum);
    }

    // Translate face roi by zoom ratio
    int index = 0;
    for (int i = 0; i < mFaceFeatureParams.miFaceCountNum; i++) {
        MIRECT faceRect = {0};
        faceRect.left = mFaceFeatureParams.face_roi[i].left / 2;
        faceRect.top = mFaceFeatureParams.face_roi[i].top / 2;
        faceRect.width = mFaceFeatureParams.face_roi[i].width / 2;
        faceRect.height = mFaceFeatureParams.face_roi[i].height / 2;

        MY_LOGI_IF(
            m_LogEnable,
            "facecount %d Get Metadata [index %d] before translate faceroi l %d, t %d, w %d, h %d",
            mFaceFeatureParams.miFaceCountNum, index, faceRect.left, faceRect.top, faceRect.width,
            faceRect.height);

        if ((0 == cropRegion.height && 0 == cropRegion.width && 0 == cropRegion.left &&
             0 == cropRegion.top) ||
            NULL == pData) {
            float verticalZoomRatio = (float)inputWidth / (float)androidCropChiRect->width;
            float horizontalZoomRatio = (float)inputHeight / (float)androidCropChiRect->height;

            if (faceRect.left > androidCropChiRect->left &&
                faceRect.top > androidCropChiRect->top) {
                faceRect.left = (faceRect.left - androidCropChiRect->left) * verticalZoomRatio;
                faceRect.top = (faceRect.top - androidCropChiRect->top) * horizontalZoomRatio;
                faceRect.width *= verticalZoomRatio;
                faceRect.height *= horizontalZoomRatio;

                mFaceFeatureParams.face_roi[index].left = faceRect.left;
                mFaceFeatureParams.face_roi[index].top = faceRect.top;
                mFaceFeatureParams.face_roi[index].width = faceRect.width;
                mFaceFeatureParams.face_roi[index].height = faceRect.height;
                MY_LOGI_IF(m_LogEnable,
                           "Get Metadata [index %d] translate faceroi l %d, t %d, w %d, h %d",
                           index, faceRect.left, faceRect.top, faceRect.width, faceRect.height);
                index++;
            } else {
                mFaceFeatureParams.miFaceCountNum--;
            }
        } else {
            float verticalZoomRatio = (float)inputWidth / (float)cropRegion.width;
            float horizontalZoomRatio = (float)inputHeight / (float)cropRegion.height;

            if (faceRect.left > cropRegion.left && faceRect.top > cropRegion.top) {
                faceRect.left = (faceRect.left - cropRegion.left) * verticalZoomRatio;
                faceRect.top = (faceRect.top - cropRegion.top) * horizontalZoomRatio;
                faceRect.width *= verticalZoomRatio;
                faceRect.height *= horizontalZoomRatio;

                mFaceFeatureParams.face_roi[index].left = faceRect.left;
                mFaceFeatureParams.face_roi[index].top = faceRect.top;
                mFaceFeatureParams.face_roi[index].width = faceRect.width;
                mFaceFeatureParams.face_roi[index].height = faceRect.height;
                MY_LOGI_IF(m_LogEnable,
                           "Get Metadata [index %d] translate faceroi l %d, t %d, w %d, h %d",
                           index, faceRect.left, faceRect.top, faceRect.width, faceRect.height);
                index++;
            } else {
                mFaceFeatureParams.miFaceCountNum--;
            }
        }
    }
}

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

    if (miBuffer->plane[0] == NULL || miBuffer->plane[1] == NULL) {
        MY_LOGE("%s:%d wrong input", __func__, __LINE__);
    }
}

bool SkinBeautifierPlugin::getFaceInfo(ProcessRequestInfo *processRequestInfo)
{
    void *pData = NULL;
    int32_t ret;
    uint32_t sensorWidth = 0;
    uint32_t sensorHeight = 0;
    size_t tcount = 0;
    uint32_t faceNum = 0;
    // int tscount = 0;

    const float AspectRatioTolerance = 0.01f;
    int gapHeight = 0;
    int gapWidth = 0;

    if (processRequestInfo->inputBuffersMap.size() <= 0) {
        return false;
    }

    auto &phInputBuffer = processRequestInfo->inputBuffersMap.at(0);
    // auto &phOutputBuffer = processRequestInfo->outputBuffersMap.at(0);
    // bool skinBeautyEnable = false;
    // camera_metadata_t *pMetaData = (camera_metadata_t *)phInputBuffer[0].pMetadata;
    camera_metadata_t *mdata = (camera_metadata_t *)phInputBuffer[0].pMetadata;

    ImageParams inputImage = phInputBuffer[0];
    // ImageParams outputImage = phOutputBuffer[0];
    int inWidth = inputImage.format.width;
    int inHeight = inputImage.format.height;

    MY_LOGI_IF(m_LogEnable, "enter");

    ret = VendorMetadataParser::getVTagValue(mdata, "xiaomi.sensorSize.size", &pData);
    if ((ret != 0) && (pData != NULL)) {
        MIDIMENSION *dimensionSize = (MiDimension *)(pData);
        sensorWidth = dimensionSize->width;
        sensorHeight = dimensionSize->height;
        MY_LOGI("get tag sensorWidth = %u sensorHeight = %u", sensorWidth, sensorHeight);
    } else {
        // syg
        if (CAM_COMBMODE_FRONT == mFrameworkCameraId) {
            sensorWidth = 3264;
            sensorHeight = 2448;
        } else {
            sensorWidth = 4000;
            sensorHeight = 3000;
        }
    }

    MY_LOGI("get final sensorWidth = %u sensorHeight = %u", sensorWidth, sensorHeight);

    pData = NULL;
    int32_t rotateAngle = 0;
    ret = VendorMetadataParser::getTagValue(mdata, ANDROID_JPEG_ORIENTATION, &pData);
    if ((ret == 0) && (pData != NULL)) {
        rotateAngle = *reinterpret_cast<int32_t *>(pData);
    }

    pData = NULL;
    // ret = VendorMetadataParser::getVTagValueExt(mdata, "xiaomi.statistics.faceRectangles",
    // &pData, tcount);
    ret = VendorMetadataParser::getVTagValueCount(mdata, "xiaomi.statistics.faceRectangles", &pData,
                                                  &tcount);
    if (NULL == pData) {
        // ret = VendorMetadataParser::getTagValueExt(mdata, ANDROID_STATISTICS_FACE_RECTANGLES,
        // &pData, tcount);
        ret = VendorMetadataParser::getTagValueCount(mdata, ANDROID_STATISTICS_FACE_RECTANGLES,
                                                     &pData, &tcount);
    }

    if (NULL != pData) {
        faceNum = ((int32_t)tcount) / 4;
        if (0 == faceNum) {
            return false;
        }

        MY_LOGI("get capture faceNum %d,%d", faceNum, ret);
    } else {
        MY_LOGI("It did not get face from tag");
        return false;
    }

    float ratioWidth = (float)inWidth / sensorWidth;
    float ratioHeight = (float)inHeight / sensorHeight;
    float heigthRwidth = (float)sensorHeight / sensorWidth;
    float widthRheight = (float)sensorWidth / sensorHeight;

    if (ratioWidth > (ratioHeight + AspectRatioTolerance)) {
        // 16:9 case run to here, it means height is cropped.
        gapHeight = ((inWidth * heigthRwidth) - inHeight) / 2;
        ratioHeight = ratioWidth;
    } else if (ratioWidth < (ratioHeight - AspectRatioTolerance)) {
        // 1:1 case run to here, it means height is cropped.
        gapWidth = ((inHeight * widthRheight) - inWidth) / 2;
        ratioWidth = ratioHeight;
    }

    int faceCount = faceNum > FD_Max_FACount ? FD_Max_FACount : faceNum;
    mFaceFeatureParams.miFaceCountNum = faceCount;
    for (uint i = 0, j = 0; i < faceCount; i++) {
        int32_t xmin = reinterpret_cast<int32_t *>(pData)[j++];
        int32_t ymin = reinterpret_cast<int32_t *>(pData)[j++];
        int32_t xmax = reinterpret_cast<int32_t *>(pData)[j++];
        int32_t ymax = reinterpret_cast<int32_t *>(pData)[j++];

        MY_LOGI("face origin: x1 = %d y1 = %d  x2 = %d y2 = %d gapHeight = %d.", xmin, ymin, xmax,
                ymax, gapHeight);
        xmin = xmin * ratioWidth;
        xmax = xmax * ratioWidth;
        ymin = ymin * ratioHeight;
        ymax = ymax * ratioHeight;

        ymin = ymin - gapHeight;
        ymax = ymax - gapHeight;
        xmin = xmin - gapWidth;
        xmax = xmax - gapWidth;

        xmin = xmin > 0 ? xmin : 0;
        xmax = xmax > 0 ? xmax : 0;
        ymin = ymin > 0 ? ymin : 0;
        ymax = ymax > 0 ? ymax : 0;

        // convert face rectangle to be based on current input dimension
        mFaceFeatureParams.face_roi[i].left = xmin;
        mFaceFeatureParams.face_roi[i].top = ymin;
        mFaceFeatureParams.face_roi[i].width = xmax - xmin;
        mFaceFeatureParams.face_roi[i].height = ymax - ymin;
        mFaceFeatureParams.rollangle[i] = rotateAngle;
    }
    return true;
}

bool SkinBeautifierPlugin::drawFaceBox(char *buffer, MiFDBeautyParam &faceFeatureParams, int width,
                                       int height, int stride, bool isIn, char mask)
{
    int faceCount = faceFeatureParams.miFaceCountNum;
    float fwidth = width;
    float fheight = height;
    MY_LOGI("drawFaceBox() faceCount = %d", faceCount);

    if (faceCount > 0) {
        for (int i = 0; i < faceCount; i++) {
            float x = faceFeatureParams.face_roi[i].left;
            float y = faceFeatureParams.face_roi[i].top;
            float w = faceFeatureParams.face_roi[i].width;
            float h = faceFeatureParams.face_roi[i].height;
            if (isIn == true) {
                // draw face rect
                drawFaceRect(buffer, x / fwidth, y / fheight, w / fwidth, h / fheight, width,
                             height, stride, mask);
            } else {
                // draw face rect
                drawFaceRect(buffer, x / fwidth, y / fheight, w / fwidth, h / fheight, width,
                             height, stride, mask);
                // draw face landmark
                drawPoints(buffer, faceFeatureParams.milandmarks[i].points, FD_Max_Landmark, width,
                           height, stride, mask);
                // struct position *foreHeadPoints = (struct position
                // *)(miFDParam->foreheadPoints[i].points); drawPoints(buffer, foreHeadPoints,
                // MAX_FOREHEAD_POINT_NUM / 2, 0xff); draw face mouth
            }
        }
    }
    return true;
}

bool SkinBeautifierPlugin::drawFaceRect(char *buffer, float fx, float fy, float fw, float fh, int w,
                                        int h, int stride, char mask)
{
    // sample: modify output buffer
    if (buffer) {
        // char mask = 0;
        int lineWidth = 6;
        int y_from = fy * h;
        int y_to = (fy + fh) * h;
        int x = fx * w;
        int width = fw * w;
        int y_start, y_end;

        y_to = std::max(0, std::min(y_to, h - 1));
        y_from = std::max(0, std::min(y_from, y_to));
        x = std::max(0, std::min(x, w - 1));
        width = std::max(0, std::min(width, w - 1));

        // top
        y_start = y_from;
        y_end = std::max(0, std::min(y_start + lineWidth, h - 1));

        for (int y = y_start; y < y_end; ++y) {
            memset(buffer + y * stride + x, mask, width);
        }

        // bot
        y_start = y_to;
        y_end = std::max(0, std::min(y_start + lineWidth, h - 1));

        for (int y = y_start; y < y_end; ++y) {
            memset(buffer + y * stride + x, mask, width);
        }
        // left
        y_start = y_from;
        y_end = y_to;
        int x_lw = std::min(x + lineWidth, w - 1) - x;
        for (int y = y_start; y < y_end; ++y) {
            memset(buffer + y * stride + x, mask, x_lw);
        }

        // right
        y_start = y_from;
        y_end = y_to;
        x = std::min(x + width, w - 1);
        // ALOGE("syg: x = %d", x);
        x_lw = std::min(x + lineWidth, w - 1) - x;
        for (int y = y_start; y < y_end; ++y) {
            memset(buffer + y * stride + x, mask, x_lw);
        }
    }

    return true;
}

bool SkinBeautifierPlugin::drawPoints(char *buffer, struct position *points, int num, int w, int h,
                                      int stride, char mask)
{
    // ALOGD("drawPoints() num = %d (%f, %f)", num, points->x, points->y);
    // sample: modify output buffer
    // char mask = 0xff;
    if (buffer) {
        for (int i = 0; i < num; i++) {
            struct position *point = points + i;
            int x = point->x;
            int y = point->y;
            x = std::min(std::max(x, 0), w - 1);
            y = std::min(std::max(y, 0), h - 1);

            if ((x < w) && (y < h)) {
                int pointWidth = 12;
                int x_start = std::max(x - pointWidth / 2, 0);
                int x_end = std::min(x + pointWidth / 2, w - 1);
                int x_width = x_end - x_start;
                int y_start = std::max(y - pointWidth / 2, 0);
                int y_end = std::min(y + pointWidth / 2, h - 1);
                // MY_LOGI("drawPoints() i = %d x_start = %d x_end = %d x_width = %d y_start = %d
                // y_end = %d",
                //                                                  i, x_start, x_end, x_width,
                //                                                  y_start, y_end);

                for (int j = y_start; j <= y_end; j++) {
                    memset(buffer + j * stride + x_start, mask, x_width);
                }
            }
        }
    }
    return true;
}