#include "../common/NR_GlobalPredictorService.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/func/hand_rotation.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/algorithm/internal_structs/hand_gesture_struct_internal.h"
#include "aisdk/algorithm/internal_structs/hand_output_struct_internal.h"
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
        cc->Inputs().Tag("INPUT_KPT").Set<Kpt3dInternal>();
        cc->Inputs().Tag("INPUT_GR").Set<HandGestureInternal>();
        cc->Outputs().Tag("OUTPUT").Set<HandOutputInternal>();
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
        const auto& kpt_data = cc->Inputs().Tag("INPUT_KPT").Get<Kpt3dInternal>();
        const auto& gesture_data = cc->Inputs().Tag("INPUT_GR").Get<HandGestureInternal>();
        const auto& timestamp = cc->InputTimestamp().Seconds();

        auto output_buffer_ = absl::make_unique<HandOutputInternal>();
        output_buffer_->timestamp = timestamp;
        if (kpt_data.lhand_valid) {
            output_buffer_->lhand_kpt = kpt_data.lhand_kpt;
            output_buffer_->lhand_valid = true;
            output_buffer_->lhand_score = kpt_data.lhand_score;
            output_buffer_->lhand_v = kpt_data.lhand_v;
            output_buffer_->lhand_gesture = gesture_data.lhand_gesture;
            compute_joint_rotation(kpt_data.lhand_kpt, true, output_buffer_->lhand_rotation);
        }
        if (kpt_data.rhand_valid) {
            output_buffer_->rhand_kpt = kpt_data.rhand_kpt;
            output_buffer_->rhand_valid = true;
            output_buffer_->rhand_score = kpt_data.rhand_score;
            output_buffer_->rhand_v = kpt_data.rhand_v;
            output_buffer_->rhand_gesture = gesture_data.rhand_gesture;
            compute_joint_rotation(kpt_data.rhand_kpt, false, output_buffer_->rhand_rotation);
        }
        auto& global_kpt3d = GlobalPredictorService::getInstance().get_kpt3d_world();
        global_kpt3d = kpt_data;

        cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
