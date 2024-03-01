#pragma once

#ifdef NRSDK

#include "nr_plugin_types.inc"

#else
typedef unsigned long long NRPluginHandle;

typedef unsigned long long NRHandle;

typedef unsigned long long NRDefinitionHandle;


#pragma pack(1)
typedef struct NRVersion {
    int32_t major;
    int32_t minor;
    int32_t revision;

} NRVersion;

typedef struct NRSize2i {
    uint32_t width;
    uint32_t height;

} NRSize2i;

typedef struct NRRectf {
    float bottom;
    float left;
    float top;
    float right;

} NRRectf;

typedef struct NRFov4f {
    float left_tan;
    float right_tan;
    float top_tan;
    float bottom_tan;

} NRFov4f;

typedef struct NRVector2f {
    float x;
    float y;

} NRVector2f;

typedef struct NRVector3f {
    float x;
    float y;
    float z;

} NRVector3f;

typedef struct NRVector4f {
    float x;
    float y;
    float z;
    float w;

} NRVector4f;

typedef struct NRQuatf {
    float qx;
    float qy;
    float qz;
    float qw;

} NRQuatf;

typedef struct NRTransform {
    NRVector3f position;
    NRQuatf rotation;

} NRTransform;

typedef struct NRMat3f {
    NRVector3f column0;
    NRVector3f column1;
    NRVector3f column2;

} NRMat3f;

typedef struct NRMat4f {
    NRVector4f column0;
    NRVector4f column1;
    NRVector4f column2;
    NRVector4f column3;

} NRMat4f;

typedef struct NRExtents {
    NRTransform transform;
    NRVector3f extents;

} NRExtents;

#pragma pack()

#endif // NRSDK