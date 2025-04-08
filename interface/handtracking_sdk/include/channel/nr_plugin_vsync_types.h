#pragma once

#include "nr_plugin_types.h"

#ifdef NRSDK

#include "nr_plugin_vsync_types.inc"

#else

#pragma pack(1)
typedef struct NRVsyncData {
    union {
        struct {
            uint64_t hmd_time_nanos_system;
            uint64_t hmd_time_nanos_device;
            uint64_t sequence;
        };
        uint8_t padding[32];
    };

} NRVsyncData;

#pragma pack()

#endif // NRSDK