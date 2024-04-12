#pragma once

#include <cstdlib>

#include "calculator_basenet.h"
#include "../func/netalgo_utils.h"

namespace aisdk::algorithm {

#define KEYPOINT_NUM 21

class LandmarkFilter : public CalculatorBaseNet {
   public:
    LandmarkFilter() : CalculatorBaseNet(){};
    ~LandmarkFilter(){};

    aisdk::xengine::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);
    void PreProcess(const std::vector<std::vector<cv::Vec2f>> &net_input);
    void PostProcess(std::vector<cv::Vec2f> &result);
    aisdk::xengine::Status Inference(const std::vector<std::vector<cv::Vec2f>> &baseinput,
                                     std::vector<cv::Vec2f> &baseresult);

   private:
    aisdk::xengine::TensorFormat itensor_format;
    aisdk::xengine::TensorFormat otensor_format;

    std::vector<float> abs_scale;
    cv::Vec2f m_center_uv;
    float m_scale;
};

}  // namespace aisdk::algorithm
