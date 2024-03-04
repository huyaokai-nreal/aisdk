#include <iostream>
#include <memory>

#include "../common/NR_GlobalPredictorService.h"
#include "../internal_structs/hand_state_struct_internal.h"
#include "../internal_structs/standard_kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"

namespace mediapipe {

// A calculator doing correction step of a global kalman filter.
// Definition:
// node {
//   name: "KalmanFilterCorrection"
//   calculator: "KalmanFilterCorrectionCalculator"
//   input_stream: "INPUT:kpt3d_standard"
//   input_stream: "STATE:hand_state"
//   input_stream: "TIMESTAMP:timestamp"
//   output_stream: "OUTPUT:kpt3d_filtered"
// }

class KalmanFilterCorrectionCalculator : public CalculatorBase {
   private:
    aisdk::algorithm::StandardKpt3dInternal kpt3d_world_pre;

   public:
    static absl::Status GetContract(CalculatorContract* cc) {
        AISDK_LOG_TRACE("[KalmanFilterCorrectionCalculator] GetContract start.");
        cc->Inputs().Tag("INPUT").Set<aisdk::algorithm::StandardKpt3dInternal>();
        cc->Inputs().Tag("STATE").Set<aisdk::algorithm::HandStateInternal>();
        cc->Inputs().Tag("TIMESTAMP").Set<uint64_t>();
        cc->Outputs().Tag("OUTPUT").Set<aisdk::algorithm::StandardKpt3dInternal>();
        AISDK_LOG_TRACE("[KalmanFilterCorrectionCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[KalmanFilterCorrectionCalculator] Open start.");
        AISDK_LOG_TRACE("[KalmanFilterCorrectionCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[KalmanFilterCorrectionCalculator] Process start.");
        const auto& kpt3d_world = cc->Inputs().Tag("INPUT").Get<aisdk::algorithm::StandardKpt3dInternal>();
        const auto& hand_state = cc->Inputs().Tag("STATE").Get<aisdk::algorithm::HandStateInternal>();
        const auto& timestamp = cc->Inputs().Tag("TIMESTAMP").Get<uint64_t>();
        std::unique_ptr<aisdk::algorithm::StandardKpt3dInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::StandardKpt3dInternal>();
        output_buffer_->clear();

        auto& predictor_lhand = aisdk::algorithm::GlobalPredictorService::getInstance().get_predictor_lhand();
        auto& predictor_rhand = aisdk::algorithm::GlobalPredictorService::getInstance().get_predictor_rhand();

        if (hand_state.lhand_valid == false) {
            predictor_lhand.stop_tracking();
            output_buffer_->lhand_valid = false;
            kpt3d_world_pre.lhand_valid = false;
        } else {
            if (predictor_lhand.get_tracking_status() == false) {
                predictor_lhand.start_tracking(timestamp, {kpt3d_world.lhand[21], {0., 0., 0.}});
            } else {
                if (kpt3d_world_pre.lhand_valid) {
                    predictor_lhand.track_with_correct(
                        timestamp, {kpt3d_world.lhand[21], kpt3d_world.lhand[21] - kpt3d_world_pre.lhand[21]});
                }
            }
            kpt3d_world_pre.lhand = kpt3d_world.lhand;
            kpt3d_world_pre.lhand_valid = true;
            output_buffer_->lhand = kpt3d_world.lhand;
            output_buffer_->lhand_valid = true;
        }

        if (hand_state.rhand_valid == false) {
            predictor_rhand.stop_tracking();
            output_buffer_->rhand_valid = false;
            kpt3d_world_pre.rhand_valid = false;
        } else {
            if (predictor_rhand.get_tracking_status() == false) {
                predictor_rhand.start_tracking(timestamp, {kpt3d_world.rhand[21], {0., 0., 0.}});
            } else {
                if (kpt3d_world_pre.rhand_valid) {
                    predictor_rhand.track_with_correct(
                        timestamp, {kpt3d_world.rhand[21], kpt3d_world.rhand[21] - kpt3d_world_pre.rhand[21]});
                }
            }
            kpt3d_world_pre.rhand = kpt3d_world.rhand;
            kpt3d_world_pre.rhand_valid = true;
            output_buffer_->rhand = kpt3d_world.rhand;
            output_buffer_->rhand_valid = true;
        }
        cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());

        AISDK_LOG_TRACE("[KalmanFilterCorrectionCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace mediapipe
