#include "miairawhdr.h"

#include <cutils/properties.h>
#include <stdio.h>
#include <utils/Log.h>

#include <map>

#include "amcomdef.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "xmi_low_light_hdr.h"

#define MAX_BUF_LEN PROPERTY_VALUE_MAX
#define MAX_FD_NUM  4
#undef LOG_TAG
#define LOG_TAG "MIAI_RAWHDR"

// set bypass, use ev0 raw buffer, default evlist is ev- ev- ev0 ev0 ev0 ev0
static const int32_t sIsBypassHdrAlgo =
    property_get_int32("persist.vendor.camera.capture.bypass.hdralgo", 0);

MHandle g_wideHdrExpEffect = MNull;
MHandle g_teleHdrExpEffect = MNull;
MHandle g_UWHdrExpEffect = MNull;
MHandle g_macroHdrExpEffect = MNull;
MHandle g_frontHdrExpEffect = MNull;
MHandle g_frontAuxHdrExpEffect = MNull;
MHandle g_rearSatWTHdrExpEffect = MNull;
MHandle g_rearSatWUHdrExpEffect = MNull;
MHandle g_frontSatHdrExpEffect = MNull;
MHandle g_rearSatTUTHdrExpEffect = MNull;
MHandle g_rearBokehWTHdrExpEffect = MNull;
MHandle g_rearBokehWUHdrExpEffect = MNull;
MHandle g_frontBokehHdrExpEffect = MNull;
MHandle g_rearSatUWWHdrExpEffect = MNull;

std::map<uint32_t, MHandle *> g_cameraHdrExpEffect = {
    {CAM_COMBMODE_REAR_WIDE, &g_wideHdrExpEffect},
    {CAM_COMBMODE_REAR_TELE, &g_teleHdrExpEffect},
    {CAM_COMBMODE_REAR_ULTRA, &g_UWHdrExpEffect},
    {CAM_COMBMODE_REAR_MACRO, &g_macroHdrExpEffect},
    {CAM_COMBMODE_FRONT, &g_frontHdrExpEffect},
    {CAM_COMBMODE_FRONT_AUX, &g_frontAuxHdrExpEffect},
    {CAM_COMBMODE_REAR_SAT_WT, &g_rearSatWTHdrExpEffect},
    {CAM_COMBMODE_REAR_SAT_WU, &g_rearSatWUHdrExpEffect},
    {CAM_COMBMODE_FRONT_SAT, &g_frontSatHdrExpEffect},
    {CAM_COMBMODE_REAR_SAT_T_UT, &g_rearSatTUTHdrExpEffect},
    {CAM_COMBMODE_REAR_BOKEH_WT, &g_rearBokehWTHdrExpEffect},
    {CAM_COMBMODE_REAR_BOKEH_WU, &g_rearBokehWUHdrExpEffect},
    {CAM_COMBMODE_FRONT_BOKEH, &g_frontBokehHdrExpEffect},
    {CAM_COMBMODE_REAR_SAT_UW_W, &g_rearSatUWWHdrExpEffect}};
uint32_t g_lastCameraId = CAM_COMBMODE_REAR_WIDE;
std::mutex g_mutex;

namespace mialgo2 {

MiAIRAWHDR::MiAIRAWHDR() : m_isCrop(false), m_flustStatus(false), mCameraId(0)
{
    memset(aecExposureData, 0, sizeof(aecExposureData));
    memset(awbGainParams, 0, sizeof(awbGainParams));
    memset(extraAecExposureData, 0, sizeof(extraAecExposureData));
    memset(&m_userData, 0, sizeof(m_userData));
}

MiAIRAWHDR::~MiAIRAWHDR()
{
    m_isCrop = false;
    m_flustStatus = false;
    memset(&m_userData, 0, sizeof(m_userData));
}

void MiAIRAWHDR::init(uint32_t CameraId)
{
    std::unique_lock<std::mutex> mtx_locker(g_mutex);
    memset(&m_userData, 0, sizeof(ARC_HDR_USERDATA));
    m_isCrop = false;
    m_flustStatus = false;
    mCameraId = CameraId;
    MLOGI(Mia2LogGroupPlugin, "[%s] mCameraId %d", LOG_TAG, mCameraId);

    if (MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_REAR_WIDE] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_REAR_TELE] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_REAR_ULTRA] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_REAR_MACRO] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_FRONT] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_FRONT_AUX] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_REAR_SAT_WT] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_REAR_SAT_WU] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_FRONT_SAT] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_REAR_SAT_T_UT] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_REAR_BOKEH_WT] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_REAR_BOKEH_WU] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_FRONT_BOKEH] &&
        MNull == *g_cameraHdrExpEffect[CAM_COMBMODE_REAR_SAT_UW_W]) {
        MLOGI(Mia2LogGroupPlugin, "[%s]: <camera id: %d> algo init begin", LOG_TAG, mCameraId);
        MRESULT ret = XMI_HDR_RAW_Init(g_cameraHdrExpEffect[mCameraId], CameraId);
        MLOGI(Mia2LogGroupPlugin, "[%s]: <camera id: %d> algo init result: %d", LOG_TAG, mCameraId,
              ret);
        g_lastCameraId = mCameraId;
        if (ret != MOK) {
            return;
        }
    } else {
        MLOGI(Mia2LogGroupPlugin, "[%s] <camera id: %d> algo has inited, needn't to do", LOG_TAG,
              mCameraId);
    }
}

void MiAIRAWHDR::destroy()
{
    std::unique_lock<std::mutex> mtx_locker(g_mutex);
    if (*g_cameraHdrExpEffect[mCameraId] != MNull) {
        MLOGI(Mia2LogGroupPlugin, "[%s]: <camera id: %d> algo uninit begin", LOG_TAG, mCameraId);
        int ret = XMI_HDR_RAW_Uninit(g_cameraHdrExpEffect[mCameraId]);
        MLOGI(Mia2LogGroupPlugin, "[%s]: <camera id: %d> algo uninit result: %d", LOG_TAG,
              mCameraId, ret);
        *g_cameraHdrExpEffect[mCameraId] = MNull;
        if (ret != MOK)
            MLOGE(Mia2LogGroupPlugin, "[%s]: algo uninit error...", LOG_TAG);
    } else {
        MLOGI(Mia2LogGroupPlugin,
              "[%s]: <camera id: %d> release rawhdr handle is empty, needn't to call unit func",
              LOG_TAG, mCameraId);
    }
}

MVoid MiAIRAWHDR::setRawOffScreen(HDR_RAW_OFFSCREEN *pImg, struct MiImageBuffer *miBuf,
                                  MIAI_HDR_RAWINFO &rawInfo)
{
    pImg->i32Width = miBuf->width;
    pImg->i32Height = miBuf->height;

    if (miBuf->format == CAM_FORMAT_RAW16) {
        pImg->u32PixelArrayFormat = getRawType(miBuf->format, rawInfo.raw_type);
        for (int i = 0; i < miBuf->numberOfPlanes; i++) {
            pImg->ppu8Plane[i] = miBuf->plane[i];
            pImg->fd[i] = miBuf->fd[i];
            pImg->pi32Pitch[i] = (int)miBuf->stride;
            MLOGD(Mia2LogGroupPlugin, "[%s] stImage.pi32Pitch[%d]:%d fd:%d numPlanes:%d", LOG_TAG,
                  i, pImg->pi32Pitch[i], pImg->fd[i], miBuf->numberOfPlanes);
        }
    } else {
        pImg->u32PixelArrayFormat = ASVL_PAF_RAW16_BGGR_16B;
        for (int i = 0; i < miBuf->numberOfPlanes; i++) {
            pImg->ppu8Plane[i] = miBuf->plane[i];
            pImg->pi32Pitch[i] = (int)miBuf->stride;
            pImg->fd[i] = miBuf->fd[i];
            MLOGD(Mia2LogGroupPlugin, "[%s] stImage.pi32Pitch[%d]:%d fd:%d numPlanes:%d", LOG_TAG,
                  i, pImg->pi32Pitch[i], pImg->fd[i], miBuf->numberOfPlanes);
        }
    }
    MLOGD(Mia2LogGroupPlugin, "[%s]  the image buffer w:h[%d, %d] format is 0x%x", LOG_TAG,
          pImg->i32Width, pImg->i32Height, pImg->u32PixelArrayFormat);
}

MRESULT MiAIRAWHDR::hdrProcessCb(MLong lProgress, // Not impletement
                                 MLong lStatus,   // Not impletement
                                 MVoid *pParam)
{
    lStatus = 0;
    lProgress = 0;
    MiAIRAWHDR *pMiAIRAWHDR = (MiAIRAWHDR *)pParam;
    ;
    if (pMiAIRAWHDR == NULL || pMiAIRAWHDR->m_flustStatus) {
        MLOGE(Mia2LogGroupPlugin, "[%s] %s cancel", LOG_TAG, __func__);
        return MERR_USER_CANCEL;
    }

    return MERR_NONE;
}

void MiAIRAWHDR::process(struct MiImageBuffer *input, struct MiImageBuffer *output, int input_num,
                         int *ev, ARC_LLHDR_AEINFO &mAeInfo, int camMode, HDRMetaData hdrMetaData,
                         MiAECExposureData *aecExposureData, MiAWBGainParams *awbGainParams,
                         MIAI_HDR_RAWINFO &rawInfo)
{
    std::unique_lock<std::mutex> mtx_locker(g_mutex);
    int32_t isBypassHdrAlgo = sIsBypassHdrAlgo;
    int hdr_mode = MODE_HDR;

    struct tm *timenow;
    time_t now;
    time(&now);
    timenow = localtime(&now);

    for (int i = 0; i < input_num; i++) {
        rawInfo.ev[i] = ev[i];
        MLOGD(Mia2LogGroupPlugin, "[%s] process ev[%d] = %d", LOG_TAG, i, ev[i]);
    }

    std::shared_ptr<PluginPerfLockManager> boostCpu(new PluginPerfLockManager(3000)); // 3000 ms

    switch (input_num) {
    case 6:
        hdr_mode = MODE_RAWHDR;
        break;
    default:
        hdr_mode = MODE_LLHDR;
    }
    if (0 == isBypassHdrAlgo) {
        switch (hdr_mode) {
        case MODE_RAWHDR:
            process_rawhdr_xmi(input, output, input_num, mAeInfo, camMode, hdrMetaData, rawInfo);
            break;
        default:
            break;
        }
    } else // bypass algo
    {
        if (1 <= isBypassHdrAlgo && input_num >= isBypassHdrAlgo) {
            PluginUtils::miCopyBuffer(output, &input[isBypassHdrAlgo - 1]);
        } else {
            PluginUtils::miCopyBuffer(output, &input[0]);
        }
    }
}

void MiAIRAWHDR::process_rawhdr_xmi(struct MiImageBuffer *input, struct MiImageBuffer *output,
                                    int input_num, ARC_LLHDR_AEINFO &mAeInfo, int camMode,
                                    HDRMetaData hdrMetaData, MIAI_HDR_RAWINFO &rawInfo)
{
    HDR_RAW_INPUTINFO m_InputInfo;
    MRESULT ret = MOK;

    double init_start = nowMSec();
    if (g_lastCameraId != mCameraId) {
        MLOGI(Mia2LogGroupPlugin, "[%s]: cameraid change from %d to %d", LOG_TAG, g_lastCameraId,
              mCameraId);
        // uninit
        if (MNull != *g_cameraHdrExpEffect[g_lastCameraId]) {
            MLOGI(Mia2LogGroupPlugin, "[%s]: <camera id: %d> algo uninit begin", LOG_TAG,
                  g_lastCameraId);
            int ret = XMI_HDR_RAW_Uninit(g_cameraHdrExpEffect[g_lastCameraId]);
            MLOGI(Mia2LogGroupPlugin, "[%s]: <camera id: %d> algo uninit result: %d", LOG_TAG,
                  g_lastCameraId, ret);
            *g_cameraHdrExpEffect[g_lastCameraId] = MNull;
            if (ret != MOK)
                MLOGE(Mia2LogGroupPlugin, "[%s]: <camera id: %d> algo uninit error...", LOG_TAG,
                      g_lastCameraId);
        }
    }

    // init
    if (MNull == *g_cameraHdrExpEffect[mCameraId]) {
        ret = XMI_HDR_RAW_Init(g_cameraHdrExpEffect[mCameraId], mCameraId);
        MLOGI(Mia2LogGroupPlugin, "[%s]:  <camera id: %d> hdr reinit ret=%ld cost time %.2f",
              LOG_TAG, mCameraId, ret, nowMSec() - init_start);
        g_lastCameraId = mCameraId;
    }

    MMemSet(&m_InputInfo, 0, sizeof(m_InputInfo));
    m_InputInfo.i32ImgNum = input_num;
    for (int i = 0; i < input_num; i++) {
        setRawOffScreen(&m_InputInfo.InputImages[i], input + i, rawInfo);
    }
    ret = XMI_HDR_RAW_PreProcess(*g_cameraHdrExpEffect[mCameraId]);
    MLOGI(Mia2LogGroupPlugin, "[%s] <camera id: %d> xml raw hdr preprocess result %d", LOG_TAG,
          mCameraId, ret);

    HDR_RAW_OFFSCREEN outputImage = {0};
    setRawOffScreen(&outputImage, output, rawInfo); // please select the ev != zero frames

    double start = nowMSec();
    ret = XMI_HDR_RAW_Process(*g_cameraHdrExpEffect[mCameraId], &m_InputInfo, &rawInfo,
                              &outputImage, &hdrMetaData);
    MLOGD(Mia2LogGroupPlugin, "[%s]:  <camera id: %d> RawHDR_Process ret=%ld cost=%.2f", LOG_TAG,
          mCameraId, ret, nowMSec() - start);
}

#ifndef ARC_USE_ALL_ION_BUFFER
void MiAIRAWHDR::mibuffer_alloc_copy(struct MiImageBuffer *dstBuffer,
                                     struct MiImageBuffer *srcBuffer)
{
    memcpy(dstBuffer, srcBuffer, sizeof(struct MiImageBuffer));
    dstBuffer->Plane[0] = (unsigned char *)malloc(dstBuffer->height * dstBuffer->Pitch[0] * 3 / 2);
    dstBuffer->Plane[1] = dstBuffer->Plane[0] + dstBuffer->height * dstBuffer->Pitch[0];
    unsigned char *psrc = srcBuffer->Plane[0];
    unsigned char *pdst = dstBuffer->Plane[0];
    for (int h = 0; h < srcBuffer->height; h++) {
        memcpy(pdst, psrc, srcBuffer->width);
        psrc += srcBuffer->Pitch[0];
        pdst += dstBuffer->Pitch[0];
    }
    psrc = srcBuffer->Plane[1];
    pdst = dstBuffer->Plane[1];
    for (int h = 0; h < srcBuffer->height / 2; h++) {
        memcpy(pdst, psrc, srcBuffer->width);
        psrc += srcBuffer->Pitch[1];
        pdst += dstBuffer->Pitch[1];
    }
}
void MiAIRAWHDR::mibuffer_release(struct MiImageBuffer *mibuf)
{
    if (mibuf->Plane[0])
        free(mibuf->Plane[0]);
    mibuf->Plane[0] = NULL;
}
#endif

double MiAIRAWHDR::nowMSec(void)
{
    struct timeval res;
    gettimeofday(&res, NULL);
    return 1000.0 * res.tv_sec + (double)res.tv_usec / 1e3;
}

void MiAIRAWHDR::WriteFileLog(char *file, const char *fmt, ...)
{
    FILE *pf = fopen(file, "wb");
    if (pf) {
        va_list ap;
        char buf[MAX_BUF_LEN] = {0};
        va_start(ap, fmt);
        vsnprintf(buf, MAX_BUF_LEN, fmt, ap);
        va_end(ap);
        fwrite(buf, strlen(buf) * sizeof(char), sizeof(char), pf);
        fclose(pf);
    }
}
ARC_HDR_USERDATA MiAIRAWHDR::getUserData()
{
    return m_userData;
}

uint32_t MiAIRAWHDR::getRawType(uint32_t rawFormat, ColorFilterPattern rawPattern)
{
    MLOGI(Mia2LogGroupPlugin, "[%s]:  GetRawType rawFormat = 0x%x ChiColorFilterPattern = %d",
          LOG_TAG, rawFormat, rawPattern);
    switch (rawPattern) {
    case Y:    ///< Monochrome pixel pattern.
    case YUYV: ///< YUYV pixel pattern.
    case YVYU: ///< YVYU pixel pattern.
    case UYVY: ///< UYVY pixel pattern.
    case VYUY: ///< VYUY pixel pattern.
        MLOGI(Mia2LogGroupPlugin, "[%s]:  GetRawType don't hanlde this colorFilterPattern %d",
              LOG_TAG, rawPattern);
        break;
    case RGGB: ///< RGGB pixel pattern.
        if (CAM_FORMAT_RAW10 == rawFormat) {
            return ASVL_PAF_RAW10_RGGB_16B;
        } else if (CAM_FORMAT_RAW12 == rawFormat) {
            return ASVL_PAF_RAW12_RGGB_16B;
        } else if (CAM_FORMAT_RAW16 == rawFormat) {
            return ASVL_PAF_RAW14_RGGB_16B;
        } else {
            return ASVL_PAF_RAW10_RGGB_16B;
        }
    case GRBG: ///< GRBG pixel pattern.
        if (CAM_FORMAT_RAW10 == rawFormat) {
            return ASVL_PAF_RAW10_GRBG_16B;
        } else if (CAM_FORMAT_RAW12 == rawFormat) {
            return ASVL_PAF_RAW12_GRBG_16B;
        } else if (CAM_FORMAT_RAW16 == rawFormat) {
            return ASVL_PAF_RAW14_GRBG_16B;
        } else {
            return ASVL_PAF_RAW10_GRBG_16B;
        }
    case GBRG: ///< GBRG pixel pattern.
        if (CAM_FORMAT_RAW10 == rawFormat) {
            return ASVL_PAF_RAW10_GBRG_16B;
        } else if (CAM_FORMAT_RAW12 == rawFormat) {
            return ASVL_PAF_RAW12_GBRG_16B;
        } else if (CAM_FORMAT_RAW16 == rawFormat) {
            return ASVL_PAF_RAW14_GBRG_16B;
        } else {
            return ASVL_PAF_RAW10_GBRG_16B;
        }
    case BGGR: ///< BGGR pixel pattern.
        if (CAM_FORMAT_RAW10 == rawFormat) {
            return ASVL_PAF_RAW10_BGGR_16B;
        } else if (CAM_FORMAT_RAW12 == rawFormat) {
            return ASVL_PAF_RAW12_BGGR_16B;
        } else if (CAM_FORMAT_RAW16 == rawFormat) {
            return ASVL_PAF_RAW14_BGGR_16B;
        } else {
            return ASVL_PAF_RAW10_BGGR_16B;
        }
        break;
    case RGB: ///< RGB pixel pattern.
        MLOGI(Mia2LogGroupPlugin, "[%s]:  GetRawType don't hanlde this colorFilterPattern %d",
              LOG_TAG, rawPattern);
        break;
    default:
        MLOGI(Mia2LogGroupPlugin,
              "[%s]:  =======================GetRawType dont have the matched "
              "format=====================",
              LOG_TAG);
        break;
    }
    return 0;
}
} // namespace mialgo2
