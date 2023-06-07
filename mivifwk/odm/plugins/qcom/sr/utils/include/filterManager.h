#ifndef __FILTER_MANAGER_H__
#define __FILTER_MANAGER_H__

void createFilter(const map<string, pair *> *filterCreaterManager, string path);

const char *getConfigVersion();

const char *getConfigComment();

unsigned int getAlgoConfigIndex();

const char *getAlgoConfig(unsigned int index);
#endif
