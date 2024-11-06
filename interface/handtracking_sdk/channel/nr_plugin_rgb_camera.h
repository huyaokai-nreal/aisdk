#pragma once

#include "nr_plugin_lifecycle.h"
#include "nr_plugin_rgb_camera_types.h"
NR_DECLARE_INTERFACE(NRRgbCameraInterface) {
    NRPluginResult(NR_INTERFACE_API *RegisterLifecycleProvider)(
        const char * plugin_id,
        const char * plugin_version,
        const NRPluginLifecycleProvider * provider,
        uint32_t provider_size
    );
    NRPluginResult(NR_INTERFACE_API *NotifyData)(
        NRPluginHandle handle,
        const NRRgbCameraFrameData * data,
        uint32_t data_size
    );
};

NR_REGISTER_INTERFACE_GUID(0xB05918CB8EE342E9ULL, 0xBEC8D08AF475803DULL,
                            NRRgbCameraInterface)

