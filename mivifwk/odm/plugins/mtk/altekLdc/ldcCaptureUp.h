#ifndef __LDC_CAPTURE_UP_H__
#define __LDC_CAPTURE_UP_H__

#include "MiaPluginWraper.h"
#include "alhLDC.h"
#include "nodeMetaDataUp.h"
//#include "alLDC.h"
#include "alAILDC.h"
#include "dev_image.h"
// alAILDC.h  alhLDC_def.h  alhLDC_Err.h  alhLDC.h  alLDC.h
namespace mialgo2 {

#define CAPTURE_INPUT_ALIGNMENT 64

typedef enum __LdcCaptureMode {
    LDC_CAPTURE_MODE_ALTEK_NO = 0,
    LDC_CAPTURE_MODE_ALTEK_AH = 1,
    LDC_CAPTURE_MODE_ALTEK_AI = 2,
    LDC_CAPTURE_MODE_ALTEK_HI = 3,
#if (ARCSOFT_LDC_CAPTURE_EN == 1)
    LDC_CAPTURE_MODE_ARCSOFT_DEF = 4,
#endif
} LDC_CAPTURE_MODE;

typedef enum __LdcCaptureDebug {
    LDC_CAPTURE_DEBUG_INFO = (1) << (0),
    LDC_CAPTURE_DEBUG_DUMP_IMAGE_CAPTURE = (1) << (2),
} LDC_CAPTURE_DEBUG;

class LdcCapture
{
public:
    LdcCapture();
    virtual ~LdcCapture();
    S64 Init(CreateInfo *pCreateInfo, U32 debug, U32 mode);
    S64 Process(DEV_IMAGE_BUF *input, DEV_IMAGE_BUF *output, NODE_METADATA_FACE_INFO *pFaceInfo,
                MIRECT *cropRegion, uint32_t streamMode, bool isFastShot);

private:
    S64 InitLib(U32 width, U32 height, U32 strideW, U32 strideH, U32 outWidth, U32 outHeight,
                U32 outStrideW, U32 outStrideH, U32 format);
    S64 GetPackData(LDC_CAPTURE_MODE mode, void **ppData0, U64 *pSize0, void **ppData1,
                    U64 *pSize1);
    S64 FreePackData(void **ppData0, void **ppData1);
    alAILDC_IMG_FORMAT transformFormat(DEV_IMAGE_FORMAT format);
    S64 GetFileData(const char *fileName, void **ppData0, U64 *pSize0);
    S64 FreeFileData(void **ppData0);
    double GetCurrentSecondValue();
    U32 m_feature;
    U32 m_debug;
    FP32 m_ratio;
    int m_vendorId;
    MIRECT m_sensorSize;
    float m_viewAngle;
    // multith reading concurrency
    S64 m_mutexId = 0;
    S64 m_detectorRunId = 0;
    S64 m_detectorInitId = 0;
    S64 m_detectorMaskId = 0;
    U32 m_inited = FALSE;
    U32 m_CameraId;
    bool m_initFirst;
    // U32 m_inFormat = 0xFF;
    // U32 m_inWidth = 0;
    // U32 m_inHeight = 0;
    // U32 m_inStrideW = 0;
    // U32 m_inStrideH = 0;

    // U32 m_outFormat = 0xFF;
    // U32 m_outWidth = 0;
    // U32 m_outHeight = 0;
    // U32 m_outStrideW = 0;
    // U32 m_outStrideH = 0;

    // U32 m_sCurMode = 0;
};

} // namespace mialgo2
#endif //__LDC_CAPTURE_UP_H__
