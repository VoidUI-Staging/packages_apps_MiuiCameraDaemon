#ifndef __FILTERCREATER_H__
#define __FILTERCREATER_H__

class FilterCreater
{
public:
    virtual Filter *create(const JsonObject *);
    const char *getName();

protected:
    FilterCreater(const char *);

private:
    const char *const name;
};

template <typename EnvType>
class EnvFilterCreater : public FilterCreater
{
protected:
    EnvFilterCreater(const char *, const EnvType *);
    EnvType *getEnv();

private:
    const EnvType *const pEnv;
};

template <typename EnvType>
class RangeFilterCreater : public EnvFilterCreater<EnvType>
{
public:
    RangeFilterCreater(const char *name, const Envtype *)
};

template <typename EnvType>
class EqualFilterCreater : public EnvFilterCreater<EnvType>
{
public:
    EqualFilterCreater(const char *name, const EnvType *);
};

#endif
