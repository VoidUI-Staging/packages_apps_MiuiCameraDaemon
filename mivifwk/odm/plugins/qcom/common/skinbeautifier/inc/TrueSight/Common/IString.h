#pragma once

#include <cstdlib>
#include <TrueSight/TrueSightDefine.hpp>
#include "IVector.h"

namespace TrueSight {

class TrueSight_PUBLIC IString {
public:
    IString();
    IString(const IString &x);
    explicit IString(const char *c_str);
    ~IString();

    IString &operator=(const char *c_str);
    IString &operator=(const IString &x);

    bool empty() const;
    size_t size() const;

    const char *c_str() const;
    const char *data() const;

private:
    void *val = nullptr;
};

} // namespace TrueSight