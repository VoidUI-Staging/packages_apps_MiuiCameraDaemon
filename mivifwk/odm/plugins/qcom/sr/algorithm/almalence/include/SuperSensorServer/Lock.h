#ifndef __LOCKABLE_H__
#define __LOCKABLE_H__

#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace almalence {

/**
 * Lockable interface.
 * Intended to increment and decrement locks count for specific holder id.
 */
class Lockable
{
public:
    virtual void lockIncrement(const int lockId) = 0;
    virtual void lockDecrement(const int lockId) = 0;

    virtual ~Lockable() {}
};

/**
 * Lock holding class. Provides access to the locked resource.
 * @tparam T locked resource type.
 */
template <typename T>
class Lock
{
private:
    Lockable &lockable;
    const std::pair<int, void *> lock;

    bool alive;

    inline void assertAlive() const
    {
        if (!this->alive) {
            throw std::runtime_error("Lock not alive.");
        }
    }

public:
    inline Lock(Lockable &lockable, const std::pair<int, void *> lock)
        : lockable(lockable), lock(lock), alive(true)
    {
    }

    inline Lock(const Lock<T> &other) = delete;

    inline Lock(Lock<T> &other) : lockable(other.lockable), lock(other.lock), alive(other.alive)
    {
        if (this->alive) {
            this->lockable.lockIncrement(this->lock.first);
        }
    }

    inline Lock(Lock<T> &&other) : lockable(other.lockable), lock(other.lock), alive(other.alive)
    {
        if (other.alive) {
            other.alive = false;
        }
    }

    inline virtual ~Lock()
    {
        if (this->alive) {
            this->lockable.lockDecrement(this->lock.first);
        }
    }

    inline const T &operator*() const
    {
        this->assertAlive();
        return *static_cast<T *>(this->lock.second);
    }

    inline T &operator*()
    {
        this->assertAlive();
        return *static_cast<T *>(this->lock.second);
    }

    inline const T *operator->() const
    {
        this->assertAlive();
        return static_cast<T *>(this->lock.second);
    }

    inline T *operator->()
    {
        this->assertAlive();
        return static_cast<T *>(this->lock.second);
    }
};

} // namespace almalence

#endif // __LOCKABLE_H__
