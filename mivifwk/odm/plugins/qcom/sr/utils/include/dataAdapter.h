#ifndef __DATAADAPTER_H__
#define __DATAADAPTER_H__

class DataAdapter
{
public:
    virtual operator char() = 0;

    virtual operator short() = 0;

    virtual operator long() = 0;

    virtual operator int() = 0;

    virtual operator float() = 0;

    virtual operator double() = 0;

    virtual operator long double() = 0;

    virtual operator unsigned char() = 0;

    virtual operator unsigned short() = 0;

    virtual operator unsigned long() = 0;

    virtual operator unsigned int() = 0;

    virtual operator bool() = 0;

    virtual ~DataAdapter() {}
};

#endif
