#pragma once

#include "nr_plugin_interface.h"
#include "nr_plugin_local_message_types.h"
typedef struct NRLocalMessageHandleProvider {

    NRPluginResult(NR_INTERFACE_API *OnLocalMessage)(
        NRPluginHandle handle,
        int32_t connid,
        const void * data,
        uint32_t size
    );

    NRPluginResult(NR_INTERFACE_API *OnConnectionChange)(
        NRPluginHandle handle,
        int32_t connid,
        NRConnectionState connection_state
    );
} NRLocalMessageHandleProvider;

NR_DECLARE_INTERFACE(NRLocalMessageSendInterface) {

    NRPluginResult(NR_INTERFACE_API *RegisterProvider)(
        NRPluginHandle handle,
        const NRLocalMessageHandleProvider * provider,
        uint32_t provider_size
    );

    NRPluginResult(NR_INTERFACE_API *SendLocalMessage)(
        NRPluginHandle handle,
        int32_t connid,
        const void * data,
        uint32_t size
    );

    NRPluginResult(NR_INTERFACE_API *BroadcastLocalMessage)(
        NRPluginHandle handle,
        const void * data,
        uint32_t size
    );
};

NR_REGISTER_INTERFACE_GUID(0x3F56FCD851564C84ULL, 0x65B827D0530384CFULL,
                            NRLocalMessageSendInterface)

