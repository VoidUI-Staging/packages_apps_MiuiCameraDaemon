#ifndef _MIALGOELLCPLUGIN_H_
#define _MIALGOELLCPLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>

#include <atomic>
#include <mutex>

#include "MiaPluginUtils.h"
#include "chistatsproperty.h"
#include "chituningmodeparam.h"
#include "chivendortag.h"
#include "mialgoai_ellc.h"

/// Convert from nano secs to micro secs
#define NanoToMicro(x) (x / 1000)
/// Convert from micro secs to nano secs
#define MicroToNano(x) (x * 1000)
/// Convert from nano secs to secs
#define NanoToSec(x) (x / 1000000000)

//时间单位都统一成微秒 = 1000毫秒 = 1000000秒
static const uint64_t NSEC_PER_SEC = 1000000000ull;
static const uint32_t QTimerSizeBytes = 8;
static const uint32_t QtimerFrequency = 19200000; ///< QTimer Freq = 19.2 MHz
static const uint32_t MAX_SAMPLES_BUF_SIZE = 200; ///< Max sample size
static const uint64_t SMAPLE_TIME = 1000000 / MAX_SAMPLES_BUF_SIZE;
static const int32_t ELLCMODE_HANDLE = 5;
static const int32_t ELLCMODE_TRIPOD = 6;
static const int32_t ELLCMODE_HANDLE_LONG = 7;
static const int32_t ELLCMODE_TRIPOD_LONG = 8;
static const int32_t ELLC_SUPPORT_FRAMES = 8;
static const uint32_t ELLC_SUPPORT_SIZE[][2] = {
    // w    h
    {4080, 3072},
    {4096, 3072},
}; // ellc算法支持的size列表
static const uint32_t ELLC_SUPPORT_MINSIZE[] = {784, 1040};
static const uint32_t ELLC_SUPPORT_FORMAT = CAM_FORMAT_RAW16;

using ELLCMODE = int32_t;
template <typename S>
using STATSTR = std::map<S, std::string>;

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
    float x[MAX_SAMPLES_BUF_SIZE];
    float y[MAX_SAMPLES_BUF_SIZE];
    float z[MAX_SAMPLES_BUF_SIZE];
    uint64_t t[MAX_SAMPLES_BUF_SIZE];
} ChiFrameGyro;

enum class PROCESSSTAT {
    INVALID,
    PROCESSINIT,
    PROCESSPRE,
    PROCESSPREWAIT,
    PROCESSRUN,
    PROCESSUINIT,
    PROCESSERROR,
    PROCESSBUFWAIT,
    PROCESSFLUSH,
    PROCESSEXIT,
    EXIT,
};

static const STATSTR<PROCESSSTAT> PROCESSSTATSTR{
    {PROCESSSTAT::INVALID, "INVALID"},
    {PROCESSSTAT::PROCESSINIT, "PROCESSINIT"},
    {PROCESSSTAT::PROCESSPRE, "PROCESSPRE"},
    {PROCESSSTAT::PROCESSPREWAIT, "PROCESSPREWAIT"},
    {PROCESSSTAT::PROCESSRUN, "PROCESSRUN"},
    {PROCESSSTAT::PROCESSUINIT, "PROCESSUINIT"},
    {PROCESSSTAT::PROCESSERROR, "PROCESSERROR"},
    {PROCESSSTAT::PROCESSFLUSH, "PROCESSFLUSH"},
    {PROCESSSTAT::PROCESSEXIT, "PROCESSEXIT"},
    {PROCESSSTAT::EXIT, "EXIT"},
    {PROCESSSTAT::PROCESSBUFWAIT, "PROCESSBUFWAIT"},
};

static const STATSTR<MialgoAi_EllcStatus> ELLCSTATSTR{
    {OK, "OK"},
    {BAD_ARG, "BAD_ARG"},
    {NULL_PTR, "NULL_PTR"},
    {BAD_BUFFER, "BAD_BUFFER"},
    {BAD_OP, "BAD_OP"},
    {BAD_PIPELINE, "BAD_PIPELINE"},
    {INTERRUPT, "INTERRUPT"},
    {TIMEOUT, "TIMEOUT"},
    {UNSPECIFIED, "UNSPECIFIED"},
    {NULL_HANDLE, "NULL_HANDLE"},
};

//匹配size, 如果收到了算法不支持的size, 做memcpy, 以防不测
static bool matchSize(const uint32_t &width, const uint32_t &height, uint32_t stride = 0)
{
#ifdef THOR_CAM
    if ((ELLC_SUPPORT_MINSIZE[0] <= width) && (ELLC_SUPPORT_MINSIZE[1] <= height)) {
        if (!(width & 1) && !(height & 1) && (width <= stride)) {
            return true;
        }
    }
#else
    for (auto &ps : ELLC_SUPPORT_SIZE) {
        if ((ps[0] == width) && (ps[1] == height)) {
            return true;
        }
    }
#endif
    return false;
}

static inline bool matchFormat(const uint32_t &format)
{
    return ELLC_SUPPORT_FORMAT == format;
}

//根据S类型返回不同的stat字典, 可以继续添加
template <typename S>
static inline const typename std::enable_if<std::is_same<S, PROCESSSTAT>::value,
                                            STATSTR<PROCESSSTAT>>::type &
sMap()
{
    return PROCESSSTATSTR;
}

//根据S类型返回不同的stat字典, 可以继续添加
template <typename S>
static inline const typename std::enable_if<std::is_same<S, MialgoAi_EllcStatus>::value,
                                            STATSTR<MialgoAi_EllcStatus>>::type &
sMap()
{
    return ELLCSTATSTR;
}

// log用, 通过stat打印对应的字符串, 知晓当前的stat
template <typename S>
static std::string statStr(const S &s)
{
    STATSTR<S> stat = sMap<S>();
    auto sr = stat.find(s);
    if (stat.end() == sr) {
        std::string ret("INVALID_STAT");
        return ret;
    }

    return sr->second;
}

//普通变量转string
template <typename T>
static inline typename std::enable_if<!std::is_enum<T>::value, std::string>::type toStr(T n)
{
    return std::to_string(n);
}

//枚举变量转string
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

//当被测试函数的返回值  是void 的时候, 调用这个函数, 返回值的double类型的毫秒时间
template <typename Func, typename... Args>
static typename std::enable_if<std::is_void<typename std::result_of<Func(Args...)>::type>::value,
                               double>::type
costTimeMs(Func &&func, Args &&...args)
{
    auto get_time_us = []() -> double {
        struct timeval time;
        gettimeofday(&time, NULL);
        double ms = time.tv_sec * 1000.0 + time.tv_usec / 1000.0;
        return ms;
    };

    double start = get_time_us();
    func(args...);
    double end = get_time_us();

    double cost = end - start;

    return cost;
}

//当被测试函数的返回值  非void  的时候, 调用这个函数, 返回值的tuple, 第一个值是double类型的毫秒时间,
//第二个值是函数的返回值
template <typename Func, typename... Args>
static
    typename std::enable_if<!(std::is_void<typename std::result_of<Func(Args...)>::type>::value),
                            std::tuple<double, typename std::result_of<Func(Args...)>::type>>::type
    costTimeMs(Func &&func, Args &&...args)
{
    using RT = typename std::result_of<Func(Args...)>::type;
    std::tuple<double, RT> ret;
    auto nfunc = [&]() -> void { std::get<1>(ret) = func(args...); };
    std::get<0>(ret) = costTimeMs(nfunc);

    return ret;
}

inline uint64_t QtimerNanoToQtimerTicks(uint64_t qtimerNano)
{
    return static_cast<uint64_t>(qtimerNano * double(QtimerFrequency / double(NSEC_PER_SEC)));
}

inline uint64_t QtimerTicksToQtimerNano(uint64_t ticks)
{
    return (uint64_t(double(ticks) * double(NSEC_PER_SEC) / double(QtimerFrequency)));
}

class MiAlgoEllcPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.mialgoellc";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual void destroy();
    virtual bool isEnabled(MiaParams settings);
    virtual void reset();

private:
    ~MiAlgoEllcPlugin();
    void initMialgoAiEllcParas(CreateInfo *createInfo);
    bool checkSuccess() const { return (OK == m_ellc_ret); }
    void getInitInfo(camera_metadata_t *); //获取初始化参数, 仅调用一次
    void getPrepInfo(camera_metadata_t *); //获取pre process参数, 每一帧都要调用
    void setExifInfo();                    //设置exif info
    void setMetaInfo();                    //更新metadata
    void fillGyroData(camera_metadata_t *, MialgoAi_EllcGyroElement *, MialgoAi_EllcFrameTime &,
                      uint32_t &);
    void getGyroInterval(camera_metadata_t *,
                         MialgoAi_EllcFrameTime &); //获取当前帧的开始和结束时间戳
    void updateEllcMode();                          //获取ellc的运行模式, 手持或脚架等
    PROCESSSTAT processRequest(const int &);
    MialgoAi_EllcStatus processInit(MiImageBuffer &);
    MialgoAi_EllcStatus processBuffer(MiImageBuffer &, const int &);
    MialgoAi_EllcStatus processAlgo(MiImageBuffer &);
    MialgoAi_EllcStatus processUinit();
    MialgoAi_EllcStatus processError();
    MialgoAi_EllcStatus processFlush();
    MialgoAi_EllcStatus processExit();
    PROCESSSTAT getCurrentStat() const { return m_process_stat; }
    // if flush triggerd, stat cannot be changed
    void setNextStat(const PROCESSSTAT &stat, const bool &force = false)
    {
        std::lock_guard<std::mutex> lock(m_state_lock);
        if (stat == PROCESSSTAT::INVALID) {
            m_stat_lock = false;
        }
        if (stat == PROCESSSTAT::PROCESSFLUSH) {
            m_stat_lock = true;
        }
        if (!m_stat_lock || force)
            m_process_stat = stat;
    }
    void parseProp();

private:
    std::atomic<bool> m_stat_lock;
    std::atomic<PROCESSSTAT> m_process_stat;
    ELLCMODE m_process_mode;

    MiaNodeInterface mNodeInterface;
    uint64_t m_processedFrame; ///< The count for processed frame
    MiImageBuffer m_inputFrame;
    MiImageBuffer m_outputFrame;
    camera_metadata_t *mp_inputMeta;
    camera_metadata_t *mp_outputMeta;
    ChiTuningModeParameter m_tuningParam;

    MialgoAi_EllcHandle mp_ellc_handle;
    MialgoAi_EllcVerison m_ellc_version;
    MialgoAi_EllcGyroElement m_ellc_gyrosample[MAX_SAMPLES_BUF_SIZE];
    MialgoAi_EllcFrameTime m_ellc_frametime;
    MialgoAi_EllcGyroData m_ellc_gyrodata;
    MialgoAi_EllcMode m_ellc_mode;
    MialgoAi_EllcRawBurstInfo m_ellc_burstinfo;
    MialgoAi_EllcGainInfo m_ellc_gaininfo;
    MialgoAi_EllcInitParams m_ellc_initparams;
    MialgoAi_EllcPreprocessParams m_ellc_preprocessparams;
    MialgoAi_EllcRunParams m_ellc_runparams;
    MialgoAi_EllcExecLog m_ellc_execlog;
    std::atomic<MialgoAi_EllcStatus> m_ellc_ret;

    bool m_exitDone;
    bool m_needDump;
    bool m_needBypass;
    std::atomic<uint32_t> m_frameIndex;
    uint32_t m_outFrameIdx;
    int64_t m_outFrameTimeStamp;
    int32_t m_forceBlackLevel;
    int32_t m_forceMode;
    double m_ellc_cost;

    std::mutex m_state_lock;
    std::mutex m_process_lock;
    std::mutex m_terminate_lock;
    std::atomic<bool> m_flushDone;
    std::condition_variable m_cond;
};

PLUMA_INHERIT_PROVIDER(MiAlgoEllcPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new MiAlgoEllcPluginProvider());
    return true;
}

#endif // _MIALGOELLCPLUGIN_H_
