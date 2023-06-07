#include "srplugin.h"

#include "MiaPerfManager.h"
#include "MiaPluginUtils.h"
#define FRAME_MAX_INPUT_NUMBER 10

using namespace mialgo2;

SrPlugin::SrPlugin()
{
    mProcessCount = 0;
}

SrPlugin::~SrPlugin()
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);
}

int SrPlugin::initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface)
{
    MLOGD(Mia2LogGroupPlugin, "%s:%d %p", __func__, __LINE__, this);

    setAdapterIndex(getIndexByName(DEFAULT_ALGO));
    sessionInfo.setCreatInfo(pCreateInfo);
    sessionInfo.setNodeInterface(&nodeInterface);
    return 0;
}

bool SrPlugin::isEnabled(MiaParams settings)
{
    void *pData = NULL;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.superResolution.enabled", &pData);
    int srEnable = pData ? *static_cast<int *>(pData) : false;
    MLOGD(Mia2LogGroupPlugin, "Get SrEnable: %d", srEnable);
    return srEnable;
}

ProcessRetStatus SrPlugin::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    MLOGI(Mia2LogGroupPlugin, "add perflock to boost Gpu when sr capture. ");
    std::shared_ptr<PluginPerfLockManager> boostGpu(new PluginPerfLockManager(5000, PerfLock_SR));

    MLOGI(Mia2LogGroupPlugin, "get request: %p", pProcessRequestInfo);
    process(&sessionInfo, pProcessRequestInfo);
    mProcessCount++;
    MLOGI(Mia2LogGroupPlugin, "finish %u", mProcessCount);

    return PROCSUCCESS;
}

ProcessRetStatus SrPlugin::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    return PROCSUCCESS;
}

int SrPlugin::flushRequest(FlushRequestInfo *pFlushRequestInfo)
{
    return 0;
}

void SrPlugin::destroy()
{
    MLOGD(Mia2LogGroupPlugin, "%p", this);
}
