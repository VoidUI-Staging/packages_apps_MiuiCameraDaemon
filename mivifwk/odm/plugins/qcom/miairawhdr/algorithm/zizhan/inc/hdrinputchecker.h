#ifndef __HDRINPUTCHECKER_H__
#define __HDRINPUTCHECKER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <vector>

#define DUMP_FILE_PATH "/data/vendor/camera/offlinelog/"
// MAX Exposure Nums, need map struct EVEXPOSURETYPE
const int MAX_EVEXPOSURETYPE = 5;
// return checker result
typedef int HDRCheckResult;
static const HDRCheckResult HDRCheckResultSuccess = 0;
static const HDRCheckResult HDRCheckResultBufferError = 1 << 1;
static const HDRCheckResult HDRCheckResultTuningModeErrorEV0 = 1 << 2;
static const HDRCheckResult HDRCheckResultTuningModeErrorUnderEv = 1 << 3;
static const HDRCheckResult HDRCheckResultTuningModeErrorUnderBostEV = 1 << 4;
static const HDRCheckResult HDRCheckResultTuningModeErrorOverEV = 1 << 5;
static const std::vector<HDRCheckResult> hdrCheckResVec{
    HDRCheckResultBufferError, HDRCheckResultTuningModeErrorEV0,
    HDRCheckResultTuningModeErrorUnderEv, HDRCheckResultTuningModeErrorUnderBostEV,
    HDRCheckResultTuningModeErrorOverEV};

// 0 - -- + bright
static std::vector<int> evModeMap(MAX_EVEXPOSURETYPE, -1);
typedef struct TuningModeType
{
    uint16_t value;
    ChiModeUsecaseSubModeType usecase;
    ChiModeFeature1SubModeType feature1;
    ChiModeFeature2SubModeType feature2;
    ChiModeSceneSubModeType scene;
    ChiModeEffectSubModeType effect;
} TUNINGMODETYPE;
typedef struct MiHDRCheckerAECFrameControl
{
    uint64_t shortExposureTime;
    uint64_t safeExposureTime;
    uint64_t longExposureTime;
    float shortLinearGain;
    float safeLinearGain;
    float longLinearGain;
    float shortSensitivity;
    float safeSensitivity;
    float longSensitivity;
    float luxIndex;
    float fRealAdrcGain;
} MIHDRCHECKERAECFRAMECONTROL;

typedef struct MiHDRParamInputInfo
{
    bool isFrontCamera;
    int inputNum;
    int EVValue[MAX_EVEXPOSURETYPE];
    int evExpOrder[HDRMAXEVEXPTYPE];
    HDRSnapshotType type;
} MIHDRPARAMINPUTINFO;

// rear camera, algo up ZSLHDR, ev order 0 - + / 0 - -, tuning mode, for L2 Rear hdr
static TUNINGMODETYPE RearAlgoUpFinalZSLHDRTuningMode[HDRMAXEVEXPTYPE] = {
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::AlgoUpFinalZSLHDR,
     ChiModeFeature2SubModeType::MMFPostFilter, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV0  take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::MFHDR,
     ChiModeFeature2SubModeType::HDRUnderexposure, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV- take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::MFHDR,
     ChiModeFeature2SubModeType::HDRUnderexposure, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV-- take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::MFHDR,
     ChiModeFeature2SubModeType::HDR, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV+  take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::None,
     ChiModeFeature2SubModeType::None, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EVBrightness
    //...
};
// front camera, algo up ZSLHDR, ev order 0 - -, tuning mode, for L2 front hdr
static TUNINGMODETYPE FrontAlgoUpFinalZSLHDRTuningMode[HDRMAXEVEXPTYPE] = {
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::AlgoUpFinalZSLHDR,
     ChiModeFeature2SubModeType::MMFPostFilter, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV0  take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::MFHDR,
     ChiModeFeature2SubModeType::HDRUnderexposure, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV-  take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::MFHDR,
     ChiModeFeature2SubModeType::HDRUnderexposure, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV-- take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::MFHDR,
     ChiModeFeature2SubModeType::HDR, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV+
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::MFHDR,
     ChiModeFeature2SubModeType::MFNRPostFilter, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EVBrightness
    //...
};

// algo up ZSLHDR, ev order + - 0
static TUNINGMODETYPE AlgoUpHDRTuningMode[HDRMAXEVEXPTYPE] = {
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::AlgoUpFinalZSLHDR,
     ChiModeFeature2SubModeType::MMFPostFilter, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV0 take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::MFHDR,
     ChiModeFeature2SubModeType::HDRUnderexposure, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV- take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::None,
     ChiModeFeature2SubModeType::None, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV--
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::MFHDR,
     ChiModeFeature2SubModeType::HDR, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV+ take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::MFHDR,
     ChiModeFeature2SubModeType::MFNRPostFilter, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EVBrightness take effect
    //...
};

// algo up SRHDR, ev order + - 0
static TUNINGMODETYPE AlgoUpSRHDRTuningMode[HDRMAXEVEXPTYPE] = {
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::HDRSR,
     ChiModeFeature2SubModeType::SR, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV0 take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::HDRSR,
     ChiModeFeature2SubModeType::None, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV- take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::HDRSR,
     ChiModeFeature2SubModeType::None, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV--
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::HDRSR,
     ChiModeFeature2SubModeType::None, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EV+ take effect
    {0, ChiModeUsecaseSubModeType::Snapshot, ChiModeFeature1SubModeType::None,
     ChiModeFeature2SubModeType::None, ChiModeSceneSubModeType::None,
     ChiModeEffectSubModeType::None}, // EVBrightness
    //...
};

template <int MAXSIZE>
void PrintHDRErrorToFile(HDRCheckResult &result, MIHDRPARAMINPUTINFO &hdrParamInputInfo,
                         MIHDRCHECKERAECFRAMECONTROL (&aecExposureData)[MAXSIZE],
                         TUNINGMODETYPE (&checkMode)[MAXSIZE])
{
    char file[PROPERTY_VALUE_MAX];
    struct tm *timenow;
    time_t now;
    time(&now);
    timenow = localtime(&now);
    snprintf(file, sizeof(file), "%s/HDRInputErrorAlgoUp_%02d%02d%02d%02d%02d.txt", DUMP_FILE_PATH,
             timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour, timenow->tm_min,
             timenow->tm_sec);

    FILE *pf = fopen(file, "wb");
    if (NULL != pf) {
        fprintf(pf,
                "------------------------------------------\n"
                "HDRSnapShotType:\n"
                "rear camera\n"
                "NoneHDR             = 0,   non hdr\n"
                "AlgoUpNormalHDR     = 1,   default algoup for hdr\n"
                "AlgoUpSTGHDR        = 2,   stagger algoup for hdr\n"
                "AlgoUpZSLHDR        = 3,   zsl algoup for hdr\n"
                "AlgoUpFinalZSLHDR   = 4,   all ev(zsl ev0, nonzsl ev-, nonzsl ev+)\n"
                "                           frame continous algoup for hdr\n"
                "AlgoUpFlashHDR      = 5,\n"
                "MFHDR               = 6,   4EV for hdr\n"
                "STGMFHDR            = 7,   stagger exp for hdr\n"
                "ZSLMFHDR            = 8,   4EV zero shutter lag for hdr\n"
                "FlashMFHDR          = 9,   4EV FlashHDR\n"
                "// front camera\n"
                "AlgoUpFrontNormalHDR     = 10,   default algoup for hdr\n"
                "AlgoUpFrontFinalZSLHDR   = 11,   all ev(zsl ev0, nonzsl ev-, nonzsl ev+)\n"
                "                                 frame continous algoup for hdr\n"
                "AlgoUpSRHDR          = 12, 3EV for hdr\n"
                "HDRCheckInputResult:%d, hdrType:%d, isFront:%d\n"
                "  EV0 Value:%d, EV0 Order:%d, Tuning Mode: Sensor[%d] Usecase[%d] Feature1[%d] "
                "Feature2[%d] "
                "Scene[%d] Effect[%d]\n"
                "  Exposure Info:\n"
                "      Short[%f, %" PRIu64
                "], sensitivity %f\n"
                "      Safe[%f, %" PRIu64
                "], sensitivity %f\n"
                "      Long[%f, %" PRIu64
                "], sensitivity %f\n"
                "  EV- Value:%d, EV- Order:%d, Tuning Mode: Sensor[%d] Usecase[%d] Feature1[%d] "
                "Feature2[%d] "
                "Scene[%d] Effect[%d]\n"
                "  Exposure Info:\n"
                "      Short[%f, %" PRIu64
                "], sensitivity %f\n"
                "      Safe[%f, %" PRIu64
                "], sensitivity %f\n"
                "      Long[%f, %" PRIu64
                "], sensitivity %f\n"
                "  EV-- Value:%d, EV-- Order:%d, Tuning Mode: Sensor[%d] Usecase[%d] Feature1[%d] "
                "Feature2[%d] "
                "Scene[%d] Effect[%d]\n"
                "  Exposure Info:\n"
                "      Short[%f, %" PRIu64
                "], sensitivity %f\n"
                "      Safe[%f, %" PRIu64
                "], sensitivity %f\n"
                "      Long[%f, %" PRIu64
                "], sensitivity %f\n"
                "  EV+ Value:%d, EV+ Order:%d, Tuning Mode: Sensor[%d] Usecase[%d] Feature1[%d] "
                "Feature2[%d] "
                "Scene[%d] Effect[%d]\n"
                "  Exposure Info:\n"
                "      Short[%f, %" PRIu64
                "], sensitivity %f\n"
                "      Safe[%f, %" PRIu64
                "], sensitivity %f\n"
                "      Long[%f, %" PRIu64 "], sensitivity %f\n",
                result, (int)(hdrParamInputInfo.type), hdrParamInputInfo.isFrontCamera,

                hdrParamInputInfo.EVValue[evModeMap[0]], hdrParamInputInfo.evExpOrder[evModeMap[0]],
                checkMode[evModeMap[0]].value, checkMode[evModeMap[0]].usecase,
                checkMode[evModeMap[0]].feature1, checkMode[evModeMap[0]].feature2,
                checkMode[evModeMap[0]].scene, checkMode[evModeMap[0]].effect,
                aecExposureData[evModeMap[0]].shortLinearGain,
                aecExposureData[evModeMap[0]].shortExposureTime,
                aecExposureData[evModeMap[0]].shortSensitivity,
                aecExposureData[evModeMap[0]].safeLinearGain,
                aecExposureData[evModeMap[0]].safeExposureTime,
                aecExposureData[evModeMap[0]].safeSensitivity,
                aecExposureData[evModeMap[0]].longLinearGain,
                aecExposureData[evModeMap[0]].longExposureTime,
                aecExposureData[evModeMap[0]].longSensitivity,

                hdrParamInputInfo.EVValue[evModeMap[1]], hdrParamInputInfo.evExpOrder[evModeMap[1]],
                checkMode[evModeMap[1]].value, checkMode[evModeMap[1]].usecase,
                checkMode[evModeMap[1]].feature1, checkMode[evModeMap[1]].feature2,
                checkMode[evModeMap[1]].scene, checkMode[evModeMap[1]].effect,
                aecExposureData[evModeMap[1]].shortLinearGain,
                aecExposureData[evModeMap[1]].shortExposureTime,
                aecExposureData[evModeMap[1]].shortSensitivity,
                aecExposureData[evModeMap[1]].safeLinearGain,
                aecExposureData[evModeMap[1]].safeExposureTime,
                aecExposureData[evModeMap[1]].safeSensitivity,
                aecExposureData[evModeMap[1]].longLinearGain,
                aecExposureData[evModeMap[1]].longExposureTime,
                aecExposureData[evModeMap[1]].longSensitivity,

                hdrParamInputInfo.EVValue[evModeMap[2]], hdrParamInputInfo.evExpOrder[evModeMap[2]],
                checkMode[evModeMap[2]].value, checkMode[evModeMap[2]].usecase,
                checkMode[evModeMap[2]].feature1, checkMode[evModeMap[2]].feature2,
                checkMode[evModeMap[2]].scene, checkMode[evModeMap[2]].effect,
                aecExposureData[evModeMap[2]].shortLinearGain,
                aecExposureData[evModeMap[2]].shortExposureTime,
                aecExposureData[evModeMap[2]].shortSensitivity,
                aecExposureData[evModeMap[2]].safeLinearGain,
                aecExposureData[evModeMap[2]].safeExposureTime,
                aecExposureData[evModeMap[2]].safeSensitivity,
                aecExposureData[evModeMap[2]].longLinearGain,
                aecExposureData[evModeMap[2]].longExposureTime,
                aecExposureData[evModeMap[2]].longSensitivity,

                hdrParamInputInfo.EVValue[evModeMap[3]], hdrParamInputInfo.evExpOrder[evModeMap[3]],
                checkMode[evModeMap[3]].value, checkMode[evModeMap[3]].usecase,
                checkMode[evModeMap[3]].feature1, checkMode[evModeMap[3]].feature2,
                checkMode[evModeMap[3]].scene, checkMode[evModeMap[3]].effect,
                aecExposureData[evModeMap[3]].shortLinearGain,
                aecExposureData[evModeMap[3]].shortExposureTime,
                aecExposureData[evModeMap[3]].shortSensitivity,
                aecExposureData[evModeMap[3]].safeLinearGain,
                aecExposureData[evModeMap[3]].safeExposureTime,
                aecExposureData[evModeMap[3]].safeSensitivity,
                aecExposureData[evModeMap[3]].longLinearGain,
                aecExposureData[evModeMap[3]].longExposureTime,
                aecExposureData[evModeMap[3]].longSensitivity);
    }
    evModeMap.assign(MAX_EVEXPOSURETYPE, -1);
    fclose(pf);
}

template <typename Lambda, int MAXSIZE>
HDRCheckResult hdrSelfChecker(MIHDRPARAMINPUTINFO &hdrParamInputInfo,
                              TUNINGMODETYPE (&checkMode)[MAXSIZE], Lambda cmpFunc)
{
    HDRCheckResult result = HDRCheckResultSuccess;
    // compare tuning mode
    int evMode = 0;
    int negEvCount = 0;
    int zeroEvCount = 0;
    bool evCompareResult = true;
    for (int i = 0; i < hdrParamInputInfo.inputNum; ++i) {
        if (hdrParamInputInfo.EVValue[i] == 0) {
            if (zeroEvCount == 0) {
                evMode = HDRNormalExp;
                evModeMap[0] = i;
                zeroEvCount++;
            } else if (zeroEvCount == 1) {
                evMode = HDRNormalBrightExp; // ev bright
                evModeMap[4] = i;
            }
        } else if (hdrParamInputInfo.EVValue[i] < 0) {
            if (negEvCount == 0) {
                evMode = HDRUnderBoostExp; // ev-
                evModeMap[1] = i;
                negEvCount++;
            } else if (negEvCount == 1) {
                evMode = HDRUnderExp; // ev--
                evModeMap[2] = i;
            }
        } else if (hdrParamInputInfo.EVValue[i] > 0) {
            evMode = HDROverExp; // ev+
            evModeMap[3] = i;
        }
        hdrParamInputInfo.evExpOrder[i] = i;
        evCompareResult = cmpFunc(checkMode[i], evMode);
        if (evCompareResult != true) {
            result |= hdrCheckResVec[i];
        }
    }

    int unvalidIndex = hdrParamInputInfo.inputNum;
    for (int i = 0; i < MAX_EVEXPOSURETYPE; ++i) {
        if (evModeMap[i] == -1) {
            evModeMap[i] = unvalidIndex;
            unvalidIndex++;
        }
    }

    return result;
}

// rear camera, algo up HDR, ev order 0 - +
template <int MAXSIZE>
HDRCheckResult AlgoUpHDRCheckInputInfo(MIHDRPARAMINPUTINFO &hdrParamInputInfo,
                                       MIHDRCHECKERAECFRAMECONTROL (&aecExposureData)[MAXSIZE],
                                       TUNINGMODETYPE (&checkMode)[MAXSIZE])
{
    HDRCheckResult result = HDRCheckResultSuccess;
    auto TuningCompare = [&](TUNINGMODETYPE &defaultTuningMode, int evExpType) -> bool {
        return ((defaultTuningMode.usecase == AlgoUpHDRTuningMode[evExpType].usecase) &&
                (defaultTuningMode.feature1 == AlgoUpHDRTuningMode[evExpType].feature1) &&
                (defaultTuningMode.feature2 == AlgoUpHDRTuningMode[evExpType].feature2));
    };
    result = hdrSelfChecker(hdrParamInputInfo, checkMode, TuningCompare);
    if (HDRCheckResultSuccess != result) {
        PrintHDRErrorToFile(result, hdrParamInputInfo, aecExposureData, checkMode);
    }
    return result;
}
// ZSL-HDR snapshot, EV order: 0 - +
template <int MAXSIZE>
HDRCheckResult RearAlgoUpFinalZSLHDRCheckInputInfo(
    MIHDRPARAMINPUTINFO &hdrParamInputInfo, MIHDRCHECKERAECFRAMECONTROL (&aecExposureData)[MAXSIZE],
    TUNINGMODETYPE (&checkMode)[MAXSIZE])
{
    HDRCheckResult result = HDRCheckResultSuccess;
    auto TuningCompare = [&](TUNINGMODETYPE &defaultTuningMode, int evExpType) -> bool {
        return (
            (defaultTuningMode.usecase == RearAlgoUpFinalZSLHDRTuningMode[evExpType].usecase) &&
            (defaultTuningMode.feature1 == RearAlgoUpFinalZSLHDRTuningMode[evExpType].feature1) &&
            (defaultTuningMode.feature2 == RearAlgoUpFinalZSLHDRTuningMode[evExpType].feature2));
    };
    result = hdrSelfChecker(hdrParamInputInfo, checkMode, TuningCompare);
    if (HDRCheckResultSuccess != result) {
        PrintHDRErrorToFile(result, hdrParamInputInfo, aecExposureData, checkMode);
    }
    return result;
}
// ZSL-HDR snapshot, EV order: 0 - -
template <int MAXSIZE>
HDRCheckResult FrontAlgoUpFinalZSLHDRCheckInputInfo(
    MIHDRPARAMINPUTINFO &hdrParamInputInfo, MIHDRCHECKERAECFRAMECONTROL (&aecExposureData)[MAXSIZE],
    TUNINGMODETYPE (&checkMode)[MAXSIZE])
{
    HDRCheckResult result = HDRCheckResultSuccess;
    auto TuningCompare = [&](TUNINGMODETYPE &defaultTuningMode, int evExpType) -> bool {
        return (
            (defaultTuningMode.usecase == FrontAlgoUpFinalZSLHDRTuningMode[evExpType].usecase) &&
            (defaultTuningMode.feature1 == FrontAlgoUpFinalZSLHDRTuningMode[evExpType].feature1) &&
            (defaultTuningMode.feature2 == FrontAlgoUpFinalZSLHDRTuningMode[evExpType].feature2));
    };
    result = hdrSelfChecker(hdrParamInputInfo, checkMode, TuningCompare);
    if (HDRCheckResultSuccess != result) {
        PrintHDRErrorToFile(result, hdrParamInputInfo, aecExposureData, checkMode);
    }
    return result;
}
template <int MAXSIZE>
HDRCheckResult AlgoUpZSLHDRChecker(MIHDRPARAMINPUTINFO &hdrParamInputInfo,
                                   MIHDRCHECKERAECFRAMECONTROL (&aecExposureData)[MAXSIZE],
                                   TUNINGMODETYPE (&checkMode)[MAXSIZE])
{
    HDRCheckResult result = HDRCheckResultSuccess;
    // front
    if (1 == hdrParamInputInfo.isFrontCamera) {
        result =
            FrontAlgoUpFinalZSLHDRCheckInputInfo(hdrParamInputInfo, aecExposureData, checkMode);
    }
    // rear
    else {
        result = RearAlgoUpFinalZSLHDRCheckInputInfo(hdrParamInputInfo, aecExposureData, checkMode);
    }
    return result;
}

// SRHDR snapshot, EV order: 0 - +
template <int MAXSIZE>
HDRCheckResult RearAlgoUpSRHDRCheckInputInfo(
    MIHDRPARAMINPUTINFO &hdrParamInputInfo, MIHDRCHECKERAECFRAMECONTROL (&aecExposureData)[MAXSIZE],
    TUNINGMODETYPE (&checkMode)[MAXSIZE])
{
    HDRCheckResult result = HDRCheckResultSuccess;
    auto TuningCompare = [&](TUNINGMODETYPE &defaultTuningMode, int evExpType) -> bool {
        return ((defaultTuningMode.usecase == AlgoUpSRHDRTuningMode[evExpType].usecase) &&
                (defaultTuningMode.feature1 == AlgoUpSRHDRTuningMode[evExpType].feature1) &&
                (defaultTuningMode.feature2 == AlgoUpSRHDRTuningMode[evExpType].feature2));
    };
    result = hdrSelfChecker(hdrParamInputInfo, checkMode, TuningCompare);
    if (HDRCheckResultSuccess != result) {
        PrintHDRErrorToFile(result, hdrParamInputInfo, aecExposureData, checkMode);
    }
    return result;
}

template <int MAXSIZE>
HDRCheckResult AlgoUpSRHDRChecker(MIHDRPARAMINPUTINFO &hdrParamInputInfo,
                                  MIHDRCHECKERAECFRAMECONTROL (&aecExposureData)[MAXSIZE],
                                  TUNINGMODETYPE (&checkMode)[MAXSIZE])
{
    HDRCheckResult result = HDRCheckResultSuccess;
    // front
    if (1 == hdrParamInputInfo.isFrontCamera) {
        // to do
        return result;
    }
    // rear
    else {
        result = RearAlgoUpSRHDRCheckInputInfo(hdrParamInputInfo, aecExposureData, checkMode);
    }
    return result;
}

// MFHDR snapshot map need info, EV order: 0 - -- +
template <int MAXSIZE>
void AlgoUpHDRInputInfoMap(HDRINPUTCHECKER &pHdrInputChecker,
                           MIHDRCHECKERAECFRAMECONTROL (&aecExposureData)[MAXSIZE],
                           TUNINGMODETYPE (&subMode)[MAXSIZE],
                           MIHDRPARAMINPUTINFO &hdrParamInputInfo)
{
    hdrParamInputInfo.inputNum = pHdrInputChecker.inputNum;
    hdrParamInputInfo.isFrontCamera = pHdrInputChecker.isFrontCamera;
    hdrParamInputInfo.EVValue[HDRNormalExp] = pHdrInputChecker.EVValue[HDRNormalExp];
    hdrParamInputInfo.evExpOrder[HDRNormalExp] = -1;
    aecExposureData[HDRNormalExp].shortExposureTime = pHdrInputChecker.aecFrameControl[HDRNormalExp]
                                                          .exposureInfo[ExposureIndexShort]
                                                          .exposureTime;
    aecExposureData[HDRNormalExp].safeExposureTime =
        pHdrInputChecker.aecFrameControl[HDRNormalExp].exposureInfo[ExposureIndexSafe].exposureTime;
    aecExposureData[HDRNormalExp].longExposureTime =
        pHdrInputChecker.aecFrameControl[HDRNormalExp].exposureInfo[ExposureIndexLong].exposureTime;
    aecExposureData[HDRNormalExp].shortLinearGain =
        pHdrInputChecker.aecFrameControl[HDRNormalExp].exposureInfo[ExposureIndexShort].linearGain;
    aecExposureData[HDRNormalExp].safeLinearGain =
        pHdrInputChecker.aecFrameControl[HDRNormalExp].exposureInfo[ExposureIndexSafe].linearGain;
    aecExposureData[HDRNormalExp].longLinearGain =
        pHdrInputChecker.aecFrameControl[HDRNormalExp].exposureInfo[ExposureIndexLong].linearGain;
    aecExposureData[HDRNormalExp].fRealAdrcGain =
        pHdrInputChecker.aecFrameControl[HDRNormalExp].exposureInfo[ExposureIndexSafe].sensitivity /
        pHdrInputChecker.aecFrameControl[HDRNormalExp].exposureInfo[ExposureIndexShort].sensitivity;
    aecExposureData[HDRNormalExp].shortSensitivity =
        pHdrInputChecker.aecFrameControl[HDRNormalExp].exposureInfo[ExposureIndexShort].sensitivity;
    aecExposureData[HDRNormalExp].safeSensitivity =
        pHdrInputChecker.aecFrameControl[HDRNormalExp].exposureInfo[ExposureIndexSafe].sensitivity;
    aecExposureData[HDRNormalExp].longSensitivity =
        pHdrInputChecker.aecFrameControl[HDRNormalExp].exposureInfo[ExposureIndexLong].sensitivity;
    aecExposureData[HDRNormalExp].luxIndex =
        pHdrInputChecker.aecFrameControl[HDRNormalExp].luxIndex;

    hdrParamInputInfo.EVValue[HDRUnderExp] = pHdrInputChecker.EVValue[HDRUnderExp];
    hdrParamInputInfo.evExpOrder[HDRUnderExp] = -1;
    aecExposureData[HDRUnderExp].shortExposureTime =
        pHdrInputChecker.aecFrameControl[HDRUnderExp].exposureInfo[ExposureIndexShort].exposureTime;
    aecExposureData[HDRUnderExp].safeExposureTime =
        pHdrInputChecker.aecFrameControl[HDRUnderExp].exposureInfo[ExposureIndexSafe].exposureTime;
    aecExposureData[HDRUnderExp].longExposureTime =
        pHdrInputChecker.aecFrameControl[HDRUnderExp].exposureInfo[ExposureIndexLong].exposureTime;
    aecExposureData[HDRUnderExp].shortLinearGain =
        pHdrInputChecker.aecFrameControl[HDRUnderExp].exposureInfo[ExposureIndexShort].linearGain;
    aecExposureData[HDRUnderExp].safeLinearGain =
        pHdrInputChecker.aecFrameControl[HDRUnderExp].exposureInfo[ExposureIndexSafe].linearGain;
    aecExposureData[HDRUnderExp].longLinearGain =
        pHdrInputChecker.aecFrameControl[HDRUnderExp].exposureInfo[ExposureIndexLong].linearGain;
    aecExposureData[HDRUnderExp].fRealAdrcGain =
        pHdrInputChecker.aecFrameControl[HDRUnderExp].exposureInfo[ExposureIndexSafe].sensitivity /
        pHdrInputChecker.aecFrameControl[HDRUnderExp].exposureInfo[ExposureIndexShort].sensitivity;
    aecExposureData[HDRUnderExp].shortSensitivity =
        pHdrInputChecker.aecFrameControl[HDRUnderExp].exposureInfo[ExposureIndexShort].sensitivity;
    aecExposureData[HDRUnderExp].safeSensitivity =
        pHdrInputChecker.aecFrameControl[HDRUnderExp].exposureInfo[ExposureIndexSafe].sensitivity;
    aecExposureData[HDRUnderExp].longSensitivity =
        pHdrInputChecker.aecFrameControl[HDRUnderExp].exposureInfo[ExposureIndexLong].sensitivity;
    aecExposureData[HDRUnderExp].luxIndex = pHdrInputChecker.aecFrameControl[HDRUnderExp].luxIndex;

    hdrParamInputInfo.EVValue[HDRUnderBoostExp] = pHdrInputChecker.EVValue[HDRUnderBoostExp];
    hdrParamInputInfo.evExpOrder[HDRUnderBoostExp] = -1;
    aecExposureData[HDRUnderBoostExp].shortExposureTime =
        pHdrInputChecker.aecFrameControl[HDRUnderBoostExp]
            .exposureInfo[ExposureIndexShort]
            .exposureTime;
    aecExposureData[HDRUnderBoostExp].safeExposureTime =
        pHdrInputChecker.aecFrameControl[HDRUnderBoostExp]
            .exposureInfo[ExposureIndexSafe]
            .exposureTime;
    aecExposureData[HDRUnderBoostExp].longExposureTime =
        pHdrInputChecker.aecFrameControl[HDRUnderBoostExp]
            .exposureInfo[ExposureIndexLong]
            .exposureTime;
    aecExposureData[HDRUnderBoostExp].shortLinearGain =
        pHdrInputChecker.aecFrameControl[HDRUnderBoostExp]
            .exposureInfo[ExposureIndexShort]
            .linearGain;
    aecExposureData[HDRUnderBoostExp].safeLinearGain =
        pHdrInputChecker.aecFrameControl[HDRUnderBoostExp]
            .exposureInfo[ExposureIndexSafe]
            .linearGain;
    aecExposureData[HDRUnderBoostExp].longLinearGain =
        pHdrInputChecker.aecFrameControl[HDRUnderBoostExp]
            .exposureInfo[ExposureIndexLong]
            .linearGain;
    aecExposureData[HDRUnderBoostExp].fRealAdrcGain =
        pHdrInputChecker.aecFrameControl[HDRUnderBoostExp]
            .exposureInfo[ExposureIndexSafe]
            .sensitivity /
        pHdrInputChecker.aecFrameControl[HDRUnderBoostExp]
            .exposureInfo[ExposureIndexShort]
            .sensitivity;
    aecExposureData[HDRUnderBoostExp].shortSensitivity =
        pHdrInputChecker.aecFrameControl[HDRUnderBoostExp]
            .exposureInfo[ExposureIndexShort]
            .sensitivity;
    aecExposureData[HDRUnderBoostExp].safeSensitivity =
        pHdrInputChecker.aecFrameControl[HDRUnderBoostExp]
            .exposureInfo[ExposureIndexSafe]
            .sensitivity;
    aecExposureData[HDRUnderBoostExp].longSensitivity =
        pHdrInputChecker.aecFrameControl[HDRUnderBoostExp]
            .exposureInfo[ExposureIndexLong]
            .sensitivity;
    aecExposureData[HDRUnderBoostExp].luxIndex =
        pHdrInputChecker.aecFrameControl[HDRUnderBoostExp].luxIndex;

    hdrParamInputInfo.EVValue[HDROverExp] = pHdrInputChecker.EVValue[HDROverExp];
    hdrParamInputInfo.evExpOrder[HDROverExp] = -1;
    aecExposureData[HDROverExp].shortExposureTime =
        pHdrInputChecker.aecFrameControl[HDROverExp].exposureInfo[ExposureIndexShort].exposureTime;
    aecExposureData[HDROverExp].safeExposureTime =
        pHdrInputChecker.aecFrameControl[HDROverExp].exposureInfo[ExposureIndexSafe].exposureTime;
    aecExposureData[HDROverExp].longExposureTime =
        pHdrInputChecker.aecFrameControl[HDROverExp].exposureInfo[ExposureIndexLong].exposureTime;
    aecExposureData[HDROverExp].shortLinearGain =
        pHdrInputChecker.aecFrameControl[HDROverExp].exposureInfo[ExposureIndexShort].linearGain;
    aecExposureData[HDROverExp].safeLinearGain =
        pHdrInputChecker.aecFrameControl[HDROverExp].exposureInfo[ExposureIndexSafe].linearGain;
    aecExposureData[HDROverExp].longLinearGain =
        pHdrInputChecker.aecFrameControl[HDROverExp].exposureInfo[ExposureIndexLong].linearGain;
    aecExposureData[HDROverExp].fRealAdrcGain =
        pHdrInputChecker.aecFrameControl[HDROverExp].exposureInfo[ExposureIndexSafe].sensitivity /
        pHdrInputChecker.aecFrameControl[HDROverExp].exposureInfo[ExposureIndexShort].sensitivity;
    aecExposureData[HDROverExp].shortSensitivity =
        pHdrInputChecker.aecFrameControl[HDROverExp].exposureInfo[ExposureIndexShort].sensitivity;
    aecExposureData[HDROverExp].safeSensitivity =
        pHdrInputChecker.aecFrameControl[HDROverExp].exposureInfo[ExposureIndexSafe].sensitivity;
    aecExposureData[HDROverExp].longSensitivity =
        pHdrInputChecker.aecFrameControl[HDROverExp].exposureInfo[ExposureIndexLong].sensitivity;
    aecExposureData[HDROverExp].luxIndex = pHdrInputChecker.aecFrameControl[HDROverExp].luxIndex;

    subMode[HDRNormalExp].value =
        pHdrInputChecker.tuningParam[HDRNormalExp].TuningMode[1].subMode.value;
    subMode[HDRNormalExp].usecase =
        pHdrInputChecker.tuningParam[HDRNormalExp].TuningMode[2].subMode.usecase;
    subMode[HDRNormalExp].feature1 =
        pHdrInputChecker.tuningParam[HDRNormalExp].TuningMode[3].subMode.feature1;
    subMode[HDRNormalExp].feature2 =
        pHdrInputChecker.tuningParam[HDRNormalExp].TuningMode[4].subMode.feature2;
    subMode[HDRNormalExp].scene =
        pHdrInputChecker.tuningParam[HDRNormalExp].TuningMode[5].subMode.scene;
    subMode[HDRNormalExp].effect =
        pHdrInputChecker.tuningParam[HDRNormalExp].TuningMode[6].subMode.effect;
    subMode[HDRUnderExp].value =
        pHdrInputChecker.tuningParam[HDRUnderExp].TuningMode[1].subMode.value;
    subMode[HDRUnderExp].usecase =
        pHdrInputChecker.tuningParam[HDRUnderExp].TuningMode[2].subMode.usecase;
    subMode[HDRUnderExp].feature1 =
        pHdrInputChecker.tuningParam[HDRUnderExp].TuningMode[3].subMode.feature1;
    subMode[HDRUnderExp].feature2 =
        pHdrInputChecker.tuningParam[HDRUnderExp].TuningMode[4].subMode.feature2;
    subMode[HDRUnderExp].scene =
        pHdrInputChecker.tuningParam[HDRUnderExp].TuningMode[5].subMode.scene;
    subMode[HDRUnderExp].effect =
        pHdrInputChecker.tuningParam[HDRUnderExp].TuningMode[6].subMode.effect;
    subMode[HDRUnderBoostExp].value =
        pHdrInputChecker.tuningParam[HDRUnderBoostExp].TuningMode[1].subMode.value;
    subMode[HDRUnderBoostExp].usecase =
        pHdrInputChecker.tuningParam[HDRUnderBoostExp].TuningMode[2].subMode.usecase;
    subMode[HDRUnderBoostExp].feature1 =
        pHdrInputChecker.tuningParam[HDRUnderBoostExp].TuningMode[3].subMode.feature1;
    subMode[HDRUnderBoostExp].feature2 =
        pHdrInputChecker.tuningParam[HDRUnderBoostExp].TuningMode[4].subMode.feature2;
    subMode[HDRUnderBoostExp].scene =
        pHdrInputChecker.tuningParam[HDRUnderBoostExp].TuningMode[5].subMode.scene;
    subMode[HDRUnderBoostExp].effect =
        pHdrInputChecker.tuningParam[HDRUnderBoostExp].TuningMode[6].subMode.effect;
    subMode[HDROverExp].value =
        pHdrInputChecker.tuningParam[HDROverExp].TuningMode[1].subMode.value;
    subMode[HDROverExp].usecase =
        pHdrInputChecker.tuningParam[HDROverExp].TuningMode[2].subMode.usecase;
    subMode[HDROverExp].feature1 =
        pHdrInputChecker.tuningParam[HDROverExp].TuningMode[3].subMode.feature1;
    subMode[HDROverExp].feature2 =
        pHdrInputChecker.tuningParam[HDROverExp].TuningMode[4].subMode.feature2;
    subMode[HDROverExp].scene =
        pHdrInputChecker.tuningParam[HDROverExp].TuningMode[5].subMode.scene;
    subMode[HDROverExp].effect =
        pHdrInputChecker.tuningParam[HDROverExp].TuningMode[6].subMode.effect;
}
// HDR check input info
HDRCheckResult IsValidCheckHDRInputInfo(HDRINPUTCHECKER &pHdrInputChecker, HDRSnapshotType type)
{
    HDRCheckResult result = HDRCheckResultSuccess;
    MIHDRCHECKERAECFRAMECONTROL aecExposureData[MAX_EVEXPOSURETYPE];
    TUNINGMODETYPE subMode[MAX_EVEXPOSURETYPE];
    MIHDRPARAMINPUTINFO hdrParamInputInfo;
    memset(&aecExposureData, 0, sizeof(MIHDRCHECKERAECFRAMECONTROL) * MAX_EVEXPOSURETYPE);
    memset(&subMode, 0, sizeof(TUNINGMODETYPE) * MAX_EVEXPOSURETYPE);
    memset(&hdrParamInputInfo, 0, sizeof(MIHDRPARAMINPUTINFO));
    AlgoUpHDRInputInfoMap(pHdrInputChecker, aecExposureData, subMode, hdrParamInputInfo);
    switch (type) {
    case HDRSnapshotType::AlgoUpNormalHDR:
        hdrParamInputInfo.type = HDRSnapshotType::AlgoUpNormalHDR;
        result = AlgoUpHDRCheckInputInfo(hdrParamInputInfo, aecExposureData, subMode);
        break;
    case HDRSnapshotType::AlgoUpFinalZSLHDR:
        hdrParamInputInfo.type = HDRSnapshotType::AlgoUpFinalZSLHDR;
        result = AlgoUpZSLHDRChecker(hdrParamInputInfo, aecExposureData, subMode);
        break;
    case HDRSnapshotType::AlgoUpSRHDR:
        hdrParamInputInfo.type = HDRSnapshotType::AlgoUpSRHDR;
        result = AlgoUpSRHDRChecker(hdrParamInputInfo, aecExposureData, subMode);
        break;
    default:
        break;
    }
    return result;
}
#endif //__HDRINPUTCHECKER_H__