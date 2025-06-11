#include "hand_rtmtiny_artosyn.h"

#include <absl/status/status.h>
#include <absl/status/statusor.h>

// #include <cstdint>
#include <numeric>

#include "aisdk/algorithm/common/math.h"
// #include "aisdk/algorithm/func/elementwise_mul.h"
// #include "aisdk/algorithm/func/permute.h"
// #include "aisdk/algorithm/func/reducesum.h"
#include "aisdk/algorithm/func/softmax.h"
#include "aisdk/base/log.h"
#include "aisdk/xengine/nrhal_define.h"
namespace aisdk::algorithm {

// 调用获取接口会崩溃，暂时这么写
void ArtosynRTMTiny::HandRtmtinyArtosynreset() {
    itensor.m_multishape_num = 1;  //只有一个tensor输入
    itensor.m_tensors[0].m_name = "input";
    // itensor.m_tensors[0].m_dimtype = TensorFormat::UNKNOWN;
    itensor.m_tensors[0].m_elementype = aisdk::xengine::ElementType::UINT8;
    itensor.m_tensors[0].m_elementbyte = 1;
    itensor.m_tensors[0].m_artosyn_dims.achName = "input";
    itensor.m_tensors[0].m_artosyn_dims.achType = "uint8";
    itensor.m_tensors[0].m_artosyn_dims.achStepType = "";
    itensor.m_tensors[0].m_artosyn_dims.achLayoutType = "";
    // itensor.m_tensors[0].m_artosyn_dims.achMemoryType = std::string(stTensor.achMemoryType);
    // itensor.m_tensors[0].m_artosyn_dims.achDdrFormat = std::string(stTensor.achDdrFormat);
    itensor.m_tensors[0].m_artosyn_dims.dScaleFactor = 0.000000;
    itensor.m_tensors[0].m_artosyn_dims.u32ID = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32Bank = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32Offset = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32Height = 128;
    itensor.m_tensors[0].m_artosyn_dims.u32KStep = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32KNormNum = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32KSizeLast = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32KSizeNorm = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32BitWidth = 8;
    itensor.m_tensors[0].m_artosyn_dims.u32Num = 1;
    itensor.m_tensors[0].m_artosyn_dims.u32OriChannels = 1;
    itensor.m_tensors[0].m_artosyn_dims.u32OriFrameSize = 1;
    itensor.m_tensors[0].m_artosyn_dims.u32Precision = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32RowStep = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32TensorStep = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32Size = 49152;
    itensor.m_tensors[0].m_artosyn_dims.u32MemorySize = 49152;
    itensor.m_tensors[0].m_artosyn_dims.u32Width = 128;
    itensor.m_tensors[0].m_artosyn_dims.s32ZeroPoint = 0;

    otensor.m_multishape_num = 3;
    otensor.m_tensors[0].m_name = "feat_x";
    // otensor.m_tensors[0].m_dimtype = TensorFormat::UNKNOWN;
    otensor.m_tensors[0].m_elementype = aisdk::xengine::ElementType::FLOAT32;
    otensor.m_tensors[0].m_elementbyte = 4;
    otensor.m_tensors[0].m_artosyn_dims.achName = "feat_x";
    otensor.m_tensors[0].m_artosyn_dims.achType = "float";
    otensor.m_tensors[0].m_artosyn_dims.achStepType = "normal";
    otensor.m_tensors[0].m_artosyn_dims.achLayoutType = "float";
    // otensor.m_tensors[0].m_artosyn_dims.achMemoryType = std::string(stTensor.achMemoryType);
    // otensor.m_tensors[0].m_artosyn_dims.achDdrFormat = std::string(stTensor.achDdrFormat);
    otensor.m_tensors[0].m_artosyn_dims.dScaleFactor = 0.267774;
    otensor.m_tensors[0].m_artosyn_dims.u32ID = 47;
    otensor.m_tensors[0].m_artosyn_dims.u32Bank = 0;
    otensor.m_tensors[0].m_artosyn_dims.u32Offset = 0;
    otensor.m_tensors[0].m_artosyn_dims.u32Height = 21;
    otensor.m_tensors[0].m_artosyn_dims.u32KStep = 21504;
    otensor.m_tensors[0].m_artosyn_dims.u32KNormNum = 0;
    otensor.m_tensors[0].m_artosyn_dims.u32KSizeLast = 1;
    otensor.m_tensors[0].m_artosyn_dims.u32KSizeNorm = 1;
    otensor.m_tensors[0].m_artosyn_dims.u32BitWidth = 0;
    otensor.m_tensors[0].m_artosyn_dims.u32Num = 1;
    otensor.m_tensors[0].m_artosyn_dims.u32OriChannels = 1;
    otensor.m_tensors[0].m_artosyn_dims.u32OriFrameSize = 1;
    otensor.m_tensors[0].m_artosyn_dims.u32Precision = 32;
    otensor.m_tensors[0].m_artosyn_dims.u32RowStep = 1024;
    otensor.m_tensors[0].m_artosyn_dims.u32TensorStep = 21504;
    otensor.m_tensors[0].m_artosyn_dims.u32Size = 5376;
    otensor.m_tensors[0].m_artosyn_dims.u32MemorySize = 21504;
    otensor.m_tensors[0].m_artosyn_dims.u32Width = 256;
    otensor.m_tensors[0].m_artosyn_dims.s32ZeroPoint = -8;

    otensor.m_tensors[1].m_name = "feat_y";
    // otensor.m_tensors[1].m_dimtype = TensorFormat::UNKNOWN;
    otensor.m_tensors[1].m_elementype = aisdk::xengine::ElementType::FLOAT32;
    otensor.m_tensors[1].m_elementbyte = 4;
    otensor.m_tensors[1].m_artosyn_dims.achName = "feat_y";
    otensor.m_tensors[1].m_artosyn_dims.achType = "float";
    otensor.m_tensors[1].m_artosyn_dims.achStepType = "normal";
    otensor.m_tensors[1].m_artosyn_dims.achLayoutType = "float";
    // otensor.m_tensors[1].m_artosyn_dims.achMemoryType = std::string(stTensor.achMemoryType);
    // otensor.m_tensors[1].m_artosyn_dims.achDdrFormat = std::string(stTensor.achDdrFormat);
    otensor.m_tensors[1].m_artosyn_dims.dScaleFactor = 0.271989;
    otensor.m_tensors[1].m_artosyn_dims.u32ID = 49;
    otensor.m_tensors[1].m_artosyn_dims.u32Bank = 0;
    otensor.m_tensors[1].m_artosyn_dims.u32Offset = 24576;
    otensor.m_tensors[1].m_artosyn_dims.u32Height = 21;
    otensor.m_tensors[1].m_artosyn_dims.u32KStep = 21504;
    otensor.m_tensors[1].m_artosyn_dims.u32KNormNum = 0;
    otensor.m_tensors[1].m_artosyn_dims.u32KSizeLast = 1;
    otensor.m_tensors[1].m_artosyn_dims.u32KSizeNorm = 1;
    otensor.m_tensors[1].m_artosyn_dims.u32BitWidth = 0;
    otensor.m_tensors[1].m_artosyn_dims.u32Num = 1;
    otensor.m_tensors[1].m_artosyn_dims.u32OriChannels = 1;
    otensor.m_tensors[1].m_artosyn_dims.u32OriFrameSize = 1;
    otensor.m_tensors[1].m_artosyn_dims.u32Precision = 32;
    otensor.m_tensors[1].m_artosyn_dims.u32RowStep = 1024;
    otensor.m_tensors[1].m_artosyn_dims.u32TensorStep = 21504;
    otensor.m_tensors[1].m_artosyn_dims.u32Size = 5376;
    otensor.m_tensors[1].m_artosyn_dims.u32MemorySize = 21504;
    otensor.m_tensors[1].m_artosyn_dims.u32Width = 256;
    otensor.m_tensors[1].m_artosyn_dims.s32ZeroPoint = 0;

    otensor.m_tensors[2].m_name = "feat_z";
    // otensor.m_tensors[2].m_dimtype = TensorFormat::UNKNOWN;
    otensor.m_tensors[2].m_elementype = aisdk::xengine::ElementType::FLOAT32;
    otensor.m_tensors[2].m_elementbyte = 4;
    otensor.m_tensors[2].m_artosyn_dims.achName = "feat_z";
    otensor.m_tensors[2].m_artosyn_dims.achType = "float";
    otensor.m_tensors[2].m_artosyn_dims.achStepType = "normal";
    otensor.m_tensors[2].m_artosyn_dims.achLayoutType = "float";
    // otensor.m_tensors[2].m_artosyn_dims.achMemoryType = std::string(stTensor.achMemoryType);
    // otensor.m_tensors[2].m_artosyn_dims.achDdrFormat = std::string(stTensor.achDdrFormat);
    otensor.m_tensors[2].m_artosyn_dims.dScaleFactor = 0.361561;
    otensor.m_tensors[2].m_artosyn_dims.u32ID = 51;
    otensor.m_tensors[2].m_artosyn_dims.u32Bank = 0;
    otensor.m_tensors[2].m_artosyn_dims.u32Offset = 49152;
    otensor.m_tensors[2].m_artosyn_dims.u32Height = 21;
    otensor.m_tensors[2].m_artosyn_dims.u32KStep = 21504;
    otensor.m_tensors[2].m_artosyn_dims.u32KNormNum = 0;
    otensor.m_tensors[2].m_artosyn_dims.u32KSizeLast = 1;
    otensor.m_tensors[2].m_artosyn_dims.u32KSizeNorm = 1;
    otensor.m_tensors[2].m_artosyn_dims.u32BitWidth = 0;
    otensor.m_tensors[2].m_artosyn_dims.u32Num = 1;
    otensor.m_tensors[2].m_artosyn_dims.u32OriChannels = 1;
    otensor.m_tensors[2].m_artosyn_dims.u32OriFrameSize = 1;
    otensor.m_tensors[2].m_artosyn_dims.u32Precision = 32;
    otensor.m_tensors[2].m_artosyn_dims.u32RowStep = 1024;
    otensor.m_tensors[2].m_artosyn_dims.u32TensorStep = 21504;
    otensor.m_tensors[2].m_artosyn_dims.u32Size = 5376;
    otensor.m_tensors[2].m_artosyn_dims.u32MemorySize = 21504;
    otensor.m_tensors[2].m_artosyn_dims.u32Width = 256;
    otensor.m_tensors[2].m_artosyn_dims.s32ZeroPoint = 24;
    return;
}

/**
 * @brief 初始化手部关键点检测网络模型
 * @param algo 网络算法配置（计算图结构、内存分配策略等）
 * @param model 模型配置（模型文件路径、输入输出设置等）
 * @param session 会话配置（线程数、执行设备等）
 * @return absl::Status 初始化状态，OK表示成功，否则包含错误信息
 */
absl::Status ArtosynRTMTiny::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                  aisdk::xengine::SessionConfig &session) {
    // step1：调用基类初始化方法，完成公共网络参数设置
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }

    // 暂时这么写，后续修改
    HandRtmtinyArtosynreset();

    // step2: 初始化坐标变换系数（创建线性映射序列： 热图坐标 -> 原始图像坐标）
    mul_coeff_ = linspace<float>(0, 1, output_shape_, false);
    return ret;
}

/**
 * @brief 预处理输入图像数据，将其转换为模型所需的二进制格式
 * @param net_input 输入图像集合，要求为灰度图格式
 */
void ArtosynRTMTiny::PreProcess(const std::vector<Image> &net_input) {
    AISDK_LOG_TRACE("ArtosynRTMTiny::PreProcess");

    // step1：校验输入批量与模型配置的一致性
    auto ai = itensor.m_batch * itensor.m_multishape_num;  // 预期输入
    auto bi = net_input.size();                            // 实际输入
    if (ai != bi || !itensor.m_packed_bybatch) {
        return;
    }

    // step2: 遍历处理每张图片
    for (size_t i = 0; i < bi; i++) {
        auto &img = net_input[i].m_mat;

        // step2.1 计算多维张量索引（支持单批次多输入或多批次配置）
        int multi_i = i / itensor.m_batch;  // 多输入张量索引
        int batch_i = i % itensor.m_batch;  // 批次内索引

        // step2.2: 根据配置获取图像高度，宽度，通道数等信息
        int channels = itensor.m_tensors[multi_i].m_artosyn_dims.u32OriChannels;
        int height = itensor.m_tensors[multi_i].m_artosyn_dims.u32Height;
        int width = itensor.m_tensors[multi_i].m_artosyn_dims.u32Width;

        // step2.3: 获取图像信息
        int element_byte = itensor.m_tensors[multi_i].m_elementbyte;  // 元素字节数
        int mem_size = height * width * channels * element_byte;
        char *mem = (char *)itensor.m_tensors[multi_i].m_viraddr + batch_i * mem_size;

        // step2.4: 图像resize和拷贝
        cv::Mat image_resized(img.size(), CV_8UC1, mem);
        img.convertTo(image_resized, CV_8UC1);
    }
}

/**
 * @brief 后处理函数，将模型输出解析为二维关键点坐标
 * @param result 输出参数，存储解析后的关键点坐标结果
 *
 * @note 处理流程：
 * 1. 获取输出层信息：深度图(depth)和热图特征(feat)的维度参数
 * 2. 初始化结果存储结构：根据批次大小预分配内存
 * 3. 遍历每个批次：
 *    a. 从NPU输出内存中提取热图数据
 *    b. 使用IPR方法计算亚像素级坐标
 *    c. 将热图坐标映射回输入图像分辨率
 */
void ArtosynRTMTiny::PostProcess(Kpt2dResult &result) {
    AISDK_LOG_TRACE("ArtosynRTMTiny::PostProcess");

    // step1: 输出格式验证
    if (!otensor.m_packed_bybatch) {
        return;
    }

    // step2: 结果内存预分配
    result.kpts.resize(otensor.m_batch);
    result.rdepths.resize(otensor.m_batch);

    // step3: 遍历处理output tensor的内容
    for (size_t multi_i = 0; multi_i < otensor.m_multishape_num; multi_i++) {
        for (size_t batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
            // step3.1 解析otensor的channel, height, width
            int channel = 1;
            int height = otensor.m_tensors[multi_i].m_artosyn_dims.u32Height;
            int width = otensor.m_tensors[multi_i].m_artosyn_dims.u32Width;
            int element_byte = otensor.m_tensors[multi_i].m_elementbyte;

            // step3.2 获取输出具体数据信息
            char *mem =
                (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * height * width * channel * element_byte;
            float *_data = (float *)mem;

            auto &rsnkpt = result.kpts[batch_i];
            auto &rdepth = result.rdepths[batch_i];
            if (rsnkpt.size() != keypoint_num_) {
                rsnkpt.resize(keypoint_num_);
                rdepth.resize(keypoint_num_);
            }

            // step3.3 Softmax概率转换（将原始输出转换为概率分布）
            std::vector<float> kpt_softmax_data(channel * height * width);

            // 在空间维度应用softmax（width方向）
            softmax_last_dim(_data, kpt_softmax_data.data(), {1, height, width});

            // step4：处理不同输出层数据
            for (size_t i = 0; i < keypoint_num_; i++) {
                // step4.1: 处理x坐标输出层
                if (otensor.m_tensors[multi_i].m_name == "feat_x") {
                    rsnkpt[i][0] = std::inner_product(mul_coeff_.begin(), mul_coeff_.end(),
                                                      kpt_softmax_data.begin() + i * width, 0.0F) *
                                   static_cast<float>(input_shape_);
                }

                // step4.2: 处理y坐标输出层
                if (otensor.m_tensors[multi_i].m_name == "feat_y") {
                    rsnkpt[i][1] = std::inner_product(mul_coeff_.begin(), mul_coeff_.end(),
                                                      kpt_softmax_data.begin() + i * width, 0.0F) *
                                   static_cast<float>(input_shape_);
                }

                // step4.3: 处理深度输出层
                if (otensor.m_tensors[multi_i].m_name == "feat_z") {
                    rdepth[i] = (std::inner_product(mul_coeff_.begin(), mul_coeff_.end(),
                                                    kpt_softmax_data.begin() + i * width, 0.0F) -
                                 0.5) *
                                0.4;
                }
            }
        }
    }
}

/**
 * @brief 执行端到端的手部关键点检测推理流程
 * @param baseinput 输入图像集合，要求符合模型输入规格（如128x128灰度图）
 * @return absl::StatusOr<Kpt2dResult> 包含关键点检测结果或错误状态
 */
absl::StatusOr<Kpt2dResult> ArtosynRTMTiny::Inference(const std::vector<Image> &baseinput) {
    // step1: 预处理
    PreProcess(baseinput);
    Kpt2dResult baseresult;

    // step2: 模型推理
    absl::Status ret = m_net->RunNet();
    if (ret.ok()) {
        // step3: 后处理
        PostProcess(baseresult);
        return baseresult;
    }

    return absl::UnavailableError("failed to get 2d hand kpt result from rtmtiny artosyn");
}

}  // namespace aisdk::algorithm
