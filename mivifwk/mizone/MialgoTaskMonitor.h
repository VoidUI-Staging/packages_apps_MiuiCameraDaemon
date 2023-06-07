#ifndef __MIALGO_TASK_MONITOR_H__
#define __MIALGO_TASK_MONITOR_H__

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>
namespace mizone {

class MialgoTaskMonitor
{
public:
    static MialgoTaskMonitor *getInstance();
    // NOTE: invoked by MonitorMialgoTask to updated each mialgo session's consumption
    void onMialgoTaskNumChanged(std::string captureTypeName, int taskConsumption);

private:
    MialgoTaskMonitor();
    ~MialgoTaskMonitor();
    void threadLoop();
    void startThread();
    void exitThread();
    // NOTE: return true only if all mialgo session can handle next snapshot
    bool canMialgoHandleNextSnapshot();
    // NOTE: to record the task consumption in each mialgo session
    std::map<std::string /*mialgo session name*/, int /*total task consumption*/> mMialgoTaskStatus;
    // NOTE: variables to cache the last status of Mialgo
    std::atomic<bool> mCanMialgoHandleNextShot;
    std::mutex mCheckMialgoStatusLock;
    int mMialgoSessionCapacity;
    std::mutex mStatusMutex;
    std::condition_variable mWaitCond;
    std::thread mThead;
    std::atomic<bool> mIsExitThread;
};

} // namespace mizone

#endif