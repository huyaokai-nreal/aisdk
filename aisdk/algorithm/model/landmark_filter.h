#pragma once

#include <opencv2/core/matx.hpp>

#include "aisdk/base/type.h"
#include "calculator_basenet.h"

namespace aisdk::algorithm {

/**
 * @class LandmarkFilter
 * @brief 关键点平滑滤波计算器
 *
 * 该类实现关键点序列的平滑滤波功能，主要用于时序关键点数据的去抖动和降噪处理。
 * 通过神经网络模型对关键点运动轨迹进行建模，提供稳定的输出结果。
 */
class LandmarkFilter : public CalculatorBaseNet {
public:
    LandmarkFilter() : CalculatorBaseNet(){};
    ~LandmarkFilter(){};

    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);         // 初始化网络模型
    void PreProcess(const std::vector<std::vector<Vec2f_t>> &net_input);         // 前处理
    void PostProcess(std::vector<Vec2f_t> &result);                              // 后处理
    absl::Status Inference(const std::vector<std::vector<Vec2f_t>> &baseinput,
                                     std::vector<Vec2f_t> &baseresult);          // 模型推理

private:
    // 模型相关配置
    aisdk::xengine::TensorFormat itensor_format;    // input tensor内存布局
    aisdk::xengine::TensorFormat otensor_format;    // output tensor内存布局

    std::vector<float> abs_scale;    // 坐标归一化缩放系数（各维度独立）
    Vec2f_t m_center_uv;             // 坐标中心点（归一化基准）
    float m_scale;                   // 整体缩放因子（保持运动一致性）
};

}  // namespace aisdk::algorithm
