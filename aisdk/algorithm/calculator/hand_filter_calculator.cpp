#include "../common/NR_GlobalPredictorService.h"
#include "aisdk/algorithm/func/hand_filters.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"

namespace aisdk::algorithm {

// A calculator doing correction step of a global kalman filter.
// Definition:
// node {
//   name: "KalmanFilterCorrection"
//   calculator: "KalmanFilterCorrectionCalculator"
//   input_stream: "INPUT:kpt3d_standard"
//   input_stream: "STATE:hand_state"
//   output_stream: "OUTPUT:kpt3d_filtered"
// }

class HandFilterCalculator : public xgraph::CalculatorBase {
   private:
    Kpt3dInternal kpt3d_world_pre;
    double last_timestamp_;  // in seconds
    std::unique_ptr<HandFilters> m_post_filter;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[KalmanFilterCorrectionCalculator] GetContract start.");
        cc->Inputs().Tag("INPUT").Set<Kpt3dInternal>();
        cc->Outputs().Tag("OUTPUT").Set<Kpt3dInternal>();
        AISDK_LOG_TRACE("[KalmanFilterCorrectionCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[KalmanFilterCorrectionCalculator] Open start.");
        m_post_filter = std::make_unique<algorithm::HandFilters>();
        m_post_filter->init();
        AISDK_LOG_TRACE("[KalmanFilterCorrectionCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(KalmanFilterCorrectionCalculator::Process);
#endif
        AISDK_LOG_TRACE("[KalmanFilterCorrectionCalculator] Process start.");
        const auto& kpt3d_world = cc->Inputs().Tag("INPUT").Get<Kpt3dInternal>();
        const auto& timestamp = cc->InputTimestamp().Seconds();
        std::unique_ptr<Kpt3dInternal> output_buffer_ = absl::make_unique<Kpt3dInternal>();
        auto& predictor_lhand = GlobalPredictorService::getInstance().get_predictor_lhand();
        auto& predictor_rhand = GlobalPredictorService::getInstance().get_predictor_rhand();

        if (!kpt3d_world.lhand_valid) {
            predictor_lhand.stop_tracking();
            output_buffer_->lhand_valid = false;
            kpt3d_world_pre.lhand_valid = false;
        } else {
            output_buffer_->lhand_kpt = convert_to_23points(kpt3d_world.lhand_kpt);
            if (!predictor_lhand.get_tracking_status()) {
                predictor_lhand.start_tracking(timestamp, {output_buffer_->lhand_kpt[21], {0., 0., 0.}});
            } else {
                if (kpt3d_world_pre.lhand_valid) {
                    auto measure_v =
                        (output_buffer_->lhand_kpt[21] - kpt3d_world_pre.lhand_kpt[21]) / (timestamp - last_timestamp_);
                    output_buffer_->lhand_v = measure_v;
                    predictor_lhand.track_with_correct(timestamp, {output_buffer_->lhand_kpt[21], measure_v});
                }
            }
            kpt3d_world_pre.lhand_kpt = output_buffer_->lhand_kpt;
            kpt3d_world_pre.lhand_valid = true;
            m_post_filter->kpt_seq_3d_filter(0, output_buffer_->lhand_kpt);
            output_buffer_->lhand_valid = true;
        }

        if (!kpt3d_world.rhand_valid) {
            predictor_rhand.stop_tracking();
            output_buffer_->rhand_valid = false;
            kpt3d_world_pre.rhand_valid = false;
        } else {
            output_buffer_->rhand_kpt = convert_to_23points(kpt3d_world.rhand_kpt);
            if (!predictor_rhand.get_tracking_status()) {
                predictor_rhand.start_tracking(timestamp, {output_buffer_->rhand_kpt[21], {0., 0., 0.}});
            } else {
                if (kpt3d_world_pre.rhand_valid) {
                    auto measure_v =
                        (output_buffer_->rhand_kpt[21] - kpt3d_world_pre.rhand_kpt[21]) / (timestamp - last_timestamp_);
                    output_buffer_->rhand_v = measure_v;
                    predictor_rhand.track_with_correct(timestamp, {output_buffer_->rhand_kpt[21], measure_v});
                }
            }
            kpt3d_world_pre.rhand_kpt = output_buffer_->rhand_kpt;
            kpt3d_world_pre.rhand_valid = true;
            m_post_filter->kpt_seq_3d_filter(1, output_buffer_->rhand_kpt);
            output_buffer_->rhand_valid = true;
        }
        cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        last_timestamp_ = timestamp;
        AISDK_LOG_TRACE("[KalmanFilterCorrectionCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
