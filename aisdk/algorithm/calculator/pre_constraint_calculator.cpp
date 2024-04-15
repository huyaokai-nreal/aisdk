#include <iostream>
#include <memory>

#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"
#include "thirdparty/MANO_IK-main/mano/AIK.h"

#define KPT_NUMS 21

static std::vector<cv::Vec3f> constrain_func_pre(const std::vector<cv::Vec3f>& input_kpt3d, bool is_left) {
    std::vector<cv::Vec3f> output_kpt3d(KPT_NUMS);
    std::vector<Eigen::Vector3f> res3d_mano_input(KPT_NUMS), res3d_mano_output(KPT_NUMS);
    for (int i = 0; i < KPT_NUMS; i++) {
        res3d_mano_input[i][0] = input_kpt3d[i][0];
        res3d_mano_input[i][1] = input_kpt3d[i][1];
        res3d_mano_input[i][2] = input_kpt3d[i][2];
    }

    res3d_mano_output = constraint_hand_v2(res3d_mano_input, is_left);

    for (int i = 0; i < KPT_NUMS; i++) {
        output_kpt3d[i][0] = res3d_mano_output[i][0];
        output_kpt3d[i][1] = res3d_mano_output[i][1];
        output_kpt3d[i][2] = res3d_mano_output[i][2];
    }
    return output_kpt3d;
}

namespace mediapipe {
// A calculator doing constraint based on MANO template.
// The implement is exactly same to PostConstrainCalculator, maybe merged in next phase.
// Definition:
// node {
//   calculator: "PreConstrainCalculator"
//   input_stream: "INPUT:kpt3d"
//   output_stream: "OUTPUT:kpt3d_constrained"
// }

class PreConstrainCalculator : public CalculatorBase {
   private:
   public:
    static absl::Status GetContract(CalculatorContract* cc) {
        AISDK_LOG_TRACE("[PreConstrainCalculator] GetContract start");
        cc->Inputs().Tag("INPUT").Set<aisdk::algorithm::Kpt3dInternal>();
        cc->Outputs().Tag("OUTPUT").Set<aisdk::algorithm::Kpt3dInternal>();
        AISDK_LOG_TRACE("[PreConstrainCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[PreConstrainCalculator] Open start");
        AISDK_LOG_TRACE("[PreConstrainCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(PreConstrainCalculator::Process);
#endif
        AISDK_LOG_TRACE("[PreConstrainCalculator] Process start");
        const auto& input_data = cc->Inputs().Tag("INPUT").Get<aisdk::algorithm::Kpt3dInternal>();

        std::unique_ptr<aisdk::algorithm::Kpt3dInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::Kpt3dInternal>();
        output_buffer_->clear();

        if (input_data.lhand_valid) {
            output_buffer_->lhand_valid = true;
            output_buffer_->lhand = constrain_func_pre(input_data.lhand, true);
        }
        if (input_data.rhand_valid) {
            output_buffer_->rhand_valid = true;
            output_buffer_->rhand = constrain_func_pre(input_data.rhand, false);
        }

        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        }

        AISDK_LOG_TRACE("[PreConstrainCalculator] Process complete");
        return absl::OkStatus();
    }
};

}  // namespace mediapipe
