#include <memory>
#include <string>

#include "../common/NR_GlobalPredictorService.h"
#include "../func/gesture_recognition_v2.h"
#include "../internal_structs/hand_output_struct_internal.h"
#include "../internal_structs/hand_state_struct_internal.h"
#include "../internal_structs/score_3d_struct_internal.h"
#include "../internal_structs/standard_kpt3d_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"

#define STANDARDIZE_TARGET_POINTS 23

namespace aisdk::algorithm {

// A calculator generate hand output summarized result.
// Definition:
// node {
//   name: "GestureRecognition"
//   calculator: "GestureRecognitionCalculator"
//   input_stream: "GR_KPT_INPUT:kpt3d_post_constrained"
//   input_stream: "GR_KPT2D_INPUT:kpt2d_filter"
//   input_stream: "GR_SCORE_INPUT:hand_score"
//   input_stream: "GR_STATE_INPUT:hand_state"
//   output_stream: "GR_OUTPUT:hand_result"
// }

class GestureRecognitionCalculator : public xgraph::CalculatorBase {
   private:
    std::unique_ptr<aisdk::algorithm::GestureRecognitionV2> m_gesture_classifier_lhand;
    std::unique_ptr<aisdk::algorithm::GestureRecognitionV2> m_gesture_classifier_rhand;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] GetContract start.");
        cc->Inputs().Tag("GR_KPT_INPUT").Set<aisdk::algorithm::StandardKpt3dInternal>();
        cc->Inputs().Tag("GR_KPT2D_INPUT").Set<aisdk::algorithm::Kpt2dInternal>();
        cc->Inputs().Tag("GR_SCORE_INPUT").Set<aisdk::algorithm::Score3dInternal>();
        cc->Inputs().Tag("GR_STATE_INPUT").Set<aisdk::algorithm::HandStateInternal>();
        cc->Outputs().Tag("GR_OUTPUT").Set<aisdk::algorithm::HandOutputInternal>();
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Open start.");
        m_gesture_classifier_lhand = std::make_unique<aisdk::algorithm::GestureRecognitionV2>();
        m_gesture_classifier_rhand = std::make_unique<aisdk::algorithm::GestureRecognitionV2>();
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(GestureRecognitionCalculator::Process);
#endif
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Process start.");

        if (cc->Inputs().Tag("GR_KPT_INPUT").IsEmpty() || cc->Inputs().Tag("GR_SCORE_INPUT").IsEmpty() ||
            cc->Inputs().Tag("GR_STATE_INPUT").IsEmpty()) {
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] kpt3d/score3d/hand_state input empty! terminated here!");
            return absl::OkStatus();
        }

        const auto& kpt3d_data = cc->Inputs().Tag("GR_KPT_INPUT").Get<aisdk::algorithm::StandardKpt3dInternal>();
        const auto& score3d_data = cc->Inputs().Tag("GR_SCORE_INPUT").Get<aisdk::algorithm::Score3dInternal>();
        const auto& state_data = cc->Inputs().Tag("GR_STATE_INPUT").Get<aisdk::algorithm::HandStateInternal>();
        const auto& timestamp = cc->InputTimestamp().Seconds();
        const auto& kpt2d_data = cc->Inputs().Tag("GR_KPT2D_INPUT").Get<aisdk::algorithm::Kpt2dInternal>();

        std::unique_ptr<aisdk::algorithm::HandOutputInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::HandOutputInternal>();
        output_buffer_->clear();

        if (kpt3d_data.lhand_valid && state_data.lhand_valid) {
            // process left hand
            // process here
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process left hand.");

            std::string gesture_res;
            std::vector<Eigen::Vector3f> points3d_in(STANDARDIZE_TARGET_POINTS);
            for (int p = 0; p < STANDARDIZE_TARGET_POINTS; p++) {
                points3d_in[p] = {kpt3d_data.lhand[p][0], kpt3d_data.lhand[p][1], kpt3d_data.lhand[p][2]};
            }
            std::tie(gesture_res, std::ignore) =
                m_gesture_classifier_lhand->predict_with_keypoints3d(points3d_in, kpt2d_data.lhand_lcam, true);

            output_buffer_->lhand_valid = true;
            output_buffer_->lhand_score = score3d_data.lhand_score;
            output_buffer_->lhand_kpt = kpt3d_data.lhand;
            output_buffer_->lhand_gesture = gesture_res;
            // now rotation will be computed at prediction thread
            // output_buffer_->lhand_rot.resize(23);
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process left hand complete. {}",
                            output_buffer_->lhand_gesture);
        }
        if (kpt3d_data.rhand_valid && state_data.rhand_valid) {
            // process left hand
            // process here
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process right hand.");
            std::string gesture_res;
            std::vector<Eigen::Vector3f> points3d_in(STANDARDIZE_TARGET_POINTS);
            for (int p = 0; p < STANDARDIZE_TARGET_POINTS; p++) {
                points3d_in[p] = {kpt3d_data.rhand[p][0], kpt3d_data.rhand[p][1], kpt3d_data.rhand[p][2]};
            }
            std::tie(gesture_res, std::ignore) =
                m_gesture_classifier_rhand->predict_with_keypoints3d(points3d_in, kpt2d_data.rhand_rcam, false);

            output_buffer_->rhand_valid = true;
            output_buffer_->rhand_score = score3d_data.rhand_score;
            output_buffer_->rhand_kpt = kpt3d_data.rhand;
            output_buffer_->rhand_gesture = gesture_res;
            // output_buffer_->rhand_rot.resize(23);
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process right hand complete. {}",
                            output_buffer_->rhand_gesture);
        }

        auto& global_kpt3d = aisdk::algorithm::GlobalPredictorService::getInstance().get_kpt3d_world();
        global_kpt3d.lhand_valid = kpt3d_data.lhand_valid;
        if (kpt3d_data.lhand_valid) {
            global_kpt3d.lhand.resize(STANDARDIZE_TARGET_POINTS);
            for (int i = 0; i < STANDARDIZE_TARGET_POINTS; i++) {
                global_kpt3d.lhand[i] = {kpt3d_data.lhand[i][0], kpt3d_data.lhand[i][1], kpt3d_data.lhand[i][2]};
            }
        }

        global_kpt3d.rhand_valid = kpt3d_data.rhand_valid;
        if (kpt3d_data.rhand_valid) {
            global_kpt3d.rhand.resize(STANDARDIZE_TARGET_POINTS);
            for (int i = 0; i < STANDARDIZE_TARGET_POINTS; i++) {
                global_kpt3d.rhand[i] = {kpt3d_data.rhand[i][0], kpt3d_data.rhand[i][1], kpt3d_data.rhand[i][2]};
            }
        }
        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            output_buffer_->timestamp = timestamp;
            cc->Outputs().Tag("GR_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            output_buffer_->timestamp = timestamp;
            cc->Outputs().Tag("GR_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] No valid hand, truncated here.");
        }

        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
