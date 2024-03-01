#pragma once
#include "nr_perception_common.h"
#include "public/nr_plugin_interface.h"
#include "public/nr_plugin_lifecycle.h"
#include "public/nr_plugin_types.h"
// clang-format off

/// @brief The type of gesture recognized
NR_PLUGIN_ENUM(GestureType){
    GESTURE_TYPE_UNKNOWN    = -1,
    GESTURE_TYPE_OPEN_HAND  = 0,
    GESTURE_TYPE_GRAB       = 1,
    GESTURE_TYPE_PINCH      = 2,
    GESTURE_TYPE_POINT      = 3,
    GESTURE_TYPE_VICTORY    = 4,
    GESTURE_TYPE_CALL       = 5,
    GESTURE_TYPE_SYSTEM     = 6,
    GESTURE_TYPE_THUMBS_UP = 7,
};

/// @brief The mask of GestureType
NR_PLUGIN_ENUM64(GestureTypeMask){
    GESTURE_TYPE_MASK_OPEN_HAND  = (1LL << GestureType::GESTURE_TYPE_OPEN_HAND),
    GESTURE_TYPE_MASK_GRAB       = (1LL << GestureType::GESTURE_TYPE_GRAB),
    GESTURE_TYPE_MASK_PINCH      = (1LL << GestureType::GESTURE_TYPE_PINCH),
    GESTURE_TYPE_MASK_POINT      = (1LL << GestureType::GESTURE_TYPE_POINT),
    GESTURE_TYPE_MASK_VICTORY    = (1LL << GestureType::GESTURE_TYPE_VICTORY),
    GESTURE_TYPE_MASK_CALL       = (1LL << GestureType::GESTURE_TYPE_CALL),
    GESTURE_TYPE_MASK_SYSTEM     = (1LL << GestureType::GESTURE_TYPE_SYSTEM),
    GESTURE_TYPE_MASK_THUMBS_UP  = (1LL << GestureType::GESTURE_TYPE_THUMBS_UP),
    GESTURE_TYPE_MASK_ALL        = 0x7FFFFFFFFFFFFFFFLL,
};

/// @brief The skeleton id of a hand
NR_PLUGIN_ENUM(HandJointType){
    HAND_JOINT_TYPE_INVALID          = -1,
    HAND_JOINT_TYPE_THUMB_0          = 0,
    HAND_JOINT_TYPE_THUMB_1          = 1,
    HAND_JOINT_TYPE_THUMB_2          = 2,
    HAND_JOINT_TYPE_THUMB_3          = 3,
    HAND_JOINT_TYPE_INDEX_1          = 4,
    HAND_JOINT_TYPE_INDEX_2          = 5,
    HAND_JOINT_TYPE_INDEX_3          = 6,
    HAND_JOINT_TYPE_INDEX_4          = 7,
    HAND_JOINT_TYPE_MIDDLE_1         = 8,
    HAND_JOINT_TYPE_MIDDLE_2         = 9,
    HAND_JOINT_TYPE_MIDDLE_3         = 10,
    HAND_JOINT_TYPE_MIDDLE_4         = 11,
    HAND_JOINT_TYPE_RING_1           = 12,
    HAND_JOINT_TYPE_RING_2           = 13,
    HAND_JOINT_TYPE_RING_3           = 14,
    HAND_JOINT_TYPE_RING_4           = 15,
    HAND_JOINT_TYPE_PINKY_0          = 16,
    HAND_JOINT_TYPE_PINKY_1          = 17,
    HAND_JOINT_TYPE_PINKY_2          = 18,
    HAND_JOINT_TYPE_PINKY_3          = 19,
    HAND_JOINT_TYPE_PINKY_4          = 20,
    HAND_JOINT_TYPE_PALM_CENTER      = 21,
    HAND_JOINT_TYPE_WRIST_BEGIN      = 22,
    HAND_JOINT_TYPE_WRIST_END        = 23,
    HAND_JOINT_TYPE_WRIST_CENTER     = 24,
    HAND_JOINT_TYPE_MAX              = 32,
};

NR_PLUGIN_ENUM64(HandJointMask){
    HAND_JOINT_TYPE_MASK_THUMB_0             = (1LL << HandJointType::HAND_JOINT_TYPE_THUMB_0),
    HAND_JOINT_TYPE_MASK_THUMB_1             = (1LL << HandJointType::HAND_JOINT_TYPE_THUMB_1),
    HAND_JOINT_TYPE_MASK_THUMB_2             = (1LL << HandJointType::HAND_JOINT_TYPE_THUMB_2),
    HAND_JOINT_TYPE_MASK_THUMB_3             = (1LL << HandJointType::HAND_JOINT_TYPE_THUMB_3),
    HAND_JOINT_TYPE_MASK_INDEX_1             = (1LL << HandJointType::HAND_JOINT_TYPE_INDEX_1),
    HAND_JOINT_TYPE_MASK_INDEX_2             = (1LL << HandJointType::HAND_JOINT_TYPE_INDEX_2),
    HAND_JOINT_TYPE_MASK_INDEX_3             = (1LL << HandJointType::HAND_JOINT_TYPE_INDEX_3),
    HAND_JOINT_TYPE_MASK_INDEX_4             = (1LL << HandJointType::HAND_JOINT_TYPE_INDEX_4),
    HAND_JOINT_TYPE_MASK_MIDDLE_1            = (1LL << HandJointType::HAND_JOINT_TYPE_MIDDLE_1),
    HAND_JOINT_TYPE_MASK_MIDDLE_2            = (1LL << HandJointType::HAND_JOINT_TYPE_MIDDLE_2),
    HAND_JOINT_TYPE_MASK_MIDDLE_3            = (1LL << HandJointType::HAND_JOINT_TYPE_MIDDLE_3),
    HAND_JOINT_TYPE_MASK_MIDDLE_4            = (1LL << HandJointType::HAND_JOINT_TYPE_MIDDLE_4),
    HAND_JOINT_TYPE_MASK_RING_1              = (1LL << HandJointType::HAND_JOINT_TYPE_RING_1),
    HAND_JOINT_TYPE_MASK_RING_2              = (1LL << HandJointType::HAND_JOINT_TYPE_RING_2),
    HAND_JOINT_TYPE_MASK_RING_3              = (1LL << HandJointType::HAND_JOINT_TYPE_RING_3),
    HAND_JOINT_TYPE_MASK_RING_4              = (1LL << HandJointType::HAND_JOINT_TYPE_RING_4),
    HAND_JOINT_TYPE_MASK_PINKY_0             = (1LL << HandJointType::HAND_JOINT_TYPE_PINKY_0),
    HAND_JOINT_TYPE_MASK_PINKY_1             = (1LL << HandJointType::HAND_JOINT_TYPE_PINKY_1),
    HAND_JOINT_TYPE_MASK_PINKY_2             = (1LL << HandJointType::HAND_JOINT_TYPE_PINKY_2),
    HAND_JOINT_TYPE_MASK_PINKY_3             = (1LL << HandJointType::HAND_JOINT_TYPE_PINKY_3),
    HAND_JOINT_TYPE_MASK_PINKY_4             = (1LL << HandJointType::HAND_JOINT_TYPE_PINKY_4),
    HAND_JOINT_TYPE_MASK_PALM_CENTER         = (1LL << HandJointType::HAND_JOINT_TYPE_PALM_CENTER),
    HAND_JOINT_TYPE_MASK_WRIST_BEGIN         = (1LL << HandJointType::HAND_JOINT_TYPE_WRIST_BEGIN),
    HAND_JOINT_TYPE_MASK_WRIST_END           = (1LL << HandJointType::HAND_JOINT_TYPE_WRIST_END),
    HAND_JOINT_TYPE_MASK_WRIST_CENTER        = (1LL << HandJointType::HAND_JOINT_TYPE_WRIST_CENTER),
    HAND_JOINT_TYPE_MASK_ALL                 = 0x7FFFFFFFFFFFFFFFLL,
};

NR_PLUGIN_ENUM(HandType){
    HAND_TYPE_UNKNOWN    = -1,
    HAND_TYPE_LEFT       = 0,
    HAND_TYPE_RIGHT      = 1,
};

NR_PLUGIN_ENUM(HandTrackingSupportFunction){
    HAND_TRACKING_SUPPORT_TIMESTAMP             = 0,
    HAND_TRACKING_SUPPORT_HAND_JOINT_POSITION   = 1,
    HAND_TRACKING_SUPPORT_HAND_JOINT_ROTATION   = 2,
};
NR_PLUGIN_ENUM64(HandTrackingSupportFunctionMask){
    HAND_TRACKING_SUPPORT_MASK_TIMESTAMP            = (1LL << HandTrackingSupportFunction::HAND_TRACKING_SUPPORT_TIMESTAMP),
    HAND_TRACKING_SUPPORT_MASK_HAND_JOINT_POSITION  = (1LL << HandTrackingSupportFunction::HAND_TRACKING_SUPPORT_HAND_JOINT_POSITION),
    HAND_TRACKING_SUPPORT_MASK_HAND_JOINT_ROTATION  = (1LL << HandTrackingSupportFunction::HAND_TRACKING_SUPPORT_HAND_JOINT_ROTATION),
};
// clang-format on

#pragma pack(1)
typedef struct HandJointData {
    union {
        struct {
            int32_t version;
            HandJointType hand_joint_type;
            NRTransform hand_joint_pose;
        };
        uint8_t padding[100];
    };
} HandJointData;

typedef struct HandData {
    union {
        struct {
            int32_t version;
            bool is_tracked;
            float confidence;
            HandType hand_type;
            GestureType gesture_type;
            uint32_t hand_joint_count;
            HandJointData hand_joint_data[32];
            uint64_t image_timestamp_nanos;
        };
        uint8_t padding[3300];
    };
} HandData;
#pragma pack()

typedef struct HandTrackingProvider {
    /// @brief Get available getsture types mask
    ///
    /// @param out_available_gesture_type_mask: The mask of getsture type
    ///        relative to NRGestureTypeMask
    NRPluginResult(NR_INTERFACE_API *GetAvailableGestureType)(NRPluginHandle handle,
                                                              uint64_t *out_available_gesture_type_mask);
    /// @brief Get available hand joint
    ///
    /// @param out_available_hand_joint_mask: The mask of hand joint
    ///        relative to NRHandJointTypeMask
    NRPluginResult(NR_INTERFACE_API *GetAvailableHandJoint)(NRPluginHandle handle,
                                                            uint64_t *out_available_hand_joint_mask);
    /// @brief Get supported functions
    ///
    /// @param out_supported_function_mask: The mask of supported functions
    ///        relative to NRHandSupportFunctionMask
    NRPluginResult(NR_INTERFACE_API *GetSupportedFunctions)(NRPluginHandle handle,
                                                            uint64_t *out_supported_function_mask);
    /// @brief Get hand data by HMD timestamp
    ///
    /// @param hmd_time_nanos: nano time to aquire hand data
    /// @param out_hand_array: The hand data array
    /// @param out_hand_num: hand count in out_hand_array
    ///
    // hand poses corresponds to world(imu) perception system
    NRPluginResult(NR_INTERFACE_API *GetHandData)(NRPluginHandle handle, uint64_t hmd_time_nanos,
                                                  HandData *out_hand_array, uint32_t *out_hand_num);

    void(NR_INTERFACE_API *NotifyData)(NRPluginHandle handle, NRChannelDataType channel_data_type, const void *data,
                                       uint32_t data_size);
} HandTrackingProvider;

NR_DECLARE_INTERFACE(HandTrackingInterface) {
    NRPluginResult(NR_INTERFACE_API * RegisterLifecycleProvider)(
        NRPluginHandle handle, const char *plugin_id, const char *plugin_version,
        const NRPluginLifecycleProvider *provider, uint32_t provider_size);
    /// @brief Register hand Provider to sdk for custom calls
    ///
    /// @param handle: The hand of hand plugin,created by sdk when lifecycle create called
    /// @param provider: The custom provider provided by plugin
    /// @param provider_size: The size of provider
    NRPluginResult(NR_INTERFACE_API * RegisterProvider)(NRPluginHandle handle, const HandTrackingProvider *provider,
                                                        uint32_t provider_size);
    NRPluginResult(NR_INTERFACE_API * GetDevicePose)(NRPluginHandle handle, DevicePose * device_pose,
                                                     uint64_t hmd_time_nanos);
};

NR_REGISTER_INTERFACE_GUID(0xAF1BDCC757114F18ULL, 0xA9BDBA967E7A4E5EULL, HandTrackingInterface)
