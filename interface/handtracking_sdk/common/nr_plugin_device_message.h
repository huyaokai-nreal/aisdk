#pragma once

#include "nr_plugin_interface.h"
#include "nr_plugin_types.h"
#include "nr_plugin_glasses_types.h"
typedef struct NRDeviceMessageHandleProvider {

    NRPluginResult(NR_INTERFACE_API *OnDeviceMessage)(
        NRPluginHandle handle,
        int32_t connid,
        const void * data,
        uint32_t size
    );
} NRDeviceMessageHandleProvider;

typedef struct NRDeviceMessageSendProvider {

    NRPluginResult(NR_INTERFACE_API *SendDeviceMessage)(
        NRPluginHandle handle,
        int32_t connid,
        const NRPluginMessage * plugin_message
    );

    NRPluginResult(NR_INTERFACE_API *BroadcastDeviceMessage)(
        NRPluginHandle handle,
        const NRPluginMessage * plugin_message
    );
} NRDeviceMessageSendProvider;

NR_DECLARE_INTERFACE(NRDeviceMessageSendInterface) {

    NRPluginResult(NR_INTERFACE_API *RegisterProvider)(
        NRPluginHandle handle,
        const NRDeviceMessageHandleProvider * provider,
        uint32_t provider_size
    );

    NRPluginResult(NR_INTERFACE_API *SendDeviceMessage)(
        NRPluginHandle handle,
        int32_t connid,
        const void * data,
        uint32_t size
    );

    NRPluginResult(NR_INTERFACE_API *BroadcastDeviceMessage)(
        NRPluginHandle handle,
        const void * data,
        uint32_t size
    );
};

NR_REGISTER_INTERFACE_GUID(0xD47A51A181154F1BULL, 0x86528F60295F6FCBULL,
                            NRDeviceMessageSendInterface)

NR_DECLARE_INTERFACE(NRDeviceMessageHandleInterface) {

    NRPluginResult(NR_INTERFACE_API *RegisterProvider)(
        NRPluginHandle handle,
        const NRDeviceMessageSendProvider * provider,
        uint32_t provider_size
    );

    NRPluginResult(NR_INTERFACE_API *HandleDeviceMessage)(
        NRPluginHandle handle,
        int32_t connid,
        const NRPluginMessage * plugin_message
    );
};

NR_REGISTER_INTERFACE_GUID(0x56FE806E4D5F4767ULL, 0xBEAD5AC5BC93E1F5ULL,
                            NRDeviceMessageHandleInterface)

