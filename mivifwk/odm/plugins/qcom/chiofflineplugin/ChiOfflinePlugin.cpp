#include "ChiOfflinePlugin.h"

#include <dlfcn.h>

#include <string>

#include "MiaPluginUtils.h"
#include "OfflineDebugDataUtils.h"

#define ADVANCEMOONMODEON (35)

using namespace mialgo2;

const static uint32_t MASK_DISABLE_STATS = 0x1 << 0;
const static uint32_t s_postproc_mask =
    property_get_int32("persist.vendor.algoengine.postproc.mask", 0x0);

static std::map<std::string, ProcessMode> gNodeInstance2ProcessMode = {
    {"Jpeg", {YUVToJPEG, JPEG}},                                /// Jpeg
    {"BurstJpeg", {YUVToJPEG, BURSTJPEG}},                      /// Jpeg for burst mode
    {"Heic", {YUVToHEIC, JPEG}},                                /// Heic jpeg
    {"RemosaicRaw2Yuv", {RAWToYUV, REMOSAICBToYUV}},            /// RemosaicRaw2Yuv
    {"IdealRaw2Yuv", {RAWToYUV, IDEALRAWToYUV}},                /// IdealRaw2Yuv
    {"BayerRaw2Yuv", {RAWToYUV, BAYERRAWToYUV}},                /// BayerRaw2Yuv
    {"OfflineStatsAWBIdeal", {RAWToRAW, OFFLINESTATSAWBIDEAL}}, /// OfflineStatAWBIdeal
    {"OfflineStatsTintless", {RAWToRAW, OFFLINESTATSTINTLESS}}, /// OfflineStatTintless
    {"Mfnr", {RAWToYUV, MFSR}},                                 /// MFSR 8250
    {"YuvReprocess", {YUVToYUV, YUV420NV12ToYUV420NV12}},       /// YUV420NV12ToYUV420NV12
    {"YuvP010ToYuvNV12", {YUVToYUV, YUV420P010ToYUV420NV12}},   /// YuvP010ToYuvNV12
    {"FormatConvertor", {RAWToRAW, FORMATCONVERTOR}}            /// FormatConvertor
};

/*Provide Raw data verification R2Y function, just for debug*/
static void updataRawData(MiImageBuffer *buffer)
{
    /// BayerRaw for CUP, format:CAM_FORMAT_RAW16 bpp:14
    // const char file[128] = {"/data/vendor/camera/SuperIQ_RAW_2592x1940.raw"};
    /// IDEAL RAW for SE, format:CAM_FORMAT_RAW16 bpp:10
    const char file[128] = {"/data/vendor/camera/SE_RAW_4096x3072.raw"};
    FILE *fp = fopen(file, "rb");
    if (fp != NULL) {
        int width, height, stride;
        if (buffer->format == mialgo2::CAM_FORMAT_RAW16) {
            width = 2 * (buffer->width);
            height = buffer->height;
            stride = buffer->stride;
            char *addr = (char *)buffer->plane[0];
            MLOGI(Mia2LogGroupPlugin, "Raw pAddr %p file path %s!", addr, file);
            for (int i = 0; i < height; i++) {
                fread(addr, width, 1, fp);
                addr += stride;
            }
        }
        fclose(fp);
    } else {
        MLOGE(Mia2LogGroupPlugin, "file %s open failed!", file);
    }
}

ChiPostProcLib::~ChiPostProcLib()
{
    if (NULL != m_pChiPostProcLib) {
        if (0 != dlclose(m_pChiPostProcLib)) {
            const CHAR *pErrorMessage = dlerror();
            MLOGE(Mia2LogGroupPlugin, "dlclose error: %s",
                  (NULL == pErrorMessage) ? pErrorMessage : "N/A");
        }
        m_pChiPostProcLib = NULL;
        m_pCameraPostProcPreProcess = NULL;
        m_pCameraPostProcFlush = NULL;
    }
}

ChiPostProcLib::ChiPostProcLib() // cost 1ms
{
    m_pChiPostProcLib = dlopen("libofflinefeatureintf.so", RTLD_NOW | RTLD_LOCAL);
    m_pCameraPostProcCreate = NULL;
    m_pCameraPostProcPreProcess = NULL;
    m_pCameraPostProcProcess = NULL;
    m_pCameraPostProcFlush = NULL;
    m_pCameraPostProcRelease = NULL;
    m_pCameraPostProcDestroy = NULL;

    if (NULL != m_pChiPostProcLib) {
        m_pCameraPostProcCreate =
            (PCameraPostProc_Create)dlsym(m_pChiPostProcLib, "CameraPostProc_Create");
        if (NULL == m_pCameraPostProcCreate) {
            MLOGE(Mia2LogGroupPlugin, "CameraPostProc_Create pointers are NULL, dlsym failed");
        }

        m_pCameraPostProcPreProcess =
            (PCameraPostProc_PreProcess)dlsym(m_pChiPostProcLib, "CameraPostProc_PreProcess");
        if (NULL == m_pCameraPostProcPreProcess) {
            MLOGI(Mia2LogGroupPlugin, "CameraPostProc_PreProcess pointers are NULL, dlsym failed");
        }

        m_pCameraPostProcProcess =
            (PCameraPostProc_Process)dlsym(m_pChiPostProcLib, "CameraPostProc_Process");
        if (NULL == m_pCameraPostProcProcess) {
            MLOGE(Mia2LogGroupPlugin, "CameraPostProc_Process pointers are NULL, dlsym failed");
        }

        m_pCameraPostProcFlush =
            (PCameraPostProc_Flush)dlsym(m_pChiPostProcLib, "CameraPostProc_Flush");
        if (NULL == m_pCameraPostProcFlush) {
            MLOGI(Mia2LogGroupPlugin, "CameraPostProc_Flush pointers are NULL, dlsym failed");
        }

        m_pCameraPostProcRelease = (PCameraPostProc_ReleaseResources)dlsym(
            m_pChiPostProcLib, "CameraPostProc_ReleaseResources");
        if (NULL == m_pCameraPostProcRelease) {
            MLOGE(Mia2LogGroupPlugin,
                  "CameraPostProc_ReleaseSources pointers are NULL, dlsym failed");
        }

        m_pCameraPostProcDestroy =
            (PCameraPostProc_Destroy)dlsym(m_pChiPostProcLib, "CameraPostProc_Destroy");
        if (NULL == m_pCameraPostProcDestroy) {
            MLOGE(Mia2LogGroupPlugin, "CameraPostProc_Destroy pointers are NULL, dlsym failed");
        }
    } else {
        MLOGE(Mia2LogGroupPlugin, "No chipostproc lib, dlopen failed");
    }
}

ChiOfflinePlugin::~ChiOfflinePlugin()
{
    MLOGI(Mia2LogGroupPlugin, "[%s][%p]", mInstanceName.c_str(), this);
}

int ChiOfflinePlugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    if (createInfo == NULL) {
        MLOGE(Mia2LogGroupPlugin, "CreateInfo is NULL");
        return -1;
    }

    mNodeInterface = nodeInterface;
    mPostProc = NULL;
    mInpreProcessing = false;
    mInstanceName = createInfo->nodeInstance;
    memset(&mCreateParams, 0, sizeof(mCreateParams));
    mProcessMode = mCreateParams.processMode = MAXPOSTPROCMODE;
    mProcessModeType = mCreateParams.processModeType = MAXPOSTPROCMODETYPE;
    mCreateParams.operationMode = createInfo->operationMode;
    mLogicalCameraId = createInfo->logicalCameraId;
    mCreateParams.cameraId = mLogicalCameraId;
    mInpostProcessing = UINT32_MAX;
    mFlushFrameNum = UINT32_MAX;

    std::string nodeFeature = mInstanceName.substr(0, mInstanceName.find("Instance"));

    if (gNodeInstance2ProcessMode.find(nodeFeature) != gNodeInstance2ProcessMode.end()) {
        ProcessMode procMode = gNodeInstance2ProcessMode[nodeFeature];
        mProcessMode = mCreateParams.processMode = procMode.postMode;
        mProcessModeType = mCreateParams.processModeType = procMode.postModeType;
    }

    uint32_t fwkCamId = 0;
    mMiviVersion = 1.0;
    camera_metadata *pMeta = StaticMetadataWraper::getInstance()->getStaticMetadata(fwkCamId);
    void *pData = VendorMetadataParser::getTag(pMeta, "xiaomi.capabilities.MiviVersion");
    if (pData) {
        mMiviVersion = *static_cast<float *>(pData);
    }

    MLOGI(Mia2LogGroupPlugin,
          "%p:%s:Name:%s, processMode:%d, modeType:%d, propertyId:%d, version %f, opMode:%x", this,
          createInfo->nodeInstance.c_str(), mInstanceName.c_str(), mCreateParams.processMode,
          mCreateParams.processModeType, createInfo->nodePropertyId, mMiviVersion,
          mCreateParams.operationMode);

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.algoengine.chioffline.dump", prop, "0");
    mDumpData = (int32_t)atoi(prop);

    return 0;
}

bool ChiOfflinePlugin::isSupperNightBokehEnabled(camera_metadata_t *pMeta)
{
    uint8_t superNightBokehEnabled = 0;
    void *pData = VendorMetadataParser::getTag(pMeta, "xiaomi.bokeh.superNightEnabled");
    if (NULL != pData) {
        superNightBokehEnabled = *static_cast<uint8_t *>(pData);
    }
    return superNightBokehEnabled;
}

bool ChiOfflinePlugin::isMfsrEnabled(camera_metadata_t *pMeta)
{
    uint8_t mfsrEnabled = 0;
    void *pData = VendorMetadataParser::getTag(pMeta, "xiaomi.mfnr.enabled");
    if (NULL != pData) {
        mfsrEnabled = *static_cast<uint8_t *>(pData);
    }
    return mfsrEnabled;
}

bool ChiOfflinePlugin::isMiviRawZslEnabled(camera_metadata_t *pMeta)
{
    uint8_t isMiviRawZsl = 0;
    void *pData = NULL;
    pData = VendorMetadataParser::getTag(pMeta, "com.xiaomi.mivi2.rawzsl.enable");
    if (NULL != pData) {
        isMiviRawZsl = *static_cast<uint8_t *>(pData);
    }

    MLOGI(Mia2LogGroupPlugin, "[%s] isMiviRawZsl: %d", mInstanceName.c_str(), isMiviRawZsl);
    return isMiviRawZsl;
}

bool ChiOfflinePlugin::isOfflineStatsMode() const
{
    return (OFFLINESTATSAWBMIPI == mProcessModeType || OFFLINESTATSAWBIDEAL == mProcessModeType ||
            OFFLINESTATSTINTLESS == mProcessModeType);
}

bool ChiOfflinePlugin::isEnabled(MiaParams settings)
{
    if (YUVToJPEG == mProcessMode) {
        bool enable = true;

        void *pData = NULL;
        VendorMetadataParser::getVTagValue(settings.metadata, "com.xiaomi.mivi2.swjpeg.enable",
                                           &pData);
        if (NULL != pData) {
            enable = !(*static_cast<uint8_t *>(pData));
        }

        MLOGD(Mia2LogGroupPlugin, "Jpeg mode enable: %d", enable);
        return enable;
    } else if (YUVToHEIC == mProcessMode) {
        bool enable = true;
        MLOGD(Mia2LogGroupPlugin, "Heic Jpeg mode enable: %d", enable);
        return true;
    }

    if (YUVToYUV == mProcessMode) {
        void *pData = NULL;
        bool isEnable = false;

        if (YUV420P010ToYUV420NV12 == mProcessModeType) {
            VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.pureView.enabled",
                                               &pData);
            if (NULL != pData) {
                isEnable = *static_cast<int *>(pData);
            }
            MLOGI(Mia2LogGroupPlugin, "[%s] processModeType: %d pureViewEnable: %d",
                  mInstanceName.c_str(), mCreateParams.processModeType, isEnable);
        } else {
            int isSrEnable = false;
            const char *tagName = "xiaomi.superResolution.enabled";
            pData = NULL;
            VendorMetadataParser::getVTagValue(settings.metadata, tagName, &pData);
            if (NULL != pData) {
                isSrEnable = *static_cast<int *>(pData);
            }

            bool supermoonEnable = false;
            int mode = 0;
            pData = NULL;
            VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.ai.asd.sceneApplied",
                                               &pData);
            if (NULL != pData) {
                mode = *static_cast<int *>(pData);
            }
            if (mode == ADVANCEMOONMODEON) {
                supermoonEnable = true;
            }

            isEnable = (isSrEnable & (!supermoonEnable));
            MLOGI(Mia2LogGroupPlugin, "[%s] isSrEnable: %d supermoonEnable: %d isEnable: %d",
                  mInstanceName.c_str(), isSrEnable, supermoonEnable, isEnable);
        }
        return isEnable;
    }

    if (RAWToYUV == mProcessMode) {
        void *pData = NULL;
        bool superNightBokehEnabled = isSupperNightBokehEnabled(settings.metadata);
        bool mfsrEnabled = isMfsrEnabled(settings.metadata);

        if (MFSR == mProcessModeType) {
            bool mfnrEnable = false;
            if (mMiviVersion == 1.0) {
                if (superNightBokehEnabled &&
                    mCreateParams.operationMode == StreamConfigModeBokeh) {
                    mfnrEnable = true;
                }
            } else { // mivi2.0 case
                if (superNightBokehEnabled &&
                    mCreateParams.operationMode == StreamConfigModeBokeh) {
                    mfnrEnable = true;
                } else if (mfsrEnabled && mCreateParams.operationMode == StreamConfigModeSAT) {
                    void *data = NULL;
                    const char *mfnrEnablePtr = "xiaomi.mfnr.enabled";
                    uint8_t mfnrTagValue = 0;
                    VendorMetadataParser::getVTagValue(settings.metadata, mfnrEnablePtr, &data);
                    if (NULL != data) {
                        mfnrTagValue = *static_cast<uint8_t *>(data);
                    }

                    data = NULL;
                    const char *hdrEnablePtr = "xiaomi.hdr.enabled";
                    VendorMetadataParser::getVTagValue(settings.metadata, hdrEnablePtr, &data);

                    // NOTE: disable chioffline mfnr plugins when mfnr+hdr snapshot
                    bool hdrEnable = false;
                    if (NULL != data) {
                        hdrEnable = *static_cast<uint8_t *>(data);
                    }
                    if (!hdrEnable && (mfnrTagValue == 2)) {
                        mfnrEnable = true;
                    }
                }
            }

            MLOGI(Mia2LogGroupPlugin, "[%s] isMFSREnable: %d operationMode: 0x%x",
                  mInstanceName.c_str(), mfnrEnable, mCreateParams.operationMode);
            return mfnrEnable;
        }

        uint8_t superNightModeValue = 0;
        uint8_t nightMotionMode = 0;
        pData = NULL;
        VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.mivi.supernight.mode",
                                           &pData);
        if (NULL != pData) {
            superNightModeValue = *static_cast<uint8_t *>(pData);
        }

        pData = NULL;
        VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.nightmotioncapture.mode",
                                           &pData);
        if (NULL != pData) {
            nightMotionMode = *static_cast<uint8_t *>(pData);
        }

        if (BAYERRAWToYUV == mProcessModeType) {
            bool isEnable = false;
            if (superNightBokehEnabled && mCreateParams.operationMode == StreamConfigModeBokeh) {
                isEnable = true;
            }
            if ((5 <= superNightModeValue) && (superNightModeValue <= 8)) {
                isEnable = true;
            }
            if (0 != nightMotionMode) {
                isEnable = true;
            }
            // we enable all b2y nodes first and then we will prune them in lookForDisableNodes
            if (isMiviRawZslEnabled(settings.metadata) && !isMfsrEnabled(settings.metadata)) {
                isEnable = true;
            }
            MLOGI(Mia2LogGroupPlugin,
                  "[%s] superNightModeValue: %d nightMotionMode: %d isEnable: %d",
                  mInstanceName.c_str(), superNightModeValue, nightMotionMode, isEnable);
            return isEnable;
        } else if (IDEALRAWToYUV == mProcessModeType) {
            uint8_t rawHdrEnabled = 0;
            pData = NULL;
            VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.hdr.raw.enabled", &pData);
            if (NULL != pData) {
                rawHdrEnabled = *static_cast<uint8_t *>(pData);
            }

            bool isSNEnable = false;
            if ((1 <= superNightModeValue) && (superNightModeValue <= 4)) {
                isSNEnable = true;
            }

            MLOGI(Mia2LogGroupPlugin,
                  "[%s] isSNEnable: %d superNightBokehEnabled: %d rawHdrEnabled: %d",
                  mInstanceName.c_str(), isSNEnable, superNightBokehEnabled, rawHdrEnabled);

            return isSNEnable || superNightBokehEnabled || rawHdrEnabled;
        }
    }

    if ((RAWToRAW == mProcessMode) && !(s_postproc_mask & MASK_DISABLE_STATS)) {
        uint8_t superNightModeValue = 0;
        void *pData = NULL;
        VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.mivi.supernight.mode",
                                           &pData);
        if (NULL != pData) {
            superNightModeValue = *static_cast<uint8_t *>(pData);
        }

        pData = NULL;
        VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.nightmotioncapture.mode",
                                           &pData);
        int mode = pData ? *static_cast<int *>(pData) : 0;
        bool isSnscEnable = (0 != mode);

        pData = NULL;
        uint8_t isMiviRawNZSL = 0;
        VendorMetadataParser::getVTagValue(settings.metadata, "com.xiaomi.mivi2.rawnonzsl.enable",
                                           &pData);
        if (NULL != pData) {
            isMiviRawNZSL = *static_cast<uint8_t *>(pData);
        }

        bool isEnable = false;
        if (isOfflineStatsMode()) {
            if ((5 <= superNightModeValue) && (superNightModeValue <= 8)) {
                isEnable = true;
            }
        } else if (FORMATCONVERTOR == mProcessModeType) {
            // we only use formatconvertor in se/ellc/snsc when full scene raw moving up
            if ((superNightModeValue || isSnscEnable) > 0 && isMiviRawNZSL) {
                isEnable = true;
            }
        }

        MLOGI(Mia2LogGroupPlugin, "[%s] superNightModeValue: %d mProcessModeType: %d isEnable: %d",
              mInstanceName.c_str(), superNightModeValue, mProcessModeType, isEnable);
        return isEnable;
    }

    MLOGW(Mia2LogGroupPlugin, "[%s] mProcessMode %d not supported", mInstanceName.c_str(),
          mProcessMode);
    return false;
}

int ChiOfflinePlugin::preProcess(PreProcessInfo preProcessInfo)
{
    int result = 0;
    int inputNum = preProcessInfo.inputFormats.size();
    int outputNum = preProcessInfo.outputFormats.size();
    MLOGI(Mia2LogGroupPlugin, "[%s][%p] E operationMode 0x%x inputNum %d outputNum %d",
          mInstanceName.c_str(), this, mCreateParams.operationMode, inputNum, outputNum);

    if (mInpreProcessing || preProcessInfo.inputFormats.empty() ||
        preProcessInfo.outputFormats.empty()) {
        return result;
    }

    if (mPostProc == NULL) {
        std::unique_lock<std::mutex> locker{mMutex};
        mInpreProcessing = true;
        locker.unlock();

        mCreateParams.numInputStreams = inputNum;
        mCreateParams.numOutputStreams = outputNum;

        void *postProc = ChiPostProcLib::GetInstance()->create(&mCreateParams);
        if (postProc == NULL) {
            MLOGI(Mia2LogGroupPlugin, "ChiPostProc create failed");
            return -1;
        }

        PostProcProcessParams processParams{};
        for (int i = 0; i < inputNum; i++) {
            std::vector<PostProcBufferParams> inputBuffers;
            inputBuffers.resize(1);

            inputBuffers[0].format = preProcessInfo.inputFormats[i].format;
            inputBuffers[0].width = preProcessInfo.inputFormats[i].width;
            inputBuffers[0].height = preProcessInfo.inputFormats[i].height;
            inputBuffers[0].physicalCameraId = preProcessInfo.inputFormats[i].cameraId;
            if (inputBuffers[0].format == mialgo2::CAM_FORMAT_RAW10) {
                inputBuffers[0].bpp = 10;
            } else if (inputBuffers[0].format == mialgo2::CAM_FORMAT_RAW16) {
                inputBuffers[0].bpp = 14;
            }

            if (i == 0) {
                processParams.masterCameraId = inputBuffers[0].physicalCameraId;
            }
            processParams.inputBuffersMap.insert({i, std::move(inputBuffers)});
        }

        for (int i = 0; i < outputNum; i++) {
            PostProcBufferParams outputBuffer;
            outputBuffer.format = preProcessInfo.outputFormats[i].format;
            outputBuffer.width = preProcessInfo.outputFormats[i].width;
            outputBuffer.height = preProcessInfo.outputFormats[i].height;
            outputBuffer.physicalCameraId = preProcessInfo.outputFormats[i].cameraId;
            processParams.outputBufferMap.insert({i, outputBuffer});
        }
        result = ChiPostProcLib::GetInstance()->preProcess(postProc, &processParams);

        locker.lock();
        mPostProc = postProc;
        mInpreProcessing = false;
        mCond.notify_one();
        locker.unlock();
    }

    MLOGI(Mia2LogGroupPlugin, "[%s] X mPostProc: %p result %d", mInstanceName.c_str(), mPostProc,
          result);
    return result;
}

bool ChiOfflinePlugin::isPreProcessed()
{
    return mPostProc ? true : false;
}

bool ChiOfflinePlugin::supportPreProcess()
{
    return true;
}

void ChiOfflinePlugin::updateMatadata(PostProcBufferParams &bufferParams,
                                      camera_metadata_t *mainMeta)
{
    if (bufferParams.pMetadata == NULL) {
        MLOGW(Mia2LogGroupPlugin, "%s Input buffer metadata is NULL!", mInstanceName.c_str());
        return;
    }
    camera_metadata_t *&pMetadata = bufferParams.pMetadata;
    int ret = 0;

    // for all mode Enable Zoom Crop & IPEIQ
    int disableZoom = 0;
    ret = VendorMetadataParser::setVTagValue(
        pMetadata, "org.quic.camera2.ref.cropsize.DisableZoomCrop", &disableZoom, 1);
    if (VendorMetadataParser::VOK != ret) {
        MLOGW(Mia2LogGroupPlugin, "Cannot set DisableZoomCrop into metadata %p", pMetadata);
    }

    /// For camxnode, skip check "IsStreamCropEnabled"
    uint8_t offlineFeatureMode = 1;
    ret = VendorMetadataParser::setVTagValue(
        pMetadata, "com.xiaomi.camera.algoup.offlineFeatureMode", &offlineFeatureMode, 1);
    if (VendorMetadataParser::VOK != ret) {
        MLOGW(Mia2LogGroupPlugin, "Cannot set offlineFeatureMode into metadata %p", pMetadata);
    }

    /// Transmission AlgoEngine operation mode to camxnode
    ret = VendorMetadataParser::setVTagValue(pMetadata, "com.xiaomi.camera.algoup.offlineOprMode",
                                             &(mCreateParams.operationMode), 1);
    if (VendorMetadataParser::VOK != ret) {
        MLOGW(Mia2LogGroupPlugin, "Cannot set MiAlgoEngine operationMode into metadata %p",
              pMetadata);
    }

    // Get sensor ActiveArraySize
    ChiRect activeArrayRect = {};
    camera_metadata *pMeta =
        StaticMetadataWraper::getInstance()->getStaticMetadata(bufferParams.physicalCameraId);
    void *pData = VendorMetadataParser::getTag(pMeta, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE);

    if (pData) {
        activeArrayRect = *static_cast<ChiRect *>(pData);
    } else {
        activeArrayRect.width = bufferParams.width;
        activeArrayRect.height = bufferParams.height;
    }

    // Update RefCropSize: The android crop region is based on sensor ActiveArraySize.
    // So we should set RefCropSize to sensor ActiveArraySize.
    RefCropWindowSize refCropWindow = {.width = static_cast<int32_t>(activeArrayRect.width),
                                       .height = static_cast<int32_t>(activeArrayRect.height)};
    ret = VendorMetadataParser::setVTagValue(pMetadata, "org.quic.camera2.ref.cropsize.RefCropSize",
                                             &refCropWindow, sizeof(RefCropWindowSize));
    if (VendorMetadataParser::VOK != ret) {
        MLOGW(Mia2LogGroupPlugin, "Cannot set RefCropSize %p", pMetadata);
    }

    /// Update Crop Region
    ChiRect cropRegion = {
        .left = 0,
        .top = 0,
        .width = activeArrayRect.width,
        .height = activeArrayRect.height,
    };

    // In PD10->Yuv and Raw reprocess, except that the crop of sat is completed in the
    // sr/mfnr node, the crop in other algoup modes is completed by the current plugin
    if (mProcessModeType != YUV420NV12ToYUV420NV12) {
        void *pData = NULL;
        VendorMetadataParser::getVTagValue(pMetadata, "com.qti.chi.multicamerainfo.MultiCameraIds",
                                           &pData);
        if (pData) {
            // For MultiCam, sat provide cropRegion
            VendorMetadataParser::getVTagValue(pMetadata, "xiaomi.snapshot.cropRegion", &pData);
            if (NULL != pData) {
                cropRegion = *static_cast<ChiRect *>(pData);
                MLOGI(Mia2LogGroupPlugin, "Android xiaomi.snapshot.cropRegion (%u,%u,%u,%u)",
                      cropRegion.left, cropRegion.top, cropRegion.width, cropRegion.height);
            }
        } else {
            // For single camera, app provide ZOOM_RATIO
            void *pData = NULL;
            float zoomratio = 1.0f;
            uint32_t inputWidth = bufferParams.width;
            uint32_t inputHeight = bufferParams.height;

            VendorMetadataParser::getTagValue(mainMeta, ANDROID_CONTROL_ZOOM_RATIO, &pData);
            if (pData != NULL) {
                zoomratio = *static_cast<float *>(pData);
                MLOGI(Mia2LogGroupPlugin, "get zoomratio: %f", zoomratio);
            }

            zoomratio = (1.0f < zoomratio) ? zoomratio : 1.0f;

            cropRegion.width = activeArrayRect.width / zoomratio;
            cropRegion.height = activeArrayRect.height / zoomratio;
            cropRegion.left = (activeArrayRect.width - cropRegion.width) / 2.0f;
            cropRegion.top = (activeArrayRect.height - cropRegion.height) / 2.0f;
        }
    }

    ret = VendorMetadataParser::setTagValue(pMetadata, ANDROID_SCALER_CROP_REGION, &cropRegion, 4);
    if (VendorMetadataParser::VOK != ret) {
        MLOGW(Mia2LogGroupPlugin, "Cannot set CROP_REGION into metadata %p", pMetadata);
    } else {
        MLOGI(Mia2LogGroupPlugin, "cropRegion=(%u,%u,%u,%u),ref=%dx%d", cropRegion.left,
              cropRegion.top, cropRegion.width, cropRegion.height, activeArrayRect.width,
              activeArrayRect.height);
    }

    if (mProcessMode == YUVToJPEG || mProcessMode == YUVToHEIC) {
        void *pData = NULL;
        uint32_t rotation = 0; // GPU rotation degree

        VendorMetadataParser::getTagValue(pMetadata, ANDROID_JPEG_ORIENTATION, &pData);
        if (NULL != pData) {
            rotation = *static_cast<uint32_t *>(pData);
            MLOGI(Mia2LogGroupPlugin, "get JPEG rotation orientation= %d", rotation);
        }

        // Set override GPU rotation flag to TRUE
        if (0 != rotation) {
            uint32_t bGPURotation = 1;
            ret = VendorMetadataParser::setVTagValue(
                pMetadata, "org.quic.camera.overrideGPURotationUsecase.OverrideGPURotationUsecase",
                &bGPURotation, 1);
            if (VendorMetadataParser::VOK != ret) {
                MLOGW(Mia2LogGroupPlugin, "Cannot set OverrideGPURotationUsecase into metadata %p",
                      pMetadata);
            }
        }

        int32_t maxJpegSize = 0;
        pData = VendorMetadataParser::getTag(mainMeta, "com.xiaomi.mivi2.maxJpegSize");
        if (pData) {
            maxJpegSize = *static_cast<int32_t *>(pData);
            MLOGD(Mia2LogGroupPlugin, "get maxJpegSize: %d", maxJpegSize);
        }

        if (maxJpegSize > 0) {
            ret = VendorMetadataParser::setVTagValue(pMetadata, "com.xiaomi.mivi2.maxJpegSize",
                                                     &maxJpegSize, 1);
            if (VendorMetadataParser::VOK != ret) {
                MLOGW(Mia2LogGroupPlugin,
                      "Cannot set com.xiaomi.mivi2.maxJpegSize into metadata %p", pMetadata);
            }
        }
    }

    // offline stats will regenerate new 3a info
    if (!isOfflineStatsMode() && JPEG != mProcessModeType) {
        // remove 3A debug and tunning debug data for chiofflinepostproc
        ret = VendorMetadataParser::eraseVendorTag(pMetadata,
                                                   "org.quic.camera.debugdata.DebugDataAll");
        if (VendorMetadataParser::VOK != ret) {
            MLOGW(Mia2LogGroupPlugin, "Cannot erase DebugDataAll from metadata %p", pMetadata);
        }
    } else {
        pData = VendorMetadataParser::getTag(pMetadata, "org.quic.camera.debugdata.DebugDataAll");
        if (pData) {
            DebugData *pDebugData = reinterpret_cast<DebugData *>(pData);
            MLOGE(Mia2LogGroupPlugin, "[Offline DebugData] debugData = %p, pData = %p", pDebugData,
                  pDebugData->pData);
        }
    }
}

void ChiOfflinePlugin::prepareInput(std::map<uint32_t, std::vector<ImageParams>> &inputBuffersMap,
                                    PostProcProcessParams &processParams)
{
    for (auto it = inputBuffersMap.begin(); it != inputBuffersMap.end(); ++it) {
        std::vector<PostProcBufferParams> inputbuffers;
        uint32_t portId = it->first;
        auto &imageParamsVec = it->second;
        std::vector<PostProcBufferParams> inputBufferParamsVec;

        camera_metadata_t *mainRequestMeta = imageParamsVec[0].pMetadata;
        for (int i = 0; i < imageParamsVec.size(); i++) {
            PostProcBufferParams inputBufferParams;
            inputBufferParams.format = imageParamsVec[i].format.format;
            inputBufferParams.width = imageParamsVec[i].format.width;
            inputBufferParams.height = imageParamsVec[i].format.height;
            inputBufferParams.phHandle = imageParamsVec[i].pNativeHandle;
            inputBufferParams.bufferType = imageParamsVec[i].bufferType;
            inputBufferParams.fd = &imageParamsVec[i].fd[0];
            inputBufferParams.physicalCameraId = imageParamsVec[i].cameraId;
            inputBufferParams.pMetadata = imageParamsVec[i].pPhyCamMetadata;
            if (inputBufferParams.pMetadata == NULL) {
                inputBufferParams.pMetadata = imageParamsVec[i].pMetadata;
            }
            updateMatadata(inputBufferParams, mainRequestMeta);
            getMatadataFillParams(inputBufferParams);

            if (portId == 0 && i == 0) {
                processParams.masterCameraId = inputBufferParams.physicalCameraId;
            }
            inputBufferParamsVec.push_back(inputBufferParams);

            MLOGI(Mia2LogGroupPlugin,
                  "[%s][%p]: Input[%d][%d] %dx%d, fmt=%d, phHandle=%p, bufType=%d, cameraId=%p",
                  mInstanceName.c_str(), this, portId, i, inputBufferParams.width,
                  inputBufferParams.height, inputBufferParams.format, inputBufferParams.phHandle,
                  inputBufferParams.bufferType, inputBufferParams.physicalCameraId);

            if (mDumpData) {
                MiImageBuffer inputFrame;
                auto &inputBuffers = inputBuffersMap.begin()->second;
                inputFrame.format = inputBuffers[i].format.format;
                inputFrame.width = inputBuffers[i].format.width;
                inputFrame.height = inputBuffers[i].format.height;
                inputFrame.plane[0] = inputBuffers[i].pAddr[0];
                inputFrame.plane[1] = inputBuffers[i].pAddr[1];
                inputFrame.stride = inputBuffers[i].planeStride;
                inputFrame.scanline = inputBuffers[i].sliceheight;

                char inputBuf[128];
                snprintf(inputBuf, sizeof(inputBuf), "chioffline_input[%d][%d]_%dx%d", portId, i,
                         inputBuffers[i].format.width, inputBuffers[i].format.height);
                // updataRawData(&inputFrame);   // Debug
                PluginUtils::dumpToFile(inputBuf, &inputFrame);
            }
        }

        processParams.inputBuffersMap.insert({portId, std::move(inputBufferParamsVec)});
    }
}

void ChiOfflinePlugin::prepareOutput(std::map<uint32_t, std::vector<ImageParams>> &outputBuffersMap,
                                     PostProcProcessParams &processParams)
{
    for (auto it = outputBuffersMap.begin(); it != outputBuffersMap.end(); ++it) {
        uint32_t portId = it->first;
        PostProcBufferParams outputBufferParams;
        outputBufferParams.format = it->second.at(0).format.format;
        outputBufferParams.width = it->second.at(0).format.width;
        outputBufferParams.height = it->second.at(0).format.height;
        outputBufferParams.phHandle = it->second.at(0).pNativeHandle;
        outputBufferParams.bufferType = it->second.at(0).bufferType;
        outputBufferParams.fd = &it->second.at(0).fd[0];
        outputBufferParams.pMetadata = it->second.at(0).pPhyCamMetadata;
        if (outputBufferParams.pMetadata == NULL) {
            outputBufferParams.pMetadata = it->second.at(0).pMetadata;
        }
        MLOGI(Mia2LogGroupPlugin,
              "[%s][%p]: Output[%d] %dx%d, fmt=%d, phHandle=%p, bufType=%d, pMetadata=%p",
              mInstanceName.c_str(), this, portId, outputBufferParams.width,
              outputBufferParams.height, outputBufferParams.format, outputBufferParams.phHandle,
              outputBufferParams.bufferType, outputBufferParams.pMetadata);

        processParams.outputBufferMap.insert({it->first, outputBufferParams});
    }
}

void ChiOfflinePlugin::getMatadataFillParams(PostProcBufferParams &bufferParams)
{
    void *pData = NULL;
    uint8_t rawBpp = 0;

    camera_metadata_t *pPhyCamMetadata = bufferParams.pMetadata;
    if (bufferParams.format == mialgo2::CAM_FORMAT_RAW10) {
        bufferParams.bpp = 10;
    } else if (bufferParams.format == mialgo2::CAM_FORMAT_RAW16) {
        bufferParams.bpp = 14;
    } else {
        pData = NULL;
        VendorMetadataParser::getVTagValue(pPhyCamMetadata, "com.xiaomi.camera.algoup.rawbpp",
                                           &pData);
        if (pData) {
            rawBpp = *static_cast<uint8_t *>(pData);
            bufferParams.bpp = rawBpp;
        }
    }

    MLOGD(Mia2LogGroupPlugin, "%s: phyCamId=%d,bpp=%d", mInstanceName.c_str(),
          bufferParams.physicalCameraId, bufferParams.bpp);
}

ProcessRetStatus ChiOfflinePlugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    int ret = validateRequest(processRequestInfo);
    if (ret != 0) {
        MLOGE(Mia2LogGroupPlugin, "[%s][%p] validateRequest failed!", mInstanceName.c_str(), this);
        resultInfo = PROCFAILED;
        return resultInfo;
    }

    if (mFlushFrameNum != UINT32_MAX && processRequestInfo->frameNum == mFlushFrameNum) {
        MLOGE(Mia2LogGroupPlugin, "[%s][%p] flush frame %u!", mInstanceName.c_str(), this,
              mFlushFrameNum);
        resultInfo = PROCFAILED;
        mFlushFrameNum = UINT32_MAX;
        return resultInfo;
    }

    // ChiPostProcLib create
    if (mPostProc == NULL) {
        if (supportPreProcess() && mInpreProcessing) {
            std::unique_lock<std::mutex> locker{mMutex};

            MLOGI(Mia2LogGroupPlugin, "[%s][%p] need to wait for the preProcess to complete",
                  mInstanceName.c_str(), this);
            mCond.wait(locker, [this]() { return !mInpreProcessing; });
            MLOGI(Mia2LogGroupPlugin, "[%s][%p] wait for preprocess end", mInstanceName.c_str(),
                  this);
            locker.unlock();

            MASSERT(mPostProc);
        } else {
            mCreateParams.numInputStreams = processRequestInfo->inputBuffersMap.size();
            mCreateParams.numOutputStreams = processRequestInfo->outputBuffersMap.size();
            mPostProc = ChiPostProcLib::GetInstance()->create(&mCreateParams);
        }
    }

    if (mPostProc == NULL) {
        MLOGE(Mia2LogGroupPlugin, "[%s][%p] CameraPostProc_Create failed!", mInstanceName.c_str(),
              this);
        MASSERT(mPostProc);
    }

    // ChiPostProcLib process
    PostProcProcessParams processParams{};
    processParams.frameNum = processRequestInfo->frameNum;
    prepareInput(processRequestInfo->inputBuffersMap, processParams);
    prepareOutput(processRequestInfo->outputBuffersMap, processParams);

    if (mFlushFrameNum != UINT32_MAX && processParams.frameNum == mFlushFrameNum) {
        MLOGE(Mia2LogGroupPlugin, "[%s][%p] flush frame %u!", mInstanceName.c_str(), this,
              mFlushFrameNum);
        resultInfo = PROCFAILED;
        mFlushFrameNum = UINT32_MAX;
        return resultInfo;
    }
    mInpostProcessing = processParams.frameNum;
    PostProcRetStatus status = ChiPostProcLib::GetInstance()->process(mPostProc, &processParams);
    mInpostProcessing = UINT32_MAX;

    if (POSTPROCSUCCESS != status) {
        MLOGE(Mia2LogGroupPlugin, "CameraPostProc_Process failed %d", status);
        resultInfo = PROCFAILED;
    } else if ((POSTPROCSUCCESS == status) && (mNodeInterface.pSetResultMetadata != NULL)) {
        /// For exif
        std::string results = mInstanceName + ":1";
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, processRequestInfo->frameNum,
                                          processRequestInfo->timeStamp, results);
    }

    // Note: Offline DebugData start
    if (JPEG == mProcessModeType && !processParams.inputBuffersMap.empty() &&
        !processParams.inputBuffersMap[0].empty()) {
        // Jpeg only need one port, one input
        camera_metadata_t *mainImageMeta = processRequestInfo->inputBuffersMap[0][0].pMetadata;
        if (mainImageMeta) {
            void *pData = VendorMetadataParser::getTag(mainImageMeta,
                                                       "org.quic.camera.debugdata.DebugDataAll");
            if (pData) {
                DebugData *pDebugData = reinterpret_cast<DebugData *>(pData);
                MLOGI(Mia2LogGroupPlugin,
                      "[Offline DebugData] debugData = %p, pData = %p, size=%zu", pDebugData,
                      pDebugData->pData, pDebugData->size);
                if (pDebugData->pData != NULL)
                    SubDebugDataReference(pDebugData);
            } else {
                MLOGW(Mia2LogGroupPlugin,
                      "[Offline DebugData]: Get debugdata failed! For burst or slave result not "
                      "have DebugDataTag");
            }
        } else {
            MLOGW(Mia2LogGroupPlugin, "[Offline DebugData]: mainImageMeta %p", mainImageMeta);
        }
    }
    // Note: Offline DebugData end

    if (mDumpData) {
        MiImageBuffer outputFrame{0};
        auto &outputBuffers = processRequestInfo->outputBuffersMap.begin()->second;
        outputFrame.format = outputBuffers[0].format.format;
        outputFrame.width = outputBuffers[0].format.width;
        outputFrame.height = outputBuffers[0].format.height;
        outputFrame.plane[0] = outputBuffers[0].pAddr[0];
        outputFrame.plane[1] = outputBuffers[0].pAddr[1];
        outputFrame.stride = outputBuffers[0].planeStride;
        outputFrame.scanline = outputBuffers[0].sliceheight;
        char outputBuf[128];
        snprintf(outputBuf, sizeof(outputBuf), "chioffline_output_%dx%d",
                 outputBuffers[0].format.width, outputBuffers[0].format.height);
        PluginUtils::dumpToFile(outputBuf, &outputFrame);
    }

    MLOGI(Mia2LogGroupPlugin, "[%s][%p] process result: %d", mInstanceName.c_str(), this, status);
    return resultInfo;
}

ProcessRetStatus ChiOfflinePlugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    if (!isOfflineStatsMode()) {
        MLOGE(Mia2LogGroupPlugin, "[%s][%p] process mode type is %d, not inplace plugin!",
              mInstanceName.c_str(), this, mProcessModeType);
        resultInfo = PROCFAILED;
        return resultInfo;
    }

    std::vector<ImageParams> &inputParams = processRequestInfo->inputBuffersMap.begin()->second;
    int inputFormat = inputParams[0].format.format;
    if (inputFormat != mialgo2::CAM_FORMAT_RAW16 && inputFormat != mialgo2::CAM_FORMAT_RAW10) {
        MLOGE(Mia2LogGroupPlugin, "[%s][%p] inputFormat not Raw!", mInstanceName.c_str(), this);
        resultInfo = PROCFAILED;
        return resultInfo;
    }

    if (mFlushFrameNum != UINT32_MAX && processRequestInfo->frameNum == mFlushFrameNum) {
        MLOGE(Mia2LogGroupPlugin, "[%s][%p] flush frame %u!", mInstanceName.c_str(), this,
              mFlushFrameNum);
        resultInfo = PROCFAILED;
        mFlushFrameNum = UINT32_MAX;
        return resultInfo;
    }

    if (mPostProc == NULL) {
        if (supportPreProcess() && mInpreProcessing) {
            std::unique_lock<std::mutex> locker{mMutex};

            MLOGI(Mia2LogGroupPlugin, "[%s][%p] need to wait for the preProcess to complete",
                  mInstanceName.c_str(), this);
            mCond.wait(locker, [this]() { return !mInpreProcessing; });
            MLOGI(Mia2LogGroupPlugin, "[%s][%p] wait for preprocess end", mInstanceName.c_str(),
                  this);
            locker.unlock();

            MASSERT(mPostProc);
        } else {
            mCreateParams.numInputStreams = 1;
            mCreateParams.numOutputStreams = 1;
            mPostProc = ChiPostProcLib::GetInstance()->create(&mCreateParams);
        }
    }
    if (mPostProc == NULL) {
        MLOGE(Mia2LogGroupPlugin, "[%s][%p] CameraPostProc_Create failed!", mInstanceName.c_str(),
              this);
        MASSERT(mPostProc);
    }

    PostProcProcessParams processParams{};
    processParams.frameNum = processRequestInfo->frameNum;
    prepareInput(processRequestInfo->inputBuffersMap, processParams);

    // NOTE: For chiofflinefeaturegraph not support without output. Just point to the same mem.
    PostProcBufferParams outputBufferParams{};
    outputBufferParams.format = inputParams[0].format.format;
    outputBufferParams.width = inputParams[0].format.width;
    outputBufferParams.height = inputParams[0].format.height;
    outputBufferParams.phHandle = inputParams[0].pNativeHandle;
    outputBufferParams.bufferType = inputParams[0].bufferType;
    outputBufferParams.fd = &inputParams[0].fd[0];
    outputBufferParams.pMetadata = inputParams[0].pPhyCamMetadata;
    if (outputBufferParams.pMetadata == NULL) {
        outputBufferParams.pMetadata = inputParams[0].pMetadata;
    }
    processParams.outputBufferMap.insert({0, outputBufferParams});

    if (mFlushFrameNum != UINT32_MAX && processParams.frameNum == mFlushFrameNum) {
        MLOGE(Mia2LogGroupPlugin, "[%s][%p] flush frame %u!", mInstanceName.c_str(), this,
              mFlushFrameNum);
        resultInfo = PROCFAILED;
        mFlushFrameNum = UINT32_MAX;
        return resultInfo;
    }
    PostProcRetStatus status = ChiPostProcLib::GetInstance()->process(mPostProc, &processParams);

    if (POSTPROCSUCCESS != status) {
        MLOGE(Mia2LogGroupPlugin, "CameraPostProc_Process failed %d", status);
        resultInfo = PROCFAILED;
    } else if ((POSTPROCSUCCESS == status) && (mNodeInterface.pSetResultMetadata != NULL)) {
        /// For exif
        std::string results = mInstanceName + ":success";
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, processRequestInfo->frameNum,
                                          processRequestInfo->timeStamp, results);
    }

    MLOGI(Mia2LogGroupPlugin, "[%s][%p] process result: %d", mInstanceName.c_str(), this, status);
    return resultInfo;
}

int ChiOfflinePlugin::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    int result = -1;
    mFlushFrameNum = flushRequestInfo->frameNum;

    if (mPostProc && mInpostProcessing != UINT32_MAX && mFlushFrameNum == mInpostProcessing) {
        MLOGI(Mia2LogGroupPlugin, "[%s][%p] mPostProc: %p frame: %u", mInstanceName.c_str(), this,
              mPostProc, mInpostProcessing);
        result = ChiPostProcLib::GetInstance()->flush(mPostProc, mFlushFrameNum);
        mFlushFrameNum = UINT32_MAX;
    }
    return result;
}

void ChiOfflinePlugin::destroy()
{
    MLOGI(Mia2LogGroupPlugin, "[%s][%p] mPostProc: %p", mInstanceName.c_str(), this, mPostProc);
    if (mPostProc != NULL) {
        ChiPostProcLib::GetInstance()->releaseResources(mPostProc);
        ChiPostProcLib::GetInstance()->destroy(mPostProc);
        mPostProc = NULL;
    }
}

int ChiOfflinePlugin::validateRequest(ProcessRequestInfo *request)
{
    int result = 0;
    if (request->inputBuffersMap.empty() || request->outputBuffersMap.empty()) {
        return -1;
    }

    std::vector<ImageParams> &inputParams = request->inputBuffersMap.begin()->second;
    std::vector<ImageParams> &outputParams = request->outputBuffersMap.begin()->second;
    int inputFormat = inputParams[0].format.format;
    int outputFormat = outputParams[0].format.format;
    switch (mProcessMode) {
    case YUVToYUV:
        if (mProcessModeType == YUV420NV12ToYUV420NV12) {
            if ((inputFormat != outputFormat) ||
                (inputFormat != mialgo2::CAM_FORMAT_YUV_420_NV21 &&
                 inputFormat != mialgo2::CAM_FORMAT_YUV_420_NV12)) {
                result = -1;
            }
        } else if (mProcessModeType == YUV420P010ToYUV420NV12) {
            if (inputFormat != mialgo2::CAM_FORMAT_P010 ||
                (outputFormat != mialgo2::CAM_FORMAT_YUV_420_NV21 &&
                 outputFormat != mialgo2::CAM_FORMAT_YUV_420_NV12)) {
                result = -1;
            }
        } else {
            MLOGE(Mia2LogGroupPlugin, "[%s][%p] unknow type %d for process mode YUVToYUV",
                  mInstanceName.c_str(), this, mProcessModeType);
            result = -1;
        }
        break;
    case RAWToYUV:
        if ((inputFormat != mialgo2::CAM_FORMAT_RAW16 &&
             inputFormat != mialgo2::CAM_FORMAT_RAW10) ||
            (outputFormat != mialgo2::CAM_FORMAT_YUV_420_NV21 &&
             outputFormat != mialgo2::CAM_FORMAT_YUV_420_NV12)) {
            result = -1;
        }
        break;
    case YUVToJPEG:
        if ((inputFormat != mialgo2::CAM_FORMAT_YUV_420_NV21 &&
             inputFormat != mialgo2::CAM_FORMAT_YUV_420_NV12) ||
            (outputFormat != mialgo2::CAM_FORMAT_BLOB)) {
            result = -1;
        }
        break;
    case YUVToHEIC:
        if ((inputFormat != mialgo2::CAM_FORMAT_YUV_420_NV21 &&
             inputFormat != mialgo2::CAM_FORMAT_YUV_420_NV12) ||
            (outputFormat != mialgo2::CAM_FORMAT_BLOB &&
             outputFormat != mialgo2::CAM_FORMAT_IMPLEMENTATION_DEFINED)) {
            result = -1;
        }
        break;
    case RAWToRAW:
        if (inputFormat != mialgo2::CAM_FORMAT_RAW10 && outputFormat != mialgo2::CAM_FORMAT_RAW16) {
            result = -1;
        }
        break;
    default:
        MLOGE(Mia2LogGroupPlugin, "[%s][%p] unknow process mode %d", mInstanceName.c_str(), this,
              mProcessMode);
        result = -1;
        break;
    }

    if (result != 0) {
        MLOGW(Mia2LogGroupPlugin, "[%s][%p] ProcessMode mode %d inputFormat: %d, outputFormat: %d",
              mInstanceName.c_str(), this, mProcessMode, inputFormat, outputFormat);
    }
    return result;
}
