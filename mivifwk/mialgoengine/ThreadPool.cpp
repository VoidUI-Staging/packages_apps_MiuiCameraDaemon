/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "ThreadPool.h"

#include <processgroup/processgroup.h>
#include <stdatomic.h> // atomic_fetch_add
#include <sys/prctl.h> // prctl
#include <sys/resource.h>
#include <sys/time.h>

#include <string> // std::string  std::to_string

#include "MiDebugUtils.h"

namespace mialgo2 {
using namespace midebug;

// the constructor just launches some amount of workers
// TODO: add more feature to enable pre-start, max threads, etc...
ThreadPool::ThreadPool(uint32_t threads) : mStop(false), mMaxThreadNum(32)
{
    threads = threads > mMaxThreadNum ? mMaxThreadNum : threads;
    mThreadId = 0;
    mIdleThreads = 0;
    mActiveThreadNum = 0;
    mInitThreadNum = threads;
    for (size_t i = 0; i < threads; ++i) {
        addThread();
    }
}

void ThreadPool::addThread()
{
    int thisThreadId = atomic_fetch_add(&mThreadId, 1);
    mWorkers.insert(std::make_pair(thisThreadId, [this, thisThreadId]() { loop(thisThreadId); }));
    mIdleThreads += 1;
    mActiveThreadNum += 1;
    MLOGI(Mia2LogGroupCore, "new thread, threadId:%d", thisThreadId);
}

void ThreadPool::loop(int thisThreadId)
{
    std::string threadName = std::string("AlgoFwkThd") + std::to_string(thisThreadId);
    prctl(PR_SET_NAME, threadName.c_str());
    SetTaskProfiles(gettid(), {"SCHED_SP_FOREGROUND"}, true);
    setpriority(PRIO_PROCESS, 0, -3); // -3 = 117 - 120, 117 is priority

    while (true) {
        Task task;
        std::unique_lock<std::mutex> locker(mMutex);

        mCond.wait(locker, [this]() { return mStop || !mTaskQueue.empty(); });

        if (mStop && mTaskQueue.empty())
            return;
        task = std::move(mTaskQueue.front());
        mTaskQueue.pop();

        mIdleThreads -= 1;
        locker.unlock();

        task();

        locker.lock();
        mIdleThreads += 1;
        /*
         * In order to prevent frequent generation and release of threads, redundant threads
         * can be released only when the number of idle threads is greater than the initial
         * number of threads and the number of idle threads is greater than twice the number
         * of remaining tasks.
         * It can ensure that when the task comes fast, the idle thread will not be released,
         * and when the task comes slow, the idle thread will be released.
         */
        uint32_t beNeededThreadNum = static_cast<uint32_t>(mTaskQueue.size()) * 2;
        if ((mIdleThreads > mInitThreadNum) && (mIdleThreads > beNeededThreadNum)) {
            mIdleThreads -= 1;
            mActiveThreadNum -= 1;
            mPendingRemoveThreadIds.push_back(thisThreadId);
            MLOGI(Mia2LogGroupCore, "idle threads number:%u, activeThreadNum:%u, taskNum: %zu",
                  mIdleThreads, mActiveThreadNum, mTaskQueue.size());
            return;
        }
    }
}

int ThreadPool::enqueue(Task task)
{
    {
        std::lock_guard<std::mutex> locker(mMutex);
        if (mStop) {
            // FIXME: assign a more readable  error code and give some log
            return -1;
        }

        while (mPendingRemoveThreadIds.size() > 0) {
            int threadId = mPendingRemoveThreadIds.back();
            auto it = mWorkers.find(threadId);
            if (it != mWorkers.end()) {
                if (it->second.joinable()) {
                    it->second.join(); // must after loop() , before  ~std::thread
                }
                mWorkers.erase(it); // ~std::thread, must after join
                MLOGI(Mia2LogGroupCore, "removed thread, threadId: %d", threadId);
            }
            mPendingRemoveThreadIds.pop_back();
        }

        mTaskQueue.emplace(task);

        uint32_t taskNums = static_cast<uint32_t>(mTaskQueue.size());
        // When the threadNum does not exceed maxThreadNum value and there are not enough idle
        // threads, add one thread
        if ((mActiveThreadNum < mMaxThreadNum) && (taskNums > mIdleThreads)) {
            addThread();
        }

        MLOGI(Mia2LogGroupCore, "Idle threads number: %u, mAllActiveThreads:%u, task num: %u",
              mIdleThreads, mActiveThreadNum, taskNums);
    }
    mCond.notify_one();
    return 0;
}

// the destructor joins all threads
ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> locker(mMutex);
        mStop = true;
    }
    mCond.notify_all();

    for (auto &worker : mWorkers)
        worker.second.join();
}

} // namespace mialgo2
