#pragma once

#include <opencv2/core/matx.hpp>

#include "aisdk/base/type.h"
#include "calculator_basenet.h"

namespace aisdk::algorithm {

#define KEYPOINT_NUM 21

class LandmarkFilter : public CalculatorBaseNet {
   public:
    LandmarkFilter() : CalculatorBaseNet(){};
    ~LandmarkFilter(){};

    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);
    void PreProcess(const std::vector<std::vector<Vec2f_t>> &net_input);
    void PostProcess(std::vector<Vec2f_t> &result);
    absl::Status Inference(const std::vector<std::vector<Vec2f_t>> &baseinput,
                                     std::vector<Vec2f_t> &baseresult);

   private:
    aisdk::xengine::TensorFormat itensor_format;
    aisdk::xengine::TensorFormat otensor_format;

    std::vector<float> abs_scale;
    Vec2f_t m_center_uv;
    float m_scale;
};

}  // namespace aisdk::algorithm
