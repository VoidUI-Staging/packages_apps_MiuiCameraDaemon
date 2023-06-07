#ifndef __MI_NODEMIBOKEH__
#define __MI_NODEMIBOKEH__

#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "mibokehmiui.h"
/*begin for add front portrait light*/
#include "arcsoft_portrait_lighting_common.h"
#include "arcsoft_portrait_lighting_image.h"
#include "asvloffscreen.h"
#include "merror.h"
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
    int processBuffer(MiImageBuffer *input, MiImageBuffer *output_bokeh,
                      MiImageBuffer *output_depth, camera_metadata_t *pMetaData);
    void prepareImage(MiImageBuffer *image, ASVLOFFSCREEN &stImage);
    void allocCopyImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage);
    void releaseImage(ASVLOFFSCREEN *pImage);
    void dumpYUVData(ASVLOFFSCREEN *pASLIn, uint32_t index, const char *namePrefix);
    void dumpRawData(uint8_t *pData, int32_t size, uint32_t index, const char *namePrefix);
    void copyImage(ASVLOFFSCREEN *pSrcImg, ASVLOFFSCREEN *pDstImg);
    void checkPersist();
    int processRelighting(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, ASVLOFFSCREEN *bokehImg);
    void processBokehOnly(int32_t iIsDumpData, ProcessRequestInfo *pProcessRequestInfo);
    void processBokehDepthRaw(int32_t iIsDumpData, ProcessRequestInfo *pProcessRequestInfo);
    int miPrepareRelightImage(MiImageBuffer *src, MIIMAGEBUFFER *dst);
    void miReleaseRelightImage(MIIMAGEBUFFER *img);
    int miCopyRelightToMiImage(MIIMAGEBUFFER *src, MiImageBuffer *dst);
    int miProcessRelightingEffect(MiImageBuffer *input, MiImageBuffer *output);

    MiBokehMiui *m_pNodeInstance;
    uint32_t m_currentFlip; ///< If gpu flip, use this to pass as metadata to dependant nodes
    MHandle m_hRelightingHandle;

    int32_t m_orientation;
    int m_orient;
    int m_processedFrame;
    float m_fNumberApplied;
    int m_sceneFlag;

    /*begin add for front light*/
    bool m_bDumpYUV;
    bool m_bDumpPriv;
    int m_lightingMode;
    int32_t m_depthSize;
    uint8_t *m_pDepthMap; // depthMap
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
