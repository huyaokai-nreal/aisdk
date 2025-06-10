#pragma once

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <vector>
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/model/calculator_basenet.h"

namespace aisdk::algorithm {

/**
 * @class ArtosynRTMTiny
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
class ArtosynRTMTiny : public HandLandmarkBaseNet {
public:
    ArtosynRTMTiny() = default;

    // 初始化
    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);    

    // 模型推理
    absl::StatusOr<Kpt2dResult> Inference(const std::vector<Image> &input) override;

private:
    void PreProcess(const std::vector<Image> &net_input);  // 前处理    
    void PostProcess(Kpt2dResult &result);                 // 后处理
    void HandRtmtinyArtosynreset();

    // 模型相关配置
    aisdk::xengine::TensorFormat itensor_format_;    // input tensor内存布局
    aisdk::xengine::TensorFormat otensor_format_;    // output tensor内存布局

    // 热力图处理缓冲区
    std::vector<float> mul_coeff_;        // 热图坐标到原图坐标的缩放系数

    // 模型结构数据
    unsigned int input_shape_ = 128;      // 输入图像缓存（正方形，128*128像素）
    unsigned int output_shape_ = 256;     // 输出图像缓存（正方形, 32*32像素）
    unsigned int keypoint_num_ = 21;      // 手部关键点数量（21个关键点）
};

}  // namespace aisdk::algorithm
