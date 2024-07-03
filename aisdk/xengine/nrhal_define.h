#ifndef _NRHAL_DEFINE_H_
#define _NRHAL_DEFINE_H_

// #include "mem_buffer.h"

#include <map>
#include <string>
#include <vector>
#include <absl/status/status.h>

#define SYM_EXPORT __attribute__((visibility("default")))

// #pragma GCC visibility push(default)
// #pragma GCC visibility pop

namespace aisdk::xengine {

// 此定义后续将废弃，目前暂时保留不再新维护
enum class Status {
    UNKNOWN = 0,
    SUCCESS = 1,
    FAILURE = 2,
    PLATFORM_NO_SUPPORT = 50,
    MODEL_LOAD_FAILURE = 100,
    MODEL_INIT_FAILURE = 101,
    SESSION_INIT_FAILURE = 102,
    FORWORD_FAILURE = 103,
};

enum class VendorType {
    UNKNOWN = 0,
    SNPE = 1,
    QNN = 2,
    MNN = 3,
    ROCKCHIP = 4,
    ARTOSYN = 5,
    XREAL = 6,
    TENSORRT = 7,
};

enum class RuntimeType {
    UNKNOWN = 0,
    CPU = 1,
    GPU = 2,
    DSP = 3,
    AIP = 4,
    NPU = 5,
};

enum class ElementType : int32_t {
    UNKNOWN = 0,
    FLOAT32 = 1,
    FLOAT16 = 2,
    UINT8 = 3,
    INT8 = 4,
    UINT16 = 5,
    INT16 = 6,
    TF8 = 7,
    TF16 = 8,
};

enum class PrecisionMode {
    UNKNOWN = 0,
    FLOAT32 = 1,
    FLOAT16 = 2,
    INT8 = 3,
    INT16 = 4,
};

enum class TensorFormat : int32_t {
    UNKNOWN = 0,
    NCHW = 1,
    NHWC = 2,
    CHW = 3,
    HWC = 4,
    NHW = 5,
    HW = 6,
    NW = 7,
    W = 8,
    NCDHW = 9,
    NDHWC = 10,
    CDHW = 11,
    DHWC = 12,
    BlockingNHWC = 13,
};

enum ImageFormat {
    UNKNOWN = 0,
    RGB = 1,
    BGR = 2,
    GRAY = 3,
};

enum class ImageCategory {
    IS_TENSOR = 0,
    IS_BLOB = 1,
    IS_CVMAT = 2,
};

struct ArtosynTensorDims {
    uint32_t u32ID;
    uint32_t u32Bank;
    uint32_t u32Offset;
    uint32_t u32Width;
    uint32_t u32Height;
    uint32_t u32KStep;
    uint32_t u32KNormNum;
    uint32_t u32KSizeLast;
    uint32_t u32KSizeNorm;
    uint32_t u32OriChannels;
    uint32_t u32RowStep;
    uint32_t u32TensorStep; //used for batch mode
};

struct Tensor {
    // tensor的名称
    std::string m_name;
    // tensor的秩
    uint32_t m_rank = 0;
    // tensor的dims
    // m_dims,不含Batch和Multi-In/Out维度
    // m_dims,从0到N-1,分别代表高维到低维
    // 举例: [c,h,w]=[1,128,123]
    std::vector<uint32_t> m_dims;
    // artosyn的内存块排布比较独特，不走m_dims，走以下配置
    ArtosynTensorDims m_artosyn_dims;
    // tensor的layout，这是个经验值，仅参考
    TensorFormat m_dimtype = TensorFormat::UNKNOWN;
    // 每个元素的数据类型
    ElementType m_elementype = ElementType::UNKNOWN;
    // 每个元素的字节大小
    uint32_t m_elementbyte = 0;
    // 全部元素的数量
    uint32_t m_elementsize = 0;
    // 保留
    uint64_t m_phyaddr = 0;
    // 对应模型的输入或者输出内存地址
    // 注意: 如果m_packed_bybatch=true, 这个是起始地址，用户读写数据请注意偏移
    // 偏移字节 mem = (char*)m_viraddr + batch_n * m_elementsize * m_elementbyte;
    void *m_viraddr = nullptr;
};

// 描述模型整个输入或输出的tensor信息
struct IoTensors {
    // 实际创建的batch值，可用户期望变更，应该是m_ori_batch的整数倍
    uint32_t m_batch = 0;
    // 模型本身的batch值
    uint32_t m_ori_batch = 0;
    // 模型多输入或多输出的tensor数，应该和m_tensors.size()是一致的
    uint32_t m_multishape_num = 0;
    // 提示batch的tensor的内存是否packed
    // 目前我们遇到的基本都是packed的，也就是说单个teneor的batch是连续的
    bool m_packed_bybatch = false;
    // tensor细节
    std::vector<Tensor> m_tensors;
};

struct ImageBlob {
    std::string m_name;
    ImageFormat m_format = ImageFormat::UNKNOWN;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_wstride = 0;
    ElementType m_elementype = ElementType::UNKNOWN;
    uint32_t m_elementbyte = 0;
    uint32_t m_elementsize = 0;
    uint64_t m_phyaddr[4] = {0};
    void *m_viraddr[4] = {nullptr};
};

struct IoImageBlobs {
    uint32_t m_batch = 0;
    uint32_t m_ori_batch = 0;
    uint32_t m_multiinput_num = 0;
    bool m_packed_bybatch = false;
    std::vector<ImageBlob> m_imageblobs;
};

struct Rect {
    float x = 0.f;
    float y = 0.f;
    float w = 0.f;
    float h = 0.f;
};

struct SYM_EXPORT PlatformStatus {
    bool is_snpe_support = false;
    bool is_snapdragon_855 = false;
    bool is_snapdragon_8Gen1 = false;
    bool is_hexagon_dsp = false;
    bool is_hexagon_signedPD_dsp = false;
    bool is_hexagon_unsignedPD_dsp = false;
    bool is_dot_support = false;
    bool is_fp16_support = false;
    uint32_t gpu_device_count = 0;
    std::vector<std::string> gpu_device_name;
    bool is_artosyn_support = false;
    bool is_mobile_eva = false;
    bool is_mobile_evapro = false;
};

}  // namespace aisdk::xengine

#endif