#ifndef _MIALGOSNSCPLUGIN_H_
#define _MIALGOSNSCPLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include <atomic>
#include <mutex>

#include "MiaPluginUtils.h"
#include "chistatsproperty.h"
#include "chituningmodeparam.h"
#include "chivendortag.h"
#include "mawsignaltask.h"
#include "mialgoai_dips.h"
#include "tinyxml2.h"

static const uint64_t NSEC_PER_SEC = 1000000000ull;
static const uint32_t QTimerSizeBytes = 8;
static const uint32_t QtimerFrequency = 19200000; ///< QTimer Freq = 19.2 MHz

/// Convert from nano secs to micro secs
#define NanoToMicro(x) (x / 1000)
/// Convert from micro secs to nano secs
#define MicroToNano(x) (x * 1000)
/// Convert from nano secs to secs
#define NanoToSec(x) (x / 1000000000)

static const uint32_t MAX_GYRO_ELEMENT_SIZE = 200; ///< Max sample size
static const uint64_t SMAPLE_TIME = 1000000 / MAX_GYRO_ELEMENT_SIZE;

static const int32_t SNSC_SUPPORT_FRAME_SIZE = 8;

typedef struct
{
    uint32_t type;
    uint32_t value;
} AiMisdScene;

typedef struct
{
    uint32_t gcount;
    uint64_t ts;
    uint64_t te;
    float x[MAX_GYRO_ELEMENT_SIZE];
    float y[MAX_GYRO_ELEMENT_SIZE];
    float z[MAX_GYRO_ELEMENT_SIZE];
    uint64_t t[MAX_GYRO_ELEMENT_SIZE];
} ChiFrameGyro;

typedef struct
{
    FLOAT velocityStaticRatio;
    FLOAT velocitySlowRatio;
    FLOAT velocityMiddleRatio;
    FLOAT velocityFastRatio;
    FLOAT velocityMax;
    FLOAT velocityMean;
} MotionVelocity;

struct NightMotionCaptureInfo
{
    UINT32 burstIndex;
    UINT32 burstCount;
};

/// @brief Structure describing sensor metadata for a frame
struct SensorMetaData
{
    UINT32 width;                       ///< Width of the frame
    UINT32 height;                      ///< Height of this frame
    FLOAT sensorGain;                   ///< Sensor real gain programed in this frame
    UINT32 frameLengthLines;            ///< Frame length lines of this frame
    UINT64 exposureTime;                ///< Exposure time of this frame
    FLOAT middleSensorGain;             ///< Middle frame sensor real gain programed in this frame
    UINT64 middleExposureTime;          ///< Middle frame sensor exposure time for this frame
    FLOAT shortSensorGain;              ///< Short frame sensor real gain programed in this frame
    UINT64 shortExposureTime;           ///< Short frame sensor exposure time for this frame
    INT32 sensitivity;                  ///< Sensor sensitivity
    INT32 middleSensitivity;            ///< Middle frame Sensor sensitivity
    INT32 shortSensitivity;             ///< Short frame Sensor sensitivity
    UINT32 testPatternMode;             ///< Test pattern mode
    UINT32 filterArrangement;           ///< Colour filter pattern
    UINT64 frameDuration;               ///< Total frame duration
    UINT64 rollingShutterSkew;          ///< Rolling shutter skew
    UINT64 longVerticalOffsetDuration;  ///< Long Vertcal Offset Duration
    UINT64 shortVerticalOffsetDuration; ///< Short Vertical Offset Duration
    UINT64 middleVertcalOffsetDuration; ///< Middle Vertical Offset Duration
};

/// brief MotionCaptureTypeValues
enum MotionCaptureTypeValues {
    BrightMotionNoMotion = 0, ///< bright motion capture case no motion
    BrightMotion,             ///< normal bright motion capture case
    BrightMotionFlicker,      ///< bright motion capture with flicker
    NightMotionNoMotion = 10, ///< night motion capture case no motion
    NightMotion,              ///< normal night motion capture case
    NightMotionFlicker,       ///< night motion capture with flicker
    NightMotionHeavyFlicker   ///< night motion capture with heavy flicker
};

inline uint64_t QtimerNanoToQtimerTicks(uint64_t qtimerNano)
{
    return static_cast<uint64_t>(qtimerNano * double(QtimerFrequency / double(NSEC_PER_SEC)));
}

inline uint64_t QtimerTicksToQtimerNano(uint64_t ticks)
{
    return (uint64_t(double(ticks) * double(NSEC_PER_SEC) / double(QtimerFrequency)));
}

template <typename T>
static inline typename std::enable_if<!std::is_enum<T>::value, std::string>::type toStr(T n)
{
    return std::to_string(n);
}

template <typename T>
static inline typename std::enable_if<std::is_enum<T>::value, std::string>::type toStr(T n)
{
    return std::to_string(static_cast<int32_t>(n));
}

template <typename T>
static std::string toStr(T n, int cut)
{
    std::string result = "";
    std::string tmp = std::to_string(n);
    bool has_dot = false;
    for (auto &c : tmp) {
        if (has_dot) {
            cut--;
        }
        if (cut >= 0) {
            result += c;
        }
        if (c == '.') {
            has_dot = true;
        }
    }
    return result;
}

enum SNSC_STATE {
    STATE_IDLE,
    STATE_INITIALIZED,
    STATE_PROCESSING,
    STATE_RUNNING,
    STATE_COMPLETED,
    STATE_ERROR,
};

enum CMD {
    CMD_INITIALIZE,
    CMD_PROCESS,
    CMD_FLUSH,
    CMD_DESTROY,
};

class MiAlgoSNSCPlugin : public PluginWraper
{
public:
    MiAlgoSNSCPlugin();

    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.mialgosnsc";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual void destroy();
    virtual bool isEnabled(MiaParams settings);

private:
    ~MiAlgoSNSCPlugin();

    int triggerCmd(int cmd, void *arg = nullptr);

    static MST::Result<MialgoAi_DIPS_Status> m_async_result;
    MialgoAi_DIPS_Status doInitModel();

    // handle mivi call
    MialgoAi_DIPS_Status doInit(CreateInfo *createInfo);
    MialgoAi_DIPS_Status doProcess(ProcessRequestInfo *processRequestInfo);
    MialgoAi_DIPS_Status doFlush(FlushRequestInfo *flushRequestInfo);
    MialgoAi_DIPS_Status doDestroy();

    // handle call snsc
    MialgoAi_DIPS_Status processSNSCInit(ProcessRequestInfo *processRequestInfo);
    MialgoAi_DIPS_Status processSNSCProcess(ProcessRequestInfo *processRequestInfo);
    MialgoAi_DIPS_Status processSNSCRun(ProcessRequestInfo *processRequestInfo);
    MialgoAi_DIPS_Status processSNSCUnit(bool isFlush);
    void fillGyroData(camera_metadata_t *pInputMeta, MialgoAi_DIPS_GyroData &gyroData);
    void fillGainInfo(camera_metadata_t *pInputMeta, MialgoAi_DIPS_GainInfo &gainInfo,
                      float shutterCapt, int &banding);
    void fillMotionInfo(camera_metadata_t *pInputMeta, float &motion);

    int safeIndex(int);

    void getDumpFileName(char *fileName, size_t size, const char *fileType, const char *timeBuf,
                         int width, int height);
    bool dumpToFile(const char *fileName, struct MiImageBuffer *imageBuffer, uint32_t index,
                    const char *fileType);

    MiaNodeInterface m_nodeInterface{};
    uint32_t m_outFrameIdx{0};
    int64_t m_outFrameTimeStamp{0};

    int m_supportFrameSize{0};
    int m_processedFrameSize{0};
    int m_SNSCState{STATE_IDLE};
    static MialgoAi_DIPS_Handle m_dipsHandle;

    MiImageBuffer m_outputImageBuffer{};
    MiImageBuffer m_inputImageBuffer{};
    camera_metadata_t *mp_inputMeta[SNSC_SUPPORT_FRAME_SIZE]{};
    camera_metadata_t *mp_outputMeta{nullptr};

    std::mutex m_lock{};

    char m_dumpPrefixs[128]{};

    // MialgoAi_DIPS_RawBurstInfo m_rawBurstInfo;
    MialgoAi_DIPS_GyroElement m_gyroElement[MAX_GYRO_ELEMENT_SIZE]{};

    std::string m_SNSCExifInfo{};

    bool m_realEnable{false};
    int m_baseFrameIndex{0};
    int64_t m_frameTimestamps[SNSC_SUPPORT_FRAME_SIZE]{};
    std::string mExifGainInfo[SNSC_SUPPORT_FRAME_SIZE]{};
    std::string mExifMotionInfo{};
    std::string mExifLPInfo{};
    int m_dumpMask{0};
};

PLUMA_INHERIT_PROVIDER(MiAlgoSNSCPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiAlgoSNSCPluginProvider());
    return true;
}

#endif // define _MIALGOSNSCPLUGIN_H_