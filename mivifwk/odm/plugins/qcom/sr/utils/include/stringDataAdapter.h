#ifndef __STRINGDATAADAPTER_H__
#define __STRINGDATAADAPTER_H__

#include <sstream>
#include <string>
#include "dataAdapter.h"

class StringDataAdapter : public DataAdapter
{
public:
    StringDataAdapter() {}
    StringDataAdapter(std::string data) : data(data) {}
    void setString(std::string data) { this->data = data; }

    template <typename T>
    T getData()
    {
        T result;
        std::stringstream(data) >> result;
        return result;
    }

    operator char() { return getData<char>(); }

    operator short() { return getData<short>(); }

    operator long() { return getData<long>(); }

    operator int() { return getData<int>(); }

    operator float() { return getData<float>(); }

    operator double() { return getData<double>(); }

    operator long double() { return getData<long double>(); }

    operator unsigned char() { return getData<unsigned char>(); }

    operator unsigned short() { return getData<unsigned short>(); }

    operator unsigned long() { return getData<unsigned long>(); }

    operator unsigned int() { return getData<unsigned int>(); }

    operator bool() { return getData<bool>(); }

private:
    std::string data;
};
#endif
