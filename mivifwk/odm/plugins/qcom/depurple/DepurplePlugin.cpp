#include "DepurplePlugin.h"

#include "MiaPluginUtils.h"
#include "MiaPostProcType.h"
#include "math.h"

using namespace mialgo2;

static const int32_t sDrawFaceRect =
    property_get_int32("persist.vendor.camera.algoengine.depurple.drawfaceroi", 0);
static const int32_t sDumpData =
    property_get_int32("persist.vendor.camera.algoengine.depurple.dump", 0);
static const int32_t sDepurpleDump =
    property_get_int32("persist.vendor.camera.algoengine.depurple.enable", -1);
static const int32_t sDepurpleLiteTaskLimit = 3;

DepurplePlugin::~DepurplePlugin() {}

int DepurplePlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mFrameworkID = createInfo->frameworkCameraId;
    MLOGI(Mia2LogGroupPlugin, "depurple init framework camera id = %d", mFrameworkID);
    mProcessCount = 0;
    mNodeInterface = nodeInterface;
    mNodeInstance = NULL;
    mNodeInstance =
        new QAltekDepurple(createInfo->inputFormat.width, createInfo->inputFormat.height);
    mCFRModeInfo = {0};
    mLiteEnable = false;
    mPreprocess = false;
    mOperationMode = createInfo->operationMode;
    mDrawFaceRect = sDrawFaceRect;
    mDumpData = sDumpData;
    return 0;
}

bool DepurplePlugin::isEnabled(MiaParams settings)
{
    void *data = NULL;
    int32_t depurpleEnable = 0;
    int32_t iIsEnable = sDepurpleDump;
    camera_metadata_t *metadata = settings.metadata;
    auto getDepurpleEnable = [this, &depurpleEnable, &data, metadata, &iIsEnable]() mutable {
        data = NULL;
        const char *tagDepurpleEnable = "xiaomi.depurple.enabled";
        VendorMetadataParser::getVTagValue(metadata, tagDepurpleEnable, &data);
        if (NULL != data) {
            depurpleEnable = *static_cast<uint8_t *>(data);
        }
        data = NULL;
        int32_t highQualityPreferred = 1;
        const char *tagHighQualityPreferred = "xiaomi.imageQuality.isHighQualityPreferred";
        VendorMetadataParser::getVTagValue(metadata, tagHighQualityPreferred, &data);
        if (NULL != data) {
            highQualityPreferred = *static_cast<uint8_t *>(data);
        }

        mCFRModeInfo.isSNMode = getSNMode(metadata);
        depurpleEnable = depurpleEnable | mCFRModeInfo.isSNMode;
        mCFRModeInfo.sensorId = getSensorId(metadata, mFrameworkID);
        mCFRModeInfo.isHDMode = getHighDefinitionMode(metadata);

#if defined(NUWA_CAM) || defined(FUXI_CAM) || defined(SOCRATES_CAM)
        if (mCFRModeInfo.sensorId != BACK_UWIDE) {
            depurpleEnable = false;
        }
#elif defined(THOR_CAM)
        mCFRModeInfo.isHDRMode = getHDRMode(metadata);
        if (mCFRModeInfo.sensorId != BACK_UWIDE) {
            depurpleEnable = false;
        }
#elif defined(ZEUS_CAM) || defined(ZIZHAN_CAM) || defined(UNICORN_CAM)
        if (mCFRModeInfo.sensorId != BACK_UWIDE) {
            depurpleEnable = false;
        }
#elif defined(CUPID_CAM) || defined(INGRES_CAM) || defined(MAYFLY_CAM) || defined(DITING_CAM)
        if (BACK_WIDE == mCFRModeInfo.sensorId &&
            (mCFRModeInfo.isSNMode || mCFRModeInfo.isHDMode)) {
            depurpleEnable = false;
        }
        if (BACK_MACRO == mCFRModeInfo.sensorId) {
            depurpleEnable = false;
        }
#endif
        depurpleEnable = ((iIsEnable == -1) ? (highQualityPreferred && depurpleEnable) : iIsEnable);
        if (depurpleEnable) {
            mCFRModeInfo.vendorId = getVendorId(mCFRModeInfo.sensorId);
        }
        MLOGI(Mia2LogGroupPlugin, "sensorId %d depurpleEnable: %d", mCFRModeInfo.sensorId,
              depurpleEnable);
    };

    getDepurpleEnable();
    return depurpleEnable ? true : false;
}

int DepurplePlugin::preProcess(PreProcessInfo preProcessInfo)
{
    mPreprocess = true;

    if (NULL != mNodeInstance) {
        mNodeInstance->incInit(mCFRModeInfo);
        MLOGI(Mia2LogGroupPlugin, "preprocess");
    }
    return MIAS_OK;
}

ProcessRetStatus DepurplePlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;

    camera_metadata_t *metadata = inputBuffers[0].pMetadata;
    camera_metadata_t *physicalmetadata = inputBuffers[0].pPhyCamMetadata;
    alCFR_FACE_T faceInfo;
    memset(&faceInfo, 0, sizeof(alCFR_FACE_T));
    int luxIndex = 0;
#if defined(NUWA_CAM) || defined(FUXI_CAM) || defined(SOCRATES_CAM)
    alCFR_RECT zoomInfo;
    memset(&zoomInfo, 0, sizeof(alCFR_RECT));
#else
    alGE_RECT zoomInfo;
    memset(&zoomInfo, 0, sizeof(alGE_RECT));
#endif
    ChiRect cropRegion;
    memset(&cropRegion, 0, sizeof(ChiRect));
    mActiveSize = {0, 0};

    MiImageBuffer inputFrame, outputFrame;
    convertImageParams(inputBuffers[0], inputFrame);
    convertImageParams(outputBuffers[0], outputFrame);

    MLOGI(Mia2LogGroupPlugin,
          "%s:%d input width: %d, height: %d, stride: %d, scanline: %d, output stride: %d "
          "scanline: %d",
          __func__, __LINE__, inputFrame.width, inputFrame.height, inputFrame.stride,
          inputFrame.scanline, outputFrame.stride, outputFrame.scanline);

    mWidth = inputFrame.width;
    mHeight = inputFrame.height;

    // dump input image start
    if (mDumpData) {
        char inputbuf[128];
        snprintf(inputbuf, sizeof(inputbuf), "Depurple_input_%dx%d_Sensor_%s_IZOOM_%d",
                 inputFrame.width, inputFrame.height, SensorIdString(mCFRModeInfo.sensorId),
                 mCFRModeInfo.isInSensorZoom);
        PluginUtils::dumpToFile(inputbuf, &inputFrame);
    }
    // dump input image end

    if (!mNodeInstance) {
        int ret = PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        if (ret == -1) {
            resultInfo = PROCFAILED;
        }
        MLOGE(Mia2LogGroupPlugin, " miCopyBuffer ret: %d", ret);
        return resultInfo;
    }

    // to make sure fastshot S2S time, switch to lite algo when concurrent running task >=3
    // resume to normal algo when concurrent<=2. (for now maxConcurrentNum = 5).
    if (processRequestInfo->runningTaskNum >= sDepurpleLiteTaskLimit)
        mLiteEnable = 1;
    else if (processRequestInfo->runningTaskNum < sDepurpleLiteTaskLimit)
        mLiteEnable = 0;
    MLOGI(Mia2LogGroupPlugin,
          "Depurple lite:liteDepurple=%d, runningTaskNum=%d, maxConcurrentNum=%d", mLiteEnable,
          processRequestInfo->runningTaskNum, processRequestInfo->maxConcurrentNum);

    if (mLiteEnable) {
        int ret = PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        if (mPreprocess) {
            mNodeInstance->decInit();
        }
        UpdateExifData(processRequestInfo->frameNum, processRequestInfo->timeStamp, luxIndex,
                       &zoomInfo, mLiteEnable);
        return resultInfo;
    }

    if (!mPreprocess) {
        mNodeInstance->incInit(mCFRModeInfo, true);
    }

    luxIndex = getLuxIndex(metadata);
    if (NULL != physicalmetadata) {
        mCFRModeInfo.zoomRatio = getZoomRatio(physicalmetadata);
        getZoomInfo(&zoomInfo, &cropRegion, physicalmetadata);
        getFaceInfo(&faceInfo, &cropRegion, physicalmetadata);
    } else {
        mCFRModeInfo.zoomRatio = getZoomRatio(metadata);
        getZoomInfo(&zoomInfo, &cropRegion, metadata);
        getFaceInfo(&faceInfo, &cropRegion, metadata);
    }
    int ret = mNodeInstance->process(&inputFrame, faceInfo, luxIndex, zoomInfo, mCFRModeInfo);
    if (ret) {
        MLOGE(Mia2LogGroupPlugin, "[CAM_LOG_SYSTEM] depurple process fail %d, skip node", ret);
    } else {
        UpdateExifData(processRequestInfo->frameNum, processRequestInfo->timeStamp, luxIndex,
                       &zoomInfo, mLiteEnable);
    }

    PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);

    if (mPreprocess) {
        mNodeInstance->decInit();
    }

    // dump output image start
    if (mDumpData) {
        char outputBuf[128];
        snprintf(outputBuf, sizeof(outputBuf), "Depurple_output_%dx%d_Sensor_%s_IZOOM_%d",
                 inputFrame.width, inputFrame.height, SensorIdString(mCFRModeInfo.sensorId),
                 mCFRModeInfo.isInSensorZoom);
        PluginUtils::dumpToFile(outputBuf, &outputFrame);
    }

    if (mDrawFaceRect) {
        drawFaceRect(&outputFrame, &faceInfo);
    }
    mProcessCount++;
    // dump output image end
    return resultInfo;
}

ProcessRetStatus DepurplePlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    camera_metadata_t *metadata = inputBuffers[0].pMetadata;
    camera_metadata_t *physicalmetadata = inputBuffers[0].pPhyCamMetadata;
    alCFR_FACE_T faceInfo;
    memset(&faceInfo, 0, sizeof(alCFR_FACE_T));
    int luxIndex = 0;
#if defined(NUWA_CAM) || defined(FUXI_CAM) || defined(SOCRATES_CAM)
    alCFR_RECT zoomInfo;
    memset(&zoomInfo, 0, sizeof(alCFR_RECT));
#else
    alGE_RECT zoomInfo;
    memset(&zoomInfo, 0, sizeof(alGE_RECT));
#endif
    CHIRECT cropRegion;
    memset(&cropRegion, 0, sizeof(CHIRECT));
    mActiveSize = {0, 0};

    MiImageBuffer inputFrame;
    convertImageParams(inputBuffers[0], inputFrame);

    mWidth = inputFrame.width;
    mHeight = inputFrame.height;

    MLOGI(Mia2LogGroupPlugin, "%s:%d input width: %d, height: %d, stride: %d, scanline: %d",
          __func__, __LINE__, inputFrame.width, inputFrame.height, inputFrame.stride,
          inputFrame.scanline);

    // dump input image start
    if (mDumpData) {
        char inputbuf[128];
        snprintf(inputbuf, sizeof(inputbuf), "Depurple_input_%dx%d_Sensor_%s_IZOOM_%d",
                 inputFrame.width, inputFrame.height, SensorIdString(mCFRModeInfo.sensorId),
                 mCFRModeInfo.isInSensorZoom);
        PluginUtils::dumpToFile(inputbuf, &inputFrame);
    }
    // dump input image end

    // to make sure fastshot S2S time, switch to lite algo when concurrent running task >=3
    // resume to normal algo when concurrent<=2. (for now maxConcurrentNum = 5).
    if (processRequestInfo->runningTaskNum >= sDepurpleLiteTaskLimit)
        mLiteEnable = 1;
    else if (processRequestInfo->runningTaskNum < sDepurpleLiteTaskLimit)
        mLiteEnable = 0;
    MLOGI(Mia2LogGroupPlugin,
          "Depurple lite:liteDepurple=%d, runningTaskNum=%d, maxConcurrentNum=%d", mLiteEnable,
          processRequestInfo->runningTaskNum, processRequestInfo->maxConcurrentNum);

    if (!mNodeInstance) {
        MLOGE(Mia2LogGroupPlugin, "mNodeInstance is NULL");
        return resultInfo;
    }

    if (mLiteEnable) {
        if (mPreprocess) {
            mNodeInstance->decInit();
        }
        UpdateExifData(processRequestInfo->frameNum, processRequestInfo->timeStamp, luxIndex,
                       &zoomInfo, mLiteEnable);
        return resultInfo;
    }

    if (!mPreprocess) {
        mNodeInstance->incInit(mCFRModeInfo, true);
    }

    luxIndex = getLuxIndex(metadata);
    if (NULL != physicalmetadata) {
        mCFRModeInfo.zoomRatio = getZoomRatio(physicalmetadata);
        getZoomInfo(&zoomInfo, &cropRegion, physicalmetadata);
        getFaceInfo(&faceInfo, &cropRegion, physicalmetadata);
    } else {
        mCFRModeInfo.zoomRatio = getZoomRatio(metadata);
        getZoomInfo(&zoomInfo, &cropRegion, metadata);
        getFaceInfo(&faceInfo, &cropRegion, metadata);
    }

    int ret = mNodeInstance->process(&inputFrame, faceInfo, luxIndex, zoomInfo, mCFRModeInfo);
    if (ret) {
        MLOGE(Mia2LogGroupPlugin, "[CAM_LOG_SYSTEM] depurple process fail %d, skip node", ret);
    } else {
        UpdateExifData(processRequestInfo->frameNum, processRequestInfo->timeStamp, luxIndex,
                       &zoomInfo, mLiteEnable);
    }

    if (mPreprocess) {
        mNodeInstance->decInit();
    }

    // dump output image start
    if (mDumpData) {
        char outputBuf[128];
        snprintf(outputBuf, sizeof(outputBuf), "Depurple_output_%dx%d_Sensor_%s_IZOOM_%d",
                 inputFrame.width, inputFrame.height, SensorIdString(mCFRModeInfo.sensorId),
                 mCFRModeInfo.isInSensorZoom);
        PluginUtils::dumpToFile(outputBuf, &inputFrame);
    }

    if (mDrawFaceRect) {
        drawFaceRect(&inputFrame, &faceInfo);
    }

    // dump output image end
    mProcessCount++;
    return resultInfo;
}

int DepurplePlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

void DepurplePlugin::destroy()
{
    if (mNodeInstance) {
        delete mNodeInstance;
        mNodeInstance = NULL;
    }
    MLOGI(Mia2LogGroupPlugin, "X");
}

int DepurplePlugin::getSensorId(camera_metadata_t *metaData, int cameraAppId)
{
    int sensorId = INVALID_SENSORID;
    // get zsl sensor id
    if (cameraAppId < mZSLmask) {
        if (CAM_COMBMODE_REAR_WIDE == cameraAppId) {
            sensorId = BACK_WIDE;
        } else if (CAM_COMBMODE_REAR_TELE == cameraAppId) {
            sensorId = BACK_TELE;
        } else if (CAM_COMBMODE_REAR_ULTRA == cameraAppId) {
            sensorId = BACK_UWIDE;
        } else if (CAM_COMBMODE_REAR_MACRO == cameraAppId) {
            sensorId = BACK_MACRO;
        } else {
            sensorId = BACK_TELE4X;
        }
        MLOGI(Mia2LogGroupPlugin, "DepurplePlugin sensorId: %d cameraID = %d", sensorId,
              cameraAppId);
    }
    // get sat sensor id
    else {
        void *data = NULL;
        const char *MultiCamera = "com.qti.chi.multicamerainfo.MultiCameraIds";
        VendorMetadataParser::getVTagValue(metaData, MultiCamera, &data);
        uint32_t cameraId;
        CameraRoleType cameraRole = CameraRoleTypeWide;
        if (NULL != data) {
            MultiCameraIds inputMetadata = *static_cast<MultiCameraIds *>(data);
            cameraRole = inputMetadata.currentCameraRole;
            cameraId = inputMetadata.currentCameraId;
            MLOGI(Mia2LogGroupPlugin,
                  "DepurplePlugin getMetaData currentCameraRole: %d currentCameraId: %d",
                  cameraRole, cameraId);
        }

        if (CameraRoleTypeWide == cameraRole) {
            sensorId = BACK_WIDE;
        } else if (CameraRoleTypeTele == cameraRole) {
            sensorId = BACK_TELE;
        } else if (CameraRoleTypeUltraWide == cameraRole) {
            sensorId = BACK_UWIDE;
        } else if (CameraRoleTypeTele4X == cameraRole) {
            sensorId = BACK_TELE4X;
        }
        // } else if (CameraRoleTypeMacro == cameraRole) {
        //     sensorId = BACK_MACRO;
        // }
        MLOGI(Mia2LogGroupPlugin, "DepurplePlugin sensorId: %d", sensorId);
    }
    return sensorId;
}

int DepurplePlugin::getVendorId(INT32 sendorId)
{
    char prop[PROPERTY_VALUE_MAX];
    INT32 vendorId = 1;
    memset(prop, 0, sizeof(prop));
    if (sendorId == BACK_WIDE) {
        property_get("persist.vendor.camera.rearMain.vendorID", prop, "0");
    } else if (sendorId == BACK_TELE) {
        property_get("persist.vendor.camera.rearTele.vendorID", prop, "0");
    } else if (sendorId == BACK_UWIDE) {
        property_get("persist.vendor.camera.rearUltra.vendorID", prop, "0");
    } else if (sendorId == BACK_TELE4X) {
        property_get("persist.vendor.camera.rearTele4x.vendorID", prop, "0");
    } else if (sendorId == BACK_MACRO) {
        property_get("persist.vendor.camera.rearMacro.vendorID", prop, "0");
    } else {
        MLOGW(Mia2LogGroupPlugin, "error invalid cameraRole");
        return vendorId;
    }
    vendorId = (uint32_t)atoi(prop);
    MLOGI(Mia2LogGroupPlugin, "get vendorid: 0x%x, sensorid: %d, prop: %s", vendorId, sendorId,
          prop);
    return vendorId;
}

#if defined(NUWA_CAM) || defined(FUXI_CAM) || defined(SOCRATES_CAM)
void DepurplePlugin::getZoomInfo(alCFR_RECT *zoomInfo, CHIRECT *cropRegion,
                                 camera_metadata_t *metaData)
#else
void DepurplePlugin::getZoomInfo(alGE_RECT *zoomInfo, CHIRECT *cropRegion,
                                 camera_metadata_t *metaData)
#endif
{
    void *data = NULL;
#if defined(INGRES_CAM)
    const char *cropInfo = "xiaomi.snapshot.residualCropRegion";
#else
    const char *cropInfo = "xiaomi.snapshot.cropRegion";
#endif
    int isAndroidRegion = VendorMetadataParser::getVTagValue(metaData, cropInfo, &data);
    if (NULL != data) {
        *cropRegion = *reinterpret_cast<CHIRECT *>(data);
        MLOGI(Mia2LogGroupPlugin, "MiaNodeDePurple get CropRegion begin [%d, %d, %d, %d]",
              cropRegion->left, cropRegion->top, cropRegion->width, cropRegion->height);
    } else if (NULL == data) {
        MLOGI(Mia2LogGroupPlugin,
              "MiaNodeDePurple get crop failed, try ANDROID_SCALER_CROP_REGION");
        VendorMetadataParser::getTagValue(metaData, ANDROID_SCALER_CROP_REGION, &data);
        if (NULL != data) {
            *cropRegion = *reinterpret_cast<CHIRECT *>(data);
            MLOGI(Mia2LogGroupPlugin, "MiaNodeDePurple get CropRegion begin [%d, %d, %d, %d]",
                  cropRegion->left, cropRegion->top, cropRegion->width, cropRegion->height);
        } else {
            MLOGE(Mia2LogGroupPlugin, "MiaNode get CropRegion failed");
            cropRegion->left = 0;
            cropRegion->top = 0;
            cropRegion->width = mWidth;
            cropRegion->height = mHeight;
        }
    }

    if (cropRegion->width == 0 || cropRegion->height == 0) {
        cropRegion->left = 0;
        cropRegion->top = 0;
        cropRegion->width = mWidth;
        cropRegion->height = mHeight;
    }

    GetActiveArraySize(metaData, &mActiveSize);
    if (mActiveSize.width == 0 || mActiveSize.height == 0) {
        mActiveSize.width = (cropRegion->left * 2) + cropRegion->width;
        mActiveSize.height = (cropRegion->top * 2) + cropRegion->height;
    }

    float pZoomRatio = 1.0;
    VendorMetadataParser::getTagValue(metaData, ANDROID_CONTROL_ZOOM_RATIO, &data);
    if (NULL != data) {
        pZoomRatio = *static_cast<float *>(data);
        MLOGI(Mia2LogGroupPlugin, "ZoomRatio  %f", pZoomRatio);
    } else {
        MLOGE(Mia2LogGroupPlugin, "get ZoomRatio failed");
    }

    INT32 pCoords[4] = {static_cast<INT32>(cropRegion->left), static_cast<INT32>(cropRegion->top),
                        static_cast<INT32>(cropRegion->width + cropRegion->left - 1),
                        static_cast<INT32>(cropRegion->height + cropRegion->top - 1)};
    if (isAndroidRegion) {
        scaleCoordinates(pCoords, 2, 1.0 / pZoomRatio);
    }

    float scaledX = 1.0;
    float scaledY = 1.0;
    if (mActiveSize.width != 0 && mActiveSize.height != 0) {
        scaledX = (float)mWidth / mActiveSize.width;
        scaledY = (float)mHeight / mActiveSize.height;
    }

    zoomInfo->m_wLeft = pCoords[0] * scaledX;
    zoomInfo->m_wTop = pCoords[1] * scaledY;
    zoomInfo->m_wWidth = (pCoords[2] - pCoords[0] + 1) * scaledX;
    zoomInfo->m_wHeight = (pCoords[3] - pCoords[1] + 1) * scaledY;

    // now we validate the rect, and ajust it if out-of-bondary found.
    if (zoomInfo->m_wLeft + zoomInfo->m_wWidth > mWidth) {
        if (zoomInfo->m_wWidth <= mWidth) {
            zoomInfo->m_wLeft = mWidth - zoomInfo->m_wWidth;
        } else {
            zoomInfo->m_wLeft = 0;
            zoomInfo->m_wWidth = mWidth;
        }
        MLOGW(Mia2LogGroupPlugin, "crop left or width is wrong, ajusted it manually!!");
    }
    if (zoomInfo->m_wTop + zoomInfo->m_wHeight > mHeight) {
        if (zoomInfo->m_wHeight <= mHeight) {
            zoomInfo->m_wTop = mHeight - zoomInfo->m_wHeight;
        } else {
            zoomInfo->m_wTop = 0;
            zoomInfo->m_wHeight = mHeight;
        }
        MLOGW(Mia2LogGroupPlugin, "crop top or height is wrong, ajusted it manually!!");
    }
    MLOGI(Mia2LogGroupPlugin, "MiaNodeDePurple zoomInfo [%d, %d, %d, %d]", zoomInfo->m_wLeft,
          zoomInfo->m_wTop, zoomInfo->m_wWidth, zoomInfo->m_wHeight);
}

void DepurplePlugin::getFaceInfo(alCFR_FACE_T *faceInfo, CHIRECT *cropRegion,
                                 camera_metadata_t *metaData)
{
    void *data = NULL;
    float zoomRatio = 1.0;

    if (cropRegion->width != 0)
        zoomRatio = (float)mWidth / cropRegion->width;

    float scaleWidth = 0.0;
    float scaleHeight = 0.0;
    if (mWidth != 0 && mHeight != 0) {
        scaleWidth = (float)cropRegion->width / mWidth;
        scaleHeight = (float)cropRegion->height / mHeight;
    } else {
        MLOGE(Mia2LogGroupPlugin, "get mWidth failed");
    }
    const float AspectRatioTolerance = 0.01f;

    if (scaleHeight > scaleWidth + AspectRatioTolerance) {
        // 16:9 case run to here, it means height is cropped.
        INT32 deltacropHeight = (INT32)((float)mHeight * scaleWidth);
        cropRegion->top = cropRegion->top + (cropRegion->height - deltacropHeight) / 2;
    } else if (scaleWidth > scaleHeight + AspectRatioTolerance) {
        // 1:1 width is cropped.
        INT32 deltacropWidth = (INT32)((float)mWidth * scaleHeight);
        cropRegion->left = cropRegion->left + (cropRegion->width - deltacropWidth) / 2;
        zoomRatio = (float)mHeight / cropRegion->height;
    }

    size_t dataSize = 0;
    faceInfo->m_dFaceInfoCount = 0;
    data = NULL;
    const char *faceRect = "xiaomi.snapshot.faceRect";
    VendorMetadataParser::getVTagValue(metaData, faceRect, &data);
    if (NULL != data) {
        FaceResult *tmpData = reinterpret_cast<FaceResult *>(data);
        faceInfo->m_dFaceInfoCount = tmpData->faceNumber;
        for (int index = 0; index < faceInfo->m_dFaceInfoCount; index++) {
            ChiRectEXT curFaceInfo = tmpData->chiFaceInfo[index];
            int32_t xMin = (curFaceInfo.left - static_cast<int32_t>(cropRegion->left)) * zoomRatio;
            int32_t xMax = (curFaceInfo.right - static_cast<int32_t>(cropRegion->left)) * zoomRatio;
            int32_t yMin = (curFaceInfo.bottom - static_cast<int32_t>(cropRegion->top)) * zoomRatio;
            int32_t yMax = (curFaceInfo.top - static_cast<int32_t>(cropRegion->top)) * zoomRatio;

            xMin = std::max(std::min(xMin, (INT32)mWidth - 2), 0);
            xMax = std::max(std::min(xMax, (INT32)mWidth - 2), 0);
            yMin = std::max(std::min(yMin, (INT32)mHeight - 2), 0);
            yMax = std::max(std::min(yMax, (INT32)mHeight - 2), 0);

            faceInfo->m_tFaceInfoWOI[index].m_wLeft = xMin;
            faceInfo->m_tFaceInfoWOI[index].m_wTop = yMin;
            faceInfo->m_tFaceInfoWOI[index].m_wWidth = xMax - xMin + 1;
            faceInfo->m_tFaceInfoWOI[index].m_wHeight = yMax - yMin + 1;
            MLOGI(Mia2LogGroupPlugin, "index:%d faceRect[%d,%d,%d,%d] cvt [%d,%d,%d,%d]", index,
                  curFaceInfo.left, curFaceInfo.right, curFaceInfo.bottom, curFaceInfo.top,
                  faceInfo->m_tFaceInfoWOI[index].m_wLeft, faceInfo->m_tFaceInfoWOI[index].m_wTop,
                  faceInfo->m_tFaceInfoWOI[index].m_wWidth,
                  faceInfo->m_tFaceInfoWOI[index].m_wHeight);
        }
    } else {
        MLOGI(Mia2LogGroupPlugin, "MiaNodeDePurple faces are not found");
    }
}

int DepurplePlugin::getLuxIndex(camera_metadata_t *metaData)
{
    void *data = NULL;
    int luxIndex = 0;
    const char *AecFrameControl = "org.quic.camera2.statsconfigs.AECFrameControl";
    VendorMetadataParser::getVTagValue(metaData, AecFrameControl, &data);
    if (NULL != data) {
        luxIndex = (int)static_cast<AECFrameControl *>(data)->luxIndex;
    }
    MLOGI(Mia2LogGroupPlugin, "get luxIndex: %d", luxIndex);
    return luxIndex;
}

bool DepurplePlugin::getSNMode(camera_metadata_t *metaData)
{
    bool isSuperNight = false;
    bool isNightMotion = false;
    int32_t superNightMode = 0;
    void *data = NULL;
    const char *tagSuperNightEnabled = "xiaomi.mivi.supernight.mode";
    VendorMetadataParser::getVTagValue(metaData, tagSuperNightEnabled, &data);
    if (NULL != data) {
        superNightMode = *static_cast<int32_t *>(data);
    }
    data = NULL;
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.supernight.enabled", &data);
    if (NULL != data) {
        isSuperNight = *static_cast<uint8_t *>(data);
    }

    data = NULL;
    VendorMetadataParser::getVTagValue(metaData, "xiaomi.nightmotioncapture.mode", &data);
    if (NULL != data) {
        isNightMotion = *static_cast<uint8_t *>(data);
    }

    isSuperNight |= (superNightMode >= 1) && (superNightMode <= 9);
    isSuperNight |= isNightMotion;
    MLOGI(Mia2LogGroupPlugin, "supernight  mode %d enabled %d", superNightMode, isSuperNight);
    return isSuperNight;
}

uint32_t DepurplePlugin::getZoomRatio(camera_metadata_t *metaData)
{
    float pZoomRatio = 1.0;
    void *data = NULL;
#if defined(INGRES_CAM)
    VendorMetadataParser::getTagValue(metaData, ANDROID_CONTROL_ZOOM_RATIO, &data);
#else
    const char *userzoom = "xiaomi.snapshot.userZoomRatio";
    VendorMetadataParser::getVTagValue(metaData, userzoom, &data);
#endif
    if (NULL != data) {
        pZoomRatio = *static_cast<float *>(data);
        MLOGI(Mia2LogGroupPlugin, "userZoomRatio: %f", pZoomRatio);
    }
    return (pZoomRatio >= DEPURPLE_ZOOMRATIO_FOUR) ? DEPURPLE_ZOOMRATIO_FOUR : 0;
}

bool DepurplePlugin::getHighDefinitionMode(camera_metadata_t *metaData)
{
    bool isHD = false;
    const char *tagRemosaicSize = "xiaomi.remosaic.enabled";
    void *data = NULL;
    VendorMetadataParser::getVTagValue(metaData, tagRemosaicSize, &data);
    if (NULL != data) {
        isHD = *static_cast<uint8_t *>(data);
        if ((mialgo2::StreamConfigModeSAT == mOperationMode) && isHD) {
            mCFRModeInfo.isInSensorZoom = true;
            isHD = false;
        }
        // add for L3 skip 50M in Manual Mode
#if defined(CUPID_CAM) || defined(INGRES_CAM) || defined(MAYFLY_CAM) || defined(DITING_CAM)
        if (mialgo2::StreamConfigModeMiuiManual == mOperationMode) {
            isHD = true;
            MLOGI(Mia2LogGroupPlugin, "remosaic enabled %d mOperationMode %d", isHD,
                  mOperationMode);
        }
#endif
        MLOGI(Mia2LogGroupPlugin, "remosaic enabled %d InSensorZoom %d", isHD,
              mCFRModeInfo.isInSensorZoom);
    } else {
        MLOGW(Mia2LogGroupPlugin, "get remosaic enabled failed");
    }

    return isHD;
}

bool DepurplePlugin::getHDRMode(camera_metadata_t *metaData)
{
    bool isHDR = false;
    const char *tagHDRmode = "xiaomi.hdr.raw.enabled";
    void *data = NULL;
    VendorMetadataParser::getVTagValue(metaData, tagHDRmode, &data);
    if (NULL != data) {
        isHDR = *static_cast<uint8_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "hdr enabled %d", isHDR);
    } else {
        MLOGW(Mia2LogGroupPlugin, "get xiaomi.hdr.raw.enabled failed");
    }
    return isHDR;
}

void DepurplePlugin::GetActiveArraySize(camera_metadata_t *metaData, ChiDimension *pActiveArraySize)
{
    void *pData = NULL;
    int cameraId = 0;
    const char *tagfwkCameraId = "xiaomi.snapshot.fwkCameraId";
    VendorMetadataParser::getVTagValue(metaData, tagfwkCameraId, &pData);
    if (NULL != pData) {
        cameraId = *static_cast<int32_t *>(pData);
        camera_metadata_t *staticMetadata =
            StaticMetadataWraper::getInstance()->getStaticMetadata(cameraId);
        pData = NULL;
        VendorMetadataParser::getTagValue(staticMetadata, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
                                          &pData);
        if (NULL != pData) {
            CHIRECT activeArray = *static_cast<CHIRECT *>(pData);
            pActiveArraySize->width = activeArray.width;
            pActiveArraySize->height = activeArray.height;
            MLOGI(Mia2LogGroupPlugin, "mActiveSize %d %d", pActiveArraySize->width,
                  pActiveArraySize->height);
        }
    }
}

void DepurplePlugin::scaleCoordinates(int32_t *coordPairs, int coordCount, float scaleRatio)
{
    // A pixel's coordinate is represented by the position of its top-left corner.
    // To avoid the rounding error, we use the coordinate for the center of the
    // pixel instead:
    // 1. First shift the coordinate system half pixel both horizontally and
    // vertically, so that [x, y] is the center of the pixel, not the top-left corner.
    // 2. Do zoom operation to scale the coordinate relative to the center of
    // the active array (shifted by 0.5 pixel as well).
    // 3. Shift the coordinate system back by directly using the pixel center
    // coordinate.
    for (int i = 0; i < coordCount * 2; i += 2) {
        float x = coordPairs[i];
        float y = coordPairs[i + 1];
        float xCentered = x - (mActiveSize.width - 2) / 2;
        float yCentered = y - (mActiveSize.height - 2) / 2;
        float scaledX = xCentered * scaleRatio;
        float scaledY = yCentered * scaleRatio;
        scaledX += (mActiveSize.width - 2) / 2;
        scaledY += (mActiveSize.height - 2) / 2;
        coordPairs[i] = static_cast<int32_t>(round(scaledX));
        coordPairs[i + 1] = static_cast<int32_t>(round(scaledY));
        int32_t right = mActiveSize.width - 1;
        int32_t bottom = mActiveSize.height - 1;
        coordPairs[i] = std::min(right, std::max(0, coordPairs[i]));
        coordPairs[i + 1] = std::min(bottom, std::max(0, coordPairs[i + 1]));
        MLOGD(Mia2LogGroupPlugin, "coordinates: %d, %d", coordPairs[i], coordPairs[i + 1]);
    }
}

void DepurplePlugin::prepareImage(MiImageBuffer *image, ASVLOFFSCREEN &stImage)
{
    stImage.i32Width = image->width;
    stImage.i32Height = image->height;

    if (image->format != CAM_FORMAT_YUV_420_NV12 && image->format != CAM_FORMAT_YUV_420_NV21 &&
        image->format != CAM_FORMAT_Y16) {
        MLOGE(Mia2LogGroupPlugin, "[ARC_CTB] format[%d] is not supported!", image->format);
        return;
    }
    // MLOGD(Mia2LogGroupPlugin, "stImage(%dx%d) PixelArrayFormat:%d",
    // stImage.i32Width,stImage.i32Height, pNodeBuff->PixelArrayFormat);
    if (image->format == CAM_FORMAT_YUV_420_NV12) {
        stImage.u32PixelArrayFormat = ASVL_PAF_NV12;
        MLOGD(Mia2LogGroupPlugin, "stImage(ASVL_PAF_NV12)");
    } else if (image->format == CAM_FORMAT_YUV_420_NV21) {
        stImage.u32PixelArrayFormat = ASVL_PAF_NV21;
        MLOGD(Mia2LogGroupPlugin, "stImage(ASVL_PAF_NV21)");
    } else if (image->format == CAM_FORMAT_Y16) {
        stImage.u32PixelArrayFormat = ASVL_PAF_GRAY;
        MLOGD(Mia2LogGroupPlugin, "stImage(ASVL_PAF_GRAY)");
    }

    for (uint32_t i = 0; i < 2; i++) {
        stImage.ppu8Plane[i] = image->plane[i];
        stImage.pi32Pitch[i] = image->stride;
    }
    MLOGD(Mia2LogGroupPlugin, "X");
}

void DepurplePlugin::drawFaceRect(MiImageBuffer *image, alCFR_FACE_T *faceInfo)
{
    ASVLOFFSCREEN stOutImage;
    prepareImage(image, stOutImage);
    for (int i = 0; i < faceInfo->m_dFaceInfoCount; i++) {
        MRECT RectDraw = {0};
        RectDraw.left = faceInfo->m_tFaceInfoWOI[i].m_wLeft;
        RectDraw.top = faceInfo->m_tFaceInfoWOI[i].m_wTop;
        RectDraw.bottom =
            faceInfo->m_tFaceInfoWOI[i].m_wTop + faceInfo->m_tFaceInfoWOI[i].m_wHeight;
        RectDraw.right = faceInfo->m_tFaceInfoWOI[i].m_wLeft + faceInfo->m_tFaceInfoWOI[i].m_wWidth;
        DrawRect(&stOutImage, RectDraw, 5, 0);
    }
}

#if defined(NUWA_CAM) || defined(FUXI_CAM) || defined(SOCRATES_CAM)
void DepurplePlugin::UpdateExifData(uint32_t frameNum, int64_t timeStamp, int lux,
                                    alCFR_RECT *zoomInfo, int liteEnable)
#else
void DepurplePlugin::UpdateExifData(uint32_t frameNum, int64_t timeStamp, int lux,
                                    alGE_RECT *zoomInfo, int liteEnable)
#endif
{
    if (mNodeInterface.pSetResultMetadata != NULL) {
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf), "depurple:{on:1 lux:%d zoom:%d,%d,%d,%d lite:%d}", lux,
                 zoomInfo->m_wLeft, zoomInfo->m_wTop, zoomInfo->m_wWidth, zoomInfo->m_wHeight,
                 liteEnable);
        std::string results(buf);
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, frameNum, timeStamp, results);
    }
}
