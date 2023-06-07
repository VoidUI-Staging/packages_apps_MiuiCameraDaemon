#ifndef __DUMPER_H__
#define __DUMPER_H__

#include <SuperSensorServer/common.h>
#include <string.h>

#include <stdexcept>

namespace almalence {

class Dumper
{
private:
    const std::uint32_t countMax;
    const std::uint32_t totalSize;
    std::uint8_t *data;
    std::uint32_t *sizes;
    std::string *tags;
    std::uint32_t *jump_table;
    std::uint32_t count;
    std::uint32_t index;

    inline std::uint8_t *getItem(const std::uint32_t index)
    {
        return &this->data[this->jump_table[index]];
    }

    inline std::uint32_t currentPosition() const { return this->jump_table[this->count]; }

public:
    Dumper(const std::uint32_t countMax, const std::uint32_t totalSize)
        : countMax(countMax),
          totalSize(totalSize),
          data(nullptr),
          sizes(nullptr),
          tags(nullptr),
          jump_table(nullptr),
          count(0),
          index(0)
    {
        try {
            this->data = new std::uint8_t[totalSize];
            this->sizes = new std::uint32_t[countMax];
            this->tags = new std::string[countMax];
            this->jump_table = new std::uint32_t[countMax + 1];
        } catch (const std::bad_alloc &e) {
            Log<ANDROID_LOG_ERROR>("Dumper ERROR");
            delete[] this->data;
            delete[] this->sizes;
            delete[] this->tags;
            delete[] this->jump_table;
            throw e;
        }

        this->jump_table[0] = 0;
    }

    virtual ~Dumper()
    {
        delete[] this->data;
        delete[] this->sizes;
        delete[] this->tags;
        delete[] this->jump_table;
    }

    bool dump(uint8_t *const data, const std::uint32_t size, const std::string &tag)
    {
        if (this->isFull(size)) {
            return false;
        }

        memcpy(this->getItem(this->count), data, size);
        this->sizes[this->count] = size;
        this->tags[this->count] = tag;
        this->count++;

        this->jump_table[this->count] = this->jump_table[this->count - 1] + size;

        return true;
    }

    void flush() { this->count = 0; }

    void save(const char *const path)
    {
        char fpath[512];
        const std::uint32_t plen = strlen(path);
        memcpy(fpath, path, plen);

        for (std::uint32_t i = 0; i < this->count; i++) {
            sprintf(fpath + plen, "/dump_%04d_%s", this->index++, this->tags[i].c_str());
            FILE *const f = fopen(fpath, "wb");
            if (f == NULL) {
                return;
            }
            fwrite(this->getItem(i), this->sizes[i], 1, f);
            fclose(f);
        }
    }

    std::uint32_t getCount() const { return this->count; }

    bool isSlotArrayFull() const { return this->getCount() == this->countMax; }

    bool isBufferFull(const std::uint32_t size) const
    {
        return this->currentPosition() + size >= this->totalSize;
    }

    bool isFull(const std::uint32_t size) const
    {
        return this->isSlotArrayFull() || this->isBufferFull(size);
    }
};

} // namespace almalence

#endif
