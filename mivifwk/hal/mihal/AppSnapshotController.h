#ifndef __APP_SNAPSHOT_CONTROLLER__
#define __APP_SNAPSHOT_CONTROLLER__

#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "Timer.h"

namespace mihal {

// NOTE: refer to https://xiaomi.f.mioffice.cn/docs/dock4sX28Wx0yYJSKu1UVCDukzc for details
class AppSnapshotController
{
public:
    static AppSnapshotController *getInstance();
    // NOTE: invoked when a record is no longer useful
    void onRecordDelete(void *obj);
    // NOTE: invoked when an object's status changed
    void onRecordUpdate(bool canDoNextSnapshot, void *obj);

private:
    enum Command { MUTE, CONTINUE };

    enum AppStatus { MUTED, ACTIVATED };

    AppSnapshotController();
    ~AppSnapshotController();

    void onRecordChangedLocked();
    void processCommandLoop();

    // NOTE: denote whether app has blocked any snapshot, used to avoid duplicate notify since BG
    // service is interprocess communication and is time-consuming
    AppStatus mAppStatus;
    std::mutex mStatusMutex;

    // NOTE: to record the status of every object which will influence whether we can do next
    // snapshot
    std::map<void *, bool /*can do next snapshot*/> mRecord;
    // NOTE: commands sent by user to mute or activate app
    std::vector<Command> mCommands;
    std::mutex mRecordMutex;

    // NOTE: dedicated thread to mute or activate app, so that AppSnapshotController won't affect
    // critical path execution
    std::thread mWorker;
    bool mThreadExit;
    std::condition_variable mNewCommandNotifier;

    // NOTE: Timer is used to reset app so that app won't be muted forever in abnormal conditions
    std::shared_ptr<Timer> mTimer;
#ifdef USERDEBUG_CAM
    static const uint32_t mWaitTime = 30000;
#else
    static const uint32_t mWaitTime = 15000;
#endif
};

} // namespace mihal

#endif