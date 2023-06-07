#ifndef _MIAI_PORTRAIT_SUPERLOWLIGHT_H_
#define _MIAI_PORTRAIT_SUPERLOWLIGHT_H_

#include "MiaPluginUtils.h"
#include "miai_supernight.h"

using namespace mialgo2;

#define FRAME_DEFAULT_INPUT_NUMBER 8 // liuhaodebug

#define FORMAT_YUV_420_NV21 0x01
#define FORMAT_YUV_420_NV12 0x02

#define HARDCODE_CAMERAID_REAR 0

#define HARDCODE_CAMERAID_FRONT 0x200

#define USE_INTERNAL_BUFFER        0
#define INTERNAL_BUFFER_PIXEL_SIZE 4132 * 3124 * 3 / 2

#define FLAG_FILL_ALL      0
#define FLAG_FILL_METADATA 1

class MiAiSuperLowlight
{
public:
    MiAiSuperLowlight();
    virtual ~MiAiSuperLowlight();
    void init();
    void process(struct MiImageBuffer *input, struct MiImageBuffer *output, int *pExposure,
                 int input_num, int camMode, float gain, LPMIAI_LLHDR_AEINFO pAeInfo,
                 bool IsFlashOn, int orient = 0);
    void uninit();

private:
    void InitInternalBuffer();
    void UninitInternalBuffer();
    void CopyImage(ASVLOFFSCREEN *pSrcImg, ASVLOFFSCREEN *pDstImg);
    void CopyImage(struct MiImageBuffer *pSrcImg, ASVLOFFSCREEN *pDstImg);
    void CopyImage(ASVLOFFSCREEN *pSrcImg, struct MiImageBuffer *pDstImg);
    void setOffScreen(ASVLOFFSCREEN *pImg, struct MiImageBuffer *miBuf);
    void FillInternalImage(ASVLOFFSCREEN *pDstImage, struct MiImageBuffer *miBuf,
                           int flag = FLAG_FILL_ALL);
    void FillInternalImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage,
                           int flag = FLAG_FILL_ALL);
    void OutputInternalImage(struct MiImageBuffer *miBuf, ASVLOFFSCREEN *pSrcImage,
                             int flag = FLAG_FILL_ALL);
    void AllocCopyImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage);
    void ReleaseImage(ASVLOFFSCREEN *pImage);

private:
    MHandle m_hSuperLowlightEngine;
    ASVLOFFSCREEN mImageInput[FRAME_DEFAULT_INPUT_NUMBER];
    ASVLOFFSCREEN mImageOutput;
    unsigned int u32ImagePixelSize;
    int mAlgoInitRet;
};
#endif
