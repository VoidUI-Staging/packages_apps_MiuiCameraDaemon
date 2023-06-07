#ifndef _ARCSOFTSRCLIENT_H_
#define _ARCSOFTSRCLIENT_H_
#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <MiaPluginWraper.h>
#include <string.h>
#include <system/camera_metadata.h>
#include <utils/Log.h>

#include <vector>

#include "amcomdef.h"
#include "arcsoft_mf_superresolution.h"

using namespace std;

namespace mialgo2 {
class ArcSoftSR
{
public:
    ArcSoftSR(uint32_t width, uint32_t height);

    virtual ~ArcSoftSR();
    int ProcessBuffer(struct MiImageBuffer *input, int input_num, struct MiImageBuffer *output,
                      camera_metadata_t **metaData, camera_metadata_t **physMeta);
    void setOffScreen(ASVLOFFSCREEN *pImg, struct mialgo2::MiImageBuffer *miBuf);
    MInt32 getCameraType(camera_metadata_t *metaData);
    void getMetadata(camera_metadata_t **inputmetadata, camera_metadata_t **inputphymeta,
                     struct MiImageBuffer *input, int input_num, ARC_MFSR_META_DATA *Metadata,
                     MRECT *Cropregion, MInt32 *RefIndex, LPARC_MFSR_FACEINFO Faceinfo);
    void getFaceInfo(camera_metadata_t *metaData, LPARC_MFSR_FACEINFO Faceinfo);
    int getCropRegion(camera_metadata_t *metaData, struct MiImageBuffer *input, MRECT *arsoftCrop);

private:
    MHandle m_hArcSoftSRAlgo;
    uint32_t m_imageWidth;
    uint32_t m_imageHeight;
    float m_userZoomRatio;
    uint32_t m_cameraId;
    char cameraTypeName[16] = {0};
};

} // namespace mialgo2
#endif
