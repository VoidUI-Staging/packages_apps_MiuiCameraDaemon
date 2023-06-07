#include "include/SkinBeautifier.h"

#include <cutils/properties.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <fstream>

#undef LOG_TAG
#define LOG_TAG              "SkinBeautifier"
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

SkinBeautifier::SkinBeautifier()
{
    m_E2EAcneEnabled = property_get_bool("persist.vendor.beauty.params.e2e", true);
    m_LogEnable = property_get_bool("persist.beauty.cap.log.enable", 1);
    MY_LOGI_IF(m_LogEnable, "enter");
    mHeight = 0;
    mfeatureCounts = 0;
    mRegion = 0;
    m_pFaceFeatures = NULL;
    m_pBeautyFeatureParams = NULL;
    m_pFeatureParams = NULL;
    mWidth = 0;
    mInitReady = false;
    mIsFrontCamera = false;
    mBeautyFullFeatureFlag = false;
    mImageOrientation = 0;
    mBeautyDimensionEnable = true;
    // consider
    mIsReLoadMakeupFilterAlpha = true;
    mFaceParsingEnable = false;
    mForeheadEnable = false;
    mEyeRobustEnable = false;
    mMouthRobustEnable = false;
    m_genderEnabled = false;
    m_mouthMaskEnabled = false;
    m_foreheadEnabled = false;
    TrueSight::SetSetGlobalResourcesPath(RESOURCES_PATH);
    mIsResetFixInfo = true;
    // consider
    // mEffectParams               = { 0 };
    m_pBeautyShotCaptureAlgorithm = new TrueSight::XRealityImage();
    m_pFaceDetector = new TrueSight::FaceDetector();
    m_pFrameInfo = new TrueSight::FrameInfo();

    // Initial algo, only once
#if defined(ZIYI_CAM)
#else
    if (NULL != m_pBeautyShotCaptureAlgorithm && NULL != m_pFaceDetector) {
        m_pBeautyShotCaptureAlgorithm->SetResourcesPath(RESOURCES_PATH);
        m_pBeautyShotCaptureAlgorithm->SetUseInnerEGLEnv(true);
        m_pBeautyShotCaptureAlgorithm->SetUseInnerSkinDetect(true);
        m_pBeautyShotCaptureAlgorithm->SetEnableE2EAcne(m_E2EAcneEnabled);

        int initResult = 0;
        initResult = m_pBeautyShotCaptureAlgorithm->Initialize();

        if (0 != initResult) {
            m_pBeautyShotCaptureAlgorithm->Release();
            m_pBeautyShotCaptureAlgorithm = NULL;

            MY_LOGI("m_pBeautyShotCaptureAlgorithm init fail!");
        }

        m_pFaceDetector->getScheduleConfig()
            .setResourcePath(RESOURCES_PATH)
            .setEnableInnerFD(false)
            .setEnableEyeRobost(true)
            .setEnableMouthRobost(true)
            .setEnableMouthMask(true)
            .setEnableGender(true)
            .setFaceLandmarkType(TrueSight::ALG_ACC_TYPE_XLARGE)
            .setEnableForehead(true);

        m_pFaceDetector->initialize();

        MY_LOGI("algo Version is %s", TrueSight::GetVersion());
        MY_LOGI_IF(m_LogEnable,
                   "create algo pointer success! m_pBeautyShotCaptureAlgorithm is %p "
                   "m_pFaceDetector is %p m_E2EAcneEnabled is %d",
                   m_pBeautyShotCaptureAlgorithm, m_pFaceDetector, m_E2EAcneEnabled);
    } else {
        MY_LOGE(
            "create algo pointer fail! m_pBeautyShotCaptureAlgorithm is %p "
            "m_pFaceDetector is %p",
            m_pBeautyShotCaptureAlgorithm, m_pFaceDetector);
    }
#endif
    MY_LOGI_IF(m_LogEnable, "finish");
}

SkinBeautifier::~SkinBeautifier()
{
    MY_LOGI_IF(m_LogEnable, "enter");
    // Destory algo pointer, only once
    if (m_pBeautyShotCaptureAlgorithm != NULL) {
        m_pBeautyShotCaptureAlgorithm->Release();
        delete m_pBeautyShotCaptureAlgorithm;
        m_pBeautyShotCaptureAlgorithm = NULL;
    }

    if (m_pFaceDetector != NULL) {
        delete m_pFaceDetector;
        m_pFaceDetector = NULL;
    }

    if (m_pFrameInfo != NULL) {
        delete m_pFrameInfo;
        m_pFrameInfo = NULL;
    }
    MY_LOGI_IF(m_LogEnable, "finish");
}

void SkinBeautifier::SetFeatureParams(BeautyFeatureParams &beautyFeatureParams, int &appModule)
{
    /* Set beauty feature params, but tag distributed by APP only have 0/1/2. However we need
        0 - Classical mode
        1 - Native mode
        2 - Front other mode
        3 - Rear other mode */
    MY_LOGI_IF(m_LogEnable, "enter");
    // consider
    bool mIsFrontCameraSwitch = false;
    int beautyMode = beautyFeatureParams.beautyFeatureMode.featureValue;
    // consider
    if (false == mIsFrontCamera && BEAUTY_EFFECT_MODE_OTHER_FRONT == beautyMode) {
        beautyMode = BEAUTY_EFFECT_MODE_OTHER_REAR;
    }

    // m_pBeautyShotCaptureAlgorithm->LoadRetouchConfig(beautyMode);
    if (BEAUTY_EFFECT_MODE_CLASSICAL == beautyMode && APP_MODULE_PORTRAIT == appModule) {
        beautyMode = BEAUTY_EFFECT_MODE_POPTRAIT_CLASSICAL;
    }
    if (BEAUTY_EFFECT_MODE_NATIVE == beautyMode && APP_MODULE_PORTRAIT == appModule) {
        beautyMode = BEAUTY_EFFECT_MODE_POPTRAIT_NATIVE;
    }

    if (beautyFeatureParams.beautyFeatureMode.lastFeatureValue != beautyMode) {
        MY_LOGI("beautyMode: %d, lastBeautyMode: %d", beautyMode,
                beautyFeatureParams.beautyFeatureMode.lastFeatureValue);
        if (BEAUTY_EFFECT_MODE_CLASSICAL == beautyMode || BEAUTY_EFFECT_MODE_NATIVE == beautyMode) {
            mIsFrontCameraSwitch = true;
        }
        LoadBeautyResources(beautyMode);
        ResetAllParameters(beautyFeatureParams);
        beautyFeatureParams.beautyFeatureMode.lastFeatureValue = beautyMode;
    }

    int makeupMode = beautyFeatureParams.beautyMakeupMode.featureValue;
    if (makeupMode) {
        mFaceParsingEnable = true;
    } else {
        mFaceParsingEnable = false;
    }

    // consider
    if (true == mIsFrontCameraSwitch && BEAUTY_STYLE_MODE_NATIVE != makeupMode) {
        LoadMakeupResources(beautyMode, makeupMode);
        ResetAllParameters(beautyFeatureParams);
    }
    if (beautyFeatureParams.beautyMakeupMode.lastFeatureValue != makeupMode) {
        LoadMakeupResources(beautyMode, makeupMode);
        ResetAllParameters(beautyFeatureParams);
        beautyFeatureParams.beautyMakeupMode.lastFeatureValue = makeupMode;
    }

    if (mRegion == REGION_CN) {
        int lightMode = beautyFeatureParams.beautyLightMode.featureValue;
        if (true == mIsFrontCameraSwitch && BEAUTY_STYLE_MODE_NATIVE != lightMode) {
            LoadLightResources(beautyMode, lightMode);
            ResetAllParameters(beautyFeatureParams);
        }
        if (beautyFeatureParams.beautyLightMode.lastFeatureValue != lightMode) {
            MY_LOGI("lightMode: %d, lastlightMode: %d", lightMode,
                    beautyFeatureParams.beautyLightMode.lastFeatureValue);
            LoadLightResources(beautyMode, lightMode);
            ResetAllParameters(beautyFeatureParams);
            beautyFeatureParams.beautyLightMode.lastFeatureValue = lightMode;
        }
    }

    int skinSmoothRatio = beautyFeatureParams.beautyFeatureInfo[0].featureValue;
    for (int i = 0; i < beautyFeatureParams.featureCounts; i++) {
        if (TrueSight::kParameterFlag_FaceRetouch ==
            beautyFeatureParams.beautyFeatureInfo[i].featureFlag) {
            if (beautyFeatureParams.beautyFeatureInfo[i].featureValue <= 40) {
                beautyFeatureParams.fixedBeautyFeatureInfo[1].featureValue =
                    beautyFeatureParams.beautyFeatureInfo[i].featureValue * 2.5;
            } else {
                beautyFeatureParams.fixedBeautyFeatureInfo[1].featureValue = 100;
            }
        }

        // When the makeup button is not turned on, the brighteye param is set to 0;
        // if turn on,capture mode default 40
        if (TrueSight::kParameterFlag_MakeupFilterAlpha ==
            beautyFeatureParams.beautyFeatureInfo[i].featureFlag) {
            if (beautyFeatureParams.beautyFeatureInfo[i].featureValue == 0) {
                beautyFeatureParams.fixedBeautyFeatureInfo[0].featureValue = 0;
            } else {
                beautyFeatureParams.fixedBeautyFeatureInfo[0].featureValue = 40;
                mEyeRobustEnable = true;
                mMouthRobustEnable = true;
            }
        }

        // eyerobust, mouthrobust and forehead enable follow preview
        if (beautyFeatureParams.beautyFeatureInfo[i].featureValue != 0) {
            if (i >= FOREHEAD_DETECT_BOUNDARY) {
                mForeheadEnable = true;
                if (TrueSight::kParameterFlag_FacialRefine_EyeZoom ==
                    beautyFeatureParams.beautyFeatureInfo[i].featureFlag) {
                    mEyeRobustEnable = true;
                } else if (TrueSight::kParameterFlag_FacialRefine_MouthSize ==
                           beautyFeatureParams.beautyFeatureInfo[i].featureFlag) {
                    mMouthRobustEnable = true;
                }
            }
        }

        SetChangedEffectParamsValue(beautyFeatureParams.beautyFeatureInfo[i], false, beautyMode,
                                    false);
    }

    if (true == mBeautyDimensionEnable) {
        mIsReLoadMakeupFilterAlpha = true;
        mIsResetFixInfo = true;
        for (int i = 0; i < beautyFeatureParams.fixedFeatureCounts; i++) {
            SetChangedEffectParamsValue(beautyFeatureParams.fixedBeautyFeatureInfo[i], false,
                                        beautyMode, false);
        }
    } else if (true == mIsResetFixInfo) {
        mIsResetFixInfo = false;
        for (int i = 0; i < beautyFeatureParams.fixedFeatureCounts; i++) {
            SetChangedEffectParamsValue(beautyFeatureParams.fixedBeautyFeatureInfo[i], false,
                                        beautyMode, true);
        }
    }

    for (int i = 0; i < beautyFeatureParams.beautyRelatedSwitchesCounts; i++) {
        SetChangedEffectParamsValue(beautyFeatureParams.beautyRelatedSwitchesInfo[i], true,
                                    beautyMode, false);
    }

    if (false == mBeautyDimensionEnable && true == mIsReLoadMakeupFilterAlpha) {
        beautyFeatureParams.beautyMakeupInfo[0].lastFeatureValue = -1;
        mIsReLoadMakeupFilterAlpha = false;
        MY_LOGI("ReLoadMakeupFilterAlpha");
    }
    if (BEAUTY_STYLE_MODE_NATIVE != makeupMode) {
        for (int i = 0; i < beautyFeatureParams.beautyMakeupCounts; i++) {
            SetChangedEffectParamsValue(beautyFeatureParams.beautyMakeupInfo[i], false, beautyMode,
                                        false);
        }
        MY_LOGI("beautyMakeupInfo");
    }

    MY_LOGI("makeupMode = %d beautyMode:: %d, app_module: %d BEAUTY_STYLE_MODE_NATIVE = %d",
            makeupMode, beautyMode, appModule, BEAUTY_STYLE_MODE_NATIVE);
    MY_LOGI_IF(m_LogEnable, "finish");
}

void SkinBeautifier::SetFrameInfo(CFrameInfo &frameInfo)
{
    MY_LOGI_IF(m_LogEnable, "enter");
    m_pFrameInfo->setISO(frameInfo.iso);
    m_pFrameInfo->setLuxIndex(frameInfo.nLuxIndex);
    m_pFrameInfo->setFlashTag(frameInfo.nFlashTag);
    m_pFrameInfo->setExposure(frameInfo.fExposure);
    m_pFrameInfo->setAdrc(frameInfo.fAdrc);
    m_pFrameInfo->setColorTemperature(frameInfo.nColorTemperature);

    if (true == mIsFrontCamera) {
        m_pFrameInfo->setCameraType(TrueSight::CameraType::kCameraFront);
    } else {
        m_pFrameInfo->setCameraType(TrueSight::CameraType::kCameraRear);
    }
    MY_LOGI_IF(m_LogEnable, "finish");
}

bool SkinBeautifier::Initialize(bool isFront, int region)
{
    MY_LOGI_IF(m_LogEnable, "enter");
    int initResult = 0;
    mRegion = region;
    if (NULL != m_pBeautyShotCaptureAlgorithm && NULL != m_pFaceDetector) {
        m_pBeautyShotCaptureAlgorithm->SetResourcesPath(RESOURCES_PATH);
        m_pBeautyShotCaptureAlgorithm->SetUseInnerEGLEnv(true);
        m_pBeautyShotCaptureAlgorithm->SetUseInnerSkinDetect(true);
        m_pBeautyShotCaptureAlgorithm->SetEnableE2EAcne(m_E2EAcneEnabled);
        //去杂毛
        m_pBeautyShotCaptureAlgorithm->SetEnableAIEffect(
            TrueSight::AIEffectType::kAIEffectType_E2Eeyebrow, true);
        if (!isFront) {
            m_pBeautyShotCaptureAlgorithm->SetEnableAIEffect(
                TrueSight::AIEffectType::kAIEffectType_E2Eacne, true);
            m_pBeautyShotCaptureAlgorithm->SetEnableAIEffect(
                TrueSight::AIEffectType::kAIEffectType_E2Ewrinkle, false);
            m_pBeautyShotCaptureAlgorithm->SetEnableAIEffect(
                TrueSight::AIEffectType::kAIEffectType_E2EskinUnify, false);
        }
        initResult = m_pBeautyShotCaptureAlgorithm->Initialize();

        if (0 != initResult) {
            m_pBeautyShotCaptureAlgorithm->Release();
            m_pBeautyShotCaptureAlgorithm = NULL;

            MY_LOGE("m_pBeautyShotCaptureAlgorithm init fail!");
            return false;
        }
        if (isFront) {
            m_pFaceDetector->getScheduleConfig()
                .setResourcePath(RESOURCES_PATH)
                .setEnableInnerFD(false)
                .setEnableEyeRobost(true)
                .setEnableMouthRobost(true)
                .setEnableMouthMask(true)
                .setEnableGender(true)
                .setFaceLandmarkType(TrueSight::ALG_ACC_TYPE_XLARGE)
                .setEnableNPU(true)
                .setEnableFaceParsing(true)
                .setEnableForehead(true)
                .setEnableFace3D(true);
            ALOGI("Initialize() 4.1");
        } else {
            m_pFaceDetector->getScheduleConfig()
                .setResourcePath(RESOURCES_PATH)
                .setEnableInnerFD(false)
                .setEnableEyeRobost(false)
                .setEnableMouthRobost(false)
                .setEnableMouthMask(false)
                .setEnableGender(false)
                .setMaxFaceCount(FD_Max_FACount)
                .setFaceLandmarkType(TrueSight::ALG_ACC_TYPE_XLARGE);
            ALOGI("Initialize() 4.2");
        }
        m_pFaceDetector->initialize();
        MY_LOGI("algo Version is %s", TrueSight::GetVersion());
        MY_LOGI_IF(
            m_LogEnable,
            "TrueSight capture: create algo pointer success! m_pBeautyShotCaptureAlgorithm is %p "
            "m_pFaceDetector is %p m_E2EAcneEnabled is %d",
            m_pBeautyShotCaptureAlgorithm, m_pFaceDetector, m_E2EAcneEnabled);
    } else {
        MY_LOGE(
            "TrueSight capture: create algo pointer fail! m_pBeautyShotCaptureAlgorithm is %p "
            "m_pFaceDetector is %p",
            m_pBeautyShotCaptureAlgorithm, m_pFaceDetector);
    }
    return initResult;
}

int SkinBeautifier::Process(struct MiImageBuffer *input, struct MiImageBuffer *output,
                            MiFDBeautyParam &faceDetectedParams, int mode)
{
    int result = -1;
    if (mode < BEAUTY_MODE_PREVIEW) {
        result = ProcessCapture(input, output, faceDetectedParams);
    } else {
        result = ProcessPreview(input, output);
    }
    return result;
}

int SkinBeautifier::ProcessPreview(struct MiImageBuffer *input, struct MiImageBuffer *output)
{
    return 0;
}

bool SkinBeautifier::ProcessCapture(struct MiImageBuffer *input, struct MiImageBuffer *output,
                                    MiFDBeautyParam &faceDetectedParams)
{
    MY_LOGI_IF(m_LogEnable, "enter");
    if (NULL == m_pBeautyShotCaptureAlgorithm || NULL == m_pFaceDetector) {
        MY_LOGE(
            "processCapture fail! m_pBeautyShotCaptureAlgorithm is %p"
            "m_pFaceDetector is %p",
            m_pBeautyShotCaptureAlgorithm, m_pFaceDetector);
        return false;
    }

    // Set frame info
    m_pBeautyShotCaptureAlgorithm->SetFrameInfo(*m_pFrameInfo);

    // Image format convert
    TrueSight::Image srcImage =
        TrueSight::Image::create_nv21(input->width, input->height, input->plane[0], input->plane[1],
                                      mImageOrientation, input->stride, input->stride);

    TrueSight::Image dstImage = TrueSight::Image::create_nv21(
        output->width, output->height, output->plane[0], output->plane[1], mImageOrientation,
        output->stride, output->stride);
    // Face detect
    double faceDetectStartTime = GetCurrentSecondValue();

    TrueSight::FaceFeatures faceFeatures;
    MY_LOGI_IF(m_LogEnable, "mIsFrontCamera = %d", mIsFrontCamera);

    if (true == mIsFrontCamera) {
#if defined(ZIYI_CAM)
        m_pFaceDetector->getScheduleConfig()
            .setEnableInnerFD(false)
            .setEnableNPU(mFaceParsingEnable)
            .setEnableFaceParsing(mFaceParsingEnable)
            .setEnableEyeRobost(mEyeRobustEnable)
            .setEnableMouthRobost(mMouthRobustEnable)
            .setEnableMouthMask(mFaceParsingEnable)
            .setEnableForehead(mForeheadEnable);

#else
        m_pFaceDetector->getScheduleConfig().setEnableInnerFD(false).setEnableGender(true);
#endif

        if (0 == faceDetectedParams.miFaceCountNum) {
#if defined(ZIYI_CAM)
            m_pFaceDetector->getScheduleConfig()
                .setEnableInnerFD(true)
                .setEnableNPU(mFaceParsingEnable)
                .setEnableFaceParsing(mFaceParsingEnable)
                .setEnableGender(true);
#else
            m_pFaceDetector->getScheduleConfig().setEnableInnerFD(true).setEnableGender(true);
#endif
        }
        faceFeatures.setFaceCount(faceDetectedParams.miFaceCountNum);
        MY_LOGI_IF(m_LogEnable, "faceCount origin %d", faceFeatures.getFaceCount());
        for (int i = 0; i < faceDetectedParams.miFaceCountNum && i < FD_Max_FACount; i++) {
            TrueSight::Rect2f box = {(float)faceDetectedParams.face_roi[i].left,
                                     (float)faceDetectedParams.face_roi[i].top,
                                     (float)faceDetectedParams.face_roi[i].width,
                                     (float)faceDetectedParams.face_roi[i].height};
            faceFeatures.setFaceBox(i, box);

            float roll = faceDetectedParams.rollangle[i];
            faceFeatures.setRollRadian(i, roll);
            if (true == faceDetectedParams.isGenderEnabled) {
                float maleScore = faceDetectedParams.male_score[i];
                float femaleScore = faceDetectedParams.female_score[i];
                faceFeatures.setGender(i, maleScore, femaleScore);
                MY_LOGI_IF(m_LogEnable, "maleScore: %f, femaleScore: %f", maleScore, femaleScore);
            }
            if (true == faceDetectedParams.isForeheadEnabled) {
                faceFeatures.setForeheadLandmark2D(i, faceDetectedParams.foreheadPoints[i].points,
                                                   MAX_FOREHEAD_POINT_NUM);
                MY_LOGI_IF(m_LogEnable, "setForeheadLandmark2D");
            }
            faceFeatures.setFaceID(i, faceDetectedParams.faceid[i]);
        }

        m_pFaceDetector->initialize();
        m_pFaceDetector->detect(srcImage, faceFeatures);
    } else {
        m_pFaceDetector->getScheduleConfig().setMaxFaceCount(FD_Max_FACount);

#if defined(ZIYI_CAM)
        if (mIsFrontCamera == false) {
            faceFeatures.setFaceCount(faceDetectedParams.miFaceCountNum);

            if (0 == faceDetectedParams.miFaceCountNum) {
                m_pFaceDetector->getScheduleConfig().setEnableInnerFD(true);
            }
            for (int i = 0; i < faceDetectedParams.miFaceCountNum && i < FD_Max_FACount; i++) {
                TrueSight::Rect2f faceBox = {(float)faceDetectedParams.face_roi[i].left,
                                             (float)faceDetectedParams.face_roi[i].top,
                                             (float)faceDetectedParams.face_roi[i].width,
                                             (float)faceDetectedParams.face_roi[i].height};
                faceFeatures.setFaceBox(i, faceBox);

                MY_LOGI_IF(m_LogEnable, "index: %d, faceBox orgin [%f %f %f %f]", i, faceBox.x,
                           faceBox.y, faceBox.width, faceBox.height);
            }
        }

#endif
        m_pFaceDetector->initialize();
        m_pFaceDetector->detect(srcImage, faceFeatures);
    }

    // syg
    TSConvert2MIFaceFeatures(faceFeatures, faceDetectedParams);
    double mialgoProcessStartTime = GetCurrentSecondValue();
    MY_LOGI_IF(m_LogEnable, "faceCount %d faceDetectTime %f", faceFeatures.getFaceCount(),
               GetCurrentSecondValue() - faceDetectStartTime);

    // Mialgo process
    int processResult = 0;
    processResult =
        m_pBeautyShotCaptureAlgorithm->RunCaptureImage(&srcImage, &dstImage, &faceFeatures, NULL);
    double mialgoProcessEndTime = GetCurrentSecondValue();
    if (0 == processResult) {
        MY_LOGI("processCapture success!");
    } else {
        MY_LOGE("processCapture fail! result: %d", processResult);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // // Dump image with stride
    // char str[256];
    // snprintf(str, sizeof(str), "width%d height%d stride%d", dstImage.m_nWidth,
    // dstImage.m_nHeight, dstImage.m_nYStride); std::string debug(str); std::string file_perfix
    // = "data/vendor/camera/truesight_output_"; std::string imgfile = file_perfix + debug +
    // ".nv12";

    // FILE *fp = fopen(imgfile.c_str(), "w+");
    // if (dstImage.m_pYData) {
    //     fwrite(dstImage.m_pYData, dstImage.m_nYStride * dstImage.m_nHeight, 1, fp);
    // }
    // if (dstImage.m_pUVData) {
    //     fwrite(dstImage.m_pUVData, dstImage.m_nUVStride * dstImage.m_nHeight / 2, 1, fp);
    // }
    // fclose(fp);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    MY_LOGI("TrueSight capture: processCapture cost time: %f",
            mialgoProcessEndTime - mialgoProcessStartTime);
    MY_LOGI_IF(m_LogEnable, "finish");
    return true;
}

void SkinBeautifier::SetImageOrientation(int orientation)
{
    if (true == mIsFrontCamera) {
        switch (orientation) {
        case 0:
            mImageOrientation = 7;
            break;
        case 90:
            mImageOrientation = 4;
            break;
        case 180:
            mImageOrientation = 5;
            break;
        case 270:
            mImageOrientation = 2;
            break;
        default:
            mImageOrientation = 7;
            break;
        }
    } else {
        switch (orientation) {
        case 0:
            mImageOrientation = 6;
            break;
        case 90:
            mImageOrientation = 3;
            break;
        case 180:
            mImageOrientation = 8;
            break;
        case 270:
            mImageOrientation = 1;
            break;
        default:
            mImageOrientation = 6;
            break;
        }
    }

    MY_LOGI_IF(m_LogEnable, "Set image orientation is %d", mImageOrientation);
}

double SkinBeautifier::GetCurrentSecondValue()
{
    timeval currentTime;
    gettimeofday(&currentTime, NULL);
    double secondValue =
        ((double)currentTime.tv_sec * 1000.0) + ((double)currentTime.tv_usec / 1000.0);
    return secondValue;
}

void SkinBeautifier::SetCameraPosition(bool isFrontCamera)
{
    mIsFrontCamera = isFrontCamera;
}

void SkinBeautifier::ResetPreviewOutput()
{
    MY_LOGI_IF(m_LogEnable, "start");
    // if (mBeautyShotVideoAlgorithm != NULL) {
    //     mBeautyShotVideoAlgorithm->ResetOutput();
    // }
    MY_LOGI_IF(m_LogEnable, "finish");
}

void SkinBeautifier::LoadBeautyResources(int beautyMode)
{
    m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_packet,
                                                    NULL);
    m_pBeautyShotCaptureAlgorithm->RefreshEffectConfig();
    char configPath[MAX_RESOURCES_PATH_LEN];
    if (BEAUTY_EFFECT_MODE_CLASSICAL == beautyMode) {
        snprintf(configPath, MAX_RESOURCES_PATH_LEN, "%s%s", RESOURCES_PATH, CLASSICAL_PATH);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_packet,
                                                        configPath);
    } else if (BEAUTY_EFFECT_MODE_NATIVE == beautyMode) {
        snprintf(configPath, MAX_RESOURCES_PATH_LEN, "%s%s", RESOURCES_PATH, NATIVE_PATH);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_packet,
                                                        configPath);
    } else if (BEAUTY_EFFECT_MODE_OTHER_FRONT == beautyMode) {
#if defined(ZIYI_CAM)
        if (mRegion == REGION_CN) {
            snprintf(configPath, MAX_RESOURCES_PATH_LEN, "%s%s", RESOURCES_PATH,
                     OTHER_FRONT_CLASSICAL_PATH);
        } else {
            snprintf(configPath, MAX_RESOURCES_PATH_LEN, "%s%s", RESOURCES_PATH,
                     OTHER_FRONT_NATIVE_PATH);
        }
#else
        snprintf(configPath, MAX_RESOURCES_PATH_LEN, "%s%s", RESOURCES_PATH, OTHER_FRONT_PATH);
#endif
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_packet,
                                                        configPath);
    } else if (BEAUTY_EFFECT_MODE_OTHER_REAR == beautyMode) {
        snprintf(configPath, MAX_RESOURCES_PATH_LEN, "%s%s", RESOURCES_PATH, OTHER_REAR_PATH);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_packet,
                                                        configPath);
    } else if (BEAUTY_EFFECT_MODE_POPTRAIT_CLASSICAL == beautyMode) {
        snprintf(configPath, MAX_RESOURCES_PATH_LEN, "%s%s", RESOURCES_PATH, CLASSICAL_PATH);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_packet,
                                                        configPath);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(
            TrueSight::EffectIndex::kEffectIndex_ColorTone, NULL);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_Makeup,
                                                        NULL);
    } else if (BEAUTY_EFFECT_MODE_POPTRAIT_NATIVE == beautyMode) {
        snprintf(configPath, MAX_RESOURCES_PATH_LEN, "%s%s", RESOURCES_PATH, NATIVE_PATH);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_packet,
                                                        configPath);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(
            TrueSight::EffectIndex::kEffectIndex_ColorTone, NULL);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_Makeup,
                                                        NULL);
    }
    MY_LOGI_IF(m_LogEnable, "LoadEffectConfig path is [%s]", configPath);
    m_pBeautyShotCaptureAlgorithm->RefreshEffectConfig();
}

int SkinBeautifier::SetChangedEffectParamsValue(BeautyFeatureInfo &info, bool isSwitches,
                                                int beautyMode, bool isReset)
{
    float adjustFeatureValue = (float)info.featureValue;
    TrueSight::ParameterFlag featureFlag = info.featureFlag;
    if (true == isSwitches) {
        if (info.lastFeatureValue != info.featureValue) {
            info.lastFeatureValue = info.featureValue;
            m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(featureFlag, adjustFeatureValue);
        } else {
            MY_LOGI_IF(m_LogEnable, "SetFeatureParams Flag[%22s]:%3d, FeatureValue:%d",
                       ParameterFlagToString(info.featureFlag), info.featureFlag,
                       info.featureValue);
            return 1;
        }
    } else {
        if (true == isReset) {
            adjustFeatureValue = 0.0;
            info.lastFeatureValue = -1;
        } else {
            if (gBeautyModeMask[beautyMode] ==
                    (info.applicableModeMask & gBeautyModeMask[beautyMode]) &&
                info.lastFeatureValue != info.featureValue) {
                if (true == info.needAdjusted) {
                    adjustFeatureValue = (adjustFeatureValue / 2.0 + 50.0) / 100.0;
                } else {
                    adjustFeatureValue = adjustFeatureValue / 100.0;
                }
                info.lastFeatureValue = info.featureValue;
            } else if (gBeautyModeMask[beautyMode] !=
                           (info.applicableModeMask & gBeautyModeMask[beautyMode]) &&
                       info.lastFeatureValue != info.featureValue) {
                adjustFeatureValue = 0.0;
                info.lastFeatureValue = info.featureValue;
            } else {
                MY_LOGI_IF(m_LogEnable, "SetFeatureParams Flag[%22s]:%3d, FeatureValue:%d",
                           ParameterFlagToString(info.featureFlag), info.featureFlag,
                           info.featureValue);
                return 1;
            }
        }
        m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(featureFlag, adjustFeatureValue);
    }
    MY_LOGI_IF(m_LogEnable, "SetFeatureParams Flag[%22s]:%3d, Value:%f",
               ParameterFlagToString(info.featureFlag), info.featureFlag, adjustFeatureValue);
    return 0;
}

void SkinBeautifier::ResetAllParameters(BeautyFeatureParams &beautyFeatureParams)
{
    for (int i = 0; i < beautyFeatureParams.featureCounts; i++) {
        beautyFeatureParams.beautyFeatureInfo[i].lastFeatureValue = -1;
    }
    for (int i = 0; i < beautyFeatureParams.fixedFeatureCounts; i++) {
        beautyFeatureParams.fixedBeautyFeatureInfo[i].lastFeatureValue = -1;
    }
    for (int i = 0; i < beautyFeatureParams.beautyRelatedSwitchesCounts; i++) {
        beautyFeatureParams.beautyRelatedSwitchesInfo[i].lastFeatureValue = -1;
    }

#if defined(ZIYI_CAM)
    for (int i = 0; i < beautyFeatureParams.beautyMakeupCounts; i++) {
        beautyFeatureParams.beautyMakeupInfo[i].lastFeatureValue = -1;
    }

    for (int i = 0; i < beautyFeatureParams.beautyLightCounts; i++) {
        beautyFeatureParams.beautyLightInfo[i].lastFeatureValue = -1;
    }
#endif
    MY_LOGI_IF(m_LogEnable, "ResetAllParameters");
}

#if defined(ZIYI_CAM)
void SkinBeautifier::LoadMakeupResources(int beautyMode, int makeupMode)
{
    if (BEAUTY_STYLE_MODE_NATIVE == makeupMode) {
        LoadBeautyResources(beautyMode);
        MY_LOGI_IF(m_LogEnable, "1");

    } else {
        MY_LOGI_IF(m_LogEnable, "2");
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_Makeup,
                                                        NULL);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(
            TrueSight::EffectIndex::kEffectIndex_ColorTone, NULL);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_Makeup,
                                                        GetBeautyStyleData(makeupMode));
        m_pBeautyShotCaptureAlgorithm->RefreshEffectConfig();
    }
    MY_LOGI_IF(m_LogEnable, "Load makeup resources mode is [%d]", makeupMode);
}

void SkinBeautifier::LoadLightResources(int beautyMode, int lightMode)
{
    if (LIGHT_STYLE_MODE_NATIVE == lightMode) {
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_Rule,
                                                        NULL);
        m_pBeautyShotCaptureAlgorithm->RefreshEffectConfig();
    } else {
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(
            TrueSight::EffectIndex::kEffectIndex_ColorTone, NULL);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_Rule,
                                                        NULL);
        m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(TrueSight::kParameterFlag_FilterAlpha,
                                                            0.0);
        m_pBeautyShotCaptureAlgorithm->RefreshEffectConfig();
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_Rule,
                                                        GetLightStyleData(lightMode));
        m_pBeautyShotCaptureAlgorithm->RefreshEffectConfig();
        m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(TrueSight::kParameterFlag_LightAlpha,
                                                            1.0);
    }
    MY_LOGI_IF(m_LogEnable, "Load light resources mode is [%d].", lightMode);
}
#endif

int SkinBeautifier::TSConvert2MIFaceFeatures(TrueSight::FaceFeatures faceFeatures,
                                             MiFDBeautyParam &miFDParam)
{
    int faceCount = faceFeatures.getFaceCount();
    size_t foreheadPoint = MAX_FOREHEAD_POINT_NUM;
    MY_LOGI_IF(m_LogEnable, "enter");
    miFDParam.miFaceCountNum = faceCount;
    if (faceCount <= 0) {
        return 0;
    }

    for (int i = 0; i < faceCount && i < MAX_NUMBER_OF_FACE_DETECT; i++) {
        const TrueSight::IVector<TrueSight::Point2f> *lmk_revert =
            faceFeatures.getFaceLandmark2D(i);
        memcpy(miFDParam.milandmarks[i].points, lmk_revert->data(),
               sizeof(TrueSight::Point2f) * FD_Max_Landmark);
        // for (int j = 0; j < FD_Max_Landmark; ++j ) {
        //    miFDParam.milandmarks[i].points[j].x = lmk_revert->at(j).x;
        //    miFDParam.milandmarks[i].points[j].y = lmk_revert->at(j).y;
        //}
        MY_LOGI_IF(m_LogEnable, "capture landmark %d (%f, %f, %f, %f)", i,
                   miFDParam.milandmarks[i].points[0].x, miFDParam.milandmarks[i].points[0].y,
                   miFDParam.milandmarks[i].points[1].x, miFDParam.milandmarks[i].points[1].y);
        auto lmkv_revert = faceFeatures.getFaceLandmark2DVisable(i);
        memcpy(miFDParam.milandmarksvis[i].value, lmkv_revert->data(),
               sizeof(float) * FD_Max_Landmark);
        // for (int j = 0; j < FD_Max_Landmark; ++j ) {
        //    miFDParam.milandmarksvis[i].score[j] = lmkv_revert->at(j);
        //}

        auto box = faceFeatures.getFaceBox(i);
        miFDParam.face_roi[i].left = box.x;
        miFDParam.face_roi[i].top = box.y;
        miFDParam.face_roi[i].width = box.width;
        miFDParam.face_roi[i].height = box.height;
        MY_LOGI_IF(m_LogEnable, "capture after face %d (%f, %f, %f, %f)", i, box.x, box.y,
                   box.width, box.height);
        miFDParam.isMouthMaskEnabled = false;
        miFDParam.isForeheadEnabled = false;
        miFDParam.isGenderEnabled = false;

        if (true == m_mouthMaskEnabled) {
            auto mouthMask = faceFeatures.getMouthMask(i, miFDParam.mouthmatrix[i].mouth_matrix);
            if (!mouthMask->empty()) {
                memcpy(miFDParam.mouthmask[i].mouth_mask, mouthMask->m_pData,
                       mouthMask->m_nWidth * mouthMask->m_nHeight);
            } else {
                memset(miFDParam.mouthmask[i].mouth_mask, 0, 64 * 64);
            }
            miFDParam.isMouthMaskEnabled = true;
        }

        MY_LOGI_IF(m_LogEnable, "forehead %d (%f, %f, %f, %f)", i,
                   miFDParam.foreheadPoints[i].points[0], miFDParam.foreheadPoints[i].points[1],
                   miFDParam.foreheadPoints[i].points[2], miFDParam.foreheadPoints[i].points[3]);

        if (true == m_foreheadEnabled) {
            const float *foreheadPoints = NULL;
            foreheadPoints = faceFeatures.getForeheadLandmark2D(i, foreheadPoint);
            if (NULL != foreheadPoints) {
                memcpy(miFDParam.foreheadPoints[i].points, foreheadPoints,
                       MAX_FOREHEAD_POINT_NUM * sizeof(float) / sizeof(char) * 2);
            } else {
                memset(miFDParam.foreheadPoints[i].points, 0,
                       MAX_FOREHEAD_POINT_NUM * sizeof(float) / sizeof(char) * 2);
            }
            miFDParam.isForeheadEnabled = true;
        }

        if (true == m_genderEnabled) {
            faceFeatures.getGender(i, miFDParam.male_score[i], miFDParam.female_score[i]);
            miFDParam.isGenderEnabled = true;
        }

        miFDParam.confidence[i] = faceFeatures.getFaceConfidence(i);
        miFDParam.rollangle[i] = faceFeatures.getRollRadian(i);
        miFDParam.faceid[i] = faceFeatures.getFaceID(i);
        miFDParam.lmk_accuracy_type[i] = faceFeatures.getFaceLandmarkAccuracyType(i);

        faceFeatures.getAgeDivide(i, miFDParam.adult_score[i], miFDParam.child_score[i]);
        miFDParam.age[i] = faceFeatures.getAge(i);
        miFDParam.size_adjusted = true;
    }
    MY_LOGI_IF(m_LogEnable, "finish");
    return 0;
}