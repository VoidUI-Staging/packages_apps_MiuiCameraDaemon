#include "MialgoTaskMonitor.h"

#include <MiDebugUtils.h>

#include "AppSnapshotController.h"
#include "MiCamJsonUtils.h"
using namespace midebug;
namespace mizone {

MialgoTaskMonitor *MialgoTaskMonitor::getInstance()
{
    static MialgoTaskMonitor mialgoMonitor{};

    return &mialgoMonitor;
}

MialgoTaskMonitor::MialgoTaskMonitor() : mCanMialgoHandleNextShot(true), mIsExitThread(false)
{
    MiCamJsonUtils::getVal("MialgoTaskConsumption.MialgoCapacity", mMialgoSessionCapacity, 6);
}

void MialgoTaskMonitor::startThread()
{
    mIsExitThread = false;
    if (mThead.joinable()) {
        MLOGI(LogGroupHAL, "thread is runing!");
    } else {
        mThead = std::thread([this]() { threadLoop(); });
    }
}

void MialgoTaskMonitor::exitThread()
{
    mIsExitThread = true;
    mWaitCond.notify_all();
    if (mThead.joinable()) {
        mThead.join();
    }
}

void MialgoTaskMonitor::onMialgoTaskNumChanged(std::string captureTypeName, int taskConsumption)
{
    std::unique_lock<std::mutex> lg(mCheckMialgoStatusLock);
    if (!mMialgoTaskStatus.count(captureTypeName)) {
        mMialgoTaskStatus[captureTypeName] = 0;
    }
    mMialgoTaskStatus[captureTypeName] += taskConsumption;

    if (mMialgoTaskStatus[captureTypeName] < 0) {
        MLOGI(LogGroupHAL,
              "[AppSnapshotController]:MialgoTask total consumption is negative!! reset to 0");
        mMialgoTaskStatus[captureTypeName] = 0;
    }

    bool newStatus = canMialgoHandleNextSnapshot();
    bool oldStatus = mCanMialgoHandleNextShot.exchange(newStatus);

    // NOTE: we only notify snapshot controller if we have different status
    if (oldStatus != newStatus) {
        exitThread();
        if (!newStatus) {
            MLOGI(LogGroupHAL,
                  "[AppSnapshotController]: Mialgo can't handle next snapshot, start timer");
            startThread();
        }
        AppSnapshotController::getInstance()->onRecordUpdate(newStatus, this);
    }
    lg.unlock();
}

bool MialgoTaskMonitor::canMialgoHandleNextSnapshot()
{
    bool canHandle = true;
    // NOTE: since ProcessorManager may hold many mialgo sessions, we will return true only if all
    // mialgo sessions can handle next snapshot
    for (auto &entry : mMialgoTaskStatus) {
        if (entry.second >= mMialgoSessionCapacity) {
            MLOGI(LogGroupHAL,
                  "[AppSnapshotController][Statistic]:Mialgo:is full! consumption:capacity = "
                  "%d:%d",
                  entry.second, mMialgoSessionCapacity);
            canHandle = false;
            break;
        }
    }

    return canHandle;
}

void MialgoTaskMonitor::threadLoop()
{
    while (true) {
        std::unique_lock<std::mutex> lock(mStatusMutex);
        bool noTimeOut = mWaitCond.wait_for(lock, std::chrono::seconds(10), [this]() {
            return mIsExitThread || mCanMialgoHandleNextShot;
        });
        lock.unlock();

        if (!noTimeOut && !mCanMialgoHandleNextShot) {
            std::unique_lock<std::mutex> lg(mCheckMialgoStatusLock);
            for (auto &entry : mMialgoTaskStatus) {
                if (entry.second >= mMialgoSessionCapacity) {
                    // NOTE: reset misbehaved mialgo session's status
                    MLOGE(LogGroupHAL,
                          "[AppSnapshotController] mialgo's session:%s is full and timeout!! its "
                          "total "
                          "consumption is: %d, reset it to 0",
                          entry.first.c_str(), entry.second);
                    entry.second = 0;
                }
            }
            mCanMialgoHandleNextShot.store(true);
            lg.unlock();
            AppSnapshotController::getInstance()->onRecordUpdate(true, this);
            return;
        }

        if (mIsExitThread) {
            return;
        }
    }
}

MialgoTaskMonitor::~MialgoTaskMonitor()
{
    mIsExitThread = true;
    mWaitCond.notify_all();
    if (mThead.joinable()) {
        mThead.join();
    }
}

} // namespace mizone
