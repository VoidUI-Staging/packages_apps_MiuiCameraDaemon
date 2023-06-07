/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MIA_PROVIDERMANAGER__
#define __MIA_PROVIDERMANAGER__

#include <MiaProvider.h>
#include <utils/Log.h>

#include <list>
#include <map>
#include <vector>

namespace mialgo2 {

class ProviderManager
{
    friend class PluginManager;
    friend class Provider;

public:
    ////////////////////////////////////////////////////////////
    /// \brief Add provider.
    ///
    /// Provider type and version are checked. Only known and
    /// valid provider types are accepted.
    ///
    /// \param provider Provider to be added.
    ///
    /// \return True if the provider is accepted.
    ///
    ////////////////////////////////////////////////////////////
    bool add(Provider *provider);

private:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor.
    ///
    /// New ProviderManager instances are not publicly allowed.
    ///
    ////////////////////////////////////////////////////////////
    ProviderManager();

    ////////////////////////////////////////////////////////////
    /// \brief Destructor.
    ///
    /// Clears all hosted providers
    ///
    ////////////////////////////////////////////////////////////
    ~ProviderManager();

    ////////////////////////////////////////////////////////////
    /// \brief Ckeck if a provider type is registered.
    ///
    /// \param type Provider type id.
    ///
    /// \return True if the type is registered
    ///
    ////////////////////////////////////////////////////////////
    bool knows(const std::string &type) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get version of a type of providers.
    ///
    /// \param type Provider type.
    ///
    /// \return The version of the provider type.
    ///
    ////////////////////////////////////////////////////////////
    unsigned int getVersion(const std::string &type) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get lowest compatible version of a type of providers.
    ///
    /// \param type Provider type.
    ///
    /// \return The lowest compatible version of the provider type.
    ///
    ////////////////////////////////////////////////////////////
    unsigned int getLowestVersion(const std::string &type) const;

    ////////////////////////////////////////////////////////////
    /// \brief Register a type of providers.
    ///
    /// \param type Provider type.
    /// \param version Current version of that provider type.
    /// \param lowestVersion Lowest compatible version of that provider type.
    ///
    ////////////////////////////////////////////////////////////
    void registerType(const std::string &type, unsigned int version, unsigned int lowestVersion);

    ////////////////////////////////////////////////////////////
    /// \brief Get providers of a certain type.
    ///
    /// \param type Provider type.
    ///
    /// \return Pointer to the list of providers of that \a type,
    /// or NULL if \a type is not registered.
    ///
    ////////////////////////////////////////////////////////////
    const std::list<Provider *> *getProviders(const std::string &type) const;

    ////////////////////////////////////////////////////////////
    /// \brief Clears all hosted providers.
    ///
    ////////////////////////////////////////////////////////////
    void clearProviders();

    ////////////////////////////////////////////////////////////
    /// \brief Validate provider type and version.
    ///
    /// \return True if the provider is acceptable.
    ///
    ////////////////////////////////////////////////////////////
    bool validateProvider(Provider *provider) const;

    ////////////////////////////////////////////////////////////
    /// \brief Clearly add a provider.
    ///
    /// Provider type and version are checked. Only known and
    /// valid provider types are accepted.
    /// If acepted, provider is directly stored.
    ///
    /// \param provider Provider to be added.
    ///
    /// \return True if the provider is accepted.
    ///
    ////////////////////////////////////////////////////////////
    bool registerProvider(Provider *provider);

    ////////////////////////////////////////////////////////////
    /// \brief Previous add calls are canceled.
    ///
    /// Added providers are not stored.
    ///
    /// \see add
    ///
    ////////////////////////////////////////////////////////////
    void cancelAddictions();

    ////////////////////////////////////////////////////////////
    /// \brief Previous add calls are confirmed.
    ///
    /// Added providers are finally stored.
    ///
    /// \return True if something was stored.
    ///
    /// \see add
    ///
    ////////////////////////////////////////////////////////////
    bool confirmAddictions();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////

private:
    ////////////////////////////////////////////////////////////
    /// \brief Structure with information about a provider type.
    ///
    ////////////////////////////////////////////////////////////
    struct ProviderInfo
    {
        unsigned int version;
        unsigned int lowestVersion;
        std::list<Provider *> providers;
    };

    typedef std::map<std::string, ProviderInfo> ProvidersMap;
    typedef std::map<std::string, std::list<Provider *> > TempProvidersMap;

    ProvidersMap mKnownTypes;      ///< Map of registered types.
    TempProvidersMap mAddRequests; ///< Temporarily added providers
};

} // namespace mialgo2

#endif //__MIA_PROVIDERMANAGER__