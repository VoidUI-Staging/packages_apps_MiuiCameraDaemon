#include "superportrait.h"

#include <cutils/properties.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include "arcsoft_debuglog.h"
#include "arcsoft_utils.h"

using namespace mialgo2;

SuperPortrait::SuperPortrait()
{
    MLOGI(Mia2LogGroupPlugin, "%s E", __func__);
    pthread_mutex_init(&m_processLock, NULL);
    pthread_mutex_init(&m_paramLock, NULL);

    m_SuperPortrait_Video_Algorithm = NULL;
    m_SuperPortrait_Image_Algorithm = NULL;
    m_initDone = false;
    m_pStyleData = NULL;

    if (m_pStyleData == NULL) {
        FILE *pf = fopen(ARC_PORTRAIT_MODEL_FILEPATH, "rb");
        if (pf) {
            fseek(pf, 0L, SEEK_END);
            m_iStyleDataSize = ftell(pf);
            m_pStyleData = malloc(m_iStyleDataSize);
            fseek(pf, 0L, SEEK_SET);
            if (m_pStyleData)
                fread(m_pStyleData, 1, m_iStyleDataSize, pf);
            fclose(pf);
        }
    }
    MLOGI(Mia2LogGroupPlugin, "%s X", __func__);
}

SuperPortrait::~SuperPortrait()
{
    MLOGI(Mia2LogGroupPlugin, "%s E", __func__);
    if (m_initDone)
        reset();
    m_SuperPortrait_Video_Algorithm = NULL;
    m_SuperPortrait_Image_Algorithm = NULL;
    pthread_mutex_destroy(&m_processLock);
    pthread_mutex_destroy(&m_paramLock);

    if (m_pStyleData)
        free(m_pStyleData);
    m_pStyleData = NULL;
    MLOGI(Mia2LogGroupPlugin, "%s X", __func__);
}

void SuperPortrait::init(int mode)
{
    if (!m_initDone) {
        m_width = m_height = 0;
        if (mode != PORTRAIT_MODE_CAPTURE) {
            if (Create_SuperPortrait_Video_Algorithm(SuperPortrait_Video_Algorithm::CLASSID,
                                                     &m_SuperPortrait_Video_Algorithm) != MOK ||
                m_SuperPortrait_Video_Algorithm == NULL) {
                MLOGI(Mia2LogGroupPlugin, "Create_SuperPortrait_Video_Algorithm, fail");
                return;
            }
        }
        m_initDone = true;
    }
}

void SuperPortrait::reset()
{
    if (m_SuperPortrait_Video_Algorithm) {
        m_SuperPortrait_Video_Algorithm->UnInit();
        m_SuperPortrait_Video_Algorithm->Release();
        m_SuperPortrait_Video_Algorithm = NULL;
    }
    m_initDone = false;
}

void SuperPortrait::enable(bool enable, int mode)
{
    if (m_initDone && !enable)
        reset();
    else if (!m_initDone && enable)
        init(mode);
}

int SuperPortrait::process(struct MiImageBuffer *input, struct MiImageBuffer *output, int orient,
                           int32_t mode)
{
    if (!m_initDone)
        return MIA_RETURN_INVALID_PARAM;
    int rc = -1;
    static int dumpIndex = 0;
    m_orientation = orient;

    //  MiaUtil::MiAllocCopyBuffer(&tmpBuffer, input);
    struct MiImageBuffer *pInput = input;
    struct MiImageBuffer *pOutput = output;
#ifndef ARC_USE_ALL_ION_BUFFER
    struct MiImageBuffer tmpBuffer;
    mibuffer_alloc_copy(&tmpBuffer, input);
    pInput = pOutput = &tmpBuffer;
#endif
    time_t now;
    time(&now);
    tim_stamp_dump = localtime(&now);
    dumpBuffer(pInput, "persist.camera.mibuf.portrait", "portrait-int", dumpIndex);

    if (mode == PORTRAIT_MODE_CAPTURE) {
        rc = processYUVArcsoft(&tmpBuffer, &tmpBuffer, orient);
    } else {
        rc = processPreviewYUVArcsoft(&tmpBuffer, &tmpBuffer, orient);
    }
    dumpBuffer(pOutput, "persist.camera.mibuf.portrait", "portrait-out", dumpIndex);

#ifndef ARC_USE_ALL_ION_BUFFER
    mibuffer_copy(output, &tmpBuffer);
    mibuffer_release(&tmpBuffer);
#endif

    return rc;
}

MVoid SuperPortrait::setOffScreen(ASVLOFFSCREEN *pImg, struct MiImageBuffer *miBuf)
{
    if (miBuf->format == CAM_FORMAT_YUV_420_NV21)
        pImg->u32PixelArrayFormat = ASVL_PAF_NV21;
    else if (miBuf->format == CAM_FORMAT_YUV_420_NV12)
        pImg->u32PixelArrayFormat = ASVL_PAF_NV12;

    pImg->i32Width = miBuf->width;
    pImg->i32Height = miBuf->height;
    pImg->pi32Pitch[0] = miBuf->stride;
    pImg->pi32Pitch[1] = miBuf->stride;
    pImg->ppu8Plane[0] = miBuf->plane[0];
    pImg->ppu8Plane[1] = miBuf->plane[1];
}

int SuperPortrait::processPreviewYUVArcsoft(struct MiImageBuffer *input,
                                            struct MiImageBuffer *output, int orient)
{
    if (!input)
        return MIA_RETURN_INVALID_PARAM;

    if ((input->width != m_width) || (input->height != m_height)) {
        if ((m_width != 0) || (m_height != 0))
            m_SuperPortrait_Video_Algorithm->UnInit();
        m_SuperPortrait_Video_Algorithm->Init();
        m_width = input->width;
        m_height = input->height;
    }

    char value[PROPERTY_VALUE_MAX];
    property_get("persist.camera.mibuf.portrait.enable", value, "1");

    m_SuperPortrait_Video_Algorithm->SetFeatureLevel(FEATURE_SUPER_PORTRAIT_KEY, atoi(value));

    MLOGI(Mia2LogGroupPlugin, "arcsoft debug preview set feature super portrait key = %d",
          atoi(value));
    ASVLOFFSCREEN tScreenSrc = {0};
    ASVLOFFSCREEN tScreenDst = {0};

    setOffScreen(&tScreenSrc, input);
    setOffScreen(&tScreenDst, output);

    double start = now_ms();
    MRESULT ret = m_SuperPortrait_Video_Algorithm->Process(&tScreenSrc, &tScreenDst, NULL, NULL,
                                                           NULL, orient);
    if (ret) {
        MLOGI(Mia2LogGroupPlugin, "arcsoft preview Process YUV process fail, ret:%ld", ret);
        return MIA_RETURN_INVALID_PARAM;
    }

    MLOGI(Mia2LogGroupPlugin,
          "arcsoft debug preview process YUV - copy :%d, h:%d,algo process time:%f", output->width,
          output->height, now_ms() - start);

    return MIA_RETURN_OK;
}

int SuperPortrait::processYUVArcsoft(struct MiImageBuffer *input, struct MiImageBuffer *output,
                                     int orient)
{
    MLOGI(Mia2LogGroupPlugin, "arcsoft Process YUV begin");
    if (!input)
        return MIA_RETURN_INVALID_PARAM;
    double t0, t1;
    t0 = now_ms();

    MLOGI(Mia2LogGroupPlugin, "Create_SuperPortrait_Image_Algorithm");
    if (Create_SuperPortrait_Image_Algorithm(SuperPortrait_Image_Algorithm::CLASSID,
                                             &m_SuperPortrait_Image_Algorithm) != MOK ||
        m_SuperPortrait_Image_Algorithm == NULL) {
        MLOGI(Mia2LogGroupPlugin, "Create_SuperPortrait_Image_Algorithm, fail");
        return MIA_RETURN_INVALID_PARAM;
    }

    MRESULT ret = m_SuperPortrait_Image_Algorithm->Init();
    const MPBASE_Version *version_info = m_SuperPortrait_Image_Algorithm->GetVersion();
    if (version_info != NULL) {
        MLOGI(Mia2LogGroupPlugin,
              "MiaNodeSuperPortrait get algorithm library version: %s, buildDate:%s",
              version_info->Version, version_info->BuildDate);
    }

    if (ret) {
        m_SuperPortrait_Image_Algorithm->Release();
        m_SuperPortrait_Image_Algorithm = NULL;
        return MIA_RETURN_INVALID_PARAM;
    }

    if (m_pStyleData != NULL && m_iStyleDataSize > 0) {
        m_SuperPortrait_Image_Algorithm->LoadStyle(m_pStyleData, m_iStyleDataSize,
                                                   0); // 0 load model.data
    }

    char value[PROPERTY_VALUE_MAX];
    property_get("persist.camera.mibuf.portrait.enable", value, "1");
    m_SuperPortrait_Image_Algorithm->SetFeatureLevel(FEATURE_SUPER_PORTRAIT_KEY, atoi(value));
    MLOGI(Mia2LogGroupPlugin, "arcsoft debug capture set feature super portrait key = %d",
          atoi(value));

    ASVLOFFSCREEN tScreenSrc = {0};
    ASVLOFFSCREEN tScreenDst = {0};

    setOffScreen(&tScreenSrc, input);
    setOffScreen(&tScreenDst, output);

    double proces_before = now_ms();
    ret = m_SuperPortrait_Image_Algorithm->Process(&tScreenSrc, &tScreenDst, NULL, NULL, NULL,
                                                   orient);
    double proces_after = now_ms();
    if (ret) {
        m_SuperPortrait_Image_Algorithm->UnInit();
        m_SuperPortrait_Image_Algorithm->Release();
        m_SuperPortrait_Image_Algorithm = NULL;
        MLOGI(Mia2LogGroupPlugin, "arcsoft Process YUV process fail, ret:%ld", ret);
        return MIA_RETURN_INVALID_PARAM;
    }

    m_SuperPortrait_Image_Algorithm->UnInit();
    m_SuperPortrait_Image_Algorithm->Release();
    m_SuperPortrait_Image_Algorithm = NULL;
    t1 = now_ms();
    MLOGI(Mia2LogGroupPlugin,
          "arcsoft debug capture process YUV w:%d, h:%d, orient=%d total time: %.2f process "
          "time=%.2f",
          output->width, output->height, orient, t1 - t0, proces_after - proces_before);
    return MIA_RETURN_OK;
}

void SuperPortrait::dumpBuffer(struct MiImageBuffer *buffer, const char *properties,
                               const char *name, int &index)
{
    char value[PROPERTY_VALUE_MAX];
    property_get(properties, value, "0");
    int enable = atoi(value);

    if (enable) {
        struct tm *timenow;
        time_t now;
        time(&now);
        timenow = localtime(&now);
        snprintf(value, PROPERTY_VALUE_MAX, "%s%s_%02d%02d%02d_%d_%dx%d_%d_%d.nv12",
                 MI_CAMERA_WORK_PATH, name, timenow->tm_hour, timenow->tm_min, timenow->tm_sec,
                 index++, buffer->width, buffer->height, buffer->stride, m_orientation);
        if (buffer->format == CAM_FORMAT_YUV_420_NV21 ||
            buffer->format == CAM_FORMAT_YUV_420_NV12) {
            FILE *pf = fopen(value, "wb");
            if (pf) {
                unsigned char *pdata = buffer->plane[0];
                for (int i = 0; i < buffer->height; i++) {
                    fwrite(pdata, buffer->stride, 1, pf);
                    pdata += buffer->stride;
                }
                pdata = buffer->plane[1];
                for (int i = 0; i < buffer->height / 2; i++) {
                    fwrite(pdata, buffer->stride, 1, pf);
                    pdata += buffer->stride;
                }
                fclose(pf);
            }
        }
    }
}
double SuperPortrait::now_ms(void)
{
    struct timeval res;
    gettimeofday(&res, NULL);
    return 1000.0 * res.tv_sec + (double)res.tv_usec / 1e3;
}
#ifndef ARC_USE_ALL_ION_BUFFER
void SuperPortrait::mibuffer_copy(struct MiImageBuffer *dstBuffer, struct MiImageBuffer *srcBuffer)
{
    unsigned char *psrc = srcBuffer->plane[0];
    unsigned char *pdst = dstBuffer->plane[0];
    for (int h = 0; h < srcBuffer->height; h++) {
        memcpy(pdst, psrc, srcBuffer->width);
        psrc += srcBuffer->stride;
        pdst += dstBuffer->stride;
    }
    psrc = srcBuffer->plane[1];
    pdst = dstBuffer->plane[1];
    for (int h = 0; h < srcBuffer->height / 2; h++) {
        memcpy(pdst, psrc, srcBuffer->width);
        psrc += srcBuffer->stride;
        pdst += dstBuffer->stride;
    }
}

void SuperPortrait::mibuffer_alloc_copy(struct MiImageBuffer *dstBuffer,
                                        struct MiImageBuffer *srcBuffer)
{
    memcpy(dstBuffer, srcBuffer, sizeof(struct MiImageBuffer));
    dstBuffer->plane[0] = (unsigned char *)malloc(dstBuffer->height * dstBuffer->stride * 3 / 2);
    dstBuffer->plane[1] = dstBuffer->plane[0] + dstBuffer->height * dstBuffer->stride;
    unsigned char *psrc = srcBuffer->plane[0];
    unsigned char *pdst = dstBuffer->plane[0];
    for (int h = 0; h < srcBuffer->height; h++) {
        memcpy(pdst, psrc, srcBuffer->width);
        psrc += srcBuffer->stride;
        pdst += dstBuffer->stride;
    }
    psrc = srcBuffer->plane[1];
    pdst = dstBuffer->plane[1];
    for (int h = 0; h < srcBuffer->height / 2; h++) {
        memcpy(pdst, psrc, srcBuffer->width);
        psrc += srcBuffer->stride;
        pdst += dstBuffer->stride;
    }
}
void SuperPortrait::mibuffer_release(struct MiImageBuffer *mibuf)
{
    free(mibuf->plane[0]);
}
#endif

void SuperPortrait::ImgBuffercopy(LPASVLOFFSCREEN lpImgDst, LPASVLOFFSCREEN lpImgSrc)
{
    if (lpImgSrc->ppu8Plane[0] == NULL || lpImgDst->ppu8Plane[0] == NULL)
        return;
    unsigned char *psrc = lpImgSrc->ppu8Plane[0];
    unsigned char *pdst = lpImgDst->ppu8Plane[0];
    for (int h = 0; h < lpImgSrc->i32Height; h++) {
        memcpy(pdst, psrc, lpImgSrc->i32Width);
        psrc += lpImgSrc->pi32Pitch[0];
        pdst += lpImgDst->pi32Pitch[0];
    }
    psrc = lpImgSrc->ppu8Plane[1];
    pdst = lpImgDst->ppu8Plane[1];
    for (int h = 0; h < lpImgSrc->i32Height / 2; h++) {
        memcpy(pdst, psrc, lpImgSrc->i32Width);
        psrc += lpImgSrc->pi32Pitch[1];
        pdst += lpImgDst->pi32Pitch[1];
    }
}

void SuperPortrait::ImgBufferalloc_copy(LPASVLOFFSCREEN lpImgDst, LPASVLOFFSCREEN lpImgSrc)
{
    if (lpImgDst->ppu8Plane[0])
        ImgBufferrelease(lpImgDst);
    memcpy(lpImgDst, lpImgSrc, sizeof(ASVLOFFSCREEN));
    lpImgDst->ppu8Plane[0] =
        (unsigned char *)malloc(lpImgDst->i32Height * lpImgDst->pi32Pitch[0] * 3 / 2);
    if (lpImgDst->ppu8Plane[0]) {
        lpImgDst->ppu8Plane[1] =
            lpImgDst->ppu8Plane[0] + lpImgDst->i32Height * lpImgDst->pi32Pitch[0];
        ImgBuffercopy(lpImgDst, lpImgSrc);
    }
}

void SuperPortrait::ImgBufferrelease(LPASVLOFFSCREEN lpImg)
{
    if (lpImg->ppu8Plane[0])
        free(lpImg->ppu8Plane[0]);
    lpImg->ppu8Plane[0] = NULL;
}
