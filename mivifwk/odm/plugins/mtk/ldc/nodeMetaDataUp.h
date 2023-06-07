#ifndef __NODE_METADATA_H__
#define __NODE_METADATA_H__
#include "dev_type.h"
// #include <system/camera_metadata.h>
// #include "mialgometadata.h"
// #include "device.h"
#include "MiaMetadataUtils.h"
#include "MiaPostProcType.h"
#include "VendorMetadataParser.h"
namespace mialgo2 {

#define NODE_METADATA_FACE_INFO_NUM_MAX (10)

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
    S32 GetCameraVendorId(void *mdata, U32 *pCameraVendorId);
    S32 SetCameraVendorId(void *mdata, U64 frameNum, U32 *pCameraVendorId);
    S32 GetCameraTypeId(void *mdata, U32 *pCameraTypeId);
    S32 GetLdcLevel(void *mdata, U64 frameNum, U32 *pLdcLevel);
    S32 GetCropRegion(void *mdata, U64 frameNum, MIRECT *pCropRegion, uint32_t mode);
    S32 GetCropRegionRef(void *mdata, U64 frameNum, MIRECT *pCropRegionRef);
    S32 GetRotateAngle(void *mdata, U64 frameNum, U32 *RotateAngle, NODE_METADATA_MODE mode);
    S32 GetActiveArraySize(void *mdata, MIRECT *pActiveArraySize);
    S32 GetFaceInfo(void *mdata, U64 frameNum, NODE_METADATA_MODE mode,
                    NODE_METADATA_FACE_INFO *pFaceInfo, MIRECT cropRegin, MIRECT sensorSize);
    S32 SetFaceInfo(void *mdata, U64 frameNum, NODE_METADATA_MODE mode,
                    NODE_METADATA_FACE_INFO *pFaceInfo);
    S32 GetQcfa(void *mdata, U64 frameNum, U32 *pQcfa);
    S32 GetRoleTypeId(void *mdata, U64 frameNum, U32 *pSensorId);
    S32 GetSensorSize(void *mdata, MIRECT *pSensorSize);
    S32 GetZoomRatio(void *mdata, float *pZoomRatio);

private:
    S32 GetMetaData(void *mdata, const char *pTagName, U32 tag, void **ppData, U32 *pSize);
    S32 SetMetaData(void *mdata, const char *pTagName, U32 tag, void *pData, U32 size);
    U32 m_mode;
    U32 m_debug;
};

} // namespace mialgo2

#endif //__NODE_METADATA_H__
