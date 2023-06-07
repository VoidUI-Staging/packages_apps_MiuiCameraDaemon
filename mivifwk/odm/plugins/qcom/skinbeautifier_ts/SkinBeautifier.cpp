#include "include/SkinBeautifier.h"

#include <cutils/properties.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <fstream>
#include "arcsoft_debuglog.h"

#define REGION_CN 1

static bool E2EAcneEnabled = property_get_bool("persist.vendor.beauty.params.e2e", true);

SkinBeautifier::SkinBeautifier()
{
    mInitReady = false;
    mIsFrontCamera = false;
    mBeautyFullFeatureFlag = false;
    mImageOrientation = 0;
    mBeautyDimensionEnable = false;
    m_pBeautyShotCaptureAlgorithm = NULL;
    m_pFaceDetector = NULL;
    m_pFrameInfo = NULL;
    arcInitCommonConfigSpec("persist.vendor.camera.truesight.log.capture");

    mIsResetFixInfo = true;

}

int SkinBeautifier::Init(bool isFrontCamera)
{
    // Create algo pointer, only once
    m_pBeautyShotCaptureAlgorithm = new TrueSight::XRealityImage();
    m_pFaceDetector = new TrueSight::FaceDetector();
    m_pFrameInfo = new TrueSight::FrameInfo();

    // Initial algo, only once
    if (NULL != m_pBeautyShotCaptureAlgorithm && NULL != m_pFaceDetector) {
        double mialgoInitStartTime = GetCurrentSecondValue();
        m_pBeautyShotCaptureAlgorithm->SetResourcesPath(RESOURCES_PATH);
        m_pBeautyShotCaptureAlgorithm->SetUseInnerEGLEnv(true);
        m_pBeautyShotCaptureAlgorithm->SetUseInnerSkinDetect(true);
        // m_pBeautyShotCaptureAlgorithm->SetEnableE2EAcne(E2EAcneEnabled);

        // add aieffect: front: e2e(NPU), wrinkle(NPU), skinUnify(GPU); rear: e2e(NPU)
        if (true == isFrontCamera) {
            m_pBeautyShotCaptureAlgorithm->SetEnableAIEffect(
                TrueSight::AIEffectType::kAIEffectType_E2Eacne, true);
            m_pBeautyShotCaptureAlgorithm->SetHardwareAccelerationType(
                TrueSight::AIEffectType::kAIEffectType_E2Eacne,
                TrueSight::HardwareAccelerationType::kHardwareAccelerationType_NPU);
            m_pBeautyShotCaptureAlgorithm->SetEnableAIEffect(
                TrueSight::AIEffectType::kAIEffectType_E2Ewrinkle, true);
            m_pBeautyShotCaptureAlgorithm->SetHardwareAccelerationType(
                TrueSight::AIEffectType::kAIEffectType_E2Ewrinkle,
                TrueSight::HardwareAccelerationType::kHardwareAccelerationType_NPU);
            m_pBeautyShotCaptureAlgorithm->SetEnableAIEffect(
                TrueSight::AIEffectType::kAIEffectType_E2EskinUnify, true);
            m_pBeautyShotCaptureAlgorithm->SetHardwareAccelerationType(
                TrueSight::AIEffectType::kAIEffectType_E2EskinUnify,
                TrueSight::HardwareAccelerationType::kHardwareAccelerationType_GPU);
        } else {
            m_pBeautyShotCaptureAlgorithm->SetEnableAIEffect(
                TrueSight::AIEffectType::kAIEffectType_E2Eacne, true);
            m_pBeautyShotCaptureAlgorithm->SetHardwareAccelerationType(
                TrueSight::AIEffectType::kAIEffectType_E2Eacne,
                TrueSight::HardwareAccelerationType::kHardwareAccelerationType_NPU);
            m_pBeautyShotCaptureAlgorithm->SetEnableAIEffect(
                TrueSight::AIEffectType::kAIEffectType_E2Ewrinkle, false);
            m_pBeautyShotCaptureAlgorithm->SetEnableAIEffect(
                TrueSight::AIEffectType::kAIEffectType_E2EskinUnify, false);
        }

        int initResult = 0;
        initResult = m_pBeautyShotCaptureAlgorithm->Initialize();

        if (0 != initResult) {
            m_pBeautyShotCaptureAlgorithm->Release();
            m_pBeautyShotCaptureAlgorithm = NULL;

            MLOGE_BF("TrueSight capture: m_pBeautyShotCaptureAlgorithm init fail!");
            return 1;
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
        double mialgoInitEndTime = GetCurrentSecondValue();
        MLOGD_BF("TrueSight capture: algo Version is %s", TrueSight::GetVersion());
        MLOGD_BF(
            "TrueSight capture: create algo pointer success! m_pBeautyShotCaptureAlgorithm is %p "
            "m_pFaceDetector is %p E2EAcneEnabled is %d, init cost time = %f",
            m_pBeautyShotCaptureAlgorithm, m_pFaceDetector, E2EAcneEnabled,
            mialgoInitEndTime - mialgoInitStartTime);
        return 0;
    } else {
        MLOGE_BF(
            "TrueSight capture: create algo pointer fail! m_pBeautyShotCaptureAlgorithm is %p "
            "m_pFaceDetector is %p",
            m_pBeautyShotCaptureAlgorithm, m_pFaceDetector);
        return 1;
    }
}

SkinBeautifier::~SkinBeautifier()
{
    MLOGD_BF("TrueSight capture: SkinBeautifier destruct");
    // Destory algo pointer, only once
    double mialgoDestroyStartTime = GetCurrentSecondValue();
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
    double mialgoDestroyEndTime = GetCurrentSecondValue();
    MLOGD_BF("TrueSight capture: destroy cost time = %f",
             mialgoDestroyEndTime - mialgoDestroyStartTime);
}

void SkinBeautifier::UnInit()
{
    MLOGD_BF("TrueSight capture: SkinBeautifier destruct in UnInit");
    double mialgoUninitStartTime = GetCurrentSecondValue();
    if (m_pBeautyShotCaptureAlgorithm != NULL) {
        m_pBeautyShotCaptureAlgorithm->Release();
        delete m_pBeautyShotCaptureAlgorithm;
        m_pBeautyShotCaptureAlgorithm = NULL;
    } else {
        MLOGD_BF(
            "TrueSight capture: SkinBeautifier destruct fail! m_pBeautyShotCaptureAlgorithm is "
            "NULL");
    }

    if (m_pFaceDetector != NULL) {
        delete m_pFaceDetector;
        m_pFaceDetector = NULL;
    } else {
        MLOGD_BF("TrueSight capture: SkinBeautifier destruct fail! m_pFaceDetector is NULL");
    }

    if (m_pFrameInfo != NULL) {
        delete m_pFrameInfo;
        m_pFrameInfo = NULL;
    } else {
        MLOGD_BF("TrueSight capture: SkinBeautifier destruct fail! m_pFrameInfo is NULL");
    }
    double mialgoUninitEndTime = GetCurrentSecondValue();
    MLOGD_BF("TrueSight capture: uninit cost time = %f",
             mialgoUninitEndTime - mialgoUninitStartTime);
}

void SkinBeautifier::SetFeatureParams(BeautyFeatureParams &beautyFeatureParams, int &appModule)
{
    /* Set beauty feature params, but tag distributed by APP only have 0/1/2. However we need
        0 - Classical mode
        1 - Native mode
        2 - Front other mode
        3 - Rear other mode */
    int beautyMode = beautyFeatureParams.beautyFeatureMode.featureValue;
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

    bool mIsFrontCameraSwitch = false;
    if (beautyFeatureParams.beautyFeatureMode.lastFeatureValue != beautyMode) {
        MLOGD_BF("TrueSight capture: beautyMode: %d, lastBeautyMode: %d", beautyMode,
                 beautyFeatureParams.beautyFeatureMode.lastFeatureValue);
        if (BEAUTY_EFFECT_MODE_CLASSICAL == beautyMode || BEAUTY_EFFECT_MODE_NATIVE == beautyMode) {
            mIsFrontCameraSwitch = true;
        }
        LoadBeautyResources(beautyMode);
        ResetAllParameters(beautyFeatureParams);
        beautyFeatureParams.beautyFeatureMode.lastFeatureValue = beautyMode;
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
            if (beautyFeatureParams.beautyFeatureInfo[i].featureValue != 0) {
                m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(
                    TrueSight::kParameterFlag_E2Ewrinkle, 1);
            } else {
                m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(
                    TrueSight::kParameterFlag_E2Ewrinkle, 0);
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
            }
        }

        SetChangedEffectParamsValue(beautyFeatureParams.beautyFeatureInfo[i], false, beautyMode,
                                    false);
    }

    // portrait rearmode  stereoPerception && enlargeEye follow skinSmooth value
    if (APP_MODULE_PORTRAIT == appModule && false == mIsFrontCamera) {
        if (0 != beautyFeatureParams.beautyFeatureInfo[0].featureValue) {
            beautyFeatureParams.beautyFeatureInfo[2].featureValue =
                beautyFeatureParams.beautyFeatureInfo[0].featureValue;
            beautyFeatureParams.beautyFeatureInfo[7].featureValue =
                beautyFeatureParams.beautyFeatureInfo[0].featureValue;
        }
        SetChangedEffectParamsValue(beautyFeatureParams.beautyFeatureInfo[2], false, beautyMode,
                                    false);
        SetChangedEffectParamsValue(beautyFeatureParams.beautyFeatureInfo[7], false, beautyMode,
                                    false);
    }

    if (true == mBeautyDimensionEnable) {
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

#if 0
    if (0 != skinSmoothRatio) {
        if (true == mIsFrontCamera) {
            m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(TrueSight::kParameterFlag_Spotless,
                                                                1.0);
            switch (appModule) {
            case APP_MODULE_DEFAULT_CAPTURE:
                if (0 == beautyMode) { // CLASSICAL MODE
                    m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(
                        TrueSight::kParameterFlag_LightenPouch, CLASSICAL_FIXED_VALUE);
                    m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(
                        TrueSight::kParameterFlag_Rhytidectomy, CLASSICAL_FIXED_VALUE);
                } else if (1 == beautyMode) { // NATIVE MODE
                    m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(
                        TrueSight::kParameterFlag_LightenPouch, NATIVE_FIXED_VALUE);
                    m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(
                        TrueSight::kParameterFlag_Rhytidectomy, NATIVE_FIXED_VALUE);
                } else { // OTHER MODE
                    MLOGE_BF(
                        "TrueSight capture: beautyMode is error value %d! in MODE_DEFAULT_CAPTURE",
                        beautyMode);
                }
                break;
            case APP_MODULE_PORTRAIT:
                m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(
                    TrueSight::kParameterFlag_SkinToneMapping, BEAUTY_WHITEN_FIXED_VALUE);
            case APP_MODULE_SUPER_NIGHT:
            default:
                break;
            }
        } else {
            switch (appModule) {
            case APP_MODULE_DEFAULT_CAPTURE:
                m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(
                    TrueSight::kParameterFlag_Spotless, 1.0);
                break;
            case APP_MODULE_PORTRAIT:
                m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(
                    TrueSight::kParameterFlag_SkinToneMapping, BEAUTY_WHITEN_FIXED_VALUE);
                m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(
                    TrueSight::kParameterFlag_Spotless, 1.0);
            case APP_MODULE_SUPER_NIGHT:
            default:
                break;
            }
        }
    }
#endif

    MLOGD_BF("TrueSight capture: beautyMode: %d, app_module: %d", beautyMode, appModule);
}

void SkinBeautifier::SetFrameInfo(FrameInfo &frameInfo)
{
    MLOGI_BF("TrueSight capture: SkinBeautifier SetFrameInfo");

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
}

int SkinBeautifier::Process(struct MiImageBuffer *input, struct MiImageBuffer *output,
                            MiFDBeautyParam &faceDetectedParams, int sdkYUVMode)
{
    int result = -1;
    result = ProcessCapture(input, output, faceDetectedParams, sdkYUVMode);
    return result;
}

int SkinBeautifier::ProcessPreview(struct MiImageBuffer *input, struct MiImageBuffer *output)
{
    return 0;
}

int SkinBeautifier::ProcessCapture(struct MiImageBuffer *input, struct MiImageBuffer *output,
                                   MiFDBeautyParam &faceDetectedParams, int sdkYUVMode)
{
    MLOGI_BF("TrueSight capture: SkinBeautifier ProcessCapture");

    if (NULL == m_pBeautyShotCaptureAlgorithm && NULL == m_pFaceDetector) {
        MLOGE_BF(
            "TrueSight capture: processCapture fail! m_pBeautyShotCaptureAlgorithm is %p"
            "m_pFaceDetector is %p",
            m_pBeautyShotCaptureAlgorithm, m_pFaceDetector);
        return 1;
    }

    // Set frame info
    m_pBeautyShotCaptureAlgorithm->SetFrameInfo(*m_pFrameInfo);

    // Image format convert
    TrueSight::Image srcImage;
    TrueSight::Image dstImage;
    // if thirdparty sdk , use nv21 format
    if (1 == sdkYUVMode) {
        srcImage = TrueSight::Image::create_nv21(input->width, input->height, input->plane[0],
                                                 input->plane[1], mImageOrientation, input->stride,
                                                 input->stride);
        dstImage = TrueSight::Image::create_nv21(output->width, output->height, output->plane[0],
                                                 output->plane[1], mImageOrientation,
                                                 output->stride, output->stride);
    } else {
        srcImage = TrueSight::Image::create_nv12(input->width, input->height, input->plane[0],
                                                 input->plane[1], mImageOrientation, input->stride,
                                                 input->stride);

        dstImage = TrueSight::Image::create_nv12(output->width, output->height, output->plane[0],
                                                 output->plane[1], mImageOrientation,
                                                 output->stride, output->stride);
    }
    // Face detect
    double faceDetectStartTime = GetCurrentSecondValue();

    TrueSight::FaceFeatures faceFeatures;

    if (true == mIsFrontCamera) {
        m_pFaceDetector->getScheduleConfig().setEnableInnerFD(false).setEnableGender(false);

        if (0 == faceDetectedParams.miFaceCountNum) {
            m_pFaceDetector->getScheduleConfig().setEnableInnerFD(true).setEnableGender(true);
        }
        faceFeatures.setFaceCount(faceDetectedParams.miFaceCountNum);
        MLOGD_BF("TrueSight capture: faceCount origin %d", faceFeatures.getFaceCount());
        for (int i = 0; i < faceDetectedParams.miFaceCountNum && i < FD_Max_FACount; i++) {
            TrueSight::Rect2f box = {
                faceDetectedParams.face_roi[i].left, faceDetectedParams.face_roi[i].top,
                faceDetectedParams.face_roi[i].width, faceDetectedParams.face_roi[i].height};
            faceFeatures.setFaceBox(i, box);

            float roll = faceDetectedParams.rollangle[i];
            faceFeatures.setRollRadian(i, roll);
            if (true == faceDetectedParams.isGenderEnabled) {
                float maleScore = faceDetectedParams.male_score[i];
                float femaleScore = faceDetectedParams.female_score[i];
                faceFeatures.setGender(i, maleScore, femaleScore);
                MLOGD_BF("TrueSight capture: maleScore: %f, femaleScore: %f", maleScore,
                         femaleScore);
            }
            if (true == faceDetectedParams.isForeheadEnabled) {
                faceFeatures.setForeheadLandmark2D(i, faceDetectedParams.foreheadPoints[i].points,
                                                   MAX_FOREHEAD_POINT_NUM);
                MLOGD_BF("TrueSight capture: setForeheadLandmark2D");
            }
            faceFeatures.setFaceID(i, faceDetectedParams.faceid[i]);
        }

        m_pFaceDetector->initialize();
        m_pFaceDetector->detect(srcImage, faceFeatures);
    } else {
        m_pFaceDetector->getScheduleConfig().setEnableInnerFD(true).setMaxFaceCount(FD_Max_FACount);
        // faceFeatures.setFaceCount(faceDetectedParams.miFaceCountNum);
        m_pFaceDetector->initialize();
        m_pFaceDetector->detect(srcImage, faceFeatures);
    }

    double mialgoProcessStartTime = GetCurrentSecondValue();
    MLOGD_BF("TrueSight capture: faceCount %d faceDetectTime %f", faceFeatures.getFaceCount(),
             GetCurrentSecondValue() - faceDetectStartTime);

    // Mialgo process
    int processResult = 0;
    processResult =
        m_pBeautyShotCaptureAlgorithm->RunCaptureImage(&srcImage, &dstImage, &faceFeatures, NULL);
    double mialgoProcessEndTime = GetCurrentSecondValue();

    if (0 == processResult) {
        MLOGD_BF("TrueSight capture: processCapture success!");
    } else {
        MLOGE_BF("TrueSight capture: processCapture fail! result: %d", processResult);
        return 1;
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
    MLOGD_BF("TrueSight capture: processCapture cost time: %f",
             mialgoProcessEndTime - mialgoProcessStartTime);

    return 0;
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

    MLOGD_BF("TrueSight capture: Set image orientation is %d", mImageOrientation);
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
    // if (mBeautyShotVideoAlgorithm != NULL) {
    //     mBeautyShotVideoAlgorithm->ResetOutput();
    // }
}

void SkinBeautifier::LoadBeautyResources(int beautyMode)
{
    m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_packet,
                                                    NULL);
    m_pBeautyShotCaptureAlgorithm->RefreshEffectConfig();
    char configPath[MAX_RESOURCES_PATH_LEN] = {0};
    if (BEAUTY_EFFECT_MODE_CLASSICAL == beautyMode) {
        snprintf(configPath, MAX_RESOURCES_PATH_LEN, "%s%s", RESOURCES_PATH, CLASSICAL_PATH);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_packet,
                                                        configPath);
    } else if (BEAUTY_EFFECT_MODE_NATIVE == beautyMode) {
        snprintf(configPath, MAX_RESOURCES_PATH_LEN, "%s%s", RESOURCES_PATH, NATIVE_PATH);
        m_pBeautyShotCaptureAlgorithm->LoadEffectConfig(TrueSight::EffectIndex::kEffectIndex_packet,
                                                        configPath);
    } else if (BEAUTY_EFFECT_MODE_OTHER_FRONT == beautyMode) {
        snprintf(configPath, MAX_RESOURCES_PATH_LEN, "%s%s", RESOURCES_PATH, OTHER_FRONT_PATH);
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
    MLOGD_BF("TrueSight capture: LoadEffectConfig path is [%s]", configPath);
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
            MLOGD_BF("TrueSight capture: SetFeatureParams Flag[%22s]:%3d, FeatureValue:%d",
                     ParameterFlagToString(info.featureFlag), info.featureFlag, info.featureValue);
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
                MLOGD_BF("TrueSight capture: SetFeatureParams Flag[%22s]:%3d, FeatureValue:%d",
                         ParameterFlagToString(info.featureFlag), info.featureFlag,
                         info.featureValue);
                return 1;
            }
        }
        m_pBeautyShotCaptureAlgorithm->SetEffectParamsValue(featureFlag, adjustFeatureValue);
    }
    MLOGD_BF("TrueSight capture: SetFeatureParams Flag[%22s]:%3d, Value:%f",
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

    MLOGD_BF("TrueSight capture: ResetAllParameters");
}