/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */
#ifndef __MICAM_PERFLOCK__
#define __MICAM_PERFLOCK__
#include <mutex>
#include <vector>

namespace mizone {

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
    // PerfHintFunc powerHint;              ///< Powerhint function to perflock API
};

class PerfLock final
{
public:
    /// Static method to create an instance of Perflock
    static PerfLock *Create(const PerfLockOps *);

    void Destory();

    CDKResult AcquirePerfLock(uint32_t timer = 0);

    CDKResult ReleasePerfLock();

private:
    /// Default constructor for PerfLockManager object
    PerfLock(const PerfLockOps *);

    /// Default destructor for PerfLock object.
    ~PerfLock() = default;

    PerfLock(const PerfLock &) = delete; ///< Disallow the copy constructor

    PerfLock &operator=(const PerfLock &) = delete; ///< Disallow assignment operator

    bool Initialize();

    // get configuration information
    void getCaptureBoostTable(std::vector<int> &table);

    const PerfLockOps *m_pPerfLockOps; ///< Perf lock API entry
    int32_t mHandle;                   ///< Perflock handle
    uint32_t mRefCount;                ///< Number of references to this lock
    std::mutex m_Mutex;                ///< Mutex object
    std::vector<int> perfLockParamsSnapshotCapture;
};

} // namespace mizone

#endif // !__MICAM_PERF_LOCK__
