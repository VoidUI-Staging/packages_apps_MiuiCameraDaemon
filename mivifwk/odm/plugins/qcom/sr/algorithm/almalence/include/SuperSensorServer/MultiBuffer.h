#ifndef __MULTIBUFFER_H__
#define __MULTIBUFFER_H__

#include <SuperSensorServer/None.h>
#include <SuperSensorServer/SyncedMediator.h>
#include <stdarg.h>
#include <stdint.h>

#include <algorithm>
#include <list>
#include <set>

namespace almalence {

/**
 * SyncedMediator implementation which carries several same type items and prioritizes which
 * should be read and written when locks are requested.
 * @tparam T stored item type.
 */
template <typename T>
class MultiBuffer : public SyncedMediator<T, T, None>
{
private:
    template <typename Y>
    static bool contains(const std::list<Y> list, const Y item)
    {
        return std::find(list.begin(), list.end(), item) != list.end();
    }

    const std::function<void(T *const)> deinitializer;

    const std::size_t count;

    T **buffers;

    std::set<T *> unlocked;
    std::list<T *> unlockedInput;
    T *current;

protected:
    virtual bool isItemReadyForWriting(None) { return this->unlocked.size() > 0; }

    virtual bool isItemReadyForReading() { return contains(this->unlockedInput, this->current); }

    virtual T *acquireItemForWriting(None)
    {
        for (auto iteratorUnlockedInput = this->unlockedInput.begin();
             iteratorUnlockedInput != this->unlockedInput.end(); ++iteratorUnlockedInput) {
            auto item = *iteratorUnlockedInput;
            auto iteratorUnlocked = this->unlocked.find(item);
            if (iteratorUnlocked != this->unlocked.end()) {
                this->unlocked.erase(iteratorUnlocked);
                this->unlockedInput.erase(iteratorUnlockedInput);
                return item;
            }
        }

        throw std::runtime_error("No unlocked item found in Multibuffer.");
    }

    virtual T *acquireItemForReading()
    {
        this->unlocked.erase(this->current);
        return this->current;
    }

    virtual void onItemWritingFinished(T *value)
    {
        this->current = value;
        this->unlocked.insert(value);
        this->unlockedInput.push_back(value);
    }

    virtual void onItemReadingFinished(T *value) { this->unlocked.insert(value); }

public:
    MultiBuffer(const std::size_t count, const std::function<T *()> initializer,
                const std::function<void(T *const)> deinitializer)
        : deinitializer(deinitializer), count(count), buffers(nullptr)
    {
        try {
            this->buffers = new T *[count];

            for (std::size_t i = 0; i < count; i++) {
                this->buffers[i] = initializer();
                this->unlocked.insert(this->buffers[i]);
                this->unlockedInput.push_back(this->buffers[i]);
            }
        } catch (const std::bad_alloc &e) {
            delete[] this->buffers;
            throw e;
        }
    }

    virtual ~MultiBuffer()
    {
        for (std::size_t i = 0; i < this->count; i++) {
            this->deinitializer(this->buffers[i]);
        }

        delete[] this->buffers;
    }
};

} // namespace almalence

#endif //__MULTIBUFFER_H__
