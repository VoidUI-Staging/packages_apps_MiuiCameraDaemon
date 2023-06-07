#include "PureViewPlugin.h"

#include <functional>
#include <iostream>

#include "MiaPluginUtils.h"
#include "pureView_api.h"
#include "pureView_basic.h"
using namespace std;

#undef LOG_TAG
#define LOG_TAG "MiA_N_PUREVIEW"

static const int32_t sIsDumpData = property_get_int32("persist.vendor.camera.pureview.dump", 0);

PureViewPlugin::~PureViewPlugin()
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);
}

int PureViewPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    if (NULL == pCreateInfo) {
        pvConfig.cameraAbnormalQuit = 1;
        return 0;
    }
    MLOGD(Mia2LogGroupPlugin, "%s:%d %p", __func__, __LINE__, this);
    // lijinglin change
    // pvConfig.inputImgFormat = pCreateInfo->inputFormat.format;
    PV_IMG_FORMAT inputformat = IMG_NV12;
    pvConfig.inputImgFormat = inputformat;
    pvConfig.imgWidth = pCreateInfo->inputFormat.width;
    pvConfig.imgHeight = pCreateInfo->inputFormat.height;
    pvConfig.cameraAbnormalQuit = 0;
    char *argv = "/vendor/etc/camera/pureView_parameter.xml";

    pvConfig.pXmlFileName = argv;
    pvConfig.callback =
        std::bind(&PureViewPlugin::PerFrameProcessDonecallback, this, std::placeholders::_1);
    MLOGE(Mia2LogGroupPlugin, "pureview initialize imgformat=%d imgwidth =%d imgHeight =%d",
          pvConfig.inputImgFormat, pvConfig.imgWidth, pvConfig.imgHeight);
    // preinit pureview
    int ret = pureViewPreInit(&pvConfig);
    if (ret != PV_OK) {
        MLOGE(Mia2LogGroupPlugin, "PureViewPreInit FAIL!! Abort to debug");
        abort();
    }
    mNodeInterface = nodeInterface;

    mStatus = INIT;
    mMultiNum = 1; // for test 3
    mProcThread = thread([this]() { loopFunc(); });
    return 0;
}

void PureViewPlugin::loopFunc()
{
    while (true) {
        // one by one
        unique_lock<std::mutex> threadLocker(mMutexLoop);

        if (mStatus == NEEDPROC) {
            if (!processFrame(mMultiNum)) {
                MLOGI(Mia2LogGroupPlugin, "Process Request finish");
            }
            mNodeInterface.pSetResultBuffer(mNodeInterface.owner, mFrameNum);
            unique_lock<std::mutex> threadLockerPerFrame(mMutexPerFrameReturn);
            mInProgress = false;
            mCondLastFrameReturn.notify_all();
            mStatus = PROCESSED;
        } else if (mStatus == END) {
            break;
        } else {
            mCondLoop.wait(threadLocker);
        }
    }
}

void PureViewPlugin::PerFrameProcessDonecallback(PV_IMG *)
{
    MLOGD(Mia2LogGroupPlugin, "receive callback");
    unique_lock<std::mutex> threadLocker(mMutexPerFrameReturn);
    mCondPerFrameReturn.notify_all();
}

int PureViewPlugin::processFrame(uint32_t inputFrameNum)
{
    int ret = ProcessBuffer(pvInput, inputFrameNum, &pvOutput, 0, mMetaData, mPhysMeta);
    return ret;
}

bool PureViewPlugin::isEnabled(MiaParams settings)
{
    if (NULL == settings.metadata) {
        pvConfig.cameraAbnormalQuit = 1;
        return 0;
    }
    void *pData = NULL;
    int PureViewEnable = false;
    const char *pvEnable = "xiaomi.pureView.enabled";
    VendorMetadataParser::getVTagValue(settings.metadata, pvEnable, &pData);
    if (NULL != pData) {
        PureViewEnable = *static_cast<int *>(pData);
    }
    MLOGD(Mia2LogGroupPlugin, "Get PureViewEnable: %d", PureViewEnable);

    return PureViewEnable;
}

ProcessRetStatus PureViewPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo;
    resultInfo = PROCSUCCESS;
    if (NULL == pProcessRequestInfo) {
        pvConfig.cameraAbnormalQuit = 1;
        resultInfo = PROCFAILED;
        return resultInfo;
    }
    int32_t iIsDumpData = sIsDumpData;
    int inputNum = pProcessRequestInfo->phInputBuffer.size();
    if (inputNum > PV_MAX_FRAME_NUM) {
        inputNum = PV_MAX_FRAME_NUM;
    }

    void *pData = NULL;
    VendorMetadataParser::getVTagValue(
        (camera_metadata_t *)pProcessRequestInfo->phInputBuffer[0].pMetadata,
        "xiaomi.burst.curReqIndex", &pData);
    if (pData) {
        unsigned int burstCurIdx = *static_cast<unsigned int *>(pData);
        unique_lock<std::mutex> threadLocker(mMutexPerFrameReturn);
        if ((1 == burstCurIdx) && mInProgress) {
            mCondLastFrameReturn.wait(threadLocker);
        } else {
            mInProgress = true;
        }
        if (1 == burstCurIdx) {
            VendorMetadataParser::getVTagValue(
                (camera_metadata_t *)pProcessRequestInfo->phInputBuffer[0].pMetadata,
                "xiaomi.burst.totalReqNum", &pData);
            if (pData) {
                mMultiNum = pvConfig.inputFrameNum = *((unsigned int *)pData);
            }
            if (NULL == pvInput) {
                pvInput = new PV_IMG[mMultiNum];
            }
            memset((void *)&pvConfig.frameReadyFlags[0], 0, sizeof(pvConfig.frameReadyFlags));
            memset((void *)pvInput, 0, sizeof(PV_IMG) * mMultiNum);
            mFrameNum = pProcessRequestInfo->frameNum;
            // lijinglin change
            // pvOutput.format = output->format;
            PV_IMG_FORMAT outputformat = IMG_P010;
            pvOutput.format = outputformat;
            mOutputFrameFormat = pProcessRequestInfo->phOutputBuffer[0].format.format;
            pvOutput.data[0] = pProcessRequestInfo->phOutputBuffer[0].pAddr[0];
            pvOutput.data[1] = pProcessRequestInfo->phOutputBuffer[0].pAddr[1];
            pvOutput.h = pProcessRequestInfo->phOutputBuffer[0].format.height;
            pvOutput.w = pProcessRequestInfo->phOutputBuffer[0].format.width;
            pvOutput.pitch[0] = pProcessRequestInfo->phOutputBuffer[0].planeStride;
            pvOutput.pitch[1] = pProcessRequestInfo->phOutputBuffer[0].planeStride;
            pvOutput.memInfo[0].fd = pProcessRequestInfo->phOutputBuffer[0].fd[0];
            pvOutput.memInfo[0].type = PV_MEM_ION;
            pvOutput.memInfo[0].size = pProcessRequestInfo->phOutputBuffer[0].planeStride *
                                           pProcessRequestInfo->phOutputBuffer[0].sliceheight +
                                       pProcessRequestInfo->phOutputBuffer[0].planeStride *
                                           pProcessRequestInfo->phOutputBuffer[0].sliceheight;
            mMetaData[0] = (void *)pProcessRequestInfo->phInputBuffer[0].pMetadata;
            mPhysMeta[0] = (void *)pProcessRequestInfo->phInputBuffer[0].pPhyCamMetadata;
            std::atomic_thread_fence(std::memory_order_seq_cst);
        }

        for (int i = burstCurIdx - 1; i < burstCurIdx - 1 + inputNum; i++) {
            // lijinglin change
            // pvInput[i].format = input[i].format;
            PV_IMG_FORMAT inputformat = IMG_NV12;
            pvInput[i].format = inputformat;
            mInputFrameFormat =
                pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].format.format;
            pvInput[i].data[0] = pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].pAddr[0];
            pvInput[i].data[1] = pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].pAddr[1];
            pvInput[i].h = pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].format.height;
            pvInput[i].w = pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].format.width;
            pvInput[i].pitch[0] =
                pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].planeStride;
            pvInput[i].pitch[1] =
                pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].planeStride;
            pvInput[i].memInfo[0].fd =
                pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].fd[0];
            pvInput[i].memInfo[0].type = PV_MEM_ION;
            pvInput[i].memInfo[0].size =
                pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].planeStride *
                pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].sliceheight * 3 / 2;
            if ((mMultiNum - 1) == burstCurIdx) {
                void *pBaseAnchor = NULL;
                VendorMetadataParser::getVTagValue(
                    (camera_metadata_t *)pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx]
                        .pMetadata,
                    "xiaomi.burst.baseAnchor", &pBaseAnchor);
                if (NULL != pBaseAnchor) {
                    pvConfig.baseAnchor = *static_cast<int *>(pBaseAnchor);
                    MLOGI(Mia2LogGroupPlugin, "pureview base anchor:%d", pvConfig.baseAnchor);
                }
            } else {
                pvConfig.baseAnchor = 0xFF;
            }
            if (iIsDumpData) {
                char inbuf[128];
                snprintf(inbuf, sizeof(inbuf), "pureview_input_%dx%d_%d", pvInput[i].w,
                         pvInput[i].h, i);
                struct MiImageBuffer InputFrame;
                InputFrame.format =
                    pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].format.format;
                InputFrame.width =
                    pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].format.width;
                InputFrame.height =
                    pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].format.height;
                InputFrame.plane[0] =
                    pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].pAddr[0];
                InputFrame.plane[1] =
                    pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].pAddr[1];
                InputFrame.stride =
                    pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].planeStride;
                InputFrame.scanline =
                    pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].sliceheight;
                InputFrame.fd[0] = pProcessRequestInfo->phInputBuffer[1 + i - burstCurIdx].fd[0];
                InputFrame.planeSize[0] = InputFrame.stride * InputFrame.scanline;
                InputFrame.planeSize[1] = (InputFrame.stride * InputFrame.scanline) / 2;
                MLOGI(Mia2LogGroupPlugin, "PureView dumpToFile %p", &InputFrame);
                PluginUtils::dumpToFile(inbuf, &InputFrame);
            }
            std::atomic_thread_fence(std::memory_order_seq_cst);
            long vaild = pvInput[i].format + (long)pvInput[i].data[0] + (long)pvInput[i].data[1] +
                         pvInput[i].h + pvInput[i].w + pvInput[i].pitch[0] + pvInput[i].pitch[1] +
                         pvInput[i].memInfo[0].fd + pvInput[i].memInfo[0].type +
                         pvInput[i].memInfo[0].size;
            if (0 != vaild)
                pvConfig.frameReadyFlags[i] = 1;
        }

        MLOGI(Mia2LogGroupPlugin,
              "%s:%d input width: %d, height: %d, stride: %d, scanline: %d, format: %d, outputi "
              " stride: %d "
              "scanline: %d, format: %d, burstCurIdx: %d, frameNum: %d, mMultiNum: %d, input Bufer"
              " num: %d, plane: %p %p",
              __func__, __LINE__, pvInput[0].w, pvInput[0].h, pvInput[0].pitch[0],
              pProcessRequestInfo->phInputBuffer[0].sliceheight, pvInput[0].format,
              pvOutput.pitch[0], (pvOutput.memInfo[0].size / pvOutput.pitch[0]) / 2,
              pvOutput.format, burstCurIdx, pProcessRequestInfo->frameNum, mMultiNum, inputNum,
              pvInput[burstCurIdx - 1].data[0], pvInput[burstCurIdx - 1].data[1]);

        if (1 == burstCurIdx) {
            unique_lock<std::mutex> threadLockerStatus(mMutexLoop);
            mStatus = NEEDPROC;
            mCondLoop.notify_all();
        }
        mCondPerFrameReturn.wait(threadLocker);
    } else {
        pvConfig.cameraAbnormalQuit = 1;
        resultInfo = PROCFAILED;
    }

    return resultInfo;
}
int PureViewPlugin::ProcessBuffer(PV_IMG *input, int input_num, PV_IMG *output, int64_t timeStamp,
                                  void **metaData, void **physMeta)
{
    int32_t iIsDumpData = sIsDumpData;
    int result = PROCSUCCESS;
    if (metaData == NULL || metaData[0] == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s error metaData is null", __func__);
        pvConfig.cameraAbnormalQuit = 1;
        return PROCFAILED;
    }
    // get iso
    std::vector<int32_t> iso(input_num);
    std::vector<PV_FACE_INFO> faceList(input_num);
    void *pData = NULL;
    ChiRect cropRegion = {};
    if (physMeta[0] == NULL) {
        physMeta[0] = metaData[0];
    }
    // lijinglin add to pvconfig!!!!!!!!!!!!!
    iso[0] = GetISO(physMeta[0]);
    faceList[0] = GetFaceList(physMeta[0], input[0]);
    cropRegion = GetCropRegion((camera_metadata_t *)physMeta[0], input[0].w, input[0].h);
    pvConfig.iso = iso[0];
    pvConfig.faceList = faceList[0];
    pvConfig.srRoiRect.x = cropRegion.left;
    pvConfig.srRoiRect.y = cropRegion.top;
    pvConfig.srRoiRect.w = cropRegion.width;
    pvConfig.srRoiRect.h = cropRegion.height;

    MLOGI(Mia2LogGroupPlugin, "pureview input size: %d, output size: %d, pureview iso: %d",
          input[0].memInfo[0].size, output->memInfo[0].size, iso[0]);
    // process pureview
    int ret = PV_OK;
    ret = pureViewProc(input, &pvConfig, output);
    if (ret != PV_OK || iIsDumpData) {
        struct MiImageBuffer InputFrame, OutputFrame;
        InputFrame.format = mInputFrameFormat;
        InputFrame.width = input[0].w;
        InputFrame.height = input[0].h;
        InputFrame.plane[0] = static_cast<uint8_t *>(input[0].data[0]);
        InputFrame.plane[1] = static_cast<uint8_t *>(input[0].data[1]);
        InputFrame.stride = input[0].pitch[0];
        InputFrame.scanline = input[0].memInfo[0].size / input[0].pitch[0] * 2 / 3;
        InputFrame.fd[0] = input[0].memInfo[0].fd;
        InputFrame.planeSize[0] = InputFrame.stride * InputFrame.scanline;
        InputFrame.planeSize[1] = (InputFrame.stride * InputFrame.scanline) >> 1;

        OutputFrame.format = mOutputFrameFormat;
        OutputFrame.width = output->w;
        OutputFrame.height = output->h;
        OutputFrame.plane[0] = static_cast<uint8_t *>(output->data[0]);
        OutputFrame.plane[1] = static_cast<uint8_t *>(output->data[1]);
        OutputFrame.stride = output->pitch[0];
        OutputFrame.scanline = (output->memInfo[0].size / output->pitch[0]) >> 1;
        OutputFrame.fd[0] = output->memInfo[0].fd;
        OutputFrame.planeSize[0] = OutputFrame.stride * OutputFrame.scanline;
        OutputFrame.planeSize[1] = OutputFrame.stride * OutputFrame.scanline;
        if (ret != PV_OK) {
            MLOGE(Mia2LogGroupPlugin, "PureViewProc FAIL!!");
            result = PROCFAILED;
        }
        if (iIsDumpData) {
            char outbuf[128];
            snprintf(outbuf, sizeof(outbuf), "pureview_output_%dx%d_iso(%d)", OutputFrame.width,
                     OutputFrame.height, iso[0]);
            PluginUtils::dumpToFile(outbuf, &OutputFrame);
            MLOGI(Mia2LogGroupPlugin, "PureView dumpToFile %p", &OutputFrame);
        }
    }
    MLOGD(Mia2LogGroupPlugin, "PureViewProc ret:%d", ret);
    return result;
}

PV_FACE_INFO PureViewPlugin::GetFaceList(void *metaData, PV_IMG input)
{
    PV_FACE_INFO FaceList;
    void *data = NULL;

    // get orientation
    VendorMetadataParser::getTagValue((camera_metadata_t *)metaData, ANDROID_JPEG_ORIENTATION,
                                      &data);
    if (NULL != data) {
        int32_t oriention = 0;
        oriention = *static_cast<int32_t *>(data);
        // lijinglin angle need to change~~
        FaceList.faceAngle[0] = static_cast<char>((oriention % 360) / 90);
        MLOGD(Mia2LogGroupPlugin, "%s:%d    pureview got oriention:%d, faceAngle: %d", __func__,
              __LINE__, oriention, FaceList.faceAngle[0]);
    } else {
        MLOGW(Mia2LogGroupPlugin, "%s:%d  pureview get faceAngle failed........", __func__,
              __LINE__);
    }

    // get facenums
    data = NULL;
    uint32_t faceNum = 0;
    camera_metadata_entry_t entry = {0};
    uint32_t numElemsRect = sizeof(ChiRect) / sizeof(uint32_t);
    if (0 != find_camera_metadata_entry((camera_metadata_t *)metaData,
                                        ANDROID_STATISTICS_FACE_RECTANGLES, &entry)) {
        FaceList.faceNum = 0;
        MLOGE(Mia2LogGroupPlugin, "Pureview, get ANDROID_STATISTICS_FACE_RECTANGLES failed");
        return FaceList;
    }
    FaceList.faceNum = entry.count / numElemsRect;

    // get sensor size for mivifwk, rather than active size for hal
    CHIDIMENSION activeSize = {0};
    uint32_t mSensorWidth = 0;
    uint32_t mSensorHeight = 0;
    data = NULL;
    const char *SensorSize = "xiaomi.sensorSize.size";
    VendorMetadataParser::getVTagValue((camera_metadata_t *)metaData, SensorSize, &data);
    if (NULL != data) {
        activeSize = *(CHIDIMENSION *)(data);
        MLOGI(Mia2LogGroupPlugin, "Pureview, get sensor activeSize %d %d", activeSize.width,
              activeSize.height);
        // sensor size is 1/2 active size
        mSensorWidth = activeSize.width / 2;
        mSensorHeight = activeSize.height / 2;
    } else {
        MLOGE(Mia2LogGroupPlugin, "Pureview, get sensor activeSize failed");
        mSensorWidth = input.w;
        mSensorHeight = input.h;
    }
    MLOGI(Mia2LogGroupPlugin, "Pureview, sensor: %d, %d; snapshot:%d, %d", mSensorWidth,
          mSensorHeight, input.w, input.h);

    // get facerect
    data = NULL;
    VendorMetadataParser::getTagValue((camera_metadata_t *)metaData,
                                      ANDROID_STATISTICS_FACE_RECTANGLES, &data);
    if (NULL != data) {
        void *tmpData = reinterpret_cast<void *>(data);
        float zoomRatio = (float)input.w / mSensorWidth;
        float yOffSize = ((float)zoomRatio * mSensorHeight - input.h) / 2.0;
        MLOGI(Mia2LogGroupPlugin, "Pureview zoomRatio:%f, yOffSize:%f", zoomRatio, yOffSize);

        uint32_t dataIndex = 0;
        for (int index = 0; index < faceNum; index++) {
            int32_t xMin = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t yMin = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t xMax = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            int32_t yMax = *((int32_t *)tmpData + dataIndex);
            dataIndex++;
            MLOGI(Mia2LogGroupPlugin, "Pureview, origin face [xMin,yMin,xMax,yMax][%d,%d,%d,%d]",
                  xMin, yMin, xMax, yMax);
            xMin = xMin * zoomRatio;
            xMax = xMax * zoomRatio;
            yMin = yMin * zoomRatio - yOffSize;
            yMax = yMax * zoomRatio - yOffSize;
            int32_t width = xMax - xMin;
            int32_t height = yMax - yMin;
            MLOGI(Mia2LogGroupPlugin,
                  "Pureview, now face [xMin,yMin,xMax,yMax][%d,%d,%d,%d], "
                  "[xMin,yMin,width,height][%d,%d,%d,%d]",
                  xMin, yMin, xMax, yMax, xMin, yMin, width, height);
            FaceList.faceRect->x = xMin;
            FaceList.faceRect->y = yMin;
            FaceList.faceRect->w = width;
            FaceList.faceRect->h = height;
        }
    } else {
        MLOGI(Mia2LogGroupPlugin, "Pureview faces are not found");
    }

    return FaceList;
}

int PureViewPlugin::GetISO(void *metaData)
{
    int32_t sensitivity = 0;
    float ispGain = 1.0;
    void *pData = NULL;

    VendorMetadataParser::getTagValue((camera_metadata_t *)metaData, ANDROID_SENSOR_SENSITIVITY,
                                      &pData);
    if (NULL != pData) {
        sensitivity = *static_cast<int32_t *>(pData);
        MLOGD(Mia2LogGroupPlugin, "PureView get sensitivity %d", sensitivity);
    }
    pData = NULL;
    VendorMetadataParser::getVTagValue((camera_metadata_t *)metaData, "com.qti.sensorbps.gain",
                                       &pData);
    if (NULL != pData) {
        ispGain = *static_cast<float *>(pData);
        MLOGD(Mia2LogGroupPlugin, "PureView get ispGain %f", ispGain);
    }

    // Workaround: Our new project's iso100gain is changed to 2.
    // This means base iso is 50 for 1x gain.
    // But legacy SR algorithm uses iso 100 corresponding to 1x gain.
    // So we return iso*2 here to make it compatible with legacy SR algorithm.
    int iso = (int)(ispGain * 100) * sensitivity / 100;
    // int iso = (int)(ispGain * 100) * sensitivity / 100;

    return iso;
}

ChiRect PureViewPlugin::GetCropRegion(camera_metadata_t *metaData, int inputWidth, int inputHeight)
{
    ChiRect cropRegion = {};
    ChiRect outputRect;
    void *pData = NULL;
    int cameraId = 0;
    int ret;

    VendorMetadataParser::getVTagValue(metaData, "xiaomi.snapshot.cropRegion", &pData);
    if (NULL != pData) {
        cropRegion = *static_cast<ChiRect *>(pData);
    } else {
        MLOGE(Mia2LogGroupPlugin,
              "Pureview, get android crop region failed,use FULL-FOV instead !!");
        cropRegion = {0, 0, static_cast<uint32_t>(inputWidth), static_cast<uint32_t>(inputHeight)};
    }

    // get RefCropSize
    int refWidth = 0, refHeight = 0;
    ret = VendorMetadataParser::getVTagValue(metaData, "xiaomi.snapshot.fwkCameraId", &pData);
    if (ret == 0) {
        cameraId = *static_cast<int32_t *>(pData);

        camera_metadata_t *staticMetadata =
            StaticMetadataWraper::getInstance()->getStaticMetadata(cameraId);
        ret = VendorMetadataParser::getTagValue(staticMetadata,
                                                ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &pData);
        if (ret == 0) {
            ChiRect &activeArray = *static_cast<ChiRect *>(pData);
            refWidth = activeArray.width;
            refHeight = activeArray.height;
        }
    } else {
        MLOGW(Mia2LogGroupPlugin, "Pureview failed to get cameraId (MetadataOwner)");
    }

    MLOGW(Mia2LogGroupPlugin, "Pureview my crop region(%d, %d, %d, %d, %d, %d)", cropRegion.left,
          cropRegion.top, cropRegion.width, cropRegion.height, refWidth, refHeight);
    if (refWidth == 0) {
        // fallback to assume it's center-crop, and get ref size using cropRegion.
        refWidth = (cropRegion.left * 2) + cropRegion.width;
        refHeight = (cropRegion.top * 2) + cropRegion.height;
    }
    // clang-format off
    /************************************************************************************************
    +-----------------------------+
    |                             |
    |   +-------------------+     |    +-----------------------------+
    |   |                   |     |    |                             |
    |   |                   |     |    |   +-------------------+     |    +------------------+
    |   |                   |     |    |   |                   |     |    |  +-----------+   |
    |   |                   |     +--->+   |                   |     +--->+  |           |   |
    |   |                   |     |    |   |                   |     |    |  +-----------+   |
    |   |                   |     |    |   +-------------------+     |    +------------------+
    |   +-------------------+     |    |                             |
    |                             |    +-----------------------------+
    +-----------------------------+     16:9 for example:                 preZoom applied:
    sensorDomain(4:3)                   tmpCrop(after align aspectRatio)  (input buf domain)
    refWidth=6016,refHeight=4512        refWidth=6016,refHeight=3384      width=1600,height=900
    cropRegion=[1504,1128,3008,2256]    tmpCrop=[1504,846,3008,1692]      crop=[400,225,800,450]
    *************************************************************************************************/
    // clang-format on

    // align cropRegion's aspect ratio with input buffer
    float tmpLeft = cropRegion.left, tmpTop = cropRegion.top;
    float tmpWidth = cropRegion.width, tmpHeight = cropRegion.height;
    float xRatio = (float)cropRegion.width / inputWidth;
    float yRatio = (float)cropRegion.height / inputHeight;
    constexpr float tolerance = 0.01f;

    if (yRatio > xRatio + tolerance) {
        tmpHeight = (float)inputHeight * xRatio;
        float tmpRefHeight = (float)inputHeight * refWidth / inputWidth;
        float delta = (refHeight - tmpRefHeight) / 2.0;
        tmpTop = cropRegion.top + (cropRegion.height - tmpHeight) / 2.0 - delta;
    } else if (xRatio > yRatio + tolerance) {
        tmpWidth = (float)inputWidth * yRatio;
        float tmpRefWidth = (float)inputWidth * refHeight / inputHeight;
        float delta = (refWidth - tmpRefWidth) / 2.0;
        tmpLeft = cropRegion.left + (cropRegion.width - tmpWidth) / 2.0 - delta;
    }

    // adjust crop according to preZoom into buffer domain
    unsigned int rectLeft, rectTop, rectWidth, rectHeight;
    float preZoomRatio = (float)inputWidth / refWidth;
    rectWidth = tmpWidth * preZoomRatio;
    rectLeft = tmpLeft * preZoomRatio;
    rectHeight = tmpHeight * preZoomRatio;
    rectTop = tmpTop * preZoomRatio;

    // now we validate the rect, and ajust it if out-of-bondary found.
    if (rectLeft + rectWidth > inputWidth) {
        if (rectWidth <= inputWidth) {
            rectLeft = inputWidth - rectWidth;
        } else {
            rectLeft = 0;
            rectWidth = inputWidth;
        }
        MLOGW(Mia2LogGroupPlugin, "Pureview crop left or width is wrong, ajusted it manually!!");
    }
    if (rectTop + rectHeight > inputHeight) {
        if (rectHeight <= inputHeight) {
            rectTop = inputHeight - rectHeight;
        } else {
            rectTop = 0;
            rectHeight = inputHeight;
        }
        MLOGW(Mia2LogGroupPlugin, "Pureview crop top or height is wrong, ajusted it manually!!");
    }
    cropRegion = {rectLeft, rectTop, rectWidth, rectHeight};

    MLOGI(Mia2LogGroupPlugin,
          "Pureview, cam=(%d),crop rect[%d,%d,%d,%d],"
          "in=%dx%d,ref=%dx%d",
          cameraId, cropRegion.left, cropRegion.top, cropRegion.width, cropRegion.height,
          inputWidth, inputHeight, refWidth, refHeight);

    return cropRegion;
}

ProcessRetStatus PureViewPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    return resultInfo;
}
int PureViewPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);
    {
        pureViewFlush(&pvConfig);
    }
    return 0;
}

void PureViewPlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);
    {
        lock_guard<std::mutex> threadLocker(mMutexLoop);
        mStatus = END;
        mCondLoop.notify_all();
    }

    mProcThread.join();
    pureViewPreDeInit(&pvConfig);
    if (NULL != pvInput) {
        delete pvInput;
        pvInput = NULL;
    }
}
