#include <iostream>
#include <memory>

#include "../common/NR_Transfer.h"
#include "../internal_structs/hand_state_struct_internal.h"
#include "../internal_structs/headpose_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"

namespace mediapipe {

std::vector<cv::Vec3f> transfer_from_cvL_to_world(NRTransform headpose, const std::vector<cv::Vec3f>& points_lcam_cv) {
    std::vector<cv::Vec3f> points_lcam_gl, points_head, points_world;

    aisdk::algorithm::TransferCVToGL(points_lcam_cv, points_lcam_gl);
    aisdk::algorithm::TransferLeftCamToHead(points_lcam_gl, points_head);
    aisdk::algorithm::TransferHeadToWorld(headpose, points_head, points_world);

    return points_world;
}

// A calculator compute 3d score metric for hand score and hand state.
// Definition:
// node {
//   calculator: "ConvertToWorldCalculator"
//   input_stream: "INPUT:kpt3d_constrained"
//   input_stream: "HEADPOSE:head_pose_checked"
//   output_stream: "OUTPUT:kpt3d_world"
// }

class ConvertToWorldCalculator : public CalculatorBase {
   private:
   public:
    static absl::Status GetContract(CalculatorContract* cc) {
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] GetContract start.");
        cc->Inputs().Tag("INPUT").Set<aisdk::algorithm::Kpt3dInternal>();
        cc->Inputs().Tag("HEADPOSE").Set<aisdk::algorithm::HeadPoseInternal>();
        cc->Inputs().Tag("STATE_INPUT").Set<aisdk::algorithm::HandStateInternal>();
        cc->Outputs().Tag("OUTPUT").Set<aisdk::algorithm::Kpt3dInternal>();
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] Open start.");
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] Process start.");

        if (!cc->Inputs().Tag("HEADPOSE").IsEmpty() && !cc->Inputs().Tag("INPUT").IsEmpty()) {
            const auto& input_data = cc->Inputs().Tag("INPUT").Get<aisdk::algorithm::Kpt3dInternal>();
            const auto& headpose_data = cc->Inputs().Tag("HEADPOSE").Get<aisdk::algorithm::HeadPoseInternal>();

            std::unique_ptr<aisdk::algorithm::Kpt3dInternal> output_buffer_ =
                absl::make_unique<aisdk::algorithm::Kpt3dInternal>();
            output_buffer_->clear();

            if (input_data.lhand_valid) {
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] Transform left hand 3d kpt form cv left to world!");
                output_buffer_->lhand = transfer_from_cvL_to_world(headpose_data.transform, input_data.lhand);
                output_buffer_->lhand_valid = true;
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] Transform left hand 3d kpt complete!");
            }
            if (input_data.rhand_valid) {
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] Transform right hand 3d kpt form cv left to world!");
                output_buffer_->rhand = transfer_from_cvL_to_world(headpose_data.transform, input_data.rhand);
                output_buffer_->rhand_valid = true;
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] Transform right hand 3d kpt complete!");
            }
            if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
                cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            } else {
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] No valid hand, truncated here.");
                cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            }

        } else {
            AISDK_LOG_TRACE(
                "[ConvertToWorldCalculator] Can not get HEADPOSE/Kpt3d input packet, input truncated here!");
        }

        AISDK_LOG_TRACE("[ConvertToWorldCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace mediapipe
