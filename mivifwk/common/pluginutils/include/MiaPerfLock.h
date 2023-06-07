/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */
#ifndef __MIA_PERFLOCK__
#define __MIA_PERFLOCK__
#include <vector>

#include "MiaPluginUtils.h"

namespace mialgo2 {

typedef enum {
    PerfLock_None = 0,
    PerfLock_SR = 1,
    PerfLock_BOKEH_DEPTH = 2,
    PerfLock_BOKEH_HDR = 3,
    PerfLock_BOKEH_BOKEH = 4,
    PerfLock_BOKEH_reserve1 = 5,
    PerfLock_BOKEH_reserve2 = 6,
} Perf_Lock_Algo;

typedef int32_t CDKResult;

/// @brief Perflock acquire function type
typedef int32_t (*PerfLockAcquireFunc)(int handle, int duration_ms, int params[], int size);
/// @brief Perflock release function type
typedef int32_t (*PerfLockReleaseFunc)(int);
/// @brief Perfhint function type
typedef int32_t (*PerfHintFunc)(int hint, const char *pkg, int duration, int type);

struct PerfLockOps
{
    PerfLockAcquireFunc perfLockAcquire; ///< Acquire function to perflock API
    PerfLockReleaseFunc perfLockRelease; ///< Release function to perflock API
    PerfHintFunc powerHint;              ///< Powerhint function to perflock API
};

class MiaPerfLock final
{
public:
    /// Static method to create an instance of Perflock
    static MiaPerfLock *Create(const PerfLockOps *);

    void Destory();

    CDKResult AcquirePerfLock(uint32_t timer = 0);

    CDKResult MiAlgoAcquirePerfLock(uint32_t timer = 0,
                                    Perf_Lock_Algo Mi_PerLockFlag = PerfLock_None);

    CDKResult ReleasePerfLock();

private:
    /// Default constructor for PerfLockManager object
    MiaPerfLock(const PerfLockOps *);

    /// Default destructor for PerfLock object.
    ~MiaPerfLock() = default;

    MiaPerfLock(const MiaPerfLock &) = delete; ///< Disallow the copy constructor

    MiaPerfLock &operator=(const MiaPerfLock &) = delete; ///< Disallow assignment operator

    bool Initialize();

    const PerfLockOps *m_pPerfLockOps; ///< Perf lock API entry
    int32_t mHandle;                   ///< Perflock handle
    uint32_t mRefCount;                ///< Number of references to this lock
    std::mutex m_Mutex;                ///< Mutex object
};

} // namespace mialgo2

#endif // !__MIA_PERF_LOCK__
