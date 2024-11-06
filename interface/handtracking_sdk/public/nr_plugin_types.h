#pragma once
#include <stdint.h>

// Use int32_t for ABI compatibility.
#if __cplusplus >= 201100 || defined(_WIN32)
#define NR_PLUGIN_ENUM8(__TYPE__) enum __TYPE__ : int8_t
#define NR_PLUGIN_ENUM16(__TYPE__) enum __TYPE__ : int16_t
#define NR_PLUGIN_ENUM32(__TYPE__) enum __TYPE__ : int32_t
#define NR_PLUGIN_ENUM64(__TYPE__) enum __TYPE__ : int64_t
#define NR_PLUGIN_ENUM(__TYPE__) NR_PLUGIN_ENUM32(__TYPE__)
#else
#error "C++ versions less than C++11 are not supported."
#endif

#if defined(NRSDK) || defined(NRPLUGIN)

#include "nr_plugin_result.inl"
#include "nr_plugin_types.inl"
#include "nr_plugin_types_ext.inl"

#else

NR_PLUGIN_ENUM(NRPluginResult){
    NR_PLUGIN_RESULT_SUCCESS = 0,
    NR_PLUGIN_RESULT_FAILURE = 1,
	NR_PLUGIN_RESULT_INVALID_ARGUMENT = 2,
	NR_PLUGIN_RESULT_NOT_ENOUGH_MEMORY = 3,
	NR_PLUGIN_RESULT_UNSUPPORTED = 4,

    // In addition to the error code above, developers can use their own error
    // code. User defined error code start from
    // NR_PLUGIN_RESULT_SELF_DEFINED_START.
    NR_PLUGIN_RESULT_SELF_DEFINED_START = 10000,
};

typedef unsigned long long NRPluginHandle;
typedef unsigned long long NRHandle;
typedef unsigned long long NRDefinitionHandle;

/// @brief The version information of NRSDK
typedef struct NRVersion {
    int32_t major;
    int32_t minor;
    int32_t revision;
} NRVersion;

/// @brief A int32_t size vector
typedef struct NRSize2i {
    uint32_t width;
    uint32_t height;
} NRSize2i;

/// @brief a floating point Rectangle
typedef struct NRRectf {
    float bottom, left, top, right;
} NRRectf;

/// @brief A floating point Field of View
typedef struct NRFov4f {
    float left_tan;
    float right_tan;
    float top_tan;
    float bottom_tan;
} NRFov4f;

/// @brief A floating point 2D vector
typedef struct NRVector2f {
    float x, y;
} NRVector2f;

/// @brief A floating point 3D vector
typedef struct NRVector3f {
    float x, y, z;
} NRVector3f;

/// @brief A floating point 4D vector
typedef struct NRVector4f {
    float x, y, z, w;
} NRVector4f;

/// @brief A floating point quaternion with hamilton format
typedef struct NRQuatf {
    float qx, qy, qz, qw;
} NRQuatf;

/// @brief A transform struct in right-hand coordinate system
typedef struct NRTransform {
    NRVector3f position;
    NRQuatf rotation;
} NRTransform;

/// @brief A floating point 4x4 matrix in right-hand coordinate system
typedef struct NRMat3f {
    NRVector3f column0;
    NRVector3f column1;
    NRVector3f column2;
} NRMat3f;

/// @brief A floating point 4x4 matrix in right-hand coordinate system
typedef struct NRMat4f {
    NRVector4f column0;
    NRVector4f column1;
    NRVector4f column2;
    NRVector4f column3;
} NRMat4f;

/// @brief Axis aligned bounding box
typedef struct NRExtents {
    /// The center of the bounding box
    NRTransform transform;
    /// The size of the bounding box
    NRVector3f extents;
} NRExtents;

#endif  // NRSDK
