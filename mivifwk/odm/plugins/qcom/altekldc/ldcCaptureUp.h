#ifndef __LDC_CAPTURE_UP_H__
#define __LDC_CAPTURE_UP_H__

#include "MiaPluginWraper.h"
#include "ldcCommon.h"
//#if (FACE_MASK_TYPE == LDC_FACE_MASK_AILAB)
#if defined(ENABLE_FACE_MASK_AILAB)
#include "miBokehUp.h"
#endif
#if defined(ENABLE_FACE_MASK_MIALGOSEG)
#include "mialgo_seg_ldc.h"
#endif
#include "alAILDC.h"
#include "alhLDC.h"
#include "nodeMetaDataUp.h"

namespace mialgo2 {

#define CAPTURE_INPUT_ALIGNMENT 64
// // clang-format off
// typedef enum {
//     LDC_CAPTURE_DEBUG_INFO                 = (1) << (0),
//     LDC_CAPTURE_DEBUG_DUMP_IMAGE_CAPTURE   = (1) << (2),
//     LDC_CAPTURE_DEBUG_OUTPUT_DRAW_FACE_BOX = (1) << (12),
//     LDC_CAPTURE_DEBUG_INPUT_DRAW_FACE_BOX  = (1) << (13),
// } LDC_CAPTURE_DEBUG;

// typedef enum {
//     LDC_CAPTURE_MODE_ALTEK_H = 0,
//     LDC_CAPTURE_MODE_ALTEK_AI  = 1,
// } LDC_CAPTURE_MODE;

// typedef enum {
//     LDC_CAPTURE_STREAMCONFIGMODE_DEF = 0,
//     LDC_CAPTURE_STREAMCONFIGMODE_SAT = 0x8001,
// } LDC_CAPTURE_STREAMCONFIGMODE;
// // clang-format on

// typedef enum __LdcCaptureMode {
//    LDC_CAPTURE_MODE_ALTEK_DEF      = 0,
//    LDC_CAPTURE_MODE_ALTEK_AI       = 1,
//} LDC_CAPTURE_MODE;

enum CAPTURE_LDC_ORIENTATION {
    CAPTURE_LDC_ORIENTATION_0 = 0,
    CAPTURE_LDC_ORIENTATION_90 = 90,
    CAPTURE_LDC_ORIENTATION_180 = 180,
    CAPTURE_LDC_ORIENTATION_270 = 270
};

class LdcCapture
{
public:
    LdcCapture(U32 mode = LDC_CAPTURE_MODE_MIPHONE, U32 debug = 0);
    virtual ~LdcCapture();
    S64 Process(LdcProcessInputInfo &inputInfo);
    S32 PreProcess();

private:
    S64 InitLib(U32 width, U32 height, U32 strideW, U32 strideH, U32 outWidth, U32 outHeight,
                U32 outStrideW, U32 outStrideH, U32 format, U32 vendorId);
    S64 GetPackData(U32 vendorId, void **ppData0, U64 *pSize0, void **ppData1, U64 *pSize1);
    S64 FreePackData(void **ppData0, void **ppData1);
    S64 GetFileData(const char *fileName, void **ppData0, U64 *pSize0);
    S64 FreeFileData(void **ppData0);
    alAILDC_IMG_FORMAT GetMiLDCFormat(int srcFormat);
#if (FACE_MASK_TYPE == LDC_FACE_MASK_MIALGOSEG)
    S32 GetMaskFromMiSeg(LdcProcessInputInfo &inputInfo, DEV_IMAGE_BUF *maskDepthoutput);
#endif
    U32 m_captureMode;
    U32 m_debug;
    U32 m_libInitFlag;
    BOOL m_lastReqHasFace;
    S64 m_detectorRunId;
    S64 m_detectorInitId;
    void *m_hMiCaptureLDC;
    void *m_hMiMaskHandle; // MialgoSeg handle
    BOOL m_miMaskInitFlag; // MialgoSeg
};

} // namespace mialgo2
#endif //__LDC_CAPTURE_UP_H__
