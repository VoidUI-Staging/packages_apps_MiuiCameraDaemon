#include <utils/Log.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "VendorMetadataParser.h"
#include "algoAdapter.h"
#include "arcsr.h"

#undef LOG_TAG
#define LOG_TAG "ARCSRADPTER"

using namespace mialgo2;
using namespace std;

typedef struct
{
    int left;
    int top;
    int width;
    int height;
} sr_rect_t;

static int getAnchor(const ImageParams *buffer, unsigned char metaSize)
{
    bool hdsrEnable = getHDSREnable(buffer, metaSize);
    if (hdsrEnable)
        return metaSize - 1;
    return getAnchorIndex(buffer, metaSize).getDataWithDefault<int>(-1);
}

static void srGetAEInfo(int i, void *metaData, float *pfLuxIndex, float *pfSensorGain,
                        float *pfDigitalGain, float *pfDRCGain, float *pfDarkBoostGain)
{
    void *pData = NULL;
    const char *bpsGain = "com.qti.sensorbps.gain";
    VendorMetadataParser::getVTagValue((camera_metadata_t *)metaData, bpsGain, &pData);
    if (NULL != pData) {
        *pfSensorGain = *static_cast<float *>(pData);
    } else {
        *pfSensorGain = 1.0f;
        MLOGW(Mia2LogGroupPlugin, "get bpsGain failed");
    }
    pData = NULL;
    const char *AecFrameControl = "org.quic.camera2.statsconfigs.AECFrameControl";

    VendorMetadataParser::getVTagValue((camera_metadata_t *)metaData, AecFrameControl, &pData);
    if (NULL != pData) {
        *pfLuxIndex = (float)static_cast<AECFrameControl *>(pData)->luxIndex;
        *pfDigitalGain =
            static_cast<AECFrameControl *>(pData)->exposureInfo[ExposureIndexSafe].linearGain /
            *pfSensorGain;
        // short =
        *pfDRCGain = static_cast<AECFrameControl *>(pData)->compenADRCGain;
        *pfDarkBoostGain = static_cast<AECFrameControl *>(pData)->compensationDarkBoostGain;
    } else {
        *pfLuxIndex = 50.0f;
        *pfDigitalGain = 1.0f;
        *pfDRCGain = 1.0f;
        *pfDarkBoostGain = 1.0f;
    }

    MLOGD(Mia2LogGroupPlugin,
          "GetAEInfo:i=%d, fLuxIndex=%.6f fSensorGain=%.6f fDigitalGain=%.6f fDRCGain=%.6f "
          "fDarkBoostGain=%.6f",
          i, *pfLuxIndex, *pfSensorGain, *pfDigitalGain, *pfDRCGain, *pfDarkBoostGain);
}

static CameraRoleType srGetCameraIdRole(camera_metadata_t *metaData, uint32_t &fwkCameraId)
{
    CameraRoleType roleID = CameraRoleTypeWide;
    if (NULL == metaData) {
        MLOGD(Mia2LogGroupPlugin, "metaData null point!");
        return roleID;
    }
    void *pData = NULL;
    MultiCameraIds multiCameraRole;
    const char *MultiCameraRole = "com.qti.chi.multicamerainfo.MultiCameraIds";
    VendorMetadataParser::getVTagValue(metaData, MultiCameraRole, &pData);
    if (NULL != pData) {
        multiCameraRole = *static_cast<MultiCameraIds *>(pData);
        MLOGI(Mia2LogGroupPlugin,
              "MultiCamera: curCamId = %d logiCamId = %d masterCamId = %d currentCameraRole = %d "
              "masterCameraRole = %d fwcamId = %d",
              multiCameraRole.currentCameraId, multiCameraRole.logicalCameraId,
              multiCameraRole.masterCameraId, multiCameraRole.currentCameraRole,
              multiCameraRole.masterCameraRole, multiCameraRole.fwkCameraId);
        roleID = multiCameraRole.currentCameraRole;
        fwkCameraId = multiCameraRole.fwkCameraId;
    } else {
        MLOGE(Mia2LogGroupPlugin, "get multicamerainfo.MultiCameraIds fail");
    }
    return roleID;
}

static void process(const AlgorithmAdapter *me, const SessionInfo *sessionInfo,
                    const Request *request)
{
    static mutex ArcprocessMutex;
    ArcSR *mArcSR;
    mArcSR = new ArcSR();

    // 1. get in, out, count, buffer
    void *handle = nullptr;
    const unsigned int inputCount = request->getInputCount();
    const ImageParams *const in = request->getInput();
    const ImageParams *const out = request->getOutput();
    const uint32_t frameNum = request->getframeNum();
    camera_metadata_t *pInputMata[10];
    struct MiImageBuffer inputFrame[inputCount], outputFrame;

    for (int i = 0; i < inputCount; i++) {
        inputFrame[i].format = in->format.format;
        inputFrame[i].width = static_cast<uint32_t>(getWidth(in + i));
        inputFrame[i].height = static_cast<uint32_t>(getHeight(in + i));
        inputFrame[i].plane[0] = getPlane(in + i)[0];
        inputFrame[i].plane[1] = getPlane(in + i)[1];
        inputFrame[i].stride = static_cast<uint32_t>(getStride(in + i));
        inputFrame[i].scanline = static_cast<uint32_t>(getScanline(in + i));
        pInputMata[i] = in->pPhyCamMetadata ? in->pPhyCamMetadata : in->pMetadata;
    }

    outputFrame.format = out->format.format;
    outputFrame.width = static_cast<uint32_t>(getWidth(out));
    outputFrame.height = static_cast<uint32_t>(getHeight(out));
    outputFrame.plane[0] = getPlane(out)[0];
    outputFrame.plane[1] = getPlane(out)[1];
    outputFrame.stride = static_cast<uint32_t>(getStride(out));
    outputFrame.scanline = static_cast<uint32_t>(getScanline(out));

    // 2. get camera role type, anchor, stylization type, isz status, crop region
    uint32_t fwkCameraId = 0;
    CameraRoleType roleType = srGetCameraIdRole(pInputMata[0], fwkCameraId);
    int anchor = getAnchor(in, inputCount);
    uint8_t stylizationType = getStyleType(in, inputCount);

    vector<int32_t> sensorModeIndexList = getSensorModeindex(in, inputCount);
    vector<int32_t> sensorModeCapList = getSensorModeCaps(in, inputCount);

    vector<int32_t> iszList(inputCount, 0);
    if (roleType == CameraRoleTypeWide) {
        for (int i = 0; i < inputCount; i++) {
            CHISENSORMODECAPS cap;
            cap.value = sensorModeCapList[i];
            iszList[i] = cap.u.InSensorZoom;
        }
    } else if (roleType == CameraRoleTypeTele) {
        iszList = getFakeSat(in, inputCount);
    }
    uint32_t inSensorZoom = 0;
    uint32_t isZslSelected = 0;
    const ChiRect *cropRegion = nullptr;
    isZslSelected = getZslSelected(in, inputCount);

    if (anchor >= 0 && anchor < inputCount) {
        inSensorZoom = iszList[anchor];
        cropRegion = getCropRegionSAT(in + anchor, inputCount - anchor);
    } else {
        inSensorZoom = iszList[0];
        cropRegion = getCropRegionSAT(in, inputCount);
    }

    // 3. select camera role name
    MInt32 cameraRoleArc;
    char *cameraRoleName;
    if (roleType == CameraRoleTypeTele && inSensorZoom && stylizationType) {
        cameraRoleName = "TeleRemosaicClassic";
        cameraRoleArc = 0x21;
    } else if (roleType == CameraRoleTypeTele && inSensorZoom && !stylizationType) {
        cameraRoleName = "TeleRemosaic";
        cameraRoleArc = 0x31;
    } else if (roleType == CameraRoleTypeTele && !inSensorZoom && stylizationType) {
        cameraRoleName = "TeleClassic";
        cameraRoleArc = 0x20;
    } else if (roleType == CameraRoleTypeTele && !inSensorZoom && !stylizationType) {
        cameraRoleName = "Tele";
        cameraRoleArc = 0x30;
    } else if (roleType == CameraRoleTypeWide && inSensorZoom && stylizationType) {
        cameraRoleName = "WideRemosaicClassic";
        cameraRoleArc = 0x01;
    } else if (roleType == CameraRoleTypeWide && inSensorZoom && !stylizationType) {
        cameraRoleName = "WideRemosaic";
        cameraRoleArc = 0x11;
    } else if (roleType == CameraRoleTypeWide && !inSensorZoom && stylizationType) {
        cameraRoleName = "WideClassic";
        cameraRoleArc = 0x00;
    } else {
        cameraRoleName = "Wide";
        cameraRoleArc = 0x10;
    }

    // 4. getCropRegion()
    sr_rect_t pRect[1] = {{0, 0, static_cast<int>(getWidth(in)), static_cast<int>(getHeight(in))}};
    MRECT cropRect;
    const ChiRect *ref = getRefSize(fwkCameraId);
    getCropRegion(cropRegion, ref, inputFrame[0].width, inputFrame[0].height,
                  [&pRect](int left, int top, int width, int height) mutable {
                      pRect->left = left;
                      pRect->top = top;
                      pRect->width = width;
                      pRect->height = height;
                  });

    if (cropRegion == NULL) {
        MLOGI(Mia2LogGroupPlugin, "cropRegion is NULL, use active region instead");
        cropRegion = (ChiRect *)(&pRect[0]);
    }

    cropRect.left = pRect->left;
    cropRect.top = pRect->top;
    cropRect.right = pRect->left + pRect->width;
    cropRect.bottom = pRect->top + pRect->height;

    // 5. get hdsr status and merge meta
    bool hdsrEnable = getHDSREnable(in, inputCount);
    if (hdsrEnable && isZslSelected && anchor < inputCount && anchor > 0) {
        MLOGW(Mia2LogGroupPlugin, "merge anchor meta to output");
        VendorMetadataParser::mergeMetadata(out->pPhyCamMetadata, (in + anchor)->pPhyCamMetadata);
    }
    MLOGI(Mia2LogGroupPlugin,
          "anchor:%d, cropRect[%d, %d, %d, %d], inSensorZoom/FakeSat:%d, camera role, name:%s, "
          "hdsrEnable %d, isZslSelected %d, fwkcameraid:%d",
          anchor, pRect->left, pRect->top, pRect->width, pRect->height, inSensorZoom,
          cameraRoleName, hdsrEnable, isZslSelected, fwkCameraId);

    // 6. get face info
    ARC_MFSR_FACEINFO faceInfo = {0};
    MRECT faceRect[MAX_FACE];
    MInt32 rotate[MAX_FACE] = {0};
    uint32_t faceNum = 0;
    int32_t RotateAngle = getJpegOrientation(in, inputCount);

    // is 16:9 or 4:3 or 1:1?
    float xRatio = (float)cropRegion->width / (float)inputFrame[0].width;
    float yRatio = (float)cropRegion->height / (float)inputFrame[0].height;
    const float tolerance = 0.01f;

    const FaceResult *tmpData = getFaceRect(in, inputCount);
    if (tmpData) {
        MLOGI(Mia2LogGroupPlugin, "MiaNodeSr getFaceRect");
        faceNum = tmpData->faceNumber > MAX_FACE ? MAX_FACE : tmpData->faceNumber;
        faceInfo.nFace = faceNum;

        for (int index = 0; index < faceNum; index++) {
            ChiRectEXT curFaceInfo = tmpData->chiFaceInfo[index];
            if (yRatio > xRatio + tolerance) {
                // 16:9 height is cropped
                int delta = (ref->height - inputFrame[0].height) / 2;
                faceRect[index] = {curFaceInfo.left, curFaceInfo.bottom - delta, curFaceInfo.right,
                                   curFaceInfo.top - delta};
                MLOGI(Mia2LogGroupPlugin, "16:9 faceIndex:%d faceRect[%d,%d,%d,%d], rotate: %d",
                      index, faceRect[index].left, faceRect[index].top, faceRect[index].right,
                      faceRect[index].bottom, RotateAngle);
            } else if (xRatio > yRatio + tolerance) {
                // 1:1 width is cropped
                int delta = (ref->width - inputFrame[0].width) / 2;
                faceRect[index] = {curFaceInfo.left - delta, curFaceInfo.bottom,
                                   curFaceInfo.right - delta, curFaceInfo.top};
                MLOGI(Mia2LogGroupPlugin, "1:1 faceIndex:%d faceRect[%d,%d,%d,%d], rotate: %d",
                      index, faceRect[index].left, faceRect[index].top, faceRect[index].right,
                      faceRect[index].bottom, RotateAngle);
            } else {
                // 4:3 "xiaomi.snapshot.faceRect" meta Top and Bottom need revert
                faceRect[index] = {curFaceInfo.left, curFaceInfo.bottom, curFaceInfo.right,
                                   curFaceInfo.top};
                MLOGI(Mia2LogGroupPlugin, "4:3 faceIndex:%d faceRect[%d,%d,%d,%d], rotate: %d",
                      index, faceRect[index].left, faceRect[index].top, faceRect[index].right,
                      faceRect[index].bottom, RotateAngle);
            }
        }
        faceInfo.rcFace = faceRect;
    } else {
        MLOGI(Mia2LogGroupPlugin, "faces are not found");
    }
    for (int i = 0; i < faceNum; i++) {
        if (RotateAngle == 90)
            rotate[i] = 1;
        else if (RotateAngle == 0)
            rotate[i] = 0;
        else if (RotateAngle == 180)
            rotate[i] = 2;
        else if (RotateAngle == 270)
            rotate[i] = 3;
        else
            rotate[i] = 1;
    }
    faceInfo.lFaceOrient = rotate;

    // 7. get AE info
    vector<int32_t> isoVec = getIso<int32_t>(in, inputCount);
    vector<int64_t> expTime = getExposureTime(in, inputCount, int64_t);

    float fLuxIndex[inputCount];
    float fSensorGain[inputCount];
    float fDigitalGain[inputCount];
    float fDRCGain[inputCount];
    float fDarkBoostGain[inputCount];
    memset(fLuxIndex, 0, inputCount * sizeof(float));
    memset(fSensorGain, 0, inputCount * sizeof(float));
    memset(fDigitalGain, 0, inputCount * sizeof(float));
    memset(fDRCGain, 0, inputCount * sizeof(float));
    memset(fDarkBoostGain, 0, inputCount * sizeof(float));

    for (int i = 0; i < inputCount; i++) {
        srGetAEInfo(i, pInputMata[i], &fLuxIndex[i], &fSensorGain[i], &fDigitalGain[i],
                    &fDRCGain[i], &fDarkBoostGain[i]);
    }

    // 8. dump and process()
    char srDirName[256];
    time_t currentTime;
    struct tm *timeInfo;

    time(&currentTime);
    timeInfo = localtime(&currentTime);
    strftime(srDirName, sizeof(srDirName), "data/vendor/camera/srdump/IMG_%Y%m%d%H%M%S/", timeInfo);
    MLOGI(Mia2LogGroupPlugin, "sr dir name: %s", srDirName);

    dump(in, inputCount, me->getName(), srDirName, [=](unsigned int index) mutable -> string {
        stringstream filename;
        filename << cameraRoleName << "_crop_" << pRect->left << '_' << pRect->top << '_'
                 << pRect->width << '_' << pRect->height << "_iso_" << static_cast<int>(isoVec[0])
                 << "_exp_time_" << expTime[0] << "_isz_" << iszList[0] << "_ZSL_" << isZslSelected
                 << "_anchor_" << anchor << "_input";
        return filename.str();
    });

    if (hdsrEnable)
        publishHDSRCropRegion(getLogicalMetaDataPointer(out), pRect[0].left, pRect[0].top,
                              pRect[0].width, pRect[0].height);
    // 9. MoonSceneFlag
    int moonSceneFlag = (static_cast<int>(getMode(in, inputCount)) == 35) ? 1 : 0;
    MLOGI(Mia2LogGroupPlugin, "sr MoonSceneFlag : %d", moonSceneFlag);

    ArcprocessMutex.lock();
    int res = mArcSR->process_still(inputFrame, &outputFrame, inputCount, &cropRect, cameraRoleArc,
                                    isoVec, expTime, fLuxIndex, fDRCGain, fDarkBoostGain,
                                    fSensorGain, fDigitalGain, anchor, &faceInfo, moonSceneFlag);
    ArcprocessMutex.unlock();

    if (0 != res) {
        MLOGE(Mia2LogGroupPlugin, "Sr process returned error! Going memcopy.");
        PluginUtils::miCopyBuffer(&outputFrame, &inputFrame[0]);
    }

    dump(out, 1, me->getName(), srDirName, [=](unsigned int index) mutable -> string {
        stringstream filename;
        filename << cameraRoleName << "_crop_" << pRect->left << '_' << pRect->top << '_'
                 << pRect->width << '_' << pRect->height << "_iso_"
                 << static_cast<int>(isoVec[anchor]) << "_exp_time_" << expTime[anchor]
                 << "_anchor_" << anchor << "_output";
        return filename.str();
    });

    // 10. exif info
    vector<int32_t> af = getLensPosition(in, inputCount, int32_t);
    vector<int32_t> IQEnabledMap = getIQEnabledMap(in, inputCount, int32_t);
    sessionInfo->setResultMetadata(
        inputCount, request->getTimeStamp(),
        (stringstream() << "sr:{algo:" << me->getName() << " inNum:" << inputCount << ' '
                        << "info:" << ((roleType == CameraRoleTypeTele) ? "tele" : "wide") << ' '
                        << vector2exifstr<int32_t>("af", af).c_str() << ' '
                        << vector2exifstr<int32_t>("IQ", IQEnabledMap).c_str() << ' '
                        << vector2exifstr<int32_t>("iso", isoVec).c_str() << ' '
                        << vector2exifstr<int64_t>("exp", expTime).c_str() << ' '
                        << vector2exifstr<int>("ISZ remosaic", iszList).c_str() << ' '
                        << "ZSL:" << isZslSelected << ' ' << "firstFrameNum:" << frameNum << ' '
                        << "anchor:" << anchor << "}")
            .str());

    delete mArcSR;
}

static SingletonAlgorithmAdapter adapter = SingletonAlgorithmAdapter("arcsoft", process);

extern Entrance arcsoft = &adapter;
