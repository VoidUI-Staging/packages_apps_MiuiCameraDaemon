#ifndef _PLUMA_H_
#define _PLUMA_H_

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <Config.h>
#include <MiaPluginManager.h>
#include <MiaProvider.h>

////////////////////////////////////////////////////////////
// Andy macro to convert parameter to string
////////////////////////////////////////////////////////////
#define PLUMA_2STRING(X) #X

// Only define the extern "C" for other platforms
#ifdef __cplusplus
#define PLUMA_CONNECTOR extern "C"
#else
#define PLUMA_CONNECTOR
#endif

////////////////////////////////////////////////////////////
// Macro that helps host applications defining
// their provider classes
////////////////////////////////////////////////////////////
#define PLUMA_PROVIDER_HEADER(TYPE)          \
    PLUMA_PROVIDER_HEADER_BEGIN(TYPE)        \
    virtual TYPE *create() const = 0;        \
    virtual std::string getName() const = 0; \
    PLUMA_PROVIDER_HEADER_END

////////////////////////////////////////////////////////////
// Macro that generate first part of the provider definition
////////////////////////////////////////////////////////////
#define PLUMA_PROVIDER_HEADER_BEGIN(TYPE)                                \
    class TYPE##Provider : public mialgo2::Provider                      \
    {                                                                    \
    private:                                                             \
        friend class mialgo2::Pluma;                                     \
        static const unsigned int PLUMA_INTERFACE_VERSION;               \
        static const unsigned int PLUMA_INTERFACE_LOWEST_VERSION;        \
        static const std::string PLUMA_PROVIDER_TYPE;                    \
        std::string plumaGetType() const { return PLUMA_PROVIDER_TYPE; } \
                                                                         \
    public:                                                              \
        unsigned int getVersion() const { return PLUMA_INTERFACE_VERSION; }

////////////////////////////////////////////////////////////
// Macro that generate last part of the provider definition
////////////////////////////////////////////////////////////
#define PLUMA_PROVIDER_HEADER_END \
    }                             \
    ;

////////////////////////////////////////////////////////////
// Macro that generate the provider declaration
////////////////////////////////////////////////////////////
#define PLUMA_PROVIDER_SOURCE(TYPE, Version, LowestVersion)                      \
    const std::string TYPE##Provider::PLUMA_PROVIDER_TYPE = PLUMA_2STRING(TYPE); \
    const unsigned int TYPE##Provider::PLUMA_INTERFACE_VERSION = Version;        \
    const unsigned int TYPE##Provider::PLUMA_INTERFACE_LOWEST_VERSION = LowestVersion;

////////////////////////////////////////////////////////////
// Macro that helps plugins generating their provider implementations
// PRE: SPECIALIZED_TYPE must inherit from BASE_TYPE
////////////////////////////////////////////////////////////
#define PLUMA_INHERIT_PROVIDER(SPECIALIZED_TYPE, BASE_TYPE)                       \
    class SPECIALIZED_TYPE##Provider : public BASE_TYPE##Provider                 \
    {                                                                             \
    public:                                                                       \
        BASE_TYPE *create() const { return new SPECIALIZED_TYPE(); }              \
        std::string getName() const { return SPECIALIZED_TYPE::getPluginName(); } \
    };

namespace mialgo2 {

////////////////////////////////////////////////////////////
/// \brief Pluma plugins management
///
////////////////////////////////////////////////////////////
class Pluma : public PluginManager
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default Constructor
    ///
    ////////////////////////////////////////////////////////////
    Pluma();

    ////////////////////////////////////////////////////////////
    /// \brief Tell Pluma to accept a certain type of providers
    ///
    /// A Pluma object is able to accept multiple types of providers.
    /// When a plugin is loaded, it tries to register it's providers
    /// implementations. Those are only accepted by the host
    /// application if it's accepting providers of that kind.
    ///
    /// \tparam ProviderType type of provider.
    ///
    ////////////////////////////////////////////////////////////
    template <typename ProviderType>
    void acceptProviderType();

    ////////////////////////////////////////////////////////////
    /// \brief Get the stored providers of a certain type.
    ///
    /// Providers are added at the end of the \a providers vector.
    ///
    /// \tparam ProviderType type of provider to be returned.
    /// \param[out] providers Vector to fill with the existing
    /// providers.
    ///
    ////////////////////////////////////////////////////////////
    template <typename ProviderType>
    void getProviders(std::vector<ProviderType *> &providers);
};

#include <pluma.inl>

} // namespace mialgo2

#endif //_PLUMA_H_