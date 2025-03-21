#pragma once

#include "nr_plugin_lifecycle.h"
NR_DECLARE_INTERFACE(NRGlassesConfigInterface) {
    NRPluginResult(NR_INTERFACE_API *RegisterLifecycleProvider)(
        const char * plugin_id,
        const char * plugin_version,
        const NRPluginLifecycleProvider * provider,
        uint32_t provider_size
    );
    NRPluginResult(NR_INTERFACE_API *SetDeviceConfig)(
        NRPluginHandle handle,
        const char * data,
        uint32_t data_size
    );
    NRPluginResult(NR_INTERFACE_API *ResetOv580)(
        NRPluginHandle handle
    );
};

NR_REGISTER_INTERFACE_GUID(0x6BE0F13F7B0E4AAEULL, 0x95B1E0E98D2FA1F6ULL,
                            NRGlassesConfigInterface)

