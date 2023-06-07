#include "MiaLdcPlugin.h"

#include "device/device.h"
#include "nodeMetaDataUp.h"

#undef LOG_TAG
#define LOG_TAG "LDC-PLUGIN"

#ifndef NODE_LDC_CONFIG_TYPE
#define NODE_LDC_CONFIG_TYPE NODE_LDC_TYPE_CAPTURE_DEF
#endif

//#define ENABLE_UW_W_FUSION_FUNC
//#undef ENABLE_UW_W_FUSION_FUNC
static long getCurrentTimeStamp()
{
    long ms = 0;
    struct timeval t;
    gettimeofday(&t, NULL);
    ms = t.tv_sec * 1000 + t.tv_usec / 1000;
    return ms;
}

namespace mialgo2 {

LDCPlugin::~LDCPlugin() {}

int LDCPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    DEV_LOGI("LDCPlugin::Initialize() enter");
    m_nodeInterface = nodeInterface;
    Device->Init();
    m_debug = property_get_int32("persist.vendor.camera.LDC.debug", 0);
    m_local_operation_mode = pCreateInfo->operationMode;
    m_mode = NODE_LDC_MODE_CAPTURE;
    m_type = NODE_LDC_CONFIG_TYPE;
    m_pLdcCapture = NULL;
    m_pNodeMetaData = NULL;
    m_processedFrame = 0;
    mInitialized = false;
    mLdcLevel = 0;
    m_CameraId = pCreateInfo->frameworkCameraId;

    if (m_debug == 1) {
        m_detectorCaptureId = Device->detector->Create();
    }

    if (m_pLdcCapture == NULL) {
        m_pLdcCapture = new LdcCapture();
    }
    if (m_pNodeMetaData == NULL) {
        m_pNodeMetaData = new NodeMetaData();
        m_pNodeMetaData->Init(NODE_METADATA_MODE_CAPTURE, m_debug);
    }

    if ((((m_debug & NODE_LDC_DEBUG_BYPASS_CAPTURE) != 0) && (m_mode == NODE_LDC_MODE_CAPTURE))) {
        m_mode = NODE_LDC_MODE_BYPASS;
    }

#ifdef ENABLE_UW_W_FUSION_FUNC
    mNodeInstanceCapture = NULL;

    if ((mNodeInstanceCapture == NULL) && (m_CameraId == CAM_COMBMODE_REAR_SAT_WU)) {
        mNodeInstanceCapture = new FusionCapture();
        if (mNodeInstanceCapture) {
            mNodeInstanceCapture->getVersion();

            int ret = mNodeInstanceCapture->init();
            if (ret != PROCSUCCESS) {
                MLOGE(Mia2LogGroupPlugin, "%s:%d MiaiiePlugin ERROR init false \n", __func__,
                      __LINE__);
                return RET_ERR;
            }
        }
    }
#endif

    return RET_OK;
}

bool LDCPlugin::isEnabled(MiaParams settings)
{
    bool ldcEnable = property_get_bool("vendor.debug.cap.c.ldc.onoff", 1);
    m_pNodeMetaData->GetLdcLevel(settings.metadata, 0, &mLdcLevel);
    DEV_LOGI("LDCPlugin::IsEnabled() mLdcLevel = %d m_CameraId = %d", mLdcLevel, m_CameraId);

    if ((ldcEnable == true) && (mLdcLevel > 0) &&
        (m_CameraId == CAM_COMBMODE_REAR_ULTRA || m_CameraId == CAM_COMBMODE_REAR_SAT_WU)) {
        return true;
        // DEV_LOGI("LDCPlugin::IsEnabled() true");
    } else {
        // DEV_LOGI("LDCPlugin::IsEnabled() false");
        return false;
    }
}

ProcessRetStatus LDCPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    S32 ret = RET_OK;
    // ProcessResultInfo resultInfo;
    // resultInfo.result = PROCFAILED;
    // resultInfo.isBypassed = false;
    auto &phInputBuffer = pProcessRequestInfo->inputBuffersMap.at(0);
    auto &phOutputBuffer = pProcessRequestInfo->outputBuffersMap.at(0);

    void *pMdata = phInputBuffer[0].pMetadata;
    S32 iIsDumpData = property_get_int32("persist.vendor.algoengine.ldc.dump", 0);
    S32 iIsFusionDumpData = property_get_int32("persist.vendor.camera.algoengine.fusion.dump", 0);
    bool bypassAlgo = property_get_int32("persist.vendor.camera.algoengine.fusion.bypass", 0);

    DEV_IMAGE_BUF inputFrame, outputFrame;
    struct ImageParams requestInputBuffer = phInputBuffer[0];
    struct ImageParams requestOutputBuffer = phOutputBuffer[0];
    ret |= Device->image->MatchToImage((void *)&requestInputBuffer, &inputFrame);
    ret |= Device->image->MatchToImage((void *)&requestOutputBuffer, &outputFrame);
    // DEV_IF_LOGE_RETURN_RET((ret != RET_OK), resultInfo, "input err");

    m_snapshotWidth = inputFrame.width;
    m_snapshotHeight = inputFrame.height;
    m_processedFrame = pProcessRequestInfo->frameNum;

    DEV_LOGI(
        "LDC RUN "
        "mode=%d,type=%d,frameNum=%lu,in:%dx%d(%dx%d),fmt=%d,out:%dx%d(%dx%d),fmt=%d,level=%d ",
        m_mode, m_type, m_processedFrame, inputFrame.width, inputFrame.height, inputFrame.stride[0],
        inputFrame.sliceHeight[0], inputFrame.format, outputFrame.width, outputFrame.height,
        outputFrame.stride[0], outputFrame.sliceHeight[0], outputFrame.format, mLdcLevel);

    // // //if current sensor is uw, need sr to do fov crop
    // void* sensor_meta = NULL;
    // MetadataUtils::getInstance()->getVTagValue(pMdata,
    // "com.xiaomi.multicamerainfo.MultiCameraIdRole", &sensor_meta); if (NULL != sensor_meta) {
    //     CameraRoleType sensorID = *static_cast< CameraRoleType* >(sensor_meta);
    //     if (sensorID == CameraRoleTypeUltraWide) {
    //         DEV_LOGI("current sensor is ultra wide, need to set SR enable");
    //         bool superResolutionEnable = true;
    //         MetadataUtils::getInstance()->setVTagValue(pMdata, "xiaomi.superResolution.enabled",
    //         &superResolutionEnable, 1);
    //     }
    // }

    if ((m_mode == NODE_LDC_MODE_BYPASS) ||
        !((mLdcLevel & NODE_LDC_LEVEL_ENABLE) == NODE_LDC_LEVEL_ENABLE)) {
        DEV_IF_LOGE_GOTO((RET_ERR != RET_OK), exit, "LDC RUN BYPASS MODE");
    } else if (m_mode == NODE_LDC_MODE_CAPTURE) {
        if (m_debug == 1) {
            Device->detector->Begin(m_detectorCaptureId);
        }
        U32 vendorId = 0;
        float zoomRatio = 0.0;
        MIRECT cropRegion;
        MIRECT sensorSize;
        MIRECT sensorActiveSize;
        NODE_METADATA_FACE_INFO faceInfo;
        memset(&faceInfo, 0, sizeof(faceInfo));

        sensorSize.width = m_snapshotWidth;
        sensorSize.height = m_snapshotHeight;

        m_pNodeMetaData->GetSensorSize(pMdata, &sensorSize);
        DEV_LOGI("sensorSize.width = %d sensorSize.height = %d", sensorSize.width,
                 sensorSize.height);
        m_pNodeMetaData->GetZoomRatio(pMdata, &zoomRatio);
        m_pNodeMetaData->GetCropRegion(pMdata, m_processedFrame, (MIRECT *)&cropRegion,
                                       m_local_operation_mode);

        //  Only Manual Mode need to be transformed,and zoomRatio always >= 1.0
        DEV_LOGI("LDC get cropRegion before: [%d %d %d %d]", cropRegion.left, cropRegion.top,
                 cropRegion.width, cropRegion.height);
        DEV_LOGI("zoomRatio = %f", zoomRatio);
        if (zoomRatio >= 1.0f) {
            int xCenter = sensorSize.width / 2;
            int yCenter = sensorSize.height / 2;
            int xDelta = (int)(sensorSize.width / (2 * zoomRatio) + 0.5);
            int yDelta = (int)(sensorSize.height / (2 * zoomRatio) + 0.5);
            DEV_LOGI("LDC Center: [%d %d %d %d]", xCenter, yCenter, xDelta, yDelta);
            cropRegion.left = xCenter - xDelta;
            cropRegion.top = yCenter - yDelta;
            cropRegion.width = sensorSize.width - cropRegion.left * 2;
            cropRegion.height = sensorSize.height - cropRegion.top * 2;
        }

        faceInfo.imgHeight = inputFrame.height;
        faceInfo.imgWidth = inputFrame.width;

        faceInfo.sensorActiveArraySize.height = sensorSize.height;
        faceInfo.sensorActiveArraySize.left = sensorSize.left;
        faceInfo.sensorActiveArraySize.top = sensorSize.top;
        faceInfo.sensorActiveArraySize.width = sensorSize.width;

        DEV_LOGI("operationmode = %x", m_local_operation_mode);
        DEV_LOGI("LDC get cropRegion after: [%d %d %d %d]", cropRegion.left, cropRegion.top,
                 cropRegion.width, cropRegion.height);

        m_pNodeMetaData->GetFaceInfo(pMdata, m_processedFrame, NODE_METADATA_MODE_CAPTURE,
                                     &faceInfo, cropRegion, sensorSize);

        if (!mInitialized) {
            U32 captureMode = LDC_CAPTURE_MODE_ALTEK_DEF;
            if (m_type == NODE_LDC_TYPE_CAPTURE_ALTEK_AI) {
                captureMode = LDC_CAPTURE_MODE_ALTEK_AI;
            }
            if ((mLdcLevel & NODE_LDC_LEVEL_CONTINUOUS_EN) == NODE_LDC_LEVEL_CONTINUOUS_EN) {
                captureMode = LDC_CAPTURE_MODE_ALTEK_DEF;
            }
            ret = m_pLdcCapture->Init(captureMode, m_debug, 0, &sensorSize, 0, vendorId);
            DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "LDC　INIT err=%d", ret);
            mInitialized = true;
        }
        if (((m_debug & NODE_LDC_DEBUG_DUMP_IMAGE_CAPTURE) != 0) || iIsDumpData) {
            Device->image->DumpImage(&inputFrame, LOG_TAG, m_processedFrame, "CAPTURE.in.nv12");
        }
        ret = m_pLdcCapture->Process(&inputFrame, &outputFrame, &faceInfo, &cropRegion,
                                     m_local_operation_mode);
        DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "LDC　RUN err=%d", ret);
        if (((m_debug & NODE_LDC_DEBUG_DUMP_IMAGE_CAPTURE) != 0) || iIsDumpData) {
            Device->image->DumpImage(&outputFrame, LOG_TAG, m_processedFrame, "CAPTURE.out.nv12");
        }
        if (m_debug == 1) {
            Device->detector->End(m_detectorCaptureId, "LDC-PLUGIN_RUN");
        }
        // resultInfo.result = PROCSUCCESS;
    } else {
        DEV_LOGE("LDC MODE err=%d", m_mode);
    }

#ifdef ENABLE_UW_W_FUSION_FUNC
    if ((mNodeInstanceCapture != NULL) && pProcessRequestInfo->inputBuffersMap.size() > 1 &&
        bypassAlgo == false) {
        struct MiImageBuffer inputUltraWideFrame, inputWideFrame, outputFusionFrame;

        // prot0 ->UltraWide frame
        auto &phInputBuffer = processRequestInfo->inputBuffersMap.at(0);
        inputUltraWideFrame.format = phInputBuffer[0].format.format;
        inputUltraWideFrame.width = phInputBuffer[0].format.width;
        inputUltraWideFrame.height = phInputBuffer[0].format.height;
        inputUltraWideFrame.plane[0] = phInputBuffer[0].pAddr[0];
        inputUltraWideFrame.plane[1] = phInputBuffer[0].pAddr[1];
        inputUltraWideFrame.stride = phInputBuffer[0].planeStride;
        inputUltraWideFrame.scanline = phInputBuffer[0].sliceheight;
        inputUltraWideFrame.numberOfPlanes = 2;

        // prot1 ->Wide frame
        auto &phInputBuffer1 = processRequestInfo->inputBuffersMap.at(1);
        inputWideFrame.format = phInputBuffer1[0].format.format;
        inputWideFrame.width = phInputBuffer1[0].format.width;
        inputWideFrame.height = phInputBuffer1[0].format.height;
        inputWideFrame.plane[0] = phInputBuffer1[0].pAddr[0];
        inputWideFrame.plane[1] = phInputBuffer1[0].pAddr[1];
        inputWideFrame.stride = phInputBuffer1[0].planeStride;
        inputWideFrame.scanline = phInputBuffer1[0].sliceheight;
        inputWideFrame.numberOfPlanes = 2;

        // prot0 ->Fusion frame
        auto &phOutputBuffer = processRequestInfo->outputBuffersMap.at(0);
        outputFusionFrame.format = phInputBuffer[0].format.format;
        // outputFusionFrame.format    = phOutputBuffer[0].format.format;
        outputFusionFrame.width = phOutputBuffer[0].format.width;
        outputFusionFrame.height = phOutputBuffer[0].format.height;
        outputFusionFrame.plane[0] = phOutputBuffer[0].pAddr[0];
        outputFusionFrame.plane[1] = phOutputBuffer[0].pAddr[1];
        outputFusionFrame.stride = phOutputBuffer[0].planeStride;
        outputFusionFrame.scanline = phOutputBuffer[0].sliceheight;
        outputFusionFrame.numberOfPlanes = 2;

        // copy outputFusionFrame to fusion inputWideFrame, and outputFusionFrame is LDC process
        // result.
        PluginUtils::MiCopyBuffer(&inputUltraWideFrame, &outputFusionFrame);

        MLOGI(Mia2LogGroupPlugin,
              "[JIIGAN_FUSION] input UltraWide width: %d, height: %d, stride: %d, scanline: %d, "
              "format: %d",
              inputUltraWideFrame.width, inputUltraWideFrame.height, inputUltraWideFrame.stride,
              inputUltraWideFrame.scanline, inputUltraWideFrame.format);
        MLOGI(Mia2LogGroupPlugin,
              "[JIIGAN_FUSION] input wide width: %d, height: %d, stride: %d, scanline: %d, format: "
              "%d",
              inputWideFrame.width, inputWideFrame.height, inputWideFrame.stride,
              inputWideFrame.scanline, inputWideFrame.format);
        MLOGI(Mia2LogGroupPlugin,
              "[JIIGAN_FUSION] output bokeh width: %d, height: %d, stride: %d, scanline: %d, "
              "format: %d",
              outputFusionFrame.width, outputFusionFrame.height, outputFusionFrame.stride,
              outputFusionFrame.scanline, outputFusionFrame.format);

        if (iIsFusionDumpData & 0x01) {
            char inputUltraWidebuf[128];
            snprintf(inputUltraWidebuf, sizeof(inputUltraWidebuf),
                     "jiiganFusion_input_UltraWide_%dx%d", inputUltraWideFrame.width,
                     inputUltraWideFrame.height);
            PluginUtils::dumpToFile(inputUltraWidebuf, &inputUltraWideFrame);

            char inputwidebuf[128];
            snprintf(inputwidebuf, sizeof(inputwidebuf), "jiiganFusion_input_wide_%dx%d",
                     inputWideFrame.width, inputWideFrame.height);
            PluginUtils::dumpToFile(inputwidebuf, &inputWideFrame);
        }

        {
            int fusionret = PROCFAILED;
            long t1 = getCurrentTimeStamp();
            fusionret = mNodeInstanceCapture->process(&inputUltraWideFrame, &inputWideFrame,
                                                      &outputFusionFrame, pMdata);
            if (PROCSUCCESS == fusionret) {
                MLOGI(Mia2LogGroupPlugin, "[JIIGAN_FUSION] process , cost time: %ldms",
                      getCurrentTimeStamp() - t1);

                if (iIsFusionDumpData & 0x02) {
                    char *ptr = (char *)outputFusionFrame.plane[0];
                    float fx = 0, fy = 0;
                    float fw = 0.05, fh = 0.05;
                    if (ptr) {
                        char mask = 128;
                        WSize size;
                        size.width = outputFusionFrame.width;
                        size.height = outputFusionFrame.height;
                        int stride = outputFusionFrame.stride;
                        int y_from = fy * size.height;
                        int y_to = (fy + fh) * size.height;
                        int x = fx * size.width;
                        int width = fw * size.width;

                        y_to = std::max(0, std::min(y_to, size.height));
                        y_from = std::max(0, std::min(y_from, y_to));
                        x = std::max(0, std::min(x, size.width));
                        width = std::max(0, std::min(width, size.width));

                        for (int y = y_from; y < y_to; ++y) {
                            memset(ptr + y * stride + x, mask, width);
                        }
                    }
                }
            } else {
                MLOGE(Mia2LogGroupPlugin, "%s:%d [JIIGAN_FUSION] ERROR process false \n", __func__,
                      __LINE__);
            }
        }

        if (iIsFusionDumpData & 0x01) {
            char outputFusionbuf[128];
            snprintf(outputFusionbuf, sizeof(outputFusionbuf), "jiiganFusion_out_frame_%dx%d",
                     outputFusionFrame.width, outputFusionFrame.height);
            PluginUtils::dumpToFile(outputFusionbuf, &outputFusionFrame);
        }
    }
#endif

    return PROCSUCCESS;

exit:
    // ret = Device->image->Copy(&outputFrame, &inputFrame);
    // DEV_IF_LOGE((ret != RET_OK), "MEMCOPY err");
    // resultInfo.result = PROCFAILED;
    // resultInfo.result = PROCSUCCESS;
    // resultInfo.isBypassed = true;
    return PROCFAILED;
}

ProcessRetStatus LDCPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    // ProcessResultInfo resultInfo;
    // resultInfo.result = PROCSUCCESS;
    // resultInfo.isBypassed = false;
    return PROCBADSTATE;
}

int LDCPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

void LDCPlugin::destroy()
{
    if (m_pLdcCapture) {
        delete m_pLdcCapture;
        m_pLdcCapture = NULL;
    }
    if (m_pNodeMetaData) {
        delete m_pNodeMetaData;
        m_pNodeMetaData = NULL;
    }

    if (m_debug == 1) {
        if (m_detectorCaptureId > 0) {
            Device->detector->Destroy(&m_detectorCaptureId);
        }
    }
#ifdef ENABLE_UW_W_FUSION_FUNC
    if (mNodeInstanceCapture) {
        mNodeInstanceCapture->deInit();

        delete mNodeInstanceCapture;
        mNodeInstanceCapture = NULL;
    }
#endif
}

} // namespace mialgo2
