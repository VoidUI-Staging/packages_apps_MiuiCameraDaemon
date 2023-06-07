/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <stack>
#include <thread>
#include <vector>

#include "MiaSettings.h"

namespace mialgo2 {

class ThreadPool
{
public:
    using Task = std::function<void()>;

    // TODO: add more feature to enable pre-start, max threads, etc...
    ThreadPool(uint32_t threads);
    ~ThreadPool();
    int enqueue(Task task);

private:
    void loop(int thisThreadId);
    void addThread();
    // need to keep track of threads so we can join them
    std::map<int, std::thread> mWorkers;
    std::vector<int> mPendingRemoveThreadIds;
    // the task queue
    std::queue<Task> mTaskQueue;
    // synchronization
    std::mutex mMutex;
    std::condition_variable mCond;
    bool mStop;
    uint32_t mInitThreadNum;
    uint32_t mIdleThreads;
    uint32_t mActiveThreadNum;
    const uint32_t mMaxThreadNum;
    std::atomic<int> mThreadId;
};

} // namespace mialgo2

#endif // __THREAD_POOL_H__
