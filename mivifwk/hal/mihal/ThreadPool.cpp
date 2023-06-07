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

namespace mihal {

// the constructor just launches some amount of workers
// TODO: add more feature to enable pre-start, max threads, etc...
ThreadPool::ThreadPool(uint32_t threads, int priority) : mStop(false), mThreadPriority(priority)
{
    mThreadId = 0;
    for (size_t i = 0; i < threads; ++i) {
        addThread();
    }
}

void ThreadPool::addThread()
{
    int thisThreadId = atomic_fetch_add(&mThreadId, 1);
    mWorkers.insert(std::make_pair(thisThreadId, [this, thisThreadId]() { loop(thisThreadId); }));
}

void ThreadPool::loop(int thisThreadId)
{
    std::stringstream sin;
    sin << std::this_thread::get_id();
    std::string threadName = std::string("MiHalThd") + sin.str();
    threadName += "_";
    threadName += std::to_string(thisThreadId);
    prctl(PR_SET_NAME, threadName.c_str());
    SetTaskProfiles(gettid(), {"SCHED_SP_FOREGROUND"}, true);

    std::unique_lock<std::mutex> locker(mMutex);
    setpriority(PRIO_PROCESS, 0, mThreadPriority);
    int priority = mThreadPriority;
    while (true) {
        Task task;

        mCond.wait(locker, [this]() { return mStop || !mTaskQueue.empty(); });

        if (mStop && mTaskQueue.empty()) {
            return;
        }

        if (priority != mThreadPriority) {
            setpriority(PRIO_PROCESS, 0, mThreadPriority);
            priority = mThreadPriority;
        }
        task = std::move(mTaskQueue.front());
        mTaskQueue.pop();

        locker.unlock();
        task();
        locker.lock();
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

        mTaskQueue.emplace(task);
    }
    mCond.notify_one();
    return 0;
}

void ThreadPool::setThreadPoolPriority(int priority = 0)
{
    std::lock_guard<std::mutex> locker(mMutex);
    mThreadPriority = priority;
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

} // namespace mihal
