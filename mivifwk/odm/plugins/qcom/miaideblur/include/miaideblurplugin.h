#ifndef _MIAI_DEBLUR_PLUGIN_H_
#define _MIAI_DEBLUR_PLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include <mutex>
#include <thread>

#include "MiaPluginUtils.h"
#include "abstypes.h"
#include "miai_buffer.h"
#include "miai_deblur.h"

struct BeautyFeatureInfo
{
    const char *tagName;
    int featureKey;
    bool featureMask;
    int featureValue;
};

struct CameraFrameRatio
{
    int frameIndex;
};

struct CameraSceneDetectResult
{
    float confidence;
    int resultFlag;
};

struct MIRECT
{
    uint32_t left;   ///< x coordinate for top-left point
    uint32_t top;    ///< y coordinate for top-left point
    uint32_t width;  ///< width
    uint32_t height; ///< height
};

typedef enum {
    DEBLUR_ASD_SCENE_NOTUSED  = 0,
    DEBLUR_ASD_SCENE_USED     = 1,
} DEBLUR_ASD_SCENE;

class MiAIDeblurPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.miaideblur";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual void destroy();
    virtual bool isEnabled(MiaParams settings);

private:
    ~MiAIDeblurPlugin();
    mia_deblur::MIAIDeblurParams mDeblurParams;
    char GetOriention(camera_metadata_t *metaData);
    bool NeedBypassRequest(camera_metadata_t *metaData);
    void getFaceInfo(camera_metadata_t *metaData, mia_deblur::FaceInfo *faceInfo, int w, int h);
    uint32_t mFrameworkCameraId;
    bool initFlag = false;
    MiaNodeInterface mNodeInterface;
    bool mLiteEnable = false;
    bool mUseAsdMeta = false;
};

PLUMA_INHERIT_PROVIDER(MiAIDeblurPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiAIDeblurPluginProvider());
    return true;
}

#endif // _MIAI_DEBLUR_LUGIN_H_