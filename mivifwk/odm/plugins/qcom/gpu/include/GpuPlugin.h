#ifndef _GPUPLUGIN_H_
#define _GPUPLUGIN_H_

#include <MiaPluginWraper.h>

#include <mutex>

#include "MiaPluginUtils.h"
#include "VendorMetadataParser.h"
#include "gpuopencl.h"

static const uint32_t ChiNodeCapsGPUFlip = 1 << 5;

class GpuPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.gpu";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface) override;

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo) override;

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo) override;

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo) override;

    virtual void destroy() override;
    ~GpuPlugin();

    virtual bool isEnabled(MiaParams settings);

private:
    void flipImage(std::vector<ImageParams> &hOutput, std::vector<ImageParams> &hInput,
                   FlipDirection targetFlip, RotationAngle currentOrientation);

    void copyImage(std::vector<ImageParams> &hOutput, std::vector<ImageParams> &hInput);

    void updateOrientation(camera_metadata_t *metaData);
    void updateFlip(camera_metadata_t *metaData);
    void doSwFlip(uint8_t *inputY, uint8_t *inputUV, uint8_t *outputY, uint8_t *outputUV, int width,
                  int height, int stride, int sliceheight, RotationAngle orient);

    uint32_t mNodeCaps; ///< The selected node caps
    RotationAngle
        mCurrentRotation; ///< If gpu rotation, use this to pass as metadata to dependant nodes
    FlipDirection mCurrentFlip; ///< If gpu flip, use this to pass as metadata to dependant nodes

    std::mutex mGpuNodeMutex; ///< GPU node mutex
    GPUOpenCL *mOpenCL;       ///< Local CL class instance pointer
};

PLUMA_INHERIT_PROVIDER(GpuPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new GpuPluginProvider());
    return true;
}

#endif // _GPUPLUGIN_H_
