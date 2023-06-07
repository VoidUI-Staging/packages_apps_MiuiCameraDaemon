#ifndef _MIFUSION_H_
#define _MIFUSION_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include "chivendortag.h"
#include "optical_zoom_fusion.h"

#define CTB_INPUT_MAIN 0
#define CTB_INPUT_AUX  1
#define MAX_CAMERA_CNT 2
#define MAX_CALI_CNT   3

#define MI_EXAMPLE_OUTPUT_BUF_CNT 10
#define MI_SAT_DEFAULT_DISTANCE   200

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

typedef int32_t CDKResult;
enum {
    MIAS_OK = 0,
    MIAS_UNKNOWN_ERROR = (-2147483647 - 1), // int32_t_MIN value
    MIAS_INVALID_PARAM,
    MIAS_NO_MEM,
    MIAS_MAP_FAILED,
    MIAS_UNMAP_FAILED,
    MIAS_OPEN_FAILED,
    MIAS_INVALID_CALL,
    MIAS_UNABLE_LOAD
};

class MiFusionPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.fusion";
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
    CDKResult setAlgoInputImages(MiImageBuffer *inputMaster, MiImageBuffer *inputSlave);
    CDKResult setAlgoInputParams(MiImageBuffer *inputMaster, MiImageBuffer *inputSlave,
                                 camera_metadata_t *pMeta);
    CDKResult iniEngine();
    CDKResult fillInputMetadata(InputMetadataOpticalZoom *satCameraMetadata,
                                camera_metadata_t *pMeta);
    wa::Image prepareImage(MiImageBuffer *imageInput);
    ChiRectINT covCropInfoAcordAspectRatio(ChiRectINT cropRegion, ImageFormat imageFormat,
                                           ChiRect activeSize, bool isQcfaSensor);
    void mapRectToInputData(ChiRect &rtSrcRect, ChiRect activeSize, ChiRectINT &asptCropRegion,
                            ImageShiftData shifData, ImageFormat imageFormat,
                            ChiRect &rtDstRectOnInputData);

    void writeFileLog(char *file, const char *fmt, ...);

    wa::OpticalZoomFusion *m_pFusionInstance;
    wa::Image::Rotation m_orient;
    camera_metadata_t *m_pMasterPhyMetadata;
    camera_metadata_t *m_pSlaverPhyMetadata;
    FusionPipelineType m_fusionType = PipelineTypeUWW;
    MiaNodeInterface m_fusionNodeInterface;
    void *m_pCaliData[MAX_CALI_CNT] = {0};
    void *m_pIntsenceData = NULL;
    int m_processedFrame;
    int m_algoRunResult;
    int32_t m_debugDumpImage;
    float m_debugMasterScale;
    float m_debugSlaverScale;
    int32_t m_iszStatus;
};

PLUMA_INHERIT_PROVIDER(MiFusionPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiFusionPluginProvider());
    return true;
}

#endif //_MIFUSION_H_