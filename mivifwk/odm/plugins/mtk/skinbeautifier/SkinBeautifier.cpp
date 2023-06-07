#include "include/SkinBeautifier.h"

#include <cutils/properties.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <fstream>

#include "arcsoft_debuglog.h"
#include "arcsoft_utils.h"
#undef LOG_TAG
#define LOG_TAG "SkinBeautifier"
using namespace mialgo2;
std::mutex gBeaCapMutex;
SkinBeautifier::SkinBeautifier()
{
    ALOGI("SkinBeautifier() enter");
    arcInitCommonConfigSpec("persist.vendor.camera.arcsoft.log.fb");
    mBeautyShotVideoAlgorithm = NULL;
    pBeautyShotImageAlgorithm = NULL;
    mInitReady = false;
    mWidth = 0;
    mHeight = 0;
#if defined(RENOIR_CAM)
    for (int i = 0; i < Beauty_Style_max; i++) {
        mBeautyStyleFileData[i].styleFileData = NULL;
        mBeautyStyleFileData[i].styleFileSize = 0;
    }
    SkinGlossRatioIsLoad = false;
#endif
}
SkinBeautifier::~SkinBeautifier()
{
    ARC_LOGD("arcsoft debug ~SkinBeautifier Start");
    ALOGI("arcsoft ~SkinBeautifier() mInitReady = %d", mInitReady);
    std::unique_lock<std::mutex> lock(gBeaCapMutex);
    if (mInitReady == true) {
        if (mBeautyShotVideoAlgorithm != NULL) {
            mBeautyShotVideoAlgorithm->UnInit();
            mBeautyShotVideoAlgorithm->Release();
            mBeautyShotVideoAlgorithm = NULL;
            ALOGI("arcsoft ~SkinBeautifier() Release ShotVideo");
        }
        if (pBeautyShotImageAlgorithm != NULL) {
            pBeautyShotImageAlgorithm->UnInit();
            pBeautyShotImageAlgorithm->Release();
            pBeautyShotImageAlgorithm = NULL;
            ALOGI("arcsoft ~SkinBeautifier() Release ShotVideo");
        }
        mInitReady == false;
    }
#if defined(RENOIR_CAM)
    for (int i = 0; i < Beauty_Style_max; i++) {
        if (mBeautyStyleFileData[i].styleFileData != NULL) {
            delete[] mBeautyStyleFileData[i].styleFileData;
            mBeautyStyleFileData[i].styleFileData = NULL;
        }
        　　mBeautyStyleFileData[i].styleFileSize = 0;
    }
#endif
    ARC_LOGD("arcsoft debug ~SkinBeautifier End");
}
void SkinBeautifier::SetFeatureParams(BeautyFeatureParams *beautyFeatureParams)
{
    mFeatureParams = beautyFeatureParams->beautyFeatureInfo;
    mfeatureCounts = beautyFeatureParams->featureCounts;
}
int SkinBeautifier::InitLib(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle, int region,
                            int mode, unsigned int faceNum)
{
    int result = -1;
    if (mode < BEAUTY_MODE_PREVIEW) {
        result = InitCapture(input, output, faceAngle, region);
    } else {
        result = InitPreview(input, output, faceAngle, faceNum);
    }
    return result;
}
int SkinBeautifier::Process(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle, int region,
                            int mode, unsigned int faceNum)
{
    int result = -1;
    std::unique_lock<std::mutex> lock(gBeaCapMutex);
    ALOGI("arcsoft Process() enter mInitReady = %d Width = %d Height = %d", mInitReady,
          input->i32Width, input->i32Height);
    if ((mInitReady == false) || (input->i32Width != mWidth) || (input->i32Height != mHeight)) {
        result = InitLib(input, output, faceAngle, region, mode, faceNum);
    }
    ALOGI("arcsoft Process() process mInitReady = %d", mInitReady);
    if (mInitReady == true) {
        if (mode < BEAUTY_MODE_PREVIEW) {
            result = ProcessCapture(input, output, faceAngle, region, faceNum);
        } else {
            result = ProcessPreview(input, output, faceAngle, faceNum);
        }
    }
    ALOGI("arcsoft Process() exit mInitReady = %d, mInitReady", mInitReady);
    return result;
}
int SkinBeautifier::InitPreview(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle,
                                unsigned int faceNum)
{
    if (mBeautyShotVideoAlgorithm == NULL) {
        MRESULT createResult = MOK;
        mWidth = 0;
        mHeight = 0;
        createResult = Create_BeautyShot_Video_Algorithm(BeautyShot_Video_Algorithm::CLASSID,
                                                         &mBeautyShotVideoAlgorithm);
        if (createResult != MOK || mBeautyShotVideoAlgorithm == NULL) {
            MLOGE(Mia2LogGroupPlugin, "arcsoft debug Create_BeautyShot_Video_Algorithm fail!");
            return 1;
        }
    }
    ARC_LOGD(
        "arcsoft debug Create_BeautyShot_Video_Algorithm Success, mBeautyShotVideoAlgorithm = %p",
        mBeautyShotVideoAlgorithm);
    if ((input->i32Width != mWidth) || (input->i32Height != mHeight)) {
        if ((mWidth != 0) || (mHeight != 0)) {
            mBeautyShotVideoAlgorithm->UnInit();
        }
        int featureLevel =
            mBeautyFullFeatureFlag ? XIAOMI_FB_FEATURE_LEVEL_2 : XIAOMI_FB_FEATURE_LEVEL_1;
        MRESULT initResult = MOK;
        initResult = mBeautyShotVideoAlgorithm->Init(ABS_INTERFACE_VERSION, featureLevel,
                                                     ABS_VIDEO_PLATFORM_TYPE_CPU, 0);
        if (initResult != MOK) {
            mBeautyShotVideoAlgorithm->Release();
            mBeautyShotVideoAlgorithm = NULL;
            MLOGE(Mia2LogGroupPlugin, "arcsoft debug mBeautyShotVideoAlgorithm Init Fail");
            return 1;
        }
        mWidth = input->i32Width;
        mHeight = input->i32Height;
        mInitReady = true;
    }
    ARC_LOGD("arcsoft debug mBeautyShotVideoAlgorithm Init Success, Width = %d, Height = %d",
             mWidth, mHeight);
    ARC_LOGD("arcsoft debug mBeautyShotVideoAlgorithm Init Success, Width = %d, Height = %d",
             mWidth, mHeight);
    /*void* pStyeData      = NULL;
    int   styleDataSize  = 0;
    //TODO: read style data from style.cng file
    if (pStyeData != NULL && styleDataSize > 0)
{
            mBeautyShotVideoAlgorithm->LoadStyle(pStyeData,styleDataSize);
}*/
    return 0;
}
int SkinBeautifier::ProcessPreview(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle,
                                   unsigned int faceNum)
{
    for (unsigned int i = 0; i < mfeatureCounts; i++) {
        mBeautyShotVideoAlgorithm->SetFeatureLevel(mFeatureParams[i].featureKey,
                                                   mFeatureParams[i].featureValue);
        ARC_LOGD("arcsoft debug beauty preview: %s = %d\n", mFeatureParams[i].TagName,
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
        MLOGE(Mia2LogGroupPlugin, "arcsoft debug preview Process YUV process fail, result: %ld",
              processResult);
        return 1;
    }
    ARC_LOGD("arcsoft debug preview process cost time: %f", processEndTime - processStartTime);
    return 0;
}
int SkinBeautifier::allocBufAndReadFromFile(const char *name, void **ppBuf, int *pSize)
{
    int ret = -1;
    void *pBuf;
    int64_t size;
    FILE *pFile = fopen(name, "rb");
    if (pFile) {
        fseek(pFile, 0, SEEK_END);
        size = ftell(pFile);
        fseek(pFile, 0, SEEK_SET);
        if (size != -1) {
            pBuf = calloc(size, 1);
            if (pBuf) {
                size_t cnt;
                cnt = fread(pBuf, 1, size, pFile);
                if (cnt != (size_t)size) {
                    free(pBuf);
                    ARC_LOGD(
                        "arcsoft allocBufAndReadFromFile(): read data from %s failed, size=%" PRIu64
                        "readout=%zu",
                        name, size, cnt);
                } else {
                    ARC_LOGD("arcsoft get data from %s, size=%" PRIu64, name, size);
                    *pSize = size;
                    *ppBuf = pBuf;
                    ret = 0;
                    // for (size_t i = 0; i < cnt; i++)
                    // LdcLOGV("data[%zu]=%d", i, ((char *)pBuf)[i]);
                }
            } else {
                ARC_LOGD("arcsoft allocBufAndReadFromFile(): calloc failed size=%" PRIu64
                         "err=%d %s",
                         size, errno, strerror(errno));
            }
        } else {
            ARC_LOGD("arcsoft allocBufAndReadFromFile(): failed to get file size of %s,err=%d %s",
                     name, errno, strerror(errno));
        }
        fclose(pFile);
    } else {
        ARC_LOGD("arcsoft allocBufAndReadFromFile(): failed to open file %s,err=%d %s", name, errno,
                 strerror(errno));
    }
    return ret;
}
int SkinBeautifier::InitCapture(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle,
                                int region)
{
    ALOGI("arcsoft InitCapture YUV begin");
    if (mInitReady == true) {
        if (pBeautyShotImageAlgorithm != NULL) {
            pBeautyShotImageAlgorithm->UnInit();
            pBeautyShotImageAlgorithm->Release();
            pBeautyShotImageAlgorithm = NULL;
            ALOGI("arcsoft InitCapture() Release ShotVideo");
        }
        mInitReady = false;
    }
    // BeautyShot_Image_Algorithm *pBeautyShotImageAlgorithm = NULL;
    MRESULT createResult = Create_BeautyShot_Image_Algorithm(BeautyShot_Image_Algorithm::CLASSID,
                                                             &pBeautyShotImageAlgorithm);
    if (createResult != MOK || pBeautyShotImageAlgorithm == NULL) {
        ALOGE("arcsoft debug Create_BeautyShot_Image_Algorithm fail!");
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
        ALOGE("arcsoft  InitCapture()  algo Init Fail");
        return 1;
    }
#if defined(DAUMIER_CAMERA)
    void *pStyeData = NULL;
    int styleDataSize = 0;
    const char *styleName = BEAUTY_STYLE_FILE;
    allocBufAndReadFromFile(styleName, &pStyeData, &styleDataSize);
    initResult = -1;
    // TODO: read style data from style.cng file
    if (pStyeData != NULL && styleDataSize > 0) {
        initResult = pBeautyShotImageAlgorithm->LoadStyle(pStyeData, styleDataSize);
    }
    if (pStyeData)
        free(pStyeData);
    if (initResult != MOK) {
        pBeautyShotImageAlgorithm->UnInit();
        pBeautyShotImageAlgorithm->Release();
        pBeautyShotImageAlgorithm = NULL;
        ALOGE("arcsoft debug BeautyShotImageAlgorithm LoadStyle Fail");
        return 1;
    } else {
        ARC_LOGD("arcsoft debug BeautyShotImageAlgorithm LoadStyle %s", BEAUTY_STYLE_FILE);
    }
#endif
    ALOGI("arcsoft InitCapture()  successuful");
    mWidth = input->i32Width;
    mHeight = input->i32Height;
    mInitReady = true;
    return 0;
}
int SkinBeautifier::ProcessCapture(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle,
                                   int region, unsigned int faceNum)
{
    ALOGI("arcsoft Process Capture YUV begin");

    if (pBeautyShotImageAlgorithm == NULL) {
        ALOGE("arcsoft Process Capture pBeautyShotImageAlgorithm == NULL");
        return 1;
    }
    for (unsigned int i = 0; i < mfeatureCounts; i++) {
        if (strcmp(mFeatureParams[i].TagName, "xiaomi.beauty.hairlineRatio") == 0) {
            mFeatureParams[i].featureValue = -mFeatureParams[i].featureValue;
        }
        pBeautyShotImageAlgorithm->SetFeatureLevel(mFeatureParams[i].featureKey,
                                                   mFeatureParams[i].featureValue);
        ARC_LOGD("arcsoft debug capture beauty params: %s = %d\n", mFeatureParams[i].TagName,
                 mFeatureParams[i].featureValue);
    }
    ABS_TFaces faceInfoIn, faceInfoOut;
    memset(&faceInfoIn, 0, sizeof(ABS_TFaces));
    memset(&faceInfoOut, 0, sizeof(ABS_TFaces));
    ALOGI("arcsoft debug capture  faceNum = %u", faceNum);
    faceInfoIn.lFaceNum = faceNum;
    if (faceNum == 0) {
        faceInfoIn.lFaceNum = 1;
    } else {
        faceInfoIn.lFaceNum = faceNum;
    }
    double processStartTime = GetCurrentSecondValue();
    MRESULT processResult = pBeautyShotImageAlgorithm->Process(input, output, &faceInfoIn,
                                                               &faceInfoOut, faceAngle, region);
    double processEndTime = GetCurrentSecondValue();
    if (processResult != MOK) {
        MLOGE(Mia2LogGroupPlugin, "arcsoft debug Process Capture YUV process fail, ret:%ld",
              processResult);
        return 1;
    }
    ALOGI("arcsoft debug capture process cost: %f", processEndTime - processStartTime);
    return 0;
}
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
void SkinBeautifier::IsFrontCamera(uint32_t frameworkCameraId)
{
    // if front camera is true
    if (CAM_COMBMODE_FRONT == frameworkCameraId || CAM_COMBMODE_FRONT_BOKEH == frameworkCameraId ||
        CAM_COMBMODE_FRONT_AUX == frameworkCameraId) {
        mIsFrontCamera = true;
    }
    // rear camera
    else {
        mIsFrontCamera = false;
    }
    ARC_LOGD("arcsoft debug camera front is %d", mIsFrontCamera);
}
