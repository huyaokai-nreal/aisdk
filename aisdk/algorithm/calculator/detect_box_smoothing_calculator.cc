#include <iostream>
#include <memory>

#include "../common/NR_Seq_Manager.h"
#include "../internal_structs/det_struct_internal.h"
#include "aisdk/base/log.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"

namespace mediapipe {

// A calculator generate smoothed detection bbox result.
// Definition:
// node {
//   calculator: "DetectBoxSmoothingCalculator"
//   input_stream: "BBOX_INPUT:detection_output"
//   output_stream: "BBOX_SMOOTHED_OUTPUT:detection_smoothed_output"
// }

class DetectBoxSmoothingCalculator : public CalculatorBase {
   private:
    std::shared_ptr<aisdk::algorithm::SeqManager> m_seq_lcam_lhand;
    std::shared_ptr<aisdk::algorithm::SeqManager> m_seq_lcam_rhand;
    std::shared_ptr<aisdk::algorithm::SeqManager> m_seq_rcam_lhand;
    std::shared_ptr<aisdk::algorithm::SeqManager> m_seq_rcam_rhand;

   public:
    static absl::Status GetContract(CalculatorContract* cc) {
        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] GetContract start");

        cc->Inputs().Tag("BBOX_INPUT").Set<aisdk::algorithm::DetOutputInternal>();
        cc->Outputs().Tag("BBOX_SMOOTHED_OUTPUT").Set<aisdk::algorithm::DetOutputInternal>();

        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Open start");

        m_seq_lcam_lhand = std::make_shared<aisdk::algorithm::SeqManager>();
        m_seq_lcam_rhand = std::make_shared<aisdk::algorithm::SeqManager>();
        m_seq_rcam_lhand = std::make_shared<aisdk::algorithm::SeqManager>();
        m_seq_rcam_rhand = std::make_shared<aisdk::algorithm::SeqManager>();

        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Process start");
        const auto& input_data = cc->Inputs().Tag("BBOX_INPUT").Get<aisdk::algorithm::DetOutputInternal>();

        std::unique_ptr<aisdk::algorithm::DetOutputInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::DetOutputInternal>();
        output_buffer_->clear();

        if (input_data.lhand_valid) {
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Do smoothing on left hand bboxes (lhand_lcam, lhand_rcam)");
            output_buffer_->lhand_valid = true;

            output_buffer_->images_lhand_rects.push_back(input_data.images_lhand_rects[0]);
            output_buffer_->images_lhand_rects.push_back(input_data.images_lhand_rects[1]);

            float p_score = 1.0f;
            m_seq_lcam_lhand->getFilterBoxData(output_buffer_->images_lhand_rects[0][0], p_score);
            m_seq_rcam_lhand->getFilterBoxData(output_buffer_->images_lhand_rects[1][0], p_score);

            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Smoothing left hand bboxes complete");
        }
        if (input_data.rhand_valid) {
            AISDK_LOG_TRACE(
                "[DetectBoxSmoothingCalculator] Do smoothing on right hand bboxes (rhand_lcam, rhand_rcam)");
            output_buffer_->rhand_valid = true;

            output_buffer_->images_rhand_rects.push_back(input_data.images_rhand_rects[0]);
            output_buffer_->images_rhand_rects.push_back(input_data.images_rhand_rects[1]);

            float p_score = 1.0f;
            m_seq_lcam_rhand->getFilterBoxData(output_buffer_->images_rhand_rects[0][0], p_score);
            m_seq_rcam_rhand->getFilterBoxData(output_buffer_->images_rhand_rects[1][0], p_score);

            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Smoothing right hand bboxes complete");
        }
        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("BBOX_SMOOTHED_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Process complete");
        return absl::OkStatus();
    }
};

}  // namespace mediapipe
