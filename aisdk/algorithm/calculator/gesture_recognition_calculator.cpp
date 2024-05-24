#include <memory>
#include <string>

#include "../func/gesture_recognition_v2.h"
#include "aisdk/algorithm/internal_structs/hand_gesture_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"

namespace aisdk::algorithm {

// A calculator generate hand gesture.
// Definition:
// node {
//   name: "GestureRecognition"
//   calculator: "GestureRecognitionCalculator"
//   input_stream: "GR_KPT_INPUT:kpt3d_post_constrained"
//   input_stream: "GR_KPT2D_INPUT:kpt2d_filter"
//   output_stream: "GR_OUTPUT:gesture"
// }

class GestureRecognitionCalculator : public xgraph::CalculatorBase {
   private:
    std::unique_ptr<GestureRecognitionV2> m_gesture_classifier_lhand;
    std::unique_ptr<GestureRecognitionV2> m_gesture_classifier_rhand;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] GetContract start.");
        cc->Inputs().Tag("GR_KPT_INPUT").Set<HandsData>();
        cc->Inputs().Tag("GR_KPT2D_INPUT").Set<Kpt2dInternal>();
        cc->Outputs().Tag("GR_OUTPUT").Set<HandGestureInternal>();
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Open start.");
        m_gesture_classifier_lhand = std::make_unique<GestureRecognitionV2>();
        m_gesture_classifier_rhand = std::make_unique<GestureRecognitionV2>();
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(GestureRecognitionCalculator::Process);
#endif
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Process start.");
        const auto& kpt3d_data = cc->Inputs().Tag("GR_KPT_INPUT").Get<HandsData>();
        const auto& kpt2d_data = cc->Inputs().Tag("GR_KPT2D_INPUT").Get<Kpt2dInternal>();

        std::unique_ptr<HandGestureInternal> output_buffer_ = absl::make_unique<HandGestureInternal>();
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Process start 2.");

        if (kpt3d_data.lhand_valid) {
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process left hand.");

            auto [gesture_res, raw_feat] = m_gesture_classifier_lhand->predict_with_keypoints3d(
                kpt3d_data.left_hand.kpt3d, kpt2d_data.lhand_lcam_kpt, true);
            output_buffer_->lhand_gesture = gesture_res;
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process left hand complete. {}",
                            output_buffer_->lhand_gesture);
        }
        if (kpt3d_data.rhand_valid) {
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process right hand.");
            auto [gesture_res, raw_feat] = m_gesture_classifier_rhand->predict_with_keypoints3d(
                kpt3d_data.right_hand.kpt3d, kpt2d_data.rhand_rcam_kpt, false);
            output_buffer_->rhand_gesture = gesture_res;
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process right hand complete. {}",
                            output_buffer_->rhand_gesture);
        }

        cc->Outputs().Tag("GR_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
