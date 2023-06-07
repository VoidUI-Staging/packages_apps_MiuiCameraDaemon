#include "Timer.h"

#include <MiDebugUtils.h>

#include <asio.hpp>
#include <memory>
#include <thread>
using namespace midebug;

namespace {
class TimerManager final
{
public:
    static TimerManager *getInstance();
    std::unique_ptr<asio::steady_timer> createSteadyTimer()
    {
        return std::make_unique<asio::steady_timer>(mContext);
    }

private:
    TimerManager();
    ~TimerManager();

    asio::io_context mContext;
    asio::io_context::work mWork;
    std::thread mThread;
};

TimerManager *TimerManager::getInstance()
{
    static TimerManager mgr{};
    return &mgr;
}

TimerManager::TimerManager()
    : mWork{mContext}, mThread{[this]() {
          MLOGI(LogGroupHAL, "Timer worker thread started");
          mContext.run();
      }}
{
}

TimerManager::~TimerManager()
{
    mContext.stop();
    mThread.join();
}

} // namespace

namespace mizone {

Timer::Timer() = default;

Timer::~Timer()
{
    cancel();
}

std::shared_ptr<Timer> Timer::createTimer()
{
    return std::shared_ptr<Timer>{new Timer{}};
}

int Timer::runAfter(uint32_t ms, TimerHandler handler)
{
    std::lock_guard<std::mutex> locker{mMutex};
    if (!mImpl)
        mImpl = TimerManager::getInstance()->createSteadyTimer();

    if (!handler) {
        MLOGE(LogGroupHAL, "timer handler is null");
        return -EINVAL;
    }
    mHandler = handler;

    std::weak_ptr<Timer> weakThis{shared_from_this()};
    mImpl->expires_from_now(asio::chrono::milliseconds(ms));
    mImpl->async_wait([weakThis](const asio::error_code &code) {
        if (code) {
            return;
        }

        std::shared_ptr<Timer> that = weakThis.lock();
        if (!that) {
            return;
        }

        std::lock_guard<std::mutex> locker{that->mMutex};
        that->mHandler();
    });

    return 0;
}

void Timer::cancel()
{
    std::lock_guard<std::mutex> locker{mMutex};
    if (mImpl)
        mImpl->cancel();
}

} // namespace mizone
