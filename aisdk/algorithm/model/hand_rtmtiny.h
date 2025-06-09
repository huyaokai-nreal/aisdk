#pragma once

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <vector>
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "calculator_basenet.h"

namespace aisdk::algorithm {

/**
 * @class RTMTiny
 * @brief 用于手部关键点检测的高效轻量级模型，继承自基础手部关键点网络类
 *
 * 该实现专注于优化推理速度和模型尺寸，适用于移动端或嵌入式设备。
 * 通过定制化的后处理算法提升关键点定位精度。
 */
class RTMTiny : public HandLandmarkBaseNet {
public:
    RTMTiny()=default;
    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);                // 初始化网络模型
    void PreProcess(const std::vector<Image> &net_input);                               // 前处理
    void PostProcess(Kpt2dResult &result);                                              // 后处理
    absl::StatusOr<Kpt2dResult> Inference(const std::vector<Image> &input) override;    // 模型推理

private:
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
