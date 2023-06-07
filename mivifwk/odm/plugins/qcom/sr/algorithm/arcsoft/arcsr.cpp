#include "arcsr.h"

#include "device.h"

using namespace std;

ArcSR::ArcSR() {}
ArcSR::~ArcSR() {}

void ArcSR::fillInputImage(ASVLOFFSCREEN *pImg, struct mialgo2::MiImageBuffer *miBuf)
{
    pImg->u32PixelArrayFormat = ASVL_PAF_NV12;

    pImg->i32Width = miBuf->width;
    pImg->i32Height = miBuf->height;
    // stride
    pImg->pi32Pitch[0] = miBuf->stride;
    pImg->pi32Pitch[1] = miBuf->stride;
    pImg->ppu8Plane[0] = miBuf->plane[0];
    pImg->ppu8Plane[1] = miBuf->plane[1];
}

MRESULT ArcSR::process_still(struct mialgo2::MiImageBuffer *pImageInput,
                             struct mialgo2::MiImageBuffer *pImageOutput, uint32_t uiImageNum,
                             MRECT *pRect, MInt32 cameraRoleArc, vector<int32_t> uiISO,
                             vector<int64_t> u64ExpTime, float fLuxIndex[], float fDRCGain[],
                             float fDarkBoostGain[], float fSensorGain[], float fDigitalGain[],
                             int anchor, LPARC_MFSR_FACEINFO pFaceInfo, int moonSceneFlag)
{
    static const int32_t logLevel = property_get_int32("persist.vendor.camera.arcsoftsr.log", 2);
    MRESULT res = MERR_UNKNOWN;
    MHandle hEnhancer = MNull;
    ASVLOFFSCREEN nSrcImgs[uiImageNum];
    ASVLOFFSCREEN nDstImg;
    MRECT rtScaleRegion = {0};
    MInt32 nRefIndex = anchor; // The output reference image index if set lRefNum = -1
    ARC_MFSR_META_DATA nMetadata = {0};
    ARC_MFSR_PARAM nParam = {0};
    // MInt32 nRefNum = -1; // set reference image index
    // MVoid* pUserData;

    memset(nSrcImgs, 0, sizeof(nSrcImgs));
    memset(&nDstImg, 0, sizeof(nDstImg));
    memcpy(&rtScaleRegion, pRect, sizeof(MRECT));
    // Fill source images off-screen
    // 1
    for (int i = 0; i < uiImageNum; i++) {
        fillInputImage(&nSrcImgs[i], (pImageInput + i));
    }
    fillInputImage(&nDstImg, pImageOutput);

    nMetadata.nCameraType = cameraRoleArc;

    MLOGI(Mia2LogGroupPlugin, "ArcSoftSR: get algorithm library version: %s",
          ARC_MFSR_GetVersion()->Version);

    res = ARC_MFSR_Init(&hEnhancer, &nSrcImgs[0], &nDstImg);
    if (MOK != res) {
        MLOGE(Mia2LogGroupPlugin, "ArcSoftSR Initialize fails %x", res);
        goto exit;
    }
    MLOGI(Mia2LogGroupPlugin, "ArcSoftSR: Initialize res=%d, hEnhancer=0x%d", res, hEnhancer);

    ARC_MFSR_SetLogLevel(logLevel);
    res = ARC_MFSR_SetCropRegion(hEnhancer, &rtScaleRegion);
    MLOGI(Mia2LogGroupPlugin, "ArcSoftSR:SetCropRegion res=%d, hEnhancer=0x%d", res, hEnhancer);

    for (int i = 0; i < uiImageNum && i < ARC_MFSR_MAX_INPUT_IMAGES; i++) {
        nMetadata.nISO = uiISO[i];
        nMetadata.nExpoTime = u64ExpTime[i];
        nMetadata.fLuxIndex = fLuxIndex[i];
        nMetadata.fDRCGain = fDRCGain[i];
        nMetadata.fDarkBoostGain = fDarkBoostGain[i];
        nMetadata.fSensorGain = fSensorGain[i];
        nMetadata.fDigitalGain = fDigitalGain[i];

        res = ARC_MFSR_PreProcess(hEnhancer, &nSrcImgs[i], i, &nMetadata);
        MLOGI(Mia2LogGroupPlugin, "ArcSoftSR: PreProcess res=%d, hEnhancer=0x%d", res, hEnhancer);
        if (MOK != res) {
            MLOGE(Mia2LogGroupPlugin, "ArcSoftSR PreProcess fails %x, image index:%d", res, i);
            goto exit;
        }
    }

    // faceinfo
    res = ARC_MFSR_SetFaceInfo(hEnhancer, pFaceInfo);
    MLOGI(Mia2LogGroupPlugin, "ArcSoftSR SetFaceInfo : %d", res);

    res = ARC_MFSR_GetDefaultParam(hEnhancer, &nParam);
    MLOGI(Mia2LogGroupPlugin, "ArcSoftSR GetDefaultParam : %d", res);

    res = ARC_MFSR_SetRefIndex(hEnhancer, &nRefIndex);
    MLOGI(Mia2LogGroupPlugin, "ArcSoftSR SetRefIndex : %d", res);

    nParam.i32MoonSceneFlag = moonSceneFlag;
    res = ARC_MFSR_Process(hEnhancer, &nParam, &nDstImg);
    if (MOK != res) {
        MLOGE(Mia2LogGroupPlugin, "ArcSoftSR Process fails %x", res);
        goto exit;
    }
    MLOGI(Mia2LogGroupPlugin, "ArcSoftSR Process end! res=%d, hEnhancer=0x%d", res, hEnhancer);
exit:
    if (hEnhancer != NULL) {
        MRESULT res_uninit = ARC_MFSR_UnInit(&hEnhancer);
        if (MOK != res_uninit) {
            MLOGE(Mia2LogGroupPlugin, "ArcSoftSR UnInit fails %x", res_uninit);
            return res_uninit;
        }
        MLOGI(Mia2LogGroupPlugin, "ArcSoftSR UnInit success %x", res_uninit);
        hEnhancer = NULL;
    }

    return res;
}
