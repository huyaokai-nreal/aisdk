#pragma once

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <vector>
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/model/calculator_basenet.h"

namespace aisdk::algorithm {

/**
 * @class ArtosynRSNTiny
 * @brief 手部关键点检测网络模型（轻量级实现），继承自基础手部关键点检测网络
 * @details 本类实现了一个基于热图回归的轻量级手部21关键点检测模型，支持128x128输入分辨率，
 *          输出32x32热图并通过后处理计算关键点坐标。包含完整的预处理、推理、后处理流程，
 *          支持NCHW格式数据输出。
 * 
 * @note 主要特性：
 * - 输入分辨率：128x128
 * - 输出热图分辨率：32x32
 * - 关键点数量：21（标准手部解剖关键点）
 * - 包含热图后处理优化（ipr方法）
 * - 支持自定义模型参数配置
 */
class ArtosynRSNTiny : public HandLandmarkBaseNet {
public:
    ArtosynRSNTiny() = default;

    // 初始化
    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);    

    // 模型推理
    absl::StatusOr<Kpt2dResult> Inference(const std::vector<Image> &input) override;

private:
    // 改进的热图坐标回归方法
    void ipr(float *__restrict input_hm, float *__restrict kpt_x_out, float *__restrict kpt_y_out);

    void PreProcess(const std::vector<Image> &net_input);  // 前处理    
    void PostProcess(Kpt2dResult &result);                 // 后处理

    unsigned int m_input_shape_ = 128;   // 网络输入分辨率（默认128x128）
    unsigned int m_output_shape_ = 32;   // 热图输出分辨率（默认32x32）
    unsigned int m_keypoint_num_ = 21;   // 手部关键点数量（标准21点）
    std::vector<float> m_hm_softmax_;           // 中间缓存：softmax处理后的热图数据
    std::vector<float> m_hm_reduce_col_;        // 中间缓存：列方向聚合后的热图数据
    std::vector<float> m_hm_reduce_row_;        // 中间缓存：行方向聚合后的热图数据
    std::vector<float> m_hm_reduce_col_row_;    // 中间缓存：先列后行聚合结果（用于坐标计算）
    std::vector<float> m_hm_reduce_row_col_;    // 中间缓存：先行后列聚合结果（用于坐标计算）
    std::vector<float> m_outputsNCHW;         // 模型输出缓存（NCHW格式）
    std::vector<float> m_mul_coeff_;            // 坐标缩放系数（用于后处理坐标映射）
};

}  // namespace aisdk::algorithm
