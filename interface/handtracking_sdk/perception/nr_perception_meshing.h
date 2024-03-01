#pragma once

#include "nr_perception_common.h"
#include "nr_plugin_interface.h"
#include "nr_plugin_lifecycle.h"
#include "nr_plugin_types.h"

NR_PLUGIN_ENUM(MeshingFlags){
    MESHING_FLAGS_NULL = 0,
    MESHING_FLAGS_COMPUTE_NORMAL = 1 << 0,
};

NR_PLUGIN_ENUM(MeshingBlockState){
    MESHING_BLOCK_STATE_NEW,
    MESHING_BLOCK_STATE_UPDATED,
    MESHING_BLOCK_STATE_DELETED,
    MESHING_BLOCK_STATE_UNCHANGED,
};

#pragma pack(1)
typedef struct MeshingBlockData {
    union {
        struct {
            uint64_t block_id;
            MeshingBlockState state;
            /// a combination of NRMeshingFlags, which mesh block took place.
            uint64_t flags;
            /// the number of elements in face-vertex-index buffer.
            uint32_t index_count;
            /// the number of vertices in vertex/normal buffer.
            uint32_t vertex_count;
            /// the pointer to vertex buffer.
            NRVector3f *vertex;
            /// the pointer to normal buffer.
            NRVector3f *normal;
            /// the pointer to face-vertex-index buffer.
            /// In the buffer, each element is a index to vertex buffer.
            /// Three index elements will define one triangle.
            /// For example:
            /// the first triangle is: vertex[index[0]], vertex[index[1]],
            /// vertex[index[2]]. The second triangle is: vertex[index[3]],
            /// vertex[index[4]], vertex[index[5]]. All faces are listed back-to-back in
            /// counter-clockwise vertex order.
            uint16_t *index;
        };
        uint8_t padding[64];
    };
} MeshingBlockData;

typedef struct MeshingData {
    union {
        struct {
            /// The timestamp (in nano seconds) when data was generated.
            uint64_t hmd_time_nanos;
            uint32_t data_count;
            MeshingBlockData *data;
        };
        uint8_t padding[64];
    };
} MeshingData;
#pragma pack()

typedef struct MeshingProvider {
    NRPluginResult(NR_INTERFACE_API *SetFlags)(NRPluginHandle handle, uint32_t flags);
    NRPluginResult(NR_INTERFACE_API *GetFlags)(NRPluginHandle handle, uint32_t *out_flags);

    NRPluginResult(NR_INTERFACE_API *SetRadius)(NRPluginHandle handle, float radius);
    NRPluginResult(NR_INTERFACE_API *GetRadius)(NRPluginHandle handle, float *out_radius);

    NRPluginResult(NR_INTERFACE_API *SetSubmitRate)(NRPluginHandle handle, float rate);
    NRPluginResult(NR_INTERFACE_API *GetSubmitRate)(NRPluginHandle handle, float *out_rate);
    void(NR_INTERFACE_API *NotifyData)(NRPluginHandle handle, NRChannelDataType channel_data_type, const void *data,
                                       uint32_t data_size);
} MeshingProvider;

NR_DECLARE_INTERFACE(MeshingInterface) {
    NRPluginResult(NR_INTERFACE_API * RegisterLifecycleProvider)(
        NRPluginHandle handle, const char *plugin_id, const char *plugin_version,
        const NRPluginLifecycleProvider *provider, uint32_t provider_size);
    NRPluginResult(NR_INTERFACE_API * RegisterProvider)(NRPluginHandle handle, const MeshingProvider *provider,
                                                        uint32_t provider_size);
    NRPluginResult(NR_INTERFACE_API * SubmitMeshData)(NRPluginHandle handle, const MeshingData *out_data);
    NRPluginResult(NR_INTERFACE_API * GetDevicePose)(NRPluginHandle handle, DevicePose * pose, uint64_t hmd_time_nanos);
};

NR_REGISTER_INTERFACE_GUID(0xBFD6EEF15AD34A77ULL, 0xB0129D1AA1A39AA2ULL, MeshingInterface)
