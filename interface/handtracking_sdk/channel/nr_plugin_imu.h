#pragma once

#include "nr_plugin_lifecycle.h"
#include "nr_plugin_imu_types.h"
NR_DECLARE_INTERFACE(NRImuInterface) {
    NRPluginResult(NR_INTERFACE_API *RegisterLifecycleProvider)(
        const char * plugin_id,
        const char * plugin_version,
        const NRPluginLifecycleProvider * provider,
        uint32_t provider_size
    );
    NRPluginResult(NR_INTERFACE_API *NotifyData)(
        NRPluginHandle handle,
        NRChannelDataType channel_data_type,
        const NRImuData * data,
        uint32_t data_size
    );
};

NR_REGISTER_INTERFACE_GUID(0x7720AB84C3C54B40ULL, 0x8709410F78AE1615ULL,
                            NRImuInterface)

