#ifndef __LDC_CAPTURE_UP_H__
#define __LDC_CAPTURE_UP_H__

#include <fstream>

#include "WideLensUndistort.h"
#include "dev_image.h"
#include "nodeMetaDataUp.h"

using wa::WideLensUndistort;

namespace mialgo2 {

#define CAPTURE_INPUT_ALIGNMENT 64

typedef struct
{
    unsigned short x;
    unsigned short y;
    unsigned short width;
    unsigned short height;
} LDC_RECT;

typedef enum __LdcCaptureMode {
    LDC_CAPTURE_MODE_ALTEK_DEF = 0,
    LDC_CAPTURE_MODE_ALTEK_AI = 1,
#if (ARCSOFT_LDC_CAPTURE_EN == 1)
    LDC_CAPTURE_MODE_ARCSOFT_DEF = 2,
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
    S64 Init(U32 mode, U32 debug, FP32 ratio, MIRECT *sensorSize, float viewAngle, int vendorId);
    S64 Process(DEV_IMAGE_BUF *input, DEV_IMAGE_BUF *output, NODE_METADATA_FACE_INFO *pFaceInfo,
                MIRECT *cropRegion, uint32_t streamMode);

private:
    S64 InitLib(U32 width, U32 height, U32 strideW, U32 strideH, U32 outWidth, U32 outHeight,
                U32 outStrideW, U32 outStrideH, U32 format);
    S64 GetPackData(LDC_CAPTURE_MODE mode, void **ppData0, U64 *pSize0, void **ppData1,
                    U64 *pSize1);
    S64 FreePackData(void **ppData0, void **ppData1);
    void TransformZoomWOI(LDC_RECT &zoomWOI, MIRECT *cropRegion, DEV_IMAGE_BUF *input);
    void drawMask(DEV_IMAGE_BUF *buffer, float fx, float fy, float fw, float fh);
    // parms of the Multiple camera ultra sensores
    const char *cameraParams[3] = {"camera_params", "camera_params_second", "camera_params_third"};
    wa::Image::ImageType transformFormat(DEV_IMAGE_FORMAT format);
    U32 m_mode;
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
    WideLensUndistort *m_algo = NULL;
    bool mDebugLog = 0;
    bool m_debugFaceDrawGet;

    U32 ldcLibInit_f = FALSE;
    std::mutex gMutex;
    U32 m_format = 0xFF;
    U32 m_Width = 0;
    U32 m_Height = 0;
    U32 m_StrideW = 0;
    U32 m_StrideH = 0;
    U32 m_sCurMode = 0;
};

} // namespace mialgo2
#endif //__LDC_CAPTURE_UP_H__
