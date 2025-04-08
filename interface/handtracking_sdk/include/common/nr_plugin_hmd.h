#pragma once

#include "nr_plugin_interface.h"
#include "nr_plugin_types.h"
#include "nr_plugin_glasses_types.h"
typedef struct NRHMDProvider {

    NRPluginResult(NR_INTERFACE_API *OnComponentRefreshRateChange)(
        NRPluginHandle handle,
        NRComponent component,
        uint32_t refresh_rate
    );
} NRHMDProvider;

NR_DECLARE_INTERFACE(NRHMDInterface) {

    NRPluginResult(NR_INTERFACE_API *GetComponentFov)(
        NRPluginHandle handle,
        NRComponent component,
        NRFov4f * out_fov
    );

    NRPluginResult(NR_INTERFACE_API *GetComponentResolution)(
        NRPluginHandle handle,
        NRComponent component,
        NRSize2i * out_resolution
    );

    NRPluginResult(NR_INTERFACE_API *GetComponentRefreshRate)(
        NRPluginHandle handle,
        NRComponent component,
        uint32_t * out_refresh_rate
    );

    NRPluginResult(NR_INTERFACE_API *GetComponentIntrinsic)(
        NRPluginHandle handle,
        NRComponent component,
        NRMat3f * out_intrinsic
    );

    NRPluginResult(NR_INTERFACE_API *GetComponentDistortion)(
        NRPluginHandle handle,
        NRComponent component,
        NRCameraDistortion * out_camera_distortion
    );

    NRPluginResult(NR_INTERFACE_API *GetComponentExtrinsic)(
        NRPluginHandle handle,
        NRComponent base_component,
        NRComponent target_component,
        NRTransform * out_extrinsic
    );

    NRPluginResult(NR_INTERFACE_API *GetComponentPoseFromHead)(
        NRPluginHandle handle,
        NRComponent component,
        NRTransform * out_transform
    );

    NRPluginResult(NR_INTERFACE_API *GetComponentFovOverFill)(
        NRPluginHandle handle,
        NRComponent component,
        NRFov4f * out_fov_overfill
    );

    NRPluginResult(NR_INTERFACE_API *RegisterProvider)(
        NRPluginHandle handle,
        const NRHMDProvider * provider,
        uint32_t provider_size
    );

    NRPluginResult(NR_INTERFACE_API *GetDisplay2D3DMode)(
        NRPluginHandle handle,
        NRDisplay2D3DMode * display_2D3D_mode
    );

    NRPluginResult(NR_INTERFACE_API *SetComponentResolution)(
        NRPluginHandle handle,
        NRComponent component,
        const NRSize2i * resolution
    );

    NRPluginResult(NR_INTERFACE_API *SetComponentRefreshRate)(
        NRPluginHandle handle,
        NRComponent component,
        uint32_t refresh_rate
    );

    NRPluginResult(NR_INTERFACE_API *SetDisplay2D3DMode)(
        NRPluginHandle handle,
        NRDisplay2D3DMode display_2D3D_mode
    );

    NRPluginResult(NR_INTERFACE_API *GetComponentDisplayDistortionSize)(
        NRPluginHandle handle,
        NRComponent component,
        NRSize2i * distortion_size
    );

    NRPluginResult(NR_INTERFACE_API *GetComponentDisplayDistortionData)(
        NRPluginHandle handle,
        NRComponent component,
        int size,
        float * data
    );
};

NR_REGISTER_INTERFACE_GUID(0x9B0D3DD6AF045C77ULL, 0x20713D540D032D67ULL,
                            NRHMDInterface)

