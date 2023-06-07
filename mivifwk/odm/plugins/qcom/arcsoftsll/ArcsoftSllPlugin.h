#ifndef _ARCSOFTSLLPLUGIN_H_
#define _ARCSOFTSLLPLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <pthread.h>
#include <system/camera_metadata.h>

#include <condition_variable> // condition_variable
#include <memory>
#include <mutex> // mutex, unique_lock
#include <queue>
#include <string>
#include <thread> // thread

#include "MiDebugUtils.h"
#include "MiaPluginUtils.h"
#include "MiaPluginWraper.h"
#include "arcsoftsuperlowlightraw.h"
#include "chistatsproperty.h"
#include "chituningmodeparam.h"
#include "chivendortag.h"

#define FRAME_MAX_INPUT_NUMBER     16
#define ARC_SN_CAMERA_MODE_UNKNOWN -1
#define INIT_EV_VALUE              0xFFFF

static const uint32_t SLLMaxRequestQueueDepth = 8;
static const uint32_t LSCTotalChannels = 4; ///< Total IFE channels R, Gr, Gb and B
static const uint32_t LSC_MESH_ROLLOFF_SIZE = 17 * 13;
static const int64_t maxSceneNum = 3;

static const uint32_t MinBLSValue                        =  1000;
static const uint32_t MaxBLSValue                        =  1050;
static const uint32_t defaultBLSValue                    =  1024;


using namespace std;

using ImageQueue = queue<ImageParams *>;

typedef enum PROCESSSTAT {
    INVALID,
    PROCESSINIT,
    PROCESSPRE,
    PROCESSPREWAIT,
    PROCESSRUN,
    PROCESSUINIT,
    PROCESSERROR,
    PROCESSBYPASS,
    PROCESSEND,
    EXIT,
} ProcessStat;

static const char *SLLProcStateStrings[] = {
    "INVALID",      "PROCESSINIT",  "PROCESSPRE",    "PROCESSPREWAIT", "PROCESSRUN",
    "PROCESSUINIT", "PROCESSERROR", "PROCESSBYPASS", "PROCESSEND",     "EXIT",
};

/// camera sensor role/type
typedef enum SensorRoleType {
    LENS_NONE = 0,  /// default camera
    LENS_FRONT = 1, /// Front camera
    LENS_ULTRA = 2, /// ultra wide camera
    LENS_WIDE = 3,  /// wide camera
    LENS_TELE = 4,  /// tele camera
} SENSORROLETYPE;

/// snapshot mode
typedef enum SnapshotMode {
    SM_NONE = 0,     /// default mode
    SM_NIGHT = 1,    /// sn in night mode
    SM_SE_SAT = 2,   /// se in sat mode
    SM_SE_BOKEN = 3, /// se in boken mode
} SNAPSHOTMODE;

static const char *SnapshotModeString[] = {
    "SM_NONE",
    "SM_NIGHT",    /// sn in night mode
    "SM_SE_SAT",   /// se in sat mode
    "SM_SE_BOKEN", /// se in boken mode
};

struct LSCDimensionCap
{
    int32_t width;  ///< Width
    int32_t height; ///< Height
};

/// Enumeration of the color filter pattern for RAW outputs
typedef enum ColorFilterPattern {
    Y,    ///< Monochrome pixel pattern.
    YUYV, ///< YUYV pixel pattern.
    YVYU, ///< YVYU pixel pattern.
    UYVY, ///< UYVY pixel pattern.
    VYUY, ///< VYUY pixel pattern.
    RGGB, ///< RGGB pixel pattern.
    GRBG, ///< GRBG pixel pattern.
    GBRG, ///< GBRG pixel pattern.
    BGGR, ///< BGGR pixel pattern.
    RGB   ///< RGB pixel pattern.
} ColorFilterPattern;

typedef enum SuperNightProMode {
    DefaultMode,
    SLLHandMode,
    SLLTripodMode,
    SLLLongHandMode,
    SLLLongTripodMode
} SUPERNIGHTPROMODE;

#ifdef MCAM_SW_DIAG
typedef struct DiffSceneInfo
{
    int32_t targetEv[16];

} DIFFSCENEINFO;

typedef struct EVInfoDiag
{
    int32_t lensPos;
    DiffSceneInfo sceneInfo[maxSceneNum];
} EVINFODIAG;

const EVINFODIAG expectedEVInfo[] = {{LENS_WIDE,
                                      {
                                          {-30, -12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                          {-30, -12, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6},
                                          {-36, -12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                      }},
                                     {LENS_TELE,
                                      {
                                          {-30, -12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                          {-30, -12, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6},
                                          {-36, -12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                      }},
                                     {LENS_ULTRA,
                                      {
                                          {-30, -12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                          {-30, -12, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6},
                                          {-36, -12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                      }}};
#endif

struct SuperNightDebugInfo
{
    int32_t nightMode;
    int32_t algoVersion;
    int32_t processTime;
    int32_t processRet;
    int32_t EV[20];
    int32_t dumpTime[2];
    int32_t initRet;
    int32_t correctImageRet;
    int32_t isOISSupport;
    int32_t isLSCControlEnable;
    ChiTuningModeParameter tuningParam;
};

class ArcsoftSllPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.arcsoftsll";
    }
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual void destroy();
    virtual bool isEnabled(MiaParams settings);
    void getFaceInfo(ARC_SN_FACEINFO *pFaceInfo, camera_metadata_t *metaData);
    uint32_t getRawType(uint32_t rawFormat);
    void getLSCDataInfo(ARC_SN_RAWINFO *rawInfo, camera_metadata_t *metaData);
    void mappingAWBGain(MInt32 i32RawType, AWBGainParams *pQcomAWBGain, MFloat *pArcAWBGain);
    void dumpBLSLSCParameter(const float *lensShadingMap);
    void dumpSdkLSCParameter(const float *lensShadingMap, int channel);
    void getCameraIdRole(camera_metadata_t *metaData);
    void getSensorSize(ChiRect *pSensorSize, camera_metadata_t *metaData);
    void setProperty();
    void debugFaceRoi(ImageParams pNodeBuff);
    void prepareImage(MiImageBuffer nodeBuff, ASVLOFFSCREEN &stImage, MInt32 &fd);
    void dumpRawInfoExt(ImageParams pNodeBuff, int index, int count, MInt32 i32SceneMode,
                        MInt32 i32CamMode, ARC_SN_FACEINFO *pFaceParams, ARC_SN_RAWINFO *rawInfo,
                        char *fileName);
    void getISOValueInfo(int *pISOValue, camera_metadata_t *metaData);
    void getActiveArraySize(ChiRect *pRect, camera_metadata_t *metaData);
    void getOrientation(MInt32 *pOrientation, camera_metadata_t *metaData);
    void updateCropRegionMetaData(MRECT *pCropRect, camera_metadata_t *metaData);
    void imageToMiBuffer(ImageParams *image, MiImageBuffer *miBuf);
    int cpyFrame(MiImageBuffer *dstFrame, MiImageBuffer *srcFrame);
    int processFrame(uint32_t inputFrameNum);
    void processRequestStat();
    bool dumpToFile(const char *fileName, struct MiImageBuffer *imageBuffer, uint32_t index,
                    const char *fileType);
    void getDumpFileName(char *fileName, size_t size, const char *fileType, const char *timeBuf);
    PROCESSSTAT getCurrentStat() const { return m_processStatus; }
    void setNextStat(const PROCESSSTAT &stat);
    int processAlgo();
    int processBuffer();
    void processByPass();
    void CopyRawInfo(ARC_SN_RAWINFO *rawInfo, ARC_SN_RAWINFO *rawInfoForProcess, int32_t inputNum);
    void getInputRawInfo(ImageParams *inputParam, camera_metadata_t *pMetaData,
                         ARC_SN_RAWINFO *rawInfo);
    void getMetaData(camera_metadata_t *pMetaData);
    void processUnintAlgo();
    void processInitAlgo();
    void processError();
    void getCurrentHWStatus();
    void resetSnapshotParam();
    void UpdateMetaData(ARC_SN_RAWINFO *rawInfo, int correctImageRet, int processRet);
    void preProcessInput();
    void getCropRegionLargeToSmall(ChiRect *cropRegion, const uint32_t refWidth,
                                   const uint32_t refHeight, const uint32_t targetWidth,
                                   const uint32_t targetHeight);
    void getCropRegionSmallToLarge(ChiRect *cropRegion, const uint32_t refWidth,
                                   const uint32_t refHeight, const uint32_t targetWidth,
                                   const uint32_t targetHeight);
    bool isSE()
    {
        return (m_snapshotMode == SNAPSHOTMODE::SM_SE_SAT) ||
               (m_snapshotMode == SNAPSHOTMODE::SM_SE_BOKEN);
    };
#ifdef MCAM_SW_DIAG
    void ParamDiag(camera_metadata_t *metaData);
    bool isLumaRatioMatched(float targetLumaRatio, float curLumaRatio);
#endif
    void getInputZoomRect(MRECT *pInputZoomRect, ChiRect *pFirstImageRect,
                          camera_metadata_t *metaData);
    void initAlgoParam();

private:
    ~ArcsoftSllPlugin();

    bool m_isOISSupport;
    bool m_isTripod;
    bool m_needUninitHTP;
    bool m_hasInitNormalInput;
    bool m_isOpenAlgoLog;
    std::atomic<bool> m_flushFlag;
    bool m_timeOutFlag;
    uint32_t m_sensorID;
    uint32_t m_dumpMask;
    uint32_t m_BypassForDebug;
    uint32_t m_FakeProcTime;
    uint32_t m_isCropInfoUpdate;
    uint32_t m_isDebugLog;
    uint32_t m_frameworkCameraId;
    uint32_t m_algoInputNum;
    uint32_t m_cameraRoleID; ///< camera lens
    uint32_t m_fwkCameraId;
    uint32_t m_dumpTime[2];
    int32_t m_camState;
    int32_t m_camStateForDebug;
    int32_t m_procRet;
    int32_t m_InitRet;
    int32_t m_imgCorrectRet;
    int32_t m_CamMode;
    int32_t m_SceneMode;
    int32_t m_superNightProMode;
    int32_t m_currentTemp;
    std::atomic<int32_t> m_InputIndex;
    int32_t m_currentCpuFreq1;
    int32_t m_currentCpuFreq4;
    int32_t m_currentCpuFreq7;
    int32_t m_maxCpuFreq1;
    int32_t m_maxCpuFreq4;
    int32_t m_maxCpuFreq7;

    int32_t m_devOrientation;
    int32_t m_outputFrameNum; ///<  output request frame num
    int32_t m_preEVDiff;
    uint64_t m_processedFrame; ///< The count for processed frame
    int64_t m_outputTimeStamp;

    float m_lensShadingMap[LSCTotalChannels *
                           LSC_MESH_ROLLOFF_SIZE]; ///< Lens Shading Map per channel
    double m_processTime;
    MRECT *m_pFaceRects;
    MRECT m_OutImgRect;
    MRECT m_InputZoomRect;  /// algo inutZoomRect,include:4:3,16:9
    ChiRect m_firstImgRect; ///  first imgRecy,only include: 4:3
    LSCDimensionCap mLSCDim;
    ImageFormat m_format;
    mutex m_algolock;
    ARC_SN_FACEINFO m_FaceParams;
    ColorFilterPattern m_rawPattern;
    SuperNightRawValue m_superNightRawValue;
    MiaNodeInterface mNodeInterface;
    camera_metadata_t *m_pStaticMetadata;
    ArcsoftSuperLowlightRaw *m_hSuperLowlightRaw;
    ChiTuningModeParameter m_tuningParam;
    MPBASE_Version m_algoVersion;
    ImageParams *m_normalInput;         ///< normal(EV0) input image param
    ARC_SN_RAWINFO m_normalRawInfo;     ///< normal(EV0) input image rawinfo
    ImageParams *m_closestEV0Input;     ///< closest(EV0) input image param
    ARC_SN_RAWINFO m_closestEV0RawInfo; ///< closest(EV0) input image rawinfo
    ImageParams *m_curInputImgParam;    ///< current input image param
    ImageParams *m_outputImgParam;      ///< output image param
    PROCESSSTAT m_processStatus;
    SnapshotMode m_snapshotMode;
    char m_dumpTimeBuf[128];
    ARC_SN_RAWINFO m_rawInfoCurFrame;
    int32_t m_inputEV[20];
    bool m_isSupportSat;
    std::mutex m_initMutex;
    std::mutex m_flushMutex;
#ifdef MCAM_SW_DIAG
    std::map<float, float> m_lumaValue;
#endif
};

PLUMA_INHERIT_PROVIDER(ArcsoftSllPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new ArcsoftSllPluginProvider());
    return true;
}

#endif // _ArcsoftSllPlugin_H_
