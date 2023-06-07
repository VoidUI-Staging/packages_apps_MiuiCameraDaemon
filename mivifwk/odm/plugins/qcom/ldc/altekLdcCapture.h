#ifndef __ALTEK_LDC_CAPTURE_H__
#define __ALTEK_LDC_CAPTURE_H__

#include "alAILDC.h"
#include "device/device.h"
#include "ldcCommon.h"

namespace mialgo2 {

typedef enum {
    ALTEK_LDC_CAPTURE_MODE_H = 0,
    ALTEK_LDC_CAPTURE_MODE_AI = 1,
} ALTEK_LDC_CAPTURE_MODE;

typedef enum {
    ALTEK_LDC_FACE_MASK_DISABLE = 0,
    ALTEK_LDC_FACE_MASK_ENABLE = 1,
} ALTEK_LDC_FACE_MASK;

class AltekLdcCapture
{
public:
    AltekLdcCapture(U32 debug = 0);
    ~AltekLdcCapture();
    S32 Process(LdcProcessInputInfo &inputInfo);

private:
    S32 InitLib(const LdcProcessInputInfo &inputInfo);
    bool CheckNeedInitLib(DEV_IMAGE_BUF *input);
    void ResetLibPara();
    S32 GetPackData(U32 vendorId, void **ppData, S64 *pSize);
    S32 GetTuningData(void **ppData, S64 *pSize);
    S32 GetModelData(void **ppData, S64 *pSize);
    S32 FreeData(void **ppData);
    alAILDC_IMG_FORMAT GetAltekLDCFormat(int srcFormat);

    U32 m_debug = 0;
    U32 m_captureMode = 0;
    U32 m_useAltekMask = 0;
    U32 m_libWidth = 0;
    U32 m_libHeight = 0;
    U32 m_libStride = 0;
    U32 m_libSliceHeight = 0;
    U32 m_libFormat = 0;
    LDC_LIBSTATUS m_libStatus = LDC_LIBSTATUS_INVALID;
    S64 m_detectorRunId = 0;
    S64 m_detectorInitId = 0;
};

} // namespace mialgo2
#endif // __ALTEK_LDC_CAPTURE_H__