#pragma once

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <vector>
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "calculator_basenet.h"

namespace aisdk::algorithm {


class RTMTiny : public HandLandmarkBaseNet {
   public:
    RTMTiny()=default;
    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);
    void PreProcess(const std::vector<Image> &net_input);
    void PostProcess(Kpt2dResult &result);
    absl::StatusOr<Kpt2dResult> Inference(const std::vector<Image> &input) override;

   private:
    aisdk::xengine::TensorFormat itensor_format_;
    aisdk::xengine::TensorFormat otensor_format_;
    float img_mean = 114.4950;            // 图像预处理均值
    float img_std = 57.63;                // 图像预处理方差
    unsigned int input_shape_ = 128;
    unsigned int output_shape_ = 256;
    unsigned int keypoint_num_ = 21;
    float hand_label_cls_thr = 0.6;
    std::vector<float> mul_coeff_;
};

}  // namespace aisdk::algorithm
