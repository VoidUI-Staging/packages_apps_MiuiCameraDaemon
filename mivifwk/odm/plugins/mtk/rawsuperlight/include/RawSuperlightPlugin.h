#ifndef _RAWSUPERLIGHTPLUGIN_H_
#define _RAWSUPERLIGHTPLUGIN_H_

#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <MiaPluginWraper.h>
#include <MiaPostProcType.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>
#include <xiaomi/MiMetaData.h>

#include "arcsoftrawsuperlight.h"

#define FRAME_MAX_INPUT_NUMBER 14
#define MAX_BLACK_LEVEL_ROW    18
#define MAX_BLACK_LEVEL_COLUMN 5

#define INIT_VALUE          33
#define RET_ADSP_INIT_ERROR 1002
#define RET_XIOMI_TIME_OUT  0xFFFF

static const uint32_t LSCTotalChannels = 4; ///< Total IFE channels R, Gr, Gb and B
static const uint32_t LSC_MESH_ROLLOFF_SIZE = 17 * 13;

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
struct LSCDimensionCap
{
    int32_t width;  ///< Width
    int32_t height; ///< Height
};

struct BlackLevel
{
    MInt32 gkey;
    MInt32 r;
    MInt32 gr;
    MInt32 gb;
    MInt32 b;
};

static BlackLevel BlackLevelTable[MAX_BLACK_LEVEL_ROW] = {0};

static BlackLevel BlackLevelTableWideMatisse[] = {
    {1, 256, 256, 256, 256}, // Default
    {2, 256, 256, 256, 256},  {8, 256, 256, 256, 256},  {16, 256, 256, 256, 256},
    {32, 256, 256, 256, 256}, {64, 256, 256, 256, 256}, {73, 256, 256, 256, 256},
};

static BlackLevel BlackLevelTableWideMatisseSE[] = {
    {0, 256, 256, 256, 256}, // Default
    {8, 256, 256, 256, 256},  {16, 256, 256, 256, 256}, {24, 256, 256, 256, 256},
    {32, 256, 256, 256, 256}, {40, 256, 256, 256, 256}, {48, 256, 256, 256, 256},
    {62, 256, 256, 256, 256}, {73, 256, 256, 256, 256},
};

static BlackLevel BlackLevelTableUW[] = {
    {0, 256, 256, 256, 256}, // Default
    {8, 256, 256, 256, 256},  {16, 256, 256, 256, 256}, {20, 256, 256, 256, 256},
    {24, 256, 256, 256, 256}, {28, 256, 256, 256, 256}, {32, 256, 256, 256, 256},
    {36, 256, 256, 256, 256}, {42, 257, 257, 257, 257},
};

static BlackLevel BlackLevelTableWideRubens[] = {
    {0, 256, 256, 256, 256},   {40, 256, 256, 256, 256},  {48, 256, 256, 256, 256},
    {56, 256, 256, 256, 256},  {64, 256, 256, 256, 256},  {72, 256, 256, 256, 256},
    {80, 256, 256, 256, 256},  {88, 256, 256, 256, 256},  {96, 256, 256, 256, 256},
    {104, 256, 256, 256, 256}, {112, 256, 256, 256, 256}, {120, 256, 256, 256, 256},
};

static BlackLevel BlackLevelTableWideRubensSE[] = {
    {0, 256, 256, 256, 256},   {40, 256, 256, 256, 256},  {48, 256, 256, 256, 256},
    {56, 256, 256, 256, 256},  {64, 256, 256, 256, 256},  {72, 256, 256, 256, 256},
    {80, 256, 256, 256, 256},  {88, 256, 256, 256, 256},  {96, 256, 256, 256, 256},
    {104, 256, 256, 256, 256}, {112, 256, 256, 256, 256}, {120, 256, 256, 256, 256},
};

static BlackLevel BlackLevelTableUWDaumierSNSE[] = {
    {0, 256, 256, 256, 256}, // Default
    {8, 256, 256, 256, 256},  {16, 256, 256, 256, 256}, {24, 256, 256, 256, 256},
    {32, 256, 256, 256, 256}, {40, 256, 256, 256, 256}, {48, 256, 256, 256, 256},
    {64, 256, 256, 256, 256},
};

typedef enum MiaColorFilterPattern {
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
} MiaColorFilterPattern;

typedef struct MiaRawFormat
{
    int32_t bitsPerPixel; ///< Bits per pixel.
    uint32_t stride;      ///< Stride in bytes.
    uint32_t sliceHeight; ///< The number of lines in the plane which can be equal to or larger than
                          ///< actual
                          ///  frame height.
    MiaColorFilterPattern colorFilterPattern; ///< Color filter pattern of the RAW format.
} MIARAWFORMAT;

enum class MiaModeType {
    Default = 0,
    // transplant from qcom platform: chituningmodeparam.h
    /*
    Sensor   = 1,
    Usecase  = 2,
    Feature1 = 3,
    Feature2 = 4,
    Scene    = 5,
    Effect   = 6
    */
};

union MiaModeSubModeType {
    uint32_t value;
    /*
    ChiModeUsecaseSubModeType  usecase;
    ChiModeFeature1SubModeType feature1;
    ChiModeFeature2SubModeType feature2;
    ChiModeSceneSubModeType    scene;
    ChiModeEffectSubModeType   effect;
    */
};

static const uint32_t MaxTuningMode = 7;

struct MiaTuningMode
{
    MiaModeType mode;
    MiaModeSubModeType subMode;
};

struct MiaTuningModeParameter
{
    uint32_t noOfSelectionParameter;
    MiaTuningMode TuningMode[MaxTuningMode];
};

class RawSuperlightPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.rawsuperlight";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings);
    // AEC static definitions
    static const uint8_t ExposureIndexShort = 0; ///< Exposure setting index for short exposure
    static const uint8_t ExposureIndexLong = 1;  ///< Exposure setting index for long exposure
    static const uint8_t ExposureIndexSafe =
        2; ///< Exposure setting index for safe exposure (between short and long)

private:
    ~RawSuperlightPlugin();
    ProcessRetStatus processBuffer(struct MiImageBuffer *input, int input_num,
                                   struct MiImageBuffer *output, camera_metadata_t **metaData,
                                   int32_t *inFD, int32_t outFD);
    void getRawInfo(camera_metadata_t *meta, MIARAWFORMAT *rawFormat, ARC_SN_RAWINFO *rawInfo,
                    int32_t index);

    void getBlackLevel(float totalGain, BlackLevel &blackLevel);

    int32_t getRawType(MIARAWFORMAT *pMiaRawFormat);

    void getFaceInfo(camera_metadata_t *meta, ARC_SN_FACEINFO *pFaceInfo);

    int32_t getSensorSize(camera_metadata_t *meta, MiDimension *pSensorSize);

    void prepareImage(struct MiImageBuffer *pNodeBuff, ASVLOFFSCREEN &stImage, int32_t &fd);

    void dumpImageData(struct MiImageBuffer *pNodeBuff, uint32_t index, const char *namePrefix);

    void dumpRawInfoExt(struct MiImageBuffer *pNodeBuff, int index, int count, int32_t i32SceneMode,
                        int32_t i32CamMode, ARC_SN_FACEINFO *pFaceParams, ARC_SN_RAWINFO *rawInfo,
                        char *namePrefix);

    void getISOValueInfo(camera_metadata_t *meta, int32_t *pISOValue);

    void copyImage(struct MiImageBuffer *dst, struct MiImageBuffer *src);

    void updateCropRegionMetaData(camera_metadata_t *meta, MRECT *pCropRect);

    void getLSCDataInfo(camera_metadata_t *meta, ARC_SN_RAWINFO *rawInfo);

    void dumpBLSLSCParameter(const float *lensShadingMap);

    void dumpSdkLSCParameter(const float *lensShadingMap, int32_t channel);

    void awbGainMapping(int32_t i32RawType, MFloat *pArcAWBGain);

    void getDumpFileName(char *fileName, size_t size, const char *fileType, const char *timeBuf);

    PROCESSSTAT getCurrentStat() const { return m_processStatus; }

    void setNextStat(const PROCESSSTAT &stat);

    int processAlgo();

    int processBuffer();

    void processByPass();

    void processUnintAlgo();

    void processInitAlgo();

    void processError();

    void processRequestStat();

    void resetSnapshotParam();

    void preProcessInput();

    void imageToMiBuffer(ImageParams *image, MiImageBuffer *miBuf);

    void copyAvailableBuffer();

private:
    int32_t bracketEv[FRAME_MAX_INPUT_NUMBER];
    int mProcessCount;
    ArcsoftRawSuperlight *m_hArcsoftRawSuperlight;
    bool m_bFrontCamera;
    uint32_t m_processedFrame;
    ARC_SN_FACEINFO m_FaceParams[FRAME_MAX_INPUT_NUMBER];
    MRECT m_OutImgRect;
    int32_t m_camState;
    MIRECT m_sensorActiveArray;
    uint32_t mCamMode;
    uint32_t m_faceOrientation;
    uint32_t m_previewWidth;
    uint32_t m_previewHeight;
    uint32_t m_dumpMask;
    uint32_t m_BypassForDebug;
    uint32_t m_CopyFrameIndex;
    uint32_t m_FakeProcTime;
    uint32_t m_isCropInfoUpdate;
    double m_processTime;
    uint32_t m_frameworkCameraId;
    MPBASE_Version m_algoVersion;
    bool m_flushing;
    bool m_nightMode;
    bool m_isOISSupport;
    // uint32_t                 m_dumpTime[2];
    LSCDimensionCap mLSCDim;
    float m_lensShadingMap[LSCTotalChannels *
                           LSC_MESH_ROLLOFF_SIZE]; ///< Lens Shading Map per channel
    MiaColorFilterPattern m_rawPattern;            /// defined in miafmt.h
    int32_t m_InitRet;
    MiaTuningModeParameter m_tuningParam;
    SuperNightRawValue m_ExtRawValue;
    int32_t m_CamMode;
    ImageFormat m_format;
    int32_t transferdata[5];
    char m_dumpTimeBuf[128];
    int32_t m_devOrientation; ///< device orirentation
    bool m_isSENeeded;
    int32_t m_EVoffset = 0;
    int32_t m_AlgoLogLevel;
    MRECT *m_pZoomRect;
    int32_t m_i32LuxIndex;
    BlackLevel m_BlackLevel = {0, 256, 256, 256, 256};
    MiaNodeInterface mNodeInterface;
    std::atomic<int32_t> m_InputIndex;
    std::atomic<bool> m_flushFlag;
    // ImageParams *m_normalInput;         ///< normal(EV0) input image param
    // ARC_SN_RAWINFO m_normalRawInfo;     ///< normal(EV0) input image rawinfo
    ImageParams *m_copyFromInput; ///< when error happend or bypass copy this to output buffer
    ImageParams *m_firstInput;
    ImageParams *m_curInputImgParam;
    PROCESSSTAT m_processStatus;
    ImageParams *m_outputImgParam;
    uint32_t m_algoInputNum;
    int32_t m_InputFD[FRAME_MAX_INPUT_NUMBER];
    ARC_SN_RAWINFO m_RawInfo[FRAME_MAX_INPUT_NUMBER];
    ASVLOFFSCREEN m_InputFrame[ARC_SN_MAX_INPUT_IMAGE_NUM];
    camera_metadata_t *m_pMetaData;

    std::mutex m_initMutex;
    std::mutex m_flushMutex;
};

PLUMA_INHERIT_PROVIDER(RawSuperlightPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new RawSuperlightPluginProvider());
    return true;
}

#endif // _RAWSUPERLIGHTPLUGIN_H_
