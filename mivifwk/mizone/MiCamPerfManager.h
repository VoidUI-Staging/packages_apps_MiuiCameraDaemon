#ifndef __MICAM_PERF_MANAGER__
#define __MICAM_PERF_MANAGER__

#include <mutex>

#include "MiCamPerfLock.h"
#include "MiDebugUtils.h"
using namespace midebug;

namespace mizone {

class PerfManager
{
public:
    ~PerfManager();
    static PerfManager *getInstance();
    PerfLock *CreatePerfLock();

private:
    PerfManager();

    void initialize();

    static PerfManager *mPerfManager;
    bool mInitialized;
    std::mutex mInitMutex;

    void *m_perfLibHandle;
    PerfLockOps mPerfLockOps;
};

class PluginPerfLockManager final
{
public:
    PluginPerfLockManager(uint32_t timer)
    {
        mPluginPerfLock = PerfManager::getInstance()->CreatePerfLock();
        if (mPluginPerfLock == NULL) {
            MLOGE(LogGroupCore, "PerfLock create fail");
            return;
        }

        if (mPluginPerfLock) {
            mPluginPerfLock->AcquirePerfLock(timer);
            MLOGI(LogGroupCore, "PluginPerfLockManager  AcquirePerfLock, boost cpu %d ms", timer);
        }
    }
    ~PluginPerfLockManager()
    {
        if (mPluginPerfLock) {
            mPluginPerfLock->ReleasePerfLock();
            mPluginPerfLock->Destory();
            MLOGI(Mia2LogGroupCore, "PluginPerfLockManager  ReleasePerfLock");
        }
    }

private:
    PerfLock *mPluginPerfLock;
};

} // namespace mizone

#endif // !__MICAM_PERF_MANAGER__
