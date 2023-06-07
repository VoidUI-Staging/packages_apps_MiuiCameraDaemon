#include "MiBokehPlugin.h"

#include "arcsoft_portrait_lighting_common.h"
#include "arcsoft_portrait_lighting_image.h"
#include "asvloffscreen.h"
#include "mi_portrait_3d_lighting_image.h"
#include "mi_portrait_lighting_common.h"

#undef LOG_TAG
#define LOG_TAG "MIBOKEH_PLUGIN"

namespace mialgo2 {

/*begin for add front portrait light*/
typedef struct
{
    int orient;
    int mode;
    int iDepthSize;
    unsigned char *pDepthMap;
    ASVLOFFSCREEN *pSrcImg;
    ASVLOFFSCREEN *pDstImg;
    ASVLOFFSCREEN *pBokehImg;
    MHandle *m_hRelightingHandle;
} _tlocal_thread_params;

typedef struct
{
    int i32Width;
    int i32Height;
    MHandle *m_phRelightingHandle;
} relight_thread_params;

static bool m_useBokeh = false;

typedef enum {
    Default,
    BokehOnly,     // 1 output   bokeh             for thridpart camera
    BokehDepth,    //
    BokehDepthRaw, // 3 output   raw,bokeh,depth   for miui camera
} MibokehOutputMode;

// used for performance
static long long __attribute__((unused)) gettime()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((long long)time.tv_sec * 1000 + time.tv_usec / 1000);
}

static void *mibokeh_relighting_thread_proc_init(void *params)
{
    int nRet = MOK;
    double t0, t1;
    t0 = t1 = 0.0f;
    relight_thread_params *prtp = (relight_thread_params *)params;
    int width = prtp->i32Width;
    int height = prtp->i32Height;
    MHandle relightingHandle = NULL;
    m_useBokeh = false;

    if (relightingHandle) {
        MLOGE(Mia2LogGroupPlugin, "ERR! relightingHandle %p", relightingHandle);
        return NULL;
    }
    t0 = gettime();
    nRet = AILAB_PLI_Init(&relightingHandle, MI_PL_INIT_MODE_PERFORMANCE, width, height);
    if (nRet != MOK) {
        MLOGE(Mia2LogGroupPlugin, "midebug: AILAB_PLI_Init =failed= %d\n", nRet);
        m_useBokeh = true;
        return NULL;
    }

    t1 = gettime();
    *(prtp->m_phRelightingHandle) = relightingHandle;
    MLOGD(Mia2LogGroupPlugin, "MIBOKEH_CTB:mibokeh_relighting_thread_proc_init= %d: init=%.2f \n",
          nRet, t1 - t0);

    return NULL;
}
/*end for add front portrait light*/

Mibokehplugin::~Mibokehplugin() {}

int Mibokehplugin::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    double startTime = PluginUtils::nowMSec();
    m_currentFlip = 0;
    m_pNodeInstance = NULL;
    m_sceneFlag = 0;
    m_pDepthMap = NULL;
    m_orientation = 0;
    m_orient = 0;
    m_hRelightingHandle = NULL;
    mNodeInterface = nodeInterface;

    if (NULL == m_pNodeInstance) {
        m_pNodeInstance = new MiBokehMiui();
        m_pNodeInstance->init(false);
    }

    MLOGI(Mia2LogGroupPlugin, "MiBokeh");

    return MIA_RETURN_OK;
}

void Mibokehplugin::destroy()
{
    /*begin add for front 3D light*/
    if (m_pDepthMap != NULL) {
        MLOGI(Mia2LogGroupPlugin, "free m_pDepthMap %p", m_pDepthMap);
        free(m_pDepthMap);
        m_pDepthMap = NULL;
        m_depthSize = 0;
    }

    if (m_hRelightingHandle) {
        ARC_PLI_Uninit(&m_hRelightingHandle);
        m_hRelightingHandle = NULL;
    }
    /*end add for front 3D light*/

    if (NULL != m_pNodeInstance) {
        delete m_pNodeInstance;
        m_pNodeInstance = NULL;
    }
}

bool Mibokehplugin::isEnabled(MiaParams settings)
{
    void *pData = NULL;
    int32_t mibokehEnable = 0;
    const char *mibokeh = "xiaomi.bokeh.enabled";
    VendorMetadataParser::getVTagValue(settings.metadata, mibokeh, &pData);
    if (NULL != pData) {
        mibokehEnable = *static_cast<int32_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "Mibokehplugin getMetaData MibokehEnabled  %d", mibokehEnable);
    } else {
        MLOGE(Mia2LogGroupPlugin, "can not found tag \"xiaomi.bokeh.enabled\"");
    }
    return mibokehEnable ? true : false;
}

ProcessRetStatus Mibokehplugin::processRequest(ProcessRequestInfo *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.algoengine.mibokeh.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);
    m_processedFrame = processRequestInfo->frameNum;
    MibokehOutputMode outputmode = Default;

    auto &phOutputBuffer = processRequestInfo->outputBuffersMap.at(0);
    outputmode = MibokehOutputMode(processRequestInfo->outputBuffersMap.size());
    MLOGI(Mia2LogGroupPlugin, "output buffer number = %d", outputmode);

    double start = PluginUtils::nowMSec();
    switch (outputmode) {
    case BokehOnly:
        processBokehOnly(iIsDumpData, processRequestInfo);
        break;
    case BokehDepth:
        break;
    case BokehDepthRaw:
        processBokehDepthRaw(iIsDumpData, processRequestInfo);
        break;
    default:
        break;
    }
    double end = PluginUtils::nowMSec();

    if (mNodeInterface.pSetResultMetadata != NULL) {
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf),
                 "miBokeh:{processedFrame:%d blurLevel:%.3f sceneFlag:%d costTime:%.3f}",
                 m_processedFrame, m_fNumberApplied, m_sceneFlag, end - start);
        std::string results(buf);
        mNodeInterface.pSetResultMetadata(mNodeInterface.owner, processRequestInfo->frameNum,
                                          processRequestInfo->timeStamp, results);
    }

    return resultInfo;
};

void Mibokehplugin::processBokehDepthRaw(int32_t iIsDumpData,
                                         ProcessRequestInfo *processRequestInfo)
{
    auto &phInputBuffer = processRequestInfo->inputBuffersMap.at(0);
    camera_metadata_t *pMeta = phInputBuffer[0].pMetadata;

    // prot0 ->bokeh frame
    auto &phOutputBuffer = processRequestInfo->outputBuffersMap.at(0);

    MiImageBuffer inputFrame, outputBokehFrame, outputRawFrame, outputDepthFrame;
    inputFrame.format = phInputBuffer[0].format.format;
    inputFrame.width = phInputBuffer[0].format.width;
    inputFrame.height = phInputBuffer[0].format.height;
    inputFrame.plane[0] = phInputBuffer[0].pAddr[0];
    inputFrame.plane[1] = phInputBuffer[0].pAddr[1];
    inputFrame.stride = phInputBuffer[0].planeStride;
    inputFrame.scanline = phInputBuffer[0].sliceheight;
    inputFrame.numberOfPlanes = phOutputBuffer[0].numberOfPlanes;

    outputBokehFrame.format = phOutputBuffer[0].format.format;
    outputBokehFrame.width = phOutputBuffer[0].format.width;
    outputBokehFrame.height = phOutputBuffer[0].format.height;
    outputBokehFrame.plane[0] = phOutputBuffer[0].pAddr[0];
    outputBokehFrame.plane[1] = phOutputBuffer[0].pAddr[1];
    outputBokehFrame.stride = phOutputBuffer[0].planeStride;
    outputBokehFrame.scanline = phOutputBuffer[0].sliceheight;
    outputBokehFrame.numberOfPlanes = phOutputBuffer[0].numberOfPlanes;

    // prot1 ->raw frame
    auto &phOutputBuffer1 = processRequestInfo->outputBuffersMap.at(1);
    outputRawFrame.format = phOutputBuffer1[0].format.format;
    outputRawFrame.width = phOutputBuffer1[0].format.width;
    outputRawFrame.height = phOutputBuffer1[0].format.height;
    outputRawFrame.plane[0] = phOutputBuffer1[0].pAddr[0];
    outputRawFrame.plane[1] = phOutputBuffer1[0].pAddr[1];
    outputRawFrame.stride = phOutputBuffer1[0].planeStride;
    outputRawFrame.scanline = phOutputBuffer1[0].sliceheight;
    outputRawFrame.numberOfPlanes = phOutputBuffer1[0].numberOfPlanes;

    // prot2 ->depth frame
    auto &phOutputBuffer2 = processRequestInfo->outputBuffersMap.at(2);
    outputDepthFrame.format = phOutputBuffer2[0].format.format; // CAM_FORMAT_Y16
    outputDepthFrame.width = phOutputBuffer2[0].format.width;
    outputDepthFrame.height = phOutputBuffer2[0].format.height;
    outputDepthFrame.plane[0] = phOutputBuffer2[0].pAddr[0];
    outputDepthFrame.stride = phOutputBuffer2[0].planeStride;
    outputDepthFrame.scanline = phOutputBuffer2[0].sliceheight;
    outputDepthFrame.numberOfPlanes = phOutputBuffer2[0].numberOfPlanes;

    MLOGI(Mia2LogGroupPlugin,
          "%s:%d input width: %d, height: %d, stride: %d, scanline: %d, format: %d", __func__,
          __LINE__, inputFrame.width, inputFrame.height, inputFrame.stride, inputFrame.scanline,
          inputFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "%s:%d output bokeh width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          __func__, __LINE__, outputBokehFrame.width, outputBokehFrame.height,
          outputBokehFrame.stride, outputBokehFrame.scanline, outputBokehFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "%s:%d output raw width: %d, height: %d, stride: %d, scanline: %d, format: %d", __func__,
          __LINE__, outputRawFrame.width, outputRawFrame.height, outputRawFrame.stride,
          outputRawFrame.scanline, outputRawFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "%s:%d output depth width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          __func__, __LINE__, outputDepthFrame.width, outputDepthFrame.height,
          outputDepthFrame.stride, outputDepthFrame.scanline, outputDepthFrame.format);

    if (iIsDumpData) {
        char inbuf[128];
        snprintf(inbuf, sizeof(inbuf), "mibokeh_input_%dx%d", inputFrame.width, inputFrame.height);
        PluginUtils::dumpToFile(inbuf, &inputFrame);
    }

    PluginUtils::miCopyBuffer(&outputRawFrame, &inputFrame);
    int ret = processBuffer(&inputFrame, &outputBokehFrame, &outputDepthFrame, pMeta);
    if (ret) {
        MLOGE(Mia2LogGroupPlugin, "[CAM_LOG_SYSTEM] MiBokeh processBuffer fail %d", ret);
    }

    if (iIsDumpData) {
        char outputbokehbuf[128];
        snprintf(outputbokehbuf, sizeof(outputbokehbuf), "mibokeh_output_bokeh_%dx%d",
                 outputBokehFrame.width, outputBokehFrame.height);
        PluginUtils::dumpToFile(outputbokehbuf, &outputBokehFrame);
        char outputRawbuf[128];
        snprintf(outputRawbuf, sizeof(outputRawbuf), "mibokeh_output_Raw_%dx%d",
                 outputRawFrame.width, outputRawFrame.height);
        PluginUtils::dumpToFile(outputRawbuf, &outputRawFrame);
        char outputdepthbuf[128];
        snprintf(outputdepthbuf, sizeof(outputdepthbuf), "mibokeh_output_depth_%dx%d",
                 outputDepthFrame.width, outputDepthFrame.height);
        PluginUtils::dumpToFile(outputdepthbuf, &outputDepthFrame);
    }
}

void Mibokehplugin::processBokehOnly(int32_t iIsDumpData, ProcessRequestInfo *processRequestInfo)
{
    auto &phInputBuffer = processRequestInfo->inputBuffersMap.at(0);
    camera_metadata_t *pMeta = phInputBuffer[0].pMetadata;

    // prot0 ->bokeh frame
    auto &phOutputBuffer = processRequestInfo->outputBuffersMap.at(0);

    MiImageBuffer inputFrame, outputBokehFrame, outputDepthFrame;
    inputFrame.format = phInputBuffer[0].format.format;
    inputFrame.width = phInputBuffer[0].format.width;
    inputFrame.height = phInputBuffer[0].format.height;
    inputFrame.plane[0] = phInputBuffer[0].pAddr[0];
    inputFrame.plane[1] = phInputBuffer[0].pAddr[1];
    inputFrame.stride = phInputBuffer[0].planeStride;
    inputFrame.scanline = phInputBuffer[0].sliceheight;
    inputFrame.numberOfPlanes = phOutputBuffer[0].numberOfPlanes;

    outputBokehFrame.format = phOutputBuffer[0].format.format;
    outputBokehFrame.width = phOutputBuffer[0].format.width;
    outputBokehFrame.height = phOutputBuffer[0].format.height;
    outputBokehFrame.plane[0] = phOutputBuffer[0].pAddr[0];
    outputBokehFrame.plane[1] = phOutputBuffer[0].pAddr[1];
    outputBokehFrame.stride = phOutputBuffer[0].planeStride;
    outputBokehFrame.scanline = phOutputBuffer[0].sliceheight;
    outputBokehFrame.numberOfPlanes = phOutputBuffer[0].numberOfPlanes;

    // prot1 ->depth frame
    outputDepthFrame.format = CAM_FORMAT_Y16; // CAM_FORMAT_Y16
    outputDepthFrame.width = phOutputBuffer[0].format.width;
    outputDepthFrame.height = phOutputBuffer[0].format.height;
    outputDepthFrame.stride = phOutputBuffer[0].planeStride;
    outputDepthFrame.scanline = phOutputBuffer[0].sliceheight;
    outputDepthFrame.plane[0] =
        (uint8_t *)malloc(outputDepthFrame.stride * outputDepthFrame.scanline);
    outputDepthFrame.numberOfPlanes = 1;

    MLOGI(Mia2LogGroupPlugin,
          "%s:%d input width: %d, height: %d, stride: %d, scanline: %d, format: %d", __func__,
          __LINE__, inputFrame.width, inputFrame.height, inputFrame.stride, inputFrame.scanline,
          inputFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "%s:%d output bokeh width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          __func__, __LINE__, outputBokehFrame.width, outputBokehFrame.height,
          outputBokehFrame.stride, outputBokehFrame.scanline, outputBokehFrame.format);
    MLOGI(Mia2LogGroupPlugin,
          "%s:%d output depth width: %d, height: %d, stride: %d, scanline: %d, format: %d",
          __func__, __LINE__, outputDepthFrame.width, outputDepthFrame.height,
          outputDepthFrame.stride, outputDepthFrame.scanline, outputDepthFrame.format);
    if (iIsDumpData) {
        char inbuf[128];
        snprintf(inbuf, sizeof(inbuf), "mibokeh_input_%dx%d", inputFrame.width, inputFrame.height);
        PluginUtils::dumpToFile(inbuf, &inputFrame);
    }

    int ret = processBuffer(&inputFrame, &outputBokehFrame, &outputDepthFrame, pMeta);
    if (ret) {
        MLOGE(Mia2LogGroupPlugin, "[CAM_LOG_SYSTEM] MiBokeh processBuffer fail %d", ret);
    }
    free(outputDepthFrame.plane[0]);

    if (iIsDumpData) {
        char outputbokehbuf[128];
        snprintf(outputbokehbuf, sizeof(outputbokehbuf), "mibokeh_output_bokeh_%dx%d",
                 outputBokehFrame.width, outputBokehFrame.height);
        PluginUtils::dumpToFile(outputbokehbuf, &outputBokehFrame);
    }
}

ProcessRetStatus Mibokehplugin::processRequest(ProcessRequestInfoV2 *processRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int Mibokehplugin::processBuffer(MiImageBuffer *input, MiImageBuffer *output_bokeh,
                                 MiImageBuffer *output_depth, camera_metadata_t *pMetaData)
{
    int result = MIA_RETURN_OK;
    if (pMetaData == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s error pMetaData is null", __func__);
        return MIA_RETURN_INVALID_PARAM;
    }

    // get flip
    void *pData = NULL;
    const char *Flip = "xiaomi.flip.enabled";
    VendorMetadataParser::getVTagValue(pMetaData, Flip, &pData);
    if (NULL != pData) {
        // m_currentFlip = *static_cast<uint32_t *>(pData); //Fix me
        MLOGI(Mia2LogGroupPlugin, "Mibokehplugin getMetaData m_currentFlip  %d", m_currentFlip);
    }

    // get BokehFNumber
    pData = NULL;
    char fNumber[8] = {0};
    const char *BokehFNumber = "xiaomi.bokeh.fNumberApplied";
    VendorMetadataParser::getVTagValue(pMetaData, BokehFNumber, &pData);
    if (NULL != pData) {
        memcpy(fNumber, static_cast<char *>(pData), sizeof(fNumber));
        m_fNumberApplied = atof(fNumber);
        MLOGI(Mia2LogGroupPlugin, "Mibokehplugin getMetaData m_fNumberApplied  %f",
              m_fNumberApplied);
    }

    // get ai sceneDetected
    pData = NULL;
    const char *AiAsdDetected = "xiaomi.ai.asd.sceneDetected";
    VendorMetadataParser::getVTagValue(pMetaData, AiAsdDetected, &pData);
    if (NULL != pData) {
        m_sceneFlag = *static_cast<int32_t *>(pData);
        MLOGI(Mia2LogGroupPlugin, "Mibokehplugin getMetaData m_sceneFlag  %d", m_sceneFlag);
    }

    // get ORIENTATION
    pData = NULL;
    VendorMetadataParser::getTagValue(pMetaData, ANDROID_JPEG_ORIENTATION, &pData);
    if (NULL != pData) {
        int orientation = *static_cast<int32_t *>(pData);
        m_orientation = (orientation) % 360;
        MLOGI(Mia2LogGroupPlugin, "MiaNodeMiBokehn getMetaData jpeg orientation  %d",
              m_orientation);
    }
    switch (m_orientation) {
    case 0:
        m_orient = ARC_PL_OPF_0;
        break;
    case 90:
        m_orient = ARC_PL_OPF_90;
        break;
    case 270:
        m_orient = ARC_PL_OPF_270;
        break;
    case 180:
        m_orient = ARC_PL_OPF_180;
        break;
    default:
        m_orient = ARC_PL_OPF_0;
        break;
    }

    int32_t light_mode = 0;
    pData = NULL;
    const char *BokehLight = "xiaomi.portrait.lighting";
    VendorMetadataParser::getVTagValue(pMetaData, BokehLight, &pData);
    if (NULL != pData) {
        light_mode = *static_cast<int32_t *>(pData);
        MLOGD(Mia2LogGroupPlugin, "MiaNodeMiBokehn getMetaData light_mode  %d", light_mode);
    }

    switch (light_mode) {
    case 9:
        m_lightingMode = MI_PL_MODE_NEON_SHADOW;
        break;
    case 10:
        m_lightingMode = MI_PL_MODE_PHANTOM;
        break;
    case 11:
        m_lightingMode = MI_PL_MODE_NOSTALGIA;
        break;
    case 12:
        m_lightingMode = MI_PL_MODE_RAINBOW;
        break;
    case 13:
        m_lightingMode = MI_PL_MODE_WANING;
        break;
    case 14:
        m_lightingMode = MI_PL_MODE_DAZZLE;
        break;
    case 15:
        m_lightingMode = MI_PL_MODE_GORGEOUS;
        break;
    case 16:
        m_lightingMode = MI_PL_MODE_PURPLES;
        break;
    case 17:
        m_lightingMode = MI_PL_MODE_DREAM;
        break;
    default:
        m_lightingMode = 0;
        break;
    }

    MLOGI(Mia2LogGroupPlugin,
          "Get m_orient %d m_orientation %d light_mode %d m_lightingMode %d m_pNodeInstance: %p",
          m_orient, m_orientation, light_mode, m_lightingMode, m_pNodeInstance);

    checkPersist();

    // get timestamp current frame
    int64_t timestamp = gettime();

    MUInt8 *depthOutput = output_depth->plane[0];
    if (depthOutput != NULL) {
        memset(depthOutput, 0x0F, output_depth->stride * output_depth->scanline);
    }
    if (NULL != m_pNodeInstance) {
        m_depthSize = m_pNodeInstance->process(input, output_bokeh, depthOutput, 0, m_orientation,
                                               timestamp, 0, m_fNumberApplied, m_sceneFlag);
    }

    /*begin for relight*/
    if (m_lightingMode > 0) {
        miProcessRelightingEffect(input, output_bokeh);
    }

    return result;
}

void Mibokehplugin::prepareImage(MiImageBuffer *image, ASVLOFFSCREEN &stImage)
{
    MLOGD(Mia2LogGroupPlugin,
          "PrepareImage width: %d, height: %d, planeStride: %d, sliceHeight:%d, plans: %d",
          image->width, image->height, image->stride, image->scanline, image->numberOfPlanes);
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

    for (uint32_t i = 0; i < image->numberOfPlanes; i++) {
        stImage.ppu8Plane[i] = image->plane[i];
        stImage.pi32Pitch[i] = image->stride;
    }
    MLOGD(Mia2LogGroupPlugin, "X");
}

void Mibokehplugin::allocCopyImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage)
{
    memcpy(pDstImage, pSrcImage, sizeof(ASVLOFFSCREEN));
    pDstImage->ppu8Plane[0] =
        (unsigned char *)malloc(pDstImage->i32Width * pDstImage->pi32Pitch[0] * 3 / 2);
    if (pDstImage->ppu8Plane[0]) {
        pDstImage->ppu8Plane[1] =
            pDstImage->ppu8Plane[0] + pDstImage->i32Width * pDstImage->pi32Pitch[0];
        copyImage(pSrcImage, pDstImage);
    }
}

void Mibokehplugin::releaseImage(ASVLOFFSCREEN *pImage)
{
    if (pImage->ppu8Plane[0])
        free(pImage->ppu8Plane[0]);
    pImage->ppu8Plane[0] = NULL;
}

void Mibokehplugin::dumpYUVData(ASVLOFFSCREEN *pASLIn, uint32_t index, const char *namePrefix)
{
    MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] %s:%d: (IN)", __func__, __LINE__);

    // long long current_time = gettime();
    char filename[256];
    memset(filename, 0, sizeof(char) * 256);
    snprintf(filename, sizeof(filename), "%s%s_%dx%d_%d.yuv", PluginUtils::getDumpPath(),
             namePrefix, pASLIn->pi32Pitch[0], pASLIn->i32Height, index);

    MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] try to open file : %s", filename);

    int file_fd = open(filename, O_RDWR | O_CREAT, 0777);
    if (file_fd) {
        MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] : DumpYUVData open success");
        uint32_t bytes_write;
        bytes_write =
            write(file_fd, pASLIn->ppu8Plane[0], pASLIn->pi32Pitch[0] * pASLIn->i32Height);
        MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB]  plane[0] bytes_read: %u", bytes_write);
        bytes_write =
            write(file_fd, pASLIn->ppu8Plane[1], pASLIn->pi32Pitch[0] * (pASLIn->i32Height >> 1));
        MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB]  plane[1] bytes_read: %u", bytes_write);
        close(file_fd);
    } else {
        MLOGE(Mia2LogGroupPlugin, "[MIBOKEH_CTB] fileopen  error: %d", errno);
    }

    MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] %s:%d: (OUT)", __func__, __LINE__);
}

void Mibokehplugin::dumpRawData(uint8_t *pData, int32_t size, uint32_t index,
                                const char *namePrefix)
{
    MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] %s:%d: (IN)", __func__, __LINE__);

    // long long current_time = gettime();
    char filename[256];
    memset(filename, 0, sizeof(char) * 256);
    snprintf(filename, sizeof(filename), "%s%s_%d_%d.pdata", PluginUtils::getDumpPath(), namePrefix,
             size, index);

    MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] try to open file : %s", filename);

    int file_fd = open(filename, O_RDWR | O_CREAT, 0777);
    if (file_fd) {
        MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] : DumpYUVData open success");
        uint32_t bytes_write;
        bytes_write = write(file_fd, pData, size);
        MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB]  pData bytes_read: %d", size);
        close(file_fd);
    } else {
        MLOGE(Mia2LogGroupPlugin, "[MIBOKEH_CTB] fileopen  error: %d", errno);
    }

    MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] %s:%d: (OUT)", __func__, __LINE__);
}

void Mibokehplugin::copyImage(ASVLOFFSCREEN *pSrcImg, ASVLOFFSCREEN *pDstImg)
{
    if (pSrcImg == NULL || pDstImg == NULL)
        return;

    int32_t i;
    uint8_t *l_pSrc = NULL;
    uint8_t *l_pDst = NULL;

    l_pSrc = pSrcImg->ppu8Plane[0];
    l_pDst = pDstImg->ppu8Plane[0];
    for (i = 0; i < pDstImg->i32Height; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->i32Width);
        l_pDst += pDstImg->pi32Pitch[0];
        l_pSrc += pSrcImg->pi32Pitch[0];
    }

    l_pSrc = pSrcImg->ppu8Plane[1];
    l_pDst = pDstImg->ppu8Plane[1];
    for (i = 0; i < pDstImg->i32Height / 2; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->i32Width);
        l_pDst += pDstImg->pi32Pitch[1];
        l_pSrc += pSrcImg->pi32Pitch[1];
    }
}

int Mibokehplugin::processRelighting(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output,
                                     ASVLOFFSCREEN *bokehImg)
{
    int nRet = MIA_RETURN_INVALID_PARAM;
    // double t1,t2;
    ARC_PL_LIGHT_PARAM LightParam = {0};
    LightParam.i32LightMode = m_lightingMode;

    m_useBokeh = false;
    if (m_hRelightingHandle) {
        ARC_PL_DEPTHINFO lDepthInfo;
        ARC_PL_DEPTHINFO *plDepthInfo = &lDepthInfo;
        lDepthInfo.lDispMapSize = m_depthSize;
        lDepthInfo.pDispMap = m_pDepthMap;
        lDepthInfo.pBokehImg = bokehImg;
        if (lDepthInfo.lDispMapSize == 0)
            plDepthInfo = NULL;
        MLOGD(Mia2LogGroupPlugin,
              "MIBOKEH_CTB: ARC_PLI_Process i32LightMode %d lDispMapSize %d pDispMap %p\n",
              LightParam.i32LightMode, lDepthInfo.lDispMapSize, lDepthInfo.pDispMap);
        // t1 = gettime();
        nRet = ARC_PLI_Process(m_hRelightingHandle, plDepthInfo, input, &LightParam, output);
        // t2 = gettime();
        if (nRet != MERR_NONE) {
            MLOGD(Mia2LogGroupPlugin, "MIBOKEH_CTB: ARC_PLI_Process =failed= %d\n", nRet);
            m_useBokeh = true;
            return nRet;
        }
        MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB]:ARC_PLI_Process= %d: mode=%d\n", nRet,
              m_lightingMode);
    }
    return nRet;
}

int Mibokehplugin::miPrepareRelightImage(MiImageBuffer *src, MIIMAGEBUFFER *dst)
{
    if ((src == NULL) || (dst == NULL)) {
        MLOGE(Mia2LogGroupPlugin, "invalid param: src=%p, dst=%p!", src, dst);
        return MIA_RETURN_INVALID_PARAM;
    }

    if (src->format != CAM_FORMAT_YUV_420_NV12 && src->format != CAM_FORMAT_YUV_420_NV21) {
        MLOGE(Mia2LogGroupPlugin, "format[%d] is not supported!", src->format);
        return MIA_RETURN_INVALID_PARAM;
    }

    if (src->format == CAM_FORMAT_YUV_420_NV12) {
        dst->u32PixelArrayFormat = FORMAT_YUV_420_NV12;
        MLOGD(Mia2LogGroupPlugin, "FORMAT_YUV_420_NV12");
    } else if (src->format == CAM_FORMAT_YUV_420_NV21) {
        dst->u32PixelArrayFormat = FORMAT_YUV_420_NV21;
        MLOGD(Mia2LogGroupPlugin, "FORMAT_YUV_420_NV21");
    }

    dst->i32Width = src->width;
    dst->i32Height = src->height;
    dst->i32Scanline = src->height; // why not scanline
    dst->pi32Pitch[0] = src->stride;
    dst->pi32Pitch[1] = src->stride;

    dst->ppu8Plane[0] = (MUInt8 *)malloc(dst->i32Height * dst->pi32Pitch[0] * 3 / 2);
    if (dst->ppu8Plane[0]) {
        dst->ppu8Plane[1] = dst->ppu8Plane[0] + dst->i32Height * dst->pi32Pitch[0];
    } else {
        MLOGE(Mia2LogGroupPlugin, "out of memory");
        return MIA_RETURN_NO_MEM;
    }

    memcpy(dst->ppu8Plane[0], src->plane[0], dst->i32Height * dst->pi32Pitch[0]);
    memcpy(dst->ppu8Plane[1], src->plane[1], dst->i32Height / 2 * dst->pi32Pitch[0]);

    return MIA_RETURN_OK;
}

void Mibokehplugin::miReleaseRelightImage(MIIMAGEBUFFER *img)
{
    if (img && img->ppu8Plane[0])
        free(img->ppu8Plane[0]);
    img->ppu8Plane[0] = NULL;
}

int Mibokehplugin::miCopyRelightToMiImage(MIIMAGEBUFFER *src, MiImageBuffer *dst)
{
    int32_t i;
    uint8_t *srcTemp = NULL;
    uint8_t *dstTemp = NULL;

    if (src == NULL || dst == NULL) {
        MLOGE(Mia2LogGroupPlugin, "invalid param: src=%p, dst=%p!", src, dst);
        return MIA_RETURN_INVALID_PARAM;
    }

    dstTemp = dst->plane[0];
    srcTemp = (uint8_t *)src->ppu8Plane[0];
    for (i = 0; i < dst->height; i++) {
        memcpy(dstTemp, srcTemp, dst->width);
        dstTemp += dst->stride;
        srcTemp += src->pi32Pitch[0];
    }

    dstTemp = dst->plane[1];
    for (i = 0; i < dst->height / 2; i++) {
        memcpy(dstTemp, srcTemp, dst->width);
        dstTemp += dst->stride;
        srcTemp += src->pi32Pitch[1];
    }

    return MIA_RETURN_OK;
}

int Mibokehplugin::miProcessRelightingEffect(MiImageBuffer *input, MiImageBuffer *output)
{
    int res = MOK;
    double t0, t1, t2, t3;

    t2 = gettime();

    // use for AILAB
    MIIMAGEBUFFER srcImg = {0};
    MIIMAGEBUFFER dstImg = {0};
    MIIMAGEBUFFER bokehImg = {0};
    miPrepareRelightImage(input, &srcImg);
    miPrepareRelightImage(output, &dstImg);
    miPrepareRelightImage(output, &bokehImg);

    if (m_bDumpYUV) {
        char fileName[256];
        memset(fileName, 0, sizeof(char) * 256);
        snprintf(fileName, sizeof(fileName), "IN_Relight_SrcImg_Frame_%d", m_processedFrame);
        PluginUtils::dumpToFile(fileName, input);

        memset(fileName, 0, sizeof(char) * 256);
        snprintf(fileName, sizeof(fileName), "IN_Relight_BokehImg_Frame_%d", m_processedFrame);
        PluginUtils::dumpToFile(fileName, output);
    }

    // init
    if (m_hRelightingHandle == NULL) {
        MInt32 width = srcImg.i32Width;
        MInt32 height = srcImg.i32Height;

        t0 = gettime();
        res = AILAB_PLI_Init(&m_hRelightingHandle, MI_PL_INIT_MODE_PERFORMANCE, width, height);
        if (MOK != res) {
            MLOGE(Mia2LogGroupPlugin, "[Relight] AILAB_PLI_Init failed = %d", res);
        }
        t1 = gettime();

        MLOGD(
            Mia2LogGroupPlugin,
            "[Relight] AILAB_PLI_Init = %d: width=%d, height=%d, m_hRelightingHandle=%p, cost %.2f "
            "ms",
            res, width, height, m_hRelightingHandle, t1 - t0);
    } else {
        MLOGE(Mia2LogGroupPlugin, "[Relight] AILAB_PLI_Init m_hRelightingHandle %p",
              m_hRelightingHandle);
    }

    // preprocess
    if (res == MOK && m_hRelightingHandle) {
        MIPLLIGHTREGION mLightRegion = {0};

        t0 = gettime();
        res = AILAB_PLI_PreProcess(m_hRelightingHandle, &srcImg, m_orientation, &mLightRegion);
        if (MOK != res) {
            MLOGE(Mia2LogGroupPlugin, "[Relight] AILAB_PLI_PreProcess failed = %d\n", res);
            AILAB_PLI_Uninit(&m_hRelightingHandle);
            m_hRelightingHandle = NULL;
        }
        t1 = gettime();

        MLOGD(Mia2LogGroupPlugin,
              "[Relight] AILAB_PLI_PreProcess = %d: orientation=%d, cost %.2f ms", res,
              m_orientation, t1 - t0);
    } else {
        MLOGE(Mia2LogGroupPlugin, "[Relight] relight process failed. m_hRelightingHandle is %p!",
              m_hRelightingHandle);
        res = MFAILED;
    }

    // process
    if (res == MOK && m_hRelightingHandle) {
        // LightParam
        MIPLLIGHTPARAM LightParam = {0};
        LightParam.i32LightMode = m_lightingMode;

        MIPLDEPTHINFO lDepthInfo = {0};
        lDepthInfo.i32MaskWidth = 0;
        lDepthInfo.i32MaskHeight = 0;
        lDepthInfo.pMaskImage = NULL;
        lDepthInfo.pBlurImage = &bokehImg;

        t0 = gettime();
        res = AILAB_PLI_Process(m_hRelightingHandle, &lDepthInfo, &srcImg, &LightParam, &dstImg);
        if (MOK != res) {
            MLOGE(Mia2LogGroupPlugin, "[Relight] AILAB_PLI_Process =failed= %d\n", res);
        }
        t1 = gettime();

        AILAB_PLI_Uninit(&m_hRelightingHandle);
        m_hRelightingHandle = NULL;

        MLOGI(Mia2LogGroupPlugin,
              "[Relight] relight process end. AILAB_PLI_Process = %d: lightingMode=%d, costime "
              "%.2f ms",
              res, m_lightingMode, t1 - t0);
    } else {
        MLOGE(Mia2LogGroupPlugin, "[Relight] relight process failed. m_hRelightingHandle is %p!",
              m_hRelightingHandle);
        res = MFAILED;
    }

    if (res == MOK) {
        miCopyRelightToMiImage(&dstImg, output);
    } else {
        miCopyRelightToMiImage(&bokehImg, output);
    }

    if (m_bDumpYUV) {
        char fileName[256];
        memset(fileName, 0, sizeof(char) * 256);
        snprintf(fileName, sizeof(fileName), "OUT_Relight_SrcImg_Frame_%d", m_processedFrame);
        PluginUtils::dumpToFile(fileName, output);
    }

    miReleaseRelightImage(&srcImg);
    miReleaseRelightImage(&dstImg);
    miReleaseRelightImage(&bokehImg);

    t3 = gettime();
    MLOGI(Mia2LogGroupPlugin, "[Relight] Mi Relighting Effect res = %d, total costime=%.2f\n", res,
          t3 - t2);

    return res;
}

void Mibokehplugin::checkPersist()
{
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.xiaomi.relight.dumpimg", prop, "0");
    m_bDumpYUV = (bool)atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.xiaomi.relight.dumppriv", prop, "0");
    m_bDumpPriv = (bool)atoi(prop);

    MLOGD(Mia2LogGroupPlugin, "[MIBOKEH_CTB] %s:%d: m_bDumpPriv %d (OUT)", __func__, __LINE__,
          m_bDumpPriv);
}

int Mibokehplugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

} // namespace mialgo2
