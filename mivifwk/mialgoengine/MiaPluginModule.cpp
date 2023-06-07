/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaPluginModule.h"

#include "MiaUtil.h"

namespace mialgo2 {

MPluginModule *MPluginModule::s_pluginModuleSingleton = new MPluginModule();

PluginWraper *MPluginModule::getPluginWraperByPluginName(std::string pluginName)
{
    if (pluginName.empty()) {
        MLOGE(Mia2LogGroupCore, "error pluginName is empty");
        return NULL;
    }
    for (auto i : mPluginWraperProviderVec) {
        if (!i->getName().compare(pluginName)) {
            MLOGI(Mia2LogGroupCore, "find pluginName: %s", pluginName.c_str());
            return i->create();
        }
    }

    MLOGE(Mia2LogGroupCore, "error not found pluginName: %s", pluginName.c_str());
    return NULL;
}

MPluginModule::MPluginModule()
{
    MLOGD(Mia2LogGroupCore, "E");
    // Tell plugins manager to accept providers of the type PluginWraperProvider
    mPlugins.acceptProviderType<PluginWraperProvider>();
    // Load libraries
    mPlugins.loadFromFolder("plugins");

    // Get device providers into a vector
    mPlugins.getProviders(mPluginWraperProviderVec);

    // create a Device  from the first provider
    int j = 0;
    for (auto i : mPluginWraperProviderVec) {
        MLOGI(Mia2LogGroupCore, "List all plugin %d name: %s", j++, i->getName().c_str());
    }
}

MPluginModule::~MPluginModule()
{
    mPluginWraperProviderVec.clear();
    mPlugins.unloadAll();
}

} // namespace mialgo2
