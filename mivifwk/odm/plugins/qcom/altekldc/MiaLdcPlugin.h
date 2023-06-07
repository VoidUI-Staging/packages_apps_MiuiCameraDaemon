#ifndef __MIA_NODE_LDC_UP_H__
#define __MIA_NODE_LDC_UP_H__

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "ldcCaptureUp.h"
#include "ldcCommon.h"
#include "miBokehUp.h"
#include "nodeMetaDataUp.h"

namespace mialgo2 {

// typedef enum __NodeLdcMode {
//     NODE_LDC_MODE_CAPTURE = 2,
//     NODE_LDC_MODE_BYPASS = 5,
// } NODE_LDC_MODE;

// typedef enum {
//     LDC_LEVEL_DISABLED = (0) << (0),
//     LDC_LEVEL_ENABLE = (1) << (0),
// } LDC_LEVEL;

// typedef enum __NodeLdcDebug {
//     NODE_LDC_DEBUG_INFO = (1) << (0),
//     NODE_LDC_DEBUG_DUMP_IMAGE_PREVIEW = (1) << (1),
//     NODE_LDC_DEBUG_DUMP_IMAGE_CAPTURE = (1) << (2),
//     NODE_LDC_DEBUG_DUMP_TABLE = (1) << (3),
//     NODE_LDC_DEBUG_DIS_UPDATE_FACE = (1) << (4),
//     NODE_LDC_DEBUG_BYPASS_PREVIEW = (1) << (6),
//     NODE_LDC_DEBUG_BYPASS_CAPTURE = (1) << (7),
//     NODE_LDC_DEBUG_TEST_CAPTURE_ALTEK_AI = (1) << (8),
//     NODE_LDC_DEBUG_OUTPUT_DRAW_FACE_BOX = (1) << (12),
//     NODE_LDC_DEBUG_INPUT_DRAW_FACE_BOX = (1) << (13),
// } NODE_LDC_DEBUG;

class LDCPlugin : public PluginWraper
{
public:
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
    virtual int preProcess(PreProcessInfo preProcessInfo);

private:
    ~LDCPlugin();
    // preProcess enable condition: supportPreProcess && !isPreProcessed && isEnabled.
    // For now, preprocess needs to be executed for each frame.
    virtual bool isPreProcessed() { return false; };
    virtual bool supportPreProcess() { return true; };
    void UpdateExifInfo(uint32_t frameNum, int64_t timeStamp,
                        const LdcProcessInputInfo *processInfo);
    void TransformZoomWOI(LDCRECT *zoomWOI, LDCRECT *cropRegion, LDCRECT *sensorSize,
                          DEV_IMAGE_BUF *input, bool isSAT);
    void DumpProcInfo(const LdcProcessInputInfo *processInfo);
    S32 mergerDebugInfo(S32 debugLevel, S32 pluginDebugLevel)
    {
        return pluginDebugLevel > 0 ? (debugLevel | LDC_DEBUG_DUMP_IMAGE_CAPTURE) : debugLevel;
    }
    U32 m_debug;
    U32 m_processMode;
    U32 m_captureMode;
    U32 m_fwkCameraId;
    U32 m_ldcLevel;
    LdcCapture *m_pLdcCapture;
    NodeMetaData *m_pNodeMetaData;
    MiaNodeInterface m_nodeInterface;
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
