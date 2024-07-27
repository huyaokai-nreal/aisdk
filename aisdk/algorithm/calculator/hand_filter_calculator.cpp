#include "../common/NR_GlobalPredictorService.h"
#include "aisdk/algorithm/calculator/hand_filter_calculator.pb.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "hand_filter_calculator.pb.h"
#include "thirdparty/MANO_IK-main/mano/AIK.h"

namespace aisdk::algorithm {

// A calculator doing correction step of a global kalman filter.
// Definition:
// node {
//   name: "HandFilter"
//   calculator: "HandFilterCalculator"
//   input_stream: "INPUT:kpt3d_standard"
//   input_stream: "STATE:hand_state"
//   output_stream: "OUTPUT:kpt3d_filtered"
// }

class HandFilterCalculator : public xgraph::CalculatorBase {
   private:
    std::string glasses_type_;
    double last_timestamp_;  // in seconds

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[HandFilterCalculator] GetContract start.");
        cc->Inputs().Tag("INPUT").Set<HandsData>();
        cc->Outputs().Tag("OUTPUT").Set<HandsData>();
        AISDK_LOG_TRACE("[HandFilterCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[HandFilterCalculator] Open start.");
        const auto& config = cc->Options<HandFilterCalculatorOptions>();
        glasses_type_ = config.glasses_type();
        auto& predictor_lhand = GlobalPredictorService::getInstance().get_predictor_lhand();
        auto& predictor_rhand = GlobalPredictorService::getInstance().get_predictor_rhand();
        predictor_lhand.set_glasses_type(glasses_type_);
        predictor_rhand.set_glasses_type(glasses_type_);
        auto& predictor_lhand_bbox = GlobalPredictorService::getInstance().get_predictor_lhand_bbox();
        auto& predictor_rhand_bbox = GlobalPredictorService::getInstance().get_predictor_rhand_bbox();
        predictor_lhand_bbox.set_glasses_type(glasses_type_);
        predictor_rhand_bbox.set_glasses_type(glasses_type_);
        AISDK_LOG_TRACE("[HandFilterCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(HandFilterCalculator::Process);
#endif
        AISDK_LOG_TRACE("[HandFilterCalculator] Process start.");
        const auto& kpt3d_world = cc->Inputs().Tag("INPUT").Get<HandsData>();
        const auto& timestamp = cc->InputTimestamp().Seconds();
        std::unique_ptr<HandsData> output_buffer_ = absl::make_unique<HandsData>();
        *output_buffer_ = kpt3d_world;
        auto& predictor_lhand = GlobalPredictorService::getInstance().get_predictor_lhand();
        auto& predictor_rhand = GlobalPredictorService::getInstance().get_predictor_rhand();
        auto& predictor_lhand_bbox = GlobalPredictorService::getInstance().get_predictor_lhand_bbox();
        auto& predictor_rhand_bbox = GlobalPredictorService::getInstance().get_predictor_rhand_bbox();
        const auto kpt3d_world_pre = GlobalPredictorService::getInstance().get_last_kpt3d_world();

        if (!kpt3d_world.lhand_valid) {
            predictor_lhand.stop_tracking();
            predictor_lhand_bbox.stop_tracking();
            output_buffer_->lhand_valid = false;
        } else {
            output_buffer_->left_hand.kpt3d = kpt3d_world.left_hand.kpt3d;
            output_buffer_->left_hand.kpt3d = constraint_hand_v2(output_buffer_->left_hand.kpt3d, true);
            output_buffer_->left_hand.kpt3d = convert_to_23points(output_buffer_->left_hand.kpt3d);
            if (!predictor_lhand.get_tracking_status()) {
                predictor_lhand.start_tracking(timestamp,
                                               {output_buffer_->left_hand.kpt3d[kKeypointRootId], {0., 0., 0.}});
                predictor_lhand_bbox.start_tracking(timestamp,
                                                    {output_buffer_->left_hand.kpt3d[kKeypointRootId], {0., 0., 0.}});
            } else {
                if (kpt3d_world_pre.lhand_valid) {
                    auto measure_v = (output_buffer_->left_hand.kpt3d[kKeypointRootId] -
                                      kpt3d_world_pre.left_hand.kpt3d[kKeypointRootId]) /
                                     (timestamp - last_timestamp_);
                    output_buffer_->left_hand.root_v = measure_v;
                    predictor_lhand.track_with_correct(timestamp,
                                                       {output_buffer_->left_hand.kpt3d[kKeypointRootId], measure_v});
                    predictor_lhand_bbox.track_with_correct(
                        timestamp, {output_buffer_->left_hand.kpt3d[kKeypointRootId], measure_v});
                }
            }
        }

        if (!kpt3d_world.rhand_valid) {
            predictor_rhand.stop_tracking();
            predictor_rhand_bbox.stop_tracking();
            output_buffer_->rhand_valid = false;
        } else {
            output_buffer_->right_hand.kpt3d = kpt3d_world.right_hand.kpt3d;
            output_buffer_->right_hand.kpt3d = constraint_hand_v2(output_buffer_->right_hand.kpt3d, false);
            output_buffer_->right_hand.kpt3d = convert_to_23points(output_buffer_->right_hand.kpt3d);
            if (!predictor_rhand.get_tracking_status()) {
                predictor_rhand.start_tracking(timestamp,
                                               {output_buffer_->right_hand.kpt3d[kKeypointRootId], {0., 0., 0.}});
                predictor_rhand_bbox.start_tracking(timestamp,
                                                    {output_buffer_->right_hand.kpt3d[kKeypointRootId], {0., 0., 0.}});
            } else {
                if (kpt3d_world_pre.rhand_valid) {
                    auto measure_v = (output_buffer_->right_hand.kpt3d[kKeypointRootId] -
                                      kpt3d_world_pre.right_hand.kpt3d[kKeypointRootId]) /
                                     (timestamp - last_timestamp_);
                    output_buffer_->right_hand.root_v = measure_v;
                    predictor_rhand.track_with_correct(timestamp,
                                                       {output_buffer_->right_hand.kpt3d[kKeypointRootId], measure_v});
                    predictor_rhand_bbox.track_with_correct(
                        timestamp, {output_buffer_->right_hand.kpt3d[kKeypointRootId], measure_v});
                }
            }
        }
        cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        last_timestamp_ = timestamp;
        AISDK_LOG_TRACE("[HandFilterCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
