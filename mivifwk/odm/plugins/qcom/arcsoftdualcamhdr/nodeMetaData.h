#include "MiBuf.h"
#include "VendorMetadataParser.h"
#include "arcsoft_bokeh_high_dynamic_range.h"
#include "arcsoft_high_dynamic_range.h"
#include "camxcdktypes.h"
#include "chioemvendortag.h"
#include "device/device.h"
#include "mihdr.h"

#ifndef __NODE_META_DATA_H__
#define __NODE_META_DATA_H__

namespace mialgo2 {

typedef struct
{
    float lux_min;
    float lux_max;
    int checkermode;
    float confval_min;
    float confval_max;
    float adrc_min;
    float adrc_max;
} MiHDRCommonAEParam;

static MiHDRCommonAEParam MiHDRCommonAEParams[] = {
    //(lux_min, lux_max, checkermode, c_min, c_max, drc_min, drc_max)
    {250, 300, 1, 0.6, 0.8, 4.0, 8.0},
};

#define BEAUTY_INFO_MAX 15
typedef struct BeautyInfo
{
    int featureKey[BEAUTY_INFO_MAX];
    int featureValue[BEAUTY_INFO_MAX];
} BeautyInfo;

class NodeMetaData
{
public:
    NodeMetaData();
    virtual ~NodeMetaData();
    S32 Init(U32 mode, U32 debug);

    S32 GetAEInfo(camera_metadata_t *mdata, ARC_HDR_AEINFO *pAeInfo);
    S32 GetFNumberApplied(camera_metadata_t *mdata, FP32 *fNumber);
    S32 GetRotateAngle(camera_metadata_t *mdata, U32 *RotateAngle);
    S32 GetInputMetadataBokeh(camera_metadata_t *mdata, InputMetadataBokeh *pInputMetaBokeh);
    S32 GetLightingMode(camera_metadata_t *mdata, S32 *lightingMode);
    S32 GetHdrEnabled(camera_metadata_t *mdata, S32 *hdrEnabled);
    S32 GetAsdEnabled(camera_metadata_t *mdata, S32 *asdEnabled);
    S32 GetSceneDetected(camera_metadata_t *mdata, S32 *sceneDetected);
    S32 GetLaserDist(camera_metadata_t *mdata, LaserDistance *laserDist);
    S32 GetSensitivity(camera_metadata_t *mdata, S32 *sensitivity);
    S32 GetLux_index(camera_metadata_t *mdata, FP32 *lux_index);
    S32 GetExposureTime(camera_metadata_t *mdata, U64 *exposureTime);
    S32 GetLinearGain(camera_metadata_t *mdata, FP32 *linearGain);
    S32 GetAECsensitivity(camera_metadata_t *mdata, FP32 *sensitivity);
    S32 GetExpValue(camera_metadata_t *mdata, S32 *expValue);
    S32 GetSuperNightEnabled(camera_metadata_t *mdata, S32 *superNightEnabled);
    S32 GetSupernightMode(camera_metadata_t *mdata, S32 *supernightMode);
    S32 GetbeautyMode(camera_metadata_t *mdata, S32 *beautyMode);
    S32 GetThermalLevel(camera_metadata_t *mdata, S32 *thermalLevel);
    S32 GetfocusDistCm(camera_metadata_t *mdata, S32 *focusDistCm);
    S32 GetActiveArraySizeMain(camera_metadata_t *mdata, CHIRECT *activeArraySizeMain);
    S32 GetFaceRect(camera_metadata_t *mdata, DEV_IMAGE_BUF *inputMain, U32 rotateAngle,
                    ARC_BHDR_FACEINFO &face_info);
    S32 GetfocusROI(camera_metadata_t *mdata, DEV_IMAGE_BUF *inputMain, DEV_IMAGE_RECT *focusROI);
    S32 GetfocusTP(camera_metadata_t *mdata, DEV_IMAGE_BUF *inputMain, DEV_IMAGE_POINT *focusTP);
    S32 MapRectToInput(CHIRECT SrcRect, CHIRECT SrcRefer, S32 DstReferInputWidth,
                       S32 DstReferInputHeight, DEV_IMAGE_RECT *DstRect);
    S32 GetMDMode(camera_metadata_t *mdata, S32 *mdmode);
    S32 GetZoomRatio(camera_metadata_t *mdata, FP32 *zoomRatio);
    S32 GetFallbackStatus(camera_metadata_t *mdata, S8 *fallbackStatus);
    S32 GetStylizationType(camera_metadata_t *mdata, U8 *stylizationType);
    S32 GetAFState(camera_metadata_t *mdata, U32 *afStatus);
    S32 GetCropRegion(camera_metadata_t *mdata, CHIRECT *pCropRegin);
    S32 GetAndroidCropRegion(camera_metadata_t *mdata, CHIRECT *pCropRegin);
    S32 SetMetaData(camera_metadata_t *mdata, const char *pTagName, U32 tag, void *pData, U32 size);

private:
    S32 GetMetaData(camera_metadata_t *mdata, const char *pTagName, U32 tag, void **ppData,
                    U32 *pSize);
    // S32 SetMetaData(camera_metadata_t *mdata, const char *pTagName, U32 tag, void *pData, U32
    // size);
    U32 m_mode;
    U32 m_debug;
};

} // namespace mialgo2

#endif //__NODE_META_DATA_H__
