#include "include/SkinBeautifierPlugin.h"

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

static void __attribute__((unused)) gettimestr(char *timestr, size_t size)
{
    time_t currentTime;
    struct tm *timeInfo;
    time(&currentTime);
    timeInfo = localtime(&currentTime);
    strftime(timestr, size, "%Y%m%d_%H%M%S", timeInfo);
}

SkinBeautifierPlugin::~SkinBeautifierPlugin()
{
    if (mSkinBeautifier != NULL) {
        delete mSkinBeautifier;
        mSkinBeautifier = NULL;
    }
}

int SkinBeautifierPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mBeautyMode = BEAUTY_MODE_CAPTURE;
    mFrameworkCameraId = createInfo->frameworkCameraId;
    mSkinBeautifier = NULL;
    mProcessCount = 0;
    mOrientation = 0;
    mRegion = 0;
    mInstanceName = createInfo->nodeInstance;
    mNodeInterface = nodeInterface;
    m_appModule = 0;
    m_appModuleMode = 0;
    memset(&mFaceAnalyze, 0, sizeof(OutputMetadataFaceAnalyze));

    if (createInfo->operationMode == 0xFF0A && createInfo->outputFormat.format == 35) {
        mSdkYuvMode = true;
    } else {
        mSdkYuvMode = false;
    }

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
        MLOGD_BF("Get Region Failed! Set Value as Global %d", mRegion);
    }
    MLOGD_BF("Get Region Success, the Value is %d", mRegion);

    switch (createInfo->operationMode) {
    case StreamConfigModeAlgoDualSAT:
    case StreamConfigModeQCFALite:
        m_appModule = APP_MODULE_DEFAULT_CAPTURE;
        break;
    case StreamConfigModeBokeh:
        m_appModule = APP_MODULE_FRONT_PORTRAIT;
        break;
    default:
        break;
    }
    MLOGD_BF("capture operationMode is %d, appModule is %d", createInfo->operationMode,
             m_appModule);

    /*for (int i = 0; i < ADVANCE_BEAUTY_FEATURE_NUM; i++)
    {
        switch (i)
        {
            case ADVANCE_FACE_SLENDER:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.slimFaceRatio";
                break;
            case ADVANCE_EYE_ENLARGE:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.enlargeEyeRatio";
                break;
            case ADVANCE_AUTO_BRIGHT:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.skinColorRatio";
                break;
            case ADVANCE_SOFTEN:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.skinSmoothRatio";
                break;
            case ADVANCE_NOSE_3D:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.noseRatio";
                break;
            case ADVANCE_RISORIUS_3D:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.risoriusRatio";
                break;
            case ADVANCE_LIP_3D:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.lipsRatio";
                break;
            case ADVANCE_CHIN_3D:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.chinRatio";
                break;
            case ADVANCE_NECK_3D:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.neckRatio";
                break;
            case ADVANCE_EYE_BROW:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.eyeBrowDyeRatio";
                break;
            case ADVANCE_PUPIL_LINE:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.pupilLineRatio";
                break;
            case ADVANCE_LIP_GLOSS:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.lipGlossRatio";
                break;
            case ADVANCE_BLUSH:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.blushRatio";
                break;
            case ADVANCE_EYE_LIGHT_STRENGTH:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.eyeLightStrength";
                break;
            case ADVANCE_EYE_LIGHT_TYPE:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.eyeLightType";
                break;
            case ADVANCE_HAIR_LINE:
                m_BeautyVendorTagArray[i] = "xiaomi.beauty.hairlineRatio";
                break;
            default:
                break;
        }
    }*/

    return 0;
}

bool SkinBeautifierPlugin::isEnabled(MiaParams settings)
{
    getBeautyParams(settings.metadata);
    return mBeautyEnabled;
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
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus SkinBeautifierPlugin::processBuffer(struct ImageParams *input,
                                                     struct ImageParams *output,
                                                     camera_metadata_t *metaData)
{
    int32_t IsDumpData = property_get_int32("persist.vendor.camera.SkinBeautifier.dump", 0);

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

    getBeautyParams(metaData);

    if (mSkinBeautifier != NULL && mBeautyEnabled == true) {
        ASVLOFFSCREEN inputFrame, outputFrame;
        imageParamsToOffScreen(&inputFrame, input);
        imageParamsToOffScreen(&outputFrame, output);

        mSkinBeautifier->SetFeatureParams(&mBeautyFeatureParams);

        mSkinBeautifier->IsFullFeature(true);

        if (IsDumpData) {
            char inputbuf[128];
            snprintf(inputbuf, sizeof(inputbuf), "SkinBeautifier_input_%dx%d", inputdumpFrame.width,
                     inputdumpFrame.height);
            PluginUtils::dumpToFile(inputbuf, &inputdumpFrame);
        }

        int result = 0;
#if defined(THOR_CAM) || defined(ZIZHAN_CAM)
        result = mSkinBeautifier->Process(&inputFrame, &outputFrame, mOrientation, mRegion,
                                          mBeautyMode, mFaceAnalyze.faceNum, m_appModule);
#else
        result = mSkinBeautifier->Process(&inputFrame, &outputFrame, mOrientation, mRegion,
                                          mBeautyMode, mFaceAnalyze.faceNum);
#endif
        // Copy buffer when algo process fail
        if (1 == result) {
            struct MiImageBuffer inputBuffer, outputBuffer;
            imageParamsToMiBuffer(&inputBuffer, input);
            imageParamsToMiBuffer(&outputBuffer, output);

            PluginUtils::miCopyBuffer(&outputBuffer, &inputBuffer);

            MLOGE_BF("SkinBeautifier algo process fail! Called MiCopyBuffer");
        }

        MLOGD_BF("mSkinBeautifier = %p, mBeautyEnabled = %d, Called SkinBeautifier::Process()",
                 mSkinBeautifier, mBeautyEnabled);

        if (IsDumpData) {
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
        MLOGD_BF("mSkinBeautifier = %p, mBeautyEnabled = %d, Called MiCopyBuffer()",
                 mSkinBeautifier, mBeautyEnabled);
    }

    return PROCSUCCESS;
}

void SkinBeautifierPlugin::getBeautyParams(camera_metadata_t *metadata)
{
    mBeautyEnabled = false;
    void *data = NULL;
    const char *faceAnalyzeResultTag = "xiaomi.faceAnalyzeResult.result";
    VendorMetadataParser::getVTagValue(metadata, faceAnalyzeResultTag, &data);
    if (NULL != data) {
        memcpy(&mFaceAnalyze, data, sizeof(OutputMetadataFaceAnalyze));
        MLOGD_BF("GetMetaData faceNum %d", mFaceAnalyze.faceNum);
    } else {
        MLOGD_BF("Get MetaData xiaomi.faceAnalyzeResult.result Failed!");
    }

    data = NULL;
    const char *deviceTag = "xiaomi.device.orientation";
    VendorMetadataParser::getVTagValue(metadata, deviceTag, &data);
    if (NULL != data) {
        mOrientation = *static_cast<int *>(data);
        mOrientation = (mOrientation + 90) % 360;
        if (mSkinBeautifier->IsFrontCamera(mFrameworkCameraId)) {
            // front camera
            mOrientation = 360 - mOrientation;
        }
        MLOGD_BF("GetMetaData [xiaomi.device.orientation] orientation = %d", mOrientation);
    } else {
        VendorMetadataParser::getTagValue(metadata, ANDROID_JPEG_ORIENTATION, &data);
        if (NULL != data) {
            mOrientation = *static_cast<int *>(data);
            mOrientation = mOrientation % 360;
            MLOGD_BF("GetMetaData [ANDROID_JPEG_ORIENTATION] orientation = %d", mOrientation);
        }
    }

#if defined(THOR_CAM) || defined(ZIZHAN_CAM)
    // 经典/原生
    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, "xiaomi.beauty.beautyMode", &data);
    if (NULL != data) {
        mBeautyFeatureParams.beautyFeatureMode.featureValue = *static_cast<int *>(data);
        MLOGD_BF("GetMetaData [xiaomi.beauty.beautyMode] beauty type = %d",
                 mBeautyFeatureParams.beautyFeatureMode.featureValue);
    } else {
        mBeautyFeatureParams.beautyFeatureMode.featureValue = 0;
        MLOGE_BF("GetMetaData [xiaomi.beauty.beautyMode] Failed!");
    }

    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, "xiaomi.app.module", &data);
    if (NULL != data) {
        m_appModuleMode = *static_cast<UINT32 *>(data);
        MLOGD_BF("GetMetaData [xiaomi.app.module] module mode = %d", m_appModuleMode);
    } else {
        m_appModuleMode = 0;
        MLOGE_BF("GetMetaData [xiaomi.app.module] Failed!");
    }

    // AI watermark front mode
    if ((MODE_AI_WATERMARK == m_appModuleMode) &&
        (mSkinBeautifier->IsFrontCamera(mFrameworkCameraId))) {
        m_appModule = 8;
    }

#endif

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
                    mBeautyEnabled = true;
                }

                if (strcmp(mBeautyFeatureParams.beautyFeatureInfo[i].tagName, "hairlineRatio") ==
                    0) {
                    mBeautyFeatureParams.beautyFeatureInfo[i].featureValue =
                        -mBeautyFeatureParams.beautyFeatureInfo[i].featureValue;
                }
            } else {
                MLOGI_BF("Get Metadata Failed! TagName: %s", tagStr);
            }
        }
    }

#if defined(RENOIR_CAM)
    // get makeup type
    memset(tagStr, 0, sizeof(tagStr));
    memcpy(tagStr, componentName, sizeof(componentName));
    memcpy(tagStr + sizeof(componentName) - 1, mBeautyFeatureParams.beautyStyleInfo.styleTagName,
           strlen(mBeautyFeatureParams.beautyStyleInfo.styleTagName) + 1);

    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, tagStr, &data);
    if (data != NULL) {
        mBeautyFeatureParams.beautyStyleInfo.styleValue = *static_cast<int *>(data);
        MLOGD_BF("Get Metadata Beauty Style TagName: %s, Value: %d", tagStr,
                 mBeautyFeatureParams.beautyStyleInfo.styleValue);

        if (mBeautyFeatureParams.beautyStyleInfo.styleValue !=
            0) // feature vlude support -100 -- + 100, 0 means no effect
        {
            mBeautyEnabled = true;
        }

    } else {
        MLOGI_BF("Get Metadata Beauty Style Failed! TagName: %s", tagStr);
    }

    // get makeup type level
    memset(tagStr, 0, sizeof(tagStr));
    memcpy(tagStr, componentName, sizeof(componentName));
    memcpy(tagStr + sizeof(componentName) - 1,
           mBeautyFeatureParams.beautyStyleInfo.styleLevelTagName,
           strlen(mBeautyFeatureParams.beautyStyleInfo.styleLevelTagName) + 1);

    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, tagStr, &data);
    if (data != NULL) {
        mBeautyFeatureParams.beautyStyleInfo.styleLevelValue = *static_cast<int *>(data);
        MLOGD_BF("Get Metadata Beauty Style TagName: %s, Value: %d", tagStr,
                 mBeautyFeatureParams.beautyStyleInfo.styleLevelValue);

        if (mBeautyFeatureParams.beautyStyleInfo.styleLevelValue !=
            0) // feature vlude support -100 -- + 100, 0 means no effect
        {
            mBeautyEnabled = true;
        }

    } else {
        MLOGI_BF("Get Metadata Beauty Style Failed! TagName: %s", tagStr);
    }

#elif (defined(ODIN_CAM) || defined(VILI_CAM) || defined(INGRES_CAM))
    // If makeup function is enable, set makeup type is nude and get makeup type level
    memset(tagStr, 0, sizeof(tagStr));
    memcpy(tagStr, componentName, sizeof(componentName));
    memcpy(tagStr + sizeof(componentName) - 1, "eyeBrowDyeRatio", strlen("eyeBrowDyeRatio") + 1);

    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, tagStr, &data);
    if (data != NULL) {
        mBeautyFeatureParams.beautyStyleInfo.styleLevelValue = *static_cast<int *>(data);
        MLOGD_BF("Get Metadata Beauty Style TagName: %s, Value: %d", tagStr,
                 mBeautyFeatureParams.beautyStyleInfo.styleLevelValue);

        if (mBeautyFeatureParams.beautyStyleInfo.styleLevelValue !=
            0) // feature vlude support -100 -- + 100, 0 means no effect
        {
            mBeautyFeatureParams.beautyStyleInfo.styleValue = 2;
            mBeautyEnabled = true;
        }

    } else {
        MLOGI_BF("Get Metadata Beauty Style Failed! TagName: %s", tagStr);
    }
#endif

    MLOGD_BF("Beauty Enabled = %d", mBeautyEnabled);
}

void SkinBeautifierPlugin::updateExifData(ProcessRequestInfo *processRequestInfo,
                                          ProcessRetStatus resultInfo, double processTime)
{
    // exif data format is skinbeautifier::{processTime:...}
    std::string results(mInstanceName);
    results += ":{";

    char data[1024] = {0};

#if defined(THOR_CAM) || defined(ZIZHAN_CAM)
    snprintf(data, sizeof(data),
             "procRet:%d procTime:%d skinSmoothRatio:%d slimFaceRatio:%d enlargeEyeRatio:%d"
             " noseRatio:%d chinRatio:%d lipsRatio:%d hairlineRatio:%d makeupRatio:%d",
             resultInfo, (int)processTime, mBeautyFeatureParams.beautyFeatureInfo[0].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[1].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[2].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[3].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[4].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[5].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[6].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[7].featureValue);
#else
    snprintf(data, sizeof(data),
             "procRet:%d procTime:%d skinSmoothRatio:%d slimFaceRatio:%d enlargeEyeRatio:%d"
             " noseRatio:%d chinRatio:%d lipsRatio:%d hairlineRatio:%d makeupRatio:%d",
             resultInfo, (int)processTime, mBeautyFeatureParams.beautyFeatureInfo[1].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[2].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[3].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[4].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[5].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[6].featureValue,
             mBeautyFeatureParams.beautyFeatureInfo[7].featureValue,
             mBeautyFeatureParams.beautyStyleInfo.styleLevelValue);
#endif

    std::string exifData(data);
    results += exifData;
    results += "}";

    if (NULL != mNodeInterface.pSetResultMetadata) {
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, processRequestInfo->frameNum,
                                          processRequestInfo->timeStamp, results);
    }
}

int SkinBeautifierPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
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

void SkinBeautifierPlugin::imageParamsToOffScreen(ASVLOFFSCREEN *offScreen,
                                                  struct ImageParams *imageParams)
{
    offScreen->u32PixelArrayFormat = mSdkYuvMode ? ASVL_PAF_NV21 : ASVL_PAF_NV12;
    offScreen->i32Width = imageParams->format.width;
    offScreen->i32Height = imageParams->format.height;
    for (int i = 0; i < imageParams->numberOfPlanes && i < 3; i++) {
        offScreen->pi32Pitch[i] = imageParams->planeStride;
        offScreen->ppu8Plane[i] = imageParams->pAddr[i];
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
}
