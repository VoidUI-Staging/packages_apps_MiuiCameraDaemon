/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _TIMER_H_
#define _TIMER_H_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace mialgo2 {

class FakeTimer
{
public:
    FakeTimer() : mExpired(true) {}

    ~FakeTimer() { stop(); }

    void start(int interval, std::function<void()> task)
    {
        // is started, do not start again
        if (mExpired == false)
            return;

        // start async timer, launch thread and wait in that thread
        mExpired = false;
        mThread = std::thread([this, interval, task]() {
            while (true) {
                // sleep every interval and do the task again and again until times up
                std::unique_lock<std::mutex> locker(mMutex);
                if (!mExpiredCond.wait_for(locker, std::chrono::milliseconds(interval),
                                           [this] { return mExpired; })) {
                    task();
                } else {
                    break;
                }
            }
        });
    }

    void stop()
    {
        // do not stop again
        if (mExpired)
            return;

        {
            // timer be stopped, update the condition variable expired and wake main thread
            std::lock_guard<std::mutex> locker(mMutex);
            mExpired = true; // change this bool value to make timer while loop stop
            mExpiredCond.notify_one();
        }

        if (mThread.joinable()) {
            mThread.join();
        }
    }

private:
    bool mExpired; // timer stopped status
    std::mutex mMutex;
    std::condition_variable mExpiredCond;
    std::thread mThread;
};

} // namespace mialgo2

#endif // !_TIMER_H_
