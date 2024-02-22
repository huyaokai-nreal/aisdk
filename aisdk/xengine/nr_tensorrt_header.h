#ifndef _TRT_EXEC_H_
#define _TRT_EXEC_H_

#include <cstddef>
#include <cstdint>
extern "C" {

#define NVIDIA_GEFORCE_GTX_1650 "NVIDIA GeForce GTX 1650"
#define NVIDIA_GEFORCE_GTX_3060 "NVIDIA GeForce RTX 3060 Laptop GPU"

struct WrapCudaDevice {
    int device_count = 0;
    char* device_name[16] = {nullptr}; // NVIDIA_GEFORCE_GTX_1650
};

// 目前trtexec内部转换 dimtype 和 elementype 参考下面这个：
// 有更新需要上下文同时变更

// enum class Xengine::ElementType : int32_t {
//     UNKNOWN = 0,
//     FLOAT32 = 1,
//     FLOAT16 = 2,
//     UINT8 = 3,
//     INT8 = 4,
//     UINT16 = 5,
//     INT16 = 6,
//     TF8 = 7,
//     TF16 = 8,
// };

// enum class Xengine::TensorFormat : int32_t {
//     UNKNOWN = 0,
//     NCHW = 1,
//     NHWC = 2,
//     CHW = 3,
//     HWC = 4,
//     NHW = 5,
//     HW = 6,
//     NW = 7,
//     W = 8,
//     NCDHW = 9,
//     NDHWC = 10,
//     CDHW = 11,
//     DHWC = 12,
// };

struct WrapTrtTensor {
    int isInput = 0;
    const char* name = nullptr;
    uint32_t rank = 0;
    uint32_t* dims = nullptr;
    int32_t dimtype = 0;     // Xengine::TensorFormat 
    int32_t elementype = 0;  // Xengine::ElementType 
    void *host_viraddr = nullptr;
};

using _getcudadevice = int (*)(WrapCudaDevice *);
using _maketrtexec = void *(*)(int, char **);
using _loadmemmodel = int (*)(void *, char *, int);
using _querytrttensor = int (*)(void *, int, WrapTrtTensor *);
using _runinfer = int (*)(void *);
using _deltrtexec = int (*)(void *);

struct TrtExecApi {
    _getcudadevice g_getcudadevice = nullptr;
    _maketrtexec g_maketrtexec = nullptr;
    _loadmemmodel g_loadmemmodel = nullptr;
    _querytrttensor g_querytrttensor = nullptr;
    _runinfer g_runinfer = nullptr;
    _deltrtexec g_deltrtexec = nullptr;
};

}

#endif
