#pragma once

#include "nr_plugin_lifecycle.h"
#include "nr_plugin_vsync_types.h"
NR_DECLARE_INTERFACE(NRVsyncInterface) {
    NRPluginResult(NR_INTERFACE_API *RegisterLifecycleProvider)(
        const char * plugin_id,
        const char * plugin_version,
        const NRPluginLifecycleProvider * provider,
        uint32_t provider_size
    );
    NRPluginResult(NR_INTERFACE_API *NotifyData)(
        NRPluginHandle handle,
        const NRVsyncData * data,
        uint32_t data_size
    );
};

NR_REGISTER_INTERFACE_GUID(0xCD37646C1A3945F5ULL, 0x90770CE4D968C8B1ULL,
                            NRVsyncInterface)

