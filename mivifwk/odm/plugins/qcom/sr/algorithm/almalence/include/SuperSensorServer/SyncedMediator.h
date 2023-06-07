#ifndef __SWAPPER_H__
#define __SWAPPER_H__

#include <SuperSensorServer/Lock.h>
#include <SuperSensorServer/Mediator.h>

#include <condition_variable>
#include <map>
#include <mutex>
#include <set>
#include <typeinfo>

namespace almalence {

/**
 * Mediator subclass which takes multithreading and resources locking into consideration.
 *
 * @tparam IN input type.
 * @tparam OUT output type.
 * @tparam IN_PARAM input parameter type.
 */
template <typename IN, typename OUT, typename IN_REQUEST>
class SyncedMediator : public Mediator<IN, OUT, IN_REQUEST>
{
protected:
    virtual bool isItemReadyForWriting(IN_REQUEST request) = 0;
    virtual bool isItemReadyForReading() = 0;
    virtual IN *acquireItemForWriting(IN_REQUEST request) = 0;
    virtual OUT *acquireItemForReading() = 0;
    virtual void onItemWritingFinished(IN *value) {}
    virtual void onItemReadingFinished(OUT *value) {}

private:
    enum State { EMPTY, AVAILABLE, AVAILABLE_NEXT };

    enum LockTuple { ITEM, WRITE, COUNT };

    std::mutex mutex;
    std::condition_variable cond;

    bool streamEnded;

    State state;
    void *latestWrite;

    std::map<int, std::tuple<void *, bool, std::uint32_t>> locks;
    int id_seq;

    std::function<void()> availabilityListener = []() -> void {};

    // Host for managing writing operations to SyncedMediator
    class OutputStreamHost : public OutputStream<IN, IN_REQUEST>
    {
    private:
        SyncedMediator &outter;

    public:
        OutputStreamHost(SyncedMediator &outter) : outter(outter) {}

        Lock<IN> acquire(IN_REQUEST request)
        {
            std::unique_lock<std::mutex> lk(this->outter.mutex);

            this->outter.cond.wait(
                lk, [&]() -> bool { return this->outter.isItemReadyForWriting(request); });

            const auto id = ++this->outter.id_seq;
            const auto item = this->outter.acquireItemForWriting(request);

            this->outter.locks.insert(std::make_pair(id, std::make_tuple(item, true, 1)));
            return Lock<IN>(this->outter, std::make_pair(id, item));
        }
    } outputStreamHost;

    // Host for managing reading operations from SyncedMediator
    class InputStreamHost : public InputStream<OUT>
    {
    private:
        SyncedMediator &outter;

    public:
        InputStreamHost(SyncedMediator &outter) : outter(outter) {}

        Lock<OUT> acquire(bool next = true)
        {
            std::unique_lock<std::mutex> lk(this->outter.mutex);

            /*
             * Wait until either:
             *  - Stream is ended
             *  - State isn't 'empty' and there is an item ready to be read and image is available
             *  or not required.
             */
            this->outter.cond.wait(lk, [&]() -> bool {
                return this->outter.streamEnded ||
                       (this->outter.state != EMPTY &&
                        (!next || this->outter.state == AVAILABLE_NEXT) &&
                        this->outter.isItemReadyForReading());
            });

            if (this->outter.state == AVAILABLE_NEXT) {
                this->outter.state = AVAILABLE;
            }

            if (this->outter.streamEnded) {
                throw std::out_of_range("End of stream.");
            }

            const auto id = ++this->outter.id_seq;
            const auto item = this->outter.acquireItemForReading();

            this->outter.locks.insert(std::make_pair(id, std::make_tuple(item, false, 1)));
            return Lock<OUT>(this->outter, std::make_pair(id, item));
        }

        void endStream()
        {
            std::unique_lock<std::mutex> lk(this->outter.mutex);
            this->outter.streamEnded = true;
            this->outter.cond.notify_all();
        }
    } inputStreamHost;

public:
    SyncedMediator()
        : streamEnded(false),
          state(EMPTY),
          latestWrite(nullptr),
          id_seq(0),
          outputStreamHost(*this),
          inputStreamHost(*this)
    {
    }

    virtual ~SyncedMediator()
    {
        /*
         * To be honest this is paranoid and should be guaranteed by the user.
         * Waits for all objects being unlocked.
         */
        std::unique_lock<std::mutex> lk(this->mutex);
        this->cond.wait(lk, [&]() -> bool { return this->locks.empty(); });
    }

    OutputStream<IN, IN_REQUEST> &asOutput() { return this->outputStreamHost; }

    InputStream<OUT> &asInput() { return this->inputStreamHost; }

    void setAvailabilityListener(const std::function<void()> &listener)
    {
        this->availabilityListener = listener;
    }

    void lockIncrement(const int lockId)
    {
        std::unique_lock<std::mutex> lk(this->mutex);
        auto iterator = this->locks.find(lockId);
        if (iterator == this->locks.end()) {
            throw std::invalid_argument("bad lock id.");
        }

        ++std::get<COUNT>(iterator->second);
    }

    void lockDecrement(const int lockId)
    {
        std::unique_lock<std::mutex> lk(this->mutex);
        auto iterator = this->locks.find(lockId);
        if (iterator == this->locks.end()) {
            throw std::invalid_argument("bad lock id.");
        }

        const auto item = std::get<ITEM>(iterator->second);
        const auto write = std::get<WRITE>(iterator->second);
        const auto count = --std::get<COUNT>(iterator->second);

        // If locks count == 0, remove item from locks list.
        if (count == 0) {
            this->locks.erase(iterator);
            if (write) {
                // If item was aquired for writing (asOutput).
                this->state = AVAILABLE_NEXT;
                this->latestWrite = item;
                this->onItemWritingFinished(static_cast<IN *>(item));

                this->availabilityListener();
            } else {
                // If item was aquired for reading (asInput).
                if (item == this->latestWrite) {
                    this->latestWrite = nullptr;
                }
                this->onItemReadingFinished(static_cast<OUT *>(item));
            }
            this->cond.notify_all();
        }
    }

    /**
     * Blocks the calling thread until the latest written object is read.
     */
    void waitDrained()
    {
        std::unique_lock<std::mutex> lk(this->mutex);
        const auto latest = this->latestWrite;
        if (latest != nullptr) {
            this->cond.wait(lk, [&]() -> bool { return latest != this->latestWrite; });
        }
    }
};

} // namespace almalence

#endif // __SWAPPER_H__
