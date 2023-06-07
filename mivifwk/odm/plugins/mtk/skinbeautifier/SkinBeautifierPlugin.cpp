#include "SkinBeautifierPlugin.h"

#undef LOG_TAG
#define LOG_TAG "SkinBeautifierPlugin"

SkinBeautifierPlugin::~SkinBeautifierPlugin() {}

int SkinBeautifierPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    m_BeautyMode = BEAUTY_MODE_CAPTURE;
    m_isBeautyIntelligent = 0;
    m_frameworkCameraId = createInfo->frameworkCameraId;
    m_beautyEnabled = false;
    mSkinBeautifier = NULL;
    mRegion = 0;
    memset(&m_faceAnalyze, 0x0, sizeof(OutputMetadataFaceAnalyze));
    mProcessCount = 0;
    if (!mSkinBeautifier)
        mSkinBeautifier = new SkinBeautifier();

    mSkinBeautifier->IsFrontCamera(m_frameworkCameraId);

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
        MLOGD(Mia2LogGroupPlugin, "MIAfym:: Get Region Failed! Set Value as Global %d", mRegion);
    }

    MLOGD(Mia2LogGroupPlugin, "MIAfym:: Get Region Success, the Value is %d", mRegion);
    /*
    for (int i = 0; i < ADVANCE_BEAUTY_FEATURE_NUM; i++) {
        switch (i) {
            case ADVANCE_FACE_SLENDER:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.slimFaceRatio";
                break;
            case ADVANCE_EYE_ENLARGE:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.enlargeEyeRatio";
                break;
            case ADVANCE_AUTO_BRIGHT:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.skinColorRatio";
                break;
            case ADVANCE_SOFTEN:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.skinSmoothRatio";
                break;
            case ADVANCE_NOSE_3D:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.noseRatio";
                break;
            case ADVANCE_RISORIUS_3D:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.risoriusRatio";
                break;
            case ADVANCE_LIP_3D:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.lipsRatio";
                break;
            case ADVANCE_CHIN_3D:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.chinRatio";
                break;
            case ADVANCE_NECK_3D:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.neckRatio";
                break;
            case ADVANCE_EYE_BROW:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.eyeBrowDyeRatio";
                break;
            case ADVANCE_PUPIL_LINE:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.pupilLineRatio";
                break;
            case ADVANCE_LIP_GLOSS:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.lipGlossRatio";
                break;
            case ADVANCE_BLUSH:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.blushRatio";
                break;
            case ADVANCE_EYE_LIGHT_STRENGTH:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.eyeLightStrength";
                break;
            case ADVANCE_EYE_LIGHT_TYPE:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.eyeLightType";
                break;
            case ADVANCE_HAIR_LINE:
                mBeautyVendorTagArray[i] = "xiaomi.beauty.hairlineRatio";
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
    return m_beautyEnabled;
}

ProcessRetStatus SkinBeautifierPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    auto &phInputBuffer = processRequestInfo->inputBuffersMap.at(0);
    auto &phOutputBuffer = processRequestInfo->outputBuffersMap.at(0);
    // bool skinBeautyEnable = false;
    camera_metadata_t *pMetaData = (camera_metadata_t *)phInputBuffer[0].pMetadata;

    ImageParams inputImage = phInputBuffer[0];
    ImageParams outputImage = phOutputBuffer[0];
    resultInfo = ProcessBuffer(&inputImage, &outputImage, pMetaData);
    mProcessCount++;

    return resultInfo;
}

ProcessRetStatus SkinBeautifierPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

ProcessRetStatus SkinBeautifierPlugin::ProcessBuffer(struct ImageParams *input,
                                                     struct ImageParams *output,
                                                     camera_metadata_t *metaData)
{
    if (input == NULL || output == NULL || metaData == NULL || mSkinBeautifier == NULL) {
        MLOGE(Mia2LogGroupPlugin, "error invalid param %p, %p, %p, mSkinBeautifier %p", input,
              output, metaData, mSkinBeautifier);
        return PROCFAILED;
    }
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.capture.beauty.dump", prop, "0");
    int32_t IsDumpData = (int32_t)atoi(prop);
    void *pData = NULL;
    pData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_JPEG_ORIENTATION, &pData);
    if (NULL != pData) {
        mOrientation = *static_cast<int32_t *>(pData);
        MLOGE(Mia2LogGroupPlugin, " beauty orientation capture=%d\n", mOrientation);
    }

    // input->format.format is get from com.mediatek.configure.setting.exactflexyuvfmt
    // input format is the actual bufer format,arcsoft algo need output buffer format must be same
    // as input
    enum_flex_yuv_exact_format_t bufferFormat = MTK_CONFIGURE_FLEX_YUV_YV12;
    if (NULL != metaData) {
        void *pFormatData = NULL;
        // get exactflexyuvfmt for YUV format
        VendorMetadataParser::getVTagValue(
            metaData, "com.mediatek.configure.setting.exactflexyuvfmt", &pFormatData);
        if (NULL != pFormatData) {
            bufferFormat = *static_cast<enum_flex_yuv_exact_format_t *>(pFormatData);
        }
    }
    if (bufferFormat == MTK_CONFIGURE_FLEX_YUV_NV12)
        output->format.format = input->format.format = CAM_FORMAT_YUV_420_NV12;
    else
        output->format.format = input->format.format = CAM_FORMAT_YUV_420_NV21;

    MLOGD(Mia2LogGroupPlugin, "input.format.format 0x%x output.format.format= 0x%x\n",
          input->format.format, output->format.format);

    struct MiImageBuffer inputdumpFrame, outputdumpFrame;
    imageParamsToMiBuffer(&inputdumpFrame, input);
    imageParamsToMiBuffer(&outputdumpFrame, output);

    MLOGI(Mia2LogGroupPlugin,
          "MIAfym:: %s:%d inputdumpFrame width: %d, height: %d, stride: %d, scanline: %d, "
          "outputdumpFrame stride: %d scanline: %d",
          __func__, __LINE__, inputdumpFrame.width, inputdumpFrame.height, inputdumpFrame.stride,
          inputdumpFrame.scanline, outputdumpFrame.stride, outputdumpFrame.scanline);

    if (mSkinBeautifier != NULL && m_beautyEnabled == true) {
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

        int result = mSkinBeautifier->Process(&inputFrame, &outputFrame, mOrientation, mRegion,
                                              m_BeautyMode, m_faceAnalyze.faceNum);

        MLOGI(Mia2LogGroupPlugin,
              "MIAfym:: mSkinBeautifier = %p, m_beautyEnabled = %d, mOrientation %d "
              " m_BeautyMode %d faceNum %d result %d",
              mSkinBeautifier, m_beautyEnabled, mOrientation, m_BeautyMode, m_faceAnalyze.faceNum,
              result);

        if (result != 0) {
            struct MiImageBuffer inputFrame, outputFrame;
            imageParamsToMiBuffer(&inputFrame, input);
            imageParamsToMiBuffer(&outputFrame, output);
            PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        }

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
        MLOGD(Mia2LogGroupPlugin,
              "MIAfym::mSkinBeautifier = %p, m_beautyEnabled = %d, Called MiCopyBuffer()",
              mSkinBeautifier, m_beautyEnabled);
    }

    return PROCSUCCESS;
}

void SkinBeautifierPlugin::getBeautyParams(camera_metadata_t *pMetaData)
{
    void *pData = NULL;
    const char *FaceAnalyzeResult = "xiaomi.faceAnalyzeResult.result";
    VendorMetadataParser::getVTagValue(pMetaData, FaceAnalyzeResult, &pData);
    if (NULL != pData) {
        memcpy(&m_faceAnalyze, pData, sizeof(OutputMetadataFaceAnalyze));
        MLOGD(Mia2LogGroupPlugin, "getMetaData faceNum %d", m_faceAnalyze.faceNum);
    }

    m_beautyEnabled = false;

    int beautyFeatureCounts = mBeautyFeatureParams.featureCounts;

    for (int i = 0; i < beautyFeatureCounts; i++) {
        if (mBeautyFeatureParams.beautyFeatureInfo[i].featureMask == true) {
            pData = NULL;
            VendorMetadataParser::getVTagValue(
                pMetaData, mBeautyFeatureParams.beautyFeatureInfo[i].TagName, &pData);
            if (NULL != pData) {
                mBeautyFeatureParams.beautyFeatureInfo[i].featureValue =
                    *static_cast<int32_t *>(pData);
            }
            if (m_beautyEnabled == false &&
                mBeautyFeatureParams.beautyFeatureInfo[i].featureValue != 0) {
                m_beautyEnabled = true;
            }
            MLOGI(Mia2LogGroupPlugin,
                  "MIAfym::MiaNodeSkinBeautifier ADVANCE_BEAUTY_FEATURE %16s : %d",
                  mBeautyFeatureParams.beautyFeatureInfo[i].TagName,
                  mBeautyFeatureParams.beautyFeatureInfo[i].featureValue);
        }
    }
    MLOGI(Mia2LogGroupPlugin, "MIAfym::getMetaData m_beautyEnabled %d", m_beautyEnabled);
}

int SkinBeautifierPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

void SkinBeautifierPlugin::destroy()
{
    if (mSkinBeautifier) {
        delete mSkinBeautifier;
        mSkinBeautifier = NULL;
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
        MLOGE(Mia2LogGroupPlugin, "%s:%d wrong input", __func__, __LINE__);
    }
}

void SkinBeautifierPlugin::imageParamsToOffScreen(ASVLOFFSCREEN *offScreen,
                                                  struct ImageParams *imageParams)
{
    if (imageParams->format.format == CAM_FORMAT_YUV_420_NV21)
        offScreen->u32PixelArrayFormat = ASVL_PAF_NV21;
    else
        offScreen->u32PixelArrayFormat = ASVL_PAF_NV12;
    MLOGD(Mia2LogGroupPlugin, "offScreen->u32PixelArrayFormat 0x%x",
          offScreen->u32PixelArrayFormat);
    offScreen->i32Width = imageParams->format.width;
    offScreen->i32Height = imageParams->format.height;
    for (int i = 0; i < imageParams->numberOfPlanes && i < 4; i++) {
        offScreen->pi32Pitch[i] = imageParams->planeStride;
        offScreen->ppu8Plane[i] = imageParams->pAddr[i];
    }
}
