/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _MI_PLUGINMODULE_H_
#define _MI_PLUGINMODULE_H_

#include <MiaPluginWraper.h>
#include <pluma.h>

#include <vector>

namespace mialgo2 {

class MPluginModule
{
public:
    /// Return the singleton instance of ExtensionModule
    static MPluginModule *getInstance() { return s_pluginModuleSingleton; }

    PluginWraper *getPluginWraperByPluginName(std::string pluginName);

private:
    /// Constructor
    MPluginModule();
    /// Destructor
    ~MPluginModule();

    MPluginModule(const MPluginModule &rMPluginModule) = delete;
    MPluginModule &operator=(const MPluginModule &rMPluginModule) = delete;

    Pluma mPlugins;
    std::vector<PluginWraperProvider *> mPluginWraperProviderVec;

    static MPluginModule *s_pluginModuleSingleton;
};

} // namespace mialgo2
#endif //_MI_PLUGINMODULE_H_