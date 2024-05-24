#include "../common/NR_GlobalPredictorService.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/func/hand_rotation.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/algorithm/internal_structs/hand_gesture_struct_internal.h"
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
        cc->Inputs().Tag("INPUT_KPT").Set<HandsData>();
        cc->Inputs().Tag("INPUT_GR").Set<HandGestureInternal>();
        cc->Outputs().Tag("OUTPUT").Set<HandsData>();
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
        const auto& kpt_data = cc->Inputs().Tag("INPUT_KPT").Get<HandsData>();
        const auto& gesture_data = cc->Inputs().Tag("INPUT_GR").Get<HandGestureInternal>();
        auto output_buffer_ = absl::make_unique<HandsData>();
        if (kpt_data.lhand_valid) {
            output_buffer_->left_hand = kpt_data.left_hand;
            output_buffer_->left_hand.kpt3d = convert_to_23points(kpt_data.left_hand.kpt3d);
            output_buffer_->lhand_valid = true;
            output_buffer_->left_hand.gesture = gesture_data.lhand_gesture;
            compute_joint_rotation(output_buffer_->left_hand.kpt3d, true, output_buffer_->left_hand.rotation);
        }
        if (kpt_data.rhand_valid) {
            output_buffer_->right_hand = kpt_data.right_hand;
            output_buffer_->right_hand.kpt3d = convert_to_23points(kpt_data.right_hand.kpt3d);
            output_buffer_->rhand_valid = true;
            output_buffer_->right_hand.gesture = gesture_data.rhand_gesture;
            compute_joint_rotation(output_buffer_->right_hand.kpt3d, false, output_buffer_->right_hand.rotation);
        }
        auto& global_kpt3d = GlobalPredictorService::getInstance().get_last_kpt3d_world();
        global_kpt3d = kpt_data;
        cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
