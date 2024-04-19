#include <memory>

#include "../internal_structs/standard_kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "thirdparty/MANO_IK-main/mano/AIK.h"

#define KPT_NUMS 21

static std::vector<cv::Vec3f> constrain_func_post(const std::vector<cv::Vec3f>& input_kpt3d, bool is_left) {
    std::vector<cv::Vec3f> output_kpt3d(KPT_NUMS + 2);
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

    output_kpt3d[21] = 0.5 * (output_kpt3d[0] + output_kpt3d[9]);
    output_kpt3d[22] = 0.5 * (0.5 * (output_kpt3d[0] - output_kpt3d[9]) + 0.5 * (output_kpt3d[0] - output_kpt3d[17])) +
                       output_kpt3d[17];

    return output_kpt3d;
}

namespace aisdk::algorithm {

// A calculator doing constraint based on MANO template.
// The implement is exactly same to PreConstrainCalculator, maybe merged in next phase.
// Definition:
// node {
//   calculator: "PostConstrainCalculator"
//   input_stream: "INPUT:kpt3d_filtered"
//   output_stream: "OUTPUT:kpt3d_post_constrained"
// }

class PostConstrainCalculator : public xgraph::CalculatorBase {
   private:
   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[PostConstrainCalculator] GetContract start.");
        cc->Inputs().Tag("INPUT").Set<aisdk::algorithm::StandardKpt3dInternal>();
        cc->Outputs().Tag("OUTPUT").Set<aisdk::algorithm::StandardKpt3dInternal>();
        AISDK_LOG_TRACE("[PostConstrainCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[PostConstrainCalculator] Open start.");
        AISDK_LOG_TRACE("[PostConstrainCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(PostConstrainCalculator::Process);
#endif
        AISDK_LOG_TRACE("[PostConstrainCalculator] Process start.");
        const auto& input_data = cc->Inputs().Tag("INPUT").Get<aisdk::algorithm::StandardKpt3dInternal>();

        std::unique_ptr<aisdk::algorithm::StandardKpt3dInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::StandardKpt3dInternal>();
        output_buffer_->clear();

        if (input_data.lhand_valid) {
            output_buffer_->lhand_valid = true;
            output_buffer_->lhand = constrain_func_post(input_data.lhand, true);
        }
        if (input_data.rhand_valid) {
            output_buffer_->rhand_valid = true;
            output_buffer_->rhand = constrain_func_post(input_data.rhand, false);
        }

        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        }

        AISDK_LOG_TRACE("[PostConstrainCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
