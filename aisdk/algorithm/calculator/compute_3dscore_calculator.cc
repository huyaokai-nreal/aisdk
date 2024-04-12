#include <iostream>
#include <memory>

#include "../common/metrics.h"
#include "../internal_structs/kpt2d_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "../internal_structs/score_3d_struct_internal.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"

// need camera model to do reproj
float compute3dscore(const std::vector<cv::Vec3f>& kpt3d, const std::vector<cv::Vec2f>& kpt2d_lcam,
                     const std::vector<cv::Vec2f>& kpt2d_rcam) {
    float score = 0.9;
    return score;
}

namespace mediapipe {

// A calculator compute 3d score metric for hand score and hand state.
// Definition:
// node {
//   name: "Compute3DScore"
//   calculator: "Compute3DScoreCalculator"
//   input_stream: "CAM_INFO_INPUT:cam_info"
//   input_stream: "LANDMARK_INPUT:kpt2d"
//   input_stream: "KPT3D_INPUT:kpt3d_blocked"
//   output_stream: "OUTPUT:hand_score"
// }

class Compute3DScoreCalculator : public CalculatorBase {
   private:
   public:
    static absl::Status GetContract(CalculatorContract* cc) {
        AISDK_LOG_TRACE("[Compute3DScoreCalculator] GetContract start");
        cc->Inputs().Tag("CAM_INFO_INPUT").Set<aisdk::algorithm::CamInfo>();
        cc->Inputs().Tag("LANDMARK_INPUT").Set<aisdk::algorithm::Kpt2dInternal>();
        cc->Inputs().Tag("KPT3D_INPUT").Set<aisdk::algorithm::Kpt3dInternal>();
        cc->Outputs().Tag("OUTPUT").Set<aisdk::algorithm::Score3dInternal>();
        AISDK_LOG_TRACE("[Compute3DScoreCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[Compute3DScoreCalculator] Open start");
        AISDK_LOG_TRACE("[Compute3DScoreCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(Compute3DScoreCalculator::Process);
#endif
        AISDK_LOG_TRACE("[Compute3DScoreCalculator] Process start");
        const auto& cam_info = cc->Inputs().Tag("CAM_INFO_INPUT").Get<aisdk::algorithm::CamInfo>();
        const auto& kpt2d_data = cc->Inputs().Tag("LANDMARK_INPUT").Get<aisdk::algorithm::Kpt2dInternal>();
        const auto& kpt3d_data = cc->Inputs().Tag("KPT3D_INPUT").Get<aisdk::algorithm::Kpt3dInternal>();
        std::unique_ptr<aisdk::algorithm::Score3dInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::Score3dInternal>();
        output_buffer_->clear();

        if (kpt3d_data.lhand_valid) {
            AISDK_LOG_TRACE("[Compute3DScoreCalculator] Compute lhand 3d score");
            output_buffer_->lhand_score =
                compute3dscore(kpt3d_data.lhand, kpt2d_data.lhand_lcam, kpt2d_data.lhand_rcam);
            AISDK_LOG_TRACE("[Compute3DScoreCalculator] Compute lhand 3d score complete");
        }
        if (kpt3d_data.rhand_valid) {
            AISDK_LOG_TRACE("[Compute3DScoreCalculator] Compute rhand 3d score");
            output_buffer_->rhand_score =
                compute3dscore(kpt3d_data.rhand, kpt2d_data.rhand_lcam, kpt2d_data.rhand_rcam);
            AISDK_LOG_TRACE("[Compute3DScoreCalculator] Compute rhand 3d score complete");
        }
        if (kpt3d_data.lhand_valid || kpt3d_data.rhand_valid) {
            cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[Compute3DScoreCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[Compute3DScoreCalculator] Process complete");
        return absl::OkStatus();
    }
};

}  // namespace mediapipe
