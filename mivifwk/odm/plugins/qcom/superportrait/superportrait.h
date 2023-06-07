#ifndef SUPERPORTRAIT_H_
#define SUPERPORTRAIT_H_

#include <pthread.h>

#include "MiaPluginUtils.h"
#include "SuperPortrait_Image_Algorithm.h"
#include "SuperPortrait_Video_Algorithm.h"
#include "merror.h"

#define ARC_PORTRAIT_MODEL_FILEPATH "/vendor/etc/camera/model.dat"
#define MI_CAMERA_WORK_PATH         "/data/vendor/camera/"

#define __TAG__ "ARCSOFT_SUPERPORTRAIT"

enum ePortraitMode {
    PORTRAIT_MODE_CAPTURE = 1,
    PORTRAIT_MODE_PREVIEW,
    PORTRAIT_MODE_RECORD,
};

#define FORMAT_YUV_420_NV21 0x01
#define FORMAT_YUV_420_NV12 0x02
#define MAX_BUFFER_PLANE    2

using namespace mialgo2;

class SuperPortrait
{
public:
    SuperPortrait();
    virtual ~SuperPortrait();
    void enable(bool enable, int mode = PORTRAIT_MODE_CAPTURE);
    int process(struct MiImageBuffer *input, struct MiImageBuffer *output, int orient,
                int32_t mode);

private:
    void init(int mode = PORTRAIT_MODE_CAPTURE);
    void reset();
    int processYUVArcsoft(struct MiImageBuffer *input, struct MiImageBuffer *output, int orient);
    int processPreviewYUVArcsoft(struct MiImageBuffer *input, struct MiImageBuffer *output,
                                 int orient);
    void dumpBuffer(struct MiImageBuffer *buffer, const char *properties, const char *name,
                    int &index);
    void mibuffer_alloc_copy(struct MiImageBuffer *dstBuffer, struct MiImageBuffer *srcBuffer);
    void mibuffer_copy(struct MiImageBuffer *dstBuffer, struct MiImageBuffer *srcBuffer);
    void mibuffer_release(struct MiImageBuffer *mibuf);
    void ImgBuffercopy(LPASVLOFFSCREEN lpImgDst, LPASVLOFFSCREEN lpImgSrc);
    void ImgBufferalloc_copy(LPASVLOFFSCREEN lpImgDst, LPASVLOFFSCREEN lpImgSrc);
    void ImgBufferrelease(LPASVLOFFSCREEN mibuf);

private:
    bool m_initDone;
    int m_width;
    int m_height;
    void *m_pStyleData;
    int m_iStyleDataSize;
    int m_orientation;
    double now_ms(void);
    SuperPortrait_Video_Algorithm *m_SuperPortrait_Video_Algorithm;
    SuperPortrait_Image_Algorithm *m_SuperPortrait_Image_Algorithm;
    MVoid setOffScreen(ASVLOFFSCREEN *pImg, struct MiImageBuffer *miBuf);
    pthread_mutex_t m_processLock;
    pthread_mutex_t m_paramLock;
    struct tm *tim_stamp_dump;
};

#endif
