#ifndef _CHIOFFLINEPLUGIN_H_
#define _CHIOFFLINEPLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>

#include "chiofflinefeatureintf.h"
#include "chivendortag.h"

// Function pointers defined in "chiofflinepostprocintf.h" add by xiaomi, Decouple
// ChiOfflinePlugin and chiofflinepostproclib
typedef void *(*PCameraPostProc_Create)(PostProcCreateParams *pParams);

typedef PostProcRetStatus (*PCameraPostProc_PreProcess)(void *pPostProcInstance,
                                                        PostProcProcessParams *pSessionParams);

typedef PostProcRetStatus (*PCameraPostProc_Process)(void *pPostProcInstance,
                                                     PostProcProcessParams *pSessionParams);

typedef PostProcRetStatus (*PCameraPostProc_Flush)(void *pPostProcInstance, uint32_t flushFrame);

typedef void (*PCameraPostProc_ReleaseResources)(void *pPostProcInstance);

typedef void (*PCameraPostProc_Destroy)(void *pPostProcInstance);

struct ProcessMode
{
    PostProcMode postMode;
    PostProcModeType postModeType;
};

// Function pointers defined in "chiofflinepostprocintf.h" add by xiaomi
class ChiPostProcLib
{
public:
    static ChiPostProcLib *GetInstance() { return &mInstance; }

    ~ChiPostProcLib();
    void *create(PostProcCreateParams *pParams)
    {
        if (NULL == m_pCameraPostProcCreate) {
            MLOGE(Mia2LogGroupPlugin, "CameraPostProc_Create pointers are NULL");
            return NULL;
        }
        return m_pCameraPostProcCreate(pParams);
    }

    int preProcess(void *pPostProcInstance, PostProcProcessParams *pSessionParams)
    {
        if (NULL == m_pCameraPostProcPreProcess) {
            MLOGE(Mia2LogGroupPlugin, "CameraPostProc_PreProcess pointers are NULL");
            return -1;
        }
        return m_pCameraPostProcPreProcess(pPostProcInstance, pSessionParams);
    }

    PostProcRetStatus process(void *pPostProcInstance, PostProcProcessParams *pSessionParams)
    {
        if (NULL == m_pCameraPostProcProcess) {
            MLOGE(Mia2LogGroupPlugin, "CameraPostProc_Process pointers are NULL");
            return POSTPROCFAILED;
        }
        return m_pCameraPostProcProcess(pPostProcInstance, pSessionParams);
    }

    int flush(void *pPostProcInstance, uint32_t flushFrame)
    {
        if (NULL == m_pCameraPostProcFlush) {
            MLOGE(Mia2LogGroupPlugin, "CameraPostProc_Flush pointers are NULL");
            return -1;
        }
        return m_pCameraPostProcFlush(pPostProcInstance, flushFrame);
    }

    void releaseResources(void *pPostProcInstance)
    {
        if (NULL == m_pCameraPostProcRelease) {
            MLOGE(Mia2LogGroupPlugin, "CameraPostProc_ReleaseResources pointers are NULL");
            return;
        }
        return m_pCameraPostProcRelease(pPostProcInstance);
    }

    void destroy(void *pPostProcInstance)
    {
        if (NULL == m_pCameraPostProcDestroy) {
            MLOGE(Mia2LogGroupPlugin, "CameraPostProc_Destroy pointers are NULL");
            return;
        }
        return m_pCameraPostProcDestroy(pPostProcInstance);
    }

private:
    ChiPostProcLib();
    ChiPostProcLib(const ChiPostProcLib &) = delete;
    ChiPostProcLib operator=(const ChiPostProcLib &) = delete;

    static ChiPostProcLib mInstance;
    void *m_pChiPostProcLib;
    PCameraPostProc_Create m_pCameraPostProcCreate;
    PCameraPostProc_PreProcess m_pCameraPostProcPreProcess;
    PCameraPostProc_Process m_pCameraPostProcProcess;
    PCameraPostProc_Flush m_pCameraPostProcFlush;
    PCameraPostProc_ReleaseResources m_pCameraPostProcRelease;
    PCameraPostProc_Destroy m_pCameraPostProcDestroy;
};

ChiPostProcLib ChiPostProcLib::mInstance;

// Include functions: Yuv reprocess, Raw reprocess. Jpeg encode
class ChiOfflinePlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.offlinepostproc";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    // Use for offlinestats feature process
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

    virtual int preProcess(PreProcessInfo preProcessInfo);

private:
    ~ChiOfflinePlugin();
    virtual bool isPreProcessed();
    virtual bool supportPreProcess();

    int validateRequest(ProcessRequestInfo *request);
    void updateMatadata(PostProcBufferParams &nativeHandleParams, camera_metadata_t *mainMeta);
    void prepareInput(std::map<uint32_t, std::vector<ImageParams>> &inputBuffersMap,
                      PostProcProcessParams &processParams);
    void prepareOutput(std::map<uint32_t, std::vector<ImageParams>> &outputBuffersMap,
                       PostProcProcessParams &processParams);
    void getMatadataFillParams(PostProcBufferParams &nativeHandleParams);
    bool isSupperNightBokehEnabled(camera_metadata_t *pMeta);
    bool isMfsrEnabled(camera_metadata_t *pMeta);
    bool isMiviRawZslEnabled(camera_metadata_t *pMeta);
    bool isOfflineStatsMode() const;

    MiaNodeInterface mNodeInterface;
    int32_t mDumpData;
    void *mPostProc;
    uint32_t mLogicalCameraId;
    PostProcMode mProcessMode;
    PostProcModeType mProcessModeType;
    PostProcCreateParams mCreateParams;
    std::string mInstanceName;
    uint32_t mInpostProcessing;
    uint32_t mFlushFrameNum;

    float mMiviVersion;
    bool mInpreProcessing;
    std::mutex mMutex;
    std::condition_variable mCond;
};

PLUMA_INHERIT_PROVIDER(ChiOfflinePlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new ChiOfflinePluginProvider());
    return true;
}
#endif /// _CHIOFFLINEPLUGIN_H_
