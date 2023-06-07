#ifndef __TIMER_H__
#define __TIMER_H__

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>

namespace asio {

template <typename Clock>
struct wait_traits;

class any_io_executor;

#if !defined(ASIO_BASIC_WAITABLE_TIMER_FWD_DECL)
#define ASIO_BASIC_WAITABLE_TIMER_FWD_DECL
template <typename Clock, typename WaitTraits = wait_traits<Clock>,
          typename Executor = any_io_executor>
class basic_waitable_timer;
#endif

typedef basic_waitable_timer<std::chrono::steady_clock> steady_timer;
} // namespace asio

namespace mihal {

class Timer final : public std::enable_shared_from_this<Timer>
{
public:
    using TimerHandler = std::function<void()>;
    static std::shared_ptr<Timer> createTimer();
    ~Timer();

    int runAfter(uint32_t ms, TimerHandler handler);
    void cancel();

private:
    Timer();
    std::unique_ptr<asio::steady_timer> mImpl;
    TimerHandler mHandler;
    std::mutex mMutex;
};

} // namespace mihal

#endif
