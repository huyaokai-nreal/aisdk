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
        cc->Inputs().Tag("GR_KPT2D_INPUT").Set<Kpt2dInternal>();
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
        auto& predictor_lhand_lcam_bbox = GlobalPredictorService::getInstance().get_predictor_lhand_lcam_bbox();
        auto& predictor_rhand_lcam_bbox = GlobalPredictorService::getInstance().get_predictor_rhand_lcam_bbox();
        auto& predictor_lhand_rcam_bbox = GlobalPredictorService::getInstance().get_predictor_lhand_rcam_bbox();
        auto& predictor_rhand_rcam_bbox = GlobalPredictorService::getInstance().get_predictor_rhand_rcam_bbox();
        predictor_lhand_lcam_bbox.set_glasses_type(glasses_type_);
        predictor_rhand_lcam_bbox.set_glasses_type(glasses_type_);
        predictor_lhand_rcam_bbox.set_glasses_type(glasses_type_);
        predictor_rhand_rcam_bbox.set_glasses_type(glasses_type_);
        AISDK_LOG_TRACE("[HandFilterCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(HandFilterCalculator::Process);
#endif
        AISDK_LOG_TRACE("[HandFilterCalculator] Process start.");
        const auto& kpt3d_world = cc->Inputs().Tag("INPUT").Get<HandsData>();
        const auto& kpt2d_data = cc->Inputs().Tag("GR_KPT2D_INPUT").Get<Kpt2dInternal>();
        const auto& timestamp = cc->InputTimestamp().Seconds();
        std::unique_ptr<HandsData> output_buffer_ = absl::make_unique<HandsData>();
        *output_buffer_ = kpt3d_world;
        auto& predictor_lhand = GlobalPredictorService::getInstance().get_predictor_lhand();
        auto& predictor_rhand = GlobalPredictorService::getInstance().get_predictor_rhand();
        auto& predictor_lhand_lcam_bbox = GlobalPredictorService::getInstance().get_predictor_lhand_lcam_bbox();
        auto& predictor_rhand_lcam_bbox = GlobalPredictorService::getInstance().get_predictor_rhand_lcam_bbox();
        auto& predictor_lhand_rcam_bbox = GlobalPredictorService::getInstance().get_predictor_lhand_rcam_bbox();
        auto& predictor_rhand_rcam_bbox = GlobalPredictorService::getInstance().get_predictor_rhand_rcam_bbox();
        const auto kpt3d_world_pre = GlobalPredictorService::getInstance().get_last_kpt3d_world();
        const auto kpt2d_data_pre = GlobalPredictorService::getInstance().get_last_kpt2d_pixel();

        GlobalPredictorService::getInstance().set_last_kpt2d_pixel(kpt2d_data);  // 当前帧使用完kpt2d以后才设置

        if (!kpt3d_world.lhand_valid) {
            predictor_lhand.stop_tracking();
            output_buffer_->lhand_valid = false;
        } else {
            output_buffer_->left_hand.kpt3d = kpt3d_world.left_hand.kpt3d;
            if (!predictor_lhand.get_tracking_status()) {
                predictor_lhand.start_tracking(timestamp,
                                               {output_buffer_->left_hand.kpt3d[kKeypointRootId], {0., 0., 0.}});
            } else {
                if (kpt3d_world_pre.lhand_valid) {
                    auto measure_v = (output_buffer_->left_hand.kpt3d[kKeypointRootId] -
                                      kpt3d_world_pre.left_hand.kpt3d[kKeypointRootId]) /
                                     (timestamp - last_timestamp_);
                    output_buffer_->left_hand.root_v = measure_v;
                    predictor_lhand.track_with_correct(timestamp,
                                                       {output_buffer_->left_hand.kpt3d[kKeypointRootId], measure_v});
                }
            }
        }

        if (!kpt3d_world.rhand_valid) {
            predictor_rhand.stop_tracking();
            output_buffer_->rhand_valid = false;
        } else {
            output_buffer_->right_hand.kpt3d = kpt3d_world.right_hand.kpt3d;
            if (!predictor_rhand.get_tracking_status()) {
                predictor_rhand.start_tracking(timestamp,
                                               {output_buffer_->right_hand.kpt3d[kKeypointRootId], {0., 0., 0.}});
            } else {
                if (kpt3d_world_pre.rhand_valid) {
                    auto measure_v = (output_buffer_->right_hand.kpt3d[kKeypointRootId] -
                                      kpt3d_world_pre.right_hand.kpt3d[kKeypointRootId]) /
                                     (timestamp - last_timestamp_);
                    output_buffer_->right_hand.root_v = measure_v;
                    predictor_rhand.track_with_correct(timestamp,
                                                       {output_buffer_->right_hand.kpt3d[kKeypointRootId], measure_v});
                }
            }
        }
        // lhand_lcam
        if (!kpt2d_data.lhand_lcam_valid || !kpt3d_world.lhand_valid) {
            predictor_lhand_lcam_bbox.stop_tracking();
        } else {
            if (!predictor_lhand_lcam_bbox.get_tracking_status()) {
                predictor_lhand_lcam_bbox.start_tracking(timestamp,
                                                         {kpt3d_world.left_hand.kpt2d_lcam[kKeypoint2dRootId], {0, 0}});
            } else {
                if (kpt2d_data_pre.lhand_lcam_valid) {
                    auto measure_v = (kpt3d_world.left_hand.kpt2d_lcam[kKeypoint2dRootId] -
                                      kpt3d_world_pre.left_hand.kpt2d_lcam[kKeypoint2dRootId]) /
                                     (timestamp - last_timestamp_);
                    predictor_lhand_lcam_bbox.track_with_correct(
                        timestamp, {kpt3d_world.left_hand.kpt2d_lcam[kKeypoint2dRootId], measure_v});
                }
            }
        }
        // rhand_lcam
        if (!kpt2d_data.rhand_lcam_valid || !kpt3d_world.rhand_valid) {
            predictor_rhand_lcam_bbox.stop_tracking();
        } else {
            if (!predictor_rhand_lcam_bbox.get_tracking_status()) {
                predictor_rhand_lcam_bbox.start_tracking(
                    timestamp, {kpt3d_world.right_hand.kpt2d_lcam[kKeypoint2dRootId], {0, 0}});
            } else {
                if (kpt2d_data_pre.rhand_lcam_valid) {
                    auto measure_v = (kpt3d_world.right_hand.kpt2d_lcam[kKeypoint2dRootId] -
                                      kpt3d_world_pre.right_hand.kpt2d_lcam[kKeypoint2dRootId]) /
                                     (timestamp - last_timestamp_);
                    predictor_rhand_lcam_bbox.track_with_correct(
                        timestamp, {kpt3d_world.right_hand.kpt2d_lcam[kKeypoint2dRootId], measure_v});
                }
            }
        }
        // lhand_rcam
        if (!kpt2d_data.lhand_rcam_valid || !kpt3d_world.lhand_valid) {
            predictor_lhand_rcam_bbox.stop_tracking();
        } else {
            if (!predictor_lhand_rcam_bbox.get_tracking_status()) {
                predictor_lhand_rcam_bbox.start_tracking(timestamp,
                                                         {kpt3d_world.left_hand.kpt2d_rcam[kKeypoint2dRootId], {0, 0}});
            } else {
                if (kpt2d_data_pre.lhand_rcam_valid) {
                    auto measure_v = (kpt3d_world.left_hand.kpt2d_rcam[kKeypoint2dRootId] -
                                      kpt3d_world_pre.left_hand.kpt2d_rcam[kKeypoint2dRootId]) /
                                     (timestamp - last_timestamp_);
                    predictor_lhand_rcam_bbox.track_with_correct(
                        timestamp, {kpt3d_world.left_hand.kpt2d_rcam[kKeypoint2dRootId], measure_v});
                }
            }
        }
        // rhand_rcam
        if (!kpt2d_data.rhand_rcam_valid || !kpt3d_world.rhand_valid) {
            predictor_rhand_rcam_bbox.stop_tracking();
        } else {
            if (!predictor_rhand_rcam_bbox.get_tracking_status()) {
                predictor_rhand_rcam_bbox.start_tracking(
                    timestamp, {kpt3d_world.right_hand.kpt2d_rcam[kKeypoint2dRootId], {0, 0}});
            } else {
                if (kpt2d_data_pre.rhand_rcam_valid) {
                    auto measure_v = (kpt3d_world.right_hand.kpt2d_rcam[kKeypoint2dRootId] -
                                      kpt3d_world_pre.right_hand.kpt2d_rcam[kKeypoint2dRootId]) /
                                     (timestamp - last_timestamp_);
                    predictor_rhand_rcam_bbox.track_with_correct(
                        timestamp, {kpt3d_world.right_hand.kpt2d_rcam[kKeypoint2dRootId], measure_v});
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
