/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __DYNAMIC_LIBRARY__
#define __DYNAMIC_LIBRARY__

#include <dlfcn.h>

#include <string>

#include "Config.h"

namespace mialgo2 {

////////////////////////////////////////////////////////////
/// \brief Manages a Dynamic Linking Library.
///
////////////////////////////////////////////////////////////
class DLibrary
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Load a library.
    ///
    /// \param path Path to the library.
    ///
    /// \return Pointer to the loaded library, or NULL if failed.
    ///
    ////////////////////////////////////////////////////////////
    static DLibrary *load(const std::string &path);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor.
    ///
    /// Close and free the opened library (if any).
    ///
    ////////////////////////////////////////////////////////////
    ~DLibrary();

    ////////////////////////////////////////////////////////////
    /// \brief Get a symbol from the library.
    ///
    /// \param symbol Symbol that we're looking for.
    ///
    /// \return Pointer to what the symbol refers to, or NULL if
    /// the symbol is not found.
    ///
    ////////////////////////////////////////////////////////////
    void *getSymbol(const std::string &symbol);

private:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor.
    ///
    /// Library instances cannot be created, use load instead.
    ///
    /// \see load
    ///
    ////////////////////////////////////////////////////////////
    DLibrary();

    ////////////////////////////////////////////////////////////
    /// \brief Constructor via library handle.
    ///
    /// Used on load function.
    ///
    /// \see load
    ///
    ////////////////////////////////////////////////////////////
    DLibrary(void *handle);

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////

private:
    void *mHandle; ///< Library handle.
};

} // namespace mialgo2

#endif //__DYNAMIC_LIBRARY__