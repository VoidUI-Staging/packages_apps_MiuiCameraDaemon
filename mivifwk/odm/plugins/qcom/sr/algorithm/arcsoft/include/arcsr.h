// clang-format off
#ifndef _MIA_ARCSR_H_
#define _MIA_ARCSR_H_
#include <MiaPluginUtils.h>
#include "metaGetter.h"
#include "chioemvendortag.h"
#include <VendorMetadataParser.h>
#include <string.h>

#include <vector>

#include "amcomdef.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "arcsoft_mf_superresolution.h"
// clang-format on
using namespace mialgo2;

class ArcSR
{
public:
    ArcSR();

    virtual ~ArcSR();

    MRESULT process_still(struct mialgo2::MiImageBuffer *pImageInput,
                          struct mialgo2::MiImageBuffer *pImageOutput, uint32_t uiImageNum,
                          MRECT *pRect, MInt32 cameraRoleArc, vector<int32_t> uiISO,
                          vector<int64_t> u64ExpTime, float fLuxIndex[], float fDRCGain[],
                          float fDarkBoostGain[], float fSensorGain[], float fDigitalGain[],
                          int anchor, LPARC_MFSR_FACEINFO pFaceInfo, int moonSceneFlag);

private:
    void fillInputImage(ASVLOFFSCREEN *pImg, struct mialgo2::MiImageBuffer *miBuf);
};

#endif
