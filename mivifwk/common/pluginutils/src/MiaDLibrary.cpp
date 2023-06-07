/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaDLibrary.h"

#include "MiaPluginUtils.h"

namespace mialgo2 {

////////////////////////////////////////////////////////////
DLibrary *DLibrary::load(const std::string &path)
{
    if (path.empty()) {
        MLOGE(Mia2LogGroupCore, "Failed to load library: Empty path");
        return NULL;
    }
    void *mHandle = NULL;
    mHandle = ::dlopen(path.c_str(), RTLD_NOW);
    if (!mHandle) {
        const char *errorString = ::dlerror();
        MLOGE(Mia2LogGroupCore, "Failed to load library %s.", path.c_str());
        if (errorString)
            MLOGE(Mia2LogGroupCore, "OS returned error: %s", errorString);
        return NULL;
    }
    // return a DLibrary with the DLL handle
    return new DLibrary(mHandle);
}

////////////////////////////////////////////////////////////
DLibrary::~DLibrary()
{
    if (mHandle) {
        ::dlclose(mHandle);
    }
}

////////////////////////////////////////////////////////////
void *DLibrary::getSymbol(const std::string &symbol)
{
    if (!mHandle) {
        MLOGE(Mia2LogGroupCore, "Cannot inspect library symbols, library isn't loaded.");
        return NULL;
    }
    void *res;
    res = (void *)(::dlsym(mHandle, symbol.c_str()));
    if (!res) {
        MLOGE(Mia2LogGroupCore, "Library symbol %s not found", symbol.c_str());
        return NULL;
    }
    return res;
}

////////////////////////////////////////////////////////////
DLibrary::DLibrary(void *handle) : mHandle(handle)
{
    // Nothing to do
}

} // namespace mialgo2
