#pragma once

#include "nr_plugin_lifecycle.h"
#include "nr_plugin_grayscale_camera_types.h"
typedef struct NRGrayscaleCameraProvider {
} NRGrayscaleCameraProvider;

NR_DECLARE_INTERFACE(NRGrayscaleCameraInterface) {
    NRPluginResult(NR_INTERFACE_API *RegisterLifecycleProvider)(
        const char * plugin_id,
        const char * plugin_version,
        const NRPluginLifecycleProvider * provider,
        uint32_t provider_size
    );
    NRPluginResult(NR_INTERFACE_API *RegisterProvider)(
        NRPluginHandle handle,
        const NRGrayscaleCameraProvider * provider,
        uint32_t provider_size
    );
    NRPluginResult(NR_INTERFACE_API *NotifyData)(
        NRPluginHandle handle,
        NRChannelDataType channel_data_type,
        const NRGrayscaleCameraFrameData * data,
        uint32_t data_size
    );
};

NR_REGISTER_INTERFACE_GUID(0x83DFEDBCDD334DFBULL, 0xBCFE0139E80F8339ULL,
                            NRGrayscaleCameraInterface)

