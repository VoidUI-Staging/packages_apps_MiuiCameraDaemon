/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MIA_PLUGINMANAGER__
#define __MIA_PLUGINMANAGER__

#include <map>
#include <string>

#include "Config.h"
#include "MiaProviderManager.h"

#define MaxPlugins 30

namespace mialgo2 {

class DLibrary;

////////////////////////////////////////////////////////////
/// \brief Manages loaded plugins.
///
////////////////////////////////////////////////////////////
class PluginManager
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Destructor.
    ///
    ////////////////////////////////////////////////////////////
    ~PluginManager();

    bool load(const std::string &path);

    bool load(const std::string &folder, const std::string &pluginName);

    int loadFromFolder(const std::string &folder, bool recursive = false);

    bool unload(const std::string &pluginName);

    void unloadAll();

    bool addProvider(Provider *provider);

    void getLoadedPlugins(std::vector<const std::string *> &pluginNames) const;

    bool isLoaded(const std::string &pluginName) const;

protected:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor.
    ///
    /// PluginManager cannot be publicly instantiated.
    ///
    ////////////////////////////////////////////////////////////
    PluginManager();

    void registerType(const std::string &type, unsigned int version, unsigned int lowestVersion);

    const std::list<Provider *> *getProviders(const std::string &type) const;

private:
    static std::string getPluginName(const std::string &path);

    static std::string resolvePathExtension(const std::string &path);

private:
    /// Signature for the plugin's registration function
    typedef bool fnRegisterPlugin(ProviderManager &);
    typedef std::map<std::string, DLibrary *> LibMap;

    LibMap mLibraries;     ///< Map containing the loaded mLibraries
    ProviderManager mHost; ///< Host app proxy, holding all providers
};

} // namespace mialgo2

#endif //__MIA_PLUGINMANAGER__