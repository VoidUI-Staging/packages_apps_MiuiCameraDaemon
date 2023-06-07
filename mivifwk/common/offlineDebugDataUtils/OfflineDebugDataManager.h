#pragma once

#include <stdlib.h>

#include <chrono>
#include <mutex>
#include <thread>

#include "MiDebugUtils.h"

#undef LOG_TAG
#define LOG_TAG "MiOfflineDebugdata"

using namespace midebug;

static const int32_t MaxNumOfflineDebugData = 50;            // Max Number of OfflineDebugData
static const uint32_t WaitForReclaimDebugDataInSeconds = 20; // Time to wait for
// Reclaim offline debugData Buffer monitor 20s
static const uint32_t ResultSuccess = 0; ///< Operation was successful.
static const uint32_t ResultEFailed = 1; ///< Operation encountered an unspecified error.

class OfflineDebugDataManager
{
public:
    static OfflineDebugDataManager *getInstance()
    {
        static OfflineDebugDataManager singleton;
        MLOGV(Mia2LogGroupDebug, "[Offline DebugData]: OfflineDebugDataManager %p", &singleton);
        return &singleton;
    }

    void setSessionStatus(bool sessionStatus)
    {
        if (!mReclaimThreadActive)
            return;

        MLOGI(Mia2LogGroupDebug, "[Offline DebugData]: mIsMihalHasSession %d mIsVTOpen %d",
              sessionStatus, mIsVTOpen.load());

        std::lock_guard<std::mutex> condLock(mCondMutex);
        mIsMihalHasSession = sessionStatus;
        if (!(mIsVTOpen || mIsMihalHasSession)) {
            mReclaimCond.notify_one();
        }
    };

    void setVTStatus(bool vtStatus)
    {
        if (!mReclaimThreadActive)
            return;

        MLOGI(Mia2LogGroupDebug, "[Offline DebugData]: mIsVTOpen %d mIsMihalHasSession %d ",
              vtStatus, mIsMihalHasSession.load());

        std::lock_guard<std::mutex> condLock(mCondMutex);
        mIsVTOpen = vtStatus;
        if (!(mIsVTOpen || mIsMihalHasSession)) {
            mReclaimCond.notify_one();
        }
    };

    void *getDebugDataBufferAndAddReference(size_t oriSize);

    uint32_t subDebugDataReference(void *pDebugData);

private:
    /// @brief Describes the debug data.
    typedef struct
    {
        size_t size; ///< Size (in bytes) of the debug data buffer.
        void *pData; ///< Pointer to the debug data buffer.
    } DebugData;

    /// For Offline Debugdata Transmission Start
    struct OfflineDebugData
    {
        DebugData debugData;
        int32_t referenceCount;
    };

    void offlineDebugDataReclaim();

    OfflineDebugDataManager()
        : mIsMihalHasSession(false), mIsVTOpen(false), mReclaimThreadActive(false)
    {
    }

    OfflineDebugDataManager(OfflineDebugDataManager &) = delete;

    OfflineDebugData mOfflineDebugDataArr[MaxNumOfflineDebugData];
    std::mutex mArrayMutex;

    std::thread mReclaimThread;
    std::atomic<bool> mReclaimThreadActive;
    std::mutex mCondMutex;
    std::condition_variable mReclaimCond;
    std::atomic<bool> mIsMihalHasSession; // rt and postprocess all finished
    std::atomic<bool> mIsVTOpen;
    // if mIsMihalHasSession == flase && VTClosed means, space no more use
};
/// For Offline Debugdata Transmission End
