#ifndef _MIFUSION_H_
#define _MIFUSION_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "chivendortag.h"
#include "dualcam_fusion_proc.h"

using namespace mialgo2;

#define MAX_CAMERA_CNT          2
#define MAX_CALI_CNT            3
#define MI_SAT_DEFAULT_DISTANCE 200

typedef int32_t CDKResult;

typedef enum {
    CALIB_TYPE_W_U = 0,                  ///< wide + ultra
    CALIB_TYPE_W_T,                      ///< wide + tele
    CALIB_TYPE_T_P,                      ///< tele + periscope
    CALIB_TYPE_W_P,                      ///< wide + periscope
    CALIB_TYPE_MAX = CALIB_TYPE_W_P + 1, ///< extend
} CalibDataType;

struct CaliConfigInfo
{
    std::string path;
    CalibDataType type;
} CALICONFIGINFO;

enum class MiAFState {
    Inactive,               ///< AF is not active
    PassiveScan,            ///< Algorithm is in CAF mode and is focusing
    PassiveFocused,         ///< Algorithm is in CAF mode and is focused
    ActiveScan,             ///< Algorithm is doing a scan initiated by App request
    FocusedLocked,          ///< Lens is locked and image is in Focus
    NotFocusedLocked,       ///< Lens is locked and image is in not in focus
    PassiveUnfocused,       ///< Algorithm is in CAF mode but not in focus
    PassiveScanWithTrigger, ///< Algorithm is in CAF mode and is focusing and App has
                            ///  initiated a trigger
    Invalid,                ///< invalid AF state
    Max = Invalid           ///< max AF state
};

typedef struct DualCameraMeta
{
    uint32_t cameraId;           ///< Camera Id this metadata belongs to
    uint32_t masterCameraId;     ///< Master camera Id
    uint32_t afState;            ///< AF state
    int32_t focusDistCm;         ///< Focus distance in cm
    int32_t isQuadCFASensor;     ///< is QCFA Sensor
    int32_t qcfaSensorSumFactor; ///< Some Qcfa sensor is 2*2 TetraCell,
                                 ///< However Someone is 3*3 NonaCell
    WeightedRegion afFocusROI;   ///< AF focus ROI
    ChiRectINT userCropRegion;   ///< Overall user crop region
    ChiRect fovRectIFE;          ///< IFE FOV wrt to active sensor array
    ChiRect activeArraySize;     ///< Wide sensor active array size
    float lux;                   ///< LUX value
} DUALCAMERAMETA;

typedef struct MiFusionInputMeta
{
    int32_t frameId;      ///< Frame ID
    uint32_t satDistance; ///< Current distance from SAT
    DUALCAMERAMETA cameraMeta[MAX_CAMERA_CNT];
    float currentZoomRatio;    ///< Current zoom ratio value
    ImageShiftData pixelShift; ///< Pixel shift
} MIFUSIONINPUTMETA;

class MiFusionPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.mifusion";
    }

    virtual int initialize(CreateInfo *pCreateInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *pProcessRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *pProcessRequestInfo);

    virtual int flushRequest(FlushRequestInfo *pFlushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);

private:
    ~MiFusionPlugin();
    CDKResult processBuffer(MiImageBuffer *masterImage, MiImageBuffer *slaveImage,
                            MiImageBuffer *outputFusion, camera_metadata_t *pMetaData);
    CDKResult setFusionParams(MiImageBuffer *inputMaster, MiImageBuffer *inputSlave,
                              camera_metadata_t *pMeta);
    CDKResult fillInputMeta(MiFusionInputMeta *fusionInputMeta, camera_metadata_t *pMeta);
    AlgoFusionImg conversionImage(MiImageBuffer *imageInput);
    ChiRectINT covCropInfoAcordAspectRatio(ChiRectINT cropRegion, ImageFormat imageFormat,
                                           ChiRect activeSize, bool isQcfaSensor);
    void mapRectToInputData(ChiRect &rtSrcRect, ChiRect activeSize, ChiRectINT &asptCropRegion,
                            ImageShiftData shifData, ImageFormat imageFormat,
                            ChiRect &rtDstRectOnInputData);

    void writeFileLog(char *file, const char *fmt, ...);

    void *m_pFusionHandle;
    camera_metadata_t *m_pMasterPhyMetadata;
    camera_metadata_t *m_pSlaverPhyMetadata;
    MiaNodeInterface m_fusionNodeInterface;
    void *m_pCaliData[MAX_CALI_CNT] = {0};
    int m_processedFrame;
    int m_algoRunResult;
    int32_t m_debugDumpImage;
    float m_masterScale;
    float m_slaverScale;
};

PLUMA_INHERIT_PROVIDER(MiFusionPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiFusionPluginProvider());
    return true;
}

#endif //_MIFUSION_H_