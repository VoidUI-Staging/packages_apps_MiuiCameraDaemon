#ifndef __MIALGO_TASK_MONITOR_H__
#define __MIALGO_TASK_MONITOR_H__

#include <atomic>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <string>

#include "Timer.h"

namespace mihal {

class MialgoTaskMonitor
{
public:
    static MialgoTaskMonitor *getInstance();
    // NOTE: invoked by MonitorMialgoTask to updated each mialgo session's consumption
    void onMialgoTaskNumChanged(std::string sessionName, int taskConsumption, uint32_t fwkFrameNum,
                                bool isFromMihalToMialgo);
    // NOTE: used to stop or enable APP snapshot when MockCamera is exharsted.
    void deleteMialgoTaskSig(const std::string &sessionName);
    void checkImageCategoryCount(const std::string &sessionName);

private:
    MialgoTaskMonitor();
    ~MialgoTaskMonitor() = default;

    // NOTE: return true only if all mialgo session can handle next snapshot
    bool canMialgoHandleNextSnapshot();

    // NOTE: to record the task consumption in each mialgo session
    std::map<std::string /*mialgo session name*/, int /*total task consumption*/> mMialgoTaskStatus;
    std::map<std::string /*mialgo session name*/, std::set<uint32_t> /*fwkFrameNum*/>
        mMialgoTaskFrame;
    std::mutex mCheckMialgoStatusLock;
    int mMialgoSessionCapacity;

    // NOTE: used to compute how long mialgo hasn't finish a task.
    std::shared_ptr<Timer> mForceWakeUpTimer;
    static const uint32_t mMialgoTaskWaitTime = 10000;
    std::function<void()> mMialgoTimeOutHandler;

    // NOTE: used to stop or enable APP snapshot when MockCamera is exharsted.
    bool mTooManyImageCategories;
    std::set<std::string> mSignatureSet;
    std::list<std::string> mImageCategories;
    std::mutex mImageCategoriesLock;

    // NOTE: denote whether this device is large RAM device. If true, then we can loose snapshot
    // restriction so that we can take more snapshots
    bool mIsLargeRam = false;
};

} // namespace mihal

#endif