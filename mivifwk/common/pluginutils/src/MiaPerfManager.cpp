#include "MiaPerfManager.h"

namespace mialgo2 {
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////Begin of MiaPerfManager
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
MiaPerfManager *MiaPerfManager::mPerfManager = new MiaPerfManager();

MiaPerfManager::MiaPerfManager()
{
    mInitialized = false;
    m_perfLibHandle = NULL;

    mPerfLockOps.perfLockAcquire = NULL;
    mPerfLockOps.powerHint = NULL;
    mPerfLockOps.perfLockRelease = NULL;
}

MiaPerfManager::~MiaPerfManager() {}

MiaPerfManager *MiaPerfManager::getInstance()
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

void MiaPerfManager::initialize()
{
    const char *pPerfModule = "libqti-perfd-client.so";

    m_perfLibHandle = PluginUtils::libMap(pPerfModule);
    if (m_perfLibHandle) {
        mPerfLockOps.perfLockAcquire = reinterpret_cast<PerfLockAcquireFunc>(
            PluginUtils::libGetAddr(m_perfLibHandle, "perf_lock_acq"));
        mPerfLockOps.perfLockRelease = reinterpret_cast<PerfLockReleaseFunc>(
            PluginUtils::libGetAddr(m_perfLibHandle, "perf_lock_rel"));
        mPerfLockOps.powerHint =
            reinterpret_cast<PerfHintFunc>(PluginUtils::libGetAddr(m_perfLibHandle, "perf_hint"));

        mInitialized = true;
    } else {
        MLOGE(Mia2LogGroupCore, "MiaPerfManager::initialize fail");
    }
}

MiaPerfLock *MiaPerfManager::CreatePerfLock()
{
    if (!mInitialized) {
        return NULL;
    }

    return MiaPerfLock::Create(&mPerfLockOps);
}

} // namespace mialgo2
