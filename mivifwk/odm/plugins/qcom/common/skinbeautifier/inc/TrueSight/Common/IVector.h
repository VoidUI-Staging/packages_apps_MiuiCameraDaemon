#pragma once

#include <cstdlib>
#include <TrueSight/TrueSightDefine.hpp>

namespace TrueSight {

/*
 * 接口参数所使用的IVector等效于std::vector
 */
template <class T> class TrueSight_PUBLIC IVector {
  public:
    typedef T valueType;
    typedef T &reference;
    typedef const T &const_reference;
	typedef T *pointer;
    typedef const T *const_pointer;
    typedef ::size_t size_t;

  public:
    IVector();
    explicit IVector(size_t n);
    IVector(size_t n, const valueType &value);
    IVector(const IVector &x);
    IVector(IVector &&x) noexcept;
    ~IVector();

    IVector &operator=(const IVector &x);
    IVector &operator=(IVector &&x) noexcept;
    void assign(size_t n, const valueType &u);

    size_t size() const;
    size_t max_size() const;
    size_t capacity() const;
    bool empty() const;
    void reserve(size_t n);
    void shrink_to_fit();

    reference operator[](size_t n);
    const_reference operator[](size_t n) const;
    reference at(size_t n);
    const_reference at(size_t n) const;

    reference front();
    const_reference front() const;
    reference back();
    const_reference back() const;

    void pop_back();
    void push_back(const_reference _x);
    void push_back(valueType &&_x);

    valueType *data();
    const valueType *data() const;

    void clear();

    void resize(size_t sz);
    void resize(size_t sz, const valueType &c);

    void swap(IVector &x);

  private:
    void *val;
};

} // namespace TrueSight
