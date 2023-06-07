#ifndef __FILTER_H__
#define __FILTER_H__

class Filter
{
public:
    virtual bool conform() = 0;
}

typename<T> class EqualFilter : public Filter
{
public:
    bool conform();
    EqualFilter(T *, T) private : const T condition;
    T *const environment;
}

typename<T> class RangeFilter : public Filter
{
public:
    bool conform();
    RangeFilter(T *, T, T);

private:
    const T big;
    const T small;
    T *const condition;
}

class ComplexFilter : public Filter
{
public:
    bool conform();
    void pushBack(const Filter *) private : vector<const Filter *> filters;
}
#endif
