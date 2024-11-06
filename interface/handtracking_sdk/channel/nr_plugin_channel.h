#pragma once

#include "nr_plugin_interface.h"
#include "nr_plugin_types.h"
typedef struct NRChannelProvider {
    NRPluginResult(NR_INTERFACE_API *OpenDeviceChannel)(
        NRPluginHandle handle,
        int32_t * fd
    );
    NRPluginResult(NR_INTERFACE_API *CloseDeviceChannel)(
        NRPluginHandle handle
    );
} NRChannelProvider;

NR_DECLARE_INTERFACE(NRChannelInterface) {
    NRPluginResult(NR_INTERFACE_API *RegisterProvider)(
        NRPluginHandle handle,
        const NRChannelProvider * provider,
        uint32_t provider_size
    );
    NRPluginResult(NR_INTERFACE_API *GetChannel)(
        NRPluginHandle handle,
        NRChannelType channel_type,
        int32_t * channel_fd
    );
    NRPluginResult(NR_INTERFACE_API *ReleaseChannel)(
        NRPluginHandle handle,
        NRChannelType channel_type
    );
    NRPluginResult(NR_INTERFACE_API *SetDeviceMiscConfig)(
        NRPluginHandle handle,
        const char * data,
        uint32_t data_size
    );
};

NR_REGISTER_INTERFACE_GUID(0x73C74ED7610E4D34ULL, 0x9DFCA1DDD898F664ULL,
                            NRChannelInterface)

