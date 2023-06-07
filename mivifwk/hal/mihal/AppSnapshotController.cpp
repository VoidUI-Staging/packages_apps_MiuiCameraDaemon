#include "AppSnapshotController.h"

#include "BGService.h"
#include "LogUtil.h"

namespace mihal {

using namespace vendor::xiaomi::hardware::bgservice::implementation;

AppSnapshotController *AppSnapshotController::getInstance()
{
    static AppSnapshotController controller;

    return &controller;
}

AppSnapshotController::AppSnapshotController()
    : mAppStatus{ACTIVATED}, mThreadExit{false}, mTimer{Timer::createTimer()}
{
    mWorker = std::thread([this]() { processCommandLoop(); });
    MLOGI(LogGroupHAL, "[AppSnapshotController]: worker thread started");
}

AppSnapshotController::~AppSnapshotController()
{
    {
        std::unique_lock<std::mutex> ulg(mRecordMutex);
        mThreadExit = true;
        MLOGI(LogGroupHAL, "[AppSnapshotController]: notify worker thread exit");
    }
    mNewCommandNotifier.notify_all();

    if (mWorker.joinable()) {
        mWorker.join();
    }
}

void AppSnapshotController::onRecordDelete(void *obj)
{
    std::lock_guard<std::mutex> lg(mRecordMutex);
    auto itr = mRecord.find(obj);
    if (itr != mRecord.end()) {
        MLOGI(LogGroupHAL, "[AppSnapshotController] obj: %p 's record get deleted", itr->first);
        mRecord.erase(itr);

        onRecordChangedLocked();
    }
}

void AppSnapshotController::onRecordUpdate(bool canDoNextSnapshot, void *obj)
{
    std::lock_guard<std::mutex> lg(mRecordMutex);
    mRecord[obj] = canDoNextSnapshot;

    onRecordChangedLocked();
}

void AppSnapshotController::onRecordChangedLocked()
{
    // NOTE: check if all objects can handle next snapshot
    bool canAllObjDoNextSnap = true;
    {
        // NOTE: check if HAL can handle next snapshot(we can handle next snapshot only if all
        // records are true)
        for (auto &entry : mRecord) {
            canAllObjDoNextSnap &= entry.second;
            if (!entry.second) {
                MLOGI(LogGroupHAL, "[AppSnapshotController] obj: %p can't handle next snapshot",
                      entry.first);
                break;
            }
        }
    }

    // NOTE: generate command
    Command command = canAllObjDoNextSnap ? CONTINUE : MUTE;

    // NOTE: send commands to worker thread
    mCommands.push_back(command);
    mNewCommandNotifier.notify_all();
}

void AppSnapshotController::processCommandLoop()
{
    while (true) {
        std::vector<Command> tempCommands;
        {
            std::unique_lock<std::mutex> ulg(mRecordMutex);
            mNewCommandNotifier.wait(ulg, [this]() { return !mCommands.empty() || mThreadExit; });

            if (mThreadExit) {
                MLOGI(LogGroupHAL,
                      "[AppSnapshotController]: worker thread get exit signal, exiting...");
                return;
            }

            tempCommands = std::move(mCommands);
            mCommands.clear();
        }

        {
            std::unique_lock<std::mutex> lg(mStatusMutex);
            MLOGD(LogGroupHAL, "[AppSnapshotController] old app status: %s",
                  mAppStatus == MUTED ? "MUTED" : "ACTIVATED");
            auto updatedAppStatus = mAppStatus;
            // NOTE: merge all commands as one to reduce IPC num
            for (auto commandEntry : tempCommands) {
                MLOGD(LogGroupHAL, "[AppSnapshotController] incomming command: %s",
                      commandEntry == MUTE ? "MUTE" : "CONTINUE");
                if (updatedAppStatus == MUTED && commandEntry == CONTINUE) {
                    updatedAppStatus = ACTIVATED;
                } else if (updatedAppStatus == ACTIVATED && commandEntry == MUTE) {
                    updatedAppStatus = MUTED;
                }
            }

            // NOTE: if updated status is different from old one, then we need to mute or activate
            // app
            if (updatedAppStatus != mAppStatus) {
                mAppStatus = updatedAppStatus;
                if (updatedAppStatus == MUTED) {
                    MLOGI(LogGroupHAL,
                          "[AppSnapshotController] some obj can't handle next snapshot, mute app");
                    BGServiceWrap::getInstance()->notifySnapshotAvailability(0);
                    lg.unlock();
                    // NOTE: start reset timer to avoid abnormal condition
                    mTimer->runAfter(mWaitTime, [this]() {
                        MLOGE(LogGroupHAL,
                              "[AppSnapshotController]: app has been muted for too long time(%ds), "
                              "force to awaken it!!",
                              mWaitTime / 1000);
                        std::lock_guard<std::mutex> lg(mStatusMutex);
                        mAppStatus = ACTIVATED;
                        BGServiceWrap::getInstance()->notifySnapshotAvailability(1);
                    });

                } else {
                    MLOGI(LogGroupHAL,
                          "[AppSnapshotController] all objs can handle next snapshot again, wake "
                          "app continue");
                    BGServiceWrap::getInstance()->notifySnapshotAvailability(1);
                    lg.unlock();
                    mTimer->cancel();
                }
            }
        }
    }
}

} // namespace mihal
