/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MIA_PROVIDER__
#define __MIA_PROVIDER__

#include <string>

namespace mialgo2 {

class ProviderManager;

////////////////////////////////////////////////////////////
/// \brief Interface to provide applications with objects from plugins.
///
////////////////////////////////////////////////////////////
class Provider
{
    friend class ProviderManager;

public:
    ////////////////////////////////////////////////////////////
    /// \brief Destructor.
    ///
    ////////////////////////////////////////////////////////////
    virtual ~Provider();

    ////////////////////////////////////////////////////////////
    /// \brief Get provider version.
    ///
    /// \return Version number.
    ///
    ////////////////////////////////////////////////////////////
    virtual unsigned int getVersion() const = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Check compatibility with host.
    ///
    /// The same provider may be compiled with different versions
    /// on host side and on plugins side. This function checks if
    /// a plugin provider is compatible with the current version of
    /// the same provider type on the host side.
    ///
    /// \param host Host, proxy of host application.
    ///
    /// \return True if it's compatible with \a host.
    ///
    ////////////////////////////////////////////////////////////
    bool isCompatible(const ProviderManager &host) const;

private:
    ////////////////////////////////////////////////////////////
    /// \brief Get provider type.
    ///
    /// Each provider defined on the host application is identified by
    /// a unique type. Those types are automatically managed internally by
    /// pluma.
    ///
    /// \return Provider type id.
    ///
    ////////////////////////////////////////////////////////////
    virtual std::string plumaGetType() const = 0;
};

} // namespace mialgo2

#endif //__MIA_PROVIDER__