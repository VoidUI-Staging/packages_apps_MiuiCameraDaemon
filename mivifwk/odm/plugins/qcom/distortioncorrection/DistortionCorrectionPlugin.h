#ifndef __MI_NODEARCSOFTDISTORTION__
#define __MI_NODEARCSOFTDISTORTION__

#include <MiaPluginWraper.h>

#include "arcsoftdistortioncorrection.h"

namespace mialgo2 {

class ArcDCPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.dc";
    }
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual void destroy();
    virtual bool isEnabled(MiaParams settings);

private:
    ProcessRetStatus processBuffer(MiImageBuffer *input, MiImageBuffer *output,
                                   camera_metadata_t *metaData);
    void getMetaData(camera_metadata_t *metaData);
    void updateExifData(ProcessRequestInfo *processRequestInfo, ProcessRetStatus resultInfo,
                        double processTime);
    ~ArcDCPlugin();

    uint64_t mProcessedFrame; ///< The count for processed frame
    ArcsoftDistortionCorrection *mDistortionCorrection;
    uint32_t mRuntimeMode;
    ARC_DC_FACE mFaceParams;
    int mFaceOrientation;
    bool mInitialized;

    uint32_t mFaceNum;
    uint32_t mSnapshotWidth;
    uint32_t mSnapshotHeight;
    uint32_t mSensorWidth;
    uint32_t mSensorHeight;
    uint32_t mDistortionLevel;
    MiaNodeInterface mNodeInterface;
    std::string mInstanceName;
    int mInitReady;
    int mCamMode;
    MInt32 mDsMode;
    int mDebug;
    int mFrameworkCameraId;
    bool mMemcopy;
};

PLUMA_INHERIT_PROVIDER(ArcDCPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(ProviderManager &host)
{
    host.add(new ArcDCPluginProvider());
    return true;
}

} // namespace mialgo2
#endif //__MI_NODEARCSOFTDISTORTION__