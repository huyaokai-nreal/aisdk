#include <memory>

#include "../common/NR_Transfer.h"
#include "../internal_structs/headpose_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/base/type.h"
#include "aisdk/xgraph/xgraph.h"

namespace aisdk::algorithm {

std::vector<Vec3f_t> transfer_from_cvL_to_world(NRTransform headpose, const std::vector<Vec3f_t>& points_lcam_cv) {
    std::vector<Vec3f_t> points_lcam_gl, points_head, points_world;

    TransferCVToGL(points_lcam_cv, points_lcam_gl);
    TransferLeftCamToHead(points_lcam_gl, points_head);
    TransferHeadToWorld(headpose, points_head, points_world);

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

class ConvertToWorldCalculator : public xgraph::CalculatorBase {
   private:
   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] GetContract start.");
        cc->Inputs().Tag("INPUT").Set<Kpt3dInternal>();
        cc->Inputs().Tag("HEADPOSE").Set<HeadPoseInternal>();
        cc->Outputs().Tag("OUTPUT").Set<Kpt3dInternal>();
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] Open start.");
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(ConvertToWorldCalculator::Process);
#endif
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] Process start.");

        if (!cc->Inputs().Tag("HEADPOSE").IsEmpty() && !cc->Inputs().Tag("INPUT").IsEmpty()) {
            const auto& input_data = cc->Inputs().Tag("INPUT").Get<Kpt3dInternal>();
            const auto& headpose_data = cc->Inputs().Tag("HEADPOSE").Get<HeadPoseInternal>();

            std::unique_ptr<Kpt3dInternal> output_buffer_ = absl::make_unique<Kpt3dInternal>();
            output_buffer_->clear();

            if (input_data.lhand_valid) {
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] Transform left hand 3d kpt form cv left to world!");
                output_buffer_->lhand_kpt = transfer_from_cvL_to_world(headpose_data.transform, input_data.lhand_kpt);
                output_buffer_->lhand_valid = true;
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] Transform left hand 3d kpt complete!");
            }
            if (input_data.rhand_valid) {
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] Transform right hand 3d kpt form cv left to world!");
                output_buffer_->rhand_kpt = transfer_from_cvL_to_world(headpose_data.transform, input_data.rhand_kpt);
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

}  // namespace aisdk::algorithm
