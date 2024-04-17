#pragma once

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <vector>
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "calculator_basenet.h"

namespace aisdk::algorithm {


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
    void PostProcess(Kpt2dResult &result);
    void PreProcessSingle(const std::vector<Image> &net_input, uint32_t batchn);
    void PostProcessSingle(Kpt2dResult &result, uint32_t batchn);
    absl::StatusOr<Kpt2dResult> Inference(const std::vector<Image> &input);

   private:
    void ipr(float *__restrict input_hm, float *__restrict kpt_x_out, float *__restrict kpt_y_out);
    aisdk::xengine::TensorFormat itensor_format_;
    aisdk::xengine::TensorFormat otensor_format_;
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
