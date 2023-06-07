#include "GpuPlugin.h"

#include <system/camera_metadata.h>

using namespace mialgo2;
#define VIDEO_RECORD_CONTROL_START 1
#define VIDEO_RECORD_CONTROL_STOP  2

#undef LOG_TAG
#define LOG_TAG "GpuPlugin"

static const int32_t sGpuDumpablity =
    property_get_int32("persist.vendor.camera.mialgo.pulgin.gpu.dumpablity", 0);

GpuPlugin::~GpuPlugin()
{
    MLOGI(Mia2LogGroupPlugin, "destruct");
}

int GpuPlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    (void)createInfo;
    MLOGI(Mia2LogGroupPlugin, "Initialize");
    mOpenCL = NULL;
    mNodeCaps = ChiNodeCapsGPUFlip;
    return 0;
}

bool GpuPlugin::isEnabled(MiaParams settings)
{
    void *data = NULL;
    uint32_t flipEnable = 0;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.flip.enabled", &data);
    if (NULL != data) {
        flipEnable = *static_cast<uint32_t *>(data);
    }
    if (1 == flipEnable) {
        mCurrentFlip = FlipLeftRight;
        return true;
    } else {
        mCurrentFlip = FlipNone;
        return false;
    }
}

ProcessRetStatus GpuPlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    if (NULL == processRequestInfo || processRequestInfo->inputBuffersMap.empty()) {
        MLOGE(Mia2LogGroupPlugin, "Invaild Parameter");
        resultInfo = PROCFAILED;
        return resultInfo;
    }
    int32_t gpuDumpAbility = sGpuDumpablity;

    std::lock_guard<std::mutex> gpuMutex(mGpuNodeMutex);

    auto &inputBuffers = processRequestInfo->inputBuffersMap.begin()->second;
    auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;
    camera_metadata_t *pMeta = inputBuffers[0].pMetadata;

    // if (mOpenCL == NULL) {
    //    mOpenCL = new GPUOpenCL();
    //}

    switch (mNodeCaps) {
    case ChiNodeCapsGPUFlip:
        updateOrientation(pMeta);
        // updateFlip(pMeta);
        MLOGI(Mia2LogGroupPlugin,
              "input width: %d, height: %d, stride: %d,scanline: %d, fd: %d, output stride: %d, "
              "scanline: %d, fd: %d",
              inputBuffers[0].format.width, inputBuffers[0].format.height,
              inputBuffers[0].planeStride, inputBuffers[0].sliceheight, inputBuffers[0].fd[0],
              outputBuffers[0].planeStride, outputBuffers[0].sliceheight, outputBuffers[0].fd[1]);
        if (gpuDumpAbility != 0) {
            MiImageBuffer inputFrame{0};
            inputFrame.format = inputBuffers[0].format.format;
            inputFrame.width = inputBuffers[0].format.width;
            inputFrame.height = inputBuffers[0].format.height;
            inputFrame.plane[0] = inputBuffers[0].pAddr[0];
            inputFrame.plane[1] = inputBuffers[0].pAddr[1];
            inputFrame.stride = inputBuffers[0].planeStride;
            inputFrame.scanline = inputBuffers[0].sliceheight;
            char inputbuf[128];
            snprintf(inputbuf, sizeof(inputbuf), "gpu_input_%d_%d_%d", inputFrame.width,
                     inputFrame.height, processRequestInfo->frameNum);
            PluginUtils::dumpToFile(inputbuf, &inputFrame);
        }
        if (FlipLeftRight == mCurrentFlip) {
            if (true) {
                MLOGD(Mia2LogGroupPlugin, "Do Software Flip");
                doSwFlip(inputBuffers[0].pAddr[0], inputBuffers[0].pAddr[1],
                         outputBuffers[0].pAddr[0], outputBuffers[0].pAddr[1],
                         inputBuffers[0].format.width, inputBuffers[0].format.height,
                         inputBuffers[0].planeStride, inputBuffers[0].sliceheight,
                         mCurrentRotation);
            } else {
                MLOGD(Mia2LogGroupPlugin, "Do Hardware Flip");
                flipImage(outputBuffers, inputBuffers, mCurrentFlip, mCurrentRotation);
            }
        } else {
            copyImage(outputBuffers, inputBuffers);
        }

        if (gpuDumpAbility != 0) {
            MiImageBuffer outputFrame{0};
            outputFrame.format = outputBuffers[0].format.format;
            outputFrame.width = outputBuffers[0].format.width;
            outputFrame.height = outputBuffers[0].format.height;
            outputFrame.plane[0] = outputBuffers[0].pAddr[0];
            outputFrame.plane[1] = outputBuffers[0].pAddr[1];
            outputFrame.stride = outputBuffers[0].planeStride;
            outputFrame.scanline = outputBuffers[0].sliceheight;
            char outputbuf[128];
            snprintf(outputbuf, sizeof(outputbuf), "gpu_output_%d_%d_%d", outputFrame.width,
                     outputFrame.height, processRequestInfo->frameNum);
            PluginUtils::dumpToFile(outputbuf, &outputFrame);
        }
        break;

    default:
        MLOGE(Mia2LogGroupPlugin, "ERR! Unsupport operation: 0x%d", mNodeCaps);
        break;
    }
    return resultInfo;
}

ProcessRetStatus GpuPlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    (void)processRequestInfo;
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int GpuPlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    if (NULL == flushRequestInfo) {
        MLOGE(Mia2LogGroupPlugin, "Invalid argument: flushRequestInfo = %p", flushRequestInfo);
    }
    return 0;
}

void GpuPlugin::destroy()
{
    // if (mOpenCL != NULL) {
    //    delete mOpenCL;
    //    mOpenCL = NULL;
    //}
    MLOGI(Mia2LogGroupPlugin, "Destroy");
}

void GpuPlugin::flipImage(std::vector<ImageParams> &hOutput, std::vector<ImageParams> &hInput,
                          FlipDirection targetFlip, RotationAngle currentOrientation)
{
    FlipDirection direction = targetFlip;
    int32_t result = MIA_RETURN_UNKNOWN_ERROR;
    if ((Rotate90Degrees == currentOrientation) || (Rotate270Degrees == currentOrientation)) {
        if (FlipLeftRight == targetFlip) {
            direction = FlipTopBottom;
        } else {
            direction = FlipLeftRight;
        }
    }

    if (mOpenCL != NULL) {
        result = mOpenCL->FlipImage(hOutput, hInput, direction);
    }
    if (result != MIA_RETURN_OK) {
        copyImage(hOutput, hInput);
    }
}

void GpuPlugin::copyImage(std::vector<ImageParams> &hOutput, std::vector<ImageParams> &hInput)
{
    MiImageBuffer inputFrame, outputFrame;

    inputFrame.format = hInput.at(0).format.format;
    inputFrame.width = hInput.at(0).format.width;
    inputFrame.height = hInput.at(0).format.height;
    inputFrame.plane[0] = hInput.at(0).pAddr[0];
    inputFrame.plane[1] = hInput.at(0).pAddr[1];
    inputFrame.stride = hInput.at(0).planeStride;
    inputFrame.scanline = hInput.at(0).sliceheight;

    outputFrame.format = hOutput.at(0).format.format;
    outputFrame.width = hOutput.at(0).format.width;
    outputFrame.height = hOutput.at(0).format.height;
    outputFrame.plane[0] = hOutput.at(0).pAddr[0];
    outputFrame.plane[1] = hOutput.at(0).pAddr[1];
    outputFrame.stride = hOutput.at(0).planeStride;
    outputFrame.scanline = hOutput.at(0).sliceheight;

    PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
}

void GpuPlugin::updateOrientation(camera_metadata_t *metaData)
{
    void *data = NULL;
    int32_t orientation = 0;
    int32_t captureIntent = 0;
    int8_t cameraId = 0;

    auto getOrientation = [&data, &cameraId, &orientation, metaData]() mutable {
        data = NULL;
        VendorMetadataParser::getTagValue(metaData, ANDROID_LENS_FACING, &data);
        if (NULL != data) {
            cameraId = *static_cast<int8_t *>(data);
        }
        data = NULL;
        const char *deviceOrientation = "xiaomi.device.orientation";
        VendorMetadataParser::getVTagValue(metaData, deviceOrientation, &data);
        if (NULL != data) {
            orientation = *static_cast<int32_t *>(data);
            orientation = (orientation + 90) % 360;
            if (1 == cameraId) {
                orientation = 360 - orientation;
            }
        }
    };

    VendorMetadataParser::getTagValue(metaData, ANDROID_CONTROL_CAPTURE_INTENT, &data);
    if (NULL != data) {
        captureIntent = *static_cast<int32_t *>(data);
    }

    if (ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD == captureIntent) {
        getOrientation();
    } else {
        data = NULL;
        VendorMetadataParser::getTagValue(metaData, ANDROID_JPEG_ORIENTATION, &data);
        if (NULL != data) {
            orientation = *static_cast<int32_t *>(data);
        }
    }

    switch (orientation) {
    case 0:
        mCurrentRotation = Rotate0Degrees;
        break;
    case 90:
        mCurrentRotation = Rotate90Degrees;
        break;
    case 180:
        mCurrentRotation = Rotate180Degrees;
        break;
    case 270:
        mCurrentRotation = Rotate270Degrees;
        break;
    default:
        break;
    }
    MLOGI(Mia2LogGroupPlugin, "mCurrentRotation = %d", mCurrentRotation);
}

void GpuPlugin::updateFlip(camera_metadata_t *metaData)
{
    void *data = NULL;
    unsigned int flipEnable = 0;
    const char *flipEnabletagName = "xiaomi.flip.enabled";
    VendorMetadataParser::getVTagValue(metaData, flipEnabletagName, &data);

    if (NULL != data) {
        flipEnable = *static_cast<unsigned int *>(data);
    }

    MLOGI(Mia2LogGroupPlugin, "flipEnable = %d", flipEnable);

    if (1 == flipEnable) {
        mCurrentFlip = FlipLeftRight;
    } else {
        mCurrentFlip = FlipNone;
    }
}

void GpuPlugin::doSwFlip(uint8_t *inputY, uint8_t *inputUV, uint8_t *outputY, uint8_t *outputUV,
                         int width, int height, int stride, int sliceHeight, RotationAngle orient)
{
    if ((Rotate90Degrees == orient) || (Rotate270Degrees == orient)) {
        int idx = 0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < stride; j += 2) {
                outputY[idx++] = *(inputY + stride * (height - 1 - i) + j);
                outputY[idx++] = *(inputY + stride * (height - 1 - i) + j + 1);
            }
        }

        // UV
        idx = 0;
        for (int i = 0; i < height / 2; i++) {
            for (int j = 0; j < stride; j += 2) {
                outputUV[idx++] = *(inputUV + stride * (height / 2 - 1 - i) + j);
                outputUV[idx++] = *(inputUV + stride * (height / 2 - 1 - i) + j + 1);
            }
        }
    } else {
        int idx = 0;
        for (int i = 0; i < height * stride; i += stride) {
            for (int j = 0; j < width; j++) {
                outputY[idx++] = *(inputY + i + width - 1 - j);
            }
            idx += (stride - width);
        }

        // UV
        idx = 0;
        for (int i = 0; i < height * stride / 2; i += stride) {
            for (int j = 0; j < width; j += 2) {
                outputUV[idx++] = *(inputUV + i + width - 2 - j);
                outputUV[idx++] = *(inputUV + i + width - 1 - j);
            }
            idx += (stride - width);
        }
    }
}
