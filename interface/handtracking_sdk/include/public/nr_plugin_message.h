#pragma once

#include "nr_plugin_interface.h"
#include "nr_plugin_types.h"

typedef struct DeviceMessageHandleProvider {
    NRPluginResult(NR_INTERFACE_API *OnDeviceMessage)(
        NRPluginHandle handle,
        const void * data,
        uint32_t size
    );
} NRDeviceMessageHandleProvider;

NR_DECLARE_INTERFACE(DeviceMessageSendInterface) {
    NRPluginResult(NR_INTERFACE_API *RegisterProvider)(
        NRPluginHandle handle,
        const DeviceMessageHandleProvider * provider,
        uint32_t provider_size
    );
    NRPluginResult(NR_INTERFACE_API *SendDeviceMessage)(
        NRPluginHandle handle,
        const void * data,
        uint32_t size
    );
    NRPluginResult(NR_INTERFACE_API *BroadcastDeviceMessage)(
        NRPluginHandle handle,
        const void * data,
        uint32_t size
    );
};

NR_REGISTER_INTERFACE_GUID(0x3f17f7f316184f89ULL, 0xa28d5e01cca9b01fULL,
                            DeviceMessageSendInterface)
