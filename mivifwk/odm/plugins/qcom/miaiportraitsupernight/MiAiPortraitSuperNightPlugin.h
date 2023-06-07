#ifndef _MIAIPORTRAITSUPERNIGHTPLUGIN_H_
#define _MIAIPORTRAITSUPERNIGHTPLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "MiaPluginUtils.h"
#include "chistatsproperty.h"
#include "chituningmodeparam.h"
#include "chivendortag.h"
#include "miaiportraitsupernight.h"
#define FRAME_MAX_INPUT_NUMBER 8

//普通变量转string
template <typename T>
static inline typename std::enable_if<!std::is_enum<T>::value, std::string>::type toStr(T n)
{
    return std::to_string(n);
}

template <typename T>
static std::string toStr(T n, int cut)
{
    std::string result = "";
    std::string tmp = std::to_string(n);
    bool has_dot = false;
    for (auto &c : tmp) {
        if (has_dot) {
            cut--;
        }
        if (cut >= 0) {
            result += c;
        }
        if (c == '.') {
            has_dot = true;
        }
    }
    return result;
}

class MiAiPortraitSuperNightPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.miaiportraitsupernight";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual void destroy();
    virtual bool isEnabled(MiaParams settings);
    // AEC static definitions
    static const uint8_t ExposureIndexShort = 0; ///< Exposure setting index for short exposure
    static const uint8_t ExposureIndexLong = 1;  ///< Exposure setting index for long exposure
    static const uint8_t ExposureIndexSafe =
        2; ///< Exposure setting index for safe exposure (between short and long)

private:
    static int32_t bracketEv[FRAME_MAX_INPUT_NUMBER];
    ~MiAiPortraitSuperNightPlugin();
    MiaNodeInterface mNodeInterface;
    int mProcessCount;
    ProcessRetStatus processBuffer(struct MiImageBuffer *input, int input_num,
                                   struct MiImageBuffer *output, camera_metadata_t *metaData,
                                   int bypass);
    void getAEInfo(camera_metadata_t *metaData, LPMIAI_LLHDR_AEINFO pAeInfo);
    MiAiSuperLowlight *mPortraitSupernight;
    int32_t isFrontnightEnabled;
    uint32_t mOutFrameIdx;
    int64_t mOutFrameTimeStamp;
};

PLUMA_INHERIT_PROVIDER(MiAiPortraitSuperNightPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiAiPortraitSuperNightPluginProvider());
    return true;
}

#endif // _MIAIPORTRAITSUPERNIGHTPLUGIN_H_