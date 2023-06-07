#include "MialgoTaskMonitor.h"

#include <sys/sysinfo.h>

#include <algorithm>

#include "AppSnapshotController.h"
#include "LogUtil.h"
#include "MiviSettings.h"
namespace mihal {
#define MOCKCAMERANUM 2

MialgoTaskMonitor *MialgoTaskMonitor::getInstance()
{
    static MialgoTaskMonitor mialgoMonitor{};

    return &mialgoMonitor;
}

MialgoTaskMonitor::MialgoTaskMonitor()
    : mForceWakeUpTimer{Timer::createTimer()}, mMialgoTimeOutHandler{[this]() {
          std::unique_lock<std::mutex> lg(mCheckMialgoStatusLock);
          MLOGE(LogGroupHAL,
                "[AppSnapshotController] mialgo's task is full and doesn't return a result to "
                "mihal for %u millisecond, now album must have small pics, check it!!!",
                mMialgoTaskWaitTime);
          for (auto &entry : mMialgoTaskStatus) {
              if (entry.second >= mMialgoSessionCapacity) {
                  // NOTE: reset misbehaved mialgo session's status
                  MLOGE(LogGroupHAL,
                        "[AppSnapshotController] mialgo's session:%s is full!! its total "
                        "consumption is: %d, reset it to 0",
                        entry.first.c_str(), entry.second);
                  entry.second = 0;
              }
          }
          // NOTE: force to wake up app
          AppSnapshotController::getInstance()->onRecordUpdate(true, this);
      }}
{
    mTooManyImageCategories = true;

    MiviSettings::getVal("MialgoTaskConsumption.MialgoCapacity", mMialgoSessionCapacity, 6);

    // NOTE: read sysinfo file to check whether this is a low RAM device
    struct sysinfo sysInfo;
    if (0 == sysinfo(&sysInfo)) {
        // NOTE: if device RAM is more than 8G, then this is a large RAM device
        mIsLargeRam = (sysInfo.totalram > (unsigned long)8 * 1024 * 1024 * 1024) ? true : false;
    } else {
        mIsLargeRam = false;
    }

    MLOGI(LogGroupHAL, "[DeviceInfo]: is device large RAM: %d", mIsLargeRam);
}

void MialgoTaskMonitor::onMialgoTaskNumChanged(std::string sessionName, int taskConsumption,
                                               uint32_t fwkFrameNum, bool isFromMihalToMialgo)
{
    // NOTE: In large RAM case, reduce task consumption. Especially, we care about bokeh senario
    if (mIsLargeRam) {
        // NOTE: now we only loose dual bokeh restriction
        std::string lowerCaseSessionName = sessionName;
        std::transform(sessionName.begin(), sessionName.end(), lowerCaseSessionName.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        auto rearPose = lowerCaseSessionName.find("dual");
        auto bokehPose = lowerCaseSessionName.find("bokeh");

        if (rearPose != std::string::npos && bokehPose != std::string::npos) {
            auto tmp = (taskConsumption - 2) > 0 ? taskConsumption - 2 : taskConsumption;
            MLOGD(LogGroupHAL,
                  "[Bokeh] rear bokeh snapshot in large RAM device, loose restriction: "
                  "taskConsumption change from %d to %d",
                  taskConsumption, tmp);
            taskConsumption = tmp;
        }
    }

    std::unique_lock<std::mutex> lg(mCheckMialgoStatusLock);
    if (!mMialgoTaskStatus.count(sessionName)) {
        mMialgoTaskStatus[sessionName] = 0;
        if (taskConsumption > 0) {
            checkImageCategoryCount(sessionName);
        }
    }

    if (isFromMihalToMialgo && taskConsumption != 0) {
        mMialgoTaskFrame[sessionName].insert(fwkFrameNum);
    } else {
        if (mMialgoTaskFrame[sessionName].count(fwkFrameNum))
            mMialgoTaskFrame[sessionName].erase(fwkFrameNum);
        else
            return;
    }

    mMialgoTaskStatus[sessionName] += isFromMihalToMialgo ? taskConsumption : -taskConsumption;
    MLOGD(LogGroupHAL,
          "[AppSnapshotController][Statistic]: Mialgo: %s's status updated, now total consumption "
          "is: %d",
          sessionName.c_str(), mMialgoTaskStatus[sessionName]);

    if (mMialgoTaskStatus[sessionName] < 0) {
        MLOGE(LogGroupHAL,
              "[AppSnapshotController] MialgoTask total consumption is negative!! reset to 0");
        mMialgoTaskStatus[sessionName] = 0;
    }

    if (mTooManyImageCategories) {
        lg.unlock();
        mForceWakeUpTimer->cancel();
        return;
    }

    bool newStatus = canMialgoHandleNextSnapshot();
    AppSnapshotController::getInstance()->onRecordUpdate(newStatus, this);

    if (newStatus) {
        MLOGD(LogGroupHAL,
              "[AppSnapshotController] Mialgo can handle more task, stop forceWakeUp timer");
        // NOTE: to avoid deadlock in Timer. deadlock occasion: A.mCheckMialgoStatusLock B.mMutex in
        // cancel(), and mMialgoTimeOutHandler get invoked, so A.mMutex in runAfter's lambda func
        // and B.mCheckMialgoStatusLock in mMialgoTimeOutHandler, and thereby a deadlock occurs
        lg.unlock();
        mForceWakeUpTimer->cancel();
    } else {
        MLOGD(LogGroupHAL,
              "[AppSnapshotController] Mialgo can't handle next snapshot, start forceWakeUp timer");
        lg.unlock();
        mForceWakeUpTimer->runAfter(mMialgoTaskWaitTime, mMialgoTimeOutHandler);
    }
}

void MialgoTaskMonitor::checkImageCategoryCount(const std::string &sessionName)
{
    auto pos = sessionName.rfind("output");
    if (pos == std::string::npos) {
        return;
    }
    auto str = sessionName.substr(pos);

    if (mSignatureSet.count(str)) {
        if (mSignatureSet.size() <= MOCKCAMERANUM) {
            mTooManyImageCategories = false;
            return;
        }
        auto it = mImageCategories.begin();
        for (int i = 0; i < MOCKCAMERANUM; i++, it++) {
            if (*it == str) {
                // stop sanpshot because there are too many image size categories.
                mTooManyImageCategories = true;
                AppSnapshotController::getInstance()->onRecordUpdate(false, this);
                return;
            }
        }
    } else {
        mSignatureSet.insert(str);
        mImageCategories.push_back(str);
    }
}

void MialgoTaskMonitor::deleteMialgoTaskSig(const std::string &sessionName)
{
    mMialgoTaskStatus.erase(sessionName);

    auto pos = sessionName.rfind("output");
    if (pos == std::string::npos) {
        return;
    }
    auto str = sessionName.substr(pos);
    mSignatureSet.erase(str);

    for (auto itr = mImageCategories.begin(); itr != mImageCategories.end();) {
        if (*itr == str) {
            itr = mImageCategories.erase(itr);
        } else {
            ++itr;
        }
    }

    if (mImageCategories.size() <= MOCKCAMERANUM) {
        mTooManyImageCategories = false;
        // At this time, one of the first two image sizes has been processed, and the camera must be
        // enabled immediately.
        if (canMialgoHandleNextSnapshot()) {
            AppSnapshotController::getInstance()->onRecordUpdate(true, this);
        } else {
            MLOGI(LogGroupHAL, "too many tasks, cant't handle next snapshot");
        }
    }
}

bool MialgoTaskMonitor::canMialgoHandleNextSnapshot()
{
    bool canHandle = true;
    // NOTE: since ProcessorManager may hold many mialgo sessions, we will return true only if all
    // mialgo sessions can handle next snapshot
    for (auto &entry : mMialgoTaskStatus) {
        if (entry.second >= mMialgoSessionCapacity) {
            MLOGD(LogGroupHAL,
                  "[AppSnapshotController][Statistic] Mialgo: %s is full! consumption:capacity = "
                  "%d:%d",
                  entry.first.c_str(), entry.second, mMialgoSessionCapacity);
            canHandle = false;
            break;
        }
    }

    return canHandle;
}

} // namespace mihal