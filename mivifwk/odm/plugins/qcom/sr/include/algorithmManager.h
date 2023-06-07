#ifndef __ALGORITHMMANAGER_H_
#define __ALGORITHMMANAGER_H_

#include "algoAdapter.h"

Entrance getAlgoAdapter(unsigned int index);

Entrance getAlgoAdapter();

void setAdapterIndex(unsigned int index);

unsigned int getIndexByName(const char * name);

void process(const SessionInfo *sessionInfo, const ProcessRequestInfo *request);

const char *getAlgoName();

#endif
