#ifndef _ANCHORSYNC_PLUGIN_
#define _ANCHORSYNC_PLUGIN_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>

#include "MiaPluginUtils.h"
#include "chivendortag.h"
#include "mianchorsync.h"

using namespace mialgo2;

class AnchorsyncPlugin : public PluginWraper
{
public:

    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.anchor";
    }

    virtual int initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *pProcessRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *pProcessRequestInfo);

    virtual int flushRequest(FlushRequestInfo *pFlushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

    virtual int preProcess(PreProcessInfo preProcessInfo);

    virtual bool isPreProcessed() { return m_pSharpnessMiAlgo != nullptr; };

    virtual bool supportPreProcess() { return true; };

private:
    ~AnchorsyncPlugin();
    void PrepareAnchorInfo(ProcessRequestInfoV2 *pProcessRequestInfo,
                           AnchorParamInfo *pAnchorParamInfo);
    void AnchorPickLatencyBased(AnchorParamInfo *pAnchorParamInfo);
    void AnchorPickSharpnessBased(AnchorParamInfo *pAnchorParamInfo);
    void AnchorPickSharpnessAndLuma(AnchorParamInfo *pAnchorParamInfo);
    void DumpFaceInfo(AnchorParamInfo *pAnchorParamInfo) const;

    SharpnessMiAlgo *m_pSharpnessMiAlgo;
    MiaNodeInterface m_interface;
    int32_t m_dumpData;
    int32_t m_dumpFaceInfo;
};
PLUMA_INHERIT_PROVIDER(AnchorsyncPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new AnchorsyncPluginProvider());
    return true;
}
#endif // _ANCHORSYNC_PLUGIN_
