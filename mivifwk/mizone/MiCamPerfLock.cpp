#include "MiCamPerfLock.h"

#include <MiDebugUtils.h>

#include "mtkperf_resource.h"
using namespace midebug;
namespace mizone {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PerfLock::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PerfLock *PerfLock::Create(const PerfLockOps *pPerfLockOps)
{
    if (NULL == pPerfLockOps) {
        return NULL;
    }

    PerfLock *pPerfLock = new PerfLock(pPerfLockOps);

    if (NULL != pPerfLock) {
        if (false == pPerfLock->Initialize()) {
            delete (pPerfLock);
            pPerfLock = NULL;
        }
    }

    return pPerfLock;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PerfLock::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PerfLock::PerfLock(const PerfLockOps *pPerfLockOps) : m_pPerfLockOps(pPerfLockOps)
{
    mRefCount = 0;
    mHandle = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PerfLock::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool PerfLock::Initialize()
{
    getCaptureBoostTable(perfLockParamsSnapshotCapture);
    return true;
}

void PerfLock::getCaptureBoostTable(std::vector<int> &table)
{
    // pull CPU max freq
    table.push_back(PERF_RES_CPUFREQ_PERF_MODE);
    table.push_back(1);

    // pull GPU max freq
    table.push_back(PERF_RES_GPU_FREQ_MIN);
    table.push_back(0);

    // pull DRAM max freq
    table.push_back(PERF_RES_DRAM_OPP_MIN);
    table.push_back(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PerfLock::AcquirePerfLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult PerfLock::AcquirePerfLock(uint32_t timer)
{
    CDKResult result = 0;

    std::lock_guard<std::mutex> autoLock(m_Mutex);

    mHandle = m_pPerfLockOps->perfLockAcquire(mHandle, timer, perfLockParamsSnapshotCapture.data(),
                                              perfLockParamsSnapshotCapture.size());

    if (mHandle > 0) {
        ++mRefCount;
        MLOGD(LogGroupCore, "mizoen PerfLock handle: %d, updated refCount: %d", mHandle, mRefCount);
    } else {
        MLOGW(LogGroupCore, "Acquire perflock failed");
        result = -1;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PerfLock::ReleasePerfLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult PerfLock::ReleasePerfLock()
{
    CDKResult result = 0;

    std::lock_guard<std::mutex> autoLock(m_Mutex);
    if (mHandle > 0) {
        if (--mRefCount == 0) {
            int returnCode = m_pPerfLockOps->perfLockRelease(mHandle);

            mHandle = 0;

            if (returnCode < 0) {
                MLOGE(LogGroupCore, "Release PerfLock failed");
                result = -1;
            }
            MLOGD(LogGroupCore, "PerfLock release released refCount %d", mRefCount);
        }
    } else {
        MLOGD(LogGroupCore, "PerfLock already released or not acquired");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PerfLock::Destory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PerfLock::Destory()
{
    if (mRefCount != 0) {
        MLOGI(Mia2LogGroupCore, "Non zero references to this PerfLock");
    }

    delete (this);
}

} // namespace mizone
