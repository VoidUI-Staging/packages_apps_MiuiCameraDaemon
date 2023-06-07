#ifndef _SUPERNIGHTPLUGIN_H_
#define _SUPERNIGHTPLUGIN_H_

#include <MiaMetadataUtils.h>
#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>
#include <xiaomi/MiMetaData.h>

#include "MiaPluginUtils.h"
#include "miaiportraitsupernight.h"

#define FRAME_MAX_INPUT_NUMBER 8

class SuperNightPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.supernight";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface) override;

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *pProcessRequestInfo) override;

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *pProcessRequestInfo) override;

    virtual int flushRequest(FlushRequestInfo *pFlushRequestInfo) override;

    virtual void destroy() override;

    virtual bool isEnabled(MiaParams setting) override;
    // AEC static definitions
    static const uint8_t ExposureIndexShort = 0; ///< Exposure setting index for short exposure
    static const uint8_t ExposureIndexLong = 1;  ///< Exposure setting index for long exposure
    static const uint8_t ExposureIndexSafe =
        2; ///< Exposure setting index for safe exposure (between short and long)

private:
    int32_t bracketEv[FRAME_MAX_INPUT_NUMBER];
    ~SuperNightPlugin();
    int mProcessCount;
    ProcessRetStatus ProcessBuffer(struct MiImageBuffer *input, int input_num,
                                   struct MiImageBuffer *output, camera_metadata_t *metaData);
    void GetAEInfo(camera_metadata_t *metaData, LPMIAI_LLHDR_AEINFO pAeInfo);
    MiAiSuperLowlight *m_hPortraitSupernight;
};

PLUMA_INHERIT_PROVIDER(SuperNightPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new SuperNightPluginProvider());
    return true;
}

#endif // _SUPERNIGHTPLUGIN_H_
