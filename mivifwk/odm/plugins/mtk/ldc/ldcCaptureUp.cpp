// #include "miautil.h"
// #include "miapluginwraper.h"
#include "ldcCaptureUp.h"

#include "MiaPluginUtils.h"
#include "device.h"
#include "nodeMetaDataUp.h"
// #include <fstream>
namespace mialgo2 {

#undef LOG_TAG
#define LOG_TAG "LDC-CAPTURE-UP"

#define LDC_TUNING_DATA_FILE_NAME            "null"
#define LDC_TUNING_DATA_AI_FILE_NAME         "null"
#define LDC_PACKDATA_VENDORID_0x01_FILE_NAME "/vendor/etc/camera/ldc/intsense_config_undistort.bin"
#define LDC_PACKDATA_VENDORID_0x03_FILE_NAME "/vendor/etc/camera/ldc/intsense_config_undistort.bin"
#define LDC_PACKDATA_VENDORID_0x07_FILE_NAME "/vendor/etc/camera/ldc/intsense_config_undistort.bin"
#define LDC_MODEL_VENDORID_PATH_NAME         "/vendor/etc/camera/ldc/"
#define DUMP_FILE_PATH                       "/data/vendor/camera_dump"

// static U32 ldcLibInit_f = FALSE;

// static std::mutex gMutex;
// static S64 m_mutexId = 0;
// static S64 m_detectorRunId = 0;
// static S64 m_detectorInitId = 0;
// static S64 m_detectorMaskId = 0;
// static U32 m_format = 0xFF;
// static U32 m_Width = 0;
// static U32 m_Height = 0;
// static U32 m_StrideW = 0;
// static U32 m_StrideH = 0;
// static U32 m_sCurMode = 0;

LdcCapture::LdcCapture()
{ // @suppress("Class members should be properly initialized")

    m_debug = property_get_int32("persist.vendor.camera.LDC.debug", 0);
    m_debugFaceDrawGet = property_get_bool("persist.ldc.face.draw.get", 0);
    m_inited = FALSE;
    m_algo = NULL;
    DEV_LOGI("LdcCapture() ldcLibInit_f = %x m_inited = %x", ldcLibInit_f, m_inited);

    if (m_debug == 1) {
        // if (m_mutexId == 0) {
        //    m_mutexId = Device->mutex->Create();
        //}
        if (m_detectorRunId == 0) {
            m_detectorRunId = Device->detector->Create();
        }
        if (m_detectorInitId == 0) {
            m_detectorInitId = Device->detector->Create();
        }
        if (m_detectorMaskId == 0) {
            m_detectorMaskId = Device->detector->Create();
        }
    }
}

LdcCapture::~LdcCapture()
{
    int err = 0;
    // Device->mutex->Lock(m_mutexId);
    { // lock(gMutex) lock protection start
        std::unique_lock<std::mutex> lock(gMutex);
        DEV_LOGI("~LdcCapture() ldcLibInit_f = %x m_inited = %x", ldcLibInit_f, m_inited);
        if (m_inited == TRUE) {
            m_Width = 0;
            m_Height = 0;
            m_StrideW = 0;
            m_StrideH = 0;

            if (m_sCurMode == LDC_CAPTURE_MODE_ALTEK_DEF) {
                if (m_algo != NULL) {
                    delete m_algo;
                    m_algo = NULL;
                    DEV_LOGI("LdcCapture::InitLib(): delete m_algo");
                }
            }
            m_format = 0xFF;
            ldcLibInit_f = FALSE;
            m_inited = FALSE;
        }
    } // lock(gMutex) lock protection finish
    // Device->mutex->Unlock(m_mutexId);
    if (m_debug == 1) {
        Device->detector->Destroy(&m_detectorRunId);
        Device->detector->Destroy(&m_detectorInitId);
        Device->detector->Destroy(&m_detectorMaskId);
        // Device->mutex->Destroy(&m_mutexId);
    }
    m_mode = 0;
    m_debug = 0;

    DEV_LOGI("~LdcCapture() destory m_algo: err:%#x.", err);
}

S64 LdcCapture::GetPackData(LDC_CAPTURE_MODE mode, void **ppData0, U64 *pSize0, void **ppData1,
                            U64 *pSize1)
{
    S64 ret = RET_OK;
    S64 packDataSize = 0;
    void *packDataBuf = NULL;
    void *tuningBuf = NULL;
    S64 tuningSize = 0;
    char packDataFileName[128] = LDC_PACKDATA_VENDORID_0x01_FILE_NAME;
    char tuningDataFileName[128] = LDC_TUNING_DATA_FILE_NAME;
    if ((m_mode == LDC_CAPTURE_MODE_ALTEK_DEF)) {
        DEV_IF_LOGE_RETURN_RET((NULL == pSize0), RET_ERR_ARG, "pSize1 is null");
        DEV_IF_LOGE_RETURN_RET((NULL == pSize1), RET_ERR_ARG, "pSize1 is null");
        if (m_mode == LDC_CAPTURE_MODE_ALTEK_AI) {
            sprintf(tuningDataFileName, "%s", LDC_TUNING_DATA_AI_FILE_NAME);
        }
        if (strncmp(tuningDataFileName, "null", strlen("null"))) {
            DEV_LOGI("LdcCapture::GetPackData() start get tuningData...");
            tuningSize = Device->file->GetSize(tuningDataFileName);
            DEV_IF_LOGE_RETURN_RET((0 == tuningSize), RET_ERR, "get file err");
            tuningBuf = malloc(tuningSize);
            DEV_IF_LOGE_RETURN_RET((NULL == tuningBuf), RET_ERR, "malloc err");
            ret = Device->file->ReadOnce(tuningDataFileName, tuningBuf, tuningSize, 0);
            DEV_IF_LOGE_RETURN_RET((ret != tuningSize), RET_ERR, "read file err");
            *pSize1 = tuningSize;
            *ppData1 = tuningBuf;
        }
        if (m_vendorId == 0x03) {
            sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x03_FILE_NAME);
        } else if (m_vendorId == 0x07) {
            sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x07_FILE_NAME);
        } else {
            sprintf(packDataFileName, "%s", LDC_PACKDATA_VENDORID_0x01_FILE_NAME);
        }
        if (strncmp(packDataFileName, "null", strlen("null"))) {
            DEV_LOGI("LdcCapture::GetPackData() start get packData...");
            packDataSize = Device->file->GetSize(packDataFileName);
            DEV_IF_LOGE_RETURN_RET((0 == packDataSize), RET_ERR, "get file err");
            packDataBuf = malloc(packDataSize);
            DEV_IF_LOGE_RETURN_RET((NULL == packDataBuf), RET_ERR, "malloc err");
            ret = Device->file->ReadOnce(packDataFileName, packDataBuf, packDataSize, 0);
            DEV_IF_LOGE_RETURN_RET((ret != packDataSize), RET_ERR, "read file err %" PRIi64, ret);
            DEV_IF_LOGI((ret == packDataSize), "LDC PACK FILE[%s]", packDataFileName);
            *pSize0 = packDataSize;
            *ppData0 = packDataBuf;
        }
    }
    return RET_OK;
}

S64 LdcCapture::FreePackData(void **ppData0, void **ppData1)
{
    if (ppData0 != NULL) {
        if (*ppData0) {
            free(*ppData0);
            *ppData0 = NULL;
        }
    }
    if (ppData1 != NULL) {
        if (*ppData1) {
            free(*ppData1);
            *ppData1 = NULL;
        }
    }
    return RET_OK;
}

// Should hold mutex m_mutexId before calling this func
S64 LdcCapture::InitLib(U32 inWidth, U32 inHeight, U32 inStrideW, U32 inStrideH, U32 outWidth,
                        U32 outHeight, U32 outStrideW, U32 outStrideH, U32 format)
{
    S64 ret = 0;
    const char *version = NULL;
    // DEV_IF_LOGE_RETURN_RET((m_algo == NULL), RET_ERR, "LdcCapture InitLib ERR m_algo is NULL");

    if ((m_debug & LDC_CAPTURE_DEBUG_INFO) != 0) {
        Device->detector->Begin(m_detectorInitId);
    }
    DEV_LOGI("LdcCapture::InitLib(): m_inited = %d", m_inited);

    if (m_inited == TRUE) {
        if (m_sCurMode == LDC_CAPTURE_MODE_ALTEK_DEF) {
            if (m_algo != NULL) {
                delete m_algo;
                m_algo = NULL;
                DEV_LOGI("LdcCapture::InitLib(): delete m_algo");
            }
        }
        m_format = 0xFF;
        // DEV_IF_LOGE((ret != RET_OK), " alhLDC_Deinit err");

        ldcLibInit_f = FALSE;
        m_inited = FALSE;
    }
    if (m_mode == LDC_CAPTURE_MODE_ALTEK_DEF) {
        void *packDataBuf = NULL;
        U64 packDataSize = 0;
        void *tuningBuf = NULL;
        U64 tuningSize = 0;
        WUndistortConfig undistort_config;
        const char *model_path = LDC_MODEL_VENDORID_PATH_NAME;

        if (m_algo == NULL) {
            m_algo = new WideLensUndistort();
        }

        DEV_IF_LOGE_RETURN_RET((m_algo == NULL), RET_ERR,
                               "Error, cannot create new wa::WideLensUndistort()");
        m_inited = TRUE;
        ret |= GetPackData(LDC_CAPTURE_MODE_ALTEK_DEF, &packDataBuf, &packDataSize, &tuningBuf,
                           &tuningSize);

        DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "LdcCapture InitLib ERR GetPackData ");

        // version = m_algo->getApiVersion();
        version = m_algo->getVersion();
        if (version != NULL) {
            DEV_LOGI("LdcCapture::init(): ldc capture algo version:%s", version);
        } else {
            DEV_LOGE("LdcCapture::init(): Error, can't get ldc capture algo version.");
        }

        undistort_config.mecpBlob.data = packDataBuf;
        undistort_config.mecpBlob.length = packDataSize;

        m_algo->init(undistort_config);
        m_algo->setMode(wa::WideLensUndistort::MODE_CAPTURE);
        m_algo->setModel(model_path, NULL, 0);

        FreePackData(&packDataBuf, &tuningBuf);
    } else {
        DEV_LOGE("LDC Capture MODE err %d", m_mode);
        ret = RET_ERR;
    }
    ldcLibInit_f = TRUE;

    m_format = format;
    if ((m_debug & LDC_CAPTURE_DEBUG_INFO) != 0) {
        Device->detector->End(m_detectorInitId, "LDC-CAPTURE_INIT");
    }

    //    if ((m_debug & LDC_CAPTURE_DEBUG_INFO) != 0) {
    //        if (m_mode == LDC_CAPTURE_MODE_ALTEK_DEF) {
    //            alhLDC_Set_LogLevel(ALHLDC_LEVEL_2);
    //        }
    //    }
    m_Width = inWidth;
    m_Height = inHeight;
    m_StrideW = inStrideW;
    m_StrideH = inStrideH;
    m_sCurMode = m_mode;
    DEV_LOGI(
        "LDC Capture INIT0 [%" PRIi64
        "] (Ver:%s) mode=%d inW=%d inH=%d (SW=%d SH=%d) outW=%d outH=%d (SW=%d SH=%d) format=%d",
        ret, (version != NULL) ? version : "NULL", m_mode, inWidth, inHeight, inStrideW, inStrideH,
        outWidth, outHeight, outStrideW, outStrideH, m_format);
    return ret;
}

S64 LdcCapture::Init(U32 mode, U32 debug, FP32 ratio, MIRECT *sensorSize, float viewAngle,
                     int vendorId)
{
    m_mode = mode;
    m_debug = debug;
    m_vendorId = vendorId;
    m_ratio = ratio;
    m_sensorSize.width = sensorSize->width;
    m_sensorSize.height = sensorSize->height;
    m_sensorSize.left = sensorSize->left;
    m_sensorSize.top = sensorSize->top;
    m_viewAngle = viewAngle;
    DEV_LOGI("LdcCapture::Init() sensor top = %d left = %d width = %d height = %d",
             m_sensorSize.top, m_sensorSize.left, m_sensorSize.width, m_sensorSize.height);
    return RET_OK;
}

void LdcCapture::drawMask(DEV_IMAGE_BUF *buffer, float fx, float fy, float fw, float fh)
{
    // sample: modify output buffer
    if (buffer) {
        char *ptr = (char *)buffer[0].plane[0];
        if (ptr) {
            char mask = 0;
            int lineWidth = 2;
            int w = buffer->width;
            int h = buffer->height;

            int stride = buffer->stride[0];
            int sliceHeight = buffer->sliceHeight[0];

            int y_from = fy * h;
            int y_to = (fy + fh) * h;
            int x = fx * w;
            int width = fw * w;
            int y_start, y_end;
            DEV_LOGI("LdcCapture::drawMask() width = %d height = %d stride = %d sliceHeight = %d",
                     w, h, stride, sliceHeight);
            y_to = std::max(0, std::min(y_to, h - 1));
            y_from = std::max(0, std::min(y_from, y_to));
            x = std::max(0, std::min(x, w - 1));
            width = std::max(0, std::min(width, w - 1));

            // top
            y_start = y_from;
            y_end = std::max(0, std::min(y_start + lineWidth, h - 1));

            for (int y = y_start; y < y_end; ++y) {
                memset(ptr + y * stride + x, mask, width);
            }

            // bot
            y_start = y_to;
            y_end = std::max(0, std::min(y_start + lineWidth, h - 1));

            for (int y = y_start; y < y_end; ++y) {
                memset(ptr + y * stride + x, mask, width);
            }

            // left
            y_start = y_from;
            y_end = y_to;
            int x_lw = std::min(x + lineWidth, w - 1) - x;
            for (int y = y_start; y < y_end; ++y) {
                memset(ptr + y * stride + x, mask, x_lw);
            }

            // right
            y_start = y_from;
            y_end = y_to;
            x = std::min(x + width, w - 1);
            x_lw = std::min(x + lineWidth, w - 1) - x;
            for (int y = y_start; y < y_end; ++y) {
                memset(ptr + y * stride + x, mask, x_lw);
            }
        }
    }
}

S64 LdcCapture::Process(DEV_IMAGE_BUF *input, DEV_IMAGE_BUF *output,
                        NODE_METADATA_FACE_INFO *pFaceInfo, MIRECT *cropRegion, uint32_t streamMode)
{
    int indexUltra = 0; // for one of the Multiple camera ultra sensores
    // DEV_IF_LOGE_RETURN_RET((m_algo == NULL), RET_ERR, "LdcCapture Init ERR m_algo is NULL");
    DEV_IF_LOGE_RETURN_RET((NULL == input), RET_ERR_ARG, "input is null");
    DEV_IF_LOGE_RETURN_RET((NULL == output), RET_ERR_ARG, "output is null");
    DEV_IF_LOGE_RETURN_RET((NULL == pFaceInfo), RET_ERR_ARG, "cropRegion is null");
    // Device->mutex->Lock(m_mutexId);    //lock to prevent re-entry, especially when deinit
    // triggered while another LDC is processing.
    S64 ret = 0;
    // lock to prevent re-entry, especially when deinit triggered while another LDC is processing or
    // other thread call the code. lock(gMutex) lock protection start
    std::unique_lock<std::mutex> lock(gMutex);
    DEV_LOGI("Process() ldcLibInit_f = %x m_inited = %x", ldcLibInit_f, m_inited);
    DEV_LOGI(
        "LDC-Info(capture#1): StreamMode:%d inW=%d inH=%d (SW=%d SH=%d) outW=%d outH=%d (SW=%d "
        "SH=%d) format=%d",
        streamMode, input->width, input->height, input->stride[0], input->sliceHeight[0],
        output->width, output->height, output->stride[0], output->sliceHeight[0], output->format);

    if ((m_Width != input->width) || (m_Height != input->height) ||
        (m_StrideW != input->stride[0]) || (m_StrideH != input->height) ||
        (m_format != input->format) || (m_sCurMode != m_mode)) {
        ret = InitLib(input->width, input->height, input->stride[0], input->sliceHeight[0],
                      output->width, output->height, output->stride[0], output->sliceHeight[0],
                      input->format);
        // DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "LdcCapture Init ERR");
        DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "LdcCapture Init ERR");
    }
    // DEV_IF_LOGE_RETURN_RET((ldcLibInit_f != TRUE), RET_ERR, "InitLib err");
    DEV_IF_LOGE_RETURN_RET((m_algo == NULL), RET_ERR, "LdcCapture Init ERR m_algo is NULL");

    LDC_RECT pa_ptInZoomWOI;
    WRect crop_rect;
    WSize oringe_size;
    wa::Image::ImageType fmt;
    const int kFaceNum = (pFaceInfo->num > 0) ? pFaceInfo->num : 0;
    WFace faces[kFaceNum + 1];
    // set faces
    if (kFaceNum) {
        for (int i = 0; i < kFaceNum; i++) {
            faces[i].rect.bottom = pFaceInfo->face[i].top + pFaceInfo->face[i].height;
            faces[i].rect.right = pFaceInfo->face[i].left + pFaceInfo->face[i].width;
            faces[i].rect.left = pFaceInfo->face[i].left;
            faces[i].rect.top = pFaceInfo->face[i].top;

            if (m_debugFaceDrawGet == true) {
                drawMask(input, (float)pFaceInfo->face[i].left / input->width,
                         (float)pFaceInfo->face[i].top / input->height,
                         (float)pFaceInfo->face[i].width / input->width,
                         (float)pFaceInfo->face[i].height / input->height);
            }
        }
        m_algo->setFaces(faces, kFaceNum);
    }

    if ((m_debug & LDC_CAPTURE_DEBUG_DUMP_IMAGE_CAPTURE) != 0) {
        char faceInfoBuf[1024 * 8] = {0};
        sprintf(faceInfoBuf + strlen(faceInfoBuf), "FACE NUM [%d]\n", pFaceInfo->num);
        sprintf(faceInfoBuf + strlen(faceInfoBuf), "-----------------------------------------\n");
        for (int i = 0; i < pFaceInfo->num; i++) {
            sprintf(faceInfoBuf + strlen(faceInfoBuf), "FACE [%d/%d] [x=%d,y=%d,w=%d,h=%d] S[%d]\n",
                    i + 1, pFaceInfo->num, pFaceInfo->face[i].left, pFaceInfo->face[i].top,
                    pFaceInfo->face[i].width, pFaceInfo->face[i].height, 100);
        }
        Device->image->DumpData(faceInfoBuf, strlen(faceInfoBuf), LOG_TAG, 0, "FaceInfo.in.txt");
    }

    // set crop
    TransformZoomWOI(pa_ptInZoomWOI, cropRegion, input);
    crop_rect.left = pa_ptInZoomWOI.x;
    crop_rect.top = pa_ptInZoomWOI.y;
    crop_rect.right = pa_ptInZoomWOI.x + pa_ptInZoomWOI.width;
    crop_rect.bottom = pa_ptInZoomWOI.y + pa_ptInZoomWOI.height;

    oringe_size.width = m_sensorSize.width;
    oringe_size.height = m_sensorSize.height;

    m_algo->setCropRect(oringe_size, crop_rect);

    // set input image
    fmt = transformFormat((DevImage_Format)(input->format));
    if (fmt == wa::Image::IMAGETYPE_MAX) {
        DEV_IF_LOGE_RETURN_RET((fmt == wa::Image::IMAGETYPE_MAX), RET_ERR,
                               "LdcCapture err input format = %d is not support ", input->format);
    }
    wa::Image input_image(input->stride[0], input->sliceHeight[0],
                          (unsigned char *)input[0].plane[0], (unsigned char *)input[0].plane[1],
                          nullptr, fmt, wa::Image::DT_8U, wa::Image::ROTATION_0);
    input_image.setImageWH(input->width, input->height);

    // DEV_LOGI("LdcPreview::process() inFd[0] = %d inFd[1] = %d", inFd[0], inFd[1]);
    // int* in_ion_fd_values = new int[2]{inFd[0], inFd[1]};
    // input_image.setIonFd(in_ion_fd_values, 2);

    // set camera params
    // get ultra camera sensor param
    const char *cam_param = cameraParams[indexUltra];
    input_image.setExifModuleInfo(const_cast<char *>(cam_param));

    // set output image
    fmt = transformFormat((DevImage_Format)(output->format));
    if (fmt == wa::Image::IMAGETYPE_MAX) {
        DEV_IF_LOGE_RETURN_RET((fmt == wa::Image::IMAGETYPE_MAX), RET_ERR,
                               "LdcCapture err output format = %d is not support ", output->format);
    }

    wa::Image output_image(output->stride[0], output->sliceHeight[0],
                           (unsigned char *)output[0].plane[0], (unsigned char *)output[0].plane[1],
                           nullptr, fmt, wa::Image::DT_8U, wa::Image::ROTATION_0);
    output_image.setImageWH(output->width, output->height);

    // DEV_LOGI("LdcPreview::process() outFd[0] = %d outFd[1] = %d", outFd[0], outFd[1]);
    // int* out_ion_fd_values = new int[2]{outFd[0], outFd[1]};
    // output_image.setIonFd(out_ion_fd_values, 2);

    if (m_mode == LDC_CAPTURE_MODE_ALTEK_DEF) {
        if ((m_debug & LDC_CAPTURE_DEBUG_INFO) != 0) {
            Device->detector->Begin(m_detectorRunId);
        }
        // set run
        m_algo->run(input_image, output_image);

        if ((m_debug & LDC_CAPTURE_DEBUG_INFO) != 0) {
            Device->detector->End(m_detectorRunId, "LDC-CAPTURE_RUN");
        }
    }
// delete[] in_ion_fd_values;
// delete[] out_ion_fd_values;
exit:
    DEV_LOGI("LDC-Info(capture#3) EXIT");
    DEV_IF_LOGE((ret != RET_OK), "LDC Capture RUN ERR");
    // Device->mutex->Unlock(m_mutexId);
    DEV_LOGI("Process() ldcLibInit_f = %x m_inited = %x", ldcLibInit_f, m_inited);
    // lock(gMutex) lock protection finish
    return ret;
}

void LdcCapture::TransformZoomWOI(LDC_RECT &zoomWOI, MIRECT *cropRegion, DEV_IMAGE_BUF *input)
{
    const float AspectRatioTolerance = 0.01f;
    const uint cropAlign = 0x2U;
    float scaleWidth = 0;
    float scaleHeight = 0;
    scaleWidth = (float)input->width / cropRegion->width;
    scaleHeight = (float)input->height / cropRegion->height;
    if (scaleHeight + AspectRatioTolerance < scaleWidth) {
        // Under the professional mode, need to do data conversion 16:9 or full size
        zoomWOI.height =
            (U32)((input->height * cropRegion->width) / input->width) & ~(cropAlign - 1);
        zoomWOI.y = cropRegion->top + ((cropRegion->height - zoomWOI.height) / 2);
        zoomWOI.x = cropRegion->left;
        zoomWOI.width = cropRegion->width;

    } else {
        // sat mode ,use sat Calculated data crop size
        zoomWOI.width = cropRegion->width;
        zoomWOI.height = cropRegion->height;
        zoomWOI.x = cropRegion->left;
        zoomWOI.y = cropRegion->top;
    }
    DEV_LOGI("LDC-Info(capture#2):Scale:%f,%f Crop(%d,%d,%d,%d) WOI(%d,%d,%d,%d)", scaleWidth,
             scaleHeight, cropRegion->left, cropRegion->top, cropRegion->width, cropRegion->height,
             zoomWOI.x, zoomWOI.y, zoomWOI.width, zoomWOI.height);
}

wa::Image::ImageType LdcCapture::transformFormat(DEV_IMAGE_FORMAT format)
{
    wa::Image::ImageType fmt;

    switch (format) {
    case DEV_IMAGE_FORMAT_YUV420NV21:
        fmt = wa::Image::IMAGETYPE_NV21;
        break;
    case DEV_IMAGE_FORMAT_YUV420NV12:
        fmt = wa::Image::IMAGETYPE_NV12;
        break;
    case DEV_IMAGE_FORMAT_YUV420P010:
        fmt = wa::Image::IMAGETYPE_P010;
        break;
    default:
        fmt = wa::Image::IMAGETYPE_MAX;
        DEV_LOGI("image format(%d) is not supported", format);
    }
    return fmt;
}

} // namespace mialgo2
