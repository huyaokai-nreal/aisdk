#include <memory>
#include <string>

#include "../common/NR_GlobalPredictorService.h"
#include "../func/gesture_recognition_v2.h"
#include "../internal_structs/hand_output_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
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
//   output_stream: "GR_OUTPUT:hand_result"
// }

class GestureRecognitionCalculator : public xgraph::CalculatorBase {
   private:
    std::unique_ptr<aisdk::algorithm::GestureRecognitionV2> m_gesture_classifier_lhand;
    std::unique_ptr<aisdk::algorithm::GestureRecognitionV2> m_gesture_classifier_rhand;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] GetContract start.");
        cc->Inputs().Tag("GR_KPT_INPUT").Set<Kpt3dInternal>();
        cc->Inputs().Tag("GR_KPT2D_INPUT").Set<aisdk::algorithm::Kpt2dInternal>();
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
        const auto& kpt3d_data = cc->Inputs().Tag("GR_KPT_INPUT").Get<Kpt3dInternal>();
        const auto& timestamp = cc->InputTimestamp().Seconds();
        const auto& kpt2d_data = cc->Inputs().Tag("GR_KPT2D_INPUT").Get<aisdk::algorithm::Kpt2dInternal>();

        std::unique_ptr<aisdk::algorithm::HandOutputInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::HandOutputInternal>();
        output_buffer_->clear();
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Process start 2.");

        if (kpt3d_data.lhand_valid) {
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process left hand.");

            std::string gesture_res;
            std::vector<Eigen::Vector3f> points3d_in(STANDARDIZE_TARGET_POINTS);
            for (int p = 0; p < STANDARDIZE_TARGET_POINTS; p++) {
                points3d_in[p] = {kpt3d_data.lhand[p][0], kpt3d_data.lhand[p][1], kpt3d_data.lhand[p][2]};
            }
            std::tie(gesture_res, std::ignore) =
                m_gesture_classifier_lhand->predict_with_keypoints3d(points3d_in, kpt2d_data.lhand_lcam, true);

            output_buffer_->lhand_valid = true;
            output_buffer_->lhand_score = kpt3d_data.lscore;
            output_buffer_->lhand_kpt = kpt3d_data.lhand;
            output_buffer_->lhand_v = kpt3d_data.lhand_v;
            output_buffer_->lhand_gesture = gesture_res;
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process left hand complete. {}",
                            output_buffer_->lhand_gesture);
        }
        if (kpt3d_data.rhand_valid) {
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process right hand.");
            std::string gesture_res;
            std::vector<Eigen::Vector3f> points3d_in(STANDARDIZE_TARGET_POINTS);
            for (int p = 0; p < STANDARDIZE_TARGET_POINTS; p++) {
                points3d_in[p] = {kpt3d_data.rhand[p][0], kpt3d_data.rhand[p][1], kpt3d_data.rhand[p][2]};
            }
            std::tie(gesture_res, std::ignore) =
                m_gesture_classifier_rhand->predict_with_keypoints3d(points3d_in, kpt2d_data.rhand_rcam, false);

            output_buffer_->rhand_valid = true;
            output_buffer_->rhand_score = kpt3d_data.rscore;
            output_buffer_->rhand_kpt = kpt3d_data.rhand;
            output_buffer_->rhand_v = kpt3d_data.rhand_v;
            output_buffer_->rhand_gesture = gesture_res;
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
