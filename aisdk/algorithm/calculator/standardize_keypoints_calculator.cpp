#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/func/hand_rotation.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"

namespace aisdk::algorithm {

// A calculator convert 3d keypoint format from 21 points to 23 points
// Definition:
// node {
//   name: "StandardizeKeypoints"
//   calculator: "StandardizeKeypointsCalculator"
//   input_stream: "INPUT:kpt3d_world"
//   output_stream: "OUTPUT:kpt3d_standard"
// }

class StandardizeKeypointsCalculator : public xgraph::CalculatorBase {
   private:
   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] GetContract start.");
        cc->Inputs().Tag("INPUT").Set<Kpt3dInternal>();
        cc->Outputs().Tag("OUTPUT").Set<Kpt3dInternal>();
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] Open start.");
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(StandardizeKeypointsCalculator::Process);
#endif
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] Process start.");
        const auto& input_data = cc->Inputs().Tag("INPUT").Get<Kpt3dInternal>();

        std::unique_ptr<Kpt3dInternal> output_buffer_ = absl::make_unique<Kpt3dInternal>();
        *output_buffer_ = input_data;

        if (input_data.lhand_valid) {
            compute_joint_rotation(input_data.lhand_kpt, true, output_buffer_->lhand_rotation);
        }
        if (input_data.rhand_valid) {
            compute_joint_rotation(input_data.rhand_kpt, false, output_buffer_->rhand_rotation);
        }

        cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
