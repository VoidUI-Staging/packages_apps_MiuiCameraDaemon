#include "OfflineDebugDataUtils.h"

void *GetDebugDataBufferAndAddReference(size_t oriSize)
{
    return OfflineDebugDataManager::getInstance()->getDebugDataBufferAndAddReference(oriSize);
}

uint32_t SubDebugDataReference(void *pDebugData)
{
    return OfflineDebugDataManager::getInstance()->subDebugDataReference(pDebugData);
}

void SetSessionStatus(bool status)
{
    OfflineDebugDataManager::getInstance()->setSessionStatus(status);
}

void SetVTStatus(bool status)
{
    OfflineDebugDataManager::getInstance()->setVTStatus(status);
}
