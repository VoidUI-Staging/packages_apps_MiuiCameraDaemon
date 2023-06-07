#ifndef __MI_LDC_CAPTURE_H__
#define __MI_LDC_CAPTURE_H__

#include "capture_ldc_proc.h"
#include "device/device.h"
#include "ldcCommon.h"

namespace mialgo2 {

class MiLdcCapture
{
public:
    MiLdcCapture(U32 debug = 0);
    ~MiLdcCapture();
    S32 Process(LdcProcessInputInfo &inputInfo);

private:
    S32 InitLib(const LdcProcessInputInfo &inputInfo);
    bool CheckNeedInitLib(DEV_IMAGE_BUF *input);
    void ResetLibPara();
    S32 GetCalibBin(U32 vendorId, void **ppData);
    S32 FreeCalibBin(void **ppData);
    CAPTURE_LDC_IMG_FORMAT GetMiLDCFormat(int srcFormat);
    S32 GetMaskFromMiDetSeg(LdcProcessInputInfo &inputInfo, DEV_IMAGE_BUF *maskDepthoutput);

    U32 m_debug = 0;
    U32 m_libWidth = 0;
    U32 m_libHeight = 0;
    U32 m_libStride = 0;
    U32 m_libSliceHeight = 0;
    U32 m_libFormat = 0;
    LDC_LIBSTATUS m_libStatus = LDC_LIBSTATUS_INVALID;
    void *m_hMiCaptureLDC = nullptr;
    S64 m_detectorRunId = 0;
    S64 m_detectorInitId = 0;
    void *m_hMiDetSeg = nullptr;   // Mi Det+Seg handle
    bool m_segLibInitFlag = false; // Mi Det+Seg init flag
};

} // namespace mialgo2
#endif // __MI_LDC_CAPTURE_H__
