/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaProvider.h"

#include "MiaProviderManager.h"

namespace mialgo2 {

////////////////////////////////////////////////////////////
Provider::~Provider()
{
    // Nothing to do
}

////////////////////////////////////////////////////////////
bool Provider::isCompatible(const ProviderManager &host) const
{
    // check compatibility with host
    const std::string &type = this->plumaGetType();
    if (!host.knows(type))
        return false;
    unsigned int lowest = host.getLowestVersion(type);
    unsigned int current = host.getVersion(type);
    unsigned int myVersion = this->getVersion();
    return lowest <= myVersion && myVersion <= current;
}

} // namespace mialgo2