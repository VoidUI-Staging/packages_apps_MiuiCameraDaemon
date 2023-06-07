#ifndef _DEPURPLEPLUGIN_H_
#define _DEPURPLEPLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "altekdepurple.h"
#include "arcsoft_utils.h"
#include "chioemvendortag.h"
#include "chivendortag.h"

class DepurplePlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.depurple";
    }
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

    virtual int preProcess(PreProcessInfo preProcessInfo);

private:
    ~DepurplePlugin();
    virtual bool isPreProcessed() { return false; };
    virtual bool supportPreProcess() { return true; };
    int getVendorId(INT32 sendorId);
    int getSensorId(camera_metadata_t *metaData, int cameraAppId);
#if defined(NUWA_CAM) || defined(FUXI_CAM) || defined(SOCRATES_CAM)
    void getZoomInfo(alCFR_RECT *zoomInfo, CHIRECT *cropRegion, camera_metadata_t *metaData);
#else
    void getZoomInfo(alGE_RECT *zoomInfo, CHIRECT *cropRegion, camera_metadata_t *metaData);
#endif
    void getFaceInfo(alCFR_FACE_T *faceInfo, CHIRECT *cropRegion, camera_metadata_t *metaData);
    int getLuxIndex(camera_metadata_t *metaData);
    bool getSNMode(camera_metadata_t *metaData);
    bool getHDRMode(camera_metadata_t *metaData);
    uint32_t getZoomRatio(camera_metadata_t *metaData);
    bool getHighDefinitionMode(camera_metadata_t *metaData);
    void GetActiveArraySize(camera_metadata_t *metaData, ChiDimension *pActiveArraySize);
    void scaleCoordinates(int32_t *coordPairs, int coordCount, float scaleRatio);
    void prepareImage(MiImageBuffer *image, ASVLOFFSCREEN &stImage);
    void drawFaceRect(MiImageBuffer *image, alCFR_FACE_T *faceInfo);
#if defined(NUWA_CAM) || defined(FUXI_CAM) || defined(SOCRATES_CAM)
    void UpdateExifData(uint32_t frameNum, int64_t timeStamp, int lux, alCFR_RECT *zoomInfo,
                        int liteEnable);
#else
    void UpdateExifData(uint32_t frameNum, int64_t timeStamp, int lux, alGE_RECT *zoomInfo,
                        int liteEnable);
#endif

    MiaNodeInterface mNodeInterface;
    QAltekDepurple *mNodeInstance;
    CFR_MODEINFO_T mCFRModeInfo;
    int mOperationMode;
    uint32_t mWidth;
    uint32_t mHeight;
    const int mZSLmask = CAM_COMBMODE_FRONT;
    int mProcessCount;
    int mFrameworkID;
    int mDrawFaceRect;
    int mDumpData;
    bool mLiteEnable; // ONLY L2/L3 use
    bool mPreprocess;
    ChiDimension mActiveSize;
};

PLUMA_INHERIT_PROVIDER(DepurplePlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(ProviderManager &host)
{
    host.add(new DepurplePluginProvider());
    return true;
}

#endif //_DEPURPLEPLUGIN_H_
