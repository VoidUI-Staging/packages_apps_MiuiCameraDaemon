#include "MiCamPerfManager.h"

namespace mizone {
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////Begin of PerfManager
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
PerfManager *PerfManager::mPerfManager = new PerfManager();

PerfManager::PerfManager()
{
    mInitialized = false;
    m_perfLibHandle = NULL;

    mPerfLockOps.perfLockAcquire = NULL;
    // mPerfLockOps.powerHint = NULL;
    mPerfLockOps.perfLockRelease = NULL;
}

PerfManager::~PerfManager() {}

PerfManager *PerfManager::getInstance()
{
    if (mPerfManager->mInitialized) {
        return mPerfManager;
    } else {
        if (!mPerfManager->mInitialized) {
            std::lock_guard<std::mutex> initLock(mPerfManager->mInitMutex);
            if (!mPerfManager->mInitialized) {
                mPerfManager->initialize();
            }
        }
        return mPerfManager;
    }
}

void PerfManager::initialize()
{
    auto libGetAddr = [](void *hLibrary, const char *procName) {
        void *procAddr = NULL;
        if (NULL != hLibrary) {
            procAddr = dlsym(hLibrary, procName);
        }
        return procAddr;
    };
    const char *pPerfModule = "libmtkperf_client_vendor.so";

    const unsigned int bindFlags = RTLD_NOW | RTLD_LOCAL;
    m_perfLibHandle = dlopen(pPerfModule, bindFlags);

    if (NULL == m_perfLibHandle) {
        MLOGE(LogGroupCore, "Error Loading Library: %s, dlopen err %s", pPerfModule, dlerror());
    }

    if (m_perfLibHandle) {
        mPerfLockOps.perfLockAcquire =
            reinterpret_cast<PerfLockAcquireFunc>(libGetAddr(m_perfLibHandle, "perf_lock_acq"));
        mPerfLockOps.perfLockRelease =
            reinterpret_cast<PerfLockReleaseFunc>(libGetAddr(m_perfLibHandle, "perf_lock_rel"));
        // mPerfLockOps.powerHint =
        //    reinterpret_cast<PerfHintFunc>(PluginUtils::libGetAddr(m_perfLibHandle, "perf_hint"));

        mInitialized = true;
    } else {
        MLOGE(LogGroupCore, "PerfManager::initialize fail");
    }
}

PerfLock *PerfManager::CreatePerfLock()
{
    if (!mInitialized) {
        return NULL;
    }

    return PerfLock::Create(&mPerfLockOps);
}

} // namespace mizone
