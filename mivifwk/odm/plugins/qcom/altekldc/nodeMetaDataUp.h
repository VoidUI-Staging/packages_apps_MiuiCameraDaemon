#include <chxdefs.h>
#include "MiaPluginUtils.h"
#include "VendorMetadataParser.h"
#include "chioemvendortag.h"
#include "chivendortag.h"
#include "device/device.h"
#include "ldcCommon.h"

#ifndef __NODE_METADATA_H__
#define __NODE_METADATA_H__

namespace mialgo2 {

#define NODE_METADATA_FACE_INFO_NUM_MAX (10)

struct MIRECT // Todo:remove MIRECT, MiaVendorTag.h not used
{
    uint32_t left;   ///< x coordinate for top-left point
    uint32_t top;    ///< y coordinate for top-left point
    uint32_t width;  ///< width
    uint32_t height; ///< height
};
typedef enum __Node_MetaData_Mode {
    NODE_METADATA_MODE_PREVIEW = 0,
    NODE_METADATA_MODE_CAPTURE = 1,
} NODE_METADATA_MODE;

typedef enum __Node_MetaData_Debug {
    NODE_METADATA_DEBUG_INFO = (1) << (0),
} NODE_METADATA_DEBUG;

typedef struct __Node_MetaData_Face_Info
{
    U32 num;
    MIRECT face[NODE_METADATA_FACE_INFO_NUM_MAX];
    U32 orient[NODE_METADATA_FACE_INFO_NUM_MAX];
    U32 score[NODE_METADATA_FACE_INFO_NUM_MAX];
    MIRECT sensorActiveArraySize; // IN
    U32 imgWidth;                 // IN
    U32 imgHeight;                // IN
    S32 topOffset;
    S32 leftOffset;
} NODE_METADATA_FACE_INFO;

class NodeMetaData
{
public:
    NodeMetaData();
    virtual ~NodeMetaData();
    S32 Init(U32 mode, U32 debug);
    S32 GetCameraVendorId(camera_metadata_t *mdata, U32 *pCameraVendorId);
    S32 SetCameraVendorId(camera_metadata_t *mdata, U64 frameNum, U32 *pCameraVendorId);
    S32 GetCameraTypeId(camera_metadata_t *mdata, U32 *pCameraTypeId);
    void GetLdcLevel(camera_metadata_t *pMeta, uint32_t *pLdcLevel);
    S32 GetCropRegion(camera_metadata_t *pMetadata, LdcProcessInputInfo *pInputInfo);
    S32 GetRotateAngle(camera_metadata_t *mdata, U64 frameNum, U32 *RotateAngle);
    S32 GetActiveArraySize(camera_metadata_t *pMetadata, LDCRECT *pActiveArraySize);
    S32 GetFaceInfo(camera_metadata_t *pMeta, const LdcProcessInputInfo &inputInfo,
                    LDCFACEINFO *pFaceInfo);
    S32 SetFaceInfo(camera_metadata_t *mdata, U64 frameNum, NODE_METADATA_MODE mode,
                    NODE_METADATA_FACE_INFO *pFaceInfo);
    S32 GetRoleTypeId(camera_metadata_t *mdata, U64 frameNum, U32 *pSensorId);
    S32 GetSensorSize(camera_metadata_t *mdata, MIRECT *pSensorSize);
    S32 GetZoomRatio(camera_metadata_t *pMetadata, float *pZoomRatio);
    S32 GetAppMode(camera_metadata_t *pMetadata, BOOL *pIsSATMode);
    S32 GetCaptureHint(camera_metadata_t *pMetadata, BOOL *pIsBurstShot);

private:
    S32 GetMetaData(camera_metadata_t *mdata, const char *pTagName, U32 tag, void **ppData,
                    U32 *pSize);
    S32 SetMetaData(camera_metadata_t *mdata, const char *pTagName, U32 tag, void *pData, U32 size);
    U32 m_mode;
    U32 m_debug;
};

} // namespace mialgo2

#endif //__NODE_METADATA_H__
