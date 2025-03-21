#pragma once

#include "nr_plugin_interface.h"
#include "nr_plugin_types.h"
typedef struct NRGlassesExtProvider {

    NRPluginResult(NR_INTERFACE_API *StartAllServer)(
        NRPluginHandle handle
    );

    NRPluginResult(NR_INTERFACE_API *StopAllServer)(
        NRPluginHandle handle
    );
} NRGlassesExtProvider;

NR_DECLARE_INTERFACE(NRGlassesExtInterface) {

    NRPluginResult(NR_INTERFACE_API *RegisterProvider)(
        NRPluginHandle handle,
        const NRGlassesExtProvider * provider,
        uint32_t provider_size
    );
};

NR_REGISTER_INTERFACE_GUID(0xE5FA6D0D9ED4482BULL, 0xBE08ED5ABAC4020BULL,
                            NRGlassesExtInterface)

