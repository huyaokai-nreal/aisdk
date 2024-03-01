#pragma once

//
// native plugin API
//

#if defined(__CYGWIN32__) || defined(_WIN32)
#define NR_INTERFACE_API __cdecl
#define NR_INTERFACE_EXPORT __declspec(dllexport)
#if defined(NR_INTERFACE_DEPRECATION_SUPPRESS)
#define NR_INTERFACE_DEPRECATED(__msg__)
#else
#define NR_INTERFACE_DEPRECATED(__msg__) __declspec(deprecated(__msg__))
#endif
#elif defined(__MACH__) || defined(__ANDROID__) || defined(__linux__)
#define NR_INTERFACE_API
#define NR_INTERFACE_EXPORT __attribute__((visibility("default")))
#if defined(NR_INTERFACE_DEPRECATION_SUPPRESS)
#define NR_INTERFACE_DEPRECATED(__msg__)
#else
#define NR_INTERFACE_DEPRECATED(__msg__) __attribute__((deprecated(__msg__)))
#endif
#else
#define NR_INTERFACE_API
#define NR_INTERFACE_EXPORT
#define NR_INTERFACE_DEPRECATED(__msg__)
#endif

//
// NRInterfaceGUID ensure NRInterface globally unique
//

struct NRInterfaceGUID {
#ifdef __cplusplus
    NRInterfaceGUID(unsigned long long high, unsigned long long low)
        : high_value_(high), low_value_(low) {}

    NRInterfaceGUID(const NRInterfaceGUID& other) {
        high_value_ = other.high_value_;
        low_value_ = other.low_value_;
    }

    NRInterfaceGUID& operator=(const NRInterfaceGUID& other) {
        high_value_ = other.high_value_;
        low_value_ = other.low_value_;
        return *this;
    }

    bool Equals(const NRInterfaceGUID& other) const {
        return high_value_ == other.high_value_ &&
               low_value_ == other.low_value_;
    }
    bool LessThan(const NRInterfaceGUID& other) const {
        return high_value_ < other.high_value_ ||
               (high_value_ == other.high_value_ &&
                low_value_ < other.low_value_);
    }
#endif
    unsigned long long high_value_;
    unsigned long long low_value_;
};
#ifdef __cplusplus
inline bool operator==(const NRInterfaceGUID& left,
                       const NRInterfaceGUID& right) {
    return left.Equals(right);
}
inline bool operator!=(const NRInterfaceGUID& left,
                       const NRInterfaceGUID& right) {
    return !left.Equals(right);
}
inline bool operator<(const NRInterfaceGUID& left,
                      const NRInterfaceGUID& right) {
    return left.LessThan(right);
}
inline bool operator>(const NRInterfaceGUID& left,
                      const NRInterfaceGUID& right) {
    return right.LessThan(left);
}
inline bool operator>=(const NRInterfaceGUID& left,
                       const NRInterfaceGUID& right) {
    return !operator<(left, right);
}
inline bool operator<=(const NRInterfaceGUID& left,
                       const NRInterfaceGUID& right) {
    return !operator>(left, right);
}
#else
typedef struct NRInterfaceGUID NRInterfaceGUID;
#endif

#ifdef __cplusplus

#define NR_DECLARE_INTERFACE(__NAME__) struct __NAME__ : NRInterface

//
// Generic version of GetNRInterfaceGUID to allow us to gain interface guid.
// The generic version do not have implementation.
//
template <typename TYPE>
inline const NRInterfaceGUID GetNRInterfaceGUID();

//
// This macro is for mapping between special type of NRInterface and
// NRInterfaceGUID
//
#define NR_REGISTER_INTERFACE_GUID(__HASHH__, __HASHL__, __TYPE__) \
    template <>                                                    \
    inline const NRInterfaceGUID GetNRInterfaceGUID<__TYPE__>() {  \
        return NRInterfaceGUID(__HASHH__, __HASHL__);              \
    }

//
// This macro is for C compatibility
//
#define NR_GET_INTERFACE_GUID(__TYPE__) GetNRInterfaceGUID<__TYPE__>()

#else
#define NR_DECLARE_INTERFACE(__NAME__) \
    typedef struct __NAME__ __NAME__;  \
    struct __NAME__

#define NR_REGISTER_INTERFACE_GUID(__HASHH__, __HASHL__, __TYPE__) \
    const NRInterfaceGUID __TYPE__##_GUID = {__HASHH__, __HASHL__};

//
// This macro is for C compatibility
//
#define NR_GET_INTERFACE_GUID(__TYPE__) __TYPE__##_GUID
#endif

//
// NRInterface is the interface between plugin and SDK
//

#ifdef __cplusplus
struct NRInterface {};
#else
typedef void NRInterface;
#endif

typedef struct NRInterfaces {
    //
    // Parameter out_interface_size can be NULL.
    // Return an interface which match the guid.
    // Return nullptr if the given plugin is not supported in current SDK
    // runtime.
    //
    NRInterface*(NR_INTERFACE_API* GetInterface)(
        NRInterfaceGUID guid, unsigned long long* out_interface_size);

#ifdef __cplusplus
    // CPP helper for GetInterface
    template <typename INTERFACE>
    INTERFACE* Get(unsigned long long* out_interface_size) {
        return static_cast<INTERFACE*>(
            GetInterface(GetNRInterfaceGUID<INTERFACE>(), out_interface_size));
    }
#endif
} NRInterfaces;

#ifdef __cplusplus
extern "C" {
#endif

// If exported as a plugin, this function will be called when the plugin is
// loaded.
void NR_INTERFACE_EXPORT NR_INTERFACE_API
NRPluginLoad(NRInterfaces* interfaces);
// If exported as a plugin, this function will be called when the plugin is
// unloaded.
void NR_INTERFACE_EXPORT NR_INTERFACE_API NRPluginUnload();

#ifdef __cplusplus
}
#endif

#ifdef NRSDK

#include "nr_plugin_interface.inc"

#else  // NRSDK

#endif  // NRSDK
