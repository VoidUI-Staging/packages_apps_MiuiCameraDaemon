#include "VendorMetadataParser.h"
#include "camxcdktypes.h"
#include "chioemvendortag.h"
#include "chivendortag.h"
#include "device/device.h"
#include "monocam_bokeh_proc.h"

#ifndef __NODE_META_DATA_H__
#define __NODE_META_DATA_H__

namespace mialgo2 {

class NodeMetaData
{
public:
    NodeMetaData();
    virtual ~NodeMetaData();
    S32 Init(U32 mode, U32 debug);
    S32 GetFNumberApplied(camera_metadata_t *pMetadata, FP32 *pFNumber);
    S32 GetOrientation(camera_metadata_t *pMetadata, S32 *pOrientation);
    S32 GetRotateAngle(camera_metadata_t *pMetadata, S32 *pRotateAngle);
    S32 GetSensitivity(camera_metadata_t *pMetadata, S32 *pSensitivity);
    S32 GetThermalLevel(camera_metadata_t *pMetadata, S32 *pThermalLevel);
    S32 GetCropRegion(camera_metadata_t *pMetadata, CHIRECT *pCropRegion);
    S32 GetActiveArraySize(camera_metadata_t *pMetadata, CHIRECT *pActiveArraySize);
    S32 GetMaxFaceRect(camera_metadata_t *pMetadata, S32 width, S32 height,
                       AlgoMultiCams::MiFaceRect *pFaceRect, AlgoMultiCams::MiPoint2i *pFaceCentor);

private:
    S32 GetMetaData(camera_metadata_t *mdata, const char *pTagName, U32 tag, void **ppData,
                    U32 *pSize);
    S32 SetMetaData(camera_metadata_t *mdata, const char *pTagName, U32 tag, void *pData, U32 size);
    U32 m_mode;
    U32 m_debug;
};

} // namespace mialgo2

#endif //__NODE_META_DATA_H__
