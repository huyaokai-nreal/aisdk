#include <memory>
#include <opencv2/core/matx.hpp>

#include "../internal_structs/kpt2d_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "../model/hand_lift.h"
#include "aisdk/algorithm/common/metrics.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/algorithm/model/calculator_basenet.h"
#include "aisdk/algorithm/model/hand_lift_nimble.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "hand_lift_calculator.pb.h"
#include "xgraph_service_utils.h"

namespace aisdk::algorithm {

// A calculator generate hand 3d keypoint result, based on liftnet.
// Definition:
// node {
//   name: "LiftNet_3D"
//   calculator: "LiftCalculator"
//   input_side_packet: "CAM_INFO_INPUT:cam_info"
//   input_stream: "LANDMARK_INPUT:kpt2d"
//   output_stream: "LIFT_OUTPUT:kpt3d"
// }

class HandLiftCalculator : public xgraph::CalculatorBase {
   private:
    // SeqGMLPLiftNet algo instance
    std::shared_ptr<LiftBaseNet> netalgo;
    std::shared_ptr<base::BaseCameraModel> lcam_model_ = nullptr;
    std::shared_ptr<base::BaseCameraModel> rcam_model_ = nullptr;
    std::string model_name_;
    bool enable_constrain_;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[LiftCalculator] GetContract start");
        cc->InputSidePackets()
            .Tag("CAM_INFO_INPUT")
            .Set<std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>,
                           std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        cc->Inputs().Tag("LANDMARK_INPUT").Set<Kpt2dInternal>();
        cc->Outputs().Tag("LIFT_OUTPUT").Set<HandsData>();
        AISDK_LOG_TRACE("[LiftCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[LiftCalculator] Open start");

        // 3d_lift
        const auto& config = cc->Options<HandLiftCalculatorOptions>();
        model_name_ = config.model_name();
        enable_constrain_ = config.enable_constrain();
        if (model_name_ == "3d_lift") {
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<GMLPLiftNet3>((void*)0x202310, "3d_lift");
        } else if (model_name_ == "3d_liftnimble") {
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<GMLPLiftNimble>((void*)0x202310, "3d_liftnimble");
        } else if (model_name_ == "3d_lift_ella") {
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<GMLPLiftNet>((void*)0x202310, "3d_lift_ella");
        } else {
            return absl::AbortedError(fmt::format("can not init model with {}", model_name_));
        }
        if (!netalgo) {
            return absl::Status(absl::StatusCode::kInvalidArgument,
                                "[LiftCalculator] CreateNetAlgoBase nodename error");
        }

        const auto& cam_info = cc->InputSidePackets()
                                   .Tag("CAM_INFO_INPUT")
                                   .Get<std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>,
                                                  std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        lcam_model_ = cam_info.first;
        rcam_model_ = cam_info.second;
        auto result = netalgo->SetCameraInfo(lcam_model_, rcam_model_);
        AISDK_LOG_TRACE("[LiftCalculator] Open complete");
        return result;
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(LiftCalculator::Process);
#endif
        AISDK_LOG_TRACE("[LiftCalculator] Process start");
        const auto& kpt2d = cc->Inputs().Tag("LANDMARK_INPUT").Get<Kpt2dInternal>();
        const auto& timestamp = cc->InputTimestamp().Seconds();
        std::unique_ptr<HandsData> output_buffer_ = absl::make_unique<HandsData>();
        if (kpt2d.lhand_lcam_valid && kpt2d.lhand_rcam_valid) {
            LiftNetInputs lift_inputs;
            lift_inputs.input_kpt_lcam = lcam_model_->undistort(kpt2d.lhand_lcam_kpt);
            lift_inputs.input_kpt_rcam = rcam_model_->undistort(kpt2d.lhand_rcam_kpt);
            lift_inputs.is_left = 1.;
            lift_inputs.timestamp = timestamp;
            const auto lift_outputs = netalgo->Inference(lift_inputs);
            if (lift_outputs.ok()) {
                output_buffer_->lhand_valid = true;
                AISDK_LOG_TRACE("[LiftCalculator] left constrain start with {} kpts", lift_outputs->res3d.size());
                output_buffer_->left_hand.kpt3d = lift_outputs->res3d;
                if (enable_constrain_) {
                    output_buffer_->left_hand.kpt3d = constrain_hand(output_buffer_->left_hand.kpt3d, true);
                    AISDK_LOG_TRACE("[LiftCalculator] left constrain finish");
                }
                if (model_name_ == "3d_liftnimble") {
                    output_buffer_->left_hand.score = lift_outputs->kpt3d_score;
                } else {
                    output_buffer_->left_hand.score =
                        compute_score_with_reprojection(output_buffer_->left_hand.kpt3d, kpt2d.lhand_lcam_kpt,
                                                        kpt2d.lhand_rcam_kpt, lcam_model_, rcam_model_);
                }
                AISDK_LOG_TRACE("[LiftCalculator] left hand score is {}", output_buffer_->lhand_score);
            } else {
                output_buffer_->lhand_valid = false;
            }
        }

        if (kpt2d.rhand_rcam_valid && kpt2d.rhand_lcam_valid) {
            LiftNetInputs lift_inputs;
            lift_inputs.input_kpt_lcam = lcam_model_->undistort(kpt2d.rhand_lcam_kpt);
            lift_inputs.input_kpt_rcam = rcam_model_->undistort(kpt2d.rhand_rcam_kpt);
            lift_inputs.is_left = 0.;
            lift_inputs.timestamp = timestamp;
            const auto lift_outputs = netalgo->Inference(lift_inputs);
            if (lift_outputs.ok()) {
                output_buffer_->rhand_valid = true;
                output_buffer_->right_hand.kpt3d = lift_outputs->res3d;
                if (enable_constrain_) {
                    output_buffer_->right_hand.kpt3d = constrain_hand(output_buffer_->right_hand.kpt3d, false);
                }
                if (model_name_ == "3d_liftnimble") {
                    output_buffer_->right_hand.score = lift_outputs->kpt3d_score;
                } else {
                    output_buffer_->right_hand.score =
                        compute_score_with_reprojection(output_buffer_->right_hand.kpt3d, kpt2d.rhand_lcam_kpt,
                                                        kpt2d.rhand_rcam_kpt, lcam_model_, rcam_model_);
                }
                AISDK_LOG_TRACE("[LiftCalculator] right hand score is {}", output_buffer_->rhand_score);
            } else {
                output_buffer_->rhand_valid = false;
            }
        }
        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("LIFT_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("LIFT_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[LiftCalculator] No valid hand, truncated here");
        }
        AISDK_LOG_TRACE("[LiftCalculator] Process complete");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
