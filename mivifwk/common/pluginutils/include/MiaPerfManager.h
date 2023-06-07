#ifndef __MIA_PERF_MANAGER__
#define __MIA_PERF_MANAGER__

#include <mutex>

#include "MiaPerfLock.h"

namespace mialgo2 {

class MiaPerfManager
{
public:
    ~MiaPerfManager();
    static MiaPerfManager *getInstance();
    MiaPerfLock *CreatePerfLock();

private:
    MiaPerfManager();

    void initialize();

    static MiaPerfManager *mPerfManager;
    bool mInitialized;
    std::mutex mInitMutex;

    void *m_perfLibHandle;
    PerfLockOps mPerfLockOps;
};

class PluginPerfLockManager final
{
public:
    PluginPerfLockManager(uint32_t timer = 0, Perf_Lock_Algo perf_lock_algo = PerfLock_None)
    {
        mPluginPerfLock = MiaPerfManager::getInstance()->CreatePerfLock();
        if (mPluginPerfLock == NULL) {
            MLOGE(Mia2LogGroupCore, "MiaPerfLock create fail");
            return;
        }

        if (mPluginPerfLock) {
            if (perf_lock_algo != PerfLock_None) {
                mPluginPerfLock->MiAlgoAcquirePerfLock(timer, perf_lock_algo);
                MLOGI(Mia2LogGroupCore,
                      "PluginPerfLockManager  MiAcquirePerfLock for Algo, boost cpu %d ms", timer);
            } else {
                mPluginPerfLock->AcquirePerfLock(timer);
                MLOGI(Mia2LogGroupCore, "PluginPerfLockManager  AcquirePerfLock, boost cpu %d ms",
                      timer);
            }
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
    MiaPerfLock *mPluginPerfLock;
};

} // namespace mialgo2

#endif // !__MIA_PERF_MANAGER__
