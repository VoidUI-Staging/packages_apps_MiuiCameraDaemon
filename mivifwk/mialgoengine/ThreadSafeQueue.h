#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace mialgo2 {

/**
 * A thread-safe wrapper around std::queue.
 * @tparam T An arbitrary type.
 */
template <class T, class Container = std::queue<T>>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue() : mRelease(false) {}

    bool push(const T &t)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!this->mRelease) {
            mQueue.push(t);
            mNotEmptyCondVar.notify_one();
            return true;
        }
        return false;
    }

    bool push(T &&t)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!this->mRelease) {
            mQueue.push(std::move(t));
            mNotEmptyCondVar.notify_one();
            return true;
        }
        return false;
    }

    bool waitAndPop(T &t)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mNotEmptyCondVar.wait(lock, [this]() { return this->mRelease || !this->mQueue.empty(); });

        if (mRelease && mQueue.empty())
            return false;

        t = std::move(mQueue.front());
        mQueue.pop();
        return true;
    }

    bool tryPop(T &t)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mQueue.empty()) {
            return false;
        }

        t = std::move(mQueue.front());
        mQueue.pop();

        return true;
    }

    bool getFront(T &t)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mQueue.empty()) {
            return false;
        }

        t = mQueue.front();

        return true;
    }

    // some error need to check
    bool waitTimePop(T &t)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        const auto status = mNotEmptyCondVar.wait_for(lock, std::chrono::milliseconds(22));
        if (status == std::cv_status::timeout) {
            return false;
        }

        if (mRelease && mQueue.empty())
            return false;

        t = std::move(mQueue.front());
        mQueue.pop();
        return true;
    }

    bool isEmpty() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mQueue.empty();
    }

    void clearQueue()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        while (!mQueue.empty())
            mQueue.pop();
        return;
    }

    void releaseQueue()
    {
        {
            std::unique_lock<std::mutex> lock(mMutex);
            this->mRelease = true;
        }
        mNotEmptyCondVar.notify_all();
    }

    int queueSize()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        return mQueue.size();
    }

    T getFront() { return mQueue.front(); }

private:
    ThreadSafeQueue(const ThreadSafeQueue &) = delete;
    ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;
    ThreadSafeQueue(ThreadSafeQueue &&) = delete;
    ThreadSafeQueue &operator=(ThreadSafeQueue &&) = delete;

private:
    Container mQueue;

    std::condition_variable mNotEmptyCondVar;
    bool mRelease;
    mutable std::mutex mMutex;
};

} // namespace mialgo2
#endif