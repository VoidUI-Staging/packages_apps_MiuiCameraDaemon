#ifndef __MIHAL_MONITOR__
#define __MIHAL_MONITOR__

#include <sys/sysinfo.h>

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace mihal {
class PsiMemMonitor
{
public:
    static PsiMemMonitor *getInstance();
    void psiMonitorLoop();
    void psiMonitorTrigger();
    unsigned long GetTotalRAM();
    bool isPsiMemFull();

    void snapshotAdd()
    {
        if (!mIsHighMem)
            mSnapShotCounter++;
        return;
    }
    void snapshotCountReclaim()
    {
        if (!mIsHighMem)
            mSnapShotCounter = 0;
        return;
    }
    void psiMonitorClose()
    {
        if (!mIsHighMem) {
            mPsiMonitorStart = false;
        }
        return;
    }
    bool isLowRamDev()
    {
        return GetTotalRAM() <= (unsigned long)8 * 1024 * 1024 * 1024 ? true : false;
    }

private:
    PsiMemMonitor();
    ~PsiMemMonitor();
    std::thread mPsiMonitorThread;
    std::mutex mPsiMonitorLock;
    std::condition_variable mPsiMonitorCond;
    int mMemPressureLevel = 0;
    std::atomic_bool mPsiMemFull;
    bool mIsHighMem = false;
    bool mThreadExit;
    std::atomic_int32_t mSnapShotCounter;
    std::atomic_bool mPsiMonitorStart;
};

} // namespace mihal

#endif //__MIHAL_MONITOR__