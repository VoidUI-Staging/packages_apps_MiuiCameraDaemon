#ifndef __MIA_NODE_LDC_UP_H__
#define __MIA_NODE_LDC_UP_H__

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "dev_image.h"
#include "ldcCaptureUp.h"
#include "nodeMetaDataUp.h"

//#define ENABLE_UW_W_FUSION_FUNC
//#undef ENABLE_UW_W_FUSION_FUNC

#ifdef ENABLE_UW_W_FUSION_FUNC
#include "FusionCapture.h"
#endif

namespace mialgo2 {

typedef enum __NodeLdcMode {
    NODE_LDC_MODE_PREVIEW = 0,
    NODE_LDC_MODE_CAPTURE = 2,
    NODE_LDC_MODE_BYPASS = 5,
} NODE_LDC_MODE;

typedef enum __NodeLdcType {
    NODE_LDC_TYPE_PREVIEW_DEF = 0,
    NODE_LDC_TYPE_CAPTURE_DEF = 0,
    NODE_LDC_TYPE_PREVIEW_ALTEK_DEF = 0,
    NODE_LDC_TYPE_CAPTURE_ALTEK_DEF = 0,
    NODE_LDC_TYPE_CAPTURE_ALTEK_AI = 5,
} NODE_LDC_TYPE;

typedef enum __NodeLdcLevel {
    NODE_LDC_LEVEL_DISABLED = (0) << (0),
    NODE_LDC_LEVEL_ENABLE = (1) << (0),
    NODE_LDC_LEVEL_CONTINUOUS_DIS = (0) << (1),
    NODE_LDC_LEVEL_CONTINUOUS_EN = (1) << (1),
} NODE_LDC_LEVEL;

typedef enum __NodeLdcDebug {
    NODE_LDC_DEBUG_INFO = (1) << (0),
    NODE_LDC_DEBUG_DUMP_IMAGE_PREVIEW = (1) << (1),
    NODE_LDC_DEBUG_DUMP_IMAGE_CAPTURE = (1) << (2),
    NODE_LDC_DEBUG_DUMP_TABLE = (1) << (3),
    NODE_LDC_DEBUG_DIS_UPDATE_FACE = (1) << (4),
    NODE_LDC_DEBUG_BYPASS_PREVIEW = (1) << (6),
    NODE_LDC_DEBUG_BYPASS_CAPTURE = (1) << (7),
    NODE_LDC_DEBUG_TEST_CAPTURE_ALTEK_AI = (1) << (8),
    NODE_LDC_DEBUG_OUTPUT_DRAW_FACE_BOX = (1) << (12),
    NODE_LDC_DEBUG_INPUT_DRAW_FACE_BOX = (1) << (13),
} NODE_LDC_DEBUG;

class LDCPlugin : public PluginWraper
{
public:
    LDCPlugin();
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.ldc";
    }
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual void destroy();
    virtual bool isEnabled(MiaParams settings);

private:
    ~LDCPlugin();
    void TransformZoomWOI(MIRECT &zoomWOI, MIRECT *cropRegion, DEV_IMAGE_BUF *input);
    void drawFaceBox(DEV_IMAGE_BUF *input, NODE_METADATA_FACE_INFO *pFaceInfo);
    void drawMask(DEV_IMAGE_BUF *buffer, float fx, float fy, float fw, float fh);
    U32 m_mode;
    U32 m_feature;
    U32 m_type;
    U32 m_debug;
    U64 m_processedFrame;
    LdcCapture *m_pLdcCapture;
    NodeMetaData *m_pNodeMetaData;
    MiaNodeInterface m_nodeInterface;
    U32 m_snapshotWidth;
    U32 m_snapshotHeight;
    U32 m_CameraId;
    S64 m_detectorCaptureId;
    U32 mLdcLevel;
    bool mInitialized;
    uint32_t m_local_operation_mode = StreamConfigModeSAT;
    bool m_debugFaceDrawGet;
#ifdef ENABLE_UW_W_FUSION_FUNC
    FusionCapture *mNodeInstanceCapture;
#endif
};

PLUMA_INHERIT_PROVIDER(LDCPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(ProviderManager &host)
{
    host.add(new LDCPluginProvider());
    return true;
}

} // namespace mialgo2
#endif //__MIA_NODE_LDC_UP_H__
