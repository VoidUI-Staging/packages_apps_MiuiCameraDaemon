#ifndef __MIA_NODE_LDC_UP_H__
#define __MIA_NODE_LDC_UP_H__

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "ldcCommon.h"
#include "nodeMetaDataUp.h"
#if defined(SOCRATES_CAM)
#include "altekLdcCapture.h"
#else
#include "miLdcCapture.h"
#endif

namespace mialgo2 {

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
    // Preprocess enable condition: supportPreProcess && !isPreProcessed && isEnabled.
    // For now, preprocess needs to be executed for each frame.
    virtual bool isPreProcessed() { return false; };
    virtual bool supportPreProcess() { return false; };
    void UpdateExifInfo(uint32_t frameNum, int64_t timeStamp, int32_t procRet,
                        const LdcProcessInputInfo *processInfo);
    void TransformZoomWOI(LDCRECT *zoomWOI, LDCRECT *cropRegion, LDCRECT *sensorSize,
                          DEV_IMAGE_BUF *input, bool isSAT);
    void DumpProcInfo(const LdcProcessInputInfo *processInfo);
    S32 mergerDebugInfo(S32 debugLevel, S32 pluginDebugLevel)
    {
        return pluginDebugLevel > 0 ? (debugLevel | LDC_DEBUG_DUMP_IMAGE_CAPTURE) : debugLevel;
    }
    U32 m_debug = 0;
    U32 m_captureVendor = 0;
    U32 m_fwkCameraId = 0;
#if defined(SOCRATES_CAM)
    AltekLdcCapture *m_pAltekLdcCapture = nullptr;
#else
    MiLdcCapture *m_pMiLdcCapture = nullptr;
#endif
    NodeMetaData *m_pNodeMetaData = nullptr;
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
