#ifndef __MI_NODEMIBOKEH__
#define __MI_NODEMIBOKEH__

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "mibokehmiui.h"

/*for xiaomi portrait light*/
#include "mi_portrait_3d_lighting_image.h"
#include "mi_portrait_lighting_common.h"

namespace mialgo2 {

class Mibokehplugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.mibokeh";
    }
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual void destroy();
    virtual bool isEnabled(MiaParams settings);

private:
    ~Mibokehplugin();
    int processBuffer(MiImageBuffer *input, MiImageBuffer *outputBokeh, MiImageBuffer *outputDepth,
                      camera_metadata_t *metaData);
    void dumpRawData(uint8_t *data, int32_t size, uint32_t index, const char *namePrefix);
    void checkPersist();
    int miPrepareRelightImage(MiImageBuffer *src, MIIMAGEBUFFER *dst);
    void miReleaseRelightImage(MIIMAGEBUFFER *img);
    int miCopyRelightToMiImage(MIIMAGEBUFFER *src, MiImageBuffer *dst);
    int miProcessRelightingEffect(MiImageBuffer *input, MiImageBuffer *output);
    void processBokehOnly(int32_t iIsDumpData, ProcessRequestInfo *processRequestInfo);
    void processBokehDepthRaw(int32_t iIsDumpData, ProcessRequestInfo *processRequestInfo);

    MiBokehMiui *mNodeInstance;
    uint32_t mCurrentFlip; ///< If gpu flip, use this to pass as metadata to dependant nodes
    MHandle mRelightingHandle;

    int32_t mOrientation;
    int mOrient;
    int mProcessedFrame;
    float mNumberApplied;
    int mSceneFlag;

    /*begin add for front light*/
    bool mDumpYUV;
    bool mDumpPriv;
    int mLightingMode;
    int32_t mDepthSize;
    uint8_t *mDepthMap; // depthMap
    MiaNodeInterface mNodeInterface;
};

PLUMA_INHERIT_PROVIDER(Mibokehplugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(ProviderManager &host)
{
    host.add(new MibokehpluginProvider());
    return true;
}

} // namespace mialgo2
#endif //__MI_NODEMIBOKEH__
