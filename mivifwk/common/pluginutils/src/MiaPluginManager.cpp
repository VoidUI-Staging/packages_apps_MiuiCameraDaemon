/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaPluginManager.h"

#include "MiaDLibrary.h"
#include "MiaPlatformSetting.h"
#include "MiaPluginUtils.h"

namespace mialgo2 {

////////////////////////////////////////////////////////////
PluginManager::PluginManager()
{
    // Nothing to do
}

////////////////////////////////////////////////////////////
PluginManager::~PluginManager()
{
    unloadAll();
}

////////////////////////////////////////////////////////////
bool PluginManager::load(const std::string &path)
{
    std::string plugName = getPluginName(path);
    std::string realPath = resolvePathExtension(path);
    DLibrary *lib = DLibrary::load(realPath);
    if (!lib)
        return false;

    fnRegisterPlugin *registerFunction;
    registerFunction = reinterpret_cast<fnRegisterPlugin *>(lib->getSymbol("connect"));

    if (!registerFunction) {
        MLOGE(Mia2LogGroupCore, "%s Failed to initialize plugin %s connect function not found",
              __func__, plugName.c_str());
        delete lib;
        return false;
    }

    MLOGD(Mia2LogGroupCore, "Initialize plugin %s connect function sucessed", plugName.c_str());

    // try to initialize plugin:
    if (!registerFunction(mHost)) {
        // plugin decided to fail
        MLOGE(Mia2LogGroupCore, "Self registry failed on plugin %s", plugName.c_str());
        mHost.cancelAddictions();
        delete lib;
        return false;
    }
    // Store the library if addictions are confirmed
    if (mHost.confirmAddictions())
        mLibraries[plugName] = lib;
    else {
        // otherwise nothing was registered
        MLOGE(Mia2LogGroupCore, "Nothing registered by plugin %s", plugName.c_str());
        delete lib;
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////
bool PluginManager::load(const std::string &folder, const std::string &pluginName)
{
    if (folder.empty())
        return load(pluginName);
    else if (folder[folder.size() - 1] == '/' || folder[folder.size() - 1] == '\\')
        return load(folder + pluginName);
    return load(folder + '/' + pluginName);
}

////////////////////////////////////////////////////////////
int PluginManager::loadFromFolder(const std::string &folder, bool recursive)
{
    // NOTE: These parameters are not used by now but may be used in future
    // we put (void) ahead of them just to make the compiler happy so that the
    // compiler won't complain about "unused parameter"
    (void)folder;
    (void)recursive;

    std::list<std::string> files;
    int fileCountTypeNode = 0;
    fileCountTypeNode = PluginUtils::getFilesFromPath(ExtendPluginPath, FILENAME_MAX, files, "*",
                                                      "plugin", "*", PLUMA_LIB_EXTENSION);

    MLOGD(Mia2LogGroupCore, "fileCountTypeNode: %d", fileCountTypeNode);

    // try to load every library
    int res = 0;
    std::list<std::string>::const_iterator it;
    for (it = files.begin(); it != files.end(); ++it) {
        if (load(*it))
            ++res;
    }
    return res;
}

////////////////////////////////////////////////////////////
bool PluginManager::unload(const std::string &pluginName)
{
    std::string plugName = getPluginName(pluginName);
    LibMap::iterator it = mLibraries.find(plugName);
    if (it != mLibraries.end()) {
        delete it->second;
        mLibraries.erase(it);
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////
void PluginManager::unloadAll()
{
    mHost.clearProviders();
    LibMap::iterator it;
    for (it = mLibraries.begin(); it != mLibraries.end(); ++it) {
        delete it->second;
    }
    mLibraries.clear();
}

////////////////////////////////////////////////////////////
std::string PluginManager::getPluginName(const std::string &path)
{
    size_t lastDash = path.find_last_of("/\\");
    size_t lastDot = path.find_last_of('.');
    if (lastDash == std::string::npos)
        lastDash = 0;
    else
        ++lastDash;
    if (lastDot < lastDash || lastDot == std::string::npos) {
        // path without extension
        lastDot = path.length();
    }
    return path.substr(lastDash, lastDot - lastDash);
}

////////////////////////////////////////////////////////////
std::string PluginManager::resolvePathExtension(const std::string &path)
{
    size_t lastDash = path.find_last_of("/\\");
    size_t lastDot = path.find_last_of('.');
    if (lastDash == std::string::npos)
        lastDash = 0;
    else
        ++lastDash;
    if (lastDot < lastDash || lastDot == std::string::npos) {
        // path without extension, add it
        return path + "." + PLUMA_LIB_EXTENSION;
    }
    return path;
}

////////////////////////////////////////////////////////////
void PluginManager::registerType(const std::string &type, unsigned int version,
                                 unsigned int lowestVersion)
{
    mHost.registerType(type, version, lowestVersion);
}

////////////////////////////////////////////////////////////
bool PluginManager::addProvider(Provider *provider)
{
    if (provider == NULL) {
        MLOGE(Mia2LogGroupCore, "Trying to add null provider");
        return false;
    }
    return mHost.registerProvider(provider);
}

////////////////////////////////////////////////////////////
void PluginManager::getLoadedPlugins(std::vector<const std::string *> &pluginNames) const
{
    pluginNames.reserve(pluginNames.size() + mLibraries.size());
    LibMap::const_iterator it;
    for (it = mLibraries.begin(); it != mLibraries.end(); ++it) {
        pluginNames.push_back(&(it->first));
    }
}

////////////////////////////////////////////////////////////
bool PluginManager::isLoaded(const std::string &pluginName) const
{
    return mLibraries.find(getPluginName(pluginName)) != mLibraries.end();
}

////////////////////////////////////////////////////////////
const std::list<Provider *> *PluginManager::getProviders(const std::string &type) const
{
    return mHost.getProviders(type);
}

} // namespace mialgo2
