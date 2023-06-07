#include "MiaPerfLock.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files
namespace mialgo2 {

typedef enum {
    CPU_SILVER_LEVEL_0 = 300,
    CPU_SILVER_LEVEL_1 = 403,
    CPU_SILVER_LEVEL_2 = 499,
    CPU_SILVER_LEVEL_3 = 595,
    CPU_SILVER_LEVEL_4 = 691,
    CPU_SILVER_LEVEL_5 = 806,
    CPU_SILVER_LEVEL_6 = 902,
    CPU_SILVER_LEVEL_7 = 998,
    CPU_SILVER_LEVEL_8 = 1094,
    CPU_SILVER_LEVEL_9 = 1209,
    CPU_SILVER_LEVEL_10 = 1305,
    CPU_SILVER_LEVEL_11 = 1401,
    CPU_SILVER_LEVEL_12 = 1497,
    CPU_SILVER_LEVEL_13 = 1612,
    CPU_SILVER_LEVEL_14 = 1708,
    CPU_SILVER_LEVEL_15 = 1804,
    CPU_SILVER_LEVEL_MAX = 1804
} cpu_silver_level_t;

typedef enum {
    CPU_GOLD_LEVEL_0 = 710,
    CPU_GOLD_LEVEL_1 = 844,
    CPU_GOLD_LEVEL_2 = 960,
    CPU_GOLD_LEVEL_3 = 1075,
    CPU_GOLD_LEVEL_4 = 1209,
    CPU_GOLD_LEVEL_5 = 1324,
    CPU_GOLD_LEVEL_6 = 1440,
    CPU_GOLD_LEVEL_7 = 1555,
    CPU_GOLD_LEVEL_8 = 1670,
    CPU_GOLD_LEVEL_9 = 1766,
    CPU_GOLD_LEVEL_10 = 1881,
    CPU_GOLD_LEVEL_11 = 1996,
    CPU_GOLD_LEVEL_12 = 2112,
    CPU_GOLD_LEVEL_13 = 2227,
    CPU_GOLD_LEVEL_14 = 2342,
    CPU_GOLD_LEVEL_15 = 2419,
    CPU_GOLD_LEVEL_MAX = 2419
} cpu_gold_level_t;

typedef enum {
    CPU_PLUS_LEVEL_0 = 844,
    CPU_PLUS_LEVEL_1 = 960,
    CPU_PLUS_LEVEL_2 = 1075,
    CPU_PLUS_LEVEL_3 = 1190,
    CPU_PLUS_LEVEL_4 = 1305,
    CPU_PLUS_LEVEL_5 = 1420,
    CPU_PLUS_LEVEL_6 = 1555,
    CPU_PLUS_LEVEL_7 = 1670,
    CPU_PLUS_LEVEL_8 = 1785,
    CPU_PLUS_LEVEL_9 = 1900,
    CPU_PLUS_LEVEL_10 = 2035,
    CPU_PLUS_LEVEL_11 = 2150,
    CPU_PLUS_LEVEL_12 = 2265,
    CPU_PLUS_LEVEL_13 = 2380,
    CPU_PLUS_LEVEL_14 = 2496,
    CPU_PLUS_LEVEL_15 = 2592,
    CPU_PLUS_LEVEL_16 = 2668,
    CPU_PLUS_LEVEL_17 = 2764,
    CPU_PLUS_LEVEL_18 = 2841,
    CPU_PLUS_LEVEL_MAX = 2841
} cpu_plus_level_t;

typedef enum {
    MPCTLV3_MIN_FREQ_CLUSTER_BIG_CORE_0 = 0x40800000,
    MPCTLV3_MIN_FREQ_CLUSTER_BIG_CORE_1 = 0x40800010,
    MPCTLV3_MIN_FREQ_CLUSTER_BIG_CORE_2 = 0x40800020,
    MPCTLV3_MIN_FREQ_CLUSTER_BIG_CORE_3 = 0x40800030,
    MPCTLV3_MIN_FREQ_CLUSTER_LITTLE_CORE_0 = 0x40800100,
    MPCTLV3_MIN_FREQ_CLUSTER_LITTLE_CORE_1 = 0x40800110,
    MPCTLV3_MIN_FREQ_CLUSTER_LITTLE_CORE_2 = 0x40800120,
    MPCTLV3_MIN_FREQ_CLUSTER_LITTLE_CORE_3 = 0x40800130,
    MPCTLV3_MIN_FREQ_CLUSTER_PLUS_CORE_0 = 0x40800200,

    MPCTLV3_MAX_FREQ_CLUSTER_BIG_CORE_0 = 0x40804000,
    MPCTLV3_MAX_FREQ_CLUSTER_BIG_CORE_1 = 0x40804010,
    MPCTLV3_MAX_FREQ_CLUSTER_BIG_CORE_2 = 0x40804020,
    MPCTLV3_MAX_FREQ_CLUSTER_BIG_CORE_3 = 0x40804030,
    MPCTLV3_MAX_FREQ_CLUSTER_LITTLE_CORE_0 = 0x40804100,
    MPCTLV3_MAX_FREQ_CLUSTER_LITTLE_CORE_1 = 0x40804110,
    MPCTLV3_MAX_FREQ_CLUSTER_LITTLE_CORE_2 = 0x40804120,
    MPCTLV3_MAX_FREQ_CLUSTER_LITTLE_CORE_3 = 0x40804130,
    MPCTLV3_MAX_FREQ_CLUSTER_PLUS_CORE_0 = 0x40804200,

    MPCTLV3_MIN_ONLINE_CPU_CLUSTER_BIG = 0x41000000,
    MPCTLV3_MIN_ONLINE_CPU_CLUSTER_LITTLE = 0x41000100,
    MPCTLV3_MAX_ONLINE_CPU_CLUSTER_BIG = 0x41004000,
    MPCTLV3_MAX_ONLINE_CPU_CLUSTER_LITTLE = 0x41004100,

    MPCTLV3_SCHED_PREFER_SPREAD = 0x40CA8000,
    MPCTLV3_MIN_ONLINE_CPU_CLUSTER_PRIME = 0x41000200,
    MPCTLV3_MAX_ONLINE_CPU_CLUSTER_PRIME = 0x41004200,

    MPCTLV3_LPM_BIAS_HYST = 0x40408000,
    MPCTLV3_WALT_TARGET_LOAD_THRESH_BIG = 0x40810000,
    MPCTLV3_WALT_TARGET_LOAD_THRESH_LITTLE = 0x40814000,
    MPCTLV3_WALT_TARGET_LOAD_THRESH_PRIME = 0x40818000,

    MPCTLV3_SCHEDUTIL_PREDICTIVE_LOAD_CLUSTER_BIG = 0x41444000,
    MPCTLV3_SCHEDUTIL_PREDICTIVE_LOAD_CLUSTER_LITTLE = 0x41444100,
    MPCTLV3_SCHEDUTIL_HISPEED_LOAD_CLUSTER_BIG = 0x41440000,
    MPCTLV3_SCHEDUTIL_HISPEED_LOAD_CLUSTER_LITTLE = 0x41440100,
    MPCTLV3_SCHEDUTIL_HISPEED_FREQ_CLUSTER_BIG = 0x4143C000,
    MPCTLV3_SCHEDUTIL_HISPEED_FREQ_CLUSTER_LITTLE = 0x4143C100,

    MPCTLV3_GPU_POWER_LEVEL = 0X42800000,
    MPCTLV3_GPU_MIN_POWER_LEVEL = 0X42804000,
    MPCTLV3_GPU_MAX_POWER_LEVEL = 0X42808000,
    MPCTLV3_GPU_MIN_FREQ = 0X4280C000,
    MPCTLV3_GPU_MAX_FREQ = 0X42810000,
    MPCTLV3_GPU_BUS_MIN_FREQ = 0X42814000,
    MPCTLV3_GPU_BUS_MAX_FREQ = 0X42818000,
    MPCTLV3_GPU_DISABLE_GPU_NAP = 0X4281C000,

    MPCTLV3_LLCBW_HWMON_MIN_FREQ = 0x43000000,
    MPCTLV3_LLCBW_HWMON_HYST_OPT = 0x43008000,
    MPCTLV3_LLCBW_HWMON_SAMPLE_MS = 0x4300C000,
    MPCTLV3_LLCBW_HWMON_IO_PERCENT = 0x43004000,

    MPCTLV3_CPUBW_HWMON_MIN_FREQ = 0x41800000,
    MPCTLV3_CPUBW_HWMON_HYST_OPT = 0x4180C000,
    MPCTLV3_CPUBW_HWMON_SAMPLE_MS = 0x41820000,
    MPCTLV3_CPUBW_HWMON_IO_PERCENT = 0x41808000,
    MPCTLV3_SCHED_GROUP_UPMIGRATE = 0x40C54000,
    MPCTLV3_SCHED_GROUP_DOWNMIGRATE = 0x40C58000,

    MPCTLV3_L2_MEMLAT_RATIO_CEIL_0 = 0x43404000,
    MPCTLV3_L2_MEMLAT_RATIO_CEIL_1 = 0x43408000,
    MPCTLV3_LLCC_DDR_LAT_MIN_FREQ_0 = 0x43430000,
    MPCTLV3_LLCC_DDR_LAT_MIN_FREQ_1 = 0x43430100,
    MPCTLV3_LLCC_MEMLAT_MIN_FREQ = 0x4341C000,
    MPCTLV3_L3_MEMLAT_MIN_FREQ = 0x43400000,
    MPCTLV3_MEMLAT_MIN_FREQ_0 = 0x43414000,
    MPCTLV3_MEMLAT_MIN_FREQ_1 = 0x43418000,

    MPCTLV3_ALL_CPUS_PWR_CLPS_DIS = 0x40400000,
    MPCTLV3_SCHED_BOOST = 0x40C00000,

    MPCTLV3_SCHED_UPMIGRATE = 0x40C1C000,
    MPCTLV3_SCHED_DOWNMIGRATE = 0x40C20000,

    MPCTLV3_LLCC_DDR_BW_MIN_FREQ_V2 = 0x4303C000,
    MPCTLV3_BUS_DCVS_LLCC_DDR_LAT_MINFREQ_SILVER = 0x43460000,
    MPCTLV3_BUS_DCVS_LLCC_DDR_LAT_MINFREQ_PRIME = 0x43484000,
    MPCTLV3_BUS_DCVS_DDR_LAT_MINFREQ_GOLD = 0x43488000,
    MPCTLV3_BUS_DCVS_LLCC_MEMLAT_MIN_FREQ_SILVER = 0x4345C000,
    MPCTLV3_BUS_DCVS_LLCC_MEMLAT_MIN_FREQ_GOLD = 0x4348C000,
    MPCTLV3_CPU_LLCC_BW_MIN_FREQ_V2 = 0x41844000,
    MPCTLV3_BUS_DCVS_L3_MEMLAT_MIN_FREQ_PRIME = 0x43458000,
    MPCTLV3_BUS_DCVS_L3_MEMLAT_MIN_FREQ_GOLD = 0x43480000,

    POWER_HINT_ID_PREVIEW = 0x00001330,
    POWER_HINT_ID_VIDEO_ENCODE = 0x00001331,
    POWER_HINT_ID_VIDEO_ENCODE_60FPS = 0x00001332,
    POWER_HINT_ID_VIDEO_ENCODE_HFR = 0x00001333,
    POWER_HINT_ID_VIDEO_ENCODE_HFR_480FPS = 0x00001334,
    POWER_HINT_ID_VIDEO_ENCODE_8K30 = 0x00001335,

    MPCTLV3_XIAOMI_KSWAPD_AFFINITY = 0x5FC00000,
    MPCTLV3_XIAOMI_KCOMPACTD_AFFINITY = 0x5FCF0000,
    MPCTLV3_BUS_DCVS_LLCC_DDR_BOOST_FREQ = 0x43498000,
    MPCTLV3_BUS_DCVS_LLCC_L3_BOOST_FREQ = 0x4349C000,
    MPCTLV3_BUS_DCVS_LLCC_LLCC_BOOST_FREQ = 0x434A0000,
} perf_lock_params;

static int32_t perfLockParamsSnapshotCapture[] = {
    // Set little cluster cores to 1.8 GHz
    MPCTLV3_ALL_CPUS_PWR_CLPS_DIS + 1,
    0x1,
    MPCTLV3_SCHED_BOOST,
    0x3,
    MPCTLV3_MAX_FREQ_CLUSTER_BIG_CORE_0,
    0xFFF,
    MPCTLV3_MIN_FREQ_CLUSTER_BIG_CORE_0,
    0xFFF,
    MPCTLV3_MAX_FREQ_CLUSTER_LITTLE_CORE_0,
    0xFFF,
    MPCTLV3_MIN_FREQ_CLUSTER_LITTLE_CORE_0,
    0xFFF,
    MPCTLV3_MIN_FREQ_CLUSTER_PLUS_CORE_0,
    0xFFF,
    MPCTLV3_MAX_FREQ_CLUSTER_PLUS_CORE_0,
    0xFFF,
    MPCTLV3_BUS_DCVS_LLCC_DDR_BOOST_FREQ,
    4224000,
    MPCTLV3_BUS_DCVS_LLCC_L3_BOOST_FREQ,
    1804000,
    MPCTLV3_BUS_DCVS_LLCC_LLCC_BOOST_FREQ,
    1066000,
    MPCTLV3_XIAOMI_KSWAPD_AFFINITY,
    0x7F01,
    MPCTLV3_XIAOMI_KCOMPACTD_AFFINITY,
    0x7F,
    // sched changes
};

static int32_t MiBoekhperfLockParamsSnapshotCapture[] = {MPCTLV3_ALL_CPUS_PWR_CLPS_DIS,
                                                         1,
                                                         MPCTLV3_MIN_FREQ_CLUSTER_LITTLE_CORE_0,
                                                         0x46C,
                                                         MPCTLV3_MIN_FREQ_CLUSTER_BIG_CORE_0,
                                                         0x553,
                                                         MPCTLV3_SCHED_DOWNMIGRATE,
                                                         0x14,
                                                         MPCTLV3_SCHED_UPMIGRATE,
                                                         0x1E,
                                                         MPCTLV3_WALT_TARGET_LOAD_THRESH_BIG,
                                                         100,
                                                         MPCTLV3_WALT_TARGET_LOAD_THRESH_LITTLE,
                                                         100,
                                                         MPCTLV3_WALT_TARGET_LOAD_THRESH_PRIME,
                                                         100};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MiaPerfLock::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MiaPerfLock *MiaPerfLock::Create(const PerfLockOps *pPerfLockOps)
{
    if (NULL == pPerfLockOps) {
        return NULL;
    }

    MiaPerfLock *pPerfLock = new MiaPerfLock(pPerfLockOps);

    if (NULL != pPerfLock) {
        if (false == pPerfLock->Initialize()) {
            delete (pPerfLock);
            pPerfLock = NULL;
        }
    }

    return pPerfLock;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MiaPerfLock::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MiaPerfLock::MiaPerfLock(const PerfLockOps *pPerfLockOps) : m_pPerfLockOps(pPerfLockOps)
{
    mRefCount = 0;
    mHandle = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MiaPerfLock::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MiaPerfLock::Initialize()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MiaPerfLock::AcquirePerfLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaPerfLock::AcquirePerfLock(uint32_t timer)
{
    CDKResult result = MIA_RETURN_OK;

    std::lock_guard<std::mutex> autoLock(m_Mutex);

    if (mRefCount == 0) {
        MICAM_TRACE_ASYNC_BEGIN_F(MialgoTraceProfile, 0, "MIA_OFFLINE_SESSION");
    }

    MLOGI(Mia2LogGroupPlugin, "add perflock when PerfLock_None.");
    mHandle =
        m_pPerfLockOps->perfLockAcquire(mHandle, timer, perfLockParamsSnapshotCapture,
                                        sizeof(perfLockParamsSnapshotCapture) / sizeof(int32_t));

    if (mHandle > 0) {
        ++mRefCount;
        MLOGD(Mia2LogGroupCore, "MiaPerfLock handle: %d, updated refCount: %d", mHandle, mRefCount);
    } else {
        MLOGW(Mia2LogGroupCore, "Acquire perflock failed");
        result = MIA_RETURN_UNKNOWN_ERROR;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MiaPerfLock::MiAcquirePerfLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaPerfLock::MiAlgoAcquirePerfLock(uint32_t timer, Perf_Lock_Algo Mi_PerLockFlag)
{
    CDKResult result = MIA_RETURN_OK;
    std::lock_guard<std::mutex> autoLock(m_Mutex);

    if (mRefCount == 0) {
        MICAM_TRACE_ASYNC_BEGIN_F(MialgoTraceProfile, 0, "MIA_OFFLINE_SESSION");
    }

    int PerfLockLength = sizeof(perfLockParamsSnapshotCapture) / sizeof(int32_t);
    std::vector<int32_t> MiperfLockParamsSnapshotCapture(
        perfLockParamsSnapshotCapture, perfLockParamsSnapshotCapture + PerfLockLength);
    if (MiperfLockParamsSnapshotCapture.empty()) {
        MLOGE(Mia2LogGroupPlugin,
              "add perflock MiperfLockParamsSnapshotCapture when capture is fail.");
        result = MIA_RETURN_UNKNOWN_ERROR;
    }
    int *MiperfLockParams = NULL;

    switch (Mi_PerLockFlag) {
    case PerfLock_SR:
        MLOGI(Mia2LogGroupPlugin, "add perflock to boost Gpu when SR capture.");
        MiperfLockParamsSnapshotCapture.push_back(MPCTLV3_GPU_MIN_POWER_LEVEL);
        MiperfLockParamsSnapshotCapture.push_back(0);
        MiperfLockParams = MiperfLockParamsSnapshotCapture.data();
        mHandle = m_pPerfLockOps->perfLockAcquire(mHandle, timer, MiperfLockParams,
                                                  MiperfLockParamsSnapshotCapture.size());
        break;
    case PerfLock_BOKEH_DEPTH:
    case PerfLock_BOKEH_BOKEH:
    case PerfLock_BOKEH_HDR:
    case PerfLock_BOKEH_reserve1:
    case PerfLock_BOKEH_reserve2:
        MLOGI(Mia2LogGroupPlugin,
              "add perflock to boost Gpu when Bokeh_Depth or Bokeh_Bokeh capture.");
        mHandle = m_pPerfLockOps->perfLockAcquire(
            mHandle, timer, MiBoekhperfLockParamsSnapshotCapture,
            sizeof(MiBoekhperfLockParamsSnapshotCapture) / sizeof(int32_t));
        break;
    default:
        MLOGE(Mia2LogGroupPlugin, "add perflock to boost Gpu fail.");
        result = MIA_RETURN_UNKNOWN_ERROR;
        break;
    }
    if (mHandle > 0) {
        ++mRefCount;
        MLOGD(Mia2LogGroupCore, "MiaPerfLock handle: %d, updated refCount: %d", mHandle, mRefCount);
    } else {
        MLOGW(Mia2LogGroupCore, "Acquire perflock failed");
        result = MIA_RETURN_UNKNOWN_ERROR;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MiaPerfLock::ReleasePerfLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaPerfLock::ReleasePerfLock()
{
    CDKResult result = MIA_RETURN_OK;

    std::lock_guard<std::mutex> autoLock(m_Mutex);

    if (mHandle > 0) {
        if (--mRefCount == 0) {
            int returnCode = m_pPerfLockOps->perfLockRelease(mHandle);

            mHandle = 0;

            if (returnCode < 0) {
                MLOGE(Mia2LogGroupCore, "Release MiaPerfLock failed");
                result = MIA_RETURN_UNKNOWN_ERROR;
            }
            MLOGD(Mia2LogGroupCore, "MiaPerfLock release released refCount %d", mRefCount);
        }
    } else {
        MLOGD(Mia2LogGroupCore, "MiaPerfLock already released or not acquired");
    }

    if (mRefCount == 0) {
        MICAM_TRACE_ASYNC_END_F(MialgoTraceProfile, 0, "MIA_OFFLINE_SESSION");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MiaPerfLock::Destory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MiaPerfLock::Destory()
{
    if (mRefCount != 0) {
        MLOGI(Mia2LogGroupCore, "Non zero references to this MiaPerfLock");
    }

    delete (this);
}

} // namespace mialgo2
