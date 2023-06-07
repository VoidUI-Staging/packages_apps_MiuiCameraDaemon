#ifndef __SUPERSENSORSERVER_PROCESSING_H__
#define __SUPERSENSORSERVER_PROCESSING_H__

#include <SuperSensorServer/Mediator.h>
#include <SuperSensorServer/SuperParcel.h>
#include <SuperSensorServer/common.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace almalence {

/**
 * Universal Service class which runs it's own thread to process data in a loop.
 * @tparam IN input data type.
 * @tparam OUT output data type.
 * @tparam OUT_PARAMS output lock parameters type list.
 */
template <typename IN, typename OUT, typename OUT_PARAM>
class Service
{
    static_assert(std::is_base_of<SuperParcel, IN>::value, "TIN must derive from SuperParcel");
    static_assert(std::is_base_of<SuperParcel, OUT>::value, "TOUT must derive from SuperParcel");

private:
    enum State { NONE, ON_START, STARTED, ON_STOP, STOPPED };

    std::mutex mutex;
    std::condition_variable cond;

    std::thread thread;

    State state;

    std::atomic_bool shutdownFlag;

    const std::function<void()> onServiceStart;
    const std::function<void()> onServiceStop;
    InputStream<IN> *const input;
    OutputStream<OUT, OUT_PARAM> *const output;

    void updateState(const State state)
    {
        std::unique_lock<std::mutex> lk(this->mutex);
        this->state = state;
        this->cond.notify_all();
    }

    void waitState(const State state)
    {
        std::unique_lock<std::mutex> lk(this->mutex);
        this->cond.wait(lk, [&]() -> bool { return this->state == state; });
    }

    void run()
    {
        // Try to increase priority to the highest via Android-specific function.
        nice(-20);

        this->onServiceStart();

        this->updateState(STARTED);

        try {
            while (true) {
                // Lock input object
                auto lockInput = this->input->acquire(true);
                if (this->shutdownFlag) {
                    break;
                }

                // Lock output object
                auto lockOutput = this->acquireOutputLock(this->composeOutputRequest(*lockInput));
                if (this->shutdownFlag) {
                    break;
                }

                try {
                    // Passing global SuperParcel objects is done with push-pop mechanics.
                    lockOutput->push(*lockInput);

                    // Call virtual processing method.
                    this->process(*lockInput, *lockOutput);

                    // Passing global SuperParcel objects is done with push-pop mechanics.
                    lockOutput->pop();
                } catch (const std::exception &ex) {
                    lockOutput->setError(ex.what());
                } catch (...) {
                    lockOutput->setError("Unknown exception.");
                }
                if (this->shutdownFlag) {
                    break;
                };
            }
        } catch (const std::out_of_range &e) {
        }

        this->updateState(ON_STOP);
        this->onServiceStop();

        this->updateState(STOPPED);
    }

    Lock<OUT> acquireOutputLock(OUT_PARAM param) { return this->output->acquire(param); }

protected:
    void stop()
    {
        this->shutdownFlag = true;
        this->input->endStream();
        this->waitState(STOPPED);
    }

    virtual OUT_PARAM composeOutputRequest(IN &in) = 0;

    virtual void process(IN &in, OUT &out) = 0;

public:
    Service(const std::function<void()> onStart, const std::function<void()> onStop,
            InputStream<IN> *const input, OutputStream<OUT, OUT_PARAM> *const output)
        : state(NONE),
          shutdownFlag(false),
          onServiceStart(onStart),
          onServiceStop(onStop),
          input(input),
          output(output)
    {
    }

    Service(const Service &) = delete;

    Service &operator=(const Service &) = delete;

    virtual ~Service()
    {
        this->stop();
        this->thread.join();
    }

    void start()
    {
        std::unique_lock<std::mutex> lk(this->mutex);

        if (this->state != NONE) {
            throw std::runtime_error("Already started");
        }

        this->state = ON_START;

        this->thread = std::thread([&]() -> void { this->run(); });
    }
};

} // namespace almalence

#endif //__SUPERSENSORSERVER_PROCESSING_H__
