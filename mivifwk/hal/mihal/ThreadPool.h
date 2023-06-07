/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MIHAL_THREAD_POOL_H__
#define __MIHAL_THREAD_POOL_H__

#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <stack>
#include <thread>
#include <vector>

#include "LogUtil.h"

namespace mihal {

template <typename T>
class Singleton
{
public:
    template <typename... Args>
    static T *Instance(Args &&... args)
    {
        if (m_pInstance == nullptr)
            m_pInstance = new T(std::forward<Args>(args)...);
        return m_pInstance;
    }

    static T *getInstance()
    {
        if (m_pInstance == nullptr) {
            MLOGE(LogGroupHAL, "the instance is not init,please initialize the instance first");
            return nullptr;
        }
        return m_pInstance;
    }

    static void DestroyInstance()
    {
        delete m_pInstance;
        m_pInstance = nullptr;
    }

private:
    Singleton(void);
    virtual ~Singleton(void);
    Singleton(const Singleton &);
    Singleton &operator=(const Singleton &);

private:
    static T *m_pInstance;
};

template <class T>
T *Singleton<T>::m_pInstance = nullptr;

class ThreadPool
{
public:
    using Task = std::function<void()>;

    // TODO: add more feature to enable pre-start, max threads, etc...
    ThreadPool(uint32_t threads, int priority = 10); // 10 = 130 - 120, 130 is priority
    ~ThreadPool();
    int enqueue(Task task);
    void setThreadPoolPriority(int priority);

private:
    void loop(int thisThreadId);
    void addThread();
    // need to keep track of threads so we can join them
    std::map<int, std::thread> mWorkers;
    // the task queue
    std::queue<Task> mTaskQueue;
    // synchronization
    std::mutex mMutex;
    std::condition_variable mCond;
    bool mStop;

    std::atomic<int> mThreadId;
    int mThreadPriority;
};

} // namespace mihal

#endif // __MIHAL_THREAD_POOL_H__
