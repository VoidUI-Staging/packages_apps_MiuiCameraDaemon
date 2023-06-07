#ifndef __CONFIG_PAIR_H__
#define __CONFIG_PAIR_H__

#include <string.h>

#include "filter.h"

using namespace std;

class Config
{
public:
    Config(int, const String);
    int getCameraId();
    const char *getParam();

private:
    const int cameraId;
    const String param;
}

class ConfigPair
{
public:
    ConfigPair(const Filter *, const Config(int, const String));
    const Filter *getFilter();
    const Config *getConfig();

private:
    const Filter *const filter;
    const Config config;
};

#endif
