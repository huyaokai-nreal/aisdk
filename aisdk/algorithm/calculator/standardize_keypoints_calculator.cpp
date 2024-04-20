#include "../internal_structs/kpt3d_struct_internal.h"
#include "../internal_structs/standard_kpt3d_struct_internal.h"
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
        cc->Inputs().Tag("INPUT").Set<aisdk::algorithm::Kpt3dInternal>();
        cc->Outputs().Tag("OUTPUT").Set<aisdk::algorithm::StandardKpt3dInternal>();
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
        const auto& input_data = cc->Inputs().Tag("INPUT").Get<aisdk::algorithm::Kpt3dInternal>();

        std::unique_ptr<aisdk::algorithm::StandardKpt3dInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::StandardKpt3dInternal>();
        output_buffer_->clear();

        if (input_data.lhand_valid) {
            output_buffer_->lhand_valid = true;
            output_buffer_->lhand = convert_to_23points(input_data.lhand);
        }
        if (input_data.rhand_valid) {
            output_buffer_->rhand_valid = true;
            output_buffer_->rhand = convert_to_23points(input_data.rhand);
        }

        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        }

        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
