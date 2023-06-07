#include "algorithmManager.h"
#include <string>

extern Entrance ENTRLIST;

static Entrance entranceList[] = {ENTRLIST};
static unsigned int index = 0;

Entrance getAlgoAdapter(unsigned int index)
{
    if (index < sizeof(entranceList) / sizeof(entranceList[0]))
        return entranceList[index];
    return *entranceList;
}

Entrance getAlgoAdapter()
{
    return getAlgoAdapter(index);
}

void setAdapterIndex(unsigned int index)
{
    ::index = index % (sizeof(entranceList) / sizeof(entranceList[0]));
}

unsigned int getIndexByName(const char * name)
{
    const std::string algoName = name;
    for (unsigned int i = 0; i < sizeof(entranceList) / sizeof(entranceList[0]); i++)
        if (algoName == entranceList[i]->getName())
            return i;
    return 0;
}

void process(const SessionInfo *sessionInfo, const ProcessRequestInfo *request)
{
    return getAlgoAdapter()->process(sessionInfo, request);
}

const char *getAlgoName()
{
    return getAlgoAdapter()->getName();
}
