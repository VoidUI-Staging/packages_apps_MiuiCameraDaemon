#include "include/SkinBeautifier.h"

#include <cutils/properties.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <fstream>

#include "arcsoft_debuglog.h"

SkinBeautifier::SkinBeautifier()
{
    arcInitCommonConfigSpec("persist.vendor.camera.arcsoft.log.fb");

    mBeautyShotVideoAlgorithm = NULL;
    mInitReady = false;
    mBeautyStyleEnable = 0;
    // BeautyLastType = 0;
    mFeatureBeautyMode = 0;
#if defined(RENOIR_CAM) || defined(ODIN_CAM) || defined(VILI_CAM) || defined(INGRES_CAM) || \
    defined(THOR_CAM) || defined(ZIZHAN_CAM)
    for (int i = 0; i < Beauty_Style_max; i++) {
        mBeautyStyleFileData[i].styleFileData = NULL;
        mBeautyStyleFileData[i].styleFileSize = 0;
    }
    SkinGlossRatioIsLoad = false;
#endif
}

SkinBeautifier::~SkinBeautifier()
{
    MLOGD_BF("arcsoft debug ~SkinBeautifier Start");

    if (mBeautyShotVideoAlgorithm != NULL) {
        if (mInitReady == true) {
            mBeautyShotVideoAlgorithm->UnInit();
        }
        mBeautyShotVideoAlgorithm->Release();
        mBeautyShotVideoAlgorithm = NULL;
    }
#if defined(RENOIR_CAM) || defined(ODIN_CAM) || defined(VILI_CAM) || defined(INGRES_CAM) || \
    defined(THOR_CAM) || defined(ZIZHAN_CAM)
    for (int i = 0; i < Beauty_Style_max; i++) {
        if (mBeautyStyleFileData[i].styleFileData != NULL) {
            delete[] mBeautyStyleFileData[i].styleFileData;
            mBeautyStyleFileData[i].styleFileData = NULL;
        }
        　　mBeautyStyleFileData[i].styleFileSize = 0;
    }
#endif

    MLOGD_BF("arcsoft debug ~SkinBeautifier End");
}

void SkinBeautifier::SetFeatureParams(BeautyFeatureParams *beautyFeatureParams)
{
    mFeatureParams = beautyFeatureParams->beautyFeatureInfo;
    mfeatureCounts = beautyFeatureParams->featureCounts;
#if defined(THOR_CAM) || defined(ZIZHAN_CAM)
    mFeatureBeautyMode = &(beautyFeatureParams->beautyFeatureMode);
#else
    mbeautyStyleInfo = &(beautyFeatureParams->beautyStyleInfo);
#endif
}

BeautyStyleFileData *SkinBeautifier::GetBeautyStyleData(int beautyStyleType)
{
    BeautyStyleFileData *result = NULL;
    int beautyStyleIndex = beautyStyleType - 1;

    　const char *beautyStylePath[Beauty_Style_max] = {
        "",
        "/vendor/etc/camera/beauty_style_skin_gloss.cng",
        "/vendor/etc/camera/beauty_style_nude.cng",
        "/vendor/etc/camera/beauty_style_pink.cng",
        "/vendor/etc/camera/beauty_style_blue.cng",
        "/vendor/etc/camera/beauty_style_neutral.cng",
        "/vendor/etc/camera/beauty_style_masculine.cng"
        // "/vendor/etc/camera/beauty_style_soft.cng",
        // "/vendor/etc/camera/beauty_style_neutral_m3.cng"
    };

    if (0 < beautyStyleType && beautyStyleType < Beauty_Style_max) {
        if (mBeautyStyleFileData[beautyStyleIndex].styleFileData == NULL) {
            std::ifstream styleReader;
            const char *styleFilePath = NULL;

            styleFilePath = beautyStylePath[beautyStyleType];

            styleReader.open(styleFilePath, std::ios::in | std::ios::binary);
            if (styleReader.is_open() == true) {
                int styleFileSize = 0;
                styleReader.seekg(0, std::ios::end);
                styleFileSize = styleReader.tellg();
                styleReader.seekg(0, std::ios::beg);

                char *styleFileData = new char[styleFileSize];
                styleReader.read(styleFileData, styleFileSize);
                styleReader.close();

                if (styleFileData != NULL && styleFileSize > 0) {
                    mBeautyStyleFileData[beautyStyleIndex].styleFileData = styleFileData;
                    mBeautyStyleFileData[beautyStyleIndex].styleFileSize = styleFileSize;
                } else {
                    if (styleFileData != NULL) {
                        delete[] styleFileData;
                        styleFileData = NULL;
                    }
                }
            } else {
                ARC_LOGE("arcsoft debug preview open style file failed! style file path: %s",
                         styleFilePath);
            }
        }

        if (mBeautyStyleFileData[beautyStyleIndex].styleFileData != NULL &&
            mBeautyStyleFileData[beautyStyleIndex].styleFileSize != 0) {
            result = &(mBeautyStyleFileData[beautyStyleIndex]);
        }
    }

    return result;
}

#if defined(THOR_CAM) || defined(ZIZHAN_CAM)
int SkinBeautifier::Process(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle, int region,
                            int mode, unsigned int faceNum, unsigned int m_appModule)
{
    int result = -1;
    if (mode < BEAUTY_MODE_PREVIEW) {
        result = ProcessCapture(input, output, faceAngle, region, m_appModule);
    } else {
        result = ProcessPreview(input, output, faceAngle, faceNum);
    }
    return result;
}
#else
int SkinBeautifier::Process(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle, int region,
                            int mode, unsigned int faceNum)
{
    int result = -1;
    if (mode < BEAUTY_MODE_PREVIEW) {
        result = ProcessCapture(input, output, faceAngle, region);
    } else {
        result = ProcessPreview(input, output, faceAngle, faceNum);
    }
    return result;
}
#endif

int SkinBeautifier::ProcessPreview(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle,
                                   unsigned int faceNum)
{
    if (mBeautyShotVideoAlgorithm == NULL) {
        MRESULT createResult = MOK;

        mWidth = 0;
        mHeight = 0;
        createResult = Create_BeautyShot_Video_Algorithm(BeautyShot_Video_Algorithm::CLASSID,
                                                         &mBeautyShotVideoAlgorithm);

        if (createResult != MOK || mBeautyShotVideoAlgorithm == NULL) {
            MLOGE_BF("arcsoft debug Create_BeautyShot_Video_Algorithm fail!");
            return 1;
        }
    }

    MLOGI_BF(
        "arcsoft debug Create_BeautyShot_Video_Algorithm Success, mBeautyShotVideoAlgorithm = %p",
        mBeautyShotVideoAlgorithm);

    if ((input->i32Width != mWidth) || (input->i32Height != mHeight)) {
        if ((mWidth != 0) || (mHeight != 0)) {
            mBeautyShotVideoAlgorithm->UnInit();
        }

        int featureLevel =
            mBeautyFullFeatureFlag ? XIAOMI_FB_FEATURE_LEVEL_2 : XIAOMI_FB_FEATURE_LEVEL_1;
        MRESULT initResult = MOK;

#if defined(THOR_CAM) || defined(ZIZHAN_CAM)
        initResult = mBeautyShotVideoAlgorithm->Init(ABS_INTERFACE_VERSION, featureLevel,
                                                     ABS_PLATFORM_TYPE_CPU, 0);
#else
        initResult = mBeautyShotVideoAlgorithm->Init(ABS_INTERFACE_VERSION, featureLevel,
                                                     ABS_VIDEO_PLATFORM_TYPE_CPU);
#endif

        if (initResult != MOK) {
            mBeautyShotVideoAlgorithm->Release();
            mBeautyShotVideoAlgorithm = NULL;
            MLOGE_BF("arcsoft debug mBeautyShotVideoAlgorithm Init Fail");
            return 1;
        }

        mWidth = input->i32Width;
        mHeight = input->i32Height;
        mInitReady = true;
    }

    MLOGD_BF("arcsoft debug mBeautyShotVideoAlgorithm Init Success, Width = %d, Height = %d",
             mWidth, mHeight);

    /*void* pStyeData      = NULL;
    int   styleDataSize  = 0;
    //TODO: read style data from style.cng file
    if (pStyeData != NULL && styleDataSize > 0)
{
            mBeautyShotVideoAlgorithm->LoadStyle(pStyeData,styleDataSize);
}*/

    for (unsigned int i = 0; i < mfeatureCounts; i++) {
        mBeautyShotVideoAlgorithm->SetFeatureLevel(mFeatureParams[i].featureKey,
                                                   mFeatureParams[i].featureValue);
        ARC_LOGD("arcsoft debug beauty preview: %s = %d\n", mFeatureParams[i].tagName,
                 mFeatureParams[i].featureValue);
    }

    ABS_TFaces faceInfoIn, faceInfoOut;
    memset(&faceInfoIn, 0, sizeof(ABS_TFaces));
    faceInfoIn.lFaceNum = faceNum;

    MRESULT processResult = MOK;

    double processStartTime = GetCurrentSecondValue();
    processResult =
        mBeautyShotVideoAlgorithm->Process(input, output, &faceInfoIn, &faceInfoOut, faceAngle);
    double processEndTime = GetCurrentSecondValue();

    if (processResult != MOK) {
        MLOGE_BF("arcsoft debug preview Process YUV process fail, result: %ld", processResult);
        return 1;
    }

    ALOGE("arcsoft debug preview process cost time: %f", processEndTime - processStartTime);

    return 0;
}

#if defined(THOR_CAM) || defined(ZIZHAN_CAM)

int SkinBeautifier::ProcessCapture(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle,
                                   int region, unsigned int m_appModule)
{
    MLOGI_BF("arcsoft Process Capture YUV begin");

    BeautyShot_Image_Algorithm *pBeautyShotImageAlgorithm = NULL;
    MRESULT createResult = Create_BeautyShot_Image_Algorithm(BeautyShot_Image_Algorithm::CLASSID,
                                                             &pBeautyShotImageAlgorithm);
    if (createResult != MOK || pBeautyShotImageAlgorithm == NULL) {
        MLOGE_BF("arcsoft debug Create_BeautyShot_Image_Algorithm fail!");
        return 1;
    }
    MRESULT initResult;

#if defined(THOR_CAM) || defined(ZIZHAN_CAM)
    // int32_t abs_mode = property_get_int32("persist.vendor.camera.absmode", 0);
    ABS_MODE abs_mode = mFeatureBeautyMode->featureValue;
    if (2 == m_appModule || 8 == m_appModule) {
        abs_mode = 1;
    }
    initResult = pBeautyShotImageAlgorithm->Init(ABS_INTERFACE_VERSION, abs_mode);
    MLOGD_BF("arcsoft debug abs_mode:%d", abs_mode);
#else
    // LEVEL_3 for front camera
    if (mIsFrontCamera == true) {
        initResult =
            pBeautyShotImageAlgorithm->Init(ABS_INTERFACE_VERSION, XIAOMI_FB_FEATURE_LEVEL_3);
    }
    // LEVEL_1 for rear camera with only skin_smooth
    else {
        initResult =
            pBeautyShotImageAlgorithm->Init(ABS_INTERFACE_VERSION, XIAOMI_FB_FEATURE_LEVEL_1);
    }
    if (initResult != MOK) {
        pBeautyShotImageAlgorithm->Release();
        pBeautyShotImageAlgorithm = NULL;
        return 1;
    }
#endif
    if (initResult != MOK) {
        pBeautyShotImageAlgorithm->Release();
        pBeautyShotImageAlgorithm = NULL;
        return 1;
    }

    // Whether or not beautystyle enable
    if (!strcmp(mFeatureParams[7].tagName, "eyeBrowDyeRatio")) {
        if (0 != mFeatureParams[7].featureValue) {
            mBeautyStyleEnable = 1;
        } else {
            // BeautyLastType = 0;
            pBeautyShotImageAlgorithm->SetFeatureLevel(mFeatureParams[7].featureKey,
                                                       mFeatureParams[7].featureValue);
            MLOGI_BF("arcsoft debug capture beauty params: %s = %d\n", mFeatureParams[7].tagName,
                     mFeatureParams[7].featureValue);
        }
    }
#if defined(ZEUS_CAM) || defined(CUPID_CAM) || defined(THOR_CAM) || defined(INGRES_CAM) || \
    defined(ZIZHAN_CAM) || defined(MAYFLY_CAM) || defined(DITING_CAM)
    BeautyStyleFileData *beautyStyleFileData = NULL;
    if (1 == mBeautyStyleEnable) {
        BeautyStyleFileData *beautyStyleFileData = GetBeautyStyleData(1);
        if (beautyStyleFileData->styleFileData != NULL && beautyStyleFileData->styleFileSize != 0) {
            pBeautyShotImageAlgorithm->LoadStyle(beautyStyleFileData->styleFileData,
                                                 beautyStyleFileData->styleFileSize);
            ARC_LOGD("arcsoft debug load style!");
            pBeautyShotImageAlgorithm->SetFeatureLevel(mFeatureParams[7].featureKey,
                                                       mFeatureParams[7].featureValue);
            MLOGI_BF("arcsoft debug capture beauty params: %s = %d\n", mFeatureParams[7].tagName,
                     mFeatureParams[7].featureValue);
        } else {
            ARC_LOGD("arcsoft debug capture set beauty style failed!");
        }
        mBeautyStyleEnable = 0;
    }
#endif

    for (unsigned int i = 0; i < (mfeatureCounts - 1); i++) {
        pBeautyShotImageAlgorithm->SetFeatureLevel(mFeatureParams[i].featureKey,
                                                   mFeatureParams[i].featureValue);
        MLOGI_BF("arcsoft debug capture beauty params: %s = %d\n", mFeatureParams[i].tagName,
                 mFeatureParams[i].featureValue);
    }

    ABS_TFaces faceInfoIn, faceInfoOut;

    double processStartTime = GetCurrentSecondValue();
    MRESULT processResult = pBeautyShotImageAlgorithm->Process(input, output, &faceInfoIn,
                                                               &faceInfoOut, faceAngle, region);
    double processEndTime = GetCurrentSecondValue();

    pBeautyShotImageAlgorithm->UnInit();
    pBeautyShotImageAlgorithm->Release();
    pBeautyShotImageAlgorithm = NULL;
    if (processResult != MOK) {
        MLOGE_BF("[CAM_LOG_SYSTEM]SkinBeautifier arcsoft Process fail : %ld", processResult);
        return 1;
    }

    MLOGD_BF("arcsoft debug capture process cost: %f", processEndTime - processStartTime);

    return 0;
}
#else
int SkinBeautifier::ProcessCapture(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle,
                                   int region)
{
    MLOGI_BF("arcsoft Process Capture YUV begin");
    BeautyShot_Image_Algorithm *pBeautyShotImageAlgorithm = NULL;
    MRESULT createResult = Create_BeautyShot_Image_Algorithm(BeautyShot_Image_Algorithm::CLASSID,
                                                             &pBeautyShotImageAlgorithm);
    if (createResult != MOK || pBeautyShotImageAlgorithm == NULL) {
        MLOGE_BF("arcsoft debug Create_BeautyShot_Image_Algorithm fail!");
        return 1;
    }
    MRESULT initResult;
    // LEVEL_3 for front camera
    if (mIsFrontCamera == true) {
        initResult =
            pBeautyShotImageAlgorithm->Init(ABS_INTERFACE_VERSION, XIAOMI_FB_FEATURE_LEVEL_3);
    }
    // LEVEL_1 for rear camera with only skin_smooth
    else {
        initResult =
            pBeautyShotImageAlgorithm->Init(ABS_INTERFACE_VERSION, XIAOMI_FB_FEATURE_LEVEL_1);
    }
    if (initResult != MOK) {
        pBeautyShotImageAlgorithm->Release();
        pBeautyShotImageAlgorithm = NULL;
        return 1;
    }
    /*void* pStyeData      = NULL;
        int   styleDataSize  = 0;
        //TODO: read style data from style.cng file
        if (pStyeData != NULL && styleDataSize > 0)
    {
                pBeautyShotImageAlgorithm->LoadStyle(pStyeData,styleDataSize);
    }*/
#if defined(RENOIR_CAM) || defined(ODIN_CAM) || defined(VILI_CAM) || defined(INGRES_CAM)
    // bool isSetStyle  = false;
    BeautyStyleFileData *beautyStyleFileData = NULL;
    if (mbeautyStyleInfo->styleValue == 0) {
        pBeautyShotImageAlgorithm->SetFeatureLevel(mbeautyStyleInfo->styleLevelFeatureKey,
                                                   mbeautyStyleInfo->styleLevelValue);
    }
    if (0 < mbeautyStyleInfo->styleValue && mbeautyStyleInfo->styleValue <= Beauty_Style_max) {
        BeautyStyleFileData *beautyStyleFileData = GetBeautyStyleData(mbeautyStyleInfo->styleValue);
        if (beautyStyleFileData->styleFileData != NULL && beautyStyleFileData->styleFileSize != 0) {
            if (mbeautyStyleInfo->styleValue != BeautyLastType) {
                pBeautyShotImageAlgorithm->LoadStyle(beautyStyleFileData->styleFileData,
                                                     beautyStyleFileData->styleFileSize);
                ARC_LOGD("arcsoft debug Capture load style!");
            }
            pBeautyShotImageAlgorithm->SetFeatureLevel(mbeautyStyleInfo->styleLevelFeatureKey,
                                                       mbeautyStyleInfo->styleLevelValue);
            // isSetStyle = true;
            ARC_LOGD("arcsoft debug Capture set beauty style: %d, level: %d",
                     mbeautyStyleInfo->styleValue, mbeautyStyleInfo->styleLevelValue);
        } else {
            ARC_LOGD("arcsoft debug Capture set beauty style failed!");
        }
    }
#endif
    for (unsigned int i = 0; i < mfeatureCounts; i++) {
        /*
        #if defined(RENOIR_CAM)
                if (strcmp(mFeatureParams[i].tagName, "skinGlossRatio") == 0 &&
                    mFeatureParams[i].featureValue != 0) {
                    if (SkinGlossRatioIsLoad == false) {
                        BeautyStyleFileData *beautyStyleFileData =
                            GetBeautyStyleData(Beauty_Style_Skin_gloss);
                        if (beautyStyleFileData->styleFileData != NULL &&
                            beautyStyleFileData->styleFileSize != 0) {
                            pBeautyShotImageAlgorithm->LoadStyle(beautyStyleFileData->styleFileData,
                                                                 beautyStyleFileData->styleFileSize);
                            SkinGlossRatioIsLoad = true;
                            ARC_LOGD("arcsoft debug capture load skinGlossRatio!");
                        }
                    }
                }
        #endif
        */
        pBeautyShotImageAlgorithm->SetFeatureLevel(mFeatureParams[i].featureKey,
                                                   mFeatureParams[i].featureValue);
        MLOGI_BF("arcsoft debug capture beauty params: %s = %d\n", mFeatureParams[i].tagName,
                 mFeatureParams[i].featureValue);
    }
    ABS_TFaces faceInfoIn, faceInfoOut;
    double processStartTime = GetCurrentSecondValue();
    MRESULT processResult = pBeautyShotImageAlgorithm->Process(input, output, &faceInfoIn,
                                                               &faceInfoOut, faceAngle, region);
    double processEndTime = GetCurrentSecondValue();
    pBeautyShotImageAlgorithm->UnInit();
    pBeautyShotImageAlgorithm->Release();
    pBeautyShotImageAlgorithm = NULL;
    if (processResult != MOK) {
        MLOGE_BF("[CAM_LOG_SYSTEM]SkinBeautifier arcsoft Process fail : %ld", processResult);
        return 1;
    }
    MLOGD_BF("arcsoft debug capture process cost: %f", processEndTime - processStartTime);
    return 0;
}
#endif

double SkinBeautifier::GetCurrentSecondValue()
{
    timeval currentTime;
    gettimeofday(&currentTime, NULL);
    double secondValue =
        ((double)currentTime.tv_sec * 1000.0) + ((double)currentTime.tv_usec / 1000.0);
    return secondValue;
}

void SkinBeautifier::ResetPreviewOutput()
{
    if (mBeautyShotVideoAlgorithm != NULL) {
        mBeautyShotVideoAlgorithm->ResetOutput();
    }
}

bool SkinBeautifier::IsFrontCamera(uint32_t frameworkCameraId)
{
    // if front camera is true
    if (CAM_COMBMODE_FRONT == frameworkCameraId || CAM_COMBMODE_FRONT_BOKEH == frameworkCameraId ||
        CAM_COMBMODE_FRONT_AUX == frameworkCameraId) {
        mIsFrontCamera = true;
        return true;
    }
    // rear camera
    else {
        mIsFrontCamera = false;
        return false;
    }
    MLOGD_BF("arcsoft debug camera front is %d", mIsFrontCamera);
}
