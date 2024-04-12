#pragma once

#include <cstdlib>
#include <vector>

#include "calculator_basenet.h"
#include "../func/netalgo_utils.h"

namespace aisdk::algorithm {

struct RSNResult {
    std::vector<std::vector<cv::Vec2f>> rsn_kpts;  // 单手 左目，右目
    std::vector<std::vector<float>> rsn_scores;
};

class RSNTiny : public CalculatorBaseNet {
   public:
    RSNTiny()=default;
    ~RSNTiny()=default;
    RSNTiny(const RSNTiny& other) = delete;
    RSNTiny(RSNTiny&& other) = delete;
    RSNTiny& operator=(RSNTiny&& other) = delete;
    RSNTiny& operator=(RSNTiny& other) = delete;
    aisdk::xengine::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);
    void PreProcess(const std::vector<Image> &net_input);
    void PostProcess(RSNResult &result);
    void PreProcessSingle(const std::vector<Image> &net_input, uint32_t batchn);
    void PostProcessSingle(RSNResult &result, uint32_t batchn);
    aisdk::xengine::Status Inference(const std::vector<Image> &baseinput, RSNResult &baseresult);

   private:
    void ipr(float *__restrict input_hm, float *__restrict kpt_x_out, float *__restrict kpt_y_out);
    aisdk::xengine::TensorFormat itensor_format;
    aisdk::xengine::TensorFormat otensor_format;
    unsigned int input_shape_ = 128;
    unsigned int output_shape_ = 32;
    unsigned int keypoint_num_ = 21;
    std::vector<float> hm_softmax_;
    std::vector<float> hm_reduce_col_;
    std::vector<float> hm_reduce_row_;
    std::vector<float> hm_reduce_col_row_;
    std::vector<float> hm_reduce_row_col_;
    std::vector<float> m_outputsNCHW;
    std::vector<float> mul_coeff_;
};

}  // namespace aisdk::algorithm
