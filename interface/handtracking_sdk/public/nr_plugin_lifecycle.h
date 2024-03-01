#pragma once

#include "nr_plugin_interface.h"
#include "nr_plugin_types.h"

typedef struct NRPluginLifecycleProvider {
    /// @brief Register provider-related-functions. Called just after
    /// "NRPluginLoad". note: Register providers except LifecycleProvider.
    /// LifecycleProvider should be registered in NRPluginLoad.
    ///
    /// @param handle:
    NRPluginResult(NR_INTERFACE_API* Register)(NRPluginHandle handle);

    /// @brief Initialize the plugin
    ///
    /// @param handle: the unique symbol to represent one plugin, which can be
    /// passed to metheds relate to this plugin
    NRPluginResult(NR_INTERFACE_API* Initialize)(NRPluginHandle handle);

    /// @brief Start the plugin
    ///
    /// @param handle:
    NRPluginResult(NR_INTERFACE_API* Start)(NRPluginHandle handle);

    /// @brief Update the plugin. Not used currently
    ///
    /// @param handle:
    NRPluginResult(NR_INTERFACE_API* Update)(NRPluginHandle handle);

    /// @brief Pause the plugin.
    ///
    /// @param handle:
    NRPluginResult(NR_INTERFACE_API* Pause)(NRPluginHandle handle);

    /// @brief Resume the plugin.
    ///
    /// @param handle:
    NRPluginResult(NR_INTERFACE_API* Resume)(NRPluginHandle handle);

    /// @brief Stop the plugin.
    ///
    /// @param handle:
    NRPluginResult(NR_INTERFACE_API* Stop)(NRPluginHandle handle);

    /// @brief Release the plugin.
    ///
    /// @param handle:
    NRPluginResult(NR_INTERFACE_API* Release)(NRPluginHandle handle);

    /// @brief Unregister lifecycle. Called just before "NRPluginUnload".
    /// note: Should release resources which allocates in Register lifecycle.
    ///
    /// @param handle:
    NRPluginResult(NR_INTERFACE_API* Unregister)(NRPluginHandle handle);

} NRPluginLifecycleProvider;
