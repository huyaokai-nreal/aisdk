#pragma once

#include "public/nr_plugin_interface.h"
#include "public/nr_plugin_types.h"
NR_DECLARE_INTERFACE(NRGenericInterface) {
    NRPluginResult(NR_INTERFACE_API *GetSDKVersion)(
        NRVersion * version
    );
    NRPluginResult(NR_INTERFACE_API *GetActivityInfo)(
        void ** java_vm,
        void ** context,
        void ** class_loader
    );
    NRPluginResult(NR_INTERFACE_API *GetPluginConfig)(
        const char ** data,
        uint32_t * data_size
    );
    NRPluginResult(NR_INTERFACE_API *GetDeviceConfig)(
        NRPluginHandle handle,
        const char ** data,
        uint32_t * data_size
    );
    NRPluginResult(NR_INTERFACE_API *GetDeviceMiscConfig)(
        NRPluginHandle handle,
        const char ** data,
        uint32_t * data_size
    );
    NRPluginResult(NR_INTERFACE_API *GetGlobalConfig)(
        NRPluginHandle handle,
        const char ** data,
        uint32_t * data_size
    );
    NRPluginResult(NR_INTERFACE_API *GetNetworkType)(
        NRPluginHandle handle,
        NRNetworkType * network_type
    );
    NRPluginResult(NR_INTERFACE_API *GetDeviceType)(
        NRPluginHandle handle,
        NRDeviceType * device_type
    );
    NRPluginResult(NR_INTERFACE_API *GetDynamicLibraryPath)(
        NRPluginHandle handle,
        const char ** out_path,
        uint32_t * path_size
    );
    NRPluginResult(NR_INTERFACE_API *GetChannelIdentifier)(
        NRPluginHandle handle,
        const char ** channel_identifier,
        uint32_t * channel_identifier_size
    );
    NRPluginResult(NR_INTERFACE_API *SetThreadPriority)(
        NRPluginHandle handle,
        int32_t tid,
        int32_t policy,
        int32_t priority
    );
    NRPluginResult(NR_INTERFACE_API *GetPropertyFlag)(
        NRPluginHandle handle,
        NRProperty property,
        int32_t * flag
    );
    NRPluginResult(NR_INTERFACE_API *UpdateMetricsui64)(
        NRPluginHandle handle,
        NRMetricsType metrics_type,
        uint64_t metrics_data
    );
    NRPluginResult(NR_INTERFACE_API *UpdateMetricsui)(
        NRPluginHandle handle,
        NRMetricsType metrics_type,
        uint32_t metrics_data
    );
    NRPluginResult(NR_INTERFACE_API *GetPluginDebugConfig)(
        const char ** data,
        uint32_t * data_size
    );
};

NR_REGISTER_INTERFACE_GUID(0xEA9C4E1B179EF26CULL, 0x99E82B632499DC03ULL,
                            NRGenericInterface)

