#ifndef __POINTERDATAADAPTER_H__
#define __POINTERDATAADAPTER_H__

#include "dataAdapter.h"

class PointerDataAdapter : public DataAdapter
{
public:
    PointerDataAdapter(const void *pointer = nullptr) : pointer(pointer) {}

    void setpPointer(const void *pointer) { this->pointer = pointer; }

    const void *getPointer() { return pointer; }

    const void *getPointer() const { return pointer; }

    template <typename T>
    T getDataWithDefault(T value)
    {
        if (pointer) {
            return *static_cast<const T *>(getPointer());
        }
        return value;
    }

    operator const void *() { return getPointer(); }

    operator const void *() const { return getPointer(); }

    operator char() { return getDataWithDefault<char>(0); }

    operator short() { return getDataWithDefault<short>(0); }

    operator long() { return getDataWithDefault<long>(0); }

    operator int() { return getDataWithDefault<int>(0); }

    operator float() { return getDataWithDefault<float>(0.0f); }

    operator double() { return getDataWithDefault<double>(0); }

    operator long double() { return getDataWithDefault<long double>(0); }

    operator unsigned char() { return getDataWithDefault<unsigned char>(0); }

    operator unsigned short() { return getDataWithDefault<unsigned short>(0); }

    operator unsigned long() { return getDataWithDefault<unsigned long>(0); }

    operator unsigned int() { return getDataWithDefault<unsigned int>(0); }

    operator bool() { return getDataWithDefault<bool>(false); }

private:
    const void *pointer;
};

#endif
