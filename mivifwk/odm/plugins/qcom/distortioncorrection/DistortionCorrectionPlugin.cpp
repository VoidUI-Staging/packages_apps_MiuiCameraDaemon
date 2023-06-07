#include "DistortionCorrectionPlugin.h"

#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "alCFR.h"
#include "alGE.h"
#include "arcsoft_utils.h"

#define FRONT_CAM_SENSOR_WIDTH  3008
#define FRONT_CAM_SENSOR_HEIGHT 2256
#define REAR_CAM_SENSOR_WIDTH   6016
#define REAR_CAM_SENSOR_HEIGHT  4512
#define FRONT_CAM_ID            1

namespace mialgo2 {

ArcDCPlugin::~ArcDCPlugin() {}

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

int ArcDCPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mDistortionCorrection = NULL;
    mProcessedFrame = 0;
    mRuntimeMode = RUNTIMEMODE_SNAPSHOT;
    mInitialized = false;
    mDistortionLevel = 0;
    mInstanceName = createInfo->nodeInstance;
    mNodeInterface = nodeInterface;

    memset(&mFaceParams, 0, sizeof(ARC_DC_FACE));
    mFaceParams.prtFace = (PMRECT)malloc(sizeof(MRECT) * MAX_FACE_NUM);
    mFaceParams.i32faceOrient = (MInt32 *)malloc(sizeof(MInt32) * MAX_FACE_NUM);
    memset(mFaceParams.prtFace, 0, sizeof(MRECT) * MAX_FACE_NUM);
    memset(mFaceParams.i32faceOrient, 0, sizeof(MInt32) * MAX_FACE_NUM);
    mFaceParams.i32FacesNum = 0;

    mFaceNum = 0;
    mSnapshotWidth = 0;
    mSnapshotHeight = 0;
    mSensorWidth = 0;
    mSensorHeight = 0;

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.arc.ds.mode", prop, "0");
    mDsMode = atoi(prop);
    // 0, face protection
    // 1, optical correction
    // 2, optical correction and face protection
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.arc.ds.debug", prop, "0");
    mDebug = atoi(prop);
    mFrameworkCameraId = createInfo->frameworkCameraId;
    MLOGD(Mia2LogGroupPlugin, "[ARCSOFT]: mDsMode: %d, frameworkCameraId 0x%x", mDsMode,
          createInfo->frameworkCameraId);

    if (!mDistortionCorrection) {
        mDistortionCorrection = new ArcsoftDistortionCorrection(mDsMode);
    }

    return MIA_RETURN_OK;
}

void ArcDCPlugin::destroy()
{
    if (mDistortionCorrection) {
        delete mDistortionCorrection;
        mDistortionCorrection = NULL;
    }

    if (mFaceParams.prtFace) {
        free(mFaceParams.prtFace);
        mFaceParams.prtFace = NULL;
    }
    if (mFaceParams.i32faceOrient) {
        free(mFaceParams.i32faceOrient);
        mFaceParams.i32faceOrient = NULL;
    }
}

int ArcDCPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

bool ArcDCPlugin::isEnabled(MiaParams *settings)
{
    void *data = NULL;
    const char *distortionLevelApplied = "xiaomi.distortion.distortionLevelApplied";
    VendorMetadataParser::getVTagValue(settings.metadata, distortionLevelApplied, &data);
    if (NULL != data) {
        mDistortionLevel = *static_cast<uint8_t *>(data);
    } else {
        mDistortionLevel = 0;
    }

    return mDistortionLevel ? true : false;
}

ProcessRetStatus ArcDCPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    bool isEndNode = false;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.algoengine.distortion.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);

    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;

    MiImageBuffer inputFrame, outputFrame;
    convertImageParams(inputBuffers[0], inputFrame);
    convertImageParams(outputBuffers[0], outputFrame);

    camera_metadata_t *metadata = inputBuffers[0].pMetadata;
    camera_metadata_t *physicalMetadata = inputBuffers[0].pPhyCamMetadata;

    if (iIsDumpData) {
        char inputbuf[128];
        snprintf(inputbuf, sizeof(inputbuf), "distortion_input_%dx%d", inputFrame.width,
                 inputFrame.height);
        PluginUtils::dumpToFile(inputbuf, &inputFrame);
    }

    mProcessedFrame = processRequestInfo->frameNum;

    double startTime = gettime();
    if (physicalMetadata != NULL) {
        resultInfo = processBuffer(&inputFrame, &outputFrame, physicalMetadata);
    } else {
        resultInfo = processBuffer(&inputFrame, &outputFrame, metadata);
    }

    // add Exif info
    updateExifData(processRequestInfo, resultInfo, gettime() - startTime);

    if (mMemcopy) {
        PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        mMemcopy = false;
    }

    if (iIsDumpData) {
        char outputbuf[128];
        snprintf(outputbuf, sizeof(outputbuf), "distortion_output_%dx%d", outputFrame.width,
                 outputFrame.height);
        PluginUtils::dumpToFile(outputbuf, &outputFrame);
    }
    return resultInfo;
};

ProcessRetStatus ArcDCPlugin::processBuffer(MiImageBuffer *input, MiImageBuffer *output,
                                            camera_metadata_t *metaData)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    if (input == NULL || output == NULL || metaData == NULL) {
        MLOGE(Mia2LogGroupPlugin, "error invalid param %p, %p, %p", input, output, metaData);
        return PROCFAILED;
    }

    mMemcopy = false;
    mCamMode = 1;
    CameraRoleType currentCameraRole = CameraRoleTypeDefault;
    uint32_t logicalCameraId = 4;
    void *data = NULL;
    MultiCameraIdRole multiCameraRole;
    const char *MultiCameraRole = "com.qti.chi.multicamerainfo.MultiCameraIds";
    VendorMetadataParser::getVTagValue(metaData, MultiCameraRole, &data);
    if (NULL != data) {
        multiCameraRole = *static_cast<MultiCameraIdRole *>(data);
        logicalCameraId = multiCameraRole.logicalCameraId;
        mCamMode = multiCameraRole.logicalCameraId;
        MLOGI(Mia2LogGroupPlugin, "get currentCameraId = %d logicalCameraId = %d cameraRole = %d",
              multiCameraRole.currentCameraId, multiCameraRole.logicalCameraId,
              multiCameraRole.currentCameraRole);
    }

    ASVLOFFSCREEN inputFrame, outputFrame;
    if ((((currentCameraRole == CameraRoleTypeWide) || (multiCameraRole.logicalCameraId == 0)) &&
         (mDistortionLevel > 0)) ||
        (mDebug > 0)) {
        mSnapshotWidth = input->width;
        mSnapshotHeight = input->height;
        getMetaData(metaData);

        inputFrame.u32PixelArrayFormat = ASVL_PAF_NV12;
        inputFrame.i32Width = input->width;
        inputFrame.i32Height = input->height;

        for (uint32_t j = 0; j < input->numberOfPlanes; j++) {
            inputFrame.pi32Pitch[j] = input->stride;
            inputFrame.ppu8Plane[j] = input->plane[j];
        }

        outputFrame.u32PixelArrayFormat = ASVL_PAF_NV12;
        outputFrame.i32Width = output->width;
        outputFrame.i32Height = output->height;
        for (uint32_t j = 0; j < output->numberOfPlanes; j++) {
            outputFrame.pi32Pitch[j] = output->stride;
            outputFrame.ppu8Plane[j] = output->plane[j];
        }
        if (mDistortionCorrection) {
#if 0
            if(mDsMode != 0) {
                unsigned char calibData[CALIBRATIONDATA_LEN];
                memset(calibData, 0, sizeof(calibData));

                GetCalibrationData(mCamMode, calibData, CALIBRATIONDATA_LEN);
                mDistortionCorrection->setcalibrationdata(calibData, CALIBRATIONDATA_LEN);
            }
#endif
            if (mInitialized == false) {
                mDistortionCorrection->init(mCamMode, mRuntimeMode);
                mInitialized = true;
            }
            mDistortionCorrection->process(&inputFrame, &outputFrame, &mFaceParams, mCamMode,
                                           mRuntimeMode);
            MLOGI(Mia2LogGroupPlugin, "DistortionCorrection process done");
            for (int i = 0; i < mFaceParams.i32FacesNum; ++i) {
                char prop[PROPERTY_VALUE_MAX];
                memset(prop, 0, sizeof(prop));
                property_get("persist.vendor.camera.arc.ds.drawface", prop, "0");
                if (atoi(prop)) {
                    DrawRect(&outputFrame, mFaceParams.prtFace[i], 5, 0);
                }
            }
        } else {
            MLOGI(Mia2LogGroupPlugin, "arcdebug Distortion copy image from in to out");
            mMemcopy = true;
        }
    } else {
        MLOGI(Mia2LogGroupPlugin, "arcdebug Distortion copy image from in to out");
        mMemcopy = true;
    }

    return resultInfo;
}

ProcessRetStatus ArcDCPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

void ArcDCPlugin::getMetaData(camera_metadata_t *metaData)
{
    if (metaData == NULL) {
        return;
    }

    // get facenums
    void *data = NULL;
    camera_metadata_entry_t entry = {0};
    uint32_t numElemsRect = sizeof(MIRECT) / sizeof(uint32_t);
    if (0 != find_camera_metadata_entry(metaData, ANDROID_STATISTICS_FACE_RECTANGLES, &entry)) {
        ALOGE("ArcDCPlugin, get ANDROID_STATISTICS_FACE_RECTANGLES failed");
        return;
    }
    mFaceNum = entry.count / numElemsRect;
    mFaceParams.i32FacesNum = mFaceNum;

    // get sensor size for mivifwk, rather than active size for hal
    MiDimension activeSize = {0};
    data = NULL;
    const char *SensorSize = "xiaomi.sensorSize.size";
    VendorMetadataParser::getVTagValue(metaData, SensorSize, &data);
    if (NULL != data) {
        activeSize = *(MiDimension *)(data);
        MLOGI(Mia2LogGroupPlugin, "ArcDCPlugin, get sensor activeSize %d %d", activeSize.width,
              activeSize.height);
        // sensor size is 1/2 active size
        mSensorWidth = activeSize.width / 2;
        mSensorHeight = activeSize.height / 2;
    } else {
        MLOGE(Mia2LogGroupPlugin, "ArcDCPlugin, get sensor activeSize failed");
        mSensorWidth = mSnapshotWidth;
        mSensorHeight = mSnapshotHeight;
    }
    MLOGI(Mia2LogGroupPlugin, "ArcDCPlugin, sensor: %d, %d; snapshot:%d, %d", mSensorWidth,
          mSensorHeight, mSnapshotWidth, mSnapshotHeight);

    // face rectangle
    data = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_STATISTICS_FACE_RECTANGLES, &data);
    if (NULL != data) {
        void *tmpData = reinterpret_cast<void *>(data);
        float zoomRatio = (float)mSnapshotWidth / mSensorWidth;
        float yOffSize = ((float)zoomRatio * mSensorHeight - mSnapshotHeight) / 2.0;
        MLOGI(Mia2LogGroupPlugin, "ArcDCPlugin zoomRatio:%f, yOffSize:%f", zoomRatio, yOffSize);

        uint32_t dataIndex = 0;
        for (int index = 0; index < mFaceNum; index++) {
            int32_t xMin = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t yMin = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t xMax = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t yMax = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            MLOGI(Mia2LogGroupPlugin,
                  "ArcDCPlugin, [arcsoft] origin face [xMin,yMin,xMax,yMax][%d,%d,%d,%d]", xMin,
                  yMin, xMax, yMax);
            xMin = xMin * zoomRatio;
            xMax = xMax * zoomRatio;
            yMin = yMin * zoomRatio - yOffSize;
            yMax = yMax * zoomRatio - yOffSize;
            MLOGI(Mia2LogGroupPlugin,
                  "ArcDCPlugin, [arcsoft] now face [xMin,yMin,xMax,yMax][%d,%d,%d,%d]", xMin, yMin,
                  xMax, yMax);

            switch (mFaceOrientation) {
            case 0:
                mFaceParams.i32faceOrient[index] = ADC_FOC_0;
                break;
            case 90:
                mFaceParams.i32faceOrient[index] = ADC_FOC_90;
                break;
            case 180:
                mFaceParams.i32faceOrient[index] = ADC_FOC_180;
                break;
            case 270:
                mFaceParams.i32faceOrient[index] = ADC_FOC_270;
                break;
            }
            mFaceParams.prtFace[index].left = xMin;
            mFaceParams.prtFace[index].top = yMin;
            mFaceParams.prtFace[index].right = xMax;
            mFaceParams.prtFace[index].bottom = yMax;
        }
    } else {
        MLOGI(Mia2LogGroupPlugin, "ArcDCPlugin faces are not found");
    }
}

void ArcDCPlugin::updateExifData(ProcessRequestInfo *processRequestInfo,
                                 ProcessRetStatus resultInfo, double processTime)
{
    // exif data format is distortionCorrection::{processTime:...}
    std::string results(mInstanceName);
    results += ":{";

    char data[1024] = {0};
    snprintf(data, sizeof(data), "procRet:%d procTime:%d faceNum:%d", resultInfo, (int)processTime,
             mFaceNum);
    std::string exifData(data);
    results += exifData;
    results += "}";

    if (NULL != mNodeInterface.pSetResultMetadata) {
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, processRequestInfo->frameNum,
                                          processRequestInfo->timeStamp, results);
    }
}

} // namespace mialgo2
