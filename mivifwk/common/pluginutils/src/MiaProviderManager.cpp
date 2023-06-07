/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include <MiaPluginUtils.h>
#include <MiaProviderManager.h>

namespace mialgo2 {

////////////////////////////////////////////////////////////
ProviderManager::ProviderManager()
{
    // Nothing to do
}

////////////////////////////////////////////////////////////
bool ProviderManager::add(Provider *provider)
{
    if (provider == NULL) {
        MLOGE(Mia2LogGroupCore, "Trying to add a null provider");
        return false;
    }
    if (!validateProvider(provider)) {
        delete provider;
        return false;
    }
    mAddRequests[provider->plumaGetType()].push_back(provider);
    return true;
}

////////////////////////////////////////////////////////////
ProviderManager::~ProviderManager()
{
    clearProviders();
    // map frees itself
}

////////////////////////////////////////////////////////////
void ProviderManager::clearProviders()
{
    ProvidersMap::iterator it;
    for (it = mKnownTypes.begin(); it != mKnownTypes.end(); ++it) {
        std::list<Provider *> &providers = it->second.providers;
        std::list<Provider *>::iterator provIt;
        for (provIt = providers.begin(); provIt != providers.end(); ++provIt) {
            delete *provIt;
        }
        std::list<Provider *>().swap(providers);
    }
}

////////////////////////////////////////////////////////////
bool ProviderManager::knows(const std::string &type) const
{
    return mKnownTypes.find(type) != mKnownTypes.end();
}

////////////////////////////////////////////////////////////
unsigned int ProviderManager::getVersion(const std::string &type) const
{
    ProvidersMap::const_iterator it = mKnownTypes.find(type);
    if (it != mKnownTypes.end())
        return it->second.version;
    return 0;
}

////////////////////////////////////////////////////////////
unsigned int ProviderManager::getLowestVersion(const std::string &type) const
{
    ProvidersMap::const_iterator it = mKnownTypes.find(type);
    if (it != mKnownTypes.end())
        return it->second.lowestVersion;
    return 0;
}

////////////////////////////////////////////////////////////
void ProviderManager::registerType(const std::string &type, unsigned int version,
                                   unsigned int lowestVersion)
{
    if (!knows(type)) {
        ProviderInfo pi;
        pi.version = version;
        pi.lowestVersion = lowestVersion;
        mKnownTypes[type] = pi;
    }
}

////////////////////////////////////////////////////////////
const std::list<Provider *> *ProviderManager::getProviders(const std::string &type) const
{
    ProvidersMap::const_iterator it = mKnownTypes.find(type);
    if (it != mKnownTypes.end())
        return &it->second.providers;
    return NULL;
}

////////////////////////////////////////////////////////////
bool ProviderManager::validateProvider(Provider *provider) const
{
    const std::string &type = provider->plumaGetType();
    if (!knows(type)) {
        MLOGE(Mia2LogGroupCore, "%s provider type isn't registered", type.c_str());
        return false;
    }
    if (!provider->isCompatible(*this)) {
        MLOGE(Mia2LogGroupCore, "Incompatible %s provider version", type.c_str());
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////
bool ProviderManager::registerProvider(Provider *provider)
{
    if (!validateProvider(provider)) {
        delete provider;
        return false;
    }
    mKnownTypes[provider->plumaGetType()].providers.push_back(provider);
    return true;
}

////////////////////////////////////////////////////////////
void ProviderManager::cancelAddictions()
{
    TempProvidersMap::iterator it;
    for (it = mAddRequests.begin(); it != mAddRequests.end(); ++it) {
        std::list<Provider *> lst = it->second;
        std::list<Provider *>::iterator providerIt;
        for (providerIt = lst.begin(); providerIt != lst.end(); ++providerIt) {
            delete *providerIt;
        }
    }
    // clear map
    TempProvidersMap().swap(mAddRequests);
}

////////////////////////////////////////////////////////////
bool ProviderManager::confirmAddictions()
{
    if (mAddRequests.empty())
        return false;
    TempProvidersMap::iterator it;
    for (it = mAddRequests.begin(); it != mAddRequests.end(); ++it) {
        std::list<Provider *> lst = it->second;
        std::list<Provider *>::iterator providerIt;
        for (providerIt = lst.begin(); providerIt != lst.end(); ++providerIt) {
            mKnownTypes[it->first].providers.push_back(*providerIt);
        }
    }
    // clear map
    TempProvidersMap().swap(mAddRequests);
    return true;
}

} // namespace mialgo2
