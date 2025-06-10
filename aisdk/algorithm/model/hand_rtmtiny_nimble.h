#pragma once

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <vector>
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/common/NR_GlobalPredictorService.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/func/hand_nimble.h"

#include "calculator_basenet.h"

namespace aisdk::algorithm {


class RTMTinyNimbleDLT : public MonoNimbleDLTBaseNet {
   public:
    RTMTinyNimbleDLT()=default;
    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);
    void PreProcess(const MonoHandNimbleInputs &baseinput);
    void PostProcess(MonoHandNimbleOutputs &result);
    absl::StatusOr<MonoHandNimbleOutputs> Inference(const MonoHandNimbleInputs &baseinput) override;

   private:
    aisdk::xengine::TensorFormat itensor_format_;
    aisdk::xengine::TensorFormat otensor_format_;
    unsigned int input_image_shape_ = 128;
    float img_mean_ = 114.495;
    float img_std_ = 57.63;
    unsigned int output_feat_shape_ = 256;
    unsigned int keypoint_num_ = 21;
    std::vector<float> mul_coeff_;

    float standard_fscale_ = 200;
    std::vector<float> mem_left_hand;
    std::vector<float> mem_right_hand;
    
    double reset_mem_time = 0.05;  // s
    double last_left_time = 0;
    double last_right_time = 0;
    
    bool is_left_ = false;
    std::shared_ptr<base::PerspectiveCameraModel> virtual_camera_ = nullptr; 
    double input_timestamp_;
};

}  // namespace aisdk::algorithm
