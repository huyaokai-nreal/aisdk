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


/**
 * @struct ArtosynTensorDims
 * @brief 描述NPU张量的多维参数和内存配置信息
 * 
 * 该结构体用于定义NPU加速器中的张量属性，包含从内存布局到量化参数的全方位配置。
 * 主要用于模型加载、输入输出张量配置等底层硬件交互场景。
 */
struct ArtosynTensorDims {
    // AR_NPU_IMG_CFG_S stImgConfig; 

    // 基础标识信息
    std::string achName;          // 张量名称标识，如"input0"/"conv1_weight"等
    std::string achType;          // 数据类型，取值范围：["float", "int16", "int8", "uint8"]
    std::string achStepType;      // 内存步长类型，"normal"-常规布局, "continue"-连续内存
    std::string achLayoutType;    // 张量布局类型，如"NHWC"、"NCHW"等硬件优化布局
    std::string achMemoryType;    // 存储介质类型，"ddr"/"sram"等，标识物理存储位置
	std::string achDdrFormat;     // DDR内存数据格式，如"RGB_Planar"等硬件特定格式

    // 量化参数
    double dScaleFactor;          // 量化缩放因子，用于INT8/UINT8类型：fp_value = int_value * scale + zero_point
    int32_t s32ZeroPoint;         // 量化零点偏移，通常为INT8/UINT8的零点补偿值

    // 内存配置参数
    uint32_t u32ID;               // 张量唯一标识符，用于硬件资源管理
    uint32_t u32Bank;             // 存储体编号，指定NPU内存bank分配（0-7）
    uint32_t u32Offset;           // 内存起始偏移量（字节），相对于bank基地址
    uint32_t u32RowStep;          // 行步长（字节），包含内存对齐填充（例如width=120时可能对齐到128）
    uint32_t u32TensorStep;       // 张量步长（字节），用于批处理时跨batch的内存间隔

    // 空间维度参数
    uint32_t u32Height;           // 空间维度高度（例如特征图高度）
    uint32_t u32Width;            // 空间维度宽度（例如特征图宽度）
    uint32_t u32OriChannels;      // 原始通道数（未分块前的通道维度）

    // 通道分块参数
    uint32_t u32KStep;            // 通道分块步长（K维度分组处理粒度）
    uint32_t u32KNormNum;         // 归一化通道组数（用于分组卷积优化）
    uint32_t u32KSizeLast;        // 最后一个通道块的大小（当总通道数非整数倍时）
    uint32_t u32KSizeNorm;        // 常规通道块的标准大小

    // 精度与位宽
    uint32_t u32BitWidth;         // 单元素位宽（bits），如float32=32, int8=8
    uint32_t u32Precision;        // 数据精度模式，32=FP32, 16=FP16, 8=INT8等

    // 容量参数
    uint32_t u32Size;             // 张量逻辑元素总数（height * width * channels等）
    uint32_t u32MemorySize;       // 实际内存占用字节数（包含所有对齐填充）
    
    // 辅助参数
    uint32_t u32Num;              // 批处理维度大小（batch size）
    uint32_t u32OriFrameSize;     // 原始单帧数据大小（不含batch维度的元素数）
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

struct SYM_EXPORT PlatformEnv {
    bool is_system_app = false;
    bool is_untrusted_app = false;
    const char* app_lib_path = nullptr;
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